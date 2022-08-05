/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _TOOS_QUEUE_H_
#define _TOOS_QUEUE_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h> /* EBUSY */
#include <pthread.h> /* pthread_mutex_t, pthread_cond_t */

/**
  * returned error codes, everything except Q_OK should be < 0
  */
typedef enum QErros {
    Q_OK = 0,
    Q_ERR_INVALID = -1,
    Q_ERR_LOCK = -2,
    Q_ERR_MEM = -3,
    Q_ERR_NONEWDATA = -4,
    Q_ERR_INVALID_ELEMENT = -5,
    Q_ERR_INVALID_CB = -6,
    Q_ERR_NUM_ELEMENTS = -7
} QErros;

namespace Tls {

class Queue {
  public:
    /**
      * initializes a queue with unlimited elements
      *
      */
    Queue();
    /**
      * initializes a queue with max element count
      *
      * maxElement - maximum number of elements which are allowed in the queue, == 0 for "unlimited"
      *
      */
    Queue(uint32_t maxElement);
    /**
      * just like Queue()
      * additionally you can specify a comparator function so that your elements are ordered in the queue
      * elements will only be ordered if you create the queue with this method
      * the compare function should return 0 if both elements are the same, < 0 if the first is smaller
      * and > 0 if the second is smaller
      *
      * asc - sort in ascending order if not 0
      * cmp - comparator function, NULL will create an error
      *
      */
    Queue(bool ascendingOrder, int (*cmp)(void *, void *));
    /**
      * initializes a queue,limit the queue max element count and set the order rule
      *
      * maxElement - maximum number of elements which are allowed in the queue, == 0 for "unlimited"
      *
      */
    Queue(uint32_t maxElement, bool ascendingOrder, int (*cmp)(void *, void *));
    virtual ~Queue();

    /**
      * put a new element at the end of the queue
      * will produce an error if you called setAllowedNewData()
      *
      * e - the element
      *
      * returns 0 if everything worked, > 0 if max_elements is reached, < 0 if error occured
      */
    int32_t push(void *ele);

    /**
      * the same as push(), but will wait if max_elements is reached,
      * until setAllowedNewData(true) is called or elements are removed
      *
      * e - the element
      *
      * returns 0 if everything worked, < 0 if error occured
      */
    int32_t pushAndWait(void *ele);

    /**
      * get the first element of the queue
      *
      * e - pointer which will be set to the element
      *
      * returns 0 if everything worked, > 0 if no elements in queue, < 0 if error occured
      */
    int32_t pop(void **e);

    /**
      * the same as pop(), but will wait if no elements are in the queue,
      * until setAllowedNewData(true) is called or new elements are added
      *
      * q - the queue
      * e - pointer which will be set to the element
      *
      * returns 0 if everything worked, < 0 if error occured
      */
    int32_t popAndWait(void **e);

    /**
     * peek the pos element
     */
    int32_t peek(void **e, int32_t pos);
    /**
     * only flush element,but not free element's data
     * returns 0 on success, other failed
     */
    int32_t flush();
    /**
     * flush element,and call ff function to notify element action
     * fcb - the callback function
     * returns 0 on success, other failed
     */
    int32_t flushAndCallback(void *userdata, void (*fcb)(void *userdata, void *ele));
    /**
     * get element count in queue
     */
    int32_t getCnt();
    /**
     * checking the queue is empty?
     */
    bool isEmpty();
    /**
     * set queue allow add new element
     */
    int32_t setAllowedNewData(bool allowed);
    /**
     * check allowed new element flag
     */
    bool isAllowedNewData();
  private:
    typedef struct Element{
      void *data;
      struct Element *next;
    } Element;
    int32_t _init();
    /**
     * destroys a queue.
     * queue will be locked.
     *
     * ff - callback function,this func will be invoked when a element been flushed
     */
    int32_t _release();
    /**
     * locks the queue
     * returns 0 on success, else not usable
     */
    int32_t _lock();
    /**
     * unlocks the queue
     * returns 0 on success, else not usable
     */
    int32_t _unlock();
    /**
      * flushes a queue.
      * deletes all elements in the queue.
      * queue _has_ to be locked.
      *
      * ff - callback function,this func will be invoked when a element been flushed
      */
    int32_t _flushElements(void *userdata, void (*fcb)(void *userdata, void *ele));
    /**
     * adds an element to the queue.
     * when isWait is true, the put element thread will block
     * if the queue is full until getElement be invoked
     * queue _has_ to be locked.
     *
     * ele - the element
     * isWait - if false,this func will return immediately
     *          if true, this func will block when queue is reached the queue capability
     *            until getElement be invoked
     *
     * returns < 0 => error, 0 okay
     */
    int32_t _pushElement(void *ele, bool isWait);
    /**
      * pop the special element from the queue.
      * the cmp param is a compare function,this function will
      * compare every element of the queue with cmpeEle,if equal cmp will return 0
      * or return < 0 if the element in the queue less than cmpEle or return > 0 if
      * the element in the queue bigger than cmpEle
      *
      * queue has to be locked.
      *
      * e - element pointer
      * isWait - check if block when there are no elements in the queue
      * cmp - comparator function, NULL will create an error
      * cmpEle - element with which should be compared
      *
      * returns < 0 => error, 0 okay
      */
    int32_t _popElement(void **e, bool isWait, int (*cmp)(void *, void *), void *cmpEle);

    int32_t _peekElement(void **e, int32_t pos);

    Element *mUsedFirstElement;
    Element *mUsedLastElement;
    uint32_t mUsedElementCnts; //the cnt of elements in queue
    uint32_t mCapability; //the capability of the queue

    bool mAllowedNewData; // no new data allowed

    //sorted queue
    bool mSort;
    bool mAscendingOrder;
    int (*cmpEleFun)(void *, void *);

    pthread_mutex_t mMutex;
    pthread_cond_t mCondGet;
    pthread_cond_t mCondPut;

};
}

#endif // _TOOS_QUEUE_H_

