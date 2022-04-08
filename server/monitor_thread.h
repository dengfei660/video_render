#ifndef __MONITOR_THREAD_H__
#define __MONITOR_THREAD_H__
#include <mutex>
#include <list>
#include <string>
#include <linux/videodev2.h>
#include "sink_manager.h"
#include "Thread.h"
#include "Poll.h"
#include "Mutex.h"

class RenderServer;
#define MAX_SUN_PATH (80)
#define SOCKET_NAME "render"

class MonitorThread : public Tls::Thread {
  public:
    MonitorThread(SinkManager *sinkMgr);
    virtual ~MonitorThread();
    bool init();

    //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    bool openUeventMonitor();
    bool openSocketMonitor();
    bool ueventEventProcess();
    bool socketEventProcess();
    int parseVdecPort(int clientfd);
    int mUeventFd;
    char mLockName[MAX_SUN_PATH+6];
    int mLockFd;
    struct sockaddr_un mAddr;
    int mSocketServerFd;
    Tls::Poll *mPoll;
    mutable Tls::Mutex mMutex;
    SinkManager *mSinkMgr;
};

#endif /*__MONITOR_THREAD_H__*/