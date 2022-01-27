#ifndef __VOD_V4L2_SERVER_H__
#define __VOD_V4L2_SERVER_H__
#include <mutex>
#include <list>
#include <string>
#include <linux/videodev2.h>
#include "render_lib.h"
#include "Thread.h"
#include "Poll.h"

#define MAX_SUN_PATH (80)
#define MAX_PLANES (3)

class RenderServer;

class VDOServerThread:public Tls::Thread {
  public:
    VDOServerThread(RenderServer *renderServer, uint32_t ctrId, uint32_t vdoPort, uint32_t vdecPort);
    virtual ~VDOServerThread();
    bool init();
    int getId() {
      return mCtrId;
    };

    //thread func
    void readyToRun();
    void readyToExit();
    virtual bool threadLoop();
    static void msgCallback(void *userData , RenderMsgType type, void *msg);
    static int getCallback(void *userData, int key, void *value);
  private:
    typedef struct {
        struct v4l2_buffer buf;
        int bufferId;
        int fd;
        void *start;
        int capacity;
        size_t length;
        int64_t pts;
        bool queued;
    } BufferInfo;
    bool setCaptureBufferFormat();
    bool getCaptureBufferFrameSize();
    bool setupCapture();
    bool setupCaptureBuffers();
    bool setupMmapCaptureBuffers();
    void tearDownCaptureBuffers();
    void tearDownMmapCaptureBuffers();
    /**
     * @brief queue capture buffer to vdo
     *
     */
    bool queueAllCaptureBuffers();
    bool queueCaptureBuffers(int bufIndex);
    /**
     * @brief dequeue a v4l2 capture buffer from vdo
     *return the capture buffer index
     * @return int capture buffer index
     */
    int dequeueCaptureBuffer();
    bool processEvent();
    bool vdoConnect();
    bool vdoDisconnect();
    Tls::Poll *mPoll;
    int mFrameWidth;
    int mFrameHeight;
    bool mDecoderEos;

    int mDecodedFrameCnt;
    int mDisplayedFrameCnt;
    bool mNeedCaptureRestart;

    BufferInfo *mCaptureBuffers;
    int mQueuedCaptureBufferCnt;
    bool mDecoderLastFrame;

    uint32_t mCtrId;
    uint32_t mVdoPort;
    uint32_t mVdecPort;

    //v4l2
    int mV4l2Fd;
    uint32_t mDeviceCaps;
    struct v4l2_format mCaptureFmt;
    int mNumCaptureBuffers;
    int mMinCaptureBuffers;
    int mCaptureMemMode;
    bool mIsSetCaptureFmt;

    void *mRenderInstance;
    RenderServer *mRenderServer;
};

#endif /*__VOD_V4L2_SERVER_H__*/