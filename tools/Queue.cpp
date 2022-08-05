/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <unistd.h>
#include "Queue.h"

namespace Tls {

Queue::Queue()
{
    _init();
}

Queue::Queue(uint32_t maxElement)
{
    _init();
    if (maxElement > 0) {
        mCapability = maxElement;
    }
}

Queue::Queue(bool ascendingOrder, int (*cmp)(void *, void *))
{
    _init();
    if (cmp) {
        mSort = true;
        mAscendingOrder = ascendingOrder;
        cmpEleFun = cmp;
    }
}

Queue::Queue(uint32_t maxElement, bool ascendingOrder, int (*cmp)(void *, void *))
{
    _init();
    if (maxElement > 0) {
        mCapability = maxElement;
    }
    if (cmp) {
        mSort = true;
        mAscendingOrder = ascendingOrder;
        cmpEleFun = cmp;
    }
}

Queue::~Queue()
{
    _release();
}

int32_t Queue::push(void *ele)
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _pushElement(ele, false);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}

int32_t Queue::pushAndWait(void *ele)
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _pushElement(ele, true);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}

int32_t Queue::pop(void **e)
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _popElement(e, false, NULL, NULL);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}

int32_t Queue::popAndWait(void **e)
{
    *e = NULL;

    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _popElement(e, true, NULL, NULL);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}

int32_t Queue::peek(void **e, int32_t pos)
{
    *e = NULL;

    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _peekElement(e, pos);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}


int32_t Queue::flush()
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _flushElements(NULL, NULL);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;
    return ret;
}
int32_t Queue::flushAndCallback(void *userdata, void (*fcb)(void *userdata, void *ele))
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    int32_t ret = _flushElements(userdata, fcb);

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return ret;
}


int32_t Queue::getCnt()
{
    int32_t cnt = 0;
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    cnt = mUsedElementCnts;

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return cnt;
}


bool Queue::isEmpty()
{
    bool isEmpty;

    _lock();

    if (mUsedFirstElement == NULL || mUsedLastElement == NULL)
        isEmpty = true;
    else
        isEmpty = false;

    _unlock();

    return isEmpty;
}


int32_t Queue::setAllowedNewData(bool allowed)
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    mAllowedNewData = allowed;

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    if (mAllowedNewData == false) {
        // notify waiting threads, when new data isn't accepted
        pthread_cond_broadcast(&mCondGet);
        pthread_cond_broadcast(&mCondPut);
    }

    return Q_OK;
}

bool Queue::isAllowedNewData()
{
    if (Q_OK != _lock())
        return Q_ERR_LOCK;

    bool allowed = mAllowedNewData;

    if (Q_OK != _unlock())
        return Q_ERR_LOCK;

    return allowed;
}


int32_t Queue::_init()
{
    pthread_mutex_init(&mMutex, NULL);

    pthread_cond_init(&mCondGet, NULL);

    pthread_cond_init(&mCondPut, NULL);

    mUsedFirstElement = NULL;
    mUsedLastElement = NULL;
    mUsedElementCnts = 0;
    mCapability = INT32_MAX - 1;//max capability
    mAllowedNewData = true;
    mSort = false;
    mAscendingOrder = 1;
    cmpEleFun = NULL;
    return Q_OK;
}

int32_t Queue::_release()
{
    // this method will not immediately return on error,
    // it will try to release all the memory that was allocated.
    int error = Q_OK;
    // make sure no new data comes and wake all waiting threads
    error = setAllowedNewData(false);
    error = _lock();

    error = _flushElements(NULL, NULL);

    mUsedFirstElement = NULL;
    mUsedLastElement = NULL;

    // destroy lock and queue etc
    error = pthread_cond_destroy(&mCondGet);
    error = pthread_cond_destroy(&mCondPut);

    error = _unlock();
    while (EBUSY == (error = pthread_mutex_destroy(&mMutex)))
        usleep(100*1000);

    return error;
}

int32_t Queue::_lock()
{
    // all errors are unrecoverable for us
    if (0 != pthread_mutex_lock(&mMutex))
        return Q_ERR_LOCK;
    return Q_OK;
}
int32_t Queue::_unlock()
{
    // all errors are unrecoverable for us
    if (0 != pthread_mutex_unlock(&mMutex))
        return Q_ERR_LOCK;
    return Q_OK;
}

int32_t Queue::_flushElements(void *userdata, void (*fcb)(void *userdata, void *ele))
{
    Element *ele  = NULL;
    while (mUsedFirstElement != NULL) {
        ele = mUsedFirstElement;
        mUsedFirstElement = mUsedFirstElement->next;
        if (fcb != NULL) {
            fcb(userdata,ele->data);
        }
        free(ele);
    }
    mUsedFirstElement = NULL;
    mUsedLastElement = NULL;
    mUsedElementCnts = 0;

    return Q_OK;
}

int32_t Queue::_pushElement(void *ele, bool isWait)
{
    Element *newEle = NULL;
    if (mAllowedNewData == false) { // no new data allowed
        return Q_ERR_NONEWDATA;
    }

    // max_elements already reached?
    // if condition _needs_ to be in sync with while loop below!
    if (mUsedElementCnts ==  mCapability) {
        if (isWait == false) {
            return Q_ERR_NUM_ELEMENTS;
        } else {
            while ((mUsedElementCnts == mCapability) && mAllowedNewData) {
                pthread_cond_wait(&mCondPut, &mMutex);
            }
            if (mAllowedNewData == false) {
                return Q_ERR_NONEWDATA;
            }
        }
    }

    newEle = (Element *)calloc(1,sizeof(Element));
    if (newEle == NULL) { // could not allocate memory for new elements
        return Q_ERR_MEM;
    }
    newEle->data = ele;
    newEle->next = NULL;
//printf("_pushElement,%p,%p,data:%p\n",mUsedLastElement,newEle,ele);
    if (mSort == false || mUsedFirstElement == NULL) {
        // insert at the end when we don't want to sort or the queue is empty
        if (mUsedLastElement == NULL)
            mUsedFirstElement = newEle;
        else
            mUsedLastElement->next = newEle;
        mUsedLastElement = newEle;
    } else {
        // search appropriate place to sort element in
        Element *s = mUsedFirstElement; // s != NULL, because of if condition above
        Element *t = NULL;
        //check if insert new element to the first element
        int asc_first_el = (mAscendingOrder == true && cmpEleFun(s->data, ele) >= 0);
        int desc_first_el = (mAscendingOrder == false && cmpEleFun(s->data, ele) <= 0);

        if (asc_first_el == 0 && desc_first_el == 0) {
            // element will be inserted between s and t
            for (s = mUsedFirstElement, t = s->next; s != NULL && t != NULL; s = t, t = t->next) {
                if (mAscendingOrder == true && cmpEleFun(s->data, ele) <= 0 && cmpEleFun(ele, t->data) <= 0) {
                    // asc: s <= e <= t
                    break;
                } else if(mAscendingOrder == false && cmpEleFun(s->data, ele) >= 0 && cmpEleFun(ele, t->data) >= 0) {
                    // desc: s >= e >= t
                    break;
                }
            }
            // actually insert
            s->next = newEle;
            newEle->next = t;
            if (t == NULL)
                mUsedLastElement = newEle;
        } else if(asc_first_el != 0 || desc_first_el != 0) {
            // add at front
            newEle->next = mUsedFirstElement;
            mUsedFirstElement = newEle;
        }
    }
    mUsedElementCnts++;
    //printf("_pushElement,%p,data:%p,%d\n",mUsedLastElement,mUsedLastElement->data,mUsedElementCnts);
    // notify only one waiting thread, so that we don't have to check and fall to sleep because we were to slow
    pthread_cond_signal(&mCondGet);

    return Q_OK;
}

int32_t Queue::_popElement(void **e, bool isWait, int (*cmp)(void *, void *), void *cmpEle)
{
    // are elements in the queue?
    if (mUsedElementCnts == 0) {
        if (isWait == false) {
            *e = NULL;
            return Q_ERR_NUM_ELEMENTS;
        } else {
            while (mUsedElementCnts == 0 && mAllowedNewData) {
                pthread_cond_wait(&mCondGet, &mMutex);
            }
            if (mUsedElementCnts == 0 && mAllowedNewData == false) {
                return Q_ERR_NONEWDATA;
             }
        }
    }

    // get first element (which fulfills the requirements)
    Element *elePrev = NULL, *ele = mUsedFirstElement;
    while (cmp != NULL && ele != NULL && 0 != cmp(ele, cmpEle)) {
        elePrev = ele;
        ele = ele->next;
    }
    if (ele != NULL && elePrev == NULL) {
        *e = mUsedFirstElement->data;
        //element is at first, remove this node
        mUsedFirstElement = mUsedFirstElement->next;
        --mUsedElementCnts;
        if (mUsedFirstElement == NULL) {
            mUsedLastElement = NULL;
        }
        //printf("_popElement,%p,data:%p,%p\n",ele,*e,mUsedFirstElement);
        free(ele);
    } else if (ele != NULL && elePrev != NULL) {
        // element is in the middle,remove this node
        elePrev->next = ele->next;
        --mUsedElementCnts;
        *e = ele->data;
        free(ele);
    } else {
        // element is invalid
        *e = NULL;
        return Q_ERR_INVALID_ELEMENT;
    }

    // notify only one waiting thread
    pthread_cond_signal(&mCondPut);

    return Q_OK;
}

int32_t Queue::_peekElement(void **e, int32_t pos)
{
    Element *ele = mUsedFirstElement;
    int32_t elePos = 0;
    if (mUsedElementCnts == 0) {
        return Q_ERR_INVALID_ELEMENT;
    }

    while (ele != NULL && elePos++ < pos) {
        ele = ele->next;
    }

    if (ele == NULL) {
        return Q_ERR_INVALID_ELEMENT;
    }
    *e = ele->data;
    return Q_OK;
}

}
