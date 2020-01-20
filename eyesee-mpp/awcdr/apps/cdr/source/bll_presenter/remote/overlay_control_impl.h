/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file overlay_control_impl.h
 * @brief OSD,Cover控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-11
 */
#pragma once

#include "device_model/media/osd_manager.h"
#include "interface/dev_ctrl_adapter.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

namespace EyeseeLinux
{

/**
 * @brief OSD,Cover控制接口
 */
class OverlayControlImpl
    : public DeviceAdapter::OverlayControl
{
    public:
        OverlayControlImpl(IPresenter *presenter);

        ~OverlayControlImpl();

        int SetTimeOsd(int stream_id, const AWTimeOsd &time_osd);

        int GetTimeOsd(int stream_id, AWTimeOsd &time_osd);

        int SetChannelOsd(int vi_chn, int stream_id, const AWChannelOsd &chn_osd);

        int GetChannelOsd(int vi_chn, int stream_id, AWChannelOsd &chn_osd);

        int SetDeviceOsd(int stream_id, const AWDeviceOsd &device_osd);

        int GetDeviceOsd(int stream_id, AWDeviceOsd &device_osd);

        int SetCoverOsd(int vi_chn, int cover_id, const AWCoverOsd &cover_osd);

        int GetCoverOsd(int vi_chn, int cover_id, AWCoverOsd &cover_osd);

        int SaveOverlayConfig(void);

        int DefaultOverlayConfig(void);

    private:
        OsdManager* overlay_manager_;
};

} // namespace EyeseeLinux
