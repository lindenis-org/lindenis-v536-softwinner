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
#include "window/status_bar_window.h"
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
//#define S_TF_INFO 
//#define S_WIFI_ICON
//#define S_ADAS_ICON

//#define S_GPS_ICON
#define S_MIC_ICON

#define S_AWMD_ICON
#define S_PARK_ICON		// 做停车监控用
#define S_RESOLUTION_ICON
#define S_LOOPREC_ICON
//#define S_GPSSPEED

#ifdef USE_CAMB
#define S_PARK_ICON
#endif

#define BATTERY_DETECT_TIME 1
#define GPS_DETECT_TIME 1
#define NET4G_DETECT_TIME 1 

using namespace std;

IMPLEMENT_DYNCRT_CLASS(StatusBarWindow)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int StatusBarWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
    case MSG_PAINT:
        //db_warn("habo---> statusbartop window  MSG_PAINT !!!");
        return HELP_ME_OUT;
	#if 0
	case MSG_ERASEBKGND:
		{
		// 用于填充窗口背景图片
                HDC hdc = (HDC)wparam;
                const RECT* clip = (const RECT*) lparam;
                BOOL fGetDC = FALSE;
                RECT rcTemp;

                if (hdc == 0) {
                    hdc = GetClientDC (hwnd);
                    fGetDC = TRUE;
                }

                if (clip) {
                    rcTemp = *clip;
                    ScreenToClient (hwnd, &rcTemp.left, &rcTemp.top);
                    ScreenToClient (hwnd, &rcTemp.right, &rcTemp.bottom);
                    IncludeClipRect (hdc, &rcTemp);
                }
                else
                    GetClientRect (hwnd, &rcTemp);

                // 清除无效区域, 如果没有为窗口设置背景图片，则以透明背景填充
                SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0xFF, 0xFF, 0x00));		// RGBA
                FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
				//
				rcTemp.top = 60;
				rcTemp.bottom = rcTemp.top + 60;
				
				db_error("rcTemp.bottom: %d",rcTemp.bottom );
				SetBrushColor (hdc, RGBA2Pixel (hdc, 0x00, 0x00, 0x00, 0x66));		// RGBA
                FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
    
                if (fGetDC)
                    ReleaseDC (hdc);
		}
		break;
	#endif
	#if 0
	case MSG_LBUTTONDOWN :
	case MSG_MOUSEMOVE :
		{
			if ((win_status_ == STATU_PREVIEW) || (win_status_ == STATU_PHOTO)) {
				int x = LOWORD(lparam);
			   	int y = HIWORD(lparam);
				db_error("x: %d y: %d",x,y);
				if((y > 60) && (y < 260)){
					WindowManager *win_mg_ = WindowManager::GetInstance();
					PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
					::SendMessage(pw->GetHandle(), message, wparam, lparam);
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
		break;
		#endif
		
		#if 1
		default:
			return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );	
		#endif
    }
    //return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
   
}

StatusBarWindow::StatusBarWindow(IComponent *parent)
    : SystemWindow(parent)
    ,win_status_(STATU_PREVIEW)
    ,m_current_battery_level(-1)
    ,m_current_gps_signal_level(-1)
    ,hwndel_(HWND_INVALID)
    ,m_time_update_4g(false)
	,record_status_flag_(false)
	,m_RecordTime(0)
	,m_bg_color(0x66000000)
	,RecordMode(0)
	,m_changemode_enable(false)
    ,m_appConnected(false)
{
    wname = "StatusBarWindow";
	db_error("StatusBarWindow");
    Load();
    R::get()->SetLangID(GetModeConfigIndex(SETTING_DEVICE_LANGUAGE));
    SetBackColor(m_bg_color);  // apha b2
    //SetBackColor(0x001A1E38);  // apha
    //  SetBackColor(0x00ffffff);  // apha
//	GraphicView::LoadImage(GetControl("statusbar_bk"), "statusbar_top_bg_a70");
    ReturnStatusBarBottomWindowHwnd();
    RecordStatus = 0;
    initRecordTimeUi();//rec_hit_icon rec_time_label
//    PauseRecordCtrl(true);

#ifdef S_TF_INFO
    //TF init
    TextView* tfcap_label_ = reinterpret_cast<TextView *>(GetControl("tfcap_label"));
    tfcap_label_->SetCaption("");
    tfcap_label_->SetCaptionColor(0xFFFFFFFF);
    tfcap_label_->SetBackColor(m_bg_color);
    create_timer(this, &timer_id_, GetTFCapacity);
    //stop_timer(timer_id_); 
    set_period_timer(1, 0, timer_id_);
#endif

    //add by zhb

    time_label = reinterpret_cast<TextView *>(GetControl("time_label"));	// 显示系统时间
    //::SetWindowFont(time_label->GetHandle(),R::get()->GetFontBySize(32));
    // time_label->SetTimeCaption("- - : - -");
    time_label->SetCaptionColor(0xFFFFFFFF);
    //time_label->SetBackColor(0xffbabbc3);  //改变字体背景色后需要同步修改字体控件SetBrushColor(hdc, 0x00000000);
    time_label->SetBackColor(m_bg_color);
    //time_label->SetTextStyle(DT_VCENTER|DT_CENTER);

	win_status_icon = reinterpret_cast<GraphicView *>(GetControl("status_icon"));
	GraphicView::LoadImage(GetControl("status_icon"), "preview");
	#ifdef PHOTO_MODE
	win_status_icon->Show();
	#ifdef SUPPORT_MODEBUTTON_TOP
	win_status_icon->OnClick.bind(this, &StatusBarWindow::StatusBarWindowProc);
	#endif
	#else
	win_status_icon->Hide();
	#endif	
    s_fileinfo_total = reinterpret_cast<TextView *>(GetControl("fileinfo_total"));
    s_fileinfo_total->SetCaptionColor(0xFFFFFFFF);//0xFF5f6174
    s_fileinfo_total->SetBackColor(0xff000000);
    s_fileinfo_total->Hide();


    m_file_create_time = reinterpret_cast<TextView *>(GetControl("file_create_time_label"));
    m_file_create_time->SetCaptionColor(0xFFFFFFFF);
    m_file_create_time->SetBackColor(0xff000000);
    m_file_create_time->Hide();
/*
	photo_time_label = reinterpret_cast<TextView *>(GetControl("photo_time_label"));
    photo_time_label->SetCaptionColor(0xFFFFFFFF);
    photo_time_label->SetBackColor(m_bg_color);
    photo_time_label->Hide();
*/
#ifdef PHOTOTIMEUSE
	m_photocountdown_lb 	= reinterpret_cast<TextView *>(GetControl("photocntdn_lb"));
	m_photocountdown_lb->SetCaptionColor(0xFFFFFFFF);
	m_photocountdown_lb->SetBackColor(0x00000000);

	
	m_photocountdown_lb->Hide();
	m_photocountdown_bg = reinterpret_cast<GraphicView *>(GetControl("photocntdn_bg"));
	GraphicView::LoadImage(m_photocountdown_bg, "photocntbg");
	m_photocountdown_bg->Hide();
#endif
#if 0
	m_speed_lb 	= reinterpret_cast<TextView *>(GetControl("speed_lb"));
	m_speed_lb->SetCaptionColor(0xFFFFFFFF);
	m_speed_lb->SetBackColor(0x00000000);
	m_speed_lb->Hide();
#endif	
#ifdef DATETIMEUSE
    create_timer(this, &timer_id_data, DateUpdateProc);
    stop_timer(timer_id_data);
    set_period_timer(1, 0, timer_id_data);
#endif
	#ifdef PHOTOTIMEUSE
	create_timer(this, &photoing_timer_id_, PhotoingTimerProc);
  	stop_timer(photoing_timer_id_);
	#endif
    #ifdef S_WIFI_ICON
    WifiIconHander(true);
    #endif
    #ifdef S_ADAS_ICON
    AdasIconHander(true);
    #endif
    SdCardIconHander(true);
	#ifdef S_PARK_ICON
    ParkIconHander(true);
	#endif
	#ifdef S_LOOPREC_ICON
    LooprecIconHander(true);
	#endif
	#ifdef S_AWMD_ICON
	AwmdIconHander(true);
	#endif
    #ifdef S_MIC_ICON
    VoiceIconHandler(true);
	#endif
    #ifdef S_GPS_ICON
    UpdateGpsStatus(false,1);
    #endif
	#ifdef S_RESOLUTION_ICON
	ResolutionIconHander(true);
	#endif
    ThreadCreate(&m_battery_detect_thread_id, NULL, StatusBarWindow::BatteryDetectThread, this);
    #ifdef S_GPS_ICON
    ThreadCreate(&m_gps_signal_thread_id, NULL, StatusBarWindow::GpsSignalDetectThread, this);
    #endif
	RecordStatusTimeUi(0);
	RecordTimeUiOnoff(0);
	#ifndef DATETIMEUSE
	WindowManager *win_mg_ = WindowManager::GetInstance();
	m_sbmiddle = static_cast<StatusBarMiddleWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
	#endif
}

StatusBarWindow::~StatusBarWindow()
{
    db_msg("destruct");
    if( m_battery_detect_thread_id > 0 )
        pthread_cancel(m_battery_detect_thread_id);
    if( m_gps_signal_thread_id > 0 )
            pthread_cancel(m_gps_signal_thread_id);

    ::stop_timer(rechint_timer_id_);
    ::delete_timer(rechint_timer_id_);
	#ifdef DATETIMEUSE
    ::stop_timer(timer_id_data);
    ::delete_timer(timer_id_data);
	#endif
    ::stop_timer(recording_timer_id_);
    ::delete_timer(recording_timer_id_);
	#ifdef PHOTOTIMEUSE
	::delete_timer(photoing_timer_id_);
	#endif
#ifdef S_TF_INFO
    delete_timer(timer_id_);
#endif
}


void StatusBarWindow::ReturnStatusBarBottomWindowHwnd()
{
	hwndel_ = GetHandle();
}

HWND StatusBarWindow::GetSBBWHwnd()
{
	return hwndel_ ;
}

#ifdef DATETIMEUSE
void StatusBarWindow::DateUpdateProc(union sigval sigval)
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

    StatusBarWindow *sb = reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
	PreviewWindow *pw = reinterpret_cast<PreviewWindow *>(sigval.sival_ptr);    // 有问题
	
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
#endif
void StatusBarWindow::initRecordTimeUi()
{
    GraphicView::LoadImage(GetControl("rec_hint_icon"), "rec_hint");
    if(GetControl("rec_hint_icon")->GetVisible())
        GetControl("rec_hint_icon")->Hide();
    m_rec_file_time = reinterpret_cast<TextView *>(GetControl("rec_time_label"));
    m_rec_file_time->SetCaptionColor(0xFFFFFFFF);
    m_rec_file_time->SetBackColor(m_bg_color);
    m_rec_file_time->SetTimeCaption("00:00");
    m_rec_file_time->Hide();
    create_timer(this, &recording_timer_id_,RecordingTimerProc);
    stop_timer(recording_timer_id_);
    create_timer(this, &rechint_timer_id_, RecHintTimerProc);
    stop_timer(rechint_timer_id_);
}
void StatusBarWindow::RecordTimeUiOnoff(int val)
{
	if (!val) {
		m_rec_file_time->Hide();
	} else {
		m_rec_file_time->Show();
	}
	db_warn("RecordTimeUiOnoff: %d", val);
}
#ifdef DATETIMEUSE
void StatusBarWindow::DateTimeUiOnoff(int val)
{
	TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
	if (!val) {	
		time_label->Hide();
	} else {
		time_label->Show();
	}
}
#endif

#ifdef PHOTOTIMEUSE
void StatusBarWindow::PhotoStatusTimeUi(int time,int mode)
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

void StatusBarWindow::HidePhotoStatusTimeUi()
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
void StatusBarWindow::RecordStatusTimeUi(bool mstart)
{
    //MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    //int val = menuconfiglua->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);
    db_error("mstart: %d",mstart);
    {
	    if(mstart)
	    {	// 录像就显示 录像时间
	        m_RecordTime = 0;
	        if(!GetControl("rec_hint_icon")->GetVisible()) {
	            GetControl("rec_hint_icon")->Show();
	        }
	        set_period_timer(1, 0, rechint_timer_id_);
	        if(!m_rec_file_time->GetVisible())
	             m_rec_file_time->Show();
	        set_period_timer(1, 0, recording_timer_id_);
	    }
		else
	    {	// 不录像就显示 剩余容量
	        GetControl("rec_hint_icon")->Hide();
			#if 0
	        m_rec_file_time->SetTimeCaption("");
	        m_rec_file_time->Hide();
			#else
			Uint32 freex, totalx;
	 		if (GetTFCapacity(&freex, &totalx) == 0) {
				char buf[32];
				float cap =(freex/(float)1024);
	     		snprintf(buf, sizeof(buf),"%1.1fG", cap); 
				m_rec_file_time->SetTimeCaption(buf);
	 		} else {
				m_rec_file_time->SetTimeCaption("---G");	
	 		}
			if(!m_rec_file_time->GetVisible())
	             m_rec_file_time->Show();
			#endif
	        stop_timer(rechint_timer_id_);
	        stop_timer(recording_timer_id_);
			system("echo 0 > /sys/class/leds/rec_led/brightness");
	    }
    }
}

void StatusBarWindow::ResetRecordTime()
{
    db_warn("mRecordTime: %d, need reset to 0", m_RecordTime);
	m_RecordTimeSave = m_RecordTime;
    m_RecordTime = 0;
}


int StatusBarWindow::GetCurrentRecordTime()
{
    return m_RecordTime;
}
void StatusBarWindow::RecordingTimerProc(union sigval sigval)//add by zhb
{

    prctl(PR_SET_NAME, "UpdateRecordTime", 0, 0, 0);

    char buf[32] = {0};
    int rec_time = 0;
    StatusBarWindow *sbw= reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
	if (sbw->RecordStatus) {
		rec_time =sbw->m_RecordTime++;
	} else {
		rec_time = sbw->m_RecordTimeSave;
	}
	if (sbw->RecordMode) {	// park mode
		if (rec_time > 15)
			rec_time = 15;
	}
	sprintf(buf, "%02d:%02d",rec_time/60%60, rec_time%60);
	sbw->m_rec_file_time->SetTimeCaption(buf);
	
}
void StatusBarWindow::ChangemodetimerProc(union sigval sigval)//add by zhb
{

    prctl(PR_SET_NAME, "Changemodetime", 0, 0, 0);
    
    StatusBarWindow *sbw= reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
	sbw->m_changemode_enable = true;
	::delete_timer(sbw->changemode_timer_id_);
	db_error("m_changemode_enable set True");
}

#ifdef PHOTOTIMEUSE
void StatusBarWindow::PhotoingTimerProc(union sigval sigval)
{
	
    prctl(PR_SET_NAME, "PhotoingTimer", 0, 0, 0);

    char buf[32] = {0};
    int rec_time = 0;
    StatusBarWindow *sbw= reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
	//PreviewWindow *pw = reinterpret_cast<PreviewWindow *>(sigval.sival_ptr);  // 有问题

	
	rec_time = sbw->current_photo_time;
	
	sprintf(buf, "%d",rec_time);

	db_warn("buf is %s",buf);	
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
		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::AUTOPHOTO_SOUND);
		//pw->SetPhotoTimeShot();   //有问题
	} else {
		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
	}

	sbw->current_photo_time-- ;	
}
#endif

void StatusBarWindow::RecHintTimerProc(union sigval sigval)
{
    prctl(PR_SET_NAME, "UpdateRecHint", 0, 0, 0);
    static bool flag = false;
    
    StatusBarWindow *self = reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
	if (self->RecordStatus) {
	    if (flag) {
	        self->GetControl("rec_hint_icon")->Hide();
			system("echo 0 > /sys/class/leds/rec_led/brightness");
	    }
	    else {
	        self->GetControl("rec_hint_icon")->Show();
			system("echo 1 > /sys/class/leds/rec_led/brightness");
	    }

	    flag = !flag;
	} else {
		self->GetControl("rec_hint_icon")->Hide();
		system("echo 0 > /sys/class/leds/rec_led/brightness");
	}
}

void StatusBarWindow::WinstatusIconHander(int state)
{
	db_error("WinstatusIconHander: %d",state);
#ifndef PHOTO_MODE
	return;
#else
	if (state < 0) {
		win_status_icon->Hide();
	} else {
	/* 
	STATU_PREVIEW = 0,
    STATU_PHOTO,
    STATU_SLOWRECOD,
    STATU_PLAYBACK,
    STATU_SETTING,
    STATU_BINDING,
    STATU_DELAYRECIRD,
    
    */
    	win_status_icon->Hide();
    	switch(state) {
			case STATU_PREVIEW:
				GraphicView::LoadImage(win_status_icon, "preview"); 
				break;
			case STATU_PHOTO:
				GraphicView::LoadImage(win_status_icon, "photo"); 
				break;
			case STATU_SLOWRECOD:
				//GraphicView::LoadImage(win_status_icon, "slowrec"); 
				//break;
			case STATU_PLAYBACK:
				//GraphicView::LoadImage(win_status_icon, "playback"); 
				//break;
			case STATU_SETTING:
			case STATU_BINDING:
			case STATU_DELAYRECIRD:
			default:
				break;
			
    	}
        win_status_icon->Show();
		
	}
#endif
}

void StatusBarWindow::LockIconHander(bool sw)
{

    if(sw)
    {
        GraphicView::LoadImage(GetControl("lock_icon"), "s_top_lock"); 
        GetControl("lock_icon")->Show();
    }
    else
    {
        GetControl("lock_icon")->Hide();
    }
}
void StatusBarWindow::WifiIconHander(bool sw)
{

	int index =-1;
    if(sw)
    {
        index= GetModeConfigIndex(SETTING_WIFI_SWITCH);
        if(index == 1)
            GraphicView::LoadImage(GetControl("wifi_icon"), "s_top_wifi_on"); 
        else
            GraphicView::LoadImage(GetControl("wifi_icon"), "s_top_wifi_off");
        GetControl("wifi_icon")->Show();
    }
    else
    {
        GetControl("wifi_icon")->Hide();
    }
}

void StatusBarWindow::ResolutionIconHander(bool sw)
{

	int index =-1;
    if(sw)
    {
		if (win_status_ == STATU_PREVIEW) {
			index= GetModeConfigIndex(SETTING_RECORD_RESOLUTION);
		
			switch((VideoQuality)index) {
				case VIDEO_QUALITY_4K30FPS:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_4K"); 
					break;
				case VIDEO_QUALITY_2_7K30FPS:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_2K7"); 
					break;
				case VIDEO_QUALITY_1080P120FPS:
				case VIDEO_QUALITY_1080P60FPS:
				case VIDEO_QUALITY_1080P30FPS:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_1080"); 
					break;
				default:
					db_error("unkonw VideoQuality");
					break;
			}
			GetControl("resolation_icon")->Show();
		} else if (win_status_ == STATU_PHOTO) {
			index= GetModeConfigIndex(SETTING_PHOTO_RESOLUTION);
		
			switch((PicResolution)index) {
				case PIC_RESOLUTION_8M:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_8M"); 
					break;
				case PIC_RESOLUTION_13M:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_13M"); 
					break;
				case PIC_RESOLUTION_16M:
					GraphicView::LoadImage(GetControl("resolation_icon"), "res_16M"); 
					break;
				default:
					db_error("unkonw PicResolution");
					break;
			}
			GetControl("resolation_icon")->Show();
		} else {  
        	GetControl("resolation_icon")->Hide();
		}
    }
    else
    {
        GetControl("resolation_icon")->Hide();
    }
}

void StatusBarWindow::LooprecIconHander(bool sw)
{

	int index =-1;
    if(sw)
    {
		if (win_status_ == STATU_PREVIEW) {
			#ifdef SUPPORT_RECTIMELAPS
			index= GetModeConfigIndex(SETTING_RECORD_TIMELAPSE);
			//db_error("index: %d",index);
			if (!index) {	// if RECORD_TIMELAPSE is off, we are in normal looprec
				index= GetModeConfigIndex(SETTING_RECORD_LOOP);
			} else {
				index +=10;		// spec for timelaps
			}
			#else
			index= GetModeConfigIndex(SETTING_RECORD_LOOP);
			#endif
			
			
			switch(index) {
				case 0:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_looprec_1"); 
					break;
				case 1:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_looprec_2"); 
					break;
				case 2:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_looprec_5"); 
					break;
				#ifdef SUPPORT_RECTIMELAPS
				case 11:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_05"); 
					break;
				case 12:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_1"); 
					break;
				case 13:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_2"); 
					break;
				case 14:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_5"); 
					break;
				case 15:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_10"); 
					break;
				case 16:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_30"); 
					break;
				case 17:
					GraphicView::LoadImage(GetControl("looprec_icon"), "s_top_lapsrec_60"); 
					break;
				#endif	
				default:
					db_error("unkonw VideoQuality");
					break;
			}
			GetControl("looprec_icon")->Show();
		} else {  
        	GetControl("looprec_icon")->Hide();
		}
    }
    else
    {
        GetControl("looprec_icon")->Hide();
    }
}

void StatusBarWindow::AdasIconHander(bool sw)
{

	int index =-1;
    if(sw)
    {
        index= GetModeConfigIndex(SETTING_ADAS_SWITCH);
         db_msg("[zhb]:AdasIconHander  index:[%d]\n", index);
        if(index == 1)
            GraphicView::LoadImage(GetControl("adas_icon"), "s_top_adas_on"); //on
        else
            GraphicView::LoadImage(GetControl("adas_icon"), "s_top_adas_off");
        GetControl("adas_icon")->Show();
    }
    else
    {
        GetControl("adas_icon")->Hide();
    }
}

void StatusBarWindow::SdCardIconHander(bool sw)
{
    int index =-1;
    if(sw)
    {
		GetControl("tf_icon")->Hide();
		StorageManager *sm = StorageManager::GetInstance();
        if ((sm->GetStorageStatus() == UMOUNT))
            GraphicView::LoadImage(GetControl("tf_icon"), "s_top_sdcard_off");//off
        else
            GraphicView::LoadImage(GetControl("tf_icon"), "s_top_sdcard_on");
       GetControl("tf_icon")->Show();
        	
    }
    else
    {
        GetControl("tf_icon")->Hide();
    }
}
void StatusBarWindow::ParkIconHander(bool sw)
{

	int index =-1;
    if(sw)
    {
        index= GetModeConfigIndex(SETTING_PARKING_MONITORY);
        db_error("[zhb]:ParkIconHander  index:[%d]\n", index);
        if(index == 1) {
            GraphicView::LoadImage(GetControl("park_icon"), "s_top_park_on"); //on
            GetControl("park_icon")->Show();
        }
        else {
            //GraphicView::LoadImage(GetControl("park_icon"), "s_top_park_off");
			GetControl("park_icon")->Hide();
        }
        
    }
    else
    {
        GetControl("park_icon")->Hide();
    }
}

void StatusBarWindow::AwmdIconHander(bool sw)
{

	int index =-1;
	//MottonDetect = sw;
    if(sw)
    {
        index= GetModeConfigIndex(SETTING_MOTION_DETECT);
        db_error("[zhb]:AwmdIconHander  index:[%d]\n", index);
        if(index == 1) {
            GraphicView::LoadImage(GetControl("awmd_icon"), "s_top_awmd"); //on
            GetControl("awmd_icon")->Show();
        }
        else {
            //GraphicView::LoadImage(GetControl("awmd_icon"), "s_top_park_off");
			GetControl("awmd_icon")->Hide();
        }
        
    }
    else
    {
        GetControl("awmd_icon")->Hide();
    }
}

void StatusBarWindow::VoiceIconHandler(bool sv)
{
#ifdef S_MIC_ICON
    int index =-1;
    if(sv){  //no photo win
        index= GetModeConfigIndex(SETTING_RECORD_VOLUME);
        db_error("[zhb]:VoiceIconHandler  index:[%d]\n", index);
		GetControl("voice_icon")->Hide();
        switch(index){
            case 0:
                GraphicView::LoadImage(GetControl("voice_icon"), "s_top_voice_off");
                GetControl("voice_icon")->Show();
                break;
            case 1:
                GraphicView::LoadImage(GetControl("voice_icon"), "s_top_voice_on");
                GetControl("voice_icon")->Show();
                break;
            default:
                break;
        }
    }else {
        GetControl("voice_icon")->Hide();
    }
#else
    GetControl("voice_icon")->Hide();
#endif
}


void StatusBarWindow::UpdatePlaybackFileInfo(int index,int count,bool flag)
{
    int idx;
    if (count != 0) {
        idx =index + 1;
    } else {
        idx = 0;
    }
    char buf[32]={0};
    snprintf(buf,sizeof(buf),"%d/%d",idx,count);
    if(idx == 0 && count == 0)
        s_fileinfo_total->SetCaption("");
    else
        s_fileinfo_total->SetCaption(buf);
    if(flag == true)
    s_fileinfo_total->Show();
    else
    	s_fileinfo_total->Hide();
}

void StatusBarWindow::UpdatePlaybackFileTime(string &fileTime,bool m_show)
{
    db_warn("[habo]---> fileTime = %s m_show = %d\n",fileTime.c_str(),m_show );
    if(m_show)
    {
        m_file_create_time->SetCaption(fileTime.c_str());
        m_file_create_time->Show();
    }
    else
    {
        m_file_create_time->SetCaption(fileTime.c_str());
        m_file_create_time->Hide();
    }
}


void StatusBarWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string StatusBarWindow::GetResourceName()
{
    return string(GetClassName());
}

void StatusBarWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
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


int StatusBarWindow::GetModeConfigIndex(int msg)
{
    int index =-1;
    MenuConfigLua *mfl=MenuConfigLua::GetInstance();
    index = mfl->GetMenuIndexConfig(msg);
    return index;
}
void StatusBarWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d  win_status_ =%d", msg,win_status_);
    StorageManager *sm = StorageManager::GetInstance();
	WindowManager *win_mg = ::WindowManager::GetInstance();
	
    string title_str;
    switch (msg) {
        case MSG_UPDATED_SYSTEM_TIEM_BY_4G:
            {
                m_time_update_4g = true;
                db_msg("zhb_debug----has been update the system time by 4G network,can start time timer");
                TimeStartStopCtrl(true);
            }
            break;
        case MSG_RECORD_AUDIO_ON:
            VoiceIconHandler(true);
            break;
        case MSG_RECORD_AUDIO_OFF:
            VoiceIconHandler(true);
            break;
        case MSG_HDMI_PLUGIN:
            break;
        case MSG_HDMI_PLUGOUT:
            break;

        case MSG_TVOUT_PLUG_IN:
            break;
        case MSG_TVOUT_PLUG_OUT:
            break;
			
        case MSG_BLUETOOTH_ENABLE:
            break;
        case MSG_BLUETOOTH_DISABLE:
            break;

        case MSG_USB_PLUG_IN:
            break;
        case MSG_USB_PLUG_OUT:
            break;

        case MSG_ETH_DISCONNECT:
            break;
        case MSG_ETH_CONNECT_LAN:
            break;
        case MSG_ETH_CONNECT_INTERNET:
            break;

        case MSG_WIFI_DISABLED:
        case MSG_SOFTAP_DISABLED:
            #ifdef S_WIFI_ICON
            WifiIconHander(true);
            #endif
            break;
        case MSG_SOFTAP_ENABLE:
        case MSG_SOFTAP_ENABLED:
            #ifdef S_WIFI_ICON
            WifiIconHander(true);
            #endif
            break;
        case MSG_WIFI_ENABLE:
        case MSG_WIFI_ENABLED:
        case MSG_WIFI_DISCONNECTED:
        case MSG_WIFI_CONNECTED:
            #ifdef S_WIFI_ICON
            WifiIconHander(true);
            #endif
            break;
        case MSG_STORAGE_MOUNTED: {
			db_error("MSG_STORAGE_MOUNTED");
            SdCardIconHander(true);  
			//PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
			RecordStatusTimeUi(0/*pre_win->GetisRecordStart()*/);
            }
            break;
        case MSG_STORAGE_UMOUNT:
			db_error("MSG_STORAGE_UNMOUNT");
            SdCardIconHander(true);
			RecordStatusTimeUi(0);
            break;
		#if 1
        case MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
		case MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM:
        	{
				db_msg("zhb---MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM");
				PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
				int win_statu_save = pre_win->Get_win_statu_save();
				db_error("zhb---MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM win_statu_save: %d",win_statu_save);
				if (win_statu_save == STATU_PREVIEW) {
					SetStatusPreview();
					
				} else if (win_statu_save == STATU_PHOTO) {
					SetStatusPhoto();
					
				}
        	}
			break;
		#endif
		
        case MSG_CHANG_STATU_PREVIEW:
            {
				#if 0
				db_msg("zhb---MSG_CHANG_STATU_PREVIEW");
				WindowManager *win_mg = ::WindowManager::GetInstance();
				PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
				::SendMessage(pre_win->GetHandle(), MSG_CHANGEWINDOWMODE, STATU_PREVIEW, 0);
                win_status_ = STATU_PREVIEW;
				WinstatusIconHander(STATU_PREVIEW);
                //show
                #ifdef S_WIFI_ICON
                WifiIconHander(true);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(true);
                #endif
                SdCardIconHander(true);
				#ifdef S_PARK_ICON
                ParkIconHander(true);
				#endif
				#ifdef S_LOOPREC_ICON
			    LooprecIconHander(true);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(true);
				#endif
                #ifdef S_MIC_ICON
	            VoiceIconHandler(true);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(true,4);
                #endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(true);
				#endif
                UpdateBatteryStatus(true);

                //start system time timer
                TimeStartStopCtrl(true);
                GetControl("time_label")->Show();
                
                //hide 
                s_fileinfo_total->Hide();
                HidePlaybackBarIcon();
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);  
				#else
				SetStatusPreview();
				#endif
            }break;			
        case MSG_CHANG_STATU_PHOTO:
			{
				#if 0
				printf("top ------- MSG_CHANG_STATU_PHOTO\n");
				 win_status_ = STATU_PHOTO;
				 WinstatusIconHander(STATU_PHOTO);
                //show
                #ifdef S_WIFI_ICON
                WifiIconHander(true);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(true);
                #endif
             //   PreviewIconHander(true);
                SdCardIconHander(true);				
                ParkIconHander(false);
                #ifdef S_MIC_ICON
	            VoiceIconHandler(false);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(false);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(false,4);
                #endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(true);
				#endif
                UpdateBatteryStatus(true);

                //start system time timer
                TimeStartStopCtrl(true);
                GetControl("time_label")->Show();
                
                //hide 
                s_fileinfo_total->Hide();
                HidePlaybackBarIcon();
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);  
				#else
				SetStatusPhoto();
				#endif
			}
            break;
        case MSG_CHANG_STATU_SLOWREC:
            break;
        case MSG_PLAY_TO_PLAYBACK_WINDOW:
        case MSG_CHANG_STATU_PLAYBACK:
            {
				#if 0
				db_msg("zhb---MSG_CHANG_STATU_PLAYBACK");
                win_status_ = STATU_PLAYBACK;
                //hide 
                #ifdef S_WIFI_ICON
                WifiIconHander(false);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(false);
                #endif
                SdCardIconHander(false);
				#ifdef S_PARK_ICON
                ParkIconHander(false);
				#endif
				#ifdef S_LOOPREC_ICON
			    LooprecIconHander(false);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(false);
				#endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(false);
				#endif
                #ifdef S_MIC_ICON
	            VoiceIconHandler(false);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(false,4);
                #endif
                UpdateBatteryStatus(false);

                //stop system time timer
                TimeStartStopCtrl(false);
                GetControl("time_label")->Hide();
            
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);
                //show 
                s_fileinfo_total->Show();
				#else
				SetStatusPlayback();
				#endif
            }break;
        case MSG_RECORD_LOOP_VALUE:
        case MSG_RECORD_TIMELAPSE_VALUE:
		{
			#if 0
			db_warn("[debug_zhb]----MSG_RECORD_TIMELAPSE_VALUE");
         //   win_status_ = STATU_DELAYRECIRD;
            //show
            #ifdef S_WIFI_ICON
            WifiIconHander(true);
            #endif
            #ifdef S_ADAS_ICON
            AdasIconHander(true);
            #endif
       //     PreviewIconHander(true);
            SdCardIconHander(true);				
            ParkIconHander(true);
            #ifdef S_MIC_ICON
            VoiceIconHandler(true);
			#endif
			#ifdef S_AWMD_ICON
			AwmdIconHander(true);
			#endif
			#ifdef S_RESOLUTION_ICON
			ResolutionIconHander(true);
			#endif
            #ifdef S_GPS_ICON
            UpdateGpsStatus(false,4);
            #endif
            UpdateBatteryStatus(true);

            //start system time timer
            TimeStartStopCtrl(true);
            GetControl("time_label")->Show();
            
            //hide 
            s_fileinfo_total->Hide();
            HidePlaybackBarIcon();
            std::string filetime = "";
            UpdatePlaybackFileTime(filetime,false);  		
			#else
			SetStatusTimelaps();
			#endif
        }
            break;
        case MSG_PHOTO_TIMED_VALUE:
        case MSG_PHOTO_AUTO_VALUE:
        case MSG_PHOTO_DRAMASHOT_VALUE:

            break;
        case MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR:
        {
			#if 0
			db_msg("[debug_zhb]----MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR");
            win_status_ = STATU_SETTING;
            //hide 
            #ifdef S_WIFI_ICON
            WifiIconHander(false);
            #endif
            #ifdef S_ADAS_ICON
            AdasIconHander(false);
            #endif
            SdCardIconHander(false);
			#ifdef S_PARK_ICON
            ParkIconHander(false);
			#endif
			#ifdef S_LOOPREC_ICON
		    LooprecIconHander(false);
			#endif
			#ifdef S_AWMD_ICON
			AwmdIconHander(false);
			#endif
			#ifdef S_RESOLUTION_ICON
			ResolutionIconHander(false);
			#endif
			#ifdef S_MIC_ICON
            VoiceIconHandler(false);
			#endif
            #ifdef S_GPS_ICON
            UpdateGpsStatus(false,4);
            #endif
            UpdateBatteryStatus(false);

            //stop system time timer
            TimeStartStopCtrl(false);
            GetControl("time_label")->Hide();

            std::string filetime = "";
            UpdatePlaybackFileTime(filetime,false);
			#else
			SetStatusSetting();
			#endif
        }break;
        case MSG_PLAYBACK_TO_PLAY_WINDOW:
            db_msg("[debug_zhb]----MSG_PLAYBACK_TO_PLAY_WINDOW");
            win_status_ = STATU_PLAYBACK;
            //hide 
            s_fileinfo_total->Hide();
            break;
        case MSG_APP_IS_CONNECTED:
            db_warn("msg is MSG_APP_IS_CONNECTED should update the icon\n");
            m_appConnected = true;
            WifiIconHander(false);
            AppConnectIconHander(true);
            break;
        case MSG_APP_IS_DISCONNECTED:
            db_warn("msg is MSG_APP_IS_DISCONNECTED should update the icon\n");
            m_appConnected = false;
            AppConnectIconHander(false);
            WifiIconHander(true);
            break;
        default:
            break;
    }
}

void StatusBarWindow::AppConnectIconHander(bool value)
{
    if(value)
    {
         //load the app connected icon
         GraphicView::LoadImage(GetControl("wifi_icon"), "s_top_app_connect");
         GetControl("wifi_icon")->Show();
    }else{
         GetControl("wifi_icon")->Hide();
    }
}


void StatusBarWindow::HidePlaybackBarIcon()
{
	if(record_status_flag_ == true){
		if(!GetControl("rec_hint_icon")->GetVisible())
			GetControl("rec_hint_icon")->Show();
		set_period_timer(1, 0, rechint_timer_id_);
	} else {

		stop_timer(rechint_timer_id_);
	}
}

void StatusBarWindow::TimeStartStopCtrl(bool flag)
{
#ifdef DATETIMEUSE
    if(flag && m_time_update_4g){
        set_period_timer(1, 0,timer_id_data );
    }else if(!flag && m_time_update_4g){
        TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
        stop_timer(timer_id_data);
        time_label->SetTimeCaption(" ");
    }else{
        TextView* time_label = reinterpret_cast<TextView *>(GetControl("time_label"));
        set_period_timer(1, 0, timer_id_data);
    }
#else
	// 
#endif
}

void StatusBarWindow::OnLanguageChanged()
{
    //update text view
   // ModeIconHandler(1);
    //ModeIconHandler(0);
}

void StatusBarWindow::GetIndexStringArray(string array_name,  string &result, int index)
{
	 StringVector title_str1;
	 vector<string>::const_iterator it;

	 R::get()->GetStringArray(array_name, title_str1); 
	 it = title_str1.begin()+index;
	 result =(*it).c_str();
}

void StatusBarWindow::GetString(string array_name, string &result)
{
    string title_str1;
    R::get()->GetString(array_name, title_str1);
    result = title_str1;
}

int StatusBarWindow::GetStringArrayIndex(int msg)
{
     int index =-1;
     MenuConfigLua *mfl=MenuConfigLua::GetInstance();
     index = mfl->GetMenuIndexConfig(msg);
     return index;
}

void StatusBarWindow::GetTFCapacity(union sigval sigval)
{
    static bool flag_hide = true;
    uint32_t free, total;
    float cap=0.0;
    char buf[32] = {0};	
    StorageManager *sm = StorageManager::GetInstance();
    StatusBarWindow *sbw = reinterpret_cast<StatusBarWindow*>(sigval.sival_ptr);
    if (!(sm->GetStorageStatus() != UMOUNT)) //umount
    {
        //sbw->GetControl("tfcap_label")->SetCaption(""); 
        if(sbw->GetControl("tfcap_label")->GetVisible())
        {
            sbw->GetControl("tfcap_label")->SetCaption(""); 
            sbw->GetControl("tfcap_label")->Hide();
        }
        return;
    }
    sm->GetStorageCapacity(&free, &total);
    cap =(free/(float)1024);
    //db_msg("[fangjj]:GetTFCapacity TF MOUNT free=:[%02d] total=:[%02d] cap[%02.2f]\n",free,total,cap);	 	  
     snprintf(buf, sizeof(buf),"%1.1fG", cap);
     if(sbw->win_status_ == STATU_PREVIEW || sbw->win_status_ == STATU_PHOTO ||sbw->win_status_ == STATU_SLOWRECOD)
     {
        if(!sbw->GetControl("tfcap_label")->GetVisible())
            sbw->GetControl("tfcap_label")->Show();
     }

     sbw->GetControl("tfcap_label")->SetCaption(buf); 
}

int StatusBarWindow::GetTFCapacity(uint32_t *free, uint32_t *total)
{
    uint32_t freet, totalt;
    StorageManager *sm = StorageManager::GetInstance();
    if (!(sm->GetStorageStatus() != UMOUNT)) //umount
    {
        return -1;
    }
    sm->GetStorageCapacity(&freet, &totalt);
	*free = freet;
	*total = totalt;
	return 0;
}
#if 0
void StatusBarWindow::UpdateGpsInfo(void* data)
{
	//EventManager::GetInstance()->getLocationInfoEx(data);
	//GpsSwitch = MenuConfigLua::GetInstance()->GetGpsSwith();
	//GpsSpeedUnit = MenuConfigLua::GetInstance()->GetSpeedunit();
}
#endif
void StatusBarWindow::UpdateGpsStatus(bool sg,int index)
{
    db_error("[zhb]:gpsIconHandler  show: %d index:[%d]\n", sg, index);
	GetControl("gps_icon")->Hide();
	if (index != EventManager::GetInstance()->GetGpsSignalLevel())
		index = EventManager::GetInstance()->GetGpsSignalLevel();
    switch(index){
        case 0:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level0");
            break;
        case 1:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level1");
            break;
        case 2:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level2");
            break;
        case 3:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level3");
            break;
        default:
            break;
    }
    if(sg) {
		if (EventManager::GetInstance()->CheckGpsOnline())
        	GetControl("gps_icon")->Show();	
		else
			GetControl("gps_icon")->Hide();
    }
    else
        GetControl("gps_icon")->Hide();

}

void StatusBarWindow::UpdateGpsStatusEx(bool sg,int index, int flash)
{
    //db_error("[zhb]:gpsIconHandler  show: %d index:[%d]\n", sg, index);
    
    switch(index){
        case 0:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level0");
            break;
        case 1:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level1");
            break;
        case 2:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level2");
            break;
        case 3:
            GraphicView::LoadImage(GetControl("gps_icon"), "s_top_gps_level3");
            break;
        default:
            break;
    }
    if(sg) {
		GetControl("gps_icon")->Hide();
       	GetControl("gps_icon")->Show();	
    }
    else
        GetControl("gps_icon")->Hide();
	

}

void StatusBarWindow::UpdateBatteryStatus(bool show)
{
	
	if (show) {
		GetControl("battery_icon")->Hide();
		UpdateBatteryStatus(true, m_current_battery_level);
	} else {
		UpdateBatteryStatus(false, m_current_battery_level);
	}
}

void StatusBarWindow::UpdateBatteryStatus(bool sb,int levelval)
{
    switch (levelval)
    {
        case 0:
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_battery0");	// 0格
            break;
        case 1:
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_battery1");	// 1格
            break;
        case 2:
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_battery2");	// 2格
            break;
        case 3:
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_battery3");	// 3格
            break;
        case 4:
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_charging");	// DC充电
            break;
        case 5: 
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_usb_charging_only");	// USB充电
            break;
		case 6: 
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_battery4");	// 4格 满格
            break;
		case 7: 
            GraphicView::LoadImage(GetControl("battery_icon"), "s_top_chargfull");	// DC充满
            break;
        default:
            break;
    }
    if(sb)
		GetControl("battery_icon")->Show();
    else
        GetControl("battery_icon")->Hide();
	GetControl("battery_icon")->Refresh();
}


void StatusBarWindow::SetWinStatus(int status)
{
	win_status_ = status;
}
void* StatusBarWindow::BatteryDetectThread(void *context)
{
    int battery_cap_level = -1;
    int batteryStatus;
    StatusBarWindow *p_statusBar = reinterpret_cast<StatusBarWindow*>(context);
    PowerManager *pm = PowerManager::GetInstance();
    WindowManager *win_mg = ::WindowManager::GetInstance();
    while(1)
    {
        #if 0
        batteryStatus = pm->GetBatteryStatus();
        if ((pm->getACconnectStatus() == 1) && (batteryStatus > 0)) {//Acc on and battery on
            battery_cap_level = 4;
        } else if ((pm->getACconnectStatus() == 1) && (batteryStatus < 0)){//˵?????ز????ڻ??????ػ?????
            battery_cap_level = 5;
        } 
        #endif
        if (pm->getACconnectStatus() == 1 && !PowerManager::GetInstance()->getUsbconnectStatus())
        {
       		 battery_cap_level = pm->GetBatteryLevel();
			 //db_error("zhb---111battery_cap_level = %d",battery_cap_level);
			 if(battery_cap_level != 6){
				battery_cap_level = 4;	// charging
			 }else{
				//battery_cap_level = 7;	// charge full 
			 }
        }
        else if(pm->getACconnectStatus() == 1 && PowerManager::GetInstance()->getUsbconnectStatus())//connect the pc
        {
        	 battery_cap_level = pm->GetBatteryLevel();
			 //db_error("zhb---222battery_cap_level = %d",battery_cap_level);
			 if(battery_cap_level != 6){
				battery_cap_level = 5;	// usb charging
			 }
        }
        else 
        {
            battery_cap_level = pm->GetBatteryLevel();
        }
		//db_error("m_current_battery_level: %d battery_cap_level :%d",p_statusBar->m_current_battery_level,battery_cap_level);
        if(p_statusBar->m_current_battery_level != battery_cap_level){
            p_statusBar->m_current_battery_level = battery_cap_level;
            if((win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW) || (win_mg ->GetCurrentWinID() ==  WINDOWID_INVALID)){
                p_statusBar->UpdateBatteryStatus(true,battery_cap_level);
            }else{
                p_statusBar->UpdateBatteryStatus(false,battery_cap_level);
            }
        }
	 
        sleep(BATTERY_DETECT_TIME);
    }
    return NULL;  
}

void* StatusBarWindow::GpsSignalDetectThread(void * context)
{
    int gps_signal_level = -1;
    StatusBarWindow *p_statusBar = reinterpret_cast<StatusBarWindow*>(context);
    EventManager *even_ = EventManager::GetInstance();
    WindowManager *win_mg = ::WindowManager::GetInstance();
	MenuConfigLua *mn = MenuConfigLua::GetInstance();
	int flashflag = 0;
    while(1)
    {
        gps_signal_level = even_->GetGpsSignalLevel();
		if ((gps_signal_level == 0)) {
			if (mn->GetGpsSwith() && even_->CheckGpsOnline()) {
				if (win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW) {
					if (flashflag & 1)
						p_statusBar->UpdateGpsStatusEx(false,-1/*gps_signal_level*/);
					else
						p_statusBar->UpdateGpsStatusEx(true,-1/*gps_signal_level*/);
					flashflag++;
				} else {
					p_statusBar->UpdateGpsStatusEx(false,gps_signal_level);
				}
			} else {
				if (win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW) {
					if(even_->CheckGpsOnline())
						p_statusBar->UpdateGpsStatusEx(true,gps_signal_level);
					else
						p_statusBar->UpdateGpsStatusEx(false,gps_signal_level);
				} else {
					p_statusBar->UpdateGpsStatusEx(false,gps_signal_level);
				}
			}
		} 
		{
	        //db_warn("[debug_jason]: GpsDetectThread: gps_signal_level = %d ,p_statusBar->m_current_gps_signal_level = %d",gps_signal_level,p_statusBar->m_current_gps_signal_level);
	        if(p_statusBar->m_current_gps_signal_level != gps_signal_level){
	            p_statusBar->m_current_gps_signal_level=gps_signal_level;
	            if((win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW) || (win_mg ->GetCurrentWinID() ==  WINDOWID_INVALID) || (win_mg ->GetCurrentWinID() ==  WINDOWID_BINDING)){
	                p_statusBar->UpdateGpsStatusEx(true,gps_signal_level);
	            }else{
	                p_statusBar->UpdateGpsStatusEx(false,gps_signal_level);
	            }
	        }
		}
        sleep(GPS_DETECT_TIME);
    }
    return NULL;  
}

void StatusBarWindow::hideStatusBarWindow()
{
	//if (this->m_sbmiddle->GetVisible())
	//	this->m_sbmiddle->Hide();
	this->m_sbmiddle->DateTimeUiOnoff(0);
	if(this->GetVisible())
        this->Hide();
}

void StatusBarWindow::showStatusBarWindow()
{
	if (/*!this->m_sbmiddle->GetVisible()*/1) {
		this->m_sbmiddle->Show();
		this->m_sbmiddle->DateTimeUiOnoff(1);
	}
	if(!this->GetVisible())
		this->Show();
}

void StatusBarWindow::StatusBarWindowProc(View *control)
{
#ifdef PHOTO_MODE
    #ifdef SUPPORT_MODEBUTTON_TOP
	WindowManager *win_mg = ::WindowManager::GetInstance();
	//if (m_changemode_enable) {	// 允许切换模式?
		if(win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW) {
			PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
			int curstatus = pre_win->GetWindowStatus();
			db_error("========click button change window status(old): %d",curstatus);
			if (curstatus == STATU_PREVIEW) {
				curstatus = STATU_PHOTO;
			} else {
				curstatus = STATU_PREVIEW;
			}
			::SendMessage(pre_win->GetHandle(), MSG_CHANGEWINDOWMODE, curstatus, 0);
		}
	//}
	#endif
#endif
}
#ifdef PHOTOTIMEUSE
void StatusBarWindow::Display_photocountdown(int onoff)
{
	if (!onoff) {
		m_photocountdown_lb->Hide();
		m_photocountdown_bg->Hide();
	} else {
		m_photocountdown_lb->Show();
		m_photocountdown_bg->Show();
	}
}
void StatusBarWindow::Set_photocountdown_caption(char *text)
{
	m_photocountdown_lb->SetCaption(text);
}
#endif
void StatusBarWindow::SetStatusPreview()
{
	db_error("zhb---MSG_CHANG_STATU_PREVIEW");
	WindowManager *win_mg = ::WindowManager::GetInstance();
	PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
//	::SendMessage(pre_win->GetHandle(), MSG_CHANGEWINDOWMODE, STATU_PREVIEW, 0);
	
     win_status_ = STATU_PREVIEW;
				WinstatusIconHander(STATU_PREVIEW);
                //show
                #ifdef S_WIFI_ICON
                WifiIconHander(true);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(true);
                #endif
                SdCardIconHander(true);
				#ifdef S_PARK_ICON
                ParkIconHander(true);
				#endif
				#ifdef S_LOOPREC_ICON
			    LooprecIconHander(true);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(true);
				#endif
                #ifdef S_MIC_ICON
	            VoiceIconHandler(true);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(true,4);
                #endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(true);
				#endif
                UpdateBatteryStatus(true);
				#ifdef DATETIMEUSE
                //start system time timer
                TimeStartStopCtrl(true);
                GetControl("time_label")->Show();
				#else
				m_sbmiddle->Show();
                m_sbmiddle->TimeStartStopCtrl(true);
                #endif
                //hide 
                s_fileinfo_total->Hide();
                HidePlaybackBarIcon();
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);  
				//RecordTimeUiOnoff(true);		// 屏蔽起来居然不会显示显示底色了?
				
}
void StatusBarWindow::SetStatusPhoto()
{
	db_error("top ------- MSG_CHANG_STATU_PHOTO\n");
				 win_status_ = STATU_PHOTO;
				 WinstatusIconHander(STATU_PHOTO);
                //show
                #ifdef S_WIFI_ICON
                WifiIconHander(true);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(true);
                #endif
             //   PreviewIconHander(true);
                SdCardIconHander(true);				
                #ifdef S_PARK_ICON
                ParkIconHander(false);
				#endif
                #ifdef S_MIC_ICON
	            VoiceIconHandler(false);
				#endif
				#ifdef S_LOOPREC_ICON
			    LooprecIconHander(false);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(false);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(false,4);
                #endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(true);
				#endif
                UpdateBatteryStatus(true);
				#ifdef DATETIMEUSE
                //start system time timer
                TimeStartStopCtrl(true);
                GetControl("time_label")->Show();
                #else
				m_sbmiddle->Show();
                m_sbmiddle->TimeStartStopCtrl(true);
				#endif
                //hide 
                s_fileinfo_total->Hide();
                HidePlaybackBarIcon();
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);  

				//RecordTimeUiOnoff(true);
				
}
void StatusBarWindow::SetStatusPlayback()
{
db_error("zhb---MSG_CHANG_STATU_PLAYBACK");
                win_status_ = STATU_PLAYBACK;
                //hide 
                #ifdef S_WIFI_ICON
                WifiIconHander(false);
                #endif
                #ifdef S_ADAS_ICON
                AdasIconHander(false);
                #endif
                SdCardIconHander(false);
				#ifdef S_PARK_ICON
                ParkIconHander(false);
				#endif
				#ifdef S_LOOPREC_ICON
			    LooprecIconHander(false);
				#endif
				#ifdef S_AWMD_ICON
				AwmdIconHander(false);
				#endif
				#ifdef S_RESOLUTION_ICON
				ResolutionIconHander(false);
				#endif
                #ifdef S_MIC_ICON
	            VoiceIconHandler(false);
				#endif
                #ifdef S_GPS_ICON
                UpdateGpsStatus(false,4);
                #endif
                UpdateBatteryStatus(false);
				#ifdef DATETIMEUSE
                //stop system time timer
                TimeStartStopCtrl(false);
                GetControl("time_label")->Hide();
            	#else
				m_sbmiddle->Hide();
                m_sbmiddle->TimeStartStopCtrl(false);
				#endif
                std::string filetime = "";
                UpdatePlaybackFileTime(filetime,false);
                //show 
                s_fileinfo_total->Show();

				RecordTimeUiOnoff(false);
}
void StatusBarWindow::SetStatusTimelaps()
{
db_error("[debug_zhb]----MSG_RECORD_TIMELAPSE_VALUE");
         //   win_status_ = STATU_DELAYRECIRD;
            //show
            #ifdef S_WIFI_ICON
            WifiIconHander(true);
            #endif
            #ifdef S_ADAS_ICON
            AdasIconHander(true);
            #endif
       //     PreviewIconHander(true);
            SdCardIconHander(true);				
            ParkIconHander(true);
            #ifdef S_MIC_ICON
            VoiceIconHandler(true);
			#endif
			#ifdef S_LOOPREC_ICON
			LooprecIconHander(false);
			#endif
			#ifdef S_AWMD_ICON
			AwmdIconHander(true);
			#endif
			#ifdef S_RESOLUTION_ICON
			ResolutionIconHander(true);
			#endif
            #ifdef S_GPS_ICON
            UpdateGpsStatus(false,4);
            #endif
            UpdateBatteryStatus(true);

            #ifdef DATETIMEUSE
            //start system time timer
            TimeStartStopCtrl(true);
            GetControl("time_label")->Show();
            #else
			m_sbmiddle->Show();
            m_sbmiddle->TimeStartStopCtrl(true);
			#endif
            
            //hide 
            s_fileinfo_total->Hide();
            HidePlaybackBarIcon();
            std::string filetime = "";
            UpdatePlaybackFileTime(filetime,false); 

			RecordTimeUiOnoff(true);
}

void StatusBarWindow::SetStatusSetting()
{

db_error("[debug_zhb]----MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR");
            win_status_ = STATU_SETTING;
            //hide 
            #ifdef S_WIFI_ICON
            WifiIconHander(false);
            #endif
            #ifdef S_ADAS_ICON
            AdasIconHander(false);
            #endif
            SdCardIconHander(false);
			#ifdef S_PARK_ICON
            ParkIconHander(false);
			#endif
			#ifdef S_LOOPREC_ICON
		    LooprecIconHander(false);
			#endif
			#ifdef S_AWMD_ICON
			AwmdIconHander(false);
			#endif
			#ifdef S_RESOLUTION_ICON
			ResolutionIconHander(false);
			#endif
			#ifdef S_MIC_ICON
            VoiceIconHandler(false);
			#endif
            #ifdef S_GPS_ICON
            UpdateGpsStatus(false,4);
            #endif
            UpdateBatteryStatus(false);

            #ifdef DATETIMEUSE
            //stop system time timer
            TimeStartStopCtrl(false);
            GetControl("time_label")->Hide();
            #else
			m_sbmiddle->Hide();
            m_sbmiddle->TimeStartStopCtrl(false);
			#endif

            std::string filetime = "";
            UpdatePlaybackFileTime(filetime,false);

			RecordTimeUiOnoff(false);
}

void StatusBarWindow::SetStatusRecordMode(int val)
{
	RecordMode = val;
}

