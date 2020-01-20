/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file device_setting.cpp
 * @brief 设置界面presenter
 * @author id:826
 * @version v0.3
 * @date 2016-09-19
 */
//#define NDEBUG
#include <time.h>
#include "bll_presenter/device_setting.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/autoshutdown.h"
#include "bll_presenter/audioCtrl.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/storage_manager.h"
#include "window/user_msg.h"
#include "common/setting_menu_id.h"
#include "uilayer_view/gui/minigui/window/status_bar_window.h"
#include "uilayer_view/gui/minigui/window/status_bar_bottom_window.h"
#include "uilayer_view/gui/minigui/window/preview_window.h"
#include "uilayer_view/gui/minigui/window/prompt.h"
#include "common/app_log.h"
#include "application.h"
#include "lua/lua_config_parser.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/recorder/recorder.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/media/recorder/recorder.h"
#include "device_model/media/recorder/recorder_factory.h"
#include "minigui-cpp/resource/resource_manager.h"
#include "device_model/menu_config_lua.h"
#include "window/setting_handler_window.h"
#include "device_model/system/led.h"
#include "device_model/media/osd_manager.h"
#include "window/newSettingWindow.h"
#include "uilayer_view/gui/minigui/window/promptBox.h"
#include "window/levelbar.h"
#include "window/setting_window.h"
#include "device_model/system/power_manager.h"
#include "device_model/system/event_manager.h"
#include "window/bulletCollection.h"
#include "device_model/system/gsensor_manager.h"
#include "device_model/dialog_status_manager.h"
#include "device_model/system/watchdog.h"
#include "dd_serv/common_define.h"
#include "device_model/version_update_manager.h"
#include "bll_presenter/statusbarsaver.h"


#undef LOG_TAG
#define LOG_TAG "DeviceSetting"

using namespace EyeseeLinux;
using namespace std;

DeviceSettingPresenter::DeviceSettingPresenter(MainModule *mm)
    : status_(MODEL_UNINIT)
    , win_mg_(::WindowManager::GetInstance())
    , net_manager_(NetManager::GetInstance())
    , softap_on_(false)
    , isStreamRecodFlag(false)
{
    pthread_mutex_init(&model_lock_, NULL);
    StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    this->Attach(status_bar);
    StatusBarBottomWindow *status_bottom_bar = static_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Attach(status_bottom_bar);
	//StorageManager::GetInstance()->Attach(this);
    mainModule_ = mm;

}

DeviceSettingPresenter::~DeviceSettingPresenter()
{
    StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    this->Detach(status_bar);
    StatusBarBottomWindow *status_bottom_bar = static_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Detach(status_bottom_bar);
}

void DeviceSettingPresenter::OnWindowLoaded()
{
    db_msg("window load");

    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }

    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void DeviceSettingPresenter::OnWindowDetached()
{
    db_msg("window detach");

    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = true;

    if (status_ == MODEL_INITED) {
        this->DeviceModelDeInit();
    }
}

int DeviceSettingPresenter::WifiSwitch(int val)
{
    PreviewWindow *pre_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    if( val )
        pre_win->Update((MSG_TYPE)MSG_SET_WIFI_ON);
    else
        pre_win->Update((MSG_TYPE)MSG_SET_WIFI_OFF);

    return 0;
}

int DeviceSettingPresenter::SoftAPSwitch(int val)
{
    return 0;
}

static int GetBeepToneConfig(int idx)
{
    int beep_map[] {100, 83, 60};
    return beep_map[idx];
}

int DeviceSettingPresenter::FormatStorage()
{
	//SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
	this->Notify((MSG_TYPE)MSG_FORMAT_START);
    int ret = StorageManager::GetInstance()->Format();
    if( ret != 0 )
    {
        db_msg("format sd card failed\n");
    }
    else
    {
        db_msg("format sd card success\n");
    }

    sleep(1);
    this->Notify((MSG_TYPE)MSG_FORMAT_FINISH);
    return 0;
}

int DeviceSettingPresenter::GetDeviceInfo(int msgid)
{
    return 0;
}

int DeviceSettingPresenter::GetWifiInfo(int msgid)
{
   vector<string> wifi_info;
    string info,title_str,str_ssid,str_passw;
    NetManager::GetInstance()->GetWifiInfo(str_ssid, str_passw);


    ::LuaConfig config;
    config.LoadFromFile("/data/menu_config.lua");

    R::get()->GetString("ml_camera_wifiswitch_ssid", title_str);
    info = title_str;
    info += str_ssid;
    wifi_info.push_back(info);

    R::get()->GetString("ml_camera_wifiswitch_passwd", title_str);
    info = title_str;
    info += str_passw;
    wifi_info.push_back(info);

    R::get()->GetString("ml_wifi_connect_info", title_str);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowInfoDialog(wifi_info,title_str,msgid);
#endif

    return 0;
}
int DeviceSettingPresenter::GetWifiAppDownloadQRcode(int msgid)
{
   db_msg("[debug_zhb]----GetWifiAppDownloadQRcode------");
   vector<string> wifi_app_download;
    string info,title_str;


    R::get()->GetString("ml_wifi_app_download_qr_code", title_str);
    info = title_str;
    wifi_app_download.push_back(info);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowInfoDialog(wifi_app_download,"",msgid);
#endif

    return 0;
}

int DeviceSettingPresenter::GetTrafficInfo(int msgid)
{
   db_msg("[debug_zhb]----GetTrafficInfo------");
   vector<string> traffic_info;
    string info,title_str;

    R::get()->GetString("ml_traffic_surplus", title_str);
    info = title_str;
    traffic_info.push_back(info);

    R::get()->GetString("ml_traffic_total", title_str);
    info = title_str;
    traffic_info.push_back(info);

    R::get()->GetString("ml_traffic_info_query", title_str);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowInfoDialog(traffic_info,title_str,msgid);
#endif

    return 0;
}
int DeviceSettingPresenter::GetFlowRechargeQRcode(int msgid)
{
   db_msg("[debug_zhb]----GetFlowRechargeQRcode------");
   vector<string> wifi_info;
    string info,title_str;

    R::get()->GetString("ml_scan_qr_code_recharge", title_str);
    info = title_str;
    wifi_info.push_back(info);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowInfoDialog(wifi_info,"",msgid);
#endif

    return 0;
}
int DeviceSettingPresenter::GetSimInfo(int msgid)
{
   db_msg("[debug_zhb]----GetSimInfo------");
   vector<string> wifi_info;
    string info,title_str;

    R::get()->GetString("ml_sim_card_id", title_str);
    info = title_str;
    wifi_info.push_back(info);

    R::get()->GetString("ml_sim_network_business", title_str);
    info = title_str;
    wifi_info.push_back(info);

    R::get()->GetString("ml_sim_info_query", title_str);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowInfoDialog(wifi_info,title_str,msgid);
#endif
    return 0;
}

int DeviceSettingPresenter::GetSubMenuData(int index)
{
    int hieght = 0;
    vector<string> submenu_info;
    string info;
    submenu_info.clear();
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    hieght = (index-USER_MSG_BASE-1);
    db_warn(" [fangjj]:----GetSubMenuData->index =: %d hieght =: %d \n",index,hieght);
    win->SetIndexToSubmenu(index);
    win->MenuToSubmenuSettingButtonStatus(index);
#endif
    //win->ShowSubmenu(hieght,submenu_info);
    return 0;
}
void DeviceSettingPresenter::ShowLevelBar(int levelbar_id)
{
   db_msg("[debug_zhb]----ShowLevelBar------");
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->ShowLevelBar(levelbar_id);
#endif
}

Recorder* DeviceSettingPresenter::getRecorder(int p_CamId, int p_recorder_id)
{
    m_CamRecMap = mainModule_->getRecoderMap();
    if(m_CamRecMap.size() == 0){
        db_error("[debug_joson]: camera rec map is empty\n");
        return NULL;
    }

    CamRecMap::iterator cam_rec_iter;
    cam_rec_iter = m_CamRecMap.find(p_CamId);
    if(cam_rec_iter == m_CamRecMap.end())
    {
        db_error("[debug_joson]: first can't find the recoder\n");
        return NULL;
    }

	std::map<int, Recorder *>::iterator rec_iter;
	for(rec_iter = m_CamRecMap[p_CamId].begin(); rec_iter != m_CamRecMap[p_CamId].end(); rec_iter++)
	{	
		if( p_recorder_id != rec_iter->first)
			continue;

		return rec_iter->second;
	}

    return NULL;
}

Camera* DeviceSettingPresenter::getCamera(int p_CamId)
{
    m_CamMap = mainModule_->getCamerMap();
    if(m_CamMap.size() == 0)
    {
        db_error("[debug_joson]: camera_map size is empty\n");
        return NULL;
    }

    CameraMap::iterator cam_iter;
    cam_iter = m_CamMap.find(p_CamId);
    if(cam_iter == m_CamMap.end())
    {
        db_error("[debug_joson]: realy can't find the camera\n");
        return NULL;
    }

    return cam_iter->second;
}
void DeviceSettingPresenter::VideoRecordDetect(bool start_)
{
	PreviewWindow *pre_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	pre_win->VideoRecordDetect(start_);
}
void DeviceSettingPresenter::SetVideoResoulation(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetVideoResoulation p_CamId %d val = %d####",p_CamId, val);
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL) {
		db_error("cam is NULL !!!");
        return;
    //if stream record is start should stop frist
    }
    Recorder *rec0;
    rec0 = getRecorder(0,1);
    if(rec0 != NULL && rec0->RecorderIsBusy())
    {
        db_error("the streamRecord is start should stop first\n");
        isStreamRecodFlag = true; 
        rec0->StopRecord();
        usleep(200*1000);
    }
    
    cam->SetVideoResolution(val);

    if(rec0 != NULL && isStreamRecodFlag)
    {   
        usleep(200*1000);     
        db_error("the streamRecord is stop should start first\n");
        isStreamRecodFlag = false;
        rec0->StartRecord();
    }

}

void DeviceSettingPresenter::SetRearVideoResoulation(int p_CamId, int val)
{
    db_msg("[debug_zhb]:####SetRearVideoResoulation val = %d####",val);
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL)
        return;

    bool previewing = cam->IsPreviewing();
    cam->SetVideoResolution(val);
    if( previewing )
        cam->ShowPreview();
}

int DeviceSettingPresenter::SetScreenBrightnessLevel(int val)
{
	db_msg("[debug_zhb]----SetScreenBrightnessLevel = %d",val);
	return  PowerManager::GetInstance()->SetBrightnessLevel(val);
}
void DeviceSettingPresenter::SetScreenDisturbMode(int val)
{
	db_msg("[debug_zhb]----SetScreenDisturbMode = %d",val);

}
void DeviceSettingPresenter::SetVoiceTakePhotoSwitch(int val)
{
	db_msg("[debug_zhb]----SetVoiceTakePhotoSwitch = %d",val);

}
void DeviceSettingPresenter::SetVolumeSelection(int val)
{
	db_msg("[debug_zhb]----SetVolumeSelection = %d",val);
    AudioCtrl::GetInstance()->SetBeepToneVolume(GetBeepToneConfig(val));
}

void DeviceSettingPresenter::SetPowerOnVoiceSwitch(int val)
{
	db_msg("[debug_zhb]----SetPowerOnVoiceSwitch = %d",val);
	char buf[64]={0};
	snprintf(buf,sizeof(buf),"echo %d > /data/power_on_voice.txt",val);
	system (buf);
}
void DeviceSettingPresenter::SetKeyPressVoiceSwitch(int val)
{
	db_msg("[debug_zhb]----SetKeyPressVoiceSwitch = %d",val);

}
void DeviceSettingPresenter::SetDriveringReportSwitch(int val)
{
	db_msg("[debug_zhb]----SetDriveringReportSwitch = %d",val);

}
void DeviceSettingPresenter::Set4GNetWorkSwitch(int val)
{
	db_msg("[debug_zhb]----Set4GNetWorkSwitch = %d",val);

}

void DeviceSettingPresenter::SetAdasSwitch(int val)
{
	if(val){
		db_msg("[debug_zhb]----open adas");
		
		}else{
		db_msg("[debug_zhb]----close adas");
			}

}
void DeviceSettingPresenter::SetAccSwitch(int val)
{
	if(val){
		db_msg("[debug_zhb]----open acc");
		
		}else{
		db_msg("[debug_zhb]----close acc");
			}

}

void DeviceSettingPresenter::SetStandbyClockSwitch(int val)
{

	db_msg("[debug_zhb]----SetStandbyClockSwitch --- val = %d",val);
}

void DeviceSettingPresenter::SetAdasLaneShiftReminding(int val)
{
  #if 0
	if(val)
	{
		db_msg("[debug_zhb]----open adas SetAdasLaneShiftReminding");
		Uber_Control::GetInstance()->setUberADAS_LDW_Enable(true);
	}
	else
	{
		db_msg("[debug_zhb]----close adas SetAdasLaneShiftReminding");
		Uber_Control::GetInstance()->setUberADAS_LDW_Enable(false);
	}
#endif
}
void DeviceSettingPresenter::SetAdasForwardCollisionWaring(int val)
{
 #if 0
	if(val)
	{
		db_msg("[debug_zhb]----open adas SetAdasForwardCollisionWaring");
		Uber_Control::GetInstance()->setUberADAS_FCW_Enable(true);
	}
	else
	{
		db_msg("[debug_zhb]----close adas SetAdasForwardCollisionWaring");
		Uber_Control::GetInstance()->setUberADAS_FCW_Enable(false);
	}
 #endif
}


void DeviceSettingPresenter::SetWatchDogSwitch(int val)
{
	if(val == 0){
		db_msg("[debug_zhb]----open watch dog all switch");
		
		}else if(val == 1){
				db_msg("[debug_zhb]----noly open watch dog probe prompt switch");
			}else if(val == 2){
					db_msg("[debug_zhb]----noly open watch dog speeding prompt switch");
				}else{
					db_msg("[debug_zhb]----invalid parameter");
					}

}
void DeviceSettingPresenter::SetWatchProbePrompt(int val)
{

	db_msg("[debug_zhb]--- SetWatchProbePrompt---val = %d",val);

}
void DeviceSettingPresenter::SetWatchSpeedPrompt(int val)
{

	db_msg("[debug_zhb]--- SetWatchSpeedPrompt---val = %d",val);

}

void DeviceSettingPresenter::SetEmerRecordSwitch(int val)
{
	if(val){
		db_msg("[debug_zhb]----open SetEmerRecordSwitch");
		
		}else{
		db_msg("[debug_zhb]----close SetEmerRecordSwitch");
			}
}

void DeviceSettingPresenter::SetEmerRecordSensitivity(int p_CamId, int val) //碰撞灵敏度
{
	GsensorManager* gs =  GsensorManager::GetInstance();
	gs->writeImpactHappenLevel(val) ;
}

void DeviceSettingPresenter::SetParkingImpactSensitivity(int p_CamId, int val) //停车监控开关
{
	GsensorManager* gs =  GsensorManager::GetInstance();
	switch(val)
	{
	    case 0:
	        val = 3;
	    break;
	    case 1:
	        val = 0;
	    break;
	}
	gs->writeParkingSensibility(val);
}

void DeviceSettingPresenter::SetParkingWarnLampStatus(int val)
{
	db_msg("[debug_zhb]----SetParkingWarnLampStatus = %d",val);
}
void DeviceSettingPresenter::SetParkingAbnormalMonitoryStatus(int val)
{
	db_msg("[debug_zhb]----SetParkingAbnormalMonitoryStatus = %d",val);
}
void DeviceSettingPresenter::SetParkingAbnormalNoticeStatus(int val)
{
	db_msg("[debug_zhb]----SetParkingAbnormalNoticeStatus = %d",val);
}


void DeviceSettingPresenter::SetParkingLoopRecordStatus(int val)
{
	db_msg("[debug_zhb]----SetParkingLoopRecordStatus = %d",val);
}
void DeviceSettingPresenter::SetParkingLoopResolution(int val)
{
	db_msg("[debug_zhb]----SetParkingLoopResolution = %d",val);
}


int DeviceSettingPresenter::GetVideoResolution(int p_CamId, Size &p_size)
{
    Camera *cam = getCamera(p_CamId);
    if( cam == NULL )
        return -1;

    cam->GetVideoResolution(p_size);

    return 0;
}

void DeviceSettingPresenter::SetEncoderType(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetVideoEncoder val = %d####",val);
    Recorder *rec0 = getRecorder(0,0);
    if(rec0 == NULL)
        return;
    rec0->SetVideoEncoderType(val);
    Recorder *rec1 = getRecorder(1,2);
    if(rec1 == NULL)
        return;
    rec1->SetVideoEncoderType(val);
}

void DeviceSettingPresenter::SetVideoDelayTime(int val)
{
    db_warn("[debug_jaosn]:####SetVideoDelayTime val = %d####",val);
    Recorder *rec0 = getRecorder(0,0);
    if(rec0 == NULL)
        return;
   // rec0->SetVideoClcRecordTime(val);
   	rec0->SetDelayRecordTime(val);
   
    Recorder *rec1 = getRecorder(1,2);
    if(rec1 == NULL)
        return;
   // rec1->SetVideoClcRecordTime(val);
}

void DeviceSettingPresenter::SetVideoRecordTime(int val)
{
    db_error("[debug_jaosn]:####SetVideoRecordTime val = %d####",val);
    Recorder *rec0 = getRecorder(0,0);
    if(rec0 == NULL)
        return;
    rec0->SetVideoClcRecordTime(val);
    Recorder *rec1 = getRecorder(1,2);
    if(rec1 == NULL)
        return;
    rec1->SetVideoClcRecordTime(val);
}

void DeviceSettingPresenter::SetEncodeSize(int p_CamId, int val)
{
    Recorder *rec = getRecorder(0,0);
    if(rec == NULL)
    	return;
	//db_error("------------------SetEncodeSize: %d",val);
	rec->SetRecordEncodeSize(val);
}

void DeviceSettingPresenter::SetPicResolution(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetPicResolution val = %d####",val);

	Camera *Cam = getCamera(p_CamId);
    if(Cam == NULL)
        return;
    Cam->SetPicResolution(val);

	Cam = getCamera(CAM_UVC_1);
	if(Cam == NULL)
		return;
	Cam->SetPicResolution(val);
}

int DeviceSettingPresenter::GetPicResolution(int p_CamId, Size &p_size)
{
    Camera *cam = getCamera(p_CamId);
    if( cam == NULL)
        return -1;

    cam->GetPicResolution(p_size);

    return 0;
}


void DeviceSettingPresenter::SetPicQuality(int val)
{
	db_warn("[debug_jaosn]:####SetPicQuality val = %d####",val);

	Camera *Cam = getCamera(0);
	if(Cam == NULL)
		return;
	Cam->SetPicQuality(val);
}

int DeviceSettingPresenter::SetSlowCameraResloution(int p_CamId, int val)
{
    Camera *cam = getCamera(p_CamId);
    if( cam == NULL)
        return -1;

    bool previewing = cam->IsPreviewing();
    cam->SetSlowVideoResloution(val);
    if( previewing )
        cam->ShowPreview();
    return 0;
}

void DeviceSettingPresenter::SetRecordAudioOnOff(int p_CamId, int val)
{
    db_warn("[debug_jaosn]:####SetRecordAudioOnOff p_CamId %d val = %d####",p_CamId,val);
    Recorder *rec = NULL;
    if(p_CamId == 0) {
		rec = getRecorder(p_CamId); //front
		if(rec == NULL)
			return;
    }else if(p_CamId == 1){
		rec = getRecorder(p_CamId,2); //back
		if(rec == NULL)
			return;
    }
    int current_status_ = rec->GetStatus();
    if(current_status_ == RECORDER_RECORDING) {
    	rec->SetMute(!val);
    }
//    rec->SetRecordAudioOnOff(val);
}

void DeviceSettingPresenter::SetExposureValue(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetExposureValue val = %d####",val);
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL)
        return;
    cam->SetExposureValue(val);
}

void DeviceSettingPresenter::SetWhiteBalance(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetWhiteBalance val = %d####",val);
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL)
        return;
    cam->SetWhiteBalance(val);
}

void DeviceSettingPresenter::SetLedSwitch(int val)
{
    db_msg("[debug_jaosn]:####LedSwitch val = %d####",val);
    LedControl *led_ctrl = LedControl::get();
	//by hero ****** close led ctrl
    //led_ctrl->SetLedMainSwitch(val);
    if (softap_on_) {
        //led_ctrl->EnableLed(LedControl::WIFI_LED, val);
        //by hero ****** wifi led ctrl
    }
}

void DeviceSettingPresenter::SetLightFreq(int p_CamId, int val)
{
    db_error("[debug_jaosn]:####SetLightFreq val = %d####",val);
    Camera *cam = getCamera(0);
    if(cam == NULL)
       return;
    cam->SetLightFreq(val);
}


void DeviceSettingPresenter::SetTimeWaterMark(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SetTimeWaterMark val = %d####",val);
	Camera *cam	= getCamera(p_CamId);
    if(cam == NULL)
        return;

    if(val)
    {
        cam->EnableOSD();
    }
    else
    {
        cam->DisableOSD();
    }
}

void DeviceSettingPresenter::ResetDevice()
{
    db_msg("[fangjj]:####ResetDevice####");
    int langval=0;
    MenuConfigLua *mcl=MenuConfigLua::GetInstance();
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
#else
    NewSettingWindow *win = static_cast<NewSettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING_NEW));
#endif
    R *mRObj = R::get();
    mcl->ResetMenuConfig();
    UpdateAllSetting(true);
// 恢复语言
	langval = mcl->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);
	mRObj->SetLangID(langval);
	//下面设置相关窗口的字符词变换
#ifndef SETTING_WIN_USE
	win->InitListViewItem();
#endif
	win_mg_->GetWindow(WINDOWID_SETTING_NEW)->OnLanguageChanged(); 
	PreviewWindow *pwin = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	pwin->USBModeWindowLangChange();

	
    win->ResetUpdate();//menu_ui update
    int win_statu_save=pwin->Get_win_statu_save();
	if (win_statu_save == STATU_PREVIEW) {
		int val = mcl->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
	
		SetVideoResoulation(0,val);
    	SetEncodeSize(0,val);
	} 
	pwin->ResetPhotoMode();
}

void DeviceSettingPresenter::SetDeviceDateTime()
{
   db_msg("[debug_jaosn]:####SetDeviceDateTime####");
#ifdef SETTING_WIN_USE
   SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
   win->ShowTimeSettingWindow();
#endif
}


void DeviceSettingPresenter::SwitchDistortionCalibration(int val)
{
   db_msg("[debug_jaosn]:####SwitchDistortionCalibration val is %d####",val);
}

void DeviceSettingPresenter::SwitchDevicesEIS(int p_CamId, int val)
{
    db_msg("[debug_jaosn]:####SwitchDevicesEIS val is %d####",val);
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL)
        return;

    cam->EnableEIS(val);
}
void DeviceSettingPresenter::HideInfoDialog()
{
#ifdef SETTING_WIN_USE
	SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
	win->HideInfoDialog();
#endif
}


void DeviceSettingPresenter::showButtonDialog()
{
#ifdef SETTING_WIN_USE
	SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
	win->s_BulletCollection->ShowButtonDialog();
#endif
}

#ifdef SETTING_WIN_USE
void DeviceSettingPresenter::HandleButtonDialogMsg(int val)
{
    db_msg("[debug_zhb]----------val = %d",val);
   SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
   PreviewWindow*pw = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	switch(win->s_BulletCollection->getButtonDialogCurrentId()){
		case BC_BUTTON_DIALOG_REBOOT:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_REBOOT");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
				 if(val == 1){
					 //system("rm -rf /overlay/upperdir/*");
	           			 //system("reboot");
				 	}
			}
			break;
		case BC_BUTTON_DIALOG_REFRESH_NETWORK_TIME:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_REFRESH_NETWORK_TIME");
				db_msg("[debug_zhb]--do network update the time and then update the item time_string  ...");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
				if(val == 1){
					usleep(300*1000);//wait laster dialog hide
					if(EventManager::GetInstance()->Get4gSignalLevel() > 0){
						EventManager::GetInstance()->get_ntptime();
						win->UpdateTimeString();
						pw->ShowPromptInfo(PROMPT_UPDATE_NET_TIME_FINISH,2);
					}else{
						 pw->ShowPromptInfo(PROMPT_UPDATE_NET_TIME_FAILED,2);
						}
				}
			}
			break;
		case BC_BUTTON_DIALOG_MANUAL_UPDATE_TIME:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_MANUAL_UPDATE_TIME");
				if(val == 1)
					win->ShowTimeSettingWindow();
			}
			break;
		case BC_BUTTON_DIALOG_RESETFACTORY:
			{
                db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_RESETFACTORY");
                win->updataSubmenuItemChoiced(false);
                db_msg("[debug_zhb]--ready to reset factory ...");
                if(val == 1){
                    DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
                    usleep(300*1000);//wait laster dialog hide
                    pw->ShowPromptInfo(PROMPT_RESET_FACTORY_ING,0);
                    win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
                    //pw->VideoRecordDetect(false);//if resetfactory before is record should stop recording and the reset
                    ResetDevice();
                    //pw->VideoRecordDetect(true);
                    pw->ShowPromptInfo(PROMPT_RESET_FACTORY_FINISH,2,true);
                    DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
                }else{
                    win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
                }
                db_msg("[debug_zhb]--finish the  reset factory ...");
            }
			break;
        case BC_BUTTON_DIALOG_ACCOUNT_UNBIND:
            {
                if(val == 1)
                {
                   // Uber_Control::GetInstance()->setUbewUnbind();
                    pw->ShowPromptInfo(PROMPT_ACCOUNT_UNBIND, 2, true);
                    win->account_bind_status_update();
                    win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
                }else
                {
                    win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
                }
            }
            break;
		case BC_BUTTON_DIALOG_FORMAT_SDCARD:	//button are confirm and cancel , info_text
			{
				if(val == 1){
					win->Update((MSG_TYPE)MSG_FORMAT_START);
					DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
					usleep(500*1000);
					pw->ShowPromptInfo(PROMPT_TF_FORMATTING,0);
					win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
					//pw->VideoRecordDetect(false);//if format before is record should stop recording and the format
					if(StorageManager::GetInstance()->Format() < 0)
						pw->ShowPromptInfo(PROMPT_TF_FORMAT_FAILED,2,true);//force to kill the front dialog and show self
					else
						pw->ShowPromptInfo(PROMPT_TF_FORMAT_FINISH,2,true);//force to kill the front dialog and show self
					win->updataSubmenuItemChoiced(false);
					DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
					win->Update((MSG_TYPE)MSG_FORMAT_FINISH);
				}else{
					win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
					win->Update((MSG_TYPE)MSG_CANCLE_DIALG);
				}
			}
			break;
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN://button are open_now and cancel ,info_text	
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
				if(val == 1)
					win->updataSubmenuItemChoiced(false);
			}
			break;
		case BC_BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
				if(val ==1)
					win->updataSubmenuItemChoiced(false);
			}
			break;
		case BC_BUTTON_DIALOG_4G_NETWORK_OPEN_WATCHDOG:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_4G_NETWORK_OPEN_WATCHDOG");
			}break;
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL://button are restart and cancel ,info_text
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL");
				
			}
			break;
		case BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW://button are flow recharge and cancel ,info_text	
			{
			  	db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW");
				
			}
			break;
		case BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION:	//button are restart and cancel ,info_text	,progressbar			
			{
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION");
				if(val == 1 && win->s_BulletCollection->getFdownload()){
					win->updataSubmenuItemChoiced(false);
				}else if(val == 1 && !win->s_BulletCollection->getFdownload()){
						win->s_BulletCollection->StopProgressbarTime();
						win->updataSubmenuItemChoiced(false);
					}else if(val == 0){////keypress the cancel
							win->s_BulletCollection->StopProgressbarTime();
						}
					
			}
			break;
		case BC_BUTTON_DIALOG_DOWNLOAD_PACKET_WATCHDOG:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_DOWNLOAD_PACKET_WATCHDOG");
			}break;
		case BC_BUTTON_DIALOG_INSTALL_PACKET_WATCHDOG:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_INSTALL_PACKET_WATCHDOG");
			}break;
		case BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION://button is cancel ,info_text,progressbar	
			{
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_INSTALL_PACKET_VERSION");
				if(val == 1 && win->s_BulletCollection->getFinstall()){
					db_msg("[debug_zhb]--install---message from handleprogressbar when packet finish------");
					win->updataSubmenuItemChoiced(false);
				 }else  if(val == 1 && !win->s_BulletCollection->getFinstall()){
						db_msg("[debug_zhb]---install---message from keypress the cancel------");
						win->s_BulletCollection->StopProgressbarTime();
					}
			}
		       break;
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH://button is I know ,info_text
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH");
				
			}
			break;
		case BC_BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH");
				
			}
			break;
		case BC_BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
			}
			break;
		case BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL://button is I know ,info_text,info_tilte
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
			}
			break;
		case BC_BUTTON_DIALOG_ADAS_OPEN_FAILE:
		case BC_BUTTON_DIALOG_WATCHDOG_OPEN_FAILE:
			{
				db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_ADAS_OPEN_FAILE");
				win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
			}
			break;
	default:
		    break;
    	}
	win->s_BulletCollection->setButtonDialogShowFlag(false);
}

#else
void DeviceSettingPresenter::HandleButtonDialogMsg(int val)
{
    NewSettingWindow *win = static_cast<NewSettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING_NEW));
    PreviewWindow*pw = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    switch(win->s_BulletCollection->getButtonDialogCurrentId()){
        case BC_BUTTON_DIALOG_RESETFACTORY:
        {
            db_msg("[debug_zhb]--HandleButtonDialogMsg------BUTTON_DIALOG_RESETFACTORY");
            db_msg("[debug_zhb]--ready to reset factory ...");
            if(val == 1){
                DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
                usleep(300*1000);//wait laster dialog hide
                pw->ShowPromptInfo(PROMPT_RESET_FACTORY_ING,0);
                win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_SHOW_UP_DWON_BUTTON);
                //pw->VideoRecordDetect(false);//if resetfactory before is record should stop recording and the reset
                ResetDevice();
                //pw->VideoRecordDetect(true);
                pw->ShowPromptInfo(PROMPT_RESET_FACTORY_FINISH,2,true);
                DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
            }else{
                win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
            }
            db_msg("[debug_zhb]--finish the  reset factory ...");
        }
        break;
        case BC_BUTTON_DIALOG_FORMAT_SDCARD:    //button are confirm and cancel , info_text
        {
            if(val == 1){
                win->Update((MSG_TYPE)MSG_FORMAT_START);
                DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
                usleep(500*1000);
                pw->ShowPromptInfo(PROMPT_TF_FORMATTING,0);
                win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
               // pw->VideoRecordDetect(false);//if format before is record should stop recording and the format
                if(StorageManager::GetInstance()->Format() < 0)
                {
                    pw->ShowPromptInfo(PROMPT_TF_FORMAT_FAILED,2,true);//force to kill the front dialog and show self
                }
                else
                {
                    pw->ShowPromptInfo(PROMPT_TF_FORMAT_FINISH,2,true);//force to kill the front dialog and show self
                }
                DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
                win->Update((MSG_TYPE)MSG_FORMAT_FINISH);
            }else{
                win->s_BulletCollection->ShowButtonDialogSettingButtonStatus(MSG_SET_HIDE_UP_DWON_BUTTON);
                win->Update((MSG_TYPE)MSG_CANCLE_DIALG);
            }
        }
        break;
        default:
            break;
    }
    win->s_BulletCollection->setButtonDialogShowFlag(false);
}

#endif

int DeviceSettingPresenter::stopSystemRecords(void)
{
    Recorder *rec;
    rec =getRecorder(0,0);
    if( rec != NULL)
        rec->StopRecord();

    rec =getRecorder(0,1);
    if( rec != NULL)
        rec->StopRecord();

    rec =getRecorder(1,2);
    if( rec != NULL)
        rec->StopRecord();

	rec =getRecorder(1,3);
    if( rec != NULL)
        rec->StopRecord();

#ifdef SUB_RECORD_SUPPORT
    rec =getRecorder(REC_S_SUB_CHN);
    if( rec != NULL)
        rec->StopRecord();
#endif
	Camera *cam = getCamera(0);
    if( cam != NULL )
        cam->DeinitCamera();

    cam = getCamera(1);
    if( cam != NULL )
        cam->DeinitCamera();

	WatchDog *dog = WatchDog::GetInstance();
    dog->StopWatchDog();

    return 0;
}

int DeviceSettingPresenter::exeOtaUpdate(int update_type)
{
    char temp[512] = {0};
    db_error("exeOtaUpdate update_type:%d", update_type);

    stopSystemRecords();

    if(update_type == TYPE_UPDATE_NET) {
        snprintf(temp,sizeof(temp),"/etc/firmware/chroot_ota.sh %s/%s/",MOUNT_PATH,VERSION_DIR_NET);
        system(temp);
        db_error("exeOtaUpdate update_success, reboot...");
        system("reboot -f");
        db_error("error: ota_update fail ready to again");
    }

    return 0;
}

void DeviceSettingPresenter::SetContinuousPictureMode(int val)
{
    db_warn("[debug_jaosn]:####SetContinuousPictureMode val = %d####",val);
    Camera *cam = getCamera(0);
    if(cam == NULL)
        return;
    cam->SetContinuousPictureMode(val);
	cam->SetPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
}

void DeviceSettingPresenter::SetCurrentPicMode(int val)
{
	db_warn("[debug_jaosn]:####SetCurrentPicMode TAKE_PICTURE_MODE_FAST  val = %d####",val);
    Camera *cam = getCamera(0);
    if(cam == NULL)
        return;
	cam->SetPictureMode(TAKE_PICTURE_MODE_FAST);

}
#ifdef SETTING_WIN_USE
int DeviceSettingPresenter::HandleGUIMessage(int msg, int val,int id)
{
    db_warn("-------------->msg[%d], val[%d]", msg, val);
    SettingWindow *win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
    switch(msg) {
	case SETTING_DEVICE_LANGUAGE:
	case SETTING_CAMERA_AUTOSCREENSAVER:
	case SETTING_SCREEN_BRIGHTNESS:
	case SETTING_WIFI_SWITCH:
	case SETTING_RECORD_RESOLUTION:
	case SETTING_SYSTEM_SOUND:
	case SETTING_DEVICE_DEVICEINFO:
	case SETTING_DEVICE_SDCARDINFO:
	case SETTING_DEVICE_VERSIONINFO:
	case SETTING_DEVICE_TIME:
	case SETTING_ACC_SWITCH:
		GetSubMenuData(msg);
		break;      
	//case SETTING_WIFI_SWITCH:
	//case SETTING_DEVICE_LANGUAGE:
#ifdef ENABLE_ENC_TYPE_SELECT
        case SETTING_RECORD_ENCODINGTYPE:
#endif
        case SETTING_RECORD_LOOP:
        case SETTING_RECORD_TIMELAPSE:
        case SETTING_RECORD_SLOWMOTION:
        case SETTING_PHOTO_RESOLUTION:
        case SETTING_PHOTO_TIMED:
        case SETTING_PHOTO_AUTO:
        case SETTING_PHOTO_DRAMASHOT:
        case SETTING_CAMERA_EXPOSURE:
        case SETTING_CAMERA_WHITEBALANCE:
        case SETTING_CAMERA_LIGHTSOURCEFREQUENCY:
        case SETTING_CAMERA_TIMEDSHUTDOWN:
		case SETTING_DEVICE_FORMAT:
		case SETTING_DEVICE_VOICESTATUS:
		case SETTING_ADAS_SWITCH:
		case SETTING_STANDBY_CLOCK:
		case SETTING_WATCH_DOG_SWITCH:
		case SETTING_TIMEWATERMARK:
		case SETTING_EMER_RECORD_SWITCH:
		case SETTING_EMER_RECORD_SENSITIVITY:
		case SETTING_PARKING_MONITORY:
		case SETTING_PARKING_WARN_LAMP_SWITCH:
		case SETTING_PARKING_ABNORMAL_MONITORY_SWITCH:
		case SETTING_PARKING_ABNORMAL_NOTICE_SWITCH:
		case SETTING_PARKING_RECORD_LOOP_SWITCH:
		case SETTING_PARKING_RECORD_LOOP_RESOLUTION:
//		case SETTING_REAR_RECORD_RESOLUTION:
		case SETTING_SCREEN_NOT_DISTURB_MODE:
//		case SETTING_RECORD_VOLUME:
		case SETTING_VOICE_TAKE_PHOTO:
		case SETTING_4G_NET_WORK:
		case SETTING_VOLUME_SELECTION:
		case SETTING_POWERON_SOUND_SWITCH:
		case SETTING_KEY_SOUND_SWITCH:
		case SETTING_DRIVERING_REPORT_SWITCH:
		case SETTING_FORWARD_COLLISION_WARNING:
		case SETTING_LANE_SHIFT_REMINDING:
		case SETTING_PROBE_PROMPT:
		case SETTING_SPEED_PROMPT:
            break;
        case MSG_RM_LANG_CHANGED:
           {
                R *mRObj = R::get(); //czh add test
                mRObj->SetLangID(val);
		 db_msg("debug_zhb------------MSG_RM_LANG_CHANGED---val = %d",val);
		 SetMenuConfig(msg,val);
                //SettingWindow *win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
                win->Update(); 
                //SettingHandlerWindowUpdateLabel();
                win_mg_->GetWindow(WINDOWID_PREVIEW)->OnLanguageChanged(); 
                win_mg_->GetWindow(WINDOWID_STATUSBAR)->OnLanguageChanged(); 
		  win->LanuageStringHeadUpdate();
		}
            break;
        case SETTING_CAMERA_TIMEWATERMARK:
               //SetMenuConfig(msg,val);//mofify by zhb
	        //SetTimeWaterMark(val);
            break;
        case SETTING_CAMERA_IMAGEROTATION:
            break;
        case MSG_SET_RECORD_VOLUME:
			db_error("[debug_zhb]-----MSG_SET_RECORD_VOLUME---val = %d",val);
	        SetMenuConfig(msg,val);
            SetRecordAudioOnOff(0, val);
            SetRecordAudioOnOff(1, val);
		  //this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
        case SETTING_CAMERA_DISTORTIONCALIBRATION:
	        SetMenuConfig(msg,val);
		    SwitchDistortionCalibration(val);
            break;
        case MSG_SET_WIFI_SWITCH:
		{
		db_msg("[debug_zhb]-----MSG_SET_WIFI_SWITCH---val = %d",val);
	        SetMenuConfig(msg,val);
            WifiSwitch(val);
        	}
            break;
	case MSG_SET_4G_NET_WORK:
		{
			db_msg("[debug_zhb]-----MSG_SET_4G_NET_WORK---val = %d",val);
	        	SetMenuConfig(msg,val);
			Set4GNetWorkSwitch(val);
		}
		break;
        case SETTING_CAMERA_LEDINDICATOR:
	        SetMenuConfig(msg,val);
               SetLedSwitch(val);
            break;
        case SETTING_BUTTON_DIALOG:
		HandleButtonDialogMsg(val);
            break;
	case SETTING_BUTTON_DIALOG_INFO:
		HideInfoDialog();
		break;
        case MSG_DEVICE_FORMAT:
	     if(val == 0){
            FormatStorage();
	     }
	    this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
        case MSG_SET_DEVICE_DEVICEINFO:
		db_msg("[debug_zhb]-----MSG_SET_DEVICE_DEVICEINFO--");
            break;
        case MSG_SET_DEVICE_VERSIONINFO:
	        SetAutoScreensaver(val);
            break;
	case MSG_SET_DEVICE_SDCARDINFO:
		db_msg("[debug_zhb]-----MSG_SET_DEVICE_SDCARDINFO--");
            break;
	case MSG_DEVICE_TIME:
		{
			db_msg("[debug_zhb]-----MSG_DEVICE_TIME--val = %d",val);
			SetMenuConfig(msg,val);
			showButtonDialog();
		}
            break;
	case MSG_SHOW_WIFI_INFO:
		GetWifiInfo(msg);
		break;
	case MSG_SET_4G_TRAFFIC_INFO_QUERY:
		GetTrafficInfo(msg);
		break;
    	case MSG_SET_4G_FLOW_RECHARGE:
		GetFlowRechargeQRcode(msg);
		break;
    	case MSG_SET_4G_SIM_CARD_INFO:
		GetSimInfo(msg);
		break;
	case MSG_SHOW_WIFI_APP_DOWNLAOD_LINK:
		GetWifiAppDownloadQRcode(msg);
		break;
        case SETTING_DEVICE_RESETFACTORY:
           {
                //ResetDevice();
                //SettingWindow *win = static_cast<SettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING));
                //win->ShowButtonDialog();
            }
            //this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
        case SETTING_DEVICE_DATETIME:
	     db_msg("[debug_zhb]-----SETTING_DEVICE_DATETIME---");
            SetDeviceDateTime();
            this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
        case MSG_SET_VIDEO_RESOULATION:
            {
		 //if is recording ,should stop 
		 PreviewWindow*pw = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
		 DialogStatusManager::GetInstance()->setMDialogEventFinish(false); 
		 VideoRecordDetect(false);//
                SetMenuConfig(msg,val);
                SetVideoResoulation(id, val);
                SetEncodeSize(id, val);
		  VideoRecordDetect(true);//
		  DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
            }
            break;
	case MSG_SET_REAR_RECORD_RESOLUTION:
		{
			db_error("[debug_zhb]-----MSG_SET_REAR_RECORD_ON_OFF---val = %d",val);
			SetMenuConfig(msg,val);
			StartCamBRecord(val);
		}
		break;
	case MSG_SET_SCREEN_BRIGHTNESS_LEVELBAR:
		{
			db_msg("[debug_zhb]-----MSG_SET_SCREEN_BRIGHTNESS_LEVELBAR---");
			this->Notify((MSG_TYPE)MSG_SET_SHOW_RIGHT_LEFT_BUTTON);
			ShowLevelBar(LEVELBAR_SCREEN_BRIGHTNESS);
		}
		break;
	case MSG_SET_SCREEN_BRIGHTNESS:
		{
			db_msg("[debug_zhb]-----MSG_SET_SCREEN_BRIGHTNESS---val = %d",val);
			SetMenuConfig(msg,val);
			SetScreenBrightnessLevel(val);
		}
		break;
	case MSG_SET_SCREEN_NOT_DISTURB_MODE:
		{
			db_msg("[debug_zhb]-----MSG_SET_SCREEN_NOT_DISTURB_MODE---val = %d",val);
			SetMenuConfig(msg,val);
			SetScreenDisturbMode(val);
		}
		break;
	case MSG_SET_VOICE_TAKE_PHOTO:
		{
			db_msg("[debug_zhb]-----MSG_SET_VOICE_TAKE_PHOTO---val = %d",val);
			SetMenuConfig(msg,val);
			SetVoiceTakePhotoSwitch(val);
		}
		break;
	case MSG_SET_POWERON_SOUND_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_POWERON_SOUND_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetPowerOnVoiceSwitch(val);
		}
		break;
	case MSG_SET_KEY_SOUND_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_KEY_SOUND_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetKeyPressVoiceSwitch(val);
		}
		break;
	case MSG_SET_ACC_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_ACC_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetAccSwitch(val);
		}
		break;		
	case MSG_SET_DRIVERING_REPORT_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_DRIVERING_REPORT_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetDriveringReportSwitch(val);
		}
		break;
	case MSG_SET_ADAS_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_ADAS_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetAdasSwitch(val);
		}
		break;
	case MSG_SET_STANDBY_CLOCK:
		{
			db_msg("[debug_zhb]-----MSG_SET_STANDBY_CLOCK---val = %d",val);
			SetMenuConfig(msg,val);
			SetStandbyClockSwitch(val);
		}
		break;
	case MSG_SET_FORWARD_COLLISION_WARNING:
		{
			db_msg("[debug_zhb]-----MSG_SET_FORWARD_COLLISION_WARNING---val = %d",val);
			SetMenuConfig(msg,val);
			SetAdasForwardCollisionWaring(val);
		}
		break;
	case MSG_SET_LANE_SHIFT_REMINDING:
		{
			db_msg("[debug_zhb]-----MSG_SET_LANE_SHIFT_REMINDING---val = %d",val);
			SetMenuConfig(msg,val);
			SetAdasLaneShiftReminding(val);
		}
		break;
	case MSG_SET_WATCH_DOG_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_WATCH_DOG_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetWatchDogSwitch(val);
	        	
		}
		break;
	case MSG_SET_VOLUME_SELECTION:
		{
			db_msg("[debug_zhb]-----MSG_SET_VOLUME_SELECTION---val = %d",val);
			SetMenuConfig(msg,val);
			SetVolumeSelection(val);
		}
		break;
	case MSG_SET_VOLUME_LEVELBAR:
		{
			db_msg("[debug_zhb]-----MSG_SET_VOLUME_LEVELBAR---");
			ShowLevelBar(LEVELBAR_VOLUME);
		}
		break;
	case SETTING_LEVELBAR_HIDE:
		{
			db_msg("[debug_zhb]---device setting--SETTING_LEVELBAR_HIDE---");
			win->HideLeverWindow();
			this->Notify((MSG_TYPE)MSG_SET_HIDE_UP_DWON_BUTTON);
		}
		break;
	case SETTING_LEVELBAR_BRIGHTNESS:
		{
			db_msg("[debug_zhb]--device setting---SETTING_LEVELBAR_BRIGHTNESS---");
			win->setLevelBrightness();
		}
		break;
	case SETTING_TIME_UPDATE_HIDE:
		db_msg("[debug_zhb]-----SETTING_TIME_UPDATE_HIDE---");
		win->UpdateTimeString();
		break;
	case MSG_SET_PROBE_PROMPT:
		{
			db_msg("[debug_zhb]-----MSG_SET_PROBE_PROMPT---val = %d",val);
			SetMenuConfig(msg,val);
			SetWatchProbePrompt(val);
	        	
		}
		break;
	case MSG_SET_SPEED_PROMPT:
		{
			db_msg("[debug_zhb]-----MSG_SET_SPEED_PROMPT---val = %d",val);
			SetMenuConfig(msg,val);
			SetWatchSpeedPrompt(val);
	        	
		}
		break;
	case MSG_SET_UPDATE_DATA:
		{
			db_msg("[debug_zhb]-----MSG_SET_UPDATE_DATA---");
	
		}
		break;
	case MSG_SET_TIEM_WATER_MARK:
		{
			db_msg("[debug_zhb]-----MSG_SET_TIEM_WATER_MARK---val = %d",val);
			SetMenuConfig(msg,val);
			SetTimeWaterMark(id, val);
		}
		break;
	case MSG_SET_EMER_RECORD_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_EMER_RECORD_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetEmerRecordSwitch(val);
		}
		break;
	case MSG_SET_EMER_RECORD_SENSITIVITY:
		{
			db_msg("[debug_zhb]-----MSG_SET_EMER_RECORD_SENSITIVITY---val = %d",val);
			SetMenuConfig(msg,val);
			SetEmerRecordSensitivity(id, val);
		}
		break;
	case MSG_SET_PARKING_WARN_LAMP_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_PARKING_WARN_LAMP_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetParkingWarnLampStatus(val);
		}
		break;
	case MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetParkingAbnormalMonitoryStatus(val);
		}
		break;
	case MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetParkingAbnormalNoticeStatus(val);
		}
		break;
	case MSG_SET_PARKING_RECORD_LOOP_SWITCH:
		{
			db_msg("[debug_zhb]-----MSG_SET_PARKING_RECORD_LOOP_SWITCH---val = %d",val);
			SetMenuConfig(msg,val);
			SetParkingLoopRecordStatus(val);
		}
		break;
	case MSG_SET_PARKING_RECORD_LOOP_RESOLUTION:
		{
			db_msg("[debug_zhb]-----MSG_SET_PARKING_RECORD_LOOP_RESOLUTION---val = %d",val);
			SetMenuConfig(msg,val);
			SetParkingLoopResolution(val);
		}
		break;
	case MSG_SET_BUTTON_STATUS:
		if(val == SET_BUTTON_LEFTRIGHT_SHOW)
			this->Notify((MSG_TYPE)MSG_SET_SHOW_RIGHT_LEFT_BUTTON);
		else if(val ==SET_BUTTON_UPDOWN_SHOW)
			this->Notify((MSG_TYPE)MSG_SET_SHOW_UP_DWON_BUTTON);
		else if(val == SET_BUTTON_UPDOWN_HIDE)
			this->Notify((MSG_TYPE)MSG_SET_HIDE_UP_DWON_BUTTON);
		else if(val == SET_BUTTON_UP_DOWN_CHOICE_HIDE)
			this->Notify((MSG_TYPE)MSG_SET_HIDE_UP_DWON_CHOICE_BUTTON);
		else if(val == SET_BUTTON_UP_DOWN_CHOICE_SHOW)
			this->Notify((MSG_TYPE)MSG_SET_SHOW_UP_DWON_CHOICE_BUTTON);
		break;
#ifdef ENABLE_ENC_TYPE_SELECT
        case MSG_SET_RECORD_ENCODE_TYPE:
            SetMenuConfig(msg,val);
            SetEncoderType(0,val);
            this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
#endif
        case MSG_SET_RECORD_TIME:
            SetMenuConfig(msg,val);
            break;
        case MSG_SET_RECORD_DELAY_TIME:
            break;
        case MSG_SET_RECORD_SLOW_TIME:
            break;
        case MSG_SET_PIC_RESOULATION:
            SetMenuConfig(msg,val);
            SetPicResolution(id, val);
            //this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
        case MSG_SET_PIC_CONTINOUS:
            break;
        case MSG_SET_TIME_TAKE_PIC:
            break;
        case MSG_SET_AUTO_TIME_TAKE_PIC:
            break;
        case MSG_SET_MENU_CONFIG_LUA:
	        SetMenuConfig(msg,val);
		 this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW);
            break;
	    case MSG_GET_MENU_CONFIG_LUA:
            break;
	    case MSG_SET_AUTO_TIME_SCREENSAVER:
	        SetMenuConfig(msg,val);
	        SetAutoScreensaver(val);
            break;
        case MSG_SET_AUTO_TIME_SHUTDOWN:
	        SetMenuConfig(msg,val);
	        SetTimedShutdown(val);
            break;
        case MSG_SET_CAMERA_EXPOSURE:
            SetMenuConfig(msg,val);
            SetExposureValue(id, val);
            break;
        case MSG_SET_CAMERA_WHITEBALANCE:
            SetMenuConfig(msg,val);
            SetWhiteBalance(id, val);
            break;
        case MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY:
            break;
	case MSG_SETTING_TO_PREVIEW:
		this->Notify((MSG_TYPE)MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM);
	break;
        case MSG_SETTING_SHOWUPDATE_DIALOG:
        {
            //SettingWindow *win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
            win->SetIndexToSubmenu(SETTING_DEVICE_DEVICEINFO);
        }
        break;
        case MSG_SYSTEM_UPDATE:
	 case MSG_4G_UPDATE:
           	{
#ifdef DEBUG_OTG
            	db_warn("debug_zhb-----MSG_SYSTEM_UPDATE-start");
#endif
                Recorder *rec;
                rec =getRecorder(0,0);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(0,1);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(1,2);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(1,3);
                if( rec != NULL)
                    rec->StopRecord();

#ifdef SUB_RECORD_SUPPORT
                rec =getRecorder(REC_S_SUB_CHN);
                if( rec != NULL)
                    rec->StopRecord();
#endif
		Camera *cam = getCamera(0);
                if( cam != NULL )
                    cam->DeinitCamera();
				cam = getCamera(1);
                if( cam != NULL )
                    cam->DeinitCamera();
#ifdef DEBUG_OTG
        db_warn("debug_zhb-----MSG_SYSTEM_UPDATE-close the video layer");
#endif
			 WatchDog *dog = WatchDog::GetInstance();
			 dog->StopWatchDog();
		PreviewWindow*pw = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
		//pw->HidePromptInfo();
		char temp[512] = {0};
		char rmcmd[512] = {0};
		if(msg == MSG_SYSTEM_UPDATE)
		{
			 if(val == 0){
			snprintf(temp,sizeof(temp),"/etc/firmware/chroot_ota.sh %s/%s/",MOUNT_PATH,VERSION_DIR);
	            		//system("/etc/firmware/chroot_ota.sh /mnt/extsd/version/");
	            		system(temp);
			 	db_error("error: ota_update fail");
			 }else{
                snprintf(temp,sizeof(temp),"/etc/firmware/chroot_ota.sh %s/%s/",MOUNT_PATH,VERSION_DIR_NET);
                //remove forceupdate flag file .forceupdate
			    snprintf(rmcmd,sizeof(rmcmd),"rm -f %s/%s/%s",MOUNT_PATH,VERSION_DIR_NET,FLAG_FORCEUPDATE);
				system(rmcmd);
				//system("/etc/firmware/chroot_ota.sh /mnt/extsd/net_version/");
				system(temp);
				db_error("error: ota_update fail ready to again");
			 }
		}
		else if(msg == MSG_4G_UPDATE)
		{
			snprintf(temp,sizeof(temp),"ota_update %s/%s/ ota_4g",MOUNT_PATH,VERSION_4G_DIR_NET);
			system(temp);
			// system("ota_update /mnt/extsd/net_4g_version/ ota_4g");
			 db_error("error: module_4g_update fail ");
		}
		 //Can be executed here,mean system failed.
		 pw->ShowPromptInfo(PROMPT_OTA_FAILE,0,true);
		 //wait 3s 
		 sleep(3);
		 //reboot
		 system("reboot -f");
		 //restart the watch dog
		 dog->RunWatchDog();
		 //show the dialog prompt 
        	}
            break;

        default:
            db_msg("unhandled message: msg[%d], val[%d]", msg, val);
            break;
    }

    return 0;
}

#else
int DeviceSettingPresenter::HandleGUIMessage(int msg, int val,int id)
{
    db_warn("----------> setting device  msg[%d], val[%d]", msg, val);
	int tzm;
    switch(msg) 
    {
        case MSG_SET_VIDEO_RESOULATION:
        {
            db_warn("[habo]---> MSG_SET_VIDEO_RESOULATION");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set video resoulation failed!");
                return -1;
            }
            SetVideoResoulation(0,val);
            SetEncodeSize(0,val);
//            VideoRecordDetect(true);
        }break;
//        case MSG_SET_REAR_RECORD_RESOLUTION://后置摄像头设置
//        {
//            db_warn("[habo]---> MSG_SET_REAR_RECORD_RESOLUTION");
//            SetRearVideoResoulation(1,val);
//        }break;
        case MSG_SET_CAMERA_EXPOSURE:
        {
            SetExposureValue(0,val);
        }
		break;
		case MSG_SET_PIC_RESOULATION:
        {
			db_warn("[habo]---> MSG_SET_PIC_RESOULATION");	
            SetPicResolution(0, val);    
		}
		break;
		case MSG_SET_PIC_CONTINOUS:
		{
			db_warn("[TF]---> MSG_SET_PIC_CONTINOUS");
            SetContinuousPictureMode(val);
		}	
        break;
        case MSG_SET_TIME_TAKE_PIC:
		{
			SetCurrentPicMode(0);
			db_warn("[TF]---> MSG_SET_TIME_TAKE_PIC");
        }	
            break;
        case MSG_SET_AUTO_TIME_TAKE_PIC:
		{
			SetCurrentPicMode(0);
			db_warn("[TF]---> MSG_SET_AUTO_TIME_TAKE_PIC");
        }
			break;
		case MSG_SET_PIC_QUALITY:
			db_warn("[TF]---> MSG_SET_PIC_QUALITY");
			SetPicQuality(val);
			break;
        case MSG_SET_VOLUME_SELECTION:
        {
            db_warn("[habo]---> MSG_SET_VOLUME_SELECTION");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set record volume failed!");
                return -1;
            }
            SetVolumeSelection(val);
        }break;
        case MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY:
        {
            db_warn("MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set camera light source frequency failed!");
                return -1;
            }
            SetLightFreq(0,val);
        }break;
        case MSG_SET_PARKING_MONITORY:
        {
            db_warn("MSG_SET_PARKING_MONITORY %d",val);
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set parking monitory failed!");
                return -1;
            }
            SetParkingImpactSensitivity(0,val);
            if(id == 1){
                db_error("set parking monitory by app,update park icon");
                StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
                status_bar->ParkIconHander(true);
            }
        }break;
        case MSG_SET_EMER_RECORD_SENSITIVITY:
        {
            db_warn("[habo]---> MSG_SET_EMER_RECORD_SENSITIVITY %d",val);
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set record sensitivity failed!");
                return -1;
            }
            SetEmerRecordSensitivity(id, val);            
        }break;
        case MSG_SET_RECORD_ENCODE_TYPE:
        {
            db_warn("[habo]---> MSG_SET_RECORD_ENCODE_TYPE");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set record type failed!");
                return -1;
            }
            SetEncoderType(0,val);
        }break;
        case MSG_SET_RECORD_TIME:
        {
            db_warn("[habo]---> MSG_SET_RECORD_TIME");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set record time failed!");
                return -1;
            }
            SetVideoRecordTime(val);            
        }break;
		case MSG_SET_RECORD_DELAY_TIME:
		{
			db_warn("[habo]---> MSG_SET_RECORD_DELAY_TIME");
			if(val != 0)
			{
				SetVideoDelayTime(val);
			}else{
				SetVideoDelayTime(val);
				SetVideoRecordTime(0);
			}
		}
		break;
        case MSG_SET_AUTO_TIME_SCREENSAVER:
        {
            db_warn("[habo]---> MSG_SET_AUTO_TIME_SCREENSAVER");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set screensave failed!");
                return -1;
            }
            SetAutoScreensaver(val);            
        }break;
        case MSG_RM_LANG_CHANGED:
        {
            db_warn("[habo]---> MSG_RM_LANG_CHANGED");
            int ret = SetMenuConfig(msg,val);
            if(ret < 0){
                db_error("set language failed!");
                return -1;
            }
            R *mRObj = R::get(); 
            mRObj->SetLangID(val);
            //下面设置相关窗口的字符词变换
#ifndef SETTING_WIN_USE
            NewSettingWindow *win = static_cast<NewSettingWindow *>(win_mg_->GetWindow(WINDOWID_SETTING_NEW));
            win->InitListViewItem();
#endif
            win_mg_->GetWindow(WINDOWID_SETTING_NEW)->OnLanguageChanged(); 
			PreviewWindow *pwin = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			pwin->USBModeWindowLangChange();

        }break;
        case SETTING_BUTTON_DIALOG:
        {
		    HandleButtonDialogMsg(val);
        }
        break;
        case MSG_SET_RECORD_VOLUME:
        {
             db_error("[debug_zhb]-----MSG_SET_RECORD_VOLUME---val = %d",val);
             int ret = SetMenuConfig(msg,val);
             if(ret < 0){
                 db_error("set record volume failed!");
                 return -1;
             }
             if(id == 1){
                 db_error("set record volume by app,update voice icon");
                 StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
                 status_bar->VoiceIconHandler(true);
             }
        }break;
        case MSG_SET_WIFI_SWITCH:
        {
            db_error("[debug_zhb]-----MSG_SET_WIFI_SWITCH---val = %d",val);
            WifiSwitch(val);
            SetMenuConfig(msg,val);
        }break;
		case MSG_SET_MOTION_DETECT:
		{
		    db_error("[debug_zhb]-----MSG_SET_MOTION_DETECT---val = %d",val);
		    int ret = SetMenuConfig(msg, val);
		    if(ret < 0){
                db_error("set motion detect failed!");
                return -1;
            }
            EnableMotionDetect(0,val);
			break;
		}
        case MSG_SETTING_TO_PREVIEW:
			db_error("[debug_zhb]-----MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM---val = %d",val);
			//this->Notify((MSG_TYPE)MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM);
			this->Notify((MSG_TYPE)MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM);
	    break;
        case MSG_SYSTEM_UPDATE:
        case MSG_4G_UPDATE:
           	{
                Recorder *rec;
                rec =getRecorder(0,0);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(0,1);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(1,2);
                if( rec != NULL)
                    rec->StopRecord();

		rec =getRecorder(1,3);
                if( rec != NULL)
                    rec->StopRecord();

#ifdef SUB_RECORD_SUPPORT
                rec =getRecorder(REC_S_SUB_CHN);
                if( rec != NULL)
                    rec->StopRecord();
#endif
		Camera *cam = getCamera(0);
                if( cam != NULL )
                    cam->DeinitCamera();
				cam = getCamera(1);
                if( cam != NULL )
                    cam->DeinitCamera();

			 WatchDog *dog = WatchDog::GetInstance();
			 dog->StopWatchDog();
		PreviewWindow*pw = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
		char temp[512] = {0};
		char rmcmd[512] = {0};
		if(msg == MSG_SYSTEM_UPDATE)
		{
			 if(val == 0){
                snprintf(temp,sizeof(temp),"/etc/chroot_ota.sh %s/%s/",MOUNT_PATH,VERSION_DIR);
                //system("/etc/firmware/chroot_ota.sh /mnt/extsd/version/");
                system(temp);
                system("/usr/bin/ota_update /mnt/extsd/version/");
			 }else{
                snprintf(temp,sizeof(temp),"/etc/chroot_ota.sh %s/%s/",MOUNT_PATH,VERSION_DIR_NET);
                //remove forceupdate flag file .forceupdate
			    snprintf(rmcmd,sizeof(rmcmd),"rm -f %s/%s/%s",MOUNT_PATH,VERSION_DIR_NET,FLAG_FORCEUPDATE);
				system(rmcmd);
				//system("/etc/firmware/chroot_ota.sh /mnt/extsd/net_version/");
				system(temp);
				db_error("error: ota_update fail ready to again");
			 }
		}
		else if(msg == MSG_4G_UPDATE)
		{
			snprintf(temp,sizeof(temp),"ota_update %s/%s/ ota_4g",MOUNT_PATH,VERSION_4G_DIR_NET);
			system(temp);
			// system("ota_update /mnt/extsd/net_4g_version/ ota_4g");
			 db_error("error: module_4g_update fail ");
		}
		 //Can be executed here,mean system failed.
		 pw->ShowPromptInfo(PROMPT_OTA_FAILE,0,true);
		 //wait 3s 
		 sleep(3);
		 //reboot
		 system("reboot -f");
		 //restart the watch dog
		 dog->RunWatchDog();
		 //show the dialog prompt 
        	}
            break;
		case MSG_SET_GPS_SWITCH:
			EventManager::GetInstance()->SetGpsswitch(val);
			break;
		case MSG_SET_SPEEDUNIT:
			break;
		case MSG_SET_TIMEZONE:
			tzm = MenuConfigLua::GetInstance()->GetTimezone();	// new timezone
			db_error("==================================MSG_SET_TIMEZONE: %d",tzm);
			EventManager::GetInstance()->set_tz_ex(tzm);
			break;
		case MSG_SET_DEVICE_CARID:
			db_error("todo: MSG_SET_DEVICE_CARID");
			break;
        default:
            break;
    }
    
    return 0;
}

#endif

int DeviceSettingPresenter::EnableMotionDetect(int p_nCamid,bool p_bEnable)
{
    Camera *cam = getCamera(p_nCamid);
    if(cam != NULL)
    {
        if(p_bEnable)
            cam->startMotionDetect();
        else
            cam->stopMotionDetect();
    }
    return 0;
}



int DeviceSettingPresenter::GetMenuConfig(int msg)
{
       int val=0;
       MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
	val = menuconfiglua->GetMenuIndexConfig(msg);
       db_msg("[fangjj]:GetMenuConfig:msg[%d], val[%d]", msg, val);
	return val;
}

int DeviceSettingPresenter::SetMenuConfig(int msg, int val)
{
      MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
      db_msg("[fangjj]:SetMenuConfig:msg[%d], val[%d]", msg, val);
      int ret = menuconfiglua->SetMenuIndexConfig(msg,val);
      return ret;
}

int DeviceSettingPresenter::SetIspModuleOnOff(int p_CamId, int val)
{
    Camera *cam = getCamera(p_CamId);
    if(cam == NULL){
        return -1;
        }
    cam->ISPModuleOnOff(val);
    return 0;
}

void DeviceSettingPresenter::SetAutoScreensaver(int val)
{
	Screensaver *ss = Screensaver::GetInstance();
	db_msg("SetAutoScreensaver:val[%d]",val);
	switch(val)
	{
		case AUTOSCREENSAVER_10SEC:
			ss->SetDelayTime(10);
			ss->SetScreensaverEnable(true);
			ss->Start();
			break;
		case AUTOSCREENSAVER_30SEC:
			ss->SetDelayTime(30);
			ss->SetScreensaverEnable(true);
			ss->Start();
			break;
		case AUTOSCREENSAVER_60SEC:
			ss->SetDelayTime(60);
			ss->SetScreensaverEnable(true);
			ss->Start();
			break;
		case AUTOSCREENSAVER_OFF:
			ss->SetScreensaverEnable(false);
			ss->Stop();
			break;
		default:
		break;
	}
}

void DeviceSettingPresenter::SetTimedShutdown(int val)
{
	Autoshutdown *ss = Autoshutdown::GetInstance();
	db_msg("SetTimedShutdown:val[%d]", val);
	switch(val)
	{
		case AUTOSHUTDOWN_OFF:
			ss->Stop();
			ss->SetAutoshutdownEnable(false);
			break;
		case AUTOSHUTDOWN_3MIN:
			ss->SetDelayTime(3*60);
			ss->SetAutoshutdownEnable(true);
			ss->Start();
			break;
		case AUTOSHUTDOWN_5MIN:
			ss->SetDelayTime(5*60);
			ss->SetAutoshutdownEnable(true);
			ss->Start();
			break;
		case AUTOSHUTDOWN_10MIN:
			ss->SetDelayTime(10*60);
			ss->SetAutoshutdownEnable(true);
			ss->Start();
			break;
		default:
		break;
	}

}
void DeviceSettingPresenter::SetAutoStatusBarSaver(bool enable_flag)
{
	StatusBarSaver *sbs = StatusBarSaver::GetInstance();
	sbs->SetDelayTime(5);
	if(enable_flag){
        sbs->SetStatusBarSaverEnable(enable_flag);
        sbs->Start();
	} else {
	    sbs->SetStatusBarSaverEnable(enable_flag);
	    sbs->Stop();
	}
}


void DeviceSettingPresenter::SettingHandlerWindowUpdateLabel()
{
    SettingHandlerWindow *shw = static_cast<SettingHandlerWindow*>(win_mg_->GetWindow(WINDOWID_SETTING_HANDLER));
	shw->UpdateLabel();

}

void DeviceSettingPresenter::UpdateAllSetting(bool update_video_setting)
{
    int val=0;
    db_msg("[fangjj]:--------------NotifyAll------------begin \n");
    /*****switch******/
   // val=GetMenuConfig(SETTING_RECORD_VOLUME);
    //SetRecordAudioOnOff(0, val);

    // HandleGUIMessage(SETTING_RECORD_AWMD, GetMenuConfig(SETTING_RECORD_AWMD));
    // HandleGUIMessage(SETTING_RECORD_DRIVINGMODE, GetMenuConfig(SETTING_RECORD_DRIVINGMODE));

    val=GetMenuConfig(SETTING_CAMERA_LEDINDICATOR);
    SetLedSwitch(val);

    //val= GetMenuConfig(SETTING_CAMERA_TIMEWATERMARK);//modify by zhb
    //SetTimeWaterMark(val);

//    val=GetMenuConfig(SETTING_CAMERA_DISTORTIONCALIBRATION);
 //   SwitchDistortionCalibration(val);

    val= GetMenuConfig(SETTING_WIFI_SWITCH);
    WifiSwitch(val);
    // HandleGUIMessage(SETTING_CAMERA_KEYTONE, GetMenuConfig(SETTING_CAMERA_KEYTONE));

    /*****record******/
    val= GetMenuConfig(SETTING_RECORD_TIMELAPSE);

    val= GetMenuConfig(SETTING_RECORD_RESOLUTION);
   // SetVideoResoulation(0, val);
    SetEncodeSize(0, val);

   	//set screen brightness level
       val = GetMenuConfig(SETTING_SCREEN_BRIGHTNESS);
   	db_msg("debug_zhb----->SETTING_SCREEN_BRIGHTNESS val= %d",val);
//	SetScreenBrightnessLevel(val);
	//set screen not disturb mode
       val = GetMenuConfig(SETTING_SCREEN_NOT_DISTURB_MODE);
	SetScreenDisturbMode(val);
	//set voice take photo switch
       val = GetMenuConfig(SETTING_VOICE_TAKE_PHOTO);
	SetVoiceTakePhotoSwitch(val);
	//set volume selection switch
       val = GetMenuConfig(SETTING_VOLUME_SELECTION);
	SetVolumeSelection(val);
	//set power on sound switch
       val = GetMenuConfig(SETTING_POWERON_SOUND_SWITCH);
	SetPowerOnVoiceSwitch(val);
	//set key sound switch
       val = GetMenuConfig(SETTING_KEY_SOUND_SWITCH);
	SetKeyPressVoiceSwitch(val);
	//set drivering report switch
       val = GetMenuConfig(SETTING_DRIVERING_REPORT_SWITCH);
	SetDriveringReportSwitch(val);
	//set acc switch
       val = GetMenuConfig(SETTING_ACC_SWITCH);	
	SetAccSwitch(val);
	//set adas switch
       val = GetMenuConfig(SETTING_ADAS_SWITCH);
	SetAdasSwitch(val);
	//set standby clock switch
       val = GetMenuConfig(SETTING_STANDBY_CLOCK);
	SetStandbyClockSwitch(val);
	//set adas forward collision waring
       val = GetMenuConfig(SETTING_FORWARD_COLLISION_WARNING);
	SetAdasLaneShiftReminding(val);
	//set adas lane shift reminding
       val = GetMenuConfig(SETTING_LANE_SHIFT_REMINDING);
	SetAdasForwardCollisionWaring(val);
	//set watch dog switch
	val = GetMenuConfig(SETTING_WATCH_DOG_SWITCH);
	SetWatchDogSwitch(val);
	//set watch dog probe prompt
	val = GetMenuConfig(SETTING_PROBE_PROMPT);
	SetWatchProbePrompt(val);
	//set watch dog speed prompt
	val = GetMenuConfig(SETTING_SPEED_PROMPT);
	SetWatchSpeedPrompt(val);
	//set time water mark
	val = GetMenuConfig(SETTING_TIMEWATERMARK);
	//SetTimeWaterMark(0, val);
	//set emergency record switch
	val = GetMenuConfig(SETTING_EMER_RECORD_SWITCH);
	SetEmerRecordSwitch(val);
	//set emergency record sensitivity
	val = GetMenuConfig(SETTING_EMER_RECORD_SENSITIVITY);
	SetEmerRecordSensitivity(0, val);
	//set parking warn lamp sw
	val = GetMenuConfig(SETTING_PARKING_WARN_LAMP_SWITCH);
	SetParkingWarnLampStatus(val);
	//set parking abnormal monitory sw
	val = GetMenuConfig(SETTING_PARKING_ABNORMAL_MONITORY_SWITCH);
	SetParkingAbnormalMonitoryStatus(val);
	//set parking abnormal notice sw
	val = GetMenuConfig(SETTING_PARKING_ABNORMAL_NOTICE_SWITCH);
	SetParkingAbnormalNoticeStatus(val);
	//set parking loop record sw
	val = GetMenuConfig(SETTING_PARKING_RECORD_LOOP_SWITCH);
	SetParkingLoopRecordStatus(val);
	//set parking loop record resolution
	val = GetMenuConfig(SETTING_PARKING_RECORD_LOOP_RESOLUTION);
	SetParkingLoopResolution(val);
	//set 4g net work
	val = GetMenuConfig(SETTING_4G_NET_WORK);
	Set4GNetWorkSwitch(val);
	
#ifdef ENABLE_ENC_TYPE_SELECT
    val= GetMenuConfig(SETTING_RECORD_ENCODINGTYPE);
    SetEncoderType(0,val);
#else
    SetEncoderType(0, VENC_H264);
#endif

    val= GetMenuConfig(SETTING_RECORD_LOOP);
    SetVideoRecordTime(val);

    val = GetMenuConfig(SETTING_PARKING_MONITORY);
    SetParkingImpactSensitivity(0,val);

    /*****photo******/
    if (update_video_setting) {
        val=GetMenuConfig(SETTING_PHOTO_RESOLUTION);
        SetPicResolution(0, val);
    }

    /*****camera******/
    val= GetMenuConfig(SETTING_CAMERA_EXPOSURE);
    SetExposureValue(0, val);

    val= GetMenuConfig(SETTING_CAMERA_WHITEBALANCE);
    SetWhiteBalance(0, val);

    val= GetMenuConfig(SETTING_CAMERA_AUTOSCREENSAVER);
    SetAutoScreensaver(val);

    val= GetMenuConfig(SETTING_CAMERA_TIMEDSHUTDOWN);
    SetTimedShutdown(val);

    val= GetMenuConfig(SETTING_CAMERA_LIGHTSOURCEFREQUENCY);
    SetLightFreq(0,val);
#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
    SetAutoStatusBarSaver(true);
#endif
    /*****device******/
    val =GetMenuConfig(SETTING_DEVICE_LANGUAGE);
    R *mRObj = R::get();
    mRObj->SetLangID(val);
#ifdef SETTING_WIN_USE
    SettingWindow *win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
    win->Update();
#endif
    win_mg_->GetWindow(WINDOWID_STATUSBAR)->OnLanguageChanged();
    win_mg_->GetWindow(WINDOWID_PREVIEW)->OnLanguageChanged();

	//val = GetMenuConfig(SETTING_REAR_RECORD_RESOLUTION);
	//StartCamBRecord(val);
    // off 3dnr, will do it when camera init
    // SetIspModuleOnOff(0);
    val =GetMenuConfig(SETTING_MOTION_DETECT);
    EnableMotionDetect(0,val);
    db_msg("--------------NotifyAll------------ over\n");

}

void DeviceSettingPresenter::NotifyAll()
{
    UpdateAllSetting(false);
}

void DeviceSettingPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

int DeviceSettingPresenter::DeviceModelInit()
{
    db_msg("device setting presenter device model init");
     StorageManager::GetInstance()->Attach(this);
    m_CamMap = mainModule_->getCamerMap();
    m_CamRecMap = mainModule_->getRecoderMap();

    // TODO: update ui status
    status_ = MODEL_INITED;
    //SettingWindow *s_win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
     // PreviewWindow *pre_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	//  if(pre_win->GetRecordStatus())
	 //	s_win->ShowPromptBox();
    return 0;
}

int DeviceSettingPresenter::DeviceModelDeInit()
{
	StorageManager::GetInstance()->Detach(this);
    status_ = MODEL_UNINIT;
    m_CamMap.clear();
    m_CamRecMap.clear();
    return 0;
}

// 等待底层通知回调
void DeviceSettingPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    if (msg_mutex_.try_lock() == false) {
        db_warn("maybe presenter is detaching, ignore this msg");
        return;
    }
    if (ignore_msg_) {
        db_warn("presenter has been detached, do not response msg");
        msg_mutex_.unlock();
        return;
    }
    msg_mutex_.unlock();

    switch (msg) {
        case MSG_SOFTAP_SWITCH_DONE:
            softap_on_ = true;
            break;
        case MSG_SOFTAP_DISABLED:
            softap_on_ = false;
            break;
        case MSG_WIFI_ENABLED:
            break;
        case MSG_WIFI_SCAN_END: 
            break;
	    case MSG_STORAGE_UMOUNT:
	    case MSG_STORAGE_MOUNTED:
		 {
	    }
		break;
	    case MSG_TO_PREVIEW_WINDOW:
		{
#ifdef SETTING_WIN_USE
            SettingWindow *win = static_cast<SettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING));
            win->DoClearOldOperation();
#endif
        }
		return ;
        default:
            break;
    }

    this->Notify(msg);
}

int DeviceSettingPresenter::StartCamBRecord(int p_nEnable)
{
	Recorder *rec = getRecorder(CAM_B,2);
	if(rec != NULL)
	{
		if(p_nEnable)
		{
			Recorder *rec_A = getRecorder(CAM_A, 0);
			if( (rec_A == NULL) || (rec_A->RecorderIsBusy() == false))
			{
				db_warn("cam A not start record yet, so could not start cam B");
				return 0;
			}

			if(!rec->RecorderIsBusy() )
			{
				rec->StartRecord();
				RecorderParam param;
				rec->GetParam(param);
				Camera *cam = getCamera(CAM_B);
				if(cam != NULL)
				{
					cam->SetVideoFileForThumb(param.MF);
					cam->SetPictureMode(TAKE_PICTURE_MODE_FAST);
					cam->TakePictureEx(2, 1);		// thumb					
				}
			}
		}
		else
		{
			if( rec->RecorderIsBusy() )
				rec->StopRecord();
		}
	}

	return 0;
}
