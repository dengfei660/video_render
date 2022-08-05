/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __VIDEO_TUNNEL_IMPLEMENT_H__
#define __VIDEO_TUNNEL_IMPLEMENT_H__
#include <unordered_map>
#include "Mutex.h"
#include "Thread.h"
#include "videotunnel_lib_wrap.h"

class VideoTunnelPlugin;

class VideoTunnelImpl : public Tls::Thread
{
  public:
    VideoTunnelImpl(VideoTunnelPlugin *plugin, int logCategory);
    virtual ~VideoTunnelImpl();
    bool init();
    bool release();
    bool connect();
    bool disconnect();
    bool displayFrame(RenderBuffer *buf, int64_t displayTime);
    void flush();
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
    VideotunnelLib *mVideotunnelLib;

    int mLogCategory;

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