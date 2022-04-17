#include <string.h>
#include <atomic>
#include "render_lib.h"
#include "render_core.h"
#include "Logger.h"
#include <mutex>

#ifdef  __cplusplus
extern "C" {
#endif

#define TAG "rlib:render_lib"
#define VERSION "V1.4.0"
static int g_renderlibId = 0;
static std::mutex g_mutext;

void *render_open_with_tag(char *name, char *userTag)
{
    //generate a renderlib id
    char tag[32];
    int renderlibId;
    int category = NO_CATEGERY;

    g_mutext.lock();
    Logger_init();

    memset(tag, 0 ,32);
    renderlibId = g_renderlibId++;
    sprintf(tag,"%s-%d","rlib",renderlibId);
    if (userTag) {
        category = Logger_set_userTag(renderlibId, userTag);
    } else {
        category = Logger_set_userTag(renderlibId, tag);
    }

    //set log file
    char *env = getenv("VIDEO_RENDER_LOG_FILE");
    if (env && strlen(env) > 0) {
        Logger_set_file(env);
        INFO(category,"VIDEO_RENDER_LOG_FILE=%s",env);
    }
    //set log level
    env = getenv("VIDEO_RENDER_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO(category,"VIDEO_RENDER_LOG_LEVEL=%d",level);
    }

    INFO(category,"build version:%s",VERSION);
    INFO(category,"open");
    RenderCore *render = new RenderCore(renderlibId, category);
    render->init(name);
    g_mutext.unlock();
    return (void*)render;
}

void* render_open(char *name) {
    return render_open_with_tag(name, NULL);
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
    int category, renderid;

    category = static_cast<RenderCore *>(handle)->getLogCategory();
    renderid = static_cast<RenderCore *>(handle)->getRenderlibId();
    INFO(category,"close end");
    static_cast<RenderCore *>(handle)->release();
    delete static_cast<RenderCore *>(handle);

    Logger_set_userTag(renderid, NULL);
    //Logger_set_file(NULL);
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
    int category = renderCore->getLogCategory();
    if (!buf) {
        ERROR(category,"Error Param buf is NUll");
        return -1;
    }
    buf->planeCnt = planecnt;
    buf->width = width;
    buf->height = height;

    ret = renderCore->accquireDmaBuffer(buf);
    if (ret != NO_ERROR) {
        ERROR(category,"Error accquire dma buffer fail");
        return -1;
    }
    return 0;
}

void render_release_dma_buffer(void *handle, RenderDmaBuffer *buffer)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    int category = renderCore->getLogCategory();
    if (!buffer) {
        ERROR(category,"Error NULL params");
        return;
    }
    renderCore->releaseDmaBuffer(buffer);
}

int render_mediasync_get_first_audio_pts(void *handle, int64_t *pts)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    int category = renderCore->getLogCategory();
    if (!pts) {
        ERROR(category,"Error NULL params");
        return -1;
    }
    return renderCore->getFirstAudioPts(pts);
}

int render_mediasync_get_current_audio_pts(void *handle, int64_t *pts)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    int category = renderCore->getLogCategory();
    if (!pts) {
        ERROR(category,"Error NULL params");
        return -1;
    }
    return renderCore->getCurrentAudioPts(pts);
}

int render_mediasync_get_playback_rate(void *handle, float *scale)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    int category = renderCore->getLogCategory();
    if (!scale) {
        ERROR(category,"Error NULL params");
        return -1;
    }
    return renderCore->getPlaybackRate(scale);
}

int render_mediasync_queue_demux_pts(void *handle, int64_t ptsUs, uint32_t size)
{
    RenderCore * renderCore = static_cast<RenderCore *>(handle);
    return renderCore->queueDemuxPts(ptsUs, size);
}

#ifdef  __cplusplus
}
#endif