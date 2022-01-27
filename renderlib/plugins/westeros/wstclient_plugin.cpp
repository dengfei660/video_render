#include "wstclient_wayland.h"
#include "wstclient_plugin.h"
#include "Logger.h"

#define TAG "rlib:wstClient_plugin"

WstClientPlugin::WstClientPlugin()
    : mDisplayLock("displaylock"),
    mRenderLock("renderlock"),
    mState(PLUGIN_STATE_IDLE),
    mFullscreen(true)
{
    mWayland = new WstClientWayland(this);
}

WstClientPlugin::~WstClientPlugin()
{
    if (mWayland) {
        delete mWayland;
        mWayland = NULL;
    }

    mState = PLUGIN_STATE_IDLE;
    TRACE2("desconstruct");
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
    int ret;

    DEBUG("openDisplay");
    ret =  mWayland->openDisplay();
    if (ret != NO_ERROR) {
        ERROR("Error open display");
        if (mCallback) {
            std::string errString = "open wayland display fail";
            mCallback->doSendErrorCallback(mUserData, PLUGIN_ERROR_DISPLAY_OPEN_FAIL,errString.c_str());
        }
        return ret;
    }
    mState |= PLUGIN_STATE_DISPLAY_OPENED;

    //run wl display queue dispatch
    DEBUG("To run wl display dispatch queue");
    mWayland->run("display queue");

    DEBUG("openDisplay end");
    return ret;
}

int WstClientPlugin::openWindow()
{
    int ret;

    DEBUG("openWindow");

    mWayland->openWindow();

    mState |= PLUGIN_STATE_WINDOW_OPENED;

    DEBUG("openWindow,end");
    return ret;
}

int WstClientPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mWayland->displayFrameBuffer(buffer, displayTime);
    return NO_ERROR;
}

int WstClientPlugin::flush()
{
    int ret;
    INFO("flush");
    ret = mWayland->flush();
    return NO_ERROR;
}

int WstClientPlugin::pause()
{
    int ret;
    INFO("pause");
    ret = mWayland->pause();
    return NO_ERROR;
}

int WstClientPlugin::resume()
{
    int ret;
    INFO("resume");
    ret = mWayland->resume();
    return NO_ERROR;
}

int WstClientPlugin::closeDisplay()
{
    mWayland->closeDisplay();
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int WstClientPlugin::closeWindow()
{
    mWayland->closeWindow();
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
                WARNING("Window had not opened");
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            PluginFrameSize * frameSize = static_cast<PluginFrameSize * >(value);
            if (mWayland) {
                mWayland->setFrameSize(frameSize->w, frameSize->h);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int videoFormat = *(int *)(value);
            DEBUG("Set video format :%d",videoFormat);
            mWayland->setVideoBufferFormat((RenderVideoFormat)videoFormat);
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
        mCallback->doSendMsgCallback(mUserData, PLUGIN_MSG_DISPLAYED, (void *) buffer);
    }
}