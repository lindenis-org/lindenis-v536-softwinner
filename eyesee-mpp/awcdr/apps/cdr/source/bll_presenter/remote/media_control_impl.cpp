/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file media_control_impl.cpp
 * @brief 多媒体相关控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */

#include "media_control_impl.h"
#include "common/app_log.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "common/setting_menu_id.h"
#include "device_model/system/rtc.h"



#undef LOG_TAG
#define LOG_TAG "MediaControlImpl"

using namespace EyeseeLinux;
using namespace std;

#define MEDIA_CONFIG_FILE "/tmp/data/media_config.lua"


MediaControlImpl::MediaControlImpl(IPresenter *presenter)
{
    memset(&media_cfg_, 0, sizeof(media_cfg_));
    lua_cfg_ = new LuaConfig();
    wm = WindowManager::GetInstance();
    int ret = LoadMediaConfig();
    if (ret) {
        db_error("Do LoadMediaConfig fail:%d !", ret);
    }
}

MediaControlImpl::~MediaControlImpl()
{
    if (NULL != lua_cfg_) {
        delete lua_cfg_;
    }
}

int MediaControlImpl::SaveMediaConfig(void)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == this->lua_cfg_) {
        db_error("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    lua_cfg_->SetIntegerValue("media.camera[1].width",  media_cfg_.vi_width);
    lua_cfg_->SetIntegerValue("media.camera[1].height", media_cfg_.vi_height);
    lua_cfg_->SetIntegerValue("media.camera[1].venc.format",  media_cfg_.encode_format);
    lua_cfg_->SetIntegerValue("media.camera[1].venc.type",    media_cfg_.encode_type);
    lua_cfg_->SetIntegerValue("media.camera[1].venc.quality", media_cfg_.quality);

    for (cnt = 0; cnt < AW_MAX_ENCODE_CHN_NUM; cnt++) {
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.bps[%d]", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, media_cfg_.bps[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.fps[%d]", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, media_cfg_.fps[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.gop[%d]", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, media_cfg_.gop[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.size[%d].width", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, media_cfg_.encode_size[cnt].width);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.size[%d].height", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, media_cfg_.encode_size[cnt].height);
    }

    ret = lua_cfg_->SyncConfigToFile(MEDIA_CONFIG_FILE, "media");
    if (ret) {
        db_error("Do SyncConfigToFile error! file:%s\n", MEDIA_CONFIG_FILE);
        return 0;
    }

    return 0;
}

int MediaControlImpl::DefaultMediaConfig(void)
{
    return 0;
}

int MediaControlImpl::LoadMediaConfig(void)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == this->lua_cfg_) {
        db_error("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(MEDIA_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", MEDIA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/media_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(MEDIA_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again", MEDIA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/media_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(MEDIA_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!", MEDIA_CONFIG_FILE);
            return -1;
        }
    }

    media_cfg_.vi_width      = lua_cfg_->GetIntegerValue("media.camera[1].width");
    media_cfg_.vi_height     = lua_cfg_->GetIntegerValue("media.camera[1].height");
    media_cfg_.encode_format = lua_cfg_->GetIntegerValue("media.camera[1].venc.format");
    media_cfg_.encode_type   = lua_cfg_->GetIntegerValue("media.camera[1].venc.type");
    media_cfg_.quality       = lua_cfg_->GetIntegerValue("media.camera[1].venc.quality");

    for (cnt = 0; cnt < AW_MAX_ENCODE_CHN_NUM; cnt++) {
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.bps[%d]", cnt + 1);
        media_cfg_.bps[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.fps[%d]", cnt + 1);
        media_cfg_.fps[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.gop[%d]", cnt + 1);
        media_cfg_.gop[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.size[%d].width", cnt + 1);
        media_cfg_.encode_size[cnt].width = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "media.camera[1].venc.size[%d].height", cnt + 1);
        media_cfg_.encode_size[cnt].height = lua_cfg_->GetIntegerValue(tmp_str);
    }

    return 0;
}

void MediaControlImpl::SetMediaCtrlInstance(const CamRecMap &cam_rec_map, const RecorderGroup &rec_grp ,const CameraGroup &cam_map)
{
    db_msg("");
    cam_rec_map_ = cam_rec_map;
    rec_group_ = rec_grp;
    cam_map_group_ = cam_map;

    // 建立控制器类型与RecorderID的绑定
    // NOTE: 当前onvif不支持同一设备多摄像头接入，所以只绑定CAM_IPC_0这一组的recorder
    RecorderIDSet tutk_rec_set;
    RecorderIDSet onvif_rec_set;

    // 暂时只支持一个camera,
    // TODO: 单目模式摄像头切换时的处理
    RecorderGroup::iterator grp_iter;
    grp_iter = rec_group_.find(CAM_IPC_0);
    if (grp_iter == rec_group_.end()) {
        grp_iter = rec_group_.find(CAM_NORMAL_0);
        if (grp_iter == rec_group_.end()) {
            return;
        }
    }

    for (RecorderIDSet::iterator s_iter = grp_iter->second.begin();
         s_iter != grp_iter->second.end(); s_iter++) {

        if ((s_iter->sender_type & STREAM_SENDER_TUTK) == STREAM_SENDER_TUTK) {
            tutk_rec_set.push_back(*s_iter);
        }
        if ((s_iter->sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
            onvif_rec_set.push_back(*s_iter);
        }
    }

    ctrl_rec_map_.insert(make_pair(CTRL_TYPE_TUTK, tutk_rec_set));

    // only for onvif
    grp_iter = rec_group_.find(CAM_IPC_1);
    if (grp_iter == rec_group_.end()) {
        goto out;
    }

    for (RecorderIDSet::iterator s_iter = grp_iter->second.begin();
         s_iter != grp_iter->second.end(); s_iter++) {

        if ((s_iter->sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
            onvif_rec_set.push_back(*s_iter);
        }
    }

out:
    ctrl_rec_map_.insert(make_pair(CTRL_TYPE_ONVIF, onvif_rec_set));
}

void MediaControlImpl::SetStreamSenderInstance(const StreamSenderMap &stream_sender_map)
{
    stream_sender_map_ = stream_sender_map;
}

int MediaControlImpl::GetVideoChannelCnt(int &count)
{
    count = ctrl_rec_map_[CTRL_TYPE_ONVIF].size();
    db_msg("count: %d", count);

    return 0;
}

int MediaControlImpl::GetStreamUrl(int ch, char *url, int size)
{
    db_msg("ch: %d", ch);

    #ifdef ENABLE_RTSP
    RecorderIDSet rec_set = ctrl_rec_map_[CTRL_TYPE_ONVIF];
    RecorderID rec_id = rec_set[ch];
    Recorder *rec = cam_rec_map_[rec_id.cam_id][rec_id.rec_type];

    string rtsp_url = stream_sender_map_[rec]->GetUrl();
    strncpy(url, rtsp_url.c_str(), size);

    db_msg("url: %s", url);
    #endif

    return 0;
}
#if 0

int MediaControlImpl::GetOnvifSimpleProfile(int ch, OnvifSimpleProfile &profile)
{
    db_msg("ch: %d", ch);

    switch (ch) {
        case 0:
            profile.video_src_cfg.bound = {0, 0, 1920, 1080};
            //profile.video_src_cfg.ext_conf = NULL;

            profile.video_enc_cfg.enc_type = ENCODING_H264;

            profile.video_enc_cfg.width = 1920;
            profile.video_enc_cfg.height = 1080;
            profile.video_enc_cfg.quality = 3;

            profile.video_enc_cfg.rate_limit.bitrate = (4 * (1 << 20));
            profile.video_enc_cfg.rate_limit.framerate = 25;
            profile.video_enc_cfg.rate_limit.enc_interval = 1;

            profile.video_enc_cfg.h264_config.gov_len = 25;
            profile.video_enc_cfg.h264_config.h264_profile = H264_PROFILE_BASELINE;

            profile.video_enc_cfg.ss_timeout = 10;

            break;
        case 1:
            profile.video_src_cfg.bound = {0, 0, 1920, 1080};
            //profile.video_src_cfg.ext_conf = NULL;

            profile.video_enc_cfg.enc_type = ENCODING_H264;

            profile.video_enc_cfg.width = 640;
            profile.video_enc_cfg.height = 480;
            profile.video_enc_cfg.quality = 3;

            profile.video_enc_cfg.rate_limit.bitrate = (4 * (1 << 20));
            profile.video_enc_cfg.rate_limit.framerate = 30;
            profile.video_enc_cfg.rate_limit.enc_interval = 1;

            profile.video_enc_cfg.h264_config.gov_len = 30;
            profile.video_enc_cfg.h264_config.h264_profile = H264_PROFILE_BASELINE;

            profile.video_enc_cfg.ss_timeout = 10;
            break;
        default:
            break;
    }
    return 0;
}

int MediaControlImpl::GetOnvifVideoEncodingConfigOptions(int ch, OnvifVideoEncodingConfigOptions &options)
{
    db_msg("ch: %d", ch);

    switch (ch) {
        case 0: {
            options.h264_quality_range.f_max = 1.0f;
            options.h264_quality_range.f_min = 0.0f;

            options.h264_reslutions.push_back(::Size(1920, 1080));
            options.h264_reslutions.push_back(::Size(1280, 720));
            options.h264_reslutions.push_back(::Size(640, 480));
            //options.h264_reslutions.push_back(::Size(480, 320));

            options.h264_gov_len_range.i_max = 25;
            options.h264_gov_len_range.i_min = 15;

            options.h264_fps_range.i_max = 25;
            options.h264_fps_range.i_min = 15;

            options.h264_bitrate_range.i_max = (4 * (1 << 20));
            options.h264_bitrate_range.i_min = 0;

            options.h264_enc_interval_range.i_max = 2;
            options.h264_enc_interval_range.i_min = 1;

            options.h264_profiles.push_back(H264_PROFILE_BASELINE);

        }
            break;
        case 1: {
            options.h264_quality_range.f_max = 5.0f;
            options.h264_quality_range.f_min = 1.0f;

            options.h264_reslutions.push_back(::Size(640, 480));
            //options.h264_reslutions.push_back(::Size(480, 320));

            options.h264_gov_len_range.i_max = 30;
            options.h264_gov_len_range.i_min = 15;

            options.h264_fps_range.i_max = 30;
            options.h264_fps_range.i_min = 15;

            options.h264_bitrate_range.i_max = (4 * (1 << 20));
            options.h264_bitrate_range.i_min = 0;

            options.h264_enc_interval_range.i_max = 2;
            options.h264_enc_interval_range.i_min = 1;

            options.h264_profiles.push_back(H264_PROFILE_BASELINE);

        }
            break;
        default:
            break;
    }

    return 0;
}
#endif
int MediaControlImpl::SetVideoQuality(ControllerType ctrl_type, int ch,
                                  int width, int height, int bitrate, int framerate)
{
    db_msg("ch: %d", ch);

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    if (ctrl_rec_map_.size() == 0) {
        db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
        return -1;
    }

    RecorderIDSet s_rec = ctrl_rec_map_[ctrl_type];
    RecorderID rec_id = s_rec[ch];

    db_msg("update cam[%d] rec[%d] parameter, width[%d], height[%d], fps[%d], bitrate[%d]",
            rec_id.cam_id, rec_id.rec_type, width, height, framerate, bitrate);

    Recorder *recorder = cam_rec_map_[rec_id.cam_id][rec_id.rec_type];

    // stop record first
    recorder->StopRecord();

    uint16_t status = recorder->GetStatus();
    if (status == RECORDER_IDLE) { // when restart record
        RecorderParam param;
        recorder->GetParam(param);
        param.video_size = Size(width, height);
        param.framerate = framerate;
        param.bitrate = bitrate;
        recorder->SetParam(param);
    }

    // init and start recorder
    recorder->StartRecord();

    return 0;
}

int MediaControlImpl::GetMediaConfig(AWEnCodeConfig &media_cfg)
{
    media_cfg = media_cfg_;
    return 0;
}

int MediaControlImpl::SnapShot(ControllerType ctrl_type, int ch)
{
    int ret = 0;

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    if (ctrl_rec_map_.size() == 0) {
        db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
        return -1;
    }

    RecorderIDSet s_rec = ctrl_rec_map_[ctrl_type];
    RecorderID rec_id = s_rec[ch];

    Camera *camera = cam_rec_map_[rec_id.cam_id][rec_id.rec_type]->GetCamera();

    camera->TakePicture(1);

    return ret;
}

int MediaControlImpl::SetVencFrameRate(ControllerType ctrl_type, int ch, int framerate)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (framerate > 100 || framerate < 0) {
        db_error("Input framerate:%d error!\n", framerate);
        return -1;
    }

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    if (ctrl_rec_map_.size() == 0) {
        db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
        return -1;
    }

    RecorderIDSet s_rec = ctrl_rec_map_[ctrl_type];
    RecorderID rec_id = s_rec[ch];
    Recorder *recorder = cam_rec_map_[rec_id.cam_id][rec_id.rec_type];

    /* stop record first */
    recorder->StopRecord();

    uint16_t status = recorder->GetStatus();
    if (status == RECORDER_IDLE) { // when restart record
        RecorderParam param;
        recorder->GetParam(param);
        param.framerate = framerate;
        recorder->SetParam(param);
    }

    /* init and start recorder */
    recorder->StartRecord();

    media_cfg_.fps[ch] = framerate;
    return 0;
}

int MediaControlImpl::SetVencBitRate(ControllerType ctrl_type, int ch, int bitrate)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (bitrate < 10) {
        db_error("Input bitrate:%d error!\n", bitrate);
        return -1;
    }

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    if (ctrl_rec_map_.size() == 0) {
        db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
        return -1;
    }

    RecorderIDSet s_rec = ctrl_rec_map_[ctrl_type];
    RecorderID rec_id = s_rec[ch];
    Recorder *recorder = cam_rec_map_[rec_id.cam_id][rec_id.rec_type];

    /* stop record first */
    recorder->StopRecord();

    uint16_t status = recorder->GetStatus();
    if (status == RECORDER_IDLE) { // when restart record
        RecorderParam param;
        recorder->GetParam(param);
        param.bitrate = bitrate;
        recorder->SetParam(param);
    }

    /* init and start recorder */
    recorder->StartRecord();

    media_cfg_.bps[ch] = bitrate;
    return 0;
}

int MediaControlImpl::SetVencGop(ControllerType ctrl_type, int ch, int gop)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (gop > 1000 || gop < 0) {
        db_error("Input gop:%d error!\n", gop);
        return -1;
    }

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    media_cfg_.gop[ch] = gop;
    return 0;
}

int MediaControlImpl::SetVencType(ControllerType ctrl_type, int ch, int type)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (type > 10) {
        db_error("Input encode_type:%d error!\n", type);
        return -1;
    }

    media_cfg_.encode_type = type;
    return 0;
}

int MediaControlImpl::SetVencFormat(ControllerType ctrl_type, int ch, int format)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (format > 100) {
        db_error("Input format:%d error!\n", format);
        return -1;
    }

    media_cfg_.encode_format = format;
    return 0;
}


int MediaControlImpl::SetVencSize(ControllerType ctrl_type, int ch, int width, int height)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (cam_rec_map_.size() == 0) {
        db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
        return -1;
    }

    if (ctrl_rec_map_.size() == 0) {
        db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
        return -1;
    }

    RecorderIDSet s_rec = ctrl_rec_map_[ctrl_type];
    RecorderID rec_id = s_rec[ch];

    db_msg("update cam[%d] rec[%d] parameter, width[%d], height[%d]",
                                        rec_id.cam_id, rec_id.rec_type, width, height);

    Recorder *recorder = cam_rec_map_[rec_id.cam_id][rec_id.rec_type];

    // stop record first
    recorder->StopRecord();

    uint16_t status = recorder->GetStatus();
    if (status == RECORDER_IDLE) { // when restart record
        RecorderParam param;
        recorder->GetParam(param);
        param.video_size = Size(width, height);
        recorder->SetParam(param);
    }

    // init and start recorder
    recorder->StartRecord();

    media_cfg_.encode_size[ch].width  = width;
    media_cfg_.encode_size[ch].height = height;
    return 0;
}

int MediaControlImpl::SetVencQuality(ControllerType ctrl_type, int ch, int quality)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if (quality > 100) {
        db_error("Input quality:%d error!\n", quality);
        return -1;
    }

    media_cfg_.quality = quality;
    return 0;
}

int MediaControlImpl::CheckStatus()
{
        if (cam_rec_map_.size() == 0) {
            db_error("cam_rec_map_ is empty, maybe should call SetMediaCtrlInstance first");
            return -1;
        }
    
        if (ctrl_rec_map_.size() == 0) {
            db_error("ctrl_rec_map_ is empty, maybe should call add a sender_type first");
            return -1;
        }
    
        if (cam_map_group_.size() == 0)
        {
            db_error("cam_map_group_ is empty, maybe should call add a camera first");
            return -1;
        }
        return 0;
}

int MediaControlImpl::RemoteSendCmdToSettingPresenter(int msg,int index)
{

     win_presenter_map_ = wm->GetGuiPresenter();
     db_error("win_presenter_map_.size = %d",win_presenter_map_.size());
     map<WindowID, IGUIPresenter*>::iterator iter = win_presenter_map_.find(WINDOWID_SETTING_NEW);
     if(iter != win_presenter_map_.end())
     {
         db_error("index = %d",index);
         if(msg == MSG_SET_PARKING_MONITORY || msg == MSG_SET_RECORD_VOLUME){
              db_error("app set parking monitory or record volume,update icon");
              win_presenter_map_[WINDOWID_SETTING_NEW]->HandleGUIMessage(msg,index,1);
          }
          else
              win_presenter_map_[WINDOWID_SETTING_NEW]->HandleGUIMessage(msg,index);
     }
    return 0;
}


int MediaControlImpl::RemoteSendCmdToPreviewPresenter(int msg,int index)
{

     win_presenter_map_ = wm->GetGuiPresenter();
     map<WindowID, IGUIPresenter*>::iterator iter = win_presenter_map_.find(WINDOWID_PREVIEW);
     if(iter != win_presenter_map_.end())
     {
         win_presenter_map_[WINDOWID_PREVIEW]->HandleGUIMessage(msg,index); 
     }
    return 0;
}



int MediaControlImpl::SetVideoResoulation(ControllerType ctrl_type, int ch, int index)
{
     if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
     }

   //  RemoteSendCmdToPreviewPresenter(MSG_STREAM_RECORD_SWITCH,0);
   //  usleep(200*1000);
     RemoteSendCmdToSettingPresenter(MSG_SET_VIDEO_RESOULATION,index);
  //   usleep(200*1000);
   //  RemoteSendCmdToPreviewPresenter(MSG_STREAM_RECORD_SWITCH,1);
     return 0;
}


int MediaControlImpl::SetRecordTime(ControllerType ctrl_type, int ch, int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_RECORD_TIME,index);
    return 0;
}

int MediaControlImpl::SetRecordType(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }
    RemoteSendCmdToSettingPresenter(MSG_SET_RECORD_ENCODE_TYPE,index);
    return 0;
}

int MediaControlImpl::SetRecordDelayTime(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_RECORD_DELAY_TIME,index);
    return 0;
}
int MediaControlImpl::SetRecordAudioOnOff(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_RECORD_VOLUME,index);
    return 0;
}

int MediaControlImpl::SetRecordEisOnOff(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }
	
    return 0;
}

int MediaControlImpl::SetSlowRecord(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToPreviewPresenter(MSG_STREAM_RECORD_SWITCH,0);
    usleep(200*1000);
    RemoteSendCmdToSettingPresenter(MSG_SET_RECORD_SLOW_TIME,index);
    usleep(200*1000);
    RemoteSendCmdToPreviewPresenter(MSG_STREAM_RECORD_SWITCH,1);

    return 0;
}

int MediaControlImpl::SetPicResolution(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_PIC_RESOULATION,index);

    return 0;
}

int MediaControlImpl::SetTimeTakePic(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_TIME_TAKE_PIC,index);

    return 0;
}

int MediaControlImpl::SetAutoTimeTakePic(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_AUTO_TIME_TAKE_PIC,index);

    return 0;
}


int MediaControlImpl::SetContinuousPictureMode(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_PIC_CONTINOUS,index);

    return 0;
}

int MediaControlImpl::SetExposureValue(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_CAMERA_EXPOSURE,index);

    return 0;
}

int MediaControlImpl::SetWhiteBalance(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_CAMERA_WHITEBALANCE,index);

    return 0;
}

int MediaControlImpl::SetLightFreq(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }


    RemoteSendCmdToSettingPresenter(MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY,index);

    return 0;
}

int MediaControlImpl::SetPreviewFlip(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_CAMERA_IMAGEROTATION,index);

    return 0;
}

int MediaControlImpl::SetTimeWaterMark(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_CAMERA_TIMEWATERMARK,index);

    return 0;
}

int MediaControlImpl::FormatStorage(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_DEVICE_FORMAT,index);

    return 0;
}

int MediaControlImpl::SetCamerAutoScreenSaver(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_AUTO_TIME_SCREENSAVER,index);
    
    return 0;
}

int MediaControlImpl::SetCameraTimeShutDown(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_SET_AUTO_TIME_SHUTDOWN,index);
    
    return 0;  
}

int MediaControlImpl::SetDeviceLanguage(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    RemoteSendCmdToSettingPresenter(MSG_RM_LANG_CHANGED,index);
    
    return 0;  
}

int MediaControlImpl::SetDeviceDateTime(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_DEVICE_DATETIME,index);
    
    return 0;  

}

int MediaControlImpl::SetDeviceReset(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
    system("reboot");

    return 0;

}

int MediaControlImpl::SetDeviceLedSwitch(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_CAMERA_LEDINDICATOR,index);
    
    return 0; 

}

int MediaControlImpl::SetDeviceDistortioncalibrationSwitch(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }

    RemoteSendCmdToSettingPresenter(SETTING_CAMERA_DISTORTIONCALIBRATION,index);
    
    return 0; 

}

int MediaControlImpl::GetDeviceConfig(ControllerType ctrl_type,int ch,int index,SMsgAVIoctrSDVDevConfigResp &resp)
{
    GetDeviceMenuConfig(resp);
    return 0;
}

int MediaControlImpl::GetDeviceMenuConfig(SMsgAVIoctrSDVDevConfigResp &resp)
{
	SunChipMenuConfig menu_config;
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
	menuconfiglua ->GetMenuConfig(menu_config);
	resp.switch_record_eis = menu_config.switch_record_eis;
	resp.switch_camera_imagerotation = menu_config.switch_camera_imagerotation;
	resp.switch_camera_ledindicator = menu_config.switch_camera_ledindicator;
	resp.switch_camera_timewatermark = menu_config.switch_camera_timewatermark;
	resp.switch_camera_distortioncalibration = menu_config.switch_camera_distortioncalibration;
	/*****record******/
	resp.record_resolution.current = menu_config.record_resolution.current;
	resp.record_sound.current = menu_config.record_sound.current;
	resp.record_switch_wifi.current = menu_config.record_switch_wifi.current;
	resp.record_rear_resolution.current = menu_config.record_rear_resolution.current;
	resp.record_screen_brightness.current = menu_config.record_screen_brightness.current;
	resp.record_screen_disturb_mode.current = menu_config.record_screen_disturb_mode.current;
	resp.record_voice_take_photo.current = menu_config.record_voice_take_photo.current;
	resp.record_4g_network_switch.current = menu_config.record_4g_network_switch.current;
	resp.record_volume_selection.current = menu_config.record_volume_selection.current;
	resp.record_power_on_sound.current = menu_config.record_power_on_sound.current;
	resp.record_key_sound.current = menu_config.record_key_sound.current;
	resp.record_drivering_report.current = menu_config.record_drivering_report.current;
	resp.record_adas.current = menu_config.record_adas.current;
	resp.record_standby_clock.current = menu_config.record_standby_clock.current;
	resp.record_adas_forward_collision_waring.current = menu_config.record_adas_forward_collision_waring.current;
	resp.record_adas_lane_shift_reminding.current = menu_config.record_adas_lane_shift_reminding.current;
	resp.record_watchdog.current = menu_config.record_watchdog.current;
	resp.record_probeprompt.current = menu_config.record_probeprompt.current;
	resp.record_speedprompt.current = menu_config.record_speedprompt.current;
	resp.record_timewatermark.current = menu_config.record_timewatermark.current;
	resp.record_emerrecord.current = menu_config.record_emerrecord.current;
	resp.record_emerrecordsen.current = menu_config.record_emerrecordsen.current;
	resp.record_parkingwarnlamp_switch.current = menu_config.record_parkingwarnlamp_switch.current;
    resp.record_parkingmonitor_switch.current = menu_config.record_parkingmonitor_switch.current;
	resp.record_parkingabnormalmonitory_switch.current = menu_config.record_parkingabnormalmonitory_switch.current;
	resp.record_parkingloopabnormalnotice_switch.current = menu_config.record_parkingloopabnormalnotice_switch.current;
	resp.record_parkingloop_switch.current = menu_config.record_parkingloop_switch.current;
	resp.record_parkingloop_resolution.current = menu_config.record_parkingloop_resolution.current;
	resp.record_encodingtype.current = menu_config.record_encodingtype.current;
	resp.record_loop.current = menu_config.record_loop.current;
	resp.record_timelapse.current = menu_config.record_timelapse.current;
	resp.record_slowmotion.current = menu_config.record_slowmotion.current;
	/*photo*/
	resp.photo_resolution.current = menu_config.photo_resolution.current;
	resp.photo_dramashot.current = menu_config.photo_dramashot.current;
	resp.photo_timed.current = menu_config.photo_timed.current;
	resp.photo_auto.current = menu_config.photo_auto.current;
	/*****camera****/
	resp.camera_exposure.current = menu_config.camera_exposure.current;
	resp.camera_whitebalance.current = menu_config.camera_whitebalance.current;
	resp.camera_lightfreq.current = menu_config.camera_lightfreq.current;
	resp.camera_autoscreensaver.current = menu_config.camera_autoscreensaver.current;
	resp.camera_timedshutdown.current = menu_config.camera_timedshutdown.current;
	resp.device_language.current = menu_config.device_language.current;
	resp.device_datatime.current = menu_config.device_datatime.current;
	/*********wifiinfo**********/
	strncpy(resp.camera_wifiinfo.string1, menu_config.camera_wifiinfo.string1, sizeof(menu_config.camera_wifiinfo.string1) - 1);
	strncpy(resp.camera_wifiinfo.string2, menu_config.camera_wifiinfo.string2, sizeof(menu_config.camera_wifiinfo.string2) - 1);
	strncpy(resp.device_sysversion.string1, menu_config.device_sysversion.string1, sizeof(menu_config.device_sysversion.string1) - 1);
	strncpy(resp.device_update.string1, menu_config.device_update.string1, sizeof(menu_config.device_update.string1) - 1);
    strncpy(resp.device_carid.string1, menu_config.device_carid.string1, sizeof(menu_config.device_carid.string1) - 1);
    return 0;
}

int MediaControlImpl::RemotePreviewChange(ControllerType ctrl_type,int ch,int index)
{
#if 0
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }

    if(CheckStatus() < 0)
    {
       db_error("CheckStatus error!\n");
       return -1;
    }  
    RemoteSendCmdToPreviewPresenter(MSG_STREAM_RECORD_SWITCH,index);
#endif
    return 0;
}

int MediaControlImpl::RemoteSaveMenuconfig(ControllerType ctrl_type,int ch,int index)
{
    int ret = 0;
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    ret = menuconfiglua->SaveMenuAllConfig();
    if(ret < 0){
        db_msg("save menu config filed");
        return -1;
    }
    return 0;
}

int MediaControlImpl::RemoteClientDisconnect(void)
{
    return RemoteSendCmdToPreviewPresenter(MSG_REMOTE_CLIENT_DISCONNECT, 1);
}


int MediaControlImpl::SetParkingMonitor(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }
    RemoteSendCmdToSettingPresenter(MSG_SET_PARKING_MONITORY,index);
    return 0;
}

int MediaControlImpl::SetParkingMonitorLevel(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }
    RemoteSendCmdToSettingPresenter(MSG_SET_EMER_RECORD_SENSITIVITY,index);
    return 0;
}

int MediaControlImpl::SetSystemAudioLevel(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }
    RemoteSendCmdToSettingPresenter(MSG_SET_VOLUME_SELECTION,index);
    return 0;
}

int MediaControlImpl::SetMotionDetect(ControllerType ctrl_type,int ch,int index)
{
    if (ch > 10 || ch < 0) {
        db_error("Input ch:%d error!\n", ch);
        return -1;
    }
    RemoteSendCmdToSettingPresenter(MSG_SET_MOTION_DETECT,index);
    return 0;
}


int MediaControlImpl::SetDeviceDate(ControllerType ctrl_type, int ch, std::string str)
{
    db_error("SetDeviceDate: devices date is %s",str.c_str());
    struct tm *ptm;
    struct tm tm;
    int pos;
    std::string year_str;
    std::string mounth_str;
    std::string day_str;
    time_t timer;
	timer = time(NULL);
    ptm = localtime(&timer);
    pos = str.find_first_of("-");
    year_str = str.substr(0,pos);
    mounth_str = str.substr(pos+1,2);
    pos = str.find_last_of("-");
    day_str = str.substr(pos+1);
    db_error("date: %s %s %s",year_str.c_str(),mounth_str.c_str(),day_str.c_str());

    tm.tm_year = atoi(year_str.c_str()) - 1900;
	tm.tm_mon = atoi(mounth_str.c_str()) - 1;
	tm.tm_mday = atoi(day_str.c_str());
	tm.tm_hour = ptm->tm_hour;
	tm.tm_min = ptm->tm_min;
	tm.tm_sec = ptm->tm_sec;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;
    
    set_date_time(&tm);

    
    return 0;
}

int MediaControlImpl::SetDeviceTime(ControllerType ctrl_type, int ch, std::string str)
{
    struct tm *ptm;
    struct tm tm;
    int pos;
    std::string hour;
    std::string min;
    std::string seconds;
    time_t timer;
	timer = time(NULL);
    ptm = localtime(&timer);
    pos = str.find_first_of(":");
    hour= str.substr(0,pos);
    min = str.substr(pos+1,2);
    pos = str.find_last_of(":");
    seconds = str.substr(pos+1);
    db_error("date: %s %s %s",hour.c_str(),min.c_str(),seconds.c_str());

    tm.tm_year = ptm->tm_year;//oi(year_str.c_str()) - 1900;
	tm.tm_mon =  ptm->tm_mon;//oi(mounth_str.c_str()) - 1;
	tm.tm_mday = ptm->tm_mday;//oi(day_str.c_str());
	tm.tm_hour = atoi(hour.c_str());//m->tm_hour;
	tm.tm_min = atoi(min.c_str());//ptm->tm_min;
	tm.tm_sec = atoi(seconds.c_str());//ptm->tm_sec;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;
    
    set_date_time(&tm);
    return 0;
}


int MediaControlImpl::SetWifiSsid(ControllerType ctrl_type, int ch, std::string str)
{
    db_error("SetWifiSsid is %s",str.c_str());
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    menuconfiglua->SetWifiSsid(str);
    return 0;
}

int MediaControlImpl::SetWifiPassword(ControllerType ctrl_type, int ch, std::string str)
{
    db_error("SetWifiPassword is %s",str.c_str());
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    menuconfiglua->SetWifiPassword(str);
    return 0;
}


