/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <linux/videodev2.h>
#include "wstclient_wayland.h"
#include "wstclient_plugin.h"
#include "Logger.h"

#define TAG "rlib:wstClient_plugin"

#define DEFAULT_VIDEO_SERVER "video"

WstClientPlugin::WstClientPlugin(int logCategory)
    : mState(PLUGIN_STATE_IDLE),
    mFullscreen(true),
    mLogCategory(logCategory)
{
    mIsVideoPip = false;
    mBufferFormat = VIDEO_FORMAT_UNKNOWN;
    mNumDroppedFrames = 0;
    mCommitFrameCnt = 0;
    mWayland = new WstClientWayland(this, logCategory);
    mWstClientSocket = NULL;
    mKeepLastFrame.isSet = false;
    mKeepLastFrame.value = 0;
    mHideVideo.isSet = false;
    mHideVideo.value = 0;
}

WstClientPlugin::~WstClientPlugin()
{
    if (mWayland) {
        delete mWayland;
        mWayland = NULL;
    }

    mState = PLUGIN_STATE_IDLE;
    TRACE2(mLogCategory,"deconstruct");
}

void WstClientPlugin::init()
{
    mState = PLUGIN_STATE_INITED;
}

void WstClientPlugin::release()
{
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

    //connect video server first
    if (!mWstClientSocket) {
        mWstClientSocket = new WstClientSocket(this, mLogCategory);
        mWstClientSocket->connectToSocket(DEFAULT_VIDEO_SERVER);
    }

    if (mWstClientSocket) {
        mWstClientSocket->sendLayerVideoClientConnection(mIsVideoPip);
        mWstClientSocket->sendResourceVideoClientConnection(mIsVideoPip);
    }

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
    mCommitFrameCnt = 0;
    mNumDroppedFrames = 0;
    /*send session info to server
    we use mediasync to sync a/v,so select AV_SYNC_MODE_VIDEO_MONO as av clock*/
    if (mWstClientSocket) {
        mWstClientSocket->sendSessionInfoVideoClientConnection(AV_SYNC_SESSION_V_MONO, AV_SYNC_MODE_VIDEO_MONO);
    }

    //send hide video
    if (mWstClientSocket && mHideVideo.isSet) {
        mWstClientSocket->sendHideVideoClientConnection(mHideVideo.value);
    }

    //send keep last video frame
    if (mWstClientSocket && mKeepLastFrame.isSet) {
        mWstClientSocket->sendKeepLastFrameVideoClientConnection(mKeepLastFrame.value);
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
        TRACE3(mLogCategory,"buffer id:%d,plane[%d],fd:%d,stride:%d,offset:%d",buffer->id,i,buffer->dma.fd[i],buffer->dma.stride[i],buffer->dma.offset[i]);
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
            handleFrameDropped(buffer);
            handleBufferRelease(buffer);
            return ERROR_FAILED_TRANSACTION;
        }
    }

    //storage render buffer to manager
    mRenderLock.lock();
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mRenderBuffersMap.insert(item);
    ++mCommitFrameCnt;
    TRACE1(mLogCategory,"commit to westeros cnt:%d",mCommitFrameCnt);

    //storage displayed render buffer
    std::pair<int, int64_t> displayitem(buffer->id, displayTime);
    mDisplayedFrameMap.insert(displayitem);
    mRenderLock.unlock();

    return NO_ERROR;
}

int WstClientPlugin::flush()
{
    int ret;
    INFO(mLogCategory,"flush");
    if (mWstClientSocket) {
        mWstClientSocket->sendFlushVideoClientConnection();
    }
    //drop frames those had commited to westeros
    std::lock_guard<std::mutex> lck(mRenderLock);
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); ) {
        int bufferid = (int)item->first;
        auto bufItem = mRenderBuffersMap.find(bufferid);
        if (bufItem == mRenderBuffersMap.end()) {
            continue;
        }
        mDisplayedFrameMap.erase(item++);
        RenderBuffer *renderbuffer = (RenderBuffer*) bufItem->second;
        if (renderbuffer) {
            handleFrameDropped(renderbuffer);
        }
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
    if (mWstClientSocket) {
        mWstClientSocket->disconnectFromSocket();
        delete mWstClientSocket;
        mWstClientSocket = NULL;
    }

    std::lock_guard<std::mutex> lck(mRenderLock);
    //drop all frames those don't displayed
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); ) {
        int bufferid = (int)item->first;
        auto bufItem = mRenderBuffersMap.find(bufferid);
        if (bufItem == mRenderBuffersMap.end()) {
            continue;
        }
        mDisplayedFrameMap.erase(item++);
        RenderBuffer *renderbuffer = (RenderBuffer*) bufItem->second;
        if (renderbuffer) {
            handleFrameDropped(renderbuffer);
        }
    }
    //release all frames those had commited to westeros server
    for (auto item = mRenderBuffersMap.begin(); item != mRenderBuffersMap.end();) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mRenderBuffersMap.erase(item++);
        if (renderbuffer) {
            handleBufferRelease(renderbuffer);
        }
    }
    mRenderBuffersMap.clear();
    mDisplayedFrameMap.clear();
    mCommitFrameCnt = 0;
    mNumDroppedFrames = 0;
    return NO_ERROR;
}

int WstClientPlugin::get(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_KEEP_LAST_FRAME: {
            *(int *)value = mKeepLastFrame.value;
            TRACE1(mLogCategory,"get keep last frame:%d",*(int *)value);
        } break;
        case PLUGIN_KEY_HIDE_VIDEO: {
            *(int *)value = mHideVideo.value;
            TRACE1(mLogCategory,"get hide video:%d",*(int *)value);
        } break;
    }
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
        case PLUGIN_KEY_KEEP_LAST_FRAME: {
            int keep = *(int *) (value);
            mKeepLastFrame.value = keep > 0? true:false;
            mKeepLastFrame.isSet = true;
            DEBUG(mLogCategory, "Set keep last frame :%d",mKeepLastFrame.value);
            if (mState & PLUGIN_STATE_WINDOW_OPENED) {
                //DEBUG(mLogCategory, "do keep last frame :%d",mKeepLastFrame.value);
                if (mWstClientSocket) {
                    mWstClientSocket->sendKeepLastFrameVideoClientConnection(mKeepLastFrame.value);
                }
            }
        } break;
        case PLUGIN_KEY_HIDE_VIDEO: {
            int hide = *(int *)(value);
            mHideVideo.value = hide > 0? true:false;
            mHideVideo.isSet = true;
            DEBUG(mLogCategory, "Set hide video:%d",mHideVideo.value);
            if (mState & PLUGIN_STATE_WINDOW_OPENED) {
                //DEBUG(mLogCategory, "do hide video :%d",mHideVideo.value);
                if (mWstClientSocket) {
                    mWstClientSocket->sendHideVideoClientConnection(mHideVideo.value);
                }
            }
        } break;
        case PLUGIN_KEY_FORCE_ASPECT_RATIO: {
            int forceAspectRatio = *(int *)(value);
            if (mWayland && (mState & PLUGIN_STATE_WINDOW_OPENED)) {
                mWayland->setForceAspectRatio(forceAspectRatio > 0? true:false);
            }
        } break;
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
            std::lock_guard<std::mutex> lck(mRenderLock);
            auto item = mRenderBuffersMap.find(bufferid);
            if (item == mRenderBuffersMap.end()) {
                WARNING(mLogCategory,"can't find map Renderbuffer");
                return ;
            }

            --mCommitFrameCnt;
            RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
            //remove had release render buffer
            mRenderBuffersMap.erase(bufferid);

            if (renderbuffer) {
                /*if we can find item in mDisplayedFrameMap,
                this buffer is dropped by westeros server,so we
                must call displayed callback*/
                auto displayFrameItem = mDisplayedFrameMap.find(bufferid);
                if (displayFrameItem != mDisplayedFrameMap.end()) {
                    mDisplayedFrameMap.erase(bufferid);
                    WARNING(mLogCategory,"Frame droped,pts:%lld,displaytime:%lld",renderbuffer->pts,(int64_t)displayFrameItem->second);
                    handleFrameDropped(renderbuffer);
                }
                handleBufferRelease(renderbuffer);
            }
            TRACE1(mLogCategory,"commit to westeros cnt:%d",mCommitFrameCnt);
        } break;
        case WST_STATUS: {
            int dropframes = event->param;
            uint64_t frameTime = event->lparam;
            if (mNumDroppedFrames != event->param) {
                mNumDroppedFrames = event->param;
                WARNING(mLogCategory,"frame dropped cnt:%d",mNumDroppedFrames);
            }
            //update status,if frameTime isn't equal -1LL
            //this buffer had displayed
            if (frameTime != -1LL) {
                std::lock_guard<std::mutex> lck(mRenderLock);
                int bufferId = getDisplayFrameBufferId(frameTime);
                if (bufferId < 0) {
                    WARNING(mLogCategory,"can't find map displayed frame:%lld",frameTime);
                    return ;
                }
                auto displayItem = mDisplayedFrameMap.find(bufferId);
                if (displayItem != mDisplayedFrameMap.end()) {
                    mDisplayedFrameMap.erase(bufferId);
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
            bool globalZoomActive = event->param1 > 0? true:false;
            bool allow4kZoom = event->param2 > 0? true:false;;
            mWayland->setZoomMode(mode, globalZoomActive, allow4kZoom);
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
            break;
        }
    }
    return bufId;
}