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
    mMonitorThread = NULL;
    mSinkMgr = new SinkManager();
    DEBUG("out");
}

RenderServer::~RenderServer()
{
    DEBUG("in");
    if (mMonitorThread) {
        delete mMonitorThread;
        mMonitorThread = NULL;
    }

    if (mSinkMgr) {
        delete mSinkMgr;
        mSinkMgr = NULL;
    }
    DEBUG("out");
}


void RenderServer::createMonitorThread()
{
    bool ret;
    mMonitorThread = new MonitorThread(mSinkMgr);
    ret = mMonitorThread->init();
    mMonitorThread->run("monitorThread");
}

void RenderServer::destroyMonitorThread()
{
    if (mMonitorThread) {
        delete mMonitorThread;
        mMonitorThread = NULL;
    }
}

static bool g_running= false;
static class RenderServer *g_renderServer = NULL;

int main( int argc, char** argv)
{
    //open log file
    char *env = getenv("VIDEO_RENDER_LOG_FILE");
    if (env && strlen(env) > 0) {
        Logger_set_file(env);
        INFO("VIDEO_RENDER_LOG_FILE=%s",env);
    }
    //set log level
    env = getenv("VIDEO_RENDER_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO("VIDEO_RENDER_LOG_LEVEL=%d",level);
    }
    g_renderServer = new RenderServer();
    g_renderServer->createMonitorThread();

    g_running= true;
    while ( g_running )
    {
        usleep( 10000 );
    }
    g_renderServer->destroyMonitorThread();
    delete g_renderServer;
    g_renderServer = NULL;
    return 0;
}