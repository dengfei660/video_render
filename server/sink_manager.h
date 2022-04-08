#ifndef __SINK_MANAGER_H__
#define __SINK_MANAGER_H__
#include <mutex>
#include <list>
#include <string>
#include <unordered_map>
#include "Thread.h"
#include "Poll.h"
#include "sink.h"

#define MAX_SINKS (2)
#define WAIT_SOCKET_TIME_MS (500)

class SinkManager : public Tls::Thread {
  public:
    SinkManager();
    virtual ~SinkManager();

    bool createVdoSink(int vdecPort, int vdoPort);
    bool createSocketSink(int socketfd, int vdecPort);
    bool destroySink(int vdecPort, int vdoPort);

    //thread func
    virtual bool threadLoop();
  private:
    /**
     * @brief find a existing sink if this sink had been created
     * or not if the sink don't been created
     *
     * @param vdecPort video decoder port
     * @return Sink* the special vdecport sink or null
     */
    Sink *findSinkByVdecPort(int vdecPort);
    /**
     * @brief find a existing sink if this sink had been created
     * or not if the sink don't been created
     *
     * @param vdoPort vdo port
     * @return Sink* the special vdoPort sink or null
     */
    Sink *findSinkByVdoPort(int vdoPort);
    void dumpSinkInfo();
    //LGE defined 2 vdo devices
    Sink *mAllSinks[MAX_SINKS];
    int mSinkCnt;
    mutable Tls::Mutex mMutex;
    Tls::Condition mCondition;
};

#endif /*__SINK_MANAGER_H__*/