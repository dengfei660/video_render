/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <assert.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include "Thread.h"
namespace Tls {
int createThread(thread_work_func entryFunction,
                               void *userData,
                               pthread_t *threadId)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr,
                    (pthread_entry_func)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        /*ALOGE("androidCreateRawThreadEtc failed (entry=%p, res=%d, %s)\n",
          entryFunction, result, strerror(errno));*/
        return 0;
    }

    // Note that *threadID is directly available to the parent only, as it is
    // assigned after the child starts.  Use memory barrier / lock if the child
    // or other threads also need access.
    if (threadId != NULL) {
        *threadId = thread; // XXX: this is not portable
    }
    return 1;
}

pthread_t getThreadId()
{
    return (pthread_t)pthread_self();
}

Thread::Thread()
    :mThread(pthread_t(-1)),
    mLock("Thread::mLock"),
    mStatus(NO_ERROR),
    mExitPending(false),
    mRunning(false)
{
}

Thread::~Thread()
{
}

void Thread::readyToRun()
{
}

void Thread::readyToExit()
{
}

int Thread::run(const char* name)
{
    Tls::Mutex::Autolock _l(mLock);

    if (mRunning) {
        // thread already started
        return ERROR_INVALID_OPERATION;
    }

    // reset status and exitPending to their default value, so we can
    // try again after an error happened (either below, or in readyToRun())
    mStatus = NO_ERROR;
    mExitPending = false;
    mThread = pthread_t(-1);
    mThreadName = name;

    bool res;
    res = createThread(_threadLoop, this, &mThread);

    if (res == false) {
        mStatus = ERROR_UNKNOWN;   // something happened!
        mRunning = false;
        mThread = pthread_t(-1);
        return ERROR_UNKNOWN;
    }

    // Do not refer to mStatus here: The thread is already running (may, in fact
    // already have exited with a valid mStatus result). The NO_ERROR indication
    // here merely indicates successfully starting the thread and does not
    // imply successful termination/execution.
    return NO_ERROR;

    // Exiting scope of mLock is a memory barrier and allows new thread to run
}

int Thread::_threadLoop(void* user)
{
    Thread* const self = static_cast<Thread*>(user);
    pthread_setname_np(pthread_self(), self->mThreadName);

    bool first = true;
    self->mRunning = true;

    do {
        bool result = true;
        if (first) {
            first = false;
            self->readyToRun();

            if (!self->isExitPending()) {
                // Binder threads (and maybe others) rely on threadLoop
                // running at least once after a successful ::readyToRun()
                // (unless, of course, the thread has already been asked to exit
                // at that point).
                // This is because threads are essentially used like this:
                //   (new ThreadSubclass())->run();
                // The caller therefore does not retain a strong reference to
                // the thread and the thread would simply disappear after the
                // successful ::readyToRun() call instead of entering the
                // threadLoop at least once.
                //result = self->threadLoop();
            }
        } else {
            result = self->threadLoop();
        }

        // establish a scope for mLock
        {
            Tls::Mutex::Autolock _l(self->mLock);
            if (result == false || self->mExitPending) {
                self->mExitPending = true;
                self->mRunning = false;
                // clear thread ID so that requestExitAndWait() does not exit if
                // called by a new thread using the same thread ID as this one.
                self->mThread = pthread_t(-1);
                // note that interested observers blocked in requestExitAndWait are
                // awoken by broadcast, but blocked on mLock until break exits scope
                self->mCondition.broadcast();
                break;
            }
        }
    } while(self->mRunning);

    self->mRunning = false;

    self->readyToExit();

    return 0;
}

void Thread::requestExit()
{
    Tls::Mutex::Autolock _l(mLock);
    mExitPending = true;
}

int Thread::requestExitAndWait()
{
    Tls::Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {
        /*ALOGW(
        "Thread (this=%p): don't call waitForExit() from this "
        "Thread object's thread. It's a guaranteed deadlock!",
        this);*/

        return ERROR_WOULD_BLOCK;
    }

    mExitPending = true;

    while (mRunning == true) {
        mCondition.wait(mLock);
    }
    // This next line is probably not needed any more, but is being left for
    // historical reference. Note that each interested party will clear flag.
    mExitPending = false;

    return mStatus;
}

int Thread::join()
{
    Tls::Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {
        /*ALOGW(
        "Thread (this=%p): don't call join() from this "
        "Thread object's thread. It's a guaranteed deadlock!",
        this);*/

        return ERROR_WOULD_BLOCK;
    }

    while (mRunning == true) {
        mCondition.wait(mLock);
    }

    return mStatus;
}

bool Thread::isRunning() const {
    Tls::Mutex::Autolock _l(mLock);
    return mRunning;
}

bool Thread::isExitPending() const
{
    Tls::Mutex::Autolock _l(mLock);
    return mExitPending;
}
}