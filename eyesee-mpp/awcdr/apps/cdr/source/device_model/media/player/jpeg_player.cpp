/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    jpeg_player.cpp
 * @brief   解码jpeg文件并通过vo显示
 * @author  id:826
 * @version v0.3
 * @date    2017-11-27
 */
#include "jpeg_player.h"
#include "common/app_log.h"
#include "common/app_def.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sstream>

#include <vo/hwdisplay.h>

using namespace EyeseeLinux;
using namespace std;

static const int stream_size = (1<<20) * 4;

JpegPlayer::JpegPlayer()
    : state_(JPEG_PLAYER_IDLE)
    , hlay_(HLAY(0, 0))
    , vdec_chn_(0)
    , vo_chn_(0)
    , new_stream_size_(stream_size)
{
    cdx_sem_init(&sem_wait_from_vo_, 0);
    memset(&stream_info_, 0, sizeof(stream_info_));
    stream_info_.pAddr = static_cast<unsigned char*>(malloc(stream_size)); // max size
    assert(stream_info_.pAddr != NULL);
}

JpegPlayer::~JpegPlayer()
{
    mutex_.lock();
    if (state_ != JPEG_PLAYER_IDLE) {
        mutex_.unlock();
        Reset();
    }
    mutex_.unlock();

    cdx_sem_deinit(&sem_wait_from_vo_);

    if (stream_info_.pAddr != NULL)
        free(stream_info_.pAddr);
}

int JpegPlayer::SetDisplay(int hlay)
{
    hlay_ = hlay;

    return 0;
}

int JpegPlayer::PrepareFile(std::string filepath,std::string type)
{
    db_msg("[sh] enter the PrepareFile");
    ERRORTYPE ret;
    MPPCallbackInfo cbInfo;

    lock_guard<mutex> lock(mutex_);

    if (state_ != JPEG_PLAYER_IDLE) {
        db_error("wrong state: %d", state_);
        return -1;
    }

    //open jpeg file
    FILE *fp_src_file = fopen(filepath.c_str(), "rb");
    if(!fp_src_file)
    {
        db_warn("can't open jpg file[%s], %s!", filepath.c_str(), strerror(errno));
        return -1;
    }
    setvbuf(fp_src_file, NULL, _IONBF, 0); //close IO buffer.
    fseek(fp_src_file, 0, SEEK_END);
    stream_info_.mLen = ftell(fp_src_file);
	if (stream_info_.mLen == 0) {
		fclose(fp_src_file);
		return -1;
	}

    /* enable vo layer */
    ret = AW_MPI_VO_EnableVideoLayer(hlay_);
    if((SUCCESS != ret) && (ERR_VO_VIDEO_NOT_DISABLE != ret)) {
        db_error("enable layer failed, hlay: %d, ret: %d", hlay_, ret);
        fclose(fp_src_file);
        return -1;
    }

    VO_PUB_ATTR_S spPubAttr;
    AW_MPI_VO_GetPubAttr(0, &spPubAttr);

    /* set screen win size */
#ifdef CUT_HDMI_DISPLAY
    int hdmi_screen_offset = CUT_HDMI_DISPLAY;
#else
    int hdmi_screen_offset = 0;
#endif
    VO_VIDEO_LAYER_ATTR_S mLayerAttr;
    AW_MPI_VO_GetVideoLayerAttr(hlay_, &mLayerAttr);
    mLayerAttr.stDispRect.X = 0;
    mLayerAttr.stDispRect.Y = 0;
    if (spPubAttr.enIntfType == VO_INTF_LCD) {
        mLayerAttr.stDispRect.X = 0;  //notes:由于屏幕比例不是16:9,修改显示宽高使图像适配16:9
        mLayerAttr.stDispRect.Y = 0;
        mLayerAttr.stDispRect.Height = SCREEN_HEIGHT;
        mLayerAttr.stDispRect.Width  = SCREEN_WIDTH;
    } else if (spPubAttr.enIntfType == VO_INTF_HDMI) {
        switch (spPubAttr.enIntfSync) {
            case VO_OUTPUT_3840x2160_25:
            case VO_OUTPUT_3840x2160_30:
                mLayerAttr.stDispRect.X += (3840 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Y += (2160 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Width = 3840 - (3840 * hdmi_screen_offset) / 100;
                mLayerAttr.stDispRect.Height = 2160 - (2160 * hdmi_screen_offset) / 100;
                break;
            case VO_OUTPUT_1080P24:
            case VO_OUTPUT_1080P25:
            case VO_OUTPUT_1080P30:
            case VO_OUTPUT_1080P60:
                mLayerAttr.stDispRect.X += (1920 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Y += (1080 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Width = 1920 - (1920 * hdmi_screen_offset) / 100;
                mLayerAttr.stDispRect.Height = 1080 - (1080 * hdmi_screen_offset) / 100;
                break;
            default:
                mLayerAttr.stDispRect.X += (1920 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Y += (1080 * hdmi_screen_offset) / (2 * 100);
                mLayerAttr.stDispRect.Width = 1920 - (1920 * hdmi_screen_offset) / 100;
                mLayerAttr.stDispRect.Height = 1080 - (1080 * hdmi_screen_offset) / 100;
                break;
        }
    }
    else {
        db_error("unknown type");
        mLayerAttr.stDispRect.Width = SCREEN_WIDTH;
        mLayerAttr.stDispRect.Height = SCREEN_HEIGHT;
    }
    AW_MPI_VO_SetVideoLayerAttr(hlay_, &mLayerAttr);

    /* create vo channel */
    BOOL bSuccessFlag = FALSE;
    while(vo_chn_ < VO_MAX_CHN_NUM)
    {
        ret = AW_MPI_VO_EnableChn(hlay_, vo_chn_);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            db_debug("create vo channel[%d] success!", vo_chn_);
            break;
        }
        else if(ERR_VO_CHN_NOT_DISABLE == ret)
        {
            db_debug("vo channel[%d] is exist, find next!", vo_chn_);
            vo_chn_++;
        }
        else
        {
            db_error("fatal error! create vo channel[%d] ret[0x%x]!", vo_chn_, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        vo_chn_ = MM_INVALID_CHN;
        db_error("fatal error! create vo channel fail!");
        fclose(fp_src_file);
        return -1;
    }
    cbInfo.cookie = this;
    cbInfo.callback = (MPPCallbackFuncType)&MPPVOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(hlay_, vo_chn_, &cbInfo);

    /* init vdec */
    VDEC_CHN_ATTR_S mVDecAttr;
    VideoStreamInfo mVDecStreamInfo;
    memset(&mVDecAttr, 0, sizeof(mVDecAttr));
    memset(&mVDecStreamInfo, 0, sizeof(mVDecStreamInfo));
    mVDecAttr.mType = PT_JPEG;
    mVDecAttr.mBufSize = stream_info_.mLen + 1024;
    mVDecAttr.mnFrameBufferNum = 1;
    mVDecAttr.mOutputPixelFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    if (spPubAttr.enIntfType == VO_INTF_LCD)
        mVDecAttr.mInitRotation = ROTATE_270;
#if 0
    if (spPubAttr.enIntfType == VO_INTF_LCD) {
            db_debug("pic size:w %d,h %d,disp size:w %d, h %d",picsize_.Width,picsize_.Height,
                    mLayerAttr.stDispRect.Width, mLayerAttr.stDispRect.Height);
            mVDecAttr.mPicWidth = mLayerAttr.stDispRect.Width;;
            mVDecAttr.mPicHeight = mLayerAttr.stDispRect.Height;
            mVDecStreamInfo.nWidth = picsize_.Width;
            mVDecStreamInfo.nHeight = picsize_.Height;
        mVDecAttr.mInitRotation = ROTATE_90;
    } else if (spPubAttr.enIntfType == VO_INTF_HDMI) {
        mVDecAttr.mInitRotation = ROTATE_NONE;
    }
#endif
    bSuccessFlag = FALSE;
    while(vdec_chn_ < VDEC_MAX_CHN_NUM)
    {
        ret = AW_MPI_VDEC_CreateChn(vdec_chn_, &mVDecAttr);
        if(SUCCESS == ret)
        {
            bSuccessFlag = TRUE;
            db_debug("create VDec channel[%d] success!", vdec_chn_);
            break;
        }
        else if (ERR_VDEC_EXIST == ret)
        {
            db_debug("VDec channel[%d] exist, find next!", vdec_chn_);
            vdec_chn_++;
        }
        else
        {
            db_error("create VDec channel[%d] fail! ret[0x%x]!", vdec_chn_, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        vdec_chn_ = MM_INVALID_CHN;
        db_error("fatal error! create VDec channel fail!");
    }
    /*if(spPubAttr.enIntfType == VO_INTF_LCD && type == TYPE_PIC) {
        db_debug("display device is lcd,set video stream info.");
        AW_MPI_VDEC_SetVideoStreamInfo(vdec_chn_, &mVDecStreamInfo);
    }*/
    cbInfo.cookie = this;
    cbInfo.callback = (MPPCallbackFuncType)&MPPVDecCallbackWrapper;
    AW_MPI_VDEC_RegisterCallback(vdec_chn_, &cbInfo);
    static VDEC_DECODE_FRAME_PARAM_S pDecodeParam;
    pDecodeParam.mDecodeFrameNum = 1;
    ret = AW_MPI_VDEC_StartRecvStreamEx(vdec_chn_, &pDecodeParam);
	if(SUCCESS != ret) // process for failure to start vdec receive
	{
		return -1;
	}

    struct stat st;
    if (stat(filepath.c_str(), &st) != 0 || st.st_size < 4) {
        db_error("'%s' is not valid jpeg file", filepath.c_str());
        fclose(fp_src_file);
        return -1;
    }

    //get stream from file
    {
        if(stream_info_.mLen > new_stream_size_)
        {
            unsigned char *ptr = NULL;
            ptr = static_cast<unsigned char*>(realloc(stream_info_.pAddr, stream_info_.mLen + 1024));
            if(!ptr)
            {
                db_error("fail to malloc memory!");
                fclose(fp_src_file);
                return -1;
            }
            stream_info_.pAddr = ptr;
            new_stream_size_ = stream_info_.mLen + 1024;
        }
        rewind(fp_src_file);
        fread(stream_info_.pAddr, 1, stream_info_.mLen, fp_src_file);
        stream_info_.mbEndOfFrame = 1;
        stream_info_.mbEndOfStream = 1;
    }
    db_debug("------------read file(len:%d,path:%s) and ready sendframe--------------", stream_info_.mLen, filepath.c_str());
    fclose(fp_src_file);

    // FIXME: 设置了dos隐藏属性的文件在linux下访问之后,
    // 文件的dos隐藏属性就会丢失, 所以需要这里重新设置一次隐藏属性
    if (filepath.rfind("thm") != string::npos) {
        sync();
        stringstream cmd;
        cmd << "mattrib +h ";
        cmd << filepath.substr(filepath.find("DCIM"));
        db_debug("cmd: %s", cmd.str().c_str());
        system(cmd.str().c_str());
    }

#if 0
    //reopen ve for pic resolution may change
    ret = AW_MPI_VDEC_ReopenVideoEngine(vdec_chn_);
    if (ret != SUCCESS)
    {
        db_error("reopen ve failed?!");
    }
#endif
    state_ = JPEG_PLAYER_PREPARED;

    return 0;
}

int JpegPlayer::ShowPic()
{
    db_msg("[sh] enter the ShowPic");
    ERRORTYPE ret;

    lock_guard<mutex> lock(mutex_);

    if (state_ != JPEG_PLAYER_PREPARED) {
        db_error("wrong state: %d", state_);
        return -1;
    }

    AW_MPI_VO_StartChn(hlay_, vo_chn_);

    //send stream to vdec
    ret = AW_MPI_VDEC_SendStream(vdec_chn_, &stream_info_, 200);
    if(ret != SUCCESS)
    {
        db_warn("send stream with 200ms timeout fail?!");
        return -1;
    }

    //get frame from vdec
    memset(&frame_info_, 0, sizeof(frame_info_));
    if (AW_MPI_VDEC_GetImage(vdec_chn_, &frame_info_, 2000) != ERR_VDEC_NOBUF)
    {
        ret = AW_MPI_VO_SendFrame(hlay_, vo_chn_, &frame_info_, 0);
        if (SUCCESS != ret) {
            db_error("send frame to vo failed, ret: %d", ret);
            return -1;
        }
        cdx_sem_down(&sem_wait_from_vo_);
        AW_MPI_VDEC_ReleaseImage(vdec_chn_, &frame_info_);
    } else {
        db_error("decode jpeg error or timeout");
    }
    this->Notify(MSG_PIC_PLAY_START);
    state_ = JPEG_PLAYER_STARTED;

    return 0;
}

int JpegPlayer::ReleasePic()
{
    lock_guard<mutex> lock(mutex_);

    if (state_ == JPEG_PLAYER_PREPARED || state_ == JPEG_PLAYER_IDLE) {
        return 0;
    }

    if (state_ != JPEG_PLAYER_STARTED) {
        db_error("wrong state: %d", state_);
        return -1;
    }

    AW_MPI_VO_StopChn(hlay_, vo_chn_);
    AW_MPI_VDEC_StopRecvStream(vdec_chn_);

    state_ = JPEG_PLAYER_PREPARED;

    return 0;
}

int JpegPlayer::Reset()
{
    lock_guard<mutex> lock(mutex_);

    if (state_ == JPEG_PLAYER_IDLE) {
        return 0;
    }

    if (state_ == JPEG_PLAYER_STARTED) {
        mutex_.unlock();
        ReleasePic();
        mutex_.lock();
    }

    if (state_ != JPEG_PLAYER_PREPARED) {
        db_error("wrong state: %d", state_);
        return -1;
    }

    AW_MPI_VO_DisableChn(hlay_, vo_chn_);
    usleep(50 * 1000);
    vo_chn_ = 0;
    AW_MPI_VO_DisableVideoLayer(hlay_);
    //stop vdec channel.
    AW_MPI_VDEC_DestroyChn(vdec_chn_);
    vdec_chn_ = 0;
    state_ = JPEG_PLAYER_IDLE;

    usleep(20 * 1000);

    return 0;
}


ERRORTYPE JpegPlayer::MPPVDecCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    JpegPlayer *pContext = (JpegPlayer*)cookie;
    if(MOD_ID_VDEC == pChn->mModId)
    {
        if(pChn->mChnId != pContext->vdec_chn_)
        {
            db_error("fatal error! VDec chnId[%d]!=[%d]", pChn->mChnId, pContext->vdec_chn_);
        }
        switch(event)
        {
            case MPP_EVENT_NOTIFY_EOF:
            {
                db_debug("VDec channel notify APP that decode complete!");
                break;
            }
            default:
            {
                db_error("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                ret = ERR_VDEC_ILLEGAL_PARAM;
                break;
            }
        }
    }
    else
    {
        db_error("fatal error! why modId[0x%x]?!", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

ERRORTYPE JpegPlayer::MPPVOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    JpegPlayer *pJpegPlayer = static_cast<JpegPlayer *>(cookie);
    if(MOD_ID_VOU == pChn->mModId)
    {
        if(pChn->mChnId != pJpegPlayer->vo_chn_)
        {
            db_error("fatal error! VO chnId[%d]!=[%d]", pChn->mChnId, pJpegPlayer->vo_chn_);
        }
        switch(event)
        {
            case MPP_EVENT_RELEASE_VIDEO_BUFFER:
            {
                //VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S*)pEventData;
                cdx_sem_up(&pJpegPlayer->sem_wait_from_vo_);
                break;
            }
            default:
            {
                break;
            }
        }

    }
    return SUCCESS;
}

void JpegPlayer::SetJpegPicSize(SIZE_S &size)
{
    picsize_ = size;
}

void JpegPlayer::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}
