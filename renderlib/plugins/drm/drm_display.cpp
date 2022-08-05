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
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>
#include <meson_drm.h>
#include "drm_display.h"
#include "Logger.h"
#include "drm_framepost.h"
#include "drm_framerecycle.h"

using namespace Tls;

#define TAG "rlib:drm_display"

#define BLACK_FRAME_WIDTH 64
#define BLACK_FRAME_HEIGHT 64

DrmDisplay::DrmDisplay(DrmPlugin *plugin, int logcategory)
    : mPlugin(plugin),
    mLogCategory(logcategory)
{
    mDrmHandle = NULL;
    mBlackFrame = 0;
    mIsPip = false;
    mDrmFramePost = NULL;
    mDrmFrameRecycle = NULL;
    mVideoFormat = VIDEO_FORMAT_UNKNOWN;
    mWinRect.x = 0;
    mWinRect.y = 0;
    mWinRect.w = 0;
    mWinRect.h = 0;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mBlackFrame = NULL;
    mBlackFrameAddr = NULL;
}

DrmDisplay::~DrmDisplay()
{
    if (mDrmHandle) {
        drm_destroy_display (mDrmHandle);
        mDrmHandle = NULL;
    }
}

bool DrmDisplay::start(bool pip)
{
    mIsPip = pip;

    DEBUG(mLogCategory, "start pip:%d",pip);
    mDrmHandle = drm_display_init();
    if (!mDrmHandle) {
        ERROR(mLogCategory, "drm display init failed");
        return false;
    }

    if (!mDrmFramePost) {
        mDrmFramePost = new DrmFramePost(this, mLogCategory);
        mDrmFramePost->start();
    }

    if (!mDrmFrameRecycle) {
        mDrmFrameRecycle = new DrmFrameRecycle(this,mLogCategory);
        mDrmFrameRecycle->start();
    }
    return true;
}

bool DrmDisplay::stop()
{
    if (mDrmFramePost) {
        DEBUG(mLogCategory, "stop frame post thread");
        mDrmFramePost->stop();
        delete mDrmFramePost;
        mDrmFramePost = NULL;
    }
    if (mDrmFrameRecycle) {
        DEBUG(mLogCategory, "stop frame recycle thread");
        mDrmFrameRecycle->stop();
        delete mDrmFrameRecycle;
        mDrmFrameRecycle = NULL;
    }
    if (mBlackFrameAddr) {
        munmap (mBlackFrameAddr, mBlackFrame->width * mBlackFrame->height * 2);
        mBlackFrameAddr = NULL;
    }
    if (mBlackFrame) {
        drm_free_buf(mBlackFrame);
        mBlackFrame = NULL;
    }
    if (mDrmHandle) {
        drm_destroy_display (mDrmHandle);
        mDrmHandle = NULL;
    }
    return true;
}

bool DrmDisplay::displayFrame(RenderBuffer *buf, int64_t displayTime)
{
    int ret;

    FrameEntity *frameEntity = createFrameEntity(buf, displayTime);
    if (frameEntity) {
        if (mDrmFramePost) {
            mDrmFramePost->readyPostFrame(frameEntity);
        } else {
            WARNING(mLogCategory,"no frame post service");
            handleDropedFrameEntity(frameEntity);
            handleReleaseFrameEntity(frameEntity);
        }
    }

    return true;
}

void DrmDisplay::flush()
{
    DEBUG(mLogCategory,"flush");
    Tls::Mutex::Autolock _l(mMutex);
    if (mDrmFramePost) {
        mDrmFramePost->flush();
    }
}

void DrmDisplay::pause()
{
    mDrmFramePost->pause();
}

void DrmDisplay::resume()
{
    mDrmFramePost->resume();
}

void DrmDisplay::setVideoFormat(RenderVideoFormat videoFormat)
{
    mVideoFormat = videoFormat;
    DEBUG(mLogCategory,"video format:%d",mVideoFormat);
}

void DrmDisplay::setWindowSize(int x, int y, int w, int h)
{
    mWinRect.x = x;
    mWinRect.y = y;
    mWinRect.w = w;
    mWinRect.h = h;
    DEBUG(mLogCategory,"window size:(%dx%dx%dx%d)",x, y,w,h);
}

void DrmDisplay::setFrameSize(int width, int height)
{
    mFrameWidth = width;
    mFrameHeight = height;
    DEBUG(mLogCategory,"frame size:%dx%d",width, height);
}

void DrmDisplay::showBlackFrame()
{
    int rc;
    struct drm_buf_metadata info;

    memset(&info, 0 , sizeof(struct drm_buf_metadata));

    /* use single planar for black frame */
    info.width = BLACK_FRAME_WIDTH;
    info.height = BLACK_FRAME_HEIGHT;
    info.flags = 0;
    info.fourcc = DRM_FORMAT_YUYV;

    if (!mIsPip)
        info.flags |= MESON_USE_VD1;
    else
        info.flags |= MESON_USE_VD2;

    mBlackFrame = drm_alloc_buf(mDrmHandle, &info);
    if (!mBlackFrame) {
        ERROR(mLogCategory,"Unable to alloc drm buf");
        goto tag_error;
    }

    mBlackFrameAddr = mmap (NULL, info.width * info.height * 2, PROT_WRITE,
        MAP_SHARED, mBlackFrame->fd[0], mBlackFrame->offsets[0]);
    if (mBlackFrameAddr == MAP_FAILED) {
        ERROR(mLogCategory,"mmap fail %d", errno);
        mBlackFrameAddr = NULL;
    }

    /* full screen black frame */
    memset (mBlackFrameAddr, 0, info.width * info.height * 2);
    mBlackFrame->crtc_x = 0;
    mBlackFrame->crtc_y = 0;
    mBlackFrame->crtc_w = -1;
    mBlackFrame->crtc_h = -1;

    //post black frame buf to drm
    rc = drm_post_buf (mDrmHandle, mBlackFrame);
    if (rc) {
        ERROR(mLogCategory, "post black frame to drm failed");
        goto tag_error;
    }

    return;
tag_error:
    if (mBlackFrameAddr) {
        munmap (mBlackFrameAddr, mBlackFrame->width * mBlackFrame->height * 2);
        mBlackFrameAddr = NULL;
    }
    if (mBlackFrame) {
        drm_free_buf(mBlackFrame);
        mBlackFrame = NULL;
    }
    return;
}


FrameEntity *DrmDisplay::createFrameEntity(RenderBuffer *buf, int64_t displayTime)
{
    struct drm_buf_import info;
    FrameEntity* frame = NULL;
    struct drm_buf * drmBuf = NULL;

    frame = (FrameEntity*)calloc(1, sizeof(FrameEntity));
    if (!frame) {
        ERROR(mLogCategory,"oom calloc FrameEntity mem failed");
        goto tag_error;
    }

    frame->displayTime = displayTime;
    frame->renderBuf = buf;

    memset(&info, 0 , sizeof(struct drm_buf_import));

    info.width = buf->dma.width;
    info.height = buf->dma.height;
    info.flags = 0;
    switch (mVideoFormat)
    {
        case VIDEO_FORMAT_NV21: {
            info.fourcc = DRM_FORMAT_NV21;
        } break;
        case VIDEO_FORMAT_NV12: {
            info.fourcc = DRM_FORMAT_NV12;
        } break;

        default: {
            info.fourcc = DRM_FORMAT_YUYV;
            WARNING(mLogCategory,"unknow video format, set to default YUYV format");
        }
    }

    if (!mIsPip) {
        info.flags |= MESON_USE_VD1;
    } else {
        info.flags |= MESON_USE_VD2;
    }

    for (int i = 0; i < buf->dma.planeCnt; i++) {
        info.fd[i] = buf->dma.fd[i];
    }

    drmBuf = drm_import_buf(mDrmHandle, &info);
    if (!drmBuf) {
        ERROR(mLogCategory, "unable drm_import_buf");
        goto tag_error;
    }

    drmBuf->src_x = 0;
    drmBuf->src_y = 0;
    drmBuf->src_w = buf->dma.width;
    drmBuf->src_h = buf->dma.height;

    //set window size
    if (mWinRect.w > 0 && mWinRect.h > 0) {
        drmBuf->crtc_x = mWinRect.x;
        drmBuf->crtc_y = mWinRect.y;
        drmBuf->crtc_w = mWinRect.w;
        drmBuf->crtc_h = mWinRect.h;
    } else { //not set window size ,full screen
        drmBuf->crtc_x = 0;
        drmBuf->crtc_y = 0;
        drmBuf->crtc_w = mDrmHandle->width;
        drmBuf->crtc_h = mDrmHandle->height;
    }

    frame->drmBuf = drmBuf;
    TRACE3(mLogCategory, "crtc(%d,%d,%d,%d),src(%d,%d,%d,%d)",drmBuf->crtc_x,drmBuf->crtc_y,drmBuf->crtc_w,drmBuf->crtc_h,
        drmBuf->src_x,drmBuf->src_y,drmBuf->src_w,drmBuf->src_h);

    return frame;
tag_error:
    if (frame) {
        free(frame);
    }
    return NULL;
}

void DrmDisplay::destroyFrameEntity(FrameEntity * frameEntity)
{
    int rc;

    if (!frameEntity) {
        return;
    }
    if (frameEntity->drmBuf) {
        rc = drm_free_buf(frameEntity->drmBuf);
        if (rc) {
            WARNING(mLogCategory, "drm_free_buf free %p failed",frameEntity->drmBuf);
        }
    }
    free(frameEntity);
}

void DrmDisplay::handlePostedFrameEntity(FrameEntity * frameEntity)
{
    if (mDrmFrameRecycle) {
        mDrmFrameRecycle->recycleFrame(frameEntity);
    }
}

void DrmDisplay::handleDropedFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleFrameDropped(frameEntity->renderBuf);
    }
}

void DrmDisplay::handleDisplayedFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleFrameDisplayed(frameEntity->renderBuf);
    }
}

void DrmDisplay::handleReleaseFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleBufferRelease(frameEntity->renderBuf);
    }
    destroyFrameEntity(frameEntity);
}