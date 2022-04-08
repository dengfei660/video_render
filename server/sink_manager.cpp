#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "sink_manager.h"
#include "socket_sink.h"
#include "vdo_sink.h"
#include "Logger.h"
#include "Times.h"
#include "Utils.h"

using namespace Tls;

#define TAG "rlib:sink_mgr"

SinkManager::SinkManager()
{
    mSinkCnt = 0;
    for (int i = 0; i < MAX_SINKS; i++) {
        mAllSinks[i] = NULL;
    }
}

SinkManager::~SinkManager()
{
    //must require thread is stopped
    if (isRunning()) {
        requestExitAndWait();
    }

    for (int i = 0; i < MAX_SINKS; i++) {
        if (mAllSinks[i]) {
            mAllSinks[i]->stop();
            delete mAllSinks[i];
            mAllSinks[i] = NULL;
        }
    }
}

bool SinkManager::createVdoSink(int vdecPort, int vdoPort)
{
    Tls::Mutex::Autolock _l(mMutex);
    Sink * sink = findSinkByVdecPort(vdecPort);
    //check if socket had create a sink for this vdecPort
    if (sink) {
        uint32_t videoDecPort, videoOutPort;
        Sink::States state = sink->getState();
        bool isSocketSink = sink->isSocketSink();
        sink->getSinkPort(&videoDecPort, &videoOutPort);

        if (isSocketSink) {
            WARNING("sink had been created,issocketsink");
            sink->setVdoPort(vdoPort);
            sink->start();
            return true;
        }

        if (vdecPort == videoDecPort && vdoPort == videoOutPort) {
            WARNING("Had created a sink with vdecPort:%d, vdoPort:%d, please release before one");
            return false;
        }
    }

    //create a vdo sink first, then if socketthread receive
    //create sink event, this sink will be destroy and
    //a new socket sink be create,then start it
    //if socketthread donot receive creating sink event,
    //manager will start it when WAIT_SOCKET_TIME_MS timeout
    int freeIndex = -1;
    for (int i = 0; i < MAX_SINKS; i++) {
        if (!mAllSinks[i]) {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex == -1) {
        ERROR("sink count is reached Max %d",mSinkCnt);
        return false;
    }

    ++mSinkCnt;
    INFO("Had created sink cnt:%d",mSinkCnt);
    mAllSinks[freeIndex] = new VDOSink(this, vdecPort, vdoPort);
    mAllSinks[freeIndex]->start();

    //now run thread to create sink
    //Tls::Mutex::Autolock _l(mMutex);
    //mCondition.waitRelative(mMutex, WAIT_SOCKET_TIME_MS/*ms*/);
    //run("sinkmgr");

    return true;
}

bool SinkManager::createSocketSink(int socketfd, int vdecPort)
{
    Sink * sink = findSinkByVdecPort(vdecPort);
    INFO("vdecPort:%d,socketfd:%d",vdecPort, socketfd);

    Tls::Mutex::Autolock _l(mMutex);
    //check if uevent thread had create a sink for vdecport
    if (sink) {
        uint32_t videoDecPort, vdoPort;
        Sink::States state = sink->getState();
        bool isSocketSink = sink->isSocketSink();
        sink->getSinkPort(&videoDecPort, &vdoPort);
        //if sink is running,so socket create sink is valid
        if (state == Sink::STATE_RUNNING && isSocketSink) {
            WARNING("socketSink is running, vdecPort:%d, vdoPort:%d",videoDecPort,vdoPort);
            return false;
        }
        //uevent thread create a vdo sink object,
        //we must delete it and create a new socket sink
        if (isSocketSink == false) {
            sink->stop();
            delete sink;
            --mSinkCnt;
        }
    }

    int freeIndex = -1;
    for (int i = 0; i < MAX_SINKS; i++) {
        if (!mAllSinks[i]) {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex == -1) {
        ERROR("sink count is reached Max %d",mSinkCnt);
        return false;
    }

    ++mSinkCnt;
    INFO("Had created sink cnt:%d",mSinkCnt);
    /*create a new socket sink to receive video frame data and
    than set vdo port*/
    mAllSinks[freeIndex] = new SocketSink(this, socketfd, vdecPort);
    mAllSinks[freeIndex]->start();
    return true;
}

bool SinkManager::destroySink(int vdecPort, int vdoPort)
{
    INFO("vdecPort:%d, vdoPort:%d",vdecPort,vdoPort);
    for (int i = 0; i < MAX_SINKS; i++) {
        uint32_t videoDecPort, voutPort;
        mAllSinks[i]->getSinkPort(&videoDecPort, &voutPort);
        if (vdecPort == videoDecPort && vdoPort == voutPort) {
            mAllSinks[i]->stop();
            delete mAllSinks[i];
            mAllSinks[i] = NULL;
            return true;
        }
    }

    WARNING("Not found sink for vdecPort:%d,vdoPort:%d",vdecPort, vdoPort);
    dumpSinkInfo();
    return true;
}

Sink *SinkManager::findSinkByVdecPort(int vdecPort)
{
    for (int i = 0; i < MAX_SINKS; i++) {
        if (mAllSinks[i]) {
            uint32_t iPort, oPort;
            mAllSinks[i]->getSinkPort(&iPort, &oPort);
            if (iPort == vdecPort) {
                return mAllSinks[i];
            }
        }
    }
    return NULL;
}

Sink *SinkManager::findSinkByVdoPort(int vdoPort)
{
    for (int i = 0; i < MAX_SINKS; i++) {
        if (mAllSinks[i]) {
            uint32_t iPort, oPort;
            mAllSinks[i]->getSinkPort(&iPort, &oPort);
            if (oPort == vdoPort) {
                return mAllSinks[i];
            }
        }
    }
    return NULL;
}

void SinkManager::dumpSinkInfo()
{
    for (int i = 0; i < MAX_SINKS; i++) {
        if (mAllSinks[i]) {
            uint32_t iPort, oPort;
            mAllSinks[i]->getSinkPort(&iPort, &oPort);
            INFO("index:%d, vdecPort:%d, vdoPort:%d",i, iPort, oPort);
        }
    }
}

bool SinkManager::threadLoop()
{
    for (int i = 0; i < MAX_SINKS; i++) {
        if (mAllSinks[i] && mAllSinks[i]->getState() < Sink::STATE_START) {
            mAllSinks[i]->start();
        }
    }

    //return false to exit thread
    return false;
}