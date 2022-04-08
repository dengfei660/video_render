#ifndef __RENDER_LIB_WRAP_H__
#define __RENDER_LIB_WRAP_H__
#include <mutex>
#include <list>
#include <string>
#include "render_lib.h"

#define COMPOSITOR_NAME "videotunnel"
#define INVALID_VIDEOTUNNEL_ID (-1)

typedef void (*notifyBufferRelease) (void * userdata, RenderBuffer *buffer);
typedef void (*notifyBufferDisplayed) (void * userdata, RenderBuffer *buffer);

typedef struct {
    notifyBufferDisplayed notifyFrameDisplayed;
    notifyBufferRelease notifyFrameRelease;
} RenderLibWrapCallback;

class RenderLibWrap {
  public:
    RenderLibWrap(uint32_t vdecPort, uint32_t vdoPort);
    virtual ~RenderLibWrap();
    void setCallback(void *userData, RenderLibWrapCallback *callback);
    bool connectRender(char *name,int videotunnelId);
    bool disconnectRender();
    bool renderFrame(RenderBuffer *buffer);
    void setWindowSize(int x, int y, int w, int h);
    void setFrameSize(int frameWidth, int frameHeight);
    void setMediasyncId(int id);
    /**
     * @brief Set the Mediasync Sync Mode object
     *
     * @param mode 0:vmaster,1:amaster,2:pcrmaster
     */
    void setMediasyncSyncMode(int mode);
    void setMediasyncPcrPid(int id);
    void setMediasyncDemuxId(int id);
    /**
     * @brief Set the Mediasync tunnel
     *
     * @param mode 0:notunnelmode,1:tunnelmode
     */
    void setMediasyncTunnelMode(int mode);
    //the format is defined in render_lib.h
    void setVideoFormat(int format);
    void setVideoFps(int num, int denom);
    void getDroppedFrames(int *cnt);
    bool flush();
    bool pause();
    bool resume();
    RenderBuffer* allocRenderBuffer();
    void releaseRenderBuffer(RenderBuffer* buffer);
    void getSinkId(uint32_t *vdecPort, uint32_t *vdoPort) {
        if (vdecPort) {
            *vdecPort = mVdecPort;
        }
        if (vdoPort) {
            *vdoPort = mVdoPort;
        }
    };
    static void doSend(void *userData , RenderMsgType type, void *msg);
    static int doGet(void *userData, int key, void *value);
  private:
    void *mRenderLibHandle;
    void *mUserData;
    RenderLibWrapCallback mCallback;

    uint32_t mCtrId;
    uint32_t mVdoPort;
    uint32_t mVdecPort;
};

#endif /*__RENDER_LIB_WRAP_H__*/