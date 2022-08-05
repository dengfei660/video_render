/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "drm_plugin.h"
#include "Logger.h"
#include "drm_display.h"

#define TAG "rlib:drm_plugin"

DrmPlugin::DrmPlugin(int logCategory)
    : mLogCategory(logCategory),
    mState(PLUGIN_STATE_IDLE)
{
    mVideoFormat = VIDEO_FORMAT_UNKNOWN;
    mIsPip = false;
    mDrmDisplay = new DrmDisplay(this,logCategory);
}

DrmPlugin::~DrmPlugin()
{
    if (mDrmDisplay) {
        delete mDrmDisplay;
        mDrmDisplay = NULL;
    }
}

void DrmPlugin::init()
{
    mState = PLUGIN_STATE_INITED;
}

void DrmPlugin::release()
{
    mState = PLUGIN_STATE_IDLE;
}

void DrmPlugin::setUserData(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int DrmPlugin::acquireDmaBuffer(int framewidth, int frameheight)
{
    return NO_ERROR;
}

int DrmPlugin::releaseDmaBuffer(int dmafd)
{
    return NO_ERROR;
}

int DrmPlugin::openDisplay()
{
    int ret;

    DEBUG(mLogCategory,"openDisplay");

    mState |= PLUGIN_STATE_DISPLAY_OPENED;
    DEBUG(mLogCategory,"openDisplay end");
    return ret;
}

int DrmPlugin::openWindow()
{
    int ret = NO_ERROR;

    DEBUG(mLogCategory,"openWindow");

    bool rc = mDrmDisplay->start(mIsPip);
    if (!rc) {
        ret = ERROR_OPEN_FAIL;
        ERROR(mLogCategory,"drm window open failed");
        return ret;
    }
    mState |= PLUGIN_STATE_WINDOW_OPENED;

    DEBUG(mLogCategory,"openWindow,end");
    return ret;
}

int DrmPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mDrmDisplay->displayFrame(buffer, displayTime);
    return NO_ERROR;
}

int DrmPlugin::flush()
{
    mDrmDisplay->flush();
    return NO_ERROR;
}

int DrmPlugin::pause()
{
    mDrmDisplay->pause();
    return NO_ERROR;
}
int DrmPlugin::resume()
{
    mDrmDisplay->resume();
    return NO_ERROR;
}

int DrmPlugin::closeDisplay()
{
    mState &= ~PLUGIN_STATE_DISPLAY_OPENED;

    return NO_ERROR;
}

int DrmPlugin::closeWindow()
{
    int ret;
    mDrmDisplay->stop();
    mState &= ~PLUGIN_STATE_WINDOW_OPENED;
    return NO_ERROR;
}


int DrmPlugin::get(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_VIDEO_FORMAT: {
            *(int *)value = mVideoFormat;
            TRACE1(mLogCategory,"get video format:%d",*(int *)value);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            *(int *)value = (mIsPip == true) ? 1 : 0;
        };
    }

    return NO_ERROR;
}

int DrmPlugin::set(int key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            PluginRect* rect = static_cast<PluginRect*>(value);
            if (mDrmDisplay) {
                mDrmDisplay->setWindowSize(rect->x, rect->y, rect->w, rect->h);
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            PluginFrameSize * frameSize = static_cast<PluginFrameSize * >(value);
            if (mDrmDisplay) {
                mDrmDisplay->setFrameSize(frameSize->w, frameSize->h);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int format = *(int *)(value);
            mVideoFormat = (RenderVideoFormat) format;
            DEBUG(mLogCategory,"Set video format :%d",mVideoFormat);
            if (mDrmDisplay) {
                mDrmDisplay->setVideoFormat(mVideoFormat);
            }
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            mIsPip = pip > 0? true:false;
        };
    }
    return NO_ERROR;
}

int DrmPlugin::getState()
{
    return mState;
}

void DrmPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void DrmPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void DrmPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}