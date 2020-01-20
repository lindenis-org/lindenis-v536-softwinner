/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file submenu.cpp
 * @brief 对话框窗口
 * @author id:690
 * @version v0.3
 * @date 2017-02-07
 */
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "widgets/menu_items.h"
#include "common/setting_menu_id.h"
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/screensaver.h"
#include "window/dialog.h"
#include "widgets/button.h"
#include "widgets/text_view.h"
#include "widgets/switch_button.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "Submenu"

using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(Submenu)


void Submenu::keyProc(int keyCode,int isLongPress)
{

    switch(keyCode){
        case SDV_KEY_LEFT:
		db_msg("[debug_zhb]------Submenu----SDV_KEY_LEFT");
            break;
        case SDV_KEY_POWER:
		 db_msg("[debug_zhb]------Submenu----SDV_KEY_POWER");
            this->DoHide();
            static_cast<Window *>(parent_)->DoShow();
            break;
        case SDV_KEY_OK:
	     db_msg("[debug_zhb]------Submenu----SDV_KEY_OK");
            if(isLongPress == LONG_PRESS){
            static_cast<Window *>(parent_)->DoShow();
            submenuitemkeyproc();
            this->DoHide();
            }
            break;
        case SDV_KEY_RIGHT:
	     db_msg("[debug_zhb]------Submenu----SDV_KEY_RIGHT");
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }

}
void Submenu::SaveSubMenuChanged(int val)
{
	db_msg("[debug_zhb]------Submenu----SaveSubMenuChanged---val = %d",val);
	static_cast<Window *>(parent_)->DoShow();
	if(val){/*if val is 1 , mean shuold change the submenu function and update the list item information*/
      		 submenuitemkeyproc();
	}else{
		SetDefaultHighLight();/*if val is 0,mean not save the changed*/
		}
       this->DoHide();
}
int Submenu::GetNotifyMessage()
{
    int ret = 0;
    SettingWindow *p = (SettingWindow *)_Parent;
    switch(p->GetMenuIndex()){
        case SETTING_RECORD_RESOLUTION:
            ret = MSG_SET_VIDEO_RESOULATION;
            break;
	case SETTING_REAR_RECORD_RESOLUTION:
            ret = MSG_SET_REAR_RECORD_RESOLUTION;
            break;
	case SETTING_RECORD_VOLUME:
	     ret = MSG_SET_RECORD_VOLUME;
		break;
	case SETTING_SCREEN_BRIGHTNESS:
            ret = MSG_SET_SCREEN_BRIGHTNESS;
            break;
	case SETTING_SCREEN_NOT_DISTURB_MODE:
            ret = MSG_SET_SCREEN_NOT_DISTURB_MODE;
            break;
	case SETTING_VOICE_TAKE_PHOTO:
	     ret = MSG_SET_VOICE_TAKE_PHOTO;
		break;
	case SETTING_VOLUME_SELECTION:
	     ret = MSG_SET_VOLUME_SELECTION;
		break;
	case SETTING_POWERON_SOUND_SWITCH:
	     ret = MSG_SET_POWERON_SOUND_SWITCH;
		break;
	case SETTING_KEY_SOUND_SWITCH:
	     ret = MSG_SET_KEY_SOUND_SWITCH;
		break;
	case SETTING_DRIVERING_REPORT_SWITCH:
	     ret = MSG_SET_DRIVERING_REPORT_SWITCH;
		break;	
	case SETTING_ADAS_SWITCH:
	     ret = MSG_SET_ADAS_SWITCH;
		break;
	case SETTING_FORWARD_COLLISION_WARNING:
	     ret = MSG_SET_FORWARD_COLLISION_WARNING;
		break;
	case SETTING_LANE_SHIFT_REMINDING:
	     ret = MSG_SET_LANE_SHIFT_REMINDING;
		break;
	case SETTING_WATCH_DOG_SWITCH:
	     ret = MSG_SET_WATCH_DOG_SWITCH;
		break;
	case SETTING_TIMEWATERMARK:
	     ret = MSG_SET_TIEM_WATER_MARK;
		break;
	case SETTING_EMER_RECORD_SWITCH:
	     ret = MSG_SET_EMER_RECORD_SWITCH;
		break;
	case SETTING_EMER_RECORD_SENSITIVITY:
	     ret = MSG_SET_EMER_RECORD_SENSITIVITY;
		break;
	case SETTING_PARKING_MONITORY:
	     ret = MSG_SET_PARKING_MONITORY;
		break;
	case SETTING_PARKING_RECORD_LOOP_SWITCH:
	     ret = MSG_SET_PARKING_RECORD_LOOP_SWITCH;
		break;
#ifdef ENABLE_ENC_TYPE_SELECT
        case SETTING_RECORD_ENCODINGTYPE:
            ret = MSG_SET_RECORD_ENCODE_TYPE;
            break;
#endif
        case SETTING_RECORD_LOOP:
            ret = MSG_SET_RECORD_TIME;
            break;
        case SETTING_RECORD_TIMELAPSE:
            ret = MSG_SET_RECORD_DELAY_TIME;
            break;
        case SETTING_RECORD_SLOWMOTION:
            ret = MSG_SET_RECORD_SLOW_TIME;
            break;
        case SETTING_PHOTO_RESOLUTION:
            ret = MSG_SET_PIC_RESOULATION;
            break;
        case SETTING_PHOTO_TIMED:
            ret = MSG_SET_TIME_TAKE_PIC;
            break;
        case SETTING_PHOTO_AUTO:
            ret = MSG_SET_AUTO_TIME_TAKE_PIC;
            break;
        case SETTING_PHOTO_DRAMASHOT:
            ret =MSG_SET_PIC_CONTINOUS;
            break;
		case SETTING_PHOTO_QUALITY:
			ret =MSG_SET_PIC_QUALITY;	
			break;
        case SETTING_CAMERA_EXPOSURE:
            ret = MSG_SET_CAMERA_EXPOSURE;
            break;
        case SETTING_CAMERA_WHITEBALANCE:
            ret = MSG_SET_CAMERA_WHITEBALANCE;
            break;
        case SETTING_CAMERA_LIGHTSOURCEFREQUENCY:
            ret = MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY;
            break;
        case SETTING_CAMERA_AUTOSCREENSAVER:
            ret = MSG_SET_AUTO_TIME_SCREENSAVER;
            break;
        case SETTING_CAMERA_TIMEDSHUTDOWN:
            ret = MSG_SET_AUTO_TIME_SHUTDOWN;
            break;
        case SETTING_DEVICE_LANGUAGE:
            ret = MSG_RM_LANG_CHANGED;
            break;
	 case SETTING_DEVICE_FORMAT:
            ret = MSG_DEVICE_FORMAT;
            break;	  	
	 case SETTING_DEVICE_RESETFACTORY:
            ret = MSG_DEVICE_RESET_FACTORY;
            break;		
        case SETTING_DEVICE_DEVICEINFO:
            ret = MSG_SYSTEM_UPDATE;
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}
/*first into submenu,shuold save the hightLight index*/
void Submenu::SaveDefaultHighLight()
{
    MenuItems *submenu = reinterpret_cast<MenuItems*>(GetControl("submenulist_View"));
    default_hilight_idx_ = submenu->GetHilight();
	db_msg("[debug_zhb]: SaveDefaultHighLight------this default_hilight_idx_ is  =%d ",default_hilight_idx_);
}
/*if the submenu item high light idex was changed ,But the selection does not save the change, this time should restore the highlight position */
void Submenu::SetDefaultHighLight()
{
    MenuItems *submenu = reinterpret_cast<MenuItems*>(GetControl("submenulist_View"));
     db_msg("[debug_zhb]: SaveDefaultHighLight------this default_hilight_idx_ is  =%d ",default_hilight_idx_);
     submenu->SetHilight(default_hilight_idx_);	
}
/*Decide whether to display a prompt box or not by judging whether the highlighting position is changed */
bool Submenu::IfDefaultHighLightChanged()
{
    MenuItems *submenu = reinterpret_cast<MenuItems*>(GetControl("submenulist_View"));
     db_msg("[debug_zhb]: IfDefaultHighLightChanged------this default_hilight_idx_ is  =%d    submenu->GetHilight() = %d",default_hilight_idx_,submenu->GetHilight());
     if(submenu->GetHilight() == default_hilight_idx_)
	 	return false;
     else
	 	return true;
}

void Submenu::submenuitemkeyproc()
{
    int select_item = 0;
    int message = 0;
    MenuItems *submenu = reinterpret_cast<MenuItems*>(GetControl("submenulist_View"));
    select_item = submenu->GetHilight();
    SetDefaultValue(select_item);
    message = GetNotifyMessage();
    db_msg("[debug_joasn]: this select_item is %d message is %d ",select_item,message);

    if( MSG_DEVICE_FORMAT ==  message )
        listener_->notify(this, message,select_item);
    else
        listener_->sendmsg(this,message,select_item);
}

void Submenu::SetDefaultValue(int select_item)
{
      int count =0;
      MenuItems *submenu = reinterpret_cast<MenuItems*>(GetControl("submenulist_View"));
      count = submenu->GetCount();
      db_msg("zhb-------------SetDefaultValue count= %d",count);
      for(int i= 0; i<count; i++ ){
      int &value = submenu->GetItemData(i)->value;
      value =select_item;
      }
      db_msg("[fangjj]: SetDefaultValue  count:=[%d]   submenu:data->value =:[%d] ",count,select_item);  
}
/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int Submenu::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message ) {
    case MSG_PAINT:
        return HELP_ME_OUT;
		case MSG_TIMER:
            break;
    default:
        return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
   }
    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

Submenu::Submenu(IComponent *parent)
    : SystemWindow(parent)
    , _Parent(parent)
    ,default_hilight_idx_(-1)
{
    Load();
    listener_ = WindowManager::GetInstance();
    //GetControl(const char * widget_name)
    
    // save_dialog_
    save_dialog_ = new Dialog(this);
    save_dialog_->SetBackColor(0xFF0d0246);
    //save_dialog_->SetPosition(30, 30, 260, 160);
  #if 0
    string del_title_str;
   R::get()->GetString("save_dialog", del_title_str);
    TextView *save_title = reinterpret_cast<TextView*>(save_dialog_->GetControl("dialog_title"));
    save_title->SetCaption(del_title_str.c_str());
    save_title->SetBackColor(0xFF999999); // 灰色
    //save_title->SetPosition(30, 30, 230, 0);
#endif
	string save_info_str;
    R::get()->GetString("save_info", save_info_str);
	TextView *save_info_text = static_cast<TextView*>(save_dialog_->GetControl("info_text"));
       save_info_text->SetCaption(save_info_str.c_str());
	save_info_text->SetCaptionColor(0xFFFFFFFF);
  	//save_info_text->SetPosition(0, 45, 260, 30);
    // confirm button of dialog
    Button *save_confirm_button = reinterpret_cast<Button*>(save_dialog_->GetControl("confirm_button"));
    save_confirm_button->SetTag(CONFIRM_BUTTON);
    save_confirm_button->OnPushed.bind(this, &Submenu::SaveDialogProc);

     //save_confirm_button->SetPosition(0,120,130,40);
    // // cancel button of dialog
    Button *save_cancel_button = reinterpret_cast<Button*>(save_dialog_->GetControl("cancel_button"));
    save_cancel_button->SetTag(CANCEL_BUTTON);
    //save_confirm_button->SetPosition(130,120,130,40);
    save_cancel_button->OnPushed.bind(this, &Submenu::SaveDialogProc);
}


Submenu::~Submenu()
{
}

void Submenu::ShowSaveDialog()
{
	db_msg("[debug_zhb]-------ShowSaveDialog ");
    save_dialog_->DoShow();
}

void Submenu::HideSaveDialog()
{
     db_msg("[debug_zhb]-------HideSaveDialog ");
    save_dialog_->ShowParent();
}

void Submenu::SaveDialogProc(View *control)
{
    save_dialog_->DoHide();

    switch (control->GetTag()) {
        case CONFIRM_BUTTON: {
			db_msg("[debug_zhb]---SaveDialogProc CONFIRM_BUTTON ");
            }
            break;
        case CANCEL_BUTTON:
			db_msg("[debug_zhb]----SaveDialogProc CANCEL_BUTTON ");
            break;
        default:
            break;
    }
}

void Submenu::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string Submenu::GetResourceName()
{
    return string(GetClassName());
}

void Submenu::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}

void Submenu::SetIndexToSubmenu(int index)
{
    select_index = index;
    db_msg("[debug_jaosn]:this select_index is %d",select_index);
}

void Submenu::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
}

void Submenu::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
}

void Submenu::close()
{
    ::DestroyWindow(parent_->GetHandle());
}

int Submenu::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;
}

