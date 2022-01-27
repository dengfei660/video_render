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
#include "render_server.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"

#include "videodev2-ext.h"
#include "v4l2-controls-ext.h"
#include "v4l2-ext-frontend.h"

using namespace Tls;

#define TAG "rlib:vdo_server"
#define V4L2_DEVICE "/dev/video28"
#define COMPOSITOR_NAME "wayland"

#define NUM_CAPTURE_BUFFERS (6)

static int IOCTL( int fd, int request, void* arg );

VDOServerThread::VDOServerThread(RenderServer *renderServer, uint32_t ctrId, uint32_t vdoPort, uint32_t vdecPort)
    :mCtrId(ctrId),
    mVdoPort(vdoPort),
    mVdecPort(vdecPort),
    mRenderServer(renderServer)
{
    DEBUG("in,ctrid:%d,vdoport:%d,vdecport:%d",ctrId,vdoPort,vdecPort);
    mFrameWidth = 0;
    mFrameHeight = 0;
    mRenderInstance = NULL;
    mV4l2Fd = -1;
    mNumCaptureBuffers = 0;
    mMinCaptureBuffers = 0;
    mCaptureBuffers = NULL;
    mIsSetCaptureFmt = false;
    mDecoderEos = false;
    mDecodedFrameCnt = 0;
    mDisplayedFrameCnt = 0;
    mQueuedCaptureBufferCnt = 0;
    mDecoderLastFrame = false;
    mPoll = new Poll(true);
    DEBUG("out");
}

VDOServerThread::~VDOServerThread()
{
    DEBUG("in");
    if (mPoll) {
        if (isRunning()) {
            DEBUG("try stop thread");
            mPoll->setFlushing(true);
            requestExitAndWait();
        }
        delete mPoll;
        mPoll = NULL;
    }

    if (mV4l2Fd >= 0) {
        close(mV4l2Fd);
        mV4l2Fd = -1;
    }
    if (mCaptureBuffers) {
        free(mCaptureBuffers);
    }
    if (mRenderInstance) {
        render_disconnect(mRenderInstance);
        render_close(mRenderInstance);
        mRenderInstance = NULL;
    }
    DEBUG("out");
}

void VDOServerThread::msgCallback(void *userData , RenderMsgType type, void *msg)
{
    int rc;
    VDOServerThread* vdoserver = static_cast<VDOServerThread *>(userData);
    switch (type)
    {
        case MSG_RELEASE_BUFFER:{
            RenderBuffer *renderBuffer = (RenderBuffer *)msg;
            BufferInfo * buf = (BufferInfo *)renderBuffer->priv;
            vdoserver->queueCaptureBuffers(buf->bufferId);
            render_free_render_buffer_wrap(vdoserver->mRenderInstance, renderBuffer);
        } break;
        case MSG_DISPLAYED_BUFFER:{
            vdoserver->mDisplayedFrameCnt += 1;
        } break;

        default:
            break;
    }
}

int VDOServerThread::getCallback(void *userData, int key, void *value)
{
    //get mediasync id
    if (KEY_MEDIASYNC_INSTANCE_ID) {

    }
    return 0;
}

bool VDOServerThread::getCaptureBufferFrameSize()
{
    struct v4l2_selection selection;
    int rc;

    memset( &selection, 0, sizeof(selection) );
    selection.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    selection.target= V4L2_SEL_TGT_COMPOSE_DEFAULT;
    rc= IOCTL( mV4l2Fd, VIDIOC_G_SELECTION, &selection );
    if ( rc < 0 )
    {
        memset( &selection, 0, sizeof(selection) );
        selection.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        selection.target= V4L2_SEL_TGT_COMPOSE_DEFAULT;
        rc= IOCTL( mV4l2Fd, VIDIOC_G_SELECTION, &selection );
        if ( rc < 0 )
        {
            WARNING("failed to get compose rect: rc %d errno %d", rc, errno );
        }
    }
    DEBUG("Out compose default: (%d, %d, %d, %d)", selection.r.left, selection.r.top, selection.r.width, selection.r.height );

    memset( &selection, 0, sizeof(selection) );
    selection.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    selection.target = V4L2_SEL_TGT_COMPOSE;
    rc= IOCTL( mV4l2Fd, VIDIOC_G_SELECTION, &selection );
    if ( rc < 0 )
    {
        WARNING("failed to get compose rect: rc %d errno %d", rc, errno );
    }
    DEBUG("Out compose: (%d, %d, %d, %d)", selection.r.left, selection.r.top, selection.r.width, selection.r.height );

    if ( rc == 0 )
    {
        mFrameWidth = selection.r.width;
        mFrameHeight = selection.r.height;
    }
    INFO("frame size %dx%d\n", mFrameWidth, mFrameHeight);
    return true;
}

bool VDOServerThread::setCaptureBufferFormat()
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

    mCaptureFmt.fmt.pix.pixelformat= V4L2_PIX_FMT_NV12;
    mCaptureFmt.fmt.pix.width= mFrameWidth;
    mCaptureFmt.fmt.pix.height= mFrameHeight;
    mCaptureFmt.fmt.pix.sizeimage= (mCaptureFmt.fmt.pix.width*mCaptureFmt.fmt.pix.height*3)/2;
    mCaptureFmt.fmt.pix.field= V4L2_FIELD_ANY;

    rc= IOCTL(mV4l2Fd, VIDIOC_S_FMT, &mCaptureFmt );
    if ( rc < 0 )
    {
        DEBUG("failed to set format for output: rc %d errno %d", rc, errno);
        return false;
    }
    mIsSetCaptureFmt = true;
    return true;
}

bool VDOServerThread::setupCapture()
{
    getCaptureBufferFrameSize();
    setCaptureBufferFormat();
    setupCaptureBuffers();
    return true;
}

bool VDOServerThread::setupCaptureBuffers()
{
    int rc, neededBuffers;
    struct v4l2_control ctl;
    struct v4l2_requestbuffers reqbuf;
    int i, j;
    bool result = false;

    if (!mIsSetCaptureFmt) {
        WARNING("not set capture buffer format");
        return false;
    }

    neededBuffers = NUM_CAPTURE_BUFFERS;

    memset( &ctl, 0, sizeof(ctl));
    ctl.id= V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
    rc= IOCTL( mV4l2Fd, VIDIOC_G_CTRL, &ctl );
    if ( rc == 0 )
    {
        mMinCaptureBuffers = ctl.value;
        if ( (mMinCaptureBuffers != 0) && (mMinCaptureBuffers > NUM_CAPTURE_BUFFERS) )
        {
            neededBuffers= mMinCaptureBuffers + 1;
        }
    }

    if ( mMinCaptureBuffers == 0 )
    {
        mMinCaptureBuffers = NUM_CAPTURE_BUFFERS;
    }

    memset( &reqbuf, 0, sizeof(reqbuf) );
    reqbuf.count= neededBuffers;
    reqbuf.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory= V4L2_MEMORY_MMAP;
    rc= IOCTL( mV4l2Fd, VIDIOC_REQBUFS, &reqbuf );
    if ( rc < 0 )
    {
        ERROR("failed to request %d mmap buffers for output: rc %d errno %d", neededBuffers, rc, errno);
        goto exit;
    }

    mNumCaptureBuffers = reqbuf.count;

    if ( reqbuf.count < mMinCaptureBuffers )
    {
        ERROR("wstSetupOutputBuffers: insufficient buffers: (%d versus %d)", reqbuf.count, neededBuffers );
        goto exit;
    }

    mCaptureBuffers = (BufferInfo *)calloc(reqbuf.count, sizeof(BufferInfo));
    if (!mCaptureBuffers) {
        ERROR("No memory to alloc capturebuffers mgr");
        goto exit;
    }

    for (i = 0; i < mNumCaptureBuffers; i++) {
        mCaptureBuffers[i].bufferId = i;
        mCaptureBuffers[i].fd= -1;
    }

    result = setupMmapCaptureBuffers();

exit:

   if ( !result )
   {
      tearDownCaptureBuffers();
   }
   return result;
}

bool VDOServerThread::setupMmapCaptureBuffers()
{
    bool result= false;
    struct v4l2_buffer *bufOut;
    struct v4l2_exportbuffer expbuf;
    void *bufStart;
    int rc, i, j;

    for( i= 0; i < mNumCaptureBuffers; ++i )
    {
        bufOut = &mCaptureBuffers[i].buf;
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

        memset( &expbuf, 0, sizeof(expbuf) );
        expbuf.type= bufOut->type;
        expbuf.index= i;
        expbuf.flags= O_CLOEXEC;
        rc= IOCTL( mV4l2Fd, VIDIOC_EXPBUF, &expbuf );
        if ( rc < 0 )
        {
            ERROR("failed to export v4l2 output buffer %d: rc %d errno %d", i, rc, errno);
        }
        DEBUG("  index %d export fd %d", expbuf.index, expbuf.fd );

        mCaptureBuffers[i].fd = expbuf.fd;
        mCaptureBuffers[i].capacity = bufOut->length;

        if ( true )
        {
            bufStart= mmap( NULL,
                            bufOut->length,
                            PROT_READ,
                            MAP_SHARED,
                            mV4l2Fd,
                            bufOut->m.offset );
            if ( bufStart != MAP_FAILED )
            {
                mCaptureBuffers[i].start= bufStart;
            }
            else
            {
                ERROR("failed to mmap input buffer %d: errno %d", i, errno);
            }
        }
    }

exit:
   return result;
}

void VDOServerThread::tearDownCaptureBuffers()
{
    int rc;
    struct v4l2_requestbuffers reqbuf;

    if ( mCaptureBuffers )
    {
        rc= IOCTL( mV4l2Fd, VIDIOC_STREAMOFF, &mCaptureFmt.type );
        if ( rc < 0 )
        {
            ERROR("wstTearDownOutputBuffers: streamoff failed for output: rc %d errno %d", rc, errno );
        }

        tearDownMmapCaptureBuffers();

        free( mCaptureBuffers );
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
            ERROR("wstTearDownOutputBuffers: failed to release v4l2 buffers for output: rc %d errno %d", rc, errno);
        }
        mNumCaptureBuffers = 0;
    }
}

void VDOServerThread::tearDownMmapCaptureBuffers()
{
    int i, j;

    for( i= 0; i < mNumCaptureBuffers; ++i )
    {
        if ( mCaptureBuffers[i].start )
        {
            munmap( mCaptureBuffers[i].start, mCaptureBuffers[i].capacity );
        }
        if ( mCaptureBuffers[i].fd >= 0 )
        {
            close( mCaptureBuffers[i].fd );
            mCaptureBuffers[i].fd = -1;
        }
    }
}

bool VDOServerThread::queueAllCaptureBuffers()
{
    bool ret = true;
    int i, j, rc;
    for( int i= 0; i < mNumCaptureBuffers; ++i )
    {
        rc= IOCTL(mV4l2Fd, VIDIOC_QBUF, &mCaptureBuffers[i].buf );
        if ( rc < 0 )
        {
            ERROR("failed to queue output buffer: rc %d errno %d", rc, errno);
            ret = false;
            goto exit;
        }
        mQueuedCaptureBufferCnt += 1;
        mCaptureBuffers[i].queued = true;
    }

exit:
    return ret;
}

bool VDOServerThread::queueCaptureBuffers(int bufIndex)
{
    bool ret = true;
    int rc;
    rc= IOCTL(mV4l2Fd, VIDIOC_QBUF, &mCaptureBuffers[bufIndex].buf );
    if ( rc < 0 )
    {
        ERROR("failed to queue output buffer: rc %d errno %d", rc, errno);
        return false;
    }
    mQueuedCaptureBufferCnt += 1;
    mCaptureBuffers[bufIndex].queued = true;
    return true;
}

int VDOServerThread::dequeueCaptureBuffer()
{
    int bufferIndex = -1;
    int rc;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[MAX_PLANES];

    if ( mDecoderLastFrame )
    {
        WARNING("had decoded last frame");
        goto exit;
    }
    memset( &buf, 0, sizeof(buf));
    buf.type= mCaptureFmt.type;
    buf.memory= V4L2_MEMORY_MMAP;
    rc = IOCTL( mV4l2Fd, VIDIOC_DQBUF, &buf );
    if ( rc == 0 )
    {
        bufferIndex = buf.index;
        memcpy(&mCaptureBuffers[bufferIndex].buf, &buf, sizeof(struct v4l2_buffer));
        //mCaptureBuffers[bufferIndex].buf = buf;
        mCaptureBuffers[bufferIndex].queued = false;
        mQueuedCaptureBufferCnt -= 1;
    }
    else
    {
        ERROR("failed to de-queue output buffer: rc %d errno %d", rc, errno);
        if ( errno == EPIPE )
        {
            /* Decoding is done: no more capture buffers can be dequeued */
            mDecoderLastFrame = true;
        }
    }

exit:
   return bufferIndex;
}

bool VDOServerThread::processEvent()
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
            memset( &fmtOut, 0, sizeof(fmtOut));
            fmtOut.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
            rc= IOCTL( mV4l2Fd, VIDIOC_G_FMT, &fmtOut );
            if ((mNumCaptureBuffers == 0) ||
                    (((fmtOut.fmt.pix.width != mCaptureFmt.fmt.pix.width) ||
                        (fmtOut.fmt.pix.height != mCaptureFmt.fmt.pix.height))) ||
                    (mDecodedFrameCnt > 0) ) {
                tearDownCaptureBuffers();
                mFrameWidth= fmtOut.fmt.pix.width;
                mFrameHeight= fmtOut.fmt.pix.height;

                getCaptureBufferFrameSize();
                setCaptureBufferFormat();
                setupCaptureBuffers();
                mNeedCaptureRestart = true;
            }
        } else if (event.type == V4L2_EVENT_EOS) {
            INFO("v4l2 decoder eos event\n");
            mDecoderEos = true;
        }
    }
    return ret;
}

bool VDOServerThread::vdoConnect()
{
    int rc;
    struct v4l2_ext_controls ext_controls;
    struct v4l2_ext_control ext_control;
    struct v4l2_ext_vdec_vdo_connection vdo_con;

    memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
    memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
    memset(&vdo_con, 0, sizeof(struct v4l2_ext_vdec_vdo_connection));

    vdo_con.vdo_port = mVdoPort;  //vdo port number
    vdo_con.vdec_port = mVdecPort; //vdec port number

    ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_controls.count = 1;
    ext_controls.controls = &ext_control;
    ext_controls.controls->id = V4L2_CID_EXT_VDO_VDEC_CONNECTING;
    ext_controls.controls->ptr = (void *)&vdo_con;

    rc = IOCTL(mV4l2Fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
    if (rc < 0) {
        ERROR("connect vdo fail");
        return false;
    }
    return true;
}

bool VDOServerThread::vdoDisconnect()
{
    int rc;
    struct v4l2_ext_controls ext_controls;
    struct v4l2_ext_control ext_control;
    struct v4l2_ext_vdec_vdo_connection vdo_con;

    memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
    memset(&ext_control, 0, sizeof(struct v4l2_ext_control));
    memset(&vdo_con, 0, sizeof(struct v4l2_ext_vdec_vdo_connection));

    vdo_con.vdo_port = mVdoPort;  //vdo port number
    vdo_con.vdec_port = mVdecPort; //vdec port number

    ext_controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_controls.count = 1;
    ext_controls.controls = &ext_control;
    ext_controls.controls->id = V4L2_CID_EXT_VDO_VDEC_DISCONNECTING;
    ext_controls.controls->ptr = (void *)&vdo_con;

    rc = IOCTL(mV4l2Fd, VIDIOC_S_EXT_CTRLS, &ext_controls);
    if (rc < 0) {
        ERROR("connect vdo fail");
        return false;
    }
    return true;
}

bool VDOServerThread::init()
{
    struct v4l2_capability caps;
    int rc;
    RenderCallback renderCallback;

    mV4l2Fd = open( V4L2_DEVICE, O_RDWR | O_CLOEXEC);
    if (mV4l2Fd < 0) {
        ERROR("open v4l2 device fail,device name:%s",V4L2_DEVICE);
        return false;
    }

    rc= IOCTL(mV4l2Fd, VIDIOC_QUERYCAP, &caps );
    if (rc < 0) {
        ERROR("failed query caps: %d errno %d", rc, errno);
        goto tag_exit;
    }

    DEBUG("driver (%s) card(%s) bus_info(%s) version %d capabilities %X device_caps %X",
           caps.driver, caps.card, caps.bus_info, caps.version, caps.capabilities, caps.device_caps );

    mDeviceCaps = (caps.capabilities & V4L2_CAP_DEVICE_CAPS ) ? caps.device_caps : caps.capabilities;

    if ( !(mDeviceCaps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE) ))
    {
        WARNING("device (%s) is not a capture device", V4L2_DEVICE );
    }

    //check support dma buffer
    struct v4l2_exportbuffer eb;
    eb.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    eb.index= -1;
    eb.plane= -1;
    eb.flags= (O_RDWR|O_CLOEXEC);
    IOCTL( mV4l2Fd, VIDIOC_EXPBUF, &eb );
    if ( errno == ENOTTY )
    {
        ERROR("device (%s) does not support dmabuf: no VIDIOC_EXPBUF", V4L2_DEVICE );
        goto tag_exit;
    }

    //config vdo connect
    if (vdoConnect() == false) {
        goto tag_exit;
    }

    //open render lib
    mRenderInstance = render_open((char *)COMPOSITOR_NAME);
    if (!mRenderInstance) {
        ERROR("open render lib fail");
        goto tag_exit;
    }

    render_set_user_data(mRenderInstance, this);
    renderCallback.doMsgSend = msgCallback;
    renderCallback.doGetValue = getCallback;
    render_set_callback(mRenderInstance, &renderCallback);
    rc = render_connect(mRenderInstance);
    if (!rc) {
        ERROR("render lib connect fail");
        goto tag_exit;
    }

    return true;

tag_exit:
    if (mV4l2Fd > 0) {
        close(mV4l2Fd);
        mV4l2Fd = 0;
    }
    if (mRenderInstance) {
        render_close(mRenderInstance);
        mRenderInstance = NULL;
    }

   return false;
}

void VDOServerThread::readyToRun()
{
    DEBUG("in");
    if (mV4l2Fd >= 0) {
        mPoll->addFd(mV4l2Fd);
        mPoll->setFdReadable(mV4l2Fd, true);
    }

    getCaptureBufferFrameSize();
    setCaptureBufferFormat();
    DEBUG("out");
}

void VDOServerThread::readyToExit()
{
    DEBUG("in");
    vdoDisconnect();
    tearDownCaptureBuffers();
    DEBUG("out");
}

bool VDOServerThread::threadLoop()
{
    int ret;
    int rc;
    struct v4l2_event event;

    if (mV4l2Fd < 0) {
        WARNING("Not open v4l2 fd");
        return false;
    }

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }
    if (mIsSetCaptureFmt == false) {
        setupCapture();
    }

    //process v4l2 event
    processEvent();

    //dqueue capture buffer
    if (mPoll->isReadable(mV4l2Fd)) {
        int bufferIndex = dequeueCaptureBuffer();
        if (bufferIndex >=0 ) { //put frame to render lib
            BufferInfo * captureBuffer = &mCaptureBuffers[bufferIndex];
            RenderBuffer *renderBuf = render_allocate_render_buffer_wrap(mRenderInstance, BUFFER_FLAG_EXTER_DMA_BUFFER, 0);
            if (!renderBuf) {
                ERROR("render allocate buffer wrap fail");
                return true;
            }
            renderBuf->priv = (void *)captureBuffer;
            memcpy(&renderBuf->dma, captureBuffer->start, captureBuffer->buf.bytesused);
            //debug print
            for (int i = 0; i < renderBuf->dma.planeCnt; i++) {
                TRACE2("dma buf index:%d,fd:%d,stride:%d,offset:%d",i, renderBuf->dma.fd[i],renderBuf->dma.stride[i], renderBuf->dma.offset[i]);
            }
            render_display_frame(mRenderInstance, renderBuf);
        }
    }

    //at the last,we process capture restart
    if (mNeedCaptureRestart) {
        mNeedCaptureRestart = false;
        queueAllCaptureBuffers();
        rc= IOCTL( mV4l2Fd, VIDIOC_STREAMON, &mCaptureFmt.type );
        if ( rc < 0 )
        {
            ERROR("wstVideoOutputThread: streamon failed for output: rc %d errno %d", rc, errno );
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
        switch( request )
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
            TRACE2("ioctl: buff: index %d s dq: type %d bytesused %d flags %X field %d mem %x length %d timestamp sec %ld usec %ld\n",
                buf->index, buf->type, buf->bytesused, buf->flags, buf->field, buf->memory, buf->length, buf->timestamp.tv_sec, buf->timestamp.tv_usec);
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
            struct v4l2_ext_vdec_vdo_connection *vdo_con;
            struct v4l2_ext_controls *ext_controls = (struct v4l2_ext_controls *)arg;
            vdo_con = (struct v4l2_ext_vdec_vdo_connection *)ext_controls->controls->ptr;

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