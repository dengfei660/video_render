#include <cstring>
#include "wayland_display.h"
#include "ErrorCode.h"
#include "Logger.h"
#include "wayland_plugin.h"
#include "wayland_videoformat.h"

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define TAG "rlib:wayland_display"

void WaylandDisplay::dmabuf_modifiers(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
         uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    Tls::Mutex::Autolock _l(self->mMutex);
    if (wl_dmabuf_format_to_video_format (format) != VIDEO_FORMAT_UNKNOWN) {
        TRACE2("regist dmabuffer format:%d (%s) hi:%x,lo:%x",format,print_dmabuf_format_name(format),modifier_hi,modifier_lo);
        uint64_t modifier = ((uint64_t)modifier_hi << 32) | modifier_lo;
        auto item = self->mDmaBufferFormats.find(format);
        if (item == self->mDmaBufferFormats.end()) {
            std::pair<uint32_t ,uint64_t> item(format, modifier);
            self->mDmaBufferFormats.insert(item);
        } else { //found format
            item->second = modifier;
        }
    }
}

void
WaylandDisplay::dmaBufferFormat (void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
    uint32_t format)
{
#if 0
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);

    if (wl_dmabuf_format_to_video_format (format) != VIDEO_FORMAT_UNKNOWN) {
        TRACE1("regist dmabuffer format:%d : %s",format);
        //self->mDmaBufferFormats.push_back(format);
    }
#endif
   /* XXX: deprecated */
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
    WaylandDisplay::dmaBufferFormat,
    WaylandDisplay::dmabuf_modifiers
};

static void
handle_xdg_wm_base_ping (void *user_data, struct xdg_wm_base *xdg_wm_base,
    uint32_t serial)
{
  xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
  handle_xdg_wm_base_ping
};

void WaylandDisplay::shmFormat(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    self->mShmFormats.push_back(format);
}

static const struct wl_shm_listener shm_listener = {
  WaylandDisplay::shmFormat
};

void
WaylandDisplay::registryHandleGlobal (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    TRACE1("registryHandleGlobal,interface:%s,version:%d",interface,version);

    if (strcmp (interface, "wl_compositor") == 0) {
        self->mCompositor = (struct wl_compositor *)wl_registry_bind (registry, id, &wl_compositor_interface, 1/*MIN (version, 3)*/);
    } else if (strcmp (interface, "wl_subcompositor") == 0) {
        self->mSubCompositor = (struct wl_subcompositor *)wl_registry_bind (registry, id, &wl_subcompositor_interface, 1);
    } else if (strcmp (interface, "xdg_wm_base") == 0) {
        self->mXdgWmBase = (struct xdg_wm_base *)wl_registry_bind (registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener (self->mXdgWmBase, &xdg_wm_base_listener, (void *)self);
    } else if (strcmp (interface, "wl_shm") == 0) {
        self->mShm = (struct wl_shm *)wl_registry_bind (registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener (self->mShm, &shm_listener, self);
    } else if (strcmp (interface, "zwp_fullscreen_shell_v1") == 0) {
        //self->mFullscreenShell = (struct zwp_fullscreen_shell_v1 *)wl_registry_bind (registry, id,
        //    &zwp_fullscreen_shell_v1_interface, 1);
    } else if (strcmp (interface, "wp_viewporter") == 0) {
        self->mViewporter = (struct wp_viewporter *)wl_registry_bind (registry, id, &wp_viewporter_interface, 1);
    } else if (strcmp (interface, "zwp_linux_dmabuf_v1") == 0) {
        if (version < 3)
            return;
        self->mDmabuf = (struct zwp_linux_dmabuf_v1 *)wl_registry_bind (registry, id, &zwp_linux_dmabuf_v1_interface, 3);
        zwp_linux_dmabuf_v1_add_listener (self->mDmabuf, &dmabuf_listener, (void *)self);
    }
}

void
WaylandDisplay::registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name)
{
    /* temporarily do nothing */
    DEBUG("wayland display remove registry handle global");
}

static const struct wl_registry_listener registry_listener = {
  WaylandDisplay::registryHandleGlobal,
  WaylandDisplay::registryHandleGlobalRemove
};

WaylandDisplay::WaylandDisplay(WaylandPlugin *plugin)
    :mBufferMutex("bufferMutex"),
    mWaylandPlugin(plugin)
{
    TRACE2("construct WaylandDisplay");
    mWlDisplay = NULL;
    mWlDisplayWrapper = NULL;
    mWlQueue = NULL;
    mRegistry = NULL;
    mCompositor = NULL;
    mXdgWmBase = NULL;
    mViewporter = NULL;
    mDmabuf = NULL;
    mShm = NULL;
    mPoll = new Tls::Poll(true);
}

WaylandDisplay::~WaylandDisplay()
{
    TRACE2("desconstruct WaylandDisplay");
    if (mPoll) {
        delete mPoll;
        mPoll = NULL;
    }
}

char *WaylandDisplay::require_xdg_runtime_dir()
{
    char *val = getenv("XDG_RUNTIME_DIR");
    INFO("XDG_RUNTIME_DIR=%s",val);

    return val;
}

int WaylandDisplay::openDisplay()
{
    char *name = require_xdg_runtime_dir();
    //DEBUG("name:%s",name);
    DEBUG("openDisplay in");
    mWlDisplay = wl_display_connect(NULL);
    if (!mWlDisplay) {
        FATAL("Failed to connect to the wayland display, XDG_RUNTIME_DIR='%s'",
        name ? name : "NULL");
        return ERROR_OPEN_FAIL;
    }

    mWlDisplayWrapper = (struct wl_display *)wl_proxy_create_wrapper ((void *)mWlDisplay);
    mWlQueue = wl_display_create_queue (mWlDisplay);
    wl_proxy_set_queue ((struct wl_proxy *)mWlDisplayWrapper, mWlQueue);

    mRegistry = wl_display_get_registry (mWlDisplayWrapper);
    wl_registry_add_listener (mRegistry, &registry_listener, (void *)this);

    /* we need exactly 2 roundtrips to discover global objects and their state */
    for (int i = 0; i < 2; i++) {
        if (wl_display_roundtrip_queue (mWlDisplay, mWlQueue) < 0) {
            FATAL("Error communicating with the wayland display");
            return ERROR_OPEN_FAIL;
        }
    }

    if (!mCompositor) {
        FATAL("Could not bind to wl_compositor. Either it is not implemented in " \
        "the compositor, or the implemented version doesn't match");
        return ERROR_OPEN_FAIL;
    }

    if (!mDmabuf) {
        FATAL("Could not bind to zwp_linux_dmabuf_v1");
        return ERROR_OPEN_FAIL;
    }

    if (!mXdgWmBase) {
        /* If wl_surface and wl_display are passed via GstContext
        * wl_shell, xdg_shell and zwp_fullscreen_shell are not used.
        * In this case is correct to continue.
        */
        FATAL("Could not bind to either wl_shell, xdg_wm_base or "
            "zwp_fullscreen_shell, video display may not work properly.");
        return ERROR_OPEN_FAIL;
    }

    DEBUG("openDisplay out");
    return NO_ERROR;
}

void WaylandDisplay::closeDisplay()
{
    DEBUG("closeDisplay in");

   if (isRunning()) {
        TRACE1("try stop dispatch thread");
        if (mPoll) {
            mPoll->setFlushing(true);
        }
        requestExitAndWait();
    }

    if (mViewporter) {
        wp_viewporter_destroy (mViewporter);
        mViewporter = NULL;
    }

    if (mDmabuf) {
        zwp_linux_dmabuf_v1_destroy (mDmabuf);
        mDmabuf = NULL;
    }

    if (mXdgWmBase) {
        xdg_wm_base_destroy (mXdgWmBase);
        mXdgWmBase = NULL;
    }

    if (mCompositor) {
        wl_compositor_destroy (mCompositor);
        mCompositor = NULL;
    }

    if (mSubCompositor) {
        wl_subcompositor_destroy (mSubCompositor);
        mSubCompositor = NULL;
    }

    if (mRegistry) {
        wl_registry_destroy (mRegistry);
        mRegistry= NULL;
    }

    if (mWlDisplayWrapper) {
        wl_proxy_wrapper_destroy (mWlDisplayWrapper);
        mWlDisplayWrapper = NULL;
    }

    if (mWlQueue) {
        wl_event_queue_destroy (mWlQueue);
        mWlQueue = NULL;
    }

    if (mWlDisplay) {
        wl_display_flush (mWlDisplay);
        wl_display_disconnect (mWlDisplay);
        mWlDisplay = NULL;
    }

    DEBUG("closeDisplay out");
}

int WaylandDisplay::toDmaBufferFormat(RenderVideoFormat format, uint32_t *outDmaformat /*out param*/, uint64_t *outDmaformatModifiers /*out param*/)
{
    if (!outDmaformat || !outDmaformatModifiers) {
        WARNING("NULL params");
        return ERROR_PARAM_NULL;
    }

    *outDmaformat = 0;
    *outDmaformatModifiers = 0;

    uint32_t dmaformat = video_format_to_wl_dmabuf_format (format);
    if (dmaformat == -1) {
        ERROR("Error not found render video format:%d to wl dmabuf format",format);
        return ERROR_NOT_FOUND;
    }

    TRACE1("render video format:%d -> dmabuf format:%d",format,dmaformat);
    *outDmaformat = (uint32_t)dmaformat;

    /*get dmaformat and modifiers*/
    auto item = mDmaBufferFormats.find(dmaformat);
    if (item == mDmaBufferFormats.end()) { //not found
        WARNING("Not found dmabuf for render video format :%d",format);
        *outDmaformatModifiers = 0;
        return NO_ERROR;
    }

    *outDmaformatModifiers = (uint64_t)item->second;

    return NO_ERROR;
}

int WaylandDisplay::toShmBufferFormat(RenderVideoFormat format, uint32_t *outformat)
{
    if (!outformat) {
        WARNING("NULL params");
        return ERROR_PARAM_NULL;
    }

    *outformat = 0;

    int shmformat = (int)video_format_to_wl_shm_format(format);
    if (shmformat < 0) {
        ERROR("Error not found render video format:%d to wl shmbuf format",format);
        return ERROR_NOT_FOUND;
    }

    for (auto item = mShmFormats.begin(); item != mShmFormats.end(); ++item) {
        uint32_t  registFormat = (uint32_t)*item;
        if (registFormat == (uint32_t)shmformat) {
            *outformat = registFormat;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

void WaylandDisplay::setVideoBufferFormat(RenderVideoFormat format)
{
    TRACE1("set video buffer format: %d",format);
    mBufferFormat = format;
};

void WaylandDisplay::readyToRun()
{
    mFd = wl_display_get_fd (mWlDisplay);
    if (mPoll) {
        mPoll->addFd(mFd);
        mPoll->setFdReadable(mFd, true);
    }
}

bool WaylandDisplay::threadLoop()
{
    int ret;

    while (wl_display_prepare_read_queue (mWlDisplay, mWlQueue) != 0) {
      wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    }

    wl_display_flush (mWlDisplay);

    /*poll timeout value must > 300 ms,otherwise zwp_linux_dmabuf will create failed,
     so do use -1 to wait for ever*/
    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        wl_display_cancel_read(mWlDisplay);
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    if (wl_display_read_events (mWlDisplay) == -1) {
        goto tag_error;
    }

    wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    return true;
tag_error:
    ERROR("Error communicating with the wayland server");
    return false;
}