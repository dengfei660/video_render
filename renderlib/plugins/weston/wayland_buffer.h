/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __WAYLAND_BUFFER_H__
#define __WAYLAND_BUFFER_H__
#include "render_lib.h"
#include "Mutex.h"
#include "Condition.h"
#include "wayland_wlwrap.h"

class WaylandDisplay;
class WaylandWindow;

/**
 * @brief create waylandbuffer include wl_buffer to use
 * if create waylandbuffer with dma buffer, the sequence is
 * 1.waylandbuffer = new WaylandBuffer
 * 2.waylandbuffer->setRenderRealTime
 * 3.setRenderRealTime->setBufferFormat
 * 4.dmabufConstructWlBuffer
 *
 * if create waylandbuffer with shm pool
 *
 */
class WaylandBuffer {
  public:
    WaylandBuffer(WaylandDisplay *display, WaylandWindow *window, int logCategory);
    virtual ~WaylandBuffer();
    int constructWlBuffer(RenderBuffer *buf);
    void setUsedByCompositor(bool used);
    bool isUsedByCompositor();
    void setRenderRealTime(int64_t realTime) {
        mRealTime = realTime;
    };
    int64_t getRenderRealTime() {
        return mRealTime;
    };
    void attach(struct wl_surface *surface);
    void setBufferFormat(RenderVideoFormat format)
    {
        mBufferFormat = format;
    };
    RenderBuffer *getRenderBuffer()
    {
        return mRenderBuffer;
    };
    WaylandDisplay *getWaylandDisplay()
    {
        return mDisplay;
    };
    bool isBusy()
    {
        return mIsBusy;
    };
    struct wl_buffer *getWlBuffer();
    static void bufferRelease (void *data, struct wl_buffer *wl_buffer);
    static void frameDisplayedCallback(void *data, struct wl_callback *callback, uint32_t time);
  private:
    int mLogCategory;
    WaylandDisplay *mDisplay;
    WaylandWindow *mWindow;
    RenderBuffer *mRenderBuffer;
    WaylandWLWrap *mWaylandWlWrap;
    bool mIsBusy;
    int64_t mRealTime;
    bool mUsedByCompositor;
    RenderVideoFormat mBufferFormat;
};

#endif /*__WAYLAND_BUFFER_H__*/