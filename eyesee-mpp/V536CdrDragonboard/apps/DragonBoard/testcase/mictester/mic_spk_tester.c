// 1. change mic record sound for 10s
// 2. adjust capture.wav's path to / and keep music's path to tf

#define LOG_TAG "sample_mic_spk"

#include "tinyalsa/asoundlib.h"

#include <utils/plat_log.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <mm_common.h>
#include <mpi_sys.h>
#include <mpi_ai.h>
#include <mpi_ao.h>
#include <mpi_clock.h>
#include <ClockCompPortIndex.h>
#include "log/log_wrapper.h"

#include <confparser.h>

#include "sample_ao_config.h"
#include "sample_ao.h"
#include "sample_ai_config.h"
#include "sample_ai.h"

#include <cdx_list.h>


#define TAG "mic&spktester" // mic and speaker tester
#include <dragonboard/dragonboard.h>

#define SPK_CFG_PATH     "/usr/bin/sample_ao.conf"
#define AI_CFG_PATH      "/usr/bin/sample_ai.conf"
#define MARK_AUDIO_PATH  "/usr/share/minigui/res/audio/startup.wav"
#define MIC_AUDIO_PATH   "/tmp/sample_ai_pcm.wav"
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1
int mic_spk_fd = 0;

int capturingFlag  = 0;
int playingType = 0;    // 0-raw music file; 1-recorded file
int playSuccess = 1;    // 1-success; 0-failed for spk



AUDIO_FRAME_S* SampleAOFrameManager_PrefetchFirstIdleFrame(void *pThiz)
{
    SampleAOFrameManager *pFrameManager = (SampleAOFrameManager*)pThiz;
    SampleAOFrameNode *pFirstNode;
    AUDIO_FRAME_S *pFrameInfo;
    pthread_mutex_lock(&pFrameManager->mLock);
    if(!list_empty(&pFrameManager->mIdleList))
    {
        pFirstNode = list_first_entry(&pFrameManager->mIdleList, SampleAOFrameNode, mList);
        pFrameInfo = &pFirstNode->mAFrame;
    }
    else
    {
        pFrameInfo = NULL;
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return pFrameInfo;
}

int SampleAOFrameManager_UseFrame(void *pThiz, AUDIO_FRAME_S *pFrame)
{
    int ret = 0;
    SampleAOFrameManager *pFrameManager = (SampleAOFrameManager*)pThiz;
    if(NULL == pFrame)
    {
        aloge("fatal error! pNode == NULL!");
        return -1;
    }
    pthread_mutex_lock(&pFrameManager->mLock);
    SampleAOFrameNode *pFirstNode = list_first_entry_or_null(&pFrameManager->mIdleList, SampleAOFrameNode, mList);
    if(pFirstNode)
    {
        if(&pFirstNode->mAFrame == pFrame)
        {
            list_move_tail(&pFirstNode->mList, &pFrameManager->mUsingList);
        }
        else
        {
            aloge("fatal error! node is not match [%p]!=[%p]", pFrame, &pFirstNode->mAFrame);
            ret = -1;
        }
    }
    else
    {
        aloge("fatal error! idle list is empty");
        ret = -1;
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return ret;
}

int SampleAOFrameManager_ReleaseFrame(void *pThiz, unsigned int nFrameId)
{
    int ret = 0;
    SampleAOFrameManager *pFrameManager = (SampleAOFrameManager*)pThiz;
    pthread_mutex_lock(&pFrameManager->mLock);
    int bFindFlag = 0;
    SampleAOFrameNode *pEntry, *pTmp;
    list_for_each_entry_safe(pEntry, pTmp, &pFrameManager->mUsingList, mList)
    {
        if(pEntry->mAFrame.mId == nFrameId)
        {
            list_move_tail(&pEntry->mList, &pFrameManager->mIdleList);
            bFindFlag = 1;
            break;
        }
    }
    if(0 == bFindFlag)
    {
        aloge("fatal error! frameId[%d] is not find", nFrameId);
        ret = -1;
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return ret;
}

int initSampleAOFrameManager(SampleAOFrameManager *pFrameManager, int nFrameNum, SampleAOConfig *pConfigPara)
{
    memset(pFrameManager, 0, sizeof(SampleAOFrameManager));
    int err = pthread_mutex_init(&pFrameManager->mLock, NULL);
    if(err!=0)
    {
        aloge("fatal error! pthread mutex init fail!");
    }
    INIT_LIST_HEAD(&pFrameManager->mIdleList);
    INIT_LIST_HEAD(&pFrameManager->mUsingList);

    int i;
    SampleAOFrameNode *pNode;
    for (i=0; i<nFrameNum; i++)
    {
        pNode = malloc(sizeof(SampleAOFrameNode));
        memset(pNode, 0, sizeof(SampleAOFrameNode));
        pNode->mAFrame.mId = i;
        pNode->mAFrame.mBitwidth = (AUDIO_BIT_WIDTH_E)(pConfigPara->mBitWidth/8 - 1);
        pNode->mAFrame.mSoundmode = (pConfigPara->mChannelCnt==1)?AUDIO_SOUND_MODE_MONO:AUDIO_SOUND_MODE_STEREO;
        pNode->mAFrame.mLen = pConfigPara->mChannelCnt * pConfigPara->mBitWidth/8 * pConfigPara->mFrameSize;
        pNode->mAFrame.mpAddr = malloc(pNode->mAFrame.mLen);
        list_add_tail(&pNode->mList, &pFrameManager->mIdleList);
    }
    pFrameManager->mNodeCnt = nFrameNum;

    pFrameManager->PrefetchFirstIdleFrame = SampleAOFrameManager_PrefetchFirstIdleFrame;
    pFrameManager->UseFrame = SampleAOFrameManager_UseFrame;
    pFrameManager->ReleaseFrame = SampleAOFrameManager_ReleaseFrame;
    return 0;
}

int destroySampleAOFrameManager(SampleAOFrameManager *pFrameManager)
{
    if(!list_empty(&pFrameManager->mUsingList))
    {
        aloge("fatal error! why using list is not empty");
    }
    int cnt = 0;
    struct list_head *pList;
    list_for_each(pList, &pFrameManager->mIdleList)
    {
        cnt++;
    }
    if(cnt != pFrameManager->mNodeCnt)
    {
        aloge("fatal error! frame count is not match [%d]!=[%d]", cnt, pFrameManager->mNodeCnt);
    }
    SampleAOFrameNode *pEntry, *pTmp;
    list_for_each_entry_safe(pEntry, pTmp, &pFrameManager->mIdleList, mList)
    {
        free(pEntry->mAFrame.mpAddr);
        list_del(&pEntry->mList);
        free(pEntry);
    }
    pthread_mutex_destroy(&pFrameManager->mLock);
    return 0;
}

int initSampleAOContext(SampleAOContext *pContext)
{
    memset(pContext, 0, sizeof(SampleAOContext));
    int err = pthread_mutex_init(&pContext->mWaitFrameLock, NULL);
    if(err!=0)
    {
        aloge("fatal error! pthread mutex init fail!");
    }
    err = cdx_sem_init(&pContext->mSemFrameCome, 0);
    err = cdx_sem_init(&pContext->mSemEofCome, 0);
    if(err!=0)
    {
        aloge("cdx sem init fail!");
    }
    return 0;
}

int destroySampleAOContext(SampleAOContext *pContext)
{
    pthread_mutex_destroy(&pContext->mWaitFrameLock);
    cdx_sem_deinit(&pContext->mSemFrameCome);
    cdx_sem_deinit(&pContext->mSemEofCome);
    return 0;
}

static ERRORTYPE SampleAOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleAOContext *pContext = (SampleAOContext*)cookie;
    if(MOD_ID_AO == pChn->mModId)
    {
        if(pChn->mChnId != pContext->mAOChn)
        {
            aloge("fatal error! AO chnId[%d]!=[%d]", pChn->mChnId, pContext->mAOChn);
        }
        switch(event)
        {
            case MPP_EVENT_RELEASE_AUDIO_BUFFER:
            {
                AUDIO_FRAME_S *pAFrame = (AUDIO_FRAME_S*)pEventData;
                pContext->mFrameManager.ReleaseFrame(&pContext->mFrameManager, pAFrame->mId);
                pthread_mutex_lock(&pContext->mWaitFrameLock);
                if(pContext->mbWaitFrameFlag)
                {
                    pContext->mbWaitFrameFlag = 0;
                    cdx_sem_up(&pContext->mSemFrameCome);
                }
                pthread_mutex_unlock(&pContext->mWaitFrameLock);
                break;
            }
            case MPP_EVENT_NOTIFY_EOF:
            {
                alogd("AO channel notify APP that play complete!");
                cdx_sem_signal(&pContext->mSemEofCome);
                break;
            }
            default:
            {
                //postEventFromNative(this, event, 0, 0, pEventData);
                aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                ret = ERR_AO_ILLEGAL_PARAM;
                break;
            }
        }
    }
    else
    {
        aloge("fatal error! why modId[0x%x]?", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

static ERRORTYPE SampleAO_CLOCKCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    alogw("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}

static int ParseCmdLine(int argc, char **argv, SampleAOCmdLineParam *pCmdLinePara)
{
    alogd("sample ao path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleAOCmdLineParam));
    while(i < argc)
    {
        if(!strcmp(argv[i], "-path"))
        {
            if(++i >= argc)
            {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE)
            {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        }
        else if(!strcmp(argv[i], "-h"))
        {
            alogd("CmdLine param:\n"
                "\t-path /home/sample_ao.conf\n");
            ret = 1;
            break;
        }
        else
        {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

void config_AIO_ATTR_S_by_SampleAOConfig(AIO_ATTR_S *dst, SampleAOConfig *src)
{
    memset(dst, 0, sizeof(AIO_ATTR_S));
    dst->u32ChnCnt = src->mChannelCnt;
    dst->enSamplerate = (AUDIO_SAMPLE_RATE_E)src->mSampleRate;
    dst->enBitwidth = (AUDIO_BIT_WIDTH_E)(src->mBitWidth/8-1);
}

void config_AIO_ATTR_S_by_SampleAIConfig(AIO_ATTR_S *dst, SampleAIConfig *src)
{
    memset(dst, 0, sizeof(AIO_ATTR_S));
    dst->u32ChnCnt = src->mChannelCnt;
    dst->enSamplerate = (AUDIO_SAMPLE_RATE_E)src->mSampleRate;
    dst->enBitwidth = (AUDIO_BIT_WIDTH_E)(src->mBitWidth/8-1);
}

void PcmDataAddWaveHeader(SampleAIContext *ctx)
{
    struct WaveHeader{
        int riff_id;
        int riff_sz;
        int riff_fmt;
        int fmt_id;
        int fmt_sz;
        short audio_fmt;
        short num_chn;
        int sample_rate;
        int byte_rate;
        short block_align;
        short bits_per_sample;
        int data_id;
        int data_sz;
    } header;
    SampleAIConfig *pConf = &ctx->mConfigPara;

    memcpy(&header.riff_id, "RIFF", 4);
    header.riff_sz = ctx->mPcmSize + sizeof(struct WaveHeader) - 8;
    memcpy(&header.riff_fmt, "WAVE", 4);
    memcpy(&header.fmt_id, "fmt ", 4);
    header.fmt_sz = 16;
    header.audio_fmt = 1;       // s16le
    header.num_chn = pConf->mChannelCnt;
    header.sample_rate = pConf->mSampleRate;
    header.byte_rate = pConf->mSampleRate * pConf->mChannelCnt * pConf->mBitWidth/8;
    header.block_align = pConf->mChannelCnt * pConf->mBitWidth/8;
    header.bits_per_sample = pConf->mBitWidth;
    memcpy(&header.data_id, "data", 4);
    header.data_sz = ctx->mPcmSize;

    fseek(ctx->mFpPcmFile, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(struct WaveHeader), ctx->mFpPcmFile);
}

static ERRORTYPE loadSampleAOConfig(SampleAOConfig *pConfig, const char *conf_path, char* audio_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S stConfParser;

    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0)
    {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleAOConfig));
   /* ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_AO_PCM_FILE_PATH, NULL);
    strncpy(pConfig->mPcmFilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->mPcmFilePath[MAX_FILE_PATH_SIZE-1] = '\0';*/
    pConfig->mSampleRate = GetConfParaInt(&stConfParser, SAMPLE_AO_PCM_SAMPLE_RATE, 0);
    pConfig->mBitWidth = GetConfParaInt(&stConfParser, SAMPLE_AO_PCM_BIT_WIDTH, 0);
    pConfig->mChannelCnt = GetConfParaInt(&stConfParser, SAMPLE_AO_PCM_CHANNEL_CNT, 0);
    pConfig->mFrameSize = GetConfParaInt(&stConfParser, SAMPLE_AO_PCM_FRAME_SIZE, 0);
    destroyConfParser(&stConfParser);

    return SUCCESS;
}

static ERRORTYPE loadSampleAIConfig(SampleAIConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S stConfParser;

    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0)
    {
        aloge("load conf fail");
        ret = FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleAIConfig));
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_AI_PCM_FILE_PATH, NULL);
    if (ptr == NULL)
    {
        aloge("parse item[%s] failed!", conf_path);
        ret = FAILURE;
    }
    else
    {
        strncpy(pConfig->mPcmFilePath, ptr, MAX_FILE_PATH_SIZE-1);
        alogd("parse info: mPcmFilePath[%s]", pConfig->mPcmFilePath);
    }
    pConfig->mPcmFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    pConfig->mSampleRate = GetConfParaInt(&stConfParser, SAMPLE_AI_PCM_SAMPLE_RATE, 0);
    pConfig->mBitWidth = GetConfParaInt(&stConfParser, SAMPLE_AI_PCM_BIT_WIDTH, 0);
    pConfig->mChannelCnt = GetConfParaInt(&stConfParser, SAMPLE_AI_PCM_CHANNEL_CNT, 0);
    pConfig->mFrameSize = GetConfParaInt(&stConfParser, SAMPLE_AI_PCM_FRAME_SIZE, 0);
    pConfig->mCapDuraSec = GetConfParaInt(&stConfParser, SAMPLE_AI_PCM_CAP_DURATION, 0);
    if (pConfig->mSampleRate && pConfig->mBitWidth && pConfig->mChannelCnt && pConfig->mFrameSize && pConfig->mCapDuraSec)
    {
        alogd("para: mSampleRate[%d], mBitWidth[%d], mChannelCnt[%d], mFrameSize[%d], mCapTime[%d]s",
            pConfig->mSampleRate, pConfig->mBitWidth, pConfig->mChannelCnt, pConfig->mFrameSize, pConfig->mCapDuraSec);
    }
    else
    {
        aloge("parse file(%s) for some items failed!", conf_path);
        ret = FAILURE;
    }
    destroyConfParser(&stConfParser);

    return ret;
}

int speaker_test_main(int audio_path_option)
{
    int ret = 0;
    alogd("Hello, sample_ao!");
    SampleAOContext stContext;
    initSampleAOContext(&stContext);
    CONFPARSER_S stConfParser;
    char *pConfigFilePath;
    pConfigFilePath = SPK_CFG_PATH;
    char *mPcmFilePath;
    char *ptr;
    char result[128] = {0};
    if(audio_path_option == 1)
    {
        aloge("choose option 1");
        mPcmFilePath = MARK_AUDIO_PATH;
#if 0
        ftruncate(mic_spk_fd,0);
        memset(&result,0,sizeof(result));
        sprintf(result,"P[MIC_SPK] music playing");
        write(mic_spk_fd,&result,strlen(result));
#endif
    }
    else if(audio_path_option == 2)
    {
        aloge("choose option 2");
        mPcmFilePath = MIC_AUDIO_PATH;
#if 0
        ftruncate(mic_spk_fd,0);
        memset(&result,0,sizeof(result));
        sprintf(result,"P[MIC_SPK] rec.. playing");
        write(mic_spk_fd,&result,strlen(result));
#endif
    }else {
        aloge("fatal error!not selecting the right option");
        ret = -1;
        return ret;
    }
    //parse config file.
    if(loadSampleAOConfig(&stContext.mConfigPara, pConfigFilePath,mPcmFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        ret = -1;
        goto _exit;
    }
    //open pcm file

    stContext.mFpPcmFile = fopen(mPcmFilePath, "rb");
    if(!stContext.mFpPcmFile)
    {
        aloge("fatal error! can't open pcm file[%s]", mPcmFilePath);
        ret = -1;
        goto _exit;
    }
    else
    {
        fseek(stContext.mFpPcmFile, 44, SEEK_SET);  // 44: size(WavHeader)
    }
    //init mpp system
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();
    //init frame manager
    initSampleAOFrameManager(&stContext.mFrameManager, 5, &stContext.mConfigPara);

    //enable ao dev
    stContext.mAODev = 0;
    config_AIO_ATTR_S_by_SampleAOConfig(&stContext.mAIOAttr, &stContext.mConfigPara);
    AW_MPI_AO_SetPubAttr(stContext.mAODev, &stContext.mAIOAttr);
    //embedded in AW_MPI_AO_EnableChn
    //AW_MPI_AO_Enable(stContext.mAODev);
    
    //create ao channel and clock channel.
    BOOL bSuccessFlag = FALSE;
    stContext.mAOChn = 0;
    while(stContext.mAOChn < AIO_MAX_CHN_NUM)
    {
        ret = AW_MPI_AO_EnableChn(stContext.mAODev, stContext.mAOChn);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            alogd("create ao channel[%d] success!", stContext.mAOChn);
            break;
        }
        else if (ERR_AO_EXIST == ret)
        {
            alogd("ao channel[%d] exist, find next!", stContext.mAOChn);
            stContext.mAOChn++;
        }
        else if(ERR_AO_NOT_ENABLED == ret)
        {
            aloge("audio_hw_ao not started!");
            break;
        }
        else
        {
            aloge("create ao channel[%d] fail! ret[0x%x]!", stContext.mAOChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        stContext.mAOChn = MM_INVALID_CHN;
        aloge("fatal error! create ao channel fail!");
    }
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)&stContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleAOCallbackWrapper;
    AW_MPI_AO_RegisterCallback(stContext.mAODev, stContext.mAOChn, &cbInfo);
    
    bSuccessFlag = FALSE;
    stContext.mClockChnAttr.nWaitMask = 0;
    stContext.mClockChnAttr.nWaitMask |= 1<<CLOCK_PORT_INDEX_AUDIO;
    stContext.mClockChn = 0;
    while(stContext.mClockChn < CLOCK_MAX_CHN_NUM)
    {
        ret = AW_MPI_CLOCK_CreateChn(stContext.mClockChn, &stContext.mClockChnAttr);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            alogd("create clock channel[%d] success!", stContext.mClockChn);
            break;
        }
        else if(ERR_CLOCK_EXIST == ret)
        {
            alogd("clock channel[%d] is exist, find next!", stContext.mClockChn);
            stContext.mClockChn++;
        }
        else
        {
            alogd("create clock channel[%d] ret[0x%x]!", stContext.mClockChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        stContext.mClockChn = MM_INVALID_CHN;
        aloge("fatal error! create clock channel fail!");
    }
    cbInfo.cookie = (void*)&stContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleAO_CLOCKCallbackWrapper;
    AW_MPI_CLOCK_RegisterCallback(stContext.mClockChn, &cbInfo);
    //bind clock and ao
    MPP_CHN_S ClockChn = {MOD_ID_CLOCK, 0, stContext.mClockChn};
    MPP_CHN_S AOChn = {MOD_ID_AO, stContext.mAODev, stContext.mAOChn};
    AW_MPI_SYS_Bind(&ClockChn, &AOChn);
    
    //test ao save file api
    //AUDIO_SAVE_FILE_INFO_S info;
    //strcpy(info.mFilePath, "/mnt/extsd/");
    //strcpy(info.mFileName, "SampleAo_AoSaveFile.pcm");
    //AW_MPI_AO_SaveFile(0,0,&info);

    //start ao and clock.
    AW_MPI_CLOCK_Start(stContext.mClockChn);
    AW_MPI_AO_StartChn(stContext.mAODev, stContext.mAOChn);
//    AW_MPI_AO_SetVolume(stContext.mAODev, 90);
    
    //read pcm from file, play pcm through mpi_ao. we set pts by stContext.mConfigPara(mSampleRate,mFrameSize).
    uint64_t nPts = 0;   //unit:us
    int nFrameSize = stContext.mConfigPara.mFrameSize;
    int nSampleRate = stContext.mConfigPara.mSampleRate;
    uint64_t nFrameInterval = 1000000*nFrameSize/nSampleRate; //128000us
    AUDIO_FRAME_S *pFrameInfo;
    int nReadLen;
    pthread_t tid;
    int exit = 0;
    while(1)
    {
        //request idle frame
        pFrameInfo = stContext.mFrameManager.PrefetchFirstIdleFrame(&stContext.mFrameManager);
        if(NULL == pFrameInfo)
        {
            pthread_mutex_lock(&stContext.mWaitFrameLock);
            pFrameInfo = stContext.mFrameManager.PrefetchFirstIdleFrame(&stContext.mFrameManager);
            if(pFrameInfo!=NULL)
            {
                pthread_mutex_unlock(&stContext.mWaitFrameLock);
            }
            else
            {
                stContext.mbWaitFrameFlag = 1;
                pthread_mutex_unlock(&stContext.mWaitFrameLock);
                cdx_sem_down_timedwait(&stContext.mSemFrameCome, 500);
                continue;
            }
        }

        //read pcm to idle frame
        int nWantedReadLen = nFrameSize * stContext.mConfigPara.mChannelCnt * (stContext.mConfigPara.mBitWidth/8);
        nReadLen = fread(pFrameInfo->mpAddr, 1, nWantedReadLen, stContext.mFpPcmFile);
        if(nReadLen < nWantedReadLen)
        {
            int bEof = feof(stContext.mFpPcmFile);
            if(bEof)
            {
                alogd("read file finish!");
            }
            break;
        }
        pFrameInfo->mTimeStamp = nPts;
        nPts += nFrameInterval;
        pFrameInfo->mId = pFrameInfo->mTimeStamp / nFrameInterval;
        stContext.mFrameManager.UseFrame(&stContext.mFrameManager, pFrameInfo);

        //send pcm to ao
        ret = AW_MPI_AO_SendFrame(stContext.mAODev, stContext.mAOChn, pFrameInfo, 0);
        if(ret != SUCCESS)
        {
            alogd("impossible, send frameId[%d] fail?", pFrameInfo->mId);
            stContext.mFrameManager.ReleaseFrame(&stContext.mFrameManager, pFrameInfo->mId);
        }
    }
    AW_MPI_AO_SetStreamEof(stContext.mAODev, stContext.mAOChn, 1, 0);
    cdx_sem_wait(&stContext.mSemEofCome);
    exit = 1;

    //stop ao channel, clock channel
    AW_MPI_AO_StopChn(stContext.mAODev, stContext.mAOChn);
    AW_MPI_CLOCK_Stop(stContext.mClockChn);
    AW_MPI_AO_DisableChn(stContext.mAODev, stContext.mAOChn);
    stContext.mAODev = MM_INVALID_DEV;
    stContext.mAOChn = MM_INVALID_CHN;
    AW_MPI_CLOCK_DestroyChn(stContext.mClockChn);
    stContext.mClockChn = MM_INVALID_CHN;
    destroySampleAOFrameManager(&stContext.mFrameManager);
    //exit mpp system
    AW_MPI_SYS_Exit();
    //close pcm file
    fclose(stContext.mFpPcmFile);
    stContext.mFpPcmFile = NULL;

_exit:
    destroySampleAOContext(&stContext);
    if (result == 0) {
        printf("sample_ao exit!\n");
    }
    return ret;
}

int mic_test_main()
{
    int result = 0;
    alogd("Hello, sample_ai!");
    SampleAIContext stContext;
    memset(&stContext, 0, sizeof(SampleAIContext));
    //parse config file.
    char *pConfigFilePath;
    pConfigFilePath = AI_CFG_PATH;
    if(loadSampleAIConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }

    //open pcm file
    stContext.mFpPcmFile = fopen(MIC_AUDIO_PATH, "wb");
    if(!stContext.mFpPcmFile)
    {
        aloge("fatal error! can't open pcm file[%s]", MIC_AUDIO_PATH);
        result = -1;
        goto _exit;
    }
    else
    {
        alogd("sample_ai produce file: %s", MIC_AUDIO_PATH);
        fseek(stContext.mFpPcmFile, 44, SEEK_SET);  // 44: size(WavHeader)
    }

    //init mpp system
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();
    //enable ai dev
    stContext.mAIDev = 0;
    config_AIO_ATTR_S_by_SampleAIConfig(&stContext.mAIOAttr, &stContext.mConfigPara);
    AW_MPI_AI_SetPubAttr(stContext.mAIDev, &stContext.mAIOAttr);
    //embedded in AW_MPI_AI_CreateChn
    //AW_MPI_AI_Enable(stContext.mAIDev);

    //create ai channel.
    ERRORTYPE ret;
    BOOL bSuccessFlag = FALSE;
    stContext.mAIChn = 0;
    while(stContext.mAIChn < AIO_MAX_CHN_NUM)
    {
        ret = AW_MPI_AI_CreateChn(stContext.mAIDev, stContext.mAIChn);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            alogd("create ai channel[%d] success!", stContext.mAIChn);
            break;
        }
        else if (ERR_AI_EXIST == ret)
        {
            alogd("ai channel[%d] exist, find next!", stContext.mAIChn);
            stContext.mAIChn++;
        }
        else if(ERR_AI_NOT_ENABLED == ret)
        {
            aloge("audio_hw_ai not started!");
            break;
        }
        else
        {
            aloge("create ai channel[%d] fail! ret[0x%x]!", stContext.mAIChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        stContext.mAIChn = MM_INVALID_CHN;
        aloge("fatal error! create ai channel fail!");
        goto _exit;
    }
    //start ai dev.
    AW_MPI_AI_EnableChn(stContext.mAIDev, stContext.mAIChn);

    int nWriteLen;
    AUDIO_FRAME_S nAFrame;
    SampleAIConfig *pAiConf = &stContext.mConfigPara;
    //cap pcm for xx s
    int nMaxWantedSize = pAiConf->mSampleRate * pAiConf->mChannelCnt * pAiConf->mBitWidth/8 * pAiConf->mCapDuraSec;
    while (1)
    {
        ret = AW_MPI_AI_GetFrame(stContext.mAIDev, stContext.mAIChn, &nAFrame, NULL, -1);
        if (SUCCESS == ret)
        {
            nWriteLen = fwrite(nAFrame.mpAddr, 1, nAFrame.mLen, stContext.mFpPcmFile);
            stContext.mPcmSize += nWriteLen;
            ret = AW_MPI_AI_ReleaseFrame(stContext.mAIDev, stContext.mAIChn, &nAFrame, NULL);
            if (SUCCESS != ret)
            {
                aloge("release frame to ai fail! ret: %#x", ret);
            }
        }
        else
        {
            aloge("get pcm from ai in block mode fail! ret: %#x", ret);
            break;
        }

        if (stContext.mPcmSize >= nMaxWantedSize)
        {
            alogd("capture %d Bytes pcm data, finish!", stContext.mPcmSize);
            break;
        }
    }
    PcmDataAddWaveHeader(&stContext);
    fclose(stContext.mFpPcmFile);
    stContext.mFpPcmFile = NULL;

    //stop ai chn.
    AW_MPI_AI_DisableChn(stContext.mAIDev, stContext.mAIChn);

    //reset and destroy ai chn & dev.
    AW_MPI_AI_ResetChn(stContext.mAIDev, stContext.mAIChn);
    AW_MPI_AI_DestroyChn(stContext.mAIDev, stContext.mAIChn);

    stContext.mAIDev = MM_INVALID_DEV;
    stContext.mAIChn = MM_INVALID_CHN;

    //exit mpp system
    AW_MPI_SYS_Exit();

_exit:
    if (result == 0) {
        printf("sample_ai exit!\n");
    }
    return result;

}
int speaker_test(int option)
{
    int fd = -1, ret = 0;
    int audio_path_option = option;
    /************  Speaker Init  ************/
    printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);
    char result[128] = {0};
    ret = speaker_test_main(audio_path_option);
    if (ret < 0) {
        goto speaker_error;
    }

    printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);
#if 0
    if (1 == option) {
        memset(&result,0,sizeof(result));
        ftruncate(mic_spk_fd,0);
        sprintf(result,"P[MIC_SPK] rec..");
        write(mic_spk_fd,&result,strlen(result));
    } else {
        memset(&result,0,sizeof(result));
        ftruncate(mic_spk_fd,0);
        sprintf(result,"P[MIC_SPK] PASS");
        write(mic_spk_fd,&result,strlen(result));
    }
#endif

    return 0;

speaker_error:
    return -1;
}


int mic_test()
{
    int fd = -1, ret = 0;
	unsigned long file_size = 0;
	char result[128] = {0};
    /************  MIC Init  ************/
#if 0
	memset(&result,0,sizeof(result));
    ftruncate(mic_spk_fd,0);
    sprintf(result,"P[MIC_SPK] recording...");
    write(mic_spk_fd,&result,strlen(result));
#endif
    /* enable ai dev */
    ret = mic_test_main();
    if (ret < 0) {
        goto mic_error;
    }
    return 0;
mic_error:
#if 0
    memset(&result,0,sizeof(result));
    ftruncate(mic_spk_fd,0);
    sprintf(result,"F[MIC_SPK] Fail");
    write(mic_spk_fd,&result,strlen(result)); // means open or create file failed
#endif
    return -1;
}

int main(int argc, char **argv)
{
    int ret = 0;
    char result[128] = {0};
    mic_spk_fd = open(FIFO_SPK_MIC_DEV, O_RDWR| O_CREAT,0666);
    system("tinymix 2 172");
    usleep(200 * 1000);
    ret = speaker_test(1);
    if(ret < 0){
        aloge("speaker_test fail");
        goto error;
    }

    usleep(300 * 1000);

    ret = mic_test();
    if(ret < 0){
        aloge("mic_test fail");
        goto error;
    }

    //usleep(100 * 1000);

    //system("tinymix 2 172");
    usleep(400 * 1000);

    ret = speaker_test(2);
    if(ret < 0){
       aloge("speaker_test fail");
       goto error;
    }
    memset(&result,0,sizeof(result));
    ftruncate(mic_spk_fd,0);
    sprintf(result,"P[MIC_SPK] PASS");
    write(mic_spk_fd,&result,strlen(result));   // means open or create file failed
    close(mic_spk_fd);
    return 0;


error:
    memset(&result,0,sizeof(result));
    ftruncate(mic_spk_fd,0);
    sprintf(result,"F[MIC_SPK] Fail");
    write(mic_spk_fd,&result,strlen(result));   // means open or create file failed
    close(mic_spk_fd);
    return 0;
}

