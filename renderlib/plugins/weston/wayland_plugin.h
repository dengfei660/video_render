/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __WAYLAND_PLUGIN_H__
#define __WAYLAND_PLUGIN_H__
#include "render_plugin.h"
#include "wayland_display.h"
#include "wayland_window.h"
#include "wayland_buffer.h"

class WaylandPlugin : public RenderPlugin
{
  public:
    WaylandPlugin(int logCatgory);
    virtual ~WaylandPlugin();
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
    void handDisplayOutputModeChanged(int width, int height, int refreshRate);
  private:
    PluginCallback *mCallback;
    WaylandDisplay *mDisplay;
    WaylandWindow *mWindow;
    PluginRect mWinRect;

    int mLogCategory;

    mutable Tls::Mutex mDisplayLock;
    mutable Tls::Mutex mRenderLock;
    int mFrameWidth;
    int mFrameHeight;
    bool mFullscreen; //default true value to full screen show video

    void *mUserData;
    int mState;
};


#endif /*__WAYLAND_PLUGIN_H__*/