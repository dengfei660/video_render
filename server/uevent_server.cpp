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
#include "Logger.h"
#include "Times.h"
#include "Utils.h"

#include "videodev2-ext.h"
#include "v4l2-controls-ext.h"
#include "v4l2-ext-frontend.h"

using namespace Tls;

#define TAG "rlib:uevent_server"


static int IOCTL( int fd, int request, void* arg );

UeventProcessThread::UeventProcessThread(RenderServer *renderServer)
    :mRenderServer(renderServer)
{
    DEBUG("in");
    mUeventFd = -1;
    mPoll = new Poll(true);
    DEBUG("out");
}

UeventProcessThread::~UeventProcessThread()
{
    DEBUG("in");
    if (mPoll) {
        if (isRunning()) {
            DEBUG("try stop thread");
            mPoll->setFlushing(true);
            requestExitAndWait();
        }
    }
    if (mUeventFd >= 0) {
        close(mUeventFd);
        mUeventFd = -1;
    }
    DEBUG("out");
}


bool UeventProcessThread::init()
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

void UeventProcessThread::readyToRun()
{
    DEBUG("in");
    if (mUeventFd >= 0) {
        mPoll->addFd(mUeventFd);
        mPoll->setFdReadable(mUeventFd, true);
    }
    DEBUG("out");
}

bool UeventProcessThread::threadLoop()
{
    int ret;

    if (mUeventFd < 0) {
        WARNING("Not open uevent fd");
        return false;
    }

    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING("poll error");
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    //read uevent data and process
    if (mPoll->isReadable(mUeventFd)) {
        int rc, i;
        char buff[1024];

        memset(buff, 0, 1024);
        rc= read(mUeventFd, buff, sizeof(buff) );
        if ( rc > 0 ) {
            uint32_t cmd = 0;
            uint32_t param0 = 0;
            uint32_t param1 = 0;
            uint32_t param2 = 0;
            bool connecting = false;
            bool disconnecting = false;
            for( i= 0; i < rc; )
            {
                char *uevent= &buff[i];
                char *ptr;
                INFO("uevent:%s",buff);
                if ( ptr = strstr( uevent, "V4L2_CMD_TYPE=" ) ) {
                    cmd = (uint32_t)strtoul(ptr + strlen("V4L2_CMD_TYPE="), 0, 0);
                } else if ( strstr( uevent, "V4L2_CMD_PARM_A=" ) ) {
                    param0 = (uint32_t)strtoul(ptr + strlen("V4L2_CMD_PARM_A="), 0, 0);
                } else if ( strstr( uevent, "V4L2_CMD_PARM_B=" ) ) {
                    param1 = (uint32_t)strtoul(ptr + strlen("V4L2_CMD_PARM_B="), 0, 0);
                } else if ( strstr( uevent, "V4L2_CMD_PARM_C=" ) ) {
                    param2 = (uint32_t)strtoul(ptr + strlen("V4L2_CMD_PARM_C="), 0, 0);
                }
                i += (strlen(uevent) + 1);
            }
            //create vdo data server thread
            if ( cmd == V4L2_CID_EXT_VDO_VDEC_CONNECTING )
            {
                INFO("VDO connecting event detected" );
                mRenderServer->createVDOServerThread(param0, param1, param2);
            } else if (cmd == V4L2_CID_EXT_VDO_VDEC_DISCONNECTING) { //destroy vdo data server thread
                INFO("VDO disconnecting event detected" );
                mRenderServer->destroyVDOServerThread(param0, param1, param2);
            }
        }
    }

    return true;
}