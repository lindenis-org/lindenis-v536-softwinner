/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * ******************************************************************************
 */
/**
 * @file recorder_factory.cpp
 * @brief 录像工厂类
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#include "device_model/media/recorder/recorder_factory.h"
#include "device_model/media/recorder/main_recorder.h"
#include "device_model/media/recorder/backcam_recorder.h"
#include "device_model/media/recorder/stream_recorder.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "common/app_log.h"
#include "common/app_def.h"

#include "device_model/menu_config_lua.h"
#include "common/setting_menu_id.h"

#include <media/mm_common.h>
#include <media/mpi_sys.h>

#undef LOG_TAG
#define LOG_TAG "recorder_factory.cpp"

using namespace EyeseeLinux;
using namespace std;

// TODO: add factory default parameters setting

RecorderFactory::RecorderFactory()
{
}

RecorderFactory::~RecorderFactory()
{
    db_msg("destruct");
}

Recorder *RecorderFactory::CreateRecorder(RecorderType rec_type, Camera *camera, int p_viChn)
{
    Recorder *recorder = NULL;
    switch (rec_type) {
        case REC_720P30FPS:
        case REC_1080P30FPS:
        case REC_4K25FPS:
            recorder = this->CreateMainRecorder(rec_type, camera,p_viChn);
            break;

        case REC_U_720P30FPS:
        case REC_U_1080P30FPS:
        case REC_U_VGA30FPS:
            recorder = this->CreateUVCRecorder(rec_type, camera,p_viChn);
            break;
        case REC_M_SUB_CHN:
        case REC_B_SUB_CHN:
            db_warn("Create sub stream record\n");
            recorder = this->CreateStreamRecorder(rec_type,camera,p_viChn);
            break;
        default:
            break;
    }

    return recorder;
}

Recorder *RecorderFactory::CreateStreamRecorder(RecorderType rec_type, Camera *camera, int p_viChn)
{
    Recorder *recorder = new StreamRecorder(camera);
    assert(recorder != NULL);
    
	RecorderParam param(REC_STREAM);
    switch(rec_type)
    {
        case REC_M_SUB_CHN:
        {
          db_warn("create Cam A stream record\n");
          param.video_size = Size(320, 240);
          param.bitrate = (1 << 20) * 1;
          param.framerate = 25;
          param.enc_type = VENC_H264;
          param.vi_chn = p_viChn;
          param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
        }
        break;
        case REC_B_SUB_CHN:
        {          
          db_warn("create Cam B stream record\n");
          param.video_size = Size(320, 240);
          param.bitrate = (1 << 20) * 2;
          param.framerate = 25;
          param.enc_type = VENC_H264;
          param.vi_chn = p_viChn;
          param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
        }
        break;
        default:
        {
          db_warn("create stream record use the default parma\n");
          param.video_size = Size(320, 240);
          param.bitrate = (1 << 20) * 2;
          param.framerate = 25;
          param.enc_type = VENC_H264;
          param.vi_chn = p_viChn;
          param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
        }
        break;

    }
    
    recorder->SetParam(param);
    return recorder;	
}

Recorder *RecorderFactory::CreateMainRecorder(RecorderType rec_type, Camera *camera, int p_viChn)
{
    Recorder *recorder = new MainRecorder(camera);
    assert(recorder != NULL);

	RecorderParam param;
	recorder->GetParam(param);
	param.audio_record = false;
	param.stream_type = 0;		  /**< 写卡录制 */
	param.pack_strategy = PACK_BY_DURATION;
	param.cycle_time = 60*1000;	  /**< 1min */
	param.delay_fps = 0.0;
	param.MF = NULL;

	MenuConfigLua *menu_config = MenuConfigLua::GetInstance();
	int val = menu_config->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);

    switch (val) {
        case REC_4K25FPS: {
				#ifdef USE_IMX335
                param.video_size = Size(3840, 2160);
				#ifdef SUPPORT_PSKIP_ENABLE
				param.framerate = 30;
				#else
				param.framerate = 25;
				#endif
				#else
				param.video_size = Size(3840, 2160);
				param.framerate = 25;
				#endif
                param.bitrate = (1 << 20) * 30;
                
                param.enc_type = VENC_H265;
                param.vi_chn = 0;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
           break;
        case REC_1080P30FPS: {
                param.video_size = Size(1920, 1080);
                param.bitrate = (1 << 20) * 10;
                param.framerate = 30;
                param.enc_type = VENC_H265;
                param.vi_chn = 0;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
           break;
        case REC_720P30FPS: {
                param.video_size = Size(1280, 720);
                param.bitrate = (1 << 20) * 6;
                param.framerate = 30;
                param.enc_type = VENC_H265;
                param.vi_chn = 0;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
           break;
        default:
			param.video_size = Size(1920, 1080);
			param.bitrate = (1 << 20) * 14;
			param.framerate = 30;
			param.enc_type = VENC_H264;
			param.vi_chn = 0;
            param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
           break;
    }

    recorder->SetParam(param);

    return recorder;
}

Recorder *RecorderFactory::CreateUVCRecorder(RecorderType rec_type, Camera *camera, int p_viChn)
{
    Recorder *recorder = new BackCameraRecorder(camera);
    assert(recorder != NULL);
	MenuConfigLua *menu_config = MenuConfigLua::GetInstance();
	
    int recod_loop,recod_type;
	RecorderParam param;
	recorder->GetParam(param);
	param.audio_record = false;
	param.stream_type = 2;		  /**< 流录制 */
	param.pack_strategy = PACK_BY_DURATION;
    param.delay_fps = 0.0;
	param.MF = NULL; 
    recod_loop = menu_config->GetMenuIndexConfig(SETTING_RECORD_LOOP);/**< 无循环录制 */
    switch(recod_loop){
        case 0:
            param.cycle_time = 1*60*1000;
            break;
        case 1:
            param.cycle_time = 2*60*1000;
            break;
        case 2:
            param.cycle_time = 3*60*1000;
            break;
    }

    recod_type = menu_config->GetMenuIndexConfig(SETTING_RECORD_ENCODINGTYPE);//VENC_MJPEG; // PT_MJPEG
    switch(recod_type){
        case 0:
            param.enc_type = VENC_H265;
            break;
        case 1:
            param.enc_type = VENC_H264;
            break;
    }
    switch (rec_type) {
        case REC_U_1080P30FPS: {
                param.video_size = Size(1920, 1080);
                param.bitrate = (1 << 20) * 10;
                #ifdef SUPPORT_PSKIP_ENABLE
				param.framerate = 30;
				#else
				param.framerate = 25;
				#endif
                param.MF = NULL;
                param.vi_chn = 2;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
            break;
        case REC_U_720P30FPS: {
                param.video_size = Size(1280, 720);
                param.bitrate = (1 << 20) * 6;
                param.framerate = 30;
                param.MF = NULL;
                param.vi_chn = 2;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
            break;
        case REC_U_VGA30FPS: {
                param.video_size = Size(640, 480);
                param.bitrate = (1 << 19) * 5;
                param.framerate = 25;
                param.MF = NULL;
                param.vi_chn = 2;
                param.bitrate_ctrl = 0; // 0:CBR 1:VBR 2:FIXQP
            }
            break;
        default: {
            db_error("no recoder: %d", rec_type);
            return NULL;
        }
    }

    recorder->SetParam(param);

    return recorder;
}
