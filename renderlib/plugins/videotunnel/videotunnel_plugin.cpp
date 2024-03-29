/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "videotunnel_plugin.h"
#include "Logger.h"
#include "video_tunnel.h"

#define TAG "rlib:videotunnel_plugin"

VideoTunnelPlugin::VideoTunnelPlugin(int logCategory)
    : mDisplayLock("displaylock"),
    mLogCategory(logCategory),
    mRenderLock("renderlock"),
    mState(PLUGIN_STATE_IDLE)
{
    mVideoTunnel = new VideoTunnelImpl(this,logCategory);
}

VideoTunnelPlugin::~VideoTunnelPlugin()
{
    if (mVideoTunnel) {
        mVideoTunnel->disconnect();
        delete mVideoTunnel;
    }
}

void VideoTunnelPlugin::init()
{
    mState = PLUGIN_STATE_INITED;
    mVideoTunnel->init();
}

void VideoTunnelPlugin::release()
{
    mState = PLUGIN_STATE_IDLE;
    mVideoTunnel->release();
}

void VideoTunnelPlugin::setUserData(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int VideoTunnelPlugin::acquireDmaBuffer(int framewidth, int frameheight)
{
    return NO_ERROR;
}

int VideoTunnelPlugin::releaseDmaBuffer(int dmafd)
{
    return NO_ERROR;
}

int VideoTunnelPlugin::openDisplay()
{
    int ret;

    DEBUG(mLogCategory,"openDisplay");

    mState |= PLUGIN_STATE_DISPLAY_OPENED;
    DEBUG(mLogCategory,"openDisplay end");
    return ret;
}

int VideoTunnelPlugin::openWindow()
{
    int ret;

    DEBUG(mLogCategory,"openWindow");

    mVideoTunnel->connect();
    mState |= PLUGIN_STATE_WINDOW_OPENED;

    DEBUG(mLogCategory,"openWindow,end");
    return ret;
}

int VideoTunnelPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mVideoTunnel->displayFrame(buffer, displayTime);
    return NO_ERROR;
}

int VideoTunnelPlugin::flush()
{
    return NO_ERROR;
}

int VideoTunnelPlugin::pause()
{
    return NO_ERROR;
}
int VideoTunnelPlugin::resume()
{
    return NO_ERROR;
}

int VideoTunnelPlugin::closeDisplay()
{
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int VideoTunnelPlugin::closeWindow()
{
    int ret;
    mVideoTunnel->disconnect();
    mState &= ~PLUGIN_STATE_WINDOW_OPENED;
    return NO_ERROR;
}


int VideoTunnelPlugin::get(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_VIDEOTUNNEL_ID: {
            mVideoTunnel->getVideotunnelId((int *)value);
        } break;
    }

    return NO_ERROR;
}

int VideoTunnelPlugin::set(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            PluginRect* rect = static_cast<PluginRect*>(value);
            mWinRect.x = rect->x;
            mWinRect.y = rect->y;
            mWinRect.w = rect->w;
            mWinRect.h = rect->h;
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            PluginFrameSize * frameSize = static_cast<PluginFrameSize * >(value);
            mFrameWidth = frameSize->w;
            mFrameHeight = frameSize->h;
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int videoFormat = *(int *)(value);
            DEBUG(mLogCategory,"Set video format :%d",videoFormat);
        } break;
        case PLUGIN_KEY_VIDEOTUNNEL_ID: {
            int videotunnelId = *(int *)(value);
            DEBUG(mLogCategory,"Set videotunnel id :%d",videotunnelId);
            mVideoTunnel->setVideotunnelId(videotunnelId);
        } break;
    }
    return NO_ERROR;
}

int VideoTunnelPlugin::getState()
{
    return mState;
}

void VideoTunnelPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void VideoTunnelPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void VideoTunnelPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}