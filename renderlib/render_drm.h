#ifndef __RENDER_DRM_H__
#define __RENDER_DRM_H__
#include <stdint.h>
#include <stdlib.h>
#include "Thread.h"

typedef int (*drm_open)();
typedef int (*drm_get_vblank_time)(int drmFd, int nextVsync,uint64_t *vblankTime, uint64_t *refreshInterval);
typedef void (*drm_close)(int drmFd);

typedef void (*vsyncCallback)(void *userdata, uint64_t vsyncTime, uint64_t vsyncInterval);

class RenderDrm : public Tls::Thread {
public:
    RenderDrm(int logCategory);
    virtual ~RenderDrm();

    void setVsyncCallback(void *userdata, vsyncCallback callback);

    //thread func
    void readyToRun();
    void readyToExit();
    virtual bool threadLoop();
private:
    int mLogCategory;
    void *mUserData;
    vsyncCallback mVsyncCallback;

    void *mLibHandle;
    int mDrmFd;
    drm_open meson_drm_open;
    drm_get_vblank_time meson_drm_get_vblank_time;
    drm_close meson_drm_close_fd;
};
#endif /*__RENDER_DRM_H__*/