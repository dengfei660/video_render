#ifndef __RENDER_CONFIG_H__
#define __RENDER_CONFIG_H__

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * wait audio timeout ms,if timeout is reached but audio
 * do not anchoring mediasync,use video anchor
 */
#define WAIT_AUDIO_TIME_MS 100 //600 is good

/*
 * the latency time from wayland client to wayland server display
 * video frame
 */
#define LATENCY_TO_HDMI_TIME_US 48000

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_CONFIG_H__*/