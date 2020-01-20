/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file record_control_impl.h
 * @brief 录像控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-11
 */
#pragma once

#include "interface/dev_ctrl_adapter.h"
#include "bll_presenter/common_type.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"
#include "window/window_manager.h"


#include <mutex>
#include <thread>

namespace EyeseeLinux
{

/**
 * @brief Record控制接口
 */
class RecordControlImpl
    : public DeviceAdapter::RecordControl
{
    public:
        RecordControlImpl(IPresenter *presenter);

        ~RecordControlImpl();

        int SetRecordAttr(int chn, const AWRecordAttr &record_attr);

        int GetRecordAttr(int chn, AWRecordAttr &record_attr);

        int SetSnapAttr(int chn, const AWSnapAttr &snap_attr);

        int GetSnapAttr(int chn, AWSnapAttr &snap_attr);

        int DoSnaptask(int chn);

        int SaveRecordConfig(void);

        int DefaultRecordConfig(void);

        int RemoteSwitchRecord(void);
        int RemoteSwitchRecord(int value);

        int RemoteTakePhoto(void);
        int RemoteSwitchSlowRecord(void);
        int GetCurretRecordTime(void);
        int GetRecordStatus(void);

    private:
        bool param_change_flag_;
        LuaConfig *lua_cfg_;
        AWRecordAttr record_attr_[AW_MAX_CHN_NUM];
        AWSnapAttr   snap_attr_[AW_MAX_CHN_NUM];
        IPresenter *prensenter_;
        
        std::map<WindowID, IGUIPresenter*> win_presenter_map_;        
        WindowManager *wm;
        int LoadRecordConfig(void);
        std::mutex mutex_;
};

} // namespace EyeseeLinux
