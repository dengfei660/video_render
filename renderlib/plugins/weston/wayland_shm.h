#ifndef __WAYLAND_SHM_H__
#define __WAYLAND_SHM_H__
#include <sys/types.h>
#include "render_lib.h"
#include "wayland_wlwrap.h"

class WaylandDisplay;

class WaylandShmBuffer : public WaylandWLWrap
{
  public:
    WaylandShmBuffer(WaylandDisplay *display, int logCategory);
    virtual ~WaylandShmBuffer();
    virtual struct wl_buffer *getWlBuffer() {
        return mWlBuffer;
    };
    virtual void *getDataPtr() {
        return mData;
    };
    virtual int getSize() {
        return mSize;
    };
    struct wl_buffer *constructWlBuffer(int width, int height, RenderVideoFormat format);
    int getWidth() {
        return mWidth;
    };
    int getHeight() {
        return mHeight;
    };
    RenderVideoFormat getFormat() {
        return mFormat;
    }
  private:
    int createAnonymousFile(off_t size);
    WaylandDisplay *mDisplay;
    struct wl_buffer *mWlBuffer;
    void *mData;
    int mStride;
    int mSize;
    int mWidth;
    int mHeight;
    RenderVideoFormat mFormat;

    int mLogCategory;
};

#endif /*__WAYLAND_SHM_H__*/