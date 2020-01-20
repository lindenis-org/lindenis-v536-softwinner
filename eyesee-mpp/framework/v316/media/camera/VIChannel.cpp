/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : VIChannel.cpp
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/06/02
  Last Modified :
  Description   : camera wrap MPP components.
  Function List :
  History       :
******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "VIChannel"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <memory.h>
#include <vector>

#include <memoryAdapter.h>
#include <SystemBase.h>
#include <VIDEO_FRAME_INFO_S.h>
#include <mpi_vi_private.h>
#include <mpi_vi.h>
#include <mpi_isp.h>
#include <mpi_videoformat_conversion.h>

#include <utils/Thread.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <MediaStructConvert.h>
#include "VIChannel.h"
#include <ConfigOption.h>


using namespace std;

#define DEBUG_STORE_VI_FRAME (0)
#define DEBUG_SAVE_FRAME_TO_TMP (0)
#define DEBUG_SAVE_FRAME_NUM  (60)

namespace EyeseeLinux {

VIChannel::DoCaptureThread::DoCaptureThread(VIChannel *pViChn)
    : mpViChn(pViChn)
{
    mCapThreadState = CAPTURE_STATE_NULL;
}

status_t VIChannel::DoCaptureThread::startThread()
{
    mCapThreadState = CAPTURE_STATE_PAUSED;
    status_t ret = run("VIChnCapture");
    if(ret != NO_ERROR)
    {
        aloge("fatal error! run thread fail!");
    }
    return ret;
}

void VIChannel::DoCaptureThread::stopThread()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCapture_Exit;
    mMsgQueue.queueMessage(&msg);
    join();
    mMsgQueue.flushMessage();
}

status_t VIChannel::DoCaptureThread::startCapture()
{
    AutoMutex lock(mStateLock);
    if(mCapThreadState == CAPTURE_STATE_STARTED)
    {
        alogd("already in started");
        return NO_ERROR;
    }
    if(mCapThreadState != CAPTURE_STATE_PAUSED)
    {
        aloge("fatal error! can't call in state[0x%x]", mCapThreadState);
        return INVALID_OPERATION;
    }
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCapture_SetState;
    msg.mPara0 = CAPTURE_STATE_STARTED;
    mMsgQueue.queueMessage(&msg);
    while(CAPTURE_STATE_STARTED != mCapThreadState)
    {
        mStartCompleteCond.wait(mStateLock);
    }
    return NO_ERROR;
}

status_t VIChannel::DoCaptureThread::pauseCapture()
{
    AutoMutex lock(mStateLock);
    if(mCapThreadState == CAPTURE_STATE_PAUSED)
    {
        alogd("already in paused");
        return NO_ERROR;
    }
    if(mCapThreadState != CAPTURE_STATE_STARTED)
    {
        aloge("fatal error! can't call in state[0x%x]", mCapThreadState);
        return INVALID_OPERATION;
    }
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCapture_SetState;
    msg.mPara0 = CAPTURE_STATE_PAUSED;
    mMsgQueue.queueMessage(&msg);
    while(CAPTURE_STATE_PAUSED != mCapThreadState)
    {
        mPauseCompleteCond.wait(mStateLock);
    }
    return NO_ERROR;
}

bool VIChannel::DoCaptureThread::threadLoop()
{
    if(!exitPending())
    {
        return mpViChn->captureThread();
    } 
    else
    {
        return false;
    }
}

status_t VIChannel::DoCaptureThread::SendCommand_TakePicture()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCapture_TakePicture;
    mMsgQueue.queueMessage(&msg);
    return NO_ERROR;
}

status_t VIChannel::DoCaptureThread::SendCommand_CancelContinuousPicture()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCapture_CancelContinuousPicture;
    mMsgQueue.queueMessage(&msg);
    return NO_ERROR;
}

VIChannel::DoPreviewThread::DoPreviewThread(VIChannel *pViChn)
    : mpViChn(pViChn)
{
    mbWaitPreviewFrame = false;
}
status_t VIChannel::DoPreviewThread::startThread()
{
    status_t ret = run("VIChnPreview");
    if(ret != NO_ERROR)
    {
        aloge("fatal error! run thread fail!");
    }
    return ret;
}

void VIChannel::DoPreviewThread::stopThread()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypePreview_Exit;
    mMsgQueue.queueMessage(&msg);
    join();
    mMsgQueue.flushMessage();
}

status_t VIChannel::DoPreviewThread::notifyNewFrameCome()
{
    AutoMutex lock(mWaitLock);
    if(mbWaitPreviewFrame)
    {
        mbWaitPreviewFrame = false;
        EyeseeMessage msg;
        msg.mMsgType = MsgTypePreview_InputFrameAvailable;
        mMsgQueue.queueMessage(&msg);
    }
    return NO_ERROR;
}

status_t VIChannel::DoPreviewThread::releaseAllFrames()
{
    AutoMutex lock(mWaitLock);
    mbWaitReleaseAllFrames = true;
    EyeseeMessage msg;
    msg.mMsgType = MsgTypePreview_releaseAllFrames;
    mMsgQueue.queueMessage(&msg);
    while(mbWaitReleaseAllFrames)
    {
        mCondReleaseAllFramesFinished.wait(mWaitLock);
    }
    return NO_ERROR;
}

bool VIChannel::DoPreviewThread::threadLoop()
{
    if(!exitPending())
    {
        return mpViChn->previewThread();
    }
    else
    {
        return false;
    }
}

VIChannel::DoPictureThread::DoPictureThread(VIChannel *pViChn)
    : mpViChn(pViChn)
{
    mbWaitPictureFrame = false;
}

status_t VIChannel::DoPictureThread::startThread() 
{
    status_t ret = run("VIChnPicture");
    if(ret != NO_ERROR)
    {
        aloge("fatal error! run thread fail!");
    }
    return ret;
}

void VIChannel::DoPictureThread::stopThread()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypePicture_Exit;
    mMsgQueue.queueMessage(&msg);
    join();
    mMsgQueue.flushMessage();
}

status_t VIChannel::DoPictureThread::notifyNewFrameCome()
{
    AutoMutex lock(mWaitLock);
    if(mbWaitPictureFrame)
    {
        mbWaitPictureFrame = false;
        EyeseeMessage msg;
        msg.mMsgType = MsgTypePicture_InputFrameAvailable;
        mMsgQueue.queueMessage(&msg);
    }
    return NO_ERROR;
}

status_t VIChannel::DoPictureThread::notifyPictureEnd()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypePicture_SendPictureEnd;
    mMsgQueue.queueMessage(&msg);
    return NO_ERROR;
}

status_t VIChannel::DoPictureThread::releaseAllFrames()
{
    AutoMutex lock(mWaitLock);
    mbWaitReleaseAllFrames = true;
    EyeseeMessage msg;
    msg.mMsgType = MsgTypePicture_releaseAllFrames;
    mMsgQueue.queueMessage(&msg);
    while(mbWaitReleaseAllFrames)
    {
        mCondReleaseAllFramesFinished.wait(mWaitLock);
    }
    return NO_ERROR;
}

bool VIChannel::DoPictureThread::threadLoop()
{
    if(!exitPending())
    {
        return mpViChn->pictureThread();
    } 
    else
    {
        return false;
    }
}

VIChannel::DoMODThread::DoMODThread(VIChannel *pViChn)
    : mpViChn(pViChn)
{
    mpMODFrameQueue = NULL;
    mbWaitFrame = false;
    mbMODDetectEnable = false;
    mhMOD = NULL;
    mFrameCounter = 0;
	m_Sensitivity = 4;
	m_awmd = NULL;
	m_MDvar = NULL;
	m_width = 0;
	m_height = 0;
}
VIChannel::DoMODThread::~DoMODThread()
{
    if(mpMODFrameQueue)
    {
        alogw("fatal error! why MOD frame queue is not null?");
        delete mpMODFrameQueue;
    }
}
status_t VIChannel::DoMODThread::startThread()
{
    status_t ret = run("VIChnMOD");
    if(ret != NO_ERROR)
    {
        aloge("fatal error! run thread fail!");
    }
    return ret;
}
void VIChannel::DoMODThread::stopThread()
{
    stopMODDetect();
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeMOD_Exit;
    mMsgQueue.queueMessage(&msg);
    join();
    mMsgQueue.flushMessage();
}
status_t VIChannel::DoMODThread::getMODParams(MOTION_DETECT_ATTR_S *pParamMD)
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mMODDetectLock);
	pParamMD->nSensitivity = m_Sensitivity;

    return ret;
}

status_t VIChannel::DoMODThread::setMODParams(MOTION_DETECT_ATTR_S pParamMD)
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mMODDetectLock);
    if(!mbMODDetectEnable)
    {
    	m_Sensitivity = pParamMD.nSensitivity;
    }
    else
    {
        aloge("fatal error! refuse to set mod params during detect on!");
        ret = UNKNOWN_ERROR;
    }
    return ret;
}

status_t VIChannel::DoMODThread::startMODDetect()
{
    status_t eRet = NO_ERROR;
    #if(MPPCFG_MOTION_DETECT_SOFT == OPTION_MOTION_DETECT_SOFT_ENABLE)
    AutoMutex autoLock(mMODDetectLock);
    if(mbMODDetectEnable)
    {
        alogw("MOD detect already enable");
        return NO_ERROR;
    }

    //prepare eve face lib.
    //initilize
    m_width = mpViChn->mFrameWidth;
    m_height = mpViChn->mFrameHeight;
	m_awmd = allocAWMD(m_width, m_height, 16, 16, 5);
	m_MDvar = &m_awmd->variable;
	eRet = m_awmd->vpInit(m_MDvar, 6);
	if( eRet != NO_ERROR)
	{
		aloge("fatal error, vpInit failed!");
		eRet = UNKNOWN_ERROR;
		goto _err0;
	}

	unsigned char *p_mask;
	p_mask = (unsigned char*)malloc(m_width*m_height);
	memset(p_mask, 255, m_width*m_height);

	eRet = m_awmd->vpSetROIMask(m_MDvar, p_mask, m_width, m_height);
	if( eRet != NO_ERROR)
	{
		free(p_mask);
		p_mask = NULL;
		aloge("fatal error, vpSetROIMask failed!");
		eRet = UNKNOWN_ERROR;
		goto _err0;
	}
	m_awmd->vpSetSensitivityLevel(m_MDvar, m_Sensitivity);
	if( eRet != NO_ERROR)
	{
		free(p_mask);
		p_mask = NULL;
		aloge("fatal error, vpSetSensitivityLevel failed!");
		eRet = UNKNOWN_ERROR;
		goto _err0;
	}

	m_awmd->vpSetShelterPara(m_MDvar, 0, 0);
	if( eRet != NO_ERROR)
	{
		free(p_mask);
		p_mask = NULL;
		aloge("fatal error, vpSetShelterPara failed!");
		eRet = UNKNOWN_ERROR;
		goto _err0;
	}
	
    //notify faceDetect thread to detectOn.
    {
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeMOD_DetectOn;
    mMsgQueue.queueMessage(&msg);
    while(!mbMODDetectEnable)
    {
        mCondMODDetect.wait(mMODDetectLock);
    }
    }
    return NO_ERROR;
_err0:
    #else
    alogd("motion detect is disable\n");
    #endif
    return eRet;
}

status_t VIChannel::DoMODThread::stopMODDetect()
{
    status_t eRet = NO_ERROR;
    #if(MPPCFG_MOTION_DETECT_SOFT == OPTION_MOTION_DETECT_SOFT_ENABLE)
    AutoMutex autoLock(mMODDetectLock);
    if(!mbMODDetectEnable)
    {
        alogw("MOD already disable");
        return NO_ERROR;
    }
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeMOD_DetectOff;
    mMsgQueue.queueMessage(&msg);
    while(mbMODDetectEnable)
    {
        mCondMODDetect.wait(mMODDetectLock);
    }

    // algo terminate
	if( m_awmd )
	{
		free(m_awmd);
		m_awmd = NULL;
	}
    #else 
    alogd("motion detect is disable\n");
    #endif
    return eRet;
}

bool VIChannel::DoMODThread::IsMODDetectEnable()
{
    AutoMutex autoLock(mMODDetectLock);
    return mbMODDetectEnable;
}
status_t VIChannel::DoMODThread::sendFrame(VIDEO_FRAME_BUFFER_S *pFrmBuf)
{
    status_t ret;
    AutoMutex lock(mMODDetectLock);
    if(false == mbMODDetectEnable)
    {
        alogw("don't send frame when MOD disable!");
        return INVALID_OPERATION;
    }

    ret = mpMODFrameQueue->PutElemDataValid((void*)pFrmBuf);
    if (NO_ERROR != ret)
    {
        return ret;
    }

    AutoMutex lock2(mWaitLock);
    if(mbWaitFrame)
    {
        mbWaitFrame = false;
        EyeseeMessage msg;
        msg.mMsgType = MsgTypeMOD_InputFrameAvailable;
        mMsgQueue.queueMessage(&msg);
    }
    return NO_ERROR;
}
bool VIChannel::DoMODThread::threadLoop()
{
    if(!exitPending())
    {
        return MODThread();
    } 
    else
    {
        return false;
    }
}

bool VIChannel::DoMODThread::MODThread()
{
    bool bRunningFlag = true;
    EyeseeMessage msg;
    status_t getMsgRet;
    ERRORTYPE ret;
    VIDEO_FRAME_BUFFER_S *pFrmbuf;
    while(1)
    {
    PROCESS_MESSAGE:
        getMsgRet = mMsgQueue.dequeueMessage(&msg);
        if(getMsgRet == NO_ERROR)
        {
            if(MsgTypeMOD_DetectOn == msg.mMsgType)
            {
                AutoMutex autoLock(mMODDetectLock);
                if(!mbMODDetectEnable)
                {
                    if(mpMODFrameQueue)
                    {
                        aloge("fatal error! why pQueue is not null?");
                        delete mpMODFrameQueue;
                    }
                    mpMODFrameQueue = new EyeseeQueue();
                    mFrameCounter = 0;
                    mbMODDetectEnable = true;
                }
                else
                {
                    alogw("already enable MOD");
                }
                mCondMODDetect.signal();
            }
            else if(MsgTypeMOD_DetectOff == msg.mMsgType)
            {
                AutoMutex autoLock(mMODDetectLock);
                if(mbMODDetectEnable)
                {
                    releaseAllFrames();
                    delete mpMODFrameQueue;
                    mpMODFrameQueue = NULL;
                    mbMODDetectEnable = false;
                }
                else
                {
                    alogw("already disable MOD");
                    if(mpMODFrameQueue)
                    {
                        aloge("fatal error! why MOD frame queue is not null?");
                        delete mpMODFrameQueue;
                        mpMODFrameQueue = NULL;
                    }
                }
                mCondMODDetect.signal();
            }
            else if(MsgTypeMOD_InputFrameAvailable == msg.mMsgType)
            {
                //alogv("MOD frame input");
            }
            else if(MsgTypeMOD_Exit == msg.mMsgType)
            {
                AutoMutex autoLock(mMODDetectLock);
                if(mbMODDetectEnable)
                {
                    aloge("fatal error! must stop MOD before exit!");
                    mbMODDetectEnable = false;
                }
                if(mpMODFrameQueue)
                {
                    releaseAllFrames();
                    delete mpMODFrameQueue;
                    mpMODFrameQueue = NULL;
                }
                bRunningFlag = false;
                goto _exit0;
            }
            else
            {
                aloge("fatal error! unknown msg[0x%x]!", msg.mMsgType);
            }
            goto PROCESS_MESSAGE;
        }

        if(mbMODDetectEnable)
        {
            pFrmbuf = (VIDEO_FRAME_BUFFER_S*)mpMODFrameQueue->GetValidElemData();
            if (pFrmbuf == NULL) 
            {
                {
                    AutoMutex lock(mWaitLock);
                    if(mpMODFrameQueue->GetValidElemNum() > 0)
                    {
                        alogd("Low probability! MOD new frame come before check again.");
                        goto PROCESS_MESSAGE;
                    }
                    else
                    {
                        mbWaitFrame = true;
                    }
                }
                mMsgQueue.waitMessage();
                goto PROCESS_MESSAGE;
            }
    #if(MPPCFG_MOTION_DETECT_SOFT == OPTION_MOTION_DETECT_SOFT_ENABLE)
#ifdef TIME_TEST
            int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start timer
#endif
            // video frame time
            mFrameCounter++;
            // process image
            MotionDetectResult result;
            result.nResult = m_awmd->vpRun((unsigned char *)(pFrmbuf->mFrameBuf.VFrame.mpVirAddr[0]), m_MDvar);
#ifdef TIME_TEST
            int64_t nEndTm = CDX_GetSysTimeUsMonotonic();
            alogv("CVE_DTCA_Process:time used:%d us", nEndTm - nStartTm);
#endif
			std::shared_ptr<CMediaMemory> spMem = std::make_shared<CMediaMemory>(sizeof(MotionDetectResult));
			memcpy(spMem->getPointer(), &result, sizeof(MotionDetectResult));
			mpViChn->mpCallbackNotifier->postMODData(spMem);

            mpViChn->releaseFrame(pFrmbuf->mFrameBuf.mId);
            mpMODFrameQueue->ReleaseElemData(pFrmbuf); //release to idle list
         #else
            aloge("motion detect is disable\n");
            return false;
         #endif
        }
        else
        {
            mMsgQueue.waitMessage();
        }
    }
_exit0:
    return bRunningFlag;
}

status_t VIChannel::DoMODThread::releaseAllFrames()
{
    VIDEO_FRAME_BUFFER_S *pFrmbuf;
    while(1)
    {
        pFrmbuf = (VIDEO_FRAME_BUFFER_S*)mpMODFrameQueue->GetValidElemData();
        if(pFrmbuf)
        {
            mpViChn->releaseFrame(pFrmbuf->mFrameBuf.mId);
            mpMODFrameQueue->ReleaseElemData(pFrmbuf); //release to idle list
        }
        else
        {
            break;
        }
    }
    return NO_ERROR;
}

VIChannel::DoCommandThread::DoCommandThread(VIChannel *pViChn)
    : mpViChn(pViChn)
{
}

status_t VIChannel::DoCommandThread::startThread()
{
    status_t ret = run("VIChnCommand");
    if(ret != NO_ERROR)
    {
        aloge("fatal error! run thread fail!");
    }
    return ret;
}

void VIChannel::DoCommandThread::stopThread()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCommand_Exit;
    mMsgQueue.queueMessage(&msg);
    join();
    mMsgQueue.flushMessage();
}

bool VIChannel::DoCommandThread::threadLoop()
{
    if(!exitPending()) 
    {
        return mpViChn->commandThread();
    } 
    else 
    {
        return false;
    }
}

status_t VIChannel::DoCommandThread::SendCommand_TakePicture(unsigned int msgType)
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCommand_TakePicture;
    msg.mPara0 = msgType;
    mMsgQueue.queueMessage(&msg);
    return NO_ERROR;
}

status_t VIChannel::DoCommandThread::SendCommand_CancelContinuousPicture()
{
    EyeseeMessage msg;
    msg.mMsgType = MsgTypeCommand_CancelContinuousPicture;
    mMsgQueue.queueMessage(&msg);
    return NO_ERROR;
}

VIChannel::VIChannel(ISP_DEV nIspDevId, int chnId, bool bForceRef)
    : mbForceRef(bForceRef)
    , mChnId(chnId)
    , mpPreviewQueue(NULL)
    , mpPictureQueue(NULL)
    //, mpVideoFrmBuffer(NULL)
    , mChannelState(VI_CHN_STATE_CONSTRUCTED)
    , mIsPicCopy(false)
    , mContinuousPictureCnt(0)
    , mContinuousPictureMax(0)
    , mContinuousPictureStartTime(0)
    , mContinuousPictureLast(0)
    , mContinuousPictureInterval(0)
    , mColorSpace(0)
    , mFrameWidth(0)
    , mFrameHeight(0)
    , mFrameWidthOut(0)
    , mFrameHeightOut(0)
    , mCamDevTimeoutCnt(0)
    , PicCapModeEn(0)
    , FrmDrpThrForPicCapMode(3)
    , FrmDrpCntForPicCapMode(0)
{
    alogv("Construct");
    mIspDevId = nIspDevId;
    memset(&mRectCrop, 0, sizeof(mRectCrop));

    mpMemOps = MemAdapterGetOpsS();
    if (CdcMemOpen(mpMemOps) < 0) {
        aloge("CdcMemOpen failed!");
    }
    alogv("CdcMemOpen ok");

    mLastZoom = -1;
    mNewZoom = 0;
    mMaxZoom = 10;
    mbPreviewEnable = true;
    mbKeepPictureEncoder = false;
    
    mpCallbackNotifier = new CallbackNotifier(mChnId, this);
    mpPreviewWindow = new PreviewWindow(this);

    mpCapThread = new DoCaptureThread(this);
    mpCapThread->startThread();

    mpPrevThread = new DoPreviewThread(this);
    mpPrevThread->startThread();

    mpPicThread = new DoPictureThread(this);
    mpPicThread->startThread();

    mpCommandThread = new DoCommandThread(this);
    mpCommandThread->startThread();
#if(MPPCFG_MOTION_DETECT_SOFT == OPTION_MOTION_DETECT_SOFT_ENABLE)
    mpMODThread = new DoMODThread(this);
    mpMODThread->startThread();
#else
    mpMODThread = NULL;
#endif
    
    mTakePicMsgType = 0;
    mbTakePictureStart = false;
    std::vector<CameraParameters::SensorParamSet> SensorParamSets;
    SensorParamSets.emplace_back(3840, 2160, 15);
    SensorParamSets.emplace_back(3840, 2160, 25);
    SensorParamSets.emplace_back(3840, 2160, 30);
    SensorParamSets.emplace_back(3840, 2160, 50);
    SensorParamSets.emplace_back(3840, 2160, 60);
    SensorParamSets.emplace_back(2160, 2160, 30);
    SensorParamSets.emplace_back(1920, 1080, 30);
    SensorParamSets.emplace_back(1920, 1080, 60);
    SensorParamSets.emplace_back(1920, 1080, 120);
    SensorParamSets.emplace_back(1280, 720, 180);
    SensorParamSets.emplace_back(1280, 540, 240);
    mParameters.setSupportedSensorParamSets(SensorParamSets);
    mParameters.setPreviewFrameRate(SensorParamSets[0].mFps);
    SIZE_S size={(unsigned int)SensorParamSets[0].mWidth, (unsigned int)SensorParamSets[0].mHeight};
    mParameters.setVideoSize(size);
    mParameters.setPreviewFormat(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
    mParameters.setColorSpace(V4L2_COLORSPACE_JPEG);
    mParameters.setVideoBufferNumber(5);
    //preview rotation setting
    mParameters.setPreviewRotation(0);
    mParameters.setPictureMode(TAKE_PICTURE_MODE_FAST);
    //digital zoom setting
    mParameters.setZoomSupported(true);
    mParameters.setZoom(mNewZoom);
    mParameters.setMaxZoom(mMaxZoom);
}

VIChannel::~VIChannel()
{
    alogv("Destruct");
    if (mpCommandThread != NULL) 
    {
        mpCommandThread->stopThread();
        delete mpCommandThread;
    }
    if (mpCapThread != NULL)
    {
        mpCapThread->stopThread();
        delete mpCapThread;
    }
    if (mpPrevThread != NULL)
    {
        mpPrevThread->stopThread();
        delete mpPrevThread;
    }
    if (mpPicThread != NULL)
    {
        mpPicThread->stopThread();
        delete mpPicThread;
    }
    if (mpMODThread != NULL)
    {
        mpMODThread->stopThread();
        delete mpMODThread;
    }

    if (mpPreviewWindow != NULL) {
        delete mpPreviewWindow;
    }
    if (mpCallbackNotifier != NULL) {
        delete mpCallbackNotifier;
    }

//    if (mpVideoFrmBuffer != NULL) {
//        free(mpVideoFrmBuffer);
//    }
    mFrameBuffers.clear();
    mFrameBufferIdList.clear();
    if (mpPreviewQueue != NULL) {
        OSAL_QueueTerminate(mpPreviewQueue);
        delete mpPreviewQueue;
    }
    if (mpPictureQueue != NULL) {
        OSAL_QueueTerminate(mpPictureQueue);
        delete mpPictureQueue;
    }

    CdcMemClose(mpMemOps);
}

void VIChannel::calculateCrop(RECT_S *rect, int zoom, int width, int height)
{
    rect->X = (width - width * 10 / (10 + zoom)) / 2;
    rect->Y = (height - height * 10 / (10 + zoom)) / 2;
    rect->Width = width - rect->X * 2;
    rect->Height = height - rect->Y * 2;
}

int VIChannel::planeNumOfV4l2PixFmt(int nV4l2PixFmt)
{
    int nPlaneNum;
    switch(nV4l2PixFmt)
    {
        case V4L2_PIX_FMT_NV12M:
        case V4L2_PIX_FMT_NV21M:
            nPlaneNum = 2;
            break;
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YUV420M:
        case V4L2_PIX_FMT_YVU420M:
            nPlaneNum = 3;
            break;
        default:
            aloge("Unknown V4l2Pixelformat[0x%x].", nV4l2PixFmt);
            nPlaneNum = 1;
            break;
    }
    return nPlaneNum;
}

status_t VIChannel::prepare()
{
    AutoMutex lock(mLock);

    if (mChannelState != VI_CHN_STATE_CONSTRUCTED) 
    {
        aloge("prepare in error state %d", mChannelState);
        return INVALID_OPERATION;
    }
    alogv("Vipp dev[%d] chn[0]", mChnId);
    ERRORTYPE eRet = AW_MPI_VI_CreateVipp(mChnId);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI CreateVipp failed");
        return UNKNOWN_ERROR;
    }

    mChannelState = VI_CHN_STATE_PREPARED;
    return OK;
}

status_t VIChannel::release()
{
    AutoMutex lock(mLock);

    if (mChannelState != VI_CHN_STATE_PREPARED) 
    {
        aloge("release in error state %d", mChannelState);
        return INVALID_OPERATION;
    }
    ERRORTYPE ret = AW_MPI_VI_DestoryVipp(mChnId);
    if(ret != SUCCESS)
    {
        aloge("fatal error! DestroyVipp fail!");
    }
    mChannelState = VI_CHN_STATE_CONSTRUCTED;
    return NO_ERROR;
}

status_t VIChannel::startChannel()
{
    AutoMutex lock1(mLock);

    if (mChannelState != VI_CHN_STATE_PREPARED)
    {
        aloge("startChannel in error state %d", mChannelState);
        return INVALID_OPERATION;
    }
	
	VI_ATTR_S attr;
    memset(&attr, 0, sizeof(VI_ATTR_S));
    attr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    attr.memtype = V4L2_MEMORY_MMAP;
    attr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(mParameters.getPreviewFormat());
    attr.format.field = V4L2_FIELD_NONE;
    attr.format.colorspace = mParameters.getColorSpace();
    SIZE_S size;
    mParameters.getVideoSize(size);
    attr.format.width = size.Width;
    attr.format.height = size.Height;
    attr.nbufs = mParameters.getVideoBufferNumber();
    attr.nplanes = 0;
    attr.fps = mParameters.getPreviewFrameRate();
    attr.capturemode = mParameters.getCaptureMode();
    attr.use_current_win = mbForceRef?0:1;
#ifdef PRODUCT_V5SDV
    attr.drop_frame_num = 6;
#endif
    //check sensor param valid.
    if(mbForceRef)
    {
        bool bMatch = false;
        CameraParameters::SensorParamSet userParam = {(int)attr.format.width, (int)attr.format.height, (int)attr.fps};
        std::vector<CameraParameters::SensorParamSet> supportedSensorParamSets;
        mParameters.getSupportedSensorParamSets(supportedSensorParamSets);
        for(CameraParameters::SensorParamSet &i : supportedSensorParamSets)
        {
            if(userParam == i)
            {
                bMatch = true;
                break;
            }
        }
        if(!bMatch)
        {
            alogw("Be careful! not find match sensor param set! isp param will be selected near user param!");
        }
        alogv("set user sensor param: vipp[%d], size[%dx%d], fps[%d]", mChnId, userParam.mWidth, userParam.mHeight, userParam.mFps);
    }
    alogd("set vipp[%d]attr:[%dx%d],fps[%d], pixFmt[0x%x], nbufs[%d]", mChnId, attr.format.width, attr.format.height, attr.fps, attr.format.pixelformat, attr.nbufs);

    ERRORTYPE eRet = AW_MPI_VI_SetVippAttr(mChnId, &attr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI SetVippAttr failed");
    }
    eRet = AW_MPI_VI_GetVippAttr(mChnId, &attr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI GetVippAttr failed");
    }
    //use VI_ATTR_S to init CameraParameters.
    alogd("get vipp[%d]attr:[%dx%d],fps[%d], pixFmt[0x%x], nbufs[%d]", mChnId, attr.format.width, attr.format.height, attr.fps, attr.format.pixelformat, attr.nbufs);
    size.Width = attr.format.width;
    size.Height = attr.format.height;
    mParameters.setVideoSize(size);
    mParameters.setPreviewFrameRate(attr.fps);
    mParameters.setPreviewFormat(map_V4L2_PIX_FMT_to_PIXEL_FORMAT_E(attr.format.pixelformat));
    mParameters.setColorSpace((enum v4l2_colorspace)attr.format.colorspace);
    mParameters.setVideoBufferNumber(attr.nbufs);

    //mirror
    int nMirror = mParameters.GetMirror();
    alogv("set vipp mirror[%d]", nMirror);
    AW_MPI_VI_SetVippMirror(mChnId, nMirror);
    //Flip
    int nFlip = mParameters.GetFlip();
    alogv("set flip[%d]", nFlip);
    AW_MPI_VI_SetVippFlip(mChnId, nFlip);
    
    //preview rotation setting
    mpPreviewWindow->setDisplayFrameRate(mParameters.getDisplayFrameRate());
    mpPreviewWindow->setPreviewRotation(mParameters.getPreviewRotation());
    mpPreviewWindow->setDispBufferNum(2);
    //digital zoom setting
    mLastZoom = -1;
    if(mParameters.isZoomSupported())
    {
        mNewZoom = mParameters.getZoom();
        mMaxZoom = mParameters.getMaxZoom();
    }

    mColorSpace = attr.format.colorspace;
    mFrameWidth = attr.format.width;
    mFrameHeight = attr.format.height;

    SIZE_S sizeOut;
    mParameters.getVideoSizeOut(sizeOut);
    mFrameWidthOut = sizeOut.Width;
    mFrameHeightOut = sizeOut.Height;
	
    //create resources
    int nBufNum = mParameters.getVideoBufferNumber();
//    if (mpVideoFrmBuffer == NULL)
//    {
//        mpVideoFrmBuffer = (VIDEO_FRAME_BUFFER_S*)malloc(sizeof(VIDEO_FRAME_BUFFER_S) * nBufNum);
//        if (mpVideoFrmBuffer == NULL) 
//        {
//            aloge("fatal error! alloc mpVideoFrmBuffer error!");
//            return NO_MEMORY;
//        }
//    }
    if(!mFrameBuffers.empty())
    {
        aloge("fatal error! why frame buffers is not empty?");
        mFrameBuffers.clear();
    }
    if(!mFrameBufferIdList.empty())
    {
        aloge("fatal error! why bufferIdList is not empty?");
        mFrameBufferIdList.clear();
    }

    if (mpPreviewQueue == NULL) 
    {
        mpPreviewQueue = new OSAL_QUEUE;
    } 
    else 
    {
        OSAL_QueueTerminate(mpPreviewQueue);
    }
    OSAL_QueueCreate(mpPreviewQueue, nBufNum);

    if (mpPictureQueue == NULL) 
    {
        mpPictureQueue = new OSAL_QUEUE;
    } 
    else 
    {
        OSAL_QueueTerminate(mpPictureQueue);
    }
    OSAL_QueueCreate(mpPictureQueue, nBufNum);
    
    alogv("startChannel");
    ERRORTYPE ret = AW_MPI_VI_EnableVipp(mChnId);
    if(ret != SUCCESS)
    {
        aloge("fatal error! enableVipp fail!");
    }
    ret = AW_MPI_VI_CreateVirChn(mChnId, 0, NULL);
    if(ret != SUCCESS)
    {
        aloge("fatal error! createVirChn fail!");
    }
    ret = AW_MPI_VI_EnableVirChn(mChnId, 0);
    if(ret != SUCCESS)
    {
        aloge("fatal error! enableVirChn fail!");
    }
    if(mbPreviewEnable)
    {
        mpPreviewWindow->startPreview();
    }
    mpCapThread->startCapture();
    mDbgFrameNum = 0;
    mDbgFrameFilePathList.clear();
    mFrameCounter = 0;
    mChannelState = VI_CHN_STATE_STARTED;
    return NO_ERROR;
}

status_t VIChannel::stopChannel()
{
    AutoMutex lock1(mLock);

    if (mChannelState != VI_CHN_STATE_STARTED)
    {
        aloge("stopChannel in error state %d", mChannelState);
        return INVALID_OPERATION;
    }

    alogv("stopChannel");
    if(mpMODThread)
    {
        mpMODThread->stopMODDetect();
    }
    mpPreviewWindow->stopPreview();
    mpCapThread->pauseCapture();
    mpPrevThread->releaseAllFrames();
    mpPicThread->releaseAllFrames();

    ERRORTYPE ret;
    ret = AW_MPI_VI_DisableVirChn(mChnId, 0);
    if(ret != SUCCESS)
    {
        aloge("fatal error! disableVirChn fail!");
    }
    ret = AW_MPI_VI_DestoryVirChn(mChnId, 0);
    if(ret != SUCCESS)
    {
        aloge("fatal error! destroyVirChn fail!");
    }
    ret = AW_MPI_VI_DisableVipp(mChnId);
    if(ret != SUCCESS)
    {
        aloge("fatal error! disableVipp fail!");
    }
    //destroy resources
//    if (mpVideoFrmBuffer != NULL) 
//    {
//        free(mpVideoFrmBuffer);
//        mpVideoFrmBuffer = NULL;
//    }
    mFrameBuffers.clear();
    if(!mFrameBufferIdList.empty())
    {
        aloge("fatal error! why bufferIdList is not empty?");
        mFrameBufferIdList.clear();
    }
    if (mpPreviewQueue != NULL) 
    {
        OSAL_QueueTerminate(mpPreviewQueue);
        delete mpPreviewQueue;
        mpPreviewQueue = NULL;
    }
    if (mpPictureQueue != NULL) 
    {
        OSAL_QueueTerminate(mpPictureQueue);
        delete mpPictureQueue;
        mpPictureQueue = NULL;
    }
    
    mChannelState = VI_CHN_STATE_PREPARED;
    return NO_ERROR;
}

VIChannel::VIChannelState VIChannel::getState()
{
    AutoMutex lock(mLock);
    return mChannelState;
}
status_t VIChannel::getMODParams(MOTION_DETECT_ATTR_S *pParamMD)
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mLock);
    if(mpMODThread)
    {
        ret = mpMODThread->getMODParams(pParamMD);
    }
    else
    {
        aloge("fatal error! mod thread is null");
        ret = UNKNOWN_ERROR;
    }
    return ret;
}

status_t VIChannel::setMODParams(MOTION_DETECT_ATTR_S pParamMD)
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mLock);
    if(mpMODThread)
    {
        ret = mpMODThread->setMODParams(pParamMD);
    }
    else
    {
        ret = UNKNOWN_ERROR;
    }
    return ret;
}

status_t VIChannel::startMODDetect()
{
    AutoMutex autoLock(mLock);
    if (mChannelState != VI_CHN_STATE_STARTED) 
    {
        aloge("start Motion Object Detection in error state %d", mChannelState);
        return INVALID_OPERATION;
    }

    if(mpMODThread)
    {
        return mpMODThread->startMODDetect();
    }
    else
    {
        return INVALID_OPERATION;
    }
}

status_t VIChannel::stopMODDetect()
{
    AutoMutex autoLock(mLock);
    if (mChannelState != VI_CHN_STATE_STARTED)
    {
        aloge("stop Motion Object Detection in error state %d", mChannelState);
        return INVALID_OPERATION;
    }
    if(mpMODThread)
    {
        return mpMODThread->stopMODDetect();
    }
    else
    {
        return INVALID_OPERATION;
    }
}

bool VIChannel::isPreviewEnabled()
{
    return mpPreviewWindow->isPreviewEnabled();
}

status_t VIChannel::startRender()
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mLock);
    mbPreviewEnable = true;
    if (VI_CHN_STATE_STARTED == mChannelState)
    {
        ret = mpPreviewWindow->startPreview();
    }
    return ret;
}

status_t VIChannel::stopRender()
{
    status_t ret = NO_ERROR;
    AutoMutex autoLock(mLock);
    mbPreviewEnable = false;
    ret = mpPreviewWindow->stopPreview();
    return ret;
}

void VIChannel::setPicCapMode(uint32_t mode)
{
    int ret; 
    ret = AW_MPI_ISP_SetScene(mIspDevId,mode);
    if(mode == 0 && SUCCESS == ret)
    {
        PicCapModeEn = 1; 
    }else{
        PicCapModeEn = 0;
    }
}

void VIChannel::setFrmDrpThrForPicCapMode(uint32_t frm_cnt)
{
    FrmDrpThrForPicCapMode = frm_cnt;
} 

status_t VIChannel::storeDisplayFrame(uint64_t framePts)
{
    return mpPreviewWindow->storeDisplayFrame(framePts);
}

status_t VIChannel::releaseFrame(uint32_t index)
{
    if (index >= (uint32_t)mParameters.getVideoBufferNumber()) 
    {
        aloge("Fatal error! invalid buffer index %d!", index);
        return BAD_VALUE;
    }
//    if(NULL == mpVideoFrmBuffer)
//    {
//        aloge("fatal error! mpVideoFrmBuffer == NULL!");
//    }
//    VIDEO_FRAME_BUFFER_S *pFrmbuf = mpVideoFrmBuffer + index;

    VIDEO_FRAME_BUFFER_S *pFrmbuf = NULL;
    AutoMutex bufLock(mFrameBuffersLock);
    for(std::list<VIDEO_FRAME_BUFFER_S>::iterator it=mFrameBuffers.begin(); it!=mFrameBuffers.end(); ++it)
    {
        if(it->mFrameBuf.mId == index)
        {
            if(NULL == pFrmbuf)
            {
                pFrmbuf = &*it;
            }
            else
            {
                aloge("fatal error! VI frame id[0x%x] is repeated!", index);
            }
        }
    }
    if(NULL == pFrmbuf)
    {
        aloge("fatal error! not find frame index[0x%x]", index);
        return UNKNOWN_ERROR;
    }

    
    int ret;

    AutoMutex lock(mRefCntLock);

    if (pFrmbuf->mRefCnt > 0 && --pFrmbuf->mRefCnt == 0) 
    {
        if(MM_PIXEL_FORMAT_YUV_AW_AFBC == pFrmbuf->mFrameBuf.VFrame.mPixelFormat)
        {
            //alogd("vipp[%d] afbc clear %dByte", mChnId, pFrmbuf->mFrameBuf.VFrame.mStride[0]);
            //memset(pFrmbuf->mFrameBuf.VFrame.mpVirAddr[0], 0x0, 200*1024);  //200*1024, pFrmbuf->mFrameBuf.VFrame.mStride[0]
        }
        else if(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pFrmbuf->mFrameBuf.VFrame.mPixelFormat)
        {
            //alogd("vipp[%d] afbc clear first 200KB", mChnId);
            //memset(pFrmbuf->mFrameBuf.VFrame.mpVirAddr[0]+200*1024, 0, 200*1024);
        }
        ret = AW_MPI_VI_ReleaseFrame(mChnId, 0, &pFrmbuf->mFrameBuf);
        if (ret != SUCCESS) 
        {
            aloge("fatal error! AW_MPI_VI ReleaseFrame error!");
        }
        int nCount = 0;
        for (std::list<unsigned int>::iterator it = mFrameBufferIdList.begin(); it!=mFrameBufferIdList.end();)
        {
            if(*it == pFrmbuf->mFrameBuf.mId)
            {
                if(0 == nCount)
                {
                    it = mFrameBufferIdList.erase(it);
                }
                else
                {
                    ++it;
                }
                nCount++;
            }
            else
            {
                ++it;
            }
        }
        if(nCount != 1)
        {
            aloge("fatal error! vipp[%d] frame buffer id list is wrong! dstId[%d], find[%d]", mChnId, pFrmbuf->mFrameBuf.mId, nCount);
            alogd("current frame buffer id list elem number:%d", mFrameBufferIdList.size());
            for (std::list<unsigned int>::iterator it = mFrameBufferIdList.begin(); it!=mFrameBufferIdList.end(); ++it)
            {
                alogd("bufid[%d]", *it);
            }
        }
    }
    return NO_ERROR;
}

status_t VIChannel::setParameters(CameraParameters &param)
{
    AutoMutex lock(mLock);
    if(VI_CHN_STATE_CONSTRUCTED == mChannelState)
    {
        mParameters = param;
        return NO_ERROR;
    }
    if (mChannelState != VI_CHN_STATE_PREPARED && mChannelState != VI_CHN_STATE_STARTED)
    {
        alogw("call in wrong channel state[0x%x]", mChannelState);
        //mParameters = param;
        return INVALID_OPERATION;
    }
    //detect param change, and update.
    bool bAttrUpdate = false;
    VI_ATTR_S attr;
    ERRORTYPE ret;
    ret = AW_MPI_VI_GetVippAttr(mChnId, &attr);
    if(ret!=SUCCESS)
    {
        aloge("fatal error! getVippAttr fail");
    }
    attr.use_current_win = mbForceRef?0:1;
    int oldCaptureMode, newCaptureMode;
    oldCaptureMode = mParameters.getCaptureMode();
    newCaptureMode = param.getCaptureMode();
    if(oldCaptureMode != newCaptureMode)
    {
        alogd("paramCaptureMode change[0x%x]->[0x%x]", oldCaptureMode, newCaptureMode);
        bAttrUpdate = true;
        attr.capturemode = newCaptureMode;
    }
    SIZE_S oldParamSize, newParamSize;
    mParameters.getVideoSize(oldParamSize);
    param.getVideoSize(newParamSize);
    if(oldParamSize.Width!=newParamSize.Width || oldParamSize.Height!=newParamSize.Height)
    {
        alogd("paramVideoSize change[%dx%d]->[%dx%d]", oldParamSize.Width, oldParamSize.Height, newParamSize.Width, newParamSize.Height);
        bAttrUpdate = true;
        attr.format.width = newParamSize.Width;
        attr.format.height = newParamSize.Height;
        mFrameWidth = attr.format.width;
        mFrameHeight = attr.format.height;
    }
    PIXEL_FORMAT_E oldFormat, newFormat;
    oldFormat = mParameters.getPreviewFormat();
    newFormat = param.getPreviewFormat();
    if(oldFormat!=newFormat)
    {
        alogd("paramPixelFormat change[0x%x]->[0x%x]", oldFormat, newFormat);
        bAttrUpdate = true;
        attr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(newFormat);
    }
    enum v4l2_colorspace oldColorspace, newColorspace;
    oldColorspace = mParameters.getColorSpace();
    newColorspace = param.getColorSpace();
    if(oldColorspace != newColorspace)
    {
        alogd("paramColorSpace change[0x%x]->[0x%x]", oldColorspace, newColorspace);
        bAttrUpdate = true;
        attr.format.colorspace = newColorspace;
        mColorSpace = attr.format.colorspace;
    }
    int oldBufNum, newBufNum;
    oldBufNum = mParameters.getVideoBufferNumber();
    newBufNum = param.getVideoBufferNumber();
    if(oldBufNum != newBufNum)
    {
        alogd("paramBufNum change[%d]->[%d]", oldBufNum, newBufNum);
        bAttrUpdate = true;
        attr.nbufs = newBufNum;
    }
    int oldFrameRate, newFrameRate;
    oldFrameRate = mParameters.getPreviewFrameRate();
    newFrameRate = param.getPreviewFrameRate();
    if(oldFrameRate != newFrameRate)
    {
        alogd("paramFrameRate change[%d]->[%d]", oldFrameRate, newFrameRate);
        bAttrUpdate = true;
        attr.fps = newFrameRate;
    }
    if(bAttrUpdate)
    {
        ret = AW_MPI_VI_SetVippAttr(mChnId, &attr);
        if(ret!=SUCCESS)
        {
            aloge("fatal error! setVippAttr fail");
        }
    }
    //mirror
    int oldMirror = mParameters.GetMirror();
    int newMirror = param.GetMirror();
    if(oldMirror != newMirror)
    {
        alogd("change mirror[%d]->[%d]", oldMirror, newMirror);
        AW_MPI_VI_SetVippMirror(mChnId, newMirror);
    }
    //Flip
    int oldFlip = mParameters.GetFlip();
    int newFlip = param.GetFlip();
    if(oldFlip != newFlip)
    {
        alogd("change flip[%d]->[%d]", oldFlip, newFlip);
        AW_MPI_VI_SetVippFlip(mChnId, newFlip);
    }
    //shut time
    VI_SHUTTIME_CFG_S oldShutTime, newShutTime;
    mParameters.getShutTime(oldShutTime);
    param.getShutTime(newShutTime);
    if(oldShutTime.eShutterMode != newShutTime.eShutterMode || oldShutTime.iTime != newShutTime.iTime)
    {
        alogd("change ShutTime config, shutmode[%d]->[%d], shuttime[%d]->[%d]", oldShutTime.eShutterMode, newShutTime.eShutterMode,
                                                                        oldShutTime.iTime, newShutTime.iTime);
        AW_MPI_VI_SetVippShutterTime(mChnId, &newShutTime);
    }

    //detect preview rotation, displayFrameRate
    int oldPreviewRotation, newPreviewRotation, curPreviewRotation;
    oldPreviewRotation = mParameters.getPreviewRotation();
    newPreviewRotation = param.getPreviewRotation();
    curPreviewRotation = oldPreviewRotation;
    if(oldPreviewRotation != newPreviewRotation)
    {
        if(newPreviewRotation!=0 && newPreviewRotation!=90 && newPreviewRotation!=180 && newPreviewRotation!=270 && newPreviewRotation!=360)
        {
            aloge("fatal error! new rotation[%d] is invalid!", newPreviewRotation);
        }
        else
        {
            alogd("paramPreviewRotation change[%d]->[%d]", oldPreviewRotation, newPreviewRotation);
            curPreviewRotation = newPreviewRotation;
            mpPreviewWindow->setPreviewRotation(newPreviewRotation);
        }
    }
    int oldDisplayFrameRate, newDisplayFrameRate;
    oldDisplayFrameRate = mParameters.getDisplayFrameRate();
    newDisplayFrameRate = param.getDisplayFrameRate();
    if(oldDisplayFrameRate != newDisplayFrameRate)
    {
        alogd("paramDisplayFrameRate change[%d]->[%d]", oldDisplayFrameRate, newDisplayFrameRate);
        mpPreviewWindow->setDisplayFrameRate(newDisplayFrameRate);
    }

    //detect digital zoom params
    int oldZoom, newZoom;
    oldZoom = mParameters.getZoom();
    newZoom = param.getZoom();
    if(oldZoom != newZoom)
    {
        if(newZoom >=0 && newZoom <= mMaxZoom)
        {
            alogd("paramZoom change[%d]->[%d]", oldZoom, newZoom);
            mNewZoom = newZoom;
        }
        else
        {
            aloge("fatal error! zoom value[%d] is invalid, keep last!", newZoom);
        }
    }

    //set parameters
    mParameters = param;
    mParameters.setPreviewRotation(curPreviewRotation);
    mParameters.setZoom(mNewZoom);
    return NO_ERROR;
}

status_t VIChannel::getParameters(CameraParameters &param) const
{
    param = mParameters;
    return NO_ERROR;
}
/*
bool VIChannel::compareOSDRectInfo(const OSDRectInfo& first, const OSDRectInfo& second)
{
    if(first.mRect.Y < second.mRect.Y)
    {
        return true;
    }
    if(first.mRect.Y > second.mRect.Y)
    {
        return false;
    }
    if(first.mRect.X < second.mRect.X)
    {
        return true;
    }
    if(first.mRect.X > second.mRect.X)
    {
        return false;
    }
    return false;
}
*/
/**
 * VIPP: video input post process.
 * VIPP process osd rects in position order of lines. From top to bottom, left to right.
 * So must sort osd rects according above orders.
 */
 /*
status_t VIChannel::setOSDRects(std::list<OSDRectInfo> &rects)
{
    mOSDRects = rects;
    mOSDRects.sort(compareOSDRectInfo);
    return NO_ERROR;
}

status_t VIChannel::getOSDRects(std::list<OSDRectInfo> **ppRects)
{
    if(ppRects)
    {
        *ppRects = &mOSDRects;
    }
    return NO_ERROR;
}

status_t VIChannel::OSDOnOff(bool bOnOff)
{
    if(bOnOff)
    {
      //  aloge("OSDOnOff unknown osd type[0x%x]", elem.mType);
        VI_OsdMaskRegion stOverlayRegion;
        memset(&stOverlayRegion, 0, sizeof(VI_OsdMaskRegion));
        VI_OsdMaskRegion stCoverRegion;
        memset(&stCoverRegion, 0, sizeof(VI_OsdMaskRegion));
        int bufSize = 0;
        for(OSDRectInfo& elem : mOSDRects)
        {
            if(OSDType_Overlay == elem.mType)
            {
                stOverlayRegion.chromakey = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(elem.mFormat);
                stOverlayRegion.global_alpha = 16;
                stOverlayRegion.bitmap[stOverlayRegion.clipcount] = elem.GetBuffer();
                stOverlayRegion.region[stOverlayRegion.clipcount].left   = elem.mRect.X;
                stOverlayRegion.region[stOverlayRegion.clipcount].top    = elem.mRect.Y;
                stOverlayRegion.region[stOverlayRegion.clipcount].width  = elem.mRect.Width;
                stOverlayRegion.region[stOverlayRegion.clipcount].height = elem.mRect.Height;
                stOverlayRegion.clipcount++;
                bufSize += elem.GetBufferSize();
            }
            else if(OSDType_Cover == elem.mType)
            {
                stCoverRegion.chromakey = elem.mColor;
                stCoverRegion.global_alpha = 16;
                stCoverRegion.bitmap[stCoverRegion.clipcount] = NULL;
                stCoverRegion.region[stCoverRegion.clipcount].left   = elem.mRect.X;
                stCoverRegion.region[stCoverRegion.clipcount].top    = elem.mRect.Y;
                stCoverRegion.region[stCoverRegion.clipcount].width  = elem.mRect.Width;
                stCoverRegion.region[stCoverRegion.clipcount].height = elem.mRect.Height;
                stCoverRegion.clipcount++;
            }
            else
            {
                aloge("fatal error! unknown osd type[0x%x]", elem.mType);
            }
        }
        if(stOverlayRegion.clipcount > 0)
        {
            AW_MPI_VI_SetOsdMaskRegion(mChnId, &stOverlayRegion);
            AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);
        }
        else
        {
            VI_OsdMaskRegion stOverlayRegion;
            memset(&stOverlayRegion, 0, sizeof(VI_OsdMaskRegion));
            char bitmap[100];
            bitmap[0] = 'c';
            stOverlayRegion.clipcount = 0;
            stOverlayRegion.bitmap[0] = &bitmap[0];
            AW_MPI_VI_SetOsdMaskRegion(mChnId, &stOverlayRegion);
            AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);
        }
        if(stCoverRegion.clipcount > 0)
        {
            AW_MPI_VI_SetOsdMaskRegion(mChnId, &stCoverRegion);
            AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);
        }
        else
        {
            VI_OsdMaskRegion stCoverRegion;
            memset(&stCoverRegion, 0, sizeof(VI_OsdMaskRegion));
            stCoverRegion.clipcount = 0;
            stCoverRegion.bitmap[0] = NULL;
            AW_MPI_VI_SetOsdMaskRegion(mChnId, &stCoverRegion);
            AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);
        }
    }
    else
    {
        VI_OsdMaskRegion stOverlayRegion;
        memset(&stOverlayRegion, 0, sizeof(VI_OsdMaskRegion));
        VI_OsdMaskRegion stCoverRegion;
        memset(&stCoverRegion, 0, sizeof(VI_OsdMaskRegion));

        char bitmap[100];
        bitmap[0] = 'c';
        stOverlayRegion.clipcount = 0;
        stOverlayRegion.bitmap[0] = &bitmap[0];
        stOverlayRegion.chromakey = V4L2_PIX_FMT_RGB32;
        AW_MPI_VI_SetOsdMaskRegion(mChnId, &stOverlayRegion);
        AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);

        stCoverRegion.clipcount = 0;
        stCoverRegion.bitmap[0] = NULL;
        AW_MPI_VI_SetOsdMaskRegion(mChnId, &stCoverRegion);
        AW_MPI_VI_UpdateOsdMaskRegion(mChnId, 1);

    }
    return NO_ERROR;
}
*/
bool VIChannel::captureThread()
{
    bool bRunningFlag = true;
    bool bTakePictureStart = false;
    EyeseeMessage msg;
    status_t getMsgRet;
    ERRORTYPE ret;
    VIDEO_FRAME_INFO_S buffer;
    VIDEO_FRAME_BUFFER_S *pFrmbuf;
PROCESS_MESSAGE:
    getMsgRet = mpCapThread->mMsgQueue.dequeueMessage(&msg);
    if(getMsgRet == NO_ERROR)
    {
        if(DoCaptureThread::MsgTypeCapture_SetState == msg.mMsgType)
        {
            AutoMutex autoLock(mpCapThread->mStateLock);
            if(msg.mPara0 == mpCapThread->mCapThreadState)
            {
                aloge("fatal error! same state[0x%x]", mpCapThread->mCapThreadState);
            }
            else if(DoCaptureThread::CAPTURE_STATE_PAUSED == msg.mPara0)
            {
                alogv("VIchn captureThread state[0x%x]->paused", mpCapThread->mCapThreadState);
                mpCapThread->mCapThreadState = DoCaptureThread::CAPTURE_STATE_PAUSED;
                mpCapThread->mPauseCompleteCond.signal();
            }
            else if(DoCaptureThread::CAPTURE_STATE_STARTED == msg.mPara0)
            {
                alogv("VIchn captureThread state[0x%x]->started", mpCapThread->mCapThreadState);
                mpCapThread->mCapThreadState = DoCaptureThread::CAPTURE_STATE_STARTED;
                mpCapThread->mStartCompleteCond.signal();
            }
            else
            {
                aloge("fatal error! check code!");
            }
        }
        else if(DoCaptureThread::MsgTypeCapture_TakePicture == msg.mMsgType)
        {
            if(!bTakePictureStart)
            {
                bTakePictureStart = true;
            }
            else
            {
                alogd("Be careful! take picture is doing already!");
            }
            alogd("take picture mode is [0x%x]", mTakePictureMode);
            //set picture number to callbackNotifier
            int nPicNum = 0;
            switch(mTakePictureMode)
            {
                case TAKE_PICTURE_MODE_FAST:
                    nPicNum = 1;
                    break;
                case TAKE_PICTURE_MODE_CONTINUOUS:
                    nPicNum = mParameters.getContinuousPictureNumber();
                    break;
                default:
                    nPicNum = 1;
                    break;
            }
            mpCallbackNotifier->setPictureNum(nPicNum);
        }
        else if(DoCaptureThread::MsgTypeCapture_CancelContinuousPicture == msg.mMsgType)
        {
            if(!bTakePictureStart)
            {
                if(TAKE_PICTURE_MODE_CONTINUOUS == mTakePictureMode)
                {
                    mpPicThread->notifyPictureEnd();
                    mContinuousPictureStartTime = 0;
                    mContinuousPictureLast = 0;
                    mContinuousPictureInterval = 0;
                    mContinuousPictureCnt = 0;
                    mContinuousPictureMax = 0;
                    bTakePictureStart = false; 
                }
                else
                {
                    aloge("fatal error! take picture mode[0x%x] is not continuous!", mTakePictureMode);
                }
            }
            else
            {
                aloge("fatal error! not start take picture, mode[0x%x]!", mTakePictureMode);
            }
        }
        else if(DoCaptureThread::MsgTypeCapture_Exit == msg.mMsgType)
        {
            bRunningFlag = false;
            goto _exit0;
        }
        else
        {
            aloge("unknown msg[0x%x]!", msg.mMsgType);
        }
        goto PROCESS_MESSAGE;
    }
    
    if(mpCapThread->mCapThreadState == DoCaptureThread::CAPTURE_STATE_PAUSED)
    {
        mpCapThread->mMsgQueue.waitMessage();
        goto PROCESS_MESSAGE;
    }

    ret = AW_MPI_VI_GetFrame(mChnId, 0, &buffer, 500);
    if (ret != SUCCESS)
    {
        mFrameBuffersLock.lock();
        alogw("vipp[%d] channel contain [%d] frame buffers", mChnId, mFrameBufferIdList.size());
        mFrameBuffersLock.unlock();
        int preview_num = OSAL_GetElemNum(mpPreviewQueue);
        int picture_num = OSAL_GetElemNum(mpPictureQueue);
        alogw("vipp[%d]: preview_num: %d, picture_num: %d, timeout: 500ms", mChnId, preview_num, picture_num);
        mCamDevTimeoutCnt++;
        if (mCamDevTimeoutCnt%20 == 0)
        {
            aloge("timeout for %d times when VI_GetFrame!", mCamDevTimeoutCnt);
            mpCallbackNotifier->NotifyCameraDeviceTimeout();
        }
        //usleep(10*1000);
        //return true;
        goto PROCESS_MESSAGE;
    }

    if(PicCapModeEn && 3<=FrmDrpThrForPicCapMode && FrmDrpCntForPicCapMode <= FrmDrpThrForPicCapMode )
    {
        ret = AW_MPI_VI_ReleaseFrame(mChnId, 0, &buffer);
        if (ret != SUCCESS) 
        {
            aloge("fatal error! AW_MPI_VI ReleaseFrame error!");
        }
        FrmDrpCntForPicCapMode++;
        goto PROCESS_MESSAGE;
    }

    FrmDrpCntForPicCapMode = 0;
    mCamDevTimeoutCnt = 0;
#ifdef PRODUCT_V5SDV
    buffer.VFrame.mEnvLV = -1;
#else
    buffer.VFrame.mEnvLV = AW_MPI_ISP_GetEnvLV(mIspDevId);
#endif
    //alogd("print vipp[%d]pts[%lld]ms", mChnId, buffer.VFrame.mpts/1000);
#if (DEBUG_STORE_VI_FRAME!=0)
    if(mDbgFrameNum%30 == 0)
    {
        AW_MPI_VI_Debug_StoreFrame(mChnId, 0, "/home/sample_EncodeResolutionChange_Files");
    }
    mDbgFrameNum++;
#endif
#if (DEBUG_SAVE_FRAME_TO_TMP!=0)
    DebugLoopSaveFrame(&buffer);
#endif
    mFrameCounter++;

    //pFrmbuf = mpVideoFrmBuffer + buffer.mId;
    pFrmbuf = NULL;
    mFrameBuffersLock.lock();
    for(std::list<VIDEO_FRAME_BUFFER_S>::iterator it=mFrameBuffers.begin(); it!=mFrameBuffers.end(); ++it)
    {
        if(it->mFrameBuf.mId == buffer.mId)
        {
            if(NULL == pFrmbuf)
            {
                pFrmbuf = &*it;
            }
            else
            {
                aloge("fatal error! VI frame id[0x%x] is repeated!", buffer.mId);
            }
        }
    }
    if(NULL == pFrmbuf)
    {
        alogv("ISP[%d]VIPP[%d] frame buffer array did not contain this bufferId[0x%x], add it.", mIspDevId, mChnId, buffer.mId);
        mFrameBuffers.emplace_back();
        memset(&mFrameBuffers.back(), 0, sizeof(VIDEO_FRAME_BUFFER_S));
        pFrmbuf = &mFrameBuffers.back();
    }
    pFrmbuf->mFrameBuf = buffer;
    if (mLastZoom != mNewZoom)
    {
        calculateCrop(&mRectCrop, mNewZoom, mFrameWidthOut, mFrameHeightOut);
        mLastZoom = mNewZoom;
        alogd("zoom[%d], CROP: [%d, %d, %d, %d]", mNewZoom, mRectCrop.X, mRectCrop.Y, mRectCrop.Width, mRectCrop.Height);
    }
    pFrmbuf->mFrameBuf.VFrame.mOffsetTop = mRectCrop.Y;
    pFrmbuf->mFrameBuf.VFrame.mOffsetBottom = mRectCrop.Y + mRectCrop.Height;
    pFrmbuf->mFrameBuf.VFrame.mOffsetLeft = mRectCrop.X;
    pFrmbuf->mFrameBuf.VFrame.mOffsetRight = mRectCrop.X + mRectCrop.Width;
    pFrmbuf->mColorSpace = mColorSpace;
    //pFrmbuf->mIsThumbAvailable = 0;
    //pFrmbuf->mThumbUsedForPhoto = 0;
    pFrmbuf->mRefCnt = 1;
    mFrameBufferIdList.push_back(pFrmbuf->mFrameBuf.mId);
    mFrameBuffersLock.unlock();

    if(false == bTakePictureStart)
    {
        {
            AutoMutex lock(mRefCntLock);
            pFrmbuf->mRefCnt++;
        }
        OSAL_Queue(mpPreviewQueue, pFrmbuf);
        mpPrevThread->notifyNewFrameCome();
    }
    else
    {
        if (mTakePictureMode == TAKE_PICTURE_MODE_NORMAL) 
        {
           aloge("fatal error! don't support normal mode take picture temporary!");
           mpPicThread->notifyPictureEnd();
           bTakePictureStart = false;
        } 
        else 
        {
            mRefCntLock.lock();
            pFrmbuf->mRefCnt++;
            mRefCntLock.unlock();
            OSAL_Queue(mpPreviewQueue, pFrmbuf);
            mpPrevThread->notifyNewFrameCome();

            if (mTakePictureMode == TAKE_PICTURE_MODE_FAST)
            {
                mRefCntLock.lock();
                pFrmbuf->mRefCnt++;
                mRefCntLock.unlock();
                mIsPicCopy = false;
                //mTakePictureMode = TAKE_PICTURE_MODE_NULL;
                OSAL_Queue(mpPictureQueue, pFrmbuf);
                mpPicThread->notifyNewFrameCome();
                mpPicThread->notifyPictureEnd();
                bTakePictureStart = false;
            }
            else if(mTakePictureMode == TAKE_PICTURE_MODE_CONTINUOUS)
            {
                bool bPermit = false;
                if (0 == mContinuousPictureStartTime)    //let's begin!
                {
                    mContinuousPictureStartTime = CDX_GetSysTimeUsMonotonic()/1000;
                    mContinuousPictureLast = mContinuousPictureStartTime;
                    mContinuousPictureInterval = mParameters.getContinuousPictureIntervalMs();
                    mContinuousPictureCnt = 0;
                    mContinuousPictureMax = mParameters.getContinuousPictureNumber();
                    bPermit = true;
                    alogd("begin continous picture, will take [%d]pics, interval[%llu]ms, curTm[%lld]ms", mContinuousPictureMax, mContinuousPictureInterval, mContinuousPictureLast);
                }
                else
                {
                    if(mContinuousPictureInterval <= 0)
                    {
                        bPermit = true;
                    }
                    else
                    {
                        uint64_t nCurTime = CDX_GetSysTimeUsMonotonic()/1000;
                        if(nCurTime >= mContinuousPictureLast + mContinuousPictureInterval)
                        {
                            //alogd("capture picture, curTm[%lld]ms, [%lld][%lld]", nCurTime, mContinuousPictureLast, mContinuousPictureInterval);
                            bPermit = true;
                        }
                    }
                }

                if(bPermit)
                {
                    mRefCntLock.lock();
                    pFrmbuf->mRefCnt++;
                    mRefCntLock.unlock();
                    mIsPicCopy = false;
                    OSAL_Queue(mpPictureQueue, pFrmbuf);
                    mpPicThread->notifyNewFrameCome();
                    mContinuousPictureCnt++;
                    if(mContinuousPictureCnt >= mContinuousPictureMax)
                    {
                        mpPicThread->notifyPictureEnd();
                        mContinuousPictureStartTime = 0;
                        mContinuousPictureLast = 0;
                        mContinuousPictureInterval = 0;
                        mContinuousPictureCnt = 0;
                        mContinuousPictureMax = 0;
                        bTakePictureStart = false;
                    }
                    else
                    {
                        mContinuousPictureLast = mContinuousPictureStartTime+mContinuousPictureCnt*mContinuousPictureInterval;
                    }
                }
            }
            else
            {
                aloge("fatal error! any other take picture mode[0x%x]?", mTakePictureMode);
                bTakePictureStart = false;
            }
        }
    }

    if(mpMODThread && mpMODThread->IsMODDetectEnable())
    {
        mRefCntLock.lock();
        pFrmbuf->mRefCnt++;
        mRefCntLock.unlock();
        if(NO_ERROR != mpMODThread->sendFrame(pFrmbuf))
        {
            AutoMutex lock(mRefCntLock);
            pFrmbuf->mRefCnt--;
        }
    }

    releaseFrame(pFrmbuf->mFrameBuf.mId);
    //return true;
    goto PROCESS_MESSAGE;
_exit0:
    return bRunningFlag;
}

bool VIChannel::previewThread()
{
    bool bRunningFlag = true;
    EyeseeMessage msg;
    status_t getMsgRet;
    ERRORTYPE ret;
    VIDEO_FRAME_BUFFER_S *pFrmbuf;
    PROCESS_MESSAGE:
    getMsgRet = mpPrevThread->mMsgQueue.dequeueMessage(&msg);
    if(getMsgRet == NO_ERROR)
    {
        if(DoPreviewThread::MsgTypePreview_InputFrameAvailable == msg.mMsgType)
        {
        }
        else if(DoPreviewThread::MsgTypePreview_releaseAllFrames == msg.mMsgType)
        {
            AutoMutex lock(mpPrevThread->mWaitLock);
            for(;;)
            {
                pFrmbuf = (VIDEO_FRAME_BUFFER_S*)OSAL_Dequeue(mpPreviewQueue);
                if(pFrmbuf)
                {
                    releaseFrame(pFrmbuf->mFrameBuf.mId);
                }
                else
                {
                    break;
                }
            }
            if(mpPrevThread->mbWaitReleaseAllFrames)
            {
                mpPrevThread->mbWaitReleaseAllFrames = false;
            }
            else
            {
                aloge("fatal error! check code!");
            }
            mpPrevThread->mCondReleaseAllFramesFinished.signal();
        }
        else if(DoPreviewThread::MsgTypePreview_Exit == msg.mMsgType)
        {
            bRunningFlag = false;
            goto _exit0;
        }
        else
        {
            aloge("unknown msg[0x%x]!", msg.mMsgType);
        }
        goto PROCESS_MESSAGE;
    }
    
    pFrmbuf = (VIDEO_FRAME_BUFFER_S*)OSAL_Dequeue(mpPreviewQueue);
    if (pFrmbuf == NULL) 
    {
        {
            AutoMutex lock(mpPrevThread->mWaitLock);
            if(OSAL_GetElemNum(mpPreviewQueue) > 0)
            {
                alogd("Low probability! preview new frame come before check again.");
                goto PROCESS_MESSAGE;
            }
            else
            {
                mpPrevThread->mbWaitPreviewFrame = true;
            }
        }
        int nWaitTime = 0;    //unit:ms, 10*1000
        int msgNum = mpPrevThread->mMsgQueue.waitMessage(nWaitTime);
        if(msgNum <= 0)
        {
            aloge("fatal error! preview thread wait message timeout[%d]ms! msgNum[%d], bWait[%d]", nWaitTime, msgNum, mpPrevThread->mbWaitPreviewFrame);
        }
        goto PROCESS_MESSAGE;
    }

    mpCallbackNotifier->onNextFrameAvailable(pFrmbuf, mChnId);
    mpPreviewWindow->onNextFrameAvailable(pFrmbuf);

    releaseFrame(pFrmbuf->mFrameBuf.mId);
    //return true;
    goto PROCESS_MESSAGE;
_exit0:
    return bRunningFlag;
}

bool VIChannel::pictureThread()
{
    bool bRunningFlag = true;
    EyeseeMessage msg;
    status_t getMsgRet;
    ERRORTYPE ret;
    bool bDrainPictureQueue = false;
    VIDEO_FRAME_BUFFER_S *pFrmbuf;
    PROCESS_MESSAGE:
    getMsgRet = mpPicThread->mMsgQueue.dequeueMessage(&msg);
    if(getMsgRet == NO_ERROR)
    {
        if(DoPictureThread::MsgTypePicture_InputFrameAvailable == msg.mMsgType)
        {
        }
        else if(DoPictureThread::MsgTypePicture_SendPictureEnd == msg.mMsgType)
        {
            bDrainPictureQueue = true;
        }
        else if(DoPictureThread::MsgTypePicture_releaseAllFrames == msg.mMsgType)
        {
            AutoMutex lock(mpPicThread->mWaitLock);
            for(;;)
            {
                pFrmbuf = (VIDEO_FRAME_BUFFER_S*)OSAL_Dequeue(mpPictureQueue);
                if(pFrmbuf)
                {
                    releaseFrame(pFrmbuf->mFrameBuf.mId);
                }
                else
                {
                    break;
                }
            }
            if(mpPicThread->mbWaitReleaseAllFrames)
            {
                mpPicThread->mbWaitReleaseAllFrames = false;
            }
            else
            {
                aloge("fatal error! check code!");
            }
            mpPicThread->mCondReleaseAllFramesFinished.signal();
        }
        else if(DoPictureThread::MsgTypePicture_Exit == msg.mMsgType)
        {
            bRunningFlag = false;
            goto _exit0;
        }
        else
        {
            aloge("unknown msg[0x%x]!", msg.mMsgType);
        }
        goto PROCESS_MESSAGE;
    }

    while(1)
    {
        pFrmbuf = (VIDEO_FRAME_BUFFER_S*)OSAL_Dequeue(mpPictureQueue);
        if (pFrmbuf == NULL) 
        {
            if(bDrainPictureQueue)
            {
                mpCallbackNotifier->disableMessage(mTakePicMsgType);
                bDrainPictureQueue = false;
                if(false == mbKeepPictureEncoder)
                {
                    AutoMutex lock(mTakePictureLock);
                    mpCallbackNotifier->takePictureEnd();
                }
                mLock.lock();
                if(!mbTakePictureStart)
                {
                    aloge("fatal error! why takePictureStart is false when we finish take picture?");
                }
                mbTakePictureStart = false;
                mLock.unlock();
            }
            {
                AutoMutex lock(mpPicThread->mWaitLock);
                if(OSAL_GetElemNum(mpPictureQueue) > 0)
                {
                    alogd("Low probability! picture new frame come before check again.");
                    goto PROCESS_MESSAGE;
                }
                else
                {
                    mpPicThread->mbWaitPictureFrame = true;
                }
            }
            mpPicThread->mMsgQueue.waitMessage();
            goto PROCESS_MESSAGE;
        }
#if 0
        static int index = 0;
        char str[50];
        sprintf(str, "./savepicture/4/%d.n21", index++);
        FILE *fp = fopen(str, "wb");
        if(fp)
        {
            VideoFrameBufferSizeInfo FrameSizeInfo;
            getVideoFrameBufferSizeInfo(&pFrmbuf->mFrameBuf, &FrameSizeInfo);
            int yuvSize[3] = {FrameSizeInfo.mYSize, FrameSizeInfo.mUSize, FrameSizeInfo.mVSize};
            for(int i=0; i<3; i++)
            {
                if(pFrmbuf->mFrameBuf.VFrame.mpVirAddr[i] != NULL)
                {
                    fwrite(pFrmbuf->mFrameBuf.VFrame.mpVirAddr[i], 1, yuvSize[i], fp);
                }
            }
            alogd("save a frame!");
            fclose(fp);
        }
#endif
        bool ret = false;
        mTakePictureLock.lock();
        ret = mpCallbackNotifier->takePicture(pFrmbuf, false, true);    // to take picture for post view
        mTakePictureLock.unlock();
        if((mTakePicMsgType&CAMERA_MSG_POSTVIEW_FRAME) && (false == ret))
        {
            aloge("post_view_take_pic_fail");
            mpCallbackNotifier->NotifyCameraTakePicFail();
        }

        struct isp_exif_attribute isp_exif_s; 
        memset(&isp_exif_s,0,sizeof(isp_exif_s));
        int iso_speed_idx = 0;
        int iso_speed_value[] = {100,200,400,800,1600,3200,6400};
        AW_MPI_ISP_AE_GetISOSensitive(mIspDevId,&iso_speed_idx);

        if(7 < iso_speed_idx)
        {
            iso_speed_idx = 7;
        }
        if(0 > iso_speed_idx)
        {
            iso_speed_idx = 0;
        }
        if(0 < iso_speed_idx)
        {
            iso_speed_idx -= 1;
        }

        isp_exif_s.iso_speed = iso_speed_value[iso_speed_idx];

        int exposure_bias_idx = 0;
        int exposure_bias_value[] = {-3,-2,-1,0,1,2,3};
        AW_MPI_ISP_AE_GetExposureBias(mIspDevId,&exposure_bias_idx);
        if(0 >= exposure_bias_idx )
        {
            exposure_bias_idx = 0;
        }
        else if(7 < exposure_bias_idx)
        {
            exposure_bias_idx = 6;
        }
        else
        {
            exposure_bias_idx -= 1;
        } 
        
        isp_exif_s.exposure_bias = exposure_bias_value[exposure_bias_idx];
        // to take picture for normal use,if failed in taking picture for post view,to take pic for normal use
        // may succeed,so not jump thie step.
        mTakePictureLock.lock();
        ret = mpCallbackNotifier->takePicture(pFrmbuf, false, false,&isp_exif_s);
        mTakePictureLock.unlock();
        if(false == ret)
        {
            aloge("normal_take_pic_fail");
            mpCallbackNotifier->NotifyCameraTakePicFail();
        }

        if (mIsPicCopy) 
        {
            aloge("fatal error! we do not copy!");
            //CdcMemPfree(mpMemOps, pFrmbuf->mFrameBuf.pVirAddr[0]);
        } 
        else
        {
            releaseFrame(pFrmbuf->mFrameBuf.mId);
        }
        if(!bDrainPictureQueue)
        {
            break;
        }
    }
    //return true;
    goto PROCESS_MESSAGE;
_exit0:
    return bRunningFlag;
}

bool VIChannel::commandThread()
{
    bool bRunningFlag = true;
    EyeseeMessage msg;
    status_t getMsgRet;
    ERRORTYPE ret;
PROCESS_MESSAGE:
    getMsgRet = mpCommandThread->mMsgQueue.dequeueMessage(&msg);
    if(getMsgRet == NO_ERROR)
    {
        if(DoCommandThread::MsgTypeCommand_TakePicture == msg.mMsgType)
        {
            doTakePicture(msg.mPara0);
        }
        else if(DoCommandThread::MsgTypeCommand_CancelContinuousPicture == msg.mMsgType)
        {
            doCancelContinuousPicture();
        }
        else if(DoCommandThread::MsgTypeCommand_Exit == msg.mMsgType)
        {
            bRunningFlag = false;
            goto _exit0;
        }
        else
        {
            aloge("unknown msg[0x%x]!", msg.mMsgType);
        }
    }
    else
    {
        mpCommandThread->mMsgQueue.waitMessage();
    }
    //return true;
    goto PROCESS_MESSAGE;
_exit0:
    return bRunningFlag;
}

void VIChannel::increaseBufRef(VIDEO_FRAME_BUFFER_S *pBuf)
{
    VIDEO_FRAME_BUFFER_S *pFrame = (VIDEO_FRAME_BUFFER_S*)pBuf;
    AutoMutex lock(mRefCntLock);
    ++pFrame->mRefCnt;
}

void VIChannel::decreaseBufRef(unsigned int nBufId)
{
    releaseFrame(nBufId);
}

void VIChannel::NotifyRenderStart()
{
    mpCallbackNotifier->NotifyRenderStart();
}
status_t VIChannel::doTakePicture(unsigned int msgType)
{
    AutoMutex lock(mLock);
    int jpeg_quality = mParameters.getJpegQuality();
    if (jpeg_quality <= 0) {
        jpeg_quality = 90;
    }
    mpCallbackNotifier->setJpegQuality(jpeg_quality);

    int jpeg_rotate = mParameters.getJpegRotation();
    if (jpeg_rotate <= 0) {
        jpeg_rotate = 0;
    }
    mpCallbackNotifier->setJpegRotate(jpeg_rotate);

    SIZE_S size;
    mParameters.getPictureSize(size);
    mpCallbackNotifier->setPictureSize(size.Width, size.Height);
    mParameters.getJpegThumbnailSize(size);
    mpCallbackNotifier->setJpegThumbnailSize(size.Width, size.Height);
    int quality = mParameters.getJpegThumbnailQuality();
    mpCallbackNotifier->setJpegThumbnailQuality(quality);

    char *pGpsMethod = mParameters.getGpsProcessingMethod();
    if (pGpsMethod != NULL) {
        mpCallbackNotifier->setGPSLatitude(mParameters.getGpsLatitude());
        mpCallbackNotifier->setGPSLongitude(mParameters.getGpsLongitude());
        mpCallbackNotifier->setGPSAltitude(mParameters.getGpsAltitude());
        mpCallbackNotifier->setGPSTimestamp(mParameters.getGpsTimestamp());
        mpCallbackNotifier->setGPSMethod(pGpsMethod);
    }
    mTakePicMsgType = msgType;
    mpCallbackNotifier->enableMessage(msgType);

//    mCapThreadLock.lock();
//    mTakePictureMode = TAKE_PICTURE_MODE_FAST;
//    mCapThreadLock.unlock();
    mTakePictureMode = mParameters.getPictureMode();
    mpCapThread->SendCommand_TakePicture();

    return NO_ERROR;
}

status_t VIChannel::doCancelContinuousPicture()
{
    AutoMutex lock(mLock);
    mpCapThread->SendCommand_CancelContinuousPicture();
    return NO_ERROR;
}

status_t VIChannel::takePicture(unsigned int msgType, PictureRegionCallback *pPicReg)
{
    AutoMutex lock(mLock);
    if(mbTakePictureStart)
    {
        aloge("fatal error! During taking picture, we don't accept new takePicture command!");
        return UNKNOWN_ERROR;
    }
    else
    {
        mbTakePictureStart = true;
        mpCallbackNotifier->setPictureRegionCallback(pPicReg);
        mpCommandThread->SendCommand_TakePicture(msgType);
        return NO_ERROR;
    }
}

status_t VIChannel::notifyPictureRelease()
{
    return mpCallbackNotifier->notifyPictureRelease();
}

status_t VIChannel::cancelContinuousPicture()
{
    AutoMutex lock(mLock);
    if(!mbTakePictureStart)
    {
        aloge("fatal error! not start take picture!");
        return NO_ERROR;
    }
    mpCommandThread->SendCommand_CancelContinuousPicture();
    return NO_ERROR;
}

status_t VIChannel::KeepPictureEncoder(bool bKeep)
{
    AutoMutex lock(mLock);
    mbKeepPictureEncoder = bKeep;
    return NO_ERROR;
}

status_t VIChannel::releasePictureEncoder()
{
    AutoMutex lock(mTakePictureLock);
    alogw("vipp[%d] force release picture encoder! user must guarantee not taking picture now.", mChnId);
    mpCallbackNotifier->takePictureEnd();
    return NO_ERROR;
}

status_t VIChannel::DebugLoopSaveFrame(VIDEO_FRAME_INFO_S *pFrame)
{
    char DbgStoreFilePath[256];
    if(MM_PIXEL_FORMAT_YUV_AW_AFBC == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] afbc buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].afbc", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else if(MM_PIXEL_FORMAT_YUV_AW_LBC_2_0X == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] LBC_2_0X buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].lbc20x", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else if(MM_PIXEL_FORMAT_YUV_AW_LBC_2_5X == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] LBC_2_5X buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].lbc25x", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else if(MM_PIXEL_FORMAT_YUV_AW_LBC_1_0X == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] LBC_1_0X buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].lbc10x", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else if(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] nv21 buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].nv21", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else if(MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pFrame->VFrame.mPixelFormat)
    {
        //alogd("store vipp[%d] nv12 buffer id[%d]", mChnId, pFrame->mId);
        snprintf(DbgStoreFilePath, 256, "/tmp/pic[%d][id%d][%lldus].nv12", mDbgFrameNum++, pFrame->mId, pFrame->VFrame.mpts);
    }
    else
    {
        aloge("fatal error! unsupport pixel format[0x%x]", pFrame->VFrame.mPixelFormat);
    }

    //alogd("prepare store frame in file[%s]", DbgStoreFilePath);
    FILE *dbgFp = fopen(DbgStoreFilePath, "wb");
    if(dbgFp != NULL)
    {
        VideoFrameBufferSizeInfo FrameSizeInfo;
        getVideoFrameBufferSizeInfo(pFrame, &FrameSizeInfo);
        int yuvSize[3] = {FrameSizeInfo.mYSize, FrameSizeInfo.mUSize, FrameSizeInfo.mVSize};
        for(int i=0; i<3; i++)
        {
            if(pFrame->VFrame.mpVirAddr[i] != NULL)
            {
                fwrite(pFrame->VFrame.mpVirAddr[i], 1, yuvSize[i], dbgFp);
                //alogd("virAddr[%d]=[%p], length=[%d]", i, pFrame->VFrame.mpVirAddr[i], yuvSize[i]);
            }
        }
        fclose(dbgFp);
        //alogd("store frame in file[%s]", DbgStoreFilePath);
        mDbgFrameFilePathList.emplace_back(DbgStoreFilePath);
    }
    else
    {
        aloge("fatal error! open file[%s] fail!", DbgStoreFilePath);
    }
    while(mDbgFrameFilePathList.size() > DEBUG_SAVE_FRAME_NUM)
    {
        remove(mDbgFrameFilePathList.front().c_str());
        mDbgFrameFilePathList.pop_front();
    }
    return NO_ERROR;
}

}; /* namespace EyeseeLinux */

