#ifndef __WAYLAND_DISPLAY_H__
#define __WAYLAND_DISPLAY_H__
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <list>
#include <unordered_map>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "linux-explicit-synchronization-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "Thread.h"
#include "Poll.h"
#include "render_lib.h"

using namespace std;

class WaylandPlugin;

class WaylandDisplay : public Tls::Thread{
  public:
    WaylandDisplay(WaylandPlugin *plugin);
    virtual ~WaylandDisplay();
    /**
     * @brief connet client to compositor server
     * and accquire a display from compositor
     *
     * @return int 0 sucess,other fail
     */
    int  openDisplay();
    /**
     * @brief release display that accquired from compositor
     *
     */
    void closeDisplay();
    /**
     * @brief change RenderVideoFormat to wayland protocol dma buffer format
     * and get the matched dmabuffer modifiers
     *
     */
    int toDmaBufferFormat(RenderVideoFormat format, uint32_t *outDmaformat /*out param*/, uint64_t *outDmaformatModifiers /*out param*/);
    /**
     * @brief change RenderVideoFormat to wayland protocol shm buffer format
     *
     * @param format RenderVideoFormat
     * @param outformat wayland protocol shm buffer format
     * @return int 0 sucess,other fail
     */
    int toShmBufferFormat(RenderVideoFormat format, uint32_t *outformat);
    /**
     * @brief Set the Video Buffer Format object
     *
     * @param format RenderVideoFormat it is defined in render_lib.h
     */
    void setVideoBufferFormat(RenderVideoFormat format);
    RenderVideoFormat getVideoBufferFormat() {
        return mBufferFormat;
    };
    //thread func
    void readyToRun();
    virtual bool threadLoop();

    struct wl_display *getWlDisplay() {
        return mWlDisplay;
    };
    struct wl_compositor * getCompositor()
    {
        return mCompositor;
    };
    struct wl_subcompositor *getSubCompositor()
    {
        return mSubCompositor;
    };
    struct wl_event_queue *getWlEventQueue() {
        return mWlQueue;
    };
    struct xdg_wm_base * getXdgWmBase()
    {
        return mXdgWmBase;
    };
    struct wp_viewporter *getViewporter()
    {
        return mViewporter;
    };
    struct zwp_linux_dmabuf_v1 * getDmaBuf()
    {
        return mDmabuf;
    };
    struct wl_shm *getShm()
    {
        return mShm;
    };
    WaylandPlugin * getPlugin()
    {
        return mWaylandPlugin;
    };
    /**callback functions**/
    static void dmabuf_modifiers(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
		 uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo);
    static void dmaBufferFormat (void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format);
    static void registryHandleGlobal (void *data, struct wl_registry *registry,
            uint32_t id, const char *interface, uint32_t version);
    static void registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name);
    static void shmFormat (void *data, struct wl_shm *wl_shm, uint32_t format);
  private:
    char *require_xdg_runtime_dir();
    WaylandPlugin *mWaylandPlugin;
    struct wl_display *mWlDisplay;
    struct wl_display *mWlDisplayWrapper;
    struct wl_event_queue *mWlQueue;

    struct wl_registry *mRegistry;
    struct wl_compositor *mCompositor;
    struct wl_subcompositor *mSubCompositor;
    struct xdg_wm_base *mXdgWmBase;
    struct wp_viewporter *mViewporter;
    struct zwp_linux_dmabuf_v1 *mDmabuf;
    struct wl_shm *mShm;

    std::list<uint32_t> mShmFormats;
    std::unordered_map<uint32_t, uint64_t> mDmaBufferFormats;
    RenderVideoFormat mBufferFormat;

    mutable Tls::Mutex mBufferMutex;
    int mFd;
    Tls::Poll *mPoll;
};

#endif /*__WAYLAND_DISPLAY_H__*/