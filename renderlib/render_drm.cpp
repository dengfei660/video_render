#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include "render_drm.h"
#include "Logger.h"

#define TAG "rlib:render_drm"

#define MESON_DRM_LIB_NAME "libdrm_meson.so"

RenderDrm::RenderDrm(int logCategory)
    : mLogCategory(logCategory)
{
    mUserData = NULL;
    mVsyncCallback = NULL;
    mLibHandle = NULL;
    mDrmFd = -1;
}

RenderDrm::~RenderDrm()
{
    TRACE2(mLogCategory,"desconstruct");
    if (mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void RenderDrm::setVsyncCallback(void *userdata, vsyncCallback callback)
{
    mUserData = userdata;
    mVsyncCallback = callback;
}

void RenderDrm::readyToRun()
{
    mLibHandle = dlopen(MESON_DRM_LIB_NAME, RTLD_NOW);
    if (mLibHandle == NULL) {
        ERROR(mLogCategory, "unable to dlopen %s : %s",MESON_DRM_LIB_NAME, dlerror());
        goto err_labal;
    }

    meson_drm_open = (drm_open)dlsym(mLibHandle, "meson_drm_open");
    if (meson_drm_open == NULL) {
        ERROR(mLogCategory,"dlsym meson_drm_open failed, err=%s \n", dlerror());
        goto err_labal;
    }

    meson_drm_get_vblank_time = (drm_get_vblank_time)dlsym(mLibHandle, "meson_drm_get_vblank_time");
    if (meson_drm_get_vblank_time == NULL) {
        ERROR(mLogCategory,"dlsym meson_drm_get_vblank_time failed, err=%s \n", dlerror());
        goto err_labal;
    }

    meson_drm_close_fd = (drm_close)dlsym(mLibHandle, "meson_drm_close_fd");
    if (meson_drm_close_fd == NULL) {
        ERROR(mLogCategory,"dlsym meson_drm_close_fd failed, err=%s \n", dlerror());
        goto err_labal;
    }

    mDrmFd = meson_drm_open();
    if (mDrmFd < 0) {
        ERROR(mLogCategory, "meson_drm_open fail ret : %d",mDrmFd);
        goto err_labal;
    }

    return;
err_labal:
    if (mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void RenderDrm::readyToExit()
{
    if (mDrmFd >= 0) {
        meson_drm_close_fd(mDrmFd);
        mDrmFd = -1;
    }

    if (mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

bool RenderDrm::threadLoop()
{
    int rc;
    int nextVsync = 1;
    uint64_t refreshInterval= 0LL;
    uint64_t vblankTime= 0LL;

    if (!mLibHandle || mDrmFd < 0) {
        ERROR(mLogCategory, "please check dlopen meson drm lib first");
        return false;
    }

     rc = meson_drm_get_vblank_time(mDrmFd, nextVsync, &vblankTime, &refreshInterval);

    TRACE3(mLogCategory, "vblanktime:%lld,refreshInterval:%lld",vblankTime, refreshInterval);

    //calculate next vblank time
    vblankTime += refreshInterval;
    //invoke callback
    if (mVsyncCallback) {
        mVsyncCallback(mUserData, vblankTime, refreshInterval);
    }
    return true;
}
