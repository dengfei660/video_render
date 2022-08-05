#ifndef __RENDER_CORE_H__
#define __RENDER_CORE_H__
#include <mutex>
#include <list>
#include <string>
#include <unordered_map>
#include "render_lib.h"
#include "Thread.h"
#include "render_plugin.h"
#include "Queue.h"

#ifdef  __cplusplus
extern "C" {
#endif
#include "MediaSyncInterface.h"
#ifdef  __cplusplus
}
#endif

/*medisync working mode*/
enum {
    VIDEO_WORK_MODE_NORMAL = 0,             // Normal mode
    VIDEO_WORK_MODE_CACHING_ONLY = 1,       // Only caching data, do not decode. Used in FCC
    VIDEO_WORK_MODE_DECODE_ONLY = 2         // Decode data but do not output
};

class RenderCore : public Tls::Thread{
  public:
    RenderCore(int renderlibId, int logCategory);
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
     * @brief Get the Current media time
     *
     * @param mediaTimeType type of media
     * @param tunit the type of time
     * @param mediaTime the current time
     * @return int 0 sucess,other fail
     */
    int getMediaTimeByType(int mediaTimeType, int tunit, int64_t* mediaTime);
    /**
     * @brief Get the Playback Rate
     *
     * @param scale the playback rate
     * @return int 0 sucess,other fail
     */
    int getPlaybackRate(float *scale);

    /**
     * @brief Set the Playback Rate
     *
     * @param scale
     * @return int 0 sucess,other fail
     */
    int setPlaybackRate(float scale);

    /**
     * @brief Get the Renderlib Id
     *
     * @return int renderlib id
     */
    int getRenderlibId() {
        return mRenderlibId;
    };

    /**
     * @brief Get the Logcategory object of print
     *
     * @return int category of log
     */
    int getLogCategory() {
        return mLogCategory;
    };

    /**
     * @brief queue pts that output from demux to mediasync for a/v sync
     * @param ptsNs the pts that output from demux, the unit is nanosecond
     * @param size the frame size or 0 if unknow
     * @return int 0 success, -1 if failed
     */
    int queueDemuxPts(int64_t ptsUs, uint32_t size);
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
    static void pluginBufferReleaseCallback(void *handle,void *data);
    static void pluginBufferDisplayedCallback(void *handle,void *data);
    static void pluginBufferDropedCallback(void *handle,void *data);
    static void queueFlushCallback(void *userdata,void *data);
  private:
    typedef struct {
        int value;
        bool changed;
    } MediasyncConfig;
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
    /**
     * @brief block the thread until timeout
     *
     * @param timeoutUs block the special value us time
     */
    void waitTimeoutUs(int64_t timeoutUs);

    void setMediasyncPropertys();

    std::string mCompositorName;
    mutable Tls::Mutex   mRenderMutex;
    mutable Tls::Mutex   mLimitMutex;
    Tls::Condition       mLimitCondition;
    Tls::Queue           *mQueue;
    mutable Tls::Mutex   mBufferMgrMutex;

    int mRenderlibId;
    int mLogCategory;
    //mediasync
    void *mMediaSync;
    int mMediaSynInstID;
    bool mMediaSyncInstanceIDSet;
    int mDemuxId;
    int mPcrId;
    int mSyncmode;
    bool mMediaSyncBind;
    bool mMediaSyncAnchor;
    MediasyncConfig mMediaSyncTunnelmode;
    MediasyncConfig mMediasyncHasAudio;
    MediasyncConfig mMediasyncSourceType;
    MediasyncConfig mMediasyncVideoWorkMode;
    MediasyncConfig mMediasyncVideoLatency;
    MediasyncConfig mMediasyncStartThreshold;
    MediasyncConfig mMediasyncPlayerInstanceId;

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

    int mWaitAnchorTimeUs; /*wait anchor mediasync time Us*/
    int64_t mLastInputPTS; /*input frame pts, ns unit*/
    int64_t mLastDisplayPTS; /*display frame pts, ns unit*/
    int64_t mLastDisplayRealtime; /*time got from mediasync to display frame*/
    int64_t mLastDisplaySystemtime; /*the local systemtime displaying last renderbuffer*/
    int mReleaseFrameCnt;
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