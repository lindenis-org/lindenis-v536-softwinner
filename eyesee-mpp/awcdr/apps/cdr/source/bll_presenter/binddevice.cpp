/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file binddevice.h
 * @brief binding
 * @author id:007
 * @version v0.3
 * @date 2018-05-12
 */
#include "bll_presenter/binddevice.h"
#include "device_model/system/event_manager.h"
#include "lua/lua_config_parser.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "window/bind_window.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "application.h"
#include "utils/utils.h"
#include <sstream>
#include "uilayer_view/gui/minigui/window/status_bar_window.h"
#include "uilayer_view/gui/minigui/window/status_bar_bottom_window.h"


#undef LOG_TAG
#define LOG_TAG "BindPresenter"

using namespace EyeseeLinux;
using namespace std;

BindPresenter::BindPresenter()
{
	//EventManager::GetInstance()->Attach(this);
}

BindPresenter::~BindPresenter()
{
	//EventManager::GetInstance()->Detach(this); 
}


void BindPresenter::OnWindowLoaded()
{
    db_msg("window load");

    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }

    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void BindPresenter::OnWindowDetached()
{
    db_msg("window detach");
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = true;
    if (status_ == MODEL_INITED) {
        this->DeviceModelDeInit();
    }
}

int BindPresenter::HandleGUIMessage(int msg, int val)
{
    int ret = 0;
    switch(msg) {
        default:
            db_msg("there is no this button defined");
            break;
    }

    return ret;
}

void BindPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

int BindPresenter::DeviceModelInit()
{
    db_msg("rec and play device model init");
	EventManager::GetInstance()->Attach(this);
	status_ = MODEL_INITED;

    return 0;
}

int BindPresenter::DeviceModelDeInit()
{
    status_ = MODEL_UNINIT;
	EventManager::GetInstance()->Detach(this); //
    return 0;
}

// 底层通知回调
void BindPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
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

    switch (msg) {
		case MSG_BIND_SUCCESS:
			 Notify(msg);
            break;
        default:
            break;
    }

}
