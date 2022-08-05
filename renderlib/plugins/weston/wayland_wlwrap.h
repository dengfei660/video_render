/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
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