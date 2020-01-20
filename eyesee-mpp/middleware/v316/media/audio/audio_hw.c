/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : audio_hw.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/05/25
  Last Modified :
  Description   : mpi functions implement
  Function List :
  History       :
******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "audio_hw"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <memory.h>
#include <SystemBase.h>

#include <audio_hw.h>

#include <ConfigOption.h>

#include "cdx_list.h"

//#define SOUND_CARD  "default:CARD=audiocodec"
#define SOUND_CARD_AUDIOCODEC   "default"
#define SOUND_CARD_SNDHDMI      "hw:1,0"


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
    pthread_mutex_t mApiCallLock;   // to protect the api call,when used in two thread asynchronously.
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
ERRORTYPE audioHw_Construct(void)
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
        err = alsaOpenMixer(&pDev->mMixer, SOUND_CARD_AUDIOCODEC);
        if (err != 0) {
            aloge("AIO device %d open mixer failed!", i);
        }
        pDev->mCap.mState = AI_STATE_INVALID;
        pDev->mPlay.mState = AO_STATE_INVALID;
        INIT_LIST_HEAD(&pDev->mCap.mChnList);
        INIT_LIST_HEAD(&pDev->mPlay.mChnList); 
        pthread_mutex_init(&pDev->mCap.mApiCallLock, NULL);
        pDev->mEnableFlag = TRUE;
    }
    return SUCCESS;
}

ERRORTYPE audioHw_Destruct(void)
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
                aloge("Why AIO still running? CapState:%d, PlayState:%d", pDev->mCap.mState, pDev->mPlay.mState);
            alsaCloseMixer(&pDev->mMixer);
        }
        
        pthread_mutex_destroy(&pDev->mCap.mApiCallLock);
        pDev->mCap.mState = AI_STATE_INVALID;
        pDev->mPlay.mState = AO_STATE_INVALID;
        pDev->mEnableFlag = FALSE;
    }
    return SUCCESS;
}


/**************************************AI_DEV*****************************************/
ERRORTYPE audioHw_AI_Dev_lock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_lock(&gAudioHwDev[AudioDevId].mCap.mChnListLock);
}

ERRORTYPE audioHw_AI_Dev_unlock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_unlock(&gAudioHwDev[AudioDevId].mCap.mChnListLock);
}

ERRORTYPE audioHw_AI_searchChannel_l(AUDIO_DEV AudioDevId, AI_CHN AiChn, AI_CHANNEL_S** pChn)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    ERRORTYPE ret = FAILURE;
    AI_CHANNEL_S *pEntry;
    list_for_each_entry(pEntry, &pCap->mChnList, mList)
    {
        if(pEntry->mId == AiChn) {
            if(pChn) {
                *pChn = pEntry;
            }
            ret = SUCCESS;
            break;
        }
    }
    return ret;
}

ERRORTYPE audioHw_AI_searchChannel(AUDIO_DEV AudioDevId, AI_CHN AiChn, AI_CHANNEL_S** pChn)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    ERRORTYPE ret = FAILURE;
    AI_CHANNEL_S *pEntry;
    pthread_mutex_lock(&pCap->mChnListLock);
    ret = audioHw_AI_searchChannel_l(AudioDevId, AiChn, pChn);
    pthread_mutex_unlock(&pCap->mChnListLock);
    return ret;
}

ERRORTYPE audioHw_AI_AddChannel_l(AUDIO_DEV AudioDevId, AI_CHANNEL_S* pChn)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    list_add_tail(&pChn->mList, &pCap->mChnList);
    struct list_head* pTmp;
    int cnt = 0;
    list_for_each(pTmp, &pCap->mChnList)
        cnt++;
    updateDebugfsByChnCnt(0, cnt);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_AddChannel(AUDIO_DEV AudioDevId, AI_CHANNEL_S* pChn)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    pthread_mutex_lock(&pCap->mChnListLock);
    ERRORTYPE ret = audioHw_AI_AddChannel_l(AudioDevId, pChn);
    pthread_mutex_unlock(&pCap->mChnListLock);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_RemoveChannel(AUDIO_DEV AudioDevId, AI_CHANNEL_S* pChn)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    pthread_mutex_lock(&pCap->mChnListLock);
    list_del(&pChn->mList);
    struct list_head* pTmp;
    int cnt = 0;
    list_for_each(pTmp, &pCap->mChnList)
        cnt++;
    updateDebugfsByChnCnt(0, cnt);
    pthread_mutex_unlock(&pCap->mChnListLock);
    return SUCCESS;
}

MM_COMPONENTTYPE *audioHw_AI_GetChnComp(PARAM_IN MPP_CHN_S *pMppChn)
{
    AI_CHANNEL_S *pChn = NULL;
    if (SUCCESS != audioHw_AI_searchChannel(pMppChn->mDevId, pMppChn->mChnId, &pChn)) {
        return NULL;
    }
    return pChn->mpComp;
}

BOOL audioHw_AI_IsDevStarted(AUDIO_DEV AudioDevId)
{
    return (gAudioHwDev[AudioDevId].mCap.mState == AI_STATE_STARTED);
}

static void *audioHw_AI_CapThread(void *pThreadData)
{
    AudioInputDevice *pCap = (AudioInputDevice*)pThreadData;
    char *pCapBuf = (char*)malloc(pCap->mCfg.chunkBytes);
    if (pCapBuf == NULL) {
        aloge("Failed to alloc %d bytes(%s)", pCap->mCfg.chunkBytes, strerror(errno));
    }
    int ret = SUCCESS;

    while (pCap->mThdRunning) {
        if (alsaReadPcm(&pCap->mCfg, pCapBuf, pCap->mCfg.chunkSize) != pCap->mCfg.chunkSize) {
            aloge("fatal error! fail to read pcm %d bytes-%d", pCap->mCfg.chunkBytes,pCap->mState);
            usleep(10*1000);
            continue;
        }

        BOOL new_a_frm = TRUE;
        AI_CHANNEL_S *pEntry;
        pthread_mutex_lock(&pCap->mChnListLock);
        if (list_empty(&pCap->mChnList))
        {
            pthread_mutex_unlock(&pCap->mChnListLock);
            continue;
        }
        list_for_each_entry(pEntry, &pCap->mChnList, mList)
        {
            AUDIO_FRAME_S frame;
            COMP_BUFFERHEADERTYPE bufferHeader;
            bufferHeader.nOutputPortIndex = AI_CHN_PORT_INDEX_CAP_IN;
            bufferHeader.pOutputPortPrivate = &frame;
            frame.mLen = pCap->mCfg.chunkBytes;
            frame.mBitwidth = pCap->mAttr.enBitwidth;
            frame.mSoundmode = pCap->mAttr.enSoundmode;
            frame.mpAddr = pCapBuf;

            if(TRUE == new_a_frm)
            {
                int64_t tm1 = CDX_GetSysTimeUsMonotonic();
                frame.tmp_pts = tm1 - pCap->mCfg.chunkSize*1000/pCap->mCfg.sampleRate*1000;

                new_a_frm = FALSE;
            }
            pEntry->mpComp->EmptyThisBuffer(pEntry->mpComp, &bufferHeader);
        }
        pthread_mutex_unlock(&pCap->mChnListLock);
    }

    alogd("AI_CapThread exit!");
    free(pCapBuf);
    return NULL;
}

ERRORTYPE audioHw_AI_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    
    pthread_mutex_lock(&pCap->mApiCallLock);
    if (pstAttr == NULL) {
        aloge("pstAttr is NULL!");
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_ILLEGAL_PARAM;
    }
    if (AI_STATE_INVALID != pCap->mState) {
        alogw("audioHw AI PublicAttr has been set!");
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return SUCCESS;
    }
    pCap->mAttr = *pstAttr;
    pCap->mState = AI_STATE_CONFIGURED;
    pthread_mutex_unlock(&pCap->mApiCallLock);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    
    pthread_mutex_lock(&pCap->mApiCallLock);
    if (pstAttr == NULL) {
        aloge("pstAttr is NULL!");
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_ILLEGAL_PARAM;
    }

    if (pCap->mState == AI_STATE_INVALID) {
        aloge("get attr when attr is not set!");
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_NOT_PERM;
    }

    *pstAttr = pCap->mAttr;
    pthread_mutex_unlock(&pCap->mApiCallLock);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_ClrPubAttr(AUDIO_DEV AudioDevId)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    
    pthread_mutex_lock(&pCap->mApiCallLock);
    if (pCap->mState == AI_STATE_STARTED) {
        aloge("please clear attr after AI disable!");
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_NOT_PERM;
    }
    memset(&pCap->mAttr, 0, sizeof(AIO_ATTR_S));
    pCap->mState = AI_STATE_INVALID;
    pthread_mutex_unlock(&pCap->mApiCallLock);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_Enable(AUDIO_DEV AudioDevId)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    int ret;
    
    pthread_mutex_lock(&pCap->mApiCallLock);

    if (pCap->mState == AI_STATE_INVALID) {
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_NOT_CONFIG;
    }
    if (pCap->mState == AI_STATE_STARTED) {
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return SUCCESS;
    }

    pCap->mCfg.chnCnt = pCap->mAttr.u32ChnCnt;
    pCap->mCfg.sampleRate = pCap->mAttr.enSamplerate;
    if (pCap->mAttr.enBitwidth == AUDIO_BIT_WIDTH_32) {
        pCap->mCfg.format = SND_PCM_FORMAT_S32_LE;
    } else if (pCap->mAttr.enBitwidth == AUDIO_BIT_WIDTH_24) {
        pCap->mCfg.format = SND_PCM_FORMAT_S24_LE;
    } else if (pCap->mAttr.enBitwidth == AUDIO_BIT_WIDTH_16) {
        pCap->mCfg.format = SND_PCM_FORMAT_S16_LE;
    } else if (pCap->mAttr.enBitwidth == AUDIO_BIT_WIDTH_8) {
        pCap->mCfg.format = SND_PCM_FORMAT_S8;
    } else {
        pCap->mCfg.format = SND_PCM_FORMAT_S16_LE;
    }
    pCap->mCfg.bitsPerSample = (pCap->mAttr.enBitwidth+1)*8;

    ret = alsaOpenPcm(&pCap->mCfg, SOUND_CARD_AUDIOCODEC, 0);
    if (ret != 0) {
        pthread_mutex_unlock(&pCap->mApiCallLock); 
        return FAILURE;
    }
    ret = alsaSetPcmParams(&pCap->mCfg);
    if (ret < 0) {
        goto ERR_SET_PCM_PARAM;
    }

    pthread_mutex_init(&pCap->mChnListLock, NULL);
    //INIT_LIST_HEAD(&pCap->mChnList);
    pCap->mThdRunning = TRUE;
    pthread_create(&pCap->mThdId, NULL, audioHw_AI_CapThread, pCap);

    pCap->mState = AI_STATE_STARTED;
    
    pthread_mutex_unlock(&pCap->mApiCallLock);

    return SUCCESS;

ERR_SET_PCM_PARAM:
    alsaClosePcm(&pCap->mCfg, 0);   // 0: cap
    pthread_mutex_unlock(&pCap->mApiCallLock);
    return FAILURE;
}

ERRORTYPE audioHw_AI_Disable(AUDIO_DEV AudioDevId)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    int ret;
    
    pthread_mutex_lock(&pCap->mApiCallLock);
    if (pCap->mState == AI_STATE_INVALID) {
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return ERR_AI_NOT_CONFIG;
    }
    if (pCap->mState != AI_STATE_STARTED) {
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return SUCCESS;
    }
    pthread_mutex_lock(&pCap->mChnListLock);
    if (!list_empty(&pCap->mChnList)) {
        pthread_mutex_unlock(&pCap->mChnListLock);
        pthread_mutex_unlock(&pCap->mApiCallLock);
        return SUCCESS;
    }
    pthread_mutex_unlock(&pCap->mChnListLock);

    pCap->mThdRunning = FALSE;
    pthread_join(pCap->mThdId, (void*) &ret);

    pthread_mutex_destroy(&pCap->mChnListLock);
    
    alsaClosePcm(&pCap->mCfg, 0);   // 0: cap
    pCap->mState = AI_STATE_CONFIGURED;
    pthread_mutex_unlock(&pCap->mApiCallLock);
    return SUCCESS;
}

ERRORTYPE audioHw_AI_SetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E enTrackMode)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    pCap->mTrackMode = enTrackMode;

    return SUCCESS;
}

ERRORTYPE audioHw_AI_GetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E *penTrackMode)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    *penTrackMode = pCap->mTrackMode;

    return SUCCESS;
}

ERRORTYPE audioHw_AI_GetPcmConfig(AUDIO_DEV AudioDevId, PCM_CONFIG_S **ppCfg)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }
    *ppCfg = &pCap->mCfg;
    return SUCCESS;
}

ERRORTYPE audioHw_AI_GetAIOAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S **ppAttr)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }
    *ppAttr = &pCap->mAttr;
    return SUCCESS;
}

ERRORTYPE audioHw_AI_SetVolume(AUDIO_DEV AudioDevId, int s32VolumeDb)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    return alsaMixerSetVolume(pMixer, 0, s32VolumeDb);
}

ERRORTYPE audioHw_AI_GetVolume(AUDIO_DEV AudioDevId, int *ps32VolumeDb)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    return alsaMixerGetVolume(pMixer, 0, (long*)ps32VolumeDb);
}

ERRORTYPE audioHw_AI_SetMute(AUDIO_DEV AudioDevId, int bEnable)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    return alsaMixerSetMute(pMixer, 0, bEnable);
}

ERRORTYPE audioHw_AI_GetMute(AUDIO_DEV AudioDevId, int *pbEnable)
{
    AudioInputDevice *pCap = &gAudioHwDev[AudioDevId].mCap;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pCap->mState != AI_STATE_STARTED) {
        return ERR_AI_NOT_ENABLED;
    }

    return alsaMixerGetMute(pMixer, 0, pbEnable);
}


/**************************************AO_DEV*****************************************/
ERRORTYPE audioHw_AO_Dev_lock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_lock(&gAudioHwDev[AudioDevId].mPlay.mChnListLock);
}

ERRORTYPE audioHw_AO_Dev_unlock(AUDIO_DEV AudioDevId)
{
    return pthread_mutex_unlock(&gAudioHwDev[AudioDevId].mPlay.mChnListLock);
}

ERRORTYPE audioHw_AO_searchChannel_l(AUDIO_DEV AudioDevId, AO_CHN AoChn, AO_CHANNEL_S** pChn)
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

ERRORTYPE audioHw_AO_searchChannel(AUDIO_DEV AudioDevId, AO_CHN AoChn, AO_CHANNEL_S** pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    ERRORTYPE ret = FAILURE;
    AO_CHANNEL_S *pEntry;
    pthread_mutex_lock(&pPlay->mChnListLock);
    ret = audioHw_AO_searchChannel_l(AudioDevId, AoChn, pChn);
    pthread_mutex_unlock(&pPlay->mChnListLock);
    return ret;
}

ERRORTYPE audioHw_AO_AddChannel_l(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
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

ERRORTYPE audioHw_AO_AddChannel(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    pthread_mutex_lock(&pPlay->mChnListLock);
    ERRORTYPE ret = audioHw_AO_AddChannel_l(AudioDevId, pChn);
    pthread_mutex_unlock(&pPlay->mChnListLock);
    return ret;
}

ERRORTYPE audioHw_AO_RemoveChannel(AUDIO_DEV AudioDevId, AO_CHANNEL_S* pChn)
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

MM_COMPONENTTYPE *audioHw_AO_GetChnComp(PARAM_IN MPP_CHN_S *pMppChn)
{
    AO_CHANNEL_S *pChn = NULL;
    if (SUCCESS != audioHw_AO_searchChannel(pMppChn->mDevId, pMppChn->mChnId, &pChn)) {
        return NULL;
    }
    return pChn->mpComp;
}

BOOL audioHw_AO_IsDevConfigured(AUDIO_DEV AudioDevId)
{
    return (gAudioHwDev[AudioDevId].mPlay.mState == AO_STATE_CONFIGURED);
}

BOOL audioHw_AO_IsDevStarted(AUDIO_DEV AudioDevId)
{
    return (gAudioHwDev[AudioDevId].mPlay.mState == AO_STATE_STARTED);
}

ERRORTYPE AudioHw_AO_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr)
{
    if (pstAttr == NULL) {
        aloge("pstAttr is NULL!");
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

ERRORTYPE AudioHw_AO_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr)
{
    if (pstAttr == NULL) {
        aloge("pstAttr is NULL!");
        return ERR_AO_ILLEGAL_PARAM;
    }

    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    if (pPlay->mState == AO_STATE_INVALID) {
        aloge("get attr when attr is not set!");
        return ERR_AO_NOT_PERM;
    }

    *pstAttr = pPlay->mAttr;
    return SUCCESS;
}

ERRORTYPE audioHw_AO_ClrPubAttr(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState == AO_STATE_STARTED) {
        aloge("please clear attr after AI disable!");
        return ERR_AO_NOT_PERM;
    }
    memset(&pPlay->mAttr, 0, sizeof(AIO_ATTR_S));
    pPlay->mState = AO_STATE_INVALID;
    return SUCCESS;
}

ERRORTYPE audioHw_AO_SetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E enTrackMode)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    pPlay->mTrackMode = enTrackMode;

    return SUCCESS;
}

ERRORTYPE audioHw_AO_GetTrackMode(AUDIO_DEV AudioDevId, AUDIO_TRACK_MODE_E *penTrackMode)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    *penTrackMode = pPlay->mTrackMode;

    return SUCCESS;
}

ERRORTYPE audioHw_AO_Enable(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    int ret;

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
    //INIT_LIST_HEAD(&pPlay->mChnList);
    //pPlay->mThdRunning = TRUE;
    //pthread_create(&pPlay->mThdId, NULL, audioHw_AO_PlayThread, &pPlay);

    pPlay->mState = AO_STATE_STARTED;

    return SUCCESS;

ERR_SET_PCM_PARAM:
    alsaClosePcm(&pPlay->mCfg, 1);  // 1: playback
    return FAILURE;
}

ERRORTYPE audioHw_AO_Disable(AUDIO_DEV AudioDevId)
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

    alogd("close pcm! current AoCardType:[%d]", pPlay->mAttr.mPcmCardId);
    alsaClosePcm(&pPlay->mCfg, 1);  // 1: playback
    pPlay->mState = AO_STATE_CONFIGURED;

    return SUCCESS;
}

ERRORTYPE audioHw_AO_SetVolume(AUDIO_DEV AudioDevId, int s32VolumeDb)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerSetVolume(pMixer, 1, s32VolumeDb);
}

ERRORTYPE audioHw_AO_GetVolume(AUDIO_DEV AudioDevId, int *ps32VolumeDb)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerGetVolume(pMixer, 1, (long*)ps32VolumeDb);
}

ERRORTYPE audioHw_AO_SetMute(AUDIO_DEV AudioDevId, BOOL bEnable, AUDIO_FADE_S *pstFade)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    return alsaMixerSetMute(pMixer, 1, (int)bEnable);
}

ERRORTYPE audioHw_AO_GetMute(AUDIO_DEV AudioDevId, BOOL *pbEnable, AUDIO_FADE_S *pstFade)
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

ERRORTYPE audioHw_AO_SetPA(AUDIO_DEV AudioDevId, BOOL bHighLevel)
{
    ERRORTYPE ret;
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    ret = alsaMixerSetPlayBackPA(pMixer, (int)bHighLevel);
    if(0 != ret)
    {
        aloge("fatal error! alsaMixer SetPlayBackPA fail[0x%x]!", ret);
    }
    return ret;
}

ERRORTYPE audioHw_AO_GetPA(AUDIO_DEV AudioDevId, BOOL *pbHighLevel)
{
    ERRORTYPE ret;
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    AIO_MIXER_S *pMixer = &gAudioHwDev[AudioDevId].mMixer;

    int bHighLevel = 0;
    ret = alsaMixerGetPlayBackPA(pMixer, &bHighLevel);
    if(0 == ret)
    {
        *pbHighLevel = bHighLevel?TRUE:FALSE;
    }
    else
    {
        aloge("fatal error! alsaMixer GetPlayBackPA fail[0x%x]!", ret);
    }

    return ret;
}

ERRORTYPE audioHw_AO_FillPcmRingBuf(AUDIO_DEV AudioDevId, void* pData, int Len)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    size_t frame_cnt = Len / (pPlay->mCfg.bitsPerFrame >> 3);
    ssize_t ret;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    ret = alsaWritePcm(&pPlay->mCfg, pData, frame_cnt);
    if (ret != frame_cnt) {
        aloge("alsaWritePcm error!");
        return FAILURE;
    }

    return SUCCESS;
}

ERRORTYPE audioHw_AO_DrainPcmRingBuf(AUDIO_DEV AudioDevId)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    alsaDrainPcm(&pPlay->mCfg);

    return SUCCESS;
}

ERRORTYPE audioHw_AO_FeedPcmData(AUDIO_DEV AudioDevId, AUDIO_FRAME_S *pFrm)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;
    size_t frame_cnt = pFrm->mLen / (pPlay->mCfg.bitsPerFrame >> 3);
    ssize_t ret;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }

    ret = alsaWritePcm(&pPlay->mCfg, pFrm->mpAddr, frame_cnt);
    if (ret != frame_cnt) {
        aloge("alsaWritePcm error!");
        return FAILURE;
    }

    return SUCCESS;
}

ERRORTYPE audioHw_AO_GetPcmConfig(AUDIO_DEV AudioDevId, PCM_CONFIG_S **ppCfg)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    *ppCfg = &pPlay->mCfg;
    return SUCCESS;
}

ERRORTYPE audioHw_AO_GetAIOAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S **ppAttr)
{
    AudioOutputDevice *pPlay = &gAudioHwDev[AudioDevId].mPlay;

    if (pPlay->mState != AO_STATE_STARTED) {
        return ERR_AO_NOT_ENABLED;
    }
    *ppAttr = &pPlay->mAttr;
    return SUCCESS;
}
