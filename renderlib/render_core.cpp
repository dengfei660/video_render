#include <string.h>
#include "render_core.h"
#include "Logger.h"
#include "wayland_plugin.h"
#include "wstclient_plugin.h"
#include "wayland_videoformat.h"
#include "Times.h"
#include "config.h"

#define TAG "rlib:render_core"

#define TIME_NANO_SEC (1000000000)

RenderCore::RenderCore():
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
    mSyncmode = MEDIA_SYNC_PCRMASTER;
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
#if MEDIASYNC_TUNNEL_MODE
    mTunnelmode = true;
#else
    mTunnelmode = false;
#endif
    mAllRenderBufferMap.clear();
}
RenderCore::~RenderCore()
{
    TRACE2("desconstruct");
}

static PluginCallback plugincallback = {
    RenderCore::pluginMsgCallback,
    RenderCore::pluginErrorCallback,
    RenderCore::pluginBufferReleaseCallback
};

int RenderCore::init(char *name)
{
    DEBUG("name:%s",name);
    char *compositor = name;
    mCompositorName = name;
    char *val = getenv("VIDEO_RENDER_COMPOSITOR");
    if (val) {
        INFO("VIDEO_RENDER_COMPOSITOR=%s",val);
        if (!strcmp(val, "weston") || !strcmp(val, "westeros")) {
            compositor = val;
        }
    }

    if (!strcmp(compositor, "wayland") || !strcmp(compositor, "weston")) {
        mPlugin = new WaylandPlugin();
    } else if (!strcmp(compositor, "westeros")) {
        mPlugin = new WstClientPlugin();
    }
    INFO("compositor:%s",compositor);
    mPlugin->init();
    mPlugin->setUserData(this, &plugincallback);
    return NO_ERROR;
}


int RenderCore::release()
{
    DEBUG("release");
    if (isRunning()) {
        DEBUG("try stop render frame thread");
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

    DEBUG("release end");

    return NO_ERROR;
}

void RenderCore::setCallback(RenderCallback *callback)
{
    if (!callback) {
        ERROR("Error param is null");
        return;
    }
    if (!mCallback) {
        mCallback = (RenderCallback *)calloc(1,sizeof(RenderCallback));
    }
    if (!mCallback) {
        ERROR("Error no memery");
        return;
    }
    TRACE1("callback:%p,doMsgSend:%p,doGetValue:%p",callback,callback->doMsgSend,callback->doGetValue);
    mCallback->doMsgSend = callback->doMsgSend;
    mCallback->doGetValue = callback->doGetValue;
}

void RenderCore::setUserData(void *data)
{
    TRACE1("data:%p",data);
    mUserData = data;
}

int RenderCore::connect()
{
    int ret;
    DEBUG("Connect");

    int pluginState = mPlugin->getState();
    if ((pluginState & PLUGIN_STATE_DISPLAY_OPENED) && (pluginState & PLUGIN_STATE_WINDOW_OPENED)) {
        WARNING("Render had connected");
        return NO_ERROR;
    }

    ret = mPlugin->openDisplay();
    if (ret != NO_ERROR) {
        ERROR("Error open display");
        return -1;
    }
    ret = mPlugin->openWindow();
    if (ret != NO_ERROR) {
        ERROR("Error open window");
        return -1;
    }

    DEBUG("Connect,end");
    return NO_ERROR;
}

int RenderCore::disconnect()
{
    DEBUG("Disconnect");
    //sleep to wait last frame displayed
    usleep(30000); //is needed?

    if (isRunning()) {
        DEBUG("stop render frame thread");
        requestExitAndWait();
    }

    if (mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
        TRACE1("try close window");
        mPlugin->closeWindow();
    }
    if (mPlugin->getState() & PLUGIN_STATE_DISPLAY_OPENED) {
        TRACE1("try close display");
        mPlugin->closeDisplay();
    }

    if (mMediaSync) {
        MediaSync_destroy(mMediaSync);
        mMediaSync = NULL;
    }
    //flush cache buffers
    TRACE1("flush cached buffers");
    flush();
    DEBUG("Disconnect,end");
    return NO_ERROR;
}

int RenderCore::displayFrame(RenderBuffer *buffer)
{
    mediaSyncInit(true);

    //if display thread is not running ,start it
    if (!isRunning()) {
        DEBUG("to run displaythread");
        //run display thread
        run("displaythread");
    }
    //try lock
    {
        Tls::Mutex::Autolock _l(mRenderMutex);
        TRACE1("+++++buffer:%p,ptsUs:%lld,ptsdiff:%d ms",buffer,buffer->pts,(buffer->pts/1000000-mLastInputPTS/1000000));

        //if pts is -1,we need calculate it
        if (buffer->pts == -1) {
            if (mVideoFPS > 0) {
                buffer->pts = mLastInputPTS + TIME_NANO_SEC/mVideoFPS;
                TRACE2("correct pts:%lld",buffer->pts);
            }
        }

        //detect input frame and last input frame pts,if equal
        //try remove last frame,if last frame had render,release input frame
        if (mLastInputPTS == buffer->pts) {
            RenderBuffer *lastbuf = mRenderBufferQueue.back();
            if (lastbuf && lastbuf->pts == mLastInputPTS) {
                WARNING("Input frame buf pts is equal last frame pts,release last frame:%p",lastbuf);
                if (mCallback) {
                    TRACE1("release buffer %p",lastbuf);
                    mCallback->doMsgSend(mUserData, MSG_RELEASE_BUFFER, lastbuf);
                }
                mRenderBufferQueue.pop_back();
                return NO_ERROR;
            } else {
                WARNING("Input frame buf pts is equal last frame pts,release input frame:%p",buffer);
                if (mCallback) {
                    TRACE1("release buffer %p",buffer);
                    mCallback->doMsgSend(mUserData, MSG_RELEASE_BUFFER, buffer);
                }
                return NO_ERROR;
            }
        }

        mInFrameCnt += 1;

        mRenderBufferQueue.push_back(buffer);
        TRACE1("queue size:%d, inFrameCnt:%d",mRenderBufferQueue.size(),mInFrameCnt);

        //fps detect
        if (mFPSDetectCnt <= 100) {
            if (mLastInputPTS > 0) {
                int internal = buffer->pts/1000000 - mLastInputPTS/1000000;
                mFPSDetectAcc += internal;
                mFPSDetectCnt++;
                mFPSIntervalMs = mFPSDetectAcc/mFPSDetectCnt;
                TRACE1("fps internal:%d ms,cnt:%d,mFPS_internal:%d",internal,mFPSDetectCnt,mFPSIntervalMs);
            }
        }

        mLastInputPTS = buffer->pts;
    }
    //queue video pts to mediasync
    if (mMediaSync && mMediaSyncInited) {
        if (mTunnelmode == false) {
            MediaSync_queueVideoFrame((void* )mMediaSync, buffer->pts/1000, 0 /*size*/, 0 /*duration*/, MEDIASYNC_UNIT_US);
        }
    }
    return NO_ERROR;
}


int RenderCore::setProp(int property, void *prop)
{
    if (!prop) {
        ERROR("Params is NULL");
        return ERROR_PARAM_NULL;
    }

    switch (property) {
        case KEY_WINDOW_SIZE: {
            RenderWindowSize *win = (RenderWindowSize *) (prop);
            mWinSize.x = win->x;
            mWinSize.y = win->y;
            mWinSize.w = win->w;
            mWinSize.h = win->h;
            DEBUG("set window size:x:%d,y:%d,w:%d,h:%d",mWinSize.x,mWinSize.y,mWinSize.w,mWinSize.h);
            //if window has opened ,set immediately
            if (mPlugin->getState() & PLUGIN_STATE_WINDOW_OPENED) {
                PluginRect rect;
                rect.x = mWinSize.x;
                rect.y = mWinSize.x;
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
            DEBUG("set frame size:w:%d,h:%d",mFrameWidth,mFrameHeight);
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
            DEBUG("set mediasync inst id:%d",mMediaSynInstID);
        } break;
        case KEY_PCR_PID: {
            mPcrId = *(int *)prop;
            DEBUG("set pcr pid:%d",mPcrId);
        } break;
        case KEY_DEMUX_ID: {
            mDemuxId = *(int *)prop;
            DEBUG("set demux id:%d",mDemuxId);
        } break;
        case KEY_MEDIASYNC_SYNC_MODE: {
            mSyncmode = (sync_mode)(*(int *)prop);
            DEBUG("set mediasync sync mode:%d",mSyncmode);
        } break;
        case KEY_VIDEO_FORMAT: {
            mVideoFormat = (RenderVideoFormat)*(int *)prop;
            DEBUG("set video format:%d",mVideoFormat);
            if (mPlugin) {
                mPlugin->set(PLUGIN_KEY_VIDEO_FORMAT, (void *)&mVideoFormat);
            }
        } break;
        case KEY_VIDEO_FPS: {
            int64_t fps = *(int64_t *)prop;
            mVideoFPS_N = int (fps >> 32 & 0xFFFFFFFF);
            mVideoFPS_D = int (fps & 0xFFFFFFFF);
            mVideoFPS = (int) mVideoFPS_N/mVideoFPS_D;
            DEBUG("set video (0x%x)fps_n:%d,fps_d:%d,fps:%d",fps,mVideoFPS_N,mVideoFPS_D,mVideoFPS);
        } break;
        default:
            break;
    }
    return NO_ERROR;
}


int RenderCore::getProp(int property, void *prop)
{
    if (!prop) {
        ERROR("Params is NULL");
        return ERROR_PARAM_NULL;
    }

    switch (property) {
        case KEY_WINDOW_SIZE: {
            RenderWindowSize *win = (RenderWindowSize *) prop;
            win->x = mWinSize.x;
            win->y = mWinSize.y;
            win->w = mWinSize.w;
            win->h = mWinSize.h;
            TRACE1("get prop window size:x:%,y:%y,w:%d,h:%d",win->x,win->y,win->w,win->h);
        } break;
        case KEY_FRAME_SIZE: {
            RenderFrameSize *frame = (RenderFrameSize *) prop;
            frame->frameWidth = mFrameWidth;
            frame->frameHeight = mFrameHeight;
            TRACE1("get prop frame size:w:%d,h:%d",mFrameWidth,mFrameHeight);
        } break;
        case KEY_MEDIASYNC_INSTANCE_ID: {
            *(int *)prop = mMediaSynInstID;
            TRACE1("get prop mediasync inst id:%d",*(int *)prop);
        } break;
        case KEY_PCR_PID: {
            *(int *)prop = mPcrId;
            TRACE1("get prop pcr pid:%d",*(int *)prop);
        } break;
        case KEY_DEMUX_ID: {
            *(int *)prop = mDemuxId;
            TRACE1("get prop demux id:%d",*(int *)prop);
        } break;
        case KEY_MEDIASYNC_SYNC_MODE: {
            *(int *)prop = mSyncmode;
            TRACE1("get mediasync sync mode:%d",*(int *)prop);
        } break;
        default:
            break;
    }
    return NO_ERROR;
}


int RenderCore::flush()
{
    DEBUG("flush start");
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
    DEBUG("flush end");
    return NO_ERROR;
}

int RenderCore::pause()
{
    DEBUG("Pause");
    mPaused = true;
    if (mMediaSync && mMediaSyncInited) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, true);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR("Error set mediasync pause ");
        }
    }
    mPlugin->pause();
    return NO_ERROR;
}

int RenderCore::resume()
{
    DEBUG("Resume");
    mPaused = false;
    if (mMediaSync && mMediaSyncInited) {
        mediasync_result ret = MediaSync_setPause(mMediaSync, false);
        if (ret != AM_MEDIASYNC_OK) {
            ERROR("Error set mediasync resume");
        }
    }
    mPlugin->resume();
    return NO_ERROR;
}

int RenderCore::accquireDmaBuffer(RenderDmaBuffer *dmabuf)
{
    int planeCount;
    int width,height;

    DEBUG("accquireDmaBuffer");
    if (!dmabuf) {
        FATAL("Error param dmabuff is null");
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
    DEBUG("releaseDmaBuffer");
    if (dmabuf) {
        for (int i = 0; i < dmabuf->planeCnt; i++) {
            mPlugin->releaseDmaBuffer(dmabuf->fd[i]);
        }
    }
}

void RenderCore::pluginMsgCallback(void *handle, int msg, void *detail)
{
    RenderCore* renderCore = static_cast<RenderCore *> (handle);
    switch (msg)
    {
        case PLUGIN_MSG_DISPLAYED: {
            renderCore->mDisplayedFrameCnt += 1;
            if (renderCore->mCallback) {
                TRACE1("displayed buffer %p, pts:%lld,cnt:%d",detail,((RenderBuffer *)detail)->pts,renderCore->mDisplayedFrameCnt);
                renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_DISPLAYED_BUFFER, detail);
            }
        } break;
        default:
            break;
    }

     if (renderCore->mCallback) {
         //renderCore->mCallback->doMsgSend();
     }
}

void RenderCore::pluginErrorCallback(void *handle, int errCode, const char *errDetail)
{
    RenderCore* renderCore = static_cast<RenderCore *>(handle);
    DEBUG("PluginErrorCallback,errCode:%d",errCode);
    switch (errCode) {
        case PLUGIN_ERROR_DISPLAY_OPEN_FAIL:
        case PLUGIN_ERROR_WINDOW_OPEN_FAIL:
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
        TRACE1("release buffer %p, pts:%lld",data,((RenderBuffer *)data)->pts);
        renderCore->mCallback->doMsgSend(renderCore->mUserData, MSG_RELEASE_BUFFER, data);
    }
}

int64_t RenderCore::nanosecToPTS90K(int64_t nanosec)
{
    return (nanosec / 100) * 9;
}

void RenderCore::mediaSyncInit(bool allocInstance)
{
#if SUPPORT_MEDIASYNC
    if (!mMediaSync) {
        mMediaSync = MediaSync_create();
        INFO("New MediaSync");
        //get mediasync instance id
        if (mMediaSynInstID <= 0 && mCallback) {
            DEBUG("to get mediasync instance id");
            mCallback->doGetValue(mUserData, KEY_MEDIASYNC_INSTANCE_ID, (void *)&mMediaSynInstID);
            DEBUG("get mediasync instance id:%d",mMediaSynInstID);
        }

        mediasync_setParameter(mMediaSync, MEDIASYNC_KEY_ISOMXTUNNELMODE, (void* )&mTunnelmode);

        //if we had got mediasync instance id,we bind it
        //otherwise we alloc a mediasync instance and bind
        if (mMediaSynInstID >= 0) {
            DEBUG("bind mediasync id:%d,set sync mode amaster",mMediaSynInstID);
            MediaSync_bindInstance(mMediaSync, mMediaSynInstID, MEDIA_VIDEO);
            MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_AMASTER);
            mSyncmode = MEDIA_SYNC_AMASTER;
            mMediaSyncInited = true;
        } else if (allocInstance) {
            MediaSync_allocInstance(mMediaSync, mDemuxId,
                                    mPcrId,
                                    &mMediaSynInstID);
            MediaSync_bindInstance(mMediaSync, mMediaSynInstID, MEDIA_VIDEO);
            MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_VMASTER);
            mSyncmode = MEDIA_SYNC_VMASTER;
            mMediaSyncInited = true;
            DEBUG("alloc mediasync id:%d,set sync mode vmaster",mMediaSynInstID);
        }

        //set midea sync mode
        if (mMediaSynInstID >= 0 && mSyncmode == MEDIA_SYNC_PCRMASTER &&
                    mPcrId > 0 && mPcrId < 0x1fff && mDemuxId > 0) {
             MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_PCRMASTER);
        }
    }
#endif
}

void RenderCore::mediaSyncTunnelmodeDisplay()
{
    mediasync_result ret;
    int64_t nowSystemtimeUs;
    int64_t nowMediasyncTimeUs; //us unit
    int64_t realtimeUs; //us unit
    int64_t delaytimeUs; //us unit

    if (!mMediaSync || !mMediaSyncInited) {
        WARNING("No create mediasync or no init mediasync");
        return;
    }

    auto item = mRenderBufferQueue.cbegin();
    RenderBuffer *buf = (RenderBuffer *)*item;

    //get video frame display time
    ret = MediaSync_getRealTimeFor(mMediaSync, buf->pts/1000/*us*/, &realtimeUs);
    if (ret != AM_MEDIASYNC_OK) {
        WARNING("get mediasync realtime fail");
    }

    //get systemtime
    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    //get mediasync systemtime
    ret = MediaSync_getRealTimeForNextVsync(mMediaSync, &nowMediasyncTimeUs);
    if (ret != AM_MEDIASYNC_OK) {
        WARNING("get mediasync time fail");
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
                    WARNING("wait audio anchor mediasync left %d ms",mWaitAudioAnchorTimeMs);
                    mWaitAudioAnchorTimeMs -= 2;
                } else {
                    WARNING("wait audio anchor mediasync timeout, use vmaster");
                    MediaSync_setSyncMode(mMediaSync, MEDIA_SYNC_VMASTER);
                    mSyncmode = MEDIA_SYNC_VMASTER;
                    MediaSync_updateAnchor(mMediaSync, buf->pts/1000, 0, 0);
                    goto display_tag;
                }

                Tls::Mutex::Autolock _l(mRenderMutex);
                mRenderCondition.waitRelative(mRenderMutex,2);
                return;
            }/* else if (mSyncmode == MEDIA_SYNC_VMASTER) {
                WARNING("wait mediasync update anchor of vmaster,realtime:%lld,mediasynctime:%lld",realtimeUs,nowMediasyncTimeUs);
                Tls::Mutex::Autolock _l(mRenderMutex);
                mRenderCondition.waitRelative(mRenderMutex,2);
                return;
            }*/
            WARNING("mediasync get realtimeUs %lld,use local systemtime",realtimeUs);
            /*use pts diff to send frame displaying,when get realtime fail,
            the first frame send imediately,but other frames will send
            interval pts diff time
            */
            if (mLastDisplayPTS >= 0) {
                int64_t ptsdifUs = (buf->pts - mLastDisplayPTS)/1000;
                delaytimeUs = (ptsdifUs - LATENCY_TO_HDMI_TIME_US) > 0 ? (ptsdifUs - LATENCY_TO_HDMI_TIME_US) : ptsdifUs;
                if (delaytimeUs > 0) {
                    Tls::Mutex::Autolock _l(mRenderMutex);
                    mRenderCondition.waitRelative(mRenderMutex,delaytimeUs/1000); 
                } else {
                    delaytimeUs = 0;
                }
                realtimeUs = mLastDisplayRealtime + ptsdifUs;
            } else if (mLastDisplayPTS == -1) { //first frame,displayed immediately
                realtimeUs = nowMediasyncTimeUs;
                delaytimeUs = 0;
            }
        }
    } else {
        //block thread to wait until delaytime is reached
        Tls::Mutex::Autolock _l(mRenderMutex);
        mRenderCondition.waitRelative(mRenderMutex,delaytimeUs/1000);
    }

display_tag:
    nowSystemtimeUs = Tls::Times::getSystemTimeUs();
    Tls::Mutex::Autolock _l(mRenderMutex);
    mRenderBufferQueue.pop_front();

    TRACE1("PTSNs:%lld,lastPTSNs:%lld,realtmUs:%lld,mtmUs:%lld,stmUs:%lld,wait:%lld ms",buf->pts,mLastDisplayPTS,realtimeUs,nowMediasyncTimeUs,nowSystemtimeUs,delaytimeUs/1000);

    //display video frame
    TRACE1("+++++display frame:%p, ptsNs:%lld(%lld ms),realtmUs:%lld,realtmDiffMs:%lld",buf,buf->pts,buf->pts/1000000,realtimeUs,(realtimeUs-mLastDisplayRealtime)/1000);
    mPlugin->displayFrame(buf, realtimeUs);
    mLastDisplayPTS = buf->pts;
    mLastRenderBuffer = buf;
    mLastDisplayRealtime = realtimeUs;
}

void RenderCore::mediaSyncNoTunnelmodeDisplay()
{
    int64_t beforeTime = 0;
    int64_t nowTime = 0;
    int64_t ptsInterval = 0;

    if (!mMediaSync || !mMediaSyncInited) {
        WARNING("No create mediasync or no init mediasync");
        return;
    }

    //DEBUG("Displaythread,threadLoop mRenderBufferQueue:%d",mRenderBufferQueue.size());
    auto item = mRenderBufferQueue.cbegin();
    RenderBuffer *buf = (RenderBuffer *)*item;

    /*calculate pts interval between this frames to next frame
      if not found next frame, the last frame be used*/
    if (mLastDisplayPTS > 0) {
        ptsInterval = buf->pts - mLastDisplayPTS;
    } else if (++item != mRenderBufferQueue.cend()) {
        RenderBuffer *nextBuf = (RenderBuffer *)*item;
        int64_t nextPts = nextBuf->pts;
        ptsInterval = nextPts - buf->pts;
    }

    //display video frame
    mediasync_result ret;
    struct mediasync_video_policy vsyncPolicy;

    beforeTime = Tls::Times::getSystemTimeMs();
    ret = MediaSync_VideoProcess((void*) mMediaSync, buf->pts/1000, mLastDisplayPTS/1000, MEDIASYNC_UNIT_US, &vsyncPolicy);
    if (ret != AM_MEDIASYNC_OK) {
        ERROR("Error MediaSync_VideoProcess");
        return;
    }
    TRACE1("PTSNs:%lld,lastPTSNs:%lld,policy:%d,realtimeUs:%lld",buf->pts,mLastDisplayPTS,vsyncPolicy.videopolicy,vsyncPolicy.param1);
    if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_NORMAL_OUTPUT) {
        mRenderBufferQueue.pop_front();
        TRACE1("+++++display frame:%p, ptsNs:%lld,realtimeUs:%lld",buf,buf->pts,vsyncPolicy.param1);
        TRACE1("PtsDiffMs:%lld,lastrealtimeUs:%lld,realtimediffUs:%lld,systimeDifMs:%lld",buf->pts/1000000-mLastDisplayPTS/1000000,mLastDisplayRealtime,vsyncPolicy.param1-mLastDisplayRealtime,beforeTime-mLastDisplaySystemtime);
        mPlugin->displayFrame(buf, vsyncPolicy.param1);
        mLastDisplayPTS = buf->pts;
        mLastRenderBuffer = buf;
        mLastDisplayRealtime = vsyncPolicy.param1;

        //we need to calculate the wait time
        int64_t nowTime = mLastDisplaySystemtime = Tls::Times::getSystemTimeMs();
        if (ptsInterval != 0) {
            int64_t needWaitTime = ptsInterval/1000000 - (nowTime - beforeTime); //ms
            TRACE1("thread suspend time:%lld ms",needWaitTime);
            if (needWaitTime < 0) {
                WARNING("displayFrame taking too long time");
                return ;
            }
            Tls::Mutex::Autolock _l(mRenderMutex);
            mRenderCondition.waitRelative(mRenderMutex,needWaitTime);
            TRACE1("thread suspend time :%lld end",needWaitTime);
        } else { //maybe it is first frame,todo
            //yunming suggest wait 8ms,if mediasync report hold policy
            Tls::Mutex::Autolock _l(mRenderMutex);
            mRenderCondition.waitRelative(mRenderMutex,8);
        }
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_HOLD) {
        //yunming suggest wait 8ms,if mediasync report hold policy
        Tls::Mutex::Autolock _l(mRenderMutex);
        mRenderCondition.waitRelative(mRenderMutex,2);
    } else if (vsyncPolicy.videopolicy == MEDIASYNC_VIDEO_DROP) {
        mDropFrameCnt += 1;
        mLastDisplayPTS = buf->pts;
        mLastRenderBuffer = buf;
        mLastDisplayRealtime = vsyncPolicy.param1;
        mRenderBufferQueue.pop_front();
        WARNING("drop frame pts:%lld",buf->pts);
    }
}

void RenderCore::readyToRun()
{
    DEBUG("Displaythread,readyToRun");
    //get video buffer format from user
    if (mVideoFormat == VIDEO_FORMAT_UNKNOWN) {
        if (mCallback) {
            mCallback->doGetValue(mUserData, KEY_VIDEO_FORMAT, &mVideoFormat);
        }
        DEBUG("get video format %d from user",mVideoFormat);
        //set video format to plugin
        mPlugin->set(PLUGIN_KEY_VIDEO_FORMAT, &mVideoFormat);
    }
}

bool RenderCore::threadLoop()
{
    int64_t beforeTime = 0;
    int64_t nowTime = 0;
    int64_t ptsInterval = 0;

    if (mPaused || mFlushing || mRenderBufferQueue.size() <= 0) {
        usleep(10*1000);
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
        if (mTunnelmode) {
            mediaSyncTunnelmodeDisplay();
        } else {
            mediaSyncNoTunnelmodeDisplay();
        }
    } else {
        auto item = mRenderBufferQueue.cbegin();
        RenderBuffer *buf = (RenderBuffer *)*item;

        TRACE1("+++++display frame:%p, pts(ns):%lld",buf,buf->pts);
        mPlugin->displayFrame(buf, 000);
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
        ERROR("Error No memory");
        return NULL;
    }
    renderBuf->flag = flag;
    if ((flag & BUFFER_FLAG_ALLOCATE_RAW_BUFFER) && rawBufferSize > 0) {
        renderBuf->raw.dataPtr = calloc(1, rawBufferSize);
        renderBuf->raw.size = rawBufferSize;
    }
    renderBuf->id = mBufferId++;
    TRACE3("<<malloc buffer id:%d,:%p",renderBuf->id,renderBuf);

    return renderBuf;
}

void RenderCore::releaseRenderBufferWrap(RenderBuffer *buffer)
{
    if (!buffer) {
        ERROR("Error NULL params");
        return;
    }

    TRACE3("<<free buffer id:%d,:%p",buffer->id,buffer);

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
    TRACE2("all renderBuffer cnt:%d",mAllRenderBufferMap.size());
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
