/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.cpp
 * @brief 状态栏窗口
 * @author id:690
 * @version v0.3
 * @date 2017-01-17
 */
//#define NDEBUG

#include "window/preview_window.h"
#include "window/status_bar_bottom_window.h"
#include "debug/app_log.h"
#include "common/app_def.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "application.h"
#include "bll_presenter/device_setting.h"
#include "device_model/menu_config_lua.h"
#include "bll_presenter/audioCtrl.h"

#include <sstream>

using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(StatusBarBottomWindow)
/*****************************************************************************
Function: ContainerWidget::HandleMessage
Description: process the messages and notify the children
@override
Parameter:
Return:
 *****************************************************************************/
int StatusBarBottomWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    //db_error("[debug_jaosn]:StatusBarBottomWindow %d",message);
    switch ( message ) {
        case MSG_PAINT:
           //db_warn("habo---> statusbarbottom window  MSG_PAINT !!!");
            return HELP_ME_OUT;
        default:
            return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
    }
}

StatusBarBottomWindow::StatusBarBottomWindow(IComponent *parent)
: SystemWindow(parent)
{
    wname = "StatusBarBottomWindow";
    Load();
    //SetBackColor(0x96000000);  // apha
    //SetBackColor(0xb21A1E38);  // apha b2
    //SetBackColor(0x66000000);  // apha b2
    SetBackColor(0x00000000);  // apha b3
 
   // PreviewWindownButtonStatus(true,true,0);
   // PlaybackWindownButtonStatus(false,false,true);
    PlayingWindownButtonStatus(false,false, false,true);
    initPreviewButtonPos();
    initPlayBackButtonPos();
}

StatusBarBottomWindow::~StatusBarBottomWindow()
{
    db_msg("destruct");
}

void StatusBarBottomWindow::initPreviewButtonPos()
{

    RECT icon_rect;
    char icon_name[128]={0};
    for(int i = 0; i< PrviewButtonPosLen ; i++)
    {
        snprintf(icon_name,sizeof(icon_name)-1,"button%d_icon",i+1);
        GetControl(icon_name)->GetRect(&icon_rect);
        PreviewButtonPos[i].x1 = icon_rect.left;
        PreviewButtonPos[i].y1 = icon_rect.top;
        PreviewButtonPos[i].x2 = icon_rect.right;
        PreviewButtonPos[i].y2 = icon_rect.bottom;
    }
}
void StatusBarBottomWindow::GetPreviewButtonPos(struct buttonPos *bps,int len)
{
    for(int i = 0; i< len ; i++)
    {
        bps[i].x1 = PreviewButtonPos[i].x1;
        bps[i].y1 = PreviewButtonPos[i].y1;
        bps[i].x2 = PreviewButtonPos[i].x2;
        bps[i].y2 = PreviewButtonPos[i].y2;
    }

}

void StatusBarBottomWindow::initPlayBackButtonPos()
{

    RECT icon_rect;
    char icon_name[128]={0};
    for(int i = 0; i< PlayBackButtonPosLen ; i++)
    {
        snprintf(icon_name,sizeof(icon_name)-1,"playback_button%d_icon",i+1);
        GetControl(icon_name)->GetRect(&icon_rect);
        PlayBackButtonPos[i].x1 = icon_rect.left;
        PlayBackButtonPos[i].y1 = icon_rect.top;
        PlayBackButtonPos[i].x2 = icon_rect.right;
        PlayBackButtonPos[i].y2 = icon_rect.bottom;
    }
}
void StatusBarBottomWindow::GetPlayBackButtonPos(struct buttonPos *bps,int len)
{
    for(int i = 0; i< len ; i++)
    {
        bps[i].x1 = PlayBackButtonPos[i].x1;
        bps[i].y1 = PlayBackButtonPos[i].y1;
        bps[i].x2 = PlayBackButtonPos[i].x2;
        bps[i].y2 = PlayBackButtonPos[i].y2;
    }

}

void StatusBarBottomWindow::DoHide()
{
    SetVisible(false);
    WindowManager *wm = WindowManager::GetInstance();
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::SetActiveWindow(cur_win->GetHandle());
    ::EnableWindow(cur_win->GetHandle(), true);
    Widget::Hide();
}

void StatusBarBottomWindow::PreviewWindownButtonStatus(bool on_off, bool reload_all, int flag)
{
	GraphicView* gv;
    if(on_off)
    {
        if(reload_all)
        {
			GraphicView::LoadImage(GetControl("button1_icon"), "preview_record", 	"preview_record_h",		GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button2_icon"), "preview_photo",  	"preview_photo_h",		GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button3_icon"), "preview_lock",   	"preview_lock_h",		GraphicView::NORMAL);
			GraphicView::LoadImage(GetControl("button4_icon"), "preview_playback", 	"preview_playback_h",	GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button5_icon"), "preview_setting", 	"preview_setting_h",	GraphicView::NORMAL);
			GraphicView::LoadImage(GetControl("button6_icon"), "preview_voice",   	"preview_voice_h",		GraphicView::NORMAL);
        }
		if (flag == 0) {
#if 0
			gv = static_cast<GraphicView*>(GetControl("button4_icon"));
			RECT rect;
			gv->GetRect(&rect);
			rect.left = 318;		// see StatusBarBottomWindow.ui
			rect.right= rect.left+106;
			gv->SetPosition(&rect);
#endif
	        GetControl("button1_icon")->Show();
	        GetControl("button2_icon")->Show();
	        GetControl("button3_icon")->Show();
	        GetControl("button4_icon")->Show();
	        GetControl("button5_icon")->Show();
			GetControl("button6_icon")->Show();
		} else {
#if 0
			gv = static_cast<GraphicView*>(GetControl("button4_icon"));
			RECT rect;
			gv->GetRect(&rect);
			rect.left = 265;
			rect.right= rect.left+106;
			gv->SetPosition(&rect);
#endif
			GetControl("button1_icon")->Hide();
	        GetControl("button2_icon")->Hide();
	        GetControl("button3_icon")->Hide();
	        GetControl("button4_icon")->Hide();
	        GetControl("button5_icon")->Hide();
			GetControl("button6_icon")->Hide();
		}
    }
    else
    {
#if 0
		gv = static_cast<GraphicView*>(GetControl("button4_icon"));
		RECT rect;
		gv->GetRect(&rect);
		rect.left = 318;		// see StatusBarBottomWindow.ui
		rect.right= rect.left+106;
		gv->SetPosition(&rect);
#endif
        GetControl("button1_icon")->Hide();
        GetControl("button2_icon")->Hide();
        GetControl("button3_icon")->Hide();
        GetControl("button4_icon")->Hide();
        GetControl("button5_icon")->Hide();
		GetControl("button6_icon")->Hide();
    }

}
void StatusBarBottomWindow::PreviewWindownModeButtonStatus(int flag)
{
	GetControl("button2_icon")->Hide();
	if (!flag) {
		GraphicView::LoadImage(GetControl("button2_icon"), "preview_mode",   	"preview_mode_h",		GraphicView::NORMAL);
	} else {
		GraphicView::LoadImage(GetControl("button2_icon"), "preview_photo", 	"preview_photo_h",		GraphicView::NORMAL);
	}
	GetControl("button2_icon")->Show();
}

void StatusBarBottomWindow::updateRecordIcon(bool record_flag)
{
#if 0
	//int third_icon_width,h;
	if(record_flag){
		GraphicView::LoadImage(GetControl("button1_icon"), "preview_record");
	}else{
		GraphicView::LoadImage(GetControl("button1_icon"), "preview_record","preview_record_h",GraphicView::NORMAL);
	}
	GetControl("button1_icon")->Show();
#endif
}

void StatusBarBottomWindow::updatePhotoIcon(bool photo_flag)
{
#if 0
	if(photo_flag){
		GraphicView::LoadImage(GetControl("button2_icon"), "preview_photo");
	}else{
		GraphicView::LoadImage(GetControl("button2_icon"), "preview_photo","preview_photo_h",GraphicView::NORMAL);
	}
    GetControl("button2_icon")->Show();
#endif
}

void StatusBarBottomWindow::updateLockIcon(bool lock_flag)
{
#if 0
	if(lock_flag){
		GraphicView::LoadImage(GetControl("button3_icon"), "preview_lock");
	}else{
		GraphicView::LoadImage(GetControl("button3_icon"), "preview_lock","preview_lock_h",GraphicView::NORMAL);
	}
    GetControl("button3_icon")->Show();
#endif
}

void StatusBarBottomWindow::PlaybackWindownButtonStatus(bool on_off,bool l_hilight,bool r_hilight)
{
    if(on_off)
    {
#ifndef USEICONTHUMB
        if(l_hilight && !r_hilight)
        {
            GraphicView::LoadImage(GetControl("playback_button1_icon"), "pw_up");
            GraphicView::LoadImage(GetControl("playback_button3_icon"), "pw_down");
        }
        else if(!l_hilight && r_hilight)
        {
            GraphicView::LoadImage(GetControl("playback_button1_icon"), "pw_up");
            GraphicView::LoadImage(GetControl("playback_button3_icon"), "pw_down");
        }
        else if(l_hilight && r_hilight)
        {
            GraphicView::LoadImage(GetControl("playback_button1_icon"), "pw_up");
            GraphicView::LoadImage(GetControl("playback_button3_icon"), "pw_down");
        }
        else if(!l_hilight && !r_hilight)
        {
            GraphicView::LoadImage(GetControl("playback_button1_icon"), "pw_up");
            GraphicView::LoadImage(GetControl("playback_button3_icon"), "pw_down");
        }
#else
		GraphicView::LoadImage(GetControl("playback_button1_icon"), "pw_delete", "pw_delete_h", GraphicView::NORMAL);
        GraphicView::LoadImage(GetControl("playback_button3_icon"), "pw_select", "pw_select_h", GraphicView::NORMAL);
#endif
        GetControl("playback_button1_icon")->Hide();
        GetControl("playback_button2_icon")->Hide();
        GraphicView::LoadImage(GetControl("playback_button2_icon"), "pw_lock", "pw_lock_y", GraphicView::NORMAL);
        GetControl("playback_button3_icon")->Hide();
		db_error("3 icon show");
    }
    else
    {
        GetControl("playback_button1_icon")->Hide();
        GetControl("playback_button2_icon")->Hide();
        GetControl("playback_button3_icon")->Hide();
		db_error("3 icon hide");
    }


}

void StatusBarBottomWindow::PlayingWindownButtonStatus(bool on_off,bool p_pause, bool reload_all,bool jpeg_play_flag)
{
    if(on_off)
    {
        if(reload_all)
        {
		#ifndef USEICONTHUMB
	        GraphicView::LoadImage(GetControl("button1_icon"), "return_y");
            GraphicView::LoadImage(GetControl("button2_icon"), "playing_left");
            GraphicView::LoadImage(GetControl("button3_icon"), "playing_right");
            GraphicView::LoadImage(GetControl("button4_icon"), "delete_y");
            GraphicView::LoadImage(GetControl("button5_icon"), "play");
		#else	
            GraphicView::LoadImage(GetControl("button1_icon"), "return_y",		"return_y_h", 		GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button2_icon"), "playing_left", 	"playing_left_h", 	GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button3_icon"), "playing_right",	"playing_right_h", 	GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button4_icon"), "delete_y", 		"delete_y_h", 		GraphicView::NORMAL);
            GraphicView::LoadImage(GetControl("button5_icon"), "play", 			"play_h", 			GraphicView::NORMAL);
		#endif
        }
        else
        {
            if(p_pause)
			#ifndef USEICONTHUMB
				GraphicView::LoadImage(GetControl("button5_icon"), "pause");
			#else
                GraphicView::LoadImage(GetControl("button5_icon"), "pause", "pause_h",  GraphicView::NORMAL);
			#endif
            else
			#ifndef USEICONTHUMB
				GraphicView::LoadImage(GetControl("button5_icon"), "play");
			#else
                GraphicView::LoadImage(GetControl("button5_icon"), "play",	"play_h", GraphicView::NORMAL);
			#endif
        }
        GetControl("button1_icon")->Show();
        GetControl("button2_icon")->Show();
        GetControl("button3_icon")->Show();
        GetControl("button4_icon")->Show();
        if(!jpeg_play_flag) {
			GetControl("button5_icon")->Hide();
            GetControl("button5_icon")->Show();
        }
        else
            GetControl("button5_icon")->Hide();
		db_error("5 icon show x");
    }
    else
    {
        GetControl("button1_icon")->Hide();
        GetControl("button2_icon")->Hide();
        GetControl("button3_icon")->Hide();
        GetControl("button4_icon")->Hide();
        GetControl("button5_icon")->Hide();
		db_error("5 icon hide");
    }


}


void StatusBarBottomWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    // params.exstyle = WS_EX_NONE ;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string StatusBarBottomWindow::GetResourceName()
{
    return string(GetClassName());
}

void StatusBarBottomWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{

    ctrl->SetCtrlTransparentStyle(true);
}

void StatusBarBottomWindow::Update(MSG_TYPE msg,int p_CamID, int p_recordId)
{
    string zoom_str;
    db_msg("handle msg:%d", msg);
    switch ((int)msg) {		
        case MSG_CHANG_STATU_PREVIEW:
           PreviewWindownButtonStatus(false,false,0);
            break;
        case MSG_SET_STATUS_PREVIEW_REC_PLAY:
			updateRecordIcon(false);
            break;
        case MSG_SET_STATUS_PREVIEW_REC_PAUSE:
			updateRecordIcon(true);
			updateLockIcon(true);
            break;
        case MSG_SET_SHOW_RIGHT_LEFT_BUTTON:
            break;
        case MSG_SET_SHOW_UP_DWON_BUTTON:
            break;
        case MSG_SET_HIDE_UP_DWON_BUTTON:
            break;
        case MSG_SET_HIDE_UP_DWON_CHOICE_BUTTON:
            break;
        case MSG_SET_SHOW_UP_DWON_CHOICE_BUTTON:
            break;
        case MSG_CHANG_STATU_PLAYBACK:
            PreviewWindownButtonStatus(false,false,0);
            //PlaybackWindownButtonStatus(true,false,true);
            break;
        case MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR_BOTTOM:
            break;
        case MSG_VIDEO_PLAY_STOP:
        case MSG_VIDEO_PLAY_COMPLETION:
			db_error("MSG_VIDEO_PLAY_STOP");
            PlayingWindownButtonStatus(true,true, false,false);//show the pause icon ,other icon not need to reload
            break;
        case MSG_PIC_PLAY_START:
			db_error("MSG_PIC_PLAY_START");
            //PlaybackWindownButtonStatus(false,false,true);//hide the playback 3 buttong icon
            PlayingWindownButtonStatus(true,true, true,true);//show the first time to playing window buttong icon
            break;
        case MSG_VIDEO_PLAY_START:
			db_error("MSG_VIDEO_PLAY_START");
            //PlaybackWindownButtonStatus(false,false,true);//hide the playback 3 buttong icon
            PlayingWindownButtonStatus(true,true, true,false);//show the first time to playing window buttong icon
            break;
        case MSG_VIDEO_PLAY_PAUSE:
			db_error("MSG_VIDEO_PLAY_PAUSE");
            PlayingWindownButtonStatus(true,true, false,false);//show the pause icon ,other icon not need to reload
            break;
		#if 1
        case MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
		case MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
        	{
			if(!this->GetVisible())
                this->Hide();
			db_error("MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM p_CamID %d",p_CamID);
			WindowManager *win_mg = ::WindowManager::GetInstance();
			PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
			int win_statu_save = pre_win->Get_win_statu_save();
			
           // PlaybackWindownButtonStatus(false,false,true);//hide the playback 3 buttong icon            
            PreviewWindownButtonStatus(false,false,win_statu_save);
			pre_win->Set_win_statu(win_statu_save);
        	}
            break;
		#endif
		#if 0
		case MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
            if(!this->GetVisible())
                this->Show();
			//db_error("MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM");
            PlaybackWindownButtonStatus(false,false,true);//hide the playback 3 buttong icon
            PreviewWindownButtonStatus(true,true,0);		// 0= 6button
            break;
		#endif
        case MSG_PLAYBACK_DELETE_PLAY_BUTTON:
            break;
        case MSG_PLAYBACK_NO_DETECT_FILE_SDCARD:
            if(this->GetVisible())
                this->Hide();
            PreviewWindownButtonStatus(false,false,0);
            //PlaybackWindownButtonStatus(false,false,false);
            break;
        case MSG_PLAYBACK_DELETE_PLAY_IMAG_SH:

            break;
       case MSG_PLAYBACK_PAGE_UP_DOWN_ICON:
       {
#if 0
            if(p_CamID == LEFT_RIGHT_SHOW)
            {
                PlaybackWindownButtonStatus(true,true,true);
            }
            else if(p_CamID == LEFT_SHOW_ONLY)
            {
                PlaybackWindownButtonStatus(true,true,false);
            }
            else if(p_CamID == RIGHT_SHOW_ONLY)
            {
                PlaybackWindownButtonStatus(true,false,true);
            }
            else if(p_CamID == LEFT_RIGHT_HIDE)
            {
                PlaybackWindownButtonStatus(true,false,false);
            }
#endif
            if(!this->GetVisible())
                this->Hide();
           // PlaybackWindownButtonStatus(true,true,true);
       }break;
        case MSG_PLAYBACK_SHOW_GRAY_ICON:

        	break;
        case MSG_PLAYBACK_SHOW_HIGHLIGHT_ICON:

        	break;
        default:
            break;
    }
}

void StatusBarBottomWindow::hideStatusBarBottomWindow()
{
    if(this->GetVisible())
        this->Hide();
    //PreviewWindownButtonStatus(false,false);
    //PlaybackWindownButtonStatus(false,false,false);
}

void StatusBarBottomWindow::showStatusBarBottomWindow()
{
	if(!this->GetVisible())
		this->Show();
	//PlaybackWindownButtonStatus(false,false,true);//hide the playback 3 buttong icon
	//PreviewWindownButtonStatus(true,true);
}

