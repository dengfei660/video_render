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
#include <linux/videodev2.h>
#include "render_server.h"
#include "socket_server.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"

using namespace Tls;

#define TAG "rlib:socket_server"
#define SOCKET_NAME "render"

SocketConnectThread::SocketConnectThread(RenderServer *renderServer)
    : mRenderServer(renderServer)
{
    DEBUG("in");
    mLockFd = -1;
    mSocketServerFd = -1;
    mPoll = new Poll(true);
    DEBUG("out");
}

SocketConnectThread::~SocketConnectThread()
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

    if (mSocketServerFd >= 0) {
        close(mSocketServerFd);
        mSocketServerFd = -1;
    }
    DEBUG("out");
}

bool SocketConnectThread::init()
{
    bool result= false;
    const char *workingDir;
    int rc, pathNameLen, addressSize;

    workingDir = getenv("XDG_RUNTIME_DIR");
    if ( !workingDir )
    {
        ERROR("XDG_RUNTIME_DIR is not set");
        goto exit;
    }

    pathNameLen = strlen(workingDir)+strlen("/")+strlen(SOCKET_NAME)+1;
    if ( pathNameLen > (int)sizeof(mAddr.sun_path) )
    {
        ERROR("name for server unix domain socket is too long: %d versus max %d",
             pathNameLen, (int)sizeof(mAddr.sun_path) );
        goto exit;
    }

    mAddr.sun_family= AF_LOCAL;
    strcpy(mAddr.sun_path, workingDir );
    strcat(mAddr.sun_path, "/" );
    strcat(mAddr.sun_path, SOCKET_NAME);

    strcpy(mLockName, mAddr.sun_path );
    strcat(mLockName, ".lock" );

    mLockFd = open(mLockName, O_CREAT|O_CLOEXEC,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    if ( mLockFd < 0 )
    {
        ERROR("failed to create lock file (%s) errno %d", mLockName, errno );
        goto exit;
    }

    rc = flock(mLockFd, LOCK_NB|LOCK_EX );
    if ( rc < 0 )
    {
        ERROR("failed to lock.  Is another server running with name %s ?", SOCKET_NAME );
        goto exit;
    }

    (void)unlink(mAddr.sun_path);

    mSocketServerFd = socket( PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0 );
    if ( mSocketServerFd < 0 )
    {
        ERROR("wstInitServiceServer: unable to open socket: errno %d", errno );
        goto exit;
    }

    addressSize = pathNameLen + offsetof(struct sockaddr_un, sun_path);

    rc= bind(mSocketServerFd, (struct sockaddr *)&mAddr, addressSize );
    if ( rc < 0 )
    {
        ERROR("wstInitServiceServer: Error: bind failed for socket: errno %d", errno );
        goto exit;
    }

    rc= listen(mSocketServerFd, 1);
    if ( rc < 0 )
    {
        ERROR("wstInitServiceServer: Error: listen failed for socket: errno %d", errno );
        goto exit;
    }

    result= true;
exit:

    if ( !result )
    {
        mAddr.sun_path[0]= '\0';
        mLockName[0]= '\0';
    }

   return result;
}

void SocketConnectThread::readyToRun()
{
    DEBUG("in");
    if (mSocketServerFd >= 0) {
        mPoll->addFd(mSocketServerFd);
        mPoll->setFdReadable(mSocketServerFd, true);
        mPoll->setFdWritable(mSocketServerFd, true);
    }
    DEBUG("out");
}

bool SocketConnectThread::threadLoop()
{
    int ret;

    if (mSocketServerFd < 0) {
        WARNING("Not open socket server fd");
        return false;
    }

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    if (mPoll->isReadable(mSocketServerFd)) {
        int fd;
        struct sockaddr_un addr;
        socklen_t addrLen= sizeof(addr);

        DEBUG("waiting for connections...");
        fd = accept4(mSocketServerFd, (struct sockaddr *)&addr, &addrLen, SOCK_CLOEXEC );
        if ( fd >= 0 ) {
            int ret = mRenderServer->createSocketServerThread(fd);
            if (ret < 0) {
                ERROR("create socket server thread fail");
                close(fd);
            }
        }
    }
    return true;
}

/***************SocketServerThread***************/
SocketServerThread::SocketServerThread(RenderServer *renderServer, int socketfd)
    : mSocketFd(socketfd),
    mRenderServer(renderServer)
{
    DEBUG("in");
    mPoll = new Poll(true);
    DEBUG("out");
}

SocketServerThread::~SocketServerThread()
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

    if (mSocketFd >= 0) {
        close(mSocketFd);
        mSocketFd = -1;
    }
    DEBUG("out");
}
bool SocketServerThread::init()
{
    return true;
}

int SocketServerThread::adaptFd( int fdin )
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

void SocketServerThread::processEvent()
{
#if 0
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
    int fd0, fd1, fd2;
    int offset0, offset1, offset2;
    int stride0, stride1, stride2;
    int bufferId= 0;
    int64_t frameTime= 0;


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
        return ;
    }

    if (len == 4) {
        unsigned char *m= mbody;
        if ( (m[0] == 'V') && (m[1] == 'S') )
        {
            int mlen, id;
            mlen= m[2];
            id= m[3];
            switch( id )
            {
                case 'F':
                    cmsg= CMSG_FIRSTHDR(&msg);
                    if ( cmsg &&
                        cmsg->cmsg_level == SOL_SOCKET &&
                        cmsg->cmsg_type == SCM_RIGHTS &&
                        cmsg->cmsg_len >= CMSG_LEN(sizeof(int)) )
                    {
                        fd0 = adaptFd( ((int*)CMSG_DATA(cmsg))[0] );
                        if ( cmsg->cmsg_len >= CMSG_LEN(2*sizeof(int)) )
                        {
                           fd1 = adaptFd( ((int*)CMSG_DATA(cmsg))[1] );
                        }
                        if ( cmsg->cmsg_len >= CMSG_LEN(3*sizeof(int)) )
                        {
                           fd2 = adaptFd( ((int*)CMSG_DATA(cmsg))[2] );
                        }
                    }
                    break;
                default:
                    break;
            }

            if ( mlen > sizeof(mbody) )
            {
                ERROR("bad message length: %d : truncating");
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
                if ( Logger_get_level() >= 7 )
                {
                     dumpMessage( (char *)mbody, len );
                }
                switch( id )
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
                        TRACE2("got frame %d buffer %d frameTime %lld", conn->videoPlane->frameCount, bufferId, frameTime);

                        TRACE2("got frame fd %d,%d,%d (%dx%d) %X (%d, %d, %d, %d) off(%d, %d, %d) stride(%d, %d, %d)",
                                fd0, fd1, fd2, frameWidth, frameHeight, frameFormat, rectX, rectY, rectW, rectH,
                                offset0, offset1, offset2, stride0, stride1, stride2 );


                        videoFrame.frameWidth= frameWidth;
                        videoFrame.frameHeight= frameHeight;
                        wstSetVideoFrameRect( &videoFrame, rectX, rectY, rectW, rectH, &frameSkipX, &frameSkipY );

                        rc= drmPrimeFDToHandle( gCtx->drmFd, fd0, &handle0 );
                        if ( !rc )
                        {
                            wstUpdateResources( WSTRES_HD_VIDEO, true, handle0, __LINE__);
                            handle1= handle0;
                            if ( fd1 >= 0 )
                              {
                                rc= drmPrimeFDToHandle( gCtx->drmFd, fd1, &handle1 );
                                if ( !rc )
                                {
                                    wstUpdateResources( WSTRES_HD_VIDEO, true, handle1, __LINE__);
                                }
                            }
                        }
                        if ( !rc )
                        {
                            uint32_t handles[4]= { handle0,
                                                    handle1,
                                                    0,
                                                    0 };
                            uint32_t pitches[4]= { stride0,
                                                    stride1,
                                                    0,
                                                    0 };
                              uint32_t offsets[4]= {offset0+frameSkipX+frameSkipY*stride0,
                                                    offset1+frameSkipX+frameSkipY*(stride1/2),
                                                    0,
                                                    0};

                            rc= drmModeAddFB2( gCtx->drmFd,
                                            frameWidth-frameSkipX,
                                            frameHeight-frameSkipY,
                                            frameFormat,
                                            handles,
                                            pitches,
                                            offsets,
                                            &fbId,
                                            0 // flags);
                            if ( !rc )
                            {
                                wstUpdateResources( WSTRES_FB_VIDEO, true, fbId, __LINE__);
                                videoFrame.hide= false;
                                videoFrame.fbId= fbId;
                                videoFrame.handle0= handle0;
                                videoFrame.handle1= handle1;
                                videoFrame.fd0= fd0;
                                videoFrame.fd1= fd1;
                                videoFrame.fd2= fd2;
                                videoFrame.frameFormat= frameFormat;
                                videoFrame.bufferId= bufferId;
                                videoFrame.frameTime= frameTime;
                                videoFrame.frameNumber= conn->videoPlane->frameCount++;
                                videoFrame.vf= 0;
                                videoFrame.canExpire= true;
                                conn->videoPlane->hidden= false;
                                wstVideoFrameManagerPushFrame( conn->videoPlane->vfm, &videoFrame );
                            }
                            else
                            {
                                ERROR("wstVideoServerConnectionThread: drmModeAddFB2 failed: rc %d errno %d", rc, errno);
                                wstClosePrimeFDHandles( gCtx, handle0, handle1, __LINE__ );
                                wstUpdateResources( WSTRES_FD_VIDEO, false, fd0, __LINE__);
                                close( fd0 );
                                if ( fd1 >= 0 )
                                {
                                close( fd1 );
                                }
                                if ( fd2 >= 0 )
                                {
                                close( fd2 );
                                }
                            }
                        }
                        else
                        {
                            ERROR("wstVideoServerConnectionThread: drmPrimeFDToHandle failed: rc %d errno %d", rc, errno);
                            wstUpdateResources( WSTRES_FD_VIDEO, false, fd0, __LINE__);
                            close( fd0 );
                            if ( fd1 >= 0 )
                            {
                                close( fd1 );
                            }
                            if ( fd2 >= 0 )
                            {
                                close( fd2 );
                            }
                        }
                    }
                    break;
                    case 'V':
                    {
                        int videoResourceId= etU32( m+1 );
                        bool primary= (videoResourceId == 0);
                        DEBUG("got video resource id %d from conn %p", videoResourceId, conn );
                        if ( conn->videoPlane && (conn->videoPlane->videoResourceId != videoResourceId))
                        {
                            wstOverlayFree( &gCtx->overlayPlanes, conn->videoPlane );
                            conn->videoPlane= 0;
                        }
                        if ( !conn->videoPlane )
                        {
                            conn->videoPlane= wstOverlayAlloc( &gCtx->overlayPlanes, false, primary );
                            INFO("video plane %p : zorder: %d videoResourceId %d",
                                conn->videoPlane, (conn->videoPlane ? conn->videoPlane->zOrder: -1), videoResourceId );

                            if ( !conn->videoPlane )
                            {
                                ERROR("No video plane avaialble");
                                goto exit;
                            }

                            conn->videoResourceId= videoResourceId;
                            conn->videoPlane->videoResourceId= videoResourceId;

                            conn->videoPlane->vfm= wstCreateVideoFrameManager( conn );
                            if ( !conn->videoPlane->vfm )
                            {
                                ERROR("Unable to allocate vfm");
                                goto exit;
                            }

                            videoFrame.plane= conn->videoPlane;
                            for( i= 0; i < ACTIVE_FRAMES; ++i )
                            {
                                conn->videoPlane->videoFrame[i].plane= conn->videoPlane;
                            }

                            conn->videoPlane->conn= conn;
                        }
                    }
                    break;
                    case 'H':
                    {
                        bool hide= (m[1] == 1);
                        DEBUG("got hide (%d) video plane %d", hide, conn->videoPlane->plane->plane_id);
                        pthread_mutex_lock( &gMutex );
                        gCtx->dirty= true;
                        conn->videoPlane->dirty= true;
                        conn->videoPlane->hide= hide;
                        pthread_mutex_unlock( &gMutex );
                    }
                    break;
                    case 'S':
                    {
                        DEBUG("got flush video plane %d", conn->videoPlane->plane->plane_id);
                        FRAME("got flush video plane %d", conn->videoPlane->plane->plane_id);
                        pthread_mutex_lock( &gMutex );
                        conn->videoPlane->flipTimeBase= 0LL;
                        conn->videoPlane->frameTimeBase= 0LL;
                        wstVideoServerFlush( conn );
                        pthread_mutex_unlock( &gMutex );
                    }
                    break;
                    case 'P':
                    {
                        bool pause= (m[1] == 1);
                        DEBUG("got pause (%d) video plane %d", pause, conn->videoPlane->plane->plane_id);
                        pthread_mutex_lock( &gMutex );
                        wstVideoFrameManagerPause( conn->videoPlane->vfm, pause );
                        pthread_mutex_unlock( &gMutex );
                    }
                    break;
                    case 'I':
                    {
                        int syncType= m[1];
                        int sessionId= wstGetU32( m+2 );
                        DEBUG("got session info: sync type %d sessionId %d video plane %d", syncType, sessionId, conn->videoPlane->plane->plane_id);
                        pthread_mutex_lock( &gMutex );
                        if ( conn->videoPlane->vfm )
                        {
                            if (
                                (conn->sessionId != sessionId) ||
                                (syncType == SYNC_IMMEDIATE) ||
                                (
                                    (conn->syncType != syncType) &&
                                    (
                                    (conn->syncType > 1) || /* current not video, not audio */
                                    (syncType > 1)  /* new not video, not audio */
                                    )
                                )
                                )
                            {
                                wstDestroyVideoFrameManager( conn->videoPlane->vfm );
                                conn->videoPlane->vfm= 0;
                            }
                            else if ( conn->syncType != syncType )
                            {
                                conn->syncType= syncType;
                                wstVideoFrameManagerSetSyncType( conn->videoPlane->vfm, syncType );
                            }
                        }
                        if ( !conn->videoPlane->vfm )
                        {
                            conn->syncType= syncType;
                            conn->sessionId= sessionId;
                            conn->videoPlane->vfm= wstCreateVideoFrameManager( conn );
                        }
                        pthread_mutex_unlock( &gMutex );
                        #ifdef WESTEROS_GL_AVSYNC
                        if ( conn->videoPlane->vfm )
                        {
                            VideoFrameManager *vfm= conn->videoPlane->vfm;
                            if ( !vfm->syncInit && (syncType != SYNC_IMMEDIATE) )
                            {
                                vfm->syncInit= true;
                                wstAVSyncInit( vfm, vfm->conn->sessionId );
                            }
                        }
                        #endif
                        #ifdef USE_GENERIC_AVSYNC
                        if ( g_useGenericAVSync )
                        {
                            if ( len >= 10 )
                            {
                                int avsCtrlSize= wstGetU32( m+6 );
                                if ( (avsCtrlSize > 0) && (fd0 >= 0 ) )
                                {
                                if ( wstVideoFrameManagerSetSyncCtrl( conn->videoPlane->vfm, fd0, avsCtrlSize ) )
                                {
                                    fd0= -1;
                                }
                                }
                                if ( fd0 >= 0 )
                                {
                                close( fd0 );
                                }
                            }
                        }
                        #endif
                    }
                    break;
                    case 'A':
                    {
                        DEBUG("got frame advance video plane %d", conn->videoPlane->plane->plane_id);
                        pthread_mutex_lock( &gMutex );
                        wstVideoFrameManagerFrameAdvance( conn->videoPlane->vfm );
                        pthread_mutex_unlock( &gMutex );
                    }
                    break;
                    case 'W':
                    {
                        rectX= (int)wstGetU32( m+1 );
                        rectY= (int)wstGetU32( m+5 );
                        rectW= (int)wstGetU32( m+9 );
                        rectH= (int)wstGetU32( m+13 );
                        DEBUG("got position video plane %d: (%d, %d, %d, %d)",
                                conn->videoPlane->plane->plane_id,
                                rectX, rectY, rectW, rectH);
                        wstVideoFrameManagerUpdateRect( conn->videoPlane->vfm, rectX, rectY, rectW, rectH );
                    }
                    break;
                    case 'R':
                    {
                        int num, denom;
                        pthread_mutex_lock( &gMutex );
                        num= (int)wstGetU32( m+1 );
                        denom= (int)wstGetU32( m+5 );
                        DEBUG("got frame rate video plane %d: (%d / %d)", conn->videoPlane->plane->plane_id, num, denom);
                        if ( (num == 0) && (gCtx->defaultRate > 0) )
                        {
                            num= gCtx->defaultRate;
                            denom= 1;
                            DEBUG("frame rate unknown, use default rate video plane %d: (%d / %d)", conn->videoPlane->plane->plane_id, num, denom);
                        }
                        if ( (num > 0) && (denom > 0) )
                        {
                            conn->videoPlane->frameRateNum= num;
                            conn->videoPlane->frameRateDenom= denom;
                            if ( gCtx->autoFRMModeEnabled && conn->videoPlane->frameRateMatchingPlane )
                            {
                                wstSelectRate( gCtx, num, denom );
                            }
                            if ( conn->videoPlane->vfm )
                            {
                                int rate;
                                VideoFrameManager *vfm= conn->videoPlane->vfm;
                                vfm->expireLimit= 0;
                                rate= conn->videoPlane->frameRateNum / conn->videoPlane->frameRateDenom;
                                if ( rate < 10 )
                                {
                                vfm->expireLimit= 1000000LL;
                                }
                            }
                        }
                        pthread_mutex_unlock( &gMutex );
                    }
                    break;
                    default:
                    ERROR("got unknown video server message: mlen %d", mlen);
                    wstDumpMessage( mbody, mlen+3 );
                    break;
                }
            }
        }
    }
#endif
}

void SocketServerThread::readyToRun()
{
    DEBUG("in");
    if (mSocketFd >= 0) {
        mPoll->addFd(mSocketFd);
        mPoll->setFdReadable(mSocketFd, true);
        mPoll->setFdWritable(mSocketFd, true);
    }
    DEBUG("out");
}

void SocketServerThread::readyToExit()
{
    DEBUG("in");
    if (mRenderServer) {
        mRenderServer->destroySocketServerThread(mSocketFd);
    }
    DEBUG("out");
}

bool SocketServerThread::threadLoop()
{
    int ret;

    if (mSocketFd < 0) {
        WARNING("Not open socket server fd");
        return false;
    }

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    processEvent();

    return true;
}