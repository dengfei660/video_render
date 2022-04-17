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
#include <sys/un.h>
#include <linux/netlink.h>
#include <time.h>
#include <unistd.h>
#include "socket_sink.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"
#include "sink_manager.h"

using namespace Tls;

#define TAG "rlib:socket_sink"
#define SOCKET_NAME "render"


static RenderLibWrapCallback renderlibCallback {
    SocketSink::handleFrameDisplayed,
    SocketSink::handleBufferRelease
};

void SocketSink::handleBufferRelease(void* userData, RenderBuffer *buffer)
{
    SocketSink *self = static_cast<SocketSink *>(userData);
    size_t bufferId = (size_t)buffer->priv;

    for (int i = 0; i < buffer->dma.planeCnt; i++) {
        if (buffer->dma.fd[i] >= 0) {
            DEBUG(NO_CATEGERY,"close dma fd:%d",buffer->dma.fd[i]);
            close(buffer->dma.fd[i]);
        }
    }

    self->videoServerSendBufferRelease(bufferId);
    self->mRenderlib->releaseRenderBuffer(buffer);
}

void SocketSink::handleFrameDisplayed(void* userData, RenderBuffer *buffer)
{
    int droppedFrames;
    SocketSink *self = static_cast<SocketSink *>(userData);
    size_t bufferId = (size_t)buffer->priv;
    self->mRenderlib->getDroppedFrames(&droppedFrames);
    self->videoServerSendStatus(buffer->pts, droppedFrames,bufferId);
}

SocketSink::SocketSink(SinkManager *sinkMgr, int socketfd, uint32_t vdecPort)
    : mSinkMgr(sinkMgr),
    mSocketFd(socketfd),
    mVdecPort(vdecPort)
{
    TRACE2(NO_CATEGERY,"in");
    mState = STATE_CREATE;
    mRenderlib = NULL;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mFrameCnt = 0;
    mIsPixFormatSet = false;
    mIsPeerSocketConnect = true;
    mVdoPort = INVALIDE_PORT;
    TRACE2(NO_CATEGERY,"out");
}

SocketSink::~SocketSink()
{
    TRACE2(NO_CATEGERY,"in");
    mState = STATE_DESTROY;
    if (mSocketFd >= 0) {
        shutdown(mSocketFd, SHUT_RDWR );
    }
    if (mRenderlib) {
        mRenderlib->disconnectRender();
        delete mRenderlib;
        mRenderlib = NULL;
    }
    if (mSocketFd >= 0) {
        close(mSocketFd);
        mSocketFd = -1;
    }
    if (isRunning()) {
        requestExitAndWait();
    }
    TRACE2(NO_CATEGERY,"out");
}

bool SocketSink::start()
{
    DEBUG(NO_CATEGERY,"in");
    States oldState = mState;
    if (mState >= STATE_START) {
        WARNING(NO_CATEGERY,"had started");
        return true;
    }
    mState = STATE_START;
    mRenderlib = new RenderLibWrap(mVdecPort, mVdoPort);
    bool ret = mRenderlib->connectRender((char *)COMPOSITOR_NAME, INVALID_VIDEOTUNNEL_ID);
    if (!ret) {
        ERROR(NO_CATEGERY,"render lib connect render fail");
        mState = oldState;
        return false;
    }

    mRenderlib->setCallback(this, &renderlibCallback);

    mRenderlib->setMediasyncTunnelMode(1); //notunnel mode

    //to start thread
    run("socketSink");
    DEBUG(NO_CATEGERY,"out");
    return true;
}

bool SocketSink::stop()
{
    DEBUG(NO_CATEGERY,"in");
    if (mState >= STATE_STOP) {
        WARNING(NO_CATEGERY,"had stopped");
        return true;
    }
    mState = STATE_STOP;
    if (mSocketFd >= 0) {
        shutdown(mSocketFd, SHUT_RDWR );
    }
    if (mRenderlib) {
        mRenderlib->disconnectRender();
        delete mRenderlib;
        mRenderlib = NULL;
    }
    if (mSocketFd >= 0) {
        close(mSocketFd);
        mSocketFd = -1;
    }
    if (isRunning()) {
        requestExitAndWait();
    }
    DEBUG(NO_CATEGERY,"out");
    return true;
}

void SocketSink::videoServerSendStatus(long long displayedFrameTime, int dropFrameCount, int bufIndex)
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[4+8+4+4];
    int len;
    int sentLen;

    Tls::Mutex::Autolock _l(mMutex);

    msg.msg_name= NULL;
    msg.msg_namelen= 0;
    msg.msg_iov= iov;
    msg.msg_iovlen= 1;
    msg.msg_control= 0;
    msg.msg_controllen= 0;
    msg.msg_flags= 0;

    len= 0;
    mbody[len++]= 'V';
    mbody[len++]= 'S';
    mbody[len++]= 17;
    mbody[len++]= 'S';
    len += putS64( &mbody[len], displayedFrameTime );
    len += putU32( &mbody[len], dropFrameCount );
    len += putU32( &mbody[len], bufIndex );

    iov[0].iov_base= (char*)mbody;
    iov[0].iov_len= len;

    do
    {
        sentLen = sendmsg(mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        TRACE1(NO_CATEGERY,"send status: frameTime %lld dropCount %d,bufferid:0x%x to client", displayedFrameTime, dropFrameCount,bufIndex);
    }
}

void SocketSink::videoServerSendBufferRelease(int bufferId)
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[4+4];
    int len;
    int sentLen;

    Tls::Mutex::Autolock _l(mMutex);

    msg.msg_name= NULL;
    msg.msg_namelen= 0;
    msg.msg_iov= iov;
    msg.msg_iovlen= 1;
    msg.msg_control= 0;
    msg.msg_controllen= 0;
    msg.msg_flags= 0;

    len= 0;
    mbody[len++]= 'V';
    mbody[len++]= 'S';
    mbody[len++]= 5;
    mbody[len++]= 'B';
    len += putU32( &mbody[4], bufferId );

    iov[0].iov_base= (char*)mbody;
    iov[0].iov_len= len;

    do
    {
        sentLen= sendmsg(mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        TRACE1(NO_CATEGERY,"send release buffer 0x%x to client", bufferId);
    }
}

int SocketSink::adaptFd( int fdin )
{
   int fdout= fdin;
   if ( fdin >= 0 )
   {
      int fddup= fcntl( fdin, F_DUPFD_CLOEXEC, 0 );
      if ( fddup >= 0 )
      {
         close( fdin );
         fdout= fddup;
      }
   }
   return fdout;
}

bool SocketSink::processEvent()
{
    int ret;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec iov[1];
    unsigned char mbody[4+64];
    char cmbody[CMSG_SPACE(3*sizeof(int))];
    uint32_t fbId= 0;
    uint32_t frameWidth, frameHeight;
    uint32_t frameFormat;
    int moff= 0, len, i, rc;
    int rectX, rectY, rectW, rectH;
    int fd0 = -1, fd1 = -1, fd2 = -1;
    int offset0, offset1, offset2;
    int stride0, stride1, stride2;
    size_t bufferId= 0;
    int64_t frameTime= 0;
    int planeCnt = 0;


    iov[0].iov_base= (char*)mbody;
    iov[0].iov_len= 4;

    cmsg= (struct cmsghdr*)cmbody;
    cmsg->cmsg_len= CMSG_LEN(3*sizeof(int));
    cmsg->cmsg_level= SOL_SOCKET;
    cmsg->cmsg_type= SCM_RIGHTS;

    msg.msg_name= NULL;
    msg.msg_namelen= 0;
    msg.msg_iov= iov;
    msg.msg_iovlen= 1;
    msg.msg_control= cmsg;
    msg.msg_controllen= cmsg->cmsg_len;
    msg.msg_flags= 0;

    len= recvmsg( mSocketFd, &msg, 0 /*flag`*/);
    if (len <= 0) { //do receive next msg
        WARNING(NO_CATEGERY,"video server peer disconnected");
        return false;
    }

    if (len == 4) {
        unsigned char *m= mbody;
        if ( (m[0] == 'V') && (m[1] == 'S') )
        {
            int mlen, id;
            mlen= m[2];
            id= m[3];
            switch ( id )
            {
                case 'F':
                    cmsg= CMSG_FIRSTHDR(&msg);
                    if ( cmsg &&
                        cmsg->cmsg_level == SOL_SOCKET &&
                        cmsg->cmsg_type == SCM_RIGHTS &&
                        cmsg->cmsg_len >= CMSG_LEN(sizeof(int)) )
                    {
                        fd0 = adaptFd( ((int*)CMSG_DATA(cmsg))[0] );
                        ++planeCnt;
                        if ( cmsg->cmsg_len >= CMSG_LEN(2*sizeof(int)) )
                        {
                           fd1 = adaptFd( ((int*)CMSG_DATA(cmsg))[1] );
                           ++planeCnt;
                        }
                        if ( cmsg->cmsg_len >= CMSG_LEN(3*sizeof(int)) )
                        {
                           fd2 = adaptFd( ((int*)CMSG_DATA(cmsg))[2] );
                           ++planeCnt;
                        }
                    }
                    break;
                default:
                    break;
            }

            if ( mlen > sizeof(mbody) )
            {
                ERROR(NO_CATEGERY,"bad message length: %d : truncating");
                mlen= sizeof(mbody);
            }
            if ( mlen > 1 )
            {
                iov[0].iov_base= (char*)mbody+4;
                iov[0].iov_len= mlen-1;

                msg.msg_name= NULL;
                msg.msg_namelen= 0;
                msg.msg_iov= iov;
                msg.msg_iovlen= 1;
                msg.msg_control= 0;
                msg.msg_controllen= 0;
                msg.msg_flags= 0;

                do
                {
                    len= recvmsg( mSocketFd, &msg, 0 );
                }
                while ( (len < 0) && (errno == EINTR));
            }

            if ( len > 0 )
            {
                len += 4;
                m += 3;
                //if ( Logger_get_level() >= 7 )
                //{
                //     dumpMessage( (char *)mbody, len );
                //}
                switch ( id )
                {
                    case 'F':
                    if ( fd0 >= 0 )
                    {
                        uint32_t handle0, handle1;

                        frameWidth= (getU32( m+1 ) & ~1);
                        frameHeight= ((getU32( m+5)+1) & ~1);
                        frameFormat= getU32( m+9);
                        rectX= (int)getU32( m+13 );
                        rectY= (int)getU32( m+17 );
                        rectW= (int)getU32( m+21 );
                        rectH= (int)getU32( m+25 );
                        offset0= (int)getU32( m+29 );
                        stride0= (int)getU32( m+33 );
                        offset1= (int)getU32( m+37 );
                        stride1= (int)getU32( m+41 );
                        offset2= (int)getU32( m+45 );
                        stride2= (int)getU32( m+49 );
                        bufferId= (int)getU32( m+53 );
                        frameTime= (long long)getS64( m+57 );
                        mFrameCnt += 1;
                        TRACE2(NO_CATEGERY,"got frame %d buffer 0x%x frameTime %lld", mFrameCnt, bufferId, frameTime);

                        TRACE2(NO_CATEGERY,"got frame fd %d,%d,%d (%dx%d) %X (%d, %d, %d, %d) off(%d, %d, %d) stride(%d, %d, %d)",
                                fd0, fd1, fd2, frameWidth, frameHeight, frameFormat, rectX, rectY, rectW, rectH,
                                offset0, offset1, offset2, stride0, stride1, stride2 );

                        if (mFrameWidth != frameWidth || mFrameHeight != frameHeight) {
                            mFrameWidth = frameWidth;
                            mFrameHeight = frameHeight;
                            mRenderlib->setFrameSize(mFrameWidth, mFrameHeight);
                        }
                        if (mIsPixFormatSet == false) {
                            mRenderlib->setVideoFormat(frameFormat);
                            mIsPixFormatSet = true;
                        }

                        RenderBuffer *renderBuf = mRenderlib->allocRenderBuffer();
                        renderBuf->pts = frameTime;
                        renderBuf->dma.width = frameWidth;
                        renderBuf->dma.height = frameHeight;
                        renderBuf->dma.planeCnt = planeCnt;
                        renderBuf->dma.fd[0] = fd0;
                        renderBuf->dma.stride[0] = stride0;
                        renderBuf->dma.offset[0] = offset0;
                        renderBuf->dma.fd[1] = fd1;
                        renderBuf->dma.stride[1] = stride1;
                        renderBuf->dma.offset[1] = offset1;
                        renderBuf->dma.fd[2] = fd0;
                        renderBuf->dma.stride[2] = stride2;
                        renderBuf->dma.offset[2] = offset2;
                        renderBuf->priv = (void *)bufferId;
                        mRenderlib->renderFrame(renderBuf);
                    }
                    break;
                    case 'S':
                    {
                        DEBUG(NO_CATEGERY,"got flush");
                        mRenderlib->flush();
                    }
                    break;
                    case 'P':
                    {
                        bool pause= (m[1] == 1);
                        DEBUG(NO_CATEGERY,"got pause (%d)", pause);
                        if (pause) {
                            mRenderlib->pause();
                        } else {
                            mRenderlib->resume();
                        }
                    }
                    break;
                    case 'I':
                    {
                        int syncType= m[1];
                        int sessionId= getU32( m+2 );
                        DEBUG(NO_CATEGERY,"got session info: sync type %d sessionId %d", syncType, sessionId);
                        mRenderlib->setMediasyncId(sessionId);
                        mRenderlib->setMediasyncSyncMode(syncType);
                    }
                    break;
                    case 'W':
                    {
                        rectX= (int)getU32( m+1 );
                        rectY= (int)getU32( m+5 );
                        rectW= (int)getU32( m+9 );
                        rectH= (int)getU32( m+13 );
                        DEBUG(NO_CATEGERY,"got position : (%d, %d, %d, %d)",rectX, rectY, rectW, rectH);
                        mRenderlib->setWindowSize(rectX, rectY, rectW, rectH);
                    }
                    break;
                    case 'R':
                    {
                        int num, denom;
                        num= (int)getU32( m+1 );
                        denom= (int)getU32( m+5 );
                        DEBUG(NO_CATEGERY,"got frame rate  (%d / %d)", num, denom);
                        mRenderlib->setVideoFps(num, denom);
                    }
                    break;
                    default:
                    ERROR(NO_CATEGERY,"got unknown video server message: mlen %d", mlen);
                    //wstDumpMessage( mbody, mlen+3 );
                    break;
                }
            }
        }
    }
    return true;
}

void SocketSink::readyToRun()
{
    DEBUG(NO_CATEGERY,"in");
    mState = STATE_RUNNING;

    DEBUG(NO_CATEGERY,"out");
}

void SocketSink::readyToExit()
{
    DEBUG(NO_CATEGERY,"in");
    /*if peer socket had disconnect
    destroy this socket sink*/
    if (!mIsPeerSocketConnect) {
        mSinkMgr->destroySink(mVdecPort, mVdoPort);
    }
    DEBUG(NO_CATEGERY,"out");
}

bool SocketSink::threadLoop()
{
    bool ret;

    if (mSocketFd < 0) {
        WARNING(NO_CATEGERY,"Not open socket server fd");
        return false;
    }

    ret = processEvent();
    if (!ret) { //exit thread
        mIsPeerSocketConnect = false;
        return false;
    }

    return true;
}