/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    status_bar.cpp
 * @brief   状态栏窗口presenter
 * @author  id:826
 * @version v0.3
 * @date    2016-12-26
 */

#include "status_bar_bottom.h"

#include "common/app_log.h"
#include "device_model/storage_manager.h"
#include "device_model/system/net/net_manager.h"
#include "uilayer_view/gui/minigui/window/window_manager.h"
#include "uilayer_view/gui/minigui/window/status_bar_bottom_window.h"

#undef LOG_TAG
#define LOG_TAG "StatusBarBottom"

using namespace EyeseeLinux;
using namespace std;

StatusBarBottomPresenter::StatusBarBottomPresenter()
    : win_mg_(WindowManager::GetInstance())
    , sb_win_(NULL)
{
    StorageManager::GetInstance()->Attach(this);
    NetManager::GetInstance()->Attach(this);
}

StatusBarBottomPresenter::~StatusBarBottomPresenter()
{
    NetManager::GetInstance()->Detach(this);
    StorageManager::GetInstance()->Detach(this);
}

int StatusBarBottomPresenter::DeviceModelInit()
{
    return 0;
}

int StatusBarBottomPresenter::DeviceModelDeInit()
{
    return 0;
}

void StatusBarBottomPresenter::OnWindowLoaded()
{
    db_msg("window load");
    this->DeviceModelInit();

    sb_win_ = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));

    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void StatusBarBottomPresenter::OnWindowDetached()
{
    db_msg("window detach");
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = true;
    this->DeviceModelDeInit();
}

int StatusBarBottomPresenter::HandleGUIMessage(int msg, int val,int id)
{
    return 0;
}

void StatusBarBottomPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

void StatusBarBottomPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
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

