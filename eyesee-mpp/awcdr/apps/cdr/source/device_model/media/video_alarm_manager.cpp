/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file video_alarm_manager.cpp
 * @brief 视频报警管理,包括移动侦测,遮盖报警.
 *
 * @author id:
 * @date 2016-12-09
 *
 * @verbatim
    History:
   @endverbatim
 */


#include "device_model/media/video_alarm_manager.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "common/thread.h"

#include <EyeseeCamera.h>
#include <utils/Mutex.h>

#include <string.h>


#undef LOG_TAG
#define LOG_TAG "video_alarm_manager.cpp"

using namespace EyeseeLinux;
using namespace std;

#define EVENT_CONFIG_FILE "/tmp/data/event_config.lua"

VideoAlarmManager::VideoAlarmManager()
{
    init_flag_ = false;
    chn_num_   = 0;
    lua_cfg_   = new LuaConfig();

    for (int cnt = 0; cnt < MAX_VI_CHN; cnt++) {
        eyesee_camera_[cnt] = NULL;

        md_enable_[cnt]     = 0;
        md_sensitive_[cnt]  = 0;
        md_delay_time_[cnt] = 0;

        cover_enable_[cnt]     = 0;
        cover_sensitive_[cnt]  = 0;
        cover_delay_time_[cnt] = 0;

        memset(&md_detect_area_[cnt],    0xff, sizeof(VideoDetectArea));
        memset(&cover_detect_area_[cnt], 0xff, sizeof(VideoDetectArea));
    }

    //ThreadCreate(&vda_thread_id_, NULL, VideoAlarmManager::VideoDetectThread, this);
}


VideoAlarmManager::~VideoAlarmManager()
{
    if (lua_cfg_) {
        delete lua_cfg_;
    }
}


void *VideoAlarmManager::VideoDetectThread(void * context)
{
    VideoAlarmManager *vda_manager = reinterpret_cast<VideoAlarmManager*>(context);
    prctl(PR_SET_NAME, "VideoDetectThread", 0, 0, 0);

    while(1) {
        #if 0
        vda_manager->Notify(MSG_VIDEO_MD_ALARM);
        printf("[FUN]:%s [LINE]:%d  =====>  MSG_VIDEO_MD_ALARM \n", __func__, __LINE__);
        sleep(2);
        vda_manager->Notify(MSG_VIDEO_COVER_ALARM);
        printf("[FUN]:%s [LINE]:%d  =====>  MSG_VIDEO_COVER_ALARM \n", __func__, __LINE__);
        #endif
        sleep(2);
    }

    return NULL;
}

int VideoAlarmManager::LoadVideoAlarmConfig()
{
    int ret = 0, tmp = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(EVENT_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", EVENT_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/event_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(EVENT_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", EVENT_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/event_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(EVENT_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!\n", EVENT_CONFIG_FILE);
            return -1;
        }
    }

    for (cnt = 0; cnt < MAX_VI_CHN; cnt++) {
        /* Get motion detect config. */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].md_enable", cnt + 1);
        md_enable_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].sensitive", cnt + 1);
        md_sensitive_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].delay_time", cnt + 1);
        md_delay_time_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        for (i = 0; i < VIDEO_DETECT_AREA; i++) {
            memset(tmp_str, 0, sizeof(tmp_str));
            snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].detect_area[%d]", cnt + 1, i + 1);
            tmp = lua_cfg_->GetIntegerValue(tmp_str);
            md_detect_area_[cnt].detect_area[i] = tmp & 0xff;
            //printf("Chn:%d  alarm_md detect_area[%d]=%d\n", cnt, i, md_detect_area_[cnt].detect_area[i]);
        }

        printf("Chn:%d  alarm_md md_enable_     %d\n", cnt, md_enable_[cnt]);
        printf("Chn:%d  alarm_md md_sensitive_  %d\n", cnt, md_sensitive_[cnt]);
        printf("Chn:%d  alarm_md md_delay_time_ %d\n", cnt, md_delay_time_[cnt]);

        /* Get cover detect config. */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].cover_enable", cnt + 1);
        cover_enable_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].sensitive", cnt + 1);
        cover_sensitive_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].delay_time", cnt + 1);
        cover_delay_time_[cnt] = lua_cfg_->GetIntegerValue(tmp_str);

        printf("Chn:%d  alarm_cover cover_enable_     %d\n", cnt, cover_enable_[cnt]);
        printf("Chn:%d  alarm_cover cover_sensitive_  %d\n", cnt, cover_sensitive_[cnt]);
        printf("Chn:%d  alarm_cover cover_delay_time_ %d\n", cnt, cover_delay_time_[cnt]);
    }

    return 0;
}


int VideoAlarmManager::SaveVideoAlarmConfig(void)
{
    int ret = 0, tmp = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    for (cnt = 0; cnt < MAX_VI_CHN; cnt++) {
        /* Save motion detect config. */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].md_enable", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, md_enable_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].sensitive", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, md_sensitive_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].delay_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, md_delay_time_[cnt]);

        for (i = 0; i < VIDEO_DETECT_AREA; i++) {
            memset(tmp_str, 0, sizeof(tmp_str));
            snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_md.camera[%d].detect_area[%d]", cnt + 1, i + 1);
            tmp = 0;
            tmp = md_detect_area_[cnt].detect_area[i];
            lua_cfg_->SetIntegerValue(tmp_str, tmp);
            //printf("Chn:%d  alarm_md detect_area[%d]=%d\n", cnt, i, md_detect_area_.detect_area[i]);
        }

        printf("Chn:%d  alarm_md md_enable_     %d\n", cnt, md_enable_[cnt]);
        printf("Chn:%d  alarm_md md_sensitive_  %d\n", cnt, md_sensitive_[cnt]);
        printf("Chn:%d  alarm_md md_delay_time_ %d\n", cnt, md_delay_time_[cnt]);

        /* Save cover detect config. */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].cover_enable", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, cover_enable_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].sensitive", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, cover_sensitive_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "event.alarm_cover.camera[%d].delay_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, cover_delay_time_[cnt]);

        printf("Chn:%d  alarm_cover cover_enable_     %d\n", cnt, cover_enable_[cnt]);
        printf("Chn:%d  alarm_cover cover_sensitive_  %d\n", cnt, cover_sensitive_[cnt]);
        printf("Chn:%d  alarm_cover cover_delay_time_ %d\n", cnt, cover_delay_time_[cnt]);
    }

    ret = lua_cfg_->SyncConfigToFile(EVENT_CONFIG_FILE, "event");
    if (ret) {
        db_error("Do SyncConfigToFile error! file:%s\n", EVENT_CONFIG_FILE);
        return 0;
    }

    return 0;
}

int VideoAlarmManager::DefaultVideoAlarmConfig(void)
{
    return 0;
}


int VideoAlarmManager::InitVideoAlarm(std::map<CameraID, Camera*> &camera_map)
{
    int ret = 0, chn = 0;

    AutoMutex lock(alarm_lock_);

    if (true == init_flag_) {
        db_error("This overlay haved inited!\n");
        return 0;
    }

    ret = LoadVideoAlarmConfig();
    if (ret) {
        db_error("Do LoadOverlayConfig fail!\n");
        return -1;
    }

    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map.begin(), chn = 0; cam_iter != camera_map.end(); cam_iter++, chn++) {
        if (chn > MAX_VI_CHN) {
            break;
        }
        eyesee_camera_[chn] = cam_iter->second->GetEyeseeCamera();
        channel_id_[chn]    = cam_iter->second->GetCameraID();
    }

    chn_num_ = chn;
    #if 0
    for (chn = 0; chn < chn_num_; chn++) {
        eyesee_camera_[chn]->setVdaDataCallback(this);
    }

    /*
     * This startVdaDetect run, must be call EyeseeCamera->startChannel(int chn) before.
     */
    status_t status = 0;
    for (chn = 0; chn < chn_num_; chn++) {
        status = eyesee_camera_[chn]->startVdaDetect(channel_id_[chn]);
        if (status) {
            db_error("Do startVdaDetect fail! status:%d\n", status);
            return -1;
        }
    }
    #endif

    init_flag_ = true;
    return 0;
}


int VideoAlarmManager::ExitVideoAlarm(void)
{
    AutoMutex lock(alarm_lock_);

    if (false == init_flag_) {
        db_error("This overlay haved Exited!\n");
        return 0;
    }

    #if 0
    for (int chn = 0; chn < chn_num_; chn++) {
        eyesee_camera_[chn]->stopVdaDetect(channel_id_[chn]);
    }
    #endif

    init_flag_ = false;
    return 0;
}


int VideoAlarmManager::EnableMdAlarm(int vi_chn)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    md_enable_[vi_chn] = 1;
    return 0;
}


int VideoAlarmManager::DisableMdAlarm(int vi_chn)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    md_enable_[vi_chn] = 0;
    return 0;
}


int VideoAlarmManager::GetMdAlarmStatus(int vi_chn, int &status)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    status = md_enable_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetMdAlarmSensitive(int vi_chn, int sensitive)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    if (sensitive < 0 || sensitive > 1000) {
        db_error("Input vi_ch:%d  sensitive:%d  error!\n", vi_chn, sensitive);
        return -1;
    }

    md_sensitive_[vi_chn] = sensitive;
    return 0;
}

int VideoAlarmManager::GetMdAlarmSensitive(int vi_chn, int &sensitive)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    sensitive = md_sensitive_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetMdAlarmDelayTime(int vi_chn, int delay_time)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    if (delay_time < 0 || delay_time > 1000) {
        db_error("Input vi_ch:%d  delay_time:%d  error!\n", vi_chn, delay_time);
        return -1;
    }

    md_delay_time_[vi_chn] = delay_time;
    return 0;
}

int VideoAlarmManager::GetMdAlarmDelayTime(int vi_chn, int &delay_time)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    delay_time = md_delay_time_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetMdAlarmArea(int vi_chn, const VideoDetectArea &detect_area)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    memcpy(&md_detect_area_[vi_chn], &detect_area, sizeof(VideoDetectArea));
    return 0;
}

int VideoAlarmManager::GetMdAlarmArea(int vi_chn, VideoDetectArea &detect_area)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    memcpy(&detect_area, &md_detect_area_[vi_chn], sizeof(VideoDetectArea));
    return 0;
}


int VideoAlarmManager::EnableCoverAlarm(int vi_chn)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    cover_enable_[vi_chn] = 1;
    return 0;
}


int VideoAlarmManager::DisableCoverAlarm(int vi_chn)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    cover_enable_[vi_chn] = 0;
    return 0;
}


int VideoAlarmManager::GetCoverAlarmStatus(int vi_chn, int &status)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    status = cover_enable_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetCoverAlarmSensitive(int vi_chn, int sensitive)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    if (sensitive < 0 || sensitive > 1000) {
        db_error("Input vi_ch:%d  sensitive:%d  error!\n", vi_chn, sensitive);
        return -1;
    }

    cover_sensitive_[vi_chn] = sensitive;
    return 0;
}

int VideoAlarmManager::GetCoverAlarmSensitive(int vi_chn, int &sensitive)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    sensitive = cover_sensitive_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetCoverAlarmDelayTime(int vi_chn, int delay_time)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    if (delay_time < 0 || delay_time > 1000) {
        db_error("Input vi_ch:%d  delay_time:%d  error!\n", vi_chn, delay_time);
        return -1;
    }

    cover_delay_time_[vi_chn] = delay_time;
    return 0;
}

int VideoAlarmManager::GetCoverAlarmDelayTime(int vi_chn, int &delay_time)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    delay_time = cover_delay_time_[vi_chn];
    return 0;
}

int VideoAlarmManager::SetCoverAlarmArea(int vi_chn, const VideoDetectArea &detect_area)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    memcpy(&cover_detect_area_[vi_chn], &detect_area, sizeof(VideoDetectArea));
    return 0;
}

int VideoAlarmManager::GetCoverAlarmArea(int vi_chn, VideoDetectArea &detect_area)
{
    int ret = 0;

    AutoMutex lock(alarm_lock_);
    if (false == init_flag_) {
        db_error("The overlay mode don't init!\n");
        return -1;
    }

    if (vi_chn >= MAX_VI_CHN) {
        db_error("Input vi_ch:%d error!\n", vi_chn);
        return -1;
    }

    memcpy(&detect_area, &cover_detect_area_[vi_chn], sizeof(VideoDetectArea));
    return 0;
}

void VideoAlarmManager::onMODData(int chnId, MotionDetectResult *p_sResult, EyeseeCamera* pCamera)
{
    // TODO: Check video alarm is tragger?
}

int VideoAlarmManager::CheckRect(RectangleArea &rect)
{
	if (rect.left < 0 || rect.top < 0 ||
		rect.width <= 0 || rect.height <= 0) {
		return -1;
	}

	if ((rect.left + rect.width) > 1920 || (rect.top + rect.height) > 1080) {
		return -1;
	}

	return 0;
}

int VideoAlarmManager::CheckRectInRect(RectangleArea &rect1 , RectangleArea &rect2)
{
	int ret = 0;

	ret = CheckRect(rect1);
    if (ret < 0) {
        db_error("Check_Rect rect1 error!\n");
        return -1;
    }

	ret = CheckRect(rect2);
    if (ret < 0) {
        db_error("Check_Rect rect2 error!\n");
        return -1;
    }

    /* 判断两个矩阵是否相交（反正法） */
    /* 矩形永在右边，不相交 */
    if (rect1.left <= rect2.left && (rect1.left + rect1.width) <= rect2.left) {
        return 0;
    }

    /* 矩形永在左边，不相交 */
    if (rect1.left >= rect2.left && rect1.left >= (rect2.left + rect2.width)) {
        return 0;
    }

    /* 矩形永在上边，不相交 */
    if (rect1.top >= rect2.top && rect1.top >= (rect2.top+ rect2.height)) {
        return 0;
    }

    /* 矩形永在下边，不相交 */
    if (rect1.top <= rect2.top && (rect1.top + rect1.height) <= rect2.top) {
        return 0;
    }

    return 1;
}


int VideoAlarmManager::JudgeRectInRect(int src_width, int src_height, RectangleArea &alarm_rect,
                                        VideoDetectArea &alarm_val, int &alarm_area_num)
{
    int ret = 0, row = 0, col = 0;
    int cell_h = 0, cell_w = 0, cnt = 0;
    int byte_num = 0, bit_num = 0, tmp;
    RectangleArea cell;

    if (src_width < VIDEO_DETECT_WIDTH || src_height < VIDEO_DETECT_HEIGTH) {
        db_error("Input src_width:%d or src_height:%d error!\n", src_width, src_height);
    }

    memset(&alarm_val, 0, sizeof(VideoDetectArea));
    alarm_area_num = 0;
    cell_w = src_width  / VIDEO_DETECT_WIDTH;
    cell_h = src_height / VIDEO_DETECT_HEIGTH;

    cnt = 0;
    for (row = 0; row < VIDEO_DETECT_HEIGTH; row++) {
        /* 赋值单元格的高和顶部坐标，且考虑最后一个单元格的高不能整除的情况 */
        cell.top    = row * cell_h;
        cell.height = cell_h;
        if (VIDEO_DETECT_HEIGTH == (row + 1)) {
            cell.height = src_height - cell.top;
        }

        for (col = 0; col < VIDEO_DETECT_WIDTH; col++) {
            /* 赋值单元格的宽和左部坐标，且考虑最后一个单元格的宽不能整除的情况 */
            cell.left  = col * cell_w;
            cell.width = cell_w;
            if (VIDEO_DETECT_WIDTH == (col + 1)) {
                cell.width = src_width - cell.left;
            }

            /* 获取每个单元格的侦测位使能标准 */
            byte_num = cnt / 8;
            bit_num  = cnt % 8;
            tmp = md_detect_area_[0].detect_area[byte_num];

            /* 判断单元格矩形框是否与底层上报的报警矩形是否相交 */
            ret = CheckRectInRect(alarm_rect, cell);
            if (ret > 0) {
                alarm_val.detect_area[byte_num] |= 0x01 << bit_num;

                /* 若该单元格矩形框被使能检测且已经相交，则报警块数使能结果加1 */
                if (tmp & (0x01 << bit_num)) {
                    alarm_area_num++;
                }
            }

            cnt++;
        }
    }

#if 0 // for debug
    tmp = 0;
    for(row = 0; row < VIDEO_DETECT_HEIGTH; row++) {
        for(col = 0; col < VIDEO_DETECT_WIDTH; col++) {
            if(alarm_val.detect_area[tmp/8] & 0x01)
                printf("1");
            else
                printf("0");
            disp[(int)(tmp/8)] >>= 1;
            tmp++;
        }
        printf("\n");
    }
#endif

    return 0;
}

