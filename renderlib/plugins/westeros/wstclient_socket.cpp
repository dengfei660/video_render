#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include "wstclient_socket.h"
#include "Logger.h"
#include "Utils.h"

#define TAG "rlib:wstclient_socket"

WstClientSocket::WstClientSocket(const char *name, void *userData, wstOnEvent onEvent)
{
    int rc;
    bool error= true;
    const char *workingDir;
    int pathNameLen, addressSize;

    mUserData = userData;
    mSocketFd = -1;
    mName = name;
    mOnEventCallback = onEvent;

    mPoll = new Tls::Poll(true);

    workingDir = getenv("XDG_RUNTIME_DIR");
    if ( !workingDir )
    {
        ERROR("wstCreateVideoClientConnection: XDG_RUNTIME_DIR is not set");
        goto exit;
    }

    pathNameLen = strlen(workingDir)+strlen("/")+strlen(mName) + 1;
    if ( pathNameLen > (int)sizeof(mAddr.sun_path) )
    {
        ERROR("wstCreateVideoClientConnection: name for server unix domain socket is too long: %d versus max %d",
                pathNameLen, (int)sizeof(mAddr.sun_path) );
        goto exit;
    }

    mAddr.sun_family = AF_LOCAL;
    strcpy( mAddr.sun_path, workingDir );
    strcat( mAddr.sun_path, "/" );
    strcat( mAddr.sun_path, mName );

    mSocketFd = socket( PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0 );
    if ( mSocketFd < 0 )
    {
        ERROR("wstCreateVideoClientConnection: unable to open socket: errno %d", errno );
        goto exit;
    }

    addressSize = pathNameLen + offsetof(struct sockaddr_un, sun_path);

    rc = connect(mSocketFd, (struct sockaddr *)&mAddr, addressSize );
    if ( rc < 0 )
    {
        ERROR("wstCreateVideoClientConnection: connect failed for socket: errno %d", errno );
        goto exit;
    }

    run("wstclientSocket");

    INFO("wstclient socket connected");

    return;

exit:

    mAddr.sun_path[0] = '\0';
    if ( mSocketFd >= 0 )
    {
        close( mSocketFd );
        mSocketFd = -1;
    }
}

WstClientSocket::

~WstClientSocket()
{
    if (isRunning()) {
        TRACE1("try stop socket thread");
        if (mPoll) {
            mPoll->setFlushing(true);
        }
        requestExitAndWait();
    }
    if (mPoll) {
        delete mPoll;
        mPoll = NULL;
    }
    if ( mSocketFd >= 0 )
    {
        mAddr.sun_path[0]= '\0';
        close( mSocketFd );
        mSocketFd = -1;
    }
}


void WstClientSocket::sendResourceVideoClientConnection(int resId)
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[8];
    int len;
    int sentLen;
    int resourceId = ((resId >= 0) ? resId : 0);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len= 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 5;
    mbody[len++] = 'V';
    len += putU32( &mbody[len], resourceId );

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen= sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent resource id to video server,resourceId:%d",resId);
    }
}

void WstClientSocket::sendFlushVideoClientConnection()
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[4];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len = 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 1;
    mbody[len++] = 'S';

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen = sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent flush to video server");
    }
}

void WstClientSocket::sendPauseVideoClientConnection(bool pause)
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[7];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len= 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 2;
    mbody[len++] = 'P';
    mbody[len++] = (pause ? 1 : 0);

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen = sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent pause %d to video server", pause);
    }
}

void WstClientSocket::sendHideVideoClientConnection(bool hide)
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[7];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len= 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 2;
    mbody[len++] = 'H';
    mbody[len++] = (hide ? 1 : 0);

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen = sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent hide %d to video server", hide);
    }
}

void WstClientSocket::sendSessionInfoVideoClientConnection(int sessionId, int syncType )
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[9];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len= 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 6;
    mbody[len++] = 'I';
    mbody[len++] = syncType;
    len += putU32( &mbody[len], sessionId );

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen = sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent session info: type %d sessionId %d to video server", syncType, sessionId);
    }
}

void WstClientSocket::sendFrameAdvanceVideoClientConnection()
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[4];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len= 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 1;
    mbody[len++] = 'A';

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen= sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent frame adavnce to video server");
    }
}

void WstClientSocket::sendRectVideoClientConnection(int videoX, int videoY, int videoWidth, int videoHeight )
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[20];
    int len;
    int sentLen;
    int vx, vy, vw, vh;

    vx = videoX;
    vy = videoY;
    vw = videoWidth;
    vh = videoHeight;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len = 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 17;
    mbody[len++] = 'W';
    len += putU32( &mbody[len], vx );
    len += putU32( &mbody[len], vy );
    len += putU32( &mbody[len], vw );
    len += putU32( &mbody[len], vh );

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen= sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent position to video server,vx:%d,vy:%d,vw:%d,vh:%d",videoX,videoY,videoWidth,videoHeight);
    }
}

void WstClientSocket::sendRateVideoClientConnection(int fpsNum, int fpsDenom )
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[12];
    int len;
    int sentLen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    len = 0;
    mbody[len++] = 'V';
    mbody[len++] = 'S';
    mbody[len++] = 9;
    mbody[len++] = 'R';
    len += putU32( &mbody[len], fpsNum );
    len += putU32( &mbody[len], fpsDenom );

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = len;

    do
    {
        sentLen= sendmsg( mSocketFd, &msg, MSG_NOSIGNAL );
    }
    while ( (sentLen < 0) && (errno == EINTR));

    if ( sentLen == len )
    {
        INFO("sent frame rate to video server: %d/%d", fpsNum, fpsDenom);
    }
}

bool WstClientSocket::sendFrameVideoClientConnection(WstBufferInfo *wstBufferInfo, WstRect *wstRect)
{
    bool result= false;
    int sentLen;

    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec iov[1];
    unsigned char mbody[4+64];
    char cmbody[CMSG_SPACE(3*sizeof(int))];
    int i, len;
    int *fd;
    int numFdToSend;
    int frameFd0 = -1, frameFd1 = -1, frameFd2 = -1;
    int fdToSend0 = -1, fdToSend1 = -1, fdToSend2 = -1;
    int offset0, offset1, offset2;
    int stride0, stride1, stride2;
    uint32_t pixelFormat;
    int bufferId = -1;
    int vx, vy, vw, vh;

    if ( wstBufferInfo )
    {
        bufferId = wstBufferInfo->bufferId;

        numFdToSend= 1;
        offset0 = offset1 = offset2= 0;
        stride0 = stride1 = stride2= wstBufferInfo->frameWidth;
        if ( wstBufferInfo->planeCount > 1 )
        {
            frameFd0 = wstBufferInfo->planeInfo[0].fd;
            stride0 = wstBufferInfo->planeInfo[0].stride;

            frameFd1 = wstBufferInfo->planeInfo[1].fd;
            stride1 = wstBufferInfo->planeInfo[1].stride;
            if ( frameFd1 < 0 )
            {
                offset1= wstBufferInfo->frameWidth*wstBufferInfo->frameHeight;
                stride1= stride0;
            }

            frameFd2= wstBufferInfo->planeInfo[2].fd;
            stride2= wstBufferInfo->planeInfo[2].stride;
            if ( frameFd2 < 0 )
            {
                offset2 = offset1+(wstBufferInfo->frameWidth*wstBufferInfo->frameHeight)/2;
                stride2 = stride0;
            }
        }
        else
        {
            frameFd0 = wstBufferInfo->planeInfo[0].fd;
            stride0 = wstBufferInfo->planeInfo[0].offset;
            offset1 = stride0*wstBufferInfo->frameHeight;
            stride1 = stride0;
            offset2 = 0;
            stride2 = 0;
        }

        pixelFormat = wstBufferInfo->pixelFormat;
#if 0
        switch( pixelFormat  )
        {
            case V4L2_PIX_FMT_NV12:
            case V4L2_PIX_FMT_NV12M:
                pixelFormat= V4L2_PIX_FMT_NV12;
            break;
            default:
                WARNING("unsupported pixel format: %X", wstBufferInfo->pixelFormat);
            break;
        }
#endif
        fdToSend0 = fcntl( frameFd0, F_DUPFD_CLOEXEC, 0 );
        if ( fdToSend0 < 0 )
        {
            ERROR("wstSendFrameVideoClientConnection: failed to dup fd0");
            goto exit;
        }
        if ( frameFd1 >= 0 )
        {
            fdToSend1 = fcntl( frameFd1, F_DUPFD_CLOEXEC, 0 );
            if ( fdToSend1 < 0 )
            {
                ERROR("wstSendFrameVideoClientConnection: failed to dup fd1");
                goto exit;
            }
            ++numFdToSend;
        }
        if ( frameFd2 >= 0 )
        {
            fdToSend2 = fcntl( frameFd2, F_DUPFD_CLOEXEC, 0 );
            if ( fdToSend2 < 0 )
            {
                ERROR("wstSendFrameVideoClientConnection: failed to dup fd2");
                goto exit;
            }
            ++numFdToSend;
        }

        vx = wstRect->x;
        vy = wstRect->y;
        vw = wstRect->w;
        vh = wstRect->h;

        i= 0;
        mbody[i++] = 'V';
        mbody[i++] = 'S';
        mbody[i++] = 65;
        mbody[i++] = 'F';
        i += putU32( &mbody[i], wstBufferInfo->frameWidth );
        i += putU32( &mbody[i], wstBufferInfo->frameHeight );
        i += putU32( &mbody[i], pixelFormat );
        i += putU32( &mbody[i], vx );
        i += putU32( &mbody[i], vy );
        i += putU32( &mbody[i], vw );
        i += putU32( &mbody[i], vh );
        i += putU32( &mbody[i], offset0 );
        i += putU32( &mbody[i], stride0 );
        i += putU32( &mbody[i], offset1 );
        i += putU32( &mbody[i], stride1 );
        i += putU32( &mbody[i], offset2 );
        i += putU32( &mbody[i], stride2 );
        i += putU32( &mbody[i], bufferId );
        i += putS64( &mbody[i], wstBufferInfo->frameTime );

        iov[0].iov_base = (char*)mbody;
        iov[0].iov_len = i;

        cmsg = (struct cmsghdr*)cmbody;
        cmsg->cmsg_len = CMSG_LEN(numFdToSend*sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;
        msg.msg_flags = 0;

        fd = (int*)CMSG_DATA(cmsg);
        fd[0]= fdToSend0;
        if ( fdToSend1 >= 0 )
        {
            fd[1] = fdToSend1;
        }
        if ( fdToSend2 >= 0 )
        {
            fd[2] = fdToSend2;
        }

        DEBUG("send frame: %d, fd (%d, %d, %d [%d, %d, %d])", bufferId, frameFd0, frameFd1, frameFd2, fdToSend0, fdToSend1, fdToSend2);

        do
        {
            sentLen= sendmsg( mSocketFd, &msg, 0 );
        } while ( (sentLen < 0) && (errno == EINTR));

        if ( sentLen == iov[0].iov_len )
        {
            result = true;
        }
        else
        {
            DEBUG("out: failed send frame %lld buffer %d ", wstBufferInfo->frameTime, bufferId);
        }
    }

exit:
    if ( fdToSend0 >= 0 )
    {
        close( fdToSend0 );
    }
    if ( fdToSend1 >= 0 )
    {
        close( fdToSend1 );
    }
    if ( fdToSend2 >= 0 )
    {
        close( fdToSend2 );
    }

   return result;
}

void WstClientSocket::processMessagesVideoClientConnection()
{
    struct msghdr msg;
    struct iovec iov[1];
    unsigned char mbody[256];
    unsigned char *m = mbody;
    int len;

    iov[0].iov_base = (char*)mbody;
    iov[0].iov_len = sizeof(mbody);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    do
    {
        len= recvmsg( mSocketFd, &msg, 0 );
    }
    while ( (len < 0) && (errno == EINTR));

    while ( len >= 4 )
    {
        if ( (m[0] == 'V') && (m[1] == 'S') )
        {
            int mlen, id;
            mlen= m[2];
            if ( len >= (mlen+3) )
            {
                id= m[3];
                switch( id )
                {
                    case 'R':
                    if ( mlen >= 5)
                    {
                        int rate = getU32( &m[4] );
                        DEBUG("out: got rate %d from video server", rate);
                        mServerRefreshRate = rate;
                        if ( mOnEventCallback )
                        {
                            WstEVent wstEvent;
                            wstEvent.event = WST_REFRESH_RATE;
                            wstEvent.param = rate;
                            mOnEventCallback(mUserData, &wstEvent);
                        }
                    }
                    break;
                    case 'B':
                    if ( mlen >= 5)
                    {
                        int bid= getU32( &m[4] );
                        DEBUG("out: release received for buffer %d", bid);
                        if ( mOnEventCallback )
                        {
                            WstEVent wstEvent;
                            wstEvent.event = WST_BUFFER_RELEASE;
                            wstEvent.param = bid;
                            mOnEventCallback(mUserData, &wstEvent);
                        }
                    }
                    break;
                    case 'S':
                    if ( mlen >= 13)
                    {
                        /* set position from frame currently presented by the video server */
                        uint64_t frameTime = getS64( &m[4] );
                        uint32_t numDropped = getU32( &m[12] );
                        DEBUG( "out: status received: frameTime %lld numDropped %d", frameTime, numDropped);
                        if ( mOnEventCallback )
                        {
                            WstEVent wstEvent;
                            wstEvent.event = WST_STATUS;
                            wstEvent.param = numDropped;
                            wstEvent.lparam2 = frameTime;
                            mOnEventCallback(mUserData, &wstEvent);
                        }
                    }
                    break;
                    case 'U':
                    if ( mlen >= 9 )
                    {
                        uint64_t frameTime = getS64( &m[4] );
                        INFO( "out: underflow received: frameTime %lld", frameTime);
                        if ( mOnEventCallback )
                        {
                            WstEVent wstEvent;
                            wstEvent.event = WST_UNDERFLOW;
                            wstEvent.lparam2 = frameTime;
                            mOnEventCallback(mUserData, &wstEvent);
                        }
                    }
                    break;
                    case 'Z':
                    if ( mlen >= 5)
                    {
                        int zoomMode = getU32( &m[4] );
                        DEBUG("out: got zoom-mode %d from video server", zoomMode);
                        if ( mOnEventCallback )
                        {
                            WstEVent wstEvent;
                            wstEvent.event = WST_ZOOM_MODE;
                            wstEvent.param = zoomMode;
                            mOnEventCallback(mUserData, &wstEvent);
                        }
                    }
                    break;
                    case 'D':
                    if ( mlen >= 5)
                    {
                        int debugLevel = getU32( &m[4] );
                        DEBUG("out: got video-debug-level %d from video server", debugLevel);
                        if ( (debugLevel >= 0) && (debugLevel <= 7) )
                        {
                            if ( mOnEventCallback )
                            {
                                WstEVent wstEvent;
                                wstEvent.event = WST_DEBUG_LEVEL;
                                wstEvent.param = debugLevel;
                                mOnEventCallback(mUserData, &wstEvent);
                            }
                        }
                    }
                    break;
                    default:
                    break;
                }
                m += (mlen+3);
                len -= (mlen+3);
            }
            else
            {
                len= 0;
            }
        }
        else
        {
            len= 0;
        }
    }
}

void WstClientSocket::readyToRun()
{
    if (mPoll) {
        mPoll->addFd(mSocketFd);
        mPoll->setFdReadable(mSocketFd, true);
    }
}

bool WstClientSocket::threadLoop()
{
    int ret;
    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }
    processMessagesVideoClientConnection();
    return true;
}