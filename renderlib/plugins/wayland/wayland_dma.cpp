#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "wayland_dma.h"
#include "Logger.h"
#include "wayland_display.h"

#define TAG "rlib:wayland_dma"

WaylandDmaBuffer::WaylandDmaBuffer(WaylandDisplay *display)
    : mDisplay(display)
{
    mRenderDmaBuffer = {0,};
    mWlBuffer = NULL;
    mData = NULL;
    mSize = 0;
}

WaylandDmaBuffer::~WaylandDmaBuffer()
{
    //release wl_buffer
    if (mWlBuffer) {
        TRACE1("destroy wl_buffer %p",mWlBuffer);
        wl_buffer_destroy(mWlBuffer);
        mWlBuffer = NULL;
    }
}

void WaylandDmaBuffer::dmabufCreateSuccess(void *data,
            struct zwp_linux_buffer_params_v1 *params,
            struct wl_buffer *new_buffer)
{
    WaylandDmaBuffer *waylandDma = static_cast<WaylandDmaBuffer*>(data);
    TRACE1("++create dma wl_buffer:%p ",new_buffer);
    Tls::Mutex::Autolock _l(waylandDma->mMutex);
    waylandDma->mWlBuffer = new_buffer;
    zwp_linux_buffer_params_v1_destroy (params);
    waylandDma->mCondition.signal();
}

void WaylandDmaBuffer::dmabufCreateFail(void *data,
            struct zwp_linux_buffer_params_v1 *params)
{
    WaylandDmaBuffer *waylandDma = static_cast<WaylandDmaBuffer*>(data);
    Tls::Mutex::Autolock _l(waylandDma->mMutex);
    TRACE1("!!!create dma wl_buffer fail");
    waylandDma->mWlBuffer = NULL;
    zwp_linux_buffer_params_v1_destroy (params);
    waylandDma->mCondition.signal();
}

static const struct zwp_linux_buffer_params_v1_listener dmabuf_params_listener = {
  WaylandDmaBuffer::dmabufCreateSuccess,
  WaylandDmaBuffer::dmabufCreateFail
};

struct wl_buffer *WaylandDmaBuffer::constructWlBuffer(RenderDmaBuffer *dmabuf, RenderVideoFormat format)
{
    struct zwp_linux_buffer_params_v1 *params = NULL;
    int ret;
    int64_t timeout;
    uint64_t formatModifier = 0;
    uint32_t flags = 0;
    uint32_t dmabufferFormat;

    ret = mDisplay->toDmaBufferFormat(format, &dmabufferFormat, &formatModifier);
    if (ret != NO_ERROR) {
        ERROR("Error change render video format to dmabuffer format fail");
        return NULL;
    }

    //check dma buffer
    if (dmabuf->planeCnt < 0) {
        ERROR("Error dmabuf plane count is 0");
        return NULL;
    }
    for (int i = 0; i < dmabuf->planeCnt; i++) {
        if (dmabuf->fd[i] <= 0) {
            ERROR("Error dmabuf plane fd is 0");
            return NULL;
        }
    }

    memcpy(&mRenderDmaBuffer, dmabuf, sizeof(RenderDmaBuffer));

    params = zwp_linux_dmabuf_v1_create_params (mDisplay->getDmaBuf());
    for (int i = 0; i < dmabuf->planeCnt; i++) {
        TRACE2("dma buf index:%d,fd:%d,stride:%d,offset:%d",i, dmabuf->fd[i],dmabuf->stride[i], dmabuf->offset[i]);
        zwp_linux_buffer_params_v1_add(params,
                   dmabuf->fd[i],
                   i, /*plane_idx*/
                   dmabuf->offset[i], /*data offset*/
                   dmabuf->stride[i],
                   formatModifier >> 32,
                   formatModifier & 0xffffffff);
    }

    /* Request buffer creation */
    zwp_linux_buffer_params_v1_add_listener (params, &dmabuf_params_listener, (void *)this);
    TRACE1("zwp_linux_buffer_params_v1_create,dma width:%d,height:%d,dmabufferformat:%d",dmabuf->width,dmabuf->height,dmabufferFormat);
    zwp_linux_buffer_params_v1_create (params, dmabuf->width, dmabuf->height, dmabufferFormat, flags);

    /* Wait for the request answer */
    wl_display_flush (mDisplay->getWlDisplay());
    Tls::Mutex::Autolock _l(mMutex);
    mWlBuffer =(struct wl_buffer *)0xffffffff;
    while (mWlBuffer == (struct wl_buffer *)0xffffffff) { //try wait for 1000 ms
        if (ERROR_TIMED_OUT == mCondition.waitRelative(mMutex, 1000/*microsecond*/)) {
            WARNING("zwp_linux_buffer_params_v1_create timeout");
            zwp_linux_buffer_params_v1_destroy (params);
            mWlBuffer = NULL;
        }
    }

    return mWlBuffer;
}