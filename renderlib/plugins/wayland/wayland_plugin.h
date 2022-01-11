#ifndef __WAYLAND_H__
#define __WAYLAND_H__
#include "render_plugin.h"
#include "wayland_display.h"
#include "wayland_window.h"
#include "wayland_buffer.h"

class WaylandPlugin : public RenderPlugin
{
  public:
    WaylandPlugin();
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
    virtual int closeDisplay();
    virtual int closeWindow();
    virtual int get(int key, void *value);
    virtual int set(int key, void *value);
    virtual int getState();
    //buffer release callback
    virtual void handleBufferRelease(RenderBuffer *buffer);
    //buffer had displayed ,but not release
    virtual void handleFrameDisplayed(RenderBuffer *buffer);
  private:
    PluginCallback *mCallback;
    WaylandDisplay *mDisplay;
    WaylandWindow *mWindow;
    PluginRect mWinRect;

    mutable Tls::Mutex mDisplayLock;
    mutable Tls::Mutex mRenderLock;
    int mFrameWidth;
    int mFrameHeight;
    bool mFullscreen; //default true value to full screen show video

    void *mUserData;
    int mState;
};


#endif /*__WAYLAND_H__*/