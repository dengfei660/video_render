#ifndef _TOOS_POLL_H_
#define _TOOS_POLL_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <atomic>
#include <poll.h>

#include "Mutex.h"

/**
 * @brief Poll is a implement wrap about
 * poll,but Poll can wakeup when poll is waiting
 * for ever,if set flushing
 * the sequence api is
 * 1.poll = new Poll(true)
 * 2.poll->addfd(fd)
 * 3.poll->setFdReadable(fd,true)/poll->setFdWritable(fd,true)
 * 4.poll->wait(waittime)
 * ........
 * 5.if (poll->isReadable(fd))
 * 6. read data from fd
 * 7.if (poll->isReadable(fd))
 * 8. write data to fd
 * if want to destroy poll,call
 * 9.poll->setFlushing
 * 10.delete poll
 */
namespace Tls {
class Poll {
  public:
    Poll(bool controllable);
    virtual ~Poll();
    /**
     * @brief add a fd to poll
     *
     * @param fd
     * @return int
     */
    int addFd(int fd);
    /**
     * @brief remove fd from poll
     *
     * @param fd
     * @return int
     */
    int removeFd(int fd);
    /**
     * @brief Set the Fd Readable ,poll will
     * check readable event
     * @param fd
     * @param readable
     * @return int 0 success, -1 fail
     */
    int setFdReadable(int fd, bool readable);
    /**
     * @brief Set the Fd Writable,poll will
     * check writable event
     * @param fd
     * @param writable
     * @return int 0 success, -1 fail
     */
    int setFdWritable(int fd, bool writable);
    /**
     * @brief wait fd event
     *
     * @param timeoutNs wait nanosecond time, -1 will wait for ever
     * otherwise wait the special nanasecond time
     * @return int active fd count
     */
    int wait(int64_t timeoutNs);
    /**
     * @brief Set the Flushing if poll wait had called
     * and waiting, this func will send a raise event to
     * wakeup poll wait
     * @param flushing
     */
    void setFlushing(bool flushing);
    /**
     * @brief check this fd if had data to read
     *
     * @param fd file fd
     * @return true
     * @return false
     */
    bool isReadable(int fd);
    /**
     * @brief check this fd if can write
     *
     * @param fd file fd
     * @return true
     * @return false
     */
    bool isWritable(int fd);
  private:
    struct pollfd * findFd(int fd);
    bool wakeEvent();
    bool releaseEvent();
    bool raiseWakeup();
    bool releaseWakeup();
    bool releaseAllWakeup();
    Tls::Mutex mMutex;
    int mControlReadFd;
    int mControlWriteFd;
    struct pollfd * mFds;
    int mFdsMaxCnt;
    int mFdsCnt;
    bool mControllable;
    std::atomic<int> mControlPending;
    std::atomic<int> mFlushing;
    std::atomic<int> mWaiting; //store waiting thread count
};

}

#endif /*_TOOS_POLL_H_*/
