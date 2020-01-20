/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file dev_ctrl_adapter.cpp
 * @brief 设备公共控制适配器接口实现类
 * @author id:826
 * @version v0.3
 * @date 2016-08-23
 */

#include "interface/dev_ctrl_adapter.h"
#include "media_control_impl.h"
#include "device_control_impl.h"
#include "system_control_impl.h"
#include "image_control_impl.h"
#include "overlay_control_impl.h"
#include "record_control_impl.h"
#include "event_control_impl.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "DeviceAdapter"

using namespace std;
using namespace EyeseeLinux;

DeviceAdapter::DeviceAdapter(EyeseeLinux::IPresenter *presenter)
{
    dev_ctrl_     = new DeviceControlImpl(presenter);
    media_ctrl_   = new MediaControlImpl(presenter);
    sys_ctrl_     = new SystemControlImpl(presenter);
    overlay_ctrl_ = new OverlayControlImpl(presenter);
    image_ctrl_   = new ImageControlImpl(presenter);
    record_ctrl_  = new RecordControlImpl(presenter);
    event_ctrl_   = new EventControlImpl(presenter);
}

DeviceAdapter::~DeviceAdapter()
{
    if (dev_ctrl_ != NULL) {
        delete dev_ctrl_;
    }

    if (media_ctrl_ != NULL) {
        delete media_ctrl_;
    }

    if (sys_ctrl_ != NULL) {
        delete sys_ctrl_;
    }

    if (overlay_ctrl_ != NULL) {
        delete overlay_ctrl_;
    }

    if (image_ctrl_ != NULL) {
        delete image_ctrl_;
    }

    if (record_ctrl_ != NULL) {
        delete record_ctrl_;
    }

    if (event_ctrl_ != NULL) {
        delete event_ctrl_;
    }
}

