#ifndef __RENDER_CONFIG_H__
#define __RENDER_CONFIG_H__

#ifdef  __cplusplus
extern "C" {
#endif
/**
 * enable mediasync
 */
#define SUPPORT_MEDIASYNC 1

/**
 * enable mediasync tunnel mode
 */
#define MEDIASYNC_TUNNEL_MODE 1

/**
 * recycling use wayland_buffer including wayland_buffer
 * if set 0 will disable recycling. wayland_buffer will
 * destroy wl_buffer,and will create new one when display
 * frame
 */
#define REUSE_WAYLAND_BUFFER 1

/*
 * wait audio timeout ms,if timeout is reached but audio
 * do not anchoring mediasync,use video anchor
 */
#define WAIT_AUDIO_TIME_MS 300 //600 is good

/*
 * the latency time from wayland client to wayland server display
 * video frame
 */
#define LATENCY_TO_HDMI_TIME_US 32000

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_CONFIG_H__*/