#include <string.h>
#include "render_core.h"
#include "Logger.h"
#include "wayland_plugin.h"
#include "wstclient_plugin.h"
#include "wayland_videoformat.h"
#include "videotunnel_plugin.h"
#include "Times.h"
#include "config.h"

#define TAG "rlib:render_core"

#define TIME_NANO_SEC (1000000000)

RenderCore::RenderCore(int renderlibId, int logCategory)
    : mRenderlibId(renderlibId),
    mLogCategory(logCategory),
    mPaused(false),
    mFlushing(false),
    mMediaSynInstID(-1),
    mVideoFormat(VIDEO_FORMAT_UNKNOWN),
    mRenderMutex("renderMutex")
{
    mCallback = NULL;
    mMediaSync= NULL;
    mWinSizeChanged = false;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mFrameChanged = false;
    mDemuxId = 0;
    mPcrId = 0;
    mSyncmode = MEDIA_SYNC_MODE_MAX;
    mMediaSyncInited = false;
    mMediaSyncConfigureChanged = false;
    mLastInputPTS = -1;
    mLastDisplayPTS = -1;
    mLastDisplayRealtime = 0;
    mLastRenderBuffer = NULL;
    mVideoFPS = 0;
    mVideoFPS_N = 0;
    mVideoFPS_D = 0;
    mFPSIntervalMs = 0;
    mFPSDetectCnt = 0;
    mFPSDetectAcc = 0;
    mDropFrameCnt = 0;
    mBufferId = 1;
    mLastDisplaySystemtime = 0;
    mWaitAudioAnchorTimeMs = WAIT_AUDIO_TIME_MS;
    mDisplayedFrameCnt = 0;
    mInFrameCnt = 0;
    mMediasyncHasAudio = -1;
    mIsLimitDisplayFrame = true;
    //limit display frame,invalid when value is 0,other > 0 is enable
    char *env = getenv("VIDEO_RENDER_LIMIT_SEND_FRAME");
    if (env) {
        int limit = atoi(env);
        if (limit == 0) {
            mIsLimitDisplayFrame = false;
            INFO(mLogCategory,"No limit send frame");
        }
    }
    mMediaSyncTunnelmode = false;

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
        if (!strcmp(val, "weston") || !strcmp(val, "westeros")) {
            compositor = val;
        }
    }

    if (!strcmp(compositor, "wayland") || !strcmp(compositor, "weston")) {
        mPlugin = new WaylandPlugin(mLogCategory);
    } else if (!strcmp(compositor, "westeros")) {
        mPlugin = new WstClientPlugin(mLogCategory);
    }
#ifdef SUPPORT_VIDEOTUNNEL
    else if (!strcmp(compositor, "videotunnel")) {
        mPlugin = new VideoTunnelPlugin(mLogCategory);
    }
#endif
    INFO(mLogCategory,"compositor:%s",compositor);
    mPlugin->init();
    mPlugin->setUserData(this, &plugincallback);

    if (!mMediaSync) {
        mMediaSync = MediaSync_create();
        INFO(mLogCategory,"New MediaSync");
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
        mMediaSyncInited = false;
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

    if (!mMediaSync) {
        mMediaSync = MediaSync_create();
        INFO(mLogCategory,"New MediaSync");
    }

    int pluginState = mPlugin->getState();
    if ((pluginState & PLUGIN_STATE_DISPLAY_OPENED) && (pluginState & PLUGIN_STATE_WINDOW_OPENED)) {
        WARNING(mLogCategory,"Render had connected");
        return NO_ERROR;
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
    usleep(30000); //is needed?

    if (isRunning()) {
        DEBUG(mLogCategory,"stop render frame thread");
        requestExitAndWait();
    }

    if (mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
        TRACE1(mLogCategory,"try close window");
        mPlugin->closeWindow();
    }
    if (mPlugin->getState() & PLUGIN_STATE_DISPLAY_OPENED) {
        TRACE1(mLogCategory,"try close display");
        mPlugin->closeDisplay();
    }

    if (mMediaSync) {
        MediaSync_destroy(mMediaSync);
        mMediaSync = NULL;
        mMediaSyncInited = false;
    }
    //flush cache buffers
    TRACE1(mLogCategory,"flush cached buffers");
    flush();
    DEBUG(mLogCategory,"Disconnect,end");
    return NO_ERROR;
}

int RenderCore::displayFrame(RenderBuffer *buffer)
{
    //if display thread is not running ,start it
    if (!isRunning()) {
        DEBUG(mLogCategory,"to run displaythread");
        //run display thread
        run("displaythread");
    }
    //try lock
    {
        Tls::Mutex::Autolock _l(mRenderMutex);
        TRACE1(mLogCategory,"+++++buffer:%p,ptsUs:%lld,ptsdiff:%d ms",buffer,buffer->pts,(buffer->pts/1000000-mLastInputPTS/1000000));

        //if pts is -1,we need calculate it
        if (buffer->pts == -1) {
            if (mVideoFPS > 0) {
                buffer->pts = mLastInputPTS + TIME_NANO_SEC/mVideoFPS;
                TRACE2(mLogCategory,"correct pts:%lld",buffer->pts);
            }
        }

        //detect input frame and last input frame pts,if equal
        //try remove last frame,if last frame had render,release input frame
        if (mLastInputPTS == buffer->pts) {
            auto item = mRenderBufferQueue.cend();
            if (item != mRenderBufferQueue.end()) {
                RenderBuffer *buf = (RenderBuffer *)*item;
                if (buf->pts = mLastInputPTS) {
                    mRenderBufferQueue.pop_back();
                    WARNING(mLogCategory,"Input frame buf pts is equal last frame pts,release last frame:%p",buf);
                    if (mCallback) {
                        TRACE1(mLogCategory,"release buffer %p",buf);
                        mCallback->doMsgSend(mUserData, MSG_DROPED_BUFFER, buf);
                        mCallback->doMsgSend(mUserData, MSG_RELEASE_BUFFER, buf);
                    }
                    return NO_ERROR;
                }
            } else {
                WARNING(mLogCategory,"Input frame buf pts is equal last frame pts,release input frame:%p",buffer);
                if (mCallback) {
                    TRACE1(mLogCategory,"release buffer %p",buffer);
                    mCallback->doMsgSend(mUserData, MSG_DROPED_BUFFER, buffer);
                    mCallback->doMsgSend(mUserData, MSG_RELEASE_BUFFER, buffer);
                }
                return NO_ERROR;
            }
        }

        mInFrameCnt += 1;

        mRenderBufferQueue.push_back(buffer);
        TRACE1(mLogCategory,"queue size:%d, inFrameCnt:%d",mRenderBufferQueue.size(),mInFrameCnt);

        //fps detect
        if (mFPSDetectCnt <= 100) {
            if (mLastInputPTS > 0) {
                int internal = buffer->pts/1000000 - mLastInputPTS/1000000;
                mFPSDetectAcc += internal;
                mFPSDetectCnt++;
                mFPSIntervalMs = mFPSDetectAcc/mFPSDetectCnt;
                TRACE1(mLogCategory,"fps internal:%d ms,cnt:%d,mFPS_internal:%d",internal,mFPSDetectCnt,mFPSIntervalMs);
            }
        }

        mLastInputPTS = buffer->pts;
    }
    //queue video pts to mediasync
    // if (mMediaSync && mMediaSyncInited) {
    //     if (mMediaSyncTunnelmode == false) {
    //         MediaSync_queueVideoFrame((void* )mMediaSync, buffer->pts/1000, 0 /*size*/, 0 /*duration*/, MEDIASYNC_UNIT_US);
    //     }
    // }
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
            if (mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
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
            if (mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
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
            DEBUG(mLogCategory,"set mediasync sync mode:%d",mSyncmode);
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
        case KEY_MEDIASYNC_TUNNEL_MODE: {
            int mode = *(int *)(prop);
            if (mode > 0) {
                mMediaSyncTunnelmode = true;
            } else {
                mMediaSyncTunnelmode = false;
            }
            DEBUG(mLogCategory,"set mediasync tunnel mode :%d",mMediaSyncTunnelmode);
        } break;
        case KEY_MEDIASYNC_HAS_AUDIO: {

        } break;
        case KEY_VIDEOTUNNEL_ID: {
            int videotunnelId = *(int *)(prop);
            DEBUG(mLogCategory,"set videotunnel id :%d",videotunnelId);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_VIDEOTUNNEL_ID, (void *)&videotunnelId);
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
        case KEY_MEDIASYNC_INSTANCE_ID: {
            if (mMediaSync && mMediaSynInstID < 0) {
                MediaSync_allocInstance(mMediaSync, mDemuxId,
                                    mPcrId,
                                    &mMediaSynInstID);
            }
            *(int *)prop = mMediaSynInstID;
            TRACE1(mLogCategory,"get prop mediasync inst id:%d",*(int *)prop);
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
        case KEY_MEDIASYNC_TUNNEL_MODE: {
            *(int *)prop = mMediaSyncTunnelmode;
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
    for (auto item = mRenderBufferQueue.begin(); item != mRenderBufferQueue.end();) {
        RenderBuffer *buf = (RenderBuffer *)*item;
        pluginBufferReleaseCallback(this, buf);
        mRenderBufferQueue.erase(item++);
    }

    //flush plugin
    mPlugin->flush();

    mFlushing = false;
    DEBUG(mLogCategory,"flush end");
    return NO_ERROR;
}

int RenderCore::pause()
{
    DEBUG(mLogCategory,"Pause");
    mPaused = true;
    if (mMediaSync && mMediaSyncInited) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, true);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR(mLogCategory,"Error set mediasync pause ");
        }
    }
    mPlugin->pause();
    return NO_ERROR;
}

int RenderCore::resume()
{
    DEBUG(mLogCategory,"Resume");
    mPaused = false;
    if (mMediaSync && mMediaSyncInited) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, false);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR(mLogCategory,"Error set mediasync resume");
        }
    }
    mPlugin->resume();
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
    if (mMediaSync && mMediaSyncInited) {
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
    if (mMediaSync && mMediaSyncInited) {
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

int RenderCore::getPlaybackRate(float *scale)
{
    //todo
    return 0;
}

int RenderCore::queueDemuxPts(int64_t ptsUs, uint32_t size)
{
    mediasync_result ret;
    TRACE3(mLogCategory,"ptsUs:%lld,size:%u",ptsUs,size);
    if (mMediaSync && mMediaSynInstID >= 0) {
        if (mMediaSyncTunnelmode == false) {
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
        TRACE1(renderCore->mLogCategory,"release buffer %p, pts:%lld",data,((RenderBuffer *)data)->pts);
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
        renderCore->mDisplayedFrameCnt += 1;
        WARNING(renderCore->mLogCategory,"drop buffer %p, pts:%lld,cnt:%d",data,((RenderBuffer *)data)->pts,renderCore->mDropFrameCnt);
        renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_DROPED_BUFFER, data);
    }
}

int64_t RenderCore::nanosecToPTS90K(int64_t nanosec)
{
    return (nanosec / 100) * 9;
}

void RenderCore::mediaSyncTunnelmodeDisplay()
{
    mediasync_result ret;
    int64_t nowSystemtimeUs; //us unit
    int64_t nowMediasyncTimeUs; //us unit
    int64_t realtimeUs; //us unit
    int64_t delaytimeUs; //us unit

    if (!mMediaSync || !mMediaSyncInited) {
        WARNING(mLogCategory,"No create mediasync or no init mediasync");
        return;
    }

    auto item = mRenderBufferQueue.cbegin();
    RenderBuffer *buf = (RenderBuffer *)*item;

    //get video frame display time
    ret = MediaSync_getRealTimeFor(mMediaSync, buf->pts/1000/*us*/, &realtimeUs);
    if (ret != AM_MEDIASYNC_OK) {
        WARNING(mLogCategory,"get mediasync realtime fail");
    }

    //get systemtime
    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    //get mediasync systemtime
    ret = MediaSync_getRealTimeForNextVsync(mMediaSync, &nowMediasyncTimeUs);
    if (ret != AM_MEDIASYNC_OK) {
        WARNING(mLogCategory,"get mediasync time fail");
    }

    //update mediasync pts when vmaster
    if (mSyncmode == MEDIA_SYNC_VMASTER) {
        if (realtimeUs < nowSystemtimeUs) {
            MediaSync_updateAnchor(mMediaSync, buf->pts/1000, 0, 0);
        }
    }

    delaytimeUs = realtimeUs - nowMediasyncTimeUs - LATENCY_TO_HDMI_TIME_US;

    if (delaytimeUs <= 0) {
        if (realtimeUs < 0) {
            if (mSyncmode == MEDIA_SYNC_AMASTER) {
                if (mWaitAudioAnchorTimeMs > 0) {
                    WARNING(mLogCategory,"wait audio anchor mediasync left %d ms",mWaitAudioAnchorTimeMs);
                    mWaitAudioAnchorTimeMs -= 2;
                    Tls::Mutex::Autolock _l(mRenderMutex);
                    mRenderCondition.waitRelative(mRenderMutex,2);
                    return;
                } else {
                    WARNING(mLogCategory,"wait audio anchor mediasync timeout, use vmaster");
                    MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_VMASTER);
                    mSyncmode = MEDIA_SYNC_VMASTER;
                    MediaSync_updateAnchor(mMediaSync, buf->pts/1000, 0, 0);
                }
            }

            WARNING(mLogCategory,"mediasync get realtimeUs %lld,use local systemtime",realtimeUs);
            /*use pts diff to send frame displaying,when get realtime fail,
            the first frame send imediately,but other frames will send
            interval pts diff time
            */
            if (mLastDisplayPTS >= 0) {
                int64_t ptsdifUs = (buf->pts - mLastDisplayPTS)/1000;
                delaytimeUs = (ptsdifUs - LATENCY_TO_HDMI_TIME_US) > 0 ? (ptsdifUs - LATENCY_TO_HDMI_TIME_US) : ptsdifUs;
                if (delaytimeUs > 0 && mIsLimitDisplayFrame) {
                    Tls::Mutex::Autolock _l(mRenderMutex);
                    mRenderCondition.waitRelative(mRenderMutex,delaytimeUs/1000);
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
            Tls::Mutex::Autolock _l(mRenderMutex);
            mRenderCondition.waitRelative(mRenderMutex,delaytimeUs/1000);
        }
    }

    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    Tls::Mutex::Autolock _l(mRenderMutex);
    mRenderBufferQueue.pop_front();

    TRACE1(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,realtmUs:%lld,mtmUs:%lld,stmUs:%lld",buf->pts,mLastDisplayPTS,realtimeUs,nowMediasyncTimeUs,nowSystemtimeUs);

    //display video frame
    TRACE1(mLogCategory,"+++++display frame:%p, ptsNs:%lld(%lld ms),realtmUs:%lld,realtmDiffMs:%lld",buf,buf->pts,buf->pts/1000000,realtimeUs,(realtimeUs-mLastDisplayRealtime)/1000);
    mPlugin->displayFrame(buf, realtimeUs);
    mLastDisplayPTS = buf->pts;
    mLastRenderBuffer = buf;
    mLastDisplayRealtime = realtimeUs;
}

void RenderCore::mediaSyncNoTunnelmodeDisplay()
{
    int64_t beforeTimeUs = 0;
    int64_t nowTimeUs = 0;
    int64_t nowMediasyncTimeUs = 0; //us unit
    int64_t realtimeUs = 0; //us unit
    int64_t nowSystemtimeUs = 0; //us unit
    int64_t ptsdifUs = 0; //us unit

    if (!mMediaSync || !mMediaSyncInited) {
        WARNING(mLogCategory,"No create mediasync or no init mediasync");
        return;
    }

    auto item = mRenderBufferQueue.cbegin();
    RenderBuffer *buf = (RenderBuffer *)*item;

    //display video frame
    mediasync_result ret;
    struct mediasync_video_policy vsyncPolicy;

    beforeTimeUs = Tls::Times::getSystemTimeUs();
    ret = MediaSync_VideoProcess((void*) mMediaSync, buf->pts/1000, mLastDisplayPTS/1000, MEDIASYNC_UNIT_US, &vsyncPolicy);
    if (ret != AM_MEDIASYNC_OK) {
        ERROR(mLogCategory,"Error MediaSync_VideoProcess");
        return;
    }

    TRACE1(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,policy:%d,realtimeUs:%lld",buf->pts,mLastDisplayPTS,vsyncPolicy.videopolicy,vsyncPolicy.param1);
    if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_NORMAL_OUTPUT) {
        mRenderBufferQueue.pop_front();
        realtimeUs = vsyncPolicy.param1;
        //get mediasync systemtime
        ret = MediaSync_getRealTimeForNextVsync(mMediaSync, &nowMediasyncTimeUs);
        if (ret != AM_MEDIASYNC_OK) {
            WARNING(mLogCategory,"get mediasync time fail");
        }

        nowSystemtimeUs = Tls::Times::getSystemTimeUs();
        TRACE1(mLogCategory,"PTSNs:%lld,lastPTSNs:%lld,realtmUs:%lld,mtmUs:%lld,stmUs:%lld",buf->pts,mLastDisplayPTS,realtimeUs,nowMediasyncTimeUs,nowSystemtimeUs);

        //display video frame
        TRACE1(mLogCategory,"+++++display frame:%p, ptsNs:%lld(%lld ms),realtmUs:%lld,realtmDiffMs:%lld",buf,buf->pts,buf->pts/1000000,realtimeUs,(realtimeUs-mLastDisplayRealtime)/1000);
        mPlugin->displayFrame(buf, realtimeUs);

        /*calculate pts interval between this frames to next frame
        if not found next frame, the last frame be used*/
        item = mRenderBufferQueue.cbegin();
        if (item != mRenderBufferQueue.cend()) {
            RenderBuffer *nextBuf = (RenderBuffer *)*item;
            int64_t nextPts = nextBuf->pts;
            ptsdifUs = (nextPts - buf->pts)/1000;
        } else {
            ptsdifUs = (buf->pts - mLastDisplayPTS)/1000;
            /*if ptsdifUs large than 24 FPS interval(40ms)
            force ptsdifUs to 8ms*/
            if (ptsdifUs > 40*1000) {
                ptsdifUs = 8*1000;
            }
        }

        mLastDisplayPTS = buf->pts;
        mLastRenderBuffer = buf;
        mLastDisplayRealtime = realtimeUs;
        mLastDisplaySystemtime = nowSystemtimeUs;

        //we need to calculate the wait time
        int64_t nowTimeUs = Tls::Times::getSystemTimeUs();
        int64_t needWaitTimeUs = ptsdifUs - (nowTimeUs - beforeTimeUs); //ms
        TRACE2(mLogCategory,"limit display frame after %lld ms",needWaitTimeUs/1000);
        if (needWaitTimeUs < 0) {
            WARNING(mLogCategory,"displayFrame taking too long time");
            return ;
        }
        Tls::Mutex::Autolock _l(mRenderMutex);
        mRenderCondition.waitRelative(mRenderMutex,needWaitTimeUs/1000);
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_HOLD) {
        //yunming suggest wait 8ms,if mediasync report hold policy
        Tls::Mutex::Autolock _l(mRenderMutex);
        mRenderCondition.waitRelative(mRenderMutex,4);
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_DROP) {
        mDropFrameCnt += 1;
        mLastDisplayPTS = buf->pts;
        mLastRenderBuffer = buf;
        //mLastDisplayRealtime = vsyncPolicy.param1;
        mRenderBufferQueue.pop_front();
        WARNING(mLogCategory,"drop frame pts:%lld",buf->pts);
        RenderCore::pluginBufferDropedCallback(this, (void *)buf);
        RenderCore::pluginBufferReleaseCallback(this, (void *)buf);
    }
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
        mPlugin->set(PLUGIN_KEY_VIDEO_FORMAT, &mVideoFormat);
    }
    //get mediasync instance id if mediasync id had not set
    if (mMediaSync && mMediaSynInstID < 0) {
        if (mCallback) {
            mCallback->doGetValue(mUserData, KEY_MEDIASYNC_INSTANCE_ID, (void *)&mMediaSynInstID);
            DEBUG(mLogCategory,"get mediasync instance id:%d",mMediaSynInstID);
            if (mMediaSynInstID >= 0 && mSyncmode == MEDIA_SYNC_MODE_MAX) {
                WARNING(mLogCategory,"user not set mediasync mode, so set amaster!!!");
                mSyncmode = MEDIA_SYNC_AMASTER;
            } else if (mMediaSynInstID < 0) {
                WARNING(mLogCategory,"get mediasync id fail !!!");
            }
        }
        if (mMediaSynInstID < 0) {
            MediaSync_allocInstance(mMediaSync, mDemuxId,
                                    mPcrId,
                                    &mMediaSynInstID);
            if (mSyncmode == MEDIA_SYNC_MODE_MAX) {
                WARNING(mLogCategory,"user not set mediasync mode, so set vmaster!!!");
                mSyncmode = MEDIA_SYNC_VMASTER;
            }
        }
    }
    //to bind mediasync id and set sync mode
    if (mMediaSync && mMediaSynInstID > 0) {
        INFO(mLogCategory,"mediasync set tunnel mode: %d",mMediaSyncTunnelmode);
        mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_ISOMXTUNNELMODE, (void* )&mMediaSyncTunnelmode);
        INFO(mLogCategory,"mediasync bindInstance %d,sync mode:%d (0:vmaster,1:amaster,2:pcrmaster)",mMediaSynInstID,mSyncmode);
        MediaSync_bindInstance(mMediaSync, mMediaSynInstID, MEDIA_VIDEO);
        if (mSyncmode == MEDIA_SYNC_MODE_MAX) {
            WARNING(mLogCategory,"user not set mediasync mode, so use default amaster!!!");
            mSyncmode = MEDIA_SYNC_AMASTER;
        }
        MediaSync_setSyncMode(mMediaSync, (sync_mode)mSyncmode);
        mMediaSyncInited = true;
    }
}

bool RenderCore::threadLoop()
{
    int64_t beforeTime = 0;
    int64_t nowTime = 0;
    int64_t ptsInterval = 0;

    if (mPaused || mFlushing || mRenderBufferQueue.size() <= 0) {
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

    if (mMediaSync && mMediaSyncInited) {
        if (mMediaSyncTunnelmode) {
            mediaSyncTunnelmodeDisplay();
        } else {
            mediaSyncNoTunnelmodeDisplay();
        }
    } else {
        auto item = mRenderBufferQueue.cbegin();
        RenderBuffer *buf = (RenderBuffer *)*item;
        int64_t nowTimeUs = Tls::Times::getSystemTimeUs();
        int64_t displayTimeUs = nowTimeUs;

        TRACE1(mLogCategory,"+++++display frame:%p, pts(ns):%lld, displaytime:%lld",buf,buf->pts,displayTimeUs);
        mPlugin->displayFrame(buf, displayTimeUs);
        mLastDisplayPTS = buf->pts;
        mLastRenderBuffer = buf;
        mRenderBufferQueue.pop_front();
        //Tls::Mutex::Autolock _l(mRenderMutex);
        //mRenderCondition.waitRelative(mRenderMutex,mFPSIntervalMs);
    }

    return true;
}

RenderBuffer *RenderCore::allocRenderBufferWrap(int flag, int rawBufferSize)
{
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

    TRACE3(mLogCategory,"<<free buffer id:%d,:%p",buffer->id,buffer);

    if ((buffer->flag & BUFFER_FLAG_ALLOCATE_RAW_BUFFER)) {
        free(buffer->raw.dataPtr);
    }
    free(buffer);
    buffer = NULL;
}

void RenderCore::addRenderBuffer(RenderBuffer *buffer)
{
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mAllRenderBufferMap.insert(item);
    TRACE2(mLogCategory,"all renderBuffer cnt:%d",mAllRenderBufferMap.size());
}

void RenderCore::removeRenderBuffer(RenderBuffer *buffer)
{
    if (!buffer) {
        return;
    }

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
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mFreeRenderBufferMap.insert(item);
}

RenderBuffer * RenderCore::getFreeRenderBuffer()
{
    auto item = mFreeRenderBufferMap.begin();
    if (item == mFreeRenderBufferMap.end()) {
        return NULL;
    }

    mFreeRenderBufferMap.erase(item);

    return (RenderBuffer*) item->second;
}
