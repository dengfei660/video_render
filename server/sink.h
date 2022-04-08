#ifndef __SINK_INTERFACE_H__
#define __SINK_INTERFACE_H__
#include "Thread.h"
#include "Poll.h"

#define INVALIDE_PORT (0xAFFFFFFF)

class Sink {
  public:
    /**
     * @brief sink state
     * the state changed is
     * create->start->running->stop->destroy
     */
    typedef enum {
      STATE_CREATE,
      STATE_START,
      STATE_RUNNING,
      STATE_STOP,
      STATE_DESTROY,
    } States;

    Sink() {};
    virtual ~Sink() {};

    /**
     * @brief sink start work
     *
     * @return true if success
     * @return false if failed
     */
    virtual bool start() = 0;

    /**
     * @brief sink stop work
     *
     * @return true if success
     * @return false if failed
     */
    virtual bool stop() = 0;
    virtual void setVdecPort(uint32_t vdecPort) = 0;
    virtual void setVdoPort(uint32_t vdoPort) = 0;
    virtual States getState() = 0;
    /**
     * @brief get sink id
     *
     */
    virtual void getSinkPort(uint32_t *vdecPort, uint32_t *vdoPort) = 0;

    virtual bool isSocketSink() = 0;
};

#endif /*__SINK_INTERFACE_H__*/