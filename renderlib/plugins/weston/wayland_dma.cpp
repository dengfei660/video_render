/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
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

WaylandDmaBuffer::WaylandDmaBuffer(WaylandDisplay *display, int logCategory)
    : mDisplay(display),
    mLogCategory(logCategory)
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
        TRACE1(mLogCategory,"destroy wl_buffer %p",mWlBuffer);
        wl_buffer_destroy(mWlBuffer);
        mWlBuffer = NULL;
    }
}

void WaylandDmaBuffer::dmabufCreateSuccess(void *data,
            struct zwp_linux_buffer_params_v1 *params,
            struct wl_buffer *new_buffer)
{
    WaylandDmaBuffer *waylandDma = static_cast<WaylandDmaBuffer*>(data);
    TRACE1(waylandDma->mLogCategory,"++create dma wl_buffer:%p ",new_buffer);
    Tls::Mutex::Autolock _l(waylandDma->mMutex);
    waylandDma->mWlBuffer = new_buffer;
    waylandDma->mCondition.signal();
}

void WaylandDmaBuffer::dmabufCreateFail(void *data,
            struct zwp_linux_buffer_params_v1 *params)
{
    WaylandDmaBuffer *waylandDma = static_cast<WaylandDmaBuffer*>(data);
    Tls::Mutex::Autolock _l(waylandDma->mMutex);
    TRACE1(waylandDma->mLogCategory,"!!!create dma wl_buffer fail");
    waylandDma->mWlBuffer = NULL;
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
        ERROR(mLogCategory,"Error change render video format to dmabuffer format fail");
        return NULL;
    }

    //check dma buffer
    if (dmabuf->planeCnt < 0) {
        ERROR(mLogCategory,"Error dmabuf plane count is 0");
        return NULL;
    }
    for (int i = 0; i < dmabuf->planeCnt; i++) {
        if (dmabuf->fd[i] <= 0) {
            ERROR(mLogCategory,"Error dmabuf plane fd is 0");
            return NULL;
        }
    }

    memcpy(&mRenderDmaBuffer, dmabuf, sizeof(RenderDmaBuffer));

    params = zwp_linux_dmabuf_v1_create_params (mDisplay->getDmaBuf());
    if (!params) {
        ERROR(mLogCategory, "zwp_linux_dmabuf_v1_create_params fail");
        return NULL;
    }
    for (int i = 0; i < dmabuf->planeCnt; i++) {
        TRACE2(mLogCategory,"dma buf index:%d,fd:%d,stride:%d,offset:%d",i, dmabuf->fd[i],dmabuf->stride[i], dmabuf->offset[i]);
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
    TRACE1(mLogCategory,"zwp_linux_buffer_params_v1_create,dma width:%d,height:%d,dmabufferformat:%d",dmabuf->width,dmabuf->height,dmabufferFormat);
    zwp_linux_buffer_params_v1_create (params, dmabuf->width, dmabuf->height, dmabufferFormat, flags);

    /* Wait for the request answer */
    wl_display_flush (mDisplay->getWlDisplay());
    Tls::Mutex::Autolock _l(mMutex);
    if (!mWlBuffer) { //if this wlbuffer had created,don't wait zwp linux buffer callback
        mWlBuffer =(struct wl_buffer *)0xffffffff;
        while (mWlBuffer == (struct wl_buffer *)0xffffffff) { //try wait for 1000 ms
            if (ERROR_TIMED_OUT == mCondition.waitRelative(mMutex, 1000/*microsecond*/)) {
                WARNING(mLogCategory,"zwp_linux_buffer_params_v1_create timeout");
                mWlBuffer = NULL;
            }
        }
    }
    //destroy zwp linux buffer params
    zwp_linux_buffer_params_v1_destroy (params);

    return mWlBuffer;
}