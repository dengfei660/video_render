/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DRM_FRAME_RECYCLE_H__
#define __DRM_FRAME_RECYCLE_H__
#include "Mutex.h"
#include "Thread.h"
#include "Queue.h"

class DrmDisplay;
struct FrameEntity;

class DrmFrameRecycle : public Tls::Thread
{
  public:
    DrmFrameRecycle(DrmDisplay *drmDisplay, int logCategory);
    virtual ~DrmFrameRecycle();
    bool start();
    bool stop();
    bool recycleFrame(FrameEntity * frameEntity);
    //thread func
    virtual bool threadLoop();
  private:
    DrmDisplay *mDrmDisplay;
    int mLogCategory;

    bool mStop;
    bool mWaitVideoFence;
    mutable Tls::Mutex mMutex;
    Tls::Queue *mQueue;
};

#endif /*__DRM_FRAME_RECYCLE_H__*/