/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __RENDER_CONFIG_H__
#define __RENDER_CONFIG_H__

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * wait audio timeout Us,if timeout is reached but audio
 * do not anchoring mediasync,use video anchor
 */
#define WAIT_AUDIO_TIME_US 3000000

/*
 * the latency time from wayland client to wayland server display
 * video frame
 */
#define LATENCY_TO_HDMI_TIME_US 48000

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_CONFIG_H__*/