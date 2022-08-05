/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __WST_CLIENT_PLUGIN_H__
#define __WST_CLIENT_PLUGIN_H__
#include "render_plugin.h"
#include "wstclient_wayland.h"
#include "wstclient_socket.h"
#include <mutex>

class WstClientPlugin : public RenderPlugin
{
  public:
    WstClientPlugin(int logCategory);
    virtual ~WstClientPlugin();
    virtual void init();
    virtual void release();
    void setUserData(void *userData, PluginCallback *callback);
    virtual int acquireDmaBuffer(int framewidth, int frameheight);
    virtual int releaseDmaBuffer(int dmafd);
    virtual int openDisplay();
    virtual int openWindow();
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime);
    virtual int flush();
    virtual int pause();
    virtual int resume();
    virtual int closeDisplay();
    virtual int closeWindow();
    virtual int get(int key, void *value);
    virtual int set(int key, void *value);
    virtual int getState();
    //buffer release callback
    void handleBufferRelease(RenderBuffer *buffer);
    //buffer had displayed ,but not release
    void handleFrameDisplayed(RenderBuffer *buffer);
    //buffer droped callback
    void handleFrameDropped(RenderBuffer *buffer);

    void onWstSocketEvent(WstEvent *event);

    void setVideoRect(int videoX, int videoY, int videoWidth, int videoHeight);
  private:
    typedef struct {
        bool isSet;
        bool value;
    } ConfigValue;
    /**
     * @brief Get the Display Frame Buffer Id object
     *
     * @param displayTime
     * @return int > 0 if sucess, -1 if not found
     */
    int getDisplayFrameBufferId(int64_t displayTime);
    PluginCallback *mCallback;
    WstClientWayland *mWayland;
    WstClientSocket *mWstClientSocket;
    PluginRect mWinRect;

    int mLogCategory;

    ConfigValue mKeepLastFrame;
    ConfigValue mHideVideo;

    std::mutex mRenderLock;
    bool mFullscreen; //default true value to full screen show video

    int mNumDroppedFrames;
    int mCommitFrameCnt; //the count frames of commiting to server

    RenderVideoFormat mBufferFormat;
    std::unordered_map<int, RenderBuffer *> mRenderBuffersMap;
    /*key is buffer id, value is display time*/
    std::unordered_map<int, int64_t> mDisplayedFrameMap;

    bool mIsVideoPip;
    mutable Tls::Mutex mMutex;
    void *mUserData;
    int mState;
};


#endif /*__WST_CLIENT_PLUGIN_H__*/