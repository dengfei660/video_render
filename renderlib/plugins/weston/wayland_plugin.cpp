/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "wayland_plugin.h"
#include "wayland_display.h"
#include "wayland_window.h"
#include "Logger.h"

#define TAG "rlib:wayland_plugin"

WaylandPlugin::WaylandPlugin(int logCatgory)
    : mDisplayLock("displaylock"),
    mRenderLock("renderlock"),
    mState(PLUGIN_STATE_IDLE),
    mFullscreen(true),
    mLogCategory(logCatgory)
{
    mDisplay = new WaylandDisplay(this, logCatgory);
    mWindow = new WaylandWindow(mDisplay, logCatgory);
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
    TRACE2(mLogCategory,"desconstruct");
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

    Tls::Mutex::Autolock _l(mRenderLock);
    DEBUG(mLogCategory,"openDisplay");
    ret =  mDisplay->openDisplay();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open display");
        if (mCallback) {
            char errString[] = "open wayland display fail";
            mCallback->doSendMsgCallback(mUserData, PLUGIN_MSG_DISPLAY_OPEN_FAIL,errString);
        }
        return ret;
    }
    mState |= PLUGIN_STATE_DISPLAY_OPENED;
    DEBUG(mLogCategory,"openDisplay end");
    return ret;
}

int WaylandPlugin::openWindow()
{
    int ret;

    Tls::Mutex::Autolock _l(mRenderLock);
    DEBUG(mLogCategory,"openWindow");
    /*open toplevel window*/
    ret = mWindow->openWindow(mFullscreen);
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open wayland window");
        if (mCallback) {
            char errString[] = "open wayland window fail";
            mCallback->doSendMsgCallback(mUserData, PLUGIN_MSG_WINDOW_OPEN_FAIL,errString);
        }
        return ret;
    }
    mState |= PLUGIN_STATE_WINDOW_OPENED;

    //run wl display queue dispatch
    DEBUG(mLogCategory,"To run wl display dispatch queue");
    mDisplay->run("display queue");
    DEBUG(mLogCategory,"openWindow,end");
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
    Tls::Mutex::Autolock _l(mRenderLock);
    mDisplay->closeDisplay();
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int WaylandPlugin::closeWindow()
{
    Tls::Mutex::Autolock _l(mRenderLock);
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
                WARNING(mLogCategory,"Window had not opened");
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
            DEBUG(mLogCategory,"Set video format :%d",videoFormat);
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
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::handDisplayOutputModeChanged(int width, int height, int refreshRate)
{
    INFO(mLogCategory, "current display mode, width:%d, height:%d,refreshRate:%d",width, height, refreshRate);
    if (mWindow) {
        mWindow->setRenderRectangle(0, 0, width, height);
    }
}