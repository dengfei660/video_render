/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
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
#include "render_server.h"
#include "monitor_thread.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"

#include "v4l2_am_comtext.h"

using namespace Tls;

#define TAG "rlib:monitor_thread"


static int IOCTL( int fd, int request, void* arg );

MonitorThread::MonitorThread(SinkManager *sinkMgr)
    :mSinkMgr(sinkMgr)
{
    DEBUG(NO_CATEGERY,"in");
    mUeventFd = -1;
    mLockFd = -1;
    mSocketServerFd = -1;
    mPoll = new Poll(true);
    DEBUG(NO_CATEGERY,"out");
}

MonitorThread::~MonitorThread()
{
    DEBUG(NO_CATEGERY,"in");
    if (mPoll) {
        if (isRunning()) {
            DEBUG(NO_CATEGERY,"try stop thread");
            mPoll->setFlushing(true);
            requestExitAndWait();
        }
        delete mPoll;
        mPoll = NULL;
    }
    if (mUeventFd >= 0) {
        close(mUeventFd);
        mUeventFd = -1;
    }
    if (mSocketServerFd >= 0) {
        close(mSocketServerFd);
        mSocketServerFd = -1;
    }
    DEBUG(NO_CATEGERY,"out");
}

bool MonitorThread::openUeventMonitor()
{
    DEBUG(NO_CATEGERY,"in");
    bool result = false;

    mUeventFd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    INFO(NO_CATEGERY,"uevent process thread: ueventFd %d", mUeventFd);
    if (mUeventFd < 0) {
        ERROR(NO_CATEGERY,"open uevent fd fail");
        goto exit;
    }

    int rc;
    struct sockaddr_nl nlAddr;
    memset(&nlAddr, 0, sizeof(nlAddr));
    nlAddr.nl_family= AF_NETLINK;
    nlAddr.nl_pid= 0;
    nlAddr.nl_groups= 0xFFFFFFFF;
    rc= bind( mUeventFd, (struct sockaddr *)&nlAddr, sizeof(nlAddr));
    if (rc) { //bind fail
        ERROR(NO_CATEGERY,"bind failed for ueventFd: rc %d", rc);
        goto exit;
    }

    result = true;
    DEBUG(NO_CATEGERY,"out");
exit:
    if (!result) {
        ERROR(NO_CATEGERY,"");
        if (mUeventFd >= 0) {
            close(mUeventFd);
            mUeventFd = -1;
        }
    }
    return result;
}

bool MonitorThread::openSocketMonitor()
{
    bool result= false;
    const char *workingDir;
    int rc, pathNameLen, addressSize;

    DEBUG(NO_CATEGERY,"in");
    workingDir = getenv("XDG_RUNTIME_DIR");
    if ( !workingDir )
    {
        ERROR(NO_CATEGERY,"XDG_RUNTIME_DIR is not set");
        goto exit;
    }

    pathNameLen = strlen(workingDir)+strlen("/")+strlen(SOCKET_NAME)+1;
    if ( pathNameLen > (int)sizeof(mAddr.sun_path) )
    {
        ERROR(NO_CATEGERY,"name for server unix domain socket is too long: %d versus max %d",
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
        ERROR(NO_CATEGERY,"failed to create lock file (%s) errno %d", mLockName, errno );
        goto exit;
    }

    rc = flock(mLockFd, LOCK_NB|LOCK_EX );
    if ( rc < 0 )
    {
        ERROR(NO_CATEGERY,"failed to lock.  Is another server running with name %s ?", SOCKET_NAME );
        goto exit;
    }

    (void)unlink(mAddr.sun_path);

    mSocketServerFd = socket( PF_LOCAL, SOCK_STREAM|SOCK_CLOEXEC, 0 );
    if ( mSocketServerFd < 0 )
    {
        ERROR(NO_CATEGERY,"wstInitServiceServer: unable to open socket: errno %d", errno );
        goto exit;
    }

    INFO(NO_CATEGERY,"socketServerFd %d", mSocketServerFd);

    addressSize = pathNameLen + offsetof(struct sockaddr_un, sun_path);

    rc= bind(mSocketServerFd, (struct sockaddr *)&mAddr, addressSize );
    if ( rc < 0 )
    {
        ERROR(NO_CATEGERY,"wstInitServiceServer: Error: bind failed for socket: errno %d", errno );
        goto exit;
    }

    rc= listen(mSocketServerFd, 1);
    if ( rc < 0 )
    {
        ERROR(NO_CATEGERY,"wstInitServiceServer: Error: listen failed for socket: errno %d", errno );
        goto exit;
    }

    result= true;
exit:

    if ( !result )
    {
        mAddr.sun_path[0]= '\0';
        mLockName[0]= '\0';
    }
    DEBUG(NO_CATEGERY,"out");
   return result;
}

bool MonitorThread::ueventEventProcess()
{
    int ret;
    int rc, i;
    char buff[1024];

    memset(buff, 0, 1024);
    rc = read(mUeventFd, buff, sizeof(buff) );
    buff[++rc] = '\0';
    if ( rc > 0 ) {
        const char *msg = buff;
        uint32_t cmd = 0;
        uint32_t param0 = 0;
        uint32_t param1 = 0;
        uint32_t param2 = 0;
        bool connecting = false;
        bool disconnecting = false;
        //INFO(NO_CATEGERY,"msg:%s",msg);
        while (*msg || *(msg + 1))
        {
            if (!strncmp(msg, "V4L2_CMD_TYPE=", strlen("V4L2_CMD_TYPE=")))
            {
                msg += strlen("V4L2_CMD_TYPE=");
                cmd = (uint32_t)strtoul(msg, 0, 0);
                ret = true;
            }
            else if (!strncmp(msg, "V4L2_CMD_PARM_A=", strlen("V4L2_CMD_PARM_A=")))
            {
                msg += strlen("V4L2_CMD_PARM_A=");
                param0 = (uint32_t)strtoul(msg, 0, 0);
            }
            else if (!strncmp(msg, "V4L2_CMD_PARM_B=", strlen("V4L2_CMD_PARM_B=")))
            {
                msg += strlen("V4L2_CMD_PARM_B=");
                param1 = (uint32_t)strtoul(msg, 0, 0);
            }
            else if (!strncmp(msg, "V4L2_CMD_PARM_C=", strlen("V4L2_CMD_PARM_C=")))
            {
                msg += strlen("V4L2_CMD_PARM_C=");
                param2 = (uint32_t)strtoul(msg, 0, 0);
            }
            while (*msg++)
                ;
        }
        //INFO(NO_CATEGERY,"cmd:%u,param0:%u,param1:%u,param2:%u",cmd,param0,param1,param2);
        //INFO(NO_CATEGERY,"expect V4L2_CID_EXT_VDO_VDEC_CONNECTING=%u,V4L2_CID_EXT_VDO_VDEC_DISCONNECTING=%u",V4L2_CID_EXT_VDO_VDEC_CONNECTING,V4L2_CID_EXT_VDO_VDEC_DISCONNECTING);
        //create vdo data server thread
        if ( param0 == AM_V4L2_CID_EXT_VDO_VDEC_CONNECTING )
        {
            Tls::Mutex::Autolock _l(mMutex);
            INFO(NO_CATEGERY,"VDO connecting event detected" );
            mSinkMgr->createVdoSink(0/*param1*/, 0/*param2*/);
        } else if (param0 == AM_V4L2_CID_EXT_VDO_VDEC_DISCONNECTING) { //destroy vdo data server thread
            INFO(NO_CATEGERY,"VDO disconnecting event detected" );
            mSinkMgr->destroySink(param1, param2);
        }
    }
    return true;
}

bool MonitorThread::socketEventProcess()
{
    int ret;
    int fd;
    struct sockaddr_un addr;
    socklen_t addrLen= sizeof(addr);

    if (mSocketServerFd < 0) {
        WARNING(NO_CATEGERY,"Not open socket server fd");
        return false;
    }

    DEBUG(NO_CATEGERY,"waiting for connections...");
    fd = accept4(mSocketServerFd, (struct sockaddr *)&addr, &addrLen, SOCK_CLOEXEC );
    if ( fd >= 0 ) {
        int vdecPort = parseVdecPort(fd);
        if (vdecPort < 0) {
               ERROR(NO_CATEGERY,"Please send vdePort first,when client connect server");
            close(fd);
            return true;
        }
        bool ret = mSinkMgr->createSocketSink(fd, vdecPort);
        if (!ret) {
            ERROR(NO_CATEGERY,"create socket sink fail");
            close(fd);
        }
    }
    return true;
}

/*read first msg and parse vdecport*/
int MonitorThread::parseVdecPort(int clientfd)
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
        len= recvmsg(clientfd, &msg, 0 );
    }
    while ( (len < 0) && (errno == EINTR));

    while ( len >= 4 )
    {
        if ( (m[0] == 'V') && (m[1] == 'S') && (m[2] == 2) && (m[3] == 'C') )
        {
            int vdecPort = m[4];
            INFO(NO_CATEGERY,"vdecPort:%d", vdecPort);
            return vdecPort;
        }
        else
        {
            len= 0;
        }
    }
    return -1;
}

bool MonitorThread::init()
{
    bool ret = true;

    ret = openUeventMonitor();
    if (!ret) {
        ERROR(NO_CATEGERY,"open uevent monitor fail");
    }
    ret = openSocketMonitor();
    if (!ret) {
        ERROR(NO_CATEGERY,"open socket monitor fail");
    }
    return ret;
}

void MonitorThread::readyToRun()
{
    DEBUG(NO_CATEGERY,"in");
    if (mUeventFd >= 0) {
        mPoll->addFd(mUeventFd);
        mPoll->setFdReadable(mUeventFd, true);
    }
    if (mSocketServerFd >= 0) {
        mPoll->addFd(mSocketServerFd);
        mPoll->setFdReadable(mSocketServerFd, true);
    }
    DEBUG(NO_CATEGERY,"out");
}

bool MonitorThread::threadLoop()
{
    int ret;

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING(NO_CATEGERY,"poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    if (mPoll->isReadable(mUeventFd)) {
        ueventEventProcess();
    }
    if (mPoll->isReadable(mSocketServerFd)) {
        socketEventProcess();
    }

    return true;
}