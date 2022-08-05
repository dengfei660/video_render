/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DRM_DISPLAY_WRAPPER_H__
#define __DRM_DISPLAY_WRAPPER_H__
#include <unordered_map>
#include <list>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>
#include "Mutex.h"
#include "Thread.h"
#include "Queue.h"
#include "drm_plugin.h"

extern "C" {
#include "meson_drm_util.h"
}

typedef struct FrameEntity
{
    RenderBuffer *renderBuf;
    int64_t displayTime;
    struct drm_buf *drmBuf;
} FrameEntity;

class DrmPlugin;
class DrmFramePost;
class DrmFrameRecycle;

class DrmDisplay
{
  public:
    DrmDisplay(DrmPlugin *plugin, int logCategory);
    virtual ~DrmDisplay();
    bool start(bool pip);
    bool stop();
    bool displayFrame(RenderBuffer *buf, int64_t displayTime);
    void flush();
    void pause();
    void resume();
    void setVideoFormat(RenderVideoFormat videoFormat);
    void setWindowSize(int x, int y, int w, int h);
    void setFrameSize(int width, int height);
    void showBlackFrame();
    struct drm_display *getDrmHandle() {
        return mDrmHandle;
    };

    /**
     * @brief handle frame that had posted to drm display,must commit this posted frame
     * to frame recycle service to wait frame display
     *
     * @param frameEntity frame resouce info
     */
    void handlePostedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle droped frame when two frames display time is in
     * one vsync duration
     *
     * @param frameEntity frame resouce info
     */
    void handleDropedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle displayed frame
     *
     * @param frameEntity
     */
    void handleDisplayedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle displayed frame and ready to release
     *
     * @param frameEntity
     */
    void handleReleaseFrameEntity(FrameEntity * frameEntity);
  private:
    FrameEntity *createFrameEntity(RenderBuffer *buf, int64_t displayTime);
    void destroyFrameEntity(FrameEntity * frameEntity);

    DrmPlugin *mPlugin;
    int mLogCategory;

    mutable Tls::Mutex mMutex;

    bool mIsPip;
    RenderVideoFormat mVideoFormat;
    PluginRect mWinRect;
    int mFrameWidth;
    int mFrameHeight;

    struct drm_display *mDrmHandle;
    struct drm_buf *mBlackFrame;
    void *mBlackFrameAddr;

    DrmFramePost *mDrmFramePost;
    DrmFrameRecycle *mDrmFrameRecycle;
};

#endif /*__DRM_DISPLAY_WRAPPER_H__*/