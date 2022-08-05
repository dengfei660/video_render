/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "Logger.h"
#include "renderlib_wrap.h"

#define TAG "rlib:renderlib_wrap"

RenderLibWrap::RenderLibWrap(uint32_t vdecPort, uint32_t vdoPort) :
    mVdecPort(vdecPort),
    mVdoPort(vdoPort)
{
    mRenderLibHandle = NULL;
    mCallback.notifyFrameDisplayed = NULL;
    mCallback.notifyFrameRelease = NULL;
}

RenderLibWrap::~RenderLibWrap()
{
    mRenderLibHandle = NULL;
    mCallback.notifyFrameDisplayed = NULL;
    mCallback.notifyFrameRelease = NULL;
}

void RenderLibWrap::setCallback(void *userData, RenderLibWrapCallback *callback)
{
    mUserData = userData;
    mCallback.notifyFrameDisplayed = callback->notifyFrameDisplayed;
    mCallback.notifyFrameRelease = callback->notifyFrameRelease;
}

bool RenderLibWrap::connectRender(char *name,int videotunnelId)
{
    int ret = 0;

    DEBUG(NO_CATEGERY,"in");
    mRenderLibHandle = render_open(name);
    if (!mRenderLibHandle) {
        ERROR(NO_CATEGERY,"open render lib fail");
        return false;
    }

    RenderCallback renderCallback;
    renderCallback.doMsgSend = RenderLibWrap::doSend;
    renderCallback.doGetValue = RenderLibWrap::doGet;

    render_set_user_data(mRenderLibHandle, (void *)this);

    render_set_callback(mRenderLibHandle, &renderCallback);

    if (videotunnelId >= 0) {
        render_set(mRenderLibHandle, KEY_VIDEOTUNNEL_ID, &videotunnelId);
    }

    ret = render_connect(mRenderLibHandle);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib connect fail");
        return false;
    }

    DEBUG(NO_CATEGERY,"out");

    return true;
}

bool RenderLibWrap::disconnectRender()
{
    int ret;
    DEBUG(NO_CATEGERY,"in");

    ret = render_disconnect(mRenderLibHandle);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib disconnect fail");
        return false;
    }

    render_close(mRenderLibHandle);
    mRenderLibHandle = NULL;

    DEBUG(NO_CATEGERY,"out");
    return true;
}

bool RenderLibWrap::renderFrame(RenderBuffer *buffer)
{
    int ret = 0;

    ret = render_display_frame(mRenderLibHandle, buffer);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib display frame fail");
        return false;
    }
    return true;
}

void RenderLibWrap::setWindowSize(int x, int y, int w, int h)
{
    int ret;
    RenderWindowSize rect;
    DEBUG(NO_CATEGERY,"window size:%d,%d,%d,%d",x,y,w,h);

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    ret = render_set(mRenderLibHandle, KEY_WINDOW_SIZE, (void *)&rect);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set window size fail");
    }
}

void RenderLibWrap::setFrameSize(int frameWidth, int frameHeight)
{
    int ret;
    RenderFrameSize frameSize;
    DEBUG(NO_CATEGERY,"frame size:%dx%d",frameWidth,frameHeight);

    frameSize.frameWidth = frameWidth;
    frameSize.frameHeight = frameHeight;

    ret = render_set(mRenderLibHandle, KEY_FRAME_SIZE, (void *)&frameSize);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set frame size fail");
    }
}

void RenderLibWrap::setMediasyncId(int id)
{
    int ret;
    DEBUG(NO_CATEGERY,"mediasync id:%d",id);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_INSTANCE_ID, (void *)&id);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync id fail");
    }
}

void RenderLibWrap::setPlaybackRate(float rate)
{
    int ret;
    DEBUG(NO_CATEGERY,"playback rate:%f",rate);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_PLAYBACK_RATE, (void *)&rate);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync id fail");
    }
}

void RenderLibWrap::setMediasyncSyncMode(int mode)
{
    int ret;
    DEBUG(NO_CATEGERY,"mediasync sync mode:%d",mode);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_SYNC_MODE, (void *)&mode);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync sync mode fail");
    }
}

void RenderLibWrap::setMediasyncPcrPid(int id)
{
    int ret;
    DEBUG(NO_CATEGERY,"set mediasync pcr id:%d",id);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_PCR_PID, (void *)&id);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync pcr id fail");
    }
}

void RenderLibWrap::setMediasyncDemuxId(int id)
{
    int ret;
    DEBUG(NO_CATEGERY,"set mediasync demux id:%d",id);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_DEMUX_ID, (void *)&id);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync demux id fail");
    }
}

void RenderLibWrap::setMediasyncTunnelMode(int mode)
{
    int ret;
    DEBUG(NO_CATEGERY,"set mediasync tunnelmode :%d",mode);

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_TUNNEL_MODE, (void *)&mode);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set mediasync tunnelmode fail");
    }
}

void RenderLibWrap::setVideoFormat(int format)
{
    int ret;
    DEBUG(NO_CATEGERY,"video pixel format:%d",format);

    ret = render_set(mRenderLibHandle, KEY_VIDEO_FORMAT, (void *)&format);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set format fail");
    }
}

void RenderLibWrap::setVideoFps(int num, int denom)
{
    int ret;
    int64_t fps;

    fps = (int64_t)(((int64_t)(num) << 32) | denom);
    DEBUG(NO_CATEGERY,"vide fps:num:%d(%x),denom:%d(%x)",num,num,denom,denom);

    ret = render_set(mRenderLibHandle, KEY_VIDEO_FPS, (void *)&fps);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set fps fail");
    }
}

void RenderLibWrap::setMediasyncThreshold(int threshold)
{
    int ret;

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_STARTTHRESHOLD, (void *)&threshold);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set threshold fail");
    }
}

void RenderLibWrap::setMediasyncDisplayLatency(int latency)
{
    int ret;

    ret = render_set(mRenderLibHandle, KEY_MEDIASYNC_VIDEOLATENCY, (void *)&latency);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib set threshold fail");
    }
}

void RenderLibWrap::getDroppedFrames(int *cnt)
{
    render_get(mRenderLibHandle, KEY_FRAME_DROPPED, cnt);
}

bool RenderLibWrap::flush()
{
    int ret;
    DEBUG(NO_CATEGERY,"flush");
    ret = render_flush(mRenderLibHandle);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib flush fail");
        return false;
    }
    return true;
}

bool RenderLibWrap::pause()
{
    int ret;
    DEBUG(NO_CATEGERY,"pause");
    ret = render_pause(mRenderLibHandle);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib pause fail");
        return false;
    }
    return true;
}

bool RenderLibWrap::resume()
{
    int ret;
    DEBUG(NO_CATEGERY,"resume");
    ret = render_resume(mRenderLibHandle);
    if (ret) {
        ERROR(NO_CATEGERY,"render lib pause fail");
        return false;
    }

    return true;
}

RenderBuffer* RenderLibWrap::allocRenderBuffer()
{
    return render_allocate_render_buffer_wrap(mRenderLibHandle, BUFFER_FLAG_EXTER_DMA_BUFFER, 0);
}

void RenderLibWrap::releaseRenderBuffer(RenderBuffer* buffer)
{
    render_free_render_buffer_wrap(mRenderLibHandle, buffer);
}

void RenderLibWrap::doSend(void *userData , RenderMsgType type, void *msg)
{
    RenderLibWrap *self = static_cast<RenderLibWrap *>(userData);
    switch (type)
    {
        case MSG_RELEASE_BUFFER: {
            RenderBuffer *buffer = (RenderBuffer *)msg;
            if (self->mCallback.notifyFrameDisplayed) {
                self->mCallback.notifyFrameRelease(self->mUserData, buffer);
            }
        } break;
        case MSG_DISPLAYED_BUFFER: {
            RenderBuffer *buffer = (RenderBuffer *)msg;
            if (self->mCallback.notifyFrameRelease) {
                self->mCallback.notifyFrameDisplayed(self->mUserData, buffer);
            }
        } break;
        default:
            break;
    }
}

int RenderLibWrap::doGet(void *userData, int key, void *value)
{
    RenderLibWrap *self = static_cast<RenderLibWrap *>(userData);

    if (key == KEY_MEDIASYNC_INSTANCE_ID) {
        *(int *)value = -1;
    }

    return 0;
}