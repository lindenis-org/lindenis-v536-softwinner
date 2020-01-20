/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file overlay_control_impl.cpp
 * @brief OSD,Cover控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-11
 */

#include "interface/dev_ctrl_adapter.h"
#include "overlay_control_impl.h"
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

OverlayControlImpl::OverlayControlImpl(IPresenter *presenter)
{
    overlay_manager_ = OsdManager::get();
}

OverlayControlImpl::~OverlayControlImpl()
{
}

int OverlayControlImpl::DefaultOverlayConfig()
{
    return 0;
}

int OverlayControlImpl::SaveOverlayConfig()
{
    int ret = 0;
    ret = overlay_manager_->saveOsdConfig();
    if (ret) {
        db_error("Do SaveOverlayConfig fail!\n");
        return -1;
    }

    return 0;
}


int OverlayControlImpl::SetTimeOsd(int stream_id, const AWTimeOsd &time_osd)
{
    int ret = 0;

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    if (0 == time_osd.osd_enable) {
        ret = overlay_manager_->disableTimeOsd();
        if (ret) {
            db_msg("Do stream_id:%d DisableTimeOsd faile!\n", stream_id);
            return -1;
        }
        //return 0;
    } else {
        ret = overlay_manager_->enableTimeOsd();
        if (ret) {
            db_msg("Do stream_id:%d EnableTimeOsd faile!\n", stream_id);
            return -1;
        }
    }

    ret = overlay_manager_->setTimeOsdPostion(time_osd.left, time_osd.top);
    if (ret) {
        db_msg("Do stream_id:%d SetTimeOsdPosition faile!\n", stream_id);
        return -1;
    }

    ret = overlay_manager_->setTimeOsdFormat(time_osd.date_format, time_osd.time_format);
    if (ret) {
        db_msg("Do stream_id:%d SetTimeOsdFormat faile!\n", stream_id);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::GetTimeOsd(int stream_id, AWTimeOsd &time_osd)
{
    int ret = 0;

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    ret = overlay_manager_->getTimeOsdStatus(time_osd.osd_enable);
    if (ret) {
        db_msg("Do stream_id:%d GetTimeOsdStatus faile!\n", stream_id);
        return -1;
    }

    ret = overlay_manager_->getTimeOsdFormat(time_osd.date_format, time_osd.time_format);
    if (ret) {
        db_msg("Do stream_id:%d GetTimeOsdFormat faile!\n", stream_id);
        return -1;
    }

    ret = overlay_manager_->getTimeOsdPostion(time_osd.left, time_osd.top);
    if (ret) {
        db_msg("Do stream_id:%d GetTimeOsdPosition faile!\n", stream_id);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::SetChannelOsd(int vi_chn, int stream_id, const AWChannelOsd &chn_osd)
{
    int ret = 0;

    if (vi_chn >= AW_MAX_CHN_NUM) {
        db_msg("Input vi_chn:%d error! AW_MAX_CHN_NUM:%d \n", vi_chn, AW_MAX_CHN_NUM);
        return -1;
    }

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    if (0 == chn_osd.osd_enable) {
        ret = overlay_manager_->disableChannelNameOsd(vi_chn);
        if (ret) {
            db_msg("Do vi_chn:%d stream_id:%d DisableChannelNameOsd faile!\n", vi_chn, stream_id);
            return -1;
        }
        //return 0;
    } else {
        ret = overlay_manager_->enableChannelNameOsd(vi_chn);
        if (ret) {
            db_msg("Do vi_chn:%d stream_id:%d EnableChannelNameOsd faile!\n", vi_chn, stream_id);
            return -1;
        }
    }

    ret = overlay_manager_->setChannelNameOsdPosition(vi_chn, chn_osd.left, chn_osd.top);
    if (ret) {
        db_msg("Do vi_chn:%d stream_id:%d SetChannelNameOsdPosition faile!\n", vi_chn, stream_id);
        return -1;
    }

    ret = overlay_manager_->setChannelNameOsdString(vi_chn, chn_osd.channel_name);
    if (ret) {
        db_msg("Do vi_chn:%d stream_id:%d SetChannelNameOsdString faile!\n", vi_chn, stream_id);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::GetChannelOsd(int vi_chn, int stream_id, AWChannelOsd &chn_osd)
{
    int ret = 0;

    if (vi_chn >= AW_MAX_CHN_NUM) {
        db_msg("Input vi_chn:%d error! AW_MAX_CHN_NUM:%d \n", vi_chn, AW_MAX_CHN_NUM);
        return -1;
    }

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    ret = overlay_manager_->getChannelNameOsdStatus(vi_chn, chn_osd.osd_enable);
    if (ret) {
        db_msg("Do vi_chn:%d stream_id:%d GetChannelNameOsdStatus faile!\n", vi_chn, stream_id);
        return -1;
    }

    ret = overlay_manager_->getChannelNameOsdPosition(vi_chn, chn_osd.left, chn_osd.top);
    if (ret) {
        db_msg("Do vi_chn:%d stream_id:%d GetChannelNameOsdPosition faile!\n", vi_chn, stream_id);
        return -1;
    }

    ret = overlay_manager_->getChannelNameOsdString(vi_chn, chn_osd.channel_name);
    if (ret) {
        db_msg("Do vi_chn:%d stream_id:%d GetChannelNameOsdString faile!\n", vi_chn, stream_id);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::SetDeviceOsd(int stream_id, const AWDeviceOsd &device_osd)
{
    int ret = 0;

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    if (0 == device_osd.osd_enable) {
        ret = overlay_manager_->disableDeviceNameOsd();
        if (ret) {
            db_msg("Do stream_id:%d DisableDeviceNameOsd faile!\n", stream_id);
            return -1;
        }
        //return 0;
    } else {
        ret = overlay_manager_->enableDeviceNameOsd();
        if (ret) {
            db_msg("Do  stream_id:%d EnableDeviceNameOsd faile!\n", stream_id);
            return -1;
        }
    }

    ret = overlay_manager_->setDeviceNameOsdPosition(device_osd.left, device_osd.top);
    if (ret) {
        db_msg("Do stream_id:%d SetDeviceNameOsdPosition faile! left:%d top:%d\n",
                                                    stream_id, device_osd.left, device_osd.top);
        return -1;
    }

    ret = overlay_manager_->setDeviceNameOsdString(device_osd.device_name);
    if (ret) {
        db_msg("Do stream_id:%d SetDeviceNameOsdString faile! device_name:%s\n",
                                                    stream_id, device_osd.device_name);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::GetDeviceOsd(int stream_id, AWDeviceOsd &device_osd)
{
    int ret = 0;

    if (stream_id >= AW_MAX_STREAM_TYPE) {
        db_msg("Input stream_id:%d error! AW_MAX_STREAM_TYPE:%d \n", stream_id, AW_MAX_STREAM_TYPE);
        return -1;
    }

    ret = overlay_manager_->getDeviceNameOsdStatus(device_osd.osd_enable);
    if (ret) {
        db_msg("Do  stream_id:%d GetDeviceNameOsdStatus faile!\n", stream_id);
        return -1;
    }

    ret = overlay_manager_->getDeviceNameOsdPosition(device_osd.left, device_osd.top);
    if (ret) {
        db_msg("Do stream_id:%d GetDeviceNameOsdPosition faile!\n", stream_id);
        return -1;
    }

    ret = overlay_manager_->getDeviceNameOsdString(device_osd.device_name);
    if (ret) {
        db_msg("Do stream_id:%d GetDeviceNameOsdString faile!\n", stream_id);
        return -1;
    }

    return 0;
}

int OverlayControlImpl::SetCoverOsd(int vi_chn, int cover_id, const AWCoverOsd &cover_osd)
{
    return 0;
}

int OverlayControlImpl::GetCoverOsd(int vi_chn, int cover_id, AWCoverOsd &cover_osd)
{
    return 0;
}
