#ifndef __WAYLAND_WINDOW_H__
#define __WAYLAND_WINDOW_H__
#include <wayland-client-protocol.h>
#include <drm_fourcc.h>
#include "wayland_display.h"
#include "wayland_buffer.h"
#include "config.h"

using namespace Tls;

class WaylandDisplay;
class WaylandShmBuffer;
class WaylandBuffer;

class WaylandWindow
{
  public:
    WaylandWindow(WaylandDisplay *wlDisplay, int logCatgory);
    virtual ~WaylandWindow();
    int openWindow(bool fullscreen);
    int closeWindow();
    void ensureFullscreen(bool fullscreen);
    void setRenderRectangle(int x, int y, int w, int h);
    void setFrameSize(int w, int h);
    void setWindowSize(int x, int y, int w, int h);
    void displayFrameBuffer(RenderBuffer * buf, int64_t realDisplayTime);
    void resizeVideoSurface(bool commit);
    void setOpaque();
    void handleBufferReleaseCallback(WaylandBuffer *buf);
    void handleFrameDisplayedCallback(WaylandBuffer *buf);
    WaylandDisplay *getDisplay()
    {
        return mDisplay;
    };
    struct wl_surface *getVideoSurface()
    {
        return mVideoSurface;
    };
    void addWaylandBuffer(RenderBuffer * buf, WaylandBuffer *waylandbuf);
    WaylandBuffer* findWaylandBuffer(RenderBuffer * buf);
    void flushBuffers();
    bool isSupportReuseWlBuffer() {
        return mSupportReUseWlBuffer;
    };
    static void handleXdgToplevelClose (void *data, struct xdg_toplevel *xdg_toplevel);
    static void handleXdgToplevelConfigure (void *data, struct xdg_toplevel *xdg_toplevel,
                      int32_t width, int32_t height, struct wl_array *states);
    static void handleXdgSurfaceConfigure (void *data, struct xdg_surface *xdg_surface, uint32_t serial);
  private:
    struct Rectangle {
        int x;
        int y;
        int w;
        int h;
    };
    void new_window_common();
    void new_window_xdg_surface(bool fullscreen);
    void videoCenterRect(Rectangle src, Rectangle dst, Rectangle *result, bool scaling);
    void updateBorders();
    std::size_t calculateDmaBufferHash(RenderDmaBuffer &dmabuf);
    void cleanSurface();
    mutable Tls::Mutex mRenderMutex;
    WaylandDisplay *mDisplay;
    struct wl_surface *mAreaSurface;
    struct wl_surface *mAreaSurfaceWrapper;
    struct wl_surface *mVideoSurface;
    struct wl_surface *mVideoSurfaceWrapper;
    struct wl_subsurface *mVideoSubSurface;
    struct xdg_surface *mXdgSurface;
    struct xdg_toplevel *mXdgToplevel;
    struct wp_viewport *mAreaViewport;
    struct wp_viewport *mVideoViewport;
    WaylandShmBuffer *mAreaShmBuffer;
    bool mConfigured;
    Tls::Condition mConfigureCond;
    Tls::Mutex mConfigureMutex;

    bool mRedrawPending = false;

    /* the size and position of the area_(sub)surface
    it is full screen size now*/
    struct Rectangle mRenderRect;

    /*the size and position of window */
    struct Rectangle mWindowRect;

    /* the size and position of the video_subsurface */
    struct Rectangle mVideoRect;
    /* the size of the video in the buffers */
    int mVideoWidth;
    int mVideoHeight;

    //the count display buffer of committed to weston
    int mCommitCnt;

    std::unordered_map<std::size_t, WaylandBuffer *> mWaylandBuffersMap;
    bool mNoBorderUpdate;

    bool mSupportReUseWlBuffer;

    int mLogCategory;
};

#endif /*__WAYLAND_WINDOW_H__*/