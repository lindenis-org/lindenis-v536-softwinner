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
//#define NDEBUG 
#include "window/bulletCollection.h"
#include "common/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "window/dialog.h"
#include "widgets/button.h"
#include "widgets/buttonOK.h"
#include "widgets/buttonCancel.h"
#include <sstream>
#include "window/setting_window.h"
#include "window/window_manager.h"
#include "device_model/version_update_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "BulletCollection"
using namespace std;
using namespace EyeseeLinux;



BulletCollection::BulletCollection()
	:button_dialog_(NULL)
	,button_dialog_title(NULL)
	,button_dialog_text(NULL)
	,button_dialog_confirm(NULL)
	,button_dialog_cancel(NULL)
	,button_dialog_progress_bar_background(NULL)
	,button_dialog_progress_bar_top_color(NULL)
	,m_button_dialog_flag_(false)
	,total_data(0.0)
	,current_data(0.0)
	,fdownload(false)
	,finstall(false)
{
	create_timer(this, &progressbar_timer_id, HandleProgressbarTime);
       stop_timer(progressbar_timer_id);
}

BulletCollection::~BulletCollection()
{
	db_msg("debug_zhb------~BulletCollection()-");
 	if (button_dialog_) {
		delete button_dialog_;
		button_dialog_ = NULL;
		}
	if (button_dialog_title) {
		delete button_dialog_title;
		button_dialog_title = NULL;
		}
	if (button_dialog_text) {
		delete button_dialog_text;
		button_dialog_text = NULL;
		}
	if (button_dialog_confirm) {
		delete button_dialog_confirm;
		button_dialog_confirm = NULL;
		}
	if (button_dialog_cancel) {
		delete button_dialog_cancel;
		button_dialog_cancel = NULL;
		}
	if (button_dialog_progress_bar_background) {
		delete button_dialog_progress_bar_background;
		button_dialog_progress_bar_background = NULL;
		}
	if (button_dialog_progress_bar_top_color) {
		delete button_dialog_progress_bar_top_color;
		button_dialog_progress_bar_top_color = NULL;
		}
}

void BulletCollection::initButtonDialog(IComponent *parent)
{
    db_msg("debug_zhb--------ready to create dialog");
    button_dialog_ = new Dialog(parent);

    std::string button_dialog_str;
    R::get()->GetString("ml_prompt", button_dialog_str);
    button_dialog_title = reinterpret_cast<TextView*>(button_dialog_->GetControl("dialog_title"));
    button_dialog_title->SetCaption(button_dialog_str.c_str());
    button_dialog_title->SetCaptionColor(0xFFFFFFFF); // 灰色
    button_dialog_title->SetOptionStyle(DT_CENTER);
    //button_dialog_title->SetPosition(0, 0, 280, 30);


    R::get()->GetString("ml_confirm_reboot", button_dialog_str);
    button_dialog_text = static_cast<TextView*>(button_dialog_->GetControl("info_text"));
    button_dialog_text->SetCaption(button_dialog_str.c_str());
    button_dialog_text->SetCaptionColor(0xFFFFFFFF);
    button_dialog_text->SetOptionStyle(DT_CENTER|DT_VCENTER|DT_WORDBREAK);

    // confirm button of dialog
    std::string txt_info;
    R::get()->GetString("ml_confirm_ok", txt_info);
    button_dialog_confirm = reinterpret_cast<ButtonOK*>(button_dialog_->GetControl("confirm_button"));
    button_dialog_confirm->SetCaption(txt_info.c_str());
    button_dialog_confirm->SetTag(CONFIRM_BUTTON);

    // // cancel button of dialog
    R::get()->GetString("ml_confirm_no", txt_info);
    button_dialog_cancel = reinterpret_cast<ButtonCancel*>(button_dialog_->GetControl("cancel_button"));
    button_dialog_cancel->SetCaption(txt_info.c_str());
    button_dialog_cancel->SetTag(CANCEL_BUTTON);

    button_dialog_progress_bar_background = reinterpret_cast<Button*>(button_dialog_->GetControl("dialog_progress_bar_background"));
    button_dialog_progress_bar_background->SetBackColor(0x4CD8D8D8);

    button_dialog_progress_bar_top_color = reinterpret_cast<Button*>(button_dialog_->GetControl("dialog_progress_bar_top_color"));
    button_dialog_progress_bar_top_color->SetBackColor(0xFF2772DB);
}

void BulletCollection::keyProc(int keyCode, int isLongPress)
{
	db_warn("this is invild key code keyCode = %d ",keyCode);
	switch(keyCode)
	{
	    case SDV_KEY_MENU:
		case SDV_KEY_MODE:
	    {
	        if(isLongPress == LONG_PRESS){
	        } else {
                this->BCDoHide();
                
	        }
	    }
	    break;
		case SDV_KEY_LEFT:
		{	
			db_warn("this is SDV_KEY_LEFT key code ");
			//button_dialog_confirm->SetTag(CONFIRM_BUTTON);
			button_dialog_->keyProc(SDV_KEY_LEFT, SHORT_PRESS);
			
		}
		break;
		case SDV_KEY_RIGHT:
		{
			db_warn("this is SDV_KEY_RIGHT key code   ");
			button_dialog_->keyProc(SDV_KEY_RIGHT, SHORT_PRESS);
			//button_dialog_cancel->SetTag(CANCEL_BUTTON);
		}
		break;
		case SDV_KEY_OK:
		{
			db_warn("this is SDV_KEY_OK key code   ");
			button_dialog_->keyProc(SDV_KEY_OK, SHORT_PRESS);
		}
		break;
        default:
            db_msg("this is invild key code");
            break;
    }
}


void BulletCollection::updateProgressbarPercentage(int value)
{
	stringstream ss;
	ss << button_dialog_text_str.c_str() << "("<<value<<"%)";
	//db_msg("updateProgressbarPercentage...   ss.str().c_str() = %d",ss.str().c_str());
       button_dialog_text->SetCaption(ss.str().c_str());
}
void BulletCollection::StopProgressbarTime()
{
	stop_timer(progressbar_timer_id);
	current_data = 0.0;//reset the packet current data
	button_dialog_progress_bar_top_color->SetPosition(30, 122, 0, 4);
}
void BulletCollection::HandleProgressbarTime(union sigval sigval)
{
     int w = 0,mpercentage = 0;
     prctl(PR_SET_NAME, "HandleProgressbarTime", 0, 0, 0);
     BulletCollection *bc = reinterpret_cast<BulletCollection*>(sigval.sival_ptr);
     VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
	vum->getProgressData(&(bc ->total_data),&(bc->current_data));
    mpercentage = (int)(100.0*bc ->current_data/bc ->total_data);
     db_msg("debug_zhb--->HandleProgressbarTime: mpercentage = %d",mpercentage);
    if(100 == mpercentage){
   //bc->total_data = 10.0;
 //  if(bc ->current_data++ == bc->total_data){
		bc ->current_data = 0.0;
		stop_timer(bc ->progressbar_timer_id);
		if(bc->getButtonDialogCurrentId()==BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION){
			bc ->fdownload = true;
		}else if(bc->getButtonDialogCurrentId()==BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION){
			bc ->finstall = true;
			}
		db_msg("[debug_zhb]--------ready to dohide the button_dialog and send message SETTING_BUTTON_DIALOG");
		bc->button_dialog_->DoHide();	
		bc->MsgToSettingWindow(true,1);
		bc ->button_dialog_progress_bar_top_color->SetPosition(30, 122, 0, 4);
		return;
    	}
    w = (int)396*(bc ->current_data/bc ->total_data);
    bc ->updateProgressbarPercentage(mpercentage);
    bc ->button_dialog_progress_bar_top_color->SetPosition(30, 122, w, 4);
}
void BulletCollection::MsgToSettingWindow(bool m_buttonDialog,int val)
{
#ifdef SETTING_WIN_USE
	WindowManager *win_mg = ::WindowManager::GetInstance();
	SettingWindow *sett_win = reinterpret_cast<SettingWindow *>(win_mg->GetWindow(WINDOWID_SETTING));
	sett_win->BcMsgSend(m_buttonDialog,val);
#endif
}
void BulletCollection::buttonDialogFilling(const bc_button_dialog_item &_button_dialog_item_,bool button_right_show,bool progress_bar_show,bool m_full)
{
	if(m_full)
		button_dialog_->SetPosition(0, 0+60*2, GUI_SCN_WIDTH, GUI_SCN_HEIGHT - 60 * 2);
	else
		button_dialog_->SetPosition((GUI_SCN_WIDTH - 460) / 2, (GUI_SCN_HEIGHT - 232) / 2, 460, 232);
	
	std::string button_dialog_str;
	R::get()->GetString(_button_dialog_item_.info_title, button_dialog_str);
    button_dialog_title->SetCaption(button_dialog_str.c_str());
    if(m_full)
        button_dialog_title->SetPosition(0, 0, 640, 50);
    else
        button_dialog_title->SetPosition(0, 0, 460, 30);
    //db_msg("[debug_zhb]--------buttonDialogFilling---info_title = %s",button_dialog_str.c_str());
    button_dialog_text_str.clear();
    R::get()->GetString(_button_dialog_item_.info_text, button_dialog_text_str);
//	button_dialog_text->SetCaption(button_dialog_text_str.c_str());
    button_dialog_text_str = button_dialog_text_str + "\n\n";
//    db_msg("button_dialog_text_str %s",button_dialog_text_str.c_str());
    button_dialog_text->SetCaptionEx(button_dialog_text_str.c_str());
	if(m_full)
		button_dialog_text->SetPosition(50, 50, 540, 134);
	else
		button_dialog_text->SetPosition(30, 70, 400, 110);
      // db_msg("[debug_zhb]--------buttonDialogFilling---info_text = %s",button_dialog_text_str.c_str());
	if(button_right_show){
		R::get()->GetString(_button_dialog_item_.button_right, button_dialog_str);
		button_dialog_cancel->SetCaption(button_dialog_str.c_str());
		button_dialog_cancel->Show();
	}else{
		button_dialog_cancel->Hide();
	}
		
	if(button_right_show){
		button_dialog_confirm->SetPosition(0,176,230,56);
	}else{
		if(m_full)
			button_dialog_confirm->SetPosition(185,184,269,56);
		else
			button_dialog_confirm->SetPosition(0,176,460,56);
	}
	R::get()->GetString(_button_dialog_item_.button_left, button_dialog_str);
	button_dialog_confirm->SetCaption(button_dialog_str.c_str());
	button_dialog_confirm->SetBackColor(0xFF2772DB);
	if(progress_bar_show){
		set_period_timer(1, 0, progressbar_timer_id);
		db_msg("[debug_zhb]---progress_bar_show is true-----buttonDialogFilling");
		button_dialog_progress_bar_background->Show();
		button_dialog_progress_bar_top_color->Show();
	}else{
	       stop_timer(progressbar_timer_id);
		button_dialog_progress_bar_background->Hide();
		button_dialog_progress_bar_top_color->Hide();
	}
}
int BulletCollection::ShowButtonDialog()
{
	db_msg("debug_zhb----ShowButtonDialog----m_button_dialog_flag_ = %d",m_button_dialog_flag_);
	if(m_button_dialog_flag_){
		db_msg("[debug_zhb]------dialog has been show");
		return -1;
	}
    switch(button_dialog_current_id_){
		case BC_BUTTON_DIALOG_REBOOT:
		case BC_BUTTON_DIALOG_REFRESH_NETWORK_TIME:
		case BC_BUTTON_DIALOG_MANUAL_UPDATE_TIME:
		case BC_BUTTON_DIALOG_RESETFACTORY:
        case BC_BUTTON_DIALOG_ACCOUNT_UNBIND:
        case BC_BUTTON_DIALOG_FORMAT_SDCARD:	//button are confirm and cancel , info_text						
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN://button are open_now and cancel ,info_text	
		case BC_BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION:
		case BC_BUTTON_DIALOG_4G_NETWORK_OPEN_WATCHDOG:
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL://button are restart and cancel ,info_text	
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW://button are flow recharge and cancel ,info_text	
		case BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION:
		case BC_BUTTON_DIALOG_DD_NOTICE:
		case BC_BUTTON_DIALOG_DELETE_VIDEO:
		case BC_BUTTON_DIALOG_DELETE_PHOTO:
		case BC_BUTTON_DIALOG_DELETE_ERROR_VIDEO:
		case BC_BUTTON_DIALOG_DELETE_ERROR_PHOTO:
		case BC_BUTTON_DIALOG_DD_TF_FS_ERROR:
		case BC_BUTTON_DIALOG_DELETE_ALLSELECTED:
			{
			  	db_msg("[debug_zhb]--------2 button  , info text");
				buttonDialogFilling(bc_button_dialog_item_[button_dialog_current_id_],true,false);
			}
			break;
		case BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION:	//button are restart and cancel ,info_text	,progressbar			
			{
				db_msg("[debug_zhb]--------2 button  , info text , progressbar,");
				buttonDialogFilling(bc_button_dialog_item_[button_dialog_current_id_],true,true);
			}
			break;
		case BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION://button is cancel ,info_text,progressbar	
			{
				db_msg("[debug_zhb]--------1 button  , info text , progressbar");
				buttonDialogFilling(bc_button_dialog_item_[button_dialog_current_id_],false,true);
			}
		break;
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH://button is I know ,info_text	
		case BC_BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH:
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH:
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL://button is I know ,info_text,info_tilte
		case BC_BUTTON_DIALOG_ADAS_OPEN_FAILE:
		case BC_BUTTON_DIALOG_WATCHDOG_OPEN_FAILE:
		case BC_BUTTON_DIALOG_MORE_IMG:
		case BC_BUTTON_DIALOG_CHECK_MD5_FAILE:
			{
				db_msg("[debug_zhb]--------1 button  , info text , info_title");
				buttonDialogFilling(bc_button_dialog_item_[button_dialog_current_id_],false,false);
			}
			break;
		case BC_BUTTON_DIALOG_DD_NOTICE_FULL:
			{
				db_msg("[debug_zhb]--------1 button  , info text , info_title  full screen");
				buttonDialogFilling(bc_button_dialog_item_[button_dialog_current_id_],false,false,true);
			}
			break;
	default:	
		break;
    	}
    m_button_dialog_flag_ = true;
    ShowButtonDialogSettingButtonStatus(button_dialog_current_id_);
    button_dialog_->DoShow();
     return 0;
}
void BulletCollection::ShowButtonDialogSettingButtonStatus(int current_id)
{
	int status_index = -1;
	switch(current_id){
		case BC_BUTTON_DIALOG_REBOOT://tow button 
		case BC_BUTTON_DIALOG_REFRESH_NETWORK_TIME:
		case BC_BUTTON_DIALOG_MANUAL_UPDATE_TIME:
		case BC_BUTTON_DIALOG_RESETFACTORY:
        case BC_BUTTON_DIALOG_ACCOUNT_UNBIND:
        case BC_BUTTON_DIALOG_FORMAT_SDCARD:
		case BC_BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION:
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN:
		case BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION:
		case BC_BUTTON_DIALOG_DOWNLOAD_PACKET_WATCHDOG:
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL:
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW:
		case MSG_SET_SHOW_RIGHT_LEFT_BUTTON:
			status_index = SET_BUTTON_LEFTRIGHT_SHOW;
			break;
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL://one button
		case BC_BUTTON_DIALOG_ADAS_OPEN_FAILE:
		case BC_BUTTON_DIALOG_WATCHDOG_OPEN_FAILE:
		case BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION:
		case BC_BUTTON_DIALOG_INSTALL_PACKET_WATCHDOG:
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH:
		case BC_BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH:
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH:
		case MSG_SET_HIDE_UP_DWON_BUTTON:
			status_index =SET_BUTTON_UPDOWN_HIDE ;
			break;
		case MSG_SET_SHOW_UP_DWON_BUTTON:
			status_index = SET_BUTTON_UPDOWN_SHOW;
			break;
		default:
			return;
			break;
		}
	db_msg("[debug_zhb]------ShowButtonDialogSettingButtonStatus-------status_index = %d",status_index);
	MsgToSettingWindow(false,status_index);
}

void BulletCollection::BCDoHide()
{
	m_button_dialog_flag_ = false;
	//button_dialog_->DoHide();
	button_dialog_->DoHideDialog();
	
}

int BulletCollection::getButtonDialogCurrentId()
{
	return button_dialog_current_id_;
}

void BulletCollection::setButtonDialogCurrentId(int val)
{
    if(m_button_dialog_flag_){
		db_msg("[debug_zhb---11---dialog has been show]");
		return;
    	}
    button_dialog_current_id_ = val;
}



