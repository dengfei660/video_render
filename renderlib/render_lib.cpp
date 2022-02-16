#include <string.h>
#include "render_lib.h"
#include "render_core.h"
#include "Logger.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define TAG "rlib:render_lib"
#define VERSION "V1.1.0"

void* render_open(char *name) {
    //open log file
    char *env = getenv("VIDEO_RENDER_LOG_FILE");
    if (env && strlen(env) > 0) {
        Logger_set_file(env);
        INFO("VIDEO_RENDER_LOG_FILE=%s",env);
    }
    //set log level
    env = getenv("VIDEO_RENDER_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO("VIDEO_RENDER_LOG_LEVEL=%d",level);
    }

    INFO("build version:%s",VERSION);

    INFO("open");
    RenderCore *render = new RenderCore();
    render->init(name);
    return (void*)render;
}

void render_set_callback(void *handle, RenderCallback *callback)
{
    static_cast<RenderCore *>(handle)->setCallback(callback);
}

void render_set_user_data(void *handle, void *userdata)
{
    static_cast<RenderCore *>(handle)->setUserData(userdata);
}

int render_connect(void *handle)
{
    return static_cast<RenderCore *>(handle)->connect();
}

int render_display_frame(void *handle, RenderBuffer *buffer)
{
    return static_cast<RenderCore *>(handle)->displayFrame(buffer);
}

int render_set(void *handle, int key, void *value)
{
    return static_cast<RenderCore *>(handle)->setProp(key, value);
}

int render_get(void *handle, int key, void *value)
{
    return static_cast<RenderCore *>(handle)->getProp(key, value);
}

int render_flush(void *handle)
{
    return static_cast<RenderCore *>(handle)->flush();
}

int render_pause(void *handle)
{
    return static_cast<RenderCore *>(handle)->pause();
}

int render_resume(void *handle)
{
    return static_cast<RenderCore *>(handle)->resume();
}

int render_disconnect(void *handle)
{
    return static_cast<RenderCore *>(handle)->disconnect();
}

int render_close(void *handle)
{
    static_cast<RenderCore *>(handle)->release();
    delete static_cast<RenderCore *>(handle);
    INFO("close end");
    Logger_set_file(NULL);
    return 0;
}

RenderBuffer *render_allocate_render_buffer_wrap(void *handle, int flag, int rawBufferSize)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);

    RenderBuffer *renderbuf = renderCore->getFreeRenderBuffer();
    if (!renderbuf) {
        renderbuf = renderCore->allocRenderBufferWrap(flag, rawBufferSize);
        if (renderbuf) {
            renderCore->addRenderBuffer(renderbuf);
            return renderbuf;
        }
    }

    /*if the free render buffer contain raw buffer
    realloc it*/
    if ((renderbuf->flag & BUFFER_FLAG_ALLOCATE_RAW_BUFFER)) {
        renderbuf->raw.dataPtr = realloc(renderbuf->raw.dataPtr, rawBufferSize);
    }

    return renderbuf;
}

void render_free_render_buffer_wrap(void *handle, RenderBuffer *buffer)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    RenderBuffer *renderbuf = renderCore->findRenderBuffer(buffer);
    if (renderbuf) { //set free if find this special buffer
        renderCore->setRenderBufferFree(buffer);
    } else { //free buffer if not found ????
        renderCore->releaseRenderBufferWrap(buffer);
    }
}

int render_accquire_dma_buffer(void *handle, int planecnt, int width, int height, RenderDmaBuffer *buf)
{
    int ret;

    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    if (!buf) {
        ERROR("Error Param buf is NUll");
        return -1;
    }
    buf->planeCnt = planecnt;
    buf->width = width;
    buf->height = height;

    ret = renderCore->accquireDmaBuffer(buf);
    if (ret != NO_ERROR) {
        ERROR("Error accquire dma buffer fail");
        return -1;
    }
    return 0;
}

void render_release_dma_buffer(void *handle, RenderDmaBuffer *buffer)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    if (!buffer) {
        ERROR("Error NULL params");
        return;
    }
    renderCore->releaseDmaBuffer(buffer);
}

#ifdef  __cplusplus
}
#endif