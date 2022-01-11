/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TOOLS_CONDITION_H_
#define _TOOLS_CONDITION_H_

#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "Mutex.h"

namespace Tls {
class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    enum WakeUpType {
        WAKE_UP_ONE = 0,
        WAKE_UP_ALL = 1
    };

    Condition();
    explicit Condition(int type);
    ~Condition();
    // Wait on the condition variable.  Lock the mutex before calling.
    // Note that spurious wake-ups may happen.
    int wait(Mutex& mutex);
    // same with relative timeout , microsecond time
    int waitRelative(Mutex& mutex, int64_t reltime);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing one or all threads to continue.
    void signal(WakeUpType type) {
        if (type == WAKE_UP_ONE) {
            signal();
        } else {
            broadcast();
        }
    }
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();

private:
    pthread_cond_t mCond;
};

inline Condition::Condition() : Condition(PRIVATE) {
}
inline Condition::Condition(int type) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

    if (type == SHARED) {
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    }

    pthread_cond_init(&mCond, &attr);
    pthread_condattr_destroy(&attr);

}
inline Condition::~Condition() {
    pthread_cond_destroy(&mCond);
}

/**
 * wait relative nanosecond time
 * return 0 if wait success, non 0 if timeout or other
*/
inline int Condition::wait(Mutex& mutex) {
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}

/**
 * wait relative microsecond time
 * return 0 if wait success, non 0 if timeout or other
 * ETIMEDOUT timeout,
*/
inline int Condition::waitRelative(Mutex& mutex, int64_t reltime/*microsecnd*/) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);


    // On 32-bit devices, tv_sec is 32-bit, but `reltime` is 64-bit.
    int64_t reltime_sec = reltime/1000;

    ts.tv_nsec += static_cast<long>(reltime*1000*1000%1000000000);
    if (reltime_sec < INT64_MAX && ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ++reltime_sec;
    }

    int64_t time_sec = ts.tv_sec;
    if (time_sec > INT64_MAX - reltime_sec) {
        time_sec = INT64_MAX;
    } else {
        time_sec += reltime_sec;
    }

    ts.tv_sec = (time_sec > LONG_MAX) ? LONG_MAX : static_cast<long>(time_sec);

    return -pthread_cond_timedwait(&mCond, &mutex.mMutex, &ts);
}

inline void Condition::signal() {
    pthread_cond_signal(&mCond);
}
inline void Condition::broadcast() {
    pthread_cond_broadcast(&mCond);
}

}

#endif // _TOOLS_CONDITION_H_
