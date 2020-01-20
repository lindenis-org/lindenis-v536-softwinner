/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    sublist.cpp
 * @brief   子列表（三级列表）
 * @author  sh
 * @version v1.0
 * @date    2018-01-02
 */
#include "info_dialog.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "widgets/list_view.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "InfoDialog"

using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(InfoDialog)

void InfoDialog::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode)
	{
	    case SDV_KEY_MENU:
	    {
	        if(isLongPress == LONG_PRESS){
	        } else {
                this->DoHide();
                
	        }
	    }
	    break;
        default:
            db_msg("this is invild key code");
            break;
    }
}


int InfoDialog::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
       case MSG_LBUTTONDOWN:
            if(m_show)
                this->keyProc(SDV_KEY_MENU, SHORT_PRESS);
            break;
        default:
            break;
    }

    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

InfoDialog::InfoDialog(IComponent *parent)
    : SystemWindow(parent)
    ,m_info_title(NULL)
    ,m_info_text(NULL)
    ,m_show(false)
{
    Load();
    listener_ = WindowManager::GetInstance();
    SetBackColor(0xFFA49F9F);
    m_info_title = reinterpret_cast<TextView *>(GetControl("_info_title"));
    m_info_title->SetCaptionColor(0xFFFFFFFF);
    m_info_title->SetCaption("");
    m_info_title->SetBackColor(0xFFA49F9F);

    

    m_info_text = reinterpret_cast<TextView *>(GetControl("_info_text"));
    m_info_text->SetCaptionColor(0xFFFFFFFF);
    m_info_text->SetCaption("");
    m_info_text->SetBackColor(0xFFA49F9F);
}

InfoDialog::~InfoDialog()
{

}


void InfoDialog::SetInfoTitle(std::string &title_str)
{

    m_info_title->SetCaption(title_str.c_str());
}

void InfoDialog::SetInfoText(std::string &text_str)
{

    m_info_text->SetCaption(text_str.c_str());
}
void InfoDialog::PreInitCtrl(View *ctrl, std::string &ctrl_name)
{
    if (ctrl_name == "_info_title") 
    {
        ctrl->SetCtrlTransparentStyle(false);
        TextView* info_label = reinterpret_cast<TextView *>(ctrl);
        info_label->SetTextStyle(DT_CENTER);
    }
    else if (ctrl_name == "_info_text")
    {
        ctrl->SetCtrlTransparentStyle(false);
        TextView* info_label = reinterpret_cast<TextView *>(ctrl);
		#ifdef INFO_ZD55
		info_label->SetTextStyle(DT_CENTER);
		#else
        info_label->SetTextStyle(DT_LEFT);
		#endif
    }
        
}


void InfoDialog::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE|WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

std::string InfoDialog::GetResourceName()
{
    return std::string(GetClassName());
}

void InfoDialog::DoShow()
{
    m_show = true;
    Window::DoShow();
     ::EnableWindow(parent_->GetHandle(), false);
}

void InfoDialog::DoHide()
{
    m_show = false;
    SetVisible(false);
     ::EnableWindow(parent_->GetHandle(), true);
    ::SetActiveWindow(parent_->GetHandle());
    //if (parent_->GetVisible()) {
     //   ::SetActiveWindow(parent_->GetHandle());
   // }
}


