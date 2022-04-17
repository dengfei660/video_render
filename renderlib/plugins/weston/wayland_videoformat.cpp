#include <drm_fourcc.h>
#include "render_lib.h"
#include "Logger.h"
#include "wayland_videoformat.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define TAG "rlib:wayland_videoformat"

typedef struct
{
    int32_t wl_shm_format;
    uint32_t dma_format;
    RenderVideoFormat render_format;
} wl_VideoFormat;

static const wl_VideoFormat wl_formats[] = {
  {WL_SHM_FORMAT_XRGB8888, DRM_FORMAT_XRGB8888, VIDEO_FORMAT_BGRx},
  {WL_SHM_FORMAT_ARGB8888, DRM_FORMAT_ARGB8888, VIDEO_FORMAT_BGRA},
  {WL_SHM_FORMAT_XBGR8888, DRM_FORMAT_XBGR8888, VIDEO_FORMAT_RGBx},
  {WL_SHM_FORMAT_RGBX8888, DRM_FORMAT_RGBX8888, VIDEO_FORMAT_xBGR},
  {WL_SHM_FORMAT_BGRX8888, DRM_FORMAT_BGRX8888, VIDEO_FORMAT_xRGB},
  {WL_SHM_FORMAT_ABGR8888, DRM_FORMAT_ABGR8888, VIDEO_FORMAT_RGBA},
  {WL_SHM_FORMAT_RGBA8888, DRM_FORMAT_RGBA8888, VIDEO_FORMAT_ABGR},
  {WL_SHM_FORMAT_BGRA8888, DRM_FORMAT_BGRA8888, VIDEO_FORMAT_ARGB},
  {WL_SHM_FORMAT_RGB888, DRM_FORMAT_RGB888, VIDEO_FORMAT_RGB},
  {WL_SHM_FORMAT_BGR888, DRM_FORMAT_BGR888, VIDEO_FORMAT_BGR},
  {WL_SHM_FORMAT_RGB565, DRM_FORMAT_RGB565, VIDEO_FORMAT_RGB16},
  {WL_SHM_FORMAT_BGR565, DRM_FORMAT_BGR565, VIDEO_FORMAT_BGR16},

  {WL_SHM_FORMAT_YUYV, DRM_FORMAT_YUYV, VIDEO_FORMAT_YUY2},
  {WL_SHM_FORMAT_YVYU, DRM_FORMAT_YVYU, VIDEO_FORMAT_YVYU},
  {WL_SHM_FORMAT_UYVY, DRM_FORMAT_UYVY, VIDEO_FORMAT_UYVY},
  {WL_SHM_FORMAT_AYUV, DRM_FORMAT_AYUV, VIDEO_FORMAT_AYUV},
  {WL_SHM_FORMAT_NV12, DRM_FORMAT_NV12, VIDEO_FORMAT_NV12},
  {WL_SHM_FORMAT_NV21, DRM_FORMAT_NV21, VIDEO_FORMAT_NV21},
  {WL_SHM_FORMAT_NV16, DRM_FORMAT_NV16, VIDEO_FORMAT_NV16},
  {WL_SHM_FORMAT_NV61, DRM_FORMAT_NV61, VIDEO_FORMAT_NV61},
  {WL_SHM_FORMAT_YUV410, DRM_FORMAT_YUV410, VIDEO_FORMAT_YUV9},
  {WL_SHM_FORMAT_YVU410, DRM_FORMAT_YVU410, VIDEO_FORMAT_YVU9},
  {WL_SHM_FORMAT_YUV411, DRM_FORMAT_YUV411, VIDEO_FORMAT_Y41B},
  {WL_SHM_FORMAT_YUV420, DRM_FORMAT_YUV420, VIDEO_FORMAT_I420},
  {WL_SHM_FORMAT_YVU420, DRM_FORMAT_YVU420, VIDEO_FORMAT_YV12},
  {WL_SHM_FORMAT_YUV422, DRM_FORMAT_YUV422, VIDEO_FORMAT_Y42B},
  {WL_SHM_FORMAT_YUV444, DRM_FORMAT_YUV444, VIDEO_FORMAT_v308},
};

/**
 * @brief translate video buffer format to wl dmabuffer format
 *
 * @param format render video format
 * @return int if found return dma format,0 if not found
 */
uint32_t video_format_to_wl_dmabuf_format (RenderVideoFormat format)
{
    int i;
    for (i = 0; i < sizeof(wl_formats)/sizeof(wl_formats[0]); i++) {
        if (wl_formats[i].render_format == format)
            return wl_formats[i].dma_format;
    }

    TRACE3(NO_CATEGERY,"wl dmabuf format not found");
    return 0;
}

RenderVideoFormat wl_dmabuf_format_to_video_format (uint32_t wl_format)
{
    int i;

    for (i = 0; i < sizeof(wl_formats)/sizeof(wl_formats[0]); i++)
      if (wl_formats[i].dma_format == wl_format)
        return (RenderVideoFormat)wl_formats[i].render_format;

    return VIDEO_FORMAT_UNKNOWN;
}

int32_t video_format_to_wl_shm_format (RenderVideoFormat format)
{
    int i;

    for (i = 0; i < sizeof(wl_formats)/sizeof(wl_formats[0]); i++)
      if (wl_formats[i].render_format == format)
        return wl_formats[i].wl_shm_format;

    return (int32_t) -1;
}

RenderVideoFormat wl_shm_format_to_video_format (enum wl_shm_format wl_format)
{
    int i;

    for (i = 0; i < sizeof(wl_formats)/sizeof(wl_formats[0]); i++)
      if (wl_formats[i].wl_shm_format == wl_format)
        return (RenderVideoFormat)wl_formats[i].render_format;

    return VIDEO_FORMAT_UNKNOWN;
}

const char * print_dmabuf_format_name(uint32_t dmabufferformat)
{
    const char* str = nullptr;
    switch (dmabufferformat) {
        case DRM_FORMAT_XRGB8888:{
            str = "DRM_FORMAT_XRGB8888";
        } break;
        case DRM_FORMAT_ARGB8888:{
            str = "DRM_FORMAT_ARGB8888";
        } break;
        case DRM_FORMAT_XBGR8888:{
            str = "DRM_FORMAT_XBGR8888";
        } break;
        case DRM_FORMAT_RGBX8888:{
            str = "DRM_FORMAT_RGBX8888";
        } break;
        case DRM_FORMAT_BGRX8888:{
            str = "DRM_FORMAT_BGRX8888";
        } break;
        case DRM_FORMAT_ABGR8888:{
            str = "DRM_FORMAT_ABGR8888";
        } break;
        case DRM_FORMAT_RGBA8888:{
            str = "DRM_FORMAT_RGBA8888";
        } break;
        case DRM_FORMAT_BGRA8888:{
            str = "DRM_FORMAT_BGRA8888";
        } break;
        case DRM_FORMAT_RGB888:{
            str = "DRM_FORMAT_RGB888";
        } break;
        case DRM_FORMAT_BGR888:{
            str = "DRM_FORMAT_BGR888";
        } break;
        case DRM_FORMAT_RGB565:{
            str = "DRM_FORMAT_RGB565";
        } break;
        case DRM_FORMAT_BGR565:{
            str = "DRM_FORMAT_BGR565";
        } break;

        case DRM_FORMAT_YUYV:{
            str = "DRM_FORMAT_YUYV";
        } break;
        case DRM_FORMAT_YVYU:{
            str = "DRM_FORMAT_YVYU";
        } break;
        case DRM_FORMAT_UYVY:{
            str = "DRM_FORMAT_UYVY";
        } break;
        case DRM_FORMAT_AYUV:{
            str = "DRM_FORMAT_AYUV";
        } break;
        case DRM_FORMAT_NV12:{
            str = "DRM_FORMAT_NV12";
        } break;
        case DRM_FORMAT_NV21:{
            str = "DRM_FORMAT_NV21";
        } break;
        case DRM_FORMAT_NV16:{
            str = "DRM_FORMAT_NV16";
        } break;
        case DRM_FORMAT_NV61:{
            str = "DRM_FORMAT_NV61";
        } break;
        case DRM_FORMAT_YUV410:{
            str = "DRM_FORMAT_YUV410";
        } break;
        case DRM_FORMAT_YVU410:{
            str = "DRM_FORMAT_YVU410";
        } break;
        case DRM_FORMAT_YUV411:{
            str = "DRM_FORMAT_YUV411";
        } break;
        case DRM_FORMAT_YUV420:{
            str = "DRM_FORMAT_YUV420";
        } break;
        case DRM_FORMAT_YVU420:{
            str = "DRM_FORMAT_YVU420";
        } break;
        case DRM_FORMAT_YUV422:{
            str = "DRM_FORMAT_YUV422";
        } break;
        case DRM_FORMAT_YUV444:{
            str = "DRM_FORMAT_YUV444";
        } break;

        default:
            str = "Unknow";
        break;
    }
    return str;
}

const char * print_render_video_format_name(RenderVideoFormat format)
{
    const char* str = nullptr;
    switch (format) {
        case VIDEO_FORMAT_UNKNOWN:{
            str = "VIDEO_FORMAT_UNKNOWN";
        } break;
        case VIDEO_FORMAT_ENCODED:{
            str = "VIDEO_FORMAT_ENCODED";
        } break;
        case VIDEO_FORMAT_I420:{
            str = "VIDEO_FORMAT_I420";
        } break;
        case VIDEO_FORMAT_YV12:{
            str = "VIDEO_FORMAT_YV12";
        } break;
        case VIDEO_FORMAT_YUY2:{
            str = "VIDEO_FORMAT_YUY2";
        } break;
        case VIDEO_FORMAT_UYVY:{
            str = "VIDEO_FORMAT_UYVY";
        } break;
        case VIDEO_FORMAT_AYUV:{
            str = "VIDEO_FORMAT_AYUV";
        } break;
        case VIDEO_FORMAT_RGBx:{
            str = "VIDEO_FORMAT_RGBx";
        } break;
        case VIDEO_FORMAT_BGRx:{
            str = "VIDEO_FORMAT_BGRx";
        } break;
        case VIDEO_FORMAT_xRGB:{
            str = "VIDEO_FORMAT_xRGB";
        } break;
        case VIDEO_FORMAT_xBGR:{
            str = "VIDEO_FORMAT_xBGR";
        } break;
        case VIDEO_FORMAT_RGBA:{
            str = "VIDEO_FORMAT_RGBA";
        } break;
        case VIDEO_FORMAT_BGRA:{
            str = "VIDEO_FORMAT_BGRA";
        } break;
        case VIDEO_FORMAT_ARGB:{
            str = "VIDEO_FORMAT_ARGB";
        } break;
        case VIDEO_FORMAT_ABGR:{
            str = "VIDEO_FORMAT_ABGR";
        } break;
        case VIDEO_FORMAT_RGB:{
            str = "VIDEO_FORMAT_RGB";
        } break;
        case VIDEO_FORMAT_BGR:{
            str = "VIDEO_FORMAT_BGR";
        } break;
        case VIDEO_FORMAT_Y41B:{
            str = "VIDEO_FORMAT_Y41B";
        } break;
        case VIDEO_FORMAT_Y42B:{
            str = "VIDEO_FORMAT_Y42B";
        } break;
        case VIDEO_FORMAT_YVYU:{
            str = "VIDEO_FORMAT_YVYU";
        } break;
        case VIDEO_FORMAT_Y444:{
            str = "VIDEO_FORMAT_Y444";
        } break;
        case VIDEO_FORMAT_v210:{
            str = "VIDEO_FORMAT_v210";
        } break;
        case VIDEO_FORMAT_v216:{
            str = "VIDEO_FORMAT_v216";
        } break;
        case VIDEO_FORMAT_NV12:{
            str = "VIDEO_FORMAT_NV12";
        } break;
        case VIDEO_FORMAT_NV21:{
            str = "VIDEO_FORMAT_NV21";
        } break;
        case VIDEO_FORMAT_GRAY8:{
            str = "VIDEO_FORMAT_GRAY8";
        } break;
        case VIDEO_FORMAT_GRAY16_BE:{
            str = "VIDEO_FORMAT_GRAY16_BE";
        } break;
        case VIDEO_FORMAT_GRAY16_LE:{
            str = "VIDEO_FORMAT_GRAY16_LE";
        } break;
        case VIDEO_FORMAT_v308:{
            str = "VIDEO_FORMAT_v308";
        } break;
        case VIDEO_FORMAT_RGB16:{
            str = "VIDEO_FORMAT_RGB16";
        } break;
        case VIDEO_FORMAT_BGR16:{
            str = "VIDEO_FORMAT_BGR16";
        } break;
        case VIDEO_FORMAT_RGB15:{
            str = "VIDEO_FORMAT_RGB15";
        } break;
        case VIDEO_FORMAT_BGR15:{
            str = "VIDEO_FORMAT_BGR15";
        } break;
        case VIDEO_FORMAT_UYVP:{
            str = "VIDEO_FORMAT_UYVP";
        } break;
        case VIDEO_FORMAT_A420:{
            str = "VIDEO_FORMAT_A420";
        } break;
        case VIDEO_FORMAT_RGB8P:{
            str = "VIDEO_FORMAT_RGB8P";
        } break;
        case VIDEO_FORMAT_YUV9:{
            str = "VIDEO_FORMAT_YUV9";
        } break;
        case VIDEO_FORMAT_YVU9:{
            str = "VIDEO_FORMAT_YVU9";
        } break;
        case VIDEO_FORMAT_IYU1:{
            str = "VIDEO_FORMAT_IYU1";
        } break;
        case VIDEO_FORMAT_ARGB64:{
            str = "VIDEO_FORMAT_ARGB64";
        } break;
        case VIDEO_FORMAT_AYUV64:{
            str = "VIDEO_FORMAT_AYUV64";
        } break;
        case VIDEO_FORMAT_r210:{
            str = "VIDEO_FORMAT_r210";
        } break;
        case VIDEO_FORMAT_I420_10BE:{
            str = "VIDEO_FORMAT_I420_10BE";
        } break;
        case VIDEO_FORMAT_I420_10LE:{
            str = "VIDEO_FORMAT_I420_10LE";
        } break;
        case VIDEO_FORMAT_I422_10BE:{
            str = "VIDEO_FORMAT_I422_10BE";
        } break;
        case VIDEO_FORMAT_I422_10LE:{
            str = "VIDEO_FORMAT_I422_10LE";
        } break;
        case VIDEO_FORMAT_Y444_10BE:{
            str = "VIDEO_FORMAT_Y444_10BE";
        } break;
        case VIDEO_FORMAT_Y444_10LE:{
            str = "VIDEO_FORMAT_Y444_10LE";
        } break;
        case VIDEO_FORMAT_GBR:{
            str = "VIDEO_FORMAT_GBR";
        } break;
        case VIDEO_FORMAT_GBR_10BE:{
            str = "VIDEO_FORMAT_GBR_10BE";
        } break;
        case VIDEO_FORMAT_GBR_10LE:{
            str = "VIDEO_FORMAT_GBR_10LE";
        } break;
        case VIDEO_FORMAT_NV16:{
            str = "VIDEO_FORMAT_NV16";
        } break;
        case VIDEO_FORMAT_NV24:{
            str = "VIDEO_FORMAT_NV24";
        } break;
        case VIDEO_FORMAT_NV12_64Z32:{
            str = "VIDEO_FORMAT_NV12_64Z32";
        } break;
        case VIDEO_FORMAT_A420_10BE:{
            str = "VIDEO_FORMAT_A420_10BE";
        } break;
        case VIDEO_FORMAT_A420_10LE:{
            str = "VIDEO_FORMAT_A420_10LE";
        } break;
        case VIDEO_FORMAT_A422_10BE:{
            str = "VIDEO_FORMAT_A422_10BE";
        } break;
        case VIDEO_FORMAT_A422_10LE:{
            str = "VIDEO_FORMAT_A422_10LE";
        } break;
        case VIDEO_FORMAT_A444_10BE:{
            str = "VIDEO_FORMAT_A444_10BE";
        } break;
        case VIDEO_FORMAT_A444_10LE:{
            str = "VIDEO_FORMAT_A444_10LE";
        } break;
        case VIDEO_FORMAT_NV61:{
            str = "VIDEO_FORMAT_NV61";
        } break;
        case VIDEO_FORMAT_P010_10BE:{
            str = "VIDEO_FORMAT_P010_10BE";
        } break;
        case VIDEO_FORMAT_P010_10LE:{
            str = "VIDEO_FORMAT_P010_10LE";
        } break;
        case VIDEO_FORMAT_IYU2:{
            str = "VIDEO_FORMAT_IYU2";
        } break;
        case VIDEO_FORMAT_VYUY:{
            str = "VIDEO_FORMAT_VYUY";
        } break;
        case VIDEO_FORMAT_GBRA:{
            str = "VIDEO_FORMAT_GBRA";
        } break;
        case VIDEO_FORMAT_GBRA_10BE:{
            str = "VIDEO_FORMAT_GBRA_10BE";
        } break;
        case VIDEO_FORMAT_GBRA_10LE:{
            str = "VIDEO_FORMAT_GBRA_10LE";
        } break;
        case VIDEO_FORMAT_GBR_12BE:{
            str = "VIDEO_FORMAT_GBR_12BE";
        } break;
        case VIDEO_FORMAT_GBR_12LE:{
            str = "VIDEO_FORMAT_GBR_12LE";
        } break;
        case VIDEO_FORMAT_GBRA_12BE:{
            str = "VIDEO_FORMAT_GBRA_12BE";
        } break;
        case VIDEO_FORMAT_GBRA_12LE:{
            str = "VIDEO_FORMAT_GBRA_12LE";
        } break;
        case VIDEO_FORMAT_I420_12BE:{
            str = "VIDEO_FORMAT_I420_12BE";
        } break;
        case VIDEO_FORMAT_I420_12LE:{
            str = "VIDEO_FORMAT_I420_12LE";
        } break;
        case VIDEO_FORMAT_I422_12BE:{
            str = "VIDEO_FORMAT_I422_12BE";
        } break;
        case VIDEO_FORMAT_I422_12LE:{
            str = "VIDEO_FORMAT_I422_12LE";
        } break;
        case VIDEO_FORMAT_Y444_12BE:{
            str = "VIDEO_FORMAT_Y444_12BE";
        } break;
        case VIDEO_FORMAT_Y444_12LE:{
            str = "VIDEO_FORMAT_Y444_12LE";
        } break;
        case VIDEO_FORMAT_GRAY10_LE32:{
            str = "VIDEO_FORMAT_GRAY10_LE32";
        } break;
        case VIDEO_FORMAT_NV12_10LE32:{
            str = "VIDEO_FORMAT_NV12_10LE32";
        } break;
        case VIDEO_FORMAT_NV16_10LE32:{
            str = "VIDEO_FORMAT_NV16_10LE32";
        } break;
        case VIDEO_FORMAT_NV12_10LE40:{
            str = "VIDEO_FORMAT_NV12_10LE40";
        } break;
        case VIDEO_FORMAT_Y210:{
            str = "VIDEO_FORMAT_Y210";
        } break;
        case VIDEO_FORMAT_Y410:{
            str = "VIDEO_FORMAT_Y410";
        } break;
        case VIDEO_FORMAT_VUYA:{
            str = "VIDEO_FORMAT_VUYA";
        } break;
        case VIDEO_FORMAT_BGR10A2_LE:{
            str = "VIDEO_FORMAT_BGR10A2_LE";
        } break;
        default:
            str = "Unknow";
        break;
    }
    return str;
}

#ifdef  __cplusplus
}
#endif