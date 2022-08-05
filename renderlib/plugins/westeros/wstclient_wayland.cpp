/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <cstring>
#include "wstclient_wayland.h"
#include "ErrorCode.h"
#include "Logger.h"
#include "wstclient_plugin.h"

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define WESTEROS_UNUSED(x) ((void)(x))

#define TAG "rlib:wstclient_wayland"

#define DEFAULT_WINDOW_X (0)
#define DEFAULT_WINDOW_Y (0)
#define DEFAULT_WINDOW_WIDTH (1920)
#define DEFAULT_WINDOW_HEIGHT (1080)

#define AV_SYNC_SESSION_V_MONO 64

enum
{
    ZOOM_NONE,
    ZOOM_DIRECT,
    ZOOM_NORMAL,
    ZOOM_16_9_STRETCH,
    ZOOM_4_3_PILLARBOX,
    ZOOM_ZOOM,
    ZOOM_GLOBAL
};

#define needBounds() (mForceAspectRatio || (mZoomMode != ZOOM_NONE))

void WstClientWayland::shellSurfaceId(void *data,
                           struct wl_simple_shell *wl_simple_shell,
                           struct wl_surface *surface,
                           uint32_t surfaceId)
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);
    self->mWlShellSurfaceId = surfaceId;
    char name[32];
    wl_fixed_t z, op;
    WESTEROS_UNUSED(wl_simple_shell);
    WESTEROS_UNUSED(surface);

    sprintf( name, "westeros-surface-%x", surfaceId );
    wl_simple_shell_set_name(self->mWlShell, surfaceId, name);
    if ( (self->mWindowWidth == 0) || (self->mWindowHeight == 0) )
    {
        wl_simple_shell_set_visible(self->mWlShell, self->mWlShellSurfaceId, false);
    }
    else
    {
        wl_simple_shell_set_visible(self->mWlShell, self->mWlShellSurfaceId, true);
        if (!self->mWlVpc)
        {
            wl_simple_shell_set_geometry(self->mWlShell, self->mWlShellSurfaceId, self->mWindowX, self->mWindowY, self->mWindowWidth, self->mWindowHeight );
        }
    }

    z = wl_fixed_from_double(self->mZorder);
    wl_simple_shell_set_zorder(self->mWlShell, self->mWlShellSurfaceId, z);
    op = wl_fixed_from_double(self->mOpacity);
    wl_simple_shell_set_opacity(self->mWlShell, self->mWlShellSurfaceId, op);
    wl_simple_shell_get_status(self->mWlShell, self->mWlShellSurfaceId);

    wl_display_flush(self->mWlDisplay);
}

void WstClientWayland::shellSurfaceCreated(void *data,
                                struct wl_simple_shell *wl_simple_shell,
                                uint32_t surfaceId,
                                const char *name)
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(wl_simple_shell);
    WESTEROS_UNUSED(surfaceId);
    WESTEROS_UNUSED(name);
}

void WstClientWayland::shellSurfaceDestroyed(void *data,
                                  struct wl_simple_shell *wl_simple_shell,
                                  uint32_t surfaceId,
                                  const char *name)
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(wl_simple_shell);
    WESTEROS_UNUSED(surfaceId);
    WESTEROS_UNUSED(name);
}

void WstClientWayland::shellSurfaceStatus(void *data,
                               struct wl_simple_shell *wl_simple_shell,
                               uint32_t surfaceId,
                               const char *name,
                               uint32_t visible,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               wl_fixed_t opacity,
                               wl_fixed_t zorder)
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);
    WESTEROS_UNUSED(wl_simple_shell);
    WESTEROS_UNUSED(surfaceId);
    WESTEROS_UNUSED(name);
    WESTEROS_UNUSED(x);
    WESTEROS_UNUSED(y);
    WESTEROS_UNUSED(width);
    WESTEROS_UNUSED(height);

    INFO(self->mLogCategory,"opacity: %d,zorder:%d", opacity,zorder);
    self->mWindowChange = true;
    self->mOpacity = opacity;
    self->mZorder = zorder;
}

void WstClientWayland::shellGetSurfacesDone(void *data, struct wl_simple_shell *wl_simple_shell)
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(wl_simple_shell);
}

static const struct wl_simple_shell_listener shellListener =
{
    WstClientWayland::shellSurfaceId,
    WstClientWayland::shellSurfaceCreated,
    WstClientWayland::shellSurfaceDestroyed,
    WstClientWayland::shellSurfaceStatus,
    WstClientWayland::shellGetSurfacesDone
};

void WstClientWayland::vpcVideoPathChange(void *data,
                               struct wl_vpc_surface *wl_vpc_surface,
                               uint32_t new_pathway )
{
    WESTEROS_UNUSED(wl_vpc_surface);
    WstClientWayland *self = static_cast<WstClientWayland *>(data);

    INFO(self->mLogCategory,"new pathway: %d\n", new_pathway);
    self->setVideoPath(new_pathway == WL_VPC_SURFACE_PATHWAY_GRAPHICS);
}

void WstClientWayland::vpcVideoXformChange(void *data,
                                struct wl_vpc_surface *wl_vpc_surface,
                                int32_t x_translation,
                                int32_t y_translation,
                                uint32_t x_scale_num,
                                uint32_t x_scale_denom,
                                uint32_t y_scale_num,
                                uint32_t y_scale_denom,
                                uint32_t output_width,
                                uint32_t output_height)
{
    WESTEROS_UNUSED(wl_vpc_surface);
    WstClientWayland *self = static_cast<WstClientWayland *>(data);

    TRACE1(self->mLogCategory,"x_trans:%d,y_trans:%d,x_scale_num:%d,x_scale_denom:%d,y_scale_num:%d,y_scale_denom:%d",\
            x_translation,y_translation,x_scale_num,x_scale_denom,y_scale_num,y_scale_denom);
    TRACE1(self->mLogCategory,"output_width:%d,output_height:%d",output_width,output_height);
    self->mTransX= x_translation;
    self->mTransY= y_translation;
    if ( x_scale_denom )
    {
        self->mScaleXNum= x_scale_num;
        self->mScaleXDenom= x_scale_denom;
    }
    if ( y_scale_denom )
    {
        self->mScaleYNum = y_scale_num;
        self->mScaleYDenom = y_scale_denom;
    }
    self->mOutputWidth= (int)output_width;
    self->mOutputHeight= (int)output_height;

    Tls::Mutex::Autolock _l(self->mMutex);
    self->updateVideoPosition();
}

static const struct wl_vpc_surface_listener vpcListener= {
    WstClientWayland::vpcVideoPathChange,
    WstClientWayland::vpcVideoXformChange
};

void WstClientWayland::outputHandleGeometry( void *data,
                                  struct wl_output *output,
                                  int x,
                                  int y,
                                  int mmWidth,
                                  int mmHeight,
                                  int subPixel,
                                  const char *make,
                                  const char *model,
                                  int transform )
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(output);
    WESTEROS_UNUSED(x);
    WESTEROS_UNUSED(y);
    WESTEROS_UNUSED(mmWidth);
    WESTEROS_UNUSED(mmHeight);
    WESTEROS_UNUSED(subPixel);
    WESTEROS_UNUSED(make);
    WESTEROS_UNUSED(model);
    WESTEROS_UNUSED(transform);
}

void WstClientWayland::outputHandleMode( void *data,
                              struct wl_output *output,
                              uint32_t flags,
                              int width,
                              int height,
                              int refreshRate )
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);

    if ( flags & WL_OUTPUT_MODE_CURRENT )
    {
        Tls::Mutex::Autolock _l(self->mMutex);
        self->mDisplayWidth = width;
        self->mDisplayHeight = height;
        DEBUG(self->mLogCategory,"compositor sets window to (%dx%d)\n", width, height);
        if (!self->mWindowSet)
        {
            self->mWindowWidth = width;
            self->mWindowHeight = height;
            if (self->mWlVpcSurface)
            {
                TRACE1(self->mLogCategory,"set window geometry:x:%d,y:%d,w:%d,h:%d",self->mWindowX,self->mWindowY,self->mWindowWidth,self->mWindowHeight);
                wl_vpc_surface_set_geometry(self->mWlVpcSurface, self->mWindowX, self->mWindowY, self->mWindowWidth, self->mWindowHeight);
            }
        }
    }
}

void WstClientWayland::outputHandleDone( void *data,
                              struct wl_output *output )
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(output);
}

void WstClientWayland::outputHandleScale( void *data,
                               struct wl_output *output,
                               int32_t scale )
{
    WESTEROS_UNUSED(data);
    WESTEROS_UNUSED(output);
    WESTEROS_UNUSED(scale);
}

static const struct wl_output_listener outputListener = {
    WstClientWayland::outputHandleGeometry,
    WstClientWayland::outputHandleMode,
    WstClientWayland::outputHandleDone,
    WstClientWayland::outputHandleScale
};

void WstClientWayland::sbFormat(void *data, struct wl_sb *wl_sb, uint32_t format)
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);
    WESTEROS_UNUSED(wl_sb);
    WESTEROS_UNUSED(data);
    TRACE2(self->mLogCategory,"registry: sbFormat: %X\n", format);
}

static const struct wl_sb_listener sbListener = {
    WstClientWayland::sbFormat
};

void
WstClientWayland::registryHandleGlobal (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);
    TRACE1(self->mLogCategory,"registryHandleGlobal,interface:%s,version:%d",interface,version);

    if (strcmp (interface, "wl_compositor") == 0) {
        self->mWlCompositor = (struct wl_compositor *)wl_registry_bind (registry, id, &wl_compositor_interface, 1/*MIN (version, 3)*/);
        wl_proxy_set_queue((struct wl_proxy*)self->mWlCompositor, self->mWlQueue);
        TRACE2(self->mLogCategory,"registry: compositor %p\n", (void*)self->mWlCompositor);
    } else if (strcmp (interface, "wl_simple_shell") == 0) {
        self->mWlShell = (struct wl_simple_shell*)wl_registry_bind(registry, id, &wl_simple_shell_interface, 1);
        wl_proxy_set_queue((struct wl_proxy*)self->mWlShell, self->mWlQueue);
        wl_simple_shell_add_listener(self->mWlShell, &shellListener, (void *)self);
        TRACE2(self->mLogCategory,"registry: simple shell %p\n", (void*)self->mWlShell);
    } else if (strcmp (interface, "wl_vpc") == 0) {
        self->mWlVpc = (struct wl_vpc*)wl_registry_bind(registry, id, &wl_vpc_interface, 1);
        wl_proxy_set_queue((struct wl_proxy*)self->mWlVpc, self->mWlQueue);
        TRACE2(self->mLogCategory,"registry: vpc %p\n", (void*)self->mWlVpc);
    } else if (strcmp (interface, "wl_output") == 0) {
        self->mWlOutput = (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
        wl_proxy_set_queue((struct wl_proxy*)self->mWlOutput, self->mWlQueue);
        wl_output_add_listener(self->mWlOutput, &outputListener, (void *)self);
        TRACE2(self->mLogCategory,"registry: output %p\n", (void*)self->mWlOutput);
    } else if (strcmp (interface, "wl_sb") == 0) {
        self->mWlSb = (struct wl_sb*)wl_registry_bind(registry, id, &wl_sb_interface, version);
        wl_proxy_set_queue((struct wl_proxy*)self->mWlSb, self->mWlQueue);
        wl_sb_add_listener(self->mWlSb, &sbListener, (void *)self);
        TRACE2(self->mLogCategory,"registry: sb %p\n", (void*)self->mWlSb);
    }
}

void
WstClientWayland::registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name)
{
    WstClientWayland *self = static_cast<WstClientWayland *>(data);
    /* temporarily do nothing */
    DEBUG(self->mLogCategory,"wayland display remove registry handle global");
}

static const struct wl_registry_listener registry_listener = {
    WstClientWayland::registryHandleGlobal,
    WstClientWayland::registryHandleGlobalRemove
};

WstClientWayland::WstClientWayland(WstClientPlugin *plugin, int logCategory)
    :mMutex("bufferMutex"),
    mPlugin(plugin),
    mLogCategory(logCategory)
{
    TRACE2(mLogCategory,"construct WstClientWayland");
    mWlDisplay = NULL;
    mWlQueue = NULL;
    mWlRegistry = NULL;
    mWlCompositor = NULL;
    mWlSurface = NULL;
    mWlVpc = NULL;
    mWlVpcSurface = NULL;
    mWlOutput = NULL;
    mWlShell = NULL;
    mWlSb = NULL;
    mDisplayWidth = -1;
    mDisplayHeight = -1;
    mWindowX = DEFAULT_WINDOW_X;
    mWindowY = DEFAULT_WINDOW_Y;
    mWindowWidth = DEFAULT_WINDOW_WIDTH;
    mWindowHeight = DEFAULT_WINDOW_HEIGHT;
    mVideoX = mWindowX;
    mVideoY = mWindowY;
    mVideoWidth = mWindowWidth;
    mVideoHeight = mWindowHeight;
    mWlShellSurfaceId = 0;
    mWindowChange = false;
    mWindowSet = false;
    mWindowSizeOverride = false;
    mZoomMode = ZOOM_NONE;
    mZoomModeGlobal = false;
    mAllow4kZoom = false;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mForceAspectRatio = false;
    mPixelAspectRatio = 1.0;
    mScaleXDenom = 0;
    mScaleYDenom = 0;
    mPoll = new Tls::Poll(true);
}

WstClientWayland::~WstClientWayland()
{
    TRACE2(mLogCategory,"deconstruct WstClientWayland");
    if (mPoll) {
        delete mPoll;
        mPoll = NULL;
    }
}

int WstClientWayland::connectToWayland()
{
    DEBUG(mLogCategory,"in");
    mWlDisplay = wl_display_connect(NULL);
    if (!mWlDisplay) {
        mWlDisplay = wl_display_connect("test-0");
        if (!mWlDisplay) {
            FATAL(mLogCategory,"Failed to connect to the wayland display, try wayland-0");
            mWlDisplay = wl_display_connect("wayland-0");
            if (!mWlDisplay) {
                ERROR(mLogCategory, "wayland connect fail");
                return ERROR_OPEN_FAIL;
            }
            INFO(mLogCategory,"wayland connect to wayland-0");
        }
        INFO(mLogCategory,"wayland connect to test-0");
    }

    mWlQueue = wl_display_create_queue (mWlDisplay);
    wl_proxy_set_queue ((struct wl_proxy *)mWlDisplay, mWlQueue);

    mWlRegistry = wl_display_get_registry (mWlDisplay);
    wl_registry_add_listener (mWlRegistry, &registry_listener, (void *)this);

    /* we need exactly 2 roundtrips to discover global objects and their state */
    for (int i = 0; i < 2; i++) {
        if (wl_display_roundtrip_queue (mWlDisplay, mWlQueue) < 0) {
            FATAL(mLogCategory,"Error communicating with the wayland display");
            return ERROR_OPEN_FAIL;
        }
    }

    //create surface from compositor
    if (mWlCompositor) {
        mWlSurface = wl_compositor_create_surface(mWlCompositor);
        wl_proxy_set_queue((struct wl_proxy*)mWlSurface, mWlQueue);
        wl_display_flush( mWlDisplay );
    }

    if (mWlVpc && mWlSurface) {
        mWlVpcSurface = wl_vpc_get_vpc_surface( mWlVpc, mWlSurface );
        if (mWlVpcSurface) {
            wl_vpc_surface_add_listener( mWlVpcSurface, &vpcListener, (void *)this);
            wl_proxy_set_queue((struct wl_proxy*)mWlVpcSurface, mWlQueue);
            wl_vpc_surface_set_geometry( mWlVpcSurface, mWindowX, mWindowY, mWindowWidth, mWindowHeight);
            wl_display_flush(mWlDisplay);
        }
    }

    DEBUG(mLogCategory,"out");
    return NO_ERROR;
}

void WstClientWayland::disconnectFromWayland()
{
    DEBUG(mLogCategory,"in");

   if (isRunning()) {
        TRACE1(mLogCategory,"try stop dispatch thread");
        if (mPoll) {
            mPoll->setFlushing(true);
        }
        requestExitAndWait();
    }

    if (mWlShell) {
        wl_simple_shell_destroy(mWlShell);
        mWlShell = NULL;
    }

    if (mWlVpcSurface) {
        wl_vpc_surface_destroy(mWlVpcSurface);
        mWlVpcSurface = NULL;
    }

    if (mWlVpc) {
        wl_vpc_destroy(mWlVpc);
        mWlVpc = NULL;
    }

    if (mWlSurface) {
        wl_surface_destroy(mWlSurface);
        mWlSurface = NULL;
    }

    if (mWlOutput) {
        wl_output_destroy(mWlOutput);
        mWlOutput = NULL;
    }

    if (mWlSb) {
        wl_sb_destroy(mWlSb);
        mWlSb = NULL;
    }

    if (mWlCompositor) {
        wl_compositor_destroy (mWlCompositor);
        mWlCompositor = NULL;
    }

    if (mWlRegistry) {
        wl_registry_destroy (mWlRegistry);
        mWlRegistry= NULL;
    }

    if (mWlQueue) {
        wl_event_queue_destroy (mWlQueue);
        mWlQueue = NULL;
    }

    if (mWlDisplay) {
        wl_display_flush (mWlDisplay);
        wl_display_disconnect (mWlDisplay);
        mWlDisplay = NULL;
    }

    DEBUG(mLogCategory,"out");
}

void WstClientWayland::setWindowSize(int x, int y, int w, int h)
{
    if (x == 0 && y == 0 && w == 0 && h == 0) {
        WARNING(mLogCategory, "set full screen? %dx%dx%dx%d",x,y,w,h);
        return;
    }
    mWindowX = x;
    mWindowY = y;
    mWindowWidth = w;
    mWindowHeight = h;
    mWindowChange = true;
    mWindowSet = true;
    mWindowSizeOverride = true;
    DEBUG(mLogCategory,"set window size:x:%d,y:%d,w:%d,h:%d",x,y,w,h);
    if (mWlVpcSurface) {
        DEBUG(mLogCategory, "wl_vpc_surface_set_geometry(%d,%d,%d,%d)",mWindowX,mWindowY,mWindowWidth,mWindowHeight);
        wl_vpc_surface_set_geometry(mWlVpcSurface, mWindowX, mWindowY, mWindowWidth, mWindowHeight);
    }
    //if window size is updated, update video position
    if (mWlVpcSurface && mWindowChange && mScaleXDenom != 0 && mScaleYDenom != 0) {
        mWindowChange = false;
        updateVideoPosition();
    }
}

void WstClientWayland::setFrameSize(int frameWidth, int frameHeight) {
    mFrameWidth = frameWidth;
    mFrameHeight = frameHeight;
    DEBUG(mLogCategory, "set frame size:%dx%d",mFrameWidth, mFrameHeight);
}

void WstClientWayland::setZoomMode(int zoomMode, bool globalZoomActive, bool allow4kZoom) {
    DEBUG(mLogCategory, "zoomMode:%d, globalZoomActive:%d, allow4kZoom:%d",zoomMode,globalZoomActive,allow4kZoom);
    mZoomModeGlobal = globalZoomActive;
    if ( !globalZoomActive )
    {
        mZoomMode = ZOOM_NONE;
    }
    mAllow4kZoom= allow4kZoom;
    if ( mZoomModeGlobal == true )
    {
        if ( (zoomMode >= ZOOM_NONE) && (zoomMode <= ZOOM_ZOOM) )
        {
            mZoomMode = zoomMode;
            mPixelAspectRatioChanged = true;
        }
    } else {
        DEBUG(mLogCategory, "global zoom disabled: ignore server value");
    }
}

void WstClientWayland::getVideoBounds(int *x, int *y, int *w, int *h)
{
    int vx, vy, vw, vh;
    int frameWidth, frameHeight;
    int zoomMode;
    double contentWidth, contentHeight;
    double roix, roiy, roiw, roih;
    double arf, ard;
    double hfactor= 1.0, vfactor= 1.0;

    //if wayland open fail,use the default window size
    if (!mWlDisplay) {
        *x = mWindowX;
        *y = mWindowY;
        *w = mWindowWidth;
        *h = mWindowHeight;
        TRACE3(mLogCategory, "no wldisplay, %d,%d,%d,%d",mVideoX,mVideoY,mVideoWidth,mVideoHeight);
        return;
    }

    //if window size is updated, update video position
    if (mWlVpcSurface && mWindowChange) {
        mWindowChange = false;
        updateVideoPosition();
    }

    vx = mVideoX;
    vy = mVideoY;
    vw = mVideoWidth;
    vh = mVideoHeight;

    TRACE3(mLogCategory, "videoX:%d,videoY:%d,videoW:%d,videoH:%d",mVideoX,mVideoY,mVideoWidth,mVideoHeight);

    frameWidth = mFrameWidth;
    frameHeight = mFrameHeight;
    contentWidth = frameWidth * mPixelAspectRatio;
    contentHeight = frameHeight;

    if (mPixelAspectRatioChanged)
        DEBUG(mLogCategory,"pixelAspectRatio: %f zoom-mode %d", mPixelAspectRatio, mZoomMode);

    ard = (double)mVideoWidth/(double)mVideoHeight;
    arf = (double)contentWidth/(double)contentHeight;

    TRACE3(mLogCategory, "frameWidth:%d,frameHeight:%d,contentWidth:%f,contentHeight:%f",frameWidth,frameHeight,contentWidth,contentHeight);

    /* Establish region of interest */
    roix = 0;
    roiy = 0;
    roiw = contentWidth;
    roih = contentHeight;

    zoomMode = mZoomMode;
    if ((mFrameWidth > 1920) || (mFrameHeight > 1080))
    {
        zoomMode= ZOOM_NORMAL;
    }
    //if (mPixelAspectRatioChanged )
        DEBUG(mLogCategory,"ard %f arf %f", ard, arf);
    switch ( zoomMode )
    {
        case ZOOM_NORMAL:
        {
            if ( arf >= ard )
            {
                vw = mVideoWidth * (1.0);
                vh = (roih * vw) / roiw;
                vx = vx+(mVideoWidth-vw)/2;
                vy = vy+(mVideoHeight-vh)/2;
            }
            else
            {
                vh = mVideoHeight * (1.0);
                vw = (roiw * vh) / roih;
                vx = vx+(mVideoWidth-vw)/2;
                vy = vy+(mVideoHeight-vh)/2;
            }
        }
        break;
        case ZOOM_NONE:
        case ZOOM_DIRECT:
        {
            if ( arf >= ard )
            {
                vh = (contentHeight * mVideoWidth) / contentWidth;
                vy = vy+(mVideoHeight-vh)/2;
            }
            else
            {
                vw = (contentWidth * mVideoHeight) / contentHeight;
                vx = vx+(mVideoWidth-vw)/2;
            }
        }
        break;
        case ZOOM_16_9_STRETCH:
        {
            if ( approxEqual(arf, ard) && approxEqual(arf, 1.777) )
            {
                /* For 16:9 content on a 16:9 display, stretch as though 4:3 */
                hfactor= 4.0/3.0;
                if (mPixelAspectRatioChanged ) DEBUG(mLogCategory,"stretch apply vfactor %f hfactor %f", vfactor, hfactor);
            }
            vh = mVideoHeight * (1.0);
            vw = vh*hfactor*16/9;
            vx = vx+(mVideoWidth-vw)/2;
            vy = vy+(mVideoHeight-vh)/2;
        }
        break;
        case ZOOM_4_3_PILLARBOX:
        {
            vh = mVideoHeight * (1.0);
            vw = vh*4/3;
            vx = vx+(mVideoWidth-vw)/2;
            vy = vy+(mVideoHeight-vh)/2;
        }
        break;
        case ZOOM_ZOOM:
        {
            if ( arf >= ard )
            {
                if (approxEqual(arf, ard) && approxEqual( arf, 1.777) )
                {
                    /* For 16:9 content on a 16:9 display, enlarge as though 4:3 */
                    vfactor= 4.0/3.0;
                    hfactor= 1.0;
                    if (mPixelAspectRatioChanged ) DEBUG(mLogCategory,"zoom apply vfactor %f hfactor %f", vfactor, hfactor);
                }
                vh = mVideoHeight * vfactor * (1.0);
                vw = (roiw * vh) * hfactor / roih;
                vx = vx+(mVideoWidth-vw)/2;
                vy = vy+(mVideoHeight-vh)/2;
            }
            else
            {
                vw = mVideoWidth * (1.0);
                vh = (roih * vw) / roiw;
                vx = vx+(mVideoWidth-vw)/2;
                vy = vy+(mVideoHeight-vh)/2;
            }
        }
        break;
    }
    if (mPixelAspectRatioChanged) DEBUG(mLogCategory,"vrect %d, %d, %d, %d", vx, vy, vw, vh);
    if (mPixelAspectRatioChanged)
    {
        if (mWlDisplay && mWlVpcSurface )
        {
            wl_vpc_surface_set_geometry(mWlVpcSurface, mWindowX, mWindowY, mWindowWidth, mWindowHeight);
            wl_display_flush(mWlDisplay);
        }
    }
    mPixelAspectRatioChanged = false;
    *x= vx;
    *y= vy;
    *w= vw;
    *h= vh;
    TRACE3(mLogCategory,"vrect %d, %d, %d, %d", vx, vy, vw, vh);
}

void WstClientWayland::setTextureCrop(int vx, int vy, int vw, int vh)
{
    DEBUG(mLogCategory,"vx %d vy %d vw %d vh %d window(%d, %d, %d, %d) display(%dx%d)",
             vx, vy, vw, vh, mWindowX, mWindowY, mWindowWidth, mWindowHeight, mDisplayWidth, mDisplayHeight);
    if ( (mDisplayWidth != -1) && (mDisplayHeight != -1) &&
        ( (vx < 0) || (vx+vw > mDisplayWidth) ||
          (vy < 0) || (vy+vh > mDisplayHeight) ) )
    {
        int cropx, cropy, cropw, croph;
        int wx1, wx2, wy1, wy2;
        cropx= 0;
        cropw= mWindowWidth;
        cropy= 0;
        croph= mWindowHeight;
        if ( (vx < mWindowX) || (vx+vw > mWindowX+mWindowWidth) )
        {
            cropx= (mWindowX-vx)*mWindowWidth/vw;
            cropw= (mWindowX+mWindowWidth-vx)*mWindowWidth/vw - cropx;
        }
        else if ( vx < 0 )
        {
            cropx= -vx*mWindowWidth/vw;
            cropw= (vw+vx)*mWindowWidth/vw;
        }
        else if ( vx+vw > mWindowWidth )
        {
            cropx= 0;
            cropw= (mWindowWidth-vx)*mWindowWidth/vw;
        }

        if ( (vy < mWindowY) || (vy+vh > mWindowY+mWindowHeight) )
        {
            cropy= (mWindowY-vy)*mWindowHeight/vh;
            croph= (mWindowY+mWindowHeight-vy)*mWindowHeight/vh - cropy;
        }
        else if ( vy < 0 )
        {
            cropy= -vy*mWindowHeight/vh;
            croph= (vh+vy)*mWindowHeight/vh;
        }
        else if ( vy+vh > mWindowHeight )
        {
            cropy= 0;
            croph= (mWindowHeight-vy)*mWindowHeight/vh;
        }

        wx1 = vx;
        wx2 = vx+vw;
        wy1 = vy;
        wy2 = vy+vh;
        vx = mWindowX;
        vy = mWindowY;
        vw = mWindowWidth;
        vh = mWindowHeight;
        if ( (wx1 > vx) && (wx1 > 0) )
        {
            vx= wx1;
        }
        else if ( (wx1 >= vx) && (wx1 < 0) )
        {
            vw += wx1;
            vx= 0;
        }
        else if ( wx2 < vx+vw )
        {
            vw= wx2-vx;
        }
        if ( (wx1 >= 0) && (wx2 > vw) )
        {
            vw= vw-wx1;
        }
        else if ( wx2 < vx+vw )
        {
            vw= wx2-vx;
        }

        if ( (wy1 > vy) && (wy1 > 0) )
        {
            vy= wy1;
        }
        else if ( (wy1 >= vy) && (wy1 < 0) )
        {
            vy= 0;
        }
        else if ( (wy1 < vy) && (wy1 > 0) )
        {
            vh -= wy1;
        }
        if ( (wy1 >= 0) && (wy2 > vh) )
        {
            vh= vh-wy1;
        }
        else if ( wy2 < vy+vh )
        {
            vh= wy2-vy;
        }
        if ( vw < 0 ) vw= 0;
        if ( vh < 0 ) vh= 0;
        cropx= (cropx*WL_VPC_SURFACE_CROP_DENOM)/mWindowWidth;
        cropy= (cropy*WL_VPC_SURFACE_CROP_DENOM)/mWindowHeight;
        cropw= (cropw*WL_VPC_SURFACE_CROP_DENOM)/mWindowWidth;
        croph= (croph*WL_VPC_SURFACE_CROP_DENOM)/mWindowHeight;
        DEBUG(mLogCategory,"%d, %d, %d, %d - %d, %d, %d, %d", vx, vy, vw, vh, cropx, cropy, cropw, croph);
        if (mWlVpcSurface) {
            wl_vpc_surface_set_geometry_with_crop(mWlVpcSurface, vx, vy, vw, vh, cropx, cropy, cropw, croph );
        }
    }
    else
    {
        if (mWlVpcSurface) {
            wl_vpc_surface_set_geometry(mWlVpcSurface, mWindowX, mWindowY, mWindowWidth, mWindowHeight);
        }
    }
}

void WstClientWayland::setForceAspectRatio(bool force)
{
    DEBUG(mLogCategory, "force aspect ratio:%d",force);
    mForceAspectRatio = force;
}

void WstClientWayland::updateVideoPosition()
{
    bool needUpdate= true;
    int vx, vy, vw, vh;
    vx= mVideoX;
    vy= mVideoY;
    vw= mVideoWidth;
    vh= mVideoHeight;

    if (mWindowSizeOverride)
    {
        mVideoX= ((mWindowX*mScaleXNum)/mScaleXDenom) + mTransX;
        mVideoY= ((mWindowY*mScaleYNum)/mScaleYDenom) + mTransY;
        mVideoWidth= (mWindowWidth*mScaleXNum)/mScaleXDenom;
        mVideoHeight= (mWindowHeight*mScaleYNum)/mScaleYDenom;
        DEBUG(mLogCategory, "window override video rectangle(%d,%d,%d,%d)",mVideoX,mVideoY,mVideoWidth,mVideoHeight);
    }
    else
    {
        mVideoX = mTransX;
        mVideoY = mTransY;
        mVideoWidth = (mOutputWidth*mScaleXNum)/mScaleXDenom;
        mVideoHeight = (mOutputHeight*mScaleYNum)/mScaleYDenom;
        DEBUG(mLogCategory, "video rectangle(%d,%d,%d,%d)",mVideoX,mVideoY,mVideoWidth,mVideoHeight);
    }

    if (vx == mVideoX && vy == mVideoY && vw == mVideoWidth && vh == mVideoHeight) {
        needUpdate = false;
    }

    if (needUpdate) {
        /* Send a buffer to compositor to update hole punch geometry */
        if (mWlSb)
        {
            struct wl_buffer *buff;

            buff = wl_sb_create_buffer( mWlSb,
                                        0,
                                        mWindowWidth,
                                        mWindowHeight,
                                        mWindowWidth*4,
                                        WL_SB_FORMAT_ARGB8888 );
            wl_surface_attach(mWlSurface, buff, mWindowX, mWindowY);
            wl_surface_damage(mWlSurface, 0, 0, mWindowWidth, mWindowHeight);
            wl_surface_commit(mWlSurface);
        }
        if (mVideoPaused && mPlugin)
        {
            mPlugin->setVideoRect(mVideoX, mVideoY, mVideoWidth, mVideoHeight);
        }
    }
}

void WstClientWayland::setVideoPath(bool useGfxPath )
{
    INFO(mLogCategory,"useGfxPath:%d",useGfxPath);
    if ( needBounds() && mWlVpcSurface )
    {
        /* Use nominal display size provided to us by
        * the compositor to calculate the video bounds
        * we should use when we transition to graphics path.
        * Save and restore current HW video rectangle. */
        int vx, vy, vw, vh;
        int tx, ty, tw, th;
        tx = mVideoX;
        ty = mVideoY;
        tw = mVideoWidth;
        th = mVideoHeight;
        mVideoX = mWindowX;
        mVideoY = mWindowY;
        mVideoWidth = mWindowWidth;
        mVideoHeight = mWindowHeight;

        DEBUG(mLogCategory, "video rect (%d,%d,%d,%d)",mVideoX,mVideoY,mVideoWidth,mVideoHeight);

        if (mFrameWidth > 0 && mFrameHeight > 0) {
            getVideoBounds(&vx, &vy, &vw, &vh);
            setTextureCrop(vx, vy, vw, vh);
        }

        mVideoX = tx;
        mVideoY = ty;
        mVideoWidth = tw;
        mVideoHeight = th;
    }
}

bool WstClientWayland::approxEqual( double v1, double v2 )
{
    bool result= false;
    if ( v1 >= v2 )
    {
        if ( (v1-v2) < 0.001 )
        {
            result= true;
        }
    }
    else
    {
        if ( (v2-v1) < 0.001 )
        {
            result= true;
        }
    }
    return result;
}

void WstClientWayland::readyToRun()
{
    mFd = wl_display_get_fd (mWlDisplay);
    if (mPoll) {
        mPoll->addFd(mFd);
        mPoll->setFdReadable(mFd, true);
    }
}

bool WstClientWayland::threadLoop()
{
    struct pollfd pfd;
    int ret;

    while (wl_display_prepare_read_queue (mWlDisplay, mWlQueue) != 0) {
      wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    }

    wl_display_flush (mWlDisplay);

    /*poll timeout value must > 300 ms,otherwise zwp_linux_dmabuf will create failed,
     so do use -1 to wait for ever*/
    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING(mLogCategory,"poll error");
        wl_display_cancel_read(mWlDisplay);
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    if (wl_display_read_events (mWlDisplay) == -1) {
        goto tag_error;
    }

    wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    return true;
tag_error:
    ERROR(mLogCategory,"Error communicating with the wayland server");
    return false;
}