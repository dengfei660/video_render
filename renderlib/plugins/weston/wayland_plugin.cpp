#include "wayland_plugin.h"
#include "wayland_display.h"
#include "wayland_window.h"
#include "Logger.h"

#define TAG "rlib:wayland_plugin"

WaylandPlugin::WaylandPlugin()
    : mDisplayLock("displaylock"),
    mRenderLock("renderlock"),
    mState(PLUGIN_STATE_IDLE),
    mFullscreen(true)
{
    mDisplay = new WaylandDisplay(this);
    mWindow = new WaylandWindow(mDisplay);
}

WaylandPlugin::~WaylandPlugin()
{
    if (mDisplay) {
        delete mDisplay;
    }

    if (mWindow) {
        delete mWindow;
    }
    mState = PLUGIN_STATE_IDLE;
    TRACE2("desconstruct");
}

void WaylandPlugin::init()
{
    mState = PLUGIN_STATE_INITED;
}

void WaylandPlugin::release()
{
    mState = PLUGIN_STATE_IDLE;
}

void WaylandPlugin::setUserData(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int WaylandPlugin::acquireDmaBuffer(int framewidth, int frameheight)
{
    return NO_ERROR;
}

int WaylandPlugin::releaseDmaBuffer(int dmafd)
{
    return NO_ERROR;
}

int WaylandPlugin::openDisplay()
{
    int ret;

    DEBUG("openDisplay");
    ret =  mDisplay->openDisplay();
    if (ret != NO_ERROR) {
        ERROR("Error open display");
        if (mCallback) {
            std::string errString = "open wayland display fail";
            mCallback->doSendErrorCallback(mUserData, PLUGIN_ERROR_DISPLAY_OPEN_FAIL,errString.c_str());
        }
        return ret;
    }
    mState |= PLUGIN_STATE_DISPLAY_OPENED;
    DEBUG("openDisplay end");
    return ret;
}

int WaylandPlugin::openWindow()
{
    int ret;

    DEBUG("openWindow");
    /*open toplevel window*/
    ret = mWindow->openWindow(mFullscreen);
    if (ret != NO_ERROR) {
        ERROR("Error open wayland window");
        if (mCallback) {
            std::string errString = "open wayland window fail";
            mCallback->doSendErrorCallback(mUserData, PLUGIN_ERROR_WINDOW_OPEN_FAIL,errString.c_str());
        }
        return ret;
    }
    mState |= PLUGIN_STATE_WINDOW_OPENED;

    //run wl display queue dispatch
    DEBUG("To run wl display dispatch queue");
    mDisplay->run("display queue");
    DEBUG("openWindow,end");
    return ret;
}

int WaylandPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mWindow->displayFrameBuffer(buffer, displayTime);
    return NO_ERROR;
}

int WaylandPlugin::flush()
{
    mWindow->flushBuffers();
    return NO_ERROR;
}

int WaylandPlugin::pause()
{
    return NO_ERROR;
}
int WaylandPlugin::resume()
{
    return NO_ERROR;
}

int WaylandPlugin::closeDisplay()
{
    mDisplay->closeDisplay();
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int WaylandPlugin::closeWindow()
{
    mWindow->closeWindow();
    mState &= ~PLUGIN_STATE_WINDOW_OPENED;
    return NO_ERROR;
}


int WaylandPlugin::get(int key, void *value)
{
    return NO_ERROR;
}

int WaylandPlugin::set(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            PluginRect* rect = static_cast<PluginRect*>(value);
            mWinRect.x = rect->x;
            mWinRect.y = rect->y;
            mWinRect.w = rect->w;
            mWinRect.h = rect->h;
            if (mWindow && (mState & PLUGIN_STATE_WINDOW_OPENED)) {
                mWindow->setWindowSize(mWinRect.x, mWinRect.y, mWinRect.w, mWinRect.h);
            } else {
                WARNING("Window had not opened");
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            PluginFrameSize * frameSize = static_cast<PluginFrameSize * >(value);
            mFrameWidth = frameSize->w;
            mFrameHeight = frameSize->h;
            if (mWindow) {
                mWindow->setFrameSize(mFrameWidth, mFrameHeight);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int videoFormat = *(int *)(value);
            DEBUG("Set video format :%d",videoFormat);
            mDisplay->setVideoBufferFormat((RenderVideoFormat)videoFormat);
        } break;
    }
    return NO_ERROR;
}

int WaylandPlugin::getState()
{
    return mState;
}

void WaylandPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doSendMsgCallback(mUserData, PLUGIN_MSG_FRAME_DROPED, (void *)buffer);
    }
}