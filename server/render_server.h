#ifndef __RENDER_SERVER_H__
#define __RENDER_SERVER_H__
#include <mutex>
#include <list>
#include <string>
#include "render_lib.h"
#include "Thread.h"
#include "Poll.h"
#include "sink_manager.h"
#include "monitor_thread.h"

class RenderServer {
  public:
    RenderServer();
    virtual ~RenderServer();
    void createMonitorThread();
    void destroyMonitorThread();
  private:
    MonitorThread *mMonitorThread;
    SinkManager *mSinkMgr;
};

#endif /*__RENDER_SERVER_H__*/