/******************************************************************************
 *   TV LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 *   Copyright(c) 2018 by LG Electronics Inc.
 *
 *   All rights reserved. No part of this work may be reproduced, stored in a
 *   retrieval system, or transmitted by any means without prior written
 *   permission of LG Electronics Inc.
 *****************************************************************************/

/** @file v4l2-lge-frontend.h
 *
 *  It is the header file for controling v4l2 frontend driver.
 *
 *  @author     Jinsuk Choi(jjinsuk.choi@lge.com)
 *  @version    1.0
 *  @date       2018.07.04
 *  @note
 *  @see
 */

#ifndef _V4L2_LGE_FRONTEND_H
#define _V4L2_LGE_FRONTEND_H

#include "linuxtv-ext-ver.h"
#include <linux/v4l2-controls.h>

#define V4L2_CID_EXT_USER_FRONTEND_BASE                                        \
    (V4L2_CID_USER_BASE + 0x9000) // (0x00980000|0x900) + 0x9000
#define V4L2_CID_EXT_FRONTEND_G_TUNERLOCK (V4L2_CID_EXT_USER_FRONTEND_BASE + 0)
#define V4L2_CID_EXT_FRONTEND_G_DEMOD_FW_VERSION                               \
    (V4L2_CID_EXT_USER_FRONTEND_BASE + 1)
#define V4L2_CID_EXT_FRONTEND_G_SIGNALSTATUS                                   \
    (V4L2_CID_EXT_USER_FRONTEND_BASE + 2)
#define V4L2_CID_EXT_FRONTEND_G_ATVTUNEPOSTJOB                                 \
    (V4L2_CID_EXT_USER_FRONTEND_BASE + 3)
#define V4L2_CID_EXT_FRONTEND_S_CTRL_ATVINIT                                   \
    (V4L2_CID_EXT_USER_FRONTEND_BASE + 4)

#endif /* _V4L2_LGE_FRONTEND_H */
