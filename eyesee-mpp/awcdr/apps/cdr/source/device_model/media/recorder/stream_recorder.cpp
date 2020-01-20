/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: stream_recorder.cpp
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2016-5-12
Description:
History:
*************************************************/

#include "device_model/media/recorder/stream_recorder.h"
#include "device_model/media/camera/camera.h"
#include "common/app_log.h"

#include <stdlib.h>
#include <string.h>
#include <string>

#undef LOG_TAG
#define LOG_TAG "stream_recorder.cpp"

using namespace EyeseeLinux;
using namespace std;

StreamRecorder::StreamRecorder(Camera *camera)
    : Recorder(camera)
{
}

StreamRecorder::~StreamRecorder()
{
    StopRecord();
}

int StreamRecorder::InitRecorder(const RecorderParam &param)
{
    int ret = -1;

    if (current_status_ != RECORDER_IDLE) {
        db_error("status: [%d], recorder is not idle, you can not init recorder", current_status_);
        return ret;
    }

    this->param_ = param;

    PAYLOAD_TYPE_E enc_type = PT_H264;
    switch (param_.enc_type) {
        case VENC_H264:
            enc_type = PT_H264;
            break;
        case VENC_H265:
            enc_type = PT_H265;
            break;
        case VENC_MJPEG:
            enc_type = PT_MJPEG;
            break;
        default:
            db_warn("unsuppoort video encode type, use h264 as default");
            enc_type = PT_H264;
            break;

    }
    recorder_->setVideoEncoder(enc_type);

    if (param.audio_record) {
        recorder_->setAudioSource(EyeseeRecorder::AudioSource::MIC);
        recorder_->setAudioSamplingRate(44100);
        recorder_->setAudioChannels(1);
        recorder_->setAudioEncodingBitRate(12200);
        recorder_->setAudioEncoder(PT_AAC);
        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_AUDIO);
    } else {
        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_ONLY);
    }

    if (camera_ != NULL) {
        recorder_->setCameraProxy(camera_->GetEyeseeCamera()->getRecordingProxy(), param_.vi_chn);
    }

    ret = recorder_->setVideoSource(EyeseeRecorder::VideoSource::CAMERA);
    if (ret != NO_ERROR) {
        db_error("setVideoSource Failed(%d)", ret);
        return ret;
    }

    int muxer_id;
    muxer_id = recorder_->addOutputFormatAndOutputSink(
            MEDIA_FILE_FORMAT_RAW, -1, 0, true);
    muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_RAW, muxer_id));

    ret = recorder_->setVideoFrameRate(param_.framerate);
    if (ret != NO_ERROR) {
        db_error("setVideoFrameRate Failed(%d)", ret);
        return ret;
    }
    recorder_->setVideoEncodingIFramesNumberInterval(param_.framerate);
    // set virtual I frame interval to 0 temporary
    //recorder_->setVirtualIFrameInterval(0);
    recorder_->enableVideoEncodingPIntra(true);

    recorder_->setVideoEncodingRateControlMode((EyeseeRecorder::VideoEncodeRateControlMode)param_.bitrate_ctrl);
    if (param_.bitrate_ctrl == EyeseeRecorder::VideoRCMode_CBR) {
        //ret = recorder_->setVideoEncodingBitRate(param_.bitrate);
        EyeseeRecorder::VEncBitRateControlAttr stRcAttr;
        stRcAttr.mVEncType = enc_type;
        stRcAttr.mRcMode = EyeseeRecorder::VideoRCMode_CBR;
        switch (stRcAttr.mVEncType)
        {
            case PT_H264:
            {
                stRcAttr.mAttrH264Cbr.mBitRate = param_.bitrate;
                stRcAttr.mAttrH264Cbr.mMaxQp = 51;
                stRcAttr.mAttrH264Cbr.mMinQp = 10;
                break;
            }
            case PT_H265:
            {
                stRcAttr.mAttrH265Cbr.mBitRate = param_.bitrate;
                stRcAttr.mAttrH265Cbr.mMaxQp = 51;
                stRcAttr.mAttrH265Cbr.mMinQp = 10;
                break;
            }
            case PT_MJPEG:
            {
                stRcAttr.mAttrMjpegCbr.mBitRate = param_.bitrate;
                break;
            }
            default:
            {
                db_error("unsupport video encode type, check code!");
                break;
            }
        }
        ret = recorder_->setVEncBitRateControlAttr(stRcAttr);
        if (ret != NO_ERROR) {
            db_error("setVideoEncodingBitRate Failed(%d)", ret);
            return ret;
        }
    }

    ret = recorder_->setVideoSize(param_.video_size.width, param_.video_size.height);
    if (ret != NO_ERROR) {
        db_error("setVideoSize Failed(%d)", ret);
        return ret;
    }
#if 0
    EyeseeRecorder::VEncAttr stVEncAttr;
    memset(&stVEncAttr, 0, sizeof(EyeseeRecorder::VEncAttr));
    stVEncAttr.mType = enc_type;
    stVEncAttr.mBufSize = 0;
    if(PT_H264 == stVEncAttr.mType)
    {
        switch(param_.profile)
        {
            case EyeseeRecorder::VEncProfile_BaseLine:
                stVEncAttr.mAttrH264.mProfile = 0;
                break;
            case EyeseeRecorder::VEncProfile_MP:
                stVEncAttr.mAttrH264.mProfile = 1;
                break;
            case EyeseeRecorder::VEncProfile_HP:
                stVEncAttr.mAttrH264.mProfile = 2;
                break;
            default:
                db_error("fatal error! unsupport h264 profile[0x%x]", param_.profile);
                stVEncAttr.mAttrH264.mProfile = 1;
                break;
        }
        stVEncAttr.mAttrH264.mLevel = H264_LEVEL_51;
    }
    else if(PT_H265 == stVEncAttr.mType)
    {
        switch(param_.profile)
        {
            case EyeseeRecorder::VEncProfile_MP:
                stVEncAttr.mAttrH265.mProfile = 0;
                break;
            default:
                db_error("fatal error! unsupport h265 profile[0x%x]", param_.profile);
                stVEncAttr.mAttrH265.mProfile = 0;
                break;
        }
        stVEncAttr.mAttrH265.mLevel = H265_LEVEL_62;
    }
    recorder_->setVEncAttr(&stVEncAttr);
#endif
    //open ve debug node
    VeProcSet mVeProcSet;
    memset(&mVeProcSet, 0, sizeof(mVeProcSet));
    mVeProcSet.bProcEnable = 1;
    mVeProcSet.nProcFreq = 30;
    mVeProcSet.nStatisBitRateTime = 1000;
    mVeProcSet.nStatisFrRateTime = 1000;
    recorder_->setProcSet(&mVeProcSet);

    ret = recorder_->prepare();
    if (ret != NO_ERROR) {
        db_error("prepare Failed(%d)", ret);
        return ret;
    }

    current_status_ = RECORDER_PREPARED;

    return ret;
}

void StreamRecorder::Update(MSG_TYPE msg,int cam_id,int rec_id)
{
    switch (msg) {
        default:
            #if 0
            Recorder::Update(msg);
            #endif
            break;
    }
}

int StreamRecorder::StopRecord()
{
    int ret;

    ret = Recorder::StopRecord();

    current_status_ = RECORDER_IDLE;

    return ret;
}

void StreamRecorder::DumpRecorderParm()
{
}

void StreamRecorder::setWifiFlag(bool flag)
{
}

int StreamRecorder::GetSoSRecorderFileName(char *p_FileName)
{
    return 0;
}
