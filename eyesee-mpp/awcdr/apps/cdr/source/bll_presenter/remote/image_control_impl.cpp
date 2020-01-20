/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file image_control_impl.cpp
 * @brief image控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-16
 */

#include "interface/dev_ctrl_adapter.h"
#include "image_control_impl.h"
#include "common/app_log.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

#include <string.h>
#include <string>
#include <iostream>

#undef LOG_TAG
#define LOG_TAG "OverlayControlImpl"

using namespace EyeseeLinux;
using namespace std;

ImageControlImpl::ImageControlImpl(IPresenter *presenter)
{
    image_manager_ = ImageManager::GetInstance();
}

ImageControlImpl::~ImageControlImpl()
{
}

int ImageControlImpl::DefaultImageConfig()
{
    int ret = 0;

    ret = image_manager_->DefaultImageConfig();
    if (ret) {
        db_error("Do DefaultImageConfig fail!\n");
        return -1;
    }

    return 0;
}

int ImageControlImpl::SaveImageConfig()
{
    int ret = 0;

    ret = image_manager_->SaveImageConfig();
    if (ret) {
        db_error("Do SaveImageConfig fail!\n");
        return -1;
    }

    return 0;
}

int ImageControlImpl:: SetImageColour(int vi_ch, const AWImageColour &image_colour)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    if (image_colour.brightness < 0 || image_colour.brightness > 100) {
        db_error("Input brightness:%d error!\n", image_colour.brightness);
        return -1;
    }

    if (image_colour.contrast < 0 || image_colour.contrast > 100) {
        db_error("Input contrast:%d error!\n", image_colour.contrast);
        return -1;
    }

    if (image_colour.hue < 0 || image_colour.hue > 100) {
        db_error("Input hue:%d error!\n", image_colour.hue);
        return -1;
    }

    if (image_colour.saturation < 0 || image_colour.saturation > 100) {
        db_error("Input saturation:%d error!\n", image_colour.saturation);
        return -1;
    }

    if (image_colour.sharpness < 0 || image_colour.sharpness > 100) {
        db_error("Input sharpness:%d error!\n", image_colour.sharpness);
        return -1;
    }

    ret = image_manager_->SetBrightness(vi_ch, image_colour.brightness);
    if (ret) {
        db_error("Do SetBrightness fail! vi_ch:%d brightness:%d\n", vi_ch, image_colour.brightness);
        return -1;
    }

    ret = image_manager_->SetContrast(vi_ch, image_colour.contrast);
    if (ret) {
        db_error("Do SetContrast fail! vi_ch:%d  contrast:%d\n", vi_ch, image_colour.contrast);
        return -1;
    }

    ret = image_manager_->SetHue(vi_ch, image_colour.hue);
    if (ret) {
        db_error("Do SetHue fail! vi_ch:%d   hue:%d\n", vi_ch, image_colour.hue);
        return -1;
    }

    ret = image_manager_->SetSaturation(vi_ch, image_colour.saturation);
    if (ret) {
        db_error("Do SetSaturation fail! vi_ch:%d saturation:%d\n", vi_ch, image_colour.saturation);
        return -1;
    }

    ret = image_manager_->SetSharpness(vi_ch, image_colour.sharpness);
    if (ret) {
        db_error("Do SetSharpness fail! vi_ch:%d   sharpness:%d\n", vi_ch, image_colour.sharpness);
        return -1;
    }

    return 0;
}


int ImageControlImpl:: GetImageColour(int vi_ch, AWImageColour &image_colour)
{
    int ret = 0;
    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetBrightness(vi_ch, image_colour.brightness);
    if (ret) {
        db_error("Do GetBrightness fail! vi_ch:%d brightness:%d\n", vi_ch, image_colour.brightness);
        return -1;
    }

    ret = image_manager_->GetContrast(vi_ch, image_colour.contrast);
    if (ret) {
        db_error("Do GetContrast fail! vi_ch:%d  contrast:%d\n", vi_ch, image_colour.contrast);
        return -1;
    }

    ret = image_manager_->GetHue(vi_ch, image_colour.hue);
    if (ret) {
        db_error("Do GetHue fail! vi_ch:%d   hue:%d\n", vi_ch, image_colour.hue);
        return -1;
    }

    ret = image_manager_->GetSaturation(vi_ch, image_colour.saturation);
    if (ret) {
        db_error("Do GetSaturation fail! vi_ch:%d saturation:%d\n", vi_ch, image_colour.saturation);
        return -1;
    }

    ret = image_manager_->GetSharpness(vi_ch, image_colour.sharpness);
    if (ret) {
        db_error("Do GetSharpness fail! vi_ch:%d   sharpness:%d\n", vi_ch, image_colour.sharpness);
        return -1;
    }

    return 0;
}


int ImageControlImpl:: SetImageWhiteBalance(int vi_ch, int wb_type)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    if (wb_type > 8 || wb_type < 0) {
        db_error("Input wb_type:%d error!\n", wb_type);
        return -1;
    }

    ret = image_manager_->SetWhiteBalance(vi_ch, wb_type);
    if (ret) {
        db_error("Do SetWhiteBalance fail! vi_ch:%d  wb_type:%d\n", vi_ch, wb_type);
        return -1;
    }

    return 0;
}


int ImageControlImpl:: GetImageWhiteBalance(int vi_ch, int &wb_type)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetWhiteBalance(vi_ch, wb_type);
    if (ret) {
        db_error("Do GetWhiteBalance fail! vi_ch:%d  wb_type:%d\n", vi_ch, wb_type);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: SetImageExposure(int vi_ch, int exp_type, int exp_time)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    if (exp_type > 8 || exp_type < 0) {
        db_error("Input exp_type:%d error!\n", exp_type);
        return -1;
    }

    if (exp_time > 1000000 || exp_time < 0) {
        db_error("Input exp_time:%d error! MAX=1000000\n", exp_time);
        return -1;
    }

    ret = image_manager_->SetExposure(vi_ch, exp_type, exp_time);
    if (ret) {
        db_error("Do SetExposure fail! vi_ch:%d exp_type:%d exp_time:%d\n", vi_ch, exp_type, exp_time);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: GetImageExposure(int vi_ch, int &exp_type, int &exp_time)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetExposure(vi_ch, exp_type, exp_time);
    if (ret) {
        db_error("Do GetExposure fail! vi_ch:%d exp_type:%d exp_time:%d\n", vi_ch, exp_type, exp_time);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: SetImageWideDynamic(int vi_ch, int wd_leve)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    if (wd_leve > 8 || wd_leve < 0) {
        db_error("Input wd_leve:%d error!\n", wd_leve);
        return -1;
    }

    ret = image_manager_->SetWideDynamic(vi_ch, wd_leve);
    if (ret) {
        db_error("Do SetWideDynamic fail! vi_ch:%d  wd_leve:%d\n", vi_ch, wd_leve);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: GetImageWideDynamic(int vi_ch, int &wd_leve)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetWideDynamic(vi_ch, wd_leve);
    if (ret) {
        db_error("Do GetWideDynamic fail! vi_ch:%d  wd_leve:%d\n", vi_ch, wd_leve);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: SetImageDayNightMode(int vi_ch, const AWDayNightMode &mode)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->SetDayNightMode(vi_ch, mode.day_night_mode);
    if (ret) {
        db_error("Do SetDenoise fail! vi_ch:%d\n", vi_ch);
        return -1;
    }

    return 0;
}

int ImageControlImpl::GetImageDayNightMode(int vi_ch, AWDayNightMode &mode)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetDayNightMode(vi_ch, mode.day_night_mode);
    if (ret) {
        db_error("Do GetDayNightMode fail! vi_ch:%d \n", vi_ch);
        return -1;
    }

    return 0;
}

int ImageControlImpl::SetImageDenoise(int vi_ch, int dn_leve)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }
    if (dn_leve > 8 || dn_leve < 0) {
        db_error("Input dn_leve:%d error!\n", dn_leve);
        return -1;
    }

    ret = image_manager_->SetDenoise(vi_ch, dn_leve);
    if (ret) {
        db_error("Do SetDenoise fail! vi_ch:%d  dn_leve:%d\n", vi_ch, dn_leve);
        return -1;
    }

    return 0;
}

int ImageControlImpl::GetImageDenoise(int vi_ch, int &dn_leve)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_msg("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetDenoise(vi_ch, dn_leve);
    if (ret) {
        db_error("Do GetDenoise fail! vi_ch:%d  dn_leve:%d\n", vi_ch, dn_leve);
        return -1;
    }

    return 0;
}

int ImageControlImpl::SetImageFlickerFreq(int vi_ch, int freq)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    if (freq > 4 || freq < 0) {
        db_error("Input freq:%d error!\n", freq);
        return -1;
    }

    ret = image_manager_->SetFlickerFreq(vi_ch, freq);
    if (ret) {
        db_error("Do SetFlickerFreq fail! vi_ch:%d  freq:%d\n", vi_ch, freq);
        return -1;
    }

    return 0;
}

int ImageControlImpl::GetImageFlickerFreq(int vi_ch, int &freq)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetFlickerFreq(vi_ch, freq);
    if (ret) {
        db_error("Do GetFlickerFreq fail! vi_ch:%d  freq:%d\n", vi_ch, freq);
        return -1;
    }

    return 0;
}

int ImageControlImpl::SetImageFlip(int vi_ch, int enable_flip)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->SetFlip(vi_ch, enable_flip);
    if (ret) {
        db_error("Do SetFlip fail! vi_ch:%d  freq:%d\n", vi_ch, enable_flip);
        return -1;
    }

    return 0;
}

int ImageControlImpl::GetImageFlip(int vi_ch, int &enable_flip)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetFlip(vi_ch, enable_flip);
    if (ret) {
        db_error("Do GetFlip fail! vi_ch:%d  freq:%d\n", vi_ch, enable_flip);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: SetImageMirror(int vi_ch, int enable_mirror)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_error("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->SetMirror(vi_ch, enable_mirror);
    if (ret) {
        db_error("Do SetMirror fail! vi_ch:%d  enable_mirror:%d\n", vi_ch, enable_mirror);
        return -1;
    }

    return 0;
}

int ImageControlImpl:: GetImageMirror(int vi_ch, int &enable_mirror)
{
    int ret = 0;

    if (vi_ch >= AW_MAX_CHN_NUM) {
        db_msg("Input vi_ch:%d error! AW_MAX_CHN_NUM:%d \n", vi_ch, AW_MAX_CHN_NUM);
        return -1;
    }

    ret = image_manager_->GetMirror(vi_ch, enable_mirror);
    if (ret) {
        db_error("Do GetMirror fail! vi_ch:%d  enable_mirror:%d\n", vi_ch, enable_mirror);
        return -1;
    }

    return 0;
}

