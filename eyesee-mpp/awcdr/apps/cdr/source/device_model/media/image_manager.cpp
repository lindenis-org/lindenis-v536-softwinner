/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file  image_manager.cpp
 * @brief overlay模块控制
 * @author id:
 * @version v0.3
 * @date 2016-11-25
 * @verbatim
    History:
    - 2016-11-25, Create.
   @endverbatim
 */

#include "device_model/media/image_manager.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "common/thread.h"

#include <EyeseeCamera.h>
#include <utils/Mutex.h>

#include <sys/mount.h>
#include <string.h>
#include <list>

#undef LOG_TAG
#define LOG_TAG "image_manager.cpp"

using namespace EyeseeLinux;
using namespace std;

#define IMAGE_CONFIG_FILE "/tmp/data/image_config.lua"

ImageManager::ImageManager()
{
    init_flag_ = false;
    lua_cfg_   = new LuaConfig();
}

ImageManager::~ImageManager()
{
    init_flag_ = false;
    if (NULL != lua_cfg_) {
        delete lua_cfg_;
    }
}

int ImageManager::LoadImageConfig()
{
    int ret = 0, tmp = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    plan_time tmp_plan;
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(IMAGE_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", IMAGE_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/image_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(IMAGE_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", IMAGE_CONFIG_FILE);
        ret = lua_cfg_->LoadFromFile(IMAGE_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/image_config.lua /tmp/data/");
        if (ret) {
            db_error("Load %s failed!\n", IMAGE_CONFIG_FILE);
            return -1;
        }
    }

    for (cnt = 0; cnt < max_chn_; cnt++) {
        /* 3A, exposure config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].white_balance", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        white_balance_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].exposure_type", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        exposure_type_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].exposure_time", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        exposure_time_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].wide_dynamic", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        wide_dynamic_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].denoise", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        denoise_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].flicker_freq", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        flicker_freq_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].flip", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        flip_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].mirror", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        mirror_.push_back(tmp);

        /* image colour config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.brightness", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        brightness_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.contrast", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        contrast_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.saturation", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        saturation_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.sharpness", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        sharpness_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.hue", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        hue_.push_back(tmp);

        /* Day night mode config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.detect_type", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        detect_src_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.day_night_mode", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        day_night_mode_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.trigger_leve", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        trigger_leve_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.delay_time", cnt + 1);
        tmp = lua_cfg_->GetIntegerValue(tmp_str);
        trigger_delay_time_.push_back(tmp);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.start_hour", cnt + 1);
        tmp_plan.start_hour = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.start_minute", cnt + 1);
        tmp_plan.start_minute = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.end_hour", cnt + 1);
        tmp_plan.end_hour = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.end_minute", cnt + 1);
        tmp_plan.end_minute = lua_cfg_->GetIntegerValue(tmp_str);
        daymode_time_.push_back(tmp_plan);

#if 0
        printf("chn:%d  white_balance:%d \n", cnt, white_balance_[cnt]);
        printf("chn:%d  exposure_type:%d \n", cnt, exposure_type_[cnt]);
        printf("chn:%d  exposure_time:%d \n", cnt, exposure_time_[cnt]);
        printf("chn:%d  wide_dynamic:%d \n",  cnt, wide_dynamic_[cnt]);
        printf("chn:%d  denoise:%d \n",       cnt, denoise_[cnt]);
        printf("chn:%d  flicker_freq:%d \n",  cnt, flicker_freq_[cnt]);
        printf("chn:%d  flip:%d \n",          cnt, flip_[cnt]);
        printf("chn:%d  mirror:%d \n",        cnt, mirror_[cnt]);
        printf("chn:%d  brightness:%d \n",    cnt, brightness_[cnt]);
        printf("chn:%d  contrast:%d \n",      cnt, contrast_[cnt]);
        printf("chn:%d  saturation:%d \n",    cnt, saturation_[cnt]);
        printf("chn:%d  sharpness:%d \n",     cnt, sharpness_[cnt]);
        printf("chn:%d  hue:%d \n",           cnt, hue_[cnt]);
        printf("chn:%d  detect_src:%d \n",    cnt, detect_src_[cnt]);
        printf("chn:%d  daynight_mode:%d \n", cnt, day_night_mode_[cnt]);
        printf("chn:%d  trigger_leve:%d \n",  cnt, trigger_leve_[cnt]);
        printf("chn:%d  delay_time:%d \n",    cnt, trigger_delay_time_[cnt]);

        printf("chn:%d  start_hour:%d \n",    cnt, daymode_time_[cnt].start_hour);
        printf("chn:%d  start_minute:%d \n",  cnt, daymode_time_[cnt].start_minute);
        printf("chn:%d  end_hour:%d \n",      cnt, daymode_time_[cnt].end_hour);
        printf("chn:%d  end_minute:%d \n",    cnt, daymode_time_[cnt].end_minute);
#endif
    }

    return 0;
}

int ImageManager::SaveImageConfig()
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    for (cnt = 0; cnt < max_chn_; cnt++) {
        /* 3A, exposure config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].white_balance", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, white_balance_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].exposure_type", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, exposure_type_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].exposure_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, exposure_time_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].wide_dynamic", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, wide_dynamic_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].denoise", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, denoise_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].flicker_freq", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, flicker_freq_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].flip", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, flip_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].mirror", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, mirror_[cnt]);

        /* image colour config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.brightness", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, brightness_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.contrast", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, contrast_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.saturation", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, saturation_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.sharpness", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, sharpness_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].image_color.hue", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, hue_[cnt]);

        /* Day night mode config */
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.detect_type", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, detect_src_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.day_night_mode", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, day_night_mode_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.trigger_leve", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, trigger_leve_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.delay_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, trigger_delay_time_[cnt]);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.start_hour", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, daymode_time_[cnt].start_hour);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.start_minute", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, daymode_time_[cnt].start_minute);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.end_hour", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, daymode_time_[cnt].end_hour);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "image.camera[%d].day_night_mode.end_minute", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, daymode_time_[cnt].end_minute);

#if 0
        printf("chn:%d  white_balance:%d \n", cnt, white_balance_[cnt]);
        printf("chn:%d  exposure_type:%d \n", cnt, exposure_type_[cnt]);
        printf("chn:%d  exposure_time:%d \n", cnt, exposure_time_[cnt]);
        printf("chn:%d  wide_dynamic:%d \n",  cnt, wide_dynamic_[cnt]);
        printf("chn:%d  denoise:%d \n",       cnt, denoise_[cnt]);
        printf("chn:%d  flicker_freq:%d \n",  cnt, flicker_freq_[cnt]);
        printf("chn:%d  flip:%d \n",          cnt, flip_[cnt]);
        printf("chn:%d  mirror:%d \n",        cnt, mirror_[cnt]);
        printf("chn:%d  brightness:%d \n",    cnt, brightness_[cnt]);
        printf("chn:%d  contrast:%d \n",      cnt, contrast_[cnt]);
        printf("chn:%d  saturation:%d \n",    cnt, saturation_[cnt]);
        printf("chn:%d  sharpness:%d \n",     cnt, sharpness_[cnt]);
        printf("chn:%d  hue:%d \n",           cnt, hue_[cnt]);
        printf("chn:%d  detect_src:%d \n",    cnt, detect_src_[cnt]);
        printf("chn:%d  daynight_mode:%d \n", cnt, day_night_mode_[cnt]);
        printf("chn:%d  trigger_leve:%d \n",  cnt, trigger_leve_[cnt]);
        printf("chn:%d  delay_time:%d \n",    cnt, trigger_delay_time_[cnt]);

        printf("chn:%d  start_hour:%d \n",    cnt, daymode_time_[cnt].start_hour);
        printf("chn:%d  start_minute:%d \n",  cnt, daymode_time_[cnt].start_minute);
        printf("chn:%d  end_hour:%d \n",      cnt, daymode_time_[cnt].end_hour);
        printf("chn:%d  end_minute:%d \n",    cnt, daymode_time_[cnt].end_minute);
#endif
    }

    ret = lua_cfg_->SyncConfigToFile(IMAGE_CONFIG_FILE, "image");
    if (ret) {
        db_error("Do SyncConfigToFile error! file:%s\n", IMAGE_CONFIG_FILE);
        return 0;
    }

    return 0;
}


int ImageManager::DefaultImageConfig()
{
    return 0;
}

int ImageManager::InitImage(std::map<CameraID, Camera*> &camera_map)
{
    int ret = 0;

    AutoMutex lock(image_lock_);

    if (true == init_flag_) {
        db_error("This Image haved Inited!\n");
        return 0;
    }

    max_chn_ = 0;
    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map.begin(); cam_iter != camera_map.end(); cam_iter++, max_chn_++) {
        camera_list_.push_back(cam_iter->second->GetEyeseeCamera());
    }

    LoadImageConfig();

    init_flag_ = true;
    return 0;
}

int ImageManager::ExitImage()
{
    int ret = 0;

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image haved Exited!\n");
        return 0;
    }
    init_flag_ = false;

    for (int cnt = 0; cnt < max_chn_; cnt++) {
        camera_list_[cnt] = NULL;
    }
    return 0;
}

/**
 * @brief 设置/获取 color 参数
 */
int ImageManager::SetBrightness(int vi_ch, int brightness)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (brightness_[vi_ch] == brightness) {
        db_error("Input vi_ch:%d brightness is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    if (camera_list_.empty()) {
        db_error("The vi_ch:%d camera_list_ is NULL!\n", vi_ch);
        return -1;
    }

    status_t status = 0;
    CameraParameters param;
    ISP_SINGLE_S     value;
    status = camera_list_[vi_ch]->getISPParameters(param);
    if (status) {
        db_error("Do getISPParameters fails!\n");
        return -1;
    }

    value = param.getBrightnessValue();
    value.value[0] = brightness;
    param.setBrightnessValue(value);

    /*
    status = camera_list_->setISPParameters(param);
    if (status) {
        db_error("Do setISPParameters fails!\n");
        return -1;
    }
    */

    brightness_[vi_ch] = brightness;
    return 0;
}

int ImageManager::GetBrightness(int vi_ch, int &brightness)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    brightness = brightness_[vi_ch];
    return 0;
}


int ImageManager::SetContrast(int vi_ch, int contrast)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (contrast_[vi_ch] == contrast) {
        db_error("Input vi_ch:%d contrast is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    if (camera_list_.empty()) {
        db_error("The vi_ch:%d camera_list_ is NULL!\n", vi_ch);
        return -1;
    }

    status_t status = 0;
    CameraParameters param;
    ISP_CONTRAST_S   value;
    status = camera_list_[vi_ch]->getISPParameters(param);
    if (status) {
        db_error("Do getISPParameters fails!\n");
        return -1;
    }

    value = param.getContrastValue();
    #if 0
    value.tuning_cfg[0].lower = 1;
    value.tuning_cfg[0].upper = 100;
    value.tuning_cfg[0].strength = contrast;
    value.strength[0] = contrast;
    #endif
    param.setContrastValue(value);

    /*
    status = camera_list_->setISPParameters(param);
    if (status) {
        db_error("Do setISPParameters fails!\n");
        return -1;
    }
    */

    contrast_[vi_ch] = contrast;
    return 0;
}

int ImageManager::GetContrast(int vi_ch, int &contrast)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    contrast = contrast_[vi_ch];
    return 0;
}


int ImageManager::SetSaturation(int vi_ch, int saturation)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (saturation_[vi_ch] == saturation) {
        db_msg("Input vi_ch:%d saturation is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    if (camera_list_.empty()) {
        db_error("The vi_ch:%d camera_list_ is NULL!\n", vi_ch);
        return -1;
    }

    status_t status = 0;
    CameraParameters param;
    ISP_SATURATION_S value;
    status = camera_list_[vi_ch]->getISPParameters(param);
    if (status) {
        db_error("Do getISPParameters fails!\n");
        return -1;
    }

    value = param.getSaturationValue();
    value.tuning_cfg[0].cb    = saturation;
    value.tuning_cfg[0].cr    = saturation;
    value.tuning_cfg[0].value[0] = saturation;
    param.setSaturationValue(value);

    /*
    status = camera_list_->setISPParameters(param);
    if (status) {
        db_error("Do setISPParameters fails!\n");
        return -1;
    }
    */

    saturation_[vi_ch] = saturation;
    return 0;
}

int ImageManager::GetSaturation(int vi_ch, int &saturation)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    saturation = saturation_[vi_ch];
    return 0;
}


int ImageManager::SetSharpness(int vi_ch, int sharpness)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (sharpness_[vi_ch] == sharpness) {
        db_msg("Input vi_ch:%d sharpness is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    if (camera_list_.empty()) {
        db_error("The vi_ch:%d camera_list_ is NULL!\n", vi_ch);
        return -1;
    }

    status_t status = 0;
    CameraParameters   param;
    ISP_SHARPEN_ATTR_S value;
    status = camera_list_[vi_ch]->getISPParameters(param);
    if (status) {
        db_error("Do getISPParameters fails!\n");
        return -1;
    }

    value = param.getSharpnessValue();
    #if 0
    value.bEnable  = AW_TRUE;
    value.enOpType = OP_TYPE_MANUAL;
    value.stManual.u8SharpenD   = 0x33;
    value.stManual.u8SharpenUd  = 0x33;
    value.stManual.u8SharpenRGB = 0x33;
    value.stRGBSharpenAttr.u8LutStrength = sharpness;
    #endif
    param.setSharpnessValue(value);

    /*
    status = camera_list_->setISPParameters(param);
    if (status) {
        db_error("Do setISPParameters fails!\n");
        return -1;
    }
    */

    sharpness_[vi_ch] = sharpness;
    return 0;
}

int ImageManager::GetSharpness(int vi_ch, int &sharpness)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    sharpness = sharpness_[vi_ch];
    return 0;
}

int ImageManager::SetHue(int vi_ch, int hue)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (hue_[vi_ch] == hue) {
        db_msg("Input vi_ch:%d hue is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    if (camera_list_.empty()) {
        db_error("The vi_ch:%d camera_list_ is NULL!\n", vi_ch);
        return -1;
    }

    status_t status = 0;
    CameraParameters param;
    ISP_HUE_S        value;
    status = camera_list_[vi_ch]->getISPParameters(param);
    if (status) {
        db_error("Do getISPParameters fails!\n");
        return -1;
    }

    value = param.getHueValue();
    value.xxx = hue;
    param.setHueValue(value);

    /*
    status = camera_list_->setISPParameters(param);
    if (status) {
        db_error("Do setISPParameters fails!\n");
        return -1;
    }
    */

    hue_[vi_ch] = hue;
    return 0;
}

int ImageManager::GetHue(int vi_ch, int &hue)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }

    hue = hue_[vi_ch];
    return 0;
}



/**
 * @brief 设置/获取 white balance 参数
 */
int ImageManager::SetWhiteBalance(int vi_ch, int wb_type)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (white_balance_[vi_ch] == wb_type) {
        db_msg("Input vi_ch:%d wb_type is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    white_balance_[vi_ch] = wb_type;
    return 0;
}

int ImageManager::GetWhiteBalance(int vi_ch, int &wb_type)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    wb_type = white_balance_[vi_ch];
    return 0;
}

/**
 * @brief 设置/获取 exposure 参数
 */
int ImageManager::SetExposure(int vi_ch, int exp_type, int exp_time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (exposure_type_[vi_ch] == exp_type && exposure_time_[vi_ch] == exp_time) {
        db_msg("Input vi_ch:%d exp_type and exp_time is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    exposure_type_[vi_ch] = exp_type;
    exposure_time_[vi_ch] = exp_time;
    return 0;
}

int ImageManager::GetExposure(int vi_ch, int &exp_type, int &exp_time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    exp_type = exposure_type_[vi_ch];
    exp_time = exposure_time_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 wide dynamic 参数
 */
int ImageManager::SetWideDynamic(int vi_ch, int wd_leve)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (wide_dynamic_[vi_ch] == wd_leve) {
        db_msg("Input vi_ch:%d wd_leve is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    wide_dynamic_[vi_ch] = wd_leve;
    return 0;
}

int ImageManager::GetWideDynamic(int vi_ch, int &wd_leve)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    wd_leve = wide_dynamic_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 denoise 参数
 */
int ImageManager::SetDenoise(int vi_ch, int dn_leve)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (denoise_[vi_ch] == dn_leve) {
        db_msg("Input vi_ch:%d dn_leve is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    denoise_[vi_ch] = dn_leve;
    return 0;
}

int ImageManager::GetDenoise(int vi_ch, int &dn_leve)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    dn_leve = denoise_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 flicker freq 参数
 */
int ImageManager::SetFlickerFreq(int vi_ch, int freq)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (flicker_freq_[vi_ch] == freq) {
        db_msg("Input vi_ch:%d freq is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    flicker_freq_[vi_ch] = freq;
    return 0;
}

int ImageManager::GetFlickerFreq(int vi_ch, int &freq)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    freq = flicker_freq_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 flip 参数
 */
int ImageManager::SetFlip(int vi_ch, int flip_enable)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (flip_[vi_ch] == flip_enable) {
        db_msg("Input vi_ch:%d flip_enable is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    flip_[vi_ch] = flip_enable;
    return 0;
}

int ImageManager::GetFlip(int vi_ch, int &flip_enable)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    flip_enable = flip_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 mirror 参数
 */
int ImageManager::SetMirror(int vi_ch, int mirror_enable)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (mirror_[vi_ch] == mirror_enable) {
        db_msg("Input vi_ch:%d mirror_enable is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    mirror_[vi_ch] = mirror_enable;
    return 0;
}

int ImageManager::GetMirror(int vi_ch, int &mirror_enable)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    mirror_enable = mirror_[vi_ch];
    return 0;
}


/**
 * @brief 设置/获取 Day Night mode 参数
 */
int ImageManager::SetIrcutDetect(int vi_ch, int detect_src)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (detect_src_[vi_ch] == detect_src) {
        db_msg("Input vi_ch:%d detect_src is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    detect_src_[vi_ch] = detect_src;
    return 0;
}

int ImageManager::GetIrcutDetect(int vi_ch, int &detect_src)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    detect_src = detect_src_[vi_ch];
    return 0;
}


int ImageManager::SetDayNightMode(int vi_ch, int daynight_mode)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (day_night_mode_[vi_ch] == daynight_mode) {
        db_msg("Input vi_ch:%d daynight_mode is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    day_night_mode_[vi_ch] = daynight_mode;
    return 0;
}

int ImageManager::GetDayNightMode(int vi_ch, int &daynight_mode)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    daynight_mode = day_night_mode_[vi_ch];
    return 0;
}

int ImageManager::SetTriggerSensitive(int vi_ch, int sensitive)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (trigger_leve_[vi_ch] == sensitive) {
        db_msg("Input vi_ch:%d sensitive is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    trigger_leve_[vi_ch] = sensitive;
    return 0;
}

int ImageManager::GetTriggerSensitive(int vi_ch, int &sensitive)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    sensitive = trigger_leve_[vi_ch];
    return 0;
}


int ImageManager::SetTriggerDelayTime(int vi_ch, int delay_time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (trigger_delay_time_[vi_ch] == delay_time) {
        db_msg("Input vi_ch:%d sensitive is the same!\n", vi_ch);
        return 0;
    }

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }



    trigger_delay_time_[vi_ch] = delay_time;
    return 0;
}

int ImageManager::GetTriggerDelayTime(int vi_ch, int &delay_time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    delay_time = trigger_delay_time_[vi_ch];
    return 0;
}


int ImageManager::SetPlanTime(int vi_ch, plan_time time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);
    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    daymode_time_[vi_ch].start_hour   = time.start_hour;
    daymode_time_[vi_ch].start_minute = time.start_minute;
    daymode_time_[vi_ch].end_hour     = time.end_hour;
    daymode_time_[vi_ch].end_minute   = time.end_minute;
    return 0;
}

int ImageManager::GetPlanTime(int vi_ch, plan_time &time)
{
    int ret = 0;

    if (vi_ch >= max_chn_) {
        db_error("Input vi_ch:%d error!\n", vi_ch);
        return -1;
    }

    AutoMutex lock(image_lock_);

    if (false == init_flag_) {
        db_error("This Image mode don't inited!\n");
        return -1;
    }


    time.start_hour   = daymode_time_[vi_ch].start_hour;
    time.start_minute = daymode_time_[vi_ch].start_minute;
    time.end_hour     = daymode_time_[vi_ch].end_hour;
    time.end_minute   = daymode_time_[vi_ch].end_minute;
    return 0;
}

