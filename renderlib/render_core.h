#ifndef __RENDER_CORE_H__
#define __RENDER_CORE_H__
#include <mutex>
#include <list>
#include <string>
#include <unordered_map>
#include "render_lib.h"
#include "Thread.h"
#include "render_plugin.h"

#ifdef  __cplusplus
extern "C" {
#endif
#include "MediaSyncInterface.h"
#ifdef  __cplusplus
}
#endif

class RenderCore : public Tls::Thread{
  public:
    RenderCore();
    virtual ~RenderCore();
    /**
     * @brief init core resource
     *
     * @param name compositor name
     * @return int 0 sucess,other fail
     */
    int init(char *name);
    void setCallback(RenderCallback *callback);
    void setUserData(void *data);
    /**
     * @brief connect client to compositor server
     * create dispay and window resources
     * @return int 0 sucess,other fail
     */
    int connect();
    /**
     * @brief push a video frame to core queue
     * render core will put video frame to compositor when
     * rendering video frame time reached
     *
     * @param buffer video frame buffer
     * @return int 0 sucess,other fail
     */
    int displayFrame(RenderBuffer *buffer);
    /**
     * @brief Set the Prop object to render core
     *
     * @param property property key
     * @param prop property value
     * @return int 0 sucess,other fail
     */
    int setProp(int property, void *prop);
    /**
     * @brief Get the Prop object from render core
     *
     * @param property property key
     * @param prop property value
     * @return int 0 sucess,other fail
     */
    int getProp(int property, void *prop);
    int flush();
    int pause();
    int resume();
    /**
     * @brief disconnect client from compositor server
     * client will free all resources those accquired from
     * compositor
     *
     * @return int 0 sucess,other fail
     */
    int disconnect();
    /**
     * @brief release render core local resources
     *
     * @return int 0 sucess,other fail
     */
    int release();
    /**
     * @brief alloc a render buffer wrapper(maybe include rawbuffer)
     *
     * @param flag alloc flag
     * @param rawBufferSize the raw buffer size of alloctation
     * @return RenderBuffer*  or NULL if fail
     */
    RenderBuffer *allocRenderBufferWrap(int flag, int rawBufferSize);

    /**
     * @brief release render buffer wrapper
     *
     * @param buffer the renderbuffer wrapper
     */
    void releaseRenderBufferWrap(RenderBuffer *buffer);
    /**
     * @brief accquire dma buffer from compositor plugin
     *
     * @param dmabuf dma buffer params
     * @return int 0 sucess,other fail
     */
    int accquireDmaBuffer(RenderDmaBuffer *dmabuf);
    /**
     * @brief free dma buffer
     *
     * @param dmabuf
     */
    void releaseDmaBuffer(RenderDmaBuffer *dmabuf);
    /**
     * @brief Get the First rendered Audio Pts
     *
     * @param pts the First rendered Audio Pts
     * @return int 0 sucess,other fail
     */
    int getFirstAudioPts(int64_t *pts);
    /**
     * @brief Get the Current rendering Audio Pts
     *
     * @param pts the currint rendering Audio Pts
     * @return int 0 sucess,other fail
     */
    int getCurrentAudioPts(int64_t *pts);
    /**
     * @brief Get the Playback Rate
     *
     * @param scale the playback rate
     * @return int 0 sucess,other fail
     */
    int getPlaybackRate(float *scale);
    //thread func
    void readyToRun();
    virtual bool threadLoop();

    /**
     * @brief add the allocated render buffer
     * to render buffer manager
     * @param buffer render buffer ptr
     */
    void addRenderBuffer(RenderBuffer *buffer);
    /**
     * @brief remote render buffer from render buffer
     * manager
     * @param buffer render buffer ptr
     */
    void removeRenderBuffer(RenderBuffer *buffer);
    /**
     * @brief find the special render buffer from
     * render buffer manager
     * @param buffer render buffer
     * @return RenderBuffer* or NULL if not found
     */
    RenderBuffer *findRenderBuffer(RenderBuffer *buffer);
    /**
     * @brief Set the Render Buffer Free
     *
     * @param buffer render buffer ptr
     */
    void setRenderBufferFree(RenderBuffer *buffer);
    /**
     * @brief Get the Free Render Buffer
     * if not found a free render buffer ,NULL ptr will be returned
     * @return RenderBuffer*  or NULL if fail
     */
    RenderBuffer * getFreeRenderBuffer();

    //static func,callback functions
    static void pluginMsgCallback(void *handle, int msg, void *detail);
    static void pluginErrorCallback(void *handle, int errCode, const char *errDetail);
    static void pluginBufferReleaseCallback(void *handle,void *data);
    static void pluginBufferDisplayedCallback(void *handle,void *data);
  private:
    /**
     * @brief init mediasync when render core received
     * video frame
     *
     * @param allocInstance check if alloc mediasync instance id
     */
    void mediaSyncInit(bool allocInstance);
    void mediaSyncTunnelmodeDisplay();
    void mediaSyncNoTunnelmodeDisplay();
    int64_t nanosecToPTS90K(int64_t nanosec);
    std::string mCompositorName;
    std::list<RenderBuffer *> mRenderBufferQueue;
    mutable Tls::Mutex   mRenderMutex;
    Tls::Condition       mRenderCondition;

    //mediasync
    void *mMediaSync;
    int mMediaSynInstID;
    int mDemuxId;
    int mPcrId;
    int mSyncmode;
    bool mMediaSyncInited;
    bool mMediaSyncConfigureChanged;
    bool mMediaSyncTunnelmode;
    int mMediasyncHasAudio;

    bool mPaused;
    bool mFlushing;
    RenderCallback *mCallback;
    void *mUserData;

    //window size
    bool mWinSizeChanged;
    RenderWindowSize mWinSize;
    RenderVideoFormat mVideoFormat;

    //frame size
    bool mFrameChanged;
    int mFrameWidth;
    int mFrameHeight;

    int mWaitAudioAnchorTimeMs; /*wait audio anchor mediasync time ms*/
    int64_t mLastInputPTS; /*input frame pts, ns unit*/
    int64_t mLastDisplayPTS; /*display frame pts, ns unit*/
    int64_t mLastDisplayRealtime; /*time got from mediasync to display frame*/
    RenderBuffer *mLastRenderBuffer; /*the last display renderbuffer*/
    int64_t mLastDisplaySystemtime; /*the local systemtime displaying last renderbuffer*/
    int mDropFrameCnt; /*the frame cnt that droped by mediasync*/
    int mDisplayedFrameCnt;
    int mInFrameCnt;

    //fps
    int mVideoFPS;
    int mVideoFPS_N; //fps numerator
    int mVideoFPS_D; //fps denominator

    //fps detection,ms
    int mFPSIntervalMs;
    int mFPSDetectCnt;
    int mFPSDetectAcc;
    //plugin
    RenderPlugin *mPlugin;
    bool mIsLimitDisplayFrame;

    //buffer manager
    int mBufferId;
    std::unordered_map<int /*buffer id*/, RenderBuffer *> mAllRenderBufferMap;
    std::unordered_map<int /*buffer id*/, RenderBuffer *> mFreeRenderBufferMap;
};

#endif /*__RENDER_CORE_H__*/