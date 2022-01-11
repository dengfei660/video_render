#ifndef _TOOS_THREAD_H_
#define _TOOS_THREAD_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "Mutex.h"
#include "Condition.h"

typedef int (*thread_work_func) (void *);
typedef void* (*pthread_entry_func)(void*);
namespace Tls {
class Thread
{
  public:
    explicit Thread();
    virtual ~Thread();
    // Start the thread in threadLoop() which needs to be implemented.
    virtual int run(const char *name);

    // Good place to do one-time initializations
    virtual void readyToRun();

    //good place to do one-time exit thread
    virtual void readyToExit();
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void requestExit();
    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
    int    requestExitAndWait();
    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
    int join();
    // Indicates whether this thread is running or not.
    bool isRunning() const;
    void pause() {
        mPaused = true;
    };
    void resume() {
      if (!mPaused) {
              return;
      }
      Tls::Mutex::Autolock _l(mLock);
          mPaused = false;
          mCondition.broadcast();
    };
  protected:
    // isExitPending() returns true if requestExit() has been called.
    bool isExitPending() const;
  private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool threadLoop() = 0;
private:
    Thread& operator=(const Thread&);
    static  int _threadLoop(void* user);
    // always hold mLock when reading or writing
    pthread_t       mThread;
    const char *    mThreadName;
    mutable Tls::Mutex   mLock;
    Tls::Condition       mCondition;
    int        mStatus;
    // note that all accesses of mExitPending and mRunning need to hold mLock
    volatile bool           mExitPending;
    volatile bool           mRunning;
    volatile bool           mPaused;
};
}




#endif /*_TOOS_THREAD_H_*/