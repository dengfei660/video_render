/* Generated by wayland-scanner 1.17.0 */

#ifndef SIMPLE_SHELL_SERVER_PROTOCOL_H
#define SIMPLE_SHELL_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

/**
 * @page page_simple_shell The simple_shell protocol
 * @section page_ifaces_simple_shell Interfaces
 * - @subpage page_iface_wl_simple_shell - control the layout of surfaces
 * @section page_copyright_simple_shell Copyright
 * <pre>
 *
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2010-2011 Intel Corporation
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that\n the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 * </pre>
 */
struct wl_simple_shell;
struct wl_surface;

/**
 * @page page_iface_wl_simple_shell wl_simple_shell
 * @section page_iface_wl_simple_shell_desc Description
 *
 * The simple_shell provides control over the size, position,
 * z-order, opacity, etc., of the surfaces making up the user
 * interfaces suitable for situations such as embedded systems.
 * @section page_iface_wl_simple_shell_api API
 * See @ref iface_wl_simple_shell.
 */
/**
 * @defgroup iface_wl_simple_shell The wl_simple_shell interface
 *
 * The simple_shell provides control over the size, position,
 * z-order, opacity, etc., of the surfaces making up the user
 * interfaces suitable for situations such as embedded systems.
 */
extern const struct wl_interface wl_simple_shell_interface;

/**
 * @ingroup iface_wl_simple_shell
 * @struct wl_simple_shell_interface
 */
struct wl_simple_shell_interface {
	/**
	 */
	void (*set_name)(struct wl_client *client,
			 struct wl_resource *resource,
			 uint32_t surfaceId,
			 const char *name);
	/**
	 */
	void (*set_visible)(struct wl_client *client,
			    struct wl_resource *resource,
			    uint32_t surfaceId,
			    uint32_t visible);
	/**
	 */
	void (*set_geometry)(struct wl_client *client,
			     struct wl_resource *resource,
			     uint32_t surfaceId,
			     int32_t x,
			     int32_t y,
			     int32_t width,
			     int32_t height);
	/**
	 */
	void (*set_opacity)(struct wl_client *client,
			    struct wl_resource *resource,
			    uint32_t surfaceId,
			    wl_fixed_t opacity);
	/**
	 */
	void (*set_zorder)(struct wl_client *client,
			   struct wl_resource *resource,
			   uint32_t surfaceId,
			   wl_fixed_t zorder);
	/**
	 */
	void (*get_status)(struct wl_client *client,
			   struct wl_resource *resource,
			   uint32_t surfaceId);
	/**
	 * get all existing surfaces
	 *
	 * Causes a surface status event for each existing surface to be
	 * sent to the listener registered by the client issuing the
	 * request.
	 */
	void (*get_surfaces)(struct wl_client *client,
			     struct wl_resource *resource);
	/**
	 */
	void (*set_focus)(struct wl_client *client,
			  struct wl_resource *resource,
			  uint32_t surfaceId);
};

#define WL_SIMPLE_SHELL_SURFACE_ID 0
#define WL_SIMPLE_SHELL_SURFACE_CREATED 1
#define WL_SIMPLE_SHELL_SURFACE_DESTROYED 2
#define WL_SIMPLE_SHELL_SURFACE_STATUS 3
#define WL_SIMPLE_SHELL_GET_SURFACES_DONE 4

/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SURFACE_ID_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SURFACE_CREATED_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SURFACE_DESTROYED_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SURFACE_STATUS_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_GET_SURFACES_DONE_SINCE_VERSION 1

/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_NAME_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_VISIBLE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_GEOMETRY_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_OPACITY_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_ZORDER_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_GET_STATUS_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_GET_SURFACES_SINCE_VERSION 1
/**
 * @ingroup iface_wl_simple_shell
 */
#define WL_SIMPLE_SHELL_SET_FOCUS_SINCE_VERSION 1

/**
 * @ingroup iface_wl_simple_shell
 * Sends an surface_id event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
wl_simple_shell_send_surface_id(struct wl_resource *resource_, struct wl_resource *surface, uint32_t surfaceId)
{
	wl_resource_post_event(resource_, WL_SIMPLE_SHELL_SURFACE_ID, surface, surfaceId);
}

/**
 * @ingroup iface_wl_simple_shell
 * Sends an surface_created event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
wl_simple_shell_send_surface_created(struct wl_resource *resource_, uint32_t surfaceId, const char *name)
{
	wl_resource_post_event(resource_, WL_SIMPLE_SHELL_SURFACE_CREATED, surfaceId, name);
}

/**
 * @ingroup iface_wl_simple_shell
 * Sends an surface_destroyed event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
wl_simple_shell_send_surface_destroyed(struct wl_resource *resource_, uint32_t surfaceId, const char *name)
{
	wl_resource_post_event(resource_, WL_SIMPLE_SHELL_SURFACE_DESTROYED, surfaceId, name);
}

/**
 * @ingroup iface_wl_simple_shell
 * Sends an surface_status event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
wl_simple_shell_send_surface_status(struct wl_resource *resource_, uint32_t surfaceId, const char *name, uint32_t visible, int32_t x, int32_t y, int32_t width, int32_t height, wl_fixed_t opacity, wl_fixed_t zorder)
{
	wl_resource_post_event(resource_, WL_SIMPLE_SHELL_SURFACE_STATUS, surfaceId, name, visible, x, y, width, height, opacity, zorder);
}

/**
 * @ingroup iface_wl_simple_shell
 * Sends an get_surfaces_done event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
wl_simple_shell_send_get_surfaces_done(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, WL_SIMPLE_SHELL_GET_SURFACES_DONE);
}

#ifdef  __cplusplus
}
#endif

#endif