#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include "wayland_shm.h"
#include "Logger.h"
#include "wayland_display.h"
#include "ErrorCode.h"
#include "Utils.h"

#define TAG "rlib:wayland_shm"

WaylandShmBuffer::WaylandShmBuffer(WaylandDisplay *display)
{
    mDisplay = display;
    mWlBuffer = NULL;
    mStride = 0;
    mData = NULL;
    mSize = 0;
    mWidth = 0;
    mHeight = 0;
    mFormat = VIDEO_FORMAT_UNKNOWN;
}

WaylandShmBuffer::~WaylandShmBuffer()
{

}

struct wl_buffer *WaylandShmBuffer::constructWlBuffer(int width, int height, RenderVideoFormat format)
{
    struct wl_shm_pool *pool;
    int fd;
    int ret;

    mWidth = width;
    mHeight = height;
    mFormat = format;

    switch (format)
    {
        case VIDEO_FORMAT_YUY2:
        case VIDEO_FORMAT_YVYU:
        case VIDEO_FORMAT_UYVY:
        case VIDEO_FORMAT_VYUY: {
            mStride = ROUND_UP_4 (width * 2);
            mSize = mStride * height;
        } break;
        case VIDEO_FORMAT_AYUV:
        case VIDEO_FORMAT_RGBx:
        case VIDEO_FORMAT_RGBA:
        case VIDEO_FORMAT_BGRA:
        case VIDEO_FORMAT_xRGB:
        case VIDEO_FORMAT_ARGB:
        case VIDEO_FORMAT_xBGR:
        case VIDEO_FORMAT_ABGR:
        case VIDEO_FORMAT_r210:
        case VIDEO_FORMAT_Y410:
        case VIDEO_FORMAT_VUYA:
        case VIDEO_FORMAT_BGR10A2_LE:
        case VIDEO_FORMAT_BGRx: {
            mStride = width * 4;
            mSize = mStride * height;
        } break;

        default:
            break;
    }

    if (mStride <= 0 || mSize <= 0) {
        WARNING("Unsupport format");
        goto tag_err;
    }

    fd = createAnonymousFile(mSize);
    if (fd < 0) {
        ERROR("create anonymous file fail");
        return NULL;
    }

    mData = mmap(NULL, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mData == MAP_FAILED) {
        ERROR("mmap failed: %s",strerror(errno));
        goto tag_err;
    }

    if (mDisplay->getShm() == NULL) {
        ERROR("Shm is null");
        goto tag_err;
    }

    uint32_t shmFormat;
    ret = mDisplay->toShmBufferFormat(format, &shmFormat);
    if (ret != NO_ERROR) {
        ERROR("video format to shm format fail");
        goto tag_err;
    }

    pool = wl_shm_create_pool(mDisplay->getShm(), fd, mSize);
    mWlBuffer = wl_shm_pool_create_buffer(pool, 0,
                           width, height,
                           mStride, shmFormat);

    wl_shm_pool_destroy(pool);
    close(fd);
    return mWlBuffer;

tag_err:
    if (fd > 0) {
        close(fd);
    }
    return NULL;
}

int WaylandShmBuffer::createAnonymousFile(off_t size)
{
    char filename[1024];
    const char *path;
    static int init = 0;
    int fd = -1;
    int ret;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        WARNING("not set XDG_RUNTIME_DIR env");
        goto tag_err;
    }

    /* allocate shm pool */
    snprintf (filename, 1024, "%s/%s-%d-%s", path, "wayland-shm", init++, "XXXXXX");

    fd = mkstemp(filename);
	if (fd < 0) {
        ERROR("make anonymous file fail");
        goto tag_err;
	}

    unlink(filename);

    //set cloexec
    {
        long flags;

        flags = fcntl(fd, F_GETFD);
        if (flags == -1)
            goto tag_err;

        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
            goto tag_err;
    }

    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        goto tag_err;
    }

    return fd;

tag_err:
    if (fd > 0)
        close(fd);

    return -1;
}