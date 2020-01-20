/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file device_control_impl.h
 * @brief 设备控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */
#pragma once

#include "interface/dev_ctrl_adapter.h"
#include "bll_presenter/common_type.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

namespace EyeseeLinux {

/**
 * @brief 设备控制接口
 */
class DeviceControlImpl: public DeviceAdapter::DeviceControl {
    public:
        DeviceControlImpl(IPresenter *presenter);

        ~DeviceControlImpl();

        int SetDeviceName(const char *dev_name);

        int SetDeviceId(int dev_id);

        int GetDeviceId(int *dev_id);

        void GetDeviceInfo(AWDeviceInfo &dev_info);
        void GetDeviceInfo_ex(AWDeviceInfo_ex &dev_info);

        int GetDiskInfo(AWDiskInfo &disk_info);

        int FormatDisk(int disk_id);

        int GetFormatStatus(int disk_id, AWFormatStatus &format_status);

        int MountDisk(int disk_id);

        int UmountDisk(int disk_id);

        int RemoveFile(const char *filename);

        int SaveDeviceConfig(void);

        int DefaultDeviceConfig(void);
        
        int GetSystemVersion(char *buf);

    private:
        bool param_change_flag_;
        LuaConfig *lua_cfg_;
        AWDeviceInfo device_info_;

        int LoadDeviceConfig(void);
};

} // namespace EyeseeLinux
