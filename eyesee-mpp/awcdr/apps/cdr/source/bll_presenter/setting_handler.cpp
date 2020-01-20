/* *******************************************************************************
 * Copyright (c), 2017-2027, Sunchip Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    setting_handler.cpp
 * @brief   设置控制窗口presenter
 * @author  id:fangjj
 * @version v1.0
 * @date    2017-04-21
 */

#include "bll_presenter/setting_handler.h"
#include "common/app_log.h"
#include "device_model/storage_manager.h"
#include "device_model/system/net/net_manager.h"
#include "uilayer_view/gui/minigui/window/window_manager.h"
#include "window/user_msg.h"
#include "uilayer_view/gui/minigui/window/status_bar_window.h"
#include "uilayer_view/gui/minigui/window/status_bar_bottom_window.h"


#undef LOG_TAG
#define LOG_TAG "SettingHandler"

using namespace EyeseeLinux;
using namespace std;

SettingHandlerPresenter::SettingHandlerPresenter()
    : win_mg_(WindowManager::GetInstance())
    , shw_win_(NULL)
{
    //StorageManager::GetInstance()->Attach(this);
    StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    this->Attach(status_bar);
    StatusBarBottomWindow *status_bottom_bar = static_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Attach(status_bottom_bar);
}

SettingHandlerPresenter::~SettingHandlerPresenter()
{
    //StorageManager::GetInstance()->Detach(this);
    StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    this->Detach(status_bar);
    StatusBarBottomWindow *status_bottom_bar = static_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Detach(status_bottom_bar);
}

int SettingHandlerPresenter::DeviceModelInit()
{
    NetManager::GetInstance()->Attach(this);
    return 0;
}

int SettingHandlerPresenter::DeviceModelDeInit()
{
    // NetManager::GetInstance()->Detach(this);
    return 0;
}

void SettingHandlerPresenter::OnWindowLoaded()
{
    this->DeviceModelInit();
    shw_win_ = reinterpret_cast<SettingHandlerWindow *>(win_mg_->GetWindow(WINDOWID_SETTING_HANDLER));
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void SettingHandlerPresenter::OnWindowDetached()
{
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = true;
    this->DeviceModelDeInit();
}

int SettingHandlerPresenter::HandleGUIMessage(int msg, int val)
{
    db_msg("msg[%d], val[%d]", msg, val);
    //SettingHandlerWindow*win = static_cast<SettingHandlerWindow*>(win_mg_->GetWindow(WINDOWID_SETTING_HANDLER));
    switch(msg) {
	    case MSG_SET_MENU_CONFIG_LUA:			
		 this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
	    default:
		db_msg("unhandled message: msg[%d], val[%d]", msg, val);
		break;
	}
    return 0;
}

void SettingHandlerPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

void SettingHandlerPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    if (msg_mutex_.try_lock() == false) {
        db_warn("maybe presenter is detaching, ignore this msg");
        return;
    }
    if (ignore_msg_) {
        db_warn("presenter has been detached, do not response msg");
        msg_mutex_.unlock();
        return;
    }
    msg_mutex_.unlock();

  //  db_msg("msg: %d", msg);
    this->Notify(msg);
}

