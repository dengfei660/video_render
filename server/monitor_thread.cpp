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
    DEBUG("in");
    mUeventFd = -1;
    mLockFd = -1;
    mSocketServerFd = -1;
    mPoll = new Poll(true);
    DEBUG("out");
}

MonitorThread::~MonitorThread()
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
    if (mUeventFd >= 0) {
        close(mUeventFd);
        mUeventFd = -1;
    }
    if (mSocketServerFd >= 0) {
        close(mSocketServerFd);
        mSocketServerFd = -1;
    }
    DEBUG("out");
}

bool MonitorThread::openUeventMonitor()
{
    DEBUG("in");
    bool result = false;

    mUeventFd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    INFO("uevent process thread: ueventFd %d", mUeventFd);
    if (mUeventFd < 0) {
        ERROR("open uevent fd fail");
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
        ERROR("bind failed for ueventFd: rc %d", rc);
        goto exit;
    }

    result = true;
    DEBUG("out");
exit:
    if (!result) {
        ERROR("");
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

    DEBUG("in");
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

    INFO("socketServerFd %d", mSocketServerFd);

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
    DEBUG("out");
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
        //INFO("msg:%s",msg);
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
        //INFO("cmd:%u,param0:%u,param1:%u,param2:%u",cmd,param0,param1,param2);
        //INFO("expect V4L2_CID_EXT_VDO_VDEC_CONNECTING=%u,V4L2_CID_EXT_VDO_VDEC_DISCONNECTING=%u",V4L2_CID_EXT_VDO_VDEC_CONNECTING,V4L2_CID_EXT_VDO_VDEC_DISCONNECTING);
        //create vdo data server thread
        if ( param0 == AM_V4L2_CID_EXT_VDO_VDEC_CONNECTING )
        {
            Tls::Mutex::Autolock _l(mMutex);
            INFO("VDO connecting event detected" );
            mSinkMgr->createVdoSink(0/*param1*/, 0/*param2*/);
        } else if (param0 == AM_V4L2_CID_EXT_VDO_VDEC_DISCONNECTING) { //destroy vdo data server thread
            INFO("VDO disconnecting event detected" );
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
        WARNING("Not open socket server fd");
        return false;
    }

    DEBUG("waiting for connections...");
    fd = accept4(mSocketServerFd, (struct sockaddr *)&addr, &addrLen, SOCK_CLOEXEC );
    if ( fd >= 0 ) {
        int vdecPort = parseVdecPort(fd);
        if (vdecPort < 0) {
               ERROR("Please send vdePort first,when client connect server");
            close(fd);
            return true;
        }
        bool ret = mSinkMgr->createSocketSink(fd, vdecPort);
        if (!ret) {
            ERROR("create socket sink fail");
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
            INFO("vdecPort:%d", vdecPort);
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
        ERROR("open uevent monitor fail");
    }
    ret = openSocketMonitor();
    if (!ret) {
        ERROR("open socket monitor fail");
    }
    return ret;
}

void MonitorThread::readyToRun()
{
    DEBUG("in");
    if (mUeventFd >= 0) {
        mPoll->addFd(mUeventFd);
        mPoll->setFdReadable(mUeventFd, true);
    }
    if (mSocketServerFd >= 0) {
        mPoll->addFd(mSocketServerFd);
        mPoll->setFdReadable(mSocketServerFd, true);
    }
    DEBUG("out");
}

bool MonitorThread::threadLoop()
{
    int ret;

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
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