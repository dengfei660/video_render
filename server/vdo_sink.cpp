#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "vdo_sink.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"
#include "sink_manager.h"

#include "v4l2_am_comtext.h"

typedef struct {
    int width;
    int height;
    uint32_t pixel;
    uint64_t pts;
    int planeCnt;
    uint32_t handle[3];
    uint32_t stride[3];
    uint32_t offset[3];
    uint32_t size[3];
    int fd[3];
} VoutDmaBuffer;

using namespace Tls;

#define TAG "rlib:vdo_sink"
#define VDO_DEVIDE_0 "/dev/video28"
#define VDO_DEVIDE_1 "/dev/video29"

#define VOUT_DEVIDE_0 "/dev/video40"

#define NUM_CAPTURE_BUFFERS (8)

static int IOCTL( int fd, int request, void* arg );
static void dumpRenderBuffer(RenderBuffer * buffer);
static void dumpVoutDmaBuffer(VoutDmaBuffer * buffer);
static int adaptFd( int fdin );
static int64_t pts90KToNs(int64_t pts);

static RenderLibWrapCallback renderlibCallback {
    VDOSink::handleFrameDisplayed,
    VDOSink::handleBufferRelease
};

void VDOSink::handleBufferRelease(void* userData, RenderBuffer *buffer)
{
    VDOSink *self = static_cast<VDOSink *>(userData);
    BufferInfo * captureBuffer = (BufferInfo *)buffer->priv;
    int index = captureBuffer->bufferId;
    TRACE3("rb:%p,bi:%p,buffer id:%d,fd:%d",buffer,captureBuffer,index,buffer->dma.fd[0]);
    Tls::Mutex::Autolock _l(self->mMutex);
    //must close vfm dup fd
    for (int i = 0; i < buffer->dma.planeCnt; i++) {
        TRACE3("close uvm fd:%d dup drm fd:%d",buffer->dma.fd[i]);
        close(buffer->dma.fd[i]);
    }

    self->queueBuffer(index);
    self->mRenderlib->releaseRenderBuffer(buffer);
}

void VDOSink::handleFrameDisplayed(void* userData, RenderBuffer *buffer)
{
    VDOSink *self = static_cast<VDOSink *>(userData);
    self->mDisplayedFrameCnt += 1;
}

VDOSink::VDOSink(SinkManager *sinkMgr, uint32_t vdecPort, uint32_t vdoPort)
    : mSinkMgr(sinkMgr),
    mVdecPort(vdecPort),
    mVdoPort(vdoPort)
{
    DEBUG("in,vdecport:%d,vdoport:%d",vdecPort, vdoPort);
    mState = STATE_CREATE;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mRenderlib = new RenderLibWrap(mVdecPort, mVdoPort);
    mV4l2Fd = -1;
    mNumCaptureBuffers = 0;
    mCaptureBuffers = NULL;
    mIsSetCaptureFmt = false;
    mDecoderEos = false;
    mDecodedFrameCnt = 0;
    mDisplayedFrameCnt = 0;
    mQueuedCaptureBufferCnt = 0;
    mDecoderLastFrame = false;
    mIsVDOConnected = false;
    mHasEvents = false;
    mHasEOSEvents = false;
    mEpollFd = epoll_create(2);
    DEBUG("out");
}

VDOSink::~VDOSink()
{
    DEBUG("in");
    mState = STATE_DESTROY;
    if (mEpollFd > 0) {
        close(mEpollFd);
        mEpollFd = -1;
    }

    if (mV4l2Fd >= 0) {
        close(mV4l2Fd);
        mV4l2Fd = -1;
    }
    if (mCaptureBuffers) {
        free(mCaptureBuffers);
        mCaptureBuffers = NULL;
    }
    if (mRenderlib) {
        delete mRenderlib;
        mRenderlib = NULL;
    }
    DEBUG("out");
}

bool VDOSink::start()
{
    struct v4l2_capability caps;
    int rc;
    bool bret;
    char * deviveName = NULL;

    DEBUG("in");
    Tls::Mutex::Autolock _l(mMutex);

    if (mState >= STATE_START) {
        WARNING("had started");
        return true;
    }

    //open render lib
    bret = mRenderlib->connectRender((char *)COMPOSITOR_NAME, mVdoPort);
    if (!bret) {
        ERROR("connect to render fail");
    }

    mRenderlib->setCallback(this, &renderlibCallback);

    mState = STATE_START;
    deviveName = (char *)VOUT_DEVIDE_0;

    DEBUG("Open device:%s",deviveName);
    mV4l2Fd = open( deviveName, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (mV4l2Fd < 0) {
        ERROR("open v4l2 device fail,device name:%s",deviveName);
        return false;
    }

    //subscribe events
    startEvents();

    //request capture buffer
    setupBuffers();

    //queue all buffer to v4l2 device
    queueAllBuffers();

    //after queue all buffers,do connecting vdo
    bret = voutConnect();
    if (!bret) {
        goto tag_exit;
    }

    //to start run thread
    DEBUG("to run vdosink thread");
    run("vdoSink");

    DEBUG("out");
    return true;

tag_exit:
    if (mV4l2Fd > 0) {
        close(mV4l2Fd);
        mV4l2Fd = 0;
    }

   return false;
}

bool VDOSink::stop()
{
    DEBUG("in");
    if (mState >= STATE_STOP) {
        WARNING("had stopped");
        return true;
    }
    //dead lock,when buffer release
    //Tls::Mutex::Autolock _l(mMutex);
    mState = STATE_STOP;
    if (mEpollFd > 0) {
        close(mEpollFd);
        mEpollFd = -1;
    }

    if (isRunning()) {
        requestExitAndWait();
    }

    //first disconnect render lib,render lib need release buffers
    //and queue buffer to v4l2,if streamoff queue will fail
    if (mRenderlib) {
        mRenderlib->disconnectRender();
        delete mRenderlib;
        mRenderlib = NULL;
    }

    //second streamoff
    int rc;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rc= IOCTL( mV4l2Fd, VIDIOC_STREAMOFF, &type);
    if ( rc < 0 )
    {
        ERROR("streamoff failed for output: rc %d errno %d", rc, errno );
    }
    DEBUG("streamoff success");

    //first disconnect vdo
    voutDisconnect();

    tearDownBuffers();

    if (mV4l2Fd >= 0) {
        if (mHasEvents) {
            stopEvents();
        }
        close(mV4l2Fd);
        mV4l2Fd = -1;
    }

    if (mCaptureBuffers) {
        free(mCaptureBuffers);
        mCaptureBuffers = NULL;
    }

    DEBUG("out");
    return true;
}

bool VDOSink::voutConnect()
{
    int rc;
    char * deviveName = NULL;
    struct v4l2_ext_controls ext_controls;
    struct v4l2_ext_control ext_control;
    struct am_v4l2_ext_vdec_vdo_connection vout_con;

    memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
    memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
    memset(&vout_con, 0, sizeof(struct am_v4l2_ext_vdec_vdo_connection));

    vout_con.vdo_port = mVdoPort;  //vdo port number
    vout_con.vdec_port = mVdecPort; //vdec port number

    ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_controls.count = 1;
    ext_controls.controls = &ext_control;
    ext_controls.controls->id = AM_V4L2_CID_EXT_VDO_VDEC_CONNECTING;
    ext_controls.controls->ptr = (void *)&vout_con;

    rc = IOCTL(mV4l2Fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
    if (rc < 0) {
        ERROR("connect vdo fail");
        return false;
    }
    INFO("Connect vdecPort:%d to vdoPort:%d success",mVdecPort,mVdoPort);

    mIsVDOConnected = true;
    return true;
}

bool VDOSink::voutDisconnect()
{
    int rc;
    char * deviveName = NULL;
    struct v4l2_ext_controls ext_controls;
    struct v4l2_ext_control ext_control;
    struct am_v4l2_ext_vdec_vdo_connection vout_con;

    if (mV4l2Fd < 0 || !mIsVDOConnected) {
        ERROR("Error vdo device, fd:%d, isconnect:%d",mV4l2Fd, mIsVDOConnected);
        return false;
    }

    memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
    memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
    memset(&vout_con, 0, sizeof(struct am_v4l2_ext_vdec_vdo_connection));

    vout_con.vdo_port = mVdoPort;  //vdo port number
    vout_con.vdec_port = mVdecPort; //vdec port number

    ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_controls.count = 1;
    ext_controls.controls = &ext_control;
    ext_controls.controls->id = AM_V4L2_CID_EXT_VDO_VDEC_DISCONNECTING;
    ext_controls.controls->ptr = (void *)&vout_con;

    rc = IOCTL(mV4l2Fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
    if (rc < 0) {
        ERROR("connect vdo fail");
        return false;
    }

    INFO("Disconnect vdecPort:%d to vdoPort:%d success",mVdecPort,mVdoPort);
    mIsVDOConnected = false;
    return true;
}

void VDOSink::startEvents()
{
    int rc;
    struct v4l2_event_subscription evtsub;

    memset( &evtsub, 0, sizeof(evtsub));
    evtsub.type= V4L2_EVENT_SOURCE_CHANGE;
    rc= IOCTL( mV4l2Fd, VIDIOC_SUBSCRIBE_EVENT, &evtsub );
    if ( rc == 0 )
    {
        mHasEvents = true;
        DEBUG("subscribe source change event success");
    }
    else
    {
        ERROR("source change event subscribe failed rc %d (errno %d)", rc, errno);
    }

    memset( &evtsub, 0, sizeof(evtsub));
    evtsub.type= V4L2_EVENT_EOS;
    rc= IOCTL(mV4l2Fd, VIDIOC_SUBSCRIBE_EVENT, &evtsub );
    if ( rc == 0 )
    {
        mHasEOSEvents = true;
        DEBUG("subscribe eos event success");
    }
    else
    {
        ERROR("eos event subcribe for eos failed rc %d (errno %d)", rc, errno );
    }
}
void VDOSink::stopEvents()
{
    int rc;
    struct v4l2_event_subscription evtsub;

    memset( &evtsub, 0, sizeof(evtsub));

    evtsub.type= V4L2_EVENT_ALL;
    rc= IOCTL(mV4l2Fd, VIDIOC_UNSUBSCRIBE_EVENT, &evtsub );
    if ( rc != 0 )
    {
        ERROR("event unsubscribe failed rc %d (errno %d)", rc, errno);
        return;
    }
    DEBUG("unsubscribe event success");
}

bool VDOSink::setBufferFormat()
{
    int rc;

    if (mIsSetCaptureFmt) {
        return true;
    }

    if (mFrameWidth <= 0 || mFrameHeight <= 0) {
        WARNING("need get frame width and height");
        return false;
    }

    memset( &mCaptureFmt, 0, sizeof(struct v4l2_format) );
    mCaptureFmt.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* Get current settings from driver */
    rc= IOCTL( mV4l2Fd, VIDIOC_G_FMT, &mCaptureFmt );
    if ( rc < 0 )
    {
        ERROR("failed get format for output: rc %d errno %d", rc, errno);
    }

    mCaptureFmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    mCaptureFmt.fmt.pix.width = mFrameWidth;
    mCaptureFmt.fmt.pix.height = mFrameHeight;
    mCaptureFmt.fmt.pix.sizeimage = (mCaptureFmt.fmt.pix.width*mCaptureFmt.fmt.pix.height*3)/2;
    mCaptureFmt.fmt.pix.field = V4L2_FIELD_ANY;

    rc= IOCTL(mV4l2Fd, VIDIOC_S_FMT, &mCaptureFmt );
    if ( rc < 0 )
    {
        DEBUG("failed to set format for output: rc %d errno %d", rc, errno);
        return false;
    }

    mIsSetCaptureFmt = true;
    return true;
}

bool VDOSink::setupBuffers()
{
    int rc, neededBuffers;
    struct v4l2_control ctl;
    struct v4l2_requestbuffers reqbuf;
    int i, j;
    bool result = false;
    int minBufferCnt;

/*
    if (!mIsSetCaptureFmt) {
        WARNING("not set capture buffer format");
        return false;
    }*/

    neededBuffers = NUM_CAPTURE_BUFFERS;

    memset( &ctl, 0, sizeof(ctl));
    ctl.id= V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
    rc= IOCTL( mV4l2Fd, VIDIOC_G_CTRL, &ctl );
    if ( rc == 0 )
    {
        minBufferCnt = ctl.value;
        if ( (minBufferCnt != 0) && (minBufferCnt > NUM_CAPTURE_BUFFERS) )
        {
            neededBuffers= minBufferCnt + 1;
        }
    }

    if ( minBufferCnt == 0 )
    {
        minBufferCnt = NUM_CAPTURE_BUFFERS;
    }

    memset( &reqbuf, 0, sizeof(reqbuf) );
    reqbuf.count= neededBuffers;
    reqbuf.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory= V4L2_MEMORY_MMAP;
    rc= IOCTL( mV4l2Fd, VIDIOC_REQBUFS, &reqbuf );
    if ( rc < 0 )
    {
        ERROR("failed to request %d mmap buffers for capture: rc %d errno %d", neededBuffers, rc, errno);
        goto exit;
    }

    mNumCaptureBuffers = reqbuf.count;
    DEBUG("neededBuffers cnt:%d,req cnt:%d",neededBuffers,reqbuf.count);

    if ( reqbuf.count < minBufferCnt )
    {
        ERROR("insufficient buffers: (%d versus %d)", reqbuf.count, neededBuffers );
        goto exit;
    }

    mCaptureBuffers = (BufferInfo *)calloc(reqbuf.count, sizeof(BufferInfo));
    if (!mCaptureBuffers) {
        ERROR("No memory to alloc capturebuffers mgr");
        goto exit;
    }

    for (i = 0; i < mNumCaptureBuffers; i++) {
        struct v4l2_buffer *bufOut;
        struct v4l2_exportbuffer expbuf;
        void *bufStart;

        mCaptureBuffers[i].bufferId = i;

        bufOut = &mCaptureBuffers[i].v4l2buf;
        memset(bufOut, 0, sizeof(struct v4l2_buffer));
        bufOut->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufOut->index= i;
        bufOut->memory= V4L2_MEMORY_MMAP;

        rc= IOCTL( mV4l2Fd, VIDIOC_QUERYBUF, bufOut );
        if ( rc < 0 )
        {
            ERROR("failed to query input buffer %d: rc %d errno %d", i, rc, errno);
            goto exit;
        }

        DEBUG("index: %d bytesUsed %d offset %d length %d flags %08x",
                bufOut->index, bufOut->bytesused, bufOut->m.offset, bufOut->length, bufOut->flags );

        bufStart = mmap( NULL,
                        bufOut->length,
                        PROT_READ,
                        MAP_SHARED,
                        mV4l2Fd,
                        bufOut->m.offset );
        if ( bufStart != MAP_FAILED )
        {
            mCaptureBuffers[i].start= bufStart;
            mCaptureBuffers[i].length = bufOut->length;
            result = true;
        }
        else
        {
            ERROR("failed to mmap buffer %d: errno %d", i, errno);
        }
    }

exit:

   if ( !result )
   {
      tearDownBuffers();
   }
   return result;
}

void VDOSink::tearDownBuffers()
{
    int rc;
    struct v4l2_requestbuffers reqbuf;

    if ( mCaptureBuffers )
    {
        for (int i = 0; i < mNumCaptureBuffers; ++i )
        {
            if ( mCaptureBuffers[i].start )
            {
                munmap( mCaptureBuffers[i].start, mCaptureBuffers[i].length );
            }
        }

        free( mCaptureBuffers);
        mCaptureBuffers = NULL;
    }

    if ( mNumCaptureBuffers )
    {
        memset( &reqbuf, 0, sizeof(reqbuf) );
        reqbuf.count= 0;
        reqbuf.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuf.memory= V4L2_MEMORY_MMAP;
        rc= IOCTL( mV4l2Fd, VIDIOC_REQBUFS, &reqbuf );
        if ( rc < 0 )
        {
            ERROR("failed to release v4l2 buffers for output: rc %d errno %d", rc, errno);
        }
        mNumCaptureBuffers = 0;
    }
}

bool VDOSink::queueAllBuffers()
{
    bool ret = true;
    int i, j, rc;
    for ( int i= 0; i < mNumCaptureBuffers; ++i )
    {
        rc= IOCTL(mV4l2Fd, VIDIOC_QBUF, &mCaptureBuffers[i].v4l2buf );
        if ( rc < 0 )
        {
            ERROR("failed to queue buffer: rc %d errno %d", rc, errno);
            ret = false;
            goto exit;
        }
        mQueuedCaptureBufferCnt += 1;
        mCaptureBuffers[i].queued = true;
    }

exit:
    return ret;
}

bool VDOSink::queueBuffer(int bufIndex)
{
    bool ret = true;
    int rc;
    rc= IOCTL(mV4l2Fd, VIDIOC_QBUF, &mCaptureBuffers[bufIndex].v4l2buf );
    if ( rc < 0 )
    {
        ERROR("failed to queue output buffer: rc %d errno %d", rc, errno);
        return false;
    }
    mQueuedCaptureBufferCnt += 1;
    mCaptureBuffers[bufIndex].queued = true;
    return true;
}

int VDOSink::dequeueBuffer()
{
    int bufferIndex = -1;
    int rc;
    struct v4l2_buffer buf;

    if ( mDecoderLastFrame )
    {
        WARNING("had decoded last frame");
        goto exit;
    }
    memset( &buf, 0, sizeof(buf));
    buf.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory= V4L2_MEMORY_MMAP;
    rc = IOCTL( mV4l2Fd, VIDIOC_DQBUF, &buf );
    if ( rc == 0 )
    {
        bufferIndex = buf.index;
        memcpy(&mCaptureBuffers[bufferIndex].v4l2buf, &buf, sizeof(struct v4l2_buffer));
        mCaptureBuffers[bufferIndex].queued = false;
        mQueuedCaptureBufferCnt -= 1;
        TRACE1("dqbuffer id:%d,start:%p,bytesused:%d",bufferIndex,mCaptureBuffers[bufferIndex].start,mCaptureBuffers[bufferIndex].v4l2buf.bytesused);
    }
    else
    {
        ERROR("failed to de-queue buffer: rc %d errno %d", rc, errno);
        if ( errno == EPIPE )
        {
            /* Decoding is done: no more capture buffers can be dequeued */
            mDecoderLastFrame = true;
        }
    }

exit:
   return bufferIndex;
}

bool VDOSink::processEvent()
{
    int rc;
    struct v4l2_event event;
    bool ret = false;

    for ( ; ; ) {
        memset( &event, 0, sizeof(event));
        rc= IOCTL( mV4l2Fd, VIDIOC_DQEVENT, &event );
        if (rc != 0) { //dq event fail,break
            break;
        }
        //get this event
        ret = true;

        if ( (event.type == V4L2_EVENT_SOURCE_CHANGE) &&
                (event.u.src_change.changes & V4L2_EVENT_SRC_CH_RESOLUTION) ) {
            struct v4l2_format fmtOut;

            INFO("source change event\n");
            if (!mIsSetCaptureFmt) {
                memset( &mCaptureFmt, 0, sizeof(struct v4l2_format) );
                mCaptureFmt.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;

                /* Get current settings from driver */
                rc= IOCTL( mV4l2Fd, VIDIOC_G_FMT, &mCaptureFmt );
                if ( rc < 0 )
                {
                    ERROR("failed get format for output: rc %d errno %d", rc, errno);
                    return false;
                }
                mFrameWidth = mCaptureFmt.fmt.pix.width;
                mFrameHeight = mCaptureFmt.fmt.pix.height;
                INFO("frame size %dx%d",mFrameWidth,mFrameHeight);
                mRenderlib->setFrameSize(mFrameWidth, mFrameHeight);
                switch (mCaptureFmt.fmt.pix.pixelformat) {
                    case V4L2_PIX_FMT_NV12: {
                        mRenderlib->setVideoFormat(VIDEO_FORMAT_NV12);
                    } break;
                    case V4L2_PIX_FMT_NV21: {
                        mRenderlib->setVideoFormat(VIDEO_FORMAT_NV21);
                    } break;
                    default:{
                        ERROR("Unknow pix format");
                    }break;
                }
                return true;
            } else {
                memset( &fmtOut, 0, sizeof(fmtOut));
                fmtOut.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
                rc= IOCTL( mV4l2Fd, VIDIOC_G_FMT, &fmtOut );
                if (rc != 0) {
                    ERROR("get fmt failed");
                    return false;
                }
            }

            if ((mNumCaptureBuffers == 0) ||
                    (((fmtOut.fmt.pix.width != mCaptureFmt.fmt.pix.width) ||
                        (fmtOut.fmt.pix.height != mCaptureFmt.fmt.pix.height))) ||
                    (mDecodedFrameCnt > 0) ) {
                tearDownBuffers();

                setupBuffers();
                mNeedCaptureRestart = true;
                mFrameWidth = mCaptureFmt.fmt.pix.width;
                mFrameHeight = mCaptureFmt.fmt.pix.height;
                INFO("frame size %dx%d",mFrameWidth,mFrameHeight);
                mRenderlib->setFrameSize(mFrameWidth, mFrameHeight);
                switch (mCaptureFmt.fmt.pix.pixelformat) {
                    case V4L2_PIX_FMT_NV12: {
                        mRenderlib->setVideoFormat(VIDEO_FORMAT_NV12);
                    } break;
                    case V4L2_PIX_FMT_NV21: {
                        mRenderlib->setVideoFormat(VIDEO_FORMAT_NV21);
                    } break;
                    default:{
                        ERROR("Unknow pix format");
                    }break;
                }
            }
            //copy format
            memcpy(&mCaptureFmt, &fmtOut, sizeof(struct v4l2_format));
        } else if (event.type == V4L2_EVENT_EOS) {
            INFO("v4l2 decoder eos event\n");
            mDecoderEos = true;
        }
    }
    return ret;
}

void VDOSink::readyToRun()
{
    DEBUG("in");
    bool bret;
    int rc;

    mState = STATE_RUNNING;

    //streamon
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rc= IOCTL( mV4l2Fd, VIDIOC_STREAMON, &type);
    if ( rc < 0 )
    {
        ERROR("streamon failed for output: rc %d errno %d", rc, errno );
    }

    DEBUG("streamon success");

    if (mV4l2Fd >= 0) {
        struct epoll_event event;
        event.data.fd = mV4l2Fd;
        event.events = EPOLLIN | EPOLLPRI | EPOLLRDNORM;
        epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mV4l2Fd, &event);
    }

    DEBUG("out");
}

bool VDOSink::threadLoop()
{
    int ret;
    int rc;
    struct epoll_event epollEvent;

    if (mV4l2Fd < 0) {
        WARNING("Not open v4l2 fd");
        return false;
    }
    if (mState >= STATE_STOP) {
        INFO("will stop thread");
        return false;
    }

    rc = epoll_wait (mEpollFd, &epollEvent, 1, 2000);
    if (rc < 0) {
        WARNING("epoll error");
        return true;
    } else if (rc == 0) { //timeout
        return true;
    }

    if (mHasEvents) {
        if (epollEvent.events & EPOLLPRI) {
            bool bret;
            //process v4l2 event
            bret = processEvent();
            if (bret) {
                return true;
            }
        }
    }

    //dqueue capture buffer
    int bufferIndex = dequeueBuffer();

    if (bufferIndex >=0 ) { //put frame to render lib
        VoutDmaBuffer voutDmaBuffer;
        BufferInfo * captureBuffer = &mCaptureBuffers[bufferIndex];
        RenderBuffer *renderBuf = mRenderlib->allocRenderBuffer();
        if (!renderBuf) {
            ERROR("render allocate buffer wrap fail");
            return true;
        }
        memcpy(&voutDmaBuffer, captureBuffer->start, sizeof(VoutDmaBuffer));
        //dumpVoutDmaBuffer(&voutDmaBuffer);

        renderBuf->priv = (void *)captureBuffer;
        renderBuf->dma.width = voutDmaBuffer.width;
        renderBuf->dma.height = voutDmaBuffer.height;
        renderBuf->pts = voutDmaBuffer.pts * 1000; //the dq buff pts is us unite
        //for fix pts error,if set -1, render lib will cal a new pts with fps
        if (renderBuf->pts == 0) {
            renderBuf->pts = -1;
        }
        renderBuf->dma.planeCnt = voutDmaBuffer.planeCnt;
        for (int i = 0; i < renderBuf->dma.planeCnt; i++) {
            int fdin = voutDmaBuffer.fd[i];
            renderBuf->dma.fd[i] = adaptFd(fdin);
            TRACE3("fd:%d,dumpfd:%d",fdin,renderBuf->dma.fd[i]);
            renderBuf->dma.stride[i] = voutDmaBuffer.stride[i];
            renderBuf->dma.offset[i] = voutDmaBuffer.offset[i];
            renderBuf->dma.size[i] = voutDmaBuffer.size[i];
        }
        //dumpRenderBuffer(renderBuf);
        mRenderlib->renderFrame(renderBuf);
    }

    //at the last,we process capture restart
    if (mNeedCaptureRestart) {
        mNeedCaptureRestart = false;
        queueAllBuffers();
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        rc= IOCTL( mV4l2Fd, VIDIOC_STREAMON, &type);
        if ( rc < 0 )
        {
            ERROR("streamon failed for output: rc %d errno %d", rc, errno );
            return false;
        }
    }

    return true;
}

static int IOCTL( int fd, int request, void* arg )
{
    const char *req= 0;
    int rc;
    int level;

    level = Logger_get_level();

    if ( level >= LOG_LEVEL_TRACE1 )
    {
        switch ( request )
        {
            case VIDIOC_QUERYCAP: req= "VIDIOC_QUERYCAP"; break;
            case VIDIOC_ENUM_FMT: req= "VIDIOC_ENUM_FMT"; break;
            case VIDIOC_G_FMT: req= "VIDIOC_G_FMT"; break;
            case VIDIOC_S_FMT: req= "VIDIOC_S_FMT"; break;
            case VIDIOC_REQBUFS: req= "VIDIOC_REQBUFS"; break;
            case VIDIOC_QUERYBUF: req= "VIDIOC_QUERYBUF"; break;
            case VIDIOC_G_FBUF: req= "VIDIOC_G_FBUF"; break;
            case VIDIOC_S_FBUF: req= "VIDIOC_S_FBUF"; break;
            case VIDIOC_OVERLAY: req= "VIDIOC_OVERLAY"; break;
            case VIDIOC_QBUF: req= "VIDIOC_QBUF"; break;
            case VIDIOC_EXPBUF: req= "VIDIOC_EXPBUF"; break;
            case VIDIOC_DQBUF: req= "VIDIOC_DQBUF"; break;
            case VIDIOC_DQEVENT: req= "VIDIOC_DQEVENT"; break;
            case VIDIOC_STREAMON: req= "VIDIOC_STREAMON"; break;
            case VIDIOC_STREAMOFF: req= "VIDIOC_STREAMOFF"; break;
            case VIDIOC_G_PARM: req= "VIDIOC_G_PARM"; break;
            case VIDIOC_S_PARM: req= "VIDIOC_S_PARM"; break;
            case VIDIOC_G_STD: req= "VIDIOC_G_STD"; break;
            case VIDIOC_S_STD: req= "VIDIOC_S_STD"; break;
            case VIDIOC_ENUMSTD: req= "VIDIOC_ENUMSTD"; break;
            case VIDIOC_ENUMINPUT: req= "VIDIOC_ENUMINPUT"; break;
            case VIDIOC_G_CTRL: req= "VIDIOC_G_CTRL"; break;
            case VIDIOC_S_CTRL: req= "VIDIOC_S_CTRL"; break;
            case VIDIOC_QUERYCTRL: req= "VIDIOC_QUERYCTRL"; break;
            case VIDIOC_ENUM_FRAMESIZES: req= "VIDIOC_ENUM_FRAMESIZES"; break;
            case VIDIOC_TRY_FMT: req= "VIDIOC_TRY_FMT"; break;
            case VIDIOC_CROPCAP: req= "VIDIOC_CROPCAP"; break;
            case VIDIOC_CREATE_BUFS: req= "VIDIOC_CREATE_BUFS"; break;
            case VIDIOC_G_SELECTION: req= "VIDIOC_G_SELECTION"; break;
            case VIDIOC_SUBSCRIBE_EVENT: req= "VIDIOC_SUBSCRIBE_EVENT"; break;
            case VIDIOC_UNSUBSCRIBE_EVENT: req= "VIDIOC_UNSUBSCRIBE_EVENT"; break;
            case VIDIOC_DECODER_CMD: req= "VIDIOC_DECODER_CMD"; break;
            case VIDIOC_S_EXT_CTRLS: req = "VIDIOC_S_EXT_CTRLS";break;
            default: req= "NA"; break;
        }
        TRACE2("ioct( %d, %x ( %s ) )\n", fd, request, req );
        if ( request == (int)VIDIOC_S_FMT )
        {
            struct v4l2_format *format= (struct v4l2_format*)arg;
            TRACE2("ioctl: : type %d\n", format->type);
            if ( (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
                (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) )
            {
                TRACE2("ioctl: pix_mp: pixelFormat %X w %d h %d field %d cs %d flg %x num_planes %d p0: sz %d bpl %d p1: sz %d bpl %d\n",
                    format->fmt.pix_mp.pixelformat,
                    format->fmt.pix_mp.width,
                    format->fmt.pix_mp.height,
                    format->fmt.pix_mp.field,
                    format->fmt.pix_mp.colorspace,
                    format->fmt.pix_mp.flags,
                    format->fmt.pix_mp.num_planes,
                    format->fmt.pix_mp.plane_fmt[0].sizeimage,
                    format->fmt.pix_mp.plane_fmt[0].bytesperline,
                    format->fmt.pix_mp.plane_fmt[1].sizeimage,
                    format->fmt.pix_mp.plane_fmt[1].bytesperline
                );
            }
            else if ( (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
                   (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) )
            {
                TRACE2("ioctl: pix: pixelFormat %X w %d h %d field %d bpl %d\n",
                    format->fmt.pix.pixelformat,
                    format->fmt.pix.width,
                    format->fmt.pix.height,
                    format->fmt.pix.field,
                    format->fmt.pix.bytesperline
                );
            }
        }
        else if ( request == (int)VIDIOC_REQBUFS )
        {
            struct v4l2_requestbuffers *rb= (struct v4l2_requestbuffers*)arg;
            TRACE2("ioctl: count %d type %d mem %d\n", rb->count, rb->type, rb->memory);
        }
        else if ( request == (int)VIDIOC_QUERYBUF )
        {
            struct v4l2_buffer *buf= (struct v4l2_buffer*)arg;
            TRACE2("ioctl: index %d type %d mem %d\n", buf->index, buf->type, buf->memory);
        }
        else if ( request == (int)VIDIOC_S_CTRL )
        {
         struct v4l2_control *control= (struct v4l2_control*)arg;
         TRACE2("ioctl: ctrl id %d value %d\n", control->id, control->value);
        }
        else if ( request == (int)VIDIOC_CREATE_BUFS )
        {
            struct v4l2_create_buffers *cb= (struct v4l2_create_buffers*)arg;
            struct v4l2_format *format= &cb->format;
            TRACE2("ioctl: count %d mem %d\n", cb->count, cb->memory);
            TRACE2("ioctl: pix_mp: pixelFormat %X w %d h %d num_planes %d p0: sz %d bpl %d p1: sz %d bpl %d\n",
                 format->fmt.pix_mp.pixelformat,
                 format->fmt.pix_mp.width,
                 format->fmt.pix_mp.height,
                 format->fmt.pix_mp.num_planes,
                 format->fmt.pix_mp.plane_fmt[0].sizeimage,
                 format->fmt.pix_mp.plane_fmt[0].bytesperline,
                 format->fmt.pix_mp.plane_fmt[1].sizeimage,
                 format->fmt.pix_mp.plane_fmt[1].bytesperline
            );
        }
        else if ( request == (int)VIDIOC_QBUF )
        {
            struct v4l2_buffer *buf= (struct v4l2_buffer*)arg;
            TRACE2("ioctl: buff: index %d q: type %d bytesused %d flags %X field %d mem %x length %d timestamp sec %ld usec %ld\n",
                    buf->index, buf->type, buf->bytesused, buf->flags, buf->field, buf->memory, buf->length, buf->timestamp.tv_sec, buf->timestamp.tv_usec);
            if ( buf->m.planes &&
                    ( (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
                    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ) )
            {
                if ( buf->memory == V4L2_MEMORY_DMABUF )
                {
                    TRACE2("ioctl: buff: p0 bu %d len %d fd %d doff %d p1 bu %d len %d fd %d doff %d\n",
                        buf->m.planes[0].bytesused, buf->m.planes[0].length, buf->m.planes[0].m.fd, buf->m.planes[0].data_offset,
                        buf->m.planes[1].bytesused, buf->m.planes[1].length, buf->m.planes[1].m.fd, buf->m.planes[1].data_offset );
                }
                else
                {
                    TRACE2("ioctl: buff: p0: bu %d len %d moff %d doff %d p1: bu %d len %d moff %d doff %d\n",
                        buf->m.planes[0].bytesused, buf->m.planes[0].length, buf->m.planes[0].m.mem_offset, buf->m.planes[0].data_offset,
                        buf->m.planes[1].bytesused, buf->m.planes[1].length, buf->m.planes[1].m.mem_offset, buf->m.planes[1].data_offset );
                }
            }
        }
        else if ( request == (int)VIDIOC_DQBUF )
        {
            struct v4l2_buffer *buf= (struct v4l2_buffer*)arg;
            //TRACE2("ioctl: buff: index %d s dq: type %d bytesused %d flags %X field %d mem %x length %d timestamp sec %ld usec %ld\n",
            //    buf->index, buf->type, buf->bytesused, buf->flags, buf->field, buf->memory, buf->length, buf->timestamp.tv_sec, buf->timestamp.tv_usec);
            if ( buf->m.planes &&
                    ( (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
                    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ) )
            {
                TRACE2("ioctl: buff: p0: bu %d len %d moff %d doff %d p1: bu %d len %d moff %d doff %d\n",
                    buf->m.planes[0].bytesused, buf->m.planes[0].length, buf->m.planes[0].m.mem_offset, buf->m.planes[0].data_offset,
                    buf->m.planes[1].bytesused, buf->m.planes[1].length, buf->m.planes[1].m.mem_offset, buf->m.planes[1].data_offset );
            }
        }
        else if ( (request == (int)VIDIOC_STREAMON) || (request == (int)VIDIOC_STREAMOFF) )
        {
            int *type= (int*)arg;
            TRACE2("ioctl: : type %d\n", *type);
        }
        else if ( request == (int)VIDIOC_SUBSCRIBE_EVENT )
        {
            struct v4l2_event_subscription *evtsub= (struct v4l2_event_subscription*)arg;
            TRACE2("ioctl: type %d\n", evtsub->type);
        }
        else if ( request == (int)VIDIOC_DECODER_CMD )
        {
            struct v4l2_decoder_cmd *dcmd= (struct v4l2_decoder_cmd*)arg;
            TRACE2("ioctl: cmd %d\n", dcmd->cmd);
        }
        else if (request == VIDIOC_S_EXT_CTRLS)
        {
            struct am_v4l2_ext_vdec_vdo_connection *vdo_con;
            struct v4l2_ext_controls *ext_controls = (struct v4l2_ext_controls *)arg;
            vdo_con = (struct am_v4l2_ext_vdec_vdo_connection *)ext_controls->controls->ptr;

            TRACE2("ioctl: vdo port: %d, vdec port:%d\n", vdo_con->vdo_port, vdo_con->vdec_port);
        }
    }

    rc= ioctl( fd, request, arg );


    if ( level >= LOG_LEVEL_TRACE1 )
    {
        if ( rc < 0 )
        {
            TRACE2("ioctl: ioct( %d, %x ) rc %d errno %d\n", fd, request, rc, errno );
        }
        else
        {
            TRACE2("ioctl: ioct( %d, %x ) rc %d\n", fd, request, rc );
            if ( (request == (int)VIDIOC_G_FMT) || (request == (int)VIDIOC_S_FMT) )
            {
                struct v4l2_format *format= (struct v4l2_format*)arg;
                if ( (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
                    (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) )
                {
                    TRACE2("ioctl: pix_mp: pixelFormat %X w %d h %d field %d cs %d flg %x num_planes %d p0: sz %d bpl %d p1: sz %d bpl %d\n",
                        format->fmt.pix_mp.pixelformat,
                        format->fmt.pix_mp.width,
                        format->fmt.pix_mp.height,
                        format->fmt.pix_mp.field,
                        format->fmt.pix_mp.colorspace,
                        format->fmt.pix_mp.flags,
                        format->fmt.pix_mp.num_planes,
                        format->fmt.pix_mp.plane_fmt[0].sizeimage,
                        format->fmt.pix_mp.plane_fmt[0].bytesperline,
                        format->fmt.pix_mp.plane_fmt[1].sizeimage,
                        format->fmt.pix_mp.plane_fmt[1].bytesperline
                    );
                }
                else if ( (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
                        (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) )
                {
                    TRACE2("ioctl: pix: pixelFormat %X w %d h %d field %d bpl %d\n",
                        format->fmt.pix.pixelformat,
                        format->fmt.pix.width,
                        format->fmt.pix.height,
                        format->fmt.pix.field,
                        format->fmt.pix.bytesperline
                    );
                }
            }
            else if ( request == (int)VIDIOC_REQBUFS )
            {
                struct v4l2_requestbuffers *rb= (struct v4l2_requestbuffers*)arg;
                TRACE2("ioctl: count %d type %d mem %d\n", rb->count, rb->type, rb->memory);
            }
            else if ( request == (int)VIDIOC_CREATE_BUFS )
            {
                struct v4l2_create_buffers *cb= (struct v4l2_create_buffers*)arg;
                TRACE2("ioctl: index %d count %d mem %d\n", cb->index, cb->count, cb->memory);
            }
            else if ( request == (int)VIDIOC_QUERYBUF )
            {
                struct v4l2_buffer *buf= (struct v4l2_buffer*)arg;
                TRACE2("ioctl: index %d type %d flags %X mem %d\n", buf->index, buf->type, buf->flags, buf->memory);
            }
            else if ( request == (int)VIDIOC_G_CTRL )
            {
                struct v4l2_control *ctrl= (struct v4l2_control*)arg;
                TRACE2("ioctl: id %d value %d\n", ctrl->id, ctrl->value);
            }
            else if ( request == (int)VIDIOC_DQBUF )
            {
                struct v4l2_buffer *buf= (struct v4l2_buffer*)arg;
                TRACE2("ioctl: buff: index %d f dq: type %d bytesused %d flags %X field %d mem %x length %d seq %d timestamp sec %ld usec %ld\n",
                    buf->index, buf->type, buf->bytesused, buf->flags, buf->field, buf->memory, buf->length, buf->sequence, buf->timestamp.tv_sec, buf->timestamp.tv_usec);
                if ( buf->m.planes &&
                    ( (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
                    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ) )
                {
                    if ( buf->memory == V4L2_MEMORY_MMAP )
                    {
                        TRACE2("ioctl: buff: p0: bu %d len %d moff %d doff %d p1: bu %d len %d moff %d doff %d\n",
                            buf->m.planes[0].bytesused, buf->m.planes[0].length, buf->m.planes[0].m.mem_offset, buf->m.planes[0].data_offset,
                            buf->m.planes[1].bytesused, buf->m.planes[1].length, buf->m.planes[1].m.mem_offset, buf->m.planes[1].data_offset );
                    }
                    else if ( buf->memory == V4L2_MEMORY_DMABUF )
                    {
                        TRACE2("ioctl: buff: p0: bu %d len %d fd %d doff %d p1: bu %d len %d fd %d doff %d\n",
                            buf->m.planes[0].bytesused, buf->m.planes[0].length, buf->m.planes[0].m.fd, buf->m.planes[0].data_offset,
                            buf->m.planes[1].bytesused, buf->m.planes[1].length, buf->m.planes[1].m.fd, buf->m.planes[1].data_offset );
                    }
                }
            }
            else if ( request == (int)VIDIOC_ENUM_FRAMESIZES )
            {
                struct v4l2_frmsizeenum *frmsz= (struct v4l2_frmsizeenum*)arg;
                TRACE2("ioctl: fmt %x idx %d type %d\n", frmsz->pixel_format, frmsz->index, frmsz->type);
                if ( frmsz->type == V4L2_FRMSIZE_TYPE_DISCRETE )
                {
                    TRACE2("ioctl: discrete %dx%d\n", frmsz->discrete.width, frmsz->discrete.height);
                }
                else if ( frmsz->type == V4L2_FRMSIZE_TYPE_STEPWISE )
                {
                    TRACE2("ioctl: stepwise minw %d maxw %d stepw %d minh %d maxh %d steph %d\n",
                        frmsz->stepwise.min_width, frmsz->stepwise.max_width, frmsz->stepwise.step_width,
                        frmsz->stepwise.min_height, frmsz->stepwise.max_height, frmsz->stepwise.step_height );
                }
                else
                {
                    TRACE2("ioctl: continuous\n");
                }
            }
            else if ( request == (int)VIDIOC_DQEVENT )
            {
                struct v4l2_event *event= (struct v4l2_event*)arg;
                TRACE2("ioctl: event: type %d pending %d\n", event->type, event->pending);
                if ( event->type == V4L2_EVENT_SOURCE_CHANGE )
                {
                    TRACE2("ioctl: changes %X\n", event->u.src_change.changes );
                }
            }
        }
    }
    return rc;
}

static void dumpRenderBuffer(RenderBuffer * buffer)
{
    if (!buffer) {
        return;
    }
    DEBUG("+++++flag:%x,pts:%lld,priv:%p",buffer->flag,buffer->pts,buffer->priv);
    DEBUG("+++++dma buffer,width:%d,height:%d",buffer->dma.width,buffer->dma.height);
    for (int i = 0; i < buffer->dma.planeCnt; i++) {
        DEBUG("+++++handle:%d,fd:%d,stride:%d,offset:%d,size:%d",buffer->dma.handle[i],buffer->dma.fd[i],buffer->dma.stride[i],buffer->dma.offset[i],buffer->dma.size[i]);
    }
}

static void dumpVoutDmaBuffer(VoutDmaBuffer * buffer)
{
    if (!buffer) {
        return;
    }
    TRACE3("+++++vout dma buffer,width:%d,height:%d,pixel:%u,pts:%llu",buffer->width,buffer->height,buffer->pixel,buffer->pts);
    for (int i = 0; i < buffer->planeCnt; i++) {
        TRACE3("+++++handle:%d,fd:%d,stride:%d,offset:%d,size:%d",buffer->handle[i],buffer->fd[i],buffer->stride[i],buffer->offset[i],buffer->size[i]);
    }
}

static int adaptFd( int fdin )
{
    int fdout= fdin;
    if ( fdin >= 0 )
    {
        //int fddup = fcntl( fdin, F_DUPFD_CLOEXEC, 0 );
        int fddup = dup(fdin);
        if ( fddup >= 0 )
        {
            close( fdin );
            fdout= fddup;
        }
    }
    return fdout;
}

static int64_t pts90KToNs(int64_t pts)
{
    return (((pts*1000000))/90);
}