/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DRM_FRAME_POST_H__
#define __DRM_FRAME_POST_H__
#include "Mutex.h"
#include "Thread.h"
#include "Queue.h"

class DrmDisplay;
struct FrameEntity;

class DrmFramePost : public Tls::Thread
{
  public:
    DrmFramePost(DrmDisplay *drmDisplay,int logCategory);
    virtual ~DrmFramePost();
    bool start();
    bool stop();
    bool readyPostFrame(FrameEntity * frameEntity);
    void flush();
    void pause();
    void resume();
    //thread func
    virtual bool threadLoop();
  private:
    DrmDisplay *mDrmDisplay;
    int mLogCategory;

    bool mPaused;
    bool mStop;
    mutable Tls::Mutex mMutex;
    Tls::Queue *mQueue;
};

#endif /*__DRM_FRAME_POST_H__*/