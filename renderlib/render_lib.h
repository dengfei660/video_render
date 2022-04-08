#ifndef __RENDER_LIB_H__
#define __RENDER_LIB_H__
#include <stdint.h>
#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define RENDER_MAX_PLANES 3

/*allocate render buffer flag */
enum _BufferFlag {
    BUFFER_FLAG_NONE       = 0,
    BUFFER_FLAG_ALLOCATE_DMA_BUFFER = 1 << 1,
    BUFFER_FLAG_ALLOCATE_RAW_BUFFER = 1 << 2,
    BUFFER_FLAG_EXTER_DMA_BUFFER    = 1 << 3,
};

typedef struct _RenderRawBuffer {
    void *dataPtr;
    int size;
} RenderRawBuffer;

typedef struct _RenderDmaBuffer {
    int width;
    int height;
    int planeCnt;
    uint32_t handle[RENDER_MAX_PLANES];
    uint32_t stride[RENDER_MAX_PLANES];
    uint32_t offset[RENDER_MAX_PLANES];
    uint32_t size[RENDER_MAX_PLANES];
    int fd[RENDER_MAX_PLANES];
} RenderDmaBuffer;

typedef struct _RenderBuffer {
    int id; //buffer id
    int flag; /*render buffer flag, see  enum _BufferFlag*/
    RenderDmaBuffer dma;
    RenderRawBuffer raw;
    int64_t pts; //time is nano second
    void *priv;
} RenderBuffer;

/*render key*/
enum _RenderKey {
    KEY_WINDOW_SIZE = 300, //set/get the video window size,value type is RenderWindowSize
    KEY_FRAME_SIZE, //set/get frame size,value type is RenderFrameSize
    KEY_VIDEO_FORMAT, //set/get video pixel format, value type is int,detail see RenderVideoFormat enum
    KEY_VIDEO_FPS, //set/get video framerate, value type is int64_t, hight 32bit is numerator,low 32bit is denominator
    KEY_VIDEO_PIP, //set/get pip window flag, value type is int, 0:prime video,1:pip,this flag must set before render_connect
    KEY_FRAME_DROPPED,//get dropped video frames count,value type is int
    KEY_ZORDER, //set/get zorder of video plane,value type is int
    KEY_MEDIASYNC_INSTANCE_ID = 400, //set/get mediasync instance id, value type is int
    KEY_MEDIASYNC_PCR_PID, ///set/get mediasync pcr id ,value type is int
    KEY_MEDIASYNC_DEMUX_ID, //set/get mediasync demux id ,value type is int
    KEY_MEDIASYNC_SYNC_MODE, //set/get mediasync sync mode,value type is int, 0:vmaster,1:amaster,2:pcrmaster
    KEY_MEDIASYNC_TUNNEL_MODE, //set mediasync to use tunnel mode, 0:notunnelmode 1:tunnelmode
    KEY_MEDIASYNC_HAS_AUDIO, //set/get having audio,value type is int,0:not,1:has
    KEY_MEDIASYNC_VIDEOLATENCY, //set/get
    KEY_MEDIASYNC_STARTTHRESHOLD, //set/get
    KEY_MEDIASYNC_VIDEOWORKMODE, //set/get
    KEY_MEDIASYNC_AUDIO_MUTE, //set/get
    KEY_MEDIASYNC_SOURCETYPE, //set/get
    KEY_MEDIASYNC_VIDEOFRAME, //set/get
    //set/get video tunnel instance id when videotunnel plugin be selected,value type is int,this key must set before render_connect
    KEY_VIDEOTUNNEL_ID = 450,
};

/*video display window size
 if will be used by PROP_WINDOW_SIZE prop */
typedef struct _RenderWindowSize {
    int x;
    int y;
    int w;
    int h;
} RenderWindowSize;

/*frame size info
 it will be used by PROP_UPDATE_FRAME_SIZE prop*/
typedef struct _RenderFrameSize {
    int frameWidth;
    int frameHeight;
} RenderFrameSize;

typedef enum _RenderMsgType {
    //frame buffer is released
    MSG_RELEASE_BUFFER   = 100, //the msg type is RenderBuffer
    //frame buffer is displayed
    MSG_DISPLAYED_BUFFER = 101, //the msg type is RenderBuffer

    //render lib connected failed
    MSG_CONNECTED_FAIL   = 200, //the msg type is string
    //render lib disconnected failed
    MSG_DISCONNECTED_FAIL, //the msg type is string
} RenderMsgType;

/**
 * video render send msg callback, user must regist this callback to receive msg from render
 *
 * @param userData the user data registed to video render lib
 * @param type  see enum _RenderMsgType
 * @param msg it is difference according to type value.
 *      when key is MSG_RELEASE_BUFFER, msg is defined by struct _RenderBuffer
 *      when key is MSG_CONNECTED_FAIL or MSG_DISCONNECTED_FAIL, msg is a char string.
 *
 */
typedef void (*onRenderMsgSend)(void *userData , RenderMsgType type, void *msg);
/**
 * video render lib gets values from user
 * @param key the value key,see enum _RenderKey
 * @param value the value return from user
 *
 * @return 0 success, -1 failed
 *
 */
typedef int (*onRenderGet)(void *userData, int key, void *value);

/**
 * a callback function that rend device call
 * this func to send msg to user,please not do
 * time-consuming action,please copy msg context,
 * render lib will free msg buffer when msg_callback called end
 * @param msg the message of sending
 * @return
 */
typedef struct _RenderCallback {
    onRenderMsgSend doMsgSend;
    onRenderGet doGetValue;
} RenderCallback;

/**video format*/
typedef enum {
    VIDEO_FORMAT_UNKNOWN,
    VIDEO_FORMAT_ENCODED,
    VIDEO_FORMAT_I420,
    VIDEO_FORMAT_YV12,
    VIDEO_FORMAT_YUY2,
    VIDEO_FORMAT_UYVY,
    VIDEO_FORMAT_AYUV,
    VIDEO_FORMAT_RGBx,
    VIDEO_FORMAT_BGRx,
    VIDEO_FORMAT_xRGB,
    VIDEO_FORMAT_xBGR,
    VIDEO_FORMAT_RGBA,
    VIDEO_FORMAT_BGRA,
    VIDEO_FORMAT_ARGB,
    VIDEO_FORMAT_ABGR,
    VIDEO_FORMAT_RGB,
    VIDEO_FORMAT_BGR,
    VIDEO_FORMAT_Y41B,
    VIDEO_FORMAT_Y42B,
    VIDEO_FORMAT_YVYU,
    VIDEO_FORMAT_Y444,
    VIDEO_FORMAT_v210,
    VIDEO_FORMAT_v216,
    VIDEO_FORMAT_NV12,
    VIDEO_FORMAT_NV21,
    VIDEO_FORMAT_GRAY8,
    VIDEO_FORMAT_GRAY16_BE,
    VIDEO_FORMAT_GRAY16_LE,
    VIDEO_FORMAT_v308,
    VIDEO_FORMAT_RGB16,
    VIDEO_FORMAT_BGR16,
    VIDEO_FORMAT_RGB15,
    VIDEO_FORMAT_BGR15,
    VIDEO_FORMAT_UYVP,
    VIDEO_FORMAT_A420,
    VIDEO_FORMAT_RGB8P,
    VIDEO_FORMAT_YUV9,
    VIDEO_FORMAT_YVU9,
    VIDEO_FORMAT_IYU1,
    VIDEO_FORMAT_ARGB64,
    VIDEO_FORMAT_AYUV64,
    VIDEO_FORMAT_r210,
    VIDEO_FORMAT_I420_10BE,
    VIDEO_FORMAT_I420_10LE,
    VIDEO_FORMAT_I422_10BE,
    VIDEO_FORMAT_I422_10LE,
    VIDEO_FORMAT_Y444_10BE,
    VIDEO_FORMAT_Y444_10LE,
    VIDEO_FORMAT_GBR,
    VIDEO_FORMAT_GBR_10BE,
    VIDEO_FORMAT_GBR_10LE,
    VIDEO_FORMAT_NV16,
    VIDEO_FORMAT_NV24,
    VIDEO_FORMAT_NV12_64Z32,
    VIDEO_FORMAT_A420_10BE,
    VIDEO_FORMAT_A420_10LE,
    VIDEO_FORMAT_A422_10BE,
    VIDEO_FORMAT_A422_10LE,
    VIDEO_FORMAT_A444_10BE,
    VIDEO_FORMAT_A444_10LE,
    VIDEO_FORMAT_NV61,
    VIDEO_FORMAT_P010_10BE,
    VIDEO_FORMAT_P010_10LE,
    VIDEO_FORMAT_IYU2,
    VIDEO_FORMAT_VYUY,
    VIDEO_FORMAT_GBRA,
    VIDEO_FORMAT_GBRA_10BE,
    VIDEO_FORMAT_GBRA_10LE,
    VIDEO_FORMAT_GBR_12BE,
    VIDEO_FORMAT_GBR_12LE,
    VIDEO_FORMAT_GBRA_12BE,
    VIDEO_FORMAT_GBRA_12LE,
    VIDEO_FORMAT_I420_12BE,
    VIDEO_FORMAT_I420_12LE,
    VIDEO_FORMAT_I422_12BE,
    VIDEO_FORMAT_I422_12LE,
    VIDEO_FORMAT_Y444_12BE,
    VIDEO_FORMAT_Y444_12LE,
    VIDEO_FORMAT_GRAY10_LE32,
    VIDEO_FORMAT_NV12_10LE32,
    VIDEO_FORMAT_NV16_10LE32,
    VIDEO_FORMAT_NV12_10LE40,
    VIDEO_FORMAT_Y210,
    VIDEO_FORMAT_Y410,
    VIDEO_FORMAT_VUYA,
    VIDEO_FORMAT_BGR10A2_LE,
} RenderVideoFormat;

/**
 * open a render lib,render lib will open a compositer with the special
 * render name
 * @param name the render device name
 *   the name value list is:
 *   wayland will open wayland render
 *   videotunnel will open tunnel render
 *
 * @return a handle of render lib , return null if failed
 */
void *render_open(char *name);

/**
 * registe callback to render lib, render device will call
 * these callbacks to send msg to user or get some value from user
 * @param handle a handle of render device that was opened
 * @param callback  callback function struct that render will use
 * @return 0 sucess,-1 fail
 */
void render_set_callback(void *handle, RenderCallback *callback);

/**
 * set user data to render lib
 * @param handle a handle of render lib that was opened
 * @param userdata the set userdata
 * @return 0 sucess,-1 fail
 */
void render_set_user_data(void *handle, void *userdata);

/**
 * connect to render device
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_connect(void *handle);

/*************************************************/

/**
 * display a video frame, the buffer will be obtained by render lib
 * until render lib release it, so please allcating buffer from memory heap
 * @param handle a handle of render device that was opened
 * @param buffer a video buffer will be displayed
 * @return 0 sucess,-1 fail
 */
int render_display_frame(void *handle, RenderBuffer *buffer);

/**
 * set value to render device
 * @param handle a handle of render device that was opened
 * @param key a key of render device
 * @param value the value of key
 * @return 0 sucess,-1 fail
 */
int render_set(void *handle, int key, void *value);

/**
 * get value from render device
 * @param handle a handle of render device that was opened
 * @param key a key of render device
 * @param value the value of key
 * @return 0 sucess,-1 fail
 */
int render_get(void *handle, int key, void *value);

/**
 * flush render lib buffer
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_flush(void *handle);

/**
 * pause display video frame
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_pause(void *handle);

/**
 * resume display video frame
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_resume(void *handle);

/**
 * disconnect to render device
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_disconnect(void *handle);

/**
 * close render device
 * @param handle a handle of render device that was opened
 * @return 0 sucess,-1 fail
 */
int render_close(void *handle);


/**********************tools func for render devices***************************/
/**
 * only alloc a RenderBuffer wrapper from render lib,
 * @param handle a handle of render device that was opened
 * @param flag buffer flag value, defined allocate buffer action, the value see _BufferFlag defined
 * @param rawBufferSize allocated raw buffer size, if only allocate render buffer wrap, the bufferSize can 0
 * @return buffer handler or null if failed
 */
RenderBuffer *render_allocate_render_buffer_wrap(void *handle, int flag, int rawBufferSize);

/**
 * free render buffer that allocated from render lib
 * @param handle a handle of render device that was opened
 * @return
 */
void render_free_render_buffer_wrap(void *handle, RenderBuffer *buffer);

/**
 * accquire dma buffer from render lib
 * @param handle a handle of render device that was opened
 * @param planecnt the dma buffer plane count
 * @param width video width
 * @param height video height
 * @param dmabuffer  output parma,dmabuffer
 * @return 0 success, -1 if failed
 *
*/
int render_accquire_dma_buffer(void *handle, int planecnt, int width, int height, RenderDmaBuffer *dmabuffer);

/**
 * release dma buffer that allocated from render lib
*/
void render_release_dma_buffer(void *handle, RenderDmaBuffer *buffer);


/**
 * @brief get first audio pts from mediasync
 *
 * @param handle a handle of render device that was opened
 * @param pts the first rendered audio pts
 * @return int 0 success, -1 if failed
 */
int render_mediasync_get_first_audio_pts(void *handle, int64_t *pts);

/**
 * @brief get current rendering audio pts from mediasync
 *
 *@param handle a handle of render device that was opened
 * @param pts the current rendering audio pts
 * @return int 0 success, -1 if failed
 */
int render_mediasync_get_current_audio_pts(void *handle, int64_t *pts);

/**
 * @brief get playback rate from mediasync
 *
 * @param handle a handle of render device that was opened
 * @param scale the playback rate(output param)
 * @return int 0 success, -1 if failed
 */
int render_mediasync_get_playback_rate(void *handle, float *scale);

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_LIB_H__*/