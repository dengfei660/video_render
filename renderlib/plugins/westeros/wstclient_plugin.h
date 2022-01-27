#ifndef __WST_CLIENT_H__
#define __WST_CLIENT_H__
#include "render_plugin.h"
#include "wstclient_wayland.h"
#include "Mutex.h"

class WstClientPlugin : public RenderPlugin
{
  public:
    WstClientPlugin();
    virtual ~WstClientPlugin();
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
  private:
    PluginCallback *mCallback;
    WstClientWayland *mWayland;
    PluginRect mWinRect;

    mutable Tls::Mutex mDisplayLock;
    mutable Tls::Mutex mRenderLock;
    bool mFullscreen; //default true value to full screen show video

    void *mUserData;
    int mState;
};


#endif /*__WST_CLIENT_H__*/