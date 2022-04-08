#include <string.h>
#include "wayland_window.h"
#include "wayland_display.h"
#include "wayland_plugin.h"
#include "ErrorCode.h"
#include "Logger.h"
#include "wayland_shm.h"
#include "wayland_dma.h"

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define TAG "rlib:wayland_window"

void WaylandWindow::handleXdgToplevelClose (void *data, struct xdg_toplevel *xdg_toplevel)
{
    WaylandWindow *window = static_cast<WaylandWindow *>(data);

    INFO("XDG toplevel got a close event.");
}

void WaylandWindow::handleXdgToplevelConfigure (void *data, struct xdg_toplevel *xdg_toplevel,
            int32_t width, int32_t height, struct wl_array *states)
{
    WaylandWindow *window = static_cast<WaylandWindow *>(data);
    uint32_t *state;

    INFO ("XDG toplevel got a configure event, width:height [ %d, %d ].", width, height);
    /*
    wl_array_for_each (state, states) {
        switch (*state) {
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
        case XDG_TOPLEVEL_STATE_RESIZING:
        case XDG_TOPLEVEL_STATE_ACTIVATED:
            break;
        }
    }
    */

    if (width <= 0 || height <= 0)
        return;

    window->setRenderRectangle(0, 0, width, height);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    WaylandWindow::handleXdgToplevelConfigure,
    WaylandWindow::handleXdgToplevelClose,
};

void WaylandWindow::handleXdgSurfaceConfigure (void *data, struct xdg_surface *xdg_surface,
    uint32_t serial)
{
    WaylandWindow *window = static_cast<WaylandWindow *>(data);
    xdg_surface_ack_configure (xdg_surface, serial);

    TRACE1("handleXdgSurfaceConfigure");
    Tls::Mutex::Autolock _l(window->mConfigureMutex);
    window->mConfigured = true;
    //window->mConfigureCond.signal();
}

static const struct xdg_surface_listener xdg_surface_listener = {
    WaylandWindow::handleXdgSurfaceConfigure,
};

WaylandWindow::WaylandWindow(WaylandDisplay *wlDisplay)
    :mDisplay(wlDisplay),
    mRenderMutex("renderMutex"),
    mConfigureMutex("configMutex")
{
    mVideoWidth = 0;
    mVideoHeight = 0;
    mVideoSurface = NULL;
    mXdgSurface = NULL;
    mXdgToplevel = NULL;
    mAreaViewport = NULL;
    mVideoViewport = NULL;
    mNoBorderUpdate = false;
    mAreaShmBuffer = NULL;
    mCommitCnt = 0;
    memset(&mRenderRect, 0, sizeof(struct Rectangle));
    memset(&mVideoRect, 0, sizeof(struct Rectangle));
    memset(&mWindowRect, 0, sizeof(struct Rectangle));
    mSupportReUseWlBuffer = false;
    char *env = getenv("VIDEO_RENDER_REUSE_WESTON_WLBUFFER");
    if (env) {
        int limit = atoi(env);
        if (limit > 0) {
            mSupportReUseWlBuffer = true;
            INFO("set reuse wl_buffer");
        }
    }
}
WaylandWindow::~WaylandWindow()
{

}

void WaylandWindow::new_window_common()
{
    struct wl_region *region;

    mAreaSurface = wl_compositor_create_surface (mDisplay->getCompositor());
    mVideoSurface = wl_compositor_create_surface (mDisplay->getCompositor());
    mAreaSurfaceWrapper = (struct wl_surface *)wl_proxy_create_wrapper (mAreaSurface);
    mVideoSurfaceWrapper = (struct wl_surface *)wl_proxy_create_wrapper (mVideoSurface);

    wl_proxy_set_queue ((struct wl_proxy *) mAreaSurfaceWrapper, mDisplay->getWlEventQueue());
    wl_proxy_set_queue ((struct wl_proxy *) mVideoSurfaceWrapper, mDisplay->getWlEventQueue());

    /* embed video_surface in area_surface */
    mVideoSubSurface = wl_subcompositor_get_subsurface (mDisplay->getSubCompositor(),
            mVideoSurface, mAreaSurface);
    wl_subsurface_set_desync (mVideoSubSurface);

    if (mDisplay->getViewporter()) {
        mAreaViewport = wp_viewporter_get_viewport (mDisplay->getViewporter(), mAreaSurface);
        mVideoViewport = wp_viewporter_get_viewport (mDisplay->getViewporter(), mVideoSurface);
    }

    /* do not accept input */
    region = wl_compositor_create_region (mDisplay->getCompositor());
    wl_surface_set_input_region (mAreaSurface, region);
    wl_region_destroy (region);

    region = wl_compositor_create_region (mDisplay->getCompositor());
    wl_surface_set_input_region (mVideoSurface, region);
    wl_region_destroy (region);
}

void WaylandWindow::new_window_xdg_surface(bool fullscreen)
{
    DEBUG("fullscreen:%d",fullscreen);
    /* Check which protocol we will use (in order of preference) */
    if (mDisplay->getXdgWmBase()) {

        /* First create the XDG surface */
        mXdgSurface= xdg_wm_base_get_xdg_surface (mDisplay->getXdgWmBase(), mAreaSurface);
        if (!mXdgSurface) {
            ERROR ("Unable to get xdg_surface");
            return;
        }
        xdg_surface_add_listener (mXdgSurface, &xdg_surface_listener,(void *)this);

        /* Then the toplevel */
        mXdgToplevel= xdg_surface_get_toplevel (mXdgSurface);
        if (!mXdgSurface) {
            ERROR ("Unable to get xdg_toplevel");
            return;
        }
        xdg_toplevel_add_listener (mXdgToplevel, &xdg_toplevel_listener, this);

        /* Finally, commit the xdg_surface state as toplevel */
        mConfigured = false;
        wl_surface_commit (mAreaSurface);
        wl_display_flush (mDisplay->getWlDisplay());
        /* we need exactly 2 roundtrips to discover global objects and their state */
        for (int i = 0; i < 2; i++) {
            if (wl_display_roundtrip_queue(mDisplay->getWlDisplay(), mDisplay->getWlEventQueue()) < 0) {
                FATAL("Error communicating with the wayland display");
            }
        }

        if (mConfigured) {
            INFO("xdg surface had configured");
        } else {
            WARNING("xdg surface not configured");
        }

        //full xdg surface to detect output window size
        //window size will be taken by handleXdgToplevelConfigure callback
        ensureFullscreen(fullscreen);
    } else {
        ERROR ("Unable to use xdg_wm_base ");
        return;
    }

    TRACE1("new_window_toplevel_surface, end");
}

int WaylandWindow::openWindow(bool fullscreen)
{
    DEBUG("openWindow");
    new_window_common();
    new_window_xdg_surface(fullscreen);
    DEBUG("openWindow, end");
    return NO_ERROR;
}

int WaylandWindow::closeWindow()
{
    //cleanSurface();

    if (mAreaShmBuffer) {
        delete mAreaShmBuffer;
        mAreaShmBuffer = NULL;
    }

    //free all obtain buff
    for (auto item = mWaylandBuffersMap.begin(); item != mWaylandBuffersMap.end(); ) {
        WaylandBuffer *waylandbuf = (WaylandBuffer*)item->second;
        mWaylandBuffersMap.erase(item++);
        delete waylandbuf;
    }

    if (mXdgToplevel) {
        xdg_toplevel_destroy (mXdgToplevel);
        mXdgToplevel = NULL;
    }

    if (mXdgSurface) {
        xdg_surface_destroy (mXdgSurface);
        mXdgSurface = NULL;
    }

    if (mVideoSurfaceWrapper) {
        wl_proxy_wrapper_destroy (mVideoSurfaceWrapper);
        mVideoSurfaceWrapper = NULL;
    }

    if (mVideoSubSurface) {
        wl_subsurface_destroy (mVideoSubSurface);
        mVideoSubSurface = NULL;
    }

    if (mVideoSurface) {
        wl_surface_destroy (mVideoSurface);
        mVideoSurface = NULL;
    }

    if (mAreaSurfaceWrapper) {
        wl_proxy_wrapper_destroy (mAreaSurfaceWrapper);
        mAreaSurfaceWrapper = NULL;
    }

    if (mAreaSurface) {
        wl_surface_destroy (mAreaSurface);
        mAreaSurface = NULL;
    }

    return NO_ERROR;
}

void WaylandWindow::ensureFullscreen(bool fullscreen)
{
    if (mDisplay->getXdgWmBase()) {
        if (fullscreen) {
            xdg_toplevel_set_fullscreen (mXdgToplevel, NULL);
        } else {
            xdg_toplevel_unset_fullscreen (mXdgToplevel);
        }
    }
}

void WaylandWindow::setRenderRectangle(int x, int y, int w, int h)
{
    DEBUG("set render rect:x:%d,y:%d,w:%d,h:%d",x,y,w,h);

    mRenderRect.x = x;
    mRenderRect.y = y;
    mRenderRect.w = w;
    mRenderRect.h = h;

    if (mAreaViewport) {
        wp_viewport_set_destination (mAreaViewport, w, h);
    }

    updateBorders();

    if (!mConfigured) {
        WARNING("Not configured xdg");
        return;
    }

    if (mVideoWidth != 0 && mVideoSurface) {
        wl_subsurface_set_sync (mVideoSubSurface);
        resizeVideoSurface(true);
    }

    wl_surface_damage (mAreaSurfaceWrapper, 0, 0, w, h);
    wl_surface_commit (mAreaSurfaceWrapper);

    if (mVideoWidth != 0) {
        wl_subsurface_set_desync (mVideoSubSurface);
    }
}

void WaylandWindow::setFrameSize(int w, int h)
{
    mVideoWidth = w;
    mVideoHeight = h;
    TRACE1("frame w:%d,h:%d",mVideoWidth,mVideoHeight);
    if (mRenderRect.w > 0 && mVideoSurface) {
        resizeVideoSurface(true);
    }
}

void WaylandWindow::setWindowSize(int x, int y, int w, int h)
{
    mWindowRect.x = x;
    mWindowRect.y = y;
    mWindowRect.w = w;
    mWindowRect.h = h;
    TRACE1("window size:x:%d,y:%d,w:%d,h:%d",mWindowRect.x,mWindowRect.y,mWindowRect.w,mWindowRect.h);
    if (mWindowRect.w > 0 && mVideoWidth > 0 && mVideoSurface) {
        resizeVideoSurface(true);
    }
}

void WaylandWindow::resizeVideoSurface(bool commit)
{
    Rectangle src = {0,};
    Rectangle dst = {0,};
    Rectangle res;

    /* center the video_subsurface inside area_subsurface */
    src.w = mVideoWidth;
    src.h = mVideoHeight;
    /*if had set the window size, we will scall
    video surface to this window size*/
    if (mWindowRect.w > 0 && mWindowRect.h > 0) {
        dst.x = mWindowRect.x;
        dst.y = mWindowRect.y;
        dst.w = mWindowRect.w;
        dst.h = mWindowRect.h;
        if (mWindowRect.w > mRenderRect.w && mWindowRect.h > mRenderRect.h) {
            WARNING("Error window size:%dx%d, but render size:%dx%d,reset to render size",
                mWindowRect.w,mWindowRect.h,mRenderRect.w,mRenderRect.h);
            dst.x = mRenderRect.x;
            dst.y = mRenderRect.y;
            dst.w = mRenderRect.w;
            dst.h = mRenderRect.h;
        }
        //to do,we need set geometry?
        //if (mXdgSurface) {
        //    xdg_surface_set_window_geometry(mXdgSurface, mWindowRect.x, mWindowRect.y, mWindowRect.w, mWindowRect.h);
        //}
    } else { //scal video to full screen
        dst.w = mRenderRect.w;
        dst.h = mRenderRect.h;
    }

    if (mDisplay->getViewporter()) {
        videoCenterRect(src, dst, &res, true);
    } else {
        videoCenterRect(src, dst, &res, false);
    }

    wl_subsurface_set_position (mVideoSubSurface, res.x, res.y);

    if (commit) {
        wl_surface_damage (mVideoSurfaceWrapper, 0, 0, res.w, res.h);
        wl_surface_commit (mVideoSurfaceWrapper);
    }

    //top level setting
    if (mXdgToplevel) {
        struct wl_region *region;

        region = wl_compositor_create_region (mDisplay->getCompositor());
        wl_region_add (region, 0, 0, mRenderRect.w, mRenderRect.h);
        wl_surface_set_input_region (mAreaSurface, region);
        wl_region_destroy (region);
    }

    /* this is saved for use in wl_surface_damage */
    mVideoRect.x = res.x;
    mVideoRect.y = res.y;
    mVideoRect.w = res.w;
    mVideoRect.h = res.h;

    //to scale video surface to full screen
    wp_viewport_set_destination(mVideoViewport, res.w, res.h);
    TRACE1("video rectangle,x:%d,y:%d,w:%d,h:%d",mVideoRect.x, mVideoRect.y, mVideoRect.w, mVideoRect.h);
}

void WaylandWindow::setOpaque()
{
    struct wl_region *region;

    /* Set area opaque */
    region = wl_compositor_create_region (mDisplay->getCompositor());
    wl_region_add (region, 0, 0, mRenderRect.w, mRenderRect.h);
    wl_surface_set_opaque_region (mAreaSurface, region);
    wl_region_destroy (region);
}

void WaylandWindow::displayFrameBuffer(RenderBuffer * buf, int64_t realDisplayTime)
{
    WaylandBuffer *waylandBuf = NULL;
    struct wl_buffer * wlbuffer = NULL;
    int ret;

    if (!buf) {
        ERROR("Error input params, waylandbuffer is null");
        return;
    }

    TRACE1("display renderBuffer:%p,PTS:%lld,realtime:%lld",buf, buf->pts, realDisplayTime);

    if (buf->flag & BUFFER_FLAG_ALLOCATE_DMA_BUFFER ||
        buf->flag & BUFFER_FLAG_EXTER_DMA_BUFFER) {
        if (buf->dma.width <=0 || buf->dma.height <=0) {
            buf->dma.width = mVideoWidth;
            buf->dma.height = mVideoHeight;
        }
        if (mSupportReUseWlBuffer) {
            waylandBuf = findWaylandBuffer(buf);
            if (waylandBuf == NULL) {
                waylandBuf = new WaylandBuffer(mDisplay, this);
                waylandBuf->setRenderRealTime(realDisplayTime);
                waylandBuf->setBufferFormat(mDisplay->getVideoBufferFormat());
                ret = waylandBuf->constructWlBuffer(buf);
                if (ret != NO_ERROR) {
                    WARNING("dmabufConstructWlBuffer fail,release waylandbuf");
                    //delete waylanBuf,WaylandBuffer object destruct will call release callback
                    goto waylandbuf_fail;
                } else {
                    addWaylandBuffer(buf, waylandBuf);
                }
            } else {
                waylandBuf->setRenderRealTime(realDisplayTime);
                ret = waylandBuf->constructWlBuffer(buf);
                if (ret != NO_ERROR) {
                    WARNING("dmabufConstructWlBuffer fail,release waylandbuf");
                    //delete waylanBuf,WaylandBuffer object destruct will call release callback
                    goto waylandbuf_fail;
                }
            }
        } else {
            waylandBuf = new WaylandBuffer(mDisplay, this);
            waylandBuf->setRenderRealTime(realDisplayTime);
            waylandBuf->setBufferFormat(mDisplay->getVideoBufferFormat());
            ret = waylandBuf->constructWlBuffer(buf);
            if (ret != NO_ERROR) {
                WARNING("dmabufConstructWlBuffer fail,release waylandbuf");
                goto waylandbuf_fail;
            }
        }
    }

    if (waylandBuf) {
        wlbuffer = waylandBuf->getWlBuffer();
    }
    if (wlbuffer) {
        Tls::Mutex::Autolock _l(mRenderMutex);
        ++mCommitCnt;
        uint32_t hiPts = realDisplayTime >> 32;
        uint32_t lowPts = realDisplayTime & 0xFFFFFFFF;
        //attach this wl_buffer to weston
        TRACE2("++attach,renderbuf:%p,wl_buffer:%p,commitCnt:%d",buf,wlbuffer,mCommitCnt);
        waylandBuf->attach(mVideoSurfaceWrapper);
        TRACE1("display time:%lld,hiPts:%d,lowPts:%d",realDisplayTime, hiPts, lowPts);
        wl_surface_set_pts(mVideoSurfaceWrapper, hiPts, lowPts);
        //wl_surface_attach (mVideoSurfaceWrapper, wlbuffer, 0, 0);
        wl_surface_damage (mVideoSurfaceWrapper, 0, 0, mVideoRect.w, mVideoRect.h);
        wl_surface_commit (mVideoSurfaceWrapper);
    } else {
        WARNING("wlbuffer is NULL");
        /* clear both video and parent surfaces */
        cleanSurface();
    }

    wl_display_flush (mDisplay->getWlDisplay());

    /*after commiting wl_buffer to weston,
    notify this buffer had displayed,this is used to count
    displayed frames*/
    if (waylandBuf) {
        handleFrameDisplayedCallback(waylandBuf);
    }

    return;
waylandbuf_fail:
    WaylandPlugin *plugin = mDisplay->getPlugin();
    //notify dropped
    plugin->handleFrameDropped(buf);
    //notify display that app can count displayed frames
    plugin->handleFrameDisplayed(buf);
    //notify app release this buf
    plugin->handleBufferRelease(buf);
    //delete waylandbuf
    delete waylandBuf;
    waylandBuf = NULL;
    return;
}

void WaylandWindow::handleBufferReleaseCallback(WaylandBuffer *buf)
{
    {
        Tls::Mutex::Autolock _l(mRenderMutex);
        --mCommitCnt;
    }
    RenderBuffer *renderBuffer = buf->getRenderBuffer();
    TRACE1("handle release renderBuffer :%p,priv:%p,PTS:%lld,realtime:%lld,commitCnt:%d",renderBuffer,renderBuffer->priv,renderBuffer->pts,buf->getRenderRealTime(),mCommitCnt);
    WaylandPlugin *plugin = mDisplay->getPlugin();
    plugin->handleBufferRelease(renderBuffer);
}

void WaylandWindow::handleFrameDisplayedCallback(WaylandBuffer *buf)
{
    RenderBuffer *renderBuffer = buf->getRenderBuffer();
    TRACE2("handle displayed renderBuffer :%p,PTS:%lld,realtime:%lld",renderBuffer,renderBuffer->pts,buf->getRenderRealTime());
    WaylandPlugin *plugin = mDisplay->getPlugin();
    plugin->handleFrameDisplayed(renderBuffer);
}

void WaylandWindow::videoCenterRect(Rectangle src, Rectangle dst, Rectangle *result, bool scaling)
{
    if (!scaling) {
        result->w = MIN (src.w, dst.w);
        result->h = MIN (src.h, dst.h);
        result->x = dst.x + (dst.w - result->w) / 2;
        result->y = dst.y + (dst.h - result->h) / 2;
    } else {
        double src_ratio, dst_ratio;

        src_ratio = (double) src.w / src.h;
        dst_ratio = (double) dst.w / dst.h;

        if (src_ratio > dst_ratio) {
            result->w = dst.w;
            result->h = dst.w / src_ratio;
            result->x = dst.x;
            result->y = dst.y + (dst.h - result->h) / 2;
        } else if (src_ratio < dst_ratio) {
            result->w = dst.h * src_ratio;
            result->h = dst.h;
            result->x = dst.x + (dst.w - result->w) / 2;
            result->y = dst.y;
        } else {
            result->x = dst.x;
            result->y = dst.y;
            result->w = dst.w;
            result->h = dst.h;
        }
  }

  DEBUG ("source is %dx%d dest is %dx%d, result is %dx%d with x,y %dx%d",
      src.w, src.h, dst.w, dst.h, result->w, result->h, result->x, result->y);
}

void WaylandWindow::updateBorders()
{
    int width,height;

    if (mNoBorderUpdate)
        return;

    if (mDisplay->getViewporter()) {
        width = height = 1;
        mNoBorderUpdate = true;
    } else {
        width = mRenderRect.w;
        height = mRenderRect.h;
    }

    RenderVideoFormat format = VIDEO_FORMAT_BGRx;
    mAreaShmBuffer = new WaylandShmBuffer(mDisplay);
    struct wl_buffer *wlbuf = mAreaShmBuffer->constructWlBuffer(width, height, format);
    if (wlbuf == NULL) {
        delete mAreaShmBuffer;
        mAreaShmBuffer = NULL;
    }

    wl_surface_attach(mAreaSurfaceWrapper, wlbuf, 0, 0);
}

std::size_t WaylandWindow::calculateDmaBufferHash(RenderDmaBuffer &dmabuf)
{
    std::string hashString("");
    for (int i = 0; i < dmabuf.planeCnt; i++) {
        char hashtmp[1024];
        snprintf (hashtmp, 1024, "%d%d%d%d%d%d", dmabuf.width,dmabuf.height,dmabuf.planeCnt,
                dmabuf.stride[i],dmabuf.offset[i],dmabuf.fd[i]);
        std::string tmp(hashtmp);
        hashString += tmp;
    }

    std::size_t hashval = std::hash<std::string>()(hashString);
    TRACE3("hashstr:%s,val:%zu",hashString.c_str(),hashval);
    return hashval;
}

void WaylandWindow::addWaylandBuffer(RenderBuffer * buf, WaylandBuffer *waylandbuf)
{
    if (buf->flag & BUFFER_FLAG_ALLOCATE_DMA_BUFFER ||
          buf->flag & BUFFER_FLAG_EXTER_DMA_BUFFER) {
        std::size_t hashval = calculateDmaBufferHash(buf->dma);
        std::pair<std::size_t, WaylandBuffer *> item(hashval, waylandbuf);
        TRACE3("fd:%d,w:%d,h:%d,%p,hash:%zu",buf->dma.fd[0],buf->dma.width,buf->dma.height,waylandbuf,hashval);
        mWaylandBuffersMap.insert(item);
    }
}

WaylandBuffer* WaylandWindow::findWaylandBuffer(RenderBuffer * buf)
{
    std::size_t hashval = calculateDmaBufferHash(buf->dma);
    auto item = mWaylandBuffersMap.find(hashval);
    if (item == mWaylandBuffersMap.end()) {
        return NULL;
    }

    return (WaylandBuffer*) item->second;
}

void WaylandWindow::flushBuffers()
{
    for (auto item = mWaylandBuffersMap.begin(); item != mWaylandBuffersMap.end(); ) {
        WaylandBuffer *waylandbuf = (WaylandBuffer*)item->second;
        mWaylandBuffersMap.erase(item++);
        delete waylandbuf;
    }
}

void WaylandWindow::cleanSurface()
{
    /* clear both video and parent surfaces */
    wl_surface_attach (mVideoSurfaceWrapper, NULL, 0, 0);
    wl_surface_commit (mVideoSurfaceWrapper);
    wl_surface_attach (mAreaSurfaceWrapper, NULL, 0, 0);
    wl_surface_commit (mAreaSurfaceWrapper);
}