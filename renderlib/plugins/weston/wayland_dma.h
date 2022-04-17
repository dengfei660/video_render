#ifndef __WAYLAND_DMA_H__
#define __WAYLAND_DMA_H__
#include "render_lib.h"
#include "Mutex.h"
#include "Condition.h"
#include "wayland_wlwrap.h"
#include "render_lib.h"

class WaylandDisplay;

class WaylandDmaBuffer : public WaylandWLWrap
{
  public:
    WaylandDmaBuffer(WaylandDisplay *display, int logCategory);
    virtual ~WaylandDmaBuffer();
    virtual struct wl_buffer *getWlBuffer() {
        return mWlBuffer;
    };
    virtual void *getDataPtr() {
        return mData;
    };
    virtual int getSize() {
        return mSize;
    };
    struct wl_buffer *constructWlBuffer(RenderDmaBuffer *dmabuf, RenderVideoFormat format);
    static void dmabufCreateSuccess(void *data,
            struct zwp_linux_buffer_params_v1 *params,
            struct wl_buffer *new_buffer);
    static void dmabufCreateFail(void *data,
            struct zwp_linux_buffer_params_v1 *params);
  private:
    WaylandDisplay *mDisplay;
    RenderDmaBuffer mRenderDmaBuffer;
    struct wl_buffer *mWlBuffer;
    mutable Tls::Mutex mMutex;
    Tls::Condition mCondition;
    void *mData;
    int mSize;

    int mLogCategory;
};
#endif /*__WAYLAND_DMA_H__*/