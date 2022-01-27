/*
 *   TV LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 *   Copyright(c) 2018 by LG Electronics Inc.
 *
 *   All rights reserved. No part of this work may be reproduced, stored in a
 *   retrieval system, or transmitted by any means without prior written
 *   permission of LG Electronics Inc.
 */

#ifndef _V4L2_CONTROLS_EXT_H
#define _V4L2_CONTROLS_EXT_H

#include "linuxtv-ext-ver.h"
#include <linux/v4l2-controls.h>

//
// User-class control Bases
//

#define V4L2_CID_USER_EXT_VSC_BASE (V4L2_CID_USER_BASE + 0x2000)
#define V4L2_CID_USER_EXT_CAPTURE_BASE (V4L2_CID_USER_BASE + 0x2100)
#define V4L2_CID_USER_EXT_PQ_BASE (V4L2_CID_USER_BASE + 0x3000)
#define V4L2_CID_USER_EXT_VBE_BASE (V4L2_CID_USER_BASE + 0x4000)
#define V4L2_CID_USER_EXT_HDMI_BASE (V4L2_CID_USER_BASE + 0x5000)
#define V4L2_CID_USER_EXT_ADC_BASE (V4L2_CID_USER_BASE + 0x6000)
#define V4L2_CID_USER_EXT_AVD_BASE (V4L2_CID_USER_BASE + 0x7000)
#define V4L2_CID_USER_EXT_VDEC_BASE (V4L2_CID_USER_BASE + 0x8000)
// 0x9000 is being used by FE: see v4l2-ext-frontend.h
#define V4L2_CID_USER_EXT_GPSCALER_BASE (V4L2_CID_USER_BASE + 0xA000)
#define V4L2_CID_USER_EXT_EARC_BASE (V4L2_CID_USER_BASE + 0xB000)
#define V4L2_CID_USER_EXT_VBI_BASE (V4L2_CID_USER_BASE + 0xC000)

//
// Scaler class control IDs
//

/**
 * @brief Set video window
 *
 * @rst
 * This will use when video window set. (VSC SetIntputOutputRegion) It will
 * replace previous HAL function, that is, RotateVideo, SetInputRegion,
 * SetOutputRegion.
 *
 * It is a function that will replace by integrating the exisitng
 * RotateVideo/RotateVideo/SetOutputRegion features.
 *
 * Below conditions should be satisfied to implement this function.
 *
 * 1. SetOutputRegion, this function is used for setting video window size. LG
 *    will provide the window position and size (pOutputRegion). Driver should
 *    draw window of the fd.
 *
 * 2. SetInputRegion, this function is used for cropping video. LG will provide
 *    original video size (originalInput) and cropped size (inRegion). Driver
 *    should crop video to over scan or zoom.
 *
 * 3. RotateVideo, this function is used for rotating video to 90', 180', 270',
 *    0'. This should be rotated by clockwise. LG will provide rotate info
 *    (rotation) and rotated window size (pOutputRegion). Driver should draw the
 *    rotated video.
 *
 * To reduce window setting time, we’ll integrate previous HAL functions. To
 * avoid garbage, driver should apply the above settings during v blank. To meet
 * setting time, driver can have to skip many previous window size settings
 * within 1 vsync and apply only latest command.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_WIN_REGION-1.png
 *
 * Skip window size setting about 1,2,3 commands. Only apply window size setting
 * about 4 command. (the above picture ppt file)
 *
 * Driver operation status according to user client parameter value. You must
 * operate according to the table below.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_WIN_REGION-2.png
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_WIN_REGION
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vsc_rotation
 *     {
 *         V4L2_EXT_VSC_ROTATE_0 = 0,
 *         V4L2_EXT_VSC_ROTATE_90,
 *         V4L2_EXT_VSC_ROTATE_180,
 *         V4L2_EXT_VSC_ROTATE_270
 *     };
 *
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;   // horizontal start position
 *         unsigned short y;   // vertical start position
 *         unsigned short w;   // horizontal size(width)
 *         unsigned short h;   // vertical size(height)
 *     };
 *
 *     struct v4l2_ext_vsc_input_region
 *     {
 *         struct v4l2_ext_video_rect  crop; // video input crop size to set
 *         struct v4l2_ext_video_rect  res;  // original source resolution
 *     };
 *
 *     struct v4l2_ext_vsc_win_region
 *     {
 *         struct v4l2_ext_vsc_input_region in;
 *         struct v4l2_ext_video_rect out;         // video output size to set
 *         enum v4l2_ext_vsc_rotation rotation;    // rotation information
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Window Info
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_win_region win{0};
 *
 *     // original input resolution
 *     win.in.res.x = in.res.x;
 *     win.in.res.y = in.res.y;
 *     win.in.res.w = in.res.w;
 *     win.in.res.h = in.res.h;
 *
 *     // cropped input size
 *     win.in.crop.x = in.crop.x;
 *     win.in.crop.y = in.crop.y;
 *     win.in.crop.w = in.crop.w;
 *     win.in.crop.h = in.crop.h;
 *
 *     // output size
 *     win.out.x = out.x;
 *     win.out.y = out.y;
 *     win.out.w = out.w;
 *     win.out.h = out.h;
 *
 *     // rotation info
 *     win.rotation = V4L2_EXT_VSC_ROTATE_0;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_WIN_REGION;
 *     ext_controls.controls->ptr = (void *)&win_info;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Get Window Info
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_win_region win{0};
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_WIN_REGION;
 *     ext_controls.controls->ptr = (void *)&win;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_WIN_REGION (V4L2_CID_USER_EXT_VSC_BASE + 0)

/**
 * @brief freezes or unfreezes the specified video window
 *
 * @rst
 * This function freezes or unfreezes the specified video window of
 * video_device. (VSC SetWinFreeze) Freezing means that the last frame is
 * continuously displayed on the specified video window of video_device.
 *
 * .. important::
 *   Decoding for input source should be kept continuously and
 *   SetInputOutputRegion should also be worked after this function is done.
 *   This function should be frozen after DI H/W block.
 *
 * This function freezes or unfreezes the video windw of specific video
 * device(v412).
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set Window Freeze
 *     VIDIOC_G_CTRL       // Get Window Freeze Status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_FREEZE
 *
 *     //
 *     // control value
 *     //
 *     0 (Off), 1 (On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set VSC Freeze
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_FREEZE;
 *     control.value = 1;      // On
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get Video Window Color
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_VSC_FREEZE;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_FREEZE (V4L2_CID_USER_EXT_VSC_BASE + 1)

/**
 * @brief Set sub window mode
 *
 * @rst
 * The mode of sub window set for displaying setting. (VSC SetSubWinModeEx)
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_WIN_PROP
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vsc_win_mode
 *     {
 *
 *         V4L2_EXT_VSC_WIN_MODE_NONE = 0,
 *         V4L2_EXT_VSC_WIN_MODE_PIP,
 *         V4L2_EXT_VSC_WIN_MODE_PBP
 *     };
 *     // *This is for composition window.
 *     // Initial value of driver should be VSC_SUB_MODE_NONE.
 *
 *     enum v4l2_ext_vsc_mirror_mode
 *     {
 *         V4L2_EXT_VSC_MIRROR_MODE_NONE = 0,
 *         V4L2_EXT_VSC_MIRROR_MODE_ON,
 *         V4L2_EXT_VSC_MIRROR_MODE_OFF
 *     };
 *     // *This is for which source connect to sub window.
 *     // Initial value of driver should be VSC_SUB_CONNECT_NONE.
 *
 *     enum v4l2_ext_vsc_memory_type
 *     {
 *         V4L2_EXT_VSC_MEMORY_TYPE_NONE = 0,
 *         V4L2_EXT_VSC_MEMORY_TYPE_SINGLE,
 *         V4L2_EXT_VSC_MEMORY_TYPE_MULTI
 *     };
 *
 *     struct v4l2_ext_vsc_win_prop
 *     {
 *         enum v4l2_ext_vsc_win_mode win_mode;
 *         enum v4l2_ext_vsc_mirror_mode mirror_mode;
 *         enum v4l2_ext_vsc_memory_type mem_type;
 *     };
 *
 * Explanation for parameters
 *
 *   1. V4L2_EXT_VSC_MEMORY_TYPE_SINGLE in enum v4l2_ext_vsc_memory_type
 *     - You don’t need to allocate memory for sub window. You can get video
 *       data from main video. This parameter is optional. If you have enough
 *       memory, you don`t need this mode.
 *
 *   2. V4L2_EXT_VSC_MEMORY_TYPE_MULTI in enum v4l2_ext_vsc_memory_type
 *     - This parameter is mandatory. You can’t get video data from main
 *       window. You have to allocate memory for sub window.
 *
 *   3. V4L2_EXT_VSC_MIRROR_MODE_ON in enum v4l2_ext_vsc_mirror_mode
 *     - sub window is the same as main input.
 *
 *   4. V4L2_EXT_VSC_MIRROR_MODE_OFF in enum v4l2_ext_vsc_mirror_mode
 *     - It’s different input source between main and sub.
 *
 *   5. V4L2_EXT_VSC_WIN_MODE_PIP in enum v4l2_ext_vsc_win_mode
 *     - Picture in picture,  sub window is overlapped main window.
 *
 *   6. V4L2_EXT_VSC_WIN_MODE_PBP in enum v4l2_ext_vsc_win_mode
 *     - Picture by picture, sub window isn’t overlapped main window.
 *
 *   .. code-block:: cpp
 *
 *     // The parameters of Multi-view Case
 *     subWinMode.mem_type     = V4L2_EXT_VSC_MEMORY_TYPE_MULTI;
 *     subWinMode.mirror_mode  = V4L2_EXT_VSC_MIRROR_MODE_OFF;
 *     subWinMode.win_mode     = V4L2_EXT_VSC_WIN_MODE_PBP;
 *
 *     // The parameters of Overlay Miracast Case
 *     subWinMode.mem_type     = V4L2_EXT_VSC_MEMORY_TYPE_MULTI;
 *     subWinMode.mirror_mode  = V4L2_EXT_VSC_MIRROR_MODE_OFF;
 *     subWinMode.win_mode     = V4L2_EXT_VSC_WIN_MODE_PIP;
 *
 *     // The parameters of Youtube dual video Case
 *     subWinMode.mem_type     = V4L2_EXT_VSC_MEMORY_TYPE_MULTI;
 *     subWinMode.mirror_mode  = V4L2_EXT_VSC_MIRROR_MODE_OFF;
 *     subWinMode.win_mode     = V4L2_EXT_VSC_WIN_MODE_PIP;
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Sub Window mode
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_win_prop win_prop{0};
 *
 *     win_prop.memoryType = VSC_SUB_MEMORY_USE_SINGLE;
 *     win_prop.connectType = VSC_SUB_CONNECT_TYPE_NON_MIRROR;
 *     win_prop.modeType = VSC_SUB_MODE_PBP;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_WIN_PROP;
 *     ext_controls.controls->ptr = (void *)&win_prop;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get Sub Window mode
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_win_prop win_prop{0};
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_WIN_PROP;
 *     ext_controls.controls->ptr = (void *)&win_prop;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_WIN_PROP (V4L2_CID_USER_EXT_VSC_BASE + 2)

/**
 * @brief Set pattern mode
 *
 * @rst
 * This function shows video internal pattern from designated block. (VSC SetPattern)
 *   1. MUX: after Frontend(AVD/ADC/HDMI/VDEC), before video processing(DI/NR)
 *   2. DI/NR: after mux, before scaler
 *   3. Scaler: after video processing(DI/NR), before MEMC
 *   4. MEMC: after scaling, before mixing video with OSD
 *   5. Display: after mixing video with OSD
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_PATTERN-1.png
 *
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_PATTERN
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vsc_pattern
 *     {
 *         V4L2_EXT_VSC_PATTERN_OFF = 0,
 *         V4L2_EXT_VSC_PATTERN_MUX,
 *         V4L2_EXT_VSC_PATTERN_DI_NR,
 *         V4L2_EXT_VSC_PATTERN_SCALER,
 *         V4L2_EXT_VSC_PATTERN_MEMC,
 *         V4L2_EXT_VSC_PATTERN_DISPLAY,
 *         V4L2_EXT_VSC_PATTERN_RESERVED0,
 *         V4L2_EXT_VSC_PATTERN_RESERVED1,
 *         V4L2_EXT_VSC_PATTERN_RESERVED2,
 *         V4L2_EXT_VSC_PATTERN_RESERVED3
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set pattern
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_PATTERN;
 *     control.value = V4L2_EXT_VSC_PATTERN_SCALER;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get pattern
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_PATTERN;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_PATTERN (V4L2_CID_USER_EXT_VSC_BASE + 3)
enum v4l2_ext_vsc_pattern {
    V4L2_EXT_VSC_PATTERN_OFF = 0,
    V4L2_EXT_VSC_PATTERN_MUX,
    V4L2_EXT_VSC_PATTERN_DI_NR,
    V4L2_EXT_VSC_PATTERN_SCALER,
    V4L2_EXT_VSC_PATTERN_MEMC,
    V4L2_EXT_VSC_PATTERN_DISPLAY,
    V4L2_EXT_VSC_PATTERN_RESERVED0,
    V4L2_EXT_VSC_PATTERN_RESERVED1,
    V4L2_EXT_VSC_PATTERN_RESERVED2,
    V4L2_EXT_VSC_PATTERN_RESERVED3
};

/**
 * @brief Set frame delay
 *
 * @rst
 * This function sets delay buffer. Video buffer has to be added to set delay
 * buffer. This is needed to adjust audio lip sync. (VSC SetDelayBuffer)
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_FRAME_DELAY
 *
 *     //
 *     // parameter
 *     //
 *     Delayed frame count
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set pattern
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_FRAME_DELAY;
 *     control.value = 2;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get pattern
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_FRAME_DELAY;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_FRAME_DELAY (V4L2_CID_USER_EXT_VSC_BASE + 4)

/**
 * @brief Set zOrder mode
 *
 * @rst
 * This function sets Z order. (VSC SetZorder) The user client ensures that the
 * v4l2 main device and the v4l2 sub device have the zorder value set
 * sequentially. In the SoC vendor, after setting zorder value of v4l2 main
 * device, when zorder value of v4l2 sub device is set, actually apply zorder of
 * main path and sub path.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_ZORDER
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vsc_zorder
 *     {
 *         unsigned char zorder;   // 0: bottom, 1: bottom+1,..
 *         unsigned char alpha;    // 0 ~ 255
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set zOrder
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_zorder z{0};
 *
 *     z.uZorder= 1;
 *     z.uAlpha= 45;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_ZORDER;
 *     ext_controls.controls->ptr = (void *)&z;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get zOrder
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_zorder z{0};
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_ZORDER;
 *     ext_controls.controls->ptr = (void *)&z;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_ZORDER (V4L2_CID_USER_EXT_VSC_BASE + 5)

/**
 * @brief Set pattern mode
 *
 * @rst
 * We want to know the scaler performance for window setting. (VSC
 * GetLimitedWindow) If SOC can support any video size(there isn’t any
 * limitation in window setting), this function Just returns 0x0. If SOC has
 * some limitation, this function returns the limited value. This function gets
 * the Min/Max that can be scaling setting.
 *
 *   Example)
 *   if Scaler can down scaling by 1/32(horizontal), 1/64(vertical)
 *   And Scaler can up scaling by 24(horizontal), 48(vertical),
 *   h_scalerdown_ratio = 32, v_scalerdown_ratio = 64, h_scaleup_ratio = 24,
 *   v_scaleup_ratio = 48 in VSC_SCALER_RATIO_T structure.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_VSC_LIMITED_WIN_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vsc_scaler_ratio
 *     {
 *         int h_scaleup_ratio;
 *         int v_scaleup_ratio;
 *         int h_scaledown_ratio;
 *         int v_scaledown_ratio;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get limited window
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_scaler_ratio ratio{0};
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_VSC_LIMITED_WIN_INFO;
 *     ext_controls.controls->ptr = (void *)&ratio;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_LIMITED_WIN_RATIO (V4L2_CID_USER_EXT_VSC_BASE + 6)

/**
 * @brief Set pattern mode
 *
 * @rst
 * This function is for measurement of video latency using SoC internal pattern
 * and GPIO port. (VSC SetVideoLatencyPattern) How to measure video latency is
 * like below by using this function.
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_LATENCY_PATTERN-1.png
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_LATENCY_PATTERN-2.png
 *
 * Requirement
 *   1. Generate white/black pattern which is located on video input.
 *   2. The pattern is overlayed on original video scene. Display pattern and video on same time.
 *   3. The pattern is repeated black and white, and syncronized by v-sync
 *   4. A GPIO port generate pulse which is syncronized by v-sync and changed by the pattern color.
 *      ex) When the pattern is black the pulse is low, when the pattern is
 *      white the pulse is changed to high syncronized by v-sync.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_LATENCY_PATTERN
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vsc_zorder
 *     {
 *         unsigned char zorder;   // 0: bottom, 1: bottom+1,..
 *         unsigned char alpha;    // 0 ~ 255
 *     };
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;
 *         unsigned short y;
 *         unsigned short w;
 *         unsigned short h;
 *     };
 *
 *     enum v4l2_ext_vsc_latency_pattern
 *     {
 *         V4L2_EXT_VSC_PATTERN_BLACK = 0,
 *         V4L2_EXT_VSC_PATTERN_WHITE
 *     };
 *
 *     struct v4l2_ext_vsc_latency_pattern_info
 *     {
 *         struct v4l2_ext_video_rect r;
 *         enum v4l2_ext_vsc_latency_pattern p;
 *     };
 *
 *     //
 *     // On/Off
 *     //
 *     // On - w and h are not '0'.
 *     info.r.x = 100;
 *     info.r.y = 100;
 *     info.r.w = 300;
 *     info.r.h = 300;
 *
 *     // Off - w or h is '0'.
 *     info.r.x = 0;
 *     info.r.y = 0;
 *     info.r.w = 0;
 *     info.r.h = 0;
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // On latency pattern
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_vsc_latency_pattern_info info{0};
 *
 *     info.p = V4L2_EXT_VSC_PATTERN_WHITE;
 *     info.r.x = 100;
 *     info.r.y = 100;
 *     info.r.w = 300;
 *     info.r.h = 300;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_LATENCY_PATTERN;
 *     ext_controls.controls->ptr = (void *)&info;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Off latency pattern
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_vsc_latency_pattern_info info{0};
 *
 *     info.p = V4L2_EXT_VSC_PATTERN_WHITE;
 *     info.r.x = 0;
 *     info.r.y = 0;
 *     info.r.w = 0;
 *     info.r.h = 0;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_LATENCY_PATTERN;
 *     ext_controls.controls->ptr = (void *)&info;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_LATENCY_PATTERN (V4L2_CID_USER_EXT_VSC_BASE + 7)

/**
 * @brief set adaptive stream or airplay
 *
 * @rst
 * This function sets '1' when video content is a adaptive stream to work
 * seamless change in SOC driver. (VSC SetAdaptiveStream)
 *
 * This function sets '2' when running airplay to change seamless output region
 * in SOC driver.
 *
 * **Air play mode '2'**
 *
 * When change source resolution seamlessly,
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ADAPTIVE_STREAM-1.png
 *
 * Change output region to the same AR seamlessly.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ADAPTIVE_STREAM-2.png
 *
 * **Calculate Output Region**
 *
 * When adaptive is 2, app will set the output region to 16x9 size. BSP change
 * output region to the correct AR.
 *
 * * ex1) app output region (0, 0, 3840, 2160), source resolution (0, 0, 1080,
 *   1920) => final output region (1312, 0, 1215, 2160)
 *
 *   * w = (2160 / 1920) x 1080 = 1215
 *   * h = 2160
 *   * x = 0 + (3840 - 1215) / 2 = 1312
 *   * y = 0
 *
 * * ex2) app output region (100, 100, 1920, 1080), source resolution (0, 0,
 *   1080, 1080) => final output region (520, 100, 1080, 1080)
 *
 *   * w = 1080
 *   * h = 1080
 *   * x = 100 + (1920 - 1080) / 2 = 520
 *   * y = 100
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ADAPTIVE_STREAM-3.png
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_ADAPTIVE_STREAM
 *
 *     //
 *     // control value
 *     //
 *     0 (Off), 1 (On), 2 (Air play mode)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Adaptive Stream Mode
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_ADAPTIVE_STREAM;
 *     control.value = 1;      // On
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get Adaptive Stream Mode
 *     struct v4l2_control control{0};
 *
 *     control.id = V4L2_CID_EXT_VSC_ADAPTIVE_STREAM;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_ADAPTIVE_STREAM (V4L2_CID_USER_EXT_VSC_BASE + 8)

/**
 * @brief sets RGB 444 Mode
 *
 * @rst
 * This function sets RGB 444 Mode. This should be bypassed for RGB/YUV444
 * format losslessly. (VSC SetRGB444)
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_RGB444
 *
 *     //
 *     // control value
 *     //
 *     0 (Off), 1 (On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set RGB444 Mode
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_RGB444;
 *     control.value = 1;      // On
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get RGB444 Mode
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_RGB444;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_RGB444 (V4L2_CID_USER_EXT_VSC_BASE + 9)

/**
 * @brief connects VDEC output with Video Decoder Output module
 *
 * @rst
 * .. warning::
 *
 *   Not Use
 *
 * This control id connects VDEC output with Video Decoder Output module. (VDO
 * Connect / VDO Disconnect) This is a function for connection operation of
 * Video Decoder Output module which transmits VDEC output Video Data to Scaler.
 * Assume that VDEC 0 is connected by DTV and VDEC 1 is connected by Youtube.
 * Below is connecting VDEC0(DTV) with VDO0 by calling this function.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_VDO_MODE-1.png
 *
 * After that, the scaler can be connected like below by calling
 * :c:macro:`V4L2_CID_EXT_VSC_CONNECT_INFO` function.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_VDO_MODE-2.png
 *
 * If input is changed into Youtube at this time, VDEC0(DTV) could be
 * disconnected by :c:macro:`V4L2_CID_EXT_VSC_VDO_MODE` function. LG application
 * will not call :c:macro:`V4L2_CID_EXT_VSC_CONNECT_INFO` function at this time.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_VDO_MODE-3.png
 *
 * Below is connecting VDEC1(Youtube) with VDO0 by
 * :c:macro:`V4L2_CID_EXT_VSC_VDO_MODE` function. Youtube has to be displayed
 * rightly without disconnecting VSC.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_VDO_MODE-4.png
 *
 * But there are rules like below.
 *
 * 1. The connection between VDO and VSC is fixed like below except mirror mode.
 *    In other words, VDO 0 cannot be connected into VSC1 and VDO 1 cannot be
 *    connected VSC 0.
 *
 *    .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_VDO_MODE-5.png
 *
 * 2. For sub scaler connection and disconnection, LG application always calls
 *    :c:macro:`V4L2_CID_EXT_VSC_VDO_MODE` and
 *    :c:macro:`V4L2_CID_EXT_VSC_CONNECT_INFO` at the same time.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_VDO_MODE
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vsc_vdo_port
 *     {
 *         // VSC_VDO_PORT_NONE means "VDO Disconnect".
 *         V4L2_EXT_VSC_VDO_PORT_NONE = 0,
 *         // The below enum values mean "VDO Connect".
 *         V4L2_EXT_VSC_VDO_PORT_0,
 *         V4L2_EXT_VSC_VDO_PORT_1
 *     };
 *
 *     struct v4l2_ext_vsc_vdo_mode
 *     {
 *         enum v4l2_ext_vsc_vdo_port vdo_port;
 *         unsigned short vdec_port;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set VDO mode
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vsc_vdo_mode mode;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&vdo_mode, 0, sizeof(struct v4l2_ext_vsc_vdo_mode));
 *
 *     mode.vdo_port = V4L2_EXT_VSC_VDO_PORT_0;
 *     mode.vdec_port = 0;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_VDO_MODE;
 *     ext_controls.controls->ptr = (void *)&mode;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Get VDO mode info
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vsc_vdo_mode mode;
 *
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&mode, 0, sizeof(struct v4l2_ext_vsc_vdo_mode));
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_VDO_MODE;
 *     ext_controls.controls->ptr = (void *)&mode;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 * @endrst
 */
#define V4L2_CID_EXT_VSC_VDO_MODE (V4L2_CID_USER_EXT_VSC_BASE + 10)

/**
 * @brief Sets SDR/HDR Type
 *
 * @rst
 * This is a function that sets SDR/HDR type. It operates as HDR_Disconnect when
 * HDR type is SDR, and operates as HDR_Connect when HDR type is others.
 *
 * Commands & Parameters
 *   see :cpp:any:`v4l2_ext_vsc_hdr_type`.
 *
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VSC_HDR_TYPE
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vsc_hdr_type
 *     {
 *         V4L2_EXT_VSC_HDR_TYPE_SDR = 0,
 *         V4L2_EXT_VSC_HDR_TYPE_HDR10,
 *         V4L2_EXT_VSC_HDR_TYPE_DOLBY,
 *         V4L2_EXT_VSC_HDR_TYPE_HLG,
 *         V4L2_EXT_VSC_HDR_TYPE_PRIME,
 *         V4L2_EXT_VSC_HDR_TYPE_DOLBY_LL,
 *         V4L2_EXT_VSC_HDR_TYPE_DOLBY_RF,
 *         V4L2_EXT_VSC_HDR_TYPE_HDR_RESERVED2,
 *         V4L2_EXT_VSC_HDR_TYPE_HDR_RESERVED3
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set HDR Type
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_HDR_TYPE;
 *     control.value = V4L2_EXT_VSC_HDR_TYPE_HDR10;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get HDR Type
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_HDR_TYPE;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_HDR_TYPE (V4L2_CID_USER_EXT_VSC_BASE + 11)

enum v4l2_ext_vsc_hdr_type {
    V4L2_EXT_VSC_HDR_TYPE_SDR = 0,
    V4L2_EXT_VSC_HDR_TYPE_HDR10,
    V4L2_EXT_VSC_HDR_TYPE_DOLBY,
    V4L2_EXT_VSC_HDR_TYPE_HLG,
    V4L2_EXT_VSC_HDR_TYPE_PRIME,
    V4L2_EXT_VSC_HDR_TYPE_DOLBY_LL,
    V4L2_EXT_VSC_HDR_TYPE_DOLBY_RF,
    V4L2_EXT_VSC_HDR_TYPE_HDR_RESERVED2,
    V4L2_EXT_VSC_HDR_TYPE_HDR_RESERVED3
};

/**
 * @brief Connects VFE(Video Front End)
 *
 * @rst
 * This function connects VFE(Video Front End) module by input and sets output
 * mode for a scaler and disconnects VFE module by input and releases output
 * mode for a scaler. (VSC Connect / VSC Disconnect)
 * Black image should be displayed when input type is
 * :cpp:any:`V4L2_EXT_VSC_INPUT_SRC_NONE` or output type is
 * :cpp:any:`V4L2_EXT_VSC_DEST_NONE`.
 *
 * Please refer below Application Example.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_CONNECT_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vsc_connect_info
 *     {
 *         struct v4l2_ext_vsc_input_src_info in;
 *         enum v4l2_ext_vsc_dest out;
 *     };
 *
 *     struct v4l2_ext_vsc_input_src_info
 *     {
 *         enum v4l2_ext_vsc_input_src src;
 *         unsigned char index;
 *         unsigned char attr;  //ATV : attr = 0, AV : attr = 1
 *     };
 *
 *     enum v4l2_ext_vsc_input_src
 *     {
 *         // V4L2_EXT_VSC_INPUT_SRC_NONE means "VSC Disconnect".
 *         V4L2_EXT_VSC_INPUT_SRC_NONE = 0,
 *         // The below enum values mean "VSC Connect".
 *         V4L2_EXT_VSC_INPUT_SRC_AVD,    //ATV : attr = 0, AV : attr = 1
 *         V4L2_EXT_VSC_INPUT_SRC_ADC,
 *         V4L2_EXT_VSC_INPUT_SRC_HDMI,
 *         V4L2_EXT_VSC_INPUT_SRC_VDEC,
 *         V4L2_EXT_VSC_INPUT_SRC_JPEG
 *         V4L2_EXT_VSC_INPUT_SRC_FB      //Miracast TX
 *     };
 *
 *     enum v4l2_ext_vsc_dest
 *     {
 *         // V4L2_EXT_VSC_DEST_NONE means "VSC Disconnect".
 *         V4L2_EXT_VSC_DEST_NONE = 0,
 *         // The below enum values mean "VSC Connect".
 *         V4L2_EXT_VSC_DEST_DISPLAY,  // DISPLAY (ex. PIP, Live Zoom)
 *         V4L2_EXT_VSC_DEST_VENC,     // VENC (ex. Analog Recording)
 *         V4L2_EXT_VSC_DEST_MEMORY,   // MEMORY (ex. Capture)
 *         V4L2_EXT_VSC_DEST_AVE
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :cpp:any:`VIDIOC_S_EXT_CTRLS` and this control id.
 *
 *   Application can get data using :cpp:any:`VIDIOC_G_EXT_CTRLS` and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Connect
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_connect_info info{0};
 *
 *     info.in.src = V4L2_VSC_EXT_INPUT_SRC_HDMI;
 *     info.in.index = 2;
 *     info.out_mode = V4L2_EXT_VSC_DEST_DISPLAY;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_CONNECT_INFO;
 *     ext_controls.controls->ptr = (void *)&info;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Disconnect
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_connect_info info{0};
 *
 *     info.in.src = V4L2_VSC_EXT_INPUT_SRC_NONE;
 *     info.out = V4L2_EXT_VSC_DEST_NONE;
 *
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_CONNECT_INFO;
 *     ext_controls.controls->ptr = (void *)&info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 * @endrst
 */
#define V4L2_CID_EXT_VSC_CONNECT_INFO (V4L2_CID_USER_EXT_VSC_BASE + 12)

/**
 * @brief Get active window data
 *
 * @rst
 * This function registers active video window callback function. (VSC
 * RegisterActiveWindowCallback) The video window dimension is an output frame
 * from video decoder and is not cropped. Active video window means area with
 * real video area from pillar box or letter box video image which includes
 * black bars. Usually, active area might be 16:9,16:10,4:3 or 90 degreee
 * rotated of them. But it is acceptable to return dimension that does not fit
 * this aspec ratio. LG Application will detect aspect ratio and rotation using
 * the active area size. This is a function that register Active Video Window
 * Callback.
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO.png
 *
 * When there are two video objects like above picture, that is, red cirle and
 * yellow rectange, Original video window is black rectangle. Active video
 * window detected will be blue rectangle. To calculate active area, assume that
 * inactive area is filled with black color. It should not be assumed that
 * active area is center aligned in the screen. If there's a performance issue,
 * assume active window's height is the same with video window height.
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 *   - This command works in pairs with V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN.
 *     Please refer to the command of V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *     VIDIOC_DQEVENT
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vsc_active_win_info
 *     {
 *         struct v4l2_ext_video_rect original;
 *         struct v4l2_ext_video_rect active;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // After epoll event occurs
 *     struct v4l2_event ev{0};
 *
 *     ioctl(fd, VIDIOC_DQEVENT, &ev);
 *
 *     if(ev.id == V4L2_CID_VSC_SUBSCRIBE_MUTE_OFF)
 *     {
 *         struct v4l2_ext_controls ext_controls{0};
 *         struct v4l2_ext_control ext_control{0};
 *         struct v4l2_ext_vsc_active_win_info info{0};
 *
 *         ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *         ext_controls.count = 1;
 *         ext_controls.controls = &ext_control;
 *         ext_controls.controls->id = V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO;
 *         ext_controls.controls->ptr = (void *)&info;
 *
 *         ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *     }
 * @endrst
 */
#define V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO (V4L2_CID_USER_EXT_VSC_BASE + 13)

/**
 * @brief Control freeze mode
 *
 * @rst
 * This function freezes or unfreezes Input video frame buffer for getting video
 * data.  (VSC_FreezeVideoFrameBuffer) This is used by ADC & Self-Diagnostics
 * and video data should be frozen before start ADC & Self-Diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VSC_FREEZE_FRAME_BUFFER
 *
 *     //
 *     // parameter
 *     //
 *     0 (Off), 1 (On);
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Freeze Buffer
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_FREEZE_FRAME_BUFFER;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get Freeze Buffer
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_FREEZE_FRAME_BUFFER;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_FREEZE_FRAME_BUFFER (V4L2_CID_USER_EXT_VSC_BASE + 14)

/**
 * @brief Read frame information
 *
 * @rst
 * This function reads input video frame buffer for checking video data. This is
 * used by ADC & Self-Diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VSC_READ_FRAME_BUFFER_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;
 *         unsigned short y;
 *         unsigned short w;
 *         unsigned short h;
 *     };
 *     struct v4l2_ext_vsc_color_pixel_data
 *     {
 *         unsigned int y;    // standard pixel color Y
 *         unsigned int cb;   // standard pixel color Cb
 *         unsigned int cr;   // standard pixel color Cr
 *     };
 *
 *     enum v4l2_ext_vsc_color_pixel_format
 *     {
 *         V4L2_EXT_VSC_COLOR_PIXEL_FORMAT_YUV = 0,
 *         V4L2_EXT_VSC_COLOR_PIXEL_FORMAT_RGB
 *     };
 *
 *     enum v4l2_ext_vsc_color_pixel_depth
 *     {
 *         V4L2_EXT_VSC_COLOR_PIXEL_8BIT = 0,
 *         V4L2_EXT_VSC_COLOR_PIXEL_10BIT
 *     };
 *
 *     struct v4l2_ext_vsc_pixel_color_info
 *     {
 *         struct v4l2_ext_video_rect r;
 *         union {
 *             struct v4l2_ext_vsc_color_pixel_data *p_data;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *         enum v4l2_ext_vsc_color_pixel_format format;
 *         enum v4l2_ext_vsc_color_pixel_depth depth;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Color Pixel info
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_pixel_color_info info{0};
 *
 *     info.p_data = (struct v4l2_ext_vsc_color_pixel_data*)malloc  \
 *                   (GRAB_SIZE * sizeof(struct v4l2_ext_vsc_color_pixel_data));
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VSC_READ_FRAME_BUFFER_INFO;
 *     ext_controls.controls->ptr = (void *)&info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 * @endrst
 */
#define V4L2_CID_EXT_VSC_READ_FRAME_BUFFER_INFO                                \
    (V4L2_CID_USER_EXT_VSC_BASE + 15)

/**
 * @brief Set occupation of another path
 *
 * @rst
 * This function is used to send HFR information to soc. Some of SoC need to use
 * 2 video path for displaying HFR source(over 2K resolution). We'll enable HFR
 * mode when we detect HFR mode. (VDEC/Gstreamer will send HFR information to
 * VSC.)
 *
 *   - This control id is used for the video device(#30) of main scaler only.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VSC_OCCUPATION_SUB_SCALER
 *
 *     //
 *     // parameter
 *     //
 *     0 (Off)
 *     1 (On) : Use 2 video path (both main and sub) for displaying HFR source.
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set data
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_OCCUPATION_SUB_SCALER;
 *     control.value = 1 or 0; // 1 : on, 0 : off
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 *     // Get data
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VSC_OCCUPATION_SUB_SCALER;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_OCCUPATION_SUB_SCALER (V4L2_CID_USER_EXT_VSC_BASE + 16)

/**
 * @brief Set video/OSD input crop window and output window regiond for Orbit
 *
 * @rst
 * This function is used to send input crop, output window size information for Orbit.
 * When this is called, driver needs to do:
 *
 *   * | Crop video/OSD input window properly by given input crop window information.
 *   * | Scale up or down cropped video/OSD to given output window size.
 *     | Note that this output window size is not just displayed on the screen,
 *     | but the actual display area will be determined by the :c:macro:`V4L2_CID_EXT_VSC_ORBIT_MOVE`.
 *   * | Parameters
 *     | video_input/osd_input : crop size of original input source.
 *     | video_output/osd_output : scaling size of video_crop/osd_crop.
 *   * | Return error when driver can't do above for some reasons such as Bandwidth, HW limitation.
 *
 * To distinguish Orbit justscan and overscan modes, driver can use crop/output window size values.
 *
 * Orbit justscan case :
 *
 *   * | video_input/osd_input(crop sizes) are same with original sources.
 *     | And output window sizes will be same as screen(panel) size.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-1.png
 *
 * Orbit overscan case :
 *
 *   * | video_input is smaller than original source.
 *     | video_input is bigger than crop of :c:macro:`V4L2_CID_EXT_VSC_WIN_REGION`.
 *     | osd_input is smaller than original source.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-3.png
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-4.png
 *
 * Instructions
 *   * | Video windows(usually called Main/Sub) will be distinguished by device fd,
 *     | :c:macro:`V4L2_EXT_DEV_PATH_SCALER0` and :c:macro:`V4L2_EXT_DEV_PATH_SCALER1`.
 *   * | OSD will be controlled by only :c:macro:`V4L2_EXT_DEV_PATH_SCALER0`.
 *     | Driver should ingnore any OSD related parameters given in other device fd.
 *   * | Driver ignore parameters with the same value as previous,
 *     | except when new :c:macro:`V4L2_CID_EXT_VSC_WIN_REGION` is called.
 *     | For example, if OSD input crop window is same but video input crop window is changed,
 *     | driver needs to ignore OSD parameters and only change video crop window.
 *   * | This should be called together with new :c:macro:`V4L2_CID_EXT_VSC_WIN_REGION`.
 *     | If Orbit feature is on in current machine.
 *   * | Width and height value should be even number always.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VSC_ORBIT_WINDOW
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;
 *         unsigned short y;
 *         unsigned short w;
 *         unsigned short h;
 *     };
 *
 *     struct v4l2_ext_vsc_orbit_window {
 *         struct v4l2_ext_video_rect osd_input;
 *         struct v4l2_ext_video_rect video_input;
 *         struct v4l2_ext_video_rect osd_output;
 *         struct v4l2_ext_video_rect video_output;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Orbit justscan
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_orbit_window orbit_window{0};
 *
 *     osd_input.x = 0;
 *     osd_input.y = 0;
 *     osd_input.w = 1920;
 *     osd_input.h = 1080;
 *
 *     video_input.x = 0;
 *     video_input.y = 0;
 *     video_input.w = 1920;
 *     video_input.h = 1080;
 *
 *     osd_output.x = 0;
 *     osd_output.y = 0;
 *     osd_output.w = 3840;
 *     osd_output.h = 2160;
 *
 *     video_output.x = 0;
 *     video_output.y = 0;
 *     video_output.w = 3840;
 *     video_output.h = 2160;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count         = 1;
 *     ext_controls.controls      = &ext_control;
 *     ext_controls.controls->id  = V4L2_CID_EXT_VSC_ORBIT_WINDOW;
 *     ext_controls.controls->ptr = (void *)&orbit_window;
 *
 *     // Orbit overscan
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_orbit_window orbit_window{0};
 *
 *     osd_input.x = 0;
 *     osd_input.y = 0;
 *     osd_input.w = 1908;
 *     osd_input.h = 1074;
 *
 *     video_input.x = 0;
 *     video_input.y = 0;
 *     video_input.w = 1860;
 *     video_input.h = 1044;
 *
 *     osd_output.x = 0;
 *     osd_output.y = 0;
 *     osd_output.w = 3860;
 *     osd_output.h = 2170;
 *
 *     video_output.x = 0;
 *     video_output.y = 0;
 *     video_output.w = 3872;
 *     video_output.h = 2176;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count         = 1;
 *     ext_controls.controls      = &ext_control;
 *     ext_controls.controls->id  = V4L2_CID_EXT_VSC_ORBIT_WINDOW;
 *     ext_controls.controls->ptr = (void *)&orbit_window;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_ORBIT_WINDOW (V4L2_CID_USER_EXT_VSC_BASE + 17)

/**
 * @brief Set video/OSD input crop window and output window regiond for Orbit
 *
 * @rst
 * This function is used to send display window information for Orbit.
 * When this is called, driver needs to do:
 *
 *   * | Move display window on the scaled input window corresponding to given parameters.
 *   * | Parameters
 *     | video_input/osd_input : the part of video/OSD area to be actually displayed
 *     | of video_scale/osd_scale set by :c:macro:`V4L2_EXT_DEV_PATH_SCALER0`.
 *     | video_output/osd_output : the position and width/height where
 *     | video_input/osd_input will be shown on the screen(panel).
 *   * | Return error when driver can't do above for some reasons such as Bandwidth, HW limitation.
 *
 * Orbit justscan case :
 *
 *   * | Scaled video/OSD is moving on the screen.
 *   * | As Orbit moves, some lines of scaled input can be out of screen(panel).
 *   * | As Orbit moves, some lines of screen can be blank and filled with black video.
 *   * | Check below sample images as examples.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-1.png
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-2.png
 *
 * Orbit overscan case :
 *
 *   * | Display area moves on the scaled input video/OSD.
 *   * | As Orbit moves, input video/OSD area will be changed.
 *   * | There is no blank area in screen, always full size will be shown.
 *
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-3.png
 * .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ORBIT-4.png
 *
 * Instructions
 *   * | Video and OSD can be moved at the same time or at different times.
 *     | Because they can have different time period for their each step.
 *   * | Driver ignore parameters with the same value as previous,
 *     | For example, if only video is moving in current step, OSD parameters will be the same as previous.
 *   * | OSD will be controlled by only :c:macro:`V4L2_EXT_DEV_PATH_SCALER0`.
 *     | Driver should ingnore any OSD related parameters given in other device fd.
 *   * | Driver does not scale up/down by this CID.
 *     | If displaying area is not enough to show video/OSD,
 *     | driver should cut the video or OSD out of display area rather than scaling up/down.
 *     | Check 'Orbit justscan case' as an example.
 *   * | Width and height value should be even number always.
 *   * | :c:macro:`V4L2_CID_EXT_VSC_ORBIT_WINDOW` should be called before this.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VSC_ORBIT_MOVE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;
 *         unsigned short y;
 *         unsigned short w;
 *         unsigned short h;
 *     };
 *
 *     struct v4l2_ext_vsc_orbit_move {
 *         struct v4l2_ext_video_rect osd_input;
 *         struct v4l2_ext_video_rect video_input;
 *         struct v4l2_ext_video_rect osd_output;
 *         struct v4l2_ext_video_rect video_output;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Orbit move
 *     struct v4l2_ext_controls ext_controls{0};
 *     struct v4l2_ext_control ext_control{0};
 *     struct v4l2_ext_vsc_orbit_move orbit_move{0};
 *
 *     osd_input.x = 0;
 *     osd_input.y = 0;
 *     osd_input.w = 3839;
 *     osd_input.h = 2159;
 *
 *     video_input.x = 0;
 *     video_input.y = 0;
 *     video_input.w = 3839;
 *     video_input.h = 2159;
 *
 *     osd_output.x = 1;
 *     osd_output.y = 1;
 *     osd_output.w = 3839;
 *     osd_output.h = 2159;
 *
 *     video_output.x = 1;
 *     video_output.y = 1;
 *     video_output.w = 3839;
 *     video_output.h = 2159;
 *
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count         = 1;
 *     ext_controls.controls      = &ext_control;
 *     ext_controls.controls->id  = V4L2_CID_EXT_VSC_ORBIT_MOVE;
 *     ext_controls.controls->ptr = (void *)&orbit_move;
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_ORBIT_MOVE (V4L2_CID_USER_EXT_VSC_BASE + 18)

//
// VSC class subscription IDs
//

/**
 * @brief Notify mute off
 *
 * @rst
 * This function is used to check time that internal mute is turned off. (VSC
 * RegisterMuteOffCallBack) Driver should make the EVENT of file_handler when
 * driver internal mute is turned off. This relatives with "V4L2_CID_BG_COLOR".
 * This is a function that registers callback function to know the driver side
 * interanl mute off time.
 *
 *   - After the event of "V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF" is registered by
 *     the command of "VIDIOC_SUBSCRIBE_EVENT" in driver, driver should generate
 *     the event of "V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF" whenever the internal
 *     mute is turned off.
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF;
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF;
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_MUTE_OFF (V4L2_CID_USER_EXT_VSC_BASE + 30)

/**
 * @brief Register/Unregister active window event
 *
 * @rst
 * This function registers active video window callback function. (VSC
 * RegisterActiveWindowCallback) The video window dimension is an output frame
 * from video decoder and is not cropped. Active video window means area with
 * real video area from pillar box or letter box video image which includes
 * black bars. Usually, active area might be 16:9,16:10,4:3 or 90 degreee
 * rotated of them. But it is acceptable to return dimension that does not fit
 * this aspec ratio. LG Application will detect aspect ratio and rotation using
 * the active area size. This is a function that register Active Video Window
 * Callback.
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO.png
 *
 * When there are two video objects like above picture, that is, red cirle and
 * yellow rectange, Original video window is black rectangle. Active video
 * window detected will be blue rectangle. To calculate active area, assume that
 * inactive area is filled with black color. It should not be assumed that
 * active area is center aligned in the screen. If there's a performance issue,
 * assume active window's height is the same with video window height.
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 *   - This command works in pairs with V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO.
 *     Please refer to the command of V4L2_CID_EXT_VSC_ACTIVE_WIN_INFO.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN;
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN;
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_ACTIVE_WIN (V4L2_CID_USER_EXT_VSC_BASE + 32)
/**
 * @brief Do not use. We will delete this command soon.
 * @endrst
 */
#define V4L2_CID_EXT_VSC_CLEAR_SUBSCRIBE_MUTE_OFF                              \
    (V4L2_CID_USER_EXT_VSC_BASE + 31)

/**
 * @brief Do not use. We will delete this command soon.
 * @endrst
 */
#define V4L2_CID_EXT_VSC_CLEAR_SUBSCRIBE_ACTIVE_WIN                            \
    (V4L2_CID_USER_EXT_VSC_BASE + 33)

/**
 * @brief Get video size and delay
 *
 * @rst
 * Delay occurs due to video buffers which exist to enhance the picture quality
 * on a video path. The video is shown slower than the OSD border due to the
 * delay. We cannot perfectly remove video delay to match between video and OSD,
 * so we will add the delay to OSD punch out. To add the delay to OSD punch out,
 * application need to know the video delay size. Driver should send the delay
 * time between scaling and OSD mixing to webOS thru this callback function.
 * For a requirement, we will add the delay to OSD punch out to match between
 * video and OSD. To add the delay to OSD punch out, application need to know
 * the video delay size. (HAL_VSC_RegisterWindowSettingDelayCallback()). The
 * registered kernel event should be generated when driver apply window setting
 * physically to H/W as shown below. (EX, 4 and 5 setting of purple color).
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION-1.png
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION-2.png
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *     VIDIOC_DQEVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION
 *
 *     //
 *     // Parameter
 *     //
 *     // Use data[64] in the struct v4l2_event in videodev2.h
 *     // This struct is used by VIDIOC_DQEVENT.
 *     struct v4l2_event {
 *         __u32               type;
 *         union {
 *             struct v4l2_event_vsync     vsync;
 *             struct v4l2_event_ctrl      ctrl;
 *             struct v4l2_event_frame_sync    frame_sync;
 *             struct v4l2_event_src_change    src_change;
 *             struct v4l2_event_motion_det    motion_det;
 *             __u8                data[64];
 *         } u;
 *         __u32               pending;
 *         __u32               sequence;
 *         struct timespec         timestamp;
 *         __u32               id;
 *         __u32               reserved[8];
 *     };
 *
 *     // data[0] - window id
 *     // data[1~2] - the position of x
 *     // data[3~4] - the position of y
 *     // data[5~6] - the size of w
 *     // data[7~8] - the size of h
 *     // data[9~10] - delay time
 *     // Please refer to the shown table with [Table-1] Data type and data order
 *
 * [Table-1] Data type and data order
 *
 *   .. image:: /v4l2/scaler-V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION-3.png
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT/VIDIOC_DQEVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION;
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION;
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 *     // Get data
 *     ioctl(fd, VIDIOC_DQEVENT, &ev);
 *
 *      if(ev.id == V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION)
 *      {
 *          // get information
 *          // ev.u.data[0] ~ ev.u.data[10];
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_APPLYING_DONE_WINDOW_REGION                 \
    (V4L2_CID_USER_EXT_VSC_BASE + 34)

/**
 * @brief Do not use.
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_ADAPTIVE_STREAM_AR_INFORMATION              \
    (V4L2_CID_USER_EXT_VSC_BASE + 35)

/**
 * @brief Get bsp error
 *
 * @rst
 * To get error information whenever BSP Scaler driver detect error status for
 * debugging
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *     VIDIOC_DQEVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VSC_SUBSCRIBE_BSP_ERROR
 *
 *     //
 *     // Parameter
 *     //
 *     // Use data[64] in the struct v4l2_event in videodev2.h
 *     // This struct is used by VIDIOC_DQEVENT.
 *     struct v4l2_event {
 *         __u32               type;
 *         union {
 *             struct v4l2_event_vsync     vsync;
 *             struct v4l2_event_ctrl      ctrl;
 *             struct v4l2_event_frame_sync    frame_sync;
 *             struct v4l2_event_src_change    src_change;
 *             struct v4l2_event_motion_det    motion_det;
 *             __u8                data[64];
 *         } u;
 *         __u32               pending;
 *         __u32               sequence;
 *         struct timespec         timestamp;
 *         __u32               id;
 *         __u32               reserved[8];
 *     };
 *
 *     data[64] - error description string(Maximum string size 63)
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT/VIDIOC_DQEVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_BSP_ERROR
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_BSP_ERROR
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 *     // Get data
 *     ioctl(fd, VIDIOC_DQEVENT, &ev);
 *
 *      if(ev.id == V4L2_CID_EXT_VSC_SUBSCRIBE_BSP_ERROR)
 *      {
 *          // get error description string data[64];
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_BSP_ERROR              \
    (V4L2_CID_USER_EXT_VSC_BASE + 36)

/**
 * @brief get actual video delay from scaler input to SoC output
 *
 * @rst
 * LG want to get video delay value from scaler input to SoC output whenever video delay value is changed.
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 * If delay value is often or always changed because comparing value is small.
 * So this behavior can be affected to CPU performance. You can adjust the comparing value.
 *
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *     VIDIOC_DQEVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VSC_SUBSCRIBE_VIDEO_DELAY
 *
 *     //
 *     // Parameter
 *     //
 *     // Use data[64] in the struct v4l2_event in videodev2.h
 *     // This struct is used by VIDIOC_DQEVENT.
 *     struct v4l2_event {
 *         __u32               type;
 *         union {
 *             struct v4l2_event_vsync     vsync;
 *             struct v4l2_event_ctrl      ctrl;
 *             struct v4l2_event_frame_sync    frame_sync;
 *             struct v4l2_event_src_change    src_change;
 *             struct v4l2_event_motion_det    motion_det;
 *             __u8                data[64];
 *         } u;
 *         __u32               pending;
 *         __u32               sequence;
 *         struct timespec         timestamp;
 *         __u32               id;
 *         __u32               reserved[8];
 *     };
 *
 *     // data[0] - window id
 * 					decimal, 7~0bit(LSB)
 *					0 : #define V4L2_EXT_DEV_PATH_SCALER0 "/dev/video30"
 *					1 : #define V4L2_EXT_DEV_PATH_SCALER1 "/dev/video31"
 *					2 : #define V4L2_EXT_DEV_PATH_SCALER2 "/dev/video32"
 *					3 : #define V4L2_EXT_DEV_PATH_SCALER3 "/dev/video33"
 * 		//data[1] - delay time
 *					decimal , lower 8bits, 7~0bit(LSB)
 *		//data[2] - delay time
 *					decimal , upper 8bits , (MSB)15~8bit
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_VIDEO_DELAY;
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *		// thread control part
 *		struct v4l2_event ev;
 *		// Create thread that handles each event
 *		for (;;)
 *		{
 *			int16 delay_time = 0;
 *			epoll();
 *			ioctl(fd, VIDIOC_DQEVENT, &ev);
 *		    if(ev.id == V4L2_CID_EXT_VSC_SUBSCRIBE_VIDEO_DELAY)
 *		    {
 *		        // get information
 *		        window_id = ev.u.data[0];
 *		        delay_time = ev.u.data[1] + (ev.u.data[2] << 8);
 *		    }
 *   	}
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VSC_SUBSCRIBE_VIDEO_DELAY;
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VSC_SUBSCRIBE_VIDEO_DELAY              \
   (V4L2_CID_USER_EXT_VSC_BASE + 37)

//
// VSC class subscription types
//

#define V4L2_EVENT_PRIVATE_EXT_VSC_BASE (V4L2_EVENT_PRIVATE_START + 0x2000)
#define V4L2_EVENT_PRIVATE_EXT_VSC_EVENT (V4L2_EVENT_PRIVATE_EXT_VSC_BASE + 1)

/* PQ class control IDs */
#define V4L2_CID_EXT_LED_BASE (V4L2_CID_USER_EXT_PQ_BASE)

/**
 * @brief
 *   Control local dimming level by LUT index.
 *
 * @rst
 * Description
 *   This function Initialize LED block and is called once when the system is
 *   power on. LED block includes local dimming and (depends on whether SoC
 *   supporting or not.)
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_INIT
 *
 *     //
 *     // parameter
 *     //
 *     stPanelInfo          [IN]      v4l2_ext_vpq_cmn_data
 *     cf... (struct v4l2_ext_led_panel_info *)stPanelInfo.p_data
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Remarks
 *   useInterLocalDimming : This is for LED block of platform with external
 *   local dimming IC. A specific platform can use external local dimming IC
 *   even if SoC has it.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // LED initialize
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_vpq_cmn_data stPqContainer;
 *     struct v4l2_ext_led_panel_info stRealInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.panel_inch;  = inchType;
 *     stRealInfo.backlight_type = ledType;
 *     stRealInfo.bar_type = barType;
 *     stRealInfo.module_maker = moduleMaker;
 *     stRealInfo.local_dim_ic_type = icType;
 *     stRealInfo.panel_type = pnelType;
 *
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_INIT;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_INIT (V4L2_CID_EXT_LED_BASE + 0)

/**
 * @brief
 *   Control local dimming level by lut index
 *
 * @rst
 * Description
 *   This function control local dimming level by lut index.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL // Set PQ Data
 *     VIDIOC_G_CTRL // Get VDO info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_DB_IDX
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_led_ui_adj_type
 *     {
 *       V4L2_EXT_LED_UI_ADJ_OFF        = 0x00,
 *       V4L2_EXT_LED_UI_ADJ_SDR_LOW    = 0x01,
 *       V4L2_EXT_LED_UI_ADJ_SDR_MEDIUM = 0x02,
 *       V4L2_EXT_LED_UI_ADJ_SDR_HIGH   = 0x03,
 *       V4L2_EXT_LED_UI_ADJ_HDR_LOW    = 0x04,
 *       V4L2_EXT_LED_UI_ADJ_HDR_MEDIUM = 0x05,
 *       V4L2_EXT_LED_UI_ADJ_HDR_HIGH   = 0x06,
 *       V4L2_EXT_LED_UI_ADJ_HDR_PAT    = 0x07,
 *       V4L2_EXT_LED_UI_ADJ_MAX
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // LED Set LUT
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_LED_LUT;
 *     control.value = uiIndex;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *     // LED Get LUT
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_LED_DB_IDX;
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * See Also
 *   VPQ LOCALDIMMING DB DATA
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_DB_IDX (V4L2_CID_EXT_LED_BASE + 1)

/**
 * @brief
 *   Control local dimming demo mode
 *
 * @rst
 * Description
 *   This function control local dimming demo mode. When demo mode on, LED is
 *   on/off sequentially
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_DEMOMODE
 *
 *     //
 *     // parameter
 *     //
 *     stModeInfo          [IN]      v4l2_ext_vpq_cmn_data
 *     cf... (struct v4l2_ext_led_ldim_demo_info *)stPanelInfo.p_data
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // LED demo mode
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_vpq_cmn_data stPqContainer;
 *     struct v4l2_ext_led_ldim_demo_info stRealInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.bOnOff = TRUE;
 *     stRealInfo.eType = v4l2_ext_led_ldim_demo_type_linedemo;
 *     ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_DEMOMODE;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_DEMOMODE (V4L2_CID_EXT_LED_BASE + 2)

/**
 * @brief
 *   Control local dimming block on/off
 *
 * @rst
 * Description
 *   This function control local dimming block on/off.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *     VIDIOC_G_CTRL        // Get VDO info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_EN
 *
 *     //
 *     // control value
 *     //
 *     0(Off), 1(On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set LED Local Dimming Enable
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_LED_EN;
 *     control.value = true;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_EN (V4L2_CID_EXT_LED_BASE + 3)

/**
 * @brief
 *   Uninitialize Local Dimming
 *
 * @rst
 * Description
 *   Uninitialize Local Dimming and use for debug
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_FIN
 *
 *     //
 *     // parameter
 *     //
 *     void
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Finalize LED Local Dimming
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_LED_FIN;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_FIN (V4L2_CID_EXT_LED_BASE + 4)

/**
 * @brief
 *   Set Local Dimming DB Look up table
 *
 * @rst
 * Description
 *   Set Local Dimming DB Look up table
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_DB_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_ext_vpq_cmn_data
 *     cf... (struct v4l2_led_lut *)stContainer.p_data
 *
 *     // Local dimming LUT Data
 *     enum v4l2_ext_led_ui_adj_type
 *     {
 *       V4L2_EXT_LED_UI_ADJ_OFF    = 0x00,
 *       V4L2_EXT_LED_UI_ADJ_SDR_LOW    = 0x01,
 *       V4L2_EXT_LED_UI_ADJ_SDR_MEDIUM = 0x02,
 *       V4L2_EXT_LED_UI_ADJ_SDR_HIGH   = 0x03,
 *       V4L2_EXT_LED_UI_ADJ_HDR_LOW    = 0x04,
 *       V4L2_EXT_LED_UI_ADJ_HDR_MEDIUM = 0x05,
 *       V4L2_EXT_LED_UI_ADJ_HDR_HIGH   = 0x06,
 *       V4L2_EXT_LED_UI_ADJ_HDR_PAT    = 0x06,
 *       V4L2_EXT_LED_UI_ADJ_MAX
 *     };
 *     struct v4l2_led_db_info {
 *        unsigned int size;
 *        union {
 *             unsigned int *pData;
 *             unsigned int compat_pData;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set LED DB Data
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_vpq_cmn_data stPqContainer;
 *     struct v4l2_led_lut stRealInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.lutVersion = 0x01;
 *     ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_DB_DATA;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_DB_DATA (V4L2_CID_EXT_LED_BASE + 5)

/**
 * @brief
 *   Control SPI command bit
 *
 * @rst
 * Description
 *   Control SPI command bit connected to LED driver
 *
 *   If it set Clear Plus/BPL/Store Mode/etc LED driver specifics, use this
 *   function.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_CONTROL_SPI
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_ext_vpq_cmn_data
 *     cf... (struct v4l2_ext_led_spi_ctrl_info *)stContainer.p_data
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Control LED SPI
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_vpq_cmn_data stPqContainer;
 *     struct v4l2_led_spi_ctrl_info stRealInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     // enable bpl_bit1 & disable bpl_bit2
 *     stRealInfo.bitMask = spiCtrl;
 *     stRealInfo.ctrlValue = bCtrl;
 *
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_CONTROL_SPI;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_CONTROL_SPI (V4L2_CID_EXT_LED_BASE + 6)

/**
 * @brief
 *   Get Local Dimming APL value
 *
 * @rst
 * Description
 *   Get Local Dimming APL Min/Max value and use fort BPL On/Off Condition
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_APL_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_ext_vpq_cmn_data
 *     cf... (struct v4l2_led_apl_info *)stContainer.p_data
 *     // currently common struct for all platform. But it may have different struct for other new platform
 *     struct v4l2_led_apl_info
 *     {
 *        unsinged short block_apl_min;
 *        unsinged short block_apl_max;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp

 *     // Get LED block apl info
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_vpq_cmn_data stPqContainer;
 *     struct v4l2_led_apl_info stRealInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_APL_DATA;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_APL_DATA (V4L2_CID_EXT_LED_BASE + 7)

/**
 * @brief Set/Get BPL DATA
 *
 * @rst
 * This function set/get a data to BPL Alogrithm of Local Dimming firmware
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_LED_BPL_DATA
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_led_bpl_info {
 *        unsigned char ai_brightness; //0(Off), 1(On)
 *        unsigned char sensor_level; // 0~100
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set BPL Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_led_bpl_info bpl_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&bpl_info, 0, sizeof(v4l2_ext_led_bpl_info));
 *
 *     bpl_info.ai_brightness= 1;
 *     bpl_info.sensor_level= 100;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_led_bpl_info);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&bpl_info;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_LED_BPL_DATA;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_LED_BPL_DATA (V4L2_CID_EXT_LED_BASE + 8)

#define V4L2_CID_EXT_MEMC_BASE (V4L2_CID_USER_EXT_PQ_BASE + 0x100)

/**
 * @brief
 *   Initialize MEMC block
 *
 * @rst
 * Description
 *   Initialize MEMC block and is called once when the system is power on.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_MEMC_INIT
 *
 *     //
 *     // parameter
 *     //
 *     void
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Initialize MEMC block.
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_MEMC_INIT;
 *     control.value = 0;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_MEMC_INIT (V4L2_CID_EXT_MEMC_BASE + 0)

/**
 * @brief
 *   Control frame delay
 *
 * @rst
 * Description
 *   This function control frame delay due to MEMC working.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL // Set PQ Data
 *     VIDIOC_G_CTRL // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_MEMC_LOWDELAY_MODE
 *
 *     //
 *     // parameter
 *     //
 *     UINT8 type [IN]
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF u8DelayMode = 1
 *                        Reduce frame buffer for low frame delay
 *              ELSE
 *                        Normal mode
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     memset(&control, 0, sizeof(v4l2_control));
 *     control.id = V4L2_CID_EXT_MEMC_LOW_DELAY_MODE;
 *     control.value = 1;
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_MEMC_LOW_DELAY_MODE (V4L2_CID_EXT_MEMC_BASE + 1)

/**
 * @brief
 *   Control MEMC level
 *
 * @rst
 * Description
 *   This function control MEMC level.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_MEMC_MOTION_COMP
 *
 *     //
 *     // parameter
 *     //
 *     // MEMC Type Definition
 *     enum v4l2_ext_memc_type_old {
 *         V4L2_EXT_MEMC_TYPE_OFF = 0,
 *         V4L2_EXT_MEMC_TYPE_LOW,
 *         V4L2_EXT_MEMC_TYPE_HIGH,
 *         V4L2_EXT_MEMC_TYPE_USER,
 *         V4L2_EXT_MEMC_TYPE_55_PULLDOWN,
 *         V4L2_EXT_MEMC_TYPE_MEDIUM
 *     };
 *     enum v4l2_ext_memc_type {
 *         V4L2_EXT_MEMC_OFF          = V4L2_EXT_MEMC_TYPE_OFF,
 *         V4L2_EXT_MEMC_CINEMA_CLEAR = V4L2_EXT_MEMC_TYPE_MEDIUM,
 *         V4L2_EXT_MEMC_NATURAL      = V4L2_EXT_MEMC_TYPE_LOW,
 *         V4L2_EXT_MEMC_SMOOTH       = V4L2_EXT_MEMC_TYPE_HIGH,       //
 *         V4L2_EXT_MEMC_USER         = V4L2_EXT_MEMC_TYPE_USER,
 *         V4L2_EXT_MEMC_PULLDOWN_55  = V4L2_EXT_MEMC_TYPE_55_PULLDOWN
 *     };
 *     struct v4l2_ext_memc_motion_comp_info {
 *         unsigned char blur_level;
 *         unsigned char judder_level;
 *         enum v4l2_ext_memc_type memc_type;
 *     };
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       IF motion = V4L2_EXT_MEMC_TYPE_OFF
 *           OFF memc
 *       ELSE IF motion = V4L2_EXT_MEMC_TYPE_CINEMA_CLEAR
 *           Set De-judder 3, De-blur 10
 *       ELSE IF motion = V4L2_EXT_MEMC_TYPE_NATURAL
 *           Set De-judder 6.5, De-blur 10
 *       ELSE IF motion = V4L2_EXT_MEMC_TYPE_SMOOTH
 *           Set De-judder 10, De-blur 10
 *       ELSE IF motion = V4L2_EXT_MEM _TYPE_USER
 *           Set blurLevel to blur_level    //50,60Hz 1:1 source
 *           Set JudderLevel to judder_level    //24,25,30Hz 1:1 source and 50, 60Hz pull-down source.
 *       ELSE IF motion = MEMC_TYPE_55_PULLDOWN
 *           On 5:5 Pull-down for 24Hz Film source
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *           RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set MEMC Motion comp
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_memc_motion_comp_info motion_comp_info;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&motion_comp_info, 0, sizeof(v4l2_ext_memc_motion_comp_info));
 *     motion_comp_info.blur_level=blurlevel;
 *     motion_comp_info.judder_level=judderlevel;
 *     motion_comp_info.v4l2_ext_memc_type=memctype;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_memc_motion_comp_info);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&motion_comp_info;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_MEMC_MOTION_COMP;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_MEMC_MOTION_COMP (V4L2_CID_EXT_MEMC_BASE + 2)

/**
 * @brief
 *   Control Motion Pro enable
 *
 * @rst
 * Description
 *   This function control Motion Pro enable/disable.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *     VIDIOC_G_CTRL        // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_MEMC_MOTION_PRO
 *
 *     //
 *     // parameter
 *     //
 *     bMotionPro[IN] bool
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF bMotionPro= TRUE
 *                        Enable BFI Function
 *              ELSE
 *                        Disable BFI Function
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     memset(&control, 0, sizeof(v4l2_control));
 *     control.id = V4L2_CID_EXT_MEMC_MOTION_PRO;
 *     control.value = true;
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_MEMC_MOTION_PRO (V4L2_CID_EXT_MEMC_BASE + 3)

#define V4L2_CID_EXT_HDR_BASE (V4L2_CID_USER_EXT_PQ_BASE + 0x200)

/**
 * @brief
 *   Set inverse gamma on/off
 *
 * @rst
 * Description
 *   Set inverse gamma on/off.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDR_INV_GAMMA
 *
 *     //
 *     // parameter
 *     //
 *     unsigned int bOnOff [IN]
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF bOnOff == 1
 *                      Inverse Gamma Enable
 *              ELSE
 *                      Inverse Gamma Disable
 *
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // LED initialize
 *     v4l2_control control;
 *     unsigned int bOnOff = 0;
 *     if(isHLG == TRUE)
 *     else
 *     control.id = V4L2_CID_EXT_HDR_INV_GAMMA;
 *     control.value = bOnOff;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_INV_GAMMA (V4L2_CID_EXT_HDR_BASE + 0)

/**
 * @brief
 *   Get rgb max histogram in hdr block
 *
 * @rst
 * Description
 *   Get rgb max histogram in hdr block.
 *   Bin size is 128.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get Pic info
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_HDR_PIC_INFO
 *
 *     //
 *     // parameter
 *     //
 *     #define V4L2_EXT_HDR_PICINFO_SIZE 128
 *     unsigned int picInfo[V4L2_EXT_HDR_PICINFO_SIZE];
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              //GET  RGB max histogram
 *              unsigned int pstPicInfo[V4L2_EXT_HDR_PICINFO_SIZE]
 *              memcpy(p_data->picInfo, pstPicInfo, sizeof(pstPicInfo));
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get hdr picture info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     unsigned int hdrPicInfo[V4L2_EXT_HDR_PICINFO_SIZE];
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     pqData.version = 1;
 *     pqData.length = sizeof(hdrPicInfo);
 *     pqData.wId = 0;
 *     pqData.p_data = (char*)hdrPicInfo;
 *     ext_control.id = V4L2_CID_EXT_HDR_PIC_INFO;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_PIC_INFO (V4L2_CID_EXT_HDR_BASE + 1)

/**
 * @brief
 *   Set Tone Mapping Look up table(3D LUT) in HDR Block
 *
 * @rst
 * Description
 *   Set Tone Mapping Look up table(3D LUT) in HDR Block
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDR_3DLUT
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_hdr_mode {
 *         V4L2_EXT_HDR_MODE_SDR,
 *         V4L2_EXT_HDR_MODE_DOLBY,
 *         V4L2_EXT_HDR_MODE_HDR10,
 *         V4L2_EXT_HDR_MODE_HLG,
 *         V4L2_EXT_HDR_MODE_TECHNICOLOR,
 *         V4L2_EXT_HDR_MODE_HDREFFECT,
 *         V4L2_EXT_HDR_MODE_MAX
 *     };
 *     struct v4l2_ext_hdr_3dlut {
 *         enum v4l2_ext_hdr_mode hdr_mode;
 *         union {
 *             unsigned short *p3dlut; ///< p_data
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              SET HDR 3D LUT
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set HDR 3D LUT
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_hdr_3dlut st3dlut;
 *     static unsigned short u3DLUT[3DLUT_SIZE] = {0,};
 *     unsinged int u3dlut_size = sizeof(u3DLUT);
 *     Get3DLUT(u3DLUT);
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&st3dlut, 0, sizeof(v4l2_ext_hdr_3dlut));
 *     st3dlut.hdr_mode = V4L2_EXT_HDR_MODE_HDR10;
 *     st3dlut.data_size = u3dlut_size;
 *     st3dlut.p3dlut = &u3DLUT[0];
 *     pqData.version=1;
 *     pqData.length = sizeof(st3dlut);
 *     pqData.wId=0;
 *     pqData.p_data = (char*)st3dlut;
 *     ext_control.id = V4L2_CID_EXT_HDR_3DLUT;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_3DLUT (V4L2_CID_EXT_HDR_BASE + 2)

/**
 * @brief
 *   Set EOTF data table
 *
 * @rst
 * Description
 *   The function set EOTF data table for HDR.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDR_EOTF
 *
 *     //
 *     // parameter
 *     //
 *     unsigned int eotf[1024];
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set HDR EOTF Data
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     unsigned int uEOTF[EOTF_LUT_SIZE] = {0, };
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(PQ_CMN_DATA_T));
 *     GetEOTF(uEOTF);
 *     //PQ DATA
 *     pqData.version=1;
 *     pqData.length = sizeof(uEOTF);
 *     pqData.wId=0;
 *     pqData.p_data=(char*)uEOTF;
 *     //EXT CONTROL
 *     ext_control.id = V4L2_CID_EXT_HDR_EOTF;
 *     ext_control.ptr = (void *)&pqData;
 *     //EXT CONTROLS
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_EOTF (V4L2_CID_EXT_HDR_BASE + 3)

/**
 * @brief
 *   Set OETF data table
 *
 * @rst
 * Description
 *   The function set OETF data table for HDR.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDR_OETF
 *
 *     //
 *     // parameter
 *     //
 *     unsigned short oetf[1024];
 *
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Set HDR OETF Data
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     unsigned short uOETF[OETF_LUT_SIZE] = {0, };
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(PQ_CMN_DATA_T));
 *     GetOETF(uOETF);
 *     //PQ DATA
 *     pqData.version=1;
 *     pqData.length = sizeof(uOETF);
 *     pqData.wId = 0;
 *     pqData.p_data = (char*)uOETF;
 *     //EXT CONTROL
 *     ext_control.id = V4L2_CID_EXT_HDR_OETF;
 *     ext_control.ptr = (void *)&pqData;
 *     //EXT CONTROLS
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_OETF (V4L2_CID_EXT_HDR_BASE + 4)

/**
 * @brief
 *   Set Tone Mapping data table
 *
 * @rst
 * Description
 *   The function set Tone Mapping data table for HDR.
 *   Only for LG
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_HDR_TONEMAP
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_hdr_mode {
 *         V4L2_EXT_HDR_MODE_SDR,
 *         V4L2_EXT_HDR_MODE_DOLBY,
 *         V4L2_EXT_HDR_MODE_HDR10,
 *         V4L2_EXT_HDR_MODE_HLG,
 *         V4L2_EXT_HDR_MODE_TECHNICOLOR,
 *         V4L2_EXT_HDR_MODE_HDREFFECT,
 *         V4L2_EXT_HDR_MODE_MAX
 *     };
 *     struct v4l2_ext_hdr_tonemap {
 *         enum v4l2_ext_hdr_mode hdr_mode;
 *         unsigned int r_data[66]; ///<0~4294967295, 33point x,y data
 *         unsigned int g_data[66]; ///<0~4294967295, 33point x,y data
 *         unsigned int b_data[66]; ///<0~4294967295, 33point x,y data
 *     };
 *
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Set HDR Tone Mapping
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_ext_vpq_cmn_data pqData;
 *     v4l2_ext_hdr_tonemap stToneMapData;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(PQ_CMN_DATA_T));
 *     memset(&stToneMapData, 0, sizeof(HAL_VPQ_HDR_TONEMAP_T));
 *     stToneMapData.hdrMode = HAL_VPQ_HDR_MODE_HDR10;
 *     memcpy(stToneMapData.uRdata, toneMapLUT, sizeof(unsigned int)*66);
 *     memcpy(stToneMapData.uGdata, toneMapLUT, sizeof(unsigned int)*66);
 *     memcpy(stToneMapData.uBdata, toneMapLUT, sizeof(unsigned int)*66);
 *     //PQ DATA
 *     pqData.version=1;
 *     pqData.length = sizeof(HAL_VPQ_HDR_TONEMAP_T);
 *     pqData.wId=0;
 *     pqData.pData=(char*)&stToneMapData;
 *     //EXT CONTROL
 *     ext_control.id = V4L2_CID_EXT_HDR_TONEMAP;
 *     ext_control.ptr = (void *)&pqData;
 *     //EXT CONTROLS
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_TONEMAP (V4L2_CID_EXT_HDR_BASE + 5)

/**
 * @brief
 *   Control hue shift & saturation compensation block
 *
 * @rst
 * Description
 *   The function control hue shift & saturation compensation block for HDR
 *   Only for LG
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDR_COLOR_CORRECTION
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_hdr_color_correction {
 *         unsigned int hue_blend; ///< 0~1023
 *         unsigned int sat_blend; ///< 0~1023
 *         unsigned int l_gain_x[8]; // L gain LUT x point
 *         unsigned int l_gain_y[8]; // L gain LUT y point
 *         unsigned int s_gain_x[8]; // S gain LUT x point
 *         unsigned int s_gain_y[8]; // S gain LUT y point
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set HDR Color Correction
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_ext_vpq_cmn_data pqData;
 *     struct v4l2_ext_hdr_color_correction stColorCorrectionData;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(PQ_CMN_DATA_T));
 *     memset(&stColorCorrectionData, 0, sizeof(v4l2_ext_hdr_color_correction));
 *     stColorCorrectionData.uHueBlend = 240;
 *     stColorCorrectionData.uSatBlend = 512;
 *     stColorCorrectionData.uLgainx[8] = {0, 5000, 10000, 15000, 20000, 25000, 30000, 65535};
 *     stColorCorrectionData.uLgainy[8] = {255, 255, 255, 255, 255, 255, 255, 255};
 *     stColorCorrectionData.uSgainx[8] = {0, 5000, 10000, 15000, 20000, 25000, 30000, 65535};
 *     stColorCorrectionData.uSgainy[8] = {255, 255, 255, 255, 255, 255, 255, 255};
 *     //PQ DATA
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_hdr_color_correction);
 *     pqData.wId = 0;
 *     pqData.p_data = (char*)&stColorCorrectionData;
 *     //EXT CONTROL
 *     ext_control.id = V4L2_CID_EXT_HDR_COLOR_CORRECTION;
 *     ext_control.ptr = (void *)&pqData;
 *     //EXT CONTROLS
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_COLOR_CORRECTION (V4L2_CID_EXT_HDR_BASE + 6)

/**
 * @brief
 *   Set Y Gain table
 *
 * @rst
 * Description
 *   The function set Y Gain for HLG.
 *   Data is unsigned short array of length 66.
 *   Array index 0~32 is x axis.
 *   Array index 33~65 is y axis.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HLG_Y_GAIN_TBL
 *
 *     //
 *     // parameter
 *     //
 *     unsigned short uYgaindata[V4L2_EXT_HDR_HLG_YGAIN_TBL_SIZE]
 *     #define V4L2_EXT_HDR_HLG_YGAIN_TBL_SIZE 66
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set HLG Curve Table
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_ext_vpq_cmn_data pqData;
 *     unsigned short uYgaindata[V4L2_EXT_HDR_HLG_YGAIN_TBL_SIZE] = {0, };
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(PQ_CMN_DATA_T));
 *     GetYgainData(uYgaindata);
 *     //PQ DATA
 *     pqData.version=1;
 *     pqData.length = sizeof(uYgaindata);
 *     pqData.wId = 0;
 *     pqData.p_data=(char*) uYgaindata;
 *     //EXT CONTROL
 *     ext_control.id = V4L2_CID_EXT_HLG_Y_GAIN_TBL;
 *     ext_control.ptr = (char *)&pqData;
 *     //EXT CONTROLS
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDR_HLG_Y_GAIN_TBL (V4L2_CID_EXT_HDR_BASE + 7)

#define V4L2_CID_EXT_DOLBY_BASE (V4L2_CID_USER_EXT_PQ_BASE + 0x300)

/**
 * @brief
 *   Set path of Dolby Picture configuration file
 *
 * @rst
 * Description
 *   Set path of Dolby Picture configuration file to dolby vision module.
 *   V4L2_EXT_DOLBY_CONFIG_MAIN = primaryV4L2_EXT_DOLBY_CONFIG_BEST = for
 *   calibration data
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_CFG_PATH
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_dolby_config_type{
 *         V4L2_EXT_DOLBY_CONFIG_MAIN = 0,
 *         V4L2_EXT_DOLBY_CONFIG_BEST
 *     }
 *     struct v4l2_ext_dolby_config_path{
 *         enum v4l2_ext_dolby_config_type eConfigType;
 *         char sConfigPath[255];
 *     }
 *     v4l2_ext_dolby_config_path configFilePath [IN]
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *     SET path of Dolby Picture configuration
 *     return;
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_ext_dolby_config_path stConfigPathMain;
 *     v4l2_ext_dolby_config_path stConfigPathBest;
 *     #define DOLBY_MAIN "primary.cfg"
 *     #define DOLBY_BEST "best.cfg"
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     stConfigPathMain.eConfigType = E_MAIN_PQ;
 *     strcpy(stConfigPathMain.sConfigPath, DOLBY_MAIN);
 *     pqData.version=1;
 *     pqData.length = sizeof(stConfigPathMain);
 *     pqData.wId=0;
 *     pqData.pData= (char*)stConfigPathMain;
 *     ext_control.id = V4L2_CID_EXT_DOLBY_CFG_PATH;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_CFG_PATH (V4L2_CID_EXT_DOLBY_BASE + 0)

/**
 * @brief
 *   Set Dolby HDR On/Off and Dolby Picture Mode
 *
 * @rst
 * Description
 *   Set Dolby HDR On/Off and Dolby Picture Mode
 *
 *   uPictureMode = 0 ; //CINEMA,
 *
 *   uPictureMode = 1 ; //CINEMA_BIRHGT,
 *
 *   uPictureMode = 2 ; //VIVID,
 *
 *   uPictureMode = 3 ; //STANDARD,
 *
 *   uPictureMode = 4 ; //GAME,
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_PICTURE_MODE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_dolby_picture_mode[IN]
 *     struct v4l2_ext_dolby_picture_mode{
 *         unsigned int bOnOff;
 *         unsigned int uPictureMode;
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF bOnOff == ON
 *                      IF uPictureMode == VIVID
 *                                Set Dolby HDR VIVID picture mode
 *                      Else IF uPictureMode == CINEMA
 *                                Set Dolby HDR CINEMA picture mode
 *                      Else IF uPictureMode == STANDARD
 *                                Set Dolby HDR STANDARD picture mode
 *             ELSE
 *                       Not Set
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_dolby_picture_mode stDolbyPictureMode;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&stDolbyPictureMode, 0, sizeof(HAL_DOLBY_PICTURE_MODE_T));
 *     if(dolby vision)
 *     {
 *          stDolbyPictureMode.bOnOff = 1;
 *          stDolbyPictureMode.uPictureMode = currentPictureMode;
 *     }
 *     else
 *     {
 *          stDolbyPictureMode.bOnOff = 0;
 *     }
 *     pqData.version = 1;
 *     pqData.length = sizeof(v4l2_ext_dolby_picture_mode);
 *     pqData.wId = 0;
 *     pqData.pData=(char*)&stDolbyPictureMode;
 *     ext_control.id = V4L2_CID_EXT_DOLBY_PICTURE_MODE;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_PICTURE_MODE (V4L2_CID_EXT_DOLBY_BASE + 1)

/**
 * @brief
 *   Set Dolby HDR On/Off and Picture Menu value
 *
 * @rst
 * Description
 *   Set Dolby HDR On/Off and Picture Menu value(Backlight, Brightness, Color,
 *   Contrast). if set On for each menu value, dolby control menu value.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_PICTURE_MENU
 *
 *     //
 *     // parameter
 *     //
 *     v4l2_ext_dolby_picture_data pictureData [IN]
 *
 *     //  dolby picture menu
 *     enum v4l2_ext_dolby_picture_menu {
 *         V4L2_EXT_DOLBY_BACKLIGHT = 0,   ///< backlight
 *         V4L2_EXT_DOLBY_BRIGHTNESS,      /// brightness
 *         V4L2_EXT_DOLBY_COLOR,           ///< color
 *         V4L2_EXT_DOLBY_CONTRAST,        ///< contrast
 *         V4L2_EXT_DOLBY_PICTURE_MENU_MAX ///< max num
 *     };
 *
 *     // dolby picture menu data
 *     struct v4l2_ext_dolby_picture_data {
 *         enum v4l2_ext_dolby_picture_menu picture_menu; ///< picture menu
 *         unsigned int on_off;                           ///<0:off, 1:on
 *         int setting_value;                             ///< setting value
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF pictureData.pictureMenu == V4L2_EXT_DOLBY_BACKLIGHT THEN
 *                         IF bOnOff == ON
 *                                 Set DolbyBacklight value to pictureData.nValue.
 *                        ELSE
 *                                  Not Set
 *              ELSE IF pictureData.pictureMenu == V4L2_EXT_DOLBY_BRIGHTNESS THEN
 *                         IF bOnOff == ON
 *                                 Set DolbyBrightness value to pictureData.nValue.
 *                        ELSE
 *                                  Not Set
 *              ELSE IF pictureData.pictureMenu == V4L2_EXT_DOLBY_COLOR THEN
 *                         IF bOnOff == ON
 *                                 Set DolbyColor value to pictureData.nValue.
 *                        ELSE
 *                                  Not Set
 *              ELSE IF pictureData.pictureMenu ==V4L2_EXT_DOLBY_CONTRAST THEN
 *                         IF bOnOff == ON
 *                                 Set DolbyContrast value to pictureData.nValue.
 *                        ELSE
 *                                  Not Set
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_ext_dolby_picture_data stPicData;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&stPicData, 0, sizeof(v4l2_ext_dolby_picture_data));
 *     stPicData.pictureMenu = V4L2_EXT_DOLBY_BACKLIGHT;
 *     stPicData.onoff = 1;
 *     stPicData.value = uBacklightValue;
 *     pqData.version = 1;
 *     pqData.length = sizeof(v4l2_ext_dolby_picture_data );
 *     pqData.wId = 0;
 *     pqData.pData=(char*)&stPicData;
 *     ext_control.id = V4L2_CID_EXT_DOLBY_PICTURE_MENU;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_PICTURE_MENU (V4L2_CID_EXT_DOLBY_BASE + 2)

/**
 * @brief
 *   Get dolby software version
 *
 * @rst
 * Description
 *   Get dolby software version. If Dolby SW Version string is null, return
 *   NOT_OK.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get Dolby Version
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_SW_VERSION
 *
 *     //
 *     // parameter
 *     //
 *     char cDolbyVersion[128]
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              char * pVersion =  (char *)control.value;
 *              char cVersion[128];
 *              GET Dolby SW Version
 *              memcpy(pVersion, cVersion, sizeof(char)*128);
 *
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Dolby SW Version
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     char cDolbyVersion[128] = {0,};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     pqData.version=1;
 *     pqData.length = sizeof(pcDolbyVersion);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)cDolbyVersion;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_DOLBY_SW_VERSION;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *     return cDolbyVersion;
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_SW_VERSION (V4L2_CID_EXT_DOLBY_BASE + 3)

/**
 * @brief
 *   Pass the energy saving / backlight ratio
 *
 * @rst
 * Description
 *
 *   This function passes the energy saving / backlight ratio.
 *    Dolby PWM multiplies this ratio and outputs the brightness.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *     VIDIOC_G_CTRL        // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_PWM_RATIO
 *
 *     //
 *     // parameter
 *     //
 *     type               [IN] UINT8
 *     UINT8 u8Percent;
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Dolby Backlight output  = Dolby PWM x u8Percent/100
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Dolby Pwm Ratio
 *     struct v4l2_control control;
 *     UINT8 u8Percent;
 *     //Calculate PWM ratio
 *     u8Percent = (PWM ratio)
 *     memset(&control, 0, sizeof(v4l2_control));
 *     control.id = V4L2_CID_EXT_DOLBY_PWM_RATIO;
 *     control.value = u8Percent;
 *     ioctl(fd, VIDIOC_S_CTRL, &controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_PWM_RATIO (V4L2_CID_EXT_DOLBY_BASE + 4)

/**
 * @brief
 *   Set delay table value
 *
 * @rst
 * Description
 *   Global Dimming Delay value is different according to frame Rate and Dolby
 *   Vision certification model such as 24Hz, 30Hz, 60Hz. Set delay table value
 *   according to frame rate and Dolby Vision certification model such as 24Hz,
 *   30Hz, 60Hz.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ info
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_GD_DELAY
 *
 *     //
 *     // parameter
 *     //
 *     // dolby global dimming delay data
 *     struct v4l2_ext_dolby_gd_delay
 *     {
 *         unsigned short ott_24;
 *         unsigned short ott_30;
 *         unsigned short ott_50;
 *         unsigned short ott_60;
 *         unsigned short ott_100;
 *         unsigned short ott_120;
 *         unsigned short hdmi_24;
 *         unsigned short hdmi_30;
 *         unsigned short hdmi_50;
 *         unsigned short hdmi_60;
 *         unsigned short hdmi_100;
 *         unsigned short hdmi_120;
 *     };
 *     struct v4l2_ext_dolby_gd_delay_lut
 *     {
 *         struct v4l2_ext_dolby_gd_delay standard_frc_off;
 *         struct v4l2_ext_dolby_gd_delay standard_frc_on;
 *         struct v4l2_ext_dolby_gd_delay vivid_frc_off;
 *         struct v4l2_ext_dolby_gd_delay vivid__frc_on;
 *         struct v4l2_ext_dolby_gd_delay cinema_home_frc_off;
 *         struct v4l2_ext_dolby_gd_delay cinema_home_frc_on;
 *         struct v4l2_ext_dolby_gd_delay cinema_frc_off;
 *         struct v4l2_ext_dolby_gd_delay cinema_frc_on;
 *         struct v4l2_ext_dolby_gd_delay game_frc_off;
 *         struct v4l2_ext_dolby_gd_delay game_frc_on;
 *     };
 *     struct v4l2_ext_dolby_gd_delay_param
 *     {
 *         struct v4l2_ext_dolby_gd_delay_lut dolby_GD_standard;
 *         struct v4l2_ext_dolby_gd_delay_lut dolby_GD_latency;
 *     };
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   This V4L2 is only for model supporting Dolby Vision.
 *
 *   Other chip will implement it as dummy.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set standard/latency delay table
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Dolby GD Delay
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_datapqData;
 *     v4l2_dolby_gd_delay_param dolbyGDVal;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&dolbyGDVal, 0, sizeof(v4l2_dolby_gd_delay_param));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_dolby_gd_delay_param);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&dolbyGDVal;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_DOLBY_GD_DELAY;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_GD_DELAY (V4L2_CID_EXT_DOLBY_BASE + 5)

/**
 * @brief
 *   Setting on/off flag and sensor raw data and lux data
 *
 * @rst
 * Description
 *   PQ SW want to correct the illuminance based picture quality of the viewing
 *   environment(Dolby tone mapping)
 *
 *   This is a HAL function for setting on/off flag and sensor raw data and lux
 *   data.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ info
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_AMBIENT_LIGHT
 *
 *     //
 *     // parameter
 *     //
 *     // dolby ambient light param
 *     struct v4l2_ext_dolby_ambient_light_param
 *     {
 *       unsigned int onoff;
 *       unsigned int luxdata;
 *       union {
 *         unsigned int *rawdata; ///< p_data
 *         unsigned int compat_data;
 *         unsigned long long sizer;
 *       };
 *     };
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   This V4L2 is only for model supporting Dolby Vision.
 *
 *   Other chip will implement it as dummy.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Check On/Off flag
 *              IF On status
 *                       Set sensor raw data and lux data.
 *
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Dolby Ambient Light
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_dolby_ambient_light_param dolbyAmbientLight;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&dolbyGDVal, 0, sizeof(v4l2_dolby_ambient_light_param));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_dolby_ambient_light_param);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&dolbyAmbientLight;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_DOLBY_AMBIENT_LIGHT;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_AMBIENT_LIGHT (V4L2_CID_EXT_DOLBY_BASE + 6)

/**
 * @brief
 *   Get the Dolby Vision contents type metadata
 *
 * @rst
 * Description
 *   This function is used to get the Dolby Vision contents type metadata(L11
 *   contents-aware metadata).
 *   If this function is not called, BPS must operate at its default value.
 *
 * .. image:: /v4l2/pq-V4L2_CID_EXT_DOLBY_CONTENTS_TYPE.png
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_DOLBY_CONTENTS_TYPE
 *
 *     //
 *     // parameter
 *     //
 *     unsigned char contentType[4];
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              //GET  Dolby Content Type
 *              unsigned char contentType[4] = {?,?,?,?};
 *              memcpy(vpq_cmn_data->p_data, contentType, sizeof(contentType));
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Dolby Content Type
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     unsigned char contentType[4] = {0,};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     pqData.version = 1;
 *     pqData.length = sizeof(contentType);
 *     pqData.wId = 0;
 *     pqData.p_data = (char*)contentType;
 *     ext_control.id = V4L2_CID_EXT_DOLBY_CONTENTS_TYPE;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.ctrl_class    = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_DOLBY_CONTENTS_TYPE (V4L2_CID_EXT_DOLBY_BASE + 7)

#define V4L2_CID_EXT_VPQ_BASE (V4L2_CID_USER_EXT_PQ_BASE + 0x400)

/**
 * @brief
 *   Initializes the VPQ module
 *
 * @rst
 * Description
 *   Initializes the VPQ module
 *
 *   It calls the library functions of chip vendor for initialization and
 *   initializes the global variables for HAL functions.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_INIT
 *
 *     //
 *     // parameter
 *     //
 *     - true : initializing
 *     - false : de-initializing (for future use)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_INIT (V4L2_CID_EXT_VPQ_BASE + 0)

/**
 * @brief
 *   Set picture control
 *
 * @rst
 * Description
 *   Set picture control(brightness, contrast, Tint, saturation).
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_PICTURE_CTRL
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_picture_ctrl_data{
 *      signed int sPcVal[4];
 *      signed int sContrast;
 *      signed int sBrightness;
 *      signed int sSaturation;
 *      signed int sHue;
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.

 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input Parameters
 *       SET appropriate data on the structure with sample data
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_picture_ctrl_data pictureCtrl;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&pictureCtrl, 0, sizeof(v4l2_picture_ctrl_data));
 *     pictureCtrl.sPcVal[0] = contrastUiValue;
 *     pictureCtrl.sPcVal[1] = brightnessUiValue;
 *     pictureCtrl.sPcVal[2] = colorUiValue;
 *     pictureCtrl.sPcVal[3] = tintUiValue;
 *     pictureCtrl.sContrast = contrastVal;
 *     pictureCtrl.sBrightness= brightnessVal;
 *     pictureCtrl.sSaturation= saturationVal;
 *     pictureCtrl.sHue= hueVal;
 *     pictureCtrl.sDimming= dimmingVal;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_picture_ctrl_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&pictureCtrl;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_PICTURE_CTRL;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_PICTURE_CTRL (V4L2_CID_EXT_VPQ_BASE + 1)

/**
 * @brief
 *   Set sharpness
 *
 * @rst
 * Description
 *   Set sharpness from OSD values(UI).
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_SHARPNESS
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_sharppness_data{
 *      unsigned int compat_data;
 *     };
 *     container : v4l2_ext_vpq_cmn_data
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 *   In case of 2-chip struct. AP will use a member "wId" of
 *   struct v4l2_ext_vpq_cmn_data to distinguish it's target chip.
 *
 *   wId : 0 - main chip's main-path
 *
 *   wId : 1 - main chip's sub-path (reserved for future)
 *
 *   wId : 2 - sub chip's main-path
 *
 *   wId : 3 - sub chip's sub-path (reserved for future)
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_sharpness_data sharpness;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_ext_vpq_cmn_data));
 *     memset(&sharpness, 0, sizeof(v4l2_ext_vpq_sharpness_data));
 *     sharpness.ui_value[0] = sharpnessUiValue;
 *     sharpness.ui_value[1] = superResolutionUi;
 *     sharpness.ui_value[2] = currentInputInfo;
 *     sharpness.ui_value[3] = 360VRInput;
 *     sharpness.pst_chip_data = PQL_DB_....();
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_sharpness_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&sharpness;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_SHARPNESS;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_SHARPNESS (V4L2_CID_EXT_VPQ_BASE + 2)

/**
 * @brief
 *   Get histogram data
 *
 * @rst
 * Description
 *   Get histogram data from chip data type information for DC.
 *   It is used to check the capabilities of HW.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get VDO info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_HISTO_DATA
 *
 *     //
 *     // parameter
 *     //
 *     *pHistoInfo         [IN] v4l2_vpq_dc2p_histodata_info
 *     #define HAL_VPQ_BIN_NUM 64
 *     #define HAL_VPQ_C_BIN_NUM 32
 *     #define HAL_VPQ_H_BIN_NUM 32
 *
 *     //
 *     // dc2p histogram data info
 *     //
 *     struct v4l2_vpq_dc2p_histodata_info
 *     {
 *          signed int peak_low;      ///< peak low
 *          unsigned int diff_sum;      ///< diff sum
 *          unsigned int chrm_bin[V4L2_VPQ_C_BIN_NUM];  ///< count chroma binv
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       IF pHistoInfo == NULL THEN
 *           RETURN NOT_OK
 *       END IF
 *       FOR i=0 i < V4L2_VPQ_BIN_NUM
 *           Fill pHistoInfo->bin[i] from chip specific API
 *       END FOR
 *       FOR i=0 i < V4L2_VPQ_C_BIN_NUM
 *           Fill pHistoInfo->chrm_bin[i] from chip specific API
 *       END FOR
 *       FOR i=0 i < V4L2_VPQ_H_BIN_NUM
 *           Fill pHistoInfo->hue_bin[i] from chip specific API
 *       END FOR
 *
 *       Fill pHistoInf->apl from chip specific API
 *       Fill pHistoInf->min from chip specific API
 *       Fill pHistoInfo->max from chip specific API
 *       Fill pHistoInfo->peak_low from chip specific API
 *       Fill pHistoInfo->peak_high from chip specific API
 *       Fill pHistoInfo->skin_count from chip specific API
 *       Fill pHistoInfo->sat_statust from chip specific API
 *       Fill pHistoInfo->diff_sum from chip specific API
 *       Fill pHistoInfo->motion from chip specific API
 *       Fill pHistoInfo->texure from chip specific API
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Histo Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_dc2p_histodata_info histdata;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&histdata, 0, sizeof(v4l2_vpq_dc2p_histodata_info));
 *     pqData.version = 1;
 *     pqData.length = sizeof(v4l2_vpq_dc2p_histodata_info);
 *     pqData.p_data =(unsigned char*)&histdata;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_HISTO_DATA;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_HISTO_DATA (V4L2_CID_EXT_VPQ_BASE + 3)

/**
 * @brief
 *   Set fresh contrast
 *
 * @rst
 * Description
 *   Set fresh contrast as enable.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST
 *
 *     //
 *     // parameter
 *     //
 *     v4l2_ext_dynamnic_contrast_ctrl IN]
 *     uDcVal [IN] unsigned short UI value for fresh contrast (0~3):(off,low,med,high)
 *     pstDynamicContrast [IN] chip data for fresh contrast (chip dependency)
 *     struct v4l2_dc_dynamnic_contrast_chipData {
 *     } ;
 *     struct v4l2_ext_dynamnic_contrast_ctrl {
 *             unsigned int compat_data;
 *        };
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input Parameters
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *       v4l2_vpq_dc_dynamic_contrast *chipData;
 *       chipData = (v4l2_vpq_dynamic_contrast ) &pstDynamicContrast;
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Dynamic Contrast Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_ext_dynamnic_contrast_ctrl dynamiccontrast;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&dynamiccontrast, 0, sizeof(v4l2_ext_dynamnic_contrast_ctrl));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_dynamnic_contrast_ctrl);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&dynamiccontrast;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST (V4L2_CID_EXT_VPQ_BASE + 4)

/**
 * @brief
 *   Set fresh contrast lookup table
 *
 * @rst
 * Description
 *   Set fresh contrast lookup table.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_LUT
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_dynamnic_contrast_lut {
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input Parameters
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *         v4l2_vpq_dynamic_contrast_lut  *chipData;
 *       chipData = (v4l2_vpq_dynamic_contrast_lut  ) *pstDynamicContrastLut;
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     #define CHIP_NUM_TRANSCURVE 32 //mstar only 16
 *     // Set Dynamic Contrast LUT
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_dynamnic_contrast_lut dynamiccontrastlut;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&dynamiccontrast, 0, sizeof(dynamiccontrastlut));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_dynamnic_contrast_lut);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&dynamiccontrastlut;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_LUT;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_LUT (V4L2_CID_EXT_VPQ_BASE + 5)

/**
 * @brief
 *   Setting detail chip data for managing each color regions
 *
 * @rst
 * Description
 *   Setting detail chip data for managing each color regions
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_CM_DB_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_vpq_cmn_data
 *
 *     cf... (struct v4l2_ext_cm_info *)stContainer.p_data
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Change Dynamic Color UI
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_vpq_cmn_data stPqContainer;
 *     struct v4l2_vpq_cm_info stRealInfo;
 *     struct v4l2_vpq_cm_db stCmDb;
 *      struct v4l2_vpq_cm_ui_status
 *     stUiInfo = {{TRUE, v4l2_vpq_cm_dynamic_color_low}, {TRUE, {-5,0,0}}, {FALSE,{0,},{0,},{0,}};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.use_internal_cm_db = FALSE;
 *     memset(&stRealInfo.uiInfo , stUiInfo, sizeof(stRealInfo.uiInfo));
 *     stRealInfo.dbInfo = &stCmDb; // stCmDb setting
 *      ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_CM_DB_DATA;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */

#define V4L2_CID_EXT_VPQ_CM_DB_DATA (V4L2_CID_EXT_VPQ_BASE + 6)

/**
 * @brief
 *   Set noise reduction
 *
 * @rst
 * Description
 *   Set noise reduction such as temporal noise reduction strength from user control.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_NOISE_REDUCTION*
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_noise_reduction_data{
 *      unsigned short ui_value[2]; //UI value for noise reduction 0~4:off,low,med,high,auto
 *      union {
 *             unsigned char *pst_chip_data;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *     // container : v4l2_ext_vpq_cmn_data
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 *   In case of 2-chip struct. AP will use a member "wId" of struct
 *   v4l2_ext_vpq_cmn_data to distinguish it's target chip.
 *
 *   wId : 0 - main chip's main-path
 *
 *   wId : 1 - main chip's sub-path (reserved for future)
 *
 *   wId : 2 - sub chip's main-path
 *
 *   wId : 3 - sub chip's sub-path (reserved for future)
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_noise_reduction_data noiseReduction;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_ext_vpq_cmn_data));
 *     memset(&noiseReduction, 0, sizeof(v4l2_ext_vpq_noise_reduction_data));
 *     noiseReduction.ui_value[0] = noiseReductionUiValue;
 *     noiseReduction.ui_value[1] = currentInputInfo;
 *     noiseReduction.pst_chip_data = PQL_DB_....();
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_noise_reduction_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&noiseReduction;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_NOISE_REDUCTION;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_NOISE_REDUCTION (V4L2_CID_EXT_VPQ_BASE + 7)

/**
 * @brief
 *   Set Mpeg NR
 *
 * @rst
 * Description
 *   Set Mpeg NR(Noise Reduction).
 *   To remove unwanted ringing and block noise from images that have undergone
 *   MPEG or JPEG compression and decompression,
 *   set the required Registers(Mpeg Noise Reduction thresholds, and etc).
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS   // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS   // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_MPEG_NOISE_REDUCTION
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_mpeg_noise_reduction_data{
 *      unsigned char ui_value;
 *      union {
 *             unsigned char *pst_chip_data;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *     // container : v4l2_ext_vpq_cmn_data
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 *   In case of 2-chip struct. AP will use a member "wId" of struct
 *   v4l2_ext_vpq_cmn_data to distinguish it's target chip.
 *
 *   wId : 0 - main chip's main-path
 *
 *   wId : 1 - main chip's sub-path (reserved for future)
 *
 *   wId : 2 - sub chip's main-path
 *
 *   wId : 3 - sub chip's sub-path (reserved for future)
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_mpeg_noise_reduction_data mpegNoiseReduction;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_ext_vpq_cmn_data));
 *     memset(&mpegNoiseReduction, 0, sizeof(v4l2_ext_vpq_mpeg_noise_reduction_data));
 *     mpegNoiseReduction.ui_value = mpegNrUiValue;
 *     mpegNoiseReduction.pst_chip_data = PQL_DB_....();
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_mpeg_noise_reduction_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&mpegNoiseReduction;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_MPEG_NOISE_REDUCTION;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_MPEG_NOISE_REDUCTION (V4L2_CID_EXT_VPQ_BASE + 8)

/**
 * @brief
 *   Set bypass pq blocks
 *
 * @rst
 * Description
 *   Set bypass pq blocks
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_BYPASS_BLOCK
 *
 *     //
 *     // parameter
 *     // typedef enum v4l2_vpq_bypass_mask {
 *      V4L2_VPQ_BYPASS_MASK_CONTRAST_COLOR_ENHANCE = 0x00000004,       // BIT2: Contrast, Local Contrast, Color Enhancer
 *      V4L2_VPQ_BYPASS_MASK_HDR_EXCEPT_PCC         = 0x00000040,       // BIT6: HDR bypass, HDR-PCC enalbe
 *     };
 *     struct v4l2_ext_vpq_block_bypass {
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *       PQ_CMN_DATA_T *dataHal;
 *       dataHal = (PQ_CMN_DATA_T) *pstData;
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_block_bypass blockbypass;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&blockbypass, 0, sizeof(VPQ_BLOCKS_BYPASS));
 *     blockbypass.bOnOff=onOff;
 *     blockbypass.bypassMask=V4L2_VPQ_BYPASS_MASK_HDR_ALL;
 *     pqData.version=1;
 *     pqData.length = sizeof(blockbypass);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&blockbypass;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_BYPASS_BLOCK ;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_BYPASS_BLOCK (V4L2_CID_EXT_VPQ_BASE + 9)

/**
 * @brief
 *   Set black level
 *
 * @rst
 * Description
 *   Set black level. Depends on input source (except Digital TV source),
 *   Set the black level related .
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_BLACK_LEVEL
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_vpq_black_level_type {
 *      V4L2_EXT_VPQ_BLACKLEVEL_RGB_Y709_LIMIT_HIGH,
 *      V4L2_EXT_VPQ_BLACKLEVEL_RGB_BT2020_LIMIT_HIGH,
 *     };
 *     struct v4l2_ext_vpq_black_level_info {
 *      enum v4l2_ext_vpq_black_level_type black_level_type;
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *       CHIP_CSC_COEFF_T  *chipData;
 *       chipData = (CHIP_CSC_COEFF_T ) *pstData;
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Black Level
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_black_level_info blackLevel;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&blackLevel, 0, sizeof(v4l2_ext_vpq_black_level_info ));
 *     blackLevel.ui_value= level;
 *     blackLevel.curr_input= input;
 *     blackLevel.color_space = 0;
 *     blackLevel.black_level_type = V4L2_EXT_VPQ_BLACKLEVEL_Y709_LINEAR_LIMIT_HIGH;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_black_level_info);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&blackLevel;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_BLACK_LEVEL;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_BLACK_LEVEL (V4L2_CID_EXT_VPQ_BASE + 10)

/**
 * @brief
 *   Set gamma LUT data
 *
 * @rst
 * Description
 *   Set gamma LUT data
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_GAMMA_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stPanelInfo          [IN]      v4l2_ext_vpq_cmn_data
 *     struct v4l2_ext_gamma_lut {
 *         unsigned int table_num; // number of table elements
 *         union {
 *             unsigned int *table_red;
 *             unsigned int compat_table_red;
 *             unsigned long long sizer_r;
 *         };
 *         union {
 *             unsigned int *table_green;
 *             unsigned int compat_table_green;
 *             unsigned long long sizer_g;
 *         };
 *         union {
 *             unsigned int *table_blue;
 *             unsigned int compat_table_blue;
 *             unsigned long long sizer_b;
 *         };
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set gamma DB Data
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_vpq_cmn_data stPqContainer;
 *     struct v4l2_gamma_lut stRealInfo;
 *     unsigned int gammaRed[1024] = {0,1,2, ... , 1023};
 *     unsigned int gammaBlue[1024] = {0,1,2, ... , 1023};
 *     ...
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.table_num = 1024;
 *     stRealInfo.table_red = &gammaRed[0];
 *     ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_GAMMA_DATA;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_GAMMA_DATA (V4L2_CID_EXT_VPQ_BASE + 11)

/**
 * @brief
 *   Set Super Resolution Parameters
 *
 * @rst
 * Description
 *   Set Super Resolution Parameters.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_SUPER_RESOLUTION
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_super_resolution_data{
 *      unsigned char ui_value;
 *      union {
 *             unsigned char *pst_chip_data;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input parameters
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *       PQ_CMN_DATA_T *dataHal;
 *       dataHal = (PQ_CMN_DATA_T) *pstData;
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_super_resolution_data superResolution;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_ext_vpq_cmn_data));
 *     memset(&superResolution, 0, sizeof(v4l2_ext_vpq_super_resolution_data));
 *     superResolution.ui_val = superResolutionUiValue;
 *     superResolution.pst_chip_data = PQL_DB_....();
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_super_resolution_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&superResolution;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_SUPER_RESOLUTION;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_SUPER_RESOLUTION (V4L2_CID_EXT_VPQ_BASE + 12)

/**
 * @brief
 *   Get Noise Level
 *
 * @rst
 *
 * Description
 *   Get Noise Level.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRLS                                    // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_NOISE_LEVEL
 *
 *     //
 *     // parameter
 *     //
 *     * uLevel      [IN] unsined short
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       IF uLevel == NULL THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *    GET uLevel
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *    RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Noise Level
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data_t pqData;
 *     unsigned short uLevel[2];
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data_t ));
 *     pqData.version=1;
 *     pqData.length = sizeof(uLevel);
 *     pqData.wId=0;
 *     pqData.pData=(unsigned char*)uLevel;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_NOISE_LEVEL;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_NOISE_LEVEL (V4L2_CID_EXT_VPQ_BASE + 13)

/**
 * @brief
 *   Set Game Mode
 *
 * @rst
 * Description
 *   Set Game Mode (low delay mode).
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *     VIDIOC_G_CTRL        // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_LOW_DELAY_MODE
 *
 *     //
 *     // parameter
 *     //
 *     BOOLEAN bGameMode
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       IF bGameMode == NULL THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       GET bGameMode
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_VPQ_LOW_DELAY_MODE;
 *     control.value = TRUE;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_LOW_DELAY_MODE (V4L2_CID_EXT_VPQ_BASE + 14)

/**
 * @brief
 *   Get Chip Bypass LUT for Dynamic Contrast
 *
 * @rst
 * Description
 *   Get Chip Bypass LUT for Dynamic Contrast.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_BYPASS_LUT
 *
 *     //
 *     // parameter
 *     //
 *     *pDCBypassLUT      [IN] sined int, pDCBypassLUT[CHIP_NUM_TRANSCURVE]
 *     //RTK CHIP_NUM_TRANSCURVE 32
 *     //mstar/LG CHIP_NUM_TRANSCURVE 16
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *         IF pDCBypassLUT == NULL THEN
 *             RETURN NOT_OK
 *         END IF
 *
 *      GET pDCBypassLUT
 *         IF any error has occurred THEN
 *             RETURN NOT_OK
 *         END IF
 *
 *      RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     #define CHIP_NUM_TRANSCURVE 32 //mstar 16
 *     // Get Fresh Contrast Bypass LUT
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     signed int pDCBypassLUT[CHIP_NUM_TRANSCURVE ] = {0,};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&pDCBypassLUT, 0, sizeof(signed int)*CHIP_NUM_TRANSCURVE );
 *     pqData.version=1;
 *     pqData.length = sizeof(signed int)*CHIP_NUM_TRANSCURVE ;
 *     pqData.wId=0;
 *     pqData.pData=(unsigned char*)&pDCBypassLUT;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = CID_VPQ_DYNAMIC_CONTRAST_BYPASS_LUT;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_BYPASS_LUT                           \
    (V4L2_CID_EXT_VPQ_BASE + 15)

/**
 * @brief
 *   Set Color Gain for Dynamic Contrast
 *
 * @rst
 * Description
 *   Set Color Gain for Dynamic Contrast.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL // Set PQ Data
 *     VIDIOC_G_CTRL // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_COLOR_GAIN
 *
 *     //
 *     // parameter
 *     //
 *     nColorGain      [IN] sined int UI value for picture control (0~100)
 *     sined int nColorGain;
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *       CHECK the value of input parameters
 *       DECLARE locally data structures for executing vendor specific chip API
 *       SET appropriate data on the structure with sample data
 *       CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *
 *       IF any error has occurred THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_COLOR_GAIN ;
 *     control.value = 30;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DYNAMIC_CONTRAST_COLOR_GAIN                           \
    (V4L2_CID_EXT_VPQ_BASE + 16)

/**
 * @brief
 *   Set IRE Inner Pattern
 *
 * @rst
 * Description
 *   Set IRE Inner Pattern.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set PQ Data
 *     VIDIOC_G_CTRL       // Get VDO info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_TESTPATTERN
 *
 *     //
 *     // control value
 *     //
 *     V4L2_VPQ_INNER_PATTERN_IRE_0 ~ V4L2_VPQ_INNER_PATTERN_IRE_100 : display ire pattern corresponding level
 *     V4L2_VPQ_INNER_PATTERN_DISABLE : disable IRE pattern
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * See Also
 *   v4l2_vpq_inner_pattern_ire
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_TESTPATTERN (V4L2_CID_EXT_VPQ_BASE + 17)

/**
 * @brief
 *   Set Colortemperature offset and gain value
 *
 * @rst
 * Description
 *   Set Colortemperature offset and gain value.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_COLORTEMP_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_vpq_cmn_data
 *
 *     cf... (struct v4l2_ext_vpq_color_temp *)stContainer.p_data
 *     stChipData.pData [IN] chip data for color temperature
 *     struct v4l2_ext_vpq_color_temp {
 *         unsigned short rgb_gain[3];   ///< gain. 0:red, 1:green, 2:blue
 *         unsigned short rgb_offset[3]; ///< offset. 0:red, 1:green, 2:blue
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Color temperature Data
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_vpq_cmn_data stPqContainer;
 *     struct v4l2_ext_vpq_color_temp stRealInfo;
 *     unsigned char gain[v4l2_vpq_rgb_max] = {192,192,192};
 *     unsigned char offset[v4l2_vpq_rgb_max] = {64,64,64};
 *     ...
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     memcpy(stRealInfo.rgb_gain, gain, sizeof(stRealInfo.rgb_gain));
 *     memcpy(stRealInfo.rgb_offset, offset, sizeof(stRealInfo.rgb_offset));
 *     ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_COLORTEMP_DATA;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_COLORTEMP_DATA (V4L2_CID_EXT_VPQ_BASE + 18)

/**
 * @brief
 *   Set real cinema
 *
 * @rst
 * Description
 *   There are 3:2 film sequences of 24 Hz original film content.
 *   To convert this to an output of 48 Hz or 72 Hz vertical refresh rate,
 *   this feature reduces the noticeable uneven judder seen on a 3:2 film
 *   sequence. that is converted to 60 Hz vertical refresh rate.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL    // Set PQ Data
 *     VIDIOC_G_CTRL    // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_REAL_CINEMA
 *
 *     //
 *     // parameter
 *     //
 *     int film_mode;
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              IF bFilmMode = TRUE
 *                      Set PQ data for filmmode
 *              Else
 *                      Set PQ data for other mode
 *              IF chip_data != NULL
 *                      Set PQ by chip_data
 *
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_VPQ_REAL_CINEMA ;
 *     control.value = true;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_REAL_CINEMA (V4L2_CID_EXT_VPQ_BASE + 19)

/**
 * @brief
 *   Set color gamut LUT
 *
 * @rst
 * Description
 *   Set color gamut LUT of video standard and panel.
 *
 *   Set signal type and colorimetry information to add info structure
 *   HAL_VPQ_GAMUT_ADD_INFO_T.(Only LG platform)
 *
 *    - pu32Data is address of UINT32 array. Even index is data type, Odd index
 *      is data.
 *
 *      - array[0] = 0x01 (input type)
 *
 *      - array[1] = 0(SDR), 1(Dolby), 2(HDR10), 3(HLG), 4(Technicolor), 5(HDR
 *        Effect)
 *
 *      - array[2] = 0x02 (colorimetry)
 *
 *      - array[3] = 0(colorimetry BT601), 1(colorimetry BT709), 2(colorimetry
 *        BT2020)
 *
 *    - u32Size is size of data. In this case 16(4 array for UINT32)
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_GAMUT_3DLUT
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_gamut_add_info {
 *         unsigned int info_size;
 *         union {
 *             unsigned int *p_info_data; ///< p_data
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *     struct v4l2_ext_vpq_gamut_lut {
 *         unsigned int lut_version;
 *         unsigned int total_section_num;
 *         union {
 *             unsigned short *p_section_data; ///< p_data
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *         struct v4l2_ext_vpq_gamut_add_info add_info;
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              CHECK the value of input parameters
 *              DECLARE locally data structures for executing vendor specific chip API
 *              SET appropriate data on the structure with sample data
 *               HAL_VPQ_GAMUT_LUT_T *chipData;
 *              chipData = ((HAL_VPQ_GAMUT_LUT_T) *pstGamutLut)
 *              CALL the "vendor-specific chip functions" with that vendor-specific data structure which is declared above
 *              IF any error has occurred THEN
 *              RETURN NOT_OK
 *              END IF
 *             RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Gamut 3D LUT
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_gamut_lut gamutLut;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&gamutLut, 0, sizeof(v4l2_ext_vpq_gamut_lut ));
 *     unsigned int addInfo[4];
 *     addInfo[0] = 0x01;
 *     addInfo[1] = hdrMode;
 *     addInfo[2] = 0x02;
 *     addInfo[3] = colorimetry;
 *     v4l2_ext_vpq_gamut_lut.lut_version = 1;
 *     v4l2_ext_vpq_gamut_lut.total_section_num = 1;
 *     v4l2_ext_vpq_gamut_lut.p_section_data = &g3dLut{modelIdx][colorStandard][0];
 *     v4l2_ext_vpq_gamut_lut.add_info.info_size = sizeof(addInfo);
 *     v4l2_ext_vpq_gamut_lut.add_info.info_size = addInfo;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_gamut_lut );
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)gamutLut;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_GAMUT_3DLUT;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_GAMUT_3DLUT (V4L2_CID_EXT_VPQ_BASE + 20)

/**
 * @brief
 *   Write look up table for OD
 *
 * @rst
 * Description
 *   This function writes look up table for OD
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_OD_TABLE
 *
 *     //
 *     // parameter
 *     //
 *     unsigned char pData[OD_LUT_SIZE];
 *     #define LUT_SIZE_OD      // it depends on each platform's H/W LUT size. Not fixed value for all Platform
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *      Write OD look up table with size and pTable.
 *      IF any error has occurred THEN
 *              RETURN NOT_OK
 *      END IF
 *              RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OD
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     unsigned char pOdTable[OD_LUT_SIZE];
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     pqData.version=1;
 *     pqData.length = OD_LUT_SIZE;
 *     pqData.p_data = pOdTable;
 *     pqData.wid=0;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_OD_TABLE;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_OD_TABLE (V4L2_CID_EXT_VPQ_BASE + 21)

/**
 * @brief
 *   Set LUT for OD extension POD or PCID
 *
 * @rst
 * Description
 *   Set LUT for OD extension POD or PCID
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_OD_EXTENSION
 *
 *     //
 *     // parameter
 *     //
 *     *pData        [IN] v4l2_vpq_cmn_data: *pData
 *     struct v4l2_vpq_od_extension
 *     {
 *      unsigned char ext_type;  ///< 0: POD, 1: PCID, 2: NONE
 *      unsigned int ext_length; ///< LUT data size
 *      union
 *      {
 *       unsigned char *p_ext_data; ///< LUT data
 *       unsigned int compat_data;
 *       unsigned long long sizer;
 *      };
 *     };
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OD Extension
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     struct v4l2_vpq_od_extension odExtInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&panelInfo, 0, sizeof(LED_PANEL_INFO_T));
 *     odExtInfo.ext_type = 1; //PCID
 *     odExtInfo.ext_length = LUT_SIZE_PCID;
 *     odExtInfo.p_ext_data = pTable;
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_od_extension);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&odExtInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_OD_EXTENSION;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_OD_EXTENSION (V4L2_CID_EXT_VPQ_BASE + 22)

/**
 * @brief
 *   Set LUT table
 *
 * @rst
 * Description
 *   After dividing the full screen 94x56 Block, control contrast according to
 *   the APL block.
 *
 *   Set LUT table according to the APL block.
 *
 *   only LG Platform
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     struct v4l2_vpq_dc2p_histodata_info
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_LOCALCONTRAST_TABLE
 *
 *     //
 *     // parameter
 *     //
 *     p_data = struct v4l2_ext_vpq_dc2p_localcontrast_table
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Set Local Contrast Talbe
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Local Contrast Table
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_dc2p_localcontrast_table localcontTable;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&localcontTable, 0, sizeof(v4l2_vpq_dc2p_localcontrast_table ));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_dc2p_localcontrast_table );
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&localcontTable;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_LOCALCONTRAST_TABLE;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_LOCALCONTRAST_TABLE (V4L2_CID_EXT_VPQ_BASE + 23)

/**
 * @brief
 *   Set master gain
 *
 * @rst
 * Description
 *   After dividing the full screen 94x56 Block, control contrast according to
 *   the APL block.
 *   Set master gain according to DC UI(off,low,medium,High)
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_LOCALCONTRAST_DATA
 *
 *     //
 *     // parameter
 *     //
 *     p_data = chipdata
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *              Set Local Contrast Parameter
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Local Contrast Param
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_dc2p_localcontrast_param localcontParam;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&localcontParam, 0, sizeof(v4l2_vpq_dc2p_localcontrast_param));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_dc2p_localcontrast_param );
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&localcontParam;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_LOCALCONTRAST_DATA;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_LOCALCONTRAST_DATA (V4L2_CID_EXT_VPQ_BASE + 24)

/**
 * @brief
 *   Control the contrast between the object and the background
 *
 * @rst
 * Description
 *   After detect edge andd find object, control the contrast between the
 *   object and the background.
 *
 *   only LG Platform
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_PSP
 *
 *     //
 *     // parameter
 *     //
 *     1. LG
 *     struct v4l2_vpq_dc2p_perspective{
 *      unsigned char map_master_gain;
 *      unsigned char position_lut_x[16];
 *     };
 *
 *     2. RTK
 *     Not Use
 *
 *     3. MTK
 *     Not Use
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *               Set Psp
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Psp
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_dc2p_perspect pspdata;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&panelInfo, 0, sizeof(v4l2_vpq_dc2p_perspective));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_dc2p_perspective);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&pspdata;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_PSP;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_PSP (V4L2_CID_EXT_VPQ_BASE + 25)

/**
 * @brief
 *   Color Gamut Mapping
 *
 * @rst
 * Description
 *   Color Gamut Mapping (3x3)(HDR Bloc Pcc).
 *   Set 9 Gain setting value corresponding to 3x3.
 *   matrix value scale up x10000. (ex: 1.0 ==> 10000)
 *   signed short range is -32,768 to 32,767. 10,000 means 1.0 in float type.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_GAMUT_MATRIX_PRE
 *
 *     //
 *     // parameter
 *     //
 *     short sMatrix[9]
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set Gamut Matrix(HDR PCC)
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     float fMatrx[9] = {0,};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     GetMatrixPre(fMatrx);
 *     pqData.version=1;
 *     pqData.length = sizeof(fMatrx);
 *     pqData.wId=0;
 *     pqData.p_data = (char*)fMatrx;
 *     ext_control.id = V4L2_CID_EXT_VPQ_GAMUT_MATRIX_PRE;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_GAMUT_MATRIX_PRE (V4L2_CID_EXT_VPQ_BASE + 26)

/**
 * @brief
 *   Setting Color Gamut Mapping 3x3 matrix (CC Block PCC), gamma, degamma, mux
 *   setting
 *
 * @rst
 * Description
 *   Setting Color Gamut Mapping 3x3 matrix (CC Block PCC), gamma, degamma, mux
 *   setting.
 *
 *   gamma: set enable/disable gamma function front of PCC
 *
 *   degamma: set enable/disable degamma function back of PCC
 *
 *   matrix: set 3x3 matrix. each value scale up x10000. (ex: 1.0 ==> 10000)
 *
 *   mux_blend: set blending option for 3DLUT and 3x3 matrix result
 *
 *   pst_chip_data: Implementation guide for each vendor state additional data
 *   structure for necessary.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_GAMUT_MATRIX_POST
 *
 *     //
 *     // parameter
 *     //
 *     *pGamutData         [IN] v4l2_vpq_gamut_post
 *     #define CSC_MUX_LUT_SIZE 4
 *     struct v4l2_vpq_csc_mux_lut{
 *      unsigned int mux_oetf_out;
 *     };
 *     struct v4l2_vpq_gamut_post{
 *      struct v4l2_vpq_csc_mux_lut mux_blend;
 *             unsigned char *pst_chip_data;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set Gamut Matrix(SDR PCC)
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OD Extension
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_vpq_gamut_post gamutPost;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(pqData));
 *     memset(&gamutPost, 0, sizeof(gamutPost));
 *     //gamut mapping post
 *     PQL_DB_GetColorGamutParam(&gamutPost);
 *     pqData.version=1;
 *     pqData.length = sizeof(gamutPost);
 *     pqData.wId=0;
 *     pqData.pData=(UINT8*)&gamutPost;
 *     ext_control.id = V4L2_CID_EXT_VPQ_GAMUT_MATRIX_POST;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_GAMUT_MATRIX_POST (V4L2_CID_EXT_VPQ_BASE + 27)

/**
 * @brief
 *   setting mode info of PQ
 *
 * @rst
 * Description
 *   This HAL function for setting mode info of PQ
 *
 *     - UINT32 array which length is 5
 *
 *     - 1st entity for HDR mode, 2nd entity for Colorimetry info
 *
 *     - remains for reserved
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_PQ_MODE_INFO
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_colorimetry_info {
 *      V4L2_EXT_COLORIMETRY_MAX
 *     };
 *     enum v4l2_ext_hdr_mode {
 *         V4L2_EXT_HDR_MODE_SDR,
 *         V4L2_EXT_HDR_MODE_DOLBY,
 *         V4L2_EXT_HDR_MODE_HDR10,
 *         V4L2_EXT_HDR_MODE_HLG,
 *         V4L2_EXT_HDR_MODE_TECHNICOLOR,
 *         V4L2_EXT_HDR_MODE_HDREFFECT,
 *         V4L2_EXT_HDR_MODE_MAX
 *     };
 *     struct v4l2_ext_pq_mode_info {
 *      unsigned int supportPrime;  // support Prime
 *     };
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set Mode info for PQ
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OD Extension
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_pq_mode_info pqModeInfo;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(pqData));
 *     memset(&pqModeInfo, 0, sizeof(pqModeInfo));
 *     pqModeInfo.hdrStatus = get
 *     pqModeInfo.colorimetry;   // see v4l2_ext_colorimetry_info
 *     pqModeInfo.peakLuminance;    // peakLuminance of panel
 *     pqModeInfo.supportPrime;  // support Prime
 *     pqData.version=1;
 *     pqData.length = sizeof(pqModeInfo);
 *     pqData.wId=0;
 *     pqData.pData=(UINT8*)&pqModeInfo;
 *     ext_control.id = V4L2_CID_EXT_VPQ_PQ_MODE_INFO;
 *     ext_control.ptr = (void *)&pqData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_PQ_MODE_INFO (V4L2_CID_EXT_VPQ_BASE + 28)
#define V4L2_CID_EXT_VPQ_DEGAMMA_DATA (V4L2_CID_EXT_VPQ_BASE + 29)

/**
 * @brief
 *   Control OLED 11bit
 *
 * @rst
 * Description
 *   The function control OLED 11bit.
 *   Luminence boost up  for OLED HDR.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_LUMINANCE_BOOST
 *     //
 *     // parameter
 *     //
 *     p_data = chipdata
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set Luminence boost LUT
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OLED Luminance Boost
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_vpq_post_hdr_lut v4l2PostHdrLut;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&v4l2PostHdrLut, 0, sizeof(v4l2_vpq_post_hdr_lut));
 *     PQDB_Get_DB_Data(&v4l2PostHdrLut);
 *     pqData.version = V4L2_CID_EXT_VPQ_LUMINANCE_BOOST_VERSION;
 *     pqData.length = sizeof(v4l2_vpq_post_hdr_lut);
 *     pqData.wid=0;
 *     pqData.pData = (unsigned char *)&v4l2PostHdrLut;;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_LUMINANCE_BOOST;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_LUMINANCE_BOOST (V4L2_CID_EXT_VPQ_BASE + 30)

/**
 * @brief
 *   Get object data for OBC
 *
 * @rst
 * Description
 *   Get object data for OBC
 *
 *   only LG Platform
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_OBC_DATA
 *
 *     //
 *     // parameter
 *     //
 *     1. LG
 *     struct v4l2_vpq_obc_data
 *     {
 *         unsigned char object_relibility;
 *      unsigned short object_bg_pel_num;
 *      unsigned char object_boxh1;
 *      unsigned short object_data03;
 *      unsigned short object_data07;
 *      unsigned short  object_data11;
 *     };
 *
 *     2. RTK
 *     Not Use
 *
 *     3. MTK
 *     Not Use
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   This function is only supported by LG platform.
 *   Other chip will implement it as dummy.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Get OBC Data
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get OBC Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_obc_data obcdata;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&obcdata, 0, sizeof(v4l2_vpq_obc_data));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_obc_data);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&obcdata;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_OBC_DATA;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_OBC_DATA (V4L2_CID_EXT_VPQ_BASE + 31)

/**
 * @brief
 *   Set FG/BG  LUT factor for OBC
 *
 * @rst
 * Description
 *   Set FG/BG  LUT factor for OBC
 *
 *   only LG Platform
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ info
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_OBC_LUT
 *
 *     //
 *     // parameter
 *     //
 *     1. LG
 *     struct v4l2_vpq_obc_lut
 *     {
 *         unsigned short object_fg_x[8];
 *     };
 *
 *     2. RTK
 *     Not Use
 *
 *     3. MTK
 *     Not Use
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   This function is only supported by LG platform.
 *   Other chip will implement it as dummy.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set FG/BG LUT factor
 *             v4l2_vpq_cmn_data *chipData;
 *             chipData = (v4l2_vpq_cmn_data ) *pData;
 *             CALL the "vendor-specific chip functions" with that vendor-specific data structure
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OBC LUT
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_obc_lut obcLut;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data ));
 *     memset(&obcLut, 0, sizeof(v4l2_vpq_obc_lut ));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_obc_lut );
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&obcLut;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_OBC_LUT;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS     , &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_OBC_LUT (V4L2_CID_EXT_VPQ_BASE + 32)

/**
 * @brief
 *   Set FG/BG control factor for OBC
 *
 * @rst
 * Description
 *   Set FG/BG control factor for OBC
 *
 *   only LG Platform
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ info
 *     VIDIOC_G_EXT_CTRLS       // Get PQ info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_OBC_CTRL
 *
 *     //
 *     // parameter
 *     //
 *     1. LG
 *     struct v4l2_vpq_obc_ctrl
 *     {
 *         bool bOBC_enable;
 *      unsigned char fg_gain_y1;
 *      unsigned char bg_gain_y1;
 *      bool bOBC_sat_enable;
 *      unsigned short reg_yy_lut_pts_x2;
 *      unsigned char char reg_yy_lut_pts_y2;
 *      unsigned char reg_cc_lut_pts_x0;
 *      unsigned char reg_cc_lut_pts_y0;
 *
 *      unsigned char reg_cbst_lut_pts_x2;
 *      unsigned char reg_cbst_lut_pts_y2;
 *     };
 *
 *     2. RTK
 *     Not Use
 *
 *     3. MTK
 *     Not Use
 *
 * Return Value
 *    If the function succeeds, the return value is OK.
 *
 *    If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   This function is only supported by LG platform.
 *   Other chip will implement it as dummy.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *             Set FG/BG  control factor
 *             v4l2_vpq_cmn_data *chipData;
 *             chipData = (v4l2_vpq_cmn_data) *pData;
 *             CALL the "vendor-specific chip functions" with that vendor-specific data structure
 *              IF any error has occurred THEN
 *                        RETURN NOT_OK
 *              END IF
 *                        RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OBC Ctrl
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     v4l2_vpq_obc_ctrl obccttrl;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&obccttrl, 0, sizeof(v4l2_vpq_obc_ctrl));
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_vpq_obc_ctrl);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&obccttrl;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_OBC_CTRL;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *     // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_OBC_CTRL (V4L2_CID_EXT_VPQ_BASE + 33)

/**
 * @brief
 *   Set Decontour
 *
 * @rst
 * Description
 *   Set Decontour.
 *   set the required Registers(Decontour hresholds, and etc).
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_DECONTOUR
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_vpq_decontour_data{
 *          unsigned int compat_data;
 *      };
 *     // container : v4l2_ext_vpq_cmn_data
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 *   In case of 2-chip struct. AP will use a member "wId" of struct
 *   v4l2_ext_vpq_cmn_data to distinguish it's target chip.
 *
 *   wId : 0 - main chip's main-path
 *
 *   wId : 1 - main chip's sub-path (reserved for future)
 *
 *   wId : 2 - sub chip's main-path
 *
 *   wId : 3 - sub chip's sub-path (reserved for future)
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Picture Control
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_vpq_cmn_data pqData;
 *     struct v4l2_ext_vpq_decontour_data decontour;
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     memset(&decontour, 0, sizeof(v4l2_ext_vpq_decontour_data));
 *     decontour.ui_value= decontourUiValue;
 *     decontour.chip_data = PQL_DB_....();
 *     pqData.version=1;
 *     pqData.length = sizeof(v4l2_ext_vpq_ctrl_data);
 *     pqData.wid=0;
 *     pqData.p_data=(UINT8*)&decontour;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_DECONTOUR;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DECONTOUR (V4L2_CID_EXT_VPQ_BASE + 34)

/**
 * @brief
 *   Set extra inner pattern display
 *
 * @rst
 * Description
 *   For special use case(eg. PQ calibration), TV wants to display BSP's extra
 *   internal pattern. This is an extra inner pattern displaying requirement.
 *   (different pattern type to IREInnerPattern)
 *
 *   We need two kinds of pattern.
 *
 *   1) multiple window fill box
 *
 *   2) multiple line gradiation
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set PQ Data
 *     VIDIOC_G_EXT_CTRLS      // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     VV4L2_CID_EXT_VPQ_EXTRA_PATTERN
 *
 *     //
 *     // parameter
 *     //
 *     stContainer [IN] v4l2_vpq_cmn_data
 *     cf... (struct v4l2_vpq_ext_pattern_info *)stContainer.p_data
 *     enum V4L2_VPQ_EXT_PATTERN_MODE {
 *       V4L2_VPQ_EXT_PATTERN_WINBOX,
 *       V4L2_VPQ_EXT_PATTERN_GRADATION,
 *       V4L2_VPQ_EXT_PATTERN_MAX
 *     };
 *
 *     enum V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION {
 *       V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION_HORIZONTAL,
 *       V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION_VERTICAL,
 *       V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION_MAX
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_gradation_line_attr{
 *       unsigned char lineIdx; // gradation line index
 *       unsigned short start_R; // 1st gradation block's red level as a 10bit resolution
 *       unsigned short start_G; // 1st gradation block's green level as a 10bit resolution
 *       unsigned short start_B; // 1st gradation block's blue level as a 10bit resolution
 *       unsigned short step_R; // step size for next gradation block
 *       unsigned short step_G; // step size for next gradation block
 *       unsigned short step_B; // step size for next gradation block
 *       unsigned short strideSize; // gradation block's width(horizontal mode)/height(vertical mode)
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_gradation_info{
 *       unsigned char numGrad; // number of gradation lines in a screen
 *       enum V4L2_VPQ_EXT_PATTERN_GRADATION_DIRECTION eGradMode;
 *       struct v4l2_vpq_ext_pattern_gradation_line_attr stLineAttr[MAX_EXT_PATTERN_GRADATION_LINE];
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_winbox_win_attr{
 *       unsigned short winIdx; // window layer index. 0:background
 *       unsigned short x;
 *       unsigned short y;
 *       unsigned short w;
 *       unsigned short h;
 *       unsigned short fill_R, // 10 bit resolution
 *       unsigned short fill_G, // 10 bit resolution
 *       unsigned short fill_B, // 10 bit resolution
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_winbox_info{
 *       unsigned char u8NumWin; // number of windows in a screen(including background window)
 *       struct v4l2_vpq_ext_pattern_winbox_win_attr stWinBoxAttr[MAX_EXT_PATTERN_WINBOX];
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_info{
 *       unsigned char bOnOff;
 *       enum V4L2_VPQ_EXT_PATTERN_MODE eMode;
 *       struct v4l2_vpq_ext_pattern_gradation_info* pstGradInfo;
 *       struct v4l2_vpq_ext_pattern_winbox_info* pstWinboxInfo;
 *     };
 *
 *     typedef struct v4l2_vpq_ext_pattern_info_v2 {
 *       unsigned char bOnOff;
 *       enum V4L2_VPQ_EXT_PATTERN_MODE eMode;
 *       struct v4l2_vpq_ext_pattern_gradation_info stGradInfo;
 *       struct v4l2_vpq_ext_pattern_winbox_info stWinboxInfo;
 *     };
 *
 *     // chip dependent settings
 *     #define SUPPORT_EXT_INNER_PATTERN
 *     #ifdef SUPPORT_EXT_INNER_PATTERN
 *     #define MAX_EXT_PATTERN_GRADATION_LINE (4) // it depends on chip limitation
 *     #define MAX_EXT_PATTERN_WINBOX (10) // it depends on chip limitation
 *     #define MAX_EXT_PATTERN_GRADATION_H_STRIDE_SIZE (254) // it depends on chip limitation
 *     #define EXT_PATTERN_GRADATION_H_STRIDE_STEP (2) // it depends on chip limitation
 *     #define MAX_EXT_PATTERN_GRADATION_V_STRIDE_SIZE (127) // it depends on chip limitation
 *     #define EXT_PATTERN_GRADATION_V_STRIDE_STEP (1) // it depends on chip limitation
 *     #define EXT_PATTERN_WIDTH (3840) // it depends on chip limitation
 *     #define EXT_PATTERN_HEIGHT (2160) // it depends on chip limitation
 *     #endif
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the Generic Error Codes chapter.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set multi-window fill box inner pattern
 *     v4l2_ext_controls ext_controls;
 *     struct v4l2_vpq_cmn_data stPqContainer;
 *     struct v4l2_vpq_ext_pattern_info_v2 stRealInfo;
 *     ...
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&stPqContainer, 0, sizeof(stPqContainer));
 *     memset(&stRealInfo, 0, sizeof(stRealInfo));
 *     stRealInfo.bOnOff = TRUE;
 *     stRealInfo.eMode = V4L2_VPQ_EXT_PATTERN_WINBOX;
 *     ...
 *     stPqContainer.version=1;
 *     stPqContainer.length = sizeof(stRealInfo);
 *     stPqContainer.wId=0;
 *     stPqContainer.p_data=(unsigned char *)&stRealInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = VV4L2_CID_EXT_VPQ_EXTRA_PATTERN;
 *     ext_controls.controls->ptr = (void *)&stPqContainer;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_EXTRA_PATTERN (V4L2_CID_EXT_VPQ_BASE + 35)

/**
 * @brief
 *   Stereo Face effect for AI picture
 *
 * @rst
 * Description
 *   Stereo Face effect for AI picture
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set PQ Data
 *     VIDIOC_G_CTRL        // Get PQ Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_STEREO_FACE_CTRL
 *
 *     //
 *     // parameter
 *     //
 *     V4L2_CID_EXT_VPQ_STEREO_FACE_CTRL {
 *     #define V4L2_VPQ_EXT_STEREO_FACE_OFF   (0)             // off
 *     #define V4L2_VPQ_EXT_STEREO_FACE_ON    (1)             // normal on
 *     #define V4L2_VPQ_EXT_STEREO_FACE_DEMO  (2)             // demo mode
 *     }
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Remarks
 *   In this function, there are some access the LG DB to get the data set for
 *   vendor-specific chip API. But this process should not be concerned by chip
 *   vendor. Thus, at the first of this function, the "chip-specific data
 *   structure" should be declared locally. The "chip-specific data structure"
 *   is identical to the data structure which is used as the input parameter of
 *   the vendor-specific chip API function. And, after the declaration the
 *   structure, the data for "chip-specific data structure" have to be assigned
 *   with the best sample values. The values is to make the vendor-specific
 *   chip API function work normally.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_VPQ_STEREO_FACE_CTRL;
 *     control.value = V4L2_VPQ_EXT_STEREO_FACE_ON;
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_STEREO_FACE_CTRL (V4L2_CID_EXT_VPQ_BASE + 36)

/**
 * @brief Get video latency timing info
 *
 * @rst
 * This function gets video latency time by scaler, PQ, panel for adjust Audio, Video sync.
 * Returning time unit is 1/1000second(ms)
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRLS      // Get video latency
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_VIDEO_LATENCY
 *
 *     //
 *     // parameter
 *     //
 *     v4l2_control.value [OUT] millisecond second of video latency
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *   This control id supports VIDIOC_G_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get video latency
 *     v4l2_control v4l2_video_latency;
 *     v4l2_video_latency.id = V4L2_CID_VPQ_VIDEO_LATENCY;
 *
 *     ioctl(fd, VIDIOC_G_CTRLS, &v4l2_video_latency);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_VIDEO_LATENCY (V4L2_CID_EXT_VPQ_BASE + 37)

/**
 * @brief Set PQDB DC data to driver
 *
 * @rst
 * This function tranfer PQDB Dynamic Contrast data to backend chip(FRC)
 * It will be used for only LG platform
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     V4L2_CID_EXT_VPQ_DB_DATA
 *
 *     //
 *     // parameter
 *     //
 *     stContainer          [IN]      v4l2_vpq_cmn_data
 *     cf... (struct v4l2_ext_cm_info *)stContainer.p_data *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can Set data using VIDIOC_S_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_S_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set OBC Ctrl
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     struct pqdb_dc_db_f20 pqdbData;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     PQDB_Get_DB_Data(&pqdbData);
 *     pqData.version=1;
 *     pqData.length = sizeof(pqdbData);
 *     pqData.wid=0;
 *     pqData.p_data=(unsigned char*)&pqdbData;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_DB_DATA;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_DB_DATA (V4L2_CID_EXT_VPQ_BASE + 38)

/**
 * @brief Subscribe Video latency time
 *
 * This function is used to adjust audio and video sync. Driver should make the
 * EVENT of file_handler whenever video latency time changed. For example input,
 * TruMotion, game mode changed.
 * Don't create EVENT while doing input chage. EVENT should be created by
 * stable information of input timing after input change finished.
 * Time unit is 1/1000second(ms)
 *
 *   - After the event of "V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY" is registered by
 *     the command of "VIDIOC_SUBSCRIBE_EVENT" in driver, driver should generate
 *     the event of "V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY" when video latency time
 *     changed
 *
 *   - Driver should implement epoll() operation for this flow. You need to
 *     implement the feature of epoll() in driver side. User client gets the
 *     kernel event from epoll().
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT
 *     VIDIOC_UNSUBSCRIBE_EVENT
 *     VIDIOC_DQEVENT
 *
 *     //
 *     // v4l2_event_subscription type
 *     //
 *     V4L2_EVENT_CTRL
 *
 *     //
 *     // v4l2_event_subscription id
 *     //
 *     // You have to make the new v4l2_event_subscription id as the below.
 *     V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY
 *
 *     //
 *     // Parameter
 *     //
 *     // Use data[64] in the struct v4l2_event in videodev2.h
 *     // This struct is used by VIDIOC_DQEVENT.
 *     struct v4l2_event {
 *         __u32               type;
 *         union {
 *             struct v4l2_event_vsync     vsync;
 *             struct v4l2_event_ctrl      ctrl;
 *             struct v4l2_event_frame_sync    frame_sync;
 *             struct v4l2_event_src_change    src_change;
 *             struct v4l2_event_motion_det    motion_det;
 *             __u8                data[64];
 *         } u;
 *         __u32               pending;
 *         __u32               sequence;
 *         struct timespec         timestamp;
 *         __u32               id;
 *         __u32               reserved[8];
 *     };
 *
 *     // data[0] - low bit of time
 *     // data[1] - high bit of time.
 *     // data[2~63] - reserved
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set event using
 *   VIDIOC_SUBSCRIBE_EVENT/VIDIOC_UNSUBSCRIBE_EVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set subscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY;
 *
 *     ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &event);
 *
 *     // Set unsubscribe
 *     struct v4l2_event_subscription event{0};
 *
 *     event.type = V4L2_EVENT_CTRL;
 *     event.id = V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY;
 *
 *     ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &event);
 *
 *     //de queue
 *     struct v4l2_event ev{0};
 *     ioctl(fd, VIDIOC_DQEVENT, &ev);
 *
 *     if(ev.id == V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY)
 *     {
 *         uint16 videoLatency = ev.u.data[1] << 8 + ev.u.data[0];
 *         // use videoLatency
 *         // luna send to audio service
 *     }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_SUBSCRIBE_VIDEO_LATENCY (V4L2_CID_EXT_VPQ_BASE + 39)

/**
  * @brief
  *   Control OLED 11bit apl gain
  *
  * @rst
  * Description
  *   The function control OLED 11bit apl gain
  *   Luminence boost up  for OLED HDR.
  *
  * Command & Parameters
  *   .. code-block:: cpp
  *
  *     //
  *     // ioctl command
  *     //
  *     VIDIOC_S_EXT_CTRLS       // Set PQ Data
  *     VIDIOC_G_EXT_CTRLS       // Get PQ Data
  *     //
  *     // control id
  *     //
  *     // You have to make the new control id as the below.
  *     V4L2_CID_EXT_VPQ_PHDR_APL_GAIN_LUT
  *     //
  *     // parameter
  *     //
  *     p_data = chipdata
  *
  * Return Value
  *    If the function succeeds, the return value is OK.
  *
  *    If the function fails, the return value is NOT_OK.
  *
  * Pseudo Code
  *   .. code-block:: cpp
  *
  *     {
  *             Set Luminence boost LUT
  *              IF any error has occurred THEN
  *                        RETURN NOT_OK
  *              END IF
  *                        RETURN OK
  *     }
  *
  * Example
  *   .. code-block:: cpp
  *
  *     // Set OLED Luminance Boost
  *     v4l2_ext_controls ext_controls;
  *     v4l2_ext_control ext_control;
  *     struct v4l2_vpq_cmn_data pqData;
  *     struct v4l2_vpq_phdr_apl_gain_lut v4l2PostHdrAplGain;
  *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
  *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
  *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
  *     memset(&v4l2PostHdrAplGain, 0, sizeof(v4l2_vpq_phdr_apl_gain_lut));
  *     PQDB_Get_DB_Data(&v4l2PostHdrAplGain);
  *     pqData.version=1;
  *     pqData.length = sizeof(v4l2_vpq_phdr_apl_gain_lut);
  *     pqData.wid=0;
  *     pqData.pData = (unsigned char *)&v4l2PostHdrAplGain;;
  *     ext_controls.count = 1;
  *     ext_controls.controls = &ext_control;
  *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_PHDR_APL_GAIN_LUT;
  *     ext_controls.controls->ptr = (void *)&pqData;
  *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
  * @endrst
  */
#define V4L2_CID_EXT_VPQ_PHDR_APL_GAIN_LUT (V4L2_CID_EXT_VPQ_BASE + 40)

#define V4L2_CID_EXT_AIPQ_BASE (V4L2_CID_USER_EXT_PQ_BASE + 0x500)

/**
 * @brief Set/Get SQM Mode for AI picture
 *
 * @rst
 * Description
 *   Set/Get SQM Mode for AI picture
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Set AIPQ Mode
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_SQM_MODE
 *
 *     //
 *     // parameter
 *     //
 *     0(Off), 1(On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set/get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_SQM_MODE;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_SQM_MODE (V4L2_CID_EXT_AIPQ_BASE + 0)

/**
 * @brief Set/Get Super Resolution Mode for AI picture
 *
 * @rst
 * Description
 *   Set/Get Super Resolution Mode for AI picture
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Set AIPQ Mode
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_SR_MODE
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_aipq_mode {
 *          V4L2_EXT_AIPQ_MODE_OFF = 0,
 *          V4L2_EXT_AIPQ_MODE_ON = 1,
 *          V4L2_EXT_AIPQ_MODE_DEMO_ON = 2,
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set/get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_SR_MODE;
 *     control.value = V4L2_EXT_AIPQ_MODE_ON;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_SR_MODE (V4L2_CID_EXT_AIPQ_BASE + 1)

/**
 * @brief Set/Get Depth for AI picture
 *
 * @rst
 * Description
 *   Set/Get Depth for AI picture
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Set AIPQ Mode
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_DEPTH_MODE
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_aipq_mode {
 *          V4L2_EXT_AIPQ_MODE_OFF = 0,
 *          V4L2_EXT_AIPQ_MODE_ON = 1,
 *          V4L2_EXT_AIPQ_MODE_DEMO_ON = 2,
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set/get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_DEPTH_MODE;
 *     control.value = V4L2_EXT_AIPQ_MODE_ON;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_DEPTH_MODE (V4L2_CID_EXT_AIPQ_BASE + 2)

/**
 * @brief Set/Get SCENE Detect Mode for AI picture
 *
 * @rst
 * Description
 *   Set/Get SCENE Detect Mode for AI picture
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Set AIPQ Mode
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_SCENE_MODE
 *
 *     //
 *     // parameter
 *     //
 *     0(Off), 1(On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set/get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_SCENE_MODE;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_SCENE_MODE (V4L2_CID_EXT_AIPQ_BASE + 3)

/**
 * @brief Get a result of SCENE Detect
 *
 * @rst
 * Description
 *   Get a scene_info of SCENE Detect
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_SCENE_INFO
 *
 *     //
 *     // parameter
 *     //enum v4l2_ext_aipq_scene_info
 *     {
 *         V4L2_EXT_AIPQ_SCENE_STANDARD = 0,
 *         V4L2_EXT_AIPQ_SCENE_BUILDING = 1,
 *         V4L2_EXT_AIPQ_SCENE_LANDSCAPE = 2,
 *         V4L2_EXT_AIPQ_SCENE_NIGHT = 3,
 *         V4L2_EXT_AIPQ_SCENE_INFO_MAX
 *     };
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *   This control id supports VIDIOC_G_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_SCENE_INFO;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_SCENE_INFO (V4L2_CID_EXT_AIPQ_BASE + 4)


/**
 * @brief Set/Get Genre Selection Mode for AI picture
 *
 * @rst
 * Description
 *   Set/Get Genre Selection Mode for AI picture
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Set AIPQ Mode
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_GENRE_MODE
 *
 *     //
 *     // parameter
 *     //
 *     0(Off), 1(On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set/get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_GENRE_MODE;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_GENRE_MODE (V4L2_CID_EXT_AIPQ_BASE + 5)

/**
 * @brief Get a result of Auto Genre Selection
 *
 * @rst
 * Description
 *   Get a genre_info of Auto Genre Selection
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL      // Get AIPQ Mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AIPQ_GENRE_INFO
 *
 *     //
 *     // parameter
 *     //enum v4l2_ext_aipq_genre_info
 *     {
 *         V4L2_EXT_AIPQ_GENRE_STANDARD = 0,
 *         V4L2_EXT_AIPQ_GENRE_SPORTS = 1,
 *         V4L2_EXT_AIPQ_GENRE_ANIMATION = 2,
 *         V4L2_EXT_AIPQ_GENRE_MOVIE = 3,
 *         V4L2_EXT_AIPQ_GENRE_INFO_MAX
 *     };
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRLS and this control id.
 *   This control id supports VIDIOC_G_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control control;
 *     control.id = V4L2_CID_EXT_AIPQ_GENRE_INFO;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AIPQ_GENRE_INFO (V4L2_CID_EXT_AIPQ_BASE + 6)

/**
 * @brief
 *   Get information of video pattern
 *
 * @rst
 * Description
 *   Get information of video pattern.
 *
 * Command & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get info of video pattern
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VPQ_VIDEO_PATTERN_INFO
 *
 *     //
 *     // parameter
 *     //
 *     char* pPatternInfo         [IN]
 *
 *
 * Return Value
 *   If the function succeeds, the return value is OK.
 *
 *   If the function fails, the return value is NOT_OK.
 *
 * Pseudo Code
 *   .. code-block:: cpp
 *
 *     {
 *
 *       IF patternInfo == NULL THEN
 *           RETURN NOT_OK
 *       END IF
 *       IF pqData.length < (size of patternInfo) THEN
 *           RETURN NOT_OK
 *       END IF
 *
 *       COPY patternInfo to pqData.p_data
 *       WRITE size of patternInfo to pqData.length

 *       RETURN OK
 *     }
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Histo Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     v4l2_vpq_cmn_data pqData;
 *     char patternInfo[500] = {0,};
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pqData, 0, sizeof(v4l2_vpq_cmn_data));
 *     pqData.version = 1;
 *     pqData.length = sizeof(patternInfo);
 *     pqData.p_data =(unsigned char*)patternInfo;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VPQ_VIDEO_PATTERN_INFO;
 *     ext_controls.controls->ptr = (void *)&pqData;
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls); // implement
 *
 * @endrst
 */
#define V4L2_CID_EXT_VPQ_VIDEO_PATTERN_INFO (V4L2_CID_USER_EXT_PQ_BASE + 0x601)





/* VBE class control IDs */


#define V4L2_CID_EXT_VBE_BASE (V4L2_CID_USER_EXT_VBE_BASE)
/**
 * @brief Init VBE when cold on
 *
 * @rst
 * This function can init VBE
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_INIT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   // VBE
 *	  enum v4l2_ext_vbe_panel_inch {
 *		 V4L2_EXT_VBE_PANEL_INCH_22 = 0,
 *		 V4L2_EXT_VBE_PANEL_INCH_23,
 *		 V4L2_EXT_VBE_PANEL_INCH_24,
 *		 V4L2_EXT_VBE_PANEL_INCH_26,
 *		 V4L2_EXT_VBE_PANEL_INCH_27,
 *		 V4L2_EXT_VBE_PANEL_INCH_28,
 *		 V4L2_EXT_VBE_PANEL_INCH_32,
 *		 V4L2_EXT_VBE_PANEL_INCH_39,
 *		 V4L2_EXT_VBE_PANEL_INCH_40,
 *		 V4L2_EXT_VBE_PANEL_INCH_42,
 *		 V4L2_EXT_VBE_PANEL_INCH_43,
 *		 V4L2_EXT_VBE_PANEL_INCH_47,
 *		 V4L2_EXT_VBE_PANEL_INCH_49,
 *		 V4L2_EXT_VBE_PANEL_INCH_50,
 *		 V4L2_EXT_VBE_PANEL_INCH_55,
 *		 V4L2_EXT_VBE_PANEL_INCH_58,
 *		 V4L2_EXT_VBE_PANEL_INCH_60,
 *		 V4L2_EXT_VBE_PANEL_INCH_65,
 *		 V4L2_EXT_VBE_PANEL_INCH_70,
 *		 V4L2_EXT_VBE_PANEL_INCH_75,
 *		 V4L2_EXT_VBE_PANEL_INCH_77,
 *		 V4L2_EXT_VBE_PANEL_INCH_79,
 *		 V4L2_EXT_VBE_PANEL_INCH_84,
 *		 V4L2_EXT_VBE_PANEL_INCH_86,
 *		 V4L2_EXT_VBE_PANEL_INCH_98,
 *		 V4L2_EXT_VBE_PANEL_INCH_105,
 *		 V4L2_EXT_VBE_PANEL_INCH_48, // 17Y Added
 *		 V4L2_EXT_VBE_PANEL_INCH_MAX
 *	   };
 *
 *	   enum v4l2_ext_vbe_panel_maker {
 *		 V4L2_EXT_VBE_PANEL_MAKER_LGD = 0,
 *		 V4L2_EXT_VBE_PANEL_MAKER_AUO,
 *		 V4L2_EXT_VBE_PANEL_MAKER_SHARP,
 *		 V4L2_EXT_VBE_PANEL_MAKER_BOE,
 *		 V4L2_EXT_VBE_PANEL_MAKER_CSOT,
 *		 V4L2_EXT_VBE_PANEL_MAKER_INNOLUX,
 *		 V4L2_EXT_VBE_PANEL_MAKER_LGD_M,
 *		 V4L2_EXT_VBE_PANEL_MAKER_ODM_B,
 *		 V4L2_EXT_VBE_PANEL_MAKER_BOE_TPV,
 *		 V4L2_EXT_VBE_PANEL_MAKER_HKC,
 *		 V4L2_EXT_VBE_PANEL_MAKER_MAX
 *	 };
 *
 *  	  //Panel interface
 *	  enum v4l2_ext_vbe_panel_interface {
 *		 V4L2_EXT_VBE_PANEL_EPI = 0,
 *		 V4L2_EXT_VBE_PANEL_LVDS,
 *		 V4L2_EXT_VBE_PANEL_Vx1,
 *		 V4L2_EXT_VBE_PANEL_CEDS,
 *		 V4L2_EXT_VBE_PANEL_EPI_QSAC,
 *		 V4L2_EXT_VBE_PANEL_RESERVED1,
 *		 V4L2_EXT_VBE_PANEL_RESERVED2,
 *		 V4L2_EXT_VBE_PANEL_INTERFACE_MAX
 *	   };
 *
 *	  // Panel Resolution
 *	  enum v4l2_ext_vbe_panel_resolution {
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_1024X768 = 0,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_1280X720,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_1366X768,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_1920X1080,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_2560X1080,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_3840X2160,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_5120X2160,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_7680X4320,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_RESERVED1,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_RESERVED2,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_RESERVED3,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_RESERVED4,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_RESERVED5,
 *		 V4L2_EXT_VBE_PANEL_RESOLUTION_MAX
 *	  };
 *
 * 	  //Panel version
 *	  enum v4l2_ext_vbe_panel_version {
 *		 V4L2_EXT_VBE_PANEL_NONE = 0,
 *		 V4L2_EXT_VBE_PANEL_V12,
 *		 V4L2_EXT_VBE_PANEL_V13,
 *		 V4L2_EXT_VBE_PANEL_V14,
 *		 V4L2_EXT_VBE_PANEL_V15,
 *		 V4L2_EXT_VBE_PANEL_V16,
 *		 V4L2_EXT_VBE_PANEL_V17,
 *		 V4L2_EXT_VBE_PANEL_V18,
 *		 V4L2_EXT_VBE_PANEL_MAX
 *	  };
 *
 * 	  //Panel Framerate
 *	  enum v4l2_ext_vbe_panel_framerate {
 *		V4L2_EXT_VBE_PANEL_FRAMERATE_60HZ,
 *		 V4L2_EXT_VBE_PANEL_FRAMERATE_120HZ,
 *		 V4L2_EXT_VBE_PANEL_FRAMERATE_MAX
 *	  };
 *
 *	  //Backlight Type Enum  related with Tool OPT
 *	  enum v4l2_ext_vbe_panel_backlight_type {
 *		 V4L2_EXT_VBE_PANEL_BL_DIRECT_L = 0,
 *		 V4L2_EXT_VBE_PANEL_BL_EDGE_LED,
 *		 V4L2_EXT_VBE_PANEL_BL_OLED,
 *		 V4L2_EXT_VBE_PANEL_BL_DIRECT_VI,
 *		 V4L2_EXT_VBE_PANEL_BL_DIRECT_SKY,
 *		 V4L2_EXT_VBE_PANEL_BL_MAX
 *	 };
 *
 *	  //Panel Led_bar Type Enum	related with Tool OPT
 *	  enum v4l2_ext_vbe_panel_led_bar_type {
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_6 = 0,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_12,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_36,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_40,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_48,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_50,
 *		 V4L2_EXT_VBE_PANEL_LED_BAR_96,
 *	 	 V4L2_EXT_VBE_PANEL_LED_BAR_MAX
 *	  };
 *
 *	  //Panel Cell type
 *	  enum v4l2_ext_vbe_panel_cell_type {
 *		 V4L2_EXT_VBE_PANEL_CELL_TYPE_RGB = 0,
 *		 V4L2_EXT_VBE_PANEL_CELL_TYPE_RGBW,
 *		 V4L2_EXT_VBE_PANEL_CELL_TYPE_MAX
 *	  };
 *
 *	  //Display Output Lane Bandwidth.
 *	  enum v4l2_ext_vbe_panel_bandwidth {
 *		 V4L2_EXT_VBE_PANEL_BANDWIDTH_DEFAULT = 0,
 *		 V4L2_EXT_VBE_PANEL_BANDWIDTH_1_5G,
 *		 V4L2_EXT_VBE_PANEL_BANDWIDTH_2_1G,
 *		 V4L2_EXT_VBE_PANEL_BANDWIDTH_3_0G,
 *		 V4L2_EXT_VBE_PANEL_BANDWIDTH_MAX
 *	  };
 *
 *	  //FRC Chip type
 *	  enum v4l2_ext_vbe_frc_chip {
 *		 V4L2_EXT_VBE_FRC_CHIP_NONE = 0,
 *		 V4L2_EXT_VBE_FRC_CHIP_INTERNAL,
 *		 V4L2_EXT_VBE_FRC_CHIP_EXTERNAL1, // Reserved1
 *		 V4L2_EXT_VBE_FRC_CHIP_EXTERNAL2, // Reserved2
 *		 V4L2_EXT_VBE_FRC_CHIP_TPYE_MAX
 *	  };
 *
 *	  // LVDS bit depth.
 *	  enum v4l2_ext_vbe_lvds_colordepth {
 *		 V4L2_EXT_VBE_LVDS_COLOR_DEPTH_8BIT,
 *		 V4L2_EXT_VBE_LVDS_COLOR_DEPTH_10BIT,
 *		 V4L2_EXT_VBE_LVDS_COLOR_DEPTH_MAX
 *	  };
 *
 *	  // LVDS type.
 *	  enum v4l2_ext_vbe_lvds_type {
 *		 V4L2_EXT_VBE_LVDS_TYPE_VESA,
 *		 V4L2_EXT_VBE_LVDS_TYPE_JEIDA,
 *		 V4L2_EXT_VBE_LVDS_TYPE_MAX
 *	  };
 *
 *	  //Display User Option
 *	  union v4l2_ext_vbe_user_option {
 *		 unsigned int all;
 *		 struct _flags {
 *			 unsigned int SocOptionBIT0 : 1, SocOptionBIT1 : 1, SocOptionBIT2 : 1,
 *				 SocOptionBIT3 : 1, SocOptionBIT4 : 1, SocOptionBIT5 : 1,
 *				 SocOptionBIT6 : 1, SocOptionBIT7 : 1, SocOptionBIT8 : 1,
 *				 SocOptionBIT9 : 1, SocOptionBIT10 : 1, SocOptionBIT11 : 1,
 *				 SocOptionBIT12 : 1, SocOptionBIT13 : 1, SocOptionBIT14 : 1,
 *				 SocOptionBIT15 : 1, SocOptionBIT16 : 1, SocOptionBIT17 : 1,
 *				 SocOptionBIT18 : 1, SocOptionBIT19 : 1, SocOptionBIT20 : 1,
 *				 SocOptionBIT21 : 1, SocOptionBIT22 : 1, SocOptionBIT23 : 1,
 *				 SocOptionBIT24 : 1, SocOptionBIT25 : 1, SocOptionBIT26 : 1,
 *				 SocOptionBIT27 : 1, SocOptionBIT28 : 1, SocOptionBIT29 : 1,
 *				 SocOptionBIT30 : 1, SocOptionBIT31 : 1;
 *		 } flags;
 *	  };
 *
 *	  //Panel information
 *	  struct v4l2_ext_vbe_panel_info {
 *		 enum v4l2_ext_vbe_panel_inch panelInch;			   // Panel Inch
 *		 enum v4l2_ext_vbe_panel_maker panelMaker;			   // Panel maker
 *		 enum v4l2_ext_vbe_panel_interface panelInterface;	   // Panel Interface
 *		 enum v4l2_ext_vbe_panel_resolution panelResolution;   // Panel Resolution
 *		 enum v4l2_ext_vbe_panel_version panelVersion;		   // Panel Version
 *		 enum v4l2_ext_vbe_panel_framerate panelFrameRate;	   // Panel Frame Rate
 *		 enum v4l2_ext_vbe_panel_led_bar_type panelLedBarType; // LED bar type
 *		 enum v4l2_ext_vbe_panel_backlight_type
 *			 panelBacklightType;							// LED Backlight type
 *		 enum v4l2_ext_vbe_panel_cell_type panelCellType;	// Panel Cell type
 *		 enum v4l2_ext_vbe_panel_bandwidth dispOutLaneBW;	// Ouput lane bandwidth
 *		 enum v4l2_ext_vbe_frc_chip frcType;				// FRC type
 *		 enum v4l2_ext_vbe_lvds_colordepth lvdsColorDepth;	// LVDS Color depth
 *		 enum v4l2_ext_vbe_lvds_type lvdsType;				// LVDS Type
 *		 union v4l2_ext_vbe_user_option userSpecificOption; // Reserved Options
 *	  };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // VBE Initialize
 *	struct v4l2_ext_controls ext_controls;
 *	struct v4l2_ext_control ext_control;
 *	struct v4l2_ext_vbe_panel_info panelInfo;
 *
 *	memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *	memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *	memset(&panelInfo, 0, sizeof(struct v4l2_ext_vbe_panel_info ));
 *
 *	panelInfo.panelInch = V4L2_EXT_VBE_PANEL_INCH_65;                       // Panel Inch
 *	panelInfo.panelMaker = V4L2_EXT_VBE_PANEL_MAKER_LGD ;                  // Panel maker
 *	panelInfo.panelInterface = V4L2_EXT_VBE_PANEL_EPI;                      // Panel Interface
 *	panelInfo.panelResolution = V4L2_EXT_VBE_PANEL_RESOLUTION_3840X2160;    // Panel Resolution
 *	panelInfo.panelVersion = V4L2_EXT_VBE_PANEL_V18;                        // Panel Version
 *	panelInfo.panelFrameRate = V4L2_EXT_VBE_PANEL_FRAMERATE_120HZ;          // Panel Frame Rate
 *	panelInfo.panelLedBarType = V4L2_EXT_VBE_PANEL_LED_BAR_6 ;             // LED bar type
 *	panelInfo.panelBacklightType = V4L2_EXT_VBE_PANEL_BL_EDGE_LED;          // LED Backlight type
 *	panelInfo.panelCellType = V4L2_EXT_VBE_PANEL_CELL_TYPE_RGB;             // Panel Cell type
 *	panelInfo.dispOutLaneBW = V4L2_EXT_VBE_PANEL_BANDWIDTH_1_5G;        // Ouput lane bandwidth
 *	panelInfo.frcType = V4L2_EXT_VBE_FRC_CHIP_INTERNAL;                     // FRC type
 *	panelInfo.lvdsColorDepth = V4L2_EXT_VBE_LVDS_COLOR_DEPTH_10BIT;             // LVDS Color depth
 *	panelInfo.lvdsType = V4L2_EXT_VBE_LVDS_TYPE_VESA;                       // LVDS Type
 *	panelInfo.userSpecificOption.flags.userSpecificOption = 0;      // Reserved Option
 *
 *	ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *	ext_controls.count = 1;
 *	ext_controls.controls = &ext_control;
 *	ext_controls.controls->id = V4L2_CID_EXT_VBE_INIT;
 *	ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_info);
 *	ext_controls.controls->ptr = (void *)&panelInfo;
 *
 *	ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_INIT (V4L2_CID_EXT_VBE_BASE + 0)
/**
 * @brief Init VBE when DC on(QSM+ on)
 *
 * @rst
 * This function can set resume VBE
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_RESUME
 *
 *	   //
 *	   // parameter
 *	   //
 *	   Sams as V4L2_CID_EXT_VBE_INIT
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // VBE Resume
 *	struct v4l2_ext_controls ext_controls;
 *	struct v4l2_ext_control ext_control;
 *	struct v4l2_ext_vbe_panel_info panelInfo;
 *
 *	memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *	memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *	memset(&panelInfo, 0, sizeof(struct v4l2_ext_vbe_panel_info ));
 *
 *	panelInfo.panelInch = V4L2_EXT_VBE_PANEL_INCH_65;                       // Panel Inch
 *	panelInfo.panelMaker = V4L2_EXT_VBE_PANEL_MAKER_LGD ;                  // Panel maker
 *	panelInfo.panelInterface = V4L2_EXT_VBE_PANEL_EPI;                      // Panel Interface
 *	panelInfo.panelResolution = V4L2_EXT_VBE_PANEL_RESOLUTION_3840X2160;    // Panel Resolution
 *	panelInfo.panelVersion = V4L2_EXT_VBE_PANEL_V18;                        // Panel Version
 *	panelInfo.panelFrameRate = V4L2_EXT_VBE_PANEL_FRAMERATE_120HZ;          // Panel Frame Rate
 *	panelInfo.panelLedBarType = V4L2_EXT_VBE_PANEL_LED_BAR_6 ;             // LED bar type
 *	panelInfo.panelBacklightType = V4L2_EXT_VBE_PANEL_BL_EDGE_LED;          // LED Backlight type
 *	panelInfo.panelCellType = V4L2_EXT_VBE_PANEL_CELL_TYPE_RGB;             // Panel Cell type
 *	panelInfo.dispOutLaneBW = V4L2_EXT_VBE_PANEL_BANDWIDTH_1_5G;        // Ouput lane bandwidth
 *	panelInfo.frcType = V4L2_EXT_VBE_FRC_CHIP_INTERNAL;                     // FRC type
 *	panelInfo.lvdsColorDepth = V4L2_EXT_VBE_LVDS_COLOR_DEPTH_10BIT;             // LVDS Color depth
 *	panelInfo.lvdsType = V4L2_EXT_VBE_LVDS_TYPE_VESA;                       // LVDS Type
 *	panelInfo.userSpecificOption.flags.userSpecificOption = 0;      // Reserved Option
 *
 *	ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *	ext_controls.count = 1;
 *	ext_controls.controls = &ext_control;
 *	ext_controls.controls->id = V4L2_CID_EXT_VBE_RESUME;
 *	ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_info);
 *	ext_controls.controls->ptr = (void *)&panelInfo;
 *
 *	ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_RESUME (V4L2_CID_EXT_VBE_BASE + 1)
/**
 * @brief Set display output
 *
 * @rst
 * This function can set soc display output
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_DISPLAYOUTPUT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set displayOutput
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_DISPLAYOUTPUT;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_DISPLAYOUTPUT (V4L2_CID_EXT_VBE_BASE + 2)
/**
 * @brief Set screen mute
 *
 * @rst
 * This function can set screen mute
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MUTE
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set screen mute
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_MUTE;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MUTE (V4L2_CID_EXT_VBE_BASE + 3)
/**
 * @brief Set spread spectrum
 *
 * @rst
 * This function can set spread spectrum
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_SSC
 *
 *	   //
 *	   // parameter
 *	   //
 *       //spread spectrum control
 *        struct v4l2_ext_vbe_ssc {
 *       	 unsigned int on_off;
 *       	 unsigned short percent;
 *       	 unsigned short period;
 *        };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set spread spectrum
 *	struct v4l2_ext_controls ext_controls;
 *	struct v4l2_ext_control ext_control;
 *	struct v4l2_ext_vbe_ssc sscInfo;
 *
 *	memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *	memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *	memset(&sscInfo, 0, sizeof(struct v4l2_ext_vbe_ssc ));
 *
 *	sscInfo.on_off = 1;                     // 1:on / 0:offPanel Inch
 *	sscInfo.percent = 5 ;                  // Step is 0.1%, If value is 5, it means 0.5%
 *	sscInfo.period = 250;                 // Step is 0.1KHz, If value is 250, it means 25.0%
 *
 *	ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *	ext_controls.count = 1;
 *	ext_controls.controls = &ext_control;
 *	ext_controls.controls->id = V4L2_CID_EXT_VBE_SSC;
 *	ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_ssc);
 *	ext_controls.controls->ptr = (void *)&sscInfo;
 *
 *	ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_SSC (V4L2_CID_EXT_VBE_BASE + 4)
/**
 * @brief Set mirror mode
 *
 * @rst
 * This function can set video mirror mode
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MIRROR
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set video mirror mode
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_MIRROR;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MIRROR (V4L2_CID_EXT_VBE_BASE + 5)
/**
 * @brief Set SetRGBWBypass
 *
 * @rst
 * This function is used for only RGBW model.
 *  The BOE M+ IP in SoC is bypassed for BOE panel.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MPLUS_BOEBYPASS
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // BOE Bypass
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_MPLUS_BOEBYPASS;
 *	 control.value = 1; 	 // Bypass On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MPLUS_BOEBYPASS (V4L2_CID_EXT_VBE_BASE + 6)
/**
 * @brief Set MLE Mode
 *
 * @rst
 * This function is used for only RGBW model.
 * The LGD M+ IP will be contolled by each mode.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MPLUS_MODE
 *
 *	   //
 *	   // parameter
 *	   //
 *      enum v4l2_ext_vbe_mplus_mode {
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE0 = 0, // V4L2_EXT_VBE_MPLUS_HIGH_LUM1_MSE_ON = 0,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE1,  // V4L2_EXT_VBE_MPLUS_HIGH_LUM2,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE2,  // V4L2_EXT_VBE_MPLUS_LOW_POWER1,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE3,  // V4L2_EXT_VBE_MPLUS_MLE_MODE_OFF,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE4,  // V4L2_EXT_VBE_MPLUS_HIGH_LUM1_MSE_OFF,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE5,  // V4L2_EXT_VBE_MPLUS_LOW_POWER2,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE6,  // V4L2_EXT_VBE_MPLUS_LOW_POWER2_SC_OFF,
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE7,  // MHE Mode1 09/20/2018
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE8,  // MHE Mode2 09/20/2018
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE9,  // MHE Mode3 09/20/2018
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODE10, // MHE Mode4 09/20/2018
 *     	 V4L2_EXT_VBE_MPLUS_MPLUS_MODEMAX
 *      };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // LGD M+ Mode
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     enum v4l2_ext_vbe_mplus_mode mplusMode;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&mplusMode, 0, sizeof(enum v4l2_ext_vbe_mplus_mode));
 *
 *     mplusMode = V4L2_EXT_VBE_MPLUS_MPLUS_MODE1;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_MPLUS_MODE;
 *     ext_controls.controls->size = sizeof(enum v4l2_ext_vbe_mplus_mode);
 *     ext_controls.controls->ptr = (void *)&mplusMode;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MPLUS_MODE (V4L2_CID_EXT_VBE_BASE + 7)
/**
 * @brief Set DGA4CH
 *
 * @rst
 * This function is used for RGB model and RGBW model.
 * Digital Gamma(DGA) 4CH(R,G,B,W) setting in TCON Block.
 * The data counts are 1024 each channel. The data resolution is 10bit.
 * If RGB model, The all data of pWhiteGammaTable is discarded.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_DGA4CH
 *
 *	   //
 *	   // parameter
 *	   //
 *       struct v4l2_ext_vbe_dga4ch {
 *       	union {
 *      		 unsigned int *pRedGammaTable;
 *      		 unsigned int compat_data_red;
 *      		 unsigned long long sizer_red;
 *      	 };
 *      	 union {
 *      		 unsigned int *pGreenGammaTable;
 *      		 unsigned int compat_data_green;
 *      		 unsigned long long sizer_green;
 *      	 };
 *     	  union {
 *      		 unsigned int *pBlueGammaTable;
 *      		 unsigned int compat_data_blue;
 *      		 unsigned long long sizer_blue;
 *     	  };
 *    	   union {
 *      		 unsigned int *pWhiteGammaTable;
 *      		 unsigned int compat_data_white;
 *     			 unsigned long long sizer_white;
 *     	  };
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set DGA4CH
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_DGA4CH (V4L2_CID_EXT_VBE_BASE + 8)
/**
 * @brief setFrameGainLimit/ VBE GetFrameGainLimit/ VBE SetPixelGainLimit/ VBE GetPixelGainLimit
 *
 * @rst
 * This function is used for only RGBW model.
 * It control the LGD M+ parameters.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
  *	   VIDIOC_G_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MPLUS_PARAM
 *
 *	   //
 *	   // parameter
 *	   //
 *       struct v4l2_ext_vbe_mplus_param {
 *      	 unsigned short nFrameGainLimit;
 *      	 unsigned short nPixelGainLimit;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Mplus Parameter
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_mplus_param mplusParam;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&mplusParam, 0, sizeof(struct v4l2_ext_vbe_mplus_param));
 *
 *     mplusParam.nFrameGainLimit = 0x2000;
 *     mplusParam.nPixelGainLimit= 0x0800;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_MPLUS_PARAM;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_mplus_param);
 *     ext_controls.controls->ptr = (void *)&mplusParam;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MPLUS_PARAM (V4L2_CID_EXT_VBE_BASE + 9)
/**
 * @brief Set Inner Pattern
 *
 * @rst
 * This function is used to set inner pattern
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_INNER_PATTERN
 *
 *	   //
 *	   // parameter
 *	   //
 *       struct v4l2_ext_vbe_inner_pattern {
 *       	 unsigned char bOnOff;
 *       	 unsigned char ip;
 *       	 unsigned char type;
 *        };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set Inner Pattern
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_inner_pattern innerPattern;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&mplusParam, 0, sizeof(struct v4l2_ext_vbe_inner_pattern));
 *
 *     innerPattern.bOnOff= 1; // 1:On, 0:Off
 *     innerPattern.ip = 3; //0:CE, 1:CO, 2:DPE, 3:TCON
 *     innerPattern.type= 8; //0:0ff,1:Black,2:White, 3:HV Bar, 4:H 16Gray, 5:H 64Gray, 6:H 256Gray, 7:H1024 Gray, 8:V 16 Gray
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_INNER_PATTERN;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_inner_pattern);
 *     ext_controls.controls->ptr = (void *)&innerPattern;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_INNER_PATTERN (V4L2_CID_EXT_VBE_BASE + 10)
/**
 * @brief Set TSCIC Load
 *
 * @rst
 * This function is for setting TSCIC function of SOC TCON blocks
 * It will load an TSCIC data to SoC TCON Block.
 * TSCIC : Time Sharing Clear Image Creation
 * Requirement 1. Do not skip SoCTS
 * Requirement 2. Do not clock disable when error return
 * Requirement 3. Do not use kmalloc
 * Requirement 4. Keep data setting sequence( ex. first data set, second enable funciton)
 * Requirement 5. It can contorl disable when set u32Ctrlsize=0 or u32Tscicsize =0
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_TSCIC
 *
 *	   //
 *	   // parameter
 *	   //
 *       // TSCIC data structure
 *       struct v4l2_ext_vbe_panel_tscic {
 *      	 union {
 *      		 unsigned char *u8pControlTbl;
 *      		 unsigned int compat_data_ctrl;
 *      		 unsigned long long sizer_ctrl;
 *      	 };
 *      	 unsigned int u32Ctrlsize;
 *      	 union {
 *      		 unsigned int *u32pTSCICTbl;
 *      		 unsigned int compat_data_tscic;
 *      		 unsigned long long sizer_tscic;
 *      	 };
 *      	 unsigned int u32Tscicsize;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // TSCIC Load
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_tscic tscicData;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&tscicData, 0, sizeof(struct v4l2_ext_vbe_panel_tscic));
 *
 *     tscicData.u8pControlTbl = tscicctrldata
 *     tscicData.u32Ctrlsize =  TSCIC_CTRL_SIZE // 43 , if size is 0 then tscic disalbe
 *     tscicData.u32pTSCICTbl=  tscicdata
 *     tscicData.u32Tscicsize=  TSCIC_DATA_SIZE // 0x170C3, if size is 0 then tscic disable
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_TSCIC;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_tscic);
 *     ext_controls.controls->ptr = (void *)&tscicData;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_TSCIC (V4L2_CID_EXT_VBE_BASE + 11)
/**
 * @brief Set EPI Data scramble on/off
 *
 * @rst
 * This function can set EPI Data scramble on/off
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_EPI_SCRAMBLE
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set EPI Data scramble
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_EPI_SCRAMBLE;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_EPI_SCRAMBLE (V4L2_CID_EXT_VBE_BASE + 12)
/**
 * @brief Set EPI Advanced 10 bit on/off
 *
 * @rst
 * This function can set EPI Advanced 10 bit on/off
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_EPI_10BIT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set EPI Advanced 10 bit on/off
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_EPI_10BIT;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_EPI_10BIT (V4L2_CID_EXT_VBE_BASE + 13)
/**
 * @brief Set/Get MplusData
 *
 * @rst
 * This function is for LGD and BOE M+ register set function.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_MPLUS_DATA
 *
 *	   //
 *	   // parameter
 *	   //
 *       // Mplus data
 *       struct v4l2_ext_vbe_mplus_data {
 *      	 union {
 *      		 void *pRegisterSet;
 *      		 unsigned int compat_data;
 *      		 unsigned long long sizer;
 *      	 };
 *      	 unsigned char nPanelMaker;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // MPLUS Data
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_mplus_data mplusData;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&mplusData, 0, sizeof(struct v4l2_ext_vbe_mplus_data));
 *
 *     mplusData.pRegisterSet = gMplus_65UM63_LGD_RGBW
 *     mplusData.nPanelMaker= MODULE_LGD
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_MPLUS_DATA;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_mplus_data);
 *     ext_controls.controls->ptr = (void *)&mplusData;
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_MPLUS_DATA (V4L2_CID_EXT_VBE_BASE + 14)
/**
 * @brief Apply PWM params
 *
 * @rst
 * This function can Apply PWM params
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_PWM_APPLY_PARAM
 *
 *	   //
 *	   // parameter
 *       //
 *       // type definition about PWM device's index mask
 *       enum v4l2_ext_vbe_pwm_pin_sel_mask {
 *       V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN0, // < PWM# 0
 *       V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN1, //< PWM# 1
 *       V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN2, // PWM# 2
 *       V4L2_EXT_VBE_PWM_DEV_PIN_3_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN3, // PWM# 3
 *       V4L2_EXT_VBE_PWM_DEV_PIN_4_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN4, // PWM# 4
 *       V4L2_EXT_VBE_PWM_DEV_PIN_0_1_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK, // PWM# 0/1
 *       V4L2_EXT_VBE_PWM_DEV_PIN_0_2_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK, // PWM# 0/2
 *       V4L2_EXT_VBE_PWM_DEV_PIN_1_2_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK, // PWM# 1/2
 *       // If necessary, add case
 *       V4L2_EXT_VBE_PWM_DEV_PIN_ALL_MASK =
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK |
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_3_MASK |
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_4_MASK, // PWM# 0/1/2/3/4
 *       V4L2_EXT_VBE_PWM_DEV_PIN_DEFAULT_MASK // depended on pwmIndex setting of HAL_PWM_SetParam function
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set Apply PWM Param
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     enum v4l2_ext_vbe_pwm_pin_sel_mask pwmApplyParm;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pwmApplyParm, 0, sizeof(enum v4l2_ext_vbe_pwm_pin_sel_mask));
 *
 *     pwmApplyParm = V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_PWM_APPLY_PARAM;
 *     ext_controls.controls->size = sizeof(enum v4l2_ext_vbe_pwm_pin_sel_mask);
 *     ext_controls.controls->ptr = (void *)&pwmApplyParm;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_PWM_APPLY_PARAM (V4L2_CID_EXT_VBE_BASE + 15)
/**
 * @brief Set PWM param
 *
 * @rst
 * This function can set PWM param
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *       VIDIOC_G_EXT_CTRLS
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_PWM_PARAM
 *
 *	   //
 *	   // parameter
 *	   //
 *	   //type definition about PWM device's index
 *	   enum v4l2_ext_vbe_pwm_pin_sel {
 *		 V4L2_EXT_VBE_PWM_DEV_PIN0 = 0,    // < PWM# 0
 *		 V4L2_EXT_VBE_PWM_DEV_PIN1, 	   //< PWM# 1
 *		 V4L2_EXT_VBE_PWM_DEV_PIN2, 	   //< PWM# 2
 *		 V4L2_EXT_VBE_PWM_DEV_PIN3, 	   //< PWM# 3
 *		 V4L2_EXT_VBE_PWM_DEV_PIN4, 	   //< PWM# 4
 *		 V4L2_EXT_VBE_PWM_DEV_MAX,	   //< PWM# MAX
 *		 V4L2_EXT_VBE_PWM_DEV_NONE = 0xFF, //< PWM# Not used
 *		};
 *
 *		//PWM Frequency Parameter
 *		struct v4l2_ext_vbe_pwm_adapt_freq_param {
 *		 unsigned int pwm_adapt_freq_enable;
 *		 unsigned int pwmfreq_48nHz; // PWM frequency 48xN Hz from DB table
 *		 unsigned int pwmfreq_50nHz; // PWM frequency 50xN Hz from DB table
 *		 unsigned int pwmfreq_60nHz; // PWM frequency 60xN Hz from DB table
 *		 };
 *
 *		//PWM Setting Parameter
 *		struct v4l2_ext_vbe_pwm_param_data {
 *		 unsigned int pwm_enable;
 *		 unsigned int pwm_duty;
 *		 unsigned int pwm_frequency; // If pwm_adapt_freq_enable == TRUE, ignored
 *		 struct v4l2_ext_vbe_pwm_adapt_freq_param pwm_adapt_freq_param;
 *		 unsigned int pwm_lock;
 *		 unsigned int pwm_pos_start;
 *		 unsigned int pwm_scanning_enable;
 *		 unsigned int pwm_low_power_enable; // It has been used to set low power mode
 *											// for M16P only until now, 170210.
 *		};
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set PWM Param
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_pwm_param pwmParam;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&pwmParam, 0, sizeof(struct v4l2_ext_vbe_pwm_param));
 *
 *     pwmParam.pwmIndex = pwmIndex; //v4l2_ext_vbe_pwm_pin_sel pwmIndex is input parameter
 *     pwmParam.pstPWMParam = pstPWMParam; //v4l2_ext_vbe_pwm_param pstPWMParam is input parameter
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_PWM_PARAM;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_pwm_param);
 *     ext_controls.controls->string = (void *)&pwmParam;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get PWM Param
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_pwm_param pwmParam;
 *     struct v4l2_ext_vbe_pwm_param_data stPWMParam;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&pwmParam, 0, sizeof(struct v4l2_ext_vbe_pwm_param));
 *
 *     pwmParam.pwmIndex    = pwmIndex;
 *     pwmParam.pstPWMParam = &stPWMParam;
 *
 *     ext_controls.ctrl_class       = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count            = 1;
 *     ext_controls.controls         = &ext_control;
 *     ext_controls.controls->id     = V4L2_CID_EXT_VBE_PWM_PARAM;
 *     ext_controls.controls->size   = sizeof(struct v4l2_ext_vbe_pwm_param);
 *     ext_controls.controls->string = (void *)&pwmParam;
 *
 *     ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 * @endrst
 */
#define V4L2_CID_EXT_VBE_PWM_PARAM (V4L2_CID_EXT_VBE_BASE + 16)
/**
 * @brief Init PWM
 *
 * @rst
 * This function can Init PWM
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_PWM_INIT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   void
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Init PWM
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_PWM_INIT;
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_PWM_INIT (V4L2_CID_EXT_VBE_BASE + 17)
/**
 * @brief Set VCOM Pattern draw
 *
 * @rst
 * This function used to drawing VCOM pattern. It will be used for adjustment in the factory.
 * It will be used only one time when drawing pattern, not turn on the display. It will be displayed by HAL_VBE_DISP_VCOMPatternCtrl(VCOM_PAT_CTRL_ON);
 * The level of each pixel will not different, ex) an pattern have 127 gray for all of pixels.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_VCOM_PAT_DRAW
 *
 *	   //
 *	   // parameter
 *	   //
 *       // draw vcom pattern
 *       struct v4l2_ext_vbe_vcom_pat_draw {
 *      	 union {
 *      		 unsigned short *vcomPattern;
 *      		 unsigned int compat_data;
 *      		 unsigned long long sizer;
 *      	 };
 *      	 unsigned short nSize;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Draw VCOM Pattern
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_vcom_pat_draw vcompat;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&vcompat, 0, sizeof(struct v4l2_ext_vbe_vcom_pat_draw));
 *
 *     vcompat.vcomPattern = vcomPattern;
 *     vcompat.nSize = 96;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_VCOM_PAT_DRAW;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_vcom_pat_draw);
 *     ext_controls.controls->ptr = (void *)&vcompat;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_VCOM_PAT_DRAW (V4L2_CID_EXT_VBE_BASE + 18)
/**
 * @brief Set VCOM Pattern ctrl
 *
 * @rst
 * This function turn on/off vcom pattern.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_VCOM_PAT_CTRL
 *
 *	   //
 *	   // parameter
 *	   //
 *       // VCOM Pattern Control
 *       enum v4l2_ext_vbe_vcom_pat_ctrl {
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_OFF = 0, // Mandatory Implementation
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_ON, 	 // Mandatory Implementation
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM1, // Optional Implementation, PGEN_VCOM1 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM2, // Optional Implementation, PGEN_VCOM2 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM3, // Optional Implementation, PGEN_VCOM3 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM4, // Optional Implementation, PGEN_VCOM4 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM5, // Optional Implementation, PGEN_VCOM5 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM6, // Optional Implementation, PGEN_VCOM6 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM7, // Optional Implementation, PGEN_VCOM7 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_VCOM8, // Optional Implementation, PGEN_VCOM8 and
 *      									   // CTRL_ON, for available SoC
 *      	 V4L2_EXT_VBE_VCOM_PAT_CTRL_MAX    // Limit to control.
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Draw VCOM Pattern
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_vcom_pat_draw vcompat;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&vcompat, 0, sizeof(struct v4l2_ext_vbe_vcom_pat_draw));
 *
 *     vcompat.vcomPattern = vcomPattern;
 *     vcompat.nSize = 96;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_VCOM_PAT_DRAW;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_vcom_pat_draw);
 *     ext_controls.controls->ptr = (void *)&vcompat;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_VCOM_PAT_CTRL (V4L2_CID_EXT_VBE_BASE + 19)
/**
 * @brief Set PWM Duty
 *
 * @rst
 * This function can set  PWM Duty
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_PWM_SET_DUTY
 *
 *	   //
 *	   // parameter
 *       //
 *       // type definition about PWM device's index mask
 *       enum v4l2_ext_vbe_pwm_pin_sel_mask {
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN0, // < PWM# 0
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN1, //< PWM# 1
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN2, // PWM# 2
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_3_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN3, // PWM# 3
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_4_MASK = 1 << V4L2_EXT_VBE_PWM_DEV_PIN4, // PWM# 4
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_0_1_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK, // PWM# 0/1
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_0_2_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK, // PWM# 0/2
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_1_2_MASK =  V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK, // PWM# 1/2
 *      	 // If necessary, add case
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_ALL_MASK =
 *      		 V4L2_EXT_VBE_PWM_DEV_PIN_0_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK |
 *      		 V4L2_EXT_VBE_PWM_DEV_PIN_2_MASK | V4L2_EXT_VBE_PWM_DEV_PIN_3_MASK |
 *      		 V4L2_EXT_VBE_PWM_DEV_PIN_4_MASK, // PWM# 0/1/2/3/4
 *      	 V4L2_EXT_VBE_PWM_DEV_PIN_DEFAULT_MASK // depended on pwmIndex setting of HAL_PWM_SetParam function
 *       };
 *
 *       // pwm parameter to set pwm duty only
 *       struct v4l2_ext_vbe_pwm_duty {
 *      	 enum v4l2_ext_vbe_pwm_pin_sel_mask pwmIndex;
 *      	 unsigned int pwm_duty;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set PWM duty
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_pwm_duty pwmDutyParam;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pwmDutyParam, 0, sizeof(struct v4l2_ext_vbe_pwm_duty ));
 *
 *     stPWMdutyParam.pwmIndex = V4L2_EXT_VBE_PWM_DEV_PIN_1_MASK;
 *     stPWMdutyParam.pwm_duty = 200;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_PWM_SET_DUTY;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_pwm_duty );
 *     ext_controls.controls->ptr = (void *)&pwmDutyParam;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_PWM_SET_DUTY (V4L2_CID_EXT_VBE_BASE + 20)
/**
 * @brief Set Orbit
 *
 * @rst
 * This function can set Orbit
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_ORBIT
 *
 *	   //
 *	   // parameter
 *       //
 *       // ORBIT mode setting
 *       enum v4l2_ext_vbe_panel_orbit_mode {
 *      	 V4L2_EXT_VBE_PANEL_ORBIT_JUSTSCAN_MODE = 0,
 *      	 V4L2_EXT_VBE_PANEL_ORBIT_AUTO_MODE,
 *      	 V4L2_EXT_VBE_PANEL_ORBIT_STORE_MODE,
 *      	 V4L2_EXT_VBE_PANEL_OREBIT_MODE_MAX
 *       };
 *
 *       // ORBIT mode ctrl
 *       struct v4l2_ext_vbe_panel_orbit_info {
 *      	 unsigned int on_off;
 *      	 enum v4l2_ext_vbe_panel_orbit_mode orbitmode;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set Orbit
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_orbit_info orbitInfo;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&orbitInfo, 0, sizeof(struct v4l2_ext_vbe_panel_orbit_info ));
 *
 *     orbitInfo.on_off = (bOnOff == TRUE) ? 1 : 0;
 *     orbitInfo.orbitmode = V4L2_EXT_VBE_PANEL_ORBIT_AUTO_MODE;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_ORBIT;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_orbit_info);
 *     ext_controls.controls->string = (void *)&orbitInfo;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_ORBIT (V4L2_CID_EXT_VBE_BASE + 21)
/**
 * @brief Set LSR
 *
 * @rst
 * This function is LSR ctrl for sticky reduction(OLED only)
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_LSR
 *
 *	   //
 *	   // parameter
 *       //
 *       // LSR mode setting
 *       enum v4l2_ext_vbe_panel_lsr_step{
 *       V4L2_EXT_VBE_PANEL_LSR_OFF = 0,
 *       V4L2_EXT_VBE_PANEL_LSR_LIGHT,			// light_vivid
 *       V4L2_EXT_VBE_PANEL_LSR_STRONG, 		// strong_vivid
 *       V4L2_EXT_VBE_PANEL_LSR_STRONG_OTHERS,
 *       V4L2_EXT_VBE_PANEL_LSR_LIGHT_OTHERS,
 *       V4L2_EXT_VBE_PANEL_LSR_MAX
 *       };
 *
 *       // LSR info
 *       struct v4l2_ext_vbe_panel_lsr_info{
 *       unsigned int *pLsrTable;
 *       enum v4l2_ext_vbe_panel_lsr_mode lsrstep;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set LSR
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_lsr_info lsrinfo;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&lsrinfo, 0, sizeof(struct v4l2_ext_vbe_panel_lsr_info));
 *
 *     lsrinfo.pLsrTable = LSRTbl;
 *     lsrinfo.lsrstep = nStep; // UI val or Idx ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_LSR;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_lsr_info);
 *     ext_controls.controls->string = (void *)&lsrinfo;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_LSR (V4L2_CID_EXT_VBE_BASE + 22)
/**
 * @brief Set CPC
 *
 * @rst
 * This function is for CPC hw ip control by LG SIC drv support only(OLED only)
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_CPC
 *
 *	   //
 *	   // parameter
 *	   //
 *	   guint8 u8data
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set video mirror mode
 *     // Set CPC
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VBE_CPC;
 *     control.value = u8data;	  // On
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_CPC (V4L2_CID_EXT_VBE_BASE + 23)
/**
 * @brief Set GSR
 *
 * @rst
 * This function is ctrl of global sticky reduction for video area(OLED only).
 * In case of LGE chip, the GSR operation is performed in the internal BSP FW.
 * OS chip(other chip vendor), the GSR is handled using the kernel module "libosr" library
 * as shown in the figure below.
 * In the library, the local apl (ex, 60*34) value and Bypass RGBgain value at the chip level
 * are input parameters, and the final GSRgain output value is applied to the screen
 * through the formula in the library.
 *
 * Implement for OS chip
 *
 * .. image:: /v4l2/vbe-V4L2_CID_EXT_VBE_GSR.png
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_GSR
 *
 *	   //
 *	   // parameter
 *       //
 *       // GSR setting tbl
 *       struct v4l2_ext_vbe_panel_gsr_info{
 *           unsigned int *pGsrTable;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set GSR
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_gsr_info gsrinfo;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&gsrinfo, 0, sizeof(struct v4l2_ext_vbe_panel_gsr_info));
 *
 *     gsrinfo.pGsrTable = TblIdx;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_GSR;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_gsr_info);
 *     ext_controls.controls->string = (void *)&gsrinfo;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls)
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_GSR (V4L2_CID_EXT_VBE_BASE + 24)
/**
 * @brief Set OSDRGBlvl
 *
 * @rst
 * This function is for OSD RGB gain ctrl (OLED only)
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_OSD_GAIN
 *
 *	   //
 *	   // parameter
 *       //
 *       // OSD_RGB GAIN setting ctrl
 *       struct v4l2_ext_vbe_panel_osd_gain_info{
 *       unsigned int on_off;
 *       unsigned int *levelval;
 *       unsigned int size; 		  // 4byte size fix for SIC
 *       };
 *
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set OSD RGB gain
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_osd_gain_info osdgaininfo;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&osdgaininfo, 0, sizeof(struct v4l2_ext_vbe_panel_osd_gain_info));
 *
 *     osdgaininfo.on_off = (bOnOff == TRUE) ? 1 : 0;
 *     osdgaininfo.levelval = &u32RGBlevel; //check the val_ originally tbl val
 *     osdgaininfo.size = 4; //4byte fix by chip_vendor data_size
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_OSD_GAIN;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_osd_gain_info);
 *     ext_controls.controls->string = (void *)&osdgaininfo;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_OSD_GAIN (V4L2_CID_EXT_VBE_BASE + 25)
/**
 * @brief getOSDAlphaAPL
 *
 * @rst
 * This function is which get OSD layer alpha info from chip drv.
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_G_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_ALPHA_OSD
 *
 *	   //
 *	   // parameter
 *       //
 *       //  OSD_Alpha val ctrl
 *       struct v4l2_ext_vbe_panel_alpha_osd_info{
 *       unsigned int *alphaTable;
 *       unsigned int size; 	 // 64byte size fix for SIC
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Get OSD Alpha Val
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_ALPHA_OSD (V4L2_CID_EXT_VBE_BASE + 26)
/**
 * @brief Get GSR Status
 *
 * @rst
 * This function is which get GSR func ctrl running status from BSP by LG SIC drv support only(OLED only)
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_G_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_GSR_STATUS
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (disable), 1 (enable)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	// GSR Status
 * v4l2_control control;
 * control.id = V4L2_CID_EXT_VBE_GSR_STATUS;
 *
 * ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_GSR_STATUS (V4L2_CID_EXT_VBE_BASE + 27)
/**
 * @brief Get Lock Status
 *
 * @rst
 * This function is used to get video siganl lock status
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_G_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_LOCK_STATUS
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Unlock), 1 (Lock)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Get video signal lock status
 *     struct v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VBE_LOCK_STATUS;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_LOCK_STATUS (V4L2_CID_EXT_VBE_BASE + 28)


/**
 * @brief Load Demura data
 *
 * @rst
 * It purpose to load the demura data.
 * It was implemented for SiliconWorks TCON IP
 * 8bit config and data
 * Requirement 1. Do not skip SoCTS
 * Requirement 2. Do not clock disable when error return
 * Requirement 3. Do not use kmalloc
 * Requirement 4. Keep data setting sequence( ex. first data set, second enable funciton)
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *	   VIDIOC_G_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VBE_DEMURA
 *
 *	   //
 *	   // parameter
 *	   //
 *     // Demura data structure
 *       struct v4l2_ext_vbe_panel_demura {
 *      	 union {
 *      		 unsigned char *config;
 *      		 unsigned int compat_data_ctrl;
 *      		 unsigned long long sizer_ctrl;
 *      	 };
 *      	 unsigned int configsize;
 *      	 unsigned int configcrc;
 *      	 union {
 *      		 unsigned char *data;
 *      		 unsigned int compat_data_demura;
 *      		 unsigned long long sizer_demura;
 *      	 };
 *      	 unsigned int datasize;
 *      	 unsigned int datacrc;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *	 Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Demura Load
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_demura demuraData;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&demuraData, 0, sizeof(struct v4l2_ext_vbe_panel_demura));
 *
 *     demuraData.enable = 1;
 *     demuraData.config = demuraCfgData;
 *     demuraData.configsize =  DEMURA_CFG_SIZE; // 22
 *     demuraData.configcrc = demuraCfgCrc;      //CRC16 2byte
 *
 *     demuraData.data = demuraData;
 *     demuraData.datasize =  DEMURA_DATA_SIZE; // 0x05EEC4
 *     demuraData.datacrc = demuraDataCrc;      //CRC16 2byte
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_DEMURA;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_demura);
 *     ext_controls.controls->ptr = (void *)&demuraData;
 *
 *     //Data Load to registers.
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     //Data Read form registers.
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_DEMURA (V4L2_CID_EXT_VBE_BASE + 29)


/**
 * @brief Load PCLRC Data
 *
 * @rst
 * It purpose to load the pclrc data.
 * It was implemented for SiliconWorks TCON IP
 * 8bit data
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *	   VIDIOC_G_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VBE_PCLRC
 *
 *	   //
 *	   // parameter
 *	   //
 *     // pclrc data structure
 *       struct v4l2_ext_vbe_panel_pclrc {
 *      	 unsigned int enable; // 0: disable, 1: enable
 *      	 unsigned int positionAfterLineOD; // 0: before LineOD, 1: after LineOD
 *      	 union {
 *      		 unsigned char *data;
 *      		 unsigned int compat_data_pclrc;
 *      		 unsigned long long sizer_pclrc;
 *      	 };
 *      	 unsigned int datasize;  //4352
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *	 Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Pclrc Load
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_pclrc pclrcData;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&pclrcData, 0, sizeof(struct v4l2_ext_vbe_panel_pclrc));
 *
 *     pclrcData.positionAfterLineOD = 1; // Signal Path 0 : PCLRC -> LOD, 1 : LOD -> PCLRC
 *     pclrcData.data = pclrcData;  //It should write to 8192 to 12543 in SRAM LUT.
 *     pclrcData.datasize =  PCLRC_DATA_SIZE; // 4352
 *     pclrcData.enable = 1;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_PCLRC;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_pclrc);
 *     ext_controls.controls->ptr = (void *)&pclrcData;
 *
 *     //Data Load to registers.
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     //Data Read form registers.
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_PCLRC (V4L2_CID_EXT_VBE_BASE + 30)

/**
 * @brief Set displayoutput 5060Hz
 *
 * @rst
 * This function can set displayoutput 5060Hz
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_DISPLAYOUTPUT_5060HZ
 *
 *	   //
 *	   // parameter
 *	   //
 *	   0 (Off), 1 (On)
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Set displayoutput 5060Hz
 *	 v4l2_control control;
 *
 *	 control.id = V4L2_CID_EXT_VBE_DISPLAYOUTPUT_5060HZ;
 *	 control.value = 1; 	 // On
 *
 *	 ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_DISPLAYOUTPUT_5060HZ (V4L2_CID_EXT_VBE_BASE + 31)

//
// GSR_STRESS_COUNT control ID
//
/**
 * @brief Get GSR_STRESS_COUNT
 *
 * @rst
 * This function is when there are more than a few stop areas usng the GSR detect block
 * (threshold ex: 100) after Power ON,
 * the time is counted accumulated for 10 minutes (spec), and the counting value is delivered.
 * Operability of the FW (BSP) : Normalized for frame rate variation based on output frame rate
 * domain.
 * (ex,24/25/30hz based on input considering all input timing, In case of 120hz input,
 * when the worst img input 120frame comes, count 1 is accumulated.)
 * Accumulated val must be maintained until suspend or cold pwr state.
 * In case of OS chip, LG_GSR_StressCount symbol value in Libosr is transferred through the
 * v4l2_GetGSRStressCount function.
 *
 * USE case
 *
 * .. image:: /v4l2/vbe-V4L2_CID_EXT_VBE_GSR_STRESS_COUNT.png
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_G_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_GSR_STRESS_COUNT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   unsigned int;
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // GSR_STRESS_COUNT
 *   v4l2_control control;
 *   control.id = V4L2_CID_EXT_VBE_GSR_STRESS_COUNT;
 *
 *   ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_GSR_STRESS_COUNT (V4L2_CID_EXT_VBE_BASE + 32)

//
// SET GSR2 control ID
//
/**
 * @brief Set GSR2
 *
 * @rst
 * This function is ctrl of GSR2 (global sticky reduction 2) for video area(OLED only).
 * Reduces the luminance per pixel, which has a high risk of afterimages of high
 * luminance/high color, not the overall luminance reduction.
 *
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_GSR2
 *
 *	   //
 *	   // parameter
 *     //
 *     // GSR setting tbl
 *       struct v4l2_ext_vbe_panel_second_gsr_info{
 *              unsigned int *pGsrTable;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set GSR2
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbe_panel_second_gsr_info gsr2info;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&gsr2info, 0, sizeof(struct v4l2_ext_vbe_panel_second_gsr_info));
 *
 *     gsr2info.pGsrTable = Tbl;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBE_GSR2;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbe_panel_second_gsr_info);
 *     ext_controls.controls->string = (void *)&gsr2info;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls)
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_GSR2 (V4L2_CID_EXT_VBE_BASE + 33)

//
// SET IRRAdaptedLum control ID
//
/**
 * @brief Set IrrAdaptedLum
 *
 * @rst
 * This function is set of Image retention risk for luminance (OLED SIC chip only).
 *
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_S_EXT_CTRLS
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_IRR_ADAPTEDLUM
 *
 *	   //
 *	   // parameter
 *     //
 *     // PQ luminance setting factor(PLC portion)
 *       struct v4l2_ext_vbe_panel_irr_info{
 *              unsigned int *pIrrlum;
 *       };
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *     // Set GSR2
 *       struct v4l2_ext_controls ext_controls;
 *       struct v4l2_ext_control ext_control;
 *       struct v4l2_ext_vbe_panel_irr_info irr;
 * 
 *       memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *       memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *       memset(&irr, 0, sizeof(struct v4l2_ext_vbe_panel_irr_info));
 *
 *       irr.pIrrlum = Lumdata;
 * 
 *       ext_controls.ctrl_class       = V4L2_CTRL_CLASS_USER;
 *       ext_controls.count            = 1;
 *       ext_controls.controls         = &ext_control;
 *       ext_controls.controls->id     = V4L2_CID_EXT_VBE_IRR_ADAPTEDLUM;
 *       ext_controls.controls->size   = sizeof(struct v4l2_ext_vbe_panel_irr_info);
 *       ext_controls.controls->string = (char *)&irr;
 *
 *     ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls)
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_IRR_ADAPTEDLUM (V4L2_CID_EXT_VBE_BASE + 34)

//
// GET IRR RISK control ID
//
/**
 * @brief Get GetIrrResult
 *
 * @rst
 * This function is getter of IRR risk when using the AI image retention risk detect block
 * (OLED SIC chip only).
 *
 *
 * Commands & Parameters
 *	 .. code-block:: cpp
 *
 *	   //
 *	   // ioctl command
 *	   //
 *	   VIDIOC_G_CTRL
 *
 *	   //
 *	   // control id
 *	   //
 *	   // You have to make the new control id as the below.
 *	   V4L2_CID_EXT_VBE_IRR_RESULT
 *
 *	   //
 *	   // parameter
 *	   //
 *	   unsigned int;
 *
 * Return Value
 *	 On success 0 is returned.
 *
 *	 On error -1 and the errno variable is set appropriately. The generic error
 *	 codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *	 Application can set data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *	 .. code-block:: cpp
 *
 *	 // Image Retention Risk
 *   v4l2_control control;
 *   control.id = V4L2_CID_EXT_VBE_IRR_RESULT;
 *
 *   ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBE_IRR_RESULT (V4L2_CID_EXT_VBE_BASE + 35)

//
// HDMI class control IDs
//

/**
 * @brief Get HDMI timing info
 *
 * @rst
 * This function gets HDMI timing information to each port.
 * Correct timing information must be delivered independently for each HDMI port in background mode.
 * When the V frequency is 0, other fields must be transferred as an initial value.
 *
 * If SoC can determine a resolution for input signal, even SoC can not process the input signal, SoC should send the timing information to LG MW.
 *
 * The H/V Freq. Boundary and the associated Enum for the HFR signal shall be checked for problems.
 *
 * SoC should support both TMDS and FRL for FreeSync. When the FRL FreeSync signal input is received, it should be output to the normal screen without any problems.
 *
 * The FRL signal shall be able to detect changes when the signal changes without re-link training. ex) It happen when the frame rate was changed from 50Hz to 60Hz on RTX3080 PC.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Timinginfo
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_TIMING_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;   // horizontal start position
 *         unsigned short y;   // vertical start position
 *         unsigned short w;   // horizontal size(width)
 *         unsigned short h;   // vertical size(height)
 *     };
 *
 *     enum v4l2_ext_hdmi_mode
 *     {
 *         V4L2_EXT_HDMI_MODE_DVI = 0,
 *         V4L2_EXT_HDMI_MODE_HDMI,
 *     };
 *
 *     enum v4l2_ext_hdmi_color_depth
 *     {
 *         V4L2_EXT_HDMI_COLOR_DEPTH_8BIT = 0,
 *         V4L2_EXT_HDMI_COLOR_DEPTH_10BIT,
 *         V4L2_EXT_HDMI_COLOR_DEPTH_12BIT,
 *         V4L2_EXT_HDMI_COLOR_DEPTH_16BIT,
 *         V4L2_EXT_HDMI_COLOR_DEPTH_RESERVED
 *     };
 *
 *     struct v4l2_ext_hdmi_timing_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned short h_freq;
 *         unsigned short v_freq;
 *         unsigned short h_total;
 *         unsigned short v_total;
 *         unsigned short h_porch;
 *         unsigned short v_porch;
 *         struct v4l2_ext_video_rect active;
 *         unsigned short scan_type;
 *         enum v4l2_ext_hdmi_mode dvi_hdmi;
 *         enum v4l2_ext_hdmi_color_depth color_depth;
 *         unsigned char allm_mode;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Video Timing Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_timing_info hdmi_timing_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_timing_info, 0, sizeof(struct v4l2_ext_hdmi_timing_info));
 *
 *     hdmi_timing_info.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_TIMING_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_timing_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_timing_info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_TIMING_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 0)

/**
 * @brief Get HDMI DRM info
 *
 * @rst
 * This function return DynamicRange & Metadata for HDR.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI DRMInfo
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_DRM_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_drm_eotf
 *     {
 *         V4L2_EXT_HDMI_DRM_EOTF_SDR_LUMINANCE_RANGE = 0,
 *         V4L2_EXT_HDMI_DRM_EOTF_HDR_LUMINANCE_RANGE,
 *         V4L2_EXT_HDMI_DRM_EOTF_SMPTE_ST_2084,
 *         V4L2_EXT_HDMI_DRM_EOTF_HLG,
 *         V4L2_EXT_HDMI_DRM_EOTF_RESERVED_4,
 *         V4L2_EXT_HDMI_DRM_EOTF_RESERVED_5,
 *         V4L2_EXT_HDMI_DRM_EOTF_RESERVED_6,
 *         V4L2_EXT_HDMI_DRM_EOTF_RESERVED_7,
 *     };
 *
 *     enum v4l2_ext_hdmi_drm_meta_desc
 *     {
 *         V4L2_EXT_HDMI_DRM_META_DESC_TYPE1 = 0,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED1,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED2,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED3,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED4,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED5,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED6,
 *         V4L2_EXT_HDMI_DRM_META_DESC_RESERVED7,
 *     };
 *
 *     struct v4l2_ext_hdmi_drm_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char version;
 *         unsigned char length;
 *         enum v4l2_ext_hdmi_drm_eotf eotf_type;
 *         enum v4l2_ext_hdmi_drm_meta_desc meta_desc;
 *         unsigned short display_primaries_x0;
 *         unsigned short display_primaries_y0;
 *         unsigned short display_primaries_x1;
 *         unsigned short display_primaries_y1;
 *         unsigned short display_primaries_x2;
 *         unsigned short display_primaries_y2;
 *         unsigned short white_point_x;
 *         unsigned short white_point_y;
 *         unsigned short max_display_mastering_luminance;
 *         unsigned short min_display_mastering_luminance;
 *         unsigned short maximum_content_light_level;
 *         unsigned short maximum_frame_average_light_level;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get DRM Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_drm_info hdmi_drm_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_drm_info, 0, sizeof(struct v4l2_ext_hdmi_drm_info));
 *
 *     hdmi_drm_info.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_DRM_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_drm_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_drm_info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_DRM_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 1)

/**
 * @brief Get HDMI VSI info
 *
 * @rst
 * This function get VSI inforframe.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * It should be determined that ALLM was terminated even when VSIF Packet transmission was stopped without sending ALLM=0 at the end of the ALLM signal.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI VSI Info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_VSI_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     #define V4L2_HDMI_PACKET_DATA_LENGTH 28
 *     #define V4L2_HDMI_INFOFRAME_PACKET_LEN 28
 *     #define V4L2_HDMI_VENDOR_SPECIFIC_REGID_LEN 3
 *     #define V4L2_HDMI_VENDOR_SPECIFIC_PAYLOAD_LEN \
 *          (V4L2_HDMI_INFOFRAME_PACKET_LEN - V4L2_HDMI_VENDOR_SPECIFIC_REGID_LEN)
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_vsi_video_format {
 *         V4L2_EXT_HDMI_VSI_VIDEO_FORMAT_NO_ADDITIONAL_FORMAT = 0,
 *         V4L2_EXT_HDMI_VSI_VIDEO_FORMAT_EXTENDED_RESOLUTION_FORMAT,
 *         V4L2_EXT_HDMI_VSI_VIDEO_FORMAT_3D_FORMAT,
 *     };
 *
 *     enum v4l2_ext_hdmi_vsi_3d_structure
 *     {
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_FRAME_PACKING = 0,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_FIELD_ALTERNATIVE,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_LINE_ALTERNATIVE,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_SIDEBYSIDE_FULL,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_L_DEPTH,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_L_DEPTH_GRAPHICS,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_TOP_BOTTOM,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_SIDEBYSIDE_HALF    = 0x08,
 *         V4L2_EXT_HDMI_VSI_3D_STRUCTURE_TOP_BOTTOM_DIRECTV = 0x09,
 *     };
 *
 *     enum v4l2_ext_hdmi_vsi_3d_ext_data
 *     {
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_HOR_SUB_SAMPL_0 = 0,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_HOR_SUB_SAMPL_1,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_HOR_SUB_SAMPL_2,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_HOR_SUB_SAMPL_3,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_QUINCUNX_MATRIX_0,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_QUINCUNX_MATRIX_1,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_QUINCUNX_MATRIX_2,
 *         V4L2_EXT_HDMI_VSI_3D_EXT_DATA_QUINCUNX_MATRIX_3,
 *     };
 *
 *     enum v4l2_ext_hdmi_vsi_vic
 *     {
 *         V4L2_EXT_HDMI_VSI_VIC_RESERVED = 0,
 *         V4L2_EXT_HDMI_VSI_VIC_4K2K_30HZ,
 *         V4L2_EXT_HDMI_VSI_VIC_4K2K_25HZ,
 *         V4L2_EXT_HDMI_VSI_VIC_4K2K_24HZ,
 *         V4L2_EXT_HDMI_VSI_VIC_4K2K_24HZ_SMPTE,
 *     };
 *
 *     enum v4l2_ext_hdmi_packet_status
 *     {
 *         V4L2_EXT_HDMI_PACKET_STATUS_NOT_RECEIVED = 0,
 *         V4L2_EXT_HDMI_PACKET_STATUS_STOPPED,
 *         V4L2_EXT_HDMI_PACKET_STATUS_UPDATED,
 *     };
 *
 *     struct v4l2_ext_hdmi_in_packet
 *     {
 *         unsigned char type;
 *         unsigned char version;
 *         unsigned char length;
 *         unsigned char data_bytes[V4L2_EXT_HDMI_PACKET_DATA_LENGTH];
 *     };
 *
 *     struct v4l2_ext_hdmi_vsi_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_vsi_video_format video_format;
 *         enum v4l2_ext_hdmi_vsi_3d_structure st_3d;
 *         enum v4l2_ext_hdmi_vsi_3d_ext_data ext_data_3d;
 *         enum v4l2_ext_hdmi_vsi_vic vic;
 *         unsigned char regid[V4L2_EXT_HDMI_VENDOR_SPECIFIC_REGID_LEN];
 *         unsigned char payload[V4L2_EXT_HDMI_VENDOR_SPECIFIC_PAYLOAD_LEN];
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get VSI Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_vsi_info hdmi_vsi;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_vsi, 0, sizeof(struct v4l2_ext_hdmi_vsi_info));
 *
 *     hdmi_vsi.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_VSI_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_vsi_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_vsi;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_VSI_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 2)

/**
 * @brief Get HDMI SPD info
 *
 * @rst
 * This function get SPD inforframe.
 * In CEA-861, recommended SPD packet interval is 1 seconds.
 * Driver should keep previous SPD packet even if the packet is not received for more than 1 second as long as signal keeps valid.
 * It means V4L2_CID_EXT_HDMI_SPD_INFO always returns valid SPD data if it is received at least once after valid video timing.
 * When processing SPD Info packet, checksum should be verified.
 * If the checksum value is not correct, the SPD info packet is not delivered.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * If only SPD information changes without signal changes, the changed information should be updated.
 *
 * When forwarding AMD VSIF using SPD InfoFrame,
 *
 * VRR must be enabled when FreeSync status is Enabled and Active.
 *
 * When the FreeSync state goes back and forth between Enabled and Active, the following behavior should not occur:
 *
 *     #. Flicker shall not occur.
 *
 *     #. Signal Detect shall not occur.
 *
 *     #. Mute shall not occur.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI SPD Info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_SPD_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     #define V4L2_HDMI_PACKET_DATA_LENGTH 28
 *     #define V4L2_HDMI_SPD_IF_VENDOR_LEN      8
 *     #define V4L2_HDMI_SPD_IF_DESC_LEN       16
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_packet_status
 *     {
 *         V4L2_EXT_HDMI_PACKET_STATUS_NOT_RECEIVED = 0,
 *         V4L2_EXT_HDMI_PACKET_STATUS_STOPPED,
 *         V4L2_EXT_HDMI_PACKET_STATUS_UPDATED,
 *     };
 *
 *     struct v4l2_ext_hdmi_in_packet
 *     {
 *         unsigned char type;
 *         unsigned char version;
 *         unsigned char length;
 *         unsigned char data_bytes[V4L2_EXT_HDMI_PACKET_DATA_LENGTH];
 *     };
 *
 *     struct v4l2_ext_hdmi_spd_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char vendor_name[V4L2_EXT_HDMI_SPD_IF_VENDOR_LEN + 1];
 *         unsigned char product_description[V4L2_EXT_HDMI_SPD_IF_DESC_LEN + 1];
 *         unsigned char source_device_info;
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get SPD Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_spd_info hdmi_spd;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_spd, 0, sizeof(struct v4l2_ext_hdmi_spd_info));
 *
 *     hdmi_spd.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_SPD_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_spd_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_spd;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_SPD_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 3)

/**
 * @brief Get HDMI AVI info
 *
 * @rst
 * This function get AVI inforframe.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * If the Source Device sends only SET_AVMUTE without sending CLEAR_AVMUTE, the previous Mute state should be cleared when the signal is detected again.
 *
 * FRL signal input should be properly updated in AVI Packet information. ex) Pixel Format.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI AVI Info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_AVI_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     #define V4L2_HDMI_PACKET_DATA_LENGTH 28
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_csc
 *     {
 *         V4L2_EXT_HDMI_AVI_CSC_RGB = 0,
 *         V4L2_EXT_HDMI_AVI_CSC_YCBCR422,
 *         V4L2_EXT_HDMI_AVI_CSC_YCBCR444,
 *         V4L2_EXT_HDMI_AVI_CSC_YCBCR420,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_active_info
 *     {
 *         V4L2_EXT_HDMI_AVI_ACTIVE_INFO_INVALID = 0,
 *         V4L2_EXT_HDMI_AVI_ACTIVE_INFO_VALID,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_bar_info
 *     {
 *         V4L2_EXT_HDMI_AVI_BAR_INFO_INVALID = 0,
 *         V4L2_EXT_HDMI_AVI_BAR_INFO_VERTICALVALID,
 *         V4L2_EXT_HDMI_AVI_BAR_INFO_HORIZVALID,
 *         V4L2_EXT_HDMI_AVI_BAR_INFO_VERTHORIZVALID,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_scan_info
 *     {
 *         V4L2_EXT_HDMI_AVI_SCAN_INFO_NODATA = 0,
 *         V4L2_EXT_HDMI_AVI_SCAN_INFO_OVERSCANNED,
 *         V4L2_EXT_HDMI_AVI_SCAN_INFO_UNDERSCANNED,
 *         V4L2_EXT_HDMI_AVI_SCAN_INFO_FUTURE,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_colorimetry
 *     {
 *         V4L2_EXT_HDMI_AVI_COLORIMETRY_NODATA = 0,
 *         V4L2_EXT_HDMI_AVI_COLORIMETRY_SMPTE170,
 *         V4L2_EXT_HDMI_AVI_COLORIMETRY_ITU709,
 *         V4L2_EXT_HDMI_AVI_COLORIMETRY_FUTURE,
 *         V4L2_EXT_HDMI_AVI_COLORIMETRY_EXTENDED = V4L2_EXT_HDMI_AVI_COLORIMETRY_FUTURE,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_picture_arc
 *     {
 *         V4L2_EXT_HDMI_AVI_PICTURE_ARC_NODATA = 0,
 *         V4L2_EXT_HDMI_AVI_PICTURE_ARC_4_3,
 *         V4L2_EXT_HDMI_AVI_PICTURE_ARC_16_9,
 *         V4L2_EXT_HDMI_AVI_PICTURE_ARC_FUTURE,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_active_format_arc
 *     {
 *         V4L2_EXT_HDMI_AVI_ACTIVE_FORMAT_ARC_PICTURE = 8,
 *         V4L2_EXT_HDMI_AVI_ACTIVE_FORMAT_ARC_4_3CENTER = 9,
 *         V4L2_EXT_HDMI_AVI_ACTIVE_FORMAT_ARC_16_9CENTER = 10,
 *         V4L2_EXT_HDMI_AVI_ACTIVE_FORMAT_ARC_14_9CENTER = 11,
 *         V4L2_EXT_HDMI_AVI_ACTIVE_FORMAT_ARC_OTHER = 0,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_scaling
 *     {
 *         V4L2_EXT_HDMI_AVI_SCALING_NOSCALING = 0,
 *         V4L2_EXT_HDMI_AVI_SCALING_HSCALING,
 *         V4L2_EXT_HDMI_AVI_SCALING_VSCALING,
 *         V4L2_EXT_HDMI_AVI_SCALING_HVSCALING,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_it_content
 *     {
 *         V4L2_EXT_HDMI_AVI_IT_CONTENT_NODATA = 0,
 *         V4L2_EXT_HDMI_AVI_IT_CONTENT_ITCONTENT,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_ext_colorimetry
 *     {
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_XVYCC601 = 0,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_XVYCC709,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_SYCC601,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_ADOBEYCC601,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_ADOBERGB,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_BT2020_YCCBCCRC,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_BT2020_RGBORYCBCR,
 *         V4L2_EXT_HDMI_AVI_EXT_COLORIMETRY_XVRESERED,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_rgb_quantization_range
 *     {
 *         V4L2_EXT_HDMI_AVI_RGB_QUANTIZATION_RANGE_DEFAULT = 0,
 *         V4L2_EXT_HDMI_AVI_RGB_QUANTIZATION_RANGE_LIMITEDRANGE,
 *         V4L2_EXT_HDMI_AVI_RGB_QUANTIZATION_RANGE_FULLRANGE,
 *         V4L2_EXT_HDMI_AVI_RGB_QUANTIZATION_RANGE_RESERVED,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_ycc_quantization_range
 *     {
 *         V4L2_EXT_HDMI_AVI_YCC_QUANTIZATION_RANGE_LIMITEDRANGE = 0,
 *         V4L2_EXT_HDMI_AVI_YCC_QUANTIZATION_RANGE_FULLRANGE,
 *         V4L2_EXT_HDMI_AVI_YCC_QUANTIZATION_RANGE_RESERVED,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_content_type
 *     {
 *         V4L2_EXT_HDMI_AVI_CONTENT_TYPE_GRAPHICS = 0,
 *         V4L2_EXT_HDMI_AVI_CONTENT_TYPE_PHOTO,
 *         V4L2_EXT_HDMI_AVI_CONTENT_TYPE_CINEMA,
 *         V4L2_EXT_HDMI_AVI_CONTENT_TYPE_GAME,
 *     };
 *
 *     enum v4l2_ext_hdmi_avi_additional_colorimetry
 *     {
 *         V4L2_EXT_HDMI_AVI_ADDITIONAL_COLORIMETRY_DCI_P3_D65 = 0,
 *         V4L2_EXT_HDMI_AVI_ADDITIONAL_COLORIMETRY_DCI_P3_THEATER,
 *         V4L2_EXT_HDMI_AVI_ADDITIONAL_COLORIMETRY_RESERVED,
 *     };
 *
 *     enum v4l2_ext_hdmi_packet_status
 *     {
 *         V4L2_EXT_HDMI_PACKET_STATUS_NOT_RECEIVED = 0,
 *         V4L2_EXT_HDMI_PACKET_STATUS_STOPPED,
 *         V4L2_EXT_HDMI_PACKET_STATUS_UPDATED,
 *     };
 *
 *     struct v4l2_ext_hdmi_in_packet
 *     {
 *         unsigned char type;
 *         unsigned char version;
 *         unsigned char length;
 *         unsigned char data_bytes[V4L2_EXT_HDMI_PACKET_DATA_LENGTH];
 *     };
 *
 *     struct v4l2_ext_hdmi_avi_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_mode mode;
 *
 *         enum v4l2_ext_hdmi_avi_csc pixel_encoding;
 *         enum v4l2_ext_hdmi_avi_active_info active_info;
 *         enum v4l2_ext_hdmi_avi_bar_info bar_info;
 *         enum v4l2_ext_hdmi_avi_scan_info scan_info;
 *         enum v4l2_ext_hdmi_avi_colorimetry colorimetry;
 *         enum v4l2_ext_hdmi_avi_picture_arc picture_aspect_ratio;
 *         enum v4l2_ext_hdmi_avi_active_format_arc active_format_aspect_ratio;
 *         enum v4l2_ext_hdmi_avi_scaling scaling;
 *
 *         unsigned char vic;
 *         unsigned char pixel_repeat;
 *
 *         enum v4l2_ext_hdmi_avi_it_content it_content;
 *         enum v4l2_ext_hdmi_avi_ext_colorimetry extended_colorimetry;
 *         enum v4l2_ext_hdmi_avi_rgb_quantization_range rgb_quantization_range;
 *         enum v4l2_ext_hdmi_avi_ycc_quantization_range ycc_quantization_range;
 *         enum v4l2_ext_hdmi_avi_content_type content_type;
 *         enum v4l2_ext_hdmi_avi_additional_colorimetry additional_colorimetry;
 *
 *         unsigned short top_bar_end_line_number;
 *         unsigned short bottom_bar_start_line_number;
 *         unsigned short left_bar_end_pixel_number;
 *         unsigned short right_bar_end_pixel_number;
 *
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get AVI Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_avi_info hdmi_avi;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_avi, 0, sizeof(struct v4l2_ext_hdmi_avi_info));
 *
 *     hdmi_avi.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_AVI_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_avi_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_avi;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_AVI_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 4)

/**
 * @brief Get HDMI Packet info
 *
 * @rst
 * This function gets HDMI packet(AVI,SPD,VSI) information.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI ALL Packet Info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_PACKET_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_mode
 *     {
 *         V4L2_EXT_HDMI_MODE_DVI = 0,
 *         V4L2_EXT_HDMI_MODE_HDMI,
 *     };
 *
 *     struct v4l2_ext_hdmi_avi_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_mode mode;
 *
 *         enum v4l2_ext_hdmi_avi_csc pixel_encoding;
 *         enum v4l2_ext_hdmi_avi_active_info active_info;
 *         enum v4l2_ext_hdmi_avi_bar_info bar_info;
 *         enum v4l2_ext_hdmi_avi_scan_info scan_info;
 *         enum v4l2_ext_hdmi_avi_colorimetry colorimetry;
 *         enum v4l2_ext_hdmi_avi_picture_arc picture_aspect_ratio;
 *         enum v4l2_ext_hdmi_avi_active_format_arc active_format_aspect_ratio;
 *         enum v4l2_ext_hdmi_avi_scaling scaling;
 *
 *         unsigned char vic;
 *         unsigned char pixel_repeat;
 *
 *         enum v4l2_ext_hdmi_avi_it_content it_content;
 *         enum v4l2_ext_hdmi_avi_ext_colorimetry extended_colorimetry;
 *         enum v4l2_ext_hdmi_avi_rgb_quantization_range rgb_quantization_range;
 *         enum v4l2_ext_hdmi_avi_ycc_quantization_range ycc_quantization_range;
 *         enum v4l2_ext_hdmi_avi_content_type content_type;
 *         enum v4l2_ext_hdmi_avi_additional_colorimetry additional_colorimetry;
 *
 *         unsigned short top_bar_end_line_number;
 *         unsigned short bottom_bar_start_line_number;
 *         unsigned short left_bar_end_pixel_number;
 *         unsigned short right_bar_end_pixel_number;
 *
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 *     struct v4l2_ext_hdmi_spd_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char vendor_name[V4L2_EXT_HDMI_SPD_IF_VENDOR_LEN + 1];
 *         unsigned char product_description[V4L2_EXT_HDMI_SPD_IF_DESC_LEN + 1];
 *         unsigned char source_device_info;
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 *     struct v4l2_ext_hdmi_vsi_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_vsi_video_format video_format;
 *         enum v4l2_ext_hdmi_vsi_3d_structure st_3d;
 *         enum v4l2_ext_hdmi_vsi_3d_ext_data ext_data_3d;
 *         enum v4l2_ext_hdmi_vsi_vic vic;
 *         unsigned char regid[V4L2_EXT_HDMI_VENDOR_SPECIFIC_REGID_LEN];
 *         unsigned char payload[V4L2_EXT_HDMI_VENDOR_SPECIFIC_PAYLOAD_LEN];
 *         enum v4l2_ext_hdmi_packet_status packet_status;
 *         struct v4l2_ext_hdmi_in_packet packet;
 *     };
 *
 *     struct v4l2_ext_hdmi_packet_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_mode mode;
 *         struct v4l2_ext_hdmi_avi_info avi;
 *         struct v4l2_ext_hdmi_spd_info spd;
 *         struct v4l2_ext_hdmi_vsi_info vsi;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Packet Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_packet_info hdmi_packet;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_packet, 0, sizeof(struct v4l2_ext_hdmi_packet_info));
 *
 *     hdmi_packet.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_PACKET_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_packet_info);
 *     ext_controls.controls->ptr = (void *)&hdmi_packet;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_PACKET_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 5)

/**
 * @brief Get Dolby HDR Type
 *
 * @rst
 * This control returns Dolby HDR Type.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Dolby HDR Type
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_DOLBY_HDR
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_dolby_hdr_type
 *     {
 *         V4L2_EXT_HDMI_DOLBY_HDR_TYPE_SDR = 0,
 *         V4L2_EXT_HDMI_DOLBY_HDR_TYPE_STANDARD_VSIF_1,
 *         V4L2_EXT_HDMI_DOLBY_HDR_TYPE_STANDARD_VSIF_2,
 *         V4L2_EXT_HDMI_DOLBY_HDR_TYPE_LOW_LATENCY,
 *     };
 *
 *     struct v4l2_ext_hdmi_dolby_hdr
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_dolby_hdr_type type;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Dolby HDR Type
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_dolby_hdr dolby_hdr;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&dolby_hdr, 0, sizeof(struct v4l2_ext_hdmi_dolby_hdr));
 *
 *     dolby_hdr.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_DOLBY_HDR;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_dolby_hdr);
 *     ext_controls.controls->ptr = (void *)&dolby_hdr;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_DOLBY_HDR (V4L2_CID_USER_EXT_HDMI_BASE + 6)

/**
 * @brief Get/Set HDMI EDID
 *
 * @rst
 * This control returns Dolby HDR Type.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI EDID
 *     VIDIOC_S_EXT_CTRLS      // Set HDMI EDID
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_EDID
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_edid_size
 *     {
 *         V4L2_EXT_HDMI_EDID_SIZE_128 = 0,
 *         V4L2_EXT_HDMI_EDID_SIZE_256,
 *         V4L2_EXT_HDMI_EDID_SIZE_512,
 *     };
 *
 *     struct v4l2_ext_hdmi_edid
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_edid_size size;
 *         union
 *         {
 *             unsigned char *pData;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set EDID Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_edid edid;
 *     unsigned char data[256] = {1,2,3,4,5, };
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&edid, 0, sizeof(struct v4l2_ext_hdmi_edid));
 *
 *     edid.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     edid.size = V4L2_EXT_HDMI_EDID_SIZE_256;
 *     edid.pData = &data;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_EDID;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_edid);
 *     ext_controls.controls->ptr = (void *)&edid;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get EDID Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_edid edid;
 *     unsigned char data[256] = {0, };
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&dolby_hdr, 0, sizeof(struct v4l2_ext_hdmi_edid));
 *
 *     edid.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     edid.size = V4L2_EXT_HDMI_EDID_SIZE_128;
 *     edid.pData = &data;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_EDID;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_edid);
 *     ext_controls.controls->ptr = (void *)&edid;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_EDID (V4L2_CID_USER_EXT_HDMI_BASE + 7)

/**
 * @brief Get Port Connection Status
 *
 * @rst
 * This control returns HDMI 5V information.
 * HDMI 5V information is enable when HDMI device connect each ports.If selected port supports MHL,
 * do not enable 5V information when only connected MHL cable. 5V information is enable when MHL device connect.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get Port Connection Status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_CONNECTION_STATE
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_connection_state
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char state;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI 5V state
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_connection_state hdmi_port_state;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdmi_port_state, 0, sizeof(struct v4l2_ext_hdmi_connection_state));
 *
 *     hdmi_port_state.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_CONNECTION_STATE;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_connection_state);
 *     ext_controls.controls->ptr = (void *)&hdmi_port_state;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_CONNECTION_STATE (V4L2_CID_USER_EXT_HDMI_BASE + 8)

/**
 * @brief Set/Get Port HPD Control
 *
 * @rst
 * This control handles HPD controls. (Enable / Disable / Restart)
 * If V4L2_EXT_HDMI_HPD_RESTART is called before V4L2_EXT_HDMI_HPD_ENABLE, it should be ignored.
 * The internal mute operation should be performed when HPD is restarted, not when the EDID is changed.
 * When entering the Suspend mode, the time of the HPD low duration is set by "V4L2_CID_EXT_HDMI_HPD_LOW_DURATION_DC_ON"
 * and this value should be applied when Resume. The default is 100ms.
 * The internal mute operation should be performed when HPD is restarted, not when the EDID is changed.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set Control HPD
 *     VIDIOC_G_EXT_CTRLS      // Get Control HPD
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HPD
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_hpd_state
 *     {
 *         V4L2_EXT_HDMI_HPD_DISABLE = 0,
 *         V4L2_EXT_HDMI_HPD_ENABLE,
 *         V4L2_EXT_HDMI_HPD_RESTART,
 *     };
 *
 *     struct v4l2_ext_hdmi_hpd
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_hpd_state hpd_state;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Control HPD
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hpd hpd;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hpd, 0, sizeof(struct v4l2_ext_hdmi_hpd));
 *
 *     hpd.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     hpd.eHPDControl = V4L2_HDMI_HPD_CONTROL_DISABLE;
 *     //hpd.eHPDControl = V4L2_HDMI_HPD_CONTROL_ENABLE;
 *     //hpd.eHPDControl = V4L2_HDMI_HPD_CONTROL_RESTART;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HPD;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hpd);
 *     ext_controls.controls->ptr = (void *)&hpd;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get Control HPD
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hpd hpd;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hpd, 0, sizeof(struct v4l2_ext_hdmi_hpd));
 *
 *     hpd.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HPD;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hpd);
 *     ext_controls.controls->ptr = (void *)&hpd;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_HPD (V4L2_CID_USER_EXT_HDMI_BASE + 9)

/**
 * @brief Set Power Off
 *
 * @rst
 * In standby or suspend mode, EDID should be accessible even though we don't use external eeprom to save EEID.
 * And SoC HDMI block may goes to power down mode to maintail EDID data. This function let hdmi driver know that
 * tv goes to standby mode even when QSM is off. When QSM is off, TV does not follow normal suspend routine.
 * It is called when it goes to cold standby mode so that driver knows it is going to cold standby mode and
 * turns on 'always on module' inside HDMI SoC.
 * If this notification is not required in the SoC,  it may just return ok.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set Power Off
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_POWER_OFF
 *
 *     //
 *     // control value
 *     //
 *     0 (Ignore), 1 (Set Power Off)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *   This control id supports VIDIOC_S_CTRL only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Power off mode
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_HDMI_POWER_OFF;
 *     control.value = 1;      // power off mode
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_POWER_OFF (V4L2_CID_USER_EXT_HDMI_BASE + 10)

/**
 * @brief Write HDMI HDCP key
 *
 * @rst
 * This control writes HDCP key.
 * The HDCP key for testing should not be included in the software.
 * If a temporary HDCP key is used during Bring up, it must be deleted before event starting.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set HDCP Key data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HDCP_KEY
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_hdcp_version
 *     {
 *         V4L2_EXT_HDMI_HDCP_VERSION_14 = 0,
 *         V4L2_EXT_HDMI_HDCP_VERSION_22,
 *         V4L2_EXT_HDMI_HDCP_VERSION_RESERVED,
 *     };
 *
 *     struct v4l2_ext_hdmi_hdcp_key
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_hdcp_version version;
 *         unsigned int key_size;
 *         union {
 *             unsigned char *pData;
 *             unsigned int compat_data;
 *             unsigned long long sizer;
 *         };
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Write HDCP Key
 *     #define HDCP14_KEY_SIZE 288
 *
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hdcp_key hdcp_key;
 *     unsigned char key[HDCP14_KEY_SIZE] = {1,2,3,4,5,};
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hdcp_key, 0, sizeof(struct v4l2_ext_hdmi_hdcp_key));
 *
 *     hdcp_key.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     hdcp_key.pData = &key;
 *     hdcp_key.version = V4L2_EXT_HDMI_HDCP_VERSION_14;
 *     hdcp_key.key_size = HDCP14_KEY_SIZE;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HDCP_KEY;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hdcp_key);
 *     ext_controls.controls->ptr = (void *)&hdcp_key;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_HDCP_KEY (V4L2_CID_USER_EXT_HDMI_BASE + 11)

/**
 * @brief Dissconnect HDMI Port
 *
 * @rst
 * Disconnect HDMI port.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Disconnect HDMI Port
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_DISCONNECT
 *
 *     //
 *     // control value
 *     //
 *     enum v4l2_hdmi_input_port
 *     {
 *         HDMI_INPUT_PORT_NONE = 0, //HDMI_INPUT_PORT_NONE means "HDMI Disconnect".
 *         // The below enum values mean "HDMI Connect".
 *         HDMI_INPUT_PORT1,
 *         HDMI_INPUT_PORT2,
 *         HDMI_INPUT_PORT3,
 *         HDMI_INPUT_PORT4,
 *         HDMI_INPUT_PORT_ALL,
 *         HDMI_INPUT_PORT_MAX,
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *   This control id supports VIDIOC_S_CTRL only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Disconnet HDMI Port
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_HDMI_DISCONNECT;
 *     control.value = (int)HDMI_INPUT_PORT1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_DISCONNECT (V4L2_CID_USER_EXT_HDMI_BASE + 12)

/**
 * @brief Get HDMI VRR Frequency
 *
 * @rst
 * This control returns VRR frequency.
 * This spec is added from HDMI2.1 frequency unit is Hz*100 (59.98hz → 5998, 120hz → 12000).
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI VRR Frequency
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_VRR_FREQUENCY
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_vrr_frequency
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned short frequency;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI VRR Frequency
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_vrr_frequency freq;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&freq, 0, sizeof(struct v4l2_ext_hdmi_vrr_frequency));
 *
 *     freq.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_VRR_FREQUENCY;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_vrr_frequency);
 *     ext_controls.controls->ptr = (void *)&freq;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_VRR_FREQUENCY (V4L2_CID_USER_EXT_HDMI_BASE + 13)

/**
 * @brief Get HDMI EMPacket Info
 *
 * @rst
 * This control returns Extended Metadata Packet information. This spec is added from HDMI2.1.
 * As soon as getting EMP data from a device, BSP should send the EMP data to LG MW.
 * If HDMI clock is unstable or Vertical Frequency is 0, Infoframe packet should be cleared.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI EMPacket Info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_EMP_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_emp_type
 *     {
 *         V4L2_EXT_HDMI_EMP_TYPE_UNDEFINED = 0,
 *         V4L2_EXT_HDMI_EMP_TYPE_VSEM,
 *         V4L2_EXT_HDMI_EMP_TYPE_DYNAMICHDREM,
 *         V4L2_EXT_HDMI_EMP_TYPE_VTEM,
 *         V4L2_EXT_HDMI_EMP_TYPE_CVTEM
 *     };
 *
 *     struct v4l2_ext_hdmi_emp_info
 *     {
 *         enum v4l2_ext_hdmi_input_port port; // in
 *         enum v4l2_ext_hdmi_emp_type type; // in
 *         unsigned short current_packet_index; // in
 *         unsigned short total_packet_number; // out
 *         unsigned char data[31]; // out
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI EMPacket Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_emp_info emp;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&emp, 0, sizeof(struct v4l2_ext_hdmi_emp_info));
 *
 *     emp.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     emp.type = V4L2_EXT_HDMI_EMP_TYPE_VTEM;
 *     emp.current_packet_index = 1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_EMP_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_emp_info);
 *     ext_controls.controls->ptr = (void *)&emp;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_EMP_INFO (V4L2_CID_USER_EXT_HDMI_BASE + 14)

/**
 * @brief Set/Get HDCP Repeater
 *
 * @rst
 * This control sets value about HDCP repeater. Setting values are related with video back end.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS        // Set HDCP Repeater
 *     VIDIOC_G_EXT_CTRLS        // Get HDCP Repeater
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HDCP_REPEATER
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_hdcp_repeater
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char repeater_mode;
 *         unsigned char receiver_id[5];
 *         unsigned char repeater_hpd;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDCP Repeater
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hdcp_repeater repeater;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&repeater, 0, sizeof(struct v4l2_ext_hdmi_hdcp_repeater));
 *
 *     repeater.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     repeater.repeater_mode = 1;
 *     repeater.receiver_id[0] = 1;
 *     repeater.receiver_hpd =1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HDCP_REPEATER;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hdcp_repeater);
 *     ext_controls.controls->ptr = (void *)&repeater;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 *     // Set HDCP Repeater
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hdcp_repeater repeater;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&repeater, 0, sizeof(struct v4l2_ext_hdmi_hdcp_repeater));
 *
 *     repeater.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HDCP_REPEATER;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hdcp_repeater);
 *     ext_controls.controls->ptr = (void *)&repeater;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_HDCP_REPEATER (V4L2_CID_USER_EXT_HDMI_BASE + 15)

/**
 * @brief Set EOTF value to BSP
 *
 * @rst
 * This function used to force EOTF values to be set to BSP by HDMI port.
 * If LG delivers certain eotf values through this function,
 * the BSP must change the BSP information to the value delivered by LG,
 * not the eotf value of the actual drm packet(V4L2_CID_EXT_HDMI_DRM_INFO).
 * This value must be applied to the HDMI BSP area referenced by PQ or SCALER BSP.
 * PQ/SCALER BSP must operate with the eotf value set by LG.
 * LG will only call the function once if the EOTF value is changed.
 * If AUTO , use the EOTF value of the actual DRM PACKET.
 * If not AUTO, set the EOTF value to SETTING from LG
 * when LG set the V4L2_EXT_HDMI_OVERRIDE_EOTF_SMPTE_ST_2084 or
 * V4L2_EXT_HDMI_OVERRIDE_EOTF_HLG, BSP must set the version of drm info as 1.
 * LG checks if the version of drm info is 1 when the eotf is HDR or HLG.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_OVERRIDE_EOTF
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_hdmi_override_eotf {
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_SDR_LUMINANCE_RANGE = 0,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_HDR_LUMINANCE_RANGE,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_SMPTE_ST_2084,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_HLG,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_RESERVED_4,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_RESERVED_5,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_RESERVED_6,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_RESERVED_7,
 *         V4L2_EXT_HDMI_OVERRIDE_EOTF_AUTO,
 *     };
 *     struct v4l2_ext_hdmi_override_drm_info {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_override_eotf override_eotf;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set override EOTF value.
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_override_drm_info status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_override_drm_info));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     status.override_eotf = V4L2_CID_EXT_HDMI_OVERRIDE_HLG;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_OVERRIDE_EOTF;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_override_drm_info);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_OVERRIDE_EOTF (V4L2_CID_USER_EXT_HDMI_BASE + 16)

/**
 * @brief Get HDMI capabilities from HDMI Device
 *
 * @rst
 * This function get HDMI capabilities information about each port.
 * It is used to identify HDMI device compatible with this specification and to obtain information about driver and each HDMI port capabilities.
 * The ioctl takes a pointer to a struct which is filled by the driver.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS        // Get HDMI capabilities
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_QUERYCAP
 *
 *     //
 *     // parameter
 *     //
 *
 *     // HDMI Capabilities Flags
 *     //  The HDMI device supports HDCP1.4 Key. So, need to write HDCP1.4 key to driver.
 *     #define V4L2_EXT_HDMI_HDCP14                      0x0000000000000001  // (0x1 << 0)
 *     //  HDCP1.4 Key is OTP on Soc. So, No need to download HDCP1.4 Key to secure storage.
 *     #define V4L2_EXT_HDMI_HDCP14_KEY_OTP              0x0000000000000002  // (0x1 << 1)
 *
 *     //  The HDMI device supports HDCP2.3 key. So, need to write HDCP2.3 key to driver.
 *     //  HDCP2.2 key is include in HDCP2.3 key.
 *     #define V4L2_EXT_HDMI_HDCP23                      0x0000000000000004  // (0x1 << 2)
 *     //  HDCP2.3(HDCP2.2) Key is OTP on Soc. So, No need to download HDCP2.3(HDCP2.2) Key to secure storage.
 *     #define V4L2_EXT_HDMI_HDCP23_KEY_OTP              0x0000000000000008  // (0x1 << 3)
 *
 *     //  The HDMI device supports HDMI1.4 Specification
 *     #define V4L2_EXT_HDMI_HDMI14                      0x0000000000001000  // (0x1 << 12)
 *     //  The HDMI device supports HDMI2.0 Specification
 *     #define V4L2_EXT_HDMI_HDMI20                      0x0000000000002000  // (0x1 << 13)
 *     // The HDMI device supports HDMI2.1 Specification.
 *     // Set this flag even if the device supports one feature in the HDMI2.1 specification.
 *     #define V4L2_EXT_HDMI_HDMI21                      0x0000000000004000  // (0x1 << 14)
 *
 *     //  The HDMI device supports TMDS 3.4Gbps per Channel
 *     #define V4L2_EXT_HDMI_TMDS_3G                     0x0000000000100000  // (0x1 << 20)
 *     //  The HDMI device supports TMDS 6.0Gbps per Channel
 *     #define V4L2_EXT_HDMI_TMDS_6G                     0x0000000000200000  // (0x1 << 21)
 *     // The HDMI device supports FRL 3Gbps per Lane on 3 Lanes(0,1, and 2)
 *     #define V4L2_EXT_HDMI_FRL_3L_3G                   0x0000000000400000  // (0x1 << 22)
 *     // The HDMI device supports FRL 6Gbps per Lane on 3 Lanes(0,1, and 2)
 *     #define V4L2_EXT_HDMI_FRL_3L_6G                   0x0000000000800000  // (0x1 << 23)
 *     // The HDMI device supports FRL 6Gbps per Lane on 4 Lanes(0,1, 2, and 3)
 *     #define V4L2_EXT_HDMI_FRL_4L_6G                   0x0000000001000000  // (0x1 << 24)
 *     // The HDMI device supports FRL 8Gbps per Lane on 4 Lanes(0,1, 2, and 3)
 *     #define V4L2_EXT_HDMI_FRL_4L_8G                   0x0000000002000000  // (0x1 << 25)
 *     // The HDMI device supports FRL 10Gbps per Lane on 4 Lanes(0,1, 2, and 3)
 *     #define V4L2_EXT_HDMI_FRL_4L_10G                  0x0000000004000000  // (0x1 << 26)
 *     // The HDMI device supports FRL 12Gbps per Lane on 4 Lanes(0,1, 2, and 3)
 *     #define V4L2_EXT_HDMI_FRL_4L_12G                  0x0000000008000000  // (0x1 << 27)
 *
 *
 *     //  The HDMI device supports background timing detection.
 *     //  It means that driver can get HDMI signal information(timing, SPD, AVI, etc...) without watching HDMI port.
 *     //  If the HDMI device supports background timing detection, it also support fast switching between HDMI ports.
 *     #define V4L2_EXT_HDMI_BACKGROUND_TIMING_DETECT    0x0000000010000000  // (0x1 << 28)
 *
 *     //  The HDMI device supports FVA
 *     #define V4L2_EXT_HDMI_FVA                         0x0000000020000000  // (0x1 << 29)
 *     //  The HDMI device supports VRR
 *     #define V4L2_EXT_HDMI_VRR                         0x0000000040000000  // (0x1 << 30)
 *     // The HDMI device supports CinemaVRR
 *     #define V4L2_EXT_HDMI_CINEMAVRR                   0x0000000080000000  // (0x1 << 31)
 *
 *     //  The HDMI device supports  Dolby vision
 *     #define V4L2_EXT_HDMI_DOLBY_VISION                0x0000000100000000  // (0x1 << 32)
 *     // The HDMI device supports  Dolby vision low latency mode
 *     #define V4L2_EXT_HDMI_DOLBY_VISION_LL             0x0000000200000000  // (0x1 << 33)
 *     // The HDMI device supports  Dolby vision on HDMI2.1
 *     #define V4L2_EXT_HDMI_DOLBY_VISION_HDMI21         0x0000000400000000  // (0x1 << 34)
 *     //  The HDMI device supports 512bytes EDID
 *     #define V4L2_EXT_HDMI_512BYTE_EDID                0x0000000800000000  // (0x1 << 35)
 *     //  The HDMI device supports EMPACKET
 *     #define V4L2_EXT_HDMI_EMPACKET                    0x0000001000000000  // (0x1 << 36)
 *     //  The HDMI device supports HDCP Repeater
 *     #define V4L2_EXT_HDMI_HDCP_REPEATER               0x0000002000000000  // (0x1 << 37)
 *     //  The HDMI device supports DSC
 *     #define V4L2_EXT_HDMI_DSC                         0x0000004000000000  // (0x1 << 38)
 *
 *     struct v4l2_ext_hdmi_capability {
 *         uint8_t chip[16]; // chip name. for example "e60"
 *         uint32_t version; // linuxtv header version
 *         uint64_t capabilities; // HDMI Capabilities Flags
 *         uint64_t reserved[3];
 *     };
 *
 *     struct v4l2_ext_hdmi_querycap {
 *         enum v4l2_ext_hdmi_input_port port;
 *         struct v4l2_ext_hdmi_capability hdmi_capability;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // get HDMI capability
 *     v4l2_ext_controls ext_controls = {};
 *     v4l2_ext_control ext_control = {};
 *     struct v4l2_ext_hdmi_querycap hdmi_querycap = {};
 *
 *     hdmi_querycap.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_QUERYCAP;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_querycap);
 *     ext_controls.controls->ptr = (void *)&hdmi_querycap;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_QUERYCAP (V4L2_CID_USER_EXT_HDMI_BASE + 17)

/**
 * @brief Set HPD low Duration to each HDMI port for DC on case.
 *
 * @rst
 * Create a function to set the HPD low duration for DC on.
 * HPD low duration refers to the time from the HPD low to the HPD high when DC on.
 * The unit of HPD low duraition sent by LG is ms.
 * It should be possible to set for each port.
 * When DC on, BSP should store the existing values of EDID and HDCP without EDID, HDCP download API by LG.
 * When the set time is reached, HPD should be changed from low to high.
 * If the set has never been set by LG, the default value of 700ms is used.(value used in previous model)
 * The restart hpd time should not be changed. This should be fixed at 700ms.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HPD_LOW_DURATION_DC_ON
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_hdmi_hpd_low_duration_dc_on {
 *	       enum v4l2_ext_hdmi_input_port port;
 *	       int  hpd_low_duration;
 *     }
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set HPD low duration for DC ON case
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hpd_low_duration_dc_on status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(v4l2_ext_hdmi_hpd_low_duration_dc_on));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     status.hpd_low_duration = 200;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HPD_LOW_DURATION_DC_ON;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hpd_low_duration_dc_on);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &control);
 *
 *     // Get current HPD low duration info for DC ON case
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hpd_low_duration_dc_on hpd_low_duration
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&hpd_low_duration, 0, sizeof(struct v4l2_ext_hdmi_hpd_low_duration_dc_on));
 *
 *     hpd_low_duration.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HPD_LOW_DURATION_DC_ON;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hpd_low_duration_dc_on);
 *     ext_controls.controls->ptr = (void *)&hpd_low_duration;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 *
 * @endrst
 */

#define V4L2_CID_EXT_HDMI_HPD_LOW_DURATION_DC_ON                               \
    (V4L2_CID_USER_EXT_HDMI_BASE + 18)

/**
 * @brief Set Topology for multiple repeater device.
 *
 * @rst
 * For Miracast TX, the Topology of the Device connected to the TV is delivered to the BSP
 * This function actions are required from BSP to implement Miracast TX in webOS
 * and the function that passes the RX ID of the device connected to Miracast
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *    V4L2_CID_EXT_HDMI_HDCP_REPEATER_TOPOLOGY
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_hdmi_hdcp_repeater_topology {
 *         enum v4l2_ext_hdmi_input_port port;     // HDMI port number
 *         unsigned char repeater_mode;            // 0 : receiver, 1 : repeater
 *         unsigned char count;                    // Number of devices
 *         unsigned char depth;                    // Depth of connected device
 *         unsigned char receiver_id[32][5];       // RX IO of connected device
 *         unsigned char repeater_hpd;             // HPD control
 *         unsigned int msg_id;                     // Increase by 1 when it changes any value(init value =0, default set value = 1)
 *      };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set HDCP Repeater topology
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_repeater_topology topology;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&topology, 0, sizeof(struct v4l2_ext_hdmi_hdcp_repeater_topology));
 *
 *     topology.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HDCP_REPEATER_TOPOLOGY;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hdcp_repeater_topology);
 *     ext_controls.controls->ptr = (void *)&topology;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */

#define V4L2_CID_EXT_HDMI_HDCP_REPEATER_TOPOLOGY (V4L2_CID_USER_EXT_HDMI_BASE + 19)

/**
 * brief Get Stream Manage value from connected device
 *
 * @rst
 * Pass the Stream Manage value of the device connected to the TV to Miracast.
 * This function actions are required from BSP to implement Miracast TX in webOS
 * and the function  is the Stream Manage value of the device connected to the HDMI Port on TV.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HDCP_REPEATER_STREAM_MANAGE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_hdmi_hdcp_repeater_stream_manage {
 *          enum v4l2_ext_hdmi_input_port port;
 *          v4l2_ext_hdmi_hdcp_repeater_stream_manage_transmit value;     // 0 or 1 or reserved
 *     };
 *
 *     enum v4l2_ext_hdmi_hdcp_repeater_stream_manage_transmit{
 *          STREAM_MANAGE_TRANSMIT_ALLOW,
 *          STREAM_MANAGE_TRANSMIT_DENY,
 *          STREAM_MANAGE_RESERVED,
 *     };
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get stream manage type for repeater
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_repeater_stream_manage stream_manage;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&stream_manage, 0, sizeof(struct v4l2_ext_hdmi_repeater_stream_manage));
 *
 *     stream_manage.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_REPEATER_STREAM_MANAGE;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_repeater_stream_manage);
 *     ext_controls.controls->ptr = (void *)&stream_manage;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 *
 * @endrst
 */

#define V4L2_CID_EXT_HDMI_HDCP_REPEATER_STREAM_MANAGE (V4L2_CID_USER_EXT_HDMI_BASE + 20)

/**
 * @brief Set sleep mode for always ready function.
 *
 * @rst
 * New power state called always ready is added.
 * In this power state, one hdmi port will be running and other ports would be powered down as suspend mode.
 * webOS will let driver know which port should be running and which port should be powered down.
 * But the sleep cid may not be called during AC ON.
 * Running port :
 * - webOS would not check video timing because some SOC does not allow this.
 * - But make sure hdmi phy and link is running in this mode and hdcp should remain as auth state.
 * - When it goes back to active mode, there should be no hpd reset, reauth and retraining.
 * - It should work for both TMDS and FRL.
 * - when cable is plugged out and in again, it should receive signal again and start HDCP auth again.
 *
 * Sleeping port:
 * - Not working at all.
 * - Standby EDID should be supported in this mode, so HPD will remains as high.
 * - HPD will be reset when it goes to active mode.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_SLEEP
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_hdmi_sleep {
 *          enum v4l2_ext_hdmi_input_port port;
 *          enum v4l2_ext_hdmi_sleep_mode mode;
 *     };
 *
 *     enum v4l2_ext_hdmi_sleep_mode{
 *          V4L2_EXT_HDMI_SLEEP_MODE,
 *          V4L2_EXT_HDMI_WAKEUP_MODE,
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set sleep mode
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_sleep sleep_mode;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&sleep_mode, 0, sizeof(struct v4l2_ext_hdmi_sleep));
 *
 *     sleep_mode.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     sleep_mode.mode = V4L2_EXT_HDMI_SLEEP_MODE;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_SLEEP;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_sleep);
 *     ext_controls.controls->ptr = (void *)&sleep_mode;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */

#define V4L2_CID_EXT_HDMI_SLEEP (V4L2_CID_USER_EXT_HDMI_BASE + 21)

/* HDMI diagnostics class control IDs */
#define V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE (V4L2_CID_USER_EXT_HDMI_BASE + 0x100)

/**
 * @brief Get HDMI Diagnostics status
 *
 * @rst
 * This control returns various HDMI related status and information supported to display in HDMI diagnostics screen.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Diagnostics status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_DIAGNOSTICS_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_diagnostics_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         struct v4l2_ext_hdmi_link_status link_status;
 *         struct v4l2_ext_hdmi_phy_status phy_status;
 *         struct v4l2_ext_hdmi_video_status video_status;
 *         struct v4l2_ext_hdmi_audio_status audio_status;
 *         struct v4l2_ext_hdmi_hdcp_status hdcp_status;
 *         struct v4l2_ext_hdmi_scdc_status scdc_status;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI Diagnostics status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_diagnostics_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_diagnostics_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_DIAGNOSTICS_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_diagnostics_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_DIAGNOSTICS_STATUS                                   \
    (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 0)

/**
 * @brief Get HDMI PHY status
 *
 * @rst
 * This control returns HDMI PHY status. this information used for HDMI host diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI PHY status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_PHY_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_link_type
 *     {
 *         V4L2_EXT_HDMI_LINK_TYPE_TMDS = 0,
 *         V4L2_EXT_HDMI_LINK_TYPE_FRL,
 *     };
 *
 *     enum v4l2_ext_hdmi_link_lane_number
 *     {
 *         V4L2_EXT_HDMI_LINK_LANE_NUMBER_3 = 3,
 *         V4L2_EXT_HDMI_LINK_LANE_NUMBER_4 = 4,
 *     };
 *
 *     enum v4l2_ext_hdmi_link_rate
 *     {
 *         V4L2_EXT_HDMI_LINK_RATE_6G  = 6,
 *         V4L2_EXT_HDMI_LINK_RATE_8G  = 8,
 *         V4L2_EXT_HDMI_LINK_RATE_10G = 10,
 *         V4L2_EXT_HDMI_LINK_RATE_12G = 12,
 *     };
 *
 *     struct v4l2_ext_hdmi_phy_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char lock_status;
 *         unsigned int tmds_clk_khz;
 *
 *         enum v4l2_ext_hdmi_link_type link_type;
 *         enum v4l2_ext_hdmi_link_lane_number link_lane;
 *         enum v4l2_ext_hdmi_link_rate link_rate;
 *
 *         unsigned int ctle_eq_min_range[V4L2_EXT_HDMI_TMDS_CH_NUM];
 *         unsigned int ctle_eq_max_range[V4L2_EXT_HDMI_TMDS_CH_NUM];
 *         unsigned int ctle_eq_result[V4L2_EXT_HDMI_TMDS_CH_NUM];
 *         unsigned int error[V4L2_EXT_HDMI_TMDS_CH_NUM];
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI PHY status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_phy_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_phy_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_PHY_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_phy_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_PHY_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 1)

/**
 * @brief Get HDMI Link status
 *
 * @rst
 * This control returns HDMI Link status. this information used for HDMI host diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Link status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_LINK_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_audio_format
 *     {
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_UNKNOWN      = 0x00,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_PCM          = 0x01,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_AC3          = 0x10,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_EAC3         = 0x11,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_EAC3_ATMOS   = 0x12,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_MAT          = 0x15,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_MAT_ATMOS    = 0x16,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_TRUEHD       = 0x17,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_TRUEHD_ATMOS = 0x18,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_AAC          = 0x19,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_MPEG         = 0x20,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_DTS          = 0x30,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_DTS_HD_MA    = 0x31,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_DTS_EXPRESS  = 0x32,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_DTS_CD       = 0x33,
 *         V4L2_EXT_HDMI_AUDIO_FORMAT_NOAUDIO      = 0x41,
 *     };
 *
 *     struct v4l2_ext_hdmi_link_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char hpd;
 *         unsigned char hdmi_5v;
 *         unsigned char rx_sense;
 *         unsigned int frame_rate_x100_hz;
 *         enum v4l2_ext_hdmi_mode dvi_hdmi_mode;
 *
 *         unsigned short video_width;
 *         unsigned short video_height;
 *         enum v4l2_ext_hdmi_avi_csc color_space;
 *         unsigned char color_depth;
 *         enum v4l2_ext_hdmi_avi_colorimetry colorimetry;
 *         enum v4l2_ext_hdmi_avi_ext_colorimetry ext_colorimetry;
 *         enum v4l2_ext_hdmi_avi_additional_colorimetry additional_colorimetry;
 *         enum v4l2_ext_hdr_mode hdr_type;
 *
 *         enum v4l2_ext_hdmi_audio_format audio_format;
 *         unsigned int audio_sampling_freq;
 *         unsigned char audio_channel_number;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI Link status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_link_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_link_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_LINK_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_link_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_LINK_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 2)

/**
 * @brief Get HDMI Video status
 *
 * @rst
 * This control returns HDMI Video status. this information used for HDMI host diagnostics.
 * Pixel_clock_khz is TMDS character rate.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Video status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_VIDEO_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_video_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned short video_width_real;
 *         unsigned short video_htotal_real;
 *         unsigned short video_height_real;
 *         unsigned short video_vtotal_real;
 *         unsigned int pixel_clock_khz;
 *         unsigned int current_vrr_refresh_rate;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI Video status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_video_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_video_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_VIDEO_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_video_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_VIDEO_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 3)

/**
 * @brief Get HDMI Audio status
 *
 * @rst
 * This control returns HDMI Audio status. this information used for HDMI host diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI Audio status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_AUDIO_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_audio_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned int pcm_N;
 *         unsigned int pcm_CTS;
 *         unsigned char LayoutBitValue;
 *         unsigned char ChannelStatusBits;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI Audio status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_audio_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_audio_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_AUDIO_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_audio_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_AUDIO_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 4)

/**
 * @brief Get HDMI HDCP status
 *
 * @rst
 * This control returns HDMI HDCP status. this information used for HDMI host diagnostics.
 * If HDCP status is not authenticated or off, hdcp_version should be set to V4L2_EXT_HDMI_HDCP_VERSION_RESERVED.
 *
 * HDCP behavior on disabled ports should not affect Active Port and other ports.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI HDCP status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_HDCP_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_hdcp_auth_status
 *     {
 *         V4L2_EXT_HDMI_HDCP_AUTH_STATUS_NO_TX_CONNECTED = 0,
 *         V4L2_EXT_HDMI_HDCP_AUTH_STATUS_UNAUTHENTICATED = 1,
 *         V4L2_EXT_HDMI_HDCP_AUTH_STATUS_IN_PROGRESS     = 2,
 *         V4L2_EXT_HDMI_HDCP_AUTH_STATUS_AUTHENTICATED   = 3,
 *     };
 *
 *     struct v4l2_ext_hdmi_hdcp14_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char An[8];
 *         unsigned char Aksv[5];
 *         unsigned char Bksv[5];
 *         unsigned char Ri[2];
 *         unsigned char Bcaps;
 *         unsigned char Bstatus[2];
 *     };
 *
 *     struct v4l2_ext_hdmi_hdcp22_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned short ake_init_count_since_5v;
 *         unsigned short reauth_req_count_since_5v;
 *     };
 *
 *     struct v4l2_ext_hdmi_hdcp_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_hdcp_version hdcp_version;
 *         enum v4l2_ext_hdmi_hdcp_auth_status auth_status;
 *         unsigned char encEn;
 *         struct v4l2_ext_hdmi_hdcp14_status hdcp14_status;
 *         struct v4l2_ext_hdmi_hdcp22_status hdcp22_status;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI HDCP status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_hdcp_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_hdcp_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_HDCP_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_hdcp_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_HDCP_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 5)

/**
 * @brief Get HDMI SCDC status
 *
 * @rst
 * This control returns HDMI SCDC status. this information used for HDMI host diagnostics.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI SCDC status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_SCDC_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_hdmi_scdc_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         unsigned char source_version;
 *         unsigned char sink_version;
 *
 *         unsigned char rsed_update;
 *         unsigned char flt_update;
 *         unsigned char frl_start;
 *         unsigned char source_test_update;
 *         unsigned char rr_test;
 *         unsigned char ced_update;
 *         unsigned char status_update;
 *
 *         unsigned char tmds_bit_clock_ratio;
 *         unsigned char scrambling_enable;
 *         unsigned char tmds_scrambler_status;
 *
 *         unsigned char flt_no_retrain;
 *         unsigned char rr_enable;
 *         unsigned char ffe_levels;
 *         unsigned char frl_rate;
 *
 *         unsigned char dsc_decode_fail;
 *         unsigned char flt_ready;
 *         unsigned char clk_detect;
 *         unsigned char ch0_locked;
 *         unsigned char ch1_locked;
 *         unsigned char ch2_locked;
 *         unsigned char ch3_locked;
 *
 *         unsigned char lane0_ltp_request;
 *         unsigned char lane1_ltp_request;
 *         unsigned char lane2_ltp_request;
 *         unsigned char lane3_ltp_request;
 *
 *         unsigned char ch0_ced_valid;
 *         unsigned char ch1_ced_valid;
 *         unsigned char ch2_ced_valid;
 *         unsigned char ch3_ced_valid;
 *         unsigned int ch0_ced;
 *         unsigned int ch1_ced;
 *         unsigned int ch2_ced;
 *         unsigned int ch3_ced;
 *         unsigned char rs_correction_valid;
 *         unsigned int rs_correcton_count;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI SCDC status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_scdc_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_scdc_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_SCDC_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_scdc_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_SCDC_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 6)

/**
 * @brief Get HDMI ERROR status
 *
 * @rst
 * This function returns HDMI error from the currently selected Port. This information used for HDMI host diagnostics.
 * This function is called at regular intervals. Maintain if an error occurs before a function is called.
 * Clear the error after returning the function when it is called.
 * For example, If a GCP error and a TMDS error occur simultaneously, the value is returned as follows : 0x00000005
 * If a HDCP 2.2 Reauth error and a Audio Buffer error occur simultaneously, the value is returned as follows : 0x00000042
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get HDMI ERROR status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_ERROR_STATUS
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_error_type
 *     {
 *         V4L2_EXT_HDMI_ERROR_TYPE_NONE          = 0x0000,
 *         V4L2_EXT_HDMI_ERROR_TYPE_GCP_ERROR     = 0x0001,
 *         V4L2_EXT_HDMI_ERROR_TYPE_HDCP22_REAUTH = 0x0002,
 *         V4L2_EXT_HDMI_ERROR_TYPE_TMDS_ERROR    = 0x0004,
 *         V4L2_EXT_HDMI_ERROR_TYPE_PHY_LOW_RANGE = 0x0008,
 *         V4L2_EXT_HDMI_ERROR_TYPE_PHY_ABNORMAL  = 0x0010,
 *         V4L2_EXT_HDMI_ERROR_TYPE_CED_ERROR     = 0x0020,
 *         V4L2_EXT_HDMI_ERROR_TYPE_AUDIO_BUFFER  = 0x0040,
 *         V4L2_EXT_HDMI_ERROR_TYPE_UNSTABLE_SYNC = 0x0080,
 *         V4L2_EXT_HDMI_ERROR_TYPE_BCH           = 0x0100,
 *         V4L2_EXT_HDMI_ERROR_TYPE_FLT           = 0x0200,
 *         V4L2_EXT_HDMI_ERROR_TYPE_FAILED        = 0xFFFFFFFE,
 *     };
 *
 *     struct v4l2_ext_hdmi_error_status
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_error_type error;
 *         unsigned int param1;
 *         unsigned int param2;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get HDMI ERROR status
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_error_status status;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&status, 0, sizeof(struct v4l2_ext_hdmi_error_status));
 *
 *     status.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_ERROR_STATUS;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_error_status);
 *     ext_controls.controls->ptr = (void *)&status;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_ERROR_STATUS (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 7)

/**
 * @brief Set HDMI Expert setting
 *
 * @rst
 * This function is used to get the Dolby Vision content type metadata.
 * If this function is not called, BSP must operate at its default value.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set HDMI Expert setting
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_HDMI_EXPERT_SETTING
 *
 *     //
 *     // parameter
 *     //
 *
 *     enum v4l2_ext_hdmi_input_port
 *     {
 *         V4L2_EXT_HDMI_INPUT_PORT_NONE = 0,
 *         V4L2_EXT_HDMI_INPUT_PORT_1,
 *         V4L2_EXT_HDMI_INPUT_PORT_2,
 *         V4L2_EXT_HDMI_INPUT_PORT_3,
 *         V4L2_EXT_HDMI_INPUT_PORT_4,
 *         V4L2_EXT_HDMI_INPUT_PORT_5,
 *         V4L2_EXT_HDMI_INPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_hdmi_expert_setting_type
 *     {
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_HPD_LOW_DURATION = 0,
 *
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_MODE,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_CH0,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_CH1,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_CH2,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_CH3,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_EQ_PERIOD,
 *
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_VIDEO_STABLE_COUNT,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_AUDIO_STABLE_COUNT,
 *
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_DISABLE_HDCP22,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_REAUTH_HDCP22,
 *
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_ON_TO_RXSENSE_TIME,
 *         V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_RXSENSE_TO_HPD_TIME
 *     };
 *
 *     struct v4l2_ext_hdmi_expert_setting
 *     {
 *         enum v4l2_ext_hdmi_input_port port;
 *         enum v4l2_ext_hdmi_expert_setting_type type;
 *         unsigned int param1;
 *         unsigned int param2; // (for future)
 *         unsigned int param3; // (for future)
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_S_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set HDMI Expert Setting
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_hdmi_expert_setting setting;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&setting, 0, sizeof(struct v4l2_ext_hdmi_expert_setting));
 *
 *     setting.port = V4L2_EXT_HDMI_INPUT_PORT_1;
 *     setting.type = V4L2_EXT_HDMI_EXPERT_SETTING_TYPE_TMDS_MANUAL_EQ_CH0;
 *     setting.param1 = 7;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_HDMI_EXPERT_SETTING;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_hdmi_expert_setting);
 *     ext_controls.controls->ptr = (void *)&setting;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_HDMI_EXPERT_SETTING                                       \
    (V4L2_CID_EXT_HDMI_DIAGNOSTICS_BASE + 8)

//
// ADC class control IDs
//

/**
 * @brief Get ADC timing info
 *
 * @rst
 * This function reads signal timing information such as H / V frequency,
 * total H / V length of signal (Sync width + active) and H / V length in the active
 * period of ADC block. If the ADC is in the open / connect state and there is
 * no signal change, the decoding should continue and the timing information should be
 * maintained even if the VSC (main / sub scaler) is connected / disconnected.
 * Timing information should be reset to all '0' at ADC disconnect and close.
 * Do not arbitrarily pass timing information to '0' for unsupported specs or specific timing.
 * V frequency must be multiplied by 10 and passed as Integer. When a signal is physically
 * changed (no signal-> good signal), timing information should not be partially transmitted
 * several times, and all timing information must be transmitted at a time.
 * The VIDEO_RECT_T information shall be transmitted as follows. for example, 720 * 480 i 59.94 Hz signal.
 *
 * 본 함수는 ADC block 의 active period 에서 H/V Frequency, total H/V length of signal(Sync width + active),
 * H/V length 등의 신호 Timing 정보를 읽어온다. ADC 가 open/connect 유지 상태이고, 신호 변경이 없으면
 * VSC(mail/sub scaler)가 connect/disconnect 되더라도 Decoding 은 지속되어야 하고, Timing 정보도 유지되어야 한다.
 * ADC disconnect, close 시 timing 정보는 all '0' 으로 reset 되어야 한다. 지원하지 않는 스펙이나 특정 timing에
 * 대해서 timing 정보를 임의로 모두 '0' 으로 전달하면 안된다. V frequency 는 10을 곱해서 Integer 형태로 전달해야 한다.
 * 물리적으로 신호 변경 시(no signal->good signal) timing 정보가 부분적으로 수회에 전달되면 안되고,
 * 모든 timing 정보가 한번에 전달되어야 한다. VIDEO_RECT_T 정보는 아래와 같이 전달 되어야 한다.
 * 예) 720*480 i 59.94Hz signal.
 *
 * - some driver use double sampling so result of ADC may be 1440*480 or 720*240 because it's a interlace signal.
 * - but at this case this function should return like this x=0, y=0, w=720, h=480, vFreq=599, scanType=0.
 * - also all of input/output window setting will be done base on this values.
 *
 * Basically, it should support the following VESA standard format and Timing information should be transmitted correctly.
 * Formats that are included in VESA but are not listed in the table below are not mandatory, but must be supported.
 *
 * .. image:: /v4l2/adc-V4L2_CID_EXT_ADC_TIMING_INFO-1.png
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get ADC Timinginfo
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_TIMING_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;   // horizontal start position
 *         unsigned short y;   // vertical start position
 *         unsigned short w;   // horizontal size(width)
 *         unsigned short h;   // vertical size(height)
 *     };
 *
 *     struct v4l2_ext_adc_timing_info
 *     {
 *         unsigned short h_freq;
 *         unsigned short v_freq;
 *         unsigned short h_total;
 *         unsigned short v_total;
 *         unsigned short h_porch;
 *         unsigned short v_porch;
 *         struct v4l2_ext_video_rect active;
 *         unsigned short scan_type;
 *         unsigned short phase;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Video Timing Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_adc_timing_info adc_timing_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&adc_timing_info, 0, sizeof(struct v4l2_ext_adc_timing_info));
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_ADC_TIMING_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_adc_timing_info);
 *     ext_controls.controls->ptr = (void *)&adc_timing_info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_TIMING_INFO (V4L2_CID_USER_EXT_ADC_BASE + 0)

/**
 * @brief Get ADC OTP Data
 *
 * @rst
 * This function is called once at booting to obtain ADC calibration data stored in the OTP area.
 * The chip-vendor must complete the ADC calibration before the chip is released.
 * All input OTP data (YCbCr and SCART-RGB (DVB system)) should be transmitted at one time.
 *
 * 본 함수는 booting 시 1회 호출되어 OTP 영역에 저장된 ADC Calibration data를 얻어온다.
 * chip-vendor 는 chip 이 release 되기 전에 ADC Calibration 을 완료해야 한다.
 * 모든 입력의 OTP data(YCbCr and SCART-RGB( DVB system)) 는  한번에 전달되어야 한다.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get ADC OTP Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_OTP_DATA
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_adc_calibration_data
 *     {
 *         unsigned short r_gain;
 *         unsigned short g_gain;
 *         unsigned short b_gain;
 *         unsigned short r_offset;
 *         unsigned short g_offset;
 *         unsigned short b_offset;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get ADC Calibration Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_adc_calibration_data otp_data;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&otp_data, 0, sizeof(struct v4l2_ext_adc_calibration_data));
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_ADC_OTP_DATA;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_adc_calibration_data);
 *     ext_controls.controls->ptr = (void *)&otp_data;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_OTP_DATA (V4L2_CID_USER_EXT_ADC_BASE + 1)

/**
 * @brief Set/Get ADC Calibration Data
 *
 * @rst
 * - Set: This function sets the ADC calibration data (gain / offset) in the ADC register.
 *   When booting, when input is switched to Component or Scart-RGB, ADC calibration data is read
 *   from EEPROM and set through this function.
 * - Get: This function obtains the ADC calibration data (gain / offset) from the register after external
 *   or internal calibration. The obtained data is written to the EEPROM.
 *
 * - Set : 본 함수는 ADC Calibration data(gain/offset)를 ADC register 에 setting 한다.
 *   booting 시, Component 혹은 Scart-RGB 로 입력전환 될 때 ADC Calibration data 를 EEPROM 으로부터
 *   읽어와 본 함수를 통해 setting 한다.
 * - Get : 본 함수는External 혹은 Internal Calibration 이후 register 로부터 ADC Calibration data(gain/offset)을 얻는다.
 *   얻어온 data는 EEPROM 에 write 된다.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS        // Set ADC Calibration Data
 *     VIDIOC_G_EXT_CTRLS        // Get ADC Calibration Data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_CALIBRATION_DATA
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_adc_calibration_data
 *     {
 *         unsigned short r_gain;
 *         unsigned short g_gain;
 *         unsigned short b_gain;
 *         unsigned short r_offset;
 *         unsigned short g_offset;
 *         unsigned short b_offset;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_EXT_CTRLS and this control id.
 *
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set ADC Calibration Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_adc_calibration_data cal_data;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&cal_data, 0, sizeof(struct v4l2_ext_adc_calibration_data));
 *
 *     cal_data.rGain  = 192;
 *     cal_data.gGain  = 192;
 *     cal_data.bGain  = 192;
 *     cal_data.rOffset= 128;
 *     cal_data.gOffset= 128;
 *     cal_data.bOffset= 128;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_ADC_CALIBRATION_DATA;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_adc_calibration_data);
 *     ext_controls.controls->ptr = (void *)&cal_data;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *     // Get ADC Calibration Data
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_adc_calibration_data cal_data;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&cal_data, 0, sizeof(struct v4l2_ext_adc_calibration_data));
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_ADC_CALIBRATION_DATA;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_adc_calibration_data);
 *     ext_controls.controls->ptr = (void *)&cal_data;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_CALIBRATION_DATA (V4L2_CID_USER_EXT_ADC_BASE + 2)

/**
 * @brief Set/Get ADC Calibration Type
 *
 * @rst
 * This function performs an analog-to-digitial calibration.
 * External ADC: calibration (MSPG, time: 225 (1920 * 1080 60Hz), pattern: 65)
 * performed by the external pattern input by the instrument.
 * Internal ADC: Calibration performed by the pattern supported in the driver.
 * OTP ADC: The chip-set itself is calibrated and the data is stored in the OTP area.
 *
 * 본 함수는 analog-to-digitial calibration 을 한다.
 * External ADC: 장비에 의해 입력되는 외부 pattern 에 의해 수행되는 calibration (MSPG, time:225(1920*1080 60Hz), pattern:65)
 * Internal ADC: driver 내부에서 지원되는 pattern 에 의해 수행되는 calibration.
 * OTP ADC: chip-set 자체적으로 calibration 을 해서 data 를 OTP 영역에에 저장.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set ADC Calibration Type
 *     VIDIOC_G_CTRL        // Get ADC Calibration Type
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_CALIBRATION_TYPE
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_adc_calibration_type
 *     {
 *         V4L2_EXT_ADC_CALIBRATION_TYPE_OTP = 0,
 *         V4L2_EXT_ADC_CALIBRATION_TYPE_EXTERNAL,
 *         V4L2_EXT_ADC_CALIBRATION_TYPE_INTERNAL,
 *         V4L2_EXT_ADC_CALIBRATION_TYPE_USER,
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set ADC Calibration Type
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_ADC_CALIBRATION_TYPE;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get ADC Calibration Type
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_ADC_CALIBRATION_TYPE;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_CALIBRATION_TYPE (V4L2_CID_USER_EXT_ADC_BASE + 3)
enum v4l2_ext_adc_calibration_type {
    V4L2_EXT_ADC_CALIBRATION_TYPE_OTP = 0,
    V4L2_EXT_ADC_CALIBRATION_TYPE_EXTERNAL,
    V4L2_EXT_ADC_CALIBRATION_TYPE_INTERNAL,
    V4L2_EXT_ADC_CALIBRATION_TYPE_USER,
};

/**
 * @brief Set ADC reset Calibration
 *
 * @rst
 * This function resets the value of ADC Calibration data register to default value.
 *
 * 본 함수는 ADC Calibration data register 의 값을 default 값으로 reset 한다.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set ADC Calibration Type
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_RESET_CALIBRATION
 *
 *     //
 *     // control value
 *     //
 *     0 (Ignore), 1 (Reset)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *   This control id supports VIDIOC_S_CTRL only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Reset Calibration Data
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_ADC_RESET_CALIBRATION;
 *     control.value = 1;      // Reset
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_RESET_CALIBRATION (V4L2_CID_USER_EXT_ADC_BASE + 4)

/**
 * @brief Set/Get ADC fast switch
 *
 * @rst
 * This function enables Fast-InputChange to Component,
 * This function is called when test mode in Factory Line.
 * In Factory Line, for improving productivity, Input Change speed must be fast.
 * For High productivity, we call V4L2_CID_EXT_ADC_FAST_SWITCH as on.
 * After testing in FactoryMode,  we call V4L2_CID_EXT_ADC_FAST_SWITCH as off.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set ADC fast switch
 *     VIDIOC_G_CTRL        // Get ADC fast switch
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_ADC_FAST_SWITCH
 *
 *     //
 *     // control value
 *     //
 *     0 (Off), 1 (On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Fast Switch Mode
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_ADC_FAST_SWITCH;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get Fast Switch Mode
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_ADC_FAST_SWITCH;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_ADC_FAST_SWITCH (V4L2_CID_USER_EXT_ADC_BASE + 5)

//
// AVD class control IDs
//

/**
 * @brief Set/Get AVD Port
 *
 * @rst
 * Select the HW port that is actually connected when AVD Connect. It is an item that changes depending
 * on the circuit configuration and should be called before VIDIOC_S_INPUT.
 * This value is related with "Ext. Input Adjust" Video Index in the Adjust menu.
 *
 * AVD Connect 시 실제 연결되는 HW port 를 선택한다. 회로 구성에 따라 바뀌는 항목이고 VIDIOC_S_INPUT 전에 호출되어야 한다.
 * Adjust menu 의 "Ext. Input Adjust"  Video Index 와 연동되는 값이다.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set connected AVD port number
 *     VIDIOC_G_CTRL        // Get connected AVD port number
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AVD_PORT
 *
 *     //
 *     // control value
 *     //
 *     0 ~ 255
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Input (connect)
 *     int input = V4L2_EXT_AVD_INPUT_SRC_AV;
 *     ioctl(fd, VIDIOC_S_INPUT, &input);
 *
 *     // Set port number
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_PORT;
 *     control.value = 2;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get port number
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_PORT;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AVD_PORT (V4L2_CID_USER_EXT_AVD_BASE + 0)

/**
 * @brief Get AVD timing info
 *
 * @rst
 *
 * This function carries the information of the input signal of the current Analog Video Decoder.
 * If AVD is in the open / connect state and there is no signal change, decoding should continue
 * and Timing information should be maintained even if VSC (main / sub scaler) is connected / disconnected.
 * When the signal is physically changed (no signal-> good signal), all timing information is stabilized
 * and updated at once. When input is switched to analog TV, active.w and active.h should be fixed
 * value as follows, The other hFreq, vFreq, hPorch, and vProch must deliver the physical values detected
 * in the real HW.
 *
 * 이 함수는 현재 Analog Video Decoder의 입력 신호의 정보를 전달한다. AVD 가 open/connect 유지 상태이고,
 * 신호 변경이 없으면 VSC(main/sub scaler)가 connect/disconnect 되더라도 Decoding 은 지속되어야 하고,
 * Timing 정보도 유지되어야 한다. 신호가 물리적으로 변경 시(no signal->good signal) 모든 timing 정보가
 * 안정화 된 후 한번에 update 한다.  Analog TV 로 입력 전환 되었을 때, active.w 와 active.h 는 다음과 같이
 * 고정된 값이 전달되어야 하고, 그 외의 hFreq, vFreq, hPorch, vProch 는 실제 HW 에서 detection 하는
 * 물리적인 값을 전달해야 한다.
 *
 * ======= ======== ========= ========== ========
 * ..      active.x active.y  active.w   active.h
 * ======= ======== ========= ========== ========
 * NTSC    0        0         704        480
 * PAL     0        0         704        576
 * ======= ======== ========= ========== ========
 *
 * This function is related to the color system setting of the VIDIOC_S_STD function.
 * When the NTSC system is set up with the above function,if 576i-PAL-Signal is applied
 * to the analog TV input, 480i should be returned, If the PAL series is set, if 480i-NTSC-signal is applied
 * to the analog TV input, 576i should be returned. If analog TV input of TV model supporting PAL 576i
 * and NTSC 480i (PAL-M, PAL-N, NTSC) is white noise (no signal), active.w and active.h transmit
 * the last valid signal information, If there is no valid signal information when the TV is turned on
 * in white noise (no signal) state, active.w should transmit 704 and active.h should transmit 480.
 * When AVD disconnect and close, all timing information should be reset to '0'.
 *
 * 본 함수는 VIDIOC_S_STD 함수의 color system setting 과 관련이 있다. 위의 함수로 NTSC 계열이 setting 되었는데
 * Analog TV 입력에 576i-PAL-Signal 이 인가되면, 480i 가 반환 되어야 하고, PAL 계열이 setting 되면 Analog TV 입력에
 * 480i-NTSC-Signal 이 인가되면, 576i 가 반환 되어야 한다. PAL 576i와 NTSC 480i (PAL-M, PAL-N, NTSC) 지원 TV model 의
 * Analog TV 입력이 white noise (no signal) 상태이면, active.w 와 active.h 는 마지막 valid 신호 정보를 전달 하고,
 * 만약 white noise (no signal) 상태에서 TV 전원이 켜져서 valid 신호 정보가 없으면, active.w 는 704, active.h는 480을
 * 전달해야 한다. AVD disconnect, close 시 timing 정보는 모두 '0' 으로 reset 되어야 한다.
 *
 * Use Case
 * In the Video MW Periodic Task, use V4L2_CID_EXT_AVD_TIMING_INFO to get Timing information in 40ms cycle.
 * Video MW Periodic Task에서 40ms 주기로 V4L2_CID_EXT_AVD_TIMING_INFO 를 이용하여 Timing 정보를 얻는다.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get AVD Timinginfo
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AVD_TIMING_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x;   // horizontal start position
 *         unsigned short y;   // vertical start position
 *         unsigned short w;   // horizontal size(width)
 *         unsigned short h;   // vertical size(height)
 *     };
 *
 *     struct v4l2_ext_avd_timing_info
 *     {
 *         unsigned short h_freq;
 *         unsigned short v_freq;
 *         unsigned short h_porch;
 *         unsigned short v_porch;
 *         struct v4l2_ext_video_rect active;
 *         unsigned char vd_lock;
 *         unsigned char h_lock;
 *         unsigned char v_lock;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get Video Timing Info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_avd_timing_info avd_timing_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&avd_timing_info, 0, sizeof(struct v4l2_ext_avd_timing_info));
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_AVD_TIMING_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_avd_timing_info);
 *     ext_controls.controls->ptr = (void *)&avd_timing_info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AVD_TIMING_INFO (V4L2_CID_USER_EXT_AVD_BASE + 1)

/**
 * @brief Get Sync Status
 *
 * @rst
 * This function says whether Sync exists or Not.
 *
 * Use Case
 *
 * .. image:: /v4l2/avd-V4L2_CID_EXT_AVD_VIDEO_SYNC-1.png
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL       // Get Sync Status
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AVD_VIDEO_SYNC
 *
 *     //
 *     // control value
 *     //
 *     0 (AVD Sync Not Locked), 1 (AVD Sync Locked)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Check Video Sync
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_VIDEO_SYNC;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AVD_VIDEO_SYNC (V4L2_CID_USER_EXT_AVD_BASE + 2)

/**
 * @brief Set/Get avd auto tuning mode
 *
 * @rst
 * This function controls the sensitivity of V4L2_CID_EXT_AVD_VIDEO_SYNC.
 * If V4L2_CID_EXT_AVD_AUTO_TUNING_MODE is necessary it used for AutoTuning in FE-Tuner Side
 * because avoid tuning garbage channel.
 * This function is called with "True" when ATV-Auto-Channel-Searching is performed.
 * This function controls the sensitivity of detecting Analog-Video-Decoder Sync-Lock.
 * When V4L2_CID_EXT_AVD_AUTO_TUNING_MODE is on, the sensitivity of Sync-detection becomes lower than
 * before so that garbage channel may not be searched.
 *
 * During ATV-Auto-Channel-Searching, V4L2_CID_EXT_AVD_AUTO_TUNING_MODE is on.(if it necessary)
 * During Normal-User-Using condition, V4L2_CID_EXT_AVD_AUTO_TUNING_MODE is off.
 * This function is only for ATV, so it should not affect Composite and Scart.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set avd auto tuning mode
 *     VIDIOC_G_CTRL        // Get avd auto tuning mode
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AVD_AUTO_TUNING_MODE
 *
 *     //
 *     // control value
 *     //
 *     0 (Off), 1 (On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Auto Tuning Mode
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_AUTO_TUNING_MODE;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Get Auto Tuning Mode
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_AUTO_TUNING_MODE;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AVD_AUTO_TUNING_MODE (V4L2_CID_USER_EXT_AVD_BASE + 3)

/**
 * @brief Set/Get avd channel change
 *
 * @rst
 * This function lets AVD driver know channel change. (Replace HAL_VFE_AVD_SetChannelChange())
 * Call this function when do change channel for platforms which need to know channel change timing.
 *
 * This function is not necessary. Only some SOC need this to enhance channel change speed.
 * If driver does not need this api, do not implement this and just return error -1 and the errno EINVAL.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL        // Set avd channel change
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_AVD_CHANNEL_CHANGE
 *
 *     //
 *     // control value
 *     //
 *     // Parameter is not necessary, but set default value as "1" for consistency.
 *     1 (On)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *   If driver does not support this, just return EINVAL.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set Channel Change
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_AVD_CHANNEL_CHANGE;
 *     control.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_AVD_CHANNEL_CHANGE (V4L2_CID_USER_EXT_AVD_BASE + 4)

//
// VBI class control IDs
//

/**
 * @brief Get VBI Copy Protection info
 *
 * @rst
 * This function returns the copy protection informaion of current analog video
 * signal.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get VBI Copy Protection info
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VBI_COPY_PROTECTION_INFO
 *
 *     //
 *     // parameter
 *     //
 *
 *     // CGMS copy protection enum
 *     enum v4l2_ext_vbi_cgms
 *     {
 *         V4L2_EXT_VBI_CGMS_PERMIT = 0,
 *         V4L2_EXT_VBI_CGMS_ONCE,
 *         V4L2_EXT_VBI_CGMS_RESERVED,
 *         V4L2_EXT_VBI_CGMS_NO_PERMIT,
 *     };
 *
 *     // APS copy protection enum
 *     enum v4l2_ext_vbi_aps
 *     {
 *         V4L2_EXT_VBI_APS_OFF = 0,
 *         V4L2_EXT_VBI_APS_ON_BURST_OFF,
 *         V4L2_EXT_VBI_APS_ON_BURST_2,
 *         V4L2_EXT_VBI_APS_ON_BURST_4,
 *     };
 *
 *     // MACROVISION copy protection enum
 *     enum v4l2_ext_vbi_macrovision
 *     {
 *         V4L2_EXT_VBI_MACROVISION_PSP_OFF = 0,
 *         V4L2_EXT_VBI_MACROVISION_PSP_ON_BURST_OFF,
 *         V4L2_EXT_VBI_MACROVISION_PSP_ON_BURST_2,
 *         V4L2_EXT_VBI_MACROVISION_PSP_ON_BURST_4,
 *     };
 *
 *
 *     struct v4l2_ext_vbi_copy_protection
 *     {
 *         enum v4l2_ext_vbi_cgms        cgms_cp_info;
 *         enum v4l2_ext_vbi_aps         aps_cp_info;
 *         enum v4l2_ext_vbi_macrovision macrovision_cp_info;
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   This control id supports VIDIOC_G_EXT_CTRLS only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Get VBI Copy Protection info
 *     v4l2_ext_controls ext_controls;
 *     v4l2_ext_control ext_control;
 *     struct v4l2_ext_vbi_copy_protection copy_protention_info;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&copy_protention_info, 0, sizeof(struct v4l2_ext_vbi_copy_protection));
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_VBI_COPY_PROTECTION_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_vbi_copy_protection);
 *     ext_controls.controls->ptr = (void *)&copy_protention_info;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBI_COPY_PROTECTION_INFO (V4L2_CID_USER_EXT_VBI_BASE + 0)

/**
 * @brief Flush VBI data
 *
 * @rst
 * This function erases all VBI data which remain inside the VBI Buffer. The
 * VBI-Buffer includes the driver-side-VBI-Buffer. Driver empty buffer for given
 * V4L2 VBI Services by control.value. For example, if control.value is
 * V4L2_SLICED_VBI_625, erase Teletext and WSS data. If control.value is
 * V4L2_SLICED_CAPTION_525, erase Closed caption data.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL      // Flush VBI data
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VBI_FLUSH
 *
 *     //
 *     // control value
 *     //
 *
 *     // Sliced VBI services from V4L2 standard
 *     // Please see https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/dev-sliced-vbi.html#sliced-vbi-services
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *   This control id supports VIDIOC_S_CTRL only.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Flush Closed Caption vbi data
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VBI_FLUSH;
 *     control.value = V4L2_SLICED_CAPTION_525;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 *     // Flush PAL TTX and WSS vbi data
 *     v4l2_control control;
 *
 *     control.id = V4L2_CID_EXT_VBI_FLUSH;
 *     control.value = V4L2_SLICED_VBI_625;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VBI_FLUSH (V4L2_CID_USER_EXT_VBI_BASE + 1)

/* VDEC class control IDs */
#define V4L2_CID_EXT_VDEC_BASE (V4L2_CID_USER_EXT_VDEC_BASE + 0x0)

/**
 * @brief Set the VDEC Port where FD will be used
 *
 * @rst
 * It is a control ID that sets the ioctl function to use a specific VDEC
 * instance for fd obtained through V4L2 open. When the VDEC instance is set,
 * variables and settings for that instance must be initialized, and the
 * resource allocation required for the VDEC instance must be done. It should be
 * failed if called on fd not returned by open. The relationship between fd and
 * VDEC instance should be managed in a 1: 1 relationship. If the control ID has
 * been called for the fd already set to another VDEC instance, or for a VDEC
 * instance already set to another fd, it must be failed. If the fd is closed,
 * the connection to fd and VDEC instance is released, and then the VDEC
 * instance can be connected to another fd. The value set in the value of the
 * corresponding control ID is the number of the VDEC instance, which can have a
 * value from 0 to the number of VDEC instances supported by the driver - 1. The
 * number of instances of VDEC is 2 at the current time when the document is
 * modified, and can be extended to a maximum of 256 in the future.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_CHANNEL
 *
 *     //
 *     // control value
 *     //
 *     0~N (VDEC Port Number)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      int fd = -1;
 *      int vdec_port = 0;
 *      v4l2_control ctrl_arg;
 *
 *      fd = open(V4L2_EXT_DEV_PATH_VDEC, O_RDWR);
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_CHANNEL;
 *      ctrl_arg.value = vdec_port;
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_CHANNEL (V4L2_CID_EXT_VDEC_BASE + 1)

#define V4L2_CID_EXT_VDEC_VTP_PORT                                             \
    (V4L2_CID_EXT_VDEC_BASE + 2) // Deprecated, Not use

/**
 * @brief Reset the VDEC
 *
 * @rst
 * The control ID initializes and reset all VDEC instance settings for fd
 * assigned VDEC instance, and flushes buffers related to decoding such as CPB
 * (Coded Picture Buffer) and DPB (Decoded Picture Buffer). When the
 * corresponding control ID is called, ES data corresponding to the decoded
 * frame should be processed so as to prevent malfunction after reset. The value
 * set in the value of the corresponding control ID don't need to set.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_RESETTING
 *
 *     //
 *     // control value
 *     //
 *     Any value
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_RESETTING;
 *      ctrl_arg.value = 1; // Don't care.
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_RESETTING (V4L2_CID_EXT_VDEC_BASE + 3)
#define V4L2_CID_EXT_VDEC_VDO_PORT                                             \
    (V4L2_CID_EXT_VDEC_BASE +                                                  \
     4) // Deprecated, change to V4L2_CID_EXT_VDO_BASE cmd id

/**
 * @brief Video sync control
 *
 * @rst
 * It is control ID that turns on / off AV sync Logic for fd where VDEC instance
 * is set. The default setting is On. If the PTS (Presentation Time Stamp) of
 * the decoded picture is within the video sync threshold (default 5sec)
 * difference based on the STC (System Timing Clock) in the On state, Picture
 * data should be transferred to the VSC if it gets bigger or falls within 1sec
 * / frame rate difference. When it is off, it operates as Freerun and picture
 * data should be transferred to VSC at 1sec / frame rate. Video sync logic is
 * logic for transmitting picture data to VSC module based on STC and PTS value.
 * If the difference between STC and PTS is more than the set threshold (default
 * 5 seconds), it operates as Freerun and displays to display every 1sec / frame
 * rate. If STC has a large PTS, it drops the picture. If the STC is less than
 * the PTS and the threshold (default 5 sec), it should maintain the picture
 * currently displayed until the difference decreases within 1 sec / frame rate.
 * The values set in the value of the control ID can be 0 and 1, 1 means On, 0
 * means Off.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_AV_SYNC
 *
 *     //
 *     // control value
 *     //
 *     0: Disable , 1: Enable
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_AV_SYNC;
 *      ctrl_arg.value = 1; // On : 1, Off : 0
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_AV_SYNC (V4L2_CID_EXT_VDEC_BASE + 5)

/**
 * @brief Control Frame advanced mode for PVR
 *
 * @rst
 * It is a control ID that turns on / off the frame advance mode for fd in which
 * VDEC instance is set. After VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_FRAME_ADVANCE
 * can be called. The default setting is Off. Each time Frame advance mode On is
 * called, a frame is decoded and the data of the next picture to be displayed
 * is repeatedly transmitted to VSC. When Frame advance mode is Off, frame
 * decoding and transmission of picture data to VSC are resumed without frame
 * drop. Frame advance mode is a mode for decoding and displaying frame by frame
 * when you press the pause button two or more times. When the corresponding
 * control ID is called, the value set to value can be 0 or 1, 1 means On, 0
 * means Off.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_FRAME_ADVANCE
 *
 *     //
 *     // control value
 *     //
 *     0: Disable , 1: Enable
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and this
 *   control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and this
 *   control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_FRAME_ADVANCE;
 *      ctrl_arg.value = 1; // On : 1, Off : 0
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_FRAME_ADVANCE (V4L2_CID_EXT_VDEC_BASE + 6)

/**
 * @brief Control decoding speed
 *
 * @rst
 * It is a control ID for setting the decoding speed for fd in which VDEC
 * instance is set. Decoding and displaying is performed at a speed
 * corresponding to 1/1000 of the set value. The value set in the value of the
 * control ID is the speed to be decoded, and is set by multiplying the
 * specified speed by 1000.
 * (Ex : Normal speed : 1000, 2x : 2000, 4x : 4000, 64x : 64000, 0.5x  : 500 )
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DECODING_SPEED
 *
 *     //
 *     // control value
 *     //
 *     V4L2_EXT_VDEC_DECODING_SPEED_PAUSE 0
 *     V4L2_EXT_VDEC_DECODING_SPEED_SLOW 500
 *     V4L2_EXT_VDEC_DECODING_SPEED_NORMAL 1000
 *     V4L2_EXT_VDEC_DECODING_SPEED_2x 2000
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_DECODING_SPEED;
 *      ctrl_arg.value = V4L2_EXT_VDEC_DECODING_SPEED_SLOW
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DECODING_SPEED (V4L2_CID_EXT_VDEC_BASE + 7)

/**
 * @brief Set decode mode
 *
 * @rst
 * It is a control ID that sets Decoding Mode for fd where VDEC instance is set.
 * The DTV stream consists of one complete frame I frame, a P frame with only
 * difference data referring to the other frame, and a B frame with only
 * difference data referring to two frames. Set the Frame to be decoded by the
 * corresponding control ID call. When the corresponding control ID is called,
 * the value set in value means Decode Mode and can be 1 ~ 3. The value of each
 * value is 1, decoding only for I frame, 2, decoding for I and P frames, 3,
 * decoding for all frames, default value is 3.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DECODE_MODE
 *
 *     //
 *     // control value
 *     //
 *     #define V4L2_EXT_VDEC_DECODE_I_FRAME   ( 1 )
 *     #define V4L2_EXT_VDEC_DECODE_IP_FRAME  ( 2 )
 *     #define V4L2_EXT_VDEC_DECODE_ALL_FRAME ( 3 )
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_DECODE_MODE;
 *      ctrl_arg.value = V4L2_EXT_VDEC_DECODE_ALL_FRAME;
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DECODE_MODE (V4L2_CID_EXT_VDEC_BASE + 8)

/**
 * @brief Control displaying Video output
 *
 * @rst
 * It is a control ID that turns on / off Freeze Mode for fd in which VDEC
 * instance is set. After VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_FREEZE_MODE can be
 * called. When Freeze Mode is On, decoding should continue to operate in
 * decoding mode, and the decoded picture is dropped and the picture displayed
 * before the freeze mode is maintained. When a picture is dropped, it is
 * necessary to prevent video stop symptom at the time of releasing the freeze
 * mode by referring to DTS or PTS. DTS is information about the time when a
 * frame is to be decoded. It is a value to operate so that the corresponding
 * frame can be decoded when STC reaches DTS. When the corresponding control ID
 * is called, the value set to value can be 0 or 1, 1 means On, 0 means Off.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_FREEZE_MODE
 *
 *     //
 *     // control value
 *     //
 *     0: Disable freeze mode, 1:Enable freeze mode
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_FREEZE_MODE;
 *      ctrl_arg.value = 1;
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_FREEZE_MODE (V4L2_CID_EXT_VDEC_BASE + 9)

#define V4L2_CID_EXT_VDEC_DECODER_VERSION (V4L2_CID_EXT_VDEC_BASE + 10)
#define V4L2_CID_EXT_VDEC_KR_DUAL_3D_MODE                                      \
    (V4L2_CID_EXT_VDEC_BASE + 11) // Deprecated

/**
 * @brief Control displaying Video output
 *
 * @rst
 * It is a control ID that turns on / off STC Mode for fd in which VDEC instance
 * is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_STC_MODE can be called. Set
 * whether to use the STC generated based on the PCR in the demux as the
 * reference value of AV sync logic and decode time. The values set in the value
 * of the control ID can be 0 and 1, 1 means On, 0 means Off. The default value
 * is 1, which is the On state.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_STC_MODE
 *
 *     //
 *     // control value
 *     //
 *     // 0: Off, 1:On
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_STC_MODE;
 *      ctrl_arg.value = 0;//Off
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_STC_MODE (V4L2_CID_EXT_VDEC_BASE + 12)

/**
 * @brief Set the audio port for AV sync logic
 *
 * @rst
 * It is a control ID for setting an audio channel to be used as an audio master
 * for fd in which VDEC instance is set. Before VIDIOC_STREAMON,
 * V4L2_CID_EXT_VDEC_AUDIO_CHANNEL can be called. It is used to set the audio
 * master of a specific ADEC channel to be used as a reference of AV sync logic.
 * The value set in the value of the control ID is the number of the ADEC
 * channel, and can have a value from 0 to the number of ADECs supported by the
 * driver - 1 '.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_AUDIO_CHANNEL
 *
 *     //
 *     // control value
 *     //
 *     0~N (Audio Port)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_AUDIO_CHANNEL;
 *      ctrl_arg.value = 0;//Audio channel
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_AUDIO_CHANNEL (V4L2_CID_EXT_VDEC_BASE + 13)

/**
 * @brief Set the displaying delay
 *
 * @rst
 * It is a control ID for setting the display delay for the fd in which the VDEC
 * instance is set or checking the total display delay set.
 * After VDEC instance is opened, before VIDIOC_STREAMON is invoked,
 * V4L2_CID_EXT_VDEC_DISPLAY_DELAY is invoked once. so, the display delay
 * setting should be kept even after being invoked VIDIOC_STREAMOFF.
 * When this CID is invoked using VIDIOC_S_CTRL, driver should be delay the
 * delivery of picture data to VSC by adding delay in units of msec in the AV
 * sync logic. In this time, the additional delay should not be repeatedly
 * added with the corresponding Control ID call. Only the display delay which
 * is setting lastly is applied.
 * When this CID is invoked using VIDIOC_G_CTRL, driver should return total
 * display delay set on 'value'.
 * When this CID is invoked using VIDIOC_S_CTRL, the value set in 'value' is
 * the display delay value in msec. When this CID is invoked using
 * VIDIOC_G_CTRL, the delay value in 'value' must be set to msec.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DISPLAY_DELAY
 *
 *     //
 *     // control value
 *     //
 *
 *     // 0~N (ms)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_DISPLAY_DELAY;
 *      ctrl_arg.value = 50;//50ms
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DISPLAY_DELAY (V4L2_CID_EXT_VDEC_BASE + 14)

/**
 * @rst
 * This is the control ID that sets the Lipsync master for the fd where VDEC
 * instance is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_LIPSYNC_MASTER can
 * be called. It is used to set whether to use ADEC master or VDEC master for
 * video sync logic operation. The value to be set to value when calling the
 * corresponding control ID can be 0 ~ 2, 0 means no lipsync master, 1 means use
 * ADEC master, 2 means use VDEC master. The default value is 0.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_LIPSYNC_MASTER
 *
 *     //
 *     // control value
 *     //
 *     0  Use Demux(PCR), Default
 *     1  ADEC master
 *     2  VDEC master
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_LIPSYNC_MASTER;
 *      ctrl_arg.value = 1; //Audio master
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_LIPSYNC_MASTER (V4L2_CID_EXT_VDEC_BASE + 15)

/**
 * @rst
 * It is the control ID that sets the AV sync threshold for fd which VDEC
 * instance is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_VSYNC_THRESHOLD
 * can be called. In AV sync logic, it is called to set the reference value for
 * Freerun operation. The default value is 5000, 5 seconds in msec. The value
 * set in the value of the control ID call is set to the AV sync threshold value
 * in msec. (Ex : 5sec: 5000, 3sec : 3000 )
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_VSYNC_THRESHOLD
 *
 *     //
 *     // control value
 *     //
 *     0~N (VSync Threashold)
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_VSYNC_THRESHOLD;
 *      ctrl_arg.value = 5000; //max 5s
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_VSYNC_THRESHOLD (V4L2_CID_EXT_VDEC_BASE + 16)

/**
 * @rst
 * It is a control ID that turns on / off PVR mode for fd in which VDEC instance
 * is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_PVR_MODE can be called.  It
 * is set to notify that VDEC is used for PVR application. At this time, VDEC
 * should be set to match the PVR operation. The values set in the value of the
 * control ID can be 0 and 1, 1 means On, 0 means Off. The default value is 0,
 * which is Off.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_PVR_MODE
 *
 *     //
 *     // control value
 *     //
 *     0: Off, 1: On
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_PVR_MODE;
 *      ctrl_arg.value = 1; //PVR mode on
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_PVR_MODE (V4L2_CID_EXT_VDEC_BASE + 17)

/**
 * @rst
 * It is a control ID that turns on / off Fast I Frame mode for fd in which VDEC
 * instance is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_FAST_IFRAME_MODE
 * can be called. It is set to notify that VDEC is used for supporting specific
 * SO(System Operator) channel.
 * The values set in the value of the
 * control ID can be 0 and 1, 1 means On, 0 means Off. The default value is 0,
 * which is Off.
 * If Fast I frame mode set On, after invoking VIDIOC_STREAMON, only the first
 * I frame immediately transfers picture data to VSC without waiting that STC
 * becomes equal to PTS by following AV sync logic.
 * By enabling this feature, customers can enjoy seeing earlier the video of
 * the next channel but the early video will usually keep to be frozen for a
 * while.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_FAST_IFRAME_MODE
 *
 *     //
 *     // control value
 *     //
 *     0: Off, 1: On
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_FAST_IFRAME_MODE;
 *      ctrl_arg.value = 1; //On : 1, Off : 0
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_FAST_IFRAME_MODE (V4L2_CID_EXT_VDEC_BASE + 18)

/**
 * @rst
 * It is the control ID that sets the type for the HFR channel for the fd in
 * which the VDEC instance is set.
 *
 * Before :c:func:`v4l-dvb-apis:VIDIOC_STREAMON`, V4L2_CID_EXT_VDEC_HFR_TYPE can
 * be called. HFR (High Frame Rate) is a function to support frame rate up to
 * 120Hz for sharper and smoother image. The corresponding control ID should be
 * set and called before the ioctl VIDIOC_STREAMON, and should be set to 0,
 * which is the default value if not invoked. It should also be reset to the
 * default value after the ioctl :c:func:`v4l-dvb-apis:VIDIOC_STREAMOFF` call.
 *
 * There are HFR 1PID stream and HFR 2PID stream. In addition, since the SI
 * infomation of HFR stream is not well configured, it is possible to need to do
 * auto-detect & setting operation where the resolution or the frame rate is
 * unclear.
 *
 * ES data of HFR stream is divided into BL (Base Layer) and EL (Enhence layer).
 * if the PID of the EL & BL is different, than it is called HFR 2PID. and, if
 * the PID of the EL & BL is the same, than it is called HFR 1PID.
 *
 * When the control ID is called, the value can be set to 0 ~ 4. The default
 * value is 0, meaning SFR. The meanings of the remaining values are shown in
 * the table below. If decoder performance cannot support for each setting, it
 * should be failed.
 *
 * For HFR 2PID (0x02, 0x03), UHD HFR 1PID (0x04), the 60Hz model should return
 * fail. In the case of the HFR 2PID case, VDEC element set only the Main PID,
 * and the VDEC driver should decode only the Main PID.
 *
 * In the case of UHD 1PID with temporal scalability case, VDEC element is set
 * to the next higher layer with :c:macro:`V4L2_CID_EXT_VDEC_TEMPORAL_ID_MAX`,
 * the driver should drop the top layer and decode it to the others layer.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_HFR_TYPE
 *
 *     //
 *     // control value
 *     //
 *
 *   +------------------------------+-------+-----------------------------------------------------+
 *   | Type                         | Value | Description                                         |
 *   +==============================+=======+=====================================================+
 *   | SFR(Default)                 | 0x00  | | Equivalent to HEVC channel operation              |
 *   |                              |       | | before introduction of HFR function               |
 *   +------------------------------+-------+-----------------------------------------------------+
 *   | | SFR/HFR unclear or         | 0x01  | | Don't need to support SFR / HFR seamless change.  |
 *   | | HFR & Resolution unclear   |       | | Driver need to support HFR auto detect & setting. |
 *   +------------------------------+-------+-----------------------------------------------------+
 *   | UHD HFR 2PID                 | 0x02  | | Don't need HFR auto detect & setting.             |
 *   |                              |       | | need to support SFR / HFR seamless change.        |
 *   +------------------------------+-------+-----------------------------------------------------+
 *   | | HFR 2PID,                  | 0x03  | | Don't need to support SFR / HFR seamless change.  |
 *   | | Resolution unclear         |       | | Driver need to support HFR auto detect & setting. |
 *   +------------------------------+-------+-----------------------------------------------------+
 *   | UHD HFR 1PID                 | 0x04  | | Don't need HFR auto detect & setting.             |
 *   |                              |       | | If the stream have temporal scalability,          |
 *   |                              |       | | need to support SFR / HFR seamless change.        |
 *   +------------------------------+-------+-----------------------------------------------------+
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_HFR_TYPE;
 *      ctrl_arg.value = 0; //SFR , Default value
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_HFR_TYPE (V4L2_CID_EXT_VDEC_BASE + 19)

/**
 * @rst
 * It is a control ID that sets Temporal_id_max value for fd where VDEC instance
 * is set. Before VIDIOC_STREAMON, V4L2_CID_EXT_VDEC_TEMPORAL_ID_MAX can be
 * called. Frames with a layer higher than Temporal_id_max should be dropped
 * before decoding. Each frame of the ES stream can be layered and the entire
 * image can be constructed so that it is decoded as a reference to a frame
 * belonging to itself or to its subordinate, where the value of the highest
 * layer is temporal_id_max. When calling the corresponding control ID, the
 * value can be a value between -1 and 6, and the default value is -1. -1 value
 * mean 'not set'. Other values mean temporal_id_max value.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_TEMPORAL_ID_MAX
 *
 *     //
 *     // control value
 *     //
 *     -1 (Default), Not set
 *     value range: -1~6
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and this
 *   control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and this
 *   control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_TEMPORAL_ID_MAX;
 *      ctrl_arg.value = 0; //Temporal ID Max is 0
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_TEMPORAL_ID_MAX (V4L2_CID_EXT_VDEC_BASE + 20)

/**
 * @rst
 * It is a control ID that sets Direct Mode for fd in which VDEC instance is
 * set. In case of holding ES data or obtaining ES data through another path,
 * the mode can be changed to one that can directly input. The value set to the
 * value of the control ID call is "Don't care".
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DIRECT_MODE
 *
 *     //
 *     // control value
 *     //
 *
 *     // Don't care
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_DIRECT_MODE;
 *      ctrl_arg.value = 1; //Don't care
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DIRECT_MODE (V4L2_CID_EXT_VDEC_BASE + 21)

/**
 * @rst
 * Value of V4L2_CID_EXT_VDEC_DRIPDEC_MODE indicates the decoding status. 0 is
 * the default. 0 means decoder should decode all frames(I/B/P frames). 1 means
 * decoder should decoder I-Frame only. VIDIOC_S_FMT must be set before
 * V4L2_CID_EXT_VDEC_DRIPDEC_MODE setting.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DRIPDEC_MODE
 *
 *     //
 *     // control value
 *     //
 *     0 = Default, non-Drip Decoding mode
*      1 =  Drip Decoding mode
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_DRIPDEC_MODE;
 *      ctrl_arg.value = 1; // Decoding I frame only.
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DRIPDEC_MODE                                         \
    (V4L2_CID_EXT_VDEC_BASE + 22) // To support MHEG certification

/**
 * @rst
 * This command is used to tell VDEC to update ECP related information. The
 * ioctl is only called, and there is no information passed by REE. Effective
 * information is managed by TEE. Therefore, communication with TEE is required.
 * Calls V4L2_CID_EXT_VDEC_ECP_INFO_NOTI after VIDIOC_S_FMT is called.
 * VIDIOC_STREAMON Ensures that Decoder can check whether ECP mode is applied
 * before calling. This is because a coded picture buffer (CPB) is allocated
 * after VIDIOC_STREAMON.  If V4L2_CID_EXT_VDEC_CHANNEL is not set in fd, it
 * returns error. In case of VIDIOC_STREAMOFF, it is ECP Protection off to
 * restore to default.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL       // Set
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_ECP_INFO_NOTI
 *
 *     //
 *     // control value
 *     //
 *
 *     // Don't care
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using :c:func:`v4l-dvb-apis:VIDIOC_S_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_ECP_INFO_NOTI;
 *      ctrl_arg.value = 0;
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_ECP_INFO_NOTI                                        \
    (V4L2_CID_EXT_VDEC_BASE + 23) // To support DVB - ECP set

/**
 * @rst
 * It is a command to receive Offset of ECP processed CPB. The default value is
 * 0, which is the return of the ECP Off case.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_ECP_OFFSET
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_ECP_OFFSET;
 *      ctrl_arg.value = 0;
 *
 *      ioctl(fd, VIDIOC_G_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_ECP_OFFSET                                           \
    (V4L2_CID_EXT_VDEC_BASE + 24) // To support DVB - ECP get

/**
 * @rst
 * It is a command to receive Size of ECP processed CPB. The default value is 0,
 * which is the return of the ECP Off case.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_ECP_SIZE
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_CTRL` and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      ctrl_arg.id = V4L2_CID_EXT_VDEC_ECP_SIZE;
 *      ctrl_arg.value = 0;
 *
 *      ioctl(fd, VIDIOC_G_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_ECP_SIZE                                             \
    (V4L2_CID_EXT_VDEC_BASE + 25) // To support DVB - ECP get

/* VDEC class EXT control IDs */

/**
 * @rst
 * It is a control ID for receiving Userdata parsed by the driver from the
 * driver when a Userdata event occurs for fd in which VDEC instance is set.
 *
 * Userdata refers to 603 type closed caption, rating data, DVS 157 type closed
 * caption data, and rating data. User data event occurs when there is
 * corresponding data in ES data entered in VDEC drvier. When the Userdata event
 * is registered with the ioctl VIDIOC_SUBSCRIBE_EVENT, the VDEC driver shall
 * record the occurrence of the event in the event queue and manage the event
 * occurrence information to check the occurrence of the event through the ioctl
 * DQEVENT.
 *
 * Also, userdata should be managed in a separate space so that userdata for the
 * event can be taken with ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . If
 * the userdata in the VDEC element is copied to the ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`, the corresponding data can be
 * discarded.
 *
 * When a Userdata event occurs, the size of userdata must be stored by using 4
 * bytes of the data array of struct v4l2_event. At the same time, picType,
 * isRepeatFieldFirst, isTopFieldFirst, temporalReference, and pts should be
 * uploaded using the data array of struct v4l2_event. Based on this, the VDEC
 * element allocate a memory space and calls ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`.
 *
 * If a userdata event is detected by calling ioctl DQEVENT on the VDEC
 * instance, allocate memory space to take userdata, and set size and ptr to
 * call ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`. The VDEC driver must
 * copy userdata to the memory space pointed to by ptr and return it. When
 * calling the corresponding control ID, the value set to size is the size of
 * the memory space, and the value set in ptr is the memory space address of the
 * user space.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_USER_EVENT_DATA
 *
 *     //
 *     // Parameters
 *     //
 *     struct v4l2_event {
 *      __u32 type;                          //Type of the event
 *
 *      union {
 *         struct v4l2_event_vsync vsync;
 *         struct v4l2_event_ctrl ctrl;
 *         struct v4l2_event_frame_sync frame_sync;
 *         struct v4l2_event_motion_det motion_det;
 *         struct v4l2_event_src_change src_change;
 *         __u8 data[64];                  // Userdata event case
 *                                         // data[0] ~ data[3] : Userdata size
 *                                         // data[4] : Picture type (I Picture : 1, P Picture : 2, B Picture : 3)
 *                                         // data[5] : Repeat Field First (Yes : 1, No : 0)
 *                                         // data[6] : Top Field First (Yes : 1, No : 0)
 *                                         // data[7] : Temporal reference of the picture
 *                                         // data[8] ~ data[11] : PTS
 *      } u;
 *      __u32 pending;                     //Number of pending events excluding this one.
 *      __u32 sequence;                    //Event sequence number.
 *      struct timespec timestamp;         //Event timestamp.
 *      __u32 id;                          //The ID associated with the event source.
 *      __u32 reserved[8];                 //Reserved for future extensions.
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      pollfd poll_fd;
 *      struct v4l2_event evt_arg;
 *
 *      typedef struct {
 *          uint8_t     picType;            //Picture type (I Picture : 1, P Picture : 2, B Picture : 3)
 *          uint8_t     isRepeatFieldFirst; //Repeat Field First (Yes : 1, No : 0)
 *          uint8_t     isTopFieldFirst;    //Top Field First (Yes : 1, No : 0)
 *          uint8_t     temporalReference;  //Temporal reference of the picture
 *          uint32_t    pts;
 *          void        *pUserdata;
 *          int32_t     size;               //Userdata size
 *      } USERDATA_EVENT_T;
 *
 *      while(1) {
 *         poll_fd.fd = fd;
 *         poll_fd.event = POLLPRI; // POLLPRI is defined in linux spec
 *         poll_fd.revents = 0;
 *         while(0 < poll(&poll_fd, 1, 50)) {
 *            ioctl(fd, VIDIOC_DQEVENT, &evt_arg);
 *
 *
 *            if(V4L2_SUB_EXT_VDEC_USERDATA == evt_arg.id) {
 *               struct v4l2_ext_controls ext_ctrls_arg;
 *               struct v4l2_ext_control ext_ctrl_arg;
 *               USERDATA_EVENT_T userdataInfo = {0, };
 *
 *               ext_ctrls_arg.count = 1;
 *               ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *               memcpy(&userdataInfo.size, &evt_arg.u.data[0], 4);
 *               memcpy(&userdataInfo.picType, &evt_arg.u.data[4], 1);
 *               memcpy(&userdataInfo.isRepeatFieldFirst, &evt_arg.u.data[5], 1);
 *               memcpy(&userdataInfo.isTopFieldFirst, &evt_arg.u.data[6], 1);
 *               memcpy(&userdataInfo.temporalReference, &evt_arg.u.data[7], 1);
 *               memcpy(&userdataInfo.pts, &evt_arg.u.data[8], 4);
 *
 *               userdataInfo.pUserdata = (void *)malloc(userdataInfo.size);
 *
 *               ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_USER_EVENT_DATA;
 *               ext_ctrl_arg.size = userdataInfo.size;
 *               ext_ctrl_arg.ptr = userdataInfo.pUserdata;
 *
 *               ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 *               //send userdata from userdata_ptr to subtitle module.
 *
 *               free(userdataInfo.pUserdata);
 *            }
 *
 *
 *            if(0 == event_arg.pending) {
 *               break;
 *            }
 *         }
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_USER_EVENT_DATA (V4L2_CID_EXT_VDEC_BASE + 101)

/**
 * @rst
 * It is a control ID for receiving Picture info data from the driver when a
 * Picture info event occurs for fd in which VDEC instance is set. Picture info
 * event is an event that takes place when VSC takes picture data through VDO.
 *
 * It is an event to register information related to picture delivered at VDEC
 * element from VDEC element. The VDEC driver should record the event occurrence
 * in the event queue after registering the Picture info event and manage the
 * event occurrence information to check the occurrence of the event through
 * ioctl DQEVENT.
 *
 * Also, Picture info data should be managed in a separate space so that Picture
 * info data for the event can be taken with ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`. If the Picture info data in the
 * VDEC element is moved to the ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`,
 * the corresponding data can be discarded.
 *
 * If a picture info event is detected by calling ioctl DQEVENT on the VDEC
 * instance, allocate memory space to take picture info data, and set size and
 * ptr to call ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . The VDEC driver
 * must copy picture info data to the memory space pointed to by ptr and return
 * it. When calling the corresponding control ID, the value set to size is the
 * size of the memory space, and the value set in ptr is the memory space
 * address of the user space.
 *
 * Picture info event is an event that occurs when VSC takes picture data
 * through VDO if picture info data is different from previous picture info.
 * Because VDEC MW should get Picture info from VDEC driver, Picture info event
 * is registered to VDEC driver.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      pollfd poll_fd;
 *      struct v4l2_event evt_arg;
 *
 *      struct v4l2_ext_picinfo_msg picinfo_msg;
 *
 *      while(1) {
 *         poll_fd.fd = fd;
 *         poll_fd.event = POLLPRI; //POLLPRI is defined in linux spec
 *         poll_fd.revent = 0;
 *         while(0 < poll(&poll_fd, 1, 50)) {
 *            ioctl(fd, VIDIOC_DQEVENT, &evt_arg);
 *
 *
 *            if(V4L2_SUB_EXT_VDEC_PICINFO == evt_arg.id) {
 *               struct v4l2_ext_controls ext_ctrls_arg;
 *               struct v4l2_ext_control ext_ctrl_arg;
 *
 *               ext_ctrls_arg.count = 1;
 *               ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *               ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA;
 *               ext_ctrl_arg.size = sizeof(struct v4l2_ext_picinfo_msg);
 *               ext_ctrl_arg.ptr = &picinfo_msg;
 *
 *               ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 *               //update picinfo data
 *            }
 *
 *            if(0 == event_arg.pending) {
 *               break;
 *            }
 *         }
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA (V4L2_CID_EXT_VDEC_BASE + 102)

/**
 * @rst
 * It is a control ID to obtain the decoder status to obtain the current state
 * of the VDEC decoder driver for the fd in which the VDEC instance is set. When
 * calling the corresponding control ID, set it in the
 * :cpp:any:`v4l2_ext_decoder_status` structure that returns the state value of
 * the driver and return it.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DECODER_STATUS
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      struct v4l2_ext_controls ext_ctrls_arg;
 *      struct v4l2_ext_control ext_ctrl_arg;
 *      struct v4l2_ext_decoder_status decoder_status;
 *
 *      ext_ctrls_arg.count = 1;
 *      ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *      ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_DECODER_STATUS;
 *      ext_ctrl_arg.size = sizeof(struct v4l2_ext_decoder_status);
 *      ext_ctrl_arg.ptr = &decoder_status;
 *
 *      ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 * See also
 *   - :cpp:any:`v4l2_ext_decoder_status`
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DECODER_STATUS (V4L2_CID_EXT_VDEC_BASE + 103)

/**
 * @rst
 * V4L2_CID_EXT_VDEC_STREAM_INFO is replaced to V4L2_CID_EXT_VDEC_VIDEO_INFO.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_STREAM_INFO
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      struct v4l2_ext_controls ext_ctrls_arg;
 *      struct v4l2_ext_control ext_ctrl_arg;
 *      struct v4l2_ext_stream_info ext_stream_info;
 *
 *      ext_ctrls_arg.count = 1;
 *      ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *      ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_STREAM_INFO;
 *      ext_ctrl_arg.size = sizeof(struct v4l2_ext_stream_info);
 *      ext_ctrl_arg.ptr = &ext_stream_info;
 *
 *      ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_STREAM_INFO (V4L2_CID_EXT_VDEC_BASE + 104)

/**
 * @rst
 * It is the control ID that is to directly feed ES data on the fd that is set
 * with VDEC instance. It can be run in Direct Mode, and Direct Mode changes
 * through the call of V4L2_CID_EXT_VDEC_DIRECT_MODE. When calling the control
 * ID, put the size of ES data and OFFSET value to size and ptr.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DIRECT_ESDATA
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      struct v4l2_ext_controls ext_ctrls_arg;
 *      struct v4l2_ext_control ext_ctrl_arg;
 *
 *      ext_ctrls_arg.count = 1;
 *      ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *      struct v4l2_format format_arg = {0, };
 *
 *      format_arg.fmt.pix.pixelformat = v4l2_fourcc('H', 'E', 'V', 'C'); //HEVC codec type setting
 *
 *      ioctl(fd, VIDIOC_S_FMT, &format_arg);
 *
 *      int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
 *      ioctl(fd, VIDIOC_STREAMON, &type); // Ready for feeding
 *
 *      ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_DIRECT_ESDATA;
 *
 *      ext_ctrl_arg.size = SIZE4DATA;
 *      ext_ctrl_arg.ptr = OFFSET4DATA;
 *
 *      ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DIRECT_ESDATA (V4L2_CID_EXT_VDEC_BASE + 105)

/**
 * @rst
 * It is a control ID for obtaining video information of the current image with
 * respect to fd in which VDEC instance is set. After VIDIOC_STREAMON,
 * V4L2_CID_EXT_VDEC_VIDEO_INFO will be called. In VDEC MW, the
 * v4l2_ext_vdec_video_info ENUM value corresponding to the video info to be
 * checked in the 'value' value is set and the
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` is called. Therefore, the
 * corresponding video info should be set to 'value' and returned. Please
 * refrain from BSP implementation before Linuxtv-ext-header CCC is completed on
 * JCL4TV and internal review.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_VIDEO_INFO
 *
 *     //
 *     // control value
 *     //
 *
 *   See :cpp:enum:`v4l2_ext_vdec_video_info`
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      struct v4l2_ext_controls ext_ctrls_arg = {0, };
 *      struct v4l2_ext_control ext_ctrl_cid   = {0, };
 *      struct v4l2_control get_param;
 *
 *      memset(&ext_ctrls_arg , 0, sizeof(struct v4l2_ext_controls));
 *      memset(&ext_ctrl_cid   ,  0, sizeof(struct v4l2_ext_control));
 *      memset(&get_param,    0, sizeof(struct v4l2_control));
 *
 *      ext_ctrls_arg.count = 1
 *      ext_ctrls_arg.controls = &ext_ctrl_cid;
 *
 *      ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_VIDEO_INFO;
 *      ext_ctrl_arg.size = sizeof(struct v4l2_control);
 *      ext_ctrl_arg.ptr = &get_param;
 *      get_param.id = VDEC_PROGRESSIVE_SEQUENCE;
 *      get_param.value = 0; //initialize
 *
 *      ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg );
 *
 *      int frame_rate = get_param.value;
 *
 *  See also
 *    - :cpp:enum:`v4l2_ext_vdec_video_info`
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_VIDEO_INFO (V4L2_CID_EXT_VDEC_BASE + 106)

/**
 * @rst
 * It is a control ID for receiving Picture info data from the driver when a
 * Picture info event occurs for fd in which VDEC instance is set. Picture info
 * event is an event that takes place when VSC takes picture data through VDO.
 * It is an event to register information related to picture delivered at VDEC
 * element from VDEC element. The VDEC driver should record the event occurrence
 * in the event queue after registering the Picture info event and manage the
 * event occurrence information to check the occurrence of the event through
 * ioctl DQEVENT. Also, Picture info data should be managed in a separate space
 * so that Picture info data for the event can be taken with ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . If the Picture info data in the
 * VDEC element is moved to the ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 * , the corresponding data can be discarded. If a picture info event is
 * detected by calling ioctl DQEVENT on the VDEC instance, allocate memory space
 * to take picture info data, and set size and ptr to call ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . The VDEC driver must copy picture
 * info data to the memory space pointed to by ptr and return it. When calling
 * the corresponding control ID, the value set to size is the size of the memory
 * space, and the value set in ptr is the memory space address of the user
 * space.
 *
 * Picture info event is an event that occurs when VSC takes picture data
 * through VDO if picture info data is different from previous picture info.
 * Because VDEC MW should get Picture info from VDEC driver, Picture info event
 * is registered to VDEC driver.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA_EXT
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      pollfd poll_fd;
 *      struct v4l2_event evt_arg;
 *
 *      struct v4l2_ext_picinfo_msg_ext picinfo_msg;
 *
 *      while(1) {
 *         poll_fd.fd = fd;
 *         poll_fd.event = POLLPRI; //POLLPRI is defined in linux spec
 *         poll_fd.revent = 0;
 *         while(0 < poll(&poll_fd, 1, 50)) {
 *            ioctl(fd, VIDIOC_DQEVENT, &evt_arg);
 *
 *
 *            if(V4L2_SUB_EXT_VDEC_PICINFO == evt_arg.id) {
 *               struct v4l2_ext_controls ext_ctrls_arg;
 *               struct v4l2_ext_control ext_ctrl_arg;
 *
 *               ext_ctrls_arg.count = 1;
 *               ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *               ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA_EXT;
 *               ext_ctrl_arg.size = sizeof(struct v4l2_ext_picinfo_msg);
 *               ext_ctrl_arg.ptr = &picinfo_msg;
 *
 *               ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 *               //update picinfo data
 *            }
 *
 *            if(0 == event_arg.pending) {
 *               break;
 *            }
 *         }
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA_EXT                               \
    (V4L2_CID_EXT_VDEC_BASE + 107) // To support HFR, frame rate fraction type
                                   // structure is added. and useless member
                                   // variable is deleted.

/**
 * @rst
 * It is a control ID for receiving Picture info data from the driver when a
 * Picture info event occurs for fd in which VDEC instance is set. Picture info
 * event is an event that takes place when VSC takes picture data through VDO.
 * It is an event to register information related to picture delivered at VDEC
 * element from VDEC element. The VDEC driver should record the event occurrence
 * in the event queue after registering the Picture info event and manage the
 * event occurrence information to check the occurrence of the event through
 * ioctl DQEVENT. Also, Picture info data should be managed in a separate space
 * so that Picture info data for the event can be taken with ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . If the Picture info data in the
 * VDEC element is moved to the ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 * , the corresponding data can be discarded. If a picture info event is
 * detected by calling ioctl DQEVENT on the VDEC instance, allocate memory space
 * to take picture info data, and set size and ptr to call ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . The VDEC driver must copy picture
 * info data to the memory space pointed to by ptr and return it. When calling
 * the corresponding control ID, the value set to size is the size of the memory
 * space, and the value set in ptr is the memory space address of the user
 * space.
 *
 * Picture info event is an event that occurs when VSC takes picture data
 * through VDO if picture info data is different from previous picture info.
 * Because VDEC MW should get Picture info from VDEC driver, Picture info event
 * is registered to VDEC driver.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA_EXT
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      pollfd poll_fd;
 *      struct v4l2_event evt_arg;
 *
 *      struct v4l2_ext_picinfo_msg_ext picinfo_msg;
 *
 *      while(1) {
 *         poll_fd.fd = fd;
 *         poll_fd.event = POLLPRI; //POLLPRI is defined in linux spec
 *         poll_fd.revent = 0;
 *         while(0 < poll(&poll_fd, 1, 50)) {
 *            ioctl(fd, VIDIOC_DQEVENT, &evt_arg);
 *
 *
 *            if(V4L2_SUB_EXT_VDEC_PICINFO == evt_arg.id) {
 *               struct v4l2_ext_controls ext_ctrls_arg;
 *               struct v4l2_ext_control ext_ctrl_arg;
 *
 *               ext_ctrls_arg.count = 1;
 *               ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *               ext_ctrl_arg.id = V4L2_CID_EXT_VDEC_PICINFO_EVENT_DATA_EXT;
 *               ext_ctrl_arg.size = sizeof(struct v4l2_ext_picinfo_msg_ext);
 *               ext_ctrl_arg.ptr = &picinfo_msg;
 *
 *               ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *
 *               //update picinfo data
 *            }
 *
 *            if(0 == event_arg.pending) {
 *               break;
 *            }
 *         }
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DRIPDEC_PICTURE                                      \
    (V4L2_CID_EXT_VDEC_BASE +                                                  \
     108) // To support MHEG certification, Pass I-frame buffer information


/**
 * @rst
 * In the case when this ioctl is used for set,
 * the moment when a Video Frame whose PTS is larger or same with the PTS given
 * by LGE mw is displayed on TV Screen. The Video Frame is freezed on TV Screen.
 * Enable or Disable the Hash/CRC generation whenever the FrameType Event occurs.
 * In the case when this ioctl is used for get,
 * this API returns the information whether the VDEC is frozen or not,
 * if the VDEC is currently frozen,
 * it also returns a hash value of the frozen video frame.
 * The hash value should be so unique that it can represent the video pattern
 * of the frozen video frame.
 * The best algorithm should be found by Soc vendor.
 * For example, the value can be calculated by simply summing all pixel values of one frozen video frame
 * or it can be CRC value of one frozen video frame.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS       // Get
 *     VIDIOC_S_EXT_CTRLS       // Set
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDEC_DECODED_PICTURE_BUFFER
 *
 *     //
 *     // control value
 *     //
 *     -
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_S_EXT_CTRLS`
 *   and this control id.
 *   Application can get data using :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     struct v4l2_ext_decoded_picture_buffer{
 *         __u32 command;    // control commands for VDEC Decoded Picture Buffer
 *                           // 0: Unfreeze the fronzen VDEC
 *                           // 1: Freeze VDEC based on target PTS and Generate Hash/CRC
 *                           // 3: Enable  the Hash/CRC generation whenever the
 *                           //    PictureType event occurs
 *                           // 0: Disable the Hash/CRC generation whenever the
 *                           //    PcitureType event occurs
 *         __u64 target_pts; // 33 bits target PTS
 *         __u32 bFrozen;    // current VDEC is fronzen or not
 *         __u32 hashValue;  // unique value which represents the video
 *                           // pattern of the frozen video frame
 *     };
 *
 *      int LGE_API_IsVdecFrozen( int *bFrozen, int *pHash)
 *      {
 *          int rc;
 *
 *          struct v4l2_ext_decoded_picture_buffer vdecCont      = {0, };
 *          struct v4l2_ext_controls               ext_ctrls_arg = {0,};
 *          struct v4l2_ext_control                ext_ctrl_arg  = {0,};
 *
 *          ext_ctrls_arg.count    = 1;
 *          ext_ctrls_arg.controls = &ext_ctrl_arg;
 *
 *          ext_ctrl_arg.id        = V4L2_CID_EXT_VDEC_DECODED_PICTURE_BUFFER;
 *          ext_ctrl_arg.size      = sizeof(v4l2_ext_decoded_picture_buffer);
 *          ext_ctrl_arg.ptr       = &vdecCont;
 *
 *          rc                     = ioctl(vdec_fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls_arg);
 *          *bFrozen               = vdecCont.bFrozen;
 *          // If vdec is currently bFrozen,     *bFrozen = 1
 *          // If vdec is currently not bFrozen, *bFrozen = 0
 *
 *          *pHash                 = vdecCont.hashValue;
 *          // Hash value of the Video Frame
 *          printf("Hash value of this frame is %d \n", *pHash);
 *
 *          return rc;
 *      }
 *
 *      struct v4l2_ext_decoded_picture_buffer vdecCont = {0, };
 *      vdecCont.target_pts    = 0x12345...;
 *      vdecCont.command       = 1; // 0:UnFreeze(Melt), 1:Freeze
 *      ext_ctrl_arg.id        = V4L2_CID_EXT_VDEC_DECODED_PICTURE_BUFFER;
 *      ext_ctrls_arg.ptr      = &vdecCont;
 *
 *      rc                     = ioctl(vdec_fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls_arg);
 *
 *      int bFreeze = 0;
 *      int HashVal = 0;
 *      while(1)
 *      {
 *          rc = LGE_API_IsVdecFreezed(&bFrozen, &HashVal);
 *          // About LGE_API_IsVdecFreezed, Refer to the abvoe description
 *          if (bFrozen == 1)
 *          {
 *              printf("Video Freeze occur !! HashVal=%d \n", HashVal);
 *
 *              // Method 1
 *              if (HashVal == TARGET_VALUE)
 *                  printf("Test is OK\n");
 *              else
 *                  printf("Test is NG\n");
 *
 *              // Method 2
 *              Testing mw captures the TV Screen and compare the captured
 *              video and the target Picture
 *              Judge current video is ok or not
 *
 *              // unfreeze the Video
 *              vdecCont.command = 0; // 0:UnFreeze(Melt), 1:Freeze
 *              rc = ioctl(vdec_fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls_arg);
 *              break;
 *           }
 *           delay(100msec);
 *      }
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDEC_DECODED_PICTURE_BUFFER                                      \
    (V4L2_CID_EXT_VDEC_BASE +                                                  \
     109)


/* VDEC class subscription types */
#define V4L2_EVENT_PRIVATE_EXT_VDEC_BASE (V4L2_EVENT_PRIVATE_START + 0x8000)

/**
 * @rst
 * V4L2 Subscription Type for VDEC
 * @endrst
 */
#define V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT (V4L2_EVENT_PRIVATE_EXT_VDEC_BASE + 1)

/* VDEC class subscription event IDs */
#define V4L2_SUB_EXT_VDEC_BASE (0x8000)

/**
 * @rst
 * It is a control ID for subscribing or unsubscribing Frame Type event for fd
 * in which VDEC instance is set. Before VIDIOC_STREAMON,
 * V4L2_SUB_EXT_VDEC_FRAME can be subscribed. Frame Type event is an event that
 * occurs when frame decoding is completed in the VDEC decoder drvier. The Frame
 * Type event is registered to take the information of the decoded frame to the
 * VDEC element. The VDEC driver should record the event occurrence in the event
 * queue after registering the Frame Type event, and should manage event
 * occurrence information to check the occurrence of the event through ioctl
 * DQEVENT. To retrieve the Frame Type data for the event, ioctl DQEVENT must
 * return information about the picture type and PTS data in the u.data array of
 * the v4l2_event structure. In the u.data[0], picture type data should be
 * stored.(SeqHeader : 0, I Frame : 1, P Frame : 2, B Frame : 3). In the
 * u.data[1] ~ u.data[5], PTS data should be stored.
 *
 * The value set as the type when the corresponding control ID is called is
 * registered as V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT which means VDEC related
 * event.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT, VIDIOC_UNSUBSCRIBE_EVENT // Get
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_event_subscription subscription_arg;
 *
 *      subscription_arg.type = V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT;
 *      subscription_arg.id = V4L2_SUB_EXT_VDEC_FRAME;
 *
 *      ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &subscription_arg); // Subscribe frame decoding event
 *
 *      ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &subscription_arg); // Unsubscribe frame decoding event
 *
 *      //When Frame Type event is occurred, in the event thread, Frame type info is detected.
 *      pollfd poll_fd;
 *      struct v4l2_event evt_arg;
 *
 *      while(1) {
 *         poll_fd.fd = fd;
 *         poll_fd.event = POLLPRI; //POLLPRI is defined in linux spec
 *         poll_fd.revent = 0;
 *
 *         while(0 < poll(&poll_fd, 1, 50)) {
 *            ioctl(fd, VIDIOC_DQEVENT, &evt_arg);
 *
 *
 *            if(V4L2_SUB_EXT_VDEC_FRAME == evt_arg.id) {
 *               uint32_t frame_type = evt_arg.u.data[0];
 *               uint64_t pts = 0;
 *
 *
 *               memcpy(&(evt_arg.u.data[1]), &pts, 5);
 *            }
 *
 *            if(0 == event_arg.pending) {
 *               break;
 *            }
 *         }
 *      }
 *
 * @endrst
 */
#define V4L2_SUB_EXT_VDEC_FRAME (V4L2_SUB_EXT_VDEC_BASE + 1)

/**
 * @rst
 * It is a control ID for subscribing or unsubscribing Picture info event for fd
 * in which VDEC instance is set. Before VIDIOC_STREAMON,
 * V4L2_SUB_EXT_VDEC_PICINFO can be subscribed. Picture info event is an event
 * that always occurs when VSC takes picture data through VDO. It is an event to
 * register information related to picture delivered at this time in order to
 * take VDEC element. The VDEC driver should record the event occurrence in the
 * event queue after registering the Picture info event and manage the event
 * occurrence information to check the occurrence of the event through ioctl
 * DQEVENT. Also, Picture info data should be managed in a separate space so
 * that Picture info data for the event can be taken with ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . If the Picture info data in the
 * VDEC element is moved to the ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS`
 * , the corresponding data can be discarded. The value set as the type when the
 * corresponding control ID is called is registered as
 * V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT which means VDEC related event.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT, VIDIOC_UNSUBSCRIBE_EVENT
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_DQEVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_event_subscription subscription_arg;
 *
 *      subscription_arg.type = V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT;
 *      subscription_arg.id = V4L2_SUB_EXT_VDEC_PICINFO;
 *
 *      ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &subscription_arg); // Subscribe picture info event
 *
 *      ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &subscription_arg); // Unsubscribe picture info event
 *
 * @endrst
 */
#define V4L2_SUB_EXT_VDEC_PICINFO (V4L2_SUB_EXT_VDEC_BASE + 2)

/**
 * @rst
 * It is a control ID for subscribing or unsubscribing Userdata event for fd in
 * which VDEC instance is set. Before VIDIOC_STREAMON,
 * V4L2_SUB_EXT_VDEC_USERDATA can be subscribed. Userdata refers to 603 type
 * closed caption, rating data, DVS 157 type closed caption data, and rating
 * data. Userdata event occurs when there is userdata in ES data entered in VDEC
 * drvier. The VDEC driver should record the event occurrence in the event queue
 * after registering the Userdata event and manage the event occurrence
 * information to check the occurrence of the event through ioctl DQEVENT. Also,
 * userdata should be managed in a separate space so that userdata for the event
 * can be taken with ioctl :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` . If the
 * userdata in the VDEC element is moved to the ioctl
 * :c:func:`v4l-dvb-apis:VIDIOC_G_EXT_CTRLS` , the corresponding data can be
 * discarded. The value set as the type when the corresponding control ID is
 * called is registered as V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT which means VDEC
 * related event.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_SUBSCRIBE_EVENT, VIDIOC_UNSUBSCRIBE_EVENT
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_DQEVENT.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_event_subscription subscription_arg;
 *      int ret = -1;
 *
 *      subscription_arg.type = V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT;
 *      subscription_arg.id = V4L2_SUB_EXT_VDEC_USERDATA;
 *
 *      ret = ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &subscription_arg); // Subscribe userdata event
 *
 *
 *      subscription_arg.type = V4L2_EVENT_PRIVATE_EXT_VDEC_EVENT;
 *      subscription_arg.id = V4L2_SUB_EXT_VDEC_USERDATA;
 *
 *      ret = ioctl(fd, VIDIOC_UNSUBSCRIBE_EVENT, &subscription_arg); // Unsubscribe userdata event
 *
 * @endrst
 */
#define V4L2_SUB_EXT_VDEC_USERDATA (V4L2_SUB_EXT_VDEC_BASE + 3)

/* VDO class control ID */
#define V4L2_CID_EXT_VDO_BASE (V4L2_CID_USER_EXT_VDEC_BASE + 0x500)

/**
 * @rst
 * This is the control ID for connecting / disconnecting VDO to VDEC by
 * specifying VDEC channel through fd opened by V4L2 in VDO device minor number.
 * VDO is a path for transmitting the decoded picture to the VSC. In addition,
 * VSC is a module that receives decoded picture and performs enlargement /
 * reduction function. When the control ID is called, the value set to value is
 * the number of the VDEC channel, and when set to -1, it means disconnect.
 *
 * VDO Connecting method for webOS 5.0
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDO_VDEC_PORT
 *
 *     //
 *     // control value
 *     //
 *
 *     // 0~N (Target port number to be connected)
 *     // -1 (Disconnect)
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_control ctrl_arg;
 *
 *      fd = open(V4L2_EXT_DEV_PATH_VDO0, O_RDWR);
 *      ctrl_arg.id = V4L2_CID_EXT_VDO_VDEC_PORT;
 *      ctrl_arg.value = 0; // When you doing Connect, you need to set value argument as vdec channel num.
 *
 *      ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDO_VDEC_PORT (V4L2_CID_EXT_VDO_BASE + 1)

/**
 * @rst
 * This is control ID for connecting VDO to VDEC through fd opened by V4L2 in
 * VDO device minor number. VDO channel is set in 'vdo_port', and VDEC channel
 * is set in 'vdec_port', the VDO channel is connected to the VDEC channel
 * after invoking this CID.
 * Picture data is transferred to VSC only when VDO is connected to VDEC.
 * Therefore, Picture info event should not occur if VDO disconnected to VDEC.
 * Even after fd is closed, you should keep the last setting.
 *
 * VDO Connecting method for webOS 6.0
 *
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *      VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDO_VDEC_CONNECTING
 *
 *     //
 *     // control value
 *     //
 *     struct v4l2_ext_vdec_vdo_connection {
 *       __u32 vdo_port;
 *       __u32 vdec_port;
 *     };
 *     vdo_port =  0~1, VDO Port number, 0: VDO0 , 1: VDO1
 *     vdec_port = 0 ~ N , VDEC Port number
 *     // Get a fd from /dev/video28 to controll VDO0 and VDO1
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using  :c:func:`v4l-dvb-apis:VIDIOC_S_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_ext_controls ext_controls;
 *      v4l2_ext_control ext_control;
 *      v4l2_ext_vdec_vdo_connection vdo_con;
 *
 *      fd = open(V4L2_EXT_DEV_PATH_VDO0, O_RDWR);
 *
 *      memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *      memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *      memset(&vdo_con, 0, sizeof(struct v4l2_ext_vdec_vdo_connection));
 *      vdo_con.vdo_port = 0; //vdo port number
 *      vdo_con.vdec_port = 0 ; //vdec port number
 *
 *      ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_VDO_VDEC_CONNECTING;
 *      ext_controls.controls->ptr = (void*) &vdo_mode;
 *
 *      ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDO_VDEC_CONNECTING (V4L2_CID_EXT_VDO_BASE + 2)

/**
 * @rst
 * This is control ID for disconnecting VDO to VDEC through fd opened by V4L2 in
 * VDO device minor number. VDO channel is set in 'vdo_port', and VDEC channel
 * is set in 'vdec_port', the VDO channel is disconnected to the VDEC channel
 * after invoking this CID.
 * Picture data is transferred to VSC only when VDO is connected to VDEC. if
 * VDO don't connected to VDEC, Picture data is dropped when STC becomes the
 * same as the PTS. Therefore, Picture info event should not occur if VDO
 * disconnected to VDEC.
 * Even after fd is closed, you should keep the last setting.
 *
 * VDO Connecting method for webOS 6.0
 *
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *      VIDIOC_S_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_VDO_VDEC_DISCONNECTING
 *
 *     //
 *     // control value
 *     //
 *     struct v4l2_ext_vdec_vdo_connection {
 *       __u32 vdo_port;
 *       __u32 vdec_port;
 *     };
 *     vdo_port = 0;  // 0~1, VDO Port number, 0: VDO0 , 1: VDO1
 *     vdec_port = 0; // don't care.
 *
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately.
 *   The generic error codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using  :c:func:`v4l-dvb-apis:VIDIOC_S_EXT_CTRLS`
 *   and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      v4l2_ext_controls ext_controls;
 *      v4l2_ext_control ext_control;
 *      v4l2_ext_vdec_vdo_connection vdo_con;
 *
 *      fd = open(V4L2_EXT_DEV_PATH_VDO0, O_RDWR);
 *      memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *      memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *      memset(&vdo_con, 0, sizeof(struct v4l2_ext_vdec_vdo_connection));
 *
 *      vdo_con.vdo_port = 0; //vdo port number
 *      vdo_con.vdec_port = 0 ; //vdec port number
 *
 *      ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_VDO_VDEC_DISCONNECTING;
 *      ext_controls.controls->ptr = (void*) &vdo_mode;
 *
 *      ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_VDO_VDEC_DISCONNECTING (V4L2_CID_EXT_VDO_BASE + 3)

//
// videotexture class control IDs
//

/**
 * @brief Get the capabilities.
 *
 * @rst
 * It is a command to obtain Capture related Limitation and Capability
 * information inherent to SoC after a video device is opened. Recommend
 * V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV420_SEMI_PLANAR or
 * V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV422_SEMI_PLANAR for
 * v4l2_ext_capture_video_frame_buffer_pixel_format type, and
 * V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PLANE_SEMI_PLANAR for
 * v4l2_ext_capture_video_frame_buffer_plane_num.
 *
 * Explanation for flags
 *   * State 1 : While the interlace signal comes in the SoC.
 *   * State 2 : the frame size is 1280x720i@120hz(fps)
 *   * State 3 : the size of one field is 540.
 *   * State 4 : when user client captures some frame.
 *
 * #define V4L2_EXT_CAPTURE_CAP_INPUT_VIDEO_DEINTERLACE 0x0001
 *
 * - If the scan type of captured frame on V4L2_EXT_CAPTURE_SCALER_OUTPUT is
 *   progressive, you should set this bit in the variable of flags. The frame
 *   size of height is 720.
 * - If the scan type of captured frame on V4L2_EXT_CAPTURE_SCALER_OUTPUT is
 *   still interlace because the H/W block of capture position is located before
 *   the H/W block of de-interlace, you should unset this bit in the variable of
 *   flags. The frame size of height is 360.
 *
 * #define V4L2_EXT_CAPTURE_CAP_DISPLAY_VIDEO_DEINTERLACE 0x0002
 *
 * - If the scan type of captured frame on V4L2_EXT_CAPTURE_DISPLAY_OUTPUT is
 *   progressive, you should set this bit in the variable of flags. The frame
 *   size of height is 720.
 * - If the scan type of captured frame on V4L2_EXT_CAPTURE_DISPLAY_OUTPUT is
 *   still interlace because the H/W block of capture position is located before
 *   the H/W block of de-interlace, you should unset this bit in the variable of
 *   flags. The frame size of height is 360.
 *
 * #define V4L2_EXT_CAPTURE_CAP_SCALE_UP 0x0004
 *
 * - When user client request the size of capture frame with 1920x1080, if the
 *   vendor can capture those size after up-scaling, you should set this nit in
 *   the variable of flags.
 *
 * #define V4L2_EXT_CAPTURE_CAP_SCALE_DOWN 0x0008
 *
 * - When user client request the size of capture frame with 640x480, if the
 *   vendor can capture those size after down-scaling, you should set this nit
 *   in the variable of flags.
 *
 * #define V4L2_EXT_CAPTURE_CAP_DIVIDE_FRAMERATE 0x0010
 *
 * - When user client request to divide the fps of capture frame to 60hz or
 *   30hz, if the vendor can do dividing the fps, you should set this nit in the
 *   variable of flags.
 *
 * #define V4L2_EXT_CAPTURE_CAP_VIDEOTOVIDEO 0x0020
 *
 * - If the driver supports the feature of VTV, please set this bit to '1'.
 *   Currently LG chip only supports the VTV feature in the model specification
 *   on webOS5.0 of LG product. In case of other vendors, please set this bit to
 *   '0'.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_CAPABILITY_INFO
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_capture_video_frame_buffer_pixel_format
 *     {
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV420_PLANAR = 0,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV420_SEMI_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV420_INTERLEAVED,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV422_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV422_SEMI_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV422_INTERLEAVED,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV444_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV444_SEMI_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_YUV444_INTERLEAVED,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_RGB,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PIXEL_FORMAT_ARGB
 *     };
 *
 *     enum v4l2_ext_capture_video_frame_buffer_plane_num
 *     {
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PLANE_INTERLEAVED = 1,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PLANE_SEMI_PLANAR,
 *         V4L2_EXT_CAPTURE_VIDEO_FRAME_BUFFER_PLANE_PLANAR
 *     };
 *
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x; // horizontal start position
 *         unsigned short y; // vertical start position
 *         unsigned short w; // horizontal size(width)
 *         unsigned short h; // vertical size(height)
 *     };
 *
 *     struct v4l2_ext_capture_capability_info
 *     {
 *         unsigned int flags;
 *         unsigned int scale_up_limit_w;
 *         unsigned int scale_up_limit_h;
 *         unsigned int scale_down_limit_w;
 *         unsigned int scale_down_limit_h;
 *         unsigned int num_video_frame_buffer;
 *         struct v4l2_ext_video_rect max_res;
 *         enum v4l2_ext_capture_video_frame_buffer_plane_num num_plane;
 *         enum v4l2_ext_capture_video_frame_buffer_pixel_format pixel_format;
 *     };
 *
 *     // Flags for the 'flags' field in struct v4l2_ext_capture_capability_info
 *     #define V4L2_EXT_CAPTURE_CAP_INPUT_VIDEO_DEINTERLACE 0x0001
 *     #define V4L2_EXT_CAPTURE_CAP_DISPLAY_VIDEO_DEINTERLACE 0x0002
 *     #define V4L2_EXT_CAPTURE_CAP_SCALE_UP 0x0004
 *     #define V4L2_EXT_CAPTURE_CAP_SCALE_DOWN 0x0008
 *     #define V4L2_EXT_CAPTURE_CAP_DIVIDE_FRAMERATE 0x0010
 *     #define V4L2_EXT_CAPTURE_CAP_VIDEOTOVIDEO 0x0020
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     int ret;
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_capture_capability_info vt_capability_info;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&vt_capability_info, 0, sizeof(struct v4l2_ext_capture_capability_info));
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count      = 1;
 *     ext_controls.controls   = &ext_control;
 *
 *     ext_controls.controls->id = V4L2_CID_EXT_CAPTURE_CAPABILITY_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_capture_capability_info);
 *     ext_controls.controls->ptr = (void*)&vt_capability_info;
 *     ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_CAPABILITY_INFO                                   \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 0)

/**
 * @brief Get plane information.
 *
 * @rst
 * This is a command for obtaining information on a plane (VFOD (Video Frame
 * Output Device)). Basic plane size information and frame size information
 * stored in the plane are returned. This command can be used at any time in the
 * user client without any command or dependency when the VT Video Device is
 * open.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_PLANE_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x; // horizontal start position
 *         unsigned short y; // vertical start position
 *         unsigned short w; // horizontal size(width)
 *         unsigned short h; // vertical size(height)
 *     };
 *
 *    struct v4l2_ext_capture_plane_info
 *    {
 *        unsigned int stride;
 *        struct v4l2_ext_video_rect plane_region;
 *        struct v4l2_ext_video_rect active_region;
 *    };
 *
 * Explanation for struct
 *   1. unsigned int stride;
 *    - This value should be same with width or bigger than height.
 *
 *   2. struct v4l2_ext_video_rect plane_region;
 *    - Plane size in driver side. The value of x and y are always '0'.
 *
 *   3. struct v4l2_ext_video_rect active_region;
 *    - This values are current active size in plane.
 *      a. If plane size is 3840x2160 and the active size in plane buffer
 *      1920x540(interlace), you should return 1920x540 also x, y.
 *      b. If plane size is 3840x2160 and the active size in plane buffer
 *      1920x1080(progressive), you should return 1920x1080 also x, y.
 *      c. The status is that plane size is 3840x2160 and source input size is
 *      1920x1080(progressive) or 1920x540(interlace) and aspect ratio 4:3.
 *        - If the active size in plane buffer be calculated by applying a ratio
 *          of 4:3, you should return the calculated value.
 *        - If the active size in plane buffer is not related to aspect ratio,
 *          you should return the active size in plane buffer
 *          1920x1080(progressive) or 1920x540(interlace) also x, y.
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     struct v4l2_ext_capture_plane_info vt_palne_info;
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     int ret;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *
 *     ext_controls.ctrl_class     = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count          = 1;
 *     ext_controls.controls       = &ext_control;
 *     ext_controls.controls->id   = V4L2_CID_EXT_CAPTURE_PLANE_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_capture_plane_info);
 *     ext_controls.controls->ptr  = (void *)&vt_palne_info;
 *
 *     ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_PLANE_INFO (V4L2_CID_USER_EXT_CAPTURE_BASE + 1)

/**
 * @brief Set video window.
 *
 * @rst
 * This is a command for obtaining information on the current input, output
 * video, and panel size. This command can be used at any time in the User
 * Client without any command or dependency when the VT Video Device is open.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS (LG chip is only supported.)
 *     VIDIOC_G_EXT_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_VIDEO_WIN_INFO
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x; // horizontal start position
 *         unsigned short y; // vertical start position
 *         unsigned short w; // horizontal size(width)
 *         unsigned short h; // vertical size(height)
 *     };
 *
 *     enum v4l2_ext_capture_video_scan_type
 *     {
 *         V4L2_EXT_CAPTURE_VIDEO_INTERLACED = 0,
 *         V4L2_EXT_CAPTURE_VIDEO_PROGRESSIVE
 *     };
 *
 *     struct v4l2_ext_capture_video_win_info
 *     {
 *         enum v4l2_ext_capture_video_scan_type type;
 *         struct v4l2_ext_video_rect in;
 *         struct v4l2_ext_video_rect out;
 *         struct v4l2_ext_video_rect panel;
 *     };
 *
 * Explanation for struct
 *   User client can read(VIDIOC_G_EXT_CTLRS) only when user client set the
 *   capture location to V4L2_EXT_CAPTURE_SCALER_OUTPUT or
 *   V4L2_EXT_CAPTURE_DISPLAY_OUTPUT. In this case, the values of the structure
 *   are used as shown below.
 *
 *   1. struct v4l2_ext_video_rect in;
 *    - Current input resolution of scaler.
 *    - Read only
 *
 *   2. struct v4l2_ext_video_rect out;
 *    - Current output resolution of scaler.
 *    - Read only
 *
 *   3. struct v4l2_ext_video_rect panel;
 *    - The size of panel
 *    - Read only
 *
 *   User client can read(VIDIOC_G_EXT_CTLRS) 'in' and 'out', 'panel',
 *   set(VIDIOC_S_EXT_CTLRS) the value of 'out' when user client set the capture
 *   location to V4L2_EXT_CAPTURE_SCALER_INPUT(VTV). In this case, the values of
 *   the structure are used as shown below.
 *
 *   1. struct v4l2_ext_video_rect in;
 *    - Current input resolution of scaler.
 *    - Read only
 *
 *   2. struct v4l2_ext_video_rect out;
 *    - Current output size of vt output buffer.
 *    - Read, Write
 *
 *   3. struct v4l2_ext_video_rect panel;
 *    - The size of panel
 *    - Read only
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *   LG chip only supports the command of VIDIOC_S_EXT_CTRLS.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     struct v4l2_ext_capture_plane_info vt_palne_info;
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     int ret;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *
 *     ext_controls.ctrl_class     = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count          = 1;
 *     ext_controls.controls       = &ext_control;
 *     ext_controls.controls->id   = V4L2_CID_EXT_CAPTURE_PLANE_INFO;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_capture_plane_info);
 *     ext_controls.controls->ptr  = (void *)&vt_palne_info;
 *
 *     ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_VIDEO_WIN_INFO (V4L2_CID_USER_EXT_CAPTURE_BASE + 2)

/**
 * @brief Set/get properties.
 *
 * @rst
 * This command is used to set the position to be captured. The struct
 * v4l2_ext_video_rect plane and int buf_count are not referenced as reserved
 * areas. The LG chip only uses the value of struct v4l2_ext_video_rect plane
 * and int buf_count. Capture position uses the following structure defined by
 * user.
 *
 * Struct
 *   .. code-block:: cpp
 *
 *     enum v4l2_ext_capture_location
 *     {
 *         V4L2_EXT_CAPTURE_SCALER_INPUT = 0, // for VTV - before de-interlace
 *         V4L2_EXT_CAPTURE_SCALER_OUTPUT,
 *         V4L2_EXT_CAPTURE_DISPLAY_OUTPUT,
 *         V4L2_EXT_CAPTURE_BLENDED_OUTPUT,
 *         V4L2_EXT_CAPTURE_OSD_OUTPUT
 *     };
 *
 * Each position follows the following H / W block.
 *   .. image:: /v4l2/video-texture-V4L2_CID_EXT_CAPTURE_PLANE_PROP_v2.png
 * 1. V4L2_EXT_CAPTURE_SCALER_INPUT
 *   - LG uses this location only when VTV mode.
 *   - If not supported, the ioctl should be returned -1 and the errno variable should be set to ENOTSUP.
 * 2. V4L2_EXT_CAPTURE_SCALER_OUTPUT
 *   - Recommend location
 *      - Before PQ processing, after de-interlace block
 *      - The frame is a progressive type.
 *   - Alternative location due to chip limitation
 *      - Before PQ processing, before de-interlace block
 *      - The frame is an interlace type.
 * 3. V4L2_EXT_CAPTURE_DISPLAY_OUTPUT
 *   - Capture a video after PQ processing and before OSD blending.
 *   - The frame size calculated with ARC applied.
 * 4. V4L2_EXT_CAPTURE_BLENDED_OUTPUT
 *   - Capture a video after OSD blending.
 *   - If not supported, the ioctl should be returned -1 and the errno variable should be set to ENOTSUP.
 * 5. V4L2_EXT_CAPTURE_OSD_OUTPUT
 *   - Capture a OSD output.
 *   - If not supported, the ioctl should be returned -1 and the errno variable should be set to ENOTSUP.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTLRS
 *     VIDIOC_S_EXT_CTLRS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_PLANE_PROP
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_video_rect
 *     {
 *         unsigned short x; // horizontal start position
 *         unsigned short y; // vertical start position
 *         unsigned short w; // horizontal size(width)
 *         unsigned short h; // vertical size(height)
 *     };
 *
 *     enum v4l2_ext_capture_location
 *     {
 *         V4L2_EXT_CAPTURE_SCALER_INPUT = 0, // for VTV - before de-interlace
 *         V4L2_EXT_CAPTURE_SCALER_OUTPUT,
 *         V4L2_EXT_CAPTURE_DISPLAY_OUTPUT,
 *         V4L2_EXT_CAPTURE_BLENDED_OUTPUT,
 *         V4L2_EXT_CAPTURE_OSD_OUTPUT
 *     };
 *
 *     struct v4l2_ext_capture_plane_prop
 *     {
 *         enum v4l2_ext_capture_location l;
 *         struct v4l2_ext_video_rect plane;
 *         int buf_count;
 *     };
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS/VIDIOC_S_EXT_CTRLS and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     struct v4l2_ext_capture_plane_prop p;
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     int ret;
 *
 *     memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
 *     memset(&p, 0, sizeof(struct v4l2_ext_capture_plane_prop));
 *
 *     p.l = V4L2_EXT_CAPTURE_SCALER_OUTPUT;
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_CAPTURE_PLANE_PROP;
 *     ext_controls.controls->ptr = (void *)&p;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_PLANE_PROP (V4L2_CID_USER_EXT_CAPTURE_BASE + 3)

/**
 * @brief Set/get freeze.
 *
 * @rst
 * It is a freeze command to temporarily stop updating video frames in the
 * capture buffer.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTLRS
 *     VIDIOC_G_EXT_CTLRS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_FREEZE_MODE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_ext_capture_freeze_mode
 *     {
 *         int plane_index; //buffer index
 *         unsigned char val;
 *     }
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is
 *   set appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_S_EXT_CTRLS/VIDIOC_G_EXT_CTRLS and
 *   this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     struct v4l2_ext_controls ext_controls;
 *     struct v4l2_ext_control ext_control;
 *     struct v4l2_ext_capture_freeze_mode m;
 *
 *     memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *     memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *     memset(&m, 0, sizeof(struct v4l2_ext_capture_freeze_mode));
 *
 *     m.plane_index = index; m.val = 1; //on
 *
 *     ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_CAPTURE_FREEZE_MODE;
 *     ext_controls.controls->ptr = (void *)&m;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_FREEZE_MODE (V4L2_CID_USER_EXT_CAPTURE_BASE + 4)

/**
 * @brief Notify done processing to driver.
 *
 * @rst
 * It is a command used in VTV to process the dumped frame at the user level on
 * the platfom side and inform the completion of the processing.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_DONE_USER_PROCESSING (LG chip is only supported.)
 *
 *     //
 *     // parameter
 *     //
 *     plane_index //This index means vt output buffer index.
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is
 *   set appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *       struct  v4l2_control control;
 *       control.id = V4L2_CID_EXT_CAPTURE_DONE_USER_PROCESSING;
 *       control.value = plane_index;
 *       ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_DONE_USER_PROCESSING                              \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 5)

/**
 * @brief Get physial memory
 *
 * @rst
 * It is a command to read the physical address of the buffer through the buffer
 * index. If the user client sets the buffer index to the driver, the driver
 * sets the physical address to each of  ``y`` and ``c``. User client uses
 * V4L2_EXT_CAPTURE_INPUT_BUF to get the physical memory of input buffer when
 * VTG and VTV. In case of V4L2_EXT_CAPTURE_OUTPUT_BUF, user client uses that to
 * get the physical memory of input buffer when VTV. So if the vendor does not
 * support the feature of VTV, the vendor does not need to implement about
 * V4L2_EXT_CAPTURE_OUTPUT_BUF.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTLRS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_PHYSICAL_MEMORY_INFO
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_capture_buf_location
 *     {
 *         V4L2_EXT_CAPTURE_INPUT_BUF = 0,
 *         V4L2_EXT_CAPTURE_OUTPUT_BUF
 *     };
 *
 *     struct v4l2_ext_capture_physical_memory_info
 *     {
 *         int buf_index;
 *         union {
 *             unsigned int *y;
 *             unsigned int compat_y_data;
 *             unsigned long long y_sizer;
 *         };
 *
 *         union {
 *             unsigned int *c;
 *             unsigned int compat_c_data;
 *             unsigned long long c_sizer;
 *         };
 *         enum v4l2_ext_capture_buf_location buf_location;
 *     };
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_EXT_CTRLS and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *       struct v4l2_ext_controls ext_controls;
 *       struct v4l2_ext_control ext_control;
 *       struct v4l2_ext_capture_physical_memory_info info;
 *
 *       memset(&ext_controls, 0, sizeof(v4l2_ext_controls));
 *       memset(&ext_control, 0, sizeof(v4l2_ext_control));
 *       memset(&info, 0, sizeof(struct v4l2_ext_capture_physical_memory_info));
 *
 *       info.buf_index = index;
 *       info.y = malloc(sizeof(unsigned int));
 *       info.c = malloc(sizeof(unsigned int));
 *
 *       ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
 *       ext_controls.count = 1;
 *       ext_controls.controls = &ext_control;
 *       ext_controls.controls->id = V4L2_CID_EXT_CAPTURE_PHYSICAL_MEMORY_INFO;
 *       ext_controls.controls->ptr = (void *)&info;
 *
 *       ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_PHYSICAL_MEMORY_INFO                              \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 6)

/**
 * @brief Get output frame rate.
 *
 * @rst
 * It is a command for obtaining the output frame rate data of the buffer from
 * the user client.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_OUTPUT_FRAMERATE
 *
 *     //
 *     // parameter
 *     //
 *     frame rate (30 or 50 or 60 or 120 or etc..)
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *       struct v4l2_control control;
 *       control.id = V4L2_CID_EXT_CAPTURE_OUTPUT_FRAMERATE;
 *       ioctl(fd, VIDIOC_G_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_OUTPUT_FRAMERATE                                  \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 7)

/**
 * @brief Divide output frame rate.
 *
 * @rst
 * This function adjusts the output frame rate of the buffer. The default value
 * is 1; if the function is not supported, an error is returned. Below is an
 * example of using this command.
 *   - When the output buffer frame rate of the current buffer is 60Hz and the user
 *     client sets the DIVIDE_FRAMERATE value to 2, the final buffer output frame
 *     rate is 30Hz (60Hz / 2).
 *   - When the output buffer frame rate of current buffer is 120Hz and the user
 *     client sets DIVIDE_FRAMERATE value to 4, the final buffer output frame
 *     rate is 30Hz (120Hz / 4).
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRL
 *     VIDIOC_S_CTRL
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_CAPTURE_DIVIDE_FRAMERATE
 *
 *     //
 *     // parameter
 *     //
 *     divide value
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_S_CTRL/VIDIOC_G_CTRL and this control
 *   id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *       struct v4l2_control control;
 *       control.id = V4L2_CID_EXT_CAPTURE_DIVIDE_FRAMERATE;
 *       control.value = 2;
 *       ioctl(fd, VIDIOC_S_CTRL, &control);
 *
 * @endrst
 */
#define V4L2_CID_EXT_CAPTURE_DIVIDE_FRAMERATE                                  \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 8)

/* Capture class subscription IDs */
#define V4L2_CID_EXT_CAPTURE_SUBSCRIBE_FRAME_READY                             \
    (V4L2_CID_USER_EXT_CAPTURE_BASE + 30)

/* Capture class subscription types */
#define V4L2_EVENT_PRIVATE_EXT_CAPTURE_BASE (V4L2_EVENT_PRIVATE_START + 0x2100)
#define V4L2_EVENT_PRIVATE_EXT_CAPTURE_EVENT                                   \
    (V4L2_EVENT_PRIVATE_EXT_CAPTURE_BASE + 1)

/* Gpscaler class control IDs */
/**
 * @brief Set GPSCALER MAX FRAME SIZE
 *
 * @rst
 * This interface can set MAX frame size per GP Scaler instance to manage memory usage well. after setting MAX frame size, any frame size up to MAX frame size can be controlled at any time by VIDIOC_S_FMT for each GP Scaler instance.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_GPSCALER_MAX_FRAME_SIZE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_control_
 *     {
 *         __u32 id;   //Identifies the control, set by the application.
 *         __s32 value;//max width(up to 1920):upper 16bit/max height(up to 1080):lower 16bit
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control ctrl_arg;
 *
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_MAX_FRAME_SIZE;
 *     ctrl_arg.value = MAX_WIDTH << 16 | MAX_HEIGHT;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *     int ioctl(int fd, VIDIOC_S_CTRL, struct v4l2_control *argp)
 * @endrst
 */
#define V4L2_CID_EXT_GPSCALER_MAX_FRAME_SIZE                                   \
    (V4L2_CID_USER_EXT_GPSCALER_BASE + 1)

/**
 * @brief Get GPSCALER INPUT FRAME SIZE
 *
 * @rst
 * This interface can get input frame size per GP Scaler instance to support adaptive streaming.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_GPSCALER_INPUT_FRAME_SIZE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_control_
 *     {
 *         __u32 id;   //Identifies the control, set by the application.
 *         __s32 value;//Input width:upper 16bit/Input height:lower 16bit
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control ctrl_arg;
 *     int in_width, in_height;
 *
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_INPUT_FRAME_SIZE;
 *
 *     ioctl(fd, VIDIOC_G_CTRL, &ctrl_arg);
 *
 *     in_width = (ctrl_arg.value >> 16) & 0xffff;
 *     in_height = ctrl_arg.value & 0xffff;
 *
 * @endrst
 */
#define V4L2_CID_EXT_GPSCALER_INPUT_FRAME_SIZE                                 \
    (V4L2_CID_USER_EXT_GPSCALER_BASE + 2)

/**
 * @brief Set GPSCALER ACTIVE REGION DETECTION MODE
 *
 * @rst
 * This interface can set enable/disable active region detection function. This ioctl should be called after VIDIOC_S_INPUT when set active region detection mode on.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_MODE
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_control_
 *     {
 *         __u32 id;   //Identifies the control, set by the application.
 *         __s32 value;//1:enable/0:disable
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can set data using VIDIOC_S_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control ctrl_arg;
 *
 *     //Set active region detection mode on
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_MODE;
 *     ctrl_arg.value = 1;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 *     //Set active region detection mode off
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_MODE;
 *     ctrl_arg.value = 0;
 *
 *     ioctl(fd, VIDIOC_S_CTRL, &ctrl_arg);
 *
 * @endrst
 */
#define V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_MODE                     \
    (V4L2_CID_USER_EXT_GPSCALER_BASE + 3)

/**
 * @brief Get GPSCALER ACTIVE REGION DETECTION INFO HORIZONTAL
 *
 * @rst
 * This interface can get active region detection horizontal result. This ioctl should be called after active region detection mode on.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_H
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_control_
 *     {
 *         __u32 id;   //Identifies the control, set by the application.
 *         __s32 value;//x result:upper 16bit/width result:lower 16bit
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control ctrl_arg;
 *     int ard_x, ard_width;
 *
 *     //Get active region detection x and width result
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_H;
 *     ioctl(fd, VIDIOC_G_CTRL, &ctrl_arg);
 *
 *     ard_x = (ctrl_arg.value << 16) & 0xffff;
 *     ard_width = ctrl_arg.value & 0xffff;
 *
 * @endrst
 */
#define V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_H                   \
    (V4L2_CID_USER_EXT_GPSCALER_BASE + 4)

/**
 * @brief Get GPSCALER ACTIVE REGION DETECTION INFO VERTICAL
 *
 * @rst
 * This interface can get active region detection vertical result. This ioctl should be called after active region detection mode on.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_CTRLS
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_V
 *
 *     //
 *     // parameter
 *     //
 *     struct v4l2_control_
 *     {
 *         __u32 id;   //Identifies the control, set by the application.
 *         __s32 value;//y result:upper 16bit/height result:lower 16bit
 *     };
 *
 * Return Value
 *   On success 0 is returned.
 *
 *   On error -1 and the errno variable is set appropriately. The generic error
 *   codes are described at the :ref:`gen_errors` chapter.
 *
 * Control Type
 *   Application can get data using VIDIOC_G_CTRL and this control id.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     v4l2_control ctrl_arg;
 *     int ard_y, ard_height;
 *
 *     //Get active region detection y and height result
 *     ctrl_arg.id = V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_V;
 *     ioctl(fd, VIDIOC_G_CTRL, &ctrl_arg);
 *
 *     ard_y = (ctrl_arg.value << 16) & 0xffff;
 *     ard_height = ctrl_arg.value & 0xffff;
 *
 * @endrst
 */
#define V4L2_CID_EXT_GPSCALER_ACTIVE_REGION_DETECTION_INFO_V                   \
    (V4L2_CID_USER_EXT_GPSCALER_BASE + 5)

/* eARC class control IDs */
/**
 * @brief set eARC detection enable/disable.
 *
 * @rst
 * This control handles eARC Discovery and Disconnection enable/disable
 * controls.
 *
 * Ports may be specified when setting eARC enable/disable, or they may be set
 * without specifying. If you do not specify a specific port, set it to
 * :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE`.
 * If set to :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE`, the eARC driver must set the eARC
 * enable/disable on the port supporting eARC.
 * When calling a Get, if the Port is :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE`,
 * the driver must deliver the eARC supporting port and eARC enable/disable
 * states.
 * When set to :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE`, the driver must deliver the set
 * port's eARC enable/disable state.
 *
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set Control EARC
 *     VIDIOC_G_EXT_CTRLS      // Get Control EARC
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_EARC
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_earc_output_port {
 *          V4L2_EXT_EARC_OUTPUT_PORT_NONE = 0,
 *          V4L2_EXT_EARC_OUTPUT_PORT_1,
 *          V4L2_EXT_EARC_OUTPUT_PORT_2,
 *          V4L2_EXT_EARC_OUTPUT_PORT_3,
 *          V4L2_EXT_EARC_OUTPUT_PORT_4,
 *          V4L2_EXT_EARC_OUTPUT_PORT_ALL,
 *     };
 *     enum v4l2_ext_earc_enable {
 *          V4L2_EXT_EARC_DISABLE = 0,
 *          V4L2_EXT_EARC_ENABLE  = 1,
 *     };
 *
 *     struct v4l2_ext_earc {
 *           enum v4l2_ext_earc_output_port port;
 *           enum v4l2_ext_earc_enable earc_enable_state;
 *     };
 *
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   This control id supports both :c:macro:`VIDIOC_G_EXT_CTRLS` and :c:macro:`VIDIOC_S_EXT_CTRLS`.
 *
 * Example
 *    .. code-block:: cpp
 *
 *      // Set eARC enable/disable for port2
 *      v4l2_ext_controls ext_controls = {};
 *      v4l2_ext_control ext_control = {};
 *      struct v4l2_ext_earc earc = {};
 *
 *      earc.port = V4L2_EXT_EARC_OUTPUT_PORT_2;
 *      earc.earc_enable_state= V4L2_EXT_EARC_ENABLE;
 *      //earc.earc_enable_stat = V4L2_EXT_EARC_DISABLE;
 *
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_EARC;
 *      ext_controls.controls->size = sizeof(struct v4l2_ext_earc_enable );
 *      ext_controls.controls->ptr = (void *)&earc;
 *
 *      ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *      // Set eARC enable/disable for port to support eARC
 *      v4l2_ext_controls ext_controls = {};
 *      v4l2_ext_control ext_control = {};
 *      struct v4l2_ext_earc earc = {};
 *
 *      earc.port = V4L2_EXT_EARC_OUTPUT_PORT_NONE;
 *      earc.earc_enable_state= V4L2_EXT_EARC_ENABLE;
 *     //earc.earc_enable_stat = V4L2_EXT_EARC_DISABLE;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_EARC;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_earc_enable );
 *     ext_controls.controls->ptr = (void *)&earc;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Get eARC enable/disable status for Port2
 *     v4l2_ext_controls ext_controls = {};
 *     v4l2_ext_control ext_control = {};
 *     struct v4l2_ext_earc earc = {};
 *
 *     earc.port = V4L2_EXT_EARC_OUTPUT_PORT_2;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_EARC;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_earc_enable );
 *     ext_controls.controls->ptr = (void *)&earc;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Get eARC port and enable/disable status
 *     v4l2_ext_controls ext_controls = {};
 *     v4l2_ext_control ext_control = {};
 *     struct v4l2_ext_earc earc = {};
 *
 *     earc.port = V4L2_EXT_EARC_OUTPUT_PORT_NONE;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_EARC;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_earc_enable );
 *     ext_controls.controls->ptr = (void *)&earc;
 *
 *     ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_EARC (V4L2_CID_USER_EXT_EARC_BASE + 0)

/**
 * @brief Get eARC state, capability and RX latency.
 *
 * @rst
 * This function transmits the connection status, capability and
 * erx_latentity of the current eARC every time it is called.
 * When the port is called with the :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE` setting,
 * the lowest port information connected to the current eARC is transmitted.
 * When the port is set to :c:macro:`V4L2_EXT_EARC_OUTPUT_PORT_NONE`,
 * the set port eARC information is transmitted.
 * When the :c:macro:`V4L2_CID_EXT_EARC` is set to disable, everything is delivered to zero.
 * If the eARC is connected and the capability and erx_latency are not ready,
 * the capability and erx_latency are delivered to zero.
 * erx_latentity_req delivers the currently set value. If not set, it delivers 0.
 * Driver should be implemented so that there is no problem even if it is
 * called every 50ms.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_G_EXT_CTRLS      // Get Control EARC
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_EARC_CONNECTION_INFO
 *
 *     //
 *     // parameter
 *     //
 *     #define V4L2_EXT_EARC_ERX_LATENCY_REQ_MINIMIZE 0
 *     // 0: requeset to minimize eARC RX latency
 *     #define V4L2_EXT_EARC_ERX_LATENCY_REQ_NON_SYNC 254
 *     // 254: No syncronization required
 *     #define V4L2_EXT_EARC_ERX_LATENCY_REQ_UNKNOWN  255
 *     // 255: unknown eARC TX latency
 *
 *     enum v4l2_ext_earc_output_port {
 *           V4L2_EXT_EARC_OUTPUT_PORT_NONE = 0,
 *           V4L2_EXT_EARC_OUTPUT_PORT_1,
 *           V4L2_EXT_EARC_OUTPUT_PORT_2,
 *           V4L2_EXT_EARC_OUTPUT_PORT_3,
 *           V4L2_EXT_EARC_OUTPUT_PORT_4,
 *           V4L2_EXT_EARC_OUTPUT_PORT_ALL,
 *     };
 *
 *     enum v4l2_ext_earc_status {
 *          V4L2_EXT_EARC_IDLE1 = 0,
 *          V4L2_EXT_EARC_IDLE2 = 1,
 *          V4L2_EXT_EARC_DISC1 = 2,
 *          V4L2_EXT_EARC_DISC2 = 3,
 *          V4L2_EXT_EARC_EARC  = 4,
 *     };
 *
 *     struct v4l2_ext_earc_connection_info {
 *            enum v4l2_ext_earc_output_port port;
 *            enum v4l2_ext_earc_status status;
 *            unsigned char capability[V4L2_EXT_EARC_CAPABILITY_BYTES];
 *     //Short Audio Descriptior Max 256 byte
 *            unsigned char erx_latency_req;     //0~255
 *            unsigned char erx_latency;     //0~255
 *     };
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   This control id supports only :c:macro:`VIDIOC_G_EXT_CTRLS`.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      // Get eARC connection information for port2
 *      v4l2_ext_controls ext_controls = {};
 *      v4l2_ext_control ext_control = {};
 *      v4l2_ext_earc_connection_info earc_connection = {};
 *
 *      earc_connection.port = V4L2_EXT_EARC_OUTPUT_PORT_2;
 *
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_EARC_CONNECTION_INFO;
 *      ext_controls.controls->size = sizeof(struct v4l2_ext_earc_connection_info );
 *      ext_controls.controls->ptr = (void *)&earc_connection;
 *
 *      ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 *
 *      // Get eARC connection information for eARC port which is connected and supported.
 *      v4l2_ext_controls ext_controls = {};
 *      v4l2_ext_control ext_control = {};
 *      v4l2_ext_earc_connection_info earc_connection = {};
 *
 *      earc_connection.port = V4L2_EXT_EARC_OUTPUT_PORT_NONE;
 *
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_EARC_CONNECTION_INFO;
 *      ext_controls.controls->size = sizeof(struct v4l2_ext_earc_connection_info );
 *      ext_controls.controls->ptr = (void *)&earc_connection;
 *
 *      ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_EARC_CONNECTION_INFO (V4L2_CID_USER_EXT_EARC_BASE + 1)

/**
 * @brief Set ERX_LATENCY_REQ.
 *
 * @rst
 * This function sets erx_latentity_req in the connection of eARC.
 * erx_latentity_req is :c:macro:`ERX_LATENCY_REQ` defined
 * in HDMI Specification 2.1 document 9.5.3.5 Audio Latency Control.
 * The eARC RX sets the output audio display by referring to :c:macro:`ERX_LATENCY_REQ`.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set Control EARC
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_EARC_SET_ERX_LATENCY_REQ
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_earc_output_port {
 *            V4L2_EXT_EARC_OUTPUT_PORT_NONE = 0,
 *            V4L2_EXT_EARC_OUTPUT_PORT_1,
 *            V4L2_EXT_EARC_OUTPUT_PORT_2,
 *            V4L2_EXT_EARC_OUTPUT_PORT_3,
 *            V4L2_EXT_EARC_OUTPUT_PORT_4,
 *            V4L2_EXT_EARC_OUTPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_earc_connection_info {
 *            enum v4l2_ext_earc_output_port port;
 *            enum v4l2_ext_earc_status status;
 *            unsigned char capability[V4L2_EXT_EARC_CAPABILITY_BYTES];
 *      //Short Audio Descriptior Max 256 byte
 *            unsigned char erx_latency_req;     //0~255
 *            unsigned char erx_latency;     //0~255
 *     };
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   This control id supports only :c:macro:`VIDIOC_S_EXT_CTRLS`.
 *
 * Example
 *   .. code-block:: cpp
 *
 *     // Set eARC ERX_LATENCY_REQ as minmize delay for port2
 *     v4l2_ext_controls ext_controls = {};
 *     v4l2_ext_control ext_control = {};
 *     v4l2_ext_earc_connection_info earc_connection = {};
 *
 *     earc_connection.port = V4L2_EXT_EARC_OUTPUT_PORT_2;
 *     earc_connection.erx_latency_req = V4L2_EXT_EARC_ERX_LATENCY_REQ_MINIMIZE;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_EARC_SET_ERX_LATENCY_REQ;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_earc_connection_info );
 *     ext_controls.controls->ptr = (void *)&earc_connection;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 *
 *     // Set eARC ERX_LATENCY_REQ as 100ms delay for eARC port which is connected and supported.
 *     v4l2_ext_controls ext_controls = {};
 *     v4l2_ext_control ext_control = {};
 *     v4l2_ext_earc_connection_info earc_connection = {};
 *
 *     earc_connection.port = V4L2_EXT_EARC_OUTPUT_PORT_NONE;
 *     earc_connection.erx_latency_req = 100;
 *
 *     ext_controls.count = 1;
 *     ext_controls.controls = &ext_control;
 *     ext_controls.controls->id = V4L2_CID_EXT_EARC_SET_ERX_LATENCY_REQ;
 *     ext_controls.controls->size = sizeof(struct v4l2_ext_earc_connection_info );
 *     ext_controls.controls->ptr = (void *)&earc_connection;
 *
 *     ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_EARC_SET_ERX_LATENCY_REQ (V4L2_CID_USER_EXT_EARC_BASE + 2)

/**
 * @brief Reset eARC HDMI_HPD bit reset
 *
 * @rst
 * This function is to reset HDMI_bit during eARC connection status(TX eARC).
 * LG TV(eARC TX) does use eARC HDMI_HPD bit reset to notify eARC RX of EDID
 * update instead of Physical HPD reset.
 * If Physical HPD reset is used for eARC connected HDMI Port, the connection
 * and disconnection of eARC is infinitely repeated.
 * If eARC HDMI_HPD bit is used for eARC connected HDMI Port, it is possible to
 * notify eARC RX of EDID update without  the infinite repetition connection
 * disconnection.
 * When LG MW call :c:macro:`V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT`, driver should check
 * whether eARC connection status is TX eARC, and if the connection status is
 * TX eARC, driver should reset  HDMI_HPD bit (Clear/Set), otherwise driver
 * should do nothing.
 * For backward compatibility, if :c:macro:`V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT` is not
 * implemented in driver yet, when LG MW call
 * :c:macro:`V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT`, driver should do nothing without any
 * problem.
 * HDMI_HPD bit is defined in HDMI Specification 2.1 document 9.5.3.4 Heartbeat
 * Sequence and Status Bits and 9.5.3.6 eARC RX Capabilities Data Structure.
 * All eARC V4L2 Driver Interfaces  should support multi instance.
 *
 * Commands & Parameters
 *   .. code-block:: cpp
 *
 *     //
 *     // ioctl command
 *     //
 *     VIDIOC_S_EXT_CTRLS      // Set Control EARC
 *
 *     //
 *     // control id
 *     //
 *     // You have to make the new control id as the below.
 *     V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT
 *
 *     //
 *     // parameter
 *     //
 *     enum v4l2_ext_earc_output_port {
 *            V4L2_EXT_EARC_OUTPUT_PORT_NONE = 0,
 *            V4L2_EXT_EARC_OUTPUT_PORT_1,
 *            V4L2_EXT_EARC_OUTPUT_PORT_2,
 *            V4L2_EXT_EARC_OUTPUT_PORT_3,
 *            V4L2_EXT_EARC_OUTPUT_PORT_4,
 *            V4L2_EXT_EARC_OUTPUT_PORT_ALL,
 *     };
 *
 *     struct v4l2_ext_earc_connection_info {
 *            enum v4l2_ext_earc_output_port port;
 *            enum v4l2_ext_earc_status status;
 *            unsigned char capability[V4L2_EXT_EARC_CAPABILITY_BYTES];
 *     //Short Audio Descriptior Max 256 byte
 *            unsigned char erx_latency_req;     //0~255
 *            unsigned char erx_latency;     //0~255
 *     };
 *
 * Return Value
 *   On success 0 is returned. On error -1 and the errno variable is set
 *   appropriately. The generic error codes are described at the
 *   :ref:`gen_errors` chapter.
 *
 * Control Type
 *   This control id supports only :c:macro:`VIDIOC_S_EXT_CTRLS`.
 *
 * Example
 *   .. code-block:: cpp
 *
 *      // Set eARC ERX_LATENCY_REQ as minmize delay for port2
 *      v4l2_ext_controls ext_controls = {};
 *      v4l2_ext_control ext_control = {};
 *      v4l2_ext_earc_connection_info earc_connection = {};
 *
 *      earc_connection.port = V4L2_EXT_EARC_OUTPUT_PORT_NONE;
 *
 *      ext_controls.count = 1;
 *      ext_controls.controls = &ext_control;
 *      ext_controls.controls->id = V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT;
 *      ext_controls.controls->size = sizeof(struct v4l2_ext_earc_connection_info );
 *      ext_controls.controls->ptr = (void *)&earc_connection;
 *
 *      ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
 *
 * @endrst
 */
#define V4L2_CID_EXT_EARC_RESET_HDMI_HPD_BIT (V4L2_CID_USER_EXT_EARC_BASE + 3)

#endif
