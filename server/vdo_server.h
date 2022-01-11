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

class UeventProcessThread : public Tls::Thread {
  public:
    UeventProcessThread(RenderServer *renderServer);
    virtual ~UeventProcessThread();
    bool init();

    //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    int mUeventFd;
    Tls::Poll *mPoll;
    RenderServer *mRenderServer;
};

class VDOServerThread:public Tls::Thread {
  public:
    VDOServerThread(RenderServer *renderServer, int port);
    virtual ~VDOServerThread();
    bool init();
    int getPort() {
      return mPort;
    };

    //thread func
    void readyToRun();
    void readyToExit();
    virtual bool threadLoop();
  private:
    typedef struct {
        int fd;
        void *start;
        int capacity;
    } PlaneInfo;
    typedef struct {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[MAX_PLANES];
        PlaneInfo planeInfo[MAX_PLANES];
        int bufferId;
        bool locked;
        int lockCount;
        int planeCount;
        int fd;
        void *start;
        int capacity;
        int frameNumber;
        int64_t frameTime;
        bool drop;
        bool queued;
    } BufferInfo;
    bool setCaptureBufferFormat();
    bool getCaptureBufferFrameSize();
    bool setupCaptureBuffers();
    bool setupMmapCaptureBuffers();
    bool setupDmaCaptureBuffers();
    void tearDownCaptureBuffers();
    void tearDownMmapCaptureBuffers();
    void tearDownDmaCaptureBuffers();
    int mPort;
    Tls::Poll *mPoll;
    int mFrameWidth;
    int mFrameHeight;

    BufferInfo *mCaptureBuffers;

    //v4l2
    int mV4l2Fd;
    bool mIsMultiPlane;
    uint32_t mDeviceCaps;
    struct v4l2_format mCaptureFmt;
    int mNumCaptureBuffers;
    int mMinCaptureBuffers;
    int mCaptureMemMode;
    bool mIsCaptureFmtSet;

    void *mRenderInstance;
    RenderServer *mRenderServer;
};

#endif /*__VOD_V4L2_SERVER_H__*/