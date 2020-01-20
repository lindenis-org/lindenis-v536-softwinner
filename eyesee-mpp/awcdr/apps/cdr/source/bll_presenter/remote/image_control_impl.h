/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file image_control_impl.h
 * @brief image控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-16
 */
#pragma once

#include "device_model/media/image_manager.h"
#include "interface/dev_ctrl_adapter.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

namespace EyeseeLinux
{

/**
 * @brief image控制接口
 */
class ImageControlImpl
    : public DeviceAdapter::ImageControl
{
    public:
        ImageControlImpl(IPresenter *presenter);

        ~ImageControlImpl();

        int SetImageColour(int vi_ch, const AWImageColour &image_colour);

        int GetImageColour(int vi_ch, AWImageColour &image_colour);

        int SetImageWhiteBalance(int vi_ch, int wb_type);

        int GetImageWhiteBalance(int vi_ch, int &wb_type);

        int SetImageExposure(int vi_ch, int exp_type, int exp_time);

        int GetImageExposure(int vi_ch, int &exp_type, int &exp_time);

        int SetImageWideDynamic(int vi_ch, int wd_leve);

        int GetImageWideDynamic(int vi_ch, int &wd_leve);

        int SetImageDayNightMode(int vi_ch, const AWDayNightMode &mode);

        int GetImageDayNightMode(int vi_ch, AWDayNightMode &mode);

        int SetImageDenoise(int vi_ch, int dn_leve);

        int GetImageDenoise(int vi_ch, int &dn_leve);

        int SetImageFlickerFreq(int vi_ch, int freq);

        int GetImageFlickerFreq(int vi_ch, int &freq);

        int SetImageFlip(int vi_ch, int enable_flip);

        int GetImageFlip(int vi_ch, int &enable_flip);

        int SetImageMirror(int vi_ch, int enable_mirror);

        int GetImageMirror(int vi_ch, int &enable_mirror);

        int SaveImageConfig(void);

        int DefaultImageConfig(void);

    private:
        ImageManager *image_manager_;
};

} // namespace EyeseeLinux
