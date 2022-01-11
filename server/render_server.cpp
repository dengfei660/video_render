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

using namespace Tls;

#define TAG "rlib:render_server"

RenderServer::RenderServer()
{
    DEBUG("in");
    mUeventProcessThread = NULL;
    mSocketConnectThread = NULL;
    for (int i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        mVDOServerThread[i] == NULL;
        mSocketServerThread[i] = NULL;
    }

    mRenderInstanceCnt = 0;
    DEBUG("out");
}

RenderServer::~RenderServer()
{
    DEBUG("in");
    for (int i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        if (mVDOServerThread[i] != NULL) {
            delete mVDOServerThread[i];
            mVDOServerThread[i] = NULL;
        }
        if (mSocketServerThread[i] != NULL) {
            delete mSocketServerThread[i];
            mSocketServerThread[i] = NULL;
        }
    }

    if (mUeventProcessThread) {
        delete mUeventProcessThread;
        mUeventProcessThread = NULL;
    }

    if (mSocketConnectThread) {
        delete mSocketConnectThread;
        mSocketConnectThread = NULL;
    }
    DEBUG("out");
}

bool RenderServer::createUeventProcessThread()
{
    bool ret;
    mUeventProcessThread = new UeventProcessThread(this);
    ret = mUeventProcessThread->init();
    mUeventProcessThread->run("ueventProcessThread");
    return ret;
}
bool RenderServer::createSocketConnectThread()
{
    bool ret;
    mSocketConnectThread = new SocketConnectThread(this);
    ret = mSocketConnectThread->init();
    return ret;
}

bool RenderServer::createVDOServerThread(int port)
{
    int i = 0;
    bool ret;

    for (i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        if (mVDOServerThread[i] == NULL) {
            break;
        }
    }

    if (i >= MAX_VIDEO_RENDER_INSTANCE) {
        ERROR("too many vdo data server thread");
        return -1;
    }

    mVDOServerThread[i] = new VDOServerThread(this, port);
    ret = mVDOServerThread[i]->init();
    if (!ret) {
        ERROR("vdo server thread init fail");
        goto tag_exit;
    }

    mVDOServerThread[i]->run("vdoserver");

tag_exit:
    return ret;
}

bool RenderServer::destroyVDOServerThread(int port)
{
    int foundIdx = 0;
    for (int i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        if (mVDOServerThread[i]->getPort() == port) {
            foundIdx = i;
        }
    }

    //if found
    if (foundIdx > 0) {
        delete mVDOServerThread[foundIdx];
        mVDOServerThread[foundIdx] = NULL;
    }
    return true;
}

bool RenderServer::createSocketServerThread(int socketfd)
{
    int i = 0;
    bool ret;
    for (i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        if (mSocketServerThread[i] == NULL) {
            break;
        }
    }

    if (i >= MAX_VIDEO_RENDER_INSTANCE) {
        ERROR("too many socket data server thread");
        return -1;
    }

    mSocketServerThread[i] = new SocketServerThread(this, socketfd);
    ret = mSocketServerThread[i]->init();
    if (!ret) {
        ERROR("Error create socket");
        goto tag_exit;
    }
    mSocketServerThread[i]->run("socketserver");

tag_exit:
    return ret;
}

bool RenderServer::destroySocketServerThread(int socketfd)
{
    int foundIdx = 0;
    for (int i = 0; i < MAX_VIDEO_RENDER_INSTANCE; i++) {
        if (mSocketServerThread[i]->getSocketFd() == socketfd) {
            foundIdx = i;
        }
    }

    //if found
    if (foundIdx > 0) {
        delete mSocketServerThread[foundIdx];
        mSocketServerThread[foundIdx] = NULL;
    }
    return true;
}

static bool g_running= false;
static class RenderServer *g_renderServer = NULL;

int main( int argc, char** argv)
{
    g_renderServer = new RenderServer();
    g_renderServer->createUeventProcessThread();
    g_running= true;
    while( g_running )
    {
        usleep( 10000 );
    }
    return 0;
}