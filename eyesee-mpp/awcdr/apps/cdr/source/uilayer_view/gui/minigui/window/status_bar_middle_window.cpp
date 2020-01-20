/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.cpp
 * @brief 状态栏窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-01
 */
//#define NDEBUG

#include "window/playback_window.h"
#include "window/status_bar_middle_window.h"
#include "debug/app_log.h"
#include "common/app_def.h"
//#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "application.h"
#include "bll_presenter/device_setting.h"
#include "device_model/system/power_manager.h"
#include "device_model/menu_config_lua.h"
#include "device_model/storage_manager.h"
#include "device_model/media/media_definition.h"


#include "device_model/system/event_manager.h"
#include "bll_presenter/audioCtrl.h"

#include <sstream>
#include <iomanip>

using namespace std;

IMPLEMENT_DYNCRT_CLASS(StatusBarMiddleWindow)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int StatusBarMiddleWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
    case MSG_PAINT:
        //db_warn("habo---> statusbartop window  MSG_PAINT !!!");
        return HELP_ME_OUT;
	#if 1
	case MSG_LBUTTONDOWN :
	//case MSG_MOUSEMOVE :
		{
			if ((win_status_ == STATU_PREVIEW) || (win_status_ == STATU_PHOTO)) {
				int x = LOWORD(lparam);
			   	int y = HIWORD(lparam);
				db_error("StatusBarMiddleWindow x: %d y: %d",x,y);
				if((y > 60) && (y < 260)){
					WindowManager *win_mg_ = WindowManager::GetInstance();
					#if 1
					PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
					::SendMessage(pw->GetHandle(), message, wparam, lparam);
					db_error("StatusBarMiddleWindow 2222");
					#else					
					PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
					pw->OnOffStatusBar();
					SetActiveWindow(pw->GetHandle());
					#endif
				}
			}
			#if 0
			else if (win_status_ == STATU_PLAYBACK) {
					WindowManager *win_mg_ = WindowManager::GetInstance();
					PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PLAYBACK));
					::SendMessage(pw->GetHandle(), message, wparam, lparam);
			}
			#endif
		}
		//return DO_IT_MYSELF;
		break;
		#endif

		#if 1
		default:
			return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );	
		#endif
    }
    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
   
}

StatusBarMiddleWindow::StatusBarMiddleWindow(IComponent *parent)
    : SystemWindow(parent)
    ,win_status_(STATU_PREVIEW)
    ,hwndel_(HWND_INVALID)
	,m_bg_color(0x00000000)
{
    wname = "StatusBarMiddleWindow";
	db_error("StatusBarMiddleWindow");
    Load();
    R::get()->SetLangID(GetModeConfigIndex(SETTING_DEVICE_LANGUAGE));
    SetBackColor(m_bg_color);  // apha b2
    //SetBackColor(0x001A1E38);  // apha
    //  SetBackColor(0x00ffffff);  // apha
//	GraphicView::LoadImage(GetControl("statusbar_bk"), "statusbar_top_bg_a70");
    //ReturnStatusBarBottomWindowHwnd();
    
    //add by zhb

    time_label = reinterpret_cast<TextView *>(GetControl("time_label"));	// 显示系统时间
    //::SetWindowFont(time_label->GetHandle(),R::get()->GetFontBySize(32));
    // time_label->SetTimeCaption("- - : - -");
    time_label->SetCaptionColor(0xFFFFFFFF);
    //time_label->SetBackColor(0xffbabbc3);  //改变字体背景色后需要同步修改字体控件SetBrushColor(hdc, 0x00000000);
    time_label->SetBackColor(0x00000000);
    //time_label->SetTextStyle(DT_VCENTER|DT_CENTER);

	m_photocountdown_lb 	= reinterpret_cast<TextView *>(GetControl("photocntdn_lb"));
	m_photocountdown_lb->SetCaptionColor(0xFFFFFFFF);
	m_photocountdown_lb->SetBackColor(0x00000000);

	
	m_photocountdown_lb->Hide();
	m_photocountdown_bg = reinterpret_cast<GraphicView *>(GetControl("photocntdn_bg"));
	GraphicView::LoadImage(m_photocountdown_bg, "photocntbg");
	m_photocountdown_bg->Hide();
#if 0
	m_speed_lb 	= reinterpret_cast<TextView *>(GetControl("speed_lb"));
	m_speed_lb->SetCaptionColor(0xFFFFFFFF);
	m_speed_lb->SetBackColor(0x00000000);
	m_speed_lb->Hide();
#endif	
    create_timer(this, &timer_id_data, DateUpdateProc);
    stop_timer(timer_id_data);
    set_period_timer(1, 0, timer_id_data);
	#ifdef PHOTOTIMEUSEMIDDLE
	create_timer(this, &photoing_timer_id_, PhotoingTimerProc);
  	stop_timer(photoing_timer_id_);
	#endif

}

StatusBarMiddleWindow::~StatusBarMiddleWindow()
{
    db_msg("destruct");
	#ifdef PHOTOTIMEUSEMIDDLE
	::delete_timer(photoing_timer_id_);
	#endif
}


void StatusBarMiddleWindow::ReturnStatusBarBottomWindowHwnd()
{
	hwndel_ = GetHandle();
}

HWND StatusBarMiddleWindow::GetSBBWHwnd()
{
	return hwndel_ ;
}

void StatusBarMiddleWindow::DateUpdateProc(union sigval sigval)
{
    char buf[32] = {0};
	char buf2[32] = {0};
    struct tm * tm=NULL;
    time_t timer;
    prctl(PR_SET_NAME, "UpdateDateTime", 0, 0, 0);
	
    timer = time(NULL);
    tm = localtime(&timer);
	//struct timeval tv;
    //struct timezone tz;
    //gettimeofday(&tv, &tz);
	 
    snprintf(buf, sizeof(buf),"%04d-%02d-%02d  %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    StatusBarMiddleWindow *sb = reinterpret_cast<StatusBarMiddleWindow*>(sigval.sival_ptr);
	
    if((sb->win_status_ ==  STATU_PREVIEW) || (sb->win_status_ ==  STATU_PHOTO)) {
        sb->time_label->SetCaption(buf);
		#ifdef S_GPSSPEED
		MenuConfigLua *menuconfiglua = MenuConfigLua::GetInstance();
		EventManager *ev = EyeseeLinux::EventManager::GetInstance();
		if (ev->CheckGpsOnline()) {
			int val = menuconfiglua->GetMenuIndexConfig(SETTING_GPS_SWITCH);
			if (val) {
				EyeseeLinux::LocationInfo_t LocationInfo;
				ev->getLocationInfo(&LocationInfo);
				if (LocationInfo.status == 'A') {
					int ut = menuconfiglua->GetMenuIndexConfig(SETTING_SPEED_UNIT);
					if (ut == 0) {
						float speed = LocationInfo.speed * 1.852;	// -> km/h
						sprintf(buf2,"%.2fKM/H", speed);
						sb->m_speed_lb->SetTimeCaption(buf2);
					} else {
						float speed = LocationInfo.speed * 1.1507794;	// -> mile/h
						sprintf(buf2,"%.2fMPH", speed);
						sb->m_speed_lb->SetTimeCaption(buf2);
					}
				} else {
					sb->m_speed_lb->SetTimeCaption("");
				}
			} else {
				sb->m_speed_lb->SetTimeCaption("");
			}
 		} else {
			
			sb->m_speed_lb->SetTimeCaption("");
		}
		#endif
    }
    else 
	{
        sb->time_label->SetTimeCaption("");
		#ifdef S_GPSSPEED
		sb->m_speed_lb->SetTimeCaption("");
		#endif
    }
	
}

void StatusBarMiddleWindow::DateTimeUiOnoff(int val)
{
	TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
	if (!val) {	
		time_label->Hide();
	} else {
		time_label->Show();
	}
}


#ifdef PHOTOTIMEUSEMIDDLE
void StatusBarMiddleWindow::PhotoStatusTimeUi(int time,int mode)
{
 	db_warn("time = %d\n",time);
  
    PhotoTime = time;
	phoeoMdoe = mode;
	current_photo_time = PhotoTime - 1;
	#if 0
    if(!photo_time_label->GetVisible()) {
         photo_time_label->Show();
    }
	#endif
	if(!m_photocountdown_lb->GetVisible()) {
		char buf[32] = {0};
	    sprintf(buf, "%d",PhotoTime);
		m_photocountdown_lb->SetTimeCaption(buf);
		m_photocountdown_lb->Show();
		m_photocountdown_bg->Show();
	}
    set_period_timer(1, 0, photoing_timer_id_);    
}

void StatusBarMiddleWindow::HidePhotoStatusTimeUi()
{
 	db_warn("HidePhotoStatusTimeUi\n");
	#if 0
    if(photo_time_label->GetVisible())
    {
    	photo_time_label->SetTimeCaption(" ");
        photo_time_label->Hide();
    }
	#endif
	if(m_photocountdown_lb->GetVisible()) {
		m_photocountdown_lb->SetTimeCaption(" ");
		m_photocountdown_lb->Hide();
		m_photocountdown_bg->Hide();
	}
    stop_timer(photoing_timer_id_);    
}
#endif

#ifdef PHOTOTIMEUSEMIDDLE
void StatusBarMiddleWindow::PhotoingTimerProc(union sigval sigval)
{
	
    prctl(PR_SET_NAME, "PhotoingTimer", 0, 0, 0);

    char buf[32] = {0};
    int rec_time = 0;
    StatusBarMiddleWindow *sbw= reinterpret_cast<StatusBarMiddleWindow*>(sigval.sival_ptr);
	//PreviewWindow *pw = reinterpret_cast<PreviewWindow *>(sigval.sival_ptr);

	
	rec_time = sbw->current_photo_time;
	
	sprintf(buf, "%d",rec_time);

	//db_warn("buf is %s",buf);	
	//sbw->photo_time_label->SetTimeCaption(buf);
	sbw->m_photocountdown_lb->SetTimeCaption(buf);
	if(rec_time == 0 )
	{
		if(sbw->phoeoMdoe == MODE_PIC_TIME)
		{
			sbw->m_photocountdown_lb->SetTimeCaption(" ");
			sbw->m_photocountdown_lb->Hide();
			sbw->m_photocountdown_bg->Hide();
			//sbw->photo_time_label->SetTimeCaption(" ");
			//sbw->photo_time_label->Hide();
			stop_timer(sbw->photoing_timer_id_);
		}
		else if(sbw->phoeoMdoe == MODE_PIC_AUTO)
		{
			sbw->current_photo_time = sbw->PhotoTime +1;
		}
		//AudioCtrl::GetInstance()->PlaySound(AudioCtrl::AUTOPHOTO_SOUND);
		sbw->SetPhotoTimeShot();
	} else {
		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
	}

	sbw->current_photo_time-- ;	
}
void StatusBarMiddleWindow::SetPhotoTimeShot()
{
        WindowManager *win_mg = ::WindowManager::GetInstance();
		PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
        pre_win->SetPhotoTimeShot();
}


#endif

void StatusBarMiddleWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string StatusBarMiddleWindow::GetResourceName()
{
    return string(GetClassName());
}

void StatusBarMiddleWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    	
      if (ctrl_name == "time_label" ||
        ctrl_name == "rec_time_label" )
      {
        ctrl->SetCtrlTransparentStyle(false);
        TextView* time_label_ = reinterpret_cast<TextView *>(ctrl);
        time_label_->SetTextStyle(DT_VCENTER|DT_CENTER);
      }
      else
        ctrl->SetCtrlTransparentStyle(true);
}


int StatusBarMiddleWindow::GetModeConfigIndex(int msg)
{
    int index =-1;
    MenuConfigLua *mfl=MenuConfigLua::GetInstance();
    index = mfl->GetMenuIndexConfig(msg);
    return index;
}
void StatusBarMiddleWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d  win_status_ =%d", msg,win_status_);
    StorageManager *sm = StorageManager::GetInstance();
    WindowManager *win_mg = ::WindowManager::GetInstance();
	if(win_mg->GetisWindowChanging())
	{
        db_warn("window is now change not respone cmd");
        return;
    }
    string title_str;
    switch (msg) {
        case MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
        	{
				db_msg("zhb---MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM");
				WindowManager *win_mg = ::WindowManager::GetInstance();
				PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
				int win_statu_save = pre_win->Get_win_statu_save();
				db_error("zhb---MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM win_statu_save: %d",win_statu_save);
				if (win_statu_save == STATU_PREVIEW) {
					//SetStatusPreview();
					
				} else if (win_statu_save == STATU_PHOTO) {
					//SetStatusPhoto();
					
				}
        	}
			break;
		case MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
        case MSG_CHANG_STATU_PREVIEW:
            {
				//SetStatusPreview();
            }break;			
        case MSG_CHANG_STATU_PHOTO:
			{
				//SetStatusPhoto();
			}
            break;
        case MSG_CHANG_STATU_SLOWREC:
            break;
        case MSG_PLAY_TO_PLAYBACK_WINDOW:
        case MSG_CHANG_STATU_PLAYBACK:
            {
				//SetStatusPlayback();
            }break;
        case MSG_RECORD_LOOP_VALUE:
        case MSG_RECORD_TIMELAPSE_VALUE:
		{
			//SetStatusTimelaps();
        }
            break;
        case MSG_PHOTO_TIMED_VALUE:
        case MSG_PHOTO_AUTO_VALUE:
        case MSG_PHOTO_DRAMASHOT_VALUE:

            break;
        case MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR:
        {
			//SetStatusSetting();
        }break;
        case MSG_PLAYBACK_TO_PLAY_WINDOW:
            db_msg("[debug_zhb]----MSG_PLAYBACK_TO_PLAY_WINDOW");
            win_status_ = STATU_PLAYBACK;
            break;
        default:
            break;
    }
}

void StatusBarMiddleWindow::TimeStartStopCtrl(bool flag)
{
    if(!flag){
        TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
        stop_timer(timer_id_data);
        time_label->SetTimeCaption(" ");
    }else{
        TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
        set_period_timer(1, 0, timer_id_data);
    }
}
void StatusBarMiddleWindow::OnLanguageChanged()
{
    //update text view
   // ModeIconHandler(1);
    //ModeIconHandler(0);
}

void StatusBarMiddleWindow::GetIndexStringArray(string array_name,  string &result, int index)
{
	 StringVector title_str1;
	 vector<string>::const_iterator it;

	 R::get()->GetStringArray(array_name, title_str1); 
	 it = title_str1.begin()+index;
	 result =(*it).c_str();
}

void StatusBarMiddleWindow::GetString(string array_name, string &result)
{
    string title_str1;
    R::get()->GetString(array_name, title_str1);
    result = title_str1;
}

int StatusBarMiddleWindow::GetStringArrayIndex(int msg)
{
     int index =-1;
     MenuConfigLua *mfl=MenuConfigLua::GetInstance();
     index = mfl->GetMenuIndexConfig(msg);
     return index;
}



void StatusBarMiddleWindow::SetWinStatus(int status)
{
	win_status_ = status;
}


void StatusBarMiddleWindow::hideStatusBarWindow()
{
    if(this->GetVisible())
        this->Hide();
}

void StatusBarMiddleWindow::showStatusBarWindow()
{
	if(!this->GetVisible())
		this->Show();
}

void StatusBarMiddleWindow::Display_photocountdown(int onoff)
{
	if (!onoff) {
		m_photocountdown_lb->Hide();
		m_photocountdown_bg->Hide();
	} else {
		m_photocountdown_lb->Show();
		m_photocountdown_bg->Show();
	}
}
void StatusBarMiddleWindow::Set_photocountdown_caption(char *text)
{
	m_photocountdown_lb->SetCaption(text);
}



