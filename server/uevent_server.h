#ifndef __UEVENT_SERVER_H__
#define __UEVENT_SERVER_H__
#include <mutex>
#include <list>
#include <string>
#include <linux/videodev2.h>
#include "render_lib.h"
#include "Thread.h"
#include "Poll.h"

class RenderServer;

class UeventProcessThread : public Tls::Thread {
  public:
    UeventProcessThread(RenderServer *renderServer);
    virtual ~UeventProcessThread();
    bool init();

    //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    int mUeventFd;
    Tls::Poll *mPoll;
    RenderServer *mRenderServer;
};

#endif /*__UEVENT_SERVER_H__*/