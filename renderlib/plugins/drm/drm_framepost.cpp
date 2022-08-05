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
#include "drm_framepost.h"

using namespace Tls;

#define TAG "rlib:drm_framepost"

DrmFramePost::DrmFramePost(DrmDisplay *drmDisplay,int logCategory)
{
    mDrmDisplay = drmDisplay;
    mLogCategory = logCategory;
    mPaused = false;
    mStop = false;
    mQueue = new Tls::Queue();
}

DrmFramePost::~DrmFramePost()
{
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame post thread");
        requestExitAndWait();
    }

    if (mQueue) {
        mQueue->flush();
        delete mQueue;
        mQueue = NULL;
    }
}

bool DrmFramePost::start()
{
    DEBUG(mLogCategory,"start frame post thread");
    run("frame post thread");
    return true;
}

bool DrmFramePost::stop()
{
    DEBUG(mLogCategory,"stop frame post thread");
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame post thread");
        requestExitAndWait();
    }
    flush();
    return true;
}

bool DrmFramePost::readyPostFrame(FrameEntity * frameEntity)
{
    Tls::Mutex::Autolock _l(mMutex);
    mQueue->push(frameEntity);
    TRACE1(mLogCategory,"queue cnt:%d",mQueue->getCnt());
    return true;
}

void DrmFramePost::flush()
{
    FrameEntity *entity;
    Tls::Mutex::Autolock _l(mMutex);
    while (mQueue->pop((void **)&entity) == Q_OK)
    {
        mDrmDisplay->handleDropedFrameEntity(entity);
        mDrmDisplay->handleReleaseFrameEntity(entity);
    }
}

void DrmFramePost::pause()
{
    mPaused = true;
}

void DrmFramePost::resume()
{
    mPaused = false;
}

bool DrmFramePost::threadLoop()
{
    int rc;
    drmVBlank vbl;
    int64_t vBlankTime;
    FrameEntity *curFrameEntity = NULL;
    FrameEntity *expiredFrameEntity = NULL;
    long long refreshInterval= 0LL;
    struct drm_display *drmHandle = mDrmDisplay->getDrmHandle();

    //stop thread
    if (mStop) {
        return false;
    }

    memset(&vbl, 0, sizeof(drmVBlank));

    vbl.request.type = DRM_VBLANK_RELATIVE;
    vbl.request.sequence = 1;
    vbl.request.signal = 0;

    rc = drmWaitVBlank(drmHandle->drm_fd, &vbl);
    if (rc) {
        ERROR(mLogCategory,"drmWaitVBlank error %d", rc);
        return false;
    }

    if (drmHandle->vrefresh)
    {
        refreshInterval= (1000000LL+(drmHandle->vrefresh/2))/drmHandle->vrefresh;
    }

    vBlankTime = vbl.reply.tval_sec*1000000LL + vbl.reply.tval_usec;
    vBlankTime += refreshInterval*3; //check next blank time and adjust 2 vsync duration delay

    Tls::Mutex::Autolock _l(mMutex);
    //if queue is empty or paused, loop next
    if (mQueue->isEmpty() || mPaused) {
        //TRACE2(mLogCategory,"empty or paused");
        goto tag_next;
    }

    while (mQueue->peek((void **)&curFrameEntity, 0) == Q_OK)
    {
        TRACE2(mLogCategory,"vblanktime:%lld,frame time:%lld(pts:%lld ms),refreshInterval:%lld",vBlankTime,curFrameEntity->displayTime,curFrameEntity->renderBuf->pts/1000000,refreshInterval);
        //no frame expired,loop next
        if (vBlankTime < curFrameEntity->displayTime) {
            break;
        }

        //pop the peeked frame
        mQueue->pop((void **)&curFrameEntity);

        //drop last expired frame,got a new expired frame
        if (expiredFrameEntity) {
            DEBUG(mLogCategory,"drop frame,vblanktime:%lld,frame time:%lld(pts:%lld ms)",vBlankTime,expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
            mDrmDisplay->handleDropedFrameEntity(expiredFrameEntity);
            mDrmDisplay->handleReleaseFrameEntity(expiredFrameEntity);
            expiredFrameEntity = NULL;
        }

        expiredFrameEntity = curFrameEntity;
        TRACE2(mLogCategory,"expire,frame time:%lld(pts:%lld ms)",expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
    }

    //no frame will be posted to drm display
    if (!expiredFrameEntity) {
        TRACE2(mLogCategory,"no frame expire");
        goto tag_next;
    }

    rc = drm_post_buf (drmHandle, expiredFrameEntity->drmBuf);
    if (rc) {
        ERROR(mLogCategory, "drm_post_buf error %d", rc);
        mDrmDisplay->handleDropedFrameEntity(expiredFrameEntity);
        mDrmDisplay->handleReleaseFrameEntity(expiredFrameEntity);
    }
    TRACE2(mLogCategory,"drm_post_buf,frame time:%lld(pts:%lld ms)",expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
    mDrmDisplay->handlePostedFrameEntity(expiredFrameEntity);
tag_next:
    return true;
}
