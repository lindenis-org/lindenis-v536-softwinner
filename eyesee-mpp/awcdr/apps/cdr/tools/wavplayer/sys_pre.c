
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


#include "sys_pre.h"


#define LOG_NDEBUG 0
#include <utils/plat_log.h>

#include <pthread.h>
#include <stdbool.h>
#include <memory.h>
#include "cdx_list.h"
#include "mm_common.h"


//#define SOUND_CARD  "default:CARD=audiocodec"
#define SOUND_CARD_AUDIOCODEC   "default"
#define SOUND_CARD_SNDHDMI      "hw:1,0"

typedef struct AO_CHANNEL_S {
    AO_CHN mId;
    MM_COMPONENTTYPE *mpComp;
    cdx_sem_t mSemCompCmd;
    MPPCallbackInfo  mCallbackInfo;
    struct list_head mList;
} AO_CHANNEL_S;

typedef enum AI_STATES_E
{
    AI_STATE_INVALID = 0,
    AI_STATE_CONFIGURED,
    AI_STATE_STARTED,
} AI_STATES_E;

typedef enum AO_STATES_E
{
    AO_STATE_INVALID = 0,
    AO_STATE_CONFIGURED,
    AO_STATE_STARTED,
} AO_STATES_E;


typedef struct AudioInputDevice
{
    AI_STATES_E mState;

    AIO_ATTR_S mAttr;
    PCM_CONFIG_S mCfg;
    AUDIO_TRACK_MODE_E mTrackMode;
    pthread_t mThdId;
    volatile BOOL mThdRunning;

    struct list_head mChnList;
    pthread_mutex_t mChnListLock;
} AudioInputDevice;

typedef struct AudioOutputDevice
{
    AO_STATES_E mState;

    AIO_ATTR_S mAttr;
    PCM_CONFIG_S mCfg;
    AUDIO_TRACK_MODE_E mTrackMode;
    pthread_t mThdId;
    volatile BOOL mThdRunning;

    struct list_head mChnList;
    pthread_mutex_t mChnListLock;
} AudioOutputDevice;

typedef struct AudioHwDevice
{
    BOOL mEnableFlag;
    AIO_MIXER_S mMixer;
    AudioInputDevice mCap;
    AudioOutputDevice mPlay;
} AudioHwDevice;


static AudioHwDevice gAudioHwDev[AIO_DEV_MAX_NUM];


// 0-cap; 1-play
static ERRORTYPE Hw_AO_Construct(long vol_val)
{
    int i;
    int err;
    //memset(&gAudioHwDev, 0, sizeof(AudioHwDevice)*AIO_DEV_MAX_NUM);
    for (i = 0; i < AIO_DEV_MAX_NUM; ++i) {
        AudioHwDevice *pDev = &gAudioHwDev[i];
        if (TRUE == pDev->mEnableFlag) {
            alogw("audio_hw has already been constructed!");
            return SUCCESS;
        }
        err = alsaOpenMixer(&pDev->mMixer, SOUND_CARD_AUDIOCODEC, vol_val);
        if (err != 0) {
            printf("AIO device %d open mixer failed!", i);
        }
        pDev->mCap.mState = AI_STATE_INVALID;
        pDev->mPlay.mState = AO_STATE_INVALID;
        pDev->mEnableFlag = TRUE;
    }
    return SUCCESS;
}

static ERRORTYPE Hw_AO_Destruct(void)
{
    int i;
    for (i = 0; i < AIO_DEV_MAX_NUM; ++i) {
        AudioHwDevice *pDev = &gAudioHwDev[i];
        if (FALSE == pDev->mEnableFlag) {
            alogw("audio_hw has already been destructed!");
            return SUCCESS;
        }
        if (pDev->mMixer.handle != NULL) {
            if (AI_STATE_STARTED==pDev->mCap.mState || AO_STATE_STARTED==pDev->mPlay.mState)
                printf("Why AIO still running? CapState:%d, PlayState:%d", pDev->mCap.mState, pDev->mPlay.mState);

            printf("[FUN]:%s [LINE]:%d  alsaCloseMixer !\n", __func__, __LINE__);
            alsaCloseMixer(&pDev->mMixer);
        }
        pDev->mCap.mState = AI_STATE_INVALID;
        pDev->mPlay.mState = AO_STATE_INVALID;
        pDev->mEnableFlag = FALSE;
    }
    return SUCCESS;
}



/**************************************AO_DEV*****************************************/
static ERRORTYPE Hw_AO_Dev_lock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_lock(&gAudioHwDev[AudioDevId].mPlay.mChnListLock);
}

static ERRORTYPE Hw_AO_Dev_unlock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_unlock(&gAudioHwDev[AudioDevId].mPlay.mChnListLock);
}

static ERRORTYPE Hw_AO_searchChannel_l(AUDIO_DEV AudioDevId, AO_CHN AoChn, AO_CHANNEL_S** pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    ERRORTYPE ret = FAILURE;
    AO_CHANNEL_S *pEntry;
    list_for_each_entry(pEntry, &pPlay->mChnList, mList)
    {
        if(pEntry->mId == AoChn) {
            if(pChn) {
                *pChn = pEntry;
            }
            ret = SUCCESS;
            break;
        }
    }
    return ret;
}

static ERRORTYPE Hw_AO_searchChannel(AUDIO_DEV AudioDevId, AO_CHN AoChn, AO_CHANNEL_S** pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    ERRORTYPE ret = FAILURE;
    AO_CHANNEL_S *pEntry;
    pthread_mutex_lock(&pPlay->mChnListLock);
    ret = Hw_AO_searchChannel_l(AudioDevId, AoChn, pChn);
    pthread_mutex_unlock(&pPlay->mChnListLock);
    return ret;
}

static ERRORTYPE Hw_AO_AddChannel_l(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    list_add_tail(&pChn->mList, &pPlay->mChnList);
    struct list_head* pTmp;
    int cnt = 0;
    list_for_each(pTmp, &pPlay->mChnList)
        cnt++;
    updateDebugfsByChnCnt(1, cnt);
    return SUCCESS;
}

static ERRORTYPE Hw_AO_AddChannel(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    pthread_mutex_lock(&pPlay->mChnListLock);
    ERRORTYPE ret = Hw_AO_AddChannel_l(AudioDevId, pChn);
    pthread_mutex_unlock(&pPlay->mChnListLock);
    return ret;
}

static ERRORTYPE Hw_AO_RemoveChannel(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    pthread_mutex_lock(&pPlay->mChnListLock);
    list_del(&pChn->mList);
    struct list_head* pTmp;
    int cnt = 0;
    list_for_each(pTmp, &pPlay->mChnList)
        cnt++;
    updateDebugfsByChnCnt(1, cnt);
    pthread_mutex_unlock(&pPlay->mChnListLock);
    return SUCCESS;
}

static MM_COMPONENTTYPE *Hw_AO_GetChnComp(PARAM_IN MPP_CHN_S *pMppChn)
{
    AO_CHANNEL_S *pChn = NULL;
    if (SUCCESS != Hw_AO_searchChannel(pMppChn->mDevId, pMppChn->mChnId, &pChn)) {
        return NULL;
    }
    return pChn->mpComp;
}

static BOOL Hw_AO_IsDevStarted(AUDIO_DEV AudioDevId)
{
    return (gAudioHwDev[AudioDevId].mPlay.mState == AO_STATE_STARTED);
}

static ERRORTYPE Hw_AO_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr)
{
    if (pstAttr == NULL) {
        printf("pstAttr is NULL!");
        return ERR_AO_ILLEGAL_PARAM;
    }
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    if (pPlay->mState == AO_STATE_CONFIGURED) {
        alogw("Update AoAttr? cur_card:%d -> wanted_card:%d", pPlay->mAttr.mPcmCardId, pstAttr->mPcmCardId);
    } else if (pPlay->mState == AO_STATE_STARTED) {
        alogw("Careful for 2 AoChns at the same time! They must have the same param!");
        return SUCCESS;
    }
    pPlay->mAttr = *pstAttr;
    pPlay->mState = AO_STATE_CONFIGURED;
    return SUCCESS;
}

static ERRORTYPE Hw_AO_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr)
{
    if (pstAttr == NULL) {
        printf("pstAttr is NULL!");
        return ERR_AO_ILLEGAL_PARAM;
    }

    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    if (pPlay->mState == AO_STATE_INVALID) {
        printf("get attr when attr is not set!");
        return ERR_AO_NOT_PERM;
    }

    *pstAttr = pPlay->mAttr;
    return SUCCESS;
}

static ERRORTYPE Hw_AO_ClrPubAttr(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState == AO_STATE_STARTED) {
        printf("please clear attr after AI disable!");
        return ERR_AO_NOT_PERM;
    }
    memset(&pPlay->mAttr, 0, sizeof(AIO_ATTR_S));
    pPlay->mState = AO_STATE_INVALID;
    return SUCCESS;
}

static ERRORTYPE Hw_AO_SetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E enTrackMode)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    pPlay->mTrackMode = enTrackMode;

    return SUCCESS;
}

static ERRORTYPE Hw_AO_GetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E *penTrackMode)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    *penTrackMode = pPlay->mTrackMode;

    return SUCCESS;
}

static ERRORTYPE Hw_AO_Enable(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    int ret;

    printf("[FUN]:%s [LINE]:%d  =====> mState:%d  \n", __func__, __LINE__, pPlay->mState);

    if (pPlay->mState == AO_STATE_INVALID) {
        return ERR_AO_NOT_CONFIG;
    }
    if (pPlay->mState == AO_STATE_STARTED) {
        return SUCCESS;
    }

    pPlay->mCfg.chnCnt = pPlay->mAttr.u32ChnCnt;
    pPlay->mCfg.sampleRate = pPlay->mAttr.enSamplerate;
    if (pPlay->mAttr.enBitwidth == AUDIO_BIT_WIDTH_32) {
        pPlay->mCfg.format = SND_PCM_FORMAT_S32_LE;
    } else if (pPlay->mAttr.enBitwidth == AUDIO_BIT_WIDTH_24) {
        pPlay->mCfg.format = SND_PCM_FORMAT_S24_LE;
    } else if (pPlay->mAttr.enBitwidth == AUDIO_BIT_WIDTH_16) {
        pPlay->mCfg.format = SND_PCM_FORMAT_S16_LE;
    } else if (pPlay->mAttr.enBitwidth == AUDIO_BIT_WIDTH_8) {
        pPlay->mCfg.format = SND_PCM_FORMAT_S8;
    } else {
        pPlay->mCfg.format = SND_PCM_FORMAT_S16_LE;
    }
    pPlay->mCfg.bitsPerSample = (pPlay->mAttr.enBitwidth+1)*8;

    const char *pCardType = (pPlay->mAttr.mPcmCardId==PCM_CARD_TYPE_AUDIOCODEC) ? SOUND_CARD_AUDIOCODEC:SOUND_CARD_SNDHDMI;
    ret = alsaOpenPcm(&pPlay->mCfg, pCardType, 1);
    if (ret != 0) {
        return FAILURE;
    }
    ret = alsaSetPcmParams(&pPlay->mCfg);
    if (ret < 0) {
        goto ERR_SET_PCM_PARAM;
    }

    pthread_mutex_init(&pPlay->mChnListLock, NULL);
    INIT_LIST_HEAD(&pPlay->mChnList);
    //pPlay->mThdRunning = TRUE;
    //pthread_create(&pPlay->mThdId, NULL, Hw_AO_PlayThread, &pPlay);

    pPlay->mState = AO_STATE_STARTED;

    printf("[FUN]:%s [LINE]:%d  =====> alsaOpenPcm and alsaSetPcmParams success!\n", __func__, __LINE__);

    return SUCCESS;

ERR_SET_PCM_PARAM:
    alsaClosePcm(&pPlay->mCfg, 1);  // 1: playback
    return FAILURE;
}

static ERRORTYPE Hw_AO_Disable(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState == AO_STATE_INVALID) {
        return ERR_AO_NOT_CONFIG;
    }
    if (pPlay->mState != AO_STATE_STARTED) {
        return SUCCESS;
    }

    //pPlay->mThdRunning = FALSE;
    //pthread_join(pPlay->mThdId, (void*) &ret);

    if (!list_empty(&pPlay->mChnList)) {
        alogw("When ao_disable, still exist channle in PlayChnList?! list them below:");
        AO_CHANNEL_S *pEntry;
        list_for_each_entry(pEntry, &pPlay->mChnList, mList)
        {
            alogw("AoCardType[%d] AoChn[%d] still run!", pPlay->mAttr.mPcmCardId, pEntry->mId);
        }
        return SUCCESS;
    }

    pthread_mutex_destroy(&pPlay->mChnListLock);

    printf("close pcm! current AoCardType:[%d]\n", pPlay->mAttr.mPcmCardId);
    alsaClosePcm(&pPlay->mCfg, 1);  // 1: playback
    pPlay->mState = AO_STATE_CONFIGURED;

    return SUCCESS;
}

static ERRORTYPE Hw_AO_SetVolume(AUDIO_DEV AudioDevId, int s32VolumeDb)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerSetVolume(pMixer, 1, s32VolumeDb);
}

static ERRORTYPE Hw_AO_GetVolume(AUDIO_DEV AudioDevId, int *ps32VolumeDb)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerGetVolume(pMixer, 1, (long*)ps32VolumeDb);
}

static ERRORTYPE Hw_AO_SetMute(AUDIO_DEV AudioDevId, BOOL bEnable, AUDIO_FADE_S *pstFade)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerSetMute(pMixer, 1, (int)bEnable);
}

static ERRORTYPE Hw_AO_GetMute(AUDIO_DEV AudioDevId, BOOL *pbEnable, AUDIO_FADE_S *pstFade)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    int MainVolVal;
    alsaMixerGetMute(pMixer, 1, &MainVolVal);
    if (MainVolVal > 0)
        *pbEnable = FALSE;
    else
        *pbEnable = TRUE;

    return SUCCESS;
}

static ERRORTYPE Hw_AO_FillPcmRingBuf(AUDIO_DEV AudioDevId, void* pData, int Len)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    size_t frame_cnt = Len / (pPlay->mCfg.bitsPerFrame >> 3);
    ssize_t ret;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    //printf("[FUN]:%s [LINE]:%d  =====> AudioDevId:%d  Len:%d frame_cnt:%d  \n", __func__, __LINE__, AudioDevId, Len, frame_cnt);
    ret = alsaWritePcm(&pPlay->mCfg, pData, frame_cnt);
    if (ret != frame_cnt) {
        printf("alsaWritePcm error!");
        return FAILURE;
    }

    return SUCCESS;
}

static ERRORTYPE Hw_AO_DrainPcmRingBuf(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    alsaDrainPcm(&pPlay->mCfg);

    return SUCCESS;
}

static ERRORTYPE Hw_AO_FeedPcmData(AUDIO_DEV AudioDevId, AUDIO_FRAME_S *pFrm)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    size_t frame_cnt = pFrm->mLen / (pPlay->mCfg.bitsPerFrame >> 3);
    ssize_t ret;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    ret = alsaWritePcm(&pPlay->mCfg, pFrm->mpAddr, frame_cnt);
    if (ret != frame_cnt) {
        printf("alsaWritePcm error!");
        return FAILURE;
    }

    return SUCCESS;
}

static ERRORTYPE Hw_AO_GetPcmConfig(AUDIO_DEV AudioDevId, PCM_CONFIG_S **ppCfg)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    *ppCfg = &pPlay->mCfg;
    return SUCCESS;
}

static ERRORTYPE Hw_AO_GetAIOAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S **ppAttr)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    *ppAttr = &pPlay->mAttr;
    return SUCCESS;
}

static AO_CHANNEL_S *AOChannel_Construct(void)
{
    AO_CHANNEL_S *pChn = (AO_CHANNEL_S*)malloc(sizeof(AO_CHANNEL_S));
    if (pChn == NULL) {
        printf("alloc AO_CHANNEL_S error[%s]!", strerror(errno));
        return NULL;
    }
    memset(pChn, 0, sizeof(AO_CHANNEL_S));
    cdx_sem_init(&pChn->mSemCompCmd, 0);
    return pChn;
}

static void AOChannel_Destruct(AO_CHANNEL_S *pChn)
{
    if (pChn != NULL) {
        if (pChn->mpComp != NULL) {
            printf("fatal error! AO component need free before!");
            COMP_FreeHandle(pChn->mpComp);
            pChn->mpComp = NULL;
        }
    }
    cdx_sem_deinit(&pChn->mSemCompCmd);
    free(pChn);
}

static ERRORTYPE AOChannel_EventHandler(
     PARAM_IN COMP_HANDLETYPE hComponent,
     PARAM_IN void *pAppData,
     PARAM_IN COMP_EVENTTYPE eEvent,
     PARAM_IN unsigned int nData1,
     PARAM_IN unsigned int nData2,
     PARAM_IN void *pEventData)
{
    AO_CHANNEL_S *pChn = (AO_CHANNEL_S*)pAppData;

    switch(eEvent)
    {
        case COMP_EventCmdComplete:
        {
            if(COMP_CommandStateSet == nData1) {
                //alogv("audio device EventCmdComplete, current StateSet[%d]", nData2);
                cdx_sem_up(&pChn->mSemCompCmd);
                break;
            } else {
                alogw("Low probability! what command[0x%x]?", nData1);
                break;
            }
        }
        case COMP_EventBufferFlag:
        {
            MPP_CHN_S ChannelInfo;
            ChannelInfo.mModId = MOD_ID_AO;
            ChannelInfo.mDevId = 0;
            ChannelInfo.mChnId = pChn->mId;
            CHECK_MPP_CALLBACK(pChn->mCallbackInfo.callback);
            pChn->mCallbackInfo.callback(pChn->mCallbackInfo.cookie, &ChannelInfo, MPP_EVENT_NOTIFY_EOF, NULL);
            break;
        }
        default:
            printf("fatal error! unknown event[0x%x]", eEvent);
            break;
    }
    return SUCCESS;
}

static ERRORTYPE AOChannel_EmptyBufferDone(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN void* pAppData,
        PARAM_IN COMP_BUFFERHEADERTYPE* pBuffer)
{
    AO_CHANNEL_S *pChn = (AO_CHANNEL_S*)pAppData;
    AUDIO_FRAME_S *pAFrame = (AUDIO_FRAME_S*)pBuffer->pOutputPortPrivate;
    MPP_CHN_S ChannelInfo;
    ChannelInfo.mModId = MOD_ID_AO;
    ChannelInfo.mDevId = 0;
    ChannelInfo.mChnId = pChn->mId;
    CHECK_MPP_CALLBACK(pChn->mCallbackInfo.callback);
    pChn->mCallbackInfo.callback(pChn->mCallbackInfo.cookie, &ChannelInfo, MPP_EVENT_RELEASE_AUDIO_BUFFER, (void*)pAFrame);
    return SUCCESS;
}

static COMP_CALLBACKTYPE AOChannel_Callback = {
    .EventHandler = AOChannel_EventHandler,
    .EmptyBufferDone = AOChannel_EmptyBufferDone,
    .FillBufferDone = NULL,
};


ERRORTYPE MPI_AO_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_SetPubAttr(AudioDevId, pstAttr);
}

ERRORTYPE MPI_AO_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_GetPubAttr(AudioDevId, pstAttr);
}

ERRORTYPE MPI_AO_ClrPubAttr(AUDIO_DEV AudioDevId)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_ClrPubAttr(AudioDevId);
}

ERRORTYPE MPI_AO_SetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E enTrackMode)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_SetTrackMode(AudioDevId, enTrackMode);
}

ERRORTYPE MPI_AO_GetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E *penTrackMode)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_GetTrackMode(AudioDevId, penTrackMode);
}

ERRORTYPE MPI_AO_Enable(AUDIO_DEV AudioDevId)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_Enable(AudioDevId);
}

ERRORTYPE MPI_AO_Disable(AUDIO_DEV AudioDevId)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_Disable(AudioDevId);
}

ERRORTYPE MPI_AO_SetVolume(AUDIO_DEV AudioDevId, int s32VolumeDb)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_SetVolume(AudioDevId, s32VolumeDb);
}

ERRORTYPE MPI_AO_GetVolume(AUDIO_DEV AudioDevId, int *ps32VolumeDb)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_GetVolume(AudioDevId, ps32VolumeDb);
}

ERRORTYPE MPI_AO_SetMute(AUDIO_DEV AudioDevId, BOOL bEnable, AUDIO_FADE_S *pstFade)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_SetMute(AudioDevId, bEnable, pstFade);
}

ERRORTYPE MPI_AO_GetMute(AUDIO_DEV AudioDevId, BOOL *pbEnable, AUDIO_FADE_S *pstFade)
{
    CHECK_AO_DEV_ID(AudioDevId);
    return Hw_AO_GetMute(AudioDevId, pbEnable, pstFade);
}


ERRORTYPE MPI_AO_EnableChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;
    ERRORTYPE ret = SUCCESS;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        ret = MPI_AO_Enable(AudioDevId);
        if (ret != SUCCESS) {
            printf("enable AoDev failed!\n");
            return ERR_AO_NOT_ENABLED;
        }
    }
    Hw_AO_Dev_lock(AudioDevId);
    if (SUCCESS == Hw_AO_searchChannel_l(AudioDevId, AoChn, &pChn)) {
        Hw_AO_Dev_unlock(AudioDevId);
        return ERR_AO_EXIST;
    }
    pChn = AOChannel_Construct();
    pChn->mId = AoChn;

    //create AO Component here...
    ERRORTYPE eRet = SUCCESS;
    MPP_CHN_S ChannelInfo;
    ChannelInfo.mModId = MOD_ID_AO;
    ChannelInfo.mDevId = AudioDevId;
    ChannelInfo.mChnId = AoChn;

    Hw_AO_AddChannel_l(AudioDevId, pChn);
    Hw_AO_Dev_unlock(AudioDevId);
    return ret;
}

ERRORTYPE MPI_AO_DisableChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;
    ERRORTYPE ret;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    ERRORTYPE eRet;
    Hw_AO_RemoveChannel(AudioDevId, pChn);
    pChn->mpComp = NULL;
    AOChannel_Destruct(pChn);
    MPI_AO_Disable(AudioDevId);
    ret = SUCCESS;

    return ret;
}

ERRORTYPE MPI_AO_RegisterCallback(AUDIO_DEV AudioDevId, AO_CHN AoChn, MPPCallbackInfo *pCallback)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }
    pChn->mCallbackInfo.callback = pCallback->callback;
    pChn->mCallbackInfo.cookie = pCallback->cookie;
    return SUCCESS;
}

ERRORTYPE MPI_AO_StartChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;
    ERRORTYPE ret = SUCCESS;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return ret;
}

ERRORTYPE MPI_AO_StopChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;
    ERRORTYPE ret = SUCCESS;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return ret;
}

ERRORTYPE MPI_AO_SendFrame(AUDIO_DEV AudioDevId, AO_CHN AoChn, AUDIO_FRAME_S *pstAFrame, int s32MilliSec)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;
    ERRORTYPE ret;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    COMP_BUFFERHEADERTYPE compBuffer;
    compBuffer.pOutputPortPrivate = (void*)pstAFrame;
    ret = COMP_EmptyThisBuffer(pChn->mpComp, &compBuffer);
    return ret;
}

ERRORTYPE MPI_AO_EnableReSmp(AUDIO_DEV AudioDevId, AO_CHN AoChn, AUDIO_SAMPLE_RATE_E enInSampleRate)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorAIOReSmpEnable, &enInSampleRate);
}

ERRORTYPE MPI_AO_DisableReSmp(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorAIOReSmpDisable, NULL);
}
#if 0
ERRORTYPE MPI_AO_ClearChnBuf(AUDIO_DEV AudioDevId ,AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorAOClearChnBuffer, NULL);
}
#endif
ERRORTYPE MPI_AO_QueryChnStat(AUDIO_DEV AudioDevId ,AO_CHN AoChn, AO_CHN_STATE_S *pstStatus)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return pChn->mpComp->GetConfig(pChn->mpComp, COMP_IndexVendorAOQueryChnStat, pstStatus);
}

ERRORTYPE MPI_AO_PauseChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }
    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    int ret;
    int eRet;
    COMP_STATETYPE nCompState;
    eRet = pChn->mpComp->GetState(pChn->mpComp, &nCompState);
    if(COMP_StateExecuting == nCompState)
    {
        eRet = pChn->mpComp->SendCommand(pChn->mpComp, COMP_CommandStateSet, COMP_StatePause, NULL);
        if(eRet != SUCCESS)
        {
            printf("fatal error! Send command statePause fail!");
        }
        cdx_sem_down(&pChn->mSemCompCmd);
        ret = SUCCESS;
    }
    else if(COMP_StatePause == nCompState)
    {
        printf("AOChannel[%d] already statePause.", AoChn);
        ret = SUCCESS;
    }
    else if(COMP_StateIdle == nCompState)
    {
        printf("AOChannel[%d] stateIdle, can't turn to statePause!", AoChn);
        ret = ERR_AO_INCORRECT_STATE_OPERATION;
    }
    else
    {
        printf("fatal error! check AoChannel[%d] State[0x%x]!", AoChn, nCompState);
        ret = ERR_AO_INCORRECT_STATE_OPERATION;
    }
    return ret;
}

ERRORTYPE MPI_AO_ResumeChn(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    int ret;
    int eRet;
    COMP_STATETYPE nCompState;
    eRet = pChn->mpComp->GetState(pChn->mpComp, &nCompState);
    if(COMP_StatePause == nCompState)
    {
        eRet = pChn->mpComp->SendCommand(pChn->mpComp, COMP_CommandStateSet, COMP_StateExecuting, NULL);
        if(eRet != SUCCESS)
        {
            printf("fatal error! Send command statePause fail!");
        }
        cdx_sem_down(&pChn->mSemCompCmd);
        ret = SUCCESS;
    }
    else if(COMP_StateExecuting == nCompState)
    {
        printf("AOChannel[%d] already stateExecuting.", AoChn);
        ret = SUCCESS;
    }
    else if(COMP_StateIdle == nCompState)
    {
        printf("AOChannel[%d] stateIdle, can't turn to stateExecuting!", AoChn);
        ret = ERR_AO_INCORRECT_STATE_OPERATION;
    }
    else
    {
        printf("fatal error! check AoChannel[%d] State[0x%x]!", AoChn, nCompState);
        ret = ERR_AO_INCORRECT_STATE_OPERATION;
    }
    return ret;
}

ERRORTYPE MPI_AO_Seek(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }
    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    int ret;
    int eRet;
    COMP_STATETYPE nCompState;
    eRet = pChn->mpComp->GetState(pChn->mpComp, &nCompState);
    if(COMP_StateIdle == nCompState || COMP_StateExecuting == nCompState || COMP_StatePause == nCompState)
    {
        ret = pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorSeekToPosition, NULL);
    }
    else
    {
        printf("fatal error! can't seek in AOChannel[%d] State[0x%x]!", AoChn, nCompState);
        ret = ERR_AO_INCORRECT_STATE_OPERATION;
    }
    return ret;
}


ERRORTYPE MPI_AO_SetStreamEof(AUDIO_DEV AudioDevId, AO_CHN AoChn, BOOL bEofFlag, BOOL bDrainFlag)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    ERRORTYPE ret;
    COMP_STATETYPE nState;
    ret = pChn->mpComp->GetState(pChn->mpComp, &nState);
    if(COMP_StateExecuting != nState && COMP_StatePause != nState && COMP_StateIdle != nState)
    {
       printf("wrong state[0x%x], return!", nState);
       return ERR_AO_NOT_PERM;
    }
    if(bEofFlag)
    {
        ret = pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorSetStreamEof, &bDrainFlag);
    }
    else
    {
        ret = pChn->mpComp->SetConfig(pChn->mpComp, COMP_IndexVendorClearStreamEof, NULL);
    }
    return ret;
}

ERRORTYPE MPI_AO_SaveFile(AUDIO_DEV AudioDevId, AO_CHN AoChn, AUDIO_SAVE_FILE_INFO_S *pstSaveFileInfo)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return SUCCESS;
}

ERRORTYPE MPI_AO_QueryFileStatus(AUDIO_DEV AudioDevId, AO_CHN AoChn, AUDIO_SAVE_FILE_INFO_S *pstSaveFileInfo)
{
    CHECK_AO_DEV_ID(AudioDevId);
    CHECK_AO_CHN_ID(AoChn);
    AO_CHANNEL_S *pChn = NULL;

    if (!Hw_AO_IsDevStarted(AudioDevId)) {
        return ERR_AO_NOT_ENABLED;
    }

    if (SUCCESS != Hw_AO_searchChannel(AudioDevId, AoChn, &pChn)) {
        return ERR_AO_UNEXIST;
    }

    return SUCCESS;
}



static int ParseHeader(FILE *file, struct WaveHeader *pheader)
{
    int ret = 0;

    if (file == NULL) {
        printf("file is NULL\n");
        return -1;
    }

    fread((char *)pheader, 1, sizeof(struct WaveHeader), file);
    if ((ret=strncmp("WAVE", (char*)&(pheader->riff_fmt), 4))) {
    
        char *ptr = (char*)&pheader->riff_fmt;
        printf("audio file is not wav file! exit Parser!(%d)(%s)(%c%c%c%c) \n",
                ret, "WAVE", ptr[0], ptr[1], ptr[2], ptr[3]);
        return -1;
    }

    return 0;
}


static int GetPcmParam(FILE *file, struct PcmWaveParam *param)
{
    struct WaveHeader header;

    if (file == NULL) {
        printf("file is NULL\n");
        return -1;
    }

    if (ParseHeader(file, &header) < 0) {
        printf("parse wav header failed\n");
        return -1;
    }

    param->trackCnt   = header.num_chn;
    param->bitWidth   = header.bits_per_sample;
    param->sampleRate = header.sample_rate;

    printf("audio file format(%d, %d, %d)\n", param->trackCnt, param->bitWidth, param->sampleRate);

    return 0;
}


static int GetPcmDataSize(FILE *file)
{
    if (file == NULL) {
        printf("file is NULL\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    return ftell(file) - sizeof(struct WaveHeader);
}


static int GetPcmData(FILE *file, void *pcm_buf, int pcm_size)
{
    if (file == NULL) {
        printf("file is NULL\n");
        return -1;
    }

    if (pcm_buf == NULL) {
        printf("pcm_buf is NULL\n");
        return -1;
    }

    fseek(file, sizeof(struct WaveHeader), SEEK_SET);

    return fread(pcm_buf, 1, pcm_size, file);
}


static int AOChannelInit(AUDIO_DEV AudioDevId, const struct PcmWaveParam *param, long vol_val)
{
    int ret;

    printf("[FUN]:%s [LINE]:%d  =====>  param->aoCardType:%d \n", __func__, __LINE__, param->aoCardType);

    Hw_AO_Construct(vol_val);

#if 1
    AIO_ATTR_S mAIOAttr;
    mAIOAttr.u32ChnCnt = param->trackCnt;
    mAIOAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)param->sampleRate;
    mAIOAttr.enBitwidth = (AUDIO_BIT_WIDTH_E)(param->bitWidth / 8 - 1);
    mAIOAttr.mPcmCardId = (PCM_CARD_TYPE_E)param->aoCardType;

    printf("AIO_ATTR_S: [%d, %d, %d]\n",
            mAIOAttr.u32ChnCnt, mAIOAttr.enSamplerate, mAIOAttr.enBitwidth);

    ret = MPI_AO_SetPubAttr(AudioDevId, &mAIOAttr);
    if (SUCCESS != ret)
    {
        printf("Do MPI_AO_SetPubAttr ao_dev:%d fail! ret:0x%x\n", AudioDevId, ret);
        return -1;
    }

    ret = MPI_AO_EnableChn(AudioDevId, AO_CHN_0);
    if(SUCCESS == ret)
    {
        printf("create ao channel[%d] success!\n", AO_CHN_0);
    }
    else if (ERR_AO_EXIST == ret)
    {
        printf("ao channel[%d] exist, find next!\n", AO_CHN_0);
        return -1;
    }
    else if(ERR_AO_NOT_ENABLED == ret)
    {
        printf("audio_hw_ao not started!\n");
        return -1;
    }
    else
    {
        printf("create ao channel[%d] fail! ret[0x%x]!\n", AO_CHN_0, ret);
        return -1;
    }

    MPPCallbackInfo cbInfo;
    //cbInfo.cookie = this;
    //cbInfo.callback = (MPPCallbackFuncType)&MPPAOCallback;
    MPI_AO_RegisterCallback(AudioDevId, AO_CHN_0, &cbInfo);
    MPI_AO_StartChn(AudioDevId, AO_CHN_0);
#endif

    return 0;
}


static void AOChannelDestroy(AUDIO_DEV AudioDevId)
{
    int ret = 0;

    ret = MPI_AO_DisableChn(AudioDevId, AO_CHN_0);
    if (SUCCESS != ret)
    {
        printf("Do AW_MPI_AO_DisableChn ao_chn:%d fail! ret:0x%x\n", AO_CHN_0, ret);
    }

    Hw_AO_Destruct();
}


static int PlayPcm(const struct PcmWaveData *pcm_data)
{
    int ret        = 0;
    int rdId       = 0;
    int chunk_cnt_ = 0;
    int chunk_size = 0;

    if (pcm_data->buffer == NULL || pcm_data->size == 0) {
        printf("pcm buffer is NULL\n");
        return -1;
    }

    chunk_size = pcm_data->param.trackCnt * 2 * 1024;
    chunk_cnt_ = (pcm_data->size + chunk_size - 1) / chunk_size;

    // db_info("pcmSize:%d, chunkSize: %d, chunkTotalCnt:%d", pcm_data.size, chunk_size, chunk_cnt_);

    AUDIO_FRAME_S frmInfo;
    bzero(&frmInfo, sizeof(frmInfo));

    while (1) {
        frmInfo.mpAddr = pcm_data->buffer + chunk_size * rdId++;
        if (rdId != chunk_cnt_) {
            frmInfo.mLen = chunk_size;
        } else {
            frmInfo.mLen = chunk_size - (chunk_cnt_ * chunk_size - pcm_data->size);
        }

        ret = Hw_AO_FillPcmRingBuf(AO_DEV_0, frmInfo.mpAddr, frmInfo.mLen);
        if (ret) {
            printf("[FUN]:%s [LINE]:%d  Do  Hw_AO_FillPcmRingBuf  fail ret:%d!\n", __func__, __LINE__, ret);
        }

        //printf("[FUN]:%s [LINE]:%d  Do  frmInfo.mpAddr:%p  mLen:%d!\n", __func__, __LINE__, frmInfo.mpAddr, frmInfo.mLen);

        if (rdId == chunk_cnt_) {
            usleep(10);
            printf("[FUN]:%s [LINE]:%d  ==============PlayPcm end==========\n", __func__, __LINE__);
            /*ret = Hw_AO_DrainPcmRingBuf(AO_DEV_0);
            if (ret) {
                printf("[FUN]:%s [LINE]:%d  Do  Hw_AO_DrainPcmRingBuf  fail ret:%d!\n", __func__, __LINE__, ret);
            }*/
            rdId = 0;
            break;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int ret = 0;
    FILE *file = NULL;
    long vol_val = 168;
    AUDIO_DEV AudioDevId = AO_DEV_0;
    struct PcmWaveData pcm_data;

    if (argc > 1) {
        vol_val = atol(argv[1]);
        if (vol_val < 10 || vol_val > 255) {
            vol_val = 168;
        }
    }

    printf("[FUN]:%s [LINE]:%d  =====> argc:%d  vol_val:%ld \n", __func__, __LINE__, argc, vol_val);

    /*  Parse wav file and config PCM param */
    file = fopen(AUDIO_STARTUP_FILE, "r");
    if (NULL == file) {
        printf("fopen %s fail! errno:%d  %s\n", AUDIO_STARTUP_FILE, errno, strerror(errno));
        return -1;
    }

    ret = GetPcmParam(file, &pcm_data.param);
    if (ret < 0) {
        printf("[FUN]:%s [LINE]:%d  Do GetPcmParam fail!\n", __func__, __LINE__);
       goto MAIN_ERROR;
    }
    pcm_data.param.aoCardType = 0;
    pcm_data.size = GetPcmDataSize(file);
    printf("[FUN]:%s [LINE]:%d  =====>  pcm_data.size:%d \n", __func__, __LINE__, pcm_data.size);
    if (pcm_data.size < 10) {
        printf("[FUN]:%s [LINE]:%d  Do GetPcmDataSize fail!\n", __func__, __LINE__);
       goto MAIN_ERROR;
    }else if (pcm_data.size < 8192) {
       pcm_data.size = 8192;
    }

    pcm_data.buffer = malloc(pcm_data.size);
    if (pcm_data.buffer == NULL) {
       printf("malloc pcm data buffer failed, alloc size: %d \n", pcm_data.size);
       goto MAIN_ERROR;
    }
    memset(pcm_data.buffer, 0, pcm_data.size);
    ret = GetPcmData(file, pcm_data.buffer, pcm_data.size);
    if (ret < 0) {
        printf("[FUN]:%s [LINE]:%d  Do GetPcmData fail!\n", __func__, __LINE__);
       goto MAIN_ERROR;
    }
    fclose(file);
    file = NULL;

    /* Init and Config AO device for alase */
    ret = AOChannelInit(AudioDevId, &pcm_data.param, vol_val);
    if (ret < 0) {
        printf("[FUN]:%s [LINE]:%d  Do AOChannelInit fail!\n", __func__, __LINE__);
       goto MAIN_ERROR;
    }

    /* Play starup.wav file by alase */
    PlayPcm(&pcm_data);

    /* End */
    AOChannelDestroy(AudioDevId);

MAIN_ERROR:
    if (file)
        fclose(file);

    return 0;
}

