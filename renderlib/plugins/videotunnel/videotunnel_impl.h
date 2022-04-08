#ifndef __VIDEO_TUNNEL_IMPLEMENT_H__
#define __VIDEO_TUNNEL_IMPLEMENT_H__
#include <unordered_map>
#include "Mutex.h"
#include "Thread.h"

class VideoTunnelPlugin;

class VideoTunnelImpl : public Tls::Thread
{
  public:
    VideoTunnelImpl(VideoTunnelPlugin *plugin);
    virtual ~VideoTunnelImpl();
    bool connect();
    bool disconnect();
    bool displayFrame(RenderBuffer *buf, int64_t displayTime);
    void setVideotunnelId(int id);
    void getVideotunnelId(int *id) {
        if (id) {
            *id = mInstanceId;
        }
    };
    //thread func
    virtual void readyToRun();
    virtual bool threadLoop();
  private:
    void waitFence(int fence);
    VideoTunnelPlugin *mPlugin;
    mutable Tls::Mutex mMutex;

    int mFd;
    int mInstanceId;
    bool mIsVideoTunnelConnected;
    bool mStarted;
    bool mRequestStop;

    //fd as key index,if has two fd,the key is fd0 << 32 | fd1
    std::unordered_map<int64_t, RenderBuffer *> mQueueRenderBufferMap;
    int mQueueFrameCnt;
};

#endif /*__VIDEO_TUNNEL_IMPLEMENT_H__*/