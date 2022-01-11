#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__
#include <mutex>
#include <list>
#include <string>
#include "render_lib.h"
#include "Thread.h"
#include "Poll.h"

#define MAX_SUN_PATH (80)

class RenderServer;

class SocketConnectThread : public Tls::Thread {
  public:
    SocketConnectThread(RenderServer *renderServer);
    virtual ~SocketConnectThread();
    bool init();

    //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    char mLockName[MAX_SUN_PATH+6];
    int mLockFd;
    struct sockaddr_un mAddr;
    int mSocketServerFd;
    Tls::Poll *mPoll;
    RenderServer *mRenderServer;
};

class SocketServerThread : public Tls::Thread {
  public:
    SocketServerThread(RenderServer *renderServer, int socketfd);
    virtual ~SocketServerThread();
    bool init();
    int getSocketFd() {
        return mSocketFd;
    };
    //thread func
    void readyToRun();
    void readyToExit();
    virtual bool threadLoop();
  private:
    int adaptFd( int fdin );
    void processEvent();
    int mSocketFd;
    Tls::Mutex mMutex;
    Tls::Poll *mPoll;
    RenderServer *mRenderServer;
};


#endif /*__SOCKET_SERVER_H__*/