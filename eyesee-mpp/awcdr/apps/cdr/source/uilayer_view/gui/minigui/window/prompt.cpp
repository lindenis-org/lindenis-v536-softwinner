/* *******************************************************************************
 * Copyright (C), 2017-2027, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file prompt.cpp
 * @brief 提示窗口
 * @author id:fangjj
 * @version v10.
 * @date 2017-04-24
 */
#include "window/prompt.h"
#include "window/preview_window.h"
#include "window/playback_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/screensaver.h"
#include "device_model/system/power_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "Prompt"

using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(Prompt)

int Prompt::HandleMessage(HWND hwnd, int message, WPARAM wparam,
    LPARAM lparam)
{
    switch( message )
    {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_TIMER:
            break;
        default:
            break;
    }

    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}


void Prompt::keyProc(int keyCode,int isLongPress)
{
	db_msg("debug_zhb------>  prompt keyProc keyCode: %d",keyCode);
    return;
}

Prompt::Prompt(IComponent *parent)
        : SystemWindow(parent)
        , showtimes_(0)
        ,timercount(0)
        , m_bIsShowing(false)
        , m_prompt_standby_flag_(false)
        ,prompt_id_(PROMPT_INVALID)
{
    wname = "Prompt";
    Load();
    listener_ = WindowManager::GetInstance();
    //SetBackColor(0xE5010A2D);
    string bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
    SetWindowBackImage(bkgnd_bmp.c_str());

    m_Title = static_cast<TextView*>(GetControl("prompt_title_label"));
    m_Title->SetCaption("");
    m_Title->SetCaptionColor(0xFFFFFFFF);//rgb-->bgr
    m_Title->SetBackColor(0xE500071F);//0xE51C2443
    m_Text = reinterpret_cast<TextView*>(GetControl("prompt_info_label"));
    m_Text->SetCaption("");
    m_Text->SetCaptionColor(0xFFFFFFFF);
    m_Text->SetBackColor(0xE500071F);

    m_Time = reinterpret_cast<TextView*>(GetControl("prompt_info_label_time"));
    m_Time->SetCaption("");
    m_Time->SetCaptionColor(0xFFFFFFFF);
    m_Time->SetBackColor(0xE500071F);
  
    GraphicView::LoadImage(GetControl("prompt_info_icon"), "power");
    GetControl("prompt_info_icon")->SetBackColor(0xE500071F);
    GetControl("prompt_info_icon")->Hide();

    create_timer(this, &timer_id_, HandlePromptTime);
    stop_timer(timer_id_);
}

    Prompt::Prompt(int type , IComponent *parent)
        : SystemWindow(parent)
        , showtimes_(0)
        ,timercount(0)
        , m_bIsShowing(false)
        , m_prompt_standby_flag_(false)
        ,prompt_id_(PROMPT_INVALID)
{
    wname = "Prompt";
    Load();

    string bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
    SetWindowBackImage(bkgnd_bmp.c_str());
}

Prompt::~Prompt()
{
    delete_timer(timer_id_);
}
#define WIDTH 460//260
#define HIGHT  232//160
#define FULL_WIDTH 640//260
#define FULL_HIGHT  360-60*2//160

void Prompt::ShowPromptInfo(unsigned int prompt_id, unsigned int showtimes)
{
    db_warn("[debug-zhb]-----------prompt_id = %d  title = %s   text = %s",prompt_id,labelStringCmd[prompt_id].TitleCmd,labelStringCmd[prompt_id].InfoCmd);
	string title_str, info_str;
	int info_y,time_y,info_str_w,info_str_h,icon_x = 92,icon_w = 56,icon_h = 48,time_w = 34 , time_h = 42;  
	showtimes_ = showtimes;
	prompt_id_ = prompt_id;
	char buf[10]={0};
	m_bIsShowing = true;
	R::get()->GetString(labelStringCmd[prompt_id].TitleCmd, title_str);
	m_Title->SetCaption(title_str.c_str());
	R::get()->GetString(labelStringCmd[prompt_id].InfoCmd, info_str);
	m_Text->SetCaption(info_str.c_str());
	GraphicView::LoadImage(GetControl("prompt_info_icon"),labelStringCmd[prompt_id].image_icon);
	GraphicView::GetImageInfo(GetControl("prompt_info_icon"),&icon_w,&icon_h);
    if(prompt_id >= PROMPT_TF_FORMATTING && prompt_id <= PROMPT_RESET_FACTORY_FINISH){//style 5		
        m_Time->SetCaption("");
        icon_x = 198;
        info_y = 136;
        info_str_w = WIDTH - 30*2;
        info_str_h = HIGHT - info_y;
        GetControl("prompt_info_icon")->SetPosition(icon_x,56,icon_w,icon_h);
        GetControl("prompt_info_icon")->Show();
        m_Text->SetPosition(30,info_y,info_str_w,info_str_h);
    }else if(prompt_id >= PROMPT_TF_NULL && prompt_id <= PROMPT_EMERGENCY_RECORDING){//style 4
        m_Time->SetCaption("");
        info_y = (HIGHT/2) - (icon_h/2);
        info_str_w = WIDTH - (icon_x*2+ icon_w);//164
        info_str_h = HIGHT - info_y-5;
        GetControl("prompt_info_icon")->SetPosition(icon_x,info_y,icon_w,icon_h);
        GetControl("prompt_info_icon")->Show();
        m_Text->SetPosition(icon_x+icon_h+10,info_y+5,info_str_w,info_str_h);
    }else if(prompt_id >= PROMPT_STANDBY_MODE && prompt_id <= PROMPT_PWOER_DISCONNECTED){//style 3
        info_y = (HIGHT/2) -(icon_h+time_h)/2;//49
        time_y = info_y+icon_h;//97
        info_str_w = WIDTH - (icon_x*2+ icon_w);//164
        info_str_h = HIGHT - info_y-5;
        GetControl("prompt_info_icon")->SetPosition(icon_x,info_y,icon_w,icon_h);
        GetControl("prompt_info_icon")->Show();
        snprintf(buf,sizeof(buf),"%ds",showtimes);
        m_Time->SetPosition(icon_x+15,time_y,time_w,time_h);
        m_Time->SetCaption((const char*)buf);
        m_Time->Show();
        m_Text->SetPosition(icon_x+icon_h+54,info_y+25,info_str_w,info_str_h);
    }else if(prompt_id >= PROMPT_FULL_SDCARD_FORMAT && prompt_id<=PROMPT_FULL_WIFI_CONNET){//style 2
        info_y = (FULL_HIGHT-icon_h)/2;
        icon_x = (FULL_WIDTH-icon_w)/2;
        GetControl("prompt_info_icon")->SetPosition(icon_x,info_y,icon_w,icon_h);
        GetControl("prompt_info_icon")->Show();
        m_Title->SetPosition(0,5,FULL_WIDTH,45);
        m_Text->SetPosition(60,info_y+icon_h,FULL_WIDTH-2*60,FULL_HIGHT-(info_y+icon_h));
        m_Time->Hide();
    }
    else if(prompt_id >= PROMPT_ADAS_LEFT_LDW || prompt_id <= PROMPT_ADAS_DANGER_FATIGUE)
    {
        m_Time->SetCaption("");
        icon_x = 198;
        info_y = 136;
        info_str_w = WIDTH - 30*2;
        info_str_h = HIGHT - info_y;
        GetControl("prompt_info_icon")->SetPosition(icon_x,56,icon_w,icon_h);
        GetControl("prompt_info_icon")->Show();
        m_Text->SetPosition(30,info_y,info_str_w,info_str_h);
    } else if(prompt_id == PROMPT_NOTIC_UPDATE){
        info_y = (HIGHT / 2) -(icon_h + time_h) / 2;//49
        time_y = info_y;//97
        info_str_w = WIDTH - (icon_x * 2 + icon_w);//164
        info_str_h = HIGHT - info_y - 5;
        snprintf(buf, sizeof(buf), "%d", showtimes);
        m_Time->SetPosition(icon_x - 70, time_y, time_w, time_h);
        m_Time->SetCaption((const char*)buf);
        m_Time->Show();
        m_Text->SetPosition(icon_x - 50, info_y, info_str_w + 140, info_str_h);
    }
    else{ //style 1
        m_Time->SetCaption("");
        m_Time->Hide();
        GetControl("prompt_info_icon")->Hide();
        m_Text->SetPosition(30,70,400,130);
    }

    if(showtimes > 0)
        set_period_timer(1, 0, timer_id_);

    if(prompt_id == PROMPT_STANDBY_MODE) {
       db_warn("show stanby prompt,set m_prompt_standby_flag_ true");
       m_prompt_standby_flag_ = true;
    }
}

void Prompt::HidePromptInfo()
{
    stop_timer(timer_id_);
    m_bIsShowing = false;
    timercount = 0;
    this->DoHide();
    //setPromptId(PROMPT_INVALID);
}


void Prompt::HandlePromptTime(union sigval sigval)
{
     prctl(PR_SET_NAME, "PromptUpdate", 0, 0, 0);

     Prompt *pt = reinterpret_cast<Prompt*>(sigval.sival_ptr);
     pt->timercount++;
     db_msg("zhb---------pt->timercount = %d",pt->timercount);
     if(pt->prompt_id_ == PROMPT_PWOER_DISCONNECTED || pt->prompt_id_ == PROMPT_STANDBY_MODE){
         char buf[20]={0};
         snprintf(buf,sizeof(buf),"%ds",pt->showtimes_-pt->timercount);
         pt->m_Time->SetCaption((const char*)buf);
     }else if (pt->prompt_id_ == PROMPT_NOTIC_UPDATE)
     {
         char buf[10] = {0};
         snprintf(buf, sizeof(buf), "%d", pt->showtimes_-pt->timercount);
         pt->m_Time->SetCaption((const char*)buf);
     }

    if(pt->timercount == pt->showtimes_){
		 pt->HidePromptInfo();
		 usleep(500*1000);//wait hide finish
		 //fun
		if(pt->prompt_id_ == PROMPT_STANDBY_MODE)
		{
			pt->m_prompt_standby_flag_ = false;
			PowerManager *pm = PowerManager::GetInstance(); 			
			pm->EnterStandby();
		}
			
    }

}
void Prompt::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string Prompt::GetResourceName()
{
    return string(GetClassName());
}

void Prompt::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    switch (msg) {
        default:
            break;
    }
}

void Prompt::DoShow()
{
    WindowManager *wm = WindowManager::GetInstance();
    db_msg("debug_zhb---> prompt doshow window id:%d",wm->GetCurrentWinID());
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::EnableWindow(cur_win->GetHandle(), false);
    Widget::Show();
}

void Prompt::DoHide()
{
    WindowManager *wm = WindowManager::GetInstance();
    db_msg("debug_zhb---> prompt dohide window id:%d",wm->GetCurrentWinID());
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::SetActiveWindow(cur_win->GetHandle());
    ::EnableWindow(cur_win->GetHandle(), true);
    Widget::Hide();
}

void Prompt::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("prompt_title_label")||
            ctrl_name == string("prompt_info_icon")) {
        ctrl->SetCtrlTransparentStyle(true);
    }
    if (ctrl_name == string("prompt_info_label_time")||
            ctrl_name == string("prompt_info_label") || 
            ctrl_name == string("prompt_title_label")) {
        ctrl->SetCtrlTransparentStyle(true);
        TextView* prompt_ = reinterpret_cast<TextView *>(ctrl);
        prompt_->SetTextStyle(DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }
}

int Prompt::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;;
}

