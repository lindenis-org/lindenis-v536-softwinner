/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/*
 * @file newPreview.cpp
 * @author id:826
 * @version v0.3
 * @date 2016-11-03
 */

#include "newPreview.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include "common/app_def.h"
#include "common/app_log.h"
#include "common/posix_timer.h"
#include "window/window_manager.h"
#include "window/prompt.h"
#include "device_model/storage_manager.h"
#include "device_model/system/power_manager.h"
#include "device_model/media/osd_manager.h"
#include "bll_presenter/camRecCtrl.h"
#include "device_model/system/led.h"
#include "window/preview_window.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/dataManager.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/partitionManager.h"
#include "device_model/system/event_manager.h"
#include "device_model/media/camera/camera.h"
#include "device_model/menu_config_lua.h"
#include "uilayer_view/gui/minigui/window/status_bar_bottom_window.h"
#include "window/usb_mode_window.h"
#include "bll_presenter/audioCtrl.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/softap_controller.h"
#include "bll_presenter/remote/interface/dev_ctrl_adapter.h"



#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "newPreview.cpp"
#endif

using namespace std;

NewPreview::NewPreview(MainModule *mm)
{

	lowpower_shutdown_processing_ = false;
    m_MainModule = mm;
    playbackflag = false;
    m_cameraMap.clear();
    m_CamRecMap.clear();
    m_bOsdEnable = true;
//  m_backCameraIsRecording = false;
    m_camBShowFlag = PowerManager::GetInstance()->getAhdInsertOnline();//MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_CAMB_PREVIEWING);
    m_sosRecordisStart = false;
    m_Impact_happen_flag = false;
    isRecordStart = false;
    m_usb_attach_status = false;
    mode_ = USB_MODE_CHARGE;
	mPIPMode = UVC_ON_CSI;
    m_workMode = PowerManager::GetInstance()->getPowenOnType();
    MediaInit();
	playback_mode = false;
	motion_enable = true;
    isshutdownhappen_flag = false;
    m_camBInsertFlag = false;
    pthread_mutex_init(&ahd_lock_, NULL);
    #ifdef ENABLE_RTSP    
    m_NetManger = NetManager::GetInstance();
    m_RtspServer = RtspServer::GetInstance();
    dev_adapter_ = new DeviceAdapter(this);
    m_httpServer_ = httpServer::GetInstance();
    m_httpServer_->setAdapter(dev_adapter_);
    m_httpServer_->start();
    m_httpServer_->init();
    #endif
}

void NewPreview::InitWindow()
{
    db_error("Init window");    
    NetManager::GetInstance()->Attach(this);
    MediaFileManager::GetInstance()->Attach(this);
	PowerManager::GetInstance()->Attach(this);
	StorageManager::GetInstance()->Attach(this);
    Attach(WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    Attach(WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR));
    #ifdef SETTING_WIN_USE
    Attach(WindowManager::GetInstance()->GetWindow(WINDOWID_SETTING));
    #else
    Attach(WindowManager::GetInstance()->GetWindow(WINDOWID_SETTING_NEW));
    #endif
    win_mg_ = WindowManager::GetInstance();
	StatusBarWindow *status_bar = static_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	int x = PowerManager::GetInstance()->getPowenOnType();
	status_bar->SetStatusRecordMode(x==0? 1:0);
    CreateRecorder(CAM_A, 0);
#ifdef USE_CAMB
    if(m_camBShowFlag == 1){
        CreateRecorder(CAM_B, 2);
    }
#endif
    OsdManager::get()->initTimeOsd(m_CamRecMap);
	//OsdManager::get()->setTimeOsdPostion(3840-24*64, 2160-64*2);	// 24个字符,每个宽64点 2304,2032 (672x64)
	OsdManager::get()->setTimeOsdPostion(3056-96, 2032);	// 位置必须16倍对齐否则水印会消失不见
	OsdManager::get()->setGpsOsdPosition(3056-96, 2032 - 96);
	OsdManager::get()->setCaridOsdPosition(3696, 2032 - 96);
}

NewPreview::~NewPreview()
{
	db_msg("by hero *** ~NewPreview");
    pthread_mutex_destroy(&ahd_lock_);
}


int NewPreview::MediaInit()
{
#ifdef USE_CAMA
    CreateCamera(CAM_A);
    StartPreview(CAM_A);
#endif
#ifdef USE_CAMB
    if(m_camBShowFlag == 1){
        CreateCamera(CAM_B);
        StartPreview(CAM_B);
    }
#endif
    return 0;
}

int NewPreview::MediaUnInit()
{
	OsdManager::get()->unInitTimeOsd();

#ifdef USE_CAMA
	StopRecord(CAM_A, 0);
	StopRecord(CAM_A, 1);
#endif
#ifdef USE_CAMB	
if(m_camBShowFlag == 1){
	StopRecord(CAM_B, 2);
	StopRecord(CAM_B, 3);
}
#endif
#ifdef USE_CAMA
	StopPreview(CAM_A);
	DestoryCamera(CAM_A);
#endif
#ifdef USE_CAMB	
if(m_camBShowFlag == 1){
    StopPreview(CAM_B);
    DestoryCamera(CAM_B);
}
#endif
	sync();
	return 0;
}

void NewPreview::MediaDeInit()
{
#ifdef USE_CAMA
    DestoryRecorder(CAM_A,0);
    DestoryRecorder(CAM_A,1);
#endif

#ifdef USE_CAMB
    DestoryRecorder(CAM_B, 2);
    DestoryRecorder(CAM_B, 3);
#endif

#ifdef USE_CAMA
    StopPreview(CAM_A);
    DestoryCamera(CAM_A);
#endif
#ifdef USE_CAMB
if(m_camBShowFlag == 1){
    StopPreview(CAM_B);
    DestoryCamera(CAM_B);
}
#endif
}

Camera* NewPreview::CreateCamera(int p_CamId)
{
	if(!CheckCamIdIsValid(p_CamId))
		return NULL;

	Camera *cam = NULL;
	if(!CheckCameraExist(p_CamId))
	{
		switch(p_CamId)
		{
			case CAM_A:
				cam = CameraFactory::GetInstance()->CreateCamera(CAM_NORMAL_0);
				break;
			case CAM_B:
				cam = CameraFactory::GetInstance()->CreateCamera(CAM_UVC_1);
				break;
		}
		if( cam != NULL )
		{
			m_cameraMap.insert(make_pair(p_CamId, cam));
			m_MainModule->setCamerMap(m_cameraMap);
		}
	}
	else
	{
		cam = GetCamera(p_CamId);
	}


	return cam;
}

//#ifdef SHOW_DEBUG_INFO
void NewPreview::DebugInfoThread(NewPreview *self)
{
	db_error("creat debug info thread!!!");
    prctl(PR_SET_NAME, "DebugInfoThread", 0, 0, 0);
	//EventManager *even_ = EventManager::GetInstance();
    while (true) {
        #if 0
    	self->preview_win_->ClearDebugInfo();
    	FILE *fp = NULL;
		int ret = 0;
		char gps_infomsg_buf[10] = {0};
		char gps_num_buf[10] = {0};
		string res;
		string res1;
		string gngga_buff;
		string gpgsv_buff;
		stringstream ss;
		stringstream ss1;
#if 1
		memset(gps_infomsg_buf, 0, sizeof(gps_infomsg_buf));
		memset(gps_num_buf, 0, sizeof(gps_num_buf));
		int gps_info_msg = 0,gps_num = 0;
		gps_info_msg = even_->GetGPSSignalInfo();
		gps_num = even_->GetGpsSignalLevel();
		
		gngga_buff = even_->GNGGA_string;
		string info = gngga_buff;
		self->preview_win_->InsertDebugInfo("GNGGA:", info);

		gpgsv_buff = even_->GPGSV_string;
		string info1 = gpgsv_buff;
		self->preview_win_->InsertDebugInfo("  GNRMC: ", info1);

#endif
#endif
        db_error("############ current meminfo start ################");
		system("cat proc/meminfo");
        db_error("############ current meminfo end   ################");
        sleep(10);
    }
}
//#endif

int NewPreview::DestoryCamera(int p_CamId)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return -1;

	if(CheckCameraExist(p_CamId))
	{
		map<int, Camera*>::iterator iter;
		for(iter = m_cameraMap.begin(); iter != m_cameraMap.end(); iter++)
		{
			if( iter->first == p_CamId ){

			delete iter->second;
			m_cameraMap.erase(iter);
			m_MainModule->setCamerMap(m_cameraMap);

			return 0;
			}
		}
	}
	return -1;
}

Recorder* NewPreview::CreateRecorder(int p_CamId, int p_record_id)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return NULL;

	Recorder *rec = NULL;
	map<int, Recorder*> mRecGroup;

	if( !CheckRecorderExist(p_CamId, p_record_id) )
	{
		if(CheckCameraExist(p_CamId) )
		{
			switch(p_CamId)
			{
				case CAM_A:
				{		
						rec = RecorderFactory::GetInstance()->CreateRecorder(REC_4K25FPS, GetCamera(p_CamId), 0);
						if( rec != NULL )
						{
							mRecGroup.insert(make_pair(0, rec));
							rec->SetID(0);
                            OsdManager::get()->addCamRecordMap(CAM_A, 0,rec);
						}
                        #ifdef ENABLE_RTSP
						rec = RecorderFactory::GetInstance()->CreateRecorder(REC_M_SUB_CHN, GetCamera(p_CamId), 1);
						if( rec != NULL )
						{
							mRecGroup.insert(make_pair(1, rec));
							rec->SetID(1);
                       //     OsdManager::get()->addCamRecordMap(CAM_A, 1,rec);
                            rec->SetEncodeDataCallback(NewPreview::EncodeDataCallback, this);
						}
                        #endif
						break;
				}
				case CAM_B:
				{			
						rec = RecorderFactory::GetInstance()->CreateRecorder(REC_U_1080P30FPS, GetCamera(p_CamId), 2);
						if( rec != NULL )
						{
							mRecGroup.insert(make_pair(2, rec));
							rec->SetID(2);
                            OsdManager::get()->addCamRecordMap(CAM_B, 2,rec);
						}
                        #ifdef ENABLE_RTSP
						rec = RecorderFactory::GetInstance()->CreateRecorder(REC_B_SUB_CHN, GetCamera(p_CamId), 3);
						if( rec != NULL )
						{
							mRecGroup.insert(make_pair(3, rec));
							rec->SetID(3);
                         //   OsdManager::get()->addCamRecordMap(CAM_B, 3,rec);
                            rec->SetEncodeDataCallback(NewPreview::EncodeDataCallback, this);
						}
                        #endif
						break;
				}
			}
			if( rec != NULL )
			{
				m_CamRecMap.insert(make_pair(p_CamId, mRecGroup));
				m_MainModule->setRecoderMap(m_CamRecMap);
			}
		}
	}
	else
	{
		rec = GetRecorder(p_CamId, p_record_id);
	}

	return rec;
}

int NewPreview::DestoryRecorder(int p_CamId, int p_record_id)
{

	if(!CheckCamIdIsValid(p_CamId))
		return -1;
	if(CheckRecorderExist(p_CamId, p_record_id))
	{
		CamRecMap::iterator iter = m_CamRecMap.begin();;
		for(iter = m_CamRecMap.begin(); iter != m_CamRecMap.end(); iter++)
		{
			if( iter->first == p_CamId )
            {
			//map<RecorderType, Recorder*> mRecGroup;
			    map<int, Recorder*>::iterator rec_iter;
			    for (rec_iter = m_CamRecMap[p_CamId].begin();rec_iter != m_CamRecMap[p_CamId].end(); rec_iter++)
			    {
				    if( rec_iter->first == p_record_id ){
					
					delete rec_iter->second;
					m_CamRecMap.erase(iter);
					m_MainModule->setRecoderMap(m_CamRecMap);
                    return 0;
				    }
			    }
			}

			
		}
	}
    return 0;
}


int NewPreview::WifiSOftApDisable()
{
    int ret =0;
    ret = NetManager::GetInstance()->DisableSoftap();
    if(ret < 0){
        db_msg("WifiSOftApDisable filed");
        return ret;
    }

	//by hero ****** wifi led not light
    //LedControl::get()->EnableLed(LedControl::WIFI_LED, false);

    return 0;
}

int NewPreview::WifiSOftApEnable()
{
    string ssid, pwd;
    NetManager::GetInstance()->GetWifiInfo(ssid, pwd);
    int ret =0;
    ret = NetManager::GetInstance()->SwitchToSoftAp(ssid.c_str(),pwd.c_str(),0,0,0);
    if(ret < 0)
    {
        db_msg("WifiSOftApEnable filed");
        return ret;
    }
	//by hero ****** wifi led light
    //LedControl::get()->EnableLed(LedControl::WIFI_LED, true);
    return 0;
}



int NewPreview::StartRecord(int p_CamId, int p_record_id)
{
	MenuConfigLua *menuconfiglua = MenuConfigLua::GetInstance();
	if(!CheckCamIdIsValid(p_CamId) )
		return -1;

	if(!CheckRecorderExist(p_CamId, p_record_id))
	{
		db_error("recorder not exist p_CamId:%d p_record_id:%d\n",p_CamId,p_record_id);
		return -1;
	}
	#if 0
	Camera *cam = GetCamera(p_CamId);
	WindowManager *win_mg_ = WindowManager::GetInstance();
	PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	if (pre_win->GetWindowStatus() == STATU_PHOTO ) {
		
 		int val = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
		cam->SetVideoResolution(val);
		pre_win->SetWindowStatus(STATU_PREVIEW);
	}
	#endif
	Recorder *rec = GetRecorder(p_CamId, p_record_id);
	
	int val0 = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_TIMELAPSE);
	int val1 = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_LOOP);
	db_error("rec: LOOP: %d TIMELAPSE: %d", val1, val0);
	if (val0) {
		rec->SetVideoClcRecordTime(15);
	} else {
		rec->SetVideoClcRecordTime(val1);
	}
	return rec->StartRecord();
}

int NewPreview::StopRecord(int p_CamId, int p_record_id)
{
	if(!CheckCamIdIsValid(p_CamId) )
		return -1;

	if(!CheckRecorderExist(p_CamId, p_record_id))
		return -1;
    int ret = 0;
	Recorder *rec = GetRecorder(p_CamId, p_record_id);
    ret = rec->StopRecord();
    Camera *cam = GetCamera(p_CamId);
    if(p_CamId == CAM_A)
    {
        cam->releasePictureEncoder(1);
    }else if(p_CamId == CAM_B)
    {
        cam->releasePictureEncoder(3);
    }
    return ret;
}

int NewPreview::TakePicture(int p_CamId, int flag)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return -1;

	if( !CheckCameraExist(p_CamId) )
		return -1;

	Camera *cam = GetCamera(p_CamId);
	#if 0
	WindowManager *win_mg_ = WindowManager::GetInstance();
	PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	if (pre_win->GetWindowStatus() == STATU_PREVIEW ) {
		if (flag == TAKE_PHOTO_NORMAL) {
			cam->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
			pre_win->SetWindowStatus(STATU_PHOTO);
		}
	}
	#endif
    if(p_CamId == 0){
	    return cam->TakePictureEx(0,flag);		// normal
    }else{
        return cam->TakePictureEx(2,flag);		// normal
    }
}

int NewPreview::SwitchDisplay()
{
	if(!CheckCameraExist(CAM_A))
		return -1;

	if(!CheckCameraExist(CAM_B))
		return -1;

	ViewInfo cam0_rect, cam1_rect;
	GetCamera(CAM_A)->GetCameraDispRect(cam0_rect);
	GetCamera(CAM_B)->GetCameraDispRect(cam1_rect);

	db_msg("cam0:rect x %d y %d w %d h %d",cam0_rect.x,cam0_rect.y,cam0_rect.w,cam0_rect.h);
	db_msg("cam1:rect x %d y %d w %d h %d",cam1_rect.x,cam1_rect.y,cam1_rect.w,cam1_rect.h);

	if( cam0_rect.w > cam1_rect.w || cam0_rect.h > cam1_rect.h)
	{
		GetCamera(CAM_A)->SetCameraDispRect(cam1_rect, 2);
		GetCamera(CAM_B)->SetCameraDispRect(cam0_rect, 0);
	}
	else
	{
		GetCamera(CAM_A)->SetCameraDispRect(cam1_rect, 0);
		GetCamera(CAM_B)->SetCameraDispRect(cam0_rect, 2);
	}
	return 0;
}

int NewPreview::TransDisplay(pipMode_t mPIPMode)
{
	if(!CheckCameraExist(CAM_A))
		return -1;

	if(!CheckCameraExist(CAM_B))
		return -1;

	ViewInfo cam0_rect, cam1_rect;
	//GetCamera(CAM_A)->GetCameraDispRect(cam0_rect);
	//GetCamera(CAM_B)->GetCameraDispRect(cam1_rect);

	cam0_rect.x = 60;
	cam0_rect.y = 0;
	cam0_rect.w = SCREEN_WIDTH;
	cam0_rect.h = SCREEN_HEIGHT;
	
	cam1_rect.x = 0;
	cam1_rect.y = 0;
	cam1_rect.w = 240;
	cam1_rect.h = 320;
	db_msg("cam0:rect x %d y %d w %d h %d",cam0_rect.x,cam0_rect.y,cam0_rect.w,cam0_rect.h);
	db_msg("cam1:rect x %d y %d w %d h %d",cam1_rect.x,cam1_rect.y,cam1_rect.w,cam1_rect.h);
	db_warn("---SwitchDisplay------mPIPMode = %d ",mPIPMode);
	switch(mPIPMode) {
	case NO_PREVIEW:	

		break;
	case CSI_ON_UVC :
		{
			GetCamera(CAM_A)->SetCameraDispRect(cam1_rect, 2);
			GetCamera(CAM_B)->SetCameraDispRect(cam0_rect, 0);
			//mPIPMode = UVC_ONLY;
		}
		break;
	case UVC_ON_CSI :
		{
			GetCamera(CAM_A)->SetCameraDispRect(cam0_rect, 0);
			GetCamera(CAM_B)->SetCameraDispRect(cam1_rect, 2);
			//mPIPMode = CSI_ONLY;	
		}
		break;
	case CSI_ONLY:
		{
			GetCamera(CAM_A)->SetCameraDispRect(cam0_rect, 2);
			GetCamera(CAM_B)->SetCameraDispRect(cam1_rect, 0);
			//mPIPMode = UVC_ON_CSI;
		}
		break;
	case UVC_ONLY:
		{
			GetCamera(CAM_A)->SetCameraDispRect(cam1_rect, 0);
			GetCamera(CAM_B)->SetCameraDispRect(cam0_rect, 2);
			//mPIPMode = CSI_ON_UVC;	
		}
		break;
	}
	db_warn("---22222SwitchDisplay------mPIPMode = %d ",mPIPMode);
	return 0;
}

int NewPreview::RestoreDisplay()
{
	if(!CheckCameraExist(CAM_A))
		return -1;
#if 0
	if(m_camBShowFlag){
        if(!CheckCameraExist(CAM_B))
            return -1;
	}
#endif
	ViewInfo cam0_rect;
	cam0_rect.x = 0;
	cam0_rect.y = 0;
	cam0_rect.w = SCREEN_WIDTH;
	cam0_rect.h = SCREEN_HEIGHT;
	GetCamera(CAM_A)->SetCameraDispRect(cam0_rect, 0);

	return 0;
}

int NewPreview::SetCamPreviewRect(int p_CamId,int p_x,int p_y,int p_width,int p_height)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return -1;

	if( !CheckCameraExist(p_CamId) )
		return -1;

	ViewInfo p_info;
	p_info.x = p_x;
	p_info.y = p_y;
	p_info.w = p_width;
	p_info.h = p_height;

	if(p_CamId == 0)
		GetCamera(p_CamId)->SetCameraDispRect(p_info, 2);
	else
		GetCamera(p_CamId)->SetCameraDispRect(p_info, 0);

	return 0;
}

int NewPreview::StartPreview(int p_CamId)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return -1;

	if( !CheckCameraExist(p_CamId) )
		return -1;

	Camera *cam = GetCamera(p_CamId);

	cam->StartPreview();

	return 0;
}

int NewPreview::StopPreview(int p_CamId)
{
	if( !CheckCamIdIsValid(p_CamId) )
		return -1;

	if( !CheckCameraExist(p_CamId) )
		return -1;

	Camera *cam = GetCamera(p_CamId);

	cam->StopPreview();

	return 0;
}


int NewPreview::StopCamera(int camId)
{
    if( !CheckCamIdIsValid(camId) )
		return -1;

	if( !CheckCameraExist(camId) )
		return -1;

	Camera *cam = GetCamera(camId);
	cam->DeinitCamera();
    cam->Close();
	return 0;
}

int NewPreview::StopCamera(int camId, int closeflag)
{
    if( !CheckCamIdIsValid(camId) )
		return -1;

	if( !CheckCameraExist(camId) )
		return -1;

	Camera *cam = GetCamera(camId);
	cam->DeinitCamera();
	if (closeflag)
    	cam->Close();
	return 0;
}
int NewPreview::ReStartCamera(int camId)
{
	if( !CheckCamIdIsValid(camId) )
		return -1;

	if( !CheckCameraExist(camId) )
		return -1;

	Camera *cam = GetCamera(camId);
	CameraInitParam init_param;
	cam->GetCameraInitParam(init_param);
	cam->InitCamera(init_param);

	cam->StartPreview();
}

int NewPreview::StartCamera(int camId)
{
	if(m_camBShowFlag == 1){
	 Camera* cam = NULL;
	//m_camBShowFlag = 1;
	 cam = CreateCamera(CAM_B);
	 CreateRecorder(CAM_B, 2);
	 Recorder *rec = GetRecorder(0, 0);
	 Recorder *rec1 = GetRecorder(1, 2);
	 if(cam){
		 db_error("set camera B order 2");
		 cam->SetDispZorder(2);
	 }
	 StartPreview(CAM_B);
	}
#if 0
    if( !CheckCamIdIsValid(camId) )
		return -1;

	if( !CheckCameraExist(camId) )
		return -1;

	Camera *cam = GetCamera(camId);
	EyeseeLinux::CameraInitParam init_param;
	cam->GetCameraInitParam(&init_param);
	cam->InitCamera(init_param);
	//cam->DeinitCamera();
    //cam->Close();
#endif
	return 0;
}


void NewPreview::DoSystemShutdown()
{
	 //if start record may be stop recod;
	isshutdownhappen_flag = true;
	PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	m_CamRecCtrl->StopAllRecord();
	pw->showShutDownLogo();
	//AudioCtrl::GetInstance()->PlaySound(AudioCtrl::SHUTDOWN_SOUND);
	int status = StorageManager::GetInstance()->GetStorageStatus();
    if((status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))
    {
		db_warn("warning :sd card status is wrong be careful\n");
	}else{
		 AW_MPI_ISP_SetSaveCTX(0);
	}
	 // sync record file data to disk
	sync();
	MenuConfigLua *mcl=MenuConfigLua::GetInstance();
	mcl->UpdateSystemTime(true);
    db_warn("warning :DoSystemShutdown is happen\n");
	usleep(2000*1000);
	this->Notify(MSG_SHUTDOWN_SYSTEM); // MainModule will handle this message
}


void NewPreview::LowPowerShutdownTimerHandler(union sigval sigval)
{
    NewPreview *self = reinterpret_cast<NewPreview *>(sigval.sival_ptr);
    if (PowerManager::GetInstance()->getACconnectStatus()) {
        db_info("ac connected, stop shutdown process");
        stop_timer(self->lowpower_shutdown_timer_id_);
        self->lowpower_shutdown_processing_ = false;
        return;
    }

    self->DoSystemShutdown();
    self->lowpower_shutdown_processing_ = false;
}
void NewPreview::MotionDetectOnOffHandle(union sigval sigval)
{
    NewPreview *self = reinterpret_cast<NewPreview *>(sigval.sival_ptr);
    self->MotionDetectOnOff();
	::delete_timer(self->motiontimer_id_);
}
void NewPreview::MotionDetectOnOff()
{
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
 	int x = menuconfiglua->GetMenuIndexConfig(SETTING_MOTION_DETECT);
	this->motion_enable = x ? true:false;

	Camera *cam = GetCamera(CAM_A);
	if (cam) {
		if (x) {
			//cam->startMotionDetect();
			db_error("-----------------startMODDetect");
		} else {
			//cam->stopMotionDetect();
			db_error("-----------------stopMODDetect");
		}
	} 
}


int NewPreview::HandleGUIMessage(int p_msg,int p_val,int p_CamId)
{
    
	static bool msg_status = false;
    switch(p_msg)
	{
		case PREVIEW_RECORD_BUTTON:
		{
            PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
            db_warn(" 222p_val = %d,p_CamId = %d,pre_win->GetRecordState() = %d ",p_val,p_CamId,pre_win->GetRecordState());
            
			if (pre_win->GetWindowStatus() == STATU_PREVIEW) {
			    db_error("PREVIEW_RECORD_BUTTON start");
				if( !CheckCamIdIsValid(p_CamId) ) {
                    
					break;
                }

				if( !CheckCameraExist(p_CamId) ) {
                    msg_status = false;
                    db_error("Check Camera no Exist!!! set msg_status false");
					break;
                }

				if(msg_status){
			        db_error("PREVIEW_RECORD_BUTTON msg is true,ignore msg!!!");
			        break;
			    }
			    if(!msg_status){
			        db_error("PREVIEW_RECORD_BUTTON msg start,set msg true");
			        msg_status = true;
			    }
				if(!p_val && m_bOsdEnable)
				{
					if( CAM_A == p_CamId )
					{
	                    //OsdManager::get()->DettchVencRegion(0,0);                    
	                    OsdManager::get()->DettchVencRegion(0,1);
	                    OsdManager::get()->DettchVencRegion(0,2);
	                    OsdManager::get()->DettchVencRegion(0,3);
						OsdManager::get()->stopTimeOsd(p_CamId,0);
	                    OsdManager::get()->stopGpsOsd(p_CamId,0);
					}
					else if( CAM_B == p_CamId && m_camBShowFlag == 1)
					{
	                    #if 0
	                    OsdManager::get()->DettchVencRegion(2,4);                    
	                    OsdManager::get()->DettchVencRegion(2,5);
	                    OsdManager::get()->DettchVencRegion(2,6);
	                    OsdManager::get()->DettchVencRegion(2,7);
						OsdManager::get()->stopTimeOsd(p_CamId,2);
	                    OsdManager::get()->stopGpsOsd(p_CamId,2);
                        #endif
					}
				}

				if(p_val)
				{	db_error("rec 002");
					pre_win->SetRecordState(RECORDSTATS_PREPARE);
					Camera *cam = GetCamera(CAM_A);
					if (cam) {
						//cam->stopMotionDetect();
						db_error("-----------------stopMotionDetect");
						this->motion_enable = false;
					}
                    int ret = -1;
                    ret = StartRecord(p_CamId, 0);
					if ( ret == 0) {		//OK
		                TakePicforVideothumb(p_CamId, 0);
						if(m_camBShowFlag == 1)
						{
						    Recorder *rec = GetRecorder(CAM_B, 2);
							if(rec != NULL && !rec->RecorderIsBusy())
						    {
								ret = rec->StartRecord();
                                if(ret == 0)
                                {
								    TakePicforVideothumb(CAM_B, 2);
                                }else if(ret == -2)
                                {
                                    db_msg("R event video dir video record files is full");
                                    if(msg_status)
				                        msg_status = false;
                                    this->motion_enable = true;
                                    pre_win->isMotionDetectHappen = false;
                                    pre_win->ShowPromptBox(PROMPT_BOX_R_EVENT_DIR_FULL, 3);
				                   break;
                                }else if(ret == -3)
                                {
                                    db_msg("Park video dir video record files is full");
                                    if(msg_status)
				                        msg_status = false;
                                    this->motion_enable = true;
                                    pre_win->isMotionDetectHappen = false;
                                    pre_win->ShowPromptBox(PROMPT_BOX_PARK_DIR_FULL, 3);
				                   break;
						        }else{
                                    if(msg_status)
				                        msg_status = false;
                                    pre_win->isMotionDetectHappen = false;
                                    db_msg("Be careful start R video record filed");
                                    break;
                                }

                             }
						}
		                this->Notify((MSG_TYPE)MSG_SET_STATUS_PREVIEW_REC_PLAY);
					}else if(ret == -2){
						    pre_win->SetRecordState(RECORDSTATS_IDLE);
                            db_msg("F event video dir video record files is full");
                            if(msg_status)
				                msg_status = false;
                            this->motion_enable = true;
                            pre_win->isMotionDetectHappen = false;
                            pre_win->ShowPromptBox(PROMPT_BOX_F_EVENT_DIR_FULL, 3);
                            break;
					}else if(ret == -3)
					{
                        db_msg("F Park video dir video record files is full");
                        pre_win->SetRecordState(RECORDSTATS_IDLE);
                        if(msg_status)
				              msg_status = false;
                        this->motion_enable = true;
                        Recorder *rec1 = GetRecorder(CAM_A, 0);
                        if(rec1 != NULL)
                            rec1->setRecordMotionFlag(false);
                        Recorder *rec2 = GetRecorder(CAM_B, 2);
                        if(rec2 != NULL)
                            rec2->setRecordMotionFlag(false);
                        pre_win->isMotionDetectHappen = false;
                        pre_win->ShowPromptBox(PROMPT_BOX_PARK_DIR_FULL, 3);
                        if(m_workMode == 0){	//停车监控
                            db_warn("newPreview is reviced MSG_SYSTEM_POWEROFF 11\n");
                            this->Notify(MSG_SHUTDOWN_SYSTEM);
                        }
				        break;    
                    }else{
                        pre_win->isMotionDetectHappen = false;
                        pre_win->SetRecordState(RECORDSTATS_IDLE);
                        if(msg_status)
				              msg_status = false;
                        db_msg("Be careful start F video record filed");
                        break;
                    }
				}
				else
				{
					StopRecord(p_CamId, 0);
	                if(m_camBShowFlag == 1){
					StopRecord(CAM_B, 2);
	                }
	                this->Notify((MSG_TYPE)MSG_SET_STATUS_PREVIEW_REC_PAUSE);
					pre_win->SetRecordState(RECORDSTATS_IDLE);
				}
		
	            if(p_val && m_bOsdEnable)
				{
	                //OsdManager::get()->AttchlogoVencRegion(0,0);
					OsdManager::get()->AttchCarIdRegion(0,1);
	                OsdManager::get()->AttchVencRegion(0,2);
					OsdManager::get()->AttchGpsRegion(0,3);
		        	OsdManager::get()->startTimeOsd(CAM_A,0);
					OsdManager::get()->startGpsOsd(CAM_A,0);
	                if(m_camBShowFlag == 1){
                        #if 0
	                    OsdManager::get()->AttchlogoVencRegion(2,4);
						OsdManager::get()->AttchCarIdRegion(2,5);
	                    OsdManager::get()->AttchVencRegion(2,6);
						OsdManager::get()->AttchGpsRegion(2,7);
	                    OsdManager::get()->startTimeOsd(CAM_B,2);
						OsdManager::get()->startGpsOsd(CAM_B,2);
                        #endif
	                }

	            }			

				
				pre_win->ShowCamBRecordIcon();
				db_error("PREVIEW_RECORD_BUTTON end");
				if(msg_status){
				   db_error("PREVIEW_RECORD_BUTTON msg status is done,set msg status false");
				   msg_status = false;
				}
			}
			break;
		}
		case PREVIEW_GO_PHOTO_BUTTON:
		{			
			printf("HandleGUIMessage PREVIEW_GO_PHOTO_BUTTON p_val = %d\n",p_val);
			
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PHOTO,1);
			
		}
		break;
		case PREVIEW_GO_RECORD_BUTTON:
		{
			printf("HandleGUIMessage PREVIEW_GO_RECORD_BUTTON p_val = %d\n",p_val);
			
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW,1);	
		}
		break;	
		case PREVIEW_CAMB_PREVIEW_CONTROL:
		{
          #ifdef CAMB_PREVIEW
            Camera *cam = GetCamera(CAM_B);
			if(cam != NULL )
			{
				if(!cam->IsPreviewing())
				{
					cam->ShowPreview();
					m_camBShowFlag = 1;
				}
				else
				{
					//cam->ShowPreview();
					cam->HidePreview();
					m_camBShowFlag = 0;
				}
				MenuConfigLua::GetInstance()->SetMenuIndexConfig(MSG_CAMB_PREVIEW, m_camBShowFlag);
				StatusBarBottomWindow *status_bottom_bar = static_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
				//status_bottom_bar->ShowReturnButton1Status(PREVIEW_WINDOW);
			}
			#endif
			break;
		}
		case PREVIEW_TO_SHUTDOWN:
		{
			//db_warn("newPreview is reviced PREVIEW_TO_SHUTDOWN 33\n");
            DoSystemShutdown();
			break;
		}
		case PREVIEW_GO_PLAYBACK_BUTTON:
		{
			int sum = MediaFileManager::GetInstance()->GetMediaFileCnt("");
			if(sum <= 6)
				this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,0);
			else
				this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,1);
            #ifdef STOPCAMERA_TO_PLAYBACK
            playbackflag = true;
			this->StopCamera(0,0);	// stop only
			//this->StopCamera(CAM_A);
			#ifdef CAMB_PREVIEW
			playback_mode = true;
            #if 0
			if(m_camBShowFlag == 1){
    			TransDisplay(UVC_ON_CSI);
    		    StopCamera(CAM_B);
                DestoryRecorder(CAM_B,2);
                DestoryCamera(CAM_B);
			}
            #endif
			#endif
            #endif
		}
			break;
		case PREVIEW_ONCAMERA_FROM_PLAYBACK:		// for change mode from playback
			#ifdef STOPCAMERA_TO_PLAYBACK
			DoChangewindowStatus(p_val,true);
			#ifdef CAMB_PREVIEW
			playback_mode = false;
			#endif
			#endif
			
			break;
		case PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION:
			this->Notify((MSG_TYPE)MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION);
			break;
		case PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION:
			this->Notify((MSG_TYPE)MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION);
			break;
		case PREVIEW_TO_SETTING_BUTTON:
			//change status bar bottom button
			this->Notify((MSG_TYPE)MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR_BOTTOM);
			// change status_bar icon status 
			this->Notify((MSG_TYPE)MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR);
            break;
        case PREVIEW_TO_SETTING_NEW_WINDOW:
            this->Notify((MSG_TYPE)MSG_PREVIEW_TO_NEWSETTING_WINDOW);
            break;
        case PREVIEW_SWITCH_LAYER:
#ifdef CAMB_PREVIEW
            if(m_camBShowFlag)
               // SwitchDisplay();
               db_warn("----3333mPIPMode = %d ",mPIPMode);
               if(mPIPMode == CSI_ONLY){
					mPIPMode = UVC_ONLY;
				}else if(mPIPMode == UVC_ONLY){
					mPIPMode = UVC_ON_CSI;
				}else if(mPIPMode == UVC_ON_CSI){
					mPIPMode = CSI_ON_UVC;
				}else if(mPIPMode == CSI_ON_UVC){
					mPIPMode = CSI_ONLY;
				}
				db_warn("----4444mPIPMode = %d ",mPIPMode);
               TransDisplay(mPIPMode);
#endif
#if 0
            PartitionManager::GetInstance()->sunxi_spinor_private_sec_set(FKEY_BINDFLAG, "false");
            PartitionManager::GetInstance()->sunxi_spinor_private_set_flag(0);
#endif
            break;
        case PREVIEW_TAKE_PIC_CONTROL:
        {
            db_warn("[debug_jaosn]: take pic happen");
            int val = TakePicture(0,p_val);
			if (val == 0) {
				AudioCtrl::GetInstance()->PlaySound(AudioCtrl::AUTOPHOTO_SOUND);
			}
            if(m_camBShowFlag == 1){
            	TakePicture(1,p_val);
            }
        }
            break;
        case PREVIEW_BUTTON_DIALOG_HIDE:
			{
          	  PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
          	 	 db_msg("[debug_zhb]--->PREVIEW_BUTTON_DIALOG_HIDE");
			  pw->HandleButtonDialogMsg(p_val);
			}
           	 break;
		case PREVIEW_SET_RECORD_MUTE:
		{
			db_error("PREVIEW_SET_RECORD_MUTE p_CamId: %d p_val:%d",p_CamId,p_val);
			Recorder *rec = GetRecorder(0, 0);
			if(rec != NULL){
                rec->SetMute(!p_val);
			}
		}
        break;
		case PREVIEW_LOWPOWER_SHUTDOWN:
            {
                if (!(this->lowpower_shutdown_processing_)) {
                    this->lowpower_shutdown_processing_ = true;
                    create_timer(this, &(this->lowpower_shutdown_timer_id_), LowPowerShutdownTimerHandler);
                    set_one_shot_timer(5, 0, this->lowpower_shutdown_timer_id_);
                }
            }
            break;
		case PREVIEW_WIFI_SWITCH_BUTTON:{
                int ret = 0;
                if(!p_val){
                db_info("wifi disabled, we will stop all record");

                #ifdef ENABLE_RTSP
                Recorder *rec = GetRecorder(CAM_A, 1);
                if(rec != NULL)
                   rec->StopRecord();
                if(m_camBShowFlag == 1){
                    rec = GetRecorder(CAM_B, 3);
                    if(rec != NULL)
                        rec->StopRecord();
                }
                #endif
                ret = WifiSOftApDisable();
                if(ret < 0){
                   db_error("[error]:WifiSOftApDisable filed");
                }
                #ifdef ENABLE_RTSP
                    DestroyRtspServer();
                #endif
            }else{
                ret = WifiSOftApEnable();
                if(ret < 0)
                {
                    //may be close the wifi info tip
                    db_error("[error]:wifi enabled filed");
                    break;
                }
                db_msg("StartRecord is over");
                
				/*
                ret = StartRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN],CAM_NORMAL_0);
                if(ret < 0)
                {
                   db_error("[error]:start subchannel recorder filed");
                   break;
                }
                setWifiFlagtoRecorder(CAM_NORMAL_0,isWifiSoftEnable);
                if (osd_flag) {
                    OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
                }
                */
            }
          }
            break;
        case PREVIEW_EMAGRE_RECORD_CONTROL:
            {
              HandleSosRecord(p_val); 
            }
            break;
		case PREVIEW_AUDIO_BUTTON:
		 	{
				db_warn("-------PREVIEW_AUDIO_BUTTON: %d----------");
				MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
 			    menuconfiglua->SetMenuIndexConfig(MSG_SET_RECORD_VOLUME,0);
		 	}
            break;
#ifdef USB_MODE_WINDOW
        case USB_CHARGING:
        {
            db_warn("habo---> USB_CHARGING ---");

            mode_ = USB_MODE_CHARGE;
            m_usb_attach_status = false;
            Notify((MSG_TYPE)MSG_USB_CHARGING);
            
        }
            break;
        case USB_MASS_STORAGE:
        {
            db_warn("habo---> USB_MASS_STORAGE ---");
            mode_ = USB_MODE_MASS_STORAGE;
            if(isRecordStart)
            {
                PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                pw->VideoStopRecordCtl();
            }
            StorageManager *sm = StorageManager::GetInstance();
            int status = sm->GetStorageStatus();
            if(status == MOUNTED){
                Notify((MSG_TYPE)MSG_USB_MASS_STORAGE);
                StorageManager::GetInstance()->MountToPC();
            }else{
                db_error("sd is remove,can not mount to pc");
//                Notify((MSG_TYPE)MSG_USB_MASS_STORAGE_SD_REMOVE);
            }
        }
            break;
#endif
        case MSG_SYSTEM_SHUTDOWN:
        {
            db_warn("newpreview receive shutdown");
            DoSystemShutdown();
            MediaDeInit();
            db_warn("media deinit end");
            this->Notify(MSG_SHUTDOWN_SYSTEM);
            break;
        }
		case PREVIEW_CHANGEWINDOWSTATUS:
			DoChangewindowStatus(p_val);
			break;
		case PREVIEW_ONCAMERA_USBMODE:
			//DoChangewindowStatus(p_val,true);
			if (!p_val) {
				// close
				//MediaDeInit();
			} else {
				//MediaInit();
			}
			break;
		#if 0
		case PREVIEW_RESTARTMOD:
			{
				MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
 			    int x = menuconfiglua->GetMenuIndexConfig(MSG_SET_MOTION_DETECT);
				if (x) {
					Camera *cam = GetCamera(0);
					if (cam) {
						cam->startMotionDetect();
						db_error("-----------------startMODDetect");
					}
				}
			}
			break;
		#endif
		#if 0
		case PREVIEW_ONCAMERA:
			DoChangewindowStatus(p_val,true);
			break;
		#endif
        case PREVIEW_RESET_CAMERA_ON_ERROR: {
            db_error("PREVIEW_RESET_CAMERA_ON_ERROR");
#if 0
            MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
 			int val = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
            Camera *cam = GetCamera(0);
            cam->SetVideoResolution(val);
            cam->ShowPreview();
#endif
            if(isRecordStart){
                db_error("camera on error,stop rec");
                if(m_bOsdEnable)
                {
                    //OsdManager::get()->DettchVencRegion(0,0);
                    OsdManager::get()->DettchVencRegion(0,1);
                    OsdManager::get()->DettchVencRegion(0,2);
                    OsdManager::get()->DettchVencRegion(0,3);
                    OsdManager::get()->stopTimeOsd(0,0);
                    OsdManager::get()->stopGpsOsd(0,0);
                }
                StopRecord(0, 0);
                if(m_camBShowFlag == 1){
                    db_error("stop camera B rec");
                    StopRecord(CAM_B, 2);
                }
                this->Notify((MSG_TYPE)MSG_SET_STATUS_PREVIEW_REC_PAUSE);
				PreviewWindow *pre_win = static_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                pre_win->SetRecordState(RECORDSTATS_IDLE);
                pre_win->ShowCamBRecordIcon();
                db_error("stop rec end");
            }
            db_error("PREVIEW_RESET_CAMERA_ON_ERROR");
            MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
            int val = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
            Camera *cam = GetCamera(0);
            cam->SetVideoResolution(val);
            cam->ShowPreview();
            db_error("reinit camera end");
            this->Notify((MSG_TYPE)MSG_CAMERA_REINIT_RECORD);
            db_error("start rec");
        }
            break;
        default:
			break;
	}

	return 0;
}


int NewPreview::HandleSosRecord(int val)
{
   PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
   Recorder *rec = GetRecorder(0, 0);
   #ifdef USE_CAMB
   Recorder *rec1 = GetRecorder(1, 2);
    if(rec != NULL || rec1 != NULL)
   #else
   if(rec != NULL)
   #endif
    {
        if(!m_sosRecordisStart){
          m_sosRecordisStart = true;
          rec->setLockFileFlag(true);
        #ifdef USE_CAMB
          if(m_camBShowFlag){
          rec1->setLockFileFlag(true);
          }
        #endif
          //show lock current video file ui
          pw->showLockFileUiInfo(1);
        }else if(m_sosRecordisStart && (m_Impact_happen_flag == false)){
          m_sosRecordisStart = false;
          rec->setLockFileFlag(false);
          #ifdef USE_CAMB
          if(m_camBShowFlag){
          rec1->setLockFileFlag(false);
          }
         #endif
          //show unlock and current file ui
          pw->showLockFileUiInfo(0);
       }else if(m_sosRecordisStart && m_Impact_happen_flag){
          db_warn("current file is locked by gsensor impact can't unlock file");
          pw->showLockFileUiInfo(1);
       }
    }
    return 0;
}

void NewPreview::Update(MSG_TYPE p_msg, int p_CamID, int p_recordId)
{
	db_msg("NewPreview recive msg %d",p_msg);
	switch(p_msg)
	{
		case MSG_CAMERA_ENABLE_OSD:
			m_bOsdEnable = true;
			break;
		case MSG_CAMERA_DISABLE_OSD:
			m_bOsdEnable = false;
			break;
		case MSG_RECORD_START:
            if(!isRecordStart)
            {
                isRecordStart = true;
			    Notify(p_msg);
                 #ifdef ENABLE_RTSP 
                if(m_httpServer_ != NULL){
                m_httpServer_->sendCommdToServer(MSG_RECORD_START,1,"");
                }
                #endif
            }
			break;
       case MSG_RECORD_STOP:
           if (isRecordStart) 
            {
                isRecordStart = false;
                Notify(p_msg);
				create_timer(this, &(this->motiontimer_id_), MotionDetectOnOffHandle);
                set_one_shot_timer(3, 0, this->motiontimer_id_);
                #ifdef ENABLE_RTSP
                if(m_httpServer_ != NULL){
                m_httpServer_->sendCommdToServer(MSG_RECORD_STOP,0,"");
                }
                #endif
            }
			break;
		case MSG_CLOSE_STANDBY_DIALOG:
			{
				PowerManager *pm = PowerManager::GetInstance();
				PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				if(pw->GetPromptPoint()->GetStandbyFlagStatus()) {
					pm->setStandbyFlag(false);
					Notify(p_msg);
					db_error("send MSG_CLOSE_STANDBY_DIALOG msg");
				}
				break;	
			}
		case MSG_ACCON_HAPPEN:
		{
			if(isshutdownhappen_flag)
			{
				db_warn("notify the count is over is enter shutdown not do anything\n");
				break;
			}

			PowerManager *pm = PowerManager::GetInstance(); 
			pm->setStandbyFlag(false);
			db_error("NewPreview send MSG_ACCON_HAPPEN start");
			Notify(p_msg);
			db_msg("NewPreview send MSG_ACCON_HAPPEN end");
			break;
		}
        case MSG_ACCOFF_HAPPEN:
	   	{
	   		db_error("NewPreview receive MSG_ACCOFF_HAPPEN");
            if(m_workMode == 0){
                if(StorageManager::GetInstance()->CheckStorageIsOk() == false)
                {
                    //should poweroff tfcard is not ready
                    db_warn("Be careful should power off tf card is not ready\n");
                    PowerManager *pm = PowerManager::GetInstance(); 
                    pm->setStandbyFlag(true);
			        Notify(p_msg);
                }else{
                    db_warn("do nothing current wait park record finished \n");
                }
            }else{
                PowerManager *pm = PowerManager::GetInstance();
                pm->setStandbyFlag(true);
			    Notify(p_msg);
			    db_msg("NewPreview send MSG_ACCOFF_HAPPEN end");    
            }
			break;
       	}
		case MSG_GPSON_HAPPEN:
		{
			db_error("NewPreview receive MSG_GPSON_HAPPEN");
			Notify(p_msg);
			break;
		}
        case MSG_GPSOFF_HAPPEN:
	   	{
			db_error("NewPreview receive MSG_GPSOFF_HAPPEN");
			Notify(p_msg);
			break;
       	}
        case MSG_STORAGE_MOUNTED:
		{
			Notify(p_msg);
            
            #ifdef ENABLE_RTSP
            if(m_httpServer_ != NULL)
                m_httpServer_->sendCommdToServer(MSG_STORAGE_MOUNTED,1,"");
            #endif
			break;
        }
        case MSG_STORAGE_UMOUNT: 
            Notify(p_msg);
            #ifdef ENABLE_RTSP
            if(m_httpServer_ != NULL)
                m_httpServer_->sendCommdToServer(MSG_STORAGE_UMOUNT,0,"");
            #endif
			break;
	   case MSG_PREPARE_TO_SUSPEND:
	   		//MediaUnInit();
			Notify(p_msg);
	   		break;
	  case MSG_START_RESUME:
			Notify(p_msg);
			//usleep(200*1000);
			//MediaInit();
			break;
	   case MSG_TAKE_THUMB_VIDEO:
	   		TakePicforVideothumb(p_CamID, p_recordId);
	   		break;
	   case MSG_RECORD_FILE_DONE:{
            db_warn("revice MSG_RECORD_FILE_DONE event\n");
            if(m_workMode == 0){	// =0 停车监控
                db_warn("newPreview is reviced MSG_SYSTEM_POWEROFF 11\n");
//                DoSystemShutdown();
                this->Notify(MSG_SHUTDOWN_SYSTEM);
                break;
            }
            
            if(m_sosRecordisStart)
            {
               db_warn("MSG_RECORD_FILE_DONE should be reset m_sosRecordisStart false");
               m_sosRecordisStart = false;
            }
            if(m_Impact_happen_flag){

                db_warn("MSG_RECORD_FILE_DONE should be reset m_Impact_happen_flag false");
                m_Impact_happen_flag = false;
            }
            //add for motion detect fuction
            Recorder *rec0 = GetRecorder(0, 0);
			if(rec0 != NULL)
            	rec0->setRecordMotionFlag(false);
			Recorder *rec1 = GetRecorder(1, 2);
       		if(rec1 != NULL)
           		 rec1->setRecordMotionFlag(false);
            
			int md = rec0->getRecordMode();
			PowerManager *pm = PowerManager::GetInstance();
			PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			pw->RecordMode = md;
            if (md != 0) {  // stop timeosd for park record
                if(m_bOsdEnable)
				{
                    // CAM_A
                    {
	                    //OsdManager::get()->DettchVencRegion(0,0);                    
	                    //OsdManager::get()->DettchVencRegion(0,1);
	                    //OsdManager::get()->DettchVencRegion(0,2);
	                    //OsdManager::get()->DettchVencRegion(0,3);
						OsdManager::get()->stopTimeOsd(CAM_A,0);
	                    OsdManager::get()->stopGpsOsd(CAM_A,0);
					}
                    // CAM_B
					if( m_camBShowFlag == 1)
					{
	                    #if 0
	                    OsdManager::get()->DettchVencRegion(2,4);                    
	                    OsdManager::get()->DettchVencRegion(2,5);
	                    OsdManager::get()->DettchVencRegion(2,6);
	                    OsdManager::get()->DettchVencRegion(2,7);
						OsdManager::get()->stopTimeOsd(CAM_B,2);
	                    OsdManager::get()->stopGpsOsd(CAM_B,2);
                        #endif
					}
				}
            }
            Notify(p_msg);
	   }
            break;
	   case MSG_STORAGE_CAP_NO_SUPPORT:
			Notify(p_msg);
			break;
	   case MSG_STORAGE_FS_ERROR:
            this->Notify(p_msg);
            break;
	 //  case MSG_BATTERY_FULL:
       case MSG_BATTERY_LOW:
            this->Notify((MSG_TYPE)p_msg);
            break;
	   case MSG_SOFTAP_DISABLED:
            this->Notify((MSG_TYPE)p_msg);
            break;
	   case MSG_UNBIND_SUCCESS:
	   		db_warn("[debug_jaosn]:this is MSG_UNBIND_SUCCESS");
	   		this->Notify(p_msg);
	   		break;
	   case MSG_DELETE_VIDEOFILE:
	   		db_warn("[debug_jaosn]:this is MSG_DELETE_VIDEOFILE");
	   		this->Notify(p_msg);
			break;
		case MSG_ADAS_EVENT:
			this->Notify(p_msg, p_CamID);
			break;
		case MSG_IMPACT_HAPPEN:
		{
          db_warn("NewPreview recive MSG_IMPACT_HAPPEN\n");
          PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
            if(pw->getIsRecordStartFlag()){
               m_Impact_happen_flag = true;
               this->HandleSosRecord(0);
            }else{
				if (pw->getIsRecordStartFlag()) {
               		pw->showLockFileUiInfo(1);
				}
            }
        }
		break;
         case MSG_SYSTEM_POWEROFF:
        {
            db_warn("newPreview is reviced MSG_SYSTEM_POWEROFF 11\n");
            DoSystemShutdown();
            this->Notify(MSG_SHUTDOWN_SYSTEM);
            db_warn("newPreview is reviced MSG_SYSTEM_POWEROFF 22\n");
        }
            break;
        case MSG_CAMERA_TAKEPICTURE_ERROR:
        case MSG_CAMERA_TAKEPICTURE_FINISHED: 
	        this->Notify(p_msg);
            break;
#ifdef USB_MODE_WINDOW
         case MSG_USB_HOST_CONNECTED:
         {
              db_warn("habo--->new preiview MSG_USB_HOST_CONNECTED");
            if(isRecordStart)
            {
                db_debug("current is take recording !!!");
                //break;
            }
            StorageManager *sm = StorageManager::GetInstance();
            // update storage status
            int status = sm->GetStorageStatus();
            db_msg("sd status is %d",status);
            if(m_usb_attach_status == false && PowerManager::GetInstance()->getUsbconnectStatus() && status == MOUNTED)
            {
                this->Notify(p_msg);
                m_usb_attach_status= true;
            }
            else
            {
                db_msg("invalid usb connect message, attach_status[%d], UsbconnectStatus[%d]",m_usb_attach_status,PowerManager::GetInstance()->getUsbconnectStatus());
            }
         }
            break;
        case MSG_USB_HOST_DETACHED:
            db_warn("habo---> MSG_USB_HOST_DETACHED  usb = %d  acc= %d",!PowerManager::GetInstance()->getUsbconnectStatus(),!PowerManager::GetInstance()->getACconnectStatus());
            sleep(1); // sleep 1s to wait ac connect status check finish!
            if(m_usb_attach_status && !PowerManager::GetInstance()->getUsbconnectStatus() && !PowerManager::GetInstance()->getACconnectStatus())
            {
                db_warn("habo--->1111 MSG_USB_HOST_DETACHED");
                this->Notify(p_msg);
                m_usb_attach_status = false;
                if(mode_ == USB_MODE_MASS_STORAGE)
                    StorageManager::GetInstance()->UMountFromPC();
                if (mode_ == USB_MODE_CHARGE)
                    PowerManager::GetInstance()->ResetUDC();
                mode_ = USB_MODE_CHARGE;
            }
            else
            {
                db_msg("invalid usb disconnect message, attach_status[%d], UsbconnectStatus[%d] ACconnectStatus[%d]",m_usb_attach_status,PowerManager::GetInstance()->getUsbconnectStatus(),PowerManager::GetInstance()->getACconnectStatus());
            }
            
            break;
#endif
        case MSG_AHD_CONNECT:
       {
           db_warn("[debug_jaosn]: newPreview recived MSG_AHD_CONNECT ++++++++ start\n");
           pthread_mutex_lock(&ahd_lock_);
		   if(playback_mode == false){
             Camera* cam = NULL;
             if(!m_camBShowFlag){
                 m_camBShowFlag = 1;
                 db_warn("[debug_jaosn]: CreateCamera B\n");
                 cam = CreateCamera(CAM_B);
                 db_warn("[debug_jaosn]: CreateRecorder B Id: 2\n");
                 CreateRecorder(CAM_B, 2);
                 Recorder *rec = GetRecorder(0, 0);
                 if (rec == NULL) {
                     db_warn("[debug_jaosn]: GetRecorder(0,0) NULL\n");  
                 }
                 Recorder *rec1 = GetRecorder(1, 2);
                 if (rec1 == NULL) {
                     db_warn("[debug_jaosn]: GetRecorder(1,2) NULL\n");  
                 }
                 if(cam){
                     db_error("set camera B order 2");
                     cam->SetDispZorder(2);
                 }
                 StartPreview(CAM_B);
                 if(rec->RecorderIsBusy())
                 {
                    int ret = -1;
                    
                     PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                      if((rec1 != NULL) && (!rec1->GetRecordStartFlag()))
                      {	
                          db_error("StartRecord R video record");
                         ret = rec1->StartRecord();
                          db_error("TakePicforVideothumb CAM_B 2");
                          if(ret == 0)
                          {
                               TakePicforVideothumb(CAM_B, 2);
                          }else if(ret == -2)
                          {
                               db_msg("R event video dir video record files is full");
                               pw->ShowPromptBox(PROMPT_BOX_R_EVENT_DIR_FULL, 3);
                          }else if(ret == -3)
                          {
                               db_msg("Park video dir video record files is full");
                               pw->ShowPromptBox(PROMPT_BOX_PARK_DIR_FULL, 3);
                          }else{
                               db_msg("Be careful start R video record filed");
                          }
                      }
                 }
             }
		  }else{
			m_camBShowFlag = 1;
		  }
          m_camBInsertFlag = 1;
          db_warn("[debug_jaosn]: newPreview recived MSG_AHD_CONNECT ++++++++ end\n");
          usleep(500*1000);
          pthread_mutex_unlock(&ahd_lock_);
       }
       break;
       case MSG_AHD_REMOVE:
       {
            pthread_mutex_lock(&ahd_lock_);
            db_warn("[debug_jaosn]: [debug_jaosn]: MSG_AHD_REMOVE --------- start\n");
			if(playback_mode == false){			
              if(m_camBShowFlag){
                  m_camBShowFlag = 0;
                  //stop record
                  Recorder *rec1 = GetRecorder(1, 2);
                  if (rec1 == NULL) {
                      db_warn("[debug_jaosn]: rec1 is NULL !!!!");
                  }
                  else
                  {
                      if(rec1->RecorderIsBusy()){
                          db_warn("[debug_jaosn]: current record is on should stop\n");
                          #if 0
                          OsdManager::get()->DettchVencRegion(2,6);
                          OsdManager::get()->DettchVencRegion(2,2);
                          OsdManager::get()->stopTimeOsd(CAM_B,2);
                          #endif
                          rec1->StopRecord();
                          
                      }
                  }
                  db_warn("[debug_jaosn]: StopCamera B");
                  StopCamera(CAM_B);
                  db_warn("[debug_jaosn]: DestoryRecorder B 2");
                  DestoryRecorder(CAM_B,2);
                  db_warn("[debug_jaosn]: DestoryCamera B 2");
                  DestoryCamera(CAM_B);
              }
              db_warn("[debug_jaosn]: RestoreDisplay");
              RestoreDisplay();
              usleep(1000*1000);
			}else{
			  m_camBShowFlag = 0;
			}
            db_error("[debug_jaosn]: MSG_AHD_REMOVE --------- end");
            m_camBInsertFlag = 0;
            pthread_mutex_unlock(&ahd_lock_);
       }
       break;
       case MSG_CAMERA_MOTION_HAPPEN:
	   {
	   	db_error("MSG_CAMERA_MOTION_HAPPEN motion_enable:%d",this->motion_enable);
	   	if (this->motion_enable) 
		{
        //1.set record mode to pack mode
        Recorder *rec0 = GetRecorder(0, 0);
		if(rec0 != NULL)
            rec0->setRecordMotionFlag(true);
		Recorder *rec1 = GetRecorder(1, 2);
        if(rec1 != NULL)
            rec1->setRecordMotionFlag(true);
        this->Notify(p_msg);
		 }
       }
        break;
        case MSG_SOFTAP_ENABLED:
       {
            db_error("[debug_jaosn]:MSG_SOFTAP_ENABLED\n");
           // sleep(5);
          #ifdef ENABLE_RTSP
            Recorder *rec = NULL;
            rec = GetRecorder(CAM_A, 1);
            if(rec != NULL)
                rec->StartRecord();
            #ifdef USE_CAMB
            if(m_camBShowFlag == 1){
                rec = GetRecorder(CAM_B, 3);
                if(rec != NULL)
                    rec->StartRecord();
            }
            #endif
            //create rtsp server
            CreateRtspServer();
            //start rtspServer
            RtspServerStart();
            CreateRtspStreamSender();
        #endif
       }
       break;
       
        case MSG_CAMERA_ON_ERROR:
            this->Notify(p_msg);
            break;

          case MSG_APP_IS_CONNECTED:
        {
             db_warn("MSG_APP_IS_CONNECTED msg");
             Notify(p_msg);
        }
        break;
        case MSG_APP_IS_DISCONNECTED:
        {
            db_warn("MSG_APP_IS_DISCONNECTED msg");
            Notify(p_msg);
        }
		default:
			break;
	}
}

int NewPreview::SetRecorderDataCallBack(int p_CamId, int p_record_id)
{
	if(!CheckCamIdIsValid(p_CamId) )
		return -1;

	if(!CheckRecorderExist(p_CamId, p_record_id))
		return -1;

	Recorder *rec = GetRecorder(p_CamId,p_record_id);

//	rec->SetEncodeDataCallback(PreviewPresenter::EncodeDataCallback, this);

	return 0;
}

bool NewPreview::CheckCamIdIsValid(int p_CamId)
{
	if( p_CamId < 0  || p_CamId > 1)
	{
		db_msg("CamId:%d is invalid",p_CamId);
		return false;
	}

	return true;
}

bool NewPreview::CheckCameraExist(int p_CamId)
{
	map<int, Camera*>::iterator iter;
	for(iter = m_cameraMap.begin(); iter != m_cameraMap.end(); iter++)
	{
		if( iter->first == p_CamId ){
		    return true;
		}
	}
	
	db_msg("CamId %d did not create yet!",p_CamId);

	return false;
}

bool NewPreview::CheckRecorderExist(int p_CamId, int p_record_id)
{
	map<int, std::map<int, Recorder *>>::iterator iter;
	for(iter = m_CamRecMap.begin(); iter != m_CamRecMap.end(); iter++ )
	{
		if( iter->first != p_CamId )
			continue;

		map<int, Recorder*>::iterator rec_iter;
		for (rec_iter = m_CamRecMap[p_CamId].begin();rec_iter != m_CamRecMap[p_CamId].end(); rec_iter++)
		{
			if( rec_iter->first == p_record_id ){
			    return true;
			}
		}
	}

	return false;
}

Camera* NewPreview::GetCamera(int p_CamId)
{
	map<int, Camera*>::iterator iter;
	for(iter = m_cameraMap.begin(); iter != m_cameraMap.end(); iter++)
	{
		if( iter->first == p_CamId ){
		return iter->second;
        }
	}

	return NULL;
}

Recorder* NewPreview::GetRecorder(int p_CamId, int p_record_id)
{
	map<int, std::map<int, Recorder *>>::iterator iter;
	for(iter = m_CamRecMap.begin(); iter != m_CamRecMap.end(); iter++ )
	{
		if( iter->first == p_CamId ){

		std::map<int, Recorder*>::iterator rec_iter;
		for (rec_iter = m_CamRecMap[p_CamId].begin();rec_iter != m_CamRecMap[p_CamId].end(); rec_iter++)
		{
			if( rec_iter->first == p_record_id )
			    return rec_iter->second;
		}
		}
	}

	return NULL;
}

void NewPreview::OnWindowLoaded()
{
	#if 0
	preview_win_  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	#endif
	db_error("SetLayerAlpha LAYER_UI 150");
    #if 1
    if (playbackflag) {
        db_error("playbackflag");
        DoChangewindowStatus(0, true);
        playback_mode = false;
		//StartCamera(CAM_B);
        playbackflag =  false;
    }
    #endif
	Layer::GetInstance()->SetLayerAlpha(LAYER_UI, 150);
	Camera *cam = NULL;
	cam = GetCamera(CAM_A);
	if( cam != NULL )
	{
		cam->ShowPreview();
        cam->Attach(this);
	}
    if (m_camBInsertFlag) 
	{
		m_camBInsertFlag = false;
		cam = CreateCamera(CAM_B);
        CreateRecorder(CAM_B, 2);
        Recorder *rec = GetRecorder(CAM_A, 0);
        Recorder *rec1 = GetRecorder(CAM_B, 2);
        if(cam){
             db_error("set camera B order 2");
             cam->SetDispZorder(2);
        }
        //StartPreview(CAM_B);
	}
	if(m_camBShowFlag == 1)
	{
		cam = GetCamera(CAM_B);
		if( cam != NULL )
		{
			cam->ShowPreview();
            cam->Attach(this);
		}
	}
    //#ifdef SHOW_DEBUG_INFO
    //preview_win_->ShowDebugInfo(true);
	//debug_info_thread_ = thread(DebugInfoThread, this);
	//#endif
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    int x = menuconfiglua->GetMenuIndexConfig(SETTING_MOTION_DETECT);
	this->motion_enable = x ? true : false;
	db_error("OnWindowLoaded end");
}

void NewPreview::OnWindowDetached()
{
	Camera *cam = NULL;
	cam = GetCamera(CAM_A);
	if( cam != NULL )
	{
		cam->HidePreview();
        cam->Detach(this);
	}
#ifdef USE_CAMB
	if(m_camBShowFlag == 1 )
	{
		cam = GetCamera(CAM_B);
		if( cam != NULL ){
			cam->HidePreview();
            cam->Detach(this);
		}
	}
#endif

//	#ifdef SHOW_DEBUG_INFO
	if (debug_info_thread_.joinable()) {
		pthread_cancel(debug_info_thread_.native_handle());
		debug_info_thread_.join();
	}
	//preview_win_->ShowDebugInfo(false);
	//#endif
}

void NewPreview::OnUILoaded()
{
	//Layer::GetInstance()->OpenLayer(LAYER_UI);
	db_warn("========== OnUILoaded");
}

void NewPreview::BindGUIWindow(::Window *win)
{
    Attach(win);
}

int NewPreview::TakePicforVideothumb(int p_CamId, int p_record_id)
{
	if(!CheckCamIdIsValid(p_CamId) )
	{
		db_warn("invalid camera id:%d",p_CamId);
		return -1;
	}

	if(!CheckCameraExist(p_CamId) )
	{
		db_warn("camera id:%d not create yet",p_CamId);
		return -1;
	}
	RecorderParam param;
	Recorder* rec = GetRecorder(p_CamId,p_record_id);
	if( rec == NULL )
	{
		db_warn("get recorder failed camId:%d recorderId:%d",p_CamId);
		return -1;
	}

	rec->GetParam(param);
	Camera *cam = GetCamera(p_CamId);
	if( cam != NULL)
	{
		cam->SetVideoFileForThumb(param.MF);
	    cam->SetPictureMode(TAKE_PICTURE_MODE_FAST);
		db_warn("p_CamId: %d",p_CamId);
		if( p_CamId )
		{
			cam->TakePictureEx(3, 1);	// thumb
		}
		else
			cam->TakePicture(1);

		return 0;
	}

	return -1;
}

void NewPreview::DoChangewindowStatus(int newstatus, bool force)
{
	WindowManager *win_mg_ = WindowManager::GetInstance();
	PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	StatusBarWindow *stw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	StatusBarBottomWindow *sbw  = static_cast<StatusBarBottomWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
	Camera *cam = GetCamera(CAM_A);

	if (force == false) {	
		int oldstatus = pw->GetWindowStatus();
		db_error("new status: %d old status: %d",newstatus,oldstatus);
		if (oldstatus == newstatus) return;
	}
	switch (newstatus) {
		case STATU_PREVIEW:
			pw->SetWindowStatus(STATU_PREVIEW);
			stw->WinstatusIconHander(STATU_PREVIEW);
			//stw->WinstatusIconHander(STATU_PREVIEW);
			sbw->PreviewWindownButtonStatus(true,true,STATU_PREVIEW);
			pw->mySetRecordval();
			db_error("change to preview");	
			//cam->ShowPreview();
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PREVIEW,1);
			break;
    	case STATU_PHOTO:
			pw->SetWindowStatus(STATU_PHOTO);
			cam->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
			cam->ShowPreview();
			db_error("change to photo");
			stw->WinstatusIconHander(STATU_PHOTO);
			sbw->PreviewWindownButtonStatus(true,true,STATU_PHOTO);
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PHOTO,1);
			break;
    	default:
			break;
	}
}


#ifdef ENABLE_RTSP

void NewPreview::SendRtspData(const VEncBuffer *frame, Recorder *rec, NewPreview *self)
{
    if (self->m_stream_sender_map.size() == 0) return;
    if (self->m_RtspServer->GetServerStatus() == RtspServer::SERVER_STOPED) return;

    RtspServer::StreamSender *stream_sender = self->m_stream_sender_map[rec];
    MediaStream::FrameDataType frame_type;
   // db_msg("NewPreview SendRtspData 11\n");
    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame

            // send sps/pps first
            VencHeaderData head_info = {NULL, 0};
            rec->GetVencHeaderData(head_info);

        if (head_info.pBuffer != NULL && head_info.nLength != 0)
           stream_sender->SendVideoData(head_info.pBuffer, head_info.nLength, frame->pts, MediaStream::FRAME_DATA_TYPE_HEADER);
           frame_type = MediaStream::FRAME_DATA_TYPE_I;
        }
        else {
           frame_type = MediaStream::FRAME_DATA_TYPE_P;
        }
       // db_msg("NewPreview SendRtspData data size = %ld ; frame_type = %d\n",frame->data_size,frame_type);
        stream_sender->SendVideoData((unsigned char *) frame->data, frame->data_size, frame->pts,frame_type);
    } else if (frame->stream_type == 0x01) {
        stream_sender->SendAudioData((unsigned char *) frame->data, frame->data_size);
    } else {
       db_msg("stream type: %d", frame->stream_type);
    }
}

static void RtspOnClientConnected(void *context)
{
    db_msg("rtsp client connected, encode I Frame immediately");
    Recorder *recorder = reinterpret_cast<Recorder*>(context);
    recorder->ForceIFrame();
}

void NewPreview::CreateRtspStreamSender()
{
    for(auto iter1 = m_CamRecMap.begin(); iter1 != m_CamRecMap.end(); iter1++)
    {
        Camera *cam = m_cameraMap[iter1->first];
        for(auto iter2 = m_CamRecMap[iter1->first].begin(); iter2 != m_CamRecMap[iter1->first].end(); iter2++)
        {
            if(iter2->first == 1 || iter2->first == 3)
            {
                    Recorder *recorder = m_CamRecMap[iter1->first][iter2->first];
                    std::stringstream ss;
                    RtspServer::StreamSender *stream_sender = NULL;

                    ss << "ch" << cam->GetCameraID() << iter2->first;
                    stream_sender = m_RtspServer->CreateStreamSender(ss.str());
                  //  std::string url_ = stream_sender->streamURL();
                    stream_sender->SetSenderCallback(&RtspOnClientConnected, recorder);
                    db_error("rtsp url: %s", stream_sender->GetUrl().c_str());
                    if (stream_sender != NULL) {
                        m_stream_sender_map.insert(make_pair(recorder, stream_sender));
                    }
            }
        }
    }
}

void NewPreview::EncodeDataCallback(EncodeDataCallbackParam *param)
{
    int ret = 0;
    int sender_type = 0;
    VEncBuffer* frame = NULL;
    Recorder *rec = param->rec_;
   // db_msg("EncodeDataCallback is recived\n");
    switch (param->what_) {
        case MPP_EVENT_ERROR_ENCBUFFER_OVERFLOW:
            // TODO 考虑buffer正在被覆盖的问题
            db_error("send data too slow!!!");
            break;
        default:
            break;
    }

    NewPreview *self = static_cast<NewPreview *>(param->context_);
  //  sender_type = rec->GetStreamSenderType();

    while (1) {

        frame = rec->GetEyeseeRecorder()->getOneBsFrame();
        if (frame == NULL)
            break;

        if (frame->data == NULL) {
            static int cnt = 0;
            rec->GetEyeseeRecorder()->freeOneBsFrame(frame);
            db_error("null data: %d", ++cnt);
            break;
        }
        
       // db_msg("EncodeDataCallback is recived 11\n");
        self->SendRtspData(frame, rec, self);
#if 0
        if (self->mode_ == NORMAL_MODE) {
            if (self->rtsp_flag_ && (sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
               
            }
            if (self->tutk_flag_ && (sender_type & STREAM_SENDER_TUTK) == STREAM_SENDER_TUTK) {
                self->SendTutkData(frame, rec, self);
            }
        } else if (self->mode_ == USB_MODE_UVC) {
            if ((sender_type & STREAM_SENDER_UVC) == STREAM_SENDER_UVC) {
                self->PutUVCData(frame, rec, self);
            }
        }
#endif

#ifdef WRITE_RAW_H264_FILE
        self->WriteRawData(frame, rec, self);
#endif

        rec->GetEyeseeRecorder()->freeOneBsFrame(frame);
    }
}


int NewPreview::CreateRtspServer()
{
    string ip;
    //获取ip地址
    m_RtspServer = m_RtspServer->GetInstance();
    if (m_NetManger->GetNetDevIp("wlan0", ip) < 0)
    {
        ip = "0.0.0.0";
        db_warn("no activity net device found, set rtsp server ip to '%s", ip.c_str());
    }
    //创建RTSP server
    m_RtspServer->CreateServer(ip);
    return 0;
}

void* NewPreview::RtspThreadLoop(void *context)
{
    NewPreview *self = reinterpret_cast<NewPreview*>(context);
    prctl(PR_SET_NAME, "RtspThreadLoop", 0, 0, 0);
    db_debug("run rtsp server");
    self->m_RtspServer->Run();
    return NULL;
}


void NewPreview::RtspServerStart()
{
    pthread_t rtsp_thread_id;
    ThreadCreate(&rtsp_thread_id, NULL, NewPreview::RtspThreadLoop, this);
}

void NewPreview::DestroyRtspServer()
{
    if (m_RtspServer == NULL) {
        db_debug("rtsp server already destroyed, no need to stop");
        return;
    }
    m_RtspServer->Stop();
    db_debug("rtsp server stoped");

    map<Recorder*, RtspServer::StreamSender*>::iterator sender_iter;
    for (sender_iter = m_stream_sender_map.begin(); sender_iter != m_stream_sender_map.end(); sender_iter++) {
        delete (sender_iter->second);
    }
    m_stream_sender_map.clear();
    db_debug("delete all stream sender");

    // tempory add a delay for wait rtsp server inner thread exit
    usleep(500*1000);
    RtspServer::Destroy();
    m_RtspServer = NULL;
    db_debug("rtsp server destroyed");
//    system("/tmp/netstat -tln | grep 8554");

}
#endif

int NewPreview::RemoteSwitchRecord(int value)
{
   PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
   int ret = 0;
   if(pw->HandlerPromptInfo() < 0)
   {
        db_error("tf card is not ready\n");
        return -1;
   }
    if(value == 1 && isRecordStart)
    {
        db_warn("the main record is start do nothing\n");
        return 0;
    }else if(value == 1 && !isRecordStart)
    {
        db_warn("RemoteSwitchRecord start record\n");
        Recorder *rec1 = GetRecorder(0, 0);
        if(rec1 != NULL && !rec1->RecorderIsBusy()){
		    ret = StartRecord(0, 0);
            if(ret == 0){
            TakePicforVideothumb(0, 0);
            }else if(ret == -2)
            {
                pw->ShowPromptBox(PROMPT_BOX_F_EVENT_DIR_FULL, 3);
                return ret;
            }else if(ret == -3)
            {
                pw->ShowPromptBox(PROMPT_BOX_PARK_DIR_FULL, 3);
                return ret;
            }else{
                db_warn("RemoteSwitchRecord start record filed\n");
            }
            
        }
        if(m_camBShowFlag == 1)
		{
		    Recorder *rec = GetRecorder(1, 2);
			if(rec != NULL && !rec->RecorderIsBusy())
		    {
				ret = rec->StartRecord();
                if(ret == 0){
				TakePicforVideothumb(1, 2);
                }else if(ret == -2)
                {
                    pw->ShowPromptBox(PROMPT_BOX_R_EVENT_DIR_FULL, 3);
                    return ret;
                }else if(ret == -3)
                {
                    pw->ShowPromptBox(PROMPT_BOX_PARK_DIR_FULL, 3);
                    return ret;
                }
			}
		}
        this->Notify((MSG_TYPE)MSG_SET_STATUS_PREVIEW_REC_PLAY);

        //start osd
        #if 0
        if(m_bOsdEnable){
         OsdManager::get()->AttchlogoVencRegion(0,5);
          OsdManager::get()->AttchVencRegion(0,0);
	     OsdManager::get()->startTimeOsd(CAM_A,0);
          if(m_camBShowFlag == 1){
              OsdManager::get()->AttchlogoVencRegion(2,6);
              OsdManager::get()->AttchVencRegion(2,2);
              OsdManager::get()->startTimeOsd(CAM_B,2);
          }
        }
        #endif
	}else if(value == 0 && isRecordStart)
    {        
        db_warn("RemoteSwitchRecord stop record\n");
        //stop osd
        #if 0
        if(m_bOsdEnable){
         OsdManager::get()->DettchVencRegion(0,0);                    
         OsdManager::get()->DettchVencRegion(0,5);
         OsdManager::get()->stopTimeOsd(0,0);
         if(m_camBShowFlag == 1){
             OsdManager::get()->DettchVencRegion(2,2);                    
             OsdManager::get()->DettchVencRegion(2,6);
    		 OsdManager::get()->stopTimeOsd(1,2);
         }
        }
        #endif
        
		StopRecord(0, 0);
       if(m_camBShowFlag == 1){
		StopRecord(0, 2);
       }
       this->Notify((MSG_TYPE)MSG_SET_STATUS_PREVIEW_REC_PAUSE);
	}else{
        db_warn("the main record is aready stoped \n");
    }
    return 0;
}

int NewPreview::RemoteTakePhoto()
{
   db_warn("RemoteTakePhoto happen\n");
   PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
   if(pw->HandlerPromptInfo() < 0)
   {
        db_error("tf card is not ready\n");
        return -1;
   }
    TakePicture(0,0);
    if(m_camBShowFlag == 1){
         TakePicture(1,0);
    }
    return 0;
}

int NewPreview::GetCurretRecordTime()
{
    #if 0
    int ret = 0;
    db_error("GetCurretRecordTime get in\n");
    if(isRecordStart){
        PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
        ret = pw->GetCurrentRecordTime();
        db_error("the current is time %d",ret);
    }else{
        db_error("the record is not start\n");
    }
    #endif
    return 0;
}







