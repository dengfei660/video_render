/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __SOCKET_SINK_H__
#define __SOCKET_SINK_H__
#include <mutex>
#include <list>
#include <string>
#include <sys/un.h>
#include <linux/netlink.h>
#include <string>
#include <unordered_map>
#include "Mutex.h"
#include "Poll.h"
#include "renderlib_wrap.h"
#include "sink.h"

#define MAX_SUN_PATH (80)

class RenderServer;
class SinkManager;

class SocketSink : public Sink , public Tls::Thread {
  public:
    SocketSink(SinkManager *sinkMgr, int socketfd, uint32_t vdecPort);
    virtual ~SocketSink();
    bool start();
    bool stop();
    void setVdecPort(uint32_t vdecPort) {
        mVdecPort = vdecPort;
    };
    void setVdoPort(uint32_t vdoPort) {
        mVdoPort = vdoPort;
    };
    void getSinkPort(uint32_t *vdecPort, uint32_t *vdoPort) {
        if (vdecPort) {
            *vdecPort = mVdecPort;
        }
        if (vdoPort) {
            *vdoPort = mVdoPort;
        }
    };
    bool isSocketSink() {
        return true;
    };
    States getState() {
        return mState;
    };

    //thread func
    void readyToRun();
    void readyToExit();
    virtual bool threadLoop();

    static void handleBufferRelease(void *userData, RenderBuffer *buffer);
    //buffer had displayed ,but not release
    static void handleFrameDisplayed(void *userData, RenderBuffer *buffer);
  private:
    void videoServerSendStatus(long long displayedFrameTime, int dropFrameCount, int bufIndex);
    void videoServerSendBufferRelease(int bufferId);
    int adaptFd(int fdin);
    bool processEvent();
    SinkManager *mSinkMgr;
    int mSocketFd;
    Tls::Mutex mMutex;

    uint32_t mVdoPort;
    uint32_t mVdecPort;

    States mState;

    int mFrameWidth;
    int mFrameHeight;
    int mFrameCnt;

    bool mIsPixFormatSet;
    bool mIsPeerSocketConnect;

    RenderLibWrap *mRenderlib;
};


#endif /*__SOCKET_SINK_H__*/