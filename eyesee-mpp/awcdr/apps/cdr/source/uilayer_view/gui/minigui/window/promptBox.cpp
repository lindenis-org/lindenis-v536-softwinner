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
 #include "window/promptBox.h"
#include "window/preview_window.h"
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
#include "device_model/menu_config_lua.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "PromptBox"

using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(PromptBox)
static Mutex promptBox_mutex;
static PromptBox *m_promptBox = NULL;
int PromptBox::HandleMessage(HWND hwnd, int message, WPARAM wparam,
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


void PromptBox::keyProc(int keyCode,int isLongPress)
{
 	db_msg("[debug_zhb]----------promptBox keyproc");

    return;
}

PromptBox::PromptBox(IComponent *parent)
        : SystemWindow(parent)
        , showtimes_(0)
        , m_bIsShowing(false)
        ,m_len(-1)
{
    wname = "PromptBox";
    Load();
    listener_ = WindowManager::GetInstance();
    //SetBackColor(0xff000000);//96
//    string bkgnd_bmp = R::get()->GetImagePath("promptBoxbg");
//   SetWindowBackImage(bkgnd_bmp.c_str());
    int h;
    //string bkgnd_bmp = R::get()->GetImagePath("promptBoxbg");
   //SetWindowBackImage(bkgnd_bmp.c_str());
//    GraphicView::LoadImage(GetControl("PromptBox_icon"), "recording");
    GetControl("PromptBox_icon")->SetBackColor(0xb2000000);
    GetControl("PromptBox_icon")->Hide();
     GraphicView::LoadImage(GetControl("PromptBox_icon_left"), "prompt_left_bg");
    GraphicView::GetImageInfo(GetControl("PromptBox_icon_left"),&first_icon_width,&h);
    GetControl("PromptBox_icon_left")->SetBackColor(0x00000000);
    GetControl("PromptBox_icon_left")->Hide();

    GraphicView::LoadImage(GetControl("PromptBox_icon_right"), "prompt_right_bg");
    GraphicView::GetImageInfo(GetControl("PromptBox_icon_right"),&third_icon_width,&h);
    GetControl("PromptBox_icon_right")->SetBackColor(0x00000000);
    GetControl("PromptBox_icon_right")->Hide();

    m_Text = reinterpret_cast<TextView*>(GetControl("PromptBox_label"));
    m_Text->SetCaption("");
    m_Text->SetCaptionColor(0xFFFFFFFF);//front
    m_Text->SetBackColor(0xb2000000);
   // m_Text->SetPosition(0, 0, 0, 0);
    create_timer(this, &timer_id_, HandlePromptBoxTime);
}

PromptBox::~PromptBox()
{
	delete_timer(timer_id_);
}

void PromptBox::ShowPromptBox(unsigned int promptbox_id, unsigned int showtimes)
{
   //db_msg("[debug_zhb]-------------m_bIsShowing = %d",m_bIsShowing);
#if 1
    if (m_bIsShowing) {
		HidePromptBox();
        return ;
    }
    SetPromptBoxShowFlag(true);
#endif
    int stringLen = -1;
    int h = -1;
    int first_icon_x = -1,label_string_width = -1,second_icon_width = 0,second_icon_height = -1;
    string info_str;  
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
     int val = menuconfiglua->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);
    // db_msg("[debug_zhb]-------------val = %d",val);
    showtimes_ = showtimes;
    if(promptbox_id >= PROMPT_BOX_DEVICE_RECORDING){
		second_icon_width = 0;
		GetControl("PromptBox_icon")->Hide();
    	}else{
    		GraphicView::LoadImage(GetControl("PromptBox_icon"),promptboxinfo[promptbox_id].image_icon);
    		GraphicView::GetImageInfo(GetControl("PromptBox_icon"),&second_icon_width,&second_icon_height);
		GetControl("PromptBox_icon")->Show();
    	}
    R::get()->GetString(promptboxinfo[promptbox_id].mLabel, info_str);
    stringLen = strlen(info_str.c_str());
    if(val)
    	label_string_width = stringLen*11;//language is english
    else
	label_string_width = stringLen*4*10/5+20;//language is chinese

	first_icon_x = 0;
    //db_msg("[debug_zhb]----------first_icon_x = %d",first_icon_x);
    GetControl("PromptBox_icon_left")->SetPosition(first_icon_x,0,first_icon_width,48);
    //db_msg("[debug_zhb]----------first_icon_x+first_icon_width = %d",first_icon_x+first_icon_width);
    GetControl("PromptBox_icon")->SetPosition(first_icon_x+first_icon_width,0,second_icon_width,48); 
    //db_msg("[debug_zhb]----------first_icon_x+first_icon_width+second_icon_width = %d",first_icon_x+first_icon_width+second_icon_width);
    m_Text->SetPosition(first_icon_x+first_icon_width+second_icon_width, 0, label_string_width, 48);
    //db_msg("[debug_zhb]----------first_icon_x+first_icon_width+second_icon_width+label_string_width = %d",first_icon_x+first_icon_width+second_icon_width+label_string_width);
    GetControl("PromptBox_icon_right")->SetPosition(first_icon_x+first_icon_width+second_icon_width+label_string_width,0,third_icon_width,48);
    GetControl("PromptBox_icon_left")->Show();
    GetControl("PromptBox_icon_right")->Show();
    db_msg("[debug_zhb]-------------info_str.c_str() = %s",info_str.c_str());
//    m_Text->SetCaption(info_str.c_str());
    m_Text->SetCaptionEx(info_str.c_str());
    usleep(500*1000); //在预览界面会概率性出现提示框没有字符串显示出来,通过加延时和修改显示字符串接口来规避
    set_one_shot_timer(showtimes, 0, timer_id_);   
    //m_bIsShowing = true;
	setPromptBoxLen(first_icon_width+third_icon_width+second_icon_width+label_string_width);
	SetPromptBoxShowFlag(true);
}
void PromptBox::HandlePromptBoxTime(union sigval sigval)
{
     prctl(PR_SET_NAME, "PromptBoxUpdate", 0, 0, 0);

     PromptBox *ptb = reinterpret_cast<PromptBox*>(sigval.sival_ptr);
     ptb->HidePromptBox();

}

void PromptBox::HidePromptBox()
{
    m_bIsShowing = false;
    this->DoHide();
}
void PromptBox::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string PromptBox::GetResourceName()
{
    return string(GetClassName());
}

void PromptBox::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    switch (msg) {
	case MSG_PROMPTBOX_STANDBY_BREAK:
		db_msg("[debug_zhb]-----MSG_PROMPTBOX_STANDBY_BREAK");
		stop_timer(timer_id_); 
		HidePromptBox();
		break;
        default:
            break;
    }
}

void PromptBox::DoShow()
{
    WindowManager *wm = WindowManager::GetInstance();
	 db_msg("debug_zhb---> promptbox doshow window id:%d",wm->GetCurrentWinID());
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::EnableWindow(cur_win->GetHandle(), false);
    Widget::Show();
}

void PromptBox::DoHide()
{
    WindowManager *wm = WindowManager::GetInstance();
    db_msg("debug_zhb----PromptBox--DoHide--wm->GetCurrentWinID() = %d",wm->GetCurrentWinID());
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::SetActiveWindow(cur_win->GetHandle());
    ::EnableWindow(cur_win->GetHandle(), true);
    Widget::Hide();
}

void PromptBox::PreInitCtrl(View *ctrl, string &ctrl_name)
{
if (ctrl_name == string("PromptBox_icon_left") ||
	ctrl_name == string("PromptBox_icon_right")||
	ctrl_name == string("PromptBox_icon")) {
        ctrl->SetCtrlTransparentStyle(false);
    }
    if (ctrl_name == string("PromptBox_label")) {
        ctrl->SetCtrlTransparentStyle(false);
	 TextView* promptbox_ = reinterpret_cast<TextView *>(ctrl);
        promptbox_->SetTextStyle(DT_NOCLIP|DT_SINGLELINE| DT_CENTER | DT_VCENTER);
    }
}

int PromptBox::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;;
}

