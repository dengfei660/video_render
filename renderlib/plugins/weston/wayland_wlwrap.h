#ifndef __WAYLAND_WLWRAP_H__
#define __WAYLAND_WLWRAP_H__

class WaylandWLWrap {
  public:
    WaylandWLWrap() {}
    virtual ~WaylandWLWrap() {};
    virtual struct wl_buffer *getWlBuffer() = 0;
    virtual void *getDataPtr() = 0;
    virtual int getSize() = 0;
};

#endif /*__WAYLAND_WLWRAP_H__*/