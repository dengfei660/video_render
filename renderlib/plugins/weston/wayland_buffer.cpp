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
#include "wayland_display.h"
#include "wayland_window.h"
#include "wayland_buffer.h"
#include "Logger.h"
#include "wayland_videoformat.h"
#include "wayland_dma.h"
#include "wayland_shm.h"
#include "Utils.h"

#define TAG "rlib:wayland_buffer"

WaylandBuffer::WaylandBuffer(WaylandDisplay *display, WaylandWindow *window, int logCategory)
    : mDisplay(display),
    mWindow(window),
    mLogCategory(logCategory)
{
    mRenderBuffer = NULL;
    mWaylandWlWrap = NULL;
    mUsedByCompositor = false;
    mIsBusy = false;
    mRealTime = -1;
    mBufferFormat = VIDEO_FORMAT_UNKNOWN;
}

WaylandBuffer::~WaylandBuffer()
{
    /*if weston obtains the wl_buffer,we need
     * notify user to release renderBuffer*/
    if (mRenderBuffer) {
        mWindow->handleBufferReleaseCallback(this);
        mRenderBuffer = NULL;
    }
    if (mWaylandWlWrap) {
        delete mWaylandWlWrap;
        mWaylandWlWrap = NULL;
    }
}

void WaylandBuffer::bufferRelease (void *data, struct wl_buffer *wl_buffer)
{
    WaylandBuffer* waylandBuffer = static_cast<WaylandBuffer*>(data);
    TRACE1(waylandBuffer->mLogCategory,"--wl_buffer:%p,renderBuffer:%p",wl_buffer,waylandBuffer->mRenderBuffer);
    waylandBuffer->mUsedByCompositor = false;
    //sometimes this callback be called twice
    //this cause double free,so check mRenderBuffer
    if (waylandBuffer->mRenderBuffer) {
        waylandBuffer->mWindow->handleBufferReleaseCallback(waylandBuffer);
    }
    waylandBuffer->mRenderBuffer = NULL;
    //if do not support reuse wl_buffer,we need destroy everytime
    if (waylandBuffer->mWindow->isSupportReuseWlBuffer() == false) {
        delete waylandBuffer;
    }
}

static const struct wl_buffer_listener buffer_listener = {
    WaylandBuffer::bufferRelease
};

/*sometimes weston do not call this callback,
so we do not use this callback to count displayed frames*/
void WaylandBuffer::frameDisplayedCallback(void *data, struct wl_callback *callback, uint32_t time)
{
    WaylandBuffer* waylandBuffer = static_cast<WaylandBuffer*>(data);
    TRACE2(waylandBuffer->mLogCategory,"**frame redraw callback,wl_buffer:%p,renderBuffer:%p,time:%d",
        waylandBuffer->getWlBuffer(),waylandBuffer->getRenderBuffer(),time);

    if (waylandBuffer->mRenderBuffer) {
        waylandBuffer->mWindow->handleFrameDisplayedCallback(waylandBuffer);
    }

    wl_callback_destroy (callback);
}

static const struct wl_callback_listener frame_callback_listener = {
  WaylandBuffer::frameDisplayedCallback
};

int WaylandBuffer::constructWlBuffer(RenderBuffer *buf)
{
    struct wl_buffer * wlbuffer = NULL;

    mRenderBuffer = buf;
    if (mWaylandWlWrap) {
        return NO_ERROR;
    }

    if (buf->flag & BUFFER_FLAG_ALLOCATE_DMA_BUFFER ||
            buf->flag & BUFFER_FLAG_EXTER_DMA_BUFFER) {
        WaylandDmaBuffer *waylanddma = new WaylandDmaBuffer(mDisplay, mLogCategory);
        wlbuffer = waylanddma->constructWlBuffer(&buf->dma, mBufferFormat);
        if (!wlbuffer) {
            delete waylanddma;
            ERROR(mLogCategory,"create wl_buffer fail");
            return ERROR_INVALID_OPERATION;
        }
        mWaylandWlWrap = waylanddma;
    }

    /*register buffer release listern*/
    wl_buffer_add_listener (wlbuffer, &buffer_listener, this);

    return NO_ERROR;
}

struct wl_buffer *WaylandBuffer::getWlBuffer()
{
    if (mWaylandWlWrap) {
        return mWaylandWlWrap->getWlBuffer();
    } else {
        return NULL;
    }
}

void WaylandBuffer::attach(struct wl_surface *surface)
{
    struct wl_callback *callback;
    struct wl_buffer *wlbuffer = NULL;
    if (mUsedByCompositor) {
        DEBUG(mLogCategory,"buffer used by compositor");
        return;
    }

    //use this mothed to callback,this frame had displayed
    //we do not use now
    //callback = wl_surface_frame (surface);
    //wl_callback_add_listener (callback, &frame_callback_listener, this);

    wlbuffer = getWlBuffer();
    if (wlbuffer) {
        wl_surface_attach (surface, wlbuffer, 0, 0);
    }

    mUsedByCompositor = true;
}