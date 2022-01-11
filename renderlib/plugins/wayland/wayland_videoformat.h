#ifndef __WAYLAND_VIDEO_FORMAT_H__
#define __WAYLAND_VIDEO_FORMAT_H__
#include <string.h>
#include <wayland-client-protocol.h>
#include "render_lib.h"

#ifdef  __cplusplus
extern "C" {
#endif

uint32_t video_format_to_wl_dmabuf_format (RenderVideoFormat format);
RenderVideoFormat wl_dmabuf_format_to_video_format (uint32_t wl_format);

int32_t video_format_to_wl_shm_format (RenderVideoFormat format);
RenderVideoFormat wl_shm_format_to_video_format (enum wl_shm_format wl_format);

const char* print_dmabuf_format_name(uint32_t dmabufferformat);
const char* print_render_video_format_name(RenderVideoFormat format);

#ifdef  __cplusplus
}
#endif
#endif