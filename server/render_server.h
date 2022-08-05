/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
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