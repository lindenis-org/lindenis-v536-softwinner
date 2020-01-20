/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file dialog.cpp
 * @brief 对话框窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-07
 */
#include "window/preview_window.h"
#include "window/setting_window.h"
#include "window/dialog.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "application.h"
#include "bll_presenter/audioCtrl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "Dialog"

using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(Dialog)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int Dialog::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{    
    switch ( message ) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_TIMER:
            break;
        case MSG_LBUTTONUP:
            {
                int x = LOSWORD (lparam);
                int y = HISWORD (lparam);
                //OnMouseUp(message, x, y);
            }
            break;
        case MSG_COMMAND:
            switch(wparam)
            {
                db_msg("wparam ===== %d", wparam);
                case IDOK:
                if(cur_select != BUTTON_OK)
                {
                    cur_select = BUTTON_OK;
                    this->FocusOkButton();
                }
                //else
               // {
                    DoKeyOkHanderSelectionDialog();
                //}
                break;
                case IDCANCEL:
                if(cur_select != BUTTON_CANCEL)
                {
                    cur_select = BUTTON_CANCEL;
                    this->FocusCancelButton();
                }
                //else
               // {
                    DoKeyOkHanderSelectionDialog();
               // }
                break;
                default:
                break;
            }
        default:
            break;
    }
    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}


void Dialog::keyProc(int keyCode,int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_OK:
            {
                if(type_ == INFO_DIALOG){
                    DoKeyHanderInfoDialog(isLongPress);
                }else if(type_ == SELECTION_DIALOG){
                    DoKeyOkHanderSelectionDialog();
                }
            }
            break;
        case SDV_KEY_POWER:
#if 0
		 if(type_ == INFO_DIALOG){
                DoKeyHanderInfoDialog(isLongPress);
            }else{
			CancelDialog();
            	}
#endif
         break;
        case SDV_KEY_LEFT:
        {
            if(type_ == INFO_DIALOG){
				DoKeyLeftHanderInfoDialog();
            }else if(type_ == SELECTION_DIALOG){
                //DoKeyLeftHanderSelectionDialog();
                DoKeyHanderSelectionDialog();
			}
        }
            break;
        case SDV_KEY_RIGHT:
        {
            if(type_ == INFO_DIALOG){
				DoKeyRightHanderInfoDialog();
            }else if(type_ == SELECTION_DIALOG){
                //DoKeyRightHanderSelectionDialog();
                DoKeyHanderSelectionDialog();
			}
        }
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }

}

Dialog::Dialog(IComponent *parent)
    : SystemWindow(parent)
    , cur_select(BUTTON_OK)
    , m_nNewHandletype(false)
    , m_initFocusFlag(false)
{
    type_ = SELECTION_DIALOG;
    focus_color = 0xFF2772DB;
    //default_color = 0xFF2A3350; use for key
    default_color = 0xFF2A3350; //use for key
    //default_color = 0xFF2772DB;
    Load();
    listener_ = WindowManager::GetInstance();
    string bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
    SetWindowBackImage(bkgnd_bmp.c_str());
	pthread_mutex_init(&d_lock_, NULL);
}

Dialog::Dialog(DIALOG_TYPE type, IComponent *parent)
    : SystemWindow(parent)
    , type_(type)
    , m_nNewHandletype(false)
    , m_initFocusFlag(false)
{
    Load();
    wname="Dialog";
    listener_ = WindowManager::GetInstance();
    string bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
    SetWindowBackImage(bkgnd_bmp.c_str());
}

Dialog::~Dialog()
{
}

void Dialog::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE |WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

void Dialog::SetDialogMsgType(DIALOG_TYPE type)
{
    type_ = type;
}

string Dialog::GetResourceName()
{
    return string(GetClassName());
}

void Dialog::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}

void Dialog::DoShow()
{
    Window::DoShow();
	 db_msg("debug_zhb---> button dialog doshow window id:%d",WindowManager::GetInstance()->GetCurrentWinID());
    ::EnableWindow(parent_->GetHandle(), false);
}

void Dialog::DoHide()
{
    SetVisible(false);
	db_msg("debug_zhb---> button dialog doHide window id:%d",WindowManager::GetInstance()->GetCurrentWinID());
    ::EnableWindow(parent_->GetHandle(), true);
    ::SetActiveWindow(parent_->GetHandle());
}
void Dialog::DoHideDialog()
{
	Window *win = static_cast<Window *>(parent_);
	win->DoShow();
	this->DoHide();
	this->cur_select = BUTTON_OK;
       this->FocusOkButton();
}
void Dialog::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("dialog_title")||
	ctrl_name == string("dialog_icon1")||
	ctrl_name == string("dialog_icon2")||
	ctrl_name == string("dialog_icon3")||
	ctrl_name == string("dialog_progress_bar_background")||
	ctrl_name == string("dialog_progress_bar_top_color")) {
        ctrl->SetCtrlTransparentStyle(true);
    }


    if (ctrl_name == string("confirm_button")) {
		ctrl->SetBackColor(focus_color);
        ctrl->SetVisible((type_ == SELECTION_DIALOG));
    }

	if (ctrl_name == string("cancel_button")) {
        ctrl->SetBackColor(default_color);
        ctrl->SetVisible((type_ == SELECTION_DIALOG));
    }

    if (ctrl_name == string("info_text")) {
        //ctrl->SetVisible((type_ == INFO_DIALOG));
        ctrl->SetCtrlTransparentStyle(true);
        ctrl->SetVisible(true);
        //::ExcludeWindowStyle(ctrl->GetHandle(), SS_CENTER);
        //::IncludeWindowStyle(ctrl->GetHandle(), SS_LEFT);
	TextView* mDialog_ = reinterpret_cast<TextView *>(ctrl);
        mDialog_->SetTextStyle(DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }
}

int Dialog::OnMouseUp(unsigned int button_status, int x, int y)
{
    db_warn("[habo] button_status============%d", button_status);
    if (type_ == INFO_DIALOG &&  OnClick)
        OnClick(this);

    return HELP_ME_OUT;;
}


void Dialog::DoKeyHanderInfoDialog(int type)
{
    Window *win = static_cast<Window *>(parent_);
    WindowID win_id = (WindowID)win->GetTag();
    if(type == LONG_PRESS)
    {
        if(win_id == WINDOWID_PREVIEW)
        {
            db_msg("send PREVIEW_WIFI_BUTTON message");
            listener_->sendmsg(win, PREVIEW_WIFI_SWITCH_BUTTON, 0);
            listener_->sendmsg(win, PREVIEW_WIFI_DIALOG_HIDE, 0);
        }
    }else{
        if(win_id == WINDOWID_PREVIEW){
        }
        if(win_id == WINDOWID_SETTING || win_id == WINDOWID_SETTING_NEW){
            win->DoShow();
	     listener_->notify(win,SETTING_BUTTON_DIALOG_INFO,0);
            this->DoHide();
        }
    }
}

void Dialog::DoKeyLeftHanderInfoDialog()

{
    Window *win = static_cast<Window *>(parent_);
    WindowID win_id = (WindowID)win->GetTag();
    if(win_id == WINDOWID_PREVIEW)
    {
        db_msg("send PREVIEW_WIFI_BUTTON message");
        listener_->sendmsg(win, PREVIEW_WIFI_SWITCH_BUTTON, 0);
        listener_->sendmsg(win, PREVIEW_WIFI_DIALOG_HIDE, 0);
    }
}

void Dialog::DoKeyRightHanderInfoDialog()

{
    Window *win = static_cast<Window *>(parent_);
    WindowID win_id = (WindowID)win->GetTag();
    if(win_id == WINDOWID_SETTING || win_id == WINDOWID_SETTING_NEW )
    {
        if( access("/mnt/extsd/sun8iw12p1_linux_v5s_pro_lpddr3_uart0.img",F_OK)== 0)
        {
            win->DoShow();
            listener_->notify(win, MSG_SETTING_SHOWUPDATE_DIALOG, 0);
            this->DoHide();
        }
    }
}

void Dialog::DoKeyOkHanderInfoDialog()
{
    if( m_nNewHandletype )
        return;

    Window *win = static_cast<Window *>(parent_);
    WindowID win_id = (WindowID)win->GetTag();
    m_nNewHandletype = false;
    if( win_id == WINDOWID_PREVIEW)
    {
        listener_->sendmsg(win,PREVIEW_WIFI_DIALOG_HIDE,0); 
        return ;
    }
    win->DoShow();
    this->DoHide();
}

void Dialog::DoKeyLeftHanderSelectionDialog()

{
    db_msg("DoKeyLeftHanderSelectionDialog");
	if(cur_select != BUTTON_OK)
		cur_select = BUTTON_OK;

	this->FocusOkButton();
}

void Dialog::DoKeyRightHanderSelectionDialog()

{
    db_msg("DoKeyRightHanderSelectionDialog");
    if(cur_select != BUTTON_CANCEL)
		cur_select = BUTTON_CANCEL;

	this->FocusCancelButton();
}

void Dialog::DoKeyHanderSelectionDialog()

{
    db_msg("DoKeyHanderSelectionDialog");
    if(cur_select != BUTTON_CANCEL){
		cur_select = BUTTON_CANCEL;
		this->FocusCancelButton();
	}else{
		cur_select = BUTTON_OK;
		this->FocusOkButton();
	}
}


void Dialog::DoKeyOkHanderSelectionDialog()

{
    db_msg("DoKeyOkHanderSelectionDialog");
    Window *win = static_cast<Window *>(parent_);
	win->DoShow();
    WindowID win_id = (WindowID)win->GetTag();
    db_msg("[debug_zhb]--------win_id = %d",win_id);
    if(win_id == WINDOWID_PLAYBACK)
    {
        db_msg("send MSG_PLAYBACK_DELETE_FILE message");
        if (cur_select == BUTTON_OK)
        {
            // this->DoHide();
            listener_->notify(win,MSG_PLAYBACK_DELETE_FILE,1);
        }else {
            listener_->notify(win,MSG_PLAYBACK_DELETE_FILE,0);
        }
        // this->cur_select = BUTTON_OK;
        //  this->FocusOkButton();
    }
    else if( win_id == WINDOWID_PREVIEW)
    {
        if (cur_select == BUTTON_OK)
        {
            db_msg("[debug_zhb]---WINDOWID_PREVIEW-----BUTTON_OK");
            this->DoHide();
            listener_->notify(win,PREVIEW_BUTTON_DIALOG_HIDE,1);
        }else{
            listener_->notify(win,PREVIEW_BUTTON_DIALOG_HIDE,0);
        }
        this->cur_select = BUTTON_OK;
        this->FocusOkButton();
    }
    else if( win_id == WINDOWID_SETTING || win_id == WINDOWID_SETTING_NEW)
    {
        if (cur_select == BUTTON_OK)
        {
            db_error("[debug_zhb]---WINDOWID_SETTING-----BUTTON_OK");
            this->DoHide();
            listener_->notify(win, SETTING_BUTTON_DIALOG, 1);
        }else {
            db_error("[debug_zhb]---WINDOWID_SETTING-----BUTTON_CANCEL");
            listener_->notify(win, SETTING_BUTTON_DIALOG, 0);
        }
        this->cur_select = BUTTON_OK;
        this->FocusOkButton();
    }
    this->DoHide();
    this->cur_select = BUTTON_OK;
    this->FocusOkButton();
}
void Dialog::CancelDialog()

{
    db_msg("CancelDialog");
    Window *win = static_cast<Window *>(parent_);
	win->DoShow();
    WindowID win_id = (WindowID)win->GetTag();
    db_msg("[debug_zhb]--------win_id = %d",win_id);
    if(win_id == WINDOWID_PLAYBACK)
    {
        db_msg("send MSG_PLAYBACK_DELETE_FILE message");
        listener_->notify(win,MSG_PLAYBACK_DELETE_FILE,0);
    }
    else if( win_id == WINDOWID_PREVIEW)
    {
        db_msg("send PREVIEW_BUTTON_DIALOG_HIDE message");
        listener_->notify(win,PREVIEW_BUTTON_DIALOG_HIDE,0);
    }
    else if( win_id == WINDOWID_SETTING || win_id == WINDOWID_SETTING_NEW)
    {
        db_msg("send SETTING_BUTTON_DIALOG message");
        listener_->notify(win,SETTING_BUTTON_DIALOG,0);
    }
    this->DoHide();
    this->cur_select = BUTTON_OK;
    this->FocusOkButton();
}

void Dialog::ShowParent()
{
    Window *win = static_cast<Window *>(parent_);
    win->DoShow();
    this->DoHide();
}

void Dialog::FocusOkButton()

{
	this->GetControl("confirm_button")->SetBackColor(focus_color);
    this->GetControl("confirm_button")->Hide();
    this->GetControl("confirm_button")->Show();
    this->GetControl("cancel_button")->SetBackColor(default_color);
    this->GetControl("cancel_button")->Hide();
    this->GetControl("cancel_button")->Show();
}

void Dialog::FocusCancelButton()

{
	this->GetControl("confirm_button")->SetBackColor(default_color);
    this->GetControl("confirm_button")->Hide();
    this->GetControl("confirm_button")->Show();
    this->GetControl("cancel_button")->SetBackColor(focus_color);
    this->GetControl("cancel_button")->Hide();
    this->GetControl("cancel_button")->Show();
}

int Dialog::SetKeyNewHandleFlag(bool p_bNewType)
{
    m_nNewHandletype = p_bNewType;
    return 0;
}
