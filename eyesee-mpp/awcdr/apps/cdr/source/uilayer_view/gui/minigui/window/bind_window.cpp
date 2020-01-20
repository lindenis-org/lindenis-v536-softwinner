/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file bind_window.cpp
 * @brief 绑定界面
 * @author id:007
 * @version v0.1
 * @date 2018-05-12
 */
#include "window/bind_window.h"
#include "window/dialog.h"
#include "debug/app_log.h"
#include "widgets/graphic_view.h"
#include "resource/resource_manager.h"
//#include "widgets/buttonOK.h"
#include "widgets/text_view.h"
#include "widgets/switch_button.h"
#include "widgets/progress_bar.h"
#include "window/window_manager.h"
#include "window/preview_window.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "application.h"
#include "device_model/system/power_manager.h"
#include "device_model/partitionManager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"
#include <thread>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "BindWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(BindWindow)


void BindWindow::keyProc(int keyCode, int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_LEFT:
            break;
        case SDV_KEY_POWER:
            break;
        case SDV_KEY_OK:
		{
			DoHide();
			PreviewWindow *pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));//modify by zhb 2018.09.12
			pre_win->DetectSdcardNewVersion();
   			listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);//WINDOWID_SETTING
			#if 0
			PartitionManager::GetInstance()->sunxi_spinor_private_sec_set(FKEY_BINDFLAG, "true");
			PartitionManager::GetInstance()->sunxi_spinor_private_set_flag(1);
			#endif
        	}
            break;
        case SDV_KEY_RIGHT:
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }  
}

int BindWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    //db_msg("[debug_jaosn]:BindWindow message = %d,wparam = %u,lparam = %lu",message,wparam,lparam);
	switch(message)
	{
		case MSG_PAINT:
			{
				pthread_mutex_lock(&paint_lock);
				HDC hdc = BeginPaint (hwnd);
				if(access("/data/BindImage.png", R_OK) == 0) 
				{					 				 					
					if(qrbmp)						
						UnloadBitmap(qrbmp);					
					else						
						qrbmp = (BITMAP*)malloc(sizeof(BITMAP));			
					LoadBitmap (hdc, qrbmp, "/data/BindImage.png");					
					if (qrbmp != NULL)						
						FillBoxWithBitmap (hdc, 64, 149, 243, 243, qrbmp);					  			
				}
				EndPaint(hwnd,hdc);	
				pthread_mutex_unlock(&paint_lock);
			}
			return DO_IT_MYSELF;
		defulat:
			break;
	}
    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}

BindWindow::BindWindow(IComponent *parent)
    : SystemWindow(parent)
    ,qrbmp(NULL)
{
    wname = "BindWindow";
    Load();
	db_msg("");
       SetWindowBackImage(R::get()->GetImagePath("com_bg").c_str());
#if 0
   	bind_image = reinterpret_cast<GraphicView *>(GetControl("BindImage"));
	if(access("/data/BindImage.png", R_OK) == 0) {
		bind_image->SetPlayBackImage("/data/BindImage.png");
	}else{
		db_msg("by hero *** can not read BindImage.png");
		bind_image->SetPlayBackImage(R::get()->GetImagePath("BindImage").c_str());
	}	
#endif
    GraphicView::LoadImage(GetControl("yi_image"), "yi");
    GetControl("yi_image")->Show();
    string str;
    R::get()->GetString("ml_bindwindow_title", str);
    TextView* bw_title_label = reinterpret_cast<TextView *>(GetControl("title_label"));
    bw_title_label->SetTextStyle(DT_LEFT);
    bw_title_label->SetCaption(str.c_str());
    bw_title_label->SetCaptionColor(0xFF2772db);
    bw_title_label->Show();

    R::get()->GetString("ml_bindwindow_text", str);
    TextView* bw_text_label = reinterpret_cast<TextView *>(GetControl("text_label"));
    bw_text_label->SetTextStyle(DT_LEFT|DT_VCENTER|DT_WORDBREAK);
    bw_text_label->SetCaption(str.c_str());
    bw_text_label->SetCaptionColor(0xFFFFFFFF);
    bw_text_label->Show();

	pthread_mutex_init(&paint_lock, NULL);
    create_timer(this, &b_hide_timer_id,HideBindWindowTimerProc);
    stop_timer(b_hide_timer_id);
}

BindWindow::~BindWindow()
{
	delete_timer(b_hide_timer_id);
	pthread_mutex_destroy(&paint_lock);
	if(qrbmp != NULL)
		UnloadBitmap(qrbmp);
}

void BindWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string BindWindow::GetResourceName()
{
    return string(GetClassName());
}

void BindWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
		case MSG_BIND_SUCCESS:{
			db_msg("[debug_jason]: mssage arrive BindWindow\n");
			set_one_shot_timer(1,0,b_hide_timer_id);
			}
			break;
        default:
            break;
    }
}

void BindWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
   if (ctrl_name == "text_label" ||
   	ctrl_name == "title_label") {
        ctrl->SetCtrlTransparentStyle(true);
    }else{
	ctrl->SetCtrlTransparentStyle(true);
    	}
}

void BindWindow::DoShow()
{
    Window::DoShow();
}

void BindWindow::DoHide()
{
    SetVisible(false);
}
void BindWindow::HideBindWindowTimerProc(union sigval sigval)
{
    BindWindow *bw = reinterpret_cast<BindWindow *>(sigval.sival_ptr);
    bw->DoHideBindWindow();
}

void BindWindow::DoHideBindWindow()
{		
	PreviewWindow *pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));//modify by zhb 2018.04.17
	if(pre_win->DetectSdcardNewVersion() < 0)
		pre_win->keyProc(SDV_KEY_OK, SHORT_PRESS);
   	listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);//WINDOWID_SETTING
}
