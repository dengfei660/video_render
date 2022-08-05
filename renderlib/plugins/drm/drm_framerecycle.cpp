/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <errno.h>
#include <sys/mman.h>
#include "Logger.h"
#include "drm_display.h"
#include "drm_framerecycle.h"

using namespace Tls;

#define TAG "rlib:drm_framerecycle"

DrmFrameRecycle::DrmFrameRecycle(DrmDisplay *drmDisplay, int logCategory)
{
    mDrmDisplay = drmDisplay;
    mLogCategory = logCategory;
    mStop = false;
    mWaitVideoFence = false;
    mQueue = new Tls::Queue();
}

DrmFrameRecycle::~DrmFrameRecycle()
{
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame recycle thread");
        requestExitAndWait();
    }

    if (mQueue) {
        mQueue->flush();
        delete mQueue;
        mQueue = NULL;
    }
}

bool DrmFrameRecycle::start()
{
    DEBUG(mLogCategory,"start frame recycle thread");
    run("frame recycle thread");
    return true;
}

bool DrmFrameRecycle::stop()
{
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame recycle thread");
        requestExitAndWait();
    }
    mWaitVideoFence = false;
    return true;
}

bool DrmFrameRecycle::recycleFrame(FrameEntity * frameEntity)
{
    Tls::Mutex::Autolock _l(mMutex);
    mQueue->push(frameEntity);
    TRACE1(mLogCategory,"queue cnt:%d",mQueue->getCnt());
    /* when two frame are posted, fence can be retrieved.
     * So waiting video fence starts
     */
    if (!mWaitVideoFence && mQueue->getCnt() > 2) {
        mWaitVideoFence = true;
    }
    return true;
}

bool DrmFrameRecycle::threadLoop()
{
    int rc;
    struct drm_buf* gem_buf;
    FrameEntity *frameEntity;

    //all frame had displayed and request stop,exist thread
    if (mStop && mQueue->getCnt() == 0) {
        return false;
    }
    if (!mWaitVideoFence) {
        usleep(8000);
        return true;
    }

    rc = mQueue->popAndWait((void**)&frameEntity);
    if (rc != Q_OK) {
        return true;
    }

    TRACE1(mLogCategory,"wait video fence for frame:%lld(pts:%lld)",frameEntity->displayTime,frameEntity->renderBuf->pts);
    rc = drm_waitvideoFence(frameEntity->drmBuf->fd[0]);
    if (rc <= 0) {
        WARNING(mLogCategory, "wait fence error %d", rc);
    }
    mDrmDisplay->handleDisplayedFrameEntity(frameEntity);
    mDrmDisplay->handleReleaseFrameEntity(frameEntity);
    return true;
}