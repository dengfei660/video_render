/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "videotunnel_lib_wrap.h"
#include "Logger.h"
#include "ErrorCode.h"

#define TAG "rlib:videotunnel_lib_wrap"

#define VIDEOTUNNEL_LIB_NAME "libvideotunnel.so"

VideotunnelLib * videotunnelLoadLib(int logCategory)
{
    INFO(logCategory, "load videotunel so symbol");

    VideotunnelLib *handle = (VideotunnelLib*)calloc( 1, sizeof(VideotunnelLib));
    if (!handle) {
        ERROR(logCategory, "calloc VideotunnelLib struct fail");
        goto err_labal;
    }
    handle->libHandle = dlopen(VIDEOTUNNEL_LIB_NAME, RTLD_NOW);
    if (handle->libHandle == NULL) {
        ERROR(logCategory, "unable to dlopen %s : %s",VIDEOTUNNEL_LIB_NAME, dlerror());
        goto err_labal;
    }

    handle->vtOpen = (vt_open)dlsym(handle->libHandle, "meson_vt_open");
    if (handle->vtOpen == NULL) {
        ERROR(logCategory,"dlsym meson_vt_open failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtClose = (vt_close)dlsym(handle->libHandle, "meson_vt_close");
    if (handle->vtClose == NULL) {
        ERROR(logCategory,"dlsym meson_vt_close failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtAllocId = (vt_alloc_id)dlsym(handle->libHandle, "meson_vt_alloc_id");
    if (handle->vtAllocId == NULL) {
        ERROR(logCategory, "dlsym meson_vt_alloc_id failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtFreeId = (vt_free_id)dlsym(handle->libHandle, "meson_vt_free_id");
    if (handle->vtFreeId == NULL) {
        ERROR(logCategory,"dlsym meson_vt_free_id failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtConnect = (vt_connect)dlsym(handle->libHandle, "meson_vt_connect");
    if (handle->vtConnect == NULL) {
        ERROR(logCategory,"dlsym meson_vt_connect failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtDisconnect = (vt_disconnect)dlsym(handle->libHandle, "meson_vt_disconnect");
    if (handle->vtDisconnect == NULL) {
        ERROR(logCategory,"dlsym meson_vt_disconnect failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtQueueBuffer = (vt_queue_buffer)dlsym(handle->libHandle, "meson_vt_queue_buffer");
    if (handle->vtQueueBuffer == NULL) {
        ERROR(logCategory,"dlsym meson_vt_queue_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtDequeueBuffer = (vt_dequeue_buffer)dlsym(handle->libHandle, "meson_vt_dequeue_buffer");
    if (handle->vtDequeueBuffer == NULL) {
        ERROR(logCategory,"dlsym meson_vt_dequeue_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtCancelBuffer = (vt_cancel_buffer)dlsym(handle->libHandle, "meson_vt_cancel_buffer");
    if (handle->vtCancelBuffer == NULL) {
        ERROR(logCategory,"dlsym meson_vt_cancel_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSetSourceCrop = (vt_set_sourceCrop)dlsym(handle->libHandle, "meson_vt_set_sourceCrop");
    if (handle->vtSetSourceCrop == NULL) {
        ERROR(logCategory,"dlsym meson_vt_set_sourceCrop failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtGetDisplayVsyncAndPeriod = (vt_getDisplayVsyncAndPeriod)dlsym(handle->libHandle, "meson_vt_getDisplayVsyncAndPeriod");
    if (handle->vtGetDisplayVsyncAndPeriod == NULL) {
        ERROR(logCategory,"dlsym meson_vt_getDisplayVsyncAndPeriod failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSetMode = (vt_set_mode)dlsym(handle->libHandle, "meson_vt_set_mode");
    if (handle->vtSetMode == NULL) {
        ERROR(logCategory,"dlsym meson_vt_set_mode failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSendCmd = (vt_send_cmd)dlsym(handle->libHandle, "meson_vt_send_cmd");
    if (handle->vtSendCmd == NULL) {
        ERROR(logCategory,"dlsym meson_vt_send_cmd failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtRecvCmd = (vt_recv_cmd)dlsym(handle->libHandle, "meson_vt_recv_cmd");
    if (handle->vtRecvCmd == NULL) {
        ERROR(logCategory,"dlsym meson_vt_recv_cmd failed, err=%s \n", dlerror());
        goto err_labal;
    }

    return handle;
err_labal:
    if (handle) {
        if (handle->libHandle) {
            dlclose(handle->libHandle);
        }
        free(handle);
    }
    return NULL;
}
int videotunnelUnloadLib(int logCategory,VideotunnelLib *vt)
{
    INFO(logCategory, "unload videotunel so symbol");
    if (vt) {
        if (vt->libHandle) {
            dlclose(vt->libHandle);
        }
        free(vt);
    }
    return NO_ERROR;
}