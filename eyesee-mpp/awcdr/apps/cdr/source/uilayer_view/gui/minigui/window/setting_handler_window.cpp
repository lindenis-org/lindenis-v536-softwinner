/* *******************************************************************************
 * Copyright (C), 2017-2027, Sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file setting_handler_window.cpp
 * @brief 设置控制窗口
 * @author id:fangjj
 * @version v1.0
 * @date 2017-04-21
 */
#include "window/setting_handler_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "device_model/system/power_manager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "SettingHandlerWindow"

using namespace std;
using namespace EyeseeLinux;


IMPLEMENT_DYNCRT_CLASS(SettingHandlerWindow)


void SettingHandlerWindow::keyProc(int keyCode,int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_LEFT:
            break;
        case SDV_KEY_POWER:
    		listener_->sendmsg(this, MSG_SET_MENU_CONFIG_LUA, 0);	
    		listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
            break;
        case SDV_KEY_OK:
    	    listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_SETTING);
            break;
        case SDV_KEY_RIGHT:
            break;
        default:
            db_msg("[fangjj]:invild keycode");
            break;
    }

}

void SettingHandlerWindow::WindowSwitchonClickProc(View *control)
{
	listener_->notify(this, MSG_SET_MENU_CONFIG_LUA, 0);	
	listener_->notify(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
}


/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int SettingHandlerWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
        case MSG_PAINT:
            return HELP_ME_OUT;
		case MSG_TIMER:
            break;
        default:
            break;
    }
    return SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
}

SettingHandlerWindow::SettingHandlerWindow(IComponent *parent)
    : SystemWindow(parent)
    , _Parent(parent)
{
    wname = "SettingHandlerWindow";
    Load();
    listener_ = WindowManager::GetInstance();

      string bkgnd_bmp = R::get()->GetImagePath("black_bg");
      db_msg("[fangjj]:----set back ground pic: %s", bkgnd_bmp.c_str());
      SetWindowBackImage(bkgnd_bmp.c_str());

      GraphicView::LoadImage(GetControl("setting_handler_icon"), "setting_handler_window");
      view_setting_handler = reinterpret_cast<GraphicView *>(GetControl("setting_handler_icon"));

     /*
      GraphicView::LoadImage(GetControl("window_switch_icon"), "window_switch");
      view_window_switch = reinterpret_cast<GraphicView *>(GetControl("window_switch_icon"));
      view_window_switch->OnClick.bind(this, &SettingHandlerWindow::WindowSwitchonClickProc);
      */

      string title_str;
      R::get()->GetString("tl_setting", title_str);
      TextView *setting_handler_text = static_cast<TextView*>(GetControl("setting_handler_label")); 
      setting_handler_text->SetCaption(title_str.c_str());
      setting_handler_text->SetCaptionColor(0XFFFFFFFF);	  

}


SettingHandlerWindow::~SettingHandlerWindow()
{

}

void SettingHandlerWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string SettingHandlerWindow::GetResourceName()
{
    return string(GetClassName());
}

void SettingHandlerWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}

int SettingHandlerWindow::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;
}

void SettingHandlerWindow::UpdateLabel()
{
      string title_str;
      R::get()->GetString("tl_setting", title_str);
      TextView *setting_handler_text = static_cast<TextView*>(GetControl("setting_handler_label")); 
      setting_handler_text->SetCaption(title_str.c_str());
      db_msg("[fangjj]:----UpdateLabel tl_setting: %s", title_str.c_str());
}

