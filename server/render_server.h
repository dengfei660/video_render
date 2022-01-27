#ifndef __RENDER_SERVER_H__
#define __RENDER_SERVER_H__
#include <mutex>
#include <list>
#include <string>
#include "render_lib.h"
#include "Thread.h"
#include "Poll.h"
#include "vdo_server.h"
#include "uevent_server.h"
#include "socket_server.h"

#define MAX_VIDEO_RENDER_INSTANCE 9
#define MAX_SUN_PATH (80)
#define MAX_PLANES (3)

class RenderServer;

class RenderServer {
  public:
    RenderServer();
    virtual ~RenderServer();
    bool createUeventProcessThread();
    bool createSocketConnectThread();
    /**
     * @brief create a receive VDO data server thread
     *
     * @param port vdo channel port
     * @return int thread index or -1 if fail
     */
    bool createVDOServerThread(uint32_t ctrId, uint32_t vdoPort, uint32_t vdecPort);
    bool destroyVDOServerThread(uint32_t ctrId, uint32_t vdoPort, uint32_t vdecPort);
    bool createSocketServerThread(int socketfd);
    bool destroySocketServerThread(int socketfd);
  private:
    int initRenderlib();
    int releaseRenderlib();
    UeventProcessThread *mUeventProcessThread;
    SocketConnectThread *mSocketConnectThread;
    VDOServerThread *mVDOServerThread[MAX_VIDEO_RENDER_INSTANCE];
    SocketServerThread *mSocketServerThread[MAX_VIDEO_RENDER_INSTANCE];
    int mRenderInstanceCnt;
};

#endif /*__RENDER_SERVER_H__*/