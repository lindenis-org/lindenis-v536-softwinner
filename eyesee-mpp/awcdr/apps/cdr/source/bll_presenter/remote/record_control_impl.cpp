/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file record_control_impl.cpp
 * @brief 录像控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-11
 */

#include "interface/dev_ctrl_adapter.h"
#include "record_control_impl.h"
#include "common/app_log.h"
#include "window/preview_window.h"


#include <string.h>
#include <string>
#include <iostream>

#undef LOG_TAG
#define LOG_TAG "RecordControlImpl"

#define RECORD_CONFIG_FILE "/tmp/data/record_config.lua"

using namespace EyeseeLinux;
using namespace std;

RecordControlImpl::RecordControlImpl(IPresenter *presenter)
{
    param_change_flag_ = false;
    lua_cfg_           = new LuaConfig();
    prensenter_ = presenter;
    int ret = LoadRecordConfig();
    if (ret) {
        // TODO: default config file.
    }
}

RecordControlImpl::~RecordControlImpl()
{
    param_change_flag_ = false;

    if (lua_cfg_) {
        delete lua_cfg_;
    }
}

int RecordControlImpl::DefaultRecordConfig()
{
    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    return 0;
}

int RecordControlImpl::SaveRecordConfig()
{
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (false == param_change_flag_) {
        db_msg("Save param is the same.!\n");
        return 0;
    }

    /* save config */
    for (cnt = 0; cnt < AW_MAX_CHN_NUM; cnt++) {
//        memset(tmp_str, 0, sizeof(tmp_str));
//        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].channel", cnt + 1);
//        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].channel);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].record_type", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].record_type);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].stream_type", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].stream_type);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].audio_enable", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].audio_enable);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].cover_enable", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].cover_enable);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].pack_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].pack_time);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].perrecord_time", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, record_attr_[cnt].perrecord_time);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].channel", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].channel);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].quality", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].quality);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].pic_wide", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].pic_wide);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].pic_high", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].pic_high);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].snap_delay", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].snap_delay);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].snap_num", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].snap_num);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].interval", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, snap_attr_[cnt].interval);
    }

    /*
    ret = lua_cfg_->SyncToFile(RECORD_CONFIG_FILE);
    if (ret) {
        db_msg("Do SyncToFile fail! file:%s \n", OVERLAY_CONFIG_FILE);
        return -1;
    }
    */

    return 0;
}

int RecordControlImpl::LoadRecordConfig()
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(RECORD_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", RECORD_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/record_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(RECORD_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", RECORD_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/record_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(RECORD_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!\n", RECORD_CONFIG_FILE);
            return -1;
        }
    }

    for (cnt = 0; cnt < AW_MAX_CHN_NUM; cnt++) {
//        memset(tmp_str, 0, sizeof(tmp_str));
//        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].channel", cnt + 1);
//        record_attr_[cnt].channel = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].record_type", cnt + 1);
        record_attr_[cnt].record_type = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].stream_type", cnt + 1);
        record_attr_[cnt].stream_type = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].audio_enable", cnt + 1);
        record_attr_[cnt].audio_enable = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].cover_enable", cnt + 1);
        record_attr_[cnt].cover_enable = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].pack_time", cnt + 1);
        record_attr_[cnt].pack_time = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "record.camera[%d].perrecord_time", cnt + 1);
        record_attr_[cnt].perrecord_time = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].channel", cnt + 1);
        snap_attr_[cnt].channel = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].quality", cnt + 1);
        snap_attr_[cnt].quality = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].pic_wide", cnt + 1);
        snap_attr_[cnt].pic_wide = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].pic_high", cnt + 1);
        snap_attr_[cnt].pic_high = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].snap_delay", cnt + 1);
        snap_attr_[cnt].snap_delay = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].snap_num", cnt + 1);
        snap_attr_[cnt].snap_num = lua_cfg_->GetIntegerValue(tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "snap.camera[%d].interval", cnt + 1);
        snap_attr_[cnt].interval = lua_cfg_->GetIntegerValue(tmp_str);
    }

    return 0;
}

int RecordControlImpl::SetRecordAttr(int chn, const AWRecordAttr &record_attr)
{
    int ret = 0;

    if (chn < 0 || chn > AW_MAX_CHN_NUM) {
        db_msg("Input chn:%d error!\n", chn);
        return -1;
    }

    ret = memcmp(&record_attr_[chn], &record_attr, sizeof(AWRecordAttr));
    if (0 == ret) {
        db_msg("Input chn:%d record_attr is the same.\n", chn);
        return 0;
    }

    // TODO: Setting Record config

    memcpy(&record_attr_[chn], &record_attr, sizeof(AWRecordAttr));
    param_change_flag_ = true;
    return 0;
}

int RecordControlImpl::GetRecordAttr(int chn, AWRecordAttr &record_attr)
{
    int ret = 0;

    if (chn < 0 || chn > AW_MAX_CHN_NUM) {
        db_msg("Input chn:%d error!\n", chn);
        return -1;
    }

    memcpy(&record_attr, &record_attr_[chn], sizeof(AWRecordAttr));
    return 0;
}

int RecordControlImpl::SetSnapAttr(int chn, const AWSnapAttr &snap_attr)
{
    int ret = 0;

    if (chn < 0 || chn > AW_MAX_CHN_NUM) {
        db_msg("Input chn:%d error!\n", chn);
        return -1;
    }

    ret = memcmp(&snap_attr_[chn], &snap_attr, sizeof(AWSnapAttr));
    if (0 == ret) {
        db_msg("Input chn:%d record_attr is the same.\n", chn);
        return 0;
    }

    // TODO: Setting snap config

    memcpy(&snap_attr_[chn], &snap_attr, sizeof(AWSnapAttr));
    param_change_flag_ = true;
    return 0;
}

int RecordControlImpl::GetSnapAttr(int chn, AWSnapAttr &snap_attr)
{
    int ret = 0;

    if (chn < 0 || chn > AW_MAX_CHN_NUM) {
        db_msg("Input chn:%d error!\n", chn);
        return -1;
    }

    memcpy(&snap_attr, &snap_attr_[chn], sizeof(AWSnapAttr));
    return 0;
}

int RecordControlImpl::RemoteSwitchRecord()
{
    int ret = 0;
    ret = prensenter_->RemoteSwitchRecord();
    if(ret < 0){
        db_warn("start remote record failed");
        return -1;
    }
    return 0;
}

int RecordControlImpl::RemoteSwitchRecord(int value)
{   
    int ret = 0;
    ret = prensenter_->RemoteSwitchRecord(value);
    if(ret < 0){
        db_warn("start remote record failed");
        return -1;
    }
    return 0;
}

int RecordControlImpl::RemoteSwitchSlowRecord()
{
    int ret = 0;
    ret = prensenter_->RemoteSwitchSlowRecord();
    if(ret < 0){
        db_warn("start RemoteSwitchSlowRecord failed");
        return -1;
    }
    return 0;
}


int RecordControlImpl::RemoteTakePhoto()
{
    int ret = 0;

    std::lock_guard<std::mutex> lock(mutex_);

    std::thread([&] {
        prctl(PR_SET_NAME, "RemoteTakePhoto", 0, 0, 0);
        ret = prensenter_->RemoteTakePhoto();
        if(ret < 0){
            db_warn("start RemoteTakePhoto failed");
        }
    }).detach();
    return ret;

}

int RecordControlImpl::GetCurretRecordTime()
{
    db_error("GetCurretRecordTime 11\n");
    int ret = 0;
    Window *win = wm->GetWindow(WINDOWID_PREVIEW);
    ret = reinterpret_cast<PreviewWindow*>(win)->GetCurrentRecordTime();    
    db_error("GetCurretRecordTime %d\n",ret);
    return ret;
}

int RecordControlImpl::GetRecordStatus()
{
    int ret = 0;
    Window *win = wm->GetWindow(WINDOWID_PREVIEW);
    ret = reinterpret_cast<PreviewWindow*>(win)->GetRecordStatus();
    db_error("GetRecordStatus %d\n",ret);
    return ret;
}


int RecordControlImpl::DoSnaptask(int chn)
{
    return 0;
}
