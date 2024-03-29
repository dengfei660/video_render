/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __RENDER_PLUGIN_H__
#define __RENDER_PLUGIN_H__
#include "render_lib.h"

typedef struct _PluginCallback PluginCallback;
typedef void (*PluginMsgCallback)(void *handle, int msg, void *detail);
typedef void (*PluginBufferReleaseCallback)(void *handle,void *data);
typedef void (*PluginBufferDisplayedCallback)(void *handle,void *data);
typedef void (*PluginBufferDropedCallback)(void *handle,void *data);

struct _PluginCallback {
    PluginMsgCallback doSendMsgCallback;
    PluginBufferReleaseCallback doBufferReleaseCallback;
    PluginBufferDisplayedCallback doBufferDisplayedCallback;
    PluginBufferDropedCallback doBufferDropedCallback;
};

/**
 * @brief plugin message type
 * it is used on function PluginMsgCallback, the param detail of PluginMsgCallback
 * must mapped the message type of value
 *
 */
enum _PluginMsg {
    PLUGIN_MSG_NOTIFY = 0, //msg of notity
    PLUGIN_MSG_DISPLAY_OPEN_SUCCESS = 200, //msg of display open success
    PLUGIN_MSG_WINDOW_OPEN_SUCCESS, //msg of window open success
    PLUGIN_MSG_DISPLAY_CLOSE_SUCCESS, //msg of window close success
    PLUGIN_MSG_WINDOW_CLOSE_SUCCESS, //msg of window close success
    PLUGIN_MSG_DISPLAY_OPEN_FAIL, //msg of display open fail
    PLUGIN_MSG_WINDOW_OPEN_FAIL, //msg of window open fail
    PLUGIN_MSG_DISPLAY_CLOSE_FAIL, //msg of display close fail
    PLUGIN_MSG_WINDOW_CLOSE_FAIL, //msg of window close fail
    PLUGIN_MSG_UNKNOW_ERROR = 300, //msg of unknow error
};

/**
 * @brief plugin state
 *
 */
enum _PluginState {
    PLUGIN_STATE_IDLE = 0,
    PLUGIN_STATE_INITED = 1 << 1,
    PLUGIN_STATE_DISPLAY_OPENED = 1 << 2,
    PLUGIN_STATE_WINDOW_OPENED = 1 << 3,
};

/**
 * @brief plugin key type
 * it is used by set/get function
 *
 */
enum _PluginKey {
    PLUGIN_KEY_WINDOW_SIZE, //value type is PluginRect
    PLUGIN_KEY_FRAME_SIZE, //value type is PluginFrameSize
    PLUGIN_KEY_VIDEO_FORMAT, //value type is uint32_t,detail see RenderVideoFormat that is in render_lib.h
    PLUGIN_KEY_VIDEO_PIP, //is pip window, int type of value
    PLUGIN_KEY_VIDEOTUNNEL_ID,//set/get videotunnel instance id when videotunnel plugin is selected
    PLUGIN_KEY_KEEP_LAST_FRAME, //set/get keep last frame when play end ,value type is int, 0 not keep, 1 keep
    PLUGIN_KEY_HIDE_VIDEO, //set/get hide video,it effect immediatialy,value type is int, 0 not hide, 1 hide
    PLUGIN_KEY_FORCE_ASPECT_RATIO, //set/gst force pixel aspect ratio,value type is int, 1 is force,0 is not force
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
} PluginRect;

typedef struct {
    int w;
    int h;
} PluginFrameSize;


/**
 * render plugin interface
 * api sequence:
 * 1.new RenderPlugin
 * 2.plugin->init
 * 3.plugin->setuserData
 * 4.plugin->openDisplay
 * 5.plugin->openWindow
 * 6.plugin->set
 * 7.plugin->displayFrame
 * ......
 * after running ,stop plugin
 * 8.plugin->closeWindow
 * 9.plugin->closeDisplay
 * 10.plugin->release
 * 11.delete RenderPlugin
 *
 */
class RenderPlugin {
  public:
    RenderPlugin() {};
    virtual ~RenderPlugin() {};
    /**
     * @brief init plugin
     *
     */
    virtual void init() = 0;
    /**
     * @brief release plugin resources
     *
     */
    virtual void release() = 0;
    /**
     * @brief Set the User Data object to plugin,it will be used
     * when plugin call back fuction
     *
     * @param userData user data
     * @param callback user callback function
     */
	virtual void setUserData(void *userData, PluginCallback *callback) = 0;
    /**
     * @brief qcquire a dma buffer from compositor
     *
     * @param framewidth video frame width
     * @param frameheight video frame height
     * @return int 0 sucess,other fail
     */
    virtual int acquireDmaBuffer(int framewidth, int frameheight) = 0;
    /**
     * @brief release a dma buffer that is create by acquireDmaBuffer
     *
     * @param dmafd dma buffer fd
     * @return int 0 sucess,other fail
     */
    virtual int releaseDmaBuffer(int dmafd) = 0;
    /**
     * @brief open display from compositor
     *
     * @return int 0 sucess,other fail
     */
    virtual int openDisplay() = 0;
    /**
     * @brief open window from compositor
     *
     * @return int  0 sucess,other fail
     */
    virtual int openWindow() = 0;
    /**
     * @brief render a video frame to compositor
     *
     * @param buffer video frame buffer
     * @param displayTime the frame render realtime
     * @return int 0 sucess,other fail
     */
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime) = 0;
    /**
     * @brief flush buffers that obtain by plugin
     *
     * @return int 0 sucess,other fail
     */
    virtual int flush() = 0;
    /**
     * @brief pause plugin
     *
     * @return int 0 sucess,other fail
     */
    virtual int pause() = 0;
    /**
     * @brief resume plugin
     *
     * @return int 0 sucess,other fail
     */
    virtual int resume() = 0;
    /**
     * @brief close display
     *
     * @return int 0 sucess,other fail
     */
    virtual int closeDisplay() = 0;
    /**
     * @brief close window
     *
     * @return int 0 sucess,other fail
     */
    virtual int closeWindow() = 0;
    /**
     * @brief get property to plugin
     * the value must map the key type,the detail please see
     * enum _PluginKey
     *
     * @param key property key
     * @param value property value
     * @return int 0 sucess,other fail
     */
    virtual int get(int key, void *value) = 0;
    /**
     * @brief set property to plugin
     * the value must map the key type,the detail please see
     * enum _PluginKey
     *
     * @param key property key
     * @param value property value
     * @return int 0 sucess,other fail
     */
    virtual int set(int key, void *value) = 0;
    /**
     * @brief Get the State object about plugin
     * plugin state is definded at enum _PluginState
     *
     * @return int 0 sucess,other fail
     */
	virtual int getState() = 0;
};



#endif /*__RENDER_PLUGIN_H__*/