#ifndef _WST_SOCKET_CLIENT_H_
#define _WST_SOCKET_CLIENT_H_

#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "Thread.h"
#include "Poll.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define WST_MAX_PLANES (3)
#define AV_SYNC_SESSION_V_MONO 64 //when set it, AV_SYNC_MODE_VIDEO_MONO of sync mode is selected

enum sync_mode {
    AV_SYNC_MODE_VMASTER = 0,
    AV_SYNC_MODE_AMASTER = 1,
    AV_SYNC_MODE_PCR_MASTER = 2,
    AV_SYNC_MODE_IPTV = 3,
    AV_SYNC_MODE_FREE_RUN = 4,
    AV_SYNC_MODE_VIDEO_MONO = 5, /* video render by system mono time */
    AV_SYNC_MODE_MAX
};

typedef enum _WstEventType
{
    WST_REFRESH_RATE = 0,
    WST_BUFFER_RELEASE,
    WST_STATUS,
    WST_UNDERFLOW,
    WST_ZOOM_MODE,
    WST_DEBUG_LEVEL,
} WstEventType;

typedef struct _WstEvent
{
    WstEventType event;
    int param;
    int param1;
    int64_t lparam2;
} WstEVent;

typedef void (*wstOnEvent) (void *userData, WstEVent *event);

typedef struct _WstPlaneInfo
{
    int fd;
    int stride;
    int offset;
    void *start;
    int capacity;
} WstPlaneInfo;

typedef struct _WstBufferInfo
{
    WstPlaneInfo planeInfo[WST_MAX_PLANES];
    int planeCount;
    int bufferId;
    uint32_t pixelFormat;

    int frameWidth;
    int frameHeight;
    int64_t frameTime;
} WstBufferInfo;

typedef struct _WstRect
{
   int x;
   int y;
   int w;
   int h;
} WstRect;

#ifdef  __cplusplus
}
#endif

class WstClientSocket : public Tls::Thread{
  public:
    WstClientSocket(const char *name, void *userData, wstOnEvent onEvent);
    virtual ~WstClientSocket();
    /**
     * @brief send video plane resource id to westeros server
     * 
     * @param resourceId if 0 value,westeros server will select main video plane
     *          if other value,westeros server will select an other video plane
     */
    void sendResourceVideoClientConnection(int resourceId);
    void sendFlushVideoClientConnection();
    void sendPauseVideoClientConnection(bool pause);
    void sendHideVideoClientConnection(bool hide);
    void sendSessionInfoVideoClientConnection(int sessionId, int syncType );
    void sendFrameAdvanceVideoClientConnection();
    void sendRectVideoClientConnection(int videoX, int videoY, int videoWidth, int videoHeight );
    void sendRateVideoClientConnection(int fpsNum, int fpsDenom );
    bool sendFrameVideoClientConnection(WstBufferInfo *wstBufferInfo, WstRect *wstRect);
    void processMessagesVideoClientConnection();
        //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    const char *mName;
    struct sockaddr_un mAddr;
    int mSocketFd;
    int mServerRefreshRate;
    int64_t mServerRefreshPeriod;
    int mZoomMode;
    wstOnEvent mOnEventCallback;
    void *mUserData;
    Tls::Poll *mPoll;
};

#endif /*_WST_SOCKET_CLIENT_H_*/
