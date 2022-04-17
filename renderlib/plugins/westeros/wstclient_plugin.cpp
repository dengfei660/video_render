#include <linux/videodev2.h>
#include "wstclient_wayland.h"
#include "wstclient_plugin.h"
#include "Logger.h"

#define TAG "rlib:wstClient_plugin"

#define DEFAULT_VIDEO_SERVER "video"

WstClientPlugin::WstClientPlugin(int logCategory)
    : mDisplayLock("displaylock"),
    mRenderLock("renderlock"),
    mState(PLUGIN_STATE_IDLE),
    mFullscreen(true),
    mLogCategory(logCategory)
{
    mIsVideoPip = false;
    mHasSetVideoPip = false;
    mHasSetSessionInfo = false;
    mBufferFormat = VIDEO_FORMAT_UNKNOWN;
    mWayland = new WstClientWayland(this, logCategory);
    mWstClientSocket = new WstClientSocket(this, logCategory);
}

WstClientPlugin::~WstClientPlugin()
{
    if (mWayland) {
        delete mWayland;
        mWayland = NULL;
    }
    if (mWstClientSocket) {
        delete mWstClientSocket;
        mWstClientSocket = NULL;
    }

    mState = PLUGIN_STATE_IDLE;
    TRACE2(mLogCategory,"deconstruct");
}

void WstClientPlugin::init()
{
    mWstClientSocket->connectToSocket(DEFAULT_VIDEO_SERVER);
    mState = PLUGIN_STATE_INITED;
}

void WstClientPlugin::release()
{
    if (mWstClientSocket) {
        mWstClientSocket->disconnectFromSocket();
    }
    mState = PLUGIN_STATE_IDLE;
}

void WstClientPlugin::setUserData(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int WstClientPlugin::acquireDmaBuffer(int framewidth, int frameheight)
{
    return NO_ERROR;
}

int WstClientPlugin::releaseDmaBuffer(int dmafd)
{
    return NO_ERROR;
}

int WstClientPlugin::openDisplay()
{
    int ret = NO_ERROR;

    DEBUG(mLogCategory,"openDisplay");
    ret =  mWayland->connectToWayland();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open display");
        if (mCallback) {
            char errString[] = "open wayland display fail";
            mCallback->doSendMsgCallback(mUserData, PLUGIN_MSG_DISPLAY_OPEN_FAIL,errString);
        }
    } else {
        //run wl display queue dispatch
        DEBUG(mLogCategory,"To run wl display dispatch queue");
        mWayland->run("display queue");
    }

    mState |= PLUGIN_STATE_DISPLAY_OPENED;

    DEBUG(mLogCategory,"openDisplay end");
    return NO_ERROR;
}

int WstClientPlugin::openWindow()
{
    int ret;

    DEBUG(mLogCategory,"openWindow");

    mState |= PLUGIN_STATE_WINDOW_OPENED;
    if (mWstClientSocket && mHasSetVideoPip == false) {
        mWstClientSocket->sendLayerVideoClientConnection(mIsVideoPip);
        mWstClientSocket->sendResourceVideoClientConnection(mIsVideoPip);
        mHasSetVideoPip = true;
    }

    DEBUG(mLogCategory,"openWindow,end");
    return ret;
}

int WstClientPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    bool ret;
    WstBufferInfo wstBufferInfo;
    WstRect wstRect;
    int x,y,w,h;

    /*send session info to server
    we use mediasync to sync a/v,so select AV_SYNC_MODE_VIDEO_MONO as av clock*/
    if (mHasSetSessionInfo == false) {
        mWstClientSocket->sendSessionInfoVideoClientConnection(AV_SYNC_SESSION_V_MONO, AV_SYNC_MODE_VIDEO_MONO);
        mHasSetSessionInfo = true;
    }


    mWayland->getVideoBounds(&x, &y, &w, &h);

    //init wstBufferInfo,must set fd to -1 value
    memset(&wstBufferInfo, 0, sizeof(WstBufferInfo));
    for (int i = 0; i < WST_MAX_PLANES; i++) {
        wstBufferInfo.planeInfo[0].fd = -1;
        wstBufferInfo.planeInfo[1].fd = -1;
        wstBufferInfo.planeInfo[2].fd = -1;
    }

    wstBufferInfo.bufferId = buffer->id;
    wstBufferInfo.planeCount = buffer->dma.planeCnt;
    for (int i = 0; i < buffer->dma.planeCnt; i++) {
        wstBufferInfo.planeInfo[i].fd = buffer->dma.fd[i];
        wstBufferInfo.planeInfo[i].stride = buffer->dma.stride[i];
        wstBufferInfo.planeInfo[i].offset = buffer->dma.offset[i];
        TRACE1(mLogCategory,"buffer id:%d,plane[%d],fd:%d,stride:%d,offset:%d",buffer->id,i,buffer->dma.fd[i],buffer->dma.stride[i],buffer->dma.offset[i]);
    }

    wstBufferInfo.frameWidth = buffer->dma.width;
    wstBufferInfo.frameHeight = buffer->dma.height;
    wstBufferInfo.frameTime = displayTime;

    //change render lib video format to v4l2 support format
    if (mBufferFormat == VIDEO_FORMAT_NV12) {
        wstBufferInfo.pixelFormat = V4L2_PIX_FMT_NV12;
    } else if (mBufferFormat == VIDEO_FORMAT_NV21) {
        wstBufferInfo.pixelFormat = V4L2_PIX_FMT_NV21;
    } else {
        ERROR(mLogCategory,"unknow video buffer format:%d",mBufferFormat);
    }

    wstRect.x = x;
    wstRect.y = y;
    wstRect.w = w;
    wstRect.h = h;

    if (mWstClientSocket) {
        ret = mWstClientSocket->sendFrameVideoClientConnection(&wstBufferInfo, &wstRect);
        if (!ret) {
            ERROR(mLogCategory,"send video frame to server fail");
            handleBufferRelease(buffer);
            return ERROR_FAILED_TRANSACTION;
        }
    }

    //storage render buffer to manager
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mRenderBuffersMap.insert(item);
    //storage displayed render buffer
    std::pair<int, int64_t> displayitem(buffer->id, displayTime);
    mDisplayedFrameMap.insert(displayitem);

    return NO_ERROR;
}

int WstClientPlugin::flush()
{
    int ret;
    INFO(mLogCategory,"flush");
    if (mWstClientSocket) {
        mWstClientSocket->sendFlushVideoClientConnection();
    }
    //free all obtain render buff
    for (auto item = mRenderBuffersMap.begin(); item != mRenderBuffersMap.end(); ) {
        RenderBuffer *renderbuffer = (RenderBuffer*)item->second;
        mRenderBuffersMap.erase(item++);
        handleBufferRelease(renderbuffer);
    }
    return NO_ERROR;
}

int WstClientPlugin::pause()
{
    int ret;
    INFO(mLogCategory,"pause");
    if (mWstClientSocket) {
        mWstClientSocket->sendPauseVideoClientConnection(true);
    }
    return NO_ERROR;
}

int WstClientPlugin::resume()
{
    int ret;
    INFO(mLogCategory,"resume");
    if (mWstClientSocket) {
        mWstClientSocket->sendPauseVideoClientConnection(false);
    }
    return NO_ERROR;
}

int WstClientPlugin::closeDisplay()
{
    mWayland->disconnectFromWayland();
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int WstClientPlugin::closeWindow()
{
    mState &= ~PLUGIN_STATE_WINDOW_OPENED;
    return NO_ERROR;
}


int WstClientPlugin::get(int key, void *value)
{
    return NO_ERROR;
}

int WstClientPlugin::set(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            PluginRect* rect = static_cast<PluginRect*>(value);
            mWinRect.x = rect->x;
            mWinRect.y = rect->y;
            mWinRect.w = rect->w;
            mWinRect.h = rect->h;
            if (mWayland && (mState & PLUGIN_STATE_WINDOW_OPENED)) {
                mWayland->setWindowSize(mWinRect.x, mWinRect.y, mWinRect.w, mWinRect.h);
            } else {
                WARNING(mLogCategory,"Window had not opened");
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            PluginFrameSize * frameSize = static_cast<PluginFrameSize * >(value);
            if (mWayland) {
                mWayland->setFrameSize(frameSize->w, frameSize->h);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int format = *(int *)(value);
            mBufferFormat = (RenderVideoFormat) format;
            DEBUG(mLogCategory,"Set video format :%d",mBufferFormat);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            mIsVideoPip = pip > 0? true:false;
        };
    }
    return NO_ERROR;
}

int WstClientPlugin::getState()
{
    return mState;
}

void WstClientPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::setVideoRect(int videoX, int videoY, int videoWidth, int videoHeight)
{
    if (mWstClientSocket) {
        mWstClientSocket->sendRectVideoClientConnection(videoX, videoY, videoWidth, videoHeight);
    }
}

void WstClientPlugin::onWstSocketEvent(WstEvent *event)
{
    Tls::Mutex::Autolock _l(mMutex);
    switch (event->event)
    {
        case WST_REFRESH_RATE: {
            int rate = event->param;
            INFO(mLogCategory,"refresh rate:%d",rate);
        } break;
        case WST_BUFFER_RELEASE: {
            int bufferid = event->param;
            TRACE2(mLogCategory,"Buffer release id:%d",bufferid);
            auto item = mRenderBuffersMap.find(bufferid);
            if (item == mRenderBuffersMap.end()) {
                WARNING(mLogCategory,"can't find map Renderbuffer");
                return ;
            }
            mRenderBuffersMap.erase(item);
            RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
            if (renderbuffer) {
                /*if we can find item in mDisplayedFrameMap,
                this buffer is dropped by westeros server,so we
                must call displayed callback*/
                auto displayFrameItem = mDisplayedFrameMap.find(bufferid);
                if (displayFrameItem != mDisplayedFrameMap.end()) {
                    mDisplayedFrameMap.erase(bufferid);
                    WARNING(mLogCategory,"Frame droped,pts:%lld,displaytime:%lld",renderbuffer->pts,(int64_t)displayFrameItem->second);
                    handleFrameDropped(renderbuffer);
                    handleFrameDisplayed(renderbuffer);
                }
                handleBufferRelease(renderbuffer);
            }
        } break;
        case WST_STATUS: {
            int dropframes = event->param;
            uint64_t frameTime = event->lparam2;
            if (mNumDroppedFrames != event->param) {
                mNumDroppedFrames = event->param;
                WARNING(mLogCategory,"frame dropped cnt:%d",mNumDroppedFrames);
            }
            //update status,if frameTime isn't equal -1LL
            //this buffer had displayed
            if (frameTime != -1LL) {
                mLastDisplayFramePTS = frameTime;
                int bufferId = getDisplayFrameBufferId(frameTime);
                if (bufferId < 0) {
                    WARNING(mLogCategory,"can't find map displayed frame:%lld",frameTime);
                    return ;
                }

                auto item = mRenderBuffersMap.find(bufferId);
                if (item != mRenderBuffersMap.end()) {
                    RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
                    if (renderbuffer) {
                        handleFrameDisplayed(renderbuffer);
                    }
                }
            }
        } break;
        case WST_UNDERFLOW: {
            /* code */
        } break;
        case WST_ZOOM_MODE: {
            int mode = event->param;
            mWayland->setZoomMode(mode);
        } break;
        case WST_DEBUG_LEVEL: {
            /* code */
        } break;
        default:
            break;
    }
}

int WstClientPlugin::getDisplayFrameBufferId(int64_t displayTime)
{
    int bufId = -1;
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); item++) {
        int64_t time = (int64_t)item->second;
        if (time == displayTime) {
            bufId = (int)item->first;
            mDisplayedFrameMap.erase(bufId);
            break;
        }
    }
    return bufId;
}