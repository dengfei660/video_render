#ifndef __VIDEO_TUNNEL_PLUGIN_H__
#define __VIDEO_TUNNEL_PLUGIN_H__
#include "render_plugin.h"
#include "videotunnel_impl.h"
#include "Mutex.h"

class VideoTunnelPlugin : public RenderPlugin
{
  public:
    VideoTunnelPlugin(int category);
    virtual ~VideoTunnelPlugin();
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
    VideoTunnelImpl *mVideoTunnel;
    PluginRect mWinRect;

    int mLogCategory;

    mutable Tls::Mutex mDisplayLock;
    mutable Tls::Mutex mRenderLock;
    int mFrameWidth;
    int mFrameHeight;

    void *mUserData;
    int mState;
};


#endif /*__VIDEO_TUNNEL_PLUGIN_H__*/