#ifndef __VOD_SINK_H__
#define __VOD_SINK_H__
#include <mutex>
#include <list>
#include <string>
#include <linux/videodev2.h>
#include "sink.h"
#include "renderlib_wrap.h"
#include "Mutex.h"

#define MAX_SUN_PATH (80)
#define MAX_PLANES (3)

class RenderServer;
class SinkManager;

class VDOSink : public Sink, public Tls::Thread {
  public:
    VDOSink(SinkManager *sinkMgr, uint32_t vdecPort, uint32_t vdoPort);
    virtual ~VDOSink();
    bool start();
    bool stop();
    void setVdecPort(uint32_t vdecPort) {
        mVdecPort = vdecPort;
    };
    void setVdoPort(uint32_t vdoPort) {
        mVdoPort = vdoPort;
    };
    void getSinkPort(uint32_t *vdecPort, uint32_t *vdoPort) {
        if (vdecPort) {
            *vdecPort = mVdecPort;
        }
        if (vdoPort) {
            *vdoPort = mVdoPort;
        }
    };
    bool isSocketSink() {
        return false;
    };
    States getState() {
        return mState;
    };
    //thread func
    void readyToRun();
    virtual bool threadLoop();
    static void handleBufferRelease(void *userData, RenderBuffer *buffer);
    //buffer had displayed ,but not release
    static void handleFrameDisplayed(void *userData, RenderBuffer *buffer);
  protected:
    void getMediaSyncId(int *mediasyncId){};
  private:
    typedef struct {
        struct v4l2_buffer v4l2buf;
        int bufferId;
        void *start;
        size_t length;
        int64_t pts;
        bool queued;
    } BufferInfo;
    bool setBufferFormat();
    /**
     * @brief request v4l2 buffer from device and do mmap to
     * get buf pointer
     * @return true
     * @return false
     */
    bool setupBuffers();
    void tearDownBuffers();
    /**
     * @brief queue capture buffer to vdo
     *
     */
    bool queueAllBuffers();
    bool queueBuffer(int bufIndex);
    /**
     * @brief dequeue a v4l2 capture buffer from vdo
     *return the capture buffer index
     * @return int capture buffer index
     */
    int dequeueBuffer();
    bool processEvent();
    bool voutConnect();
    bool voutDisconnect();
    void startEvents();
    void stopEvents();
    SinkManager *mSinkMgr;
    mutable Tls::Mutex mMutex;
    int mEpollFd;
    int mFrameWidth;
    int mFrameHeight;
    bool mDecoderEos;

    int mDecodedFrameCnt;
    int mDisplayedFrameCnt;
    bool mNeedCaptureRestart;

    BufferInfo *mCaptureBuffers;
    int mQueuedCaptureBufferCnt;
    bool mDecoderLastFrame;

    //vdo
    uint32_t mVdoPort;
    uint32_t mVdecPort;
    bool mIsVDOConnected;

    States mState;

    //v4l2
    int mV4l2Fd;
    uint32_t mDeviceCaps;
    struct v4l2_format mCaptureFmt;
    int mNumCaptureBuffers;
    bool mIsSetCaptureFmt;
    bool mHasEvents;
    bool mHasEOSEvents;

    RenderLibWrap *mRenderlib;
};

#endif /*__VOD_SINK_H__*/