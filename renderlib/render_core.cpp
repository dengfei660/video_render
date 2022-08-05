#include <string.h>
#include "render_core.h"
#include "Logger.h"
#include "wayland_plugin.h"
#include "wstclient_plugin.h"
#include "wayland_videoformat.h"
#include "videotunnel_plugin.h"
#include "drm_plugin.h"
#include "Times.h"
#include "config.h"

#define TAG "rlib:render_core"

#define TIME_NANO_SEC (1000000000)
#define DEFAULT_INT_VALUE (-1)

RenderCore::RenderCore(int renderlibId, int logCategory)
    : mRenderlibId(renderlibId),
    mLogCategory(logCategory),
    mPaused(false),
    mFlushing(false),
    mMediaSynInstID(-1),
    mVideoFormat(VIDEO_FORMAT_UNKNOWN),
    mRenderMutex("renderMutex"),
    mLimitMutex("limitSendMutex"),
    mBufferMgrMutex("bufferMutex")
{
    mCallback = NULL;
    mMediaSync= NULL;
    mPlugin = NULL;
    mWinSizeChanged = false;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mFrameChanged = false;
    mDemuxId = 0;
    mPcrId = 0x1fff;
    mSyncmode = MEDIA_SYNC_MODE_MAX;
    mMediaSyncBind = false;
    mLastInputPTS = -1;
    mLastDisplayPTS = -1;
    mLastDisplayRealtime = 0;
    mVideoFPS = 0;
    mVideoFPS_N = 0;
    mVideoFPS_D = 0;
    mFPSIntervalMs = 0;
    mFPSDetectCnt = 0;
    mFPSDetectAcc = 0;
    mDropFrameCnt = 0;
    mBufferId = 1;
    mLastDisplaySystemtime = 0;
    mWaitAnchorTimeUs = 0;
    mReleaseFrameCnt = 0;
    mDisplayedFrameCnt = 0;
    mInFrameCnt = 0;
    mMediaSyncTunnelmode.value = DEFAULT_INT_VALUE;
    mMediaSyncTunnelmode.changed = false;
    mMediasyncHasAudio.value = DEFAULT_INT_VALUE;
    mMediasyncHasAudio.changed = false;
    mMediasyncSourceType.value = DEFAULT_INT_VALUE;
    mMediasyncSourceType.changed = false;
    mMediasyncVideoWorkMode.value = DEFAULT_INT_VALUE;
    mMediasyncVideoWorkMode.changed = false;
    mMediasyncVideoLatency.value = DEFAULT_INT_VALUE;
    mMediasyncVideoLatency.changed = false;
    mMediasyncStartThreshold.value = DEFAULT_INT_VALUE;
    mMediasyncStartThreshold.changed = false;
    mMediasyncPlayerInstanceId.value = DEFAULT_INT_VALUE;
    mMediasyncPlayerInstanceId.changed = false;
    mIsLimitDisplayFrame = true;
    mMediaSyncInstanceIDSet = false;
    mMediaSyncAnchor = false;
    mQueue = new Tls::Queue();
    //limit display frame,invalid when value is 0,other > 0 is enable
    char *env = getenv("VIDEO_RENDER_LIMIT_SEND_FRAME");
    if (env) {
        int limit = atoi(env);
        if (limit == 0) {
            mIsLimitDisplayFrame = false;
            INFO(mLogCategory,"No limit send frame");
        }
    }

    mAllRenderBufferMap.clear();
}
RenderCore::~RenderCore()
{
    TRACE2(mLogCategory,"desconstruct");
}

static PluginCallback plugincallback = {
    RenderCore::pluginMsgCallback,
    RenderCore::pluginBufferReleaseCallback,
    RenderCore::pluginBufferDisplayedCallback,
    RenderCore::pluginBufferDropedCallback
};

int RenderCore::init(char *name)
{
    DEBUG(mLogCategory,"name:%s",name);
    char *compositor = name;
    mCompositorName = name;
    char *val = getenv("VIDEO_RENDER_COMPOSITOR");
    if (val) {
        INFO(mLogCategory,"VIDEO_RENDER_COMPOSITOR=%s",val);
        if (!strcmp(val, "weston") || !strcmp(val, "westeros") ||
            !strcmp(val, "videotunnel") || !strcmp(val, "drmmeson")) {
            compositor = val;
        }
    }
#ifdef SUPPORT_WAYLAND
    if (!strcmp(compositor, "wayland") || !strcmp(compositor, "weston")) {
        mPlugin = new WaylandPlugin(mLogCategory);
    }

    if (!strcmp(compositor, "westeros")) {
        mPlugin = new WstClientPlugin(mLogCategory);
    }
#endif

    if (!strcmp(compositor, "videotunnel")) {
        mPlugin = new VideoTunnelPlugin(mLogCategory);
    }

    if (!strcmp(compositor, "drmmeson")) {
        mPlugin = new DrmPlugin(mLogCategory);
    }

    INFO(mLogCategory,"compositor:%s",compositor);
    if (mPlugin) {
        mPlugin->init();
        mPlugin->setUserData(this, &plugincallback);
    }

    if (!mMediaSync) {
        mMediaSync = MediaSync_create();
        INFO(mLogCategory,"New MediaSync %p",mMediaSync);
    }

    return NO_ERROR;
}

int RenderCore::release()
{
    DEBUG(mLogCategory,"release");
    if (isRunning()) {
        DEBUG(mLogCategory,"try stop render frame thread");
        requestExitAndWait();
    }

    if (mPlugin) {
        mPlugin->release();
        delete mPlugin;
        mPlugin = NULL;
    }

    for (auto item = mAllRenderBufferMap.begin(); item != mAllRenderBufferMap.end(); ) {
        RenderBuffer *renderbuf = (RenderBuffer*)item->second;
        mAllRenderBufferMap.erase(item++);
        releaseRenderBufferWrap(renderbuf);
    }

    if (mCallback) {
        free(mCallback);
        mCallback = NULL;
    }

    if (mMediaSync) {
        MediaSync_destroy(mMediaSync);
        mMediaSync = NULL;
        mMediaSyncBind = false;
    }

    if (mQueue) {
        mQueue->flush();
        delete mQueue;
        mQueue = NULL;
    }

    DEBUG(mLogCategory,"release end");

    return NO_ERROR;
}

void RenderCore::setCallback(RenderCallback *callback)
{
    if (!callback) {
        ERROR(mLogCategory,"Error param is null");
        return;
    }
    if (!mCallback) {
        mCallback = (RenderCallback *)calloc(1,sizeof(RenderCallback));
    }
    if (!mCallback) {
        ERROR(mLogCategory,"Error no memery");
        return;
    }
    TRACE1(mLogCategory,"callback:%p,doMsgSend:%p,doGetValue:%p",callback,callback->doMsgSend,callback->doGetValue);
    mCallback->doMsgSend = callback->doMsgSend;
    mCallback->doGetValue = callback->doGetValue;
}

void RenderCore::setUserData(void *data)
{
    TRACE1(mLogCategory,"data:%p",data);
    mUserData = data;
}

int RenderCore::connect()
{
    int ret;
    DEBUG(mLogCategory,"Connect");

    mDropFrameCnt = 0;
    mReleaseFrameCnt = 0;
    mDisplayedFrameCnt = 0;
    mInFrameCnt = 0;

    if (!mPlugin) {
        ERROR(mLogCategory, "please set compositor name first");
        return ERROR_NO_INIT;
    }

    int pluginState = mPlugin->getState();
    if ((pluginState & PLUGIN_STATE_DISPLAY_OPENED) && (pluginState & PLUGIN_STATE_WINDOW_OPENED)) {
        WARNING(mLogCategory,"Render had connected");
        return NO_ERROR;
    }

    if (mQueue) {
        mQueue->flush();
    }

    ret = mPlugin->openDisplay();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open display");
        return -1;
    }
    ret = mPlugin->openWindow();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open window");
        return -1;
    }

    DEBUG(mLogCategory,"Connect,end");
    return NO_ERROR;
}

int RenderCore::disconnect()
{
    DEBUG(mLogCategory,"Disconnect");
    //sleep to wait last frame displayed
    //usleep(30000); //is needed?

    if (isRunning()) {
        DEBUG(mLogCategory,"stop render frame thread");
        requestExitAndWait();
    }

    if (mQueue) {
        mQueue->flushAndCallback(this, RenderCore::queueFlushCallback);
    }

    if (mPlugin && mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
        TRACE1(mLogCategory,"try close window");
        mPlugin->closeWindow();
    }
    if (mPlugin && mPlugin->getState() & PLUGIN_STATE_DISPLAY_OPENED) {
        TRACE1(mLogCategory,"try close display");
        mPlugin->closeDisplay();
    }

    DEBUG(mLogCategory,"Disconnect,end");
    return NO_ERROR;
}

int RenderCore::displayFrame(RenderBuffer *buffer)
{
    Tls::Mutex::Autolock _l(mRenderMutex);
    //if display thread is not running ,start it
    if (!isRunning()) {
        DEBUG(mLogCategory,"to run displaythread");
        if (!mPlugin) {
            ERROR(mLogCategory,"please set compositor name first");
            if (mCallback) {
                TRACE1(mLogCategory,"release buffer %p",buffer);
                mCallback->doMsgSend(mUserData, MSG_DROPED_BUFFER, buffer);
                mCallback->doMsgSend(mUserData, MSG_RELEASE_BUFFER, buffer);
            }
            return ERROR_NO_INIT;
        }
        //run display thread
        run("displaythread");
    }

    mInFrameCnt += 1;

    TRACE1(mLogCategory,"+++++buffer:%p,ptsUs:%lld,ptsdiff:%d ms",buffer,buffer->pts/1000,(buffer->pts/1000000-mLastInputPTS/1000000));

    //if pts is -1,we need calculate it
    if (buffer->pts == -1) {
        if (mVideoFPS > 0) {
            buffer->pts = mLastInputPTS + TIME_NANO_SEC/mVideoFPS;
            TRACE2(mLogCategory,"correct pts:%lld",buffer->pts);
        }
    }

    //detect input frame and last input frame pts,if equal, release this frame
    if (mLastInputPTS == buffer->pts) {
        WARNING(mLogCategory,"frame pts equal last frame pts,release this frame:%p, queue size:%d, inFrameCnt:%d",buffer,mQueue->getCnt(),mInFrameCnt);
        if (mCallback) {
            TRACE2(mLogCategory,"drop and release buffer %p",buffer);
            pluginBufferDropedCallback(this, buffer);
            pluginBufferReleaseCallback(this, buffer);
        }
        return NO_ERROR;
    }

    mQueue->push(buffer);
    TRACE1(mLogCategory,"queue size:%d, inFrameCnt:%d",mQueue->getCnt(),mInFrameCnt);

    //fps detect
    if (mVideoFPS > 0) {
        mFPSIntervalMs = mVideoFPS;
    } else {
        if (mFPSDetectCnt <= 100) {
            if (mLastInputPTS > 0) {
                int internal = buffer->pts/1000000 - mLastInputPTS/1000000;
                mFPSDetectAcc += internal;
                mFPSDetectCnt++;
                mFPSIntervalMs = mFPSDetectAcc/mFPSDetectCnt;
                TRACE2(mLogCategory,"fps internal:%d ms,cnt:%d,mFPS_internal:%d",internal,mFPSDetectCnt,mFPSIntervalMs);
            }
        }
    }

    mLastInputPTS = buffer->pts;

    return NO_ERROR;
}


int RenderCore::setProp(int property, void *prop)
{
    if (!prop) {
        ERROR(mLogCategory,"Params is NULL");
        return ERROR_PARAM_NULL;
    }

    switch (property) {
        case KEY_WINDOW_SIZE: {
            RenderWindowSize *win = (RenderWindowSize *) (prop);
            mWinSize.x = win->x;
            mWinSize.y = win->y;
            mWinSize.w = win->w;
            mWinSize.h = win->h;
            DEBUG(mLogCategory,"set window size:x:%d,y:%d,w:%d,h:%d",mWinSize.x,mWinSize.y,mWinSize.w,mWinSize.h);
            //if window has opened ,set immediately
            if (mPlugin && mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
                PluginRect rect;
                rect.x = mWinSize.x;
                rect.y = mWinSize.y;
                rect.w = mWinSize.w;
                rect.h = mWinSize.h;
                mPlugin->set(PLUGIN_KEY_WINDOW_SIZE, &rect);
            } else {
                mWinSizeChanged = true;
            }
        } break;
        case KEY_FRAME_SIZE:{
            RenderFrameSize *frame = (RenderFrameSize *) (prop);
            mFrameWidth = frame->frameWidth;
            mFrameHeight = frame->frameHeight;
            DEBUG(mLogCategory,"set frame size:w:%d,h:%d",mFrameWidth,mFrameHeight);
            //if window has opened ,set immediately
            if (mPlugin && mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
                PluginFrameSize size;
                size.w = mFrameWidth;
                size.h = mFrameHeight;
                mPlugin->set(PLUGIN_KEY_FRAME_SIZE, &size);
            } else {
                mFrameChanged = true;
            }
        } break;
        case KEY_MEDIASYNC_INSTANCE_ID: {
            mMediaSynInstID = *(int *)(prop);
            mMediaSyncInstanceIDSet = true;
            DEBUG(mLogCategory,"set mediasync inst id:%d",mMediaSynInstID);
        } break;
        case KEY_MEDIASYNC_PCR_PID: {
            mPcrId = *(int *)prop;
            DEBUG(mLogCategory,"set pcr pid:%d",mPcrId);
        } break;
        case KEY_MEDIASYNC_DEMUX_ID: {
            mDemuxId = *(int *)prop;
            DEBUG(mLogCategory,"set demux id:%d",mDemuxId);
        } break;
        case KEY_MEDIASYNC_SYNC_MODE: {
            mSyncmode = *(int *)prop;
            DEBUG(mLogCategory,"set mediasync sync mode:%d (0:vmaster,1:amaster,2:pcrmaster)",mSyncmode);
        } break;
        case KEY_VIDEO_FORMAT: {
            mVideoFormat = (RenderVideoFormat)*(int *)prop;
            DEBUG(mLogCategory,"set video format:%d",mVideoFormat);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_VIDEO_FORMAT, (void *)&mVideoFormat);
            }
        } break;
        case KEY_VIDEO_FPS: {
            int64_t fps = *(int64_t *)prop;
            mVideoFPS_N = int ((fps >> 32) & 0xFFFFFFFF);
            mVideoFPS_D = int (fps & 0xFFFFFFFF);
            mVideoFPS = (int) mVideoFPS_N/mVideoFPS_D;
            DEBUG(mLogCategory,"set video fps_n:%d(%x),fps_d:%d(%x),fps:%d",mVideoFPS_N,mVideoFPS_N,mVideoFPS_D,mVideoFPS_D,mVideoFPS);
        } break;
        case KEY_VIDEO_PIP: {
            int pip = *(int *)(prop);
            DEBUG(mLogCategory,"set video pip :%d",pip);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_VIDEO_PIP, (void *)&pip);
            }
        };
        case KEY_KEEP_LAST_FRAME: {
            DEBUG(mLogCategory,"set keep last frame :%d",*(int *)prop);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_KEEP_LAST_FRAME, (void *)prop);
            }
        } break;
        case KEY_HIDE_VIDEO: {
            DEBUG(mLogCategory,"set hide video :%d",*(int *)prop);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_HIDE_VIDEO, (void *)prop);
            }
        } break;
        case KEY_MEDIASYNC_TUNNEL_MODE: {
            int mode = *(int *)(prop);
            if (mode > 0) {
                mMediaSyncTunnelmode.value = 1;
                mMediaSyncTunnelmode.changed = true;
            } else {
                mMediaSyncTunnelmode.value = 0;
                mMediaSyncTunnelmode.changed = true;
            }
            DEBUG(mLogCategory,"set mediasync tunnel mode :%d",mMediaSyncTunnelmode.value);
        } break;
        case KEY_MEDIASYNC_HAS_AUDIO: {
            mMediasyncHasAudio.value = *(int *)prop;
            mMediasyncHasAudio.changed = true;
            DEBUG(mLogCategory,"set mediasync has audio:%d",mMediasyncHasAudio.value);
            if (mMediaSync && mMediaSyncBind) {
                DEBUG(mLogCategory,"do set mediasync has audio:%d",mMediasyncHasAudio.value);
                mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_HASAUDIO, (void* )&mMediasyncHasAudio.value);
            }
        } break;
        case KEY_MEDIASYNC_SOURCETYPE: {
            mMediasyncSourceType.value = *(int *)prop;
            mMediasyncSourceType.changed = true;
            DEBUG(mLogCategory,"set mediasync source type:%d",mMediasyncSourceType.value);
            if (mMediaSync && mMediaSyncBind) {
                DEBUG(mLogCategory,"do set mediasync source type:%d",mMediasyncSourceType.value);
                mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_SOURCETYPE, (void* )&mMediasyncSourceType.value);
            }
        } break;
        case KEY_MEDIASYNC_VIDEOWORKMODE: {
            int workmode = *(int *)prop;
            DEBUG(mLogCategory,"set mediasync video work mode from %d to %d (0:normal,1:cache:2:decode)",mMediasyncVideoWorkMode.value,workmode);
            mMediasyncVideoWorkMode.value = workmode;
            mMediasyncVideoWorkMode.changed = true;
            if (mMediaSync && mMediaSyncBind) {
                DEBUG(mLogCategory,"do set mediasync video work mode %d 0:normal,1:cache:2:decode",mMediasyncVideoWorkMode.value);
                mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_VIDEOWORKMODE, (void* )&mMediasyncVideoWorkMode.value);
                if (mMediasyncVideoWorkMode.value == VIDEO_WORK_MODE_CACHING_ONLY &&
                        mMediaSyncTunnelmode.value == 1) {
                    Tls::Mutex::Autolock _l(mRenderMutex);
                    DEBUG(mLogCategory,"do flush queue buffers");
                    mQueue->flushAndCallback(this, RenderCore::queueFlushCallback);
                }
            }
        } break;
        case KEY_MEDIASYNC_VIDEOLATENCY: {
            mMediasyncVideoLatency.value = *(int *)prop;
            mMediasyncVideoLatency.changed = true;
            DEBUG(mLogCategory,"set mediasync video latency:%d",mMediasyncVideoLatency.value);
        } break;
        case KEY_MEDIASYNC_STARTTHRESHOLD: {
            mMediasyncStartThreshold.value = *(int *)prop;
            mMediasyncStartThreshold.changed = true;
            DEBUG(mLogCategory,"set mediasync strart threshold:%d",mMediasyncStartThreshold.value);
        } break;
        case KEY_MEDIASYNC_PLAYER_INSTANCE_ID: { //must set before mediasync bind
            mMediasyncPlayerInstanceId.value = *(int *)prop;
            mMediasyncPlayerInstanceId.changed = true;
            DEBUG(mLogCategory,"set mediasync player instance id:%d",mMediasyncPlayerInstanceId.value);
        } break;
        case KEY_VIDEOTUNNEL_ID: {
            int videotunnelId = *(int *)(prop);
            DEBUG(mLogCategory,"set videotunnel id :%d",videotunnelId);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_VIDEOTUNNEL_ID, (void *)&videotunnelId);
            }
        } break;
        case KEY_MEDIASYNC_PLAYBACK_RATE: {
            float rateValue = *(float *)(prop);
            if (rateValue >= 0.0f) {
                if (mMediaSync && mMediaSyncBind) {
                    MediaSync_setPlaybackRate(mMediaSync, rateValue);
                }
            }
        } break;
        case KEY_FORCE_ASPECT_RATIO: {
            int forceAspectRatio = *(int *)(prop);
            DEBUG(mLogCategory,"set force aspect ratio:%d",forceAspectRatio);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_FORCE_ASPECT_RATIO, prop);
            }
        } break;
        default:
            break;
    }
    return NO_ERROR;
}


int RenderCore::getProp(int property, void *prop)
{
    if (!prop) {
        ERROR(mLogCategory,"Params is NULL");
        return ERROR_PARAM_NULL;
    }

    switch (property) {
        case KEY_WINDOW_SIZE: {
            RenderWindowSize *win = (RenderWindowSize *) prop;
            win->x = mWinSize.x;
            win->y = mWinSize.y;
            win->w = mWinSize.w;
            win->h = mWinSize.h;
            TRACE1(mLogCategory,"get prop window size:x:%,y:%y,w:%d,h:%d",win->x,win->y,win->w,win->h);
        } break;
        case KEY_FRAME_SIZE: {
            RenderFrameSize *frame = (RenderFrameSize *) prop;
            frame->frameWidth = mFrameWidth;
            frame->frameHeight = mFrameHeight;
            TRACE1(mLogCategory,"get prop frame size:w:%d,h:%d",mFrameWidth,mFrameHeight);
        } break;
        case KEY_KEEP_LAST_FRAME: {
            if (mPlugin) {
                mPlugin->get(PLUGIN_KEY_KEEP_LAST_FRAME, prop);
            }
        } break;
        case KEY_HIDE_VIDEO: {
            if (mPlugin) {
                mPlugin->get(PLUGIN_KEY_HIDE_VIDEO, prop);
            }
        } break;
        case KEY_MEDIASYNC_INSTANCE_ID: {
            if (!mMediaSync) {
                mMediaSync = MediaSync_create();
                INFO(mLogCategory,"New MediaSync");
            }
            if (mMediaSync && mMediaSynInstID < 0) {
                MediaSync_allocInstance(mMediaSync, mDemuxId,
                                    mPcrId,
                                    &mMediaSynInstID);
                INFO(mLogCategory,"alloc mediasync instance id:%d",mMediaSynInstID);
            }
            mMediaSyncInstanceIDSet = true;
            *(int *)prop = mMediaSynInstID;
            TRACE1(mLogCategory,"get prop mediasync inst id:%d",*(int *)prop);
            //must set before mediasync bind
            if (mMediaSync && mMediaSynInstID >= 0 && mMediaSyncBind == false && mMediasyncPlayerInstanceId.changed) {
                DEBUG(mLogCategory,"do set mediasync player instance id:%d",mMediasyncPlayerInstanceId.value);
                mediasync_result ret = MediaSync_setPlayerInsNumber(mMediaSync, mMediasyncPlayerInstanceId.value);
                if (ret != AM_MEDIASYNC_OK) {
                    ERROR(mLogCategory, "set mediasync player instance id fail");
                }
            }
            //must set before mediasync bind
            if (mMediaSync && mMediaSynInstID >= 0 && mMediaSyncBind == false && mMediaSyncTunnelmode.changed) {
                bool tunnelMode = mMediaSyncTunnelmode.value > 0? true:false;
                DEBUG(mLogCategory,"do set mediasync tunnel mode:%d",tunnelMode);
                mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_ISOMXTUNNELMODE, (void* )&tunnelMode);
            }
            if (mMediaSync && mMediaSynInstID >= 0 && mMediaSyncBind == false) {
                DEBUG(mLogCategory,"bind mediasync instance id:%d",mMediaSynInstID);
                MediaSync_bindStaticInstance(mMediaSync, mMediaSynInstID, MEDIA_VIDEO);
                mMediaSyncBind = true;
            }
            if (mMediaSync && mMediaSyncBind && mMediasyncVideoWorkMode.changed) {
                DEBUG(mLogCategory,"do set mediasync video work mode:%d 0:normal,1:cache:2:decode",mMediasyncVideoWorkMode.value);
                mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_VIDEOWORKMODE, (void* )&mMediasyncVideoWorkMode.value);
                if (mMediasyncVideoWorkMode.value == VIDEO_WORK_MODE_CACHING_ONLY &&
                        mMediaSyncTunnelmode.value == 1) {
                    Tls::Mutex::Autolock _l(mRenderMutex);
                    DEBUG(mLogCategory,"do flush queue buffers");
                    mQueue->flushAndCallback(this, RenderCore::queueFlushCallback);
                }
            }
        } break;
        case KEY_MEDIASYNC_PCR_PID: {
            *(int *)prop = mPcrId;
            TRACE1(mLogCategory,"get prop pcr pid:%d",*(int *)prop);
        } break;
        case KEY_MEDIASYNC_DEMUX_ID: {
            *(int *)prop = mDemuxId;
            TRACE1(mLogCategory,"get prop demux id:%d",*(int *)prop);
        } break;
        case KEY_MEDIASYNC_SYNC_MODE: {
            *(int *)prop = mSyncmode;
            TRACE1(mLogCategory,"get mediasync sync mode:%d",*(int *)prop);
        } break;
        case KEY_FRAME_DROPPED: {
            *(int *)prop = mDropFrameCnt;
        } break;
        case KEY_MEDIASYNC_HAS_AUDIO: {
            *(int *)prop = mMediasyncHasAudio.value;
        } break;
        case KEY_MEDIASYNC_TUNNEL_MODE: {
            *(int *)prop = mMediaSyncTunnelmode.value;
        } break;
        case KEY_MEDIASYNC_SOURCETYPE: {
            *(int *)prop = mMediasyncSourceType.value;
        } break;
        case KEY_MEDIASYNC_VIDEOWORKMODE: {
            *(int *)prop = mMediasyncVideoWorkMode.value;
        } break;
        case KEY_MEDIASYNC_VIDEOLATENCY: {
            *(int *)prop = mMediasyncVideoLatency.value;
        } break;
        case KEY_MEDIASYNC_STARTTHRESHOLD: {
            *(int *)prop = mMediasyncStartThreshold.value;
        } break;
        case KEY_MEDIASYNC_PLAYBACK_RATE: {
            float rate;
            if (mMediaSync) {
                MediaSync_getPlaybackRate(mMediaSync, &rate);
            }
            *(float *)prop = rate;
        } break;
        default:
            break;
    }
    return NO_ERROR;
}


int RenderCore::flush()
{
    DEBUG(mLogCategory,"flush start");
    Tls::Mutex::Autolock _l(mRenderMutex);
    mFlushing = true;
    mQueue->flushAndCallback(this, RenderCore::queueFlushCallback);
    mMediaSyncAnchor = false;
    //flush plugin
    if (mPlugin) {
        mPlugin->flush();
    }

    if (mMediaSync && mMediaSyncBind) {
        MediaSync_reset(mMediaSync);
    }

    mFlushing = false;
    mWaitAnchorTimeUs = 0;
    DEBUG(mLogCategory,"flush end");
    return NO_ERROR;
}

int RenderCore::pause()
{
    DEBUG(mLogCategory,"Pause");
    if (mPaused) {
        WARNING(mLogCategory, "had paused");
        return NO_ERROR;
    }

    mPaused = true;
    if (mMediaSync && mMediaSyncBind) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, true);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR(mLogCategory,"Error set mediasync pause ");
        }
    }

    if (mPlugin) {
        mPlugin->pause();
    }

    return NO_ERROR;
}

int RenderCore::resume()
{
    DEBUG(mLogCategory,"Resume");
    if (mPaused == false) {
        WARNING(mLogCategory, "had resumed");
        return NO_ERROR;
    }

    mPaused = false;
    if (mMediaSync && mMediaSyncBind) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, false);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR(mLogCategory,"Error set mediasync resume");
        }
    }
    if (mPlugin) {
        mPlugin->resume();
    }
    return NO_ERROR;
}

int RenderCore::accquireDmaBuffer(RenderDmaBuffer *dmabuf)
{
    int planeCount;
    int width,height;

    DEBUG(mLogCategory,"accquireDmaBuffer");
    if (!dmabuf) {
        FATAL(mLogCategory,"Error param dmabuff is null");
        return ERROR_UNEXPECTED_NULL;
    }
    if (!mPlugin) {
        ERROR(mLogCategory,"please set compositor name first");
        return ERROR_NO_INIT;
    }

    planeCount = dmabuf->planeCnt;
    width = dmabuf->width;
    height = dmabuf->height;

    for (int i = 0; i < planeCount; i++) {
        dmabuf->fd[i] = mPlugin->acquireDmaBuffer(width,height);
        dmabuf->stride[i] = width;
        dmabuf->offset[i] = 0;
        dmabuf->size[i] = width * height;
    }
    dmabuf->planeCnt = planeCount;
    return NO_ERROR;
}

void RenderCore::releaseDmaBuffer(RenderDmaBuffer *dmabuf)
{
    DEBUG(mLogCategory,"releaseDmaBuffer");
    if (!mPlugin) {
        ERROR(mLogCategory,"please set compositor name first");
        return;
    }
    if (dmabuf) {
        for (int i = 0; i < dmabuf->planeCnt; i++) {
            mPlugin->releaseDmaBuffer(dmabuf->fd[i]);
        }
    }
}

int RenderCore::getFirstAudioPts(int64_t *pts)
{
    mediasync_result ret;
    mediasync_frameinfo frameInfo;
    if (mMediaSync && mMediaSyncBind) {
        ret = MediaSync_getFirstAudioFrameInfo(mMediaSync, &frameInfo);
        if (ret != AM_MEDIASYNC_OK) {
            *pts = -1;
            return -1;
        } else {
            *pts = frameInfo.framePts;
        }
    }
    return 0;
}

int RenderCore::getCurrentAudioPts(int64_t *pts)
{
    mediasync_result ret;
    mediasync_frameinfo frameInfo;
    if (mMediaSync && mMediaSyncBind) {
        ret = MediaSync_getCurAudioFrameInfo(mMediaSync, &frameInfo);
        if (ret != AM_MEDIASYNC_OK) {
            *pts = -1;
            return -1;
        } else {
            *pts = frameInfo.framePts;
        }
    }
    return 0;
}

int RenderCore::getMediaTimeByType(int mediaTimeType, int tunit, int64_t* mediaTime)
{
    mediasync_result ret;

    if (mMediaSync && mMediaSyncBind) {
        ret = MediaSync_GetMediaTimeByType(mMediaSync, (media_time_type)mediaTimeType, (mediasync_time_unit)tunit, mediaTime);
        if (ret != AM_MEDIASYNC_OK) {
            return -1;
        }
    }
    return 0;
}

int RenderCore::getPlaybackRate(float *scale)
{
    mediasync_result ret;

    if (mMediaSync && mMediaSyncBind) {
        ret = MediaSync_getPlaybackRate(mMediaSync, scale);
        if (ret != AM_MEDIASYNC_OK) {
            return -1;
        }
    }
    return 0;
}

int RenderCore::setPlaybackRate(float scale)
{
    mediasync_result ret;

    if (mMediaSync && mMediaSyncBind) {
        ret = MediaSync_setPlaybackRate(mMediaSync, scale);
        if (ret != AM_MEDIASYNC_OK) {
            return -1;
        }
    }
    return 0;
}

int RenderCore::queueDemuxPts(int64_t ptsUs, uint32_t size)
{
    mediasync_result ret;
    TRACE3(mLogCategory,"ptsUs:%lld,size:%u",ptsUs,size);
    if (mMediaSync && mMediaSynInstID >= 0) {
        if (mMediaSyncTunnelmode.value == 0) {
            ret = MediaSync_queueVideoFrame((void* )mMediaSync, ptsUs, size, 0 /*duration*/, MEDIASYNC_UNIT_US);
            if (ret != AM_MEDIASYNC_OK) {
                return -1;
            }
        }
    } else {
        WARNING(mLogCategory,"Mediasync is not create or alloc id,hand:%p,id:%d",mMediaSync,mMediaSynInstID);
        return -1;
    }
    return 0;
}

void RenderCore::pluginMsgCallback(void *handle, int msg, void *detail)
{
    RenderCore* renderCore = static_cast<RenderCore *>(handle);
    DEBUG(renderCore->mLogCategory,"pluginMsgCallback,msg:%d",msg);
    switch (msg) {
        case PLUGIN_MSG_DISPLAY_OPEN_FAIL:
        case PLUGIN_MSG_WINDOW_OPEN_FAIL:
            if (renderCore->mCallback) {
                renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_CONNECTED_FAIL, NULL);
            }
        break;
        default:
            break;
    }
}

void RenderCore::pluginBufferReleaseCallback(void *handle,void *data)
{
    RenderCore* renderCore = static_cast<RenderCore *>(handle);
    if (renderCore->mCallback) {
        renderCore->mReleaseFrameCnt += 1;
        TRACE1(renderCore->mLogCategory,"release buffer %p, pts:%lld,cnt:%d",data,((RenderBuffer *)data)->pts,renderCore->mReleaseFrameCnt);
        renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_RELEASE_BUFFER, data);
    }
}

void RenderCore::pluginBufferDisplayedCallback(void *handle,void *data)
{
    RenderCore* renderCore = static_cast<RenderCore *>(handle);
    if (renderCore->mCallback) {
        renderCore->mDisplayedFrameCnt += 1;
        TRACE1(renderCore->mLogCategory,"displayed buffer %p, pts:%lld,cnt:%d",data,((RenderBuffer *)data)->pts,renderCore->mDisplayedFrameCnt);
        renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_DISPLAYED_BUFFER, data);
    }
}

void RenderCore::pluginBufferDropedCallback(void *handle,void *data)
{
    RenderCore* renderCore = static_cast<RenderCore *>(handle);
    if (renderCore->mCallback) {
        renderCore->mDropFrameCnt += 1;
        WARNING(renderCore->mLogCategory,"drop buffer %p, pts:%lld,cnt:%d",data,((RenderBuffer *)data)->pts,renderCore->mDropFrameCnt);
        renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_DROPED_BUFFER, data);
    }
}

void RenderCore::queueFlushCallback(void *userdata,void *data)
{
    RenderCore* renderCore = static_cast<RenderCore *>(userdata);
    pluginBufferDropedCallback(renderCore, data);
    pluginBufferReleaseCallback(renderCore, data);
}

int64_t RenderCore::nanosecToPTS90K(int64_t nanosec)
{
    return (nanosec / 100) * 9;
}

void RenderCore::setMediasyncPropertys()
{
    //set mediasync source type
    if (mMediaSync && mMediaSyncBind && mMediasyncSourceType.changed) {
        INFO(mLogCategory,"do set mediasync source type: %d",mMediasyncSourceType);
        mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_SOURCETYPE, (void* )&mMediasyncSourceType);
    }

    //set mediasync has audio
    if (mMediaSync && mMediaSyncBind && mMediasyncHasAudio.changed) {
        INFO(mLogCategory,"do set mediasync has audio: %d",mMediasyncHasAudio.value);
         mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_HASAUDIO, (void* )&mMediasyncHasAudio.value);
    }

    //set mediasync video latency
    if (mMediaSync && mMediaSyncBind && mMediasyncVideoLatency.changed) {
        INFO(mLogCategory,"do set mediasync video latency: %d",mMediasyncVideoLatency);
        mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_VIDEOLATENCY, (void* )&mMediasyncVideoLatency.value);
    }

    //set mediasync start threshold
    if (mMediaSync && mMediaSyncBind && mMediasyncStartThreshold.changed) {
        INFO(mLogCategory,"do set mediasync start threshold: %d",mMediasyncStartThreshold.value);
        mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_STARTTHRESHOLD, (void* )&mMediasyncStartThreshold.value);
    }
}

void RenderCore::waitTimeoutUs(int64_t timeoutMs)
{
    Tls::Mutex::Autolock _l(mLimitMutex);
    mLimitCondition.waitRelativeUs(mLimitMutex, timeoutMs);
}

void RenderCore::mediaSyncTunnelmodeDisplay()
{
    mediasync_result ret;
    int64_t nowSystemtimeUs; //us unit
    int64_t nowMediasyncTimeUs; //us unit
    int64_t realtimeUs; //us unit
    int64_t delaytimeUs; //us unit
    int64_t nowPts;
    RenderBuffer *buf;
    int qRet;
    int64_t needWaitTimeUs = 0;

    mRenderMutex.lock();
    if (!mMediaSync || !mMediaSyncBind) {
        WARNING(mLogCategory,"No create mediasync or no init mediasync");
        goto Err_tag;
    }

    qRet = mQueue->peek((void **) &buf, 0);
    if (qRet != Q_OK) {
        WARNING(mLogCategory, "pop item from queue failed");
        goto Err_tag;
    }

    nowPts = buf->pts;

    //update mediasync pts when vmaster
    if (mSyncmode == MEDIA_SYNC_VMASTER) {
        if ( mMediaSyncAnchor == false) {
            mMediaSyncAnchor = true;
            INFO(mLogCategory,"anchor pts:%lld",nowPts);
            if (nowPts == 0) { //if pts is 0, mediasync do not update realtime,so workround
                MediaSync_updateAnchor(mMediaSync, 2*1000, 0, 0);
            } else {
                MediaSync_updateAnchor(mMediaSync, nowPts/1000, 0, 0);
            }
        }
    }

    //get video frame display time
    if (nowPts == 0) {
        ret = MediaSync_getRealTimeFor(mMediaSync, 2*1000/*us*/, &realtimeUs);
    } else {
        ret = MediaSync_getRealTimeFor(mMediaSync, nowPts/1000/*us*/, &realtimeUs);
    }
    if (ret != AM_MEDIASYNC_OK) {
        WARNING(mLogCategory,"get mediasync realtime fail");
        realtimeUs = -1;
    }

    //get systemtime
    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    //get mediasync systemtime
    ret = MediaSync_getRealTimeForNextVsync(mMediaSync, &nowMediasyncTimeUs);
    if (ret != AM_MEDIASYNC_OK) {
        WARNING(mLogCategory,"get mediasync time fail");
    }

    delaytimeUs = realtimeUs - nowMediasyncTimeUs - LATENCY_TO_HDMI_TIME_US;

    if (delaytimeUs <= 0) {
        if (realtimeUs < 0) {
            if (mSyncmode == MEDIA_SYNC_AMASTER) {
                if (mWaitAnchorTimeUs < WAIT_AUDIO_TIME_US) {
                    mWaitAnchorTimeUs += 2000;
                    WARNING(mLogCategory,"waited audio anchor mediasync %d us",mWaitAnchorTimeUs);
                    needWaitTimeUs = 2000;
                    goto Block_tag;
                } else {
                    WARNING(mLogCategory,"wait audio anchor mediasync timeout, use vmaster");
                    MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_VMASTER);
                    mSyncmode = MEDIA_SYNC_VMASTER;
                    mWaitAnchorTimeUs = 0;
                    goto Err_tag;
                }
            }
            if (mSyncmode == MEDIA_SYNC_VMASTER) {
                mWaitAnchorTimeUs += 2000;
                WARNING(mLogCategory,"video had anchored mediasync,wait realtm %d us",mWaitAnchorTimeUs);
                needWaitTimeUs = 2000;
                goto Block_tag;
            }

            /*use pts diff to send frame displaying,when get realtime fail,
            the first frame send imediately,but other frames will send
            interval pts diff time
            */
            if (mLastDisplayPTS >= 0) {
                int64_t ptsdifUs = (nowPts - mLastDisplayPTS)/1000;
                delaytimeUs = (ptsdifUs - LATENCY_TO_HDMI_TIME_US) > 0 ? (ptsdifUs - LATENCY_TO_HDMI_TIME_US) : ptsdifUs;
                if (delaytimeUs > 0 && mIsLimitDisplayFrame) {
                    needWaitTimeUs = delaytimeUs;
                    goto Block_tag;
                } else {
                    delaytimeUs = 0;
                }
                //add a LATENCY_TO_HDMI_TIME_US time that weston will check display success
                realtimeUs = mLastDisplayRealtime + ptsdifUs + LATENCY_TO_HDMI_TIME_US;
            } else if (mLastDisplayPTS == -1) { //first frame,displayed immediately
                realtimeUs = nowMediasyncTimeUs + LATENCY_TO_HDMI_TIME_US;
                delaytimeUs = 0;
            }
        }
    } else {
        if (mIsLimitDisplayFrame) {
            //block thread to wait until delaytime is reached
            TRACE2(mLogCategory,"limit display frame after %lld ms",delaytimeUs/1000);
            needWaitTimeUs = delaytimeUs;
            goto Block_tag;
        }
    }

    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    qRet = mQueue->peek((void **)&buf, 0);
    if (qRet != Q_OK || buf->pts != nowPts) {
        ERROR(mLogCategory,"error, now pts:%lld, but queue first item pts:%lld",nowPts,buf->pts);
        goto Err_tag;
    }
    mQueue->pop((void **)&buf);

    //TRACE3(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,realtmUs:%lld,mtmUs:%lld,stmUs:%lld",buf->pts,mLastDisplayPTS,realtimeUs,nowMediasyncTimeUs,nowSystemtimeUs);

    //display video frame
    TRACE1(mLogCategory,"+++++display frame:%p, ptsNs:%lld(%lld ms),realtmUs:%lld,realtmDiffMs:%lld,realToSysDiffMs:%lld",
            buf,buf->pts,buf->pts/1000000,realtimeUs,(realtimeUs-mLastDisplayRealtime)/1000,(realtimeUs-mLastDisplaySystemtime)/1000);
    if (mPlugin) {
        mPlugin->displayFrame(buf, realtimeUs);
    }
    mLastDisplayPTS = buf->pts;
    mLastDisplayRealtime = realtimeUs;
    mLastDisplaySystemtime = nowSystemtimeUs;
    mRenderMutex.unlock();
    return;
Block_tag:
    mRenderMutex.unlock();
    if (needWaitTimeUs > 0) {
        waitTimeoutUs(needWaitTimeUs);
    }
    return;
Err_tag:
    mRenderMutex.unlock();
    return;
}

void RenderCore::mediaSyncNoTunnelmodeDisplay()
{
    int64_t beforeTimeUs = 0;
    int64_t nowTimeUs = 0;
    int64_t nowMediasyncTimeUs = 0; //us unit
    int64_t realtimeUs = 0; //us unit
    int64_t nowSystemtimeUs = 0; //us unit
    int64_t ptsdifUs = 0; //us unit
    int64_t nowPts = 0;
    RenderBuffer *buf = NULL;
    int64_t needWaitTimeUs = 0;
    int qRet;

    mRenderMutex.lock();
    if (!mMediaSync || !mMediaSyncBind) {
        WARNING(mLogCategory,"No create mediasync or no init mediasync");
        goto Err_tag;
    }

    qRet = mQueue->peek((void **)&buf, 0);
    if (qRet != Q_OK) {
        goto Err_tag;
    }
    nowPts = buf->pts;

    //display video frame
    mediasync_result ret;
    struct mediasync_video_policy vsyncPolicy;

    beforeTimeUs = Tls::Times::getSystemTimeUs();
    ret = MediaSync_VideoProcess(mMediaSync, nowPts/1000, mLastDisplayPTS/1000, MEDIASYNC_UNIT_US, &vsyncPolicy);
    if (ret != AM_MEDIASYNC_OK) {
        ERROR(mLogCategory,"Error MediaSync_VideoProcess");
        goto Err_tag;
    }

    TRACE3(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,policy:%d,realtimeUs:%lld",nowPts,mLastDisplayPTS,vsyncPolicy.videopolicy,vsyncPolicy.param1);
    if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_NORMAL_OUTPUT) {
        qRet = mQueue->peek((void **)&buf, 0);
        if (qRet != Q_OK || buf->pts != nowPts) {
            ERROR(mLogCategory,"error, now pts:%lld, but queue first item pts:%lld",nowPts,buf->pts);
            goto Err_tag;
        }
        mQueue->pop((void **)&buf);

        realtimeUs = vsyncPolicy.param1;
        //get mediasync systemtime
        ret = MediaSync_getRealTimeForNextVsync(mMediaSync, &nowMediasyncTimeUs);
        if (ret != AM_MEDIASYNC_OK) {
            WARNING(mLogCategory,"get mediasync time fail");
        }

        nowSystemtimeUs = Tls::Times::getSystemTimeUs();
        //TRACE3(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,realtmUs:%lld,mtmUs:%lld,stmUs:%lld",buf->pts,mLastDisplayPTS,realtimeUs,nowMediasyncTimeUs,nowSystemtimeUs);

        //display video frame
        TRACE1(mLogCategory,"+++++display frame:%p, ptsNs:%lld(%lld ms),realtmUs:%lld,realtmDiffMs:%lld,toLastDisplayDiffMs:%lld",
            buf,buf->pts,buf->pts/1000000,realtimeUs,(realtimeUs-mLastDisplayRealtime)/1000,(realtimeUs-mLastDisplaySystemtime)/1000);
        if (mPlugin) {
            mPlugin->displayFrame(buf, realtimeUs);
        }

        mLastDisplayPTS = nowPts;
        mLastDisplayRealtime = realtimeUs;
        mLastDisplaySystemtime = nowSystemtimeUs;
        mWaitAnchorTimeUs = 0;
        needWaitTimeUs = 0;
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_HOLD) {
        //vsyncPolicy.param2 is hold time us
        needWaitTimeUs = vsyncPolicy.param2;
        if (needWaitTimeUs < 0) {
            needWaitTimeUs = 4000;
            mWaitAnchorTimeUs += 4000;
        } else {
            mWaitAnchorTimeUs += needWaitTimeUs;
        }
        TRACE2(mLogCategory,"Hold time %lld us,output:%d, allwaittime:%d us",needWaitTimeUs,vsyncPolicy.param2,mWaitAnchorTimeUs);
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_DROP) {
        qRet = mQueue->popAndWait((void **)&buf);
        if (qRet != Q_OK) {
            WARNING(mLogCategory, "pop item from queue failed");
            goto Err_tag;
        }
        if (buf->pts == nowPts) {
            WARNING(mLogCategory,"drop frame pts:%lld",nowPts);
        } else {
            ERROR(mLogCategory,"error, now pts:%lld, but queue fist item pts:%lld",nowPts,buf->pts);
        }
        RenderCore::pluginBufferDropedCallback(this, (void *)buf);
        RenderCore::pluginBufferReleaseCallback(this, (void *)buf);
    }
    mRenderMutex.unlock();
    if (needWaitTimeUs > 0) {
        waitTimeoutUs(needWaitTimeUs);
    }
    return;
Err_tag:
    mRenderMutex.unlock();
    return;
}

void RenderCore::readyToRun()
{
    DEBUG(mLogCategory,"Displaythread,readyToRun");
    //get video buffer format from user
    if (mVideoFormat == VIDEO_FORMAT_UNKNOWN) {
        if (mCallback) {
            mCallback->doGetValue(mUserData, KEY_VIDEO_FORMAT, &mVideoFormat);
        }
        DEBUG(mLogCategory,"get video format %d from user",mVideoFormat);
        //set video format to plugin
        if (mPlugin) {
            mPlugin->set(PLUGIN_KEY_VIDEO_FORMAT, &mVideoFormat);
        }
    }
    //get mediasync instance id if mediasync id had not set
    if (mMediaSync && mMediaSyncInstanceIDSet == false) {
        int mediasyncInstanceId;
        if (mCallback) {
            int ret = mCallback->doGetValue(mUserData, KEY_MEDIASYNC_INSTANCE_ID, (void *)&mediasyncInstanceId);
            DEBUG(mLogCategory,"get mediasync instance id:%d",mediasyncInstanceId);
             if (ret != 0 || mediasyncInstanceId < 0) {
                WARNING(mLogCategory,"get mediasync id fail !!!");
                if (mSyncmode == MEDIA_SYNC_MODE_MAX || mSyncmode == MEDIA_SYNC_AMASTER) {
                    WARNING(mLogCategory,"user set mediasync mode %d, so set vmaster!!!",mSyncmode);
                    mSyncmode = MEDIA_SYNC_VMASTER;
                }
                //if get mediasync id fail, we alloc a mediasync instance id
                MediaSync_allocInstance(mMediaSync, mDemuxId,
                                        mPcrId,
                                        &mMediaSynInstID);
                INFO(mLogCategory,"alloc mediasync instance id:%d",mMediaSynInstID);
            } else {
                mMediaSynInstID = mediasyncInstanceId;
            }
        }
    } else {
        if (mSyncmode == MEDIA_SYNC_MODE_MAX) {
            WARNING(mLogCategory,"user set mediasync mode %d, so set amaster!!!",mSyncmode);
            mSyncmode = MEDIA_SYNC_AMASTER;
        }
    }

    if (mMediaSync && mMediaSynInstID >= 0 && mMediaSyncBind == false) {
        //must set before mediasync bind
        if (mMediasyncPlayerInstanceId.changed) {
            DEBUG(mLogCategory,"do set mediasync player instance id:%d",mMediasyncPlayerInstanceId.value);
            MediaSync_setPlayerInsNumber(mMediaSync, mMediasyncPlayerInstanceId.value);
        }
        //must set before mediasync bind
        if (mMediaSyncTunnelmode.changed) {
            bool tunnelMode = mMediaSyncTunnelmode.value > 0? true:false;
            DEBUG(mLogCategory,"do set mediasync tunnel mode:%d",tunnelMode);
            mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_ISOMXTUNNELMODE, (void* )&tunnelMode);
        }
        if (mMediaSynInstID >= 0) {
            DEBUG(mLogCategory,"bind mediasync instance id:%d",mMediaSynInstID);
            MediaSync_bindStaticInstance(mMediaSync, mMediaSynInstID, MEDIA_VIDEO);
            mMediaSyncBind = true;
        }
    }

    // set sync mode
    INFO(mLogCategory,"do set mediasync sync mode:%d (0:vmaster,1:amaster,2:pcrmaster)",mSyncmode);
    MediaSync_setSyncMode(mMediaSync, (sync_mode)mSyncmode);

    setMediasyncPropertys();
}

bool RenderCore::threadLoop()
{
    int64_t beforeTime = 0;
    int64_t nowTime = 0;
    int64_t ptsInterval = 0;

    if (mPaused || mFlushing || mQueue->getCnt() <= 0) {
        usleep(4*1000);
        return true;
    }

    if (mWinSizeChanged) {
        PluginRect rect;
        rect.x = mWinSize.x;
        rect.y = mWinSize.y;
        rect.h = mWinSize.w;
        rect.w = mWinSize.h;
        mPlugin->set(PLUGIN_KEY_WINDOW_SIZE, &rect);
        mWinSizeChanged = false;
    }

    if (mFrameChanged) {
        PluginFrameSize frameSize;
        frameSize.w = mFrameWidth;
        frameSize.h = mFrameHeight;
        mPlugin->set(PLUGIN_KEY_FRAME_SIZE, &frameSize);
        mFrameChanged = false;
    }

    if (mMediaSync && mMediaSyncBind) {
        if (mMediaSyncTunnelmode.value == 1) {
            mediaSyncTunnelmodeDisplay();
        } else {
            mediaSyncNoTunnelmodeDisplay();
        }
    } else {
        RenderBuffer *buf = NULL;
        mRenderMutex.lock();
        int ret = mQueue->pop((void **)&buf);
        if (ret != Q_OK) {
            WARNING(mLogCategory, "pop item from queue failed");
            mRenderMutex.unlock();
            return true;
        }

        int64_t nowTimeUs = Tls::Times::getSystemTimeUs();
        int64_t displayTimeUs = nowTimeUs;
        if (buf) {
            TRACE1(mLogCategory,"+++++display frame:%p, pts(ns):%lld, displaytime:%lld",buf,buf->pts,displayTimeUs);
            mPlugin->displayFrame(buf, displayTimeUs);
            mLastDisplayPTS = buf->pts;
        }
        mRenderMutex.unlock();

        waitTimeoutUs(mFPSIntervalMs*1000);
    }

    return true;
}

RenderBuffer *RenderCore::allocRenderBufferWrap(int flag, int rawBufferSize)
{
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    RenderBuffer *renderBuf = (RenderBuffer *)calloc(1, sizeof(RenderBuffer));
    if (!renderBuf) {
        ERROR(mLogCategory,"Error No memory");
        return NULL;
    }
    renderBuf->flag = flag;
    if ((flag & BUFFER_FLAG_ALLOCATE_RAW_BUFFER) && rawBufferSize > 0) {
        renderBuf->raw.dataPtr = calloc(1, rawBufferSize);
        renderBuf->raw.size = rawBufferSize;
    }
    renderBuf->id = mBufferId++;
    TRACE3(mLogCategory,"<<malloc buffer id:%d,:%p",renderBuf->id,renderBuf);

    return renderBuf;
}

void RenderCore::releaseRenderBufferWrap(RenderBuffer *buffer)
{
    if (!buffer) {
        ERROR(mLogCategory,"Error NULL params");
        return;
    }
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    TRACE3(mLogCategory,"<<free buffer id:%d,:%p",buffer->id,buffer);

    if ((buffer->flag & BUFFER_FLAG_ALLOCATE_RAW_BUFFER)) {
        free(buffer->raw.dataPtr);
    }
    free(buffer);
    buffer = NULL;
}

void RenderCore::addRenderBuffer(RenderBuffer *buffer)
{
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mAllRenderBufferMap.insert(item);
    TRACE2(mLogCategory,"all renderBuffer cnt:%d",mAllRenderBufferMap.size());
}

void RenderCore::removeRenderBuffer(RenderBuffer *buffer)
{
    if (!buffer) {
        return;
    }
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    auto item = mAllRenderBufferMap.find(buffer->id);
    if (item == mAllRenderBufferMap.end()) {
        return ;
    }
    mAllRenderBufferMap.erase(item);
}

RenderBuffer *RenderCore::findRenderBuffer(RenderBuffer *buffer)
{
    if (!buffer) {
        return NULL;
    }
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    auto item = mAllRenderBufferMap.find(buffer->id);
    if (item == mAllRenderBufferMap.end()) {
        return NULL;
    }
    return (RenderBuffer*) item->second;
}

void RenderCore::setRenderBufferFree(RenderBuffer *buffer)
{
    if (!buffer) {
        return;
    }
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mFreeRenderBufferMap.insert(item);
}

RenderBuffer * RenderCore::getFreeRenderBuffer()
{
    Tls::Mutex::Autolock _l(mBufferMgrMutex);
    auto item = mFreeRenderBufferMap.begin();
    if (item == mFreeRenderBufferMap.end()) {
        return NULL;
    }

    mFreeRenderBufferMap.erase(item);

    return (RenderBuffer*) item->second;
}
