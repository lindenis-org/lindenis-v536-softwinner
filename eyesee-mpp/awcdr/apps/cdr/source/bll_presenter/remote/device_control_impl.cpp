/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file device_control_impl.cpp
 * @brief 设备控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */

#include "device_control_impl.h"
#include "device_model/storage_manager.h"
#include "device_model/media/media_file_manager.h"
#include "common/app_log.h"
#include "device_model/menu_config_lua.h"

#include <string.h>
#include <string>
#include <iostream>


#undef LOG_TAG
#define LOG_TAG "DeviceControlImpl"

#define DEVICE_CONFIG_FILE "/tmp/data/menu_config.lua"

using namespace EyeseeLinux;
using namespace std;

DeviceControlImpl::DeviceControlImpl(IPresenter *presenter)
{
    param_change_flag_ = false;
    lua_cfg_           = new LuaConfig();

    int ret = LoadDeviceConfig();
    if (ret) {
        // TODO: default config file.
    }
}

DeviceControlImpl::~DeviceControlImpl()
{
    param_change_flag_ = false;

    if (lua_cfg_) {
        delete lua_cfg_;
    }
}

int DeviceControlImpl::LoadDeviceConfig(void)
{
    int ret = 0;
    //std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(DEVICE_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv/", DEVICE_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(DEVICE_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", DEVICE_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(DEVICE_CONFIG_FILE);
        if (ret) {
            db_error("Load %s fail!\n", DEVICE_CONFIG_FILE);
            return -1;
        }
    }
    #if 0
    device_info_.device_id = lua_cfg_->GetIntegerValue("system.device.device_id");
    str = lua_cfg_->GetStringValue("system.device.device_name");
    strncpy(device_info_.device_name, str.c_str(), sizeof(device_info_.device_name) - 1);
    #endif
    return 0;
}

int DeviceControlImpl::SaveDeviceConfig(void)
{
    int ret = 0;
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    lua_cfg_->SetIntegerValue("system.device.device_id", device_info_.device_id);
    lua_cfg_->SetStringValue("system.device.device_name", device_info_.device_name);

    ret = lua_cfg_->SyncConfigToFile(DEVICE_CONFIG_FILE, "system");
    if (ret) {
        db_error("Do SyncConfigToFile error! file:%s\n", DEVICE_CONFIG_FILE);
        return -1;
    }

    return 0;
}

int DeviceControlImpl::DefaultDeviceConfig(void)
{
    db_msg("");
    return 0;
}

int DeviceControlImpl::SetDeviceName(const char *dev_name)
{
    if (NULL == dev_name) {
        db_msg("Input dev_name is NULL!\n");
        return -1;
    }

    strncpy(device_info_.device_name, dev_name, sizeof(device_info_.device_name) - 1);
    return 0;
}

int DeviceControlImpl::SetDeviceId(int dev_id)
{
    if (dev_id < 0) {
        db_msg("Input dev_id:%d is error!\n", dev_id);
        return -1;
    }

    device_info_.device_id = dev_id;
    return 0;
}

int DeviceControlImpl::GetDeviceId(int *dev_id)
{
    *dev_id = device_info_.device_id;

    return 0;
}

void DeviceControlImpl::GetDeviceInfo(AWDeviceInfo &dev_info)
{
    memcpy(&dev_info, &device_info_, sizeof(AWDeviceInfo));
}

int DeviceControlImpl::GetDiskInfo(AWDiskInfo &disk_info)
{
    int ret = 0;
    uint32_t avail = 0;
    uint32_t total = 0;

    db_msg("");

    disk_info.disk_num = 1;
    disk_info.disk_status[0].disk_id = 0;

    int status = StorageManager::GetInstance()->GetStorageStatus();

    switch (status) {
        case MOUNTED:
            disk_info.disk_status[0].disk_status = 0;
            break;
        case UMOUNT:
            disk_info.disk_status[0].disk_status = 1;
            break;
        case FORMATTING:
            disk_info.disk_status[0].disk_status = 2;
            break;
        case STORAGE_FS_ERROR:    
            disk_info.disk_status[0].disk_status = 3;
            break;
        case STORAGE_DISK_FULL:
            disk_info.disk_status[0].disk_status = 4;
            break;            
        default:
            disk_info.disk_status[0].disk_status = 5;
            break;
    }

    disk_info.disk_status[0].disk_type = 0;

    StorageManager::GetInstance()->GetStorageCapacity(&avail, &total);

    disk_info.disk_status[0].capacity = total;
    disk_info.disk_status[0].free_space = avail;

    return ret;
}

int DeviceControlImpl::FormatDisk(int disk_id)
{
    int ret;
    StorageManager *sm = StorageManager::GetInstance();

    db_msg("");

    sm->Update(MSG_STORAGE_UMOUNT);

    sleep(3);

    ret = sm->Format();
    if (ret < 0) db_error("format disk faild");

    return ret;
}

int DeviceControlImpl::GetFormatStatus(int disk_id, AWFormatStatus &format_status)
{
    db_msg("");
    return 0;
}

int DeviceControlImpl::MountDisk(int disk_id)
{
    int ret = 0;

    db_msg("");

    if (disk_id < 0) {
        db_msg("Input disk_id:%d is error!\n", disk_id);
        return -1;
    }

    StorageManager::GetInstance()->Update(MSG_STORAGE_MOUNTED);

    return ret;
}

int DeviceControlImpl::UmountDisk(int disk_id)
{
    int ret = 0;

    db_msg("");

    if (disk_id < 0) {
        db_msg("Input disk_id:%d is error!\n", disk_id);
        return -1;
    }

    StorageManager::GetInstance()->Update(MSG_STORAGE_UMOUNT);

    sleep(3);

    return ret;
}

int DeviceControlImpl::RemoveFile(const char *filename)
{
    return MediaFileManager::GetInstance()->RemoveFile(filename);
}


int DeviceControlImpl::GetSystemVersion(char *buf)
{
    std::string str;
    str = lua_cfg_->GetStringValue("menu.device.sysversion.version");
        //strncpy(device_info_.device_name, str.c_str(), sizeof(device_info_.device_name) - 1);
    strncpy(buf,str.c_str(),strlen(str.c_str()));
    db_error("GetSystemVersion: buf:%s, str:%s lengtth:%d",buf,str.c_str(),strlen(str.c_str()));
    return 0;
}


void DeviceControlImpl::GetDeviceInfo_ex(AWDeviceInfo_ex &dev_info)
{
   
   MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
   dev_info.device_id = 0;
   strcpy(dev_info.device_name,"V536-CDR");
   strcpy(dev_info.hardware,"V1.0");   
   strcpy(dev_info.software,"V1.0");
   dev_info.menu_config_.resoultion_value = menuconfiglua->menu_cfg_.record_resolution.current; //lua_cfg_->GetIntegerValue("menu.record.resolution.current");
   dev_info.menu_config_.loop_record_time = menuconfiglua->menu_cfg_.record_loop.current;//lua_cfg_->GetIntegerValue("menu.record.loop.current");   
   dev_info.menu_config_.recod_type = menuconfiglua->menu_cfg_.record_encodingtype.current;//lua_cfg_->GetIntegerValue("menu.record.encodingtype.current");
   dev_info.menu_config_.video_exp_value = menuconfiglua->menu_cfg_.camera_exposure.current;//lua_cfg_->GetIntegerValue("menu.camera.exposure.current");
   dev_info.menu_config_.parking_value =  menuconfiglua->menu_cfg_.record_parkingmonitor_switch.current;//lua_cfg_->GetIntegerValue("menu.record.parkingmonitor.current");
   dev_info.menu_config_.Gsensor_senstvity_value = menuconfiglua->menu_cfg_.record_emerrecordsen.current;//lua_cfg_->GetIntegerValue("menu.record.emerrecordsen.current");
   dev_info.menu_config_.screen_onoff_value =  menuconfiglua->menu_cfg_.camera_autoscreensaver.current;//lua_cfg_->GetIntegerValue("menu.camera.autoscreensaver.current");
   dev_info.menu_config_.language_value = menuconfiglua->menu_cfg_.device_language.current;//lua_cfg_->GetIntegerValue("menu.device.language.current");
   dev_info.menu_config_.record_sound_value =  menuconfiglua->menu_cfg_.record_sound.current;//lua_cfg_->GetIntegerValue("menu.record.record_sound.current");
   dev_info.menu_config_.record_sound_level_vaule = menuconfiglua->menu_cfg_.record_volume_selection.current;//lua_cfg_->GetIntegerValue("menu.record.volumeselect.current");
   dev_info.menu_config_.moton_detect_value = menuconfiglua->menu_cfg_.switch_record_awmd;//lua_cfg_->GetIntegerValue("menu.switch.record_awmd");
   dev_info.menu_config_.light_value = menuconfiglua->menu_cfg_.camera_lightfreq.current;//lua_cfg_->GetIntegerValue("menu.camera.lightfreq.current");

   //get menu item conunt

    dev_info.menu_config_.resoultion_value_count = menuconfiglua->menu_cfg_.record_resolution.count; //lua_cfg_->GetIntegerValue("menu.record.resolution.current");
    dev_info.menu_config_.loop_record_time_count = menuconfiglua->menu_cfg_.record_loop.count;//lua_cfg_->GetIntegerValue("menu.record.loop.current");   
    dev_info.menu_config_.recod_type_count = menuconfiglua->menu_cfg_.record_encodingtype.count;//lua_cfg_->GetIntegerValue("menu.record.encodingtype.current");
    dev_info.menu_config_.video_exp_value_count = menuconfiglua->menu_cfg_.camera_exposure.count;//lua_cfg_->GetIntegerValue("menu.camera.exposure.current");
    dev_info.menu_config_.parking_value_count =  menuconfiglua->menu_cfg_.record_parkingmonitor_switch.count;//lua_cfg_->GetIntegerValue("menu.record.parkingmonitor.current");
    dev_info.menu_config_.Gsensor_senstvity_value_count = menuconfiglua->menu_cfg_.record_emerrecordsen.count;//lua_cfg_->GetIntegerValue("menu.record.emerrecordsen.current");
    dev_info.menu_config_.screen_onoff_value_count =  menuconfiglua->menu_cfg_.camera_autoscreensaver.count;//lua_cfg_->GetIntegerValue("menu.camera.autoscreensaver.current");
    dev_info.menu_config_.language_value_count = menuconfiglua->menu_cfg_.device_language.count;//lua_cfg_->GetIntegerValue("menu.device.language.current");
    dev_info.menu_config_.record_sound_value_count =  menuconfiglua->menu_cfg_.record_sound.count;//lua_cfg_->GetIntegerValue("menu.record.record_sound.current");
    dev_info.menu_config_.record_sound_level_vaule_count = menuconfiglua->menu_cfg_.record_volume_selection.count;//lua_cfg_->GetIntegerValue("menu.record.volumeselect.current");
    dev_info.menu_config_.moton_detect_value_count = 2;//lua_cfg_->GetIntegerValue("menu.switch.record_awmd");
    dev_info.menu_config_.light_value_count = menuconfiglua->menu_cfg_.camera_lightfreq.count;//lua_cfg_->GetIntegerValue("menu.camera.lightfreq.current");
}


