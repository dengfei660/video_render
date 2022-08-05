/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DRM_PLUGIN_H__
#define __DRM_PLUGIN_H__
#include "render_plugin.h"
#include "Mutex.h"

class DrmDisplay;

class DrmPlugin : public RenderPlugin
{
  public:
    DrmPlugin(int category);
    virtual ~DrmPlugin();
    virtual void init();
    virtual void release();
    void setUserData(void *userData, PluginCallback *callback);
    virtual int acquireDmaBuffer(int framewidth, int frameheight);
    virtual int releaseDmaBuffer(int dmafd);
    virtual int openDisplay();
    virtual int openWindow();
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime);
    virtual int flush();
    virtual int pause();
    virtual int resume();
    virtual int closeDisplay();
    virtual int closeWindow();
    virtual int get(int key, void *value);
    virtual int set(int key, void *value);
    virtual int getState();
    //buffer release callback
    virtual void handleBufferRelease(RenderBuffer *buffer);
    //buffer had displayed ,but not release
    virtual void handleFrameDisplayed(RenderBuffer *buffer);
    //buffer droped callback
    virtual void handleFrameDropped(RenderBuffer *buffer);
  private:
    PluginCallback *mCallback;
    DrmDisplay *mDrmDisplay;

    int mLogCategory;

    bool mIsPip;

    RenderVideoFormat mVideoFormat;

    void *mUserData;
    int mState;
};


#endif /*__VIDEO_TUNNEL_PLUGIN_H__*/