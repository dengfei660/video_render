/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <errno.h>
#include <poll.h>
#include "videotunnel_plugin.h"
#include "videotunnel_impl.h"
#include "Logger.h"
#include "video_tunnel.h"
#include "videotunnel.h"

using namespace Tls;

#define TAG "rlib:videotunnel_impl"


VideoTunnelImpl::VideoTunnelImpl(VideoTunnelPlugin *plugin, int logcategory)
    : mPlugin(plugin),
    mLogCategory(logcategory)
{
    mFd = -1;
    mInstanceId = 0;
    mIsVideoTunnelConnected = false;
    mQueueFrameCnt = 0;
    mStarted = false;
    mRequestStop = false;
    mVideotunnelLib = NULL;
}

VideoTunnelImpl::~VideoTunnelImpl()
{
    if (mFd > 0) {
        if (mIsVideoTunnelConnected) {
            if (mVideotunnelLib && mVideotunnelLib->vtDisconnect) {
                mVideotunnelLib->vtDisconnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
            }
            mIsVideoTunnelConnected = false;
        }
        if (mInstanceId >= 0) {
            if (mVideotunnelLib && mVideotunnelLib->vtFreeId) {
                mVideotunnelLib->vtFreeId(mFd, mInstanceId);
            }
            mInstanceId = -1;
        }
        if (mVideotunnelLib && mVideotunnelLib->vtClose) {
            mVideotunnelLib->vtClose(mFd);
        }
        mFd = -1;
    }
}

bool VideoTunnelImpl::init()
{
    mVideotunnelLib = videotunnelLoadLib(mLogCategory);
    if (!mVideotunnelLib) {
        ERROR(mLogCategory,"videotunnelLoadLib load symbol fail");
        return false;
    }
    return true;
}

bool VideoTunnelImpl::release()
{
    if (mVideotunnelLib) {
        videotunnelUnloadLib(mLogCategory, mVideotunnelLib);
        mVideotunnelLib = NULL;
    }
    return true;
}

bool VideoTunnelImpl::connect()
{
    int ret;
    DEBUG(mLogCategory,"in");
    mRequestStop = false;
    if (mVideotunnelLib && mVideotunnelLib->vtOpen) {
        mFd = mVideotunnelLib->vtOpen();
    }
    if (mFd > 0) {
        //ret = meson_vt_alloc_id(mFd, &mInstanceId);
    }
    if (mFd > 0 && mInstanceId >= 0) {
        if (mVideotunnelLib && mVideotunnelLib->vtConnect) {
             ret = mVideotunnelLib->vtConnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
        }
        mIsVideoTunnelConnected = true;
    } else {
        ERROR(mLogCategory,"open videotunnel fail or alloc id fail");
        return false;
    }
    INFO(mLogCategory,"vt fd:%d, instance id:%d",mFd,mInstanceId);
    DEBUG(mLogCategory,"out");
    return true;
}

bool VideoTunnelImpl::disconnect()
{
    mRequestStop = true;
    DEBUG(mLogCategory,"in");
    if (isRunning()) {
        requestExitAndWait();
        mStarted = false;
    }

    if (mFd > 0) {
        if (mIsVideoTunnelConnected) {
            INFO(mLogCategory,"instance id:%d",mInstanceId);
            if (mVideotunnelLib && mVideotunnelLib->vtDisconnect) {
                mVideotunnelLib->vtDisconnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
            }
            mIsVideoTunnelConnected = false;
        }
        if (mInstanceId >= 0) {
            INFO(mLogCategory,"free instance id:%d",mInstanceId);
            //meson_vt_free_id(mFd, mInstanceId);
            mInstanceId = 0;
        }
        INFO(mLogCategory,"close vt fd:%d",mFd);
        if (mVideotunnelLib && mVideotunnelLib->vtClose) {
            mVideotunnelLib->vtClose(mFd);
        }
        mFd = -1;
    }

    DEBUG(mLogCategory,"out");
    return true;
}

bool VideoTunnelImpl::displayFrame(RenderBuffer *buf, int64_t displayTime)
{
    int ret;
    if (mStarted == false) {
        DEBUG(mLogCategory,"to run VideoTunnelImpl");
        run("VideoTunnelImpl");
        mStarted = true;
    }

    int fd0 = buf->dma.fd[0];
    //leng.fang suggest fence id set -1
    if (mVideotunnelLib && mVideotunnelLib->vtQueueBuffer) {
        ret = mVideotunnelLib->vtQueueBuffer(mFd, mInstanceId, fd0, -1 /*fence_fd*/, displayTime);
    }

    Tls::Mutex::Autolock _l(mMutex);
    std::pair<int64_t, RenderBuffer *> item(fd0, buf);
    mQueueRenderBufferMap.insert(item);
    ++mQueueFrameCnt;
    TRACE3(mLogCategory,"***fd:%d,w:%d,h:%d,displaytime:%lld,commitCnt:%d",buf->dma.fd[0],buf->dma.width,buf->dma.height,displayTime,mQueueFrameCnt);
    mPlugin->handleFrameDisplayed(buf);
    return true;
}

void VideoTunnelImpl::flush()
{
    DEBUG(mLogCategory,"flush");
    Tls::Mutex::Autolock _l(mMutex);
    if (mVideotunnelLib && mVideotunnelLib->vtCancelBuffer) {
        mVideotunnelLib->vtCancelBuffer(mFd, mInstanceId);
    }
    for (auto item = mQueueRenderBufferMap.begin(); item != mQueueRenderBufferMap.end(); ) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(item++);
        mPlugin->handleFrameDropped(renderbuffer);
        mPlugin->handleBufferRelease(renderbuffer);
    }
}

void VideoTunnelImpl::setVideotunnelId(int id)
{
    mInstanceId = id;
}

void VideoTunnelImpl::waitFence(int fence) {
    if (fence > 0) {
        struct pollfd pfd;
        pfd.fd = fence;
        pfd.events = POLLIN;
        pfd.revents = 0;

        for ( ; ; ) {
            int rc = poll(&pfd, 1, 3000);
            if ((rc == -1) && ((errno == EINTR) || (errno == EAGAIN))) {
                continue;
            } else if (rc <= 0) {
                if (rc == 0) errno = ETIME;
            }
            break;
        }
        close(fence);
    }
}

void VideoTunnelImpl::readyToRun()
{

}

bool VideoTunnelImpl::threadLoop()
{
    int ret;
    int bufferId = 0;
    int fenceId;
    RenderBuffer *buffer = NULL;

    if (mRequestStop) {
        DEBUG(mLogCategory,"request stop");
        return false;
    }

    if (mVideotunnelLib && mVideotunnelLib->vtDequeueBuffer) {
        ret = mVideotunnelLib->vtDequeueBuffer(mFd, mInstanceId, &bufferId, &fenceId);
    }

    if (ret != 0) {
        if (mRequestStop) {
            DEBUG(mLogCategory,"request stop");
            return false;
        }
        return true;
    }
    if (mRequestStop) {
        DEBUG(mLogCategory,"request stop");
        return false;
    }

    //send uvm fd to driver after getted fence
    waitFence(fenceId);

    auto item = mQueueRenderBufferMap.find(bufferId);
    if (item == mQueueRenderBufferMap.end()) {
        ERROR(mLogCategory,"Not found in mQueueRenderBufferMap bufferId:%d",bufferId);
        return true;
    }
    { // try locking when removing item from mQueueRenderBufferMap
        Tls::Mutex::Autolock _l(mMutex);
        buffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(bufferId);
        --mQueueFrameCnt;
        TRACE3(mLogCategory,"***dq buffer fd:%d,commitCnt:%d",bufferId,mQueueFrameCnt);
    }

    mPlugin->handleBufferRelease(buffer);
    return true;
}