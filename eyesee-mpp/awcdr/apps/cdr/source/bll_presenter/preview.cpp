/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview.cpp
 * @brief 单路预览录像presenter
 * @author id:826
 * @version v0.3
 * @date 2016-08-01
 */
#include "bll_presenter/preview.h"
#include "bll_presenter/remote/interface/dev_ctrl_adapter.h"
#include "bll_presenter/remote/media_control_impl.h"
#include "device_model/media/player/video_player.h"
#include "device_model/system/usb_gadget.h"
#include "device_model/system/event_manager.h"
#include "device_model/system/keyinput.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/ethernet_controller.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/bluetooth_controller.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/media/image_manager.h"
#include "device_model/media/video_alarm_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/rtsp.h"
#include "window/window.h"
#include "window/window_manager.h"
#include "window/usb_mode_window.h"
#include "window/preview_window.h"
#include "window/status_bar_window.h"
#include "window/playback_window.h"
#include "window/user_msg.h"
#include "common/setting_menu_id.h"
#include "common/app_log.h"
#include "application.h"
#include "device_model/system/power_manager.h"
#include "window/status_bar_bottom_window.h"
#include "minigui-cpp/resource/resource_manager.h"
#include "device_model/media/osd_manager.h"
#include "device_model/display.h"

#include "tutk/wrapper/tutk_wrapper.h"
#include "lua/lua_config_parser.h"
#include "device_model/system/led.h"
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/autoshutdown.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/backCameraCheck.h"
#include <sstream>
#include "window/ListItemInfoWindow.h"//add by zhb
#include "window/setting_window.h"
#include "window/promptBox.h"

#undef LOG_TAG
#define LOG_TAG "PreviewPresenter"

using namespace EyeseeLinux;
using namespace tutk;
using namespace std;

extern Camera *g_camera_front;

/*
void PreviewPresenter::TimeTakePicTimerProc(union sigval sigval)
{
    //take pic action
    int ret = 0;
    PreviewPresenter *pp = reinterpret_cast<PreviewPresenter*>(sigval.sival_ptr);
    ret = pp->camera_map_[CAM_NORMAL_0]->TakePicture();
    if(ret < 0)
    {
       stop_timer(pp->time_take_pic_timer_id_);
       pp->isAllowTakePIC = true;
       return;
    }
    system("tinyplay /usr/share/minigui/res/audio/shutter.wav");
}
*/
void PreviewPresenter::TimeTakePic()
{
    //take pic action
    int ret = 0;
    camera_map_[CAM_NORMAL_0]->SetPictureMode(TAKE_PICTURE_MODE_FAST);
    ret = camera_map_[CAM_NORMAL_0]->TakePicture(0);
    if(ret < 0)
    {
        isTimeTakeTimerStart = false;
        isAutoTakeTimerStart = false;
        db_msg("takepicure fail!");
       return;
    }
}

PreviewPresenter::PreviewPresenter(MainModule *mm)
    : status_(MODEL_UNINIT)
    , win_mg_(WindowManager::GetInstance())
    , preview_win_(NULL)
    , nm_(NetManager::GetInstance())
    , deinit_flag_(true)
    , rtsp_flag_(false)
    , onvif_flag_(false)
    , tutk_flag_(true)
    , audio_record_(false)
    , wifi_statu_(false)
    , isAllowTakePIC (true)
    , isWifiSoftEnable(false)
    , isRecordStart(false)
    , last_win_statu_msg(MSG_CHANG_STATU_PREVIEW)
    , win_statu_msg(MSG_CHANG_STATU_PREVIEW)
    , storage_status_(-1)
    , mode_(NORMAL_MODE)
    , m_usb_attach_status(false)
    , re_init_camera_slowrecod(false)
    , re_init_camera_photo(false)
    , m_HdmiDevConn(false)
    , isAutoTakeTimerStart(false)
    , isTimeTakeTimerStart(false)
    , mFrontCamRecordStartStatus(false)
    , mBackCamRecordStartStatus(false)
    , lowpower_shutdown_processing_(false)
{
    pthread_mutex_init(&model_lock_, NULL);
    //get mainmodule
    mainmodule = mm;
    memset(&m_cam_Chnparam,0,sizeof(m_cam_Chnparam));
    // GUI need this msg
    StorageManager::GetInstance()->Attach(this);
    MediaFileManager::GetInstance()->Attach(this);
    Autoshutdown::GetInstance()->Attach(this);
	BackCameraCheck::GetInstance()->Attach(this);
    this->Attach(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Attach(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    dev_adapter_ = new DeviceAdapter(this);

    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    audio_record_ = (bool)menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_VOLUME);
#if 0
    {
        ::LuaConfig wizard_config;

        const char *config_file = "/tmp/data/wizard_config.lua";
        if (!FILE_EXIST(config_file)) {
            db_warn("config file %s not exist, copy default from /usr/share/app/sdv", config_file);
            system("cp -f /usr/share/app/sdv/wizard_config.lua /tmp/data/");
        }

        int ret = wizard_config.LoadFromFile(config_file);
        if (ret < 0) {
            db_warn("Load %s failed, copy backup and try again", config_file);
            system("cp -f /usr/share/app/sdv/wizard_config.lua /tmp/data/");
            ret = wizard_config.LoadFromFile(config_file);
            if (ret < 0) {
                db_error("Load %s failed!", config_file);
            }
        }

        rtsp_flag_ = wizard_config.GetBoolValue("wizard.features.rtsp");
        onvif_flag_ = wizard_config.GetBoolValue("wizard.features.onvif");
        tutk_flag_ = wizard_config.GetBoolValue("wizard.features.tutk");
    }

#ifdef ENABLE_RTSP
    if (rtsp_flag_) {
        rtsp_server_ = RtspServer::GetInstance();
    }
#endif
#endif
        this->DeviceModelInit();

#ifdef TUTK_SUPPORT
    if (tutk_flag_) {
        // TODO: tutk去初始化会阻塞在IOTC_Device_Login导致线程无法正常退出，故暂时不停止tutk service
        tutk_connector_ = new TutkWrapper(dev_adapter_);
        ::LuaConfig config;
        config.LoadFromFile("/data/ipc_config.lua");
        string uuid = config.GetStringValue("system.uuid");
        if (uuid.empty())
            uuid = "A5H9X6ZEXT86GJKL111A";
        RemoteConnector::InitParam param;
        param.arg1 = uuid;
        tutk_connector_->Init(param);
        tutk_connector_->Start();
    }
#endif
	if( !BackCameraCheck::GetInstance()->IsBackCameraConnect() )
		Layer::GetInstance()->SetPreviewType(ONLY_FRONT);

	//by hero ****** rec log light&wifi flase light
    //LedControl::get()->EnableLed(LedControl::REC_LED,true,LedControl::LONG_LIGHT);
    //LedControl::get()->EnableLed(LedControl::WIFI_LED,false);
	LedControl::get()->EnableLed(LedControl::DEV_LED,false,LedControl::LONG_LIGHT);

    cdx_sem_init(&mTakePhotoOver,0);

//#ifdef SHOW_DEBUG_INFO
    system("mount -t debugfs none /proc/sys/debug");
//#endif

}


PreviewPresenter::~PreviewPresenter()
{
    db_msg("destruct");
    if (dev_adapter_) {
        delete dev_adapter_;
        dev_adapter_ = NULL;
    }
    this->DeviceModelDeInit();
    this->Detach(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Detach(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    StorageManager::GetInstance()->Detach(this);
    MediaFileManager::GetInstance()->Detach(this);
    Autoshutdown::GetInstance()->Detach(this);
	BackCameraCheck::GetInstance()->Detach(this);

    pthread_mutex_destroy(&model_lock_);
    cdx_sem_deinit(&mTakePhotoOver);
}

void PreviewPresenter::OnWindowLoaded()
{
    db_msg("window load");
    preview_win_ = reinterpret_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    win_statu_msg = MSG_CHANG_STATU_PREVIEW;
/*
    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }
*/
//     camera_map_[CAM_NORMAL_0]->ShowPreview();

    Layer *layer = Layer::GetInstance();
    // enable ui layer alpha to show preview, set 255 can disable it
    layer->SetLayerAlpha(LAYER_UI, 150);

    //int disp_type, tv_mode;
    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    layer->GetDisplayDeviceType(&disp_type, &tv_mode);

    ViewInfo sur;

    if (disp_type & VO_INTF_LCD/*DISP_OUTPUT_TYPE_LCD*/) {
        sur.x = 0;
        sur.y = 0;
        sur.w = SCREEN_WIDTH;
        sur.h = SCREEN_HEIGHT;
    } else if (disp_type & VO_INTF_HDMI/*DISP_OUTPUT_TYPE_HDMI*/) {
        sur.x = 0;
        sur.y = 0;
        switch (tv_mode) {
            case VO_OUTPUT_3840x2160_25:
            case VO_OUTPUT_3840x2160_30:
                sur.w = 3840;
                sur.h = 2160;
                break;
            case VO_OUTPUT_1080P24:
            case VO_OUTPUT_1080P25:
            case VO_OUTPUT_1080P30:
            case VO_OUTPUT_1080P60:
                sur.w = 1920;
                sur.h = 1080;
                break;
            default:
                sur.w = 1920;
                sur.h = 1080;
                break;
        }
    } else {
        sur.x = 0;
        sur.y = 0;
        sur.w = SCREEN_WIDTH;
        sur.h = SCREEN_HEIGHT;
        fprintf(stderr, "unknown disp type[%d], use lcd config\n", disp_type);
    }

    PowerManager *pm = PowerManager::GetInstance();
    if (!pm->IsScreenOn()) {
        db_error("screen current status is off");
    }
    layer->SetLayerRect(LAYER_CAM0, &sur);
    camera_map_[CAM_NORMAL_0]->ShowPreview();
	if(BackCameraCheck::GetInstance()->IsBackCameraConnect())
		camera_map_[CAM_UVC_1]->ShowPreview();

 //   ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
   /*
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    audio_record_ = (bool)menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_VOLUME);
    ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
    */
#ifdef SHOW_DEBUG_INFO
    preview_win_->ShowDebugInfo(true);
    debug_info_thread_ = thread(DebugInfoThread, this);
#endif
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void PreviewPresenter::OnWindowDetached()
{
    db_msg("window detach");

    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = true;

    camera_map_[CAM_NORMAL_0]->HidePreview();
	if(BackCameraCheck::GetInstance()->IsBackCameraConnect())
		camera_map_[CAM_UVC_1]->HidePreview();
/*
    if (status_ == MODEL_INITED && deinit_flag_) {
        this->DeviceModelDeInit();
    }

    deinit_flag_ = true;
    */
#ifdef SHOW_DEBUG_INFO
    if (debug_info_thread_.joinable()) {
        pthread_cancel(debug_info_thread_.native_handle());
        debug_info_thread_.join();
    }
    preview_win_->ShowDebugInfo(false);
#endif
}


void PreviewPresenter::OnUILoaded()
{
    Layer::GetInstance()->OpenLayer(LAYER_UI);
}

void PreviewPresenter::PrepareExit()
{
    StatusBarWindow *stb_win_ = reinterpret_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
   // stb_win_->StopUpdateDateTime();
    // TODO: need wait takepic done
    RecordHandle(CAM_NORMAL_0,0);
    StreamRecordHandle(0);
    if (!isAllowTakePIC || isTimeTakeTimerStart || isAutoTakeTimerStart) {
        isAllowTakePIC = true;
        db_warn("shutdown...interrupt takepic");
        TakePicButtonHandler(CAM_NORMAL_0, true, true);
        isTimeTakeTimerStart = false;
        isAutoTakeTimerStart = false;
    }
    if (StorageManager::GetInstance()->IsMounted()) {
        AW_MPI_ISP_SetSaveCTX(0);
    }
    // sync record file data to disk
    sync();

    this->Notify(MSG_SHUTDOWN_SYSTEM); // MainModule will handle this message
}

void PreviewPresenter::DoSystemShutdown()
{
	db_error("Notify MSG_SYSTEM_POWEROFF");
	this->Notify(MSG_SYSTEM_POWEROFF);
    this->PrepareExit();
}

int PreviewPresenter::WifiSOftApDisable()
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

int PreviewPresenter::WifiSOftApEnable()
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

int PreviewPresenter::RemoteSwitchRecord()
{
    int ret;
    if (preview_win_->GetWindowStatus() == STATU_SLOWRECOD) {
        // back to photo
        preview_win_->keyProc(SDV_KEY_VIRTUAL, SHORT_PRESS);
        sleep(1);
        // back to normal record
        preview_win_->keyProc(SDV_KEY_VIRTUAL, SHORT_PRESS);
    } else if (preview_win_->GetWindowStatus() == STATU_PHOTO) {
        // back to normal record
        preview_win_->keyProc(SDV_KEY_VIRTUAL, SHORT_PRESS);
    }
    if(isRecordStart){
        db_msg("RemoteSwitchRecord off");
        if(osd_flag){
            OsdManager::get()->DettchVencRegion();
            OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
            OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
        }
        if(this->RecordHandle(CAM_NORMAL_0,0) < 0)
        {
            db_error("[debug_jaosn]:RemoteSwitchRecord stop failed");
            return -1;
        }
        StreamRecordHandle(0);
        usleep(500*1000);
        StreamRecordHandle(1);

        StorageManager::GetInstance()->SetDiskFullNotifyFlag(false);
    }else{
        db_msg("RemoteSwitchRecord on");
        // if(re_init_camera_photo || re_init_camera_slowrecod){
            //close stream recorder
            StreamRecordHandle(0);
            ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
            StreamRecordHandle(1);
        // }
        if(this->RecordHandle(CAM_NORMAL_0,1) < 0)
        {
            db_error("[debug_jaosn]: RemoteSwitchRecord failed");
            return -1;
        }
        GetThumbVideoFileName(CAM_NORMAL_0);
        usleep(500*1000);
        cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->addOutputFormatAndOutputSink();
        if(osd_flag){
            OsdManager::get()->AttchVencRegion();
            OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
            OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
        }
        StorageManager::GetInstance()->SetDiskFullNotifyFlag(true);
    }
    return 0;
}


int PreviewPresenter::RemoteSwitchSlowRecord()
{
    int ret;
    if (preview_win_->GetWindowStatus() == STATU_PREVIEW) {
        // goto photo
        preview_win_->keyProc(SDV_KEY_POWER, SHORT_PRESS);
        sleep(1);
        // goto slow record
        preview_win_->keyProc(SDV_KEY_POWER, SHORT_PRESS);
    } else if (preview_win_->GetWindowStatus() == STATU_PHOTO) {
        // goto slow record
        preview_win_->keyProc(SDV_KEY_POWER, SHORT_PRESS);
    }
    if(isRecordStart){
        db_msg("RemoteSwitchSlowRecord off");
        if(osd_flag){
            OsdManager::get()->DettchVencRegion();
            OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
            OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
        }
        if(this->RecordHandle(CAM_NORMAL_0,0) < 0)
        {
            db_error("[debug_jaosn]:RemoteSwitchSlowRecord stop failed");
        }
        StreamRecordHandle(0);
        usleep(500*1000);
        StreamRecordHandle(1);
        StorageManager::GetInstance()->SetDiskFullNotifyFlag(false);
       // OsdManager::get()->DettchVencRegion();
    }else{
        db_msg("RemoteSwitchSlowRecord on");
        // if (re_init_camera_slowrecod == false){
            //close stream recorder
            StreamRecordHandle(0);
            ReConfigResolution(SLOW_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
            StreamRecordHandle(1);
        // }
        if(this->RecordHandle(CAM_NORMAL_0,1) < 0)
        {
            db_error("[debug_jaosn]: RemoteSwitchSlowRecord failed");
            return -1;
        }
        GetThumbVideoFileName(CAM_NORMAL_0);
        usleep(500*1000);
        cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->addOutputFormatAndOutputSink();
        if(osd_flag){
            OsdManager::get()->AttchVencRegion();
            OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
            OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
        }
        StorageManager::GetInstance()->SetDiskFullNotifyFlag(true);
    }
    return 0;
}



int PreviewPresenter::RemoteTakePhoto()
{
    int ret = 0;
    if(re_init_camera_photo == false){
        //1. stop stream record
        StopRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]);
        //2. reconfig camera 4k (tmp) may be use max size for nv21
        camera_map_[CAM_NORMAL_0]->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
        camera_map_[CAM_NORMAL_0]->ShowPreview();
        //3. start stream record
        StartRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN],CAM_NORMAL_0);
        //4. take pic
        remoteTakePicture();
    }else{
        remoteTakePicture();
    }
    return 0;
}


int PreviewPresenter::GetRemotePhotoInitState(bool & init_state)
{
    init_state = re_init_camera_photo;
    return 0;
}

int PreviewPresenter::setWifiFlagtoRecorder(const CameraID &p_nCamId,bool flag)
{
    map<CameraID, Camera*>::iterator iter = camera_map_.find(p_nCamId);
    if (iter == camera_map_.end())
    {
        db_error("no camera can be used");
        return -1;
    }
    Recorder *sRecoder;
    sRecoder = cam_rec_map_[p_nCamId][REC_S_SUB_CHN];
    sRecoder->setWifiFlag(flag);
    return 0;
}

int PreviewPresenter::GetThumbVideoFileName(const CameraID &p_nCamId)
{
#ifdef SUB_RECORD_SUPPORT
    map<CameraID, Camera*>::iterator iter = camera_map_.find(p_nCamId);
    if (iter == camera_map_.end())
    {
        db_error("no camera can be used");
        return -1;
    }
    Recorder *mRecoder;
    Recorder *sRecoder;
    RecorderParam mParam;
    RecorderParam sParam;
    mRecoder = cam_rec_map_[p_nCamId][REC_MAIN_CHN];
    mRecoder->GetParam(mParam);
    sRecoder = cam_rec_map_[p_nCamId][REC_S_SUB_CHN];
    sRecoder->GetParam(sParam);
    sParam.MF = mParam.MF;
    sParam.cycle_time = mParam.cycle_time;
    if(isWifiSoftEnable == false){
        sParam.delay_fps  = mParam.delay_fps;
    }
    sRecoder->SetParam(sParam);
#endif
    return 0;
}

int PreviewPresenter::StreamRecordHandle(int val)
{
    db_msg("StreamRecordHandle : the val is %d",val);
    if(val){
		db_error("rec 004");
       StartRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN],CAM_NORMAL_0);
    }else{
       StopRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]);
    }
    return 0;
}

int PreviewPresenter::HandleGUIMessage(int msg, int val)
{
    int ret = 0;
    MSG_TYPE tmp_msg;
    db_info("msg: %d, val: %d", msg, val);

    if (disp_switch_mutex_.try_lock() == false) {
        db_warn("disp switching...do not response gui message");
        return 0;
    }
    disp_switch_mutex_.unlock();

    switch(msg) {
        case PREVIEW_RECORD_BUTTON:  //normal record
            {
                Window::GlobalKeyBlocker global_key_blocker;
                Layer *layer = Layer::GetInstance();
                // enable ui layer alpha to show preview, set 255 can disable it
                layer->SetLayerAlpha(LAYER_UI, 150);
                // if(re_init_camera_photo || re_init_camera_slowrecod)
                // {
                // ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
                // }
                db_warn("last win status: %d, cur win status: %d", last_win_statu_msg, win_statu_msg);
                if (val == 1) {
                    if ((win_statu_msg == MSG_CHANG_STATU_PREVIEW && win_statu_msg != last_win_statu_msg) ||
                            re_init_camera_slowrecod == true) {
                        ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
                    }
                }
                last_win_statu_msg = win_statu_msg;

                if (!val && osd_flag) {
                    OsdManager::get()->DettchVencRegion();
                    OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
#ifdef SUB_RECORD_SUPPORT
                    OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
#endif
					if( BackCameraCheck::GetInstance()->IsBackCameraConnect() )
					{
	                    OsdManager::get()->stopTimeOsd(CAM_UVC_1,REC_U_720P30FPS);
					}
                }

                if(RecordHandle(CAM_NORMAL_0, val) < 0)
                {
                    db_error("[debug_jaosn]: start record failed");
                    break;
                }

				if( BackCameraCheck::GetInstance()->IsBackCameraConnect() )
				{
					if(RecordHandle(CAM_UVC_1, val) < 0)
	                {
	                    db_error("[debug_jaosn]: start record failed");
	                    break;
	                }
				}

                if(val == 1){
                    // ret = cdx_sem_down_timedwait(&mTakePhotoOver, 1000);
                    // if(ETIMEDOUT == ret)
                    // {
                    // db_error("PREVIEW_RECORD_BUTTON error! wait render start timeout");
                    // }
                    GetThumbVideoFileName(CAM_NORMAL_0);
                }
                //start stream recorder
#ifdef SUB_RECORD_SUPPORT
                StreamRecordHandle(val);
#endif
                if(val && osd_flag){
                    usleep(100*1000);
                    OsdManager::get()->AttchVencRegion();
                    OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
#ifdef SUB_RECORD_SUPPORT
                    OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
#endif
					if( BackCameraCheck::GetInstance()->IsBackCameraConnect() )
                    	OsdManager::get()->startTimeOsd(CAM_UVC_1,REC_U_720P30FPS);
                }
                StorageManager::GetInstance()->SetDiskFullNotifyFlag(val);
            }
            break;
        case PREVIEW_SHOTCUT_BUTTON:  //take picture
            if (win_statu_msg == MSG_CHANG_STATU_PHOTO && win_statu_msg != last_win_statu_msg) {
                camera_map_[CAM_NORMAL_0]->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
                camera_map_[CAM_NORMAL_0]->ShowPreview();
            }
            last_win_statu_msg = win_statu_msg;
            this->TakePicButtonHandler(CAM_NORMAL_0, val);
            break;
        case MSG_PREVIEW_SLOW_RECORD: //slow record
            if (val == 1) {
                if (win_statu_msg == MSG_CHANG_STATU_SLOWREC && win_statu_msg != last_win_statu_msg) {
                    ReConfigResolution(SLOW_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
                }
            }
            last_win_statu_msg = win_statu_msg;

            if (!val && osd_flag) {
                OsdManager::get()->DettchVencRegion();
                OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
                OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
            }

            if(RecordHandle(CAM_NORMAL_0, val) < 0)
            {
                db_error("[debug_jaosn]: start slowrecord failed");
                break;
            }
            if(val == 1){
                // ret = cdx_sem_down_timedwait(&mTakePhotoOver, 1000);
                // if(ETIMEDOUT == ret)
                // {
                    // db_error("MSG_PREVIEW_SLOW_RECORD error! wait render start timeout");
                // }
                GetThumbVideoFileName(CAM_NORMAL_0);
            }
            //start stream recorder
            StreamRecordHandle(val);
            if(val && osd_flag){
                usleep(100*1000);
                OsdManager::get()->AttchVencRegion();
                OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
                OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
            }
            StorageManager::GetInstance()->SetDiskFullNotifyFlag(val);
            break;
        case PREVIEW_TIME_TO_TAKEPIC:
            db_msg("PREVIEW_TIME_TO_TAKEPIC ==============zhanglm");
            this->TimeTakePic();
            break;
        case PREVIEW_GO_PLAYBACK_BUTTON:
            {
                deinit_flag_ = false;
                last_win_statu_msg = win_statu_msg;
                if(val == STATU_PHOTO)
                {
                    win_statu_msg = MSG_CHANG_STATU_PHOTO;
		      db_msg("[debug_zhb]: @@@@@@@@@@@@@@@@ MSG_CHANG_STATU_PHOTO @@@@@@@@@@");
                    // camera_map_[CAM_NORMAL_0]->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
                    // camera_map_[CAM_NORMAL_0]->ShowPreview();
                }
                else if(val == STATU_SLOWRECOD)
                {
                    isTimeTakeTimerStart = false;
                    isAutoTakeTimerStart = false;
                    win_statu_msg = MSG_CHANG_STATU_SLOWREC;
                    this->StopAutoLoopCountdown();
		     db_msg("[debug_zhb]: @@@@@@@@@@@@@@@@ MSG_CHANG_STATU_SLOWREC @@@@@@@@@@");
                    // ReConfigResolution(SLOW_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
                }
                else if(val == STATU_PLAYBACK)
                {
                    db_msg("[debug_zhb]: @@@@@@@@@@@@@@@@ MSG_CHANG_STATU_PLAYBACK @@@@@@@@@@");
                    win_statu_msg = MSG_CHANG_STATU_PLAYBACK;
                }
                else
                {
                   db_msg("[debug_zhb]: @@@@@@@@@@@@@@@@ MSG_CHANG_STATU_PREVIEW @@@@@@@@@@");
                    win_statu_msg = MSG_CHANG_STATU_PREVIEW;
                    // ReConfigResolution(NORMAL_RECORD, camera_map_[CAM_NORMAL_0], cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
                }
                this->Notify((MSG_TYPE)win_statu_msg);
            }
            break;
	case PREVIEW_TO_SETTING_BUTTON://add by zhb
		{
			db_msg("[debug_zhb]-----PREVIEW_TO_SETTING_BUTTON");
			//change status bar bottom button
			this->Notify((MSG_TYPE)MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR_BOTTOM);
			// change status_bar icon status 
			this->Notify((MSG_TYPE)MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR);

		}break;
        case PREVIEW_AUDIO_BUTTON:
            //audio_record_ = val;
            if(isAllowTakePIC){
                this->RecordAudioSwitch(CAM_NORMAL_0);
            }
            if(audio_record_){
                tmp_msg = (MSG_TYPE)MSG_RECORD_AUDIO_ON;
		  preview_win_->ShowPromptBox(PROMPT_BOX_RECORD_SOUND_OPEN,3);
            }else{
                tmp_msg = (MSG_TYPE)MSG_RECORD_AUDIO_OFF;
		  preview_win_->ShowPromptBox(PROMPT_BOX_RECORD_SOUND_CLOSE,3);
            	}
            this->Notify(tmp_msg);

            break;
#if 0
        case PREVIEW_WIFI_BUTTON:
            if(!wifi_statu_){
                wifi_statu_ = true;
                tmp_msg = (MSG_TYPE)MSG_WIFI_ENABLE;
                this->GetWifiInfo();
            }else{
                wifi_statu_ = false;
                tmp_msg = (MSG_TYPE)MSG_WIFI_DISABLED;
            }
            this->Notify(tmp_msg);

            break;
#endif
        case PREVIEW_SET_DIGHTZOOM_BUTTON:
            db_msg("[fangjj] --PREVIEW_SET_DIGHTZOOM_BUTTON--val=:[%d] \n",val);
            this->SetDightZoomButtonHandler(CAM_NORMAL_0, val);
            this->SetDightZoomValue(val);
            break;
        case MSG_SYSTEM_SHUTDOWN:
            {
                DoSystemShutdown();
            }
            break;
        case PREVIEW_WIFI_SWITCH_BUTTON:{
            int ret = 0;
            if(isWifiSoftEnable){
                MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
                menuconfiglua->SetMenuIndexConfig(SETTING_WIFI_SWITCH,0);
                // 关闭WIFI时, 确保所有录像都停止
                db_info("wifi disabled, we will stop all record");
                StopAllRecord();
                isWifiSoftEnable = false;
                setWifiFlagtoRecorder(CAM_NORMAL_0,isWifiSoftEnable);
                ret = WifiSOftApDisable();
                if(ret < 0){
                   db_error("[error]:WifiSOftApDisable filed");
                }
            }else{
                ret = WifiSOftApEnable();
                if(ret < 0)
                {
                    //may be close the wifi info tip
                    db_error("[error]:wifi enabled filed");
                    break;
                }
                this->GetWifiInfo();
                db_msg("StartRecord is over");
                isWifiSoftEnable = true;
                MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
                menuconfiglua->SetMenuIndexConfig(SETTING_WIFI_SWITCH,1);
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
            }
          }
            break;
        case PREVIEW_WIFI_DIALOG_HIDE:
            this->Notify(MSG_TYPE(MSG_WIFI_CLOSE));
            break;
        case PREVIEW_CONFIRM_FORMAT:
            if(val)
            {
                StorageManager::GetInstance()->Format();
            }
            break;
        case MSG_STREAM_RECORD_SWITCH:{
            switch(val){
                case 0:
                    db_msg("STREAM_RECORD_SWITCH is %d",val);
                    OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
                    StopRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]);
                    break;
                case 1:
                    db_error("STREAM_RECORD_SWITCH is %d",val);
                    StartRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN],CAM_NORMAL_0);
                    OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
                    break;
            }
         }
            break;
        case MSG_REMOTE_CLIENT_DISCONNECT:
            {
                if (isWifiSoftEnable) {
                    // 如果TUTK连接断开(真正断开后有1分钟超时时间), 则停止当前录像
                    db_info("tutk remote disconnect, we will stop all record");
                    StopAllRecord();
                    // 重启流录像, 以便在不重启wifi的情况下, 手机app能直接再次连接
                    int ret = StartRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN],CAM_NORMAL_0);
                    if(ret < 0) {
                        db_error("[error]:start subchannel recorder filed");
                        break;
                    }
                    setWifiFlagtoRecorder(CAM_NORMAL_0, true);
                    if (osd_flag) {
                        OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
                    }
                }
            }
            break;
        case USB_DISCONNECT:
            if (mode_ == USB_MODE_MASS_STORAGE)
            {
                mode_ = NORMAL_MODE;
                StorageManager::GetInstance()->UMountFromPC();
                if( win_mg_->GetCurrentWinID()== WINDOWID_PREVIEW)
                    camera_map_[CAM_NORMAL_0]->ShowPreview();
            }
            else if (mode_ == USB_MODE_UVC)
            {
                mode_ = NORMAL_MODE;
                int retval = UVCModeStop();
                if( retval < 0 ){
                    db_msg("UVCModeStop failed\n");
                }
                retval = UVCModeDeInit();
                if( retval < 0 ){
                    db_msg("UVCModeDeInit failed\n");
                    break;
                }
                map<CameraID, Camera*>::iterator iter = camera_map_.find(CAM_NORMAL_0);
                if (iter != camera_map_.end())
                {
                    camera_map_[CAM_NORMAL_0]->ReInitCameraChn(m_cam_Chnparam);
                    if( win_mg_->GetCurrentWinID()== WINDOWID_PREVIEW)
                        camera_map_[CAM_NORMAL_0]->ShowPreview();
                }
                win_mg_->ResetConfigLua();
            }
            else
            {
                mode_ = NORMAL_MODE;
                m_usb_attach_status = false;
                Notify((MSG_TYPE)MSG_USB_DISCONNECT);
            }
            break;
        case USB_MASS_STORAGE:
            mode_ = USB_MODE_MASS_STORAGE;
            if( win_mg_->GetCurrentWinID()== WINDOWID_PREVIEW)
                camera_map_[CAM_NORMAL_0]->HidePreview();

            StopRecord(cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]);
            StopRecord(cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]);

            StorageManager::GetInstance()->MountToPC();
            break;
        case USB_UVC:
        {
            mode_ = USB_MODE_UVC;
            camera_map_[CAM_NORMAL_0]->GetCameraChnCfg(m_cam_Chnparam);
            int retval = UVCModeInit();
            if( retval < 0 ){
                db_msg("UVCModeInit failed\n");
                break;
            }
            retval = UVCModeStart();
            if( retval < 0 ){
                db_msg("UVCModeStart failed\n");
                break;
            }
            break;
        }
        case PREVIEW_SET_RECORD_MUTE:
            SetRecordMute(val);
            break;
        case PREVIEW_LOWPOWER_SHUTDOWN:
            {
                if (!lowpower_shutdown_processing_) {
                    lowpower_shutdown_processing_ = true;
                    create_timer(this, &lowpower_shutdown_timer_id_, LowPowerShutdownTimerHandler);
                    set_one_shot_timer(5, 0, lowpower_shutdown_timer_id_);
                }
            }
            break;
		case PREVIEW_SWITCH_LAYER:
		{
			CameraPreviewType newPreviewType, oldPreviewType;
			Layer::GetInstance()->GetPreviewType(&oldPreviewType);	
			
			if( oldPreviewType == ONLY_FRONT || oldPreviewType == FRONT_BACK)
			{
				if(BackCameraCheck::GetInstance()->IsBackCameraConnect())
					newPreviewType = BACK_FRONT;
				else
					newPreviewType = ONLY_FRONT;
			}
			else if( oldPreviewType == ONLY_BACK || oldPreviewType == BACK_FRONT )
			{
				if(BackCameraCheck::GetInstance()->IsBackCameraConnect())
					newPreviewType = FRONT_BACK;
				else
					newPreviewType = ONLY_FRONT;
			}

			if( oldPreviewType != newPreviewType )
			{
				Layer::GetInstance()->SetPreviewType(newPreviewType);
				SwitchLayer(oldPreviewType, newPreviewType);
			}
			break;
		}
        default:
            db_msg("there is no this button defined");
            break;
    }

    return ret;
}

void PreviewPresenter::SetRecordMute(bool value)
{
    cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]->SetMute(value);
#ifdef SUB_RECORD_SUPPORT
    cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->SetMute(value);
#endif
}

void PreviewPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

#ifdef ENABLE_RTSP
void* PreviewPresenter::RtspThreadLoop(void *context)
{
    PreviewPresenter *self = reinterpret_cast<PreviewPresenter*>(context);

    prctl(PR_SET_NAME, "RtspThreadLoop", 0, 0, 0);

    if (self->rtsp_flag_) {
        db_debug("run rtsp server");
        self->rtsp_server_->Run();
    }

    return NULL;
}
#endif

#ifdef ENABLE_RTSP
static void RtspOnClientConnected(void *context)
{
    db_info("rtsp client connected, encode I Frame immediately");

    Recorder *recorder = reinterpret_cast<Recorder*>(context);

    recorder->ForceIFrame();
}
#endif

void PreviewPresenter::MediaInit()
{
    CameraFactory *camera_factory = CameraFactory::GetInstance();
    for (RecorderGroup::iterator grp_iter = rec_group_.begin(); grp_iter != rec_group_.end(); grp_iter++) {
		Camera *cam = NULL;
		if( CAM_NORMAL_0 == grp_iter->first)
		{
	        // create camera
	        if (g_camera_front == NULL) {
	            cam = camera_factory->CreateCamera(grp_iter->first);
	        } else {
	            cam = g_camera_front;
	        }
		}
        cam->Attach(this);
        camera_map_.insert(make_pair(grp_iter->first, cam));

        map<RecorderType, Recorder*> rec_instance_grp;

        // create rec_instance_grp
        RecorderIDSet rec_id_set = grp_iter->second;
        for (RecorderIDSet::iterator set_iter = rec_id_set.begin(); set_iter != rec_id_set.end(); set_iter++) {
            // create recorder
            RecorderFactory *rec_factory = RecorderFactory::GetInstance();
            Recorder *recorder = rec_factory->CreateRecorder(set_iter->rec_type, cam);
            assert(recorder != NULL);
            recorder->Attach(this);
            recorder->SetStreamSenderType(set_iter->sender_type);
            if(set_iter->rec_model == REC_STREAM){
               // register for tutk encode callback
               recorder->SetEncodeDataCallback(PreviewPresenter::EncodeDataCallback, this);
            }

            rec_instance_grp.insert(make_pair(set_iter->rec_type, recorder));
        }

        cam_rec_map_.insert(make_pair(grp_iter->first, rec_instance_grp));
    }
}

int PreviewPresenter::BackCameraMediaInit()
{
	for (RecorderGroup::iterator grp_iter = rec_group_.begin(); grp_iter != rec_group_.end(); grp_iter++)
	{
		if( grp_iter->first != CAM_UVC_1 )
			continue;

		Camera *cam = NULL;
		CameraFactory *camera_factory = CameraFactory::GetInstance();

		if (g_camera_back == NULL)
		{
			g_camera_back = CameraFactory::GetInstance()->CreateCamera(CAM_UVC_1);
		}
		cam = g_camera_back;

        cam->Attach(this);
        camera_map_.insert(make_pair(grp_iter->first, cam));

        map<RecorderType, Recorder*> rec_instance_grp;
        // create rec_instance_grp
        RecorderIDSet rec_id_set = grp_iter->second;
        for (RecorderIDSet::iterator set_iter = rec_id_set.begin(); set_iter != rec_id_set.end(); set_iter++) {
            // create recorder
            RecorderFactory *rec_factory = RecorderFactory::GetInstance();
            Recorder *recorder = rec_factory->CreateRecorder(set_iter->rec_type, cam);
            assert(recorder != NULL);
            recorder->Attach(this);
            recorder->SetStreamSenderType(set_iter->sender_type);
            if(set_iter->rec_model == REC_STREAM){
               // register for tutk encode callback
               recorder->SetEncodeDataCallback(PreviewPresenter::EncodeDataCallback, this);
            }
            rec_instance_grp.insert(make_pair(set_iter->rec_type, recorder));
        }

        cam_rec_map_.insert(make_pair(grp_iter->first, rec_instance_grp));

	}

	return 0;
}

int PreviewPresenter::DeviceModelInit()
{
    db_msg("rec and play device model init");

    Layer::GetInstance()->Attach(this);
    USBGadget::GetInstance()->Attach(this);

    NetManager::GetInstance()->Attach(this);
    PowerManager::GetInstance()->Attach(this);
#ifdef BT_SUPPORT
    BlueToothController *bt_ctrl = BlueToothController::GetInstance();
    bt_ctrl->Attach(this);
    bt_ctrl->InitBlueToothDevice();
    bt_ctrl->EnableBlueToothDevice();
#endif

    if (mode_ == NORMAL_MODE)
    {
    	NormalInit();
		if(BackCameraCheck::GetInstance()->IsBackCameraConnect() )
			BackCameraInit();
    }
    else if (mode_ > USB_MODE)
        UVCModeInit();

    status_ = MODEL_INITED;

    return 0;
}

int PreviewPresenter::CheckStorage(void)
{
    int ret = 0;
    int status = 0;
    StorageManager *sm = StorageManager::GetInstance();
    // update storage status
    status = sm->GetStorageStatus();
    if (status == MOUNTED)
    {
        storage_status_ = STORAGE_OK;
    }
    else if (status == STORAGE_LOOP_COVERAGE )
    {
        if (sm->FreeSpace() < 0) {
            storage_status_ = STORAGE_NOT_OK;
            return -1;
        }
        storage_status_ = STORAGE_OK;
    }
    else
    {
        storage_status_ = STORAGE_NOT_OK;
        db_error("sd card is not ok, can not start record");
        ret = -1;
    }

    return ret;
}


int PreviewPresenter::DeviceModelDeInit()
{
    // Layer::GetInstance->Detach(this);
    // NetManager::GetInstance()->Detach(this);
    // //BlueToothController::GetInstance()->Detach(this);
    // USBGadget::GetInstance()->Detach(this);
    // PowerManager::GetInstance()->Detach(this);
    if (mode_ == NORMAL_MODE){
        NormalDeInit();
    }else if (mode_ > USB_MODE){
        UVCModeDeInit();
    }
    // sync record file data to disk
    sync();
    status_ = MODEL_UNINIT;
    return 0;
}

// 底层通知回调
void PreviewPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
#if 0
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
#endif
    if (msg != MSG_CPU_TEMP_NORMAL)
        db_msg("handle msg: %d", msg);
    MSG_TYPE tmp_msg;
    switch (msg) {
		case MSG_PREPARE_TO_SUSPEND:
			if (status_ == MODEL_INITED){// && deinit_flag_) {
				this->DeviceModelDeInit();
			}
			Notify(msg);
			break;
		case MSG_START_RESUME:
			Notify(msg);
			usleep(200*1000);
			if (status_ != MODEL_INITED) {
        		this->DeviceModelInit();
    		}		
			break;
        case MSG_CAMERA_TAKEPICTURE_ERROR:
        case MSG_CAMERA_TAKEPICTURE_FINISHED: {
            isAllowTakePIC = true;
            if(isContinueTakePicMode || isAutoTimeTakePicMode){
                if(isWifiSoftEnable){
                    tutk_connector_->SendCmdData(IOTYPE_USER_SDV_CONTINUE_TAKE_PHOTO_FINISHED_RESP,0,1,NULL);
            }
            }
            if(isTimeTakePicMode){
                isTimeTakeTimerStart = false;
            }
        }
	  this->Notify(msg);
            break;
        case MSG_SHOW_HDMI_MASK:
        case MSG_HIDE_HDMI_MASK:
            Notify(msg);
            break;
        case MSG_HDMI_PLUGIN:
        case MSG_TVOUT_PLUG_IN:
            if (!PlugInHandle(msg))
                this->Notify(msg);
            break;
		case MSG_IMPACT_HAPPEN:
			{
				if( !mFrontCamRecordStartStatus )
				{
					if(RecordHandle(CAM_NORMAL_0, 1) < 0)
						break;
					OsdManager::get()->startTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
					
				}
				cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]->setSosHappened(true);

				if( BackCameraCheck::GetInstance()->IsBackCameraConnect() )
				{
					if( !mBackCamRecordStartStatus )
					{
						if(RecordHandle(CAM_UVC_1, 1) < 0)
							break;
						OsdManager::get()->startTimeOsd(CAM_UVC_1,REC_U_720P30FPS);
					}
					cam_rec_map_[CAM_UVC_1][REC_U_720P30FPS]->setSosHappened(true);
				}
			}
			break;
        case MSG_HDMI_PLUGOUT:
        case MSG_TVOUT_PLUG_OUT:
            if (!PlugOutHandle(msg))
                this->Notify(msg);
            break;
        case MSG_STORAGE_MOUNTED:
            if(isWifiSoftEnable){
            tutk_connector_->SendCmdData(IOTYPE_USER_IPCAM_SET_UNMOUNT_DISK_RESP,0,1,NULL);
            }
            this->Notify(msg);
            break;
        case MSG_STORAGE_UMOUNT:
            if(isWifiSoftEnable){
            db_error("[debug_jaosn]:SendCmdData to apk");
            tutk_connector_->SendCmdData(IOTYPE_USER_IPCAM_SET_UNMOUNT_DISK_RESP,0,0,NULL);
            remoteTakePicture();
            }
			//by hero ****** rec led long light
            LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
            this->Notify(msg);
            break;
        case MSG_RECORD_START:
            if (!isRecordStart) {
                isRecordStart = true;
                this->Notify(msg);
            }
            break;
        case MSG_RECORD_STOP:
            if (isRecordStart) {
                isRecordStart = false;
                this->Notify(msg);
            }
            break;
        case MSG_AUDIO_RECORD_CHANGE:
            if(this->GetAudioStatus(CAM_NORMAL_0)){
                 tmp_msg = (MSG_TYPE)MSG_RECORD_AUDIO_ON;
            }else{
                 tmp_msg = (MSG_TYPE)MSG_RECORD_AUDIO_OFF;
            }
            this->Notify(tmp_msg);
            break;
        case MSG_ETH_DISCONNECT:{
            NetManager::NetLinkType netlink_type;
            nm_->GetNetLinkType(netlink_type);

            db_info("current netlink type: %d", netlink_type);
            if (netlink_type == NetManager::NETLINK_WIFI_STA || netlink_type == NetManager::NETLINK_WIFI_SOFTAP)
                break;
            #if 0
            RemoteServerStop();
            RemoteServerDeInit();
            #endif
            }
            break;
        case MSG_ETH_SWITCH_DONE:
        case MSG_WIFI_SWITCH_DONE:
        case MSG_SOFTAP_SWITCH_DONE:
            #if 0
            RemoteServerStop();
            RemoteServerDeInit();
            sleep(1);
            RemoteServerInit();
            RemoteServerStart();
            #endif
            break;
        case MSG_CAMERA_CONTINOUS_ON:
            isContinueTakePicMode = true;
            isAutoTimeTakePicMode = false;
            isTimeTakePicMode = false;
	     tmp_msg = (MSG_TYPE)MSG_PHOTO_DRAMASHOT_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_CAMERA_CONTINOUS_OFF:
            isContinueTakePicMode = false;
            tmp_msg = (MSG_TYPE)MSG_PHOTO_DRAMASHOT_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_TIME_TAKE_PIC_ON:
            isContinueTakePicMode = false;
            isAutoTimeTakePicMode = false;
            isTimeTakePicMode = true;
	     tmp_msg = (MSG_TYPE)MSG_PHOTO_TIMED_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_TIME_TAKE_PIC_OFF:
            isTimeTakePicMode = false;
	     tmp_msg = (MSG_TYPE)MSG_PHOTO_TIMED_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_AUTO_TIME_TAKE_PIC_ON:
            isContinueTakePicMode = false;
            isTimeTakePicMode = false;
            isAutoTimeTakePicMode = true;
	     tmp_msg = (MSG_TYPE)MSG_PHOTO_AUTO_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_AUTO_TIME_TAKE_PIC_OFF:
            isAutoTimeTakePicMode = false;
	     tmp_msg = (MSG_TYPE)MSG_PHOTO_AUTO_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_USB_HOST_CONNECTED:
            if(!isAllowTakePIC || isRecordStart){
                db_debug("current is take pic and not finish");
                break;
            }else{
                if(isTimeTakeTimerStart || isAutoTakeTimerStart){
                    db_debug("current is excute Time take photo or auto time take photo");
                    break;
                }
            }
            if(m_usb_attach_status == false && PowerManager::GetInstance()->getUsbconnectStatus())
            {
                this->Notify(msg);
                m_usb_attach_status= true;
            }
            else
            {
                db_msg("invalid usb connect message, attach_status[%d], UsbconnectStatus[%d]",m_usb_attach_status,PowerManager::GetInstance()->getUsbconnectStatus());
            }
            break;
        case MSG_USB_HOST_DETACHED:
            sleep(1); // sleep 1s to wait ac connect status check finish!
            if(m_usb_attach_status && !PowerManager::GetInstance()->getUsbconnectStatus() && !PowerManager::GetInstance()->getACconnectStatus())
            {
                this->Notify(msg);
                m_usb_attach_status = false;
            }
            else
            {
                db_msg("invalid usb disconnect message, attach_status[%d], UsbconnectStatus[%d] ACconnectStatus[%d]",m_usb_attach_status,PowerManager::GetInstance()->getUsbconnectStatus(),PowerManager::GetInstance()->getACconnectStatus());
            }
            if (mode_ == NORMAL_MODE) {
                PowerManager::GetInstance()->ResetUDC();
            }
            break;
        case MSG_TAKE_THUMB_PIC:
            db_msg("[debug_jason]:MSG_TAKE_THUMB_PIC for clc record");
            TakePicforVideothumb(cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN],CAM_NORMAL_0);
		    break;
        case MSG_TAKE_THUMB_VIDEO:
            GetThumbVideoFileName(CAM_NORMAL_0);
            break;

		case MSG_TAKE_THUMB_BACKCAMERA_PIC:
			db_msg("[debug_jason]:MSG_TAKE_THUMB_PIC for clc record");
			TakePicforVideothumb(cam_rec_map_[CAM_UVC_1][REC_U_720P30FPS],CAM_UVC_1);
			break;
		case MSG_TAKE_THUMB_BACKCAMERA_VIDEO:
			GetThumbVideoFileName(CAM_UVC_1);
			break;

        case  MSG_RECORD_LOOP_CHANGE:
            tmp_msg = (MSG_TYPE)MSG_RECORD_LOOP_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_RECORD_FILE_DONE:
            this->Notify(msg);
            break;
        case MSG_NEED_SWITCH_SUB_FILE:
            cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->SwitchFileNormal();
            break;
        case MSG_NEED_STOP_SUB_RECORD:
            cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->StopRecord();
            break;
        case MSG_RECORD_TIMELAPSE_CHANGE:
            tmp_msg = (MSG_TYPE)MSG_RECORD_TIMELAPSE_VALUE;
            this->Notify(tmp_msg);
            break;
        case MSG_TAKE_PIC_SIGNAL:
            cdx_sem_up(&mTakePhotoOver);
            db_msg("[debug_jaosn]: take picture is over notify start camera");
            break;
        case MSG_STORAGE_FS_ERROR:
            this->Notify(msg);
            break;
        case MSG_CAMERA_SLOW_RESOULATION:
            re_init_camera_photo = false;
            re_init_camera_slowrecod = true;
            break;
        case MSG_CAMERA_VIDEO_RESOLUTION:
            re_init_camera_photo = false;
            re_init_camera_slowrecod = false;
            break;
        case MSG_CAMERA_SET_PHOTO:
            re_init_camera_photo = true;
            re_init_camera_slowrecod = false;
            break;
        case MSG_CAMERA_ENABLE_OSD:
            osd_flag = true;
            break;
        case MSG_CAMERA_DISABLE_OSD:
            osd_flag = false;
            break;
        case MSG_BATTERY_FULL:
        case MSG_BATTERY_LOW:
            this->Notify((MSG_TYPE)msg);
            break;
        case MSG_SOFTAP_DISABLED:
            this->Notify((MSG_TYPE)msg);
            break;
        case MSG_DATABASE_IS_FULL:
            // NOTE: be carefull, ui window maybe have not be created yet
            this->Notify(msg);
        case MSG_STORAGE_IS_FULL:
            camera_map_[CAM_NORMAL_0]->CancelContinuousPicture();
            // NOTE: be carefull, ui window maybe have not be created yet
            StopAutoLoopCountdown();
            isAutoTakeTimerStart = false;
			//by hero ****** rec led long light
            LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
            if(isWifiSoftEnable){
            tutk_connector_->SendCmdData(IOTYPE_USER_SDV_TFCARD_FULL_RESP,0,1,NULL);
            }
            break;
         case MSG_SYSTEM_POWEROFF:
            {
                DoSystemShutdown();
            }
            break;
         case MSG_CAMERA_ON_ERROR:
            {
                Notify(msg);
                CameraOnError();
            }
            break;
		 case MSG_BACKCAMERA_CONNECT:
		 {
			Layer::GetInstance()->SetPreviewType(FRONT_BACK);
			BackCameraInit();
			break;
		 }
		 case MSG_BACKCAMERA_DISCONNECT:
		 {
			CameraPreviewType Previewtype;
			Layer::GetInstance()->GetPreviewType(&Previewtype);
			Layer::GetInstance()->SetPreviewType(ONLY_FRONT);
			SwitchLayer(Previewtype, ONLY_FRONT);

    		OsdManager::get()->stopTimeOsd(CAM_UVC_1,REC_U_720P30FPS);
    		StopRecord(cam_rec_map_[CAM_UVC_1][REC_U_720P30FPS]);
			BackCameraDeInit();
			break;
		 }

		case MSG_PM_RECORD_STOP:
			HandleGUIMessage(PREVIEW_RECORD_BUTTON,0);
		 	break;

		case MSG_PM_RECORD_START:
			HandleGUIMessage(PREVIEW_RECORD_BUTTON,1);
			break;
        default:
            break;
    }

    //this->Notify(msg);
}

#ifdef ENABLE_RTSP
string PreviewPresenter::GetRtspUrl(const GroupID &grp_id) {
    RecorderIDSet rec_id_set = rec_group_[grp_id];
    RecorderIDSet::iterator iter;
    string demo_url;

    for (iter = rec_id_set.begin(); iter != rec_id_set.end(); iter++) {
        Recorder *recorder = cam_rec_map_[iter->cam_id][iter->rec_type];
        StreamSenderMap::iterator r_iter = stream_sender_map_.find(recorder);
        if (r_iter != stream_sender_map_.end()) {
            string url = r_iter->second->GetUrl();
            db_msg("%s", url.c_str());
            if ( (iter->rec_type <= REC_S_SUB_CHN && (iter->sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP)
                 || rec_id_set.size() == 1) {
                demo_url = url;
            }
        }
    }

    return demo_url;
}

int PreviewPresenter::CreateRtspServer()
{
    rtsp_server_ = RtspServer::GetInstance();

    string ip;

    if (nm_->GetNetDevIp("eth0", ip) < 0) {
        if (nm_->GetNetDevIp("wlan0", ip) < 0) {
            ip = "0.0.0.0";
            db_warn("no activity net device found, set rtsp server ip to '%s", ip.c_str());
//            preview_win_->ShowDialogWithMsg("net_error", 2);
        }
    }

    db_debug("create rtsp server on: %s", ip.c_str());

    return rtsp_server_->CreateServer(ip);
}

int PreviewPresenter::CreateRtspStreamSender()
{
    if (rtsp_flag_) {
        for (RecorderGroup::iterator grp_iter = rec_group_.begin(); grp_iter != rec_group_.end(); grp_iter++) {
            // create camera
            Camera *cam = camera_map_[grp_iter->first];

            RecorderIDSet rec_id_set = grp_iter->second;
            for (RecorderIDSet::iterator set_iter = rec_id_set.begin(); set_iter != rec_id_set.end(); set_iter++) {
                // create rtsp stream sender
                if (set_iter->rec_model == REC_STREAM) {
                    // set callback for stream recorder
                    Recorder *recorder = cam_rec_map_[grp_iter->first][set_iter->rec_type];
                    recorder->SetEncodeDataCallback(PreviewPresenter::EncodeDataCallback, this);

                    std::stringstream ss;
                    RtspServer::StreamSender *stream_sender = NULL;

                    ss << "ch" << cam->GetCameraID() << set_iter->rec_type;
                    stream_sender = rtsp_server_->CreateStreamSender(ss.str());
                    stream_sender->SetSenderCallback(&RtspOnClientConnected, recorder);
                    db_info("url: %s", stream_sender->GetUrl().c_str());

                    if (stream_sender != NULL) {
                        stream_sender_map_.insert(make_pair(recorder, stream_sender));
                        MediaControlImpl *media_ctrl = static_cast<MediaControlImpl *>(dev_adapter_->media_ctrl_);
                        media_ctrl->SetStreamSenderInstance(stream_sender_map_);
                    }
                }
            }
        }
    }

    return 0;
}

int PreviewPresenter::DestroyRtspServer()
{

        if (!rtsp_flag_) return 0;

        if (rtsp_server_ == NULL) {
            db_debug("rtsp server already destroyed, no need to stop");
            return 0;
        }
        rtsp_server_->Stop();
        db_debug("rtsp server stoped");

        map<Recorder*, RtspServer::StreamSender*>::iterator sender_iter;
        for (sender_iter = stream_sender_map_.begin(); sender_iter != stream_sender_map_.end(); sender_iter++) {
            delete (sender_iter->second);
        }
        stream_sender_map_.clear();
        db_debug("delete all stream sender");

        // tempory add a delay for wait rtsp server inner thread exit
        usleep(500*1000);
        RtspServer::Destroy();
        rtsp_server_ = NULL;
        db_debug("rtsp server destroyed");

    return 0;
}
#endif

int PreviewPresenter::RemoteServerInit()
{
#ifdef ENABLE_RTSP
    if (rtsp_flag_) {
        CreateRtspServer();
        CreateRtspStreamSender();
    }
#endif

#if 0
    #ifdef TUTK_SUPPORT
    tutk_connector_ = new TutkWrapper(dev_adapter_);
#endif
#endif
#ifdef ONVIF_SUPPORT
    if (onvif_flag_) {
        string ip;

        if (nm_->GetNetDevIp("eth0", ip) < 0) {
            if (nm_->GetNetDevIp("wlan0", ip) < 0) {
                ip = "0.0.0.0";
                db_warn("no activity net device found, set rtsp server ip to '%s", ip.c_str());
            }
        }

        OnvifInitParam param;
        param.ip = ip;
        onvif_connector_ = new OnvifConnector(dev_adapter_);
        onvif_connector_->Init(param);
    }
#endif
#if !(defined(TUTK_SUPPORT) || defined(ONVIF_SUPPORT))
    tutk_connector_ = new RemoteConnector();
#endif
    return 0;
}

int PreviewPresenter::RemoteServerStart()
{
#ifdef ENABLE_RTSP
    pthread_t rtsp_thread_id;
    ThreadCreate(&rtsp_thread_id, NULL, PreviewPresenter::RtspThreadLoop, this);
#endif
#if 0
    #ifdef TUTK_SUPPORT
    tutk_connector_->Start();
#endif
#endif
#ifdef ONVIF_SUPPORT
    if (onvif_flag_) {
        onvif_connector_->Start();
    }
#endif
    return 0;
}

int PreviewPresenter::RemoteServerStop()
{
#if 0
    #ifdef TUTK_SUPPORT
    tutk_connector_->Stop();
#endif
#endif
#ifdef ONVIF_SUPPORT
    if (onvif_flag_) {
      if (onvif_connector_)
         onvif_connector_->Stop();
    }
#endif
    return 0;
}

void PreviewPresenter::RemoteServerDeInit()
{
#ifdef ENABLE_RTSP
    DestroyRtspServer();
#endif

#if 0 // FIXME: uninit failed
    #ifdef TUTK_SUPPORT
    if (tutk_connector_) {
        delete tutk_connector_;
        tutk_connector_ = NULL;
    }
#endif
#endif
#ifdef ONVIF_SUPPORT
    if (onvif_flag_) {
        if (onvif_connector_) {
            delete onvif_connector_;
            onvif_connector_ = NULL;
        }
    }
#endif
}

void PreviewPresenter::TakePicforVideothumb(Recorder *recorder,const CameraID &cam_id)
{
    db_msg("[debug_jason]:TakePicforVideothumb 111");

   if(CheckStorage() < 0)
   {
       db_error("no tf card or is full");
       return;
   }
    RecorderParam param;
    recorder->GetParam(param);
    db_msg("[debug_jason]: this is TakePicforVideothumb 222");
    if(isAllowTakePIC)
	{
            camera_map_[cam_id]->SetVideoFileForThumb(param.MF);
            camera_map_[cam_id]->SetPictureMode(TAKE_PICTURE_MODE_FAST);
			if(CAM_UVC_1 != cam_id)
	            camera_map_[cam_id]->TakePicture(1);
			else
				camera_map_[cam_id]->TakePicture(3);
    }

}

void PreviewPresenter::TakePicButtonHandler(const CameraID & cam_id,bool value, bool force)
{
    map<CameraID, Camera*>::iterator iter = camera_map_.find(cam_id);
    if (iter == camera_map_.end()) {
        db_error("no camera can be used");
        return;
    }

    lock_guard<mutex> lock(disp_switch_mutex_);

    if(isAllowTakePIC)
    {
        if(CheckStorage() < 0) {
            db_error("no tf card or is full");
            if( value && (isTimeTakePicMode || isAutoTimeTakePicMode) )
                StopAutoLoopCountdown();

            return;
        }

        if (isRecordStart) {
            camera_map_[cam_id]->SetPictureMode(TAKE_PICTURE_MODE_FAST);

            if(camera_map_[cam_id]->TakePicture(0) < 0) {
                isAllowTakePIC = true;
            }
        } else {
            if(isTimeTakePicMode) {
                //start timer
                isTimeTakeTimerStart = true;
                const uint32_t time = camera_map_[cam_id]->GetTimeTakePicTime();
                if(value) {
                    StopAutoLoopCountdown();
                    if (!force)
                        RestartCountdown();
                } else {
                    SetCountdownSec(time,false);
                }
            } else if(isAutoTimeTakePicMode) {
                const uint32_t time = camera_map_[cam_id]->GetAutoTimeTakePicTime();
                if(value) {
                    isAutoTakeTimerStart = false;
                    StopAutoLoopCountdown();
                } else {
                    isAutoTakeTimerStart = true;
                    SetCountdownSec(time,true);
                }
                StorageManager::GetInstance()->SetDiskFullNotifyFlag(!value);
            } else {
                if( value )
                    return ;

                if(isContinueTakePicMode) {
                    camera_map_[cam_id]->SetPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
                    isAllowTakePIC = false;
                } else {
                    camera_map_[cam_id]->SetPictureMode(TAKE_PICTURE_MODE_FAST);
                }

                if(camera_map_[cam_id]->TakePicture(0) < 0) {
                    isAllowTakePIC = true;
                }
            }
        }
    } else {
        db_msg("[debug_joasn]: last take pic action is not finished !!!");
        Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
    }
}
void PreviewPresenter::SetDightZoomButtonHandler(const CameraID &cam_id, int value)
{
        map<CameraID, Camera*>::iterator iter = camera_map_.find(cam_id);
        if (iter == camera_map_.end()) {
            db_error("no camera can be used");
            return;
        }
        camera_map_[cam_id]->SetDightZoom(value);
}
bool PreviewPresenter::GetAudioStatus(const CameraID &cam_id)
{
    Recorder *recorder = cam_rec_map_[cam_id][REC_MAIN_CHN];
    RecorderParam param;
    recorder->GetParam(param);
    db_msg("[debug_jason]:param.audio_record is %d",param.audio_record);
    return param.audio_record;
}

void PreviewPresenter::AudioSwitch(Recorder *recorder)
{
    db_msg("[debug_jaosn]:AudioSwitch,val");
    RecorderParam param;
    recorder->GetParam(param);
    if(param.audio_record == true)
    {
        db_msg("[debug_jason]: clsoe audio_record");
        audio_record_ = false;
        recorder->SetRecordAudioOnOff(0);
    }
    else
    {
        db_msg("[debug_jason]: open audio_record");
        audio_record_ = true;
        recorder->SetRecordAudioOnOff(1);
    }
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    menuconfiglua->SetMenuIndexConfig(SETTING_RECORD_VOLUME,audio_record_);
}

void PreviewPresenter::RecordAudioSwitch(const CameraID &cam_id)
{
    RecorderIDSet rec_id_set = rec_group_[cam_id];
    RecorderIDSet::iterator rec_id_iter;
    for (rec_id_iter = rec_id_set.begin(); rec_id_iter != rec_id_set.end(); rec_id_iter++) {
            this->AudioSwitch(cam_rec_map_[cam_id][rec_id_iter->rec_type]);
    }
}

void PreviewPresenter::MediaDeInit()
{
    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map_.begin(); cam_iter != camera_map_.end(); cam_iter++) {
        // 1. delete Recorder
        map<RecorderType, Recorder*>::iterator rec_iter;
        for (rec_iter = cam_rec_map_[cam_iter->first].begin();
             rec_iter != cam_rec_map_[cam_iter->first].end(); rec_iter++) {
            rec_iter->second->Detach(this);
            delete (rec_iter->second);
            rec_iter->second = NULL;
        }

        cam_iter->second->Detach(this);
        // 2. Stop Preview
        // destruct will do this
        // cam_iter->second->StopPreview();

        // 3. delete camera;
        delete (cam_iter->second);
        if (g_camera_front == cam_iter->second)
			g_camera_front = NULL;
		else if(g_camera_back == cam_iter->second) 
			g_camera_back = NULL;
		cam_iter->second = NULL;
    }
    cam_rec_map_.clear();
    camera_map_.clear();
}


int PreviewPresenter::BackCameraMediaDeInit()
{
    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map_.begin(); cam_iter != camera_map_.end(); cam_iter++)
	{
		if( cam_iter->first != CAM_UVC_1 )
			continue;
        // 1. delete Recorder
        map<RecorderType, Recorder*>::iterator rec_iter;
        for (rec_iter = cam_rec_map_[cam_iter->first].begin();
             rec_iter != cam_rec_map_[cam_iter->first].end(); rec_iter++) {
            rec_iter->second->Detach(this);
            delete (rec_iter->second);
        }

		CamRecMap::iterator camrecMap_iter;
		for(camrecMap_iter = cam_rec_map_.begin(); camrecMap_iter != cam_rec_map_.end(); camrecMap_iter++)
		{
			if( camrecMap_iter->first != CAM_UVC_1)
				continue;
			else
				cam_rec_map_.erase(camrecMap_iter);
		}

        cam_iter->second->Detach(this);
		cam_iter->second->DeinitCamera();
		if(g_camera_back == cam_iter->second) 
			g_camera_back = NULL;

		cam_iter->second = NULL;
		camera_map_.erase(cam_iter);
    }

	return 0;
}

int PreviewPresenter::NormalInit()
{
    // group 0 recorder id set
    RecorderIDSet grp0_rec_id_set;

    // Ìí¼Ócamera idºÍrecorder type idµ½id group
    RecorderID grp0_rec0 = {CAM_NORMAL_0, REC_MAIN_CHN, REC_NORMAL, STREAM_SENDER_NONE};
    grp0_rec_id_set.push_back(grp0_rec0);
#ifdef SUB_RECORD_SUPPORT
    RecorderID grp0_rec1 = {CAM_NORMAL_0, REC_S_SUB_CHN, REC_STREAM, STREAM_SENDER_TUTK};
    grp0_rec_id_set.push_back(grp0_rec1);
#endif
    rec_group_.insert(make_pair(CAM_NORMAL_0, grp0_rec_id_set));

    MediaInit();

    MediaControlImpl *media_ctrl = static_cast<MediaControlImpl *>(dev_adapter_->media_ctrl_);
    media_ctrl->SetMediaCtrlInstance(cam_rec_map_, rec_group_,camera_map_);

    ImageManager::GetInstance()->InitImage(camera_map_);
    OsdManager::get()->initTimeOsd(cam_rec_map_,rec_group_);

    mainmodule->setCamerMap(camera_map_);
    mainmodule->setRecoderMap(cam_rec_map_,rec_group_);

    return 0;
}

int PreviewPresenter::BackCameraInit()
{
	std::map<CameraID, Camera*>::iterator cam_iter;
	cam_iter = camera_map_.find(CAM_UVC_1);
	if(cam_iter != camera_map_.end())
	{
		db_msg("back camera has been register\n");
		return 0;
	}

	RecorderIDSet grp1_rec_id_set;
	RecorderID grp1_rec0 = {CAM_UVC_1, REC_U_720P30FPS, REC_STREAM, STREAM_SENDER_UVC};
    grp1_rec_id_set.push_back(grp1_rec0);
    rec_group_.insert(make_pair(CAM_UVC_1, grp1_rec_id_set));

	BackCameraMediaInit();

    MediaControlImpl *media_ctrl = static_cast<MediaControlImpl *>(dev_adapter_->media_ctrl_);
    media_ctrl->SetMediaCtrlInstance(cam_rec_map_, rec_group_,camera_map_);
  
    ImageManager::GetInstance()->InitImage(camera_map_);
    OsdManager::get()->initTimeOsd(cam_rec_map_,rec_group_);
  
    mainmodule->setCamerMap(camera_map_);
    mainmodule->setRecoderMap(cam_rec_map_,rec_group_);
	
	return 0;
}

int PreviewPresenter::BackCameraDeInit()
{
    RecorderGroup::iterator grp_iter;
	grp_iter = rec_group_.find(CAM_UVC_1);
	if( grp_iter == rec_group_.end() )
		return 0;

	rec_group_.erase(grp_iter);
	BackCameraMediaDeInit();

    MediaControlImpl *media_ctrl = static_cast<MediaControlImpl *>(dev_adapter_->media_ctrl_);
    media_ctrl->SetMediaCtrlInstance(cam_rec_map_, rec_group_,camera_map_);
  
    ImageManager::GetInstance()->InitImage(camera_map_);
    OsdManager::get()->initTimeOsd(cam_rec_map_,rec_group_);
  
    mainmodule->setCamerMap(camera_map_);
    mainmodule->setRecoderMap(cam_rec_map_,rec_group_);

	return 0;
}

int PreviewPresenter::NormalDeInit()
{
    ImageManager::GetInstance()->ExitImage();
    OsdManager::get()->unInitTimeOsd();

    this->MediaDeInit();

    rec_group_.clear();

    return 0;
}

int PreviewPresenter::UVCModeInit()
{
    db_debug("uvc mode init");
    pthread_mutex_lock(&model_lock_);
    if( mode_ != USB_MODE_UVC ){
        db_msg("PreviewPresenter::UVCModeInit mode is not uvc\n");
        pthread_mutex_unlock(&model_lock_);
        return -1;
    }

    NormalDeInit();

    sleep(1);

    // group 0 recorder id set
    RecorderIDSet grp0_rec_id_set;

    // Ìí¼Ócamera idºÍrecorder type idµ½id group
    RecorderID grp0_rec0 = {CAM_UVC_0, REC_U_720P30FPS, REC_STREAM, STREAM_SENDER_UVC};
    grp0_rec_id_set.push_back(grp0_rec0);

    rec_group_.insert(make_pair(CAM_UVC_0, grp0_rec_id_set));
    this->MediaInit();
    for (auto grp_iter : rec_group_) {
        RecorderIDSet rec_id_set = grp_iter.second;
        for (auto set_iter : rec_id_set) {
            // create rtsp stream sender
            if (set_iter.rec_model == REC_STREAM) {
                // set callback for stream recorder
                Recorder *recorder = cam_rec_map_[grp_iter.first][set_iter.rec_type];
                recorder->SetEncodeDataCallback(PreviewPresenter::EncodeDataCallback, this);
            }
        }
    }

    pthread_mutex_unlock(&model_lock_);

    return 0;
}

int PreviewPresenter::UVCModeDeInit()
{
    pthread_mutex_lock(&model_lock_);
    if(!camera_map_.empty())
    {
        map<CameraID, Camera*>::iterator iter = camera_map_.find(CAM_UVC_0);
        if (iter != camera_map_.end())
        {
            this->MediaDeInit();
            rec_group_.clear();
            sleep(1);
            NormalInit();
        }
    }

    pthread_mutex_unlock(&model_lock_);

    return 0;
}


int PreviewPresenter::UVCModeStart()
{
    pthread_mutex_lock(&model_lock_);
    if( mode_ != USB_MODE_UVC ){
        pthread_mutex_unlock(&model_lock_);
        return -1;
    }

    USBGadget::GetInstance()->ActiveUVC();

    for (auto iter : cam_rec_map_[CAM_UVC_0]) {
        db_debug("uvc mode start");
        int ret = iter.second->StartRecord();
        if (ret < 0)
        {
            pthread_mutex_unlock(&model_lock_);
            return -1;
        }
    }

    pthread_mutex_unlock(&model_lock_);
    return 0;
}

int PreviewPresenter::UVCModeStop()
{
    pthread_mutex_lock(&model_lock_);
    for (auto iter : cam_rec_map_[CAM_UVC_0]){
        int ret = iter.second->StopRecord();
        if (ret < 0)
        {
            db_msg("PreviewPresenter::UVCModeStop failed\n");
            pthread_mutex_unlock(&model_lock_);
            return -1;
        }
    }

    USBGadget::GetInstance()->DeactiveUVC();
    pthread_mutex_unlock(&model_lock_);

    return 0;
}

int PreviewPresenter::StopRecord(Recorder *recorder)
{
    int ret = 0;

    if (recorder != NULL)
        ret = recorder->StopRecord();

    return ret;
}

int PreviewPresenter::StartRecord(Recorder *recorder, CameraID cam_id)
{
    int ret = 0;
    int crt = 0;

    uint16_t status = recorder->GetStatus();
     RecorderParam param;
     recorder->GetParam(param);
    if (status == RECORDER_IDLE) { // when restart record
        MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
        audio_record_ = (bool)menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_VOLUME);
        param.audio_record = audio_record_;
        recorder->SetParam(param);
    }
    // init and start recorder
    if (recorder != NULL)
    {	db_error("rec 005");
        ret = recorder->StartRecord();
    }

    return ret;
}

static inline int parse_nalu_type(const VEncBuffer *frame, int enc_type)
{
    uint32_t code = -1;
    int nalu_type = 0;
    int i;
    static int cnt = 0;

    for (i = 0; i < frame->data_size - 1; i++) {
        code = (code << 8) + frame->data[i];
        if ( (code & 0xffffff00) == 0x100 ) {
            if (enc_type == VENC_H265)
                nalu_type = (code & 0x7E) >> 1;
            else
                nalu_type = code & 0x1F;
            //db_debug("frame: %d, nalu type: %d", ++cnt, nalu_type);
            break;
        }
    }

    return nalu_type;
}


void PreviewPresenter::SendRtspData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self)
{
#ifdef ENABLE_RTSP
    if (self->stream_sender_map_.size() == 0) return;
    if (self->rtsp_server_->GetServerStatus() == RtspServer::SERVER_STOPED) return;

    RtspServer::StreamSender *stream_sender = self->stream_sender_map_[rec];
    MediaStream::FrameDataType frame_type;

    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame
#if 0
            int width = rand() % 200;
            int height = width;
            srand((unsigned int)time(NULL));
            int x = rand() % 1180;
            int y = (rand() % (700 - height - 64 + 1)) + 64;
            printf("left: %d, top: %d, right: %d, bottom: %d\n",
                x, y, x+width, y+height);
            RECT rect = {x, y, x+width, y+height};
            self->preview_win_->ShowBox(rect, PIXEL_red);
#endif

            // send sps/pps first
            VencHeaderData head_info = {NULL, 0};
            rec->GetVencHeaderData(head_info);

            if (head_info.pBuffer != NULL && head_info.nLength != 0)
                stream_sender->SendVideoData(head_info.pBuffer, head_info.nLength, MediaStream::FRAME_DATA_TYPE_HEADER);

            frame_type = MediaStream::FRAME_DATA_TYPE_I;
        }
        else {
            frame_type = MediaStream::FRAME_DATA_TYPE_P;
        }

        stream_sender->SendVideoData((unsigned char *) frame->data, frame->data_size, frame_type);
    } else if (frame->stream_type == 0x01) {
        stream_sender->SendAudioData((unsigned char *) frame->data, frame->data_size);
    } else {
       db_msg("stream type: %d", frame->stream_type);
    }
#endif
}

void PreviewPresenter::SendTutkData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self)
{
#ifdef TUTK_SUPPORT
    if (self->tutk_connector_ == NULL) return;

    RemoteConnector *tutk_connector = self->tutk_connector_;

    RecorderParam param;
    rec->GetParam(param);

    FRAMEINFO_t frame_info;
    memset(&frame_info, 0, sizeof(FRAMEINFO_t));
    if (param.enc_type == VENC_H264){
        frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
    }else if (param.enc_type == VENC_H265){
        frame_info.codec_id = MEDIA_CODEC_VIDEO_H265;
    }
    frame_info.timestamp = (unsigned int) frame->pts;

    if (frame->stream_type == 0x00) { // video
        int nalu_type = parse_nalu_type(frame, param.enc_type);
          if (((param.enc_type == VENC_H265) && (
                // nalu_type == 32, vps
                // nalu_type == 33, sps
                // nalu_type == 34, pps
                nalu_type == 19 ||
                nalu_type == 20 ||
                nalu_type == 21)) ||
            ((param.enc_type == VENC_H264) && (
                nalu_type == 5
        ))){ // if is I frame
            // send sps/pps first
           // db_msg("[debug_jason]: enc_type is %d",param.enc_type);
            VencHeaderData sps_info = {NULL, 0}, pps_info = {NULL, 0};
            rec->GetVencHeaderData(sps_info, pps_info);

            if (sps_info.pBuffer != NULL && sps_info.nLength != 0)
            {
                tutk_connector->SendVideoData((const char *) sps_info.pBuffer, sps_info.nLength, &frame_info, sizeof(frame_info));
            }
            if (pps_info.pBuffer != NULL && pps_info.nLength != 0)
            {
                tutk_connector->SendVideoData((const char *) pps_info.pBuffer, pps_info.nLength, &frame_info, sizeof(frame_info));
            }
            frame_info.flags = IPC_FRAME_FLAG_IFRAME;
        }
        else {
            frame_info.flags = IPC_FRAME_FLAG_PBFRAME;
        }
        tutk_connector->SendVideoData((const char *) frame->data, frame->data_size, &frame_info, sizeof(frame_info));
    }
    //else {
    //    db_msg("stream type: %d", frame->stream_type);
    //}
#endif
}

void PreviewPresenter::PutUVCData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self)
{
    if (frame->stream_type == 0x00) { // video
        USBGadget::GetInstance()->PutUVCVideoData((uint8_t*)frame->data, frame->data_size);
    }
}

void PreviewPresenter::WriteRawData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self)
{
    static int flag = 1;
#ifdef WRITE_RAW_H264_FILE
    if (frame->stream_type == 0x00) { // video
        if (self->mode_ != USB_MODE_UVC) {
            if (flag == 1 || (*(frame->data + 4) & 0x1F) == 5) { // if is I frame
                VencHeaderData head_info = {NULL, 0};
                rec->GetVencHeaderData(head_info);

                // write sps/pps at the begining of the file and the front of I frame
                if (head_info.pBuffer != NULL && head_info.nLength != 0)
                    fwrite(head_info.pBuffer, head_info.nLength, 1, rec->file_);
                flag = 0;
            }
        }

        fwrite(frame->data, frame->data_size, 1, rec->file_);
    }
    //else {
    //    db_msg("stream type: %d", frame->stream_type);
    //}
#endif
}

void PreviewPresenter::EncodeDataCallback(EncodeDataCallbackParam *param)
{
    int ret = 0;
    int sender_type = 0;
    VEncBuffer* frame = NULL;
    Recorder *rec = param->rec_;

    switch (param->what_) {
        case MPP_EVENT_ERROR_ENCBUFFER_OVERFLOW:
            // TODO 考虑buffer正在被覆盖的问题
            db_error("send data too slow!!!");
            break;
        default:
            break;
    }

    PreviewPresenter *self = static_cast<PreviewPresenter *>(param->context_);
    sender_type = rec->GetStreamSenderType();

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

        if (self->mode_ == NORMAL_MODE) {
            if (self->rtsp_flag_ && (sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
                self->SendRtspData(frame, rec, self);
            }

            if (self->tutk_flag_ && (sender_type & STREAM_SENDER_TUTK) == STREAM_SENDER_TUTK) {
                self->SendTutkData(frame, rec, self);
            }
        } else if (self->mode_ == USB_MODE_UVC) {
            if ((sender_type & STREAM_SENDER_UVC) == STREAM_SENDER_UVC) {
                self->PutUVCData(frame, rec, self);
            }
        }

#ifdef WRITE_RAW_H264_FILE
        self->WriteRawData(frame, rec, self);
#endif

        rec->GetEyeseeRecorder()->freeOneBsFrame(frame);
    }
}


int PreviewPresenter::GetWifiInfo()
{
#if 0
    vector<string> wifi_info;
    string info,title_str;

    PreviewWindow *win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));

    ::LuaConfig config;
    config.LoadFromFile("/data/menu_config.lua");


    R::get()->GetString("ml_camera_wifiswitch_ssid", title_str);
    info = title_str;
    info += config.GetStringValue("menu.camera.wifiinfo.ssid");
    wifi_info.push_back(info);

    R::get()->GetString("ml_camera_wifiswitch_passwd", title_str);
    info = title_str;
    info += config.GetStringValue("menu.camera.wifiinfo.password");
    wifi_info.push_back(info);

    //R::get()->GetString("ml_camera_wifiswitch_tip", title_str);
    //info = title_str;
    //wifi_info.push_back(info);


    win->ShowWifiInfoDialog(wifi_info);
#endif
    return 0;
}

void PreviewPresenter::SetCountdownSec(int sec,bool loop)
{
	db_msg("SetCountdownSec===================zhanglm");
#if 0 //add by zhb
    Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
    if (win != NULL) {
        StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
        stb->ShowCountdown(sec,loop);
    }
#endif
}

void PreviewPresenter::StopAutoLoopCountdown()
{
	db_msg("StopAutoLoopCountdown===================zhanglm");
#if 0//add by zhb
    Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
    if (win != NULL) {
        StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
        stb->StopAutoLoopCountdown();
    }
#endif
}

void PreviewPresenter::RestartCountdown()
{
	db_msg("RestartCountdown===================zhanglm");
#if 0//add by zhb
    Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
    if (win != NULL) {
        StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
        stb->RestartCountdown();
    }
#endif
}

void PreviewPresenter::SetDightZoomValue(int zoom_val)
{
	db_msg("[fangjj]:SetDightZoomValue  zoom_val:=[%d] \n", zoom_val);
#if 0//add by zhb
    Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
    if (win != NULL) {
        StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
        stb->UpdateDightZoomStatusBar(zoom_val);
    }
#endif
}

void PreviewPresenter::sendUsbConnectMessage()
{
    if(m_usb_attach_status == false && PowerManager::GetInstance()->getUsbconnectStatus())
     {
         this->Notify(MSG_USB_HOST_CONNECTED);
         m_usb_attach_status= true;
     }
}

int PreviewPresenter::ReConfigResolution(RecType p_nRecordType, Camera *p_Camera, Recorder *p_Record)
{
    int nIndexID;

    MenuConfigLua *menuconfiglua = MenuConfigLua::GetInstance();
    int nVal,timelapsflag;

    if( p_nRecordType == NORMAL_RECORD )
    {
        GetConfigVideoResolution(nIndexID);
        nVal = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_TIMELAPSE);
		timelapsflag = nVal;
        p_Camera->SetVideoResolution(nIndexID);
        p_Record->SetRecordEncodeSize(nIndexID);
        p_Record->SetDelayRecordTime(nVal);	// 设置帧率
        nVal = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_LOOP);
		if (timelapsflag) {	// is timelaps mode
			p_Record->SetVideoClcRecordTime(15);		// 
		} else {
        	p_Record->SetVideoClcRecordTime(nVal);
		}
        StorageManager::GetInstance()->setLoopRecord_Flag(nVal);
    }
    else if( p_nRecordType == SLOW_RECORD )
    {
        GetSlowRecordVideoResolution(nIndexID);
        nVal = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_SLOWMOTION);
        p_Camera->SetSlowVideoResloution(nVal);
        p_Record->SetRecordEncodeSize(nIndexID);
        p_Record->SetSlowRecord(nVal);
        StorageManager::GetInstance()->setLoopRecord_Flag(false);
    }

    camera_map_[CAM_NORMAL_0]->ShowPreview();

	return 0;
}

int PreviewPresenter::RecordHandle(const CameraID & p_nCamId,bool p_bVal)
{
    map<CameraID, Camera*>::iterator iter = camera_map_.find(p_nCamId);
    if (iter == camera_map_.end())
    {
        db_error("no camera can be used");
        return -1;
    }
	if( p_nCamId != CAM_UVC_1 )
	{
	    if(p_bVal)
	    {
	        if(this->StartRecord(cam_rec_map_[p_nCamId][REC_MAIN_CHN], p_nCamId)< 0)
	        {
	            db_msg("[debug_jason]:start record fialed");
	            isRecordStart = false;
	            return -1;
	        }
	        TakePicforVideothumb(cam_rec_map_[p_nCamId][REC_MAIN_CHN],p_nCamId);
			//by hero ****** rec led shining light
	        LedControl::get()->EnableLed(LedControl::DEV_LED, true);
			mFrontCamRecordStartStatus = true;
	    }
	    else
	    {
			//by hero ****** rec led long light
	        LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
	        if(this->StopRecord(cam_rec_map_[p_nCamId][REC_MAIN_CHN]) < 0)
	        {
	            db_msg("[debug_jaosn]:stop record failed");
	            return -1;
	        }
			mFrontCamRecordStartStatus = false;
	    }
	}
	else
	{
	    if(p_bVal)
	    {
	        if(this->StartRecord(cam_rec_map_[p_nCamId][REC_U_720P30FPS], p_nCamId)< 0)
	        {
	            db_msg("[debug_jason]:start record fialed");
	            isRecordStart = false;
	            return -1;
	        }
	        TakePicforVideothumb(cam_rec_map_[p_nCamId][REC_U_720P30FPS],p_nCamId);
			//by hero ****** rec led shining light
	        LedControl::get()->EnableLed(LedControl::DEV_LED, true);
			mBackCamRecordStartStatus = true;
	    }
	    else
	    {
			//by hero ****** rec led long light
	        LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
	        if(this->StopRecord(cam_rec_map_[p_nCamId][REC_U_720P30FPS]) < 0)
	        {
	            db_msg("[debug_jaosn]:stop record failed");
	            return -1;
	        }
			mBackCamRecordStartStatus = false;
	    }
	}

    return 0;
}

int PreviewPresenter::GetSlowRecordVideoResolution(int &p_nIndexID)
{
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    int resolution_id = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_SLOWMOTION);

    switch(resolution_id)
    {
        case 0:
            p_nIndexID = VIDEO_QUALITY_1080P120FPS;
            break;

        case 1:
            p_nIndexID = VIDEO_QUALITY_1080P60FPS;
            break;

        case 2:
            p_nIndexID = VIDEO_QUALITY_720P240FPS;
            break;

        case 3:
            p_nIndexID = VIDEO_QUALITY_720P120FPS;
            break;

        default:
            p_nIndexID = VIDEO_QUALITY_1080P60FPS;
            break;
    }

    return 0;
}

int PreviewPresenter::GetConfigVideoResolution(int &p_nIndexID)
{
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    int video_resolution_id = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
    p_nIndexID = video_resolution_id;

    return 0;
}

int PreviewPresenter::PlugInHandle(int msg)
{
    db_msg("hdmi plug in");
    int ret = 0;

    if (preview_win_!= NULL) preview_win_->PrepareChangeDispDev();

    // HDMI拔插时禁止所有按键响应
    Window::GlobalKeyBlocker global_key_blocker;
    // 正在响应按键处理的过程中不进行HDMI切换
    lock_guard<mutex> key_proc_lock(Window::key_proc_mutex_);
    // 显示切换时, 不允许拍照录像
    lock_guard<mutex> disp_lock(disp_switch_mutex_);

     if( !isAllowTakePIC || isTimeTakeTimerStart || isAutoTakeTimerStart || isWifiSoftEnable) {
         db_warn("isAllowTakePIC: %d, isTimeTakeTimerStart: %d, isAutoTakeTimerStart: %d, isWifiSoftEnable: %d",
                 isAllowTakePIC, isTimeTakeTimerStart, isAutoTakeTimerStart, isWifiSoftEnable);
         return -1;
     }

     Screensaver::GetInstance()->pause(true);

     if( win_mg_->IsUILoaded() )
     {
         if(cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]->GetStatus() == RECORDER_RECORDING ||
            cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->GetStatus() == RECORDER_RECORDING) {
             db_warn("hdmi plug in, interrupt record...");
             StopAllRecord();
             sleep(3);
         }
     }

     if( MSG_HDMI_PLUGIN == msg)
         Layer::GetInstance()->SwitchDisplayDevice(DISPLAY_DEV_HDMI);
     else
         Layer::GetInstance()->SwitchDisplayDevice(DISPLAY_DEV_TV);

     sleep(1);

     bool nPreviewStatus = camera_map_[CAM_NORMAL_0]->IsPreviewing();

     if( MSG_HDMI_PLUGIN == msg)
         ret = camera_map_[CAM_NORMAL_0]->ReStartCamera(DISPLAY_DEV_HDMI);
     else
         ret = camera_map_[CAM_NORMAL_0]->ReStartCamera(DISPLAY_DEV_TV);

     if (ret < 0) {
         db_error("hdmi switch failed, because restart camera failed");
         goto out;
     }

     if (nPreviewStatus) {
          camera_map_[CAM_NORMAL_0]->StartPreview();
          Layer::GetInstance()->SetLayerRectByDispType(LAYER_CAM0, -1);
     }

     Layer::GetInstance()->SetLayerRectByDispType(LAYER_UI, -1);

     // sleep(2);
     AudioCtrl::GetInstance()->SwitchAOCard(1);
     // // NOTE: 这里播放一次按键声音是为了规避切换HDMI声卡后，第一次播放没声音的问题
     // AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY_SOUND);

     m_HdmiDevConn = true;

     return ret;
out:
     Screensaver::GetInstance()->pause(false);

     return ret;
}

int PreviewPresenter::PlugOutHandle(int msg)
{
    db_msg("hdmi plug out");
    int ret = 0;

    if (preview_win_!= NULL) preview_win_->PrepareChangeDispDev();

    Window::GlobalKeyBlocker global_key_blocker;
    lock_guard<mutex> key_proc_lock(Window::key_proc_mutex_);
    lock_guard<mutex> disp_lock(disp_switch_mutex_);

    if( !m_HdmiDevConn)
        return 0;

    if( !isAllowTakePIC || isTimeTakeTimerStart || isAutoTakeTimerStart) {
        isAllowTakePIC = true;
        db_warn("hdmi plug out, interrupt takepic...");
        disp_switch_mutex_.unlock();
        TakePicButtonHandler(CAM_NORMAL_0, true, true);
        isTimeTakeTimerStart = false;
        isAutoTakeTimerStart = false;
        disp_switch_mutex_.lock();
    }

    AudioCtrl::GetInstance()->SwitchAOCard(0);

    if( win_mg_->IsUILoaded() )
    {
         if(cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]->GetStatus() == RECORDER_RECORDING ||
            cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->GetStatus() == RECORDER_RECORDING) {
             db_warn("hdmi plug out, interrupt record...");
             StopAllRecord();
             sleep(3);
         }
    }

    Layer::GetInstance()->SwitchDisplayDevice(DISPLAY_DEV_LCD);

    usleep(250*1000);

    bool nPreviewStatus = camera_map_[CAM_NORMAL_0]->IsPreviewing();

    ret = camera_map_[CAM_NORMAL_0]->ReStartCamera(DISPLAY_DEV_LCD);
    if (ret < 0) {
        db_error("hdmi switch failed, because restart camera failed");
        goto out;
    }

    if( nPreviewStatus ) {
        camera_map_[CAM_NORMAL_0]->StartPreview();
        Layer::GetInstance()->SetLayerRectByDispType(LAYER_CAM0, -1);
    }

    Layer::GetInstance()->SetLayerRectByDispType(LAYER_UI, -1);

out:
    m_HdmiDevConn = false;

    PowerManager::GetInstance()->resetScreenStatus();
    Screensaver::GetInstance()->pause(false);

    return ret;
}

int PreviewPresenter::remoteTakePicture()
{
    lock_guard<mutex> lock(disp_switch_mutex_);

    if(CheckStorage() < 0)
    {
        db_error("no tf card or is full");
        if( isTimeTakePicMode || isAutoTimeTakePicMode)
        {
            StopAutoLoopCountdown();
            isAutoTakeTimerStart = false;
        }
        return 0;
    }

    if( isAllowTakePIC)
    {
        if(isTimeTakePicMode) {
          //start timer
          isTimeTakeTimerStart = true;
           const uint32_t time = camera_map_[CAM_NORMAL_0]->GetTimeTakePicTime();
           SetCountdownSec(time,false);
        } else if(isAutoTimeTakePicMode) {
           const uint32_t time = camera_map_[CAM_NORMAL_0]->GetAutoTimeTakePicTime();
           if(isAutoTakeTimerStart){
            StopAutoLoopCountdown();
           isAutoTakeTimerStart = false;
           }else{
           isAutoTakeTimerStart = true;
           SetCountdownSec(time,true);
           }
           StorageManager::GetInstance()->SetDiskFullNotifyFlag(isAutoTakeTimerStart);
        } else {
         if(isContinueTakePicMode)
         {
             camera_map_[CAM_NORMAL_0]->SetPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
	         isAllowTakePIC = false;
         }
         else
         {
	     	camera_map_[CAM_NORMAL_0]->SetPictureMode(TAKE_PICTURE_MODE_FAST);
	     }

         if(camera_map_[CAM_NORMAL_0]->TakePicture(0) < 0)
         {
             isAllowTakePIC = true;
         }
    }
    }else{
        db_msg("[debug_jaosn]:last action is not finished");
    }
    return 0;

}

void PreviewPresenter::StopAllRecord()
{
    if (isRecordStart) {
        if (osd_flag) {
            OsdManager::get()->DettchVencRegion();
            OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_MAIN_CHN);
        }

        if(RecordHandle(CAM_NORMAL_0, 0) < 0)
        {
            db_error("[debug_jaosn]: stop main record failed");
        }
        GetThumbVideoFileName(CAM_NORMAL_0);
        StorageManager::GetInstance()->SetDiskFullNotifyFlag(0);
    }

    if (osd_flag) {
        OsdManager::get()->DettchVencRegion();
#ifdef SUB_RECORD_SUPPORT
        OsdManager::get()->stopTimeOsd(CAM_NORMAL_0,REC_S_SUB_CHN);
#endif
    }
    StreamRecordHandle(0);
}

void PreviewPresenter::CameraOnError()
{
    db_debug("handle camera on error");

    if( !isAllowTakePIC || isTimeTakeTimerStart || isAutoTakeTimerStart) {
        isAllowTakePIC = true;
        db_warn("camera on error, interrupt takepic...");
        TakePicButtonHandler(CAM_NORMAL_0, true, true);
    }

    if(cam_rec_map_[CAM_NORMAL_0][REC_MAIN_CHN]->GetStatus() == RECORDER_RECORDING ||
        cam_rec_map_[CAM_NORMAL_0][REC_S_SUB_CHN]->GetStatus() == RECORDER_RECORDING) {
        db_warn("camera on error, interrupt record...");
        StopAllRecord();
    }

    bool nPreviewStatus = camera_map_[CAM_NORMAL_0]->IsPreviewing();

    int ret = camera_map_[CAM_NORMAL_0]->ReStartCamera(DISPLAY_DEV_LCD);
    if (ret < 0) {
        db_error("restart camera failed");
    }

    if(nPreviewStatus) {
        camera_map_[CAM_NORMAL_0]->StartPreview();
    }
}

void PreviewPresenter::LowPowerShutdownTimerHandler(union sigval sigval)
{
    PreviewPresenter *self = reinterpret_cast<PreviewPresenter *>(sigval.sival_ptr);
    if (PowerManager::GetInstance()->getACconnectStatus()) {
        db_info("ac connected, stop shutdown process");
        stop_timer(self->lowpower_shutdown_timer_id_);
        self->lowpower_shutdown_processing_ = false;
        return;
    }

    self->DoSystemShutdown();
    self->lowpower_shutdown_processing_ = false;
}

#ifdef SHOW_DEBUG_INFO
void PreviewPresenter::DebugInfoThread(PreviewPresenter *self)
{
    prctl(PR_SET_NAME, "DebugInfoThread", 0, 0, 0);
    while (true) {
        char *env = getenv("HIDE_DEBUG_INFO");
        if (env != NULL) {
            self->preview_win_->ShowDebugInfo(false);
            break;
        }

        self->preview_win_->ClearDebugInfo();

        // mem info
        FILE *fp = NULL;
        char buf[128] = {0};

        fp = fopen("/proc/meminfo", "r");

        if (fp == NULL) {
            db_error("open file failed, %s", strerror(errno));
            break;
        }

        while (fgets(buf, sizeof(buf), fp) != NULL) {
            buf[strlen(buf) - 1] = '\0';
            string line = buf;
            if (line.find("MemFree") != string::npos) {
                self->preview_win_->InsertDebugInfo("1_MemFree", line);
            } else if (line.find("Cached") != string::npos) {
                self->preview_win_->InsertDebugInfo("2_Cached", line);
            } else if (line.find("Committed_AS") != string::npos) {
                self->preview_win_->InsertDebugInfo("3_Committed_AS", line);
            } else if (line.find("CmaFree") != string::npos) {
                self->preview_win_->InsertDebugInfo("4_CmaFree", line);
            }
        }

        fclose(fp);
        fp = NULL;

        int ret = 0;
        // cpu temperature
        memset(buf, 0, sizeof(buf));
        ret = getoneline(CPU_TEMPERATURE_FILE, buf, sizeof(buf));
        if (ret == 0) {
            buf[strlen(buf) - 1] = '\0';
            string info = "CPUTemp:         " + string(buf) + "C";
            self->preview_win_->InsertDebugInfo("5_CPUTemp", info);
        }

        memset(buf, 0, sizeof(buf));
        ret = getoneline(BATTERY_CAPACITY, buf, sizeof(buf));
        if (ret == 0) {
            buf[strlen(buf) - 1] = '\0';
            string info = "Battery:           " + string(buf) + "%";
            self->preview_win_->InsertDebugInfo("6_Battery", info);
        }

        // vi frame lost
        fp = fopen("/proc/sys/debug/mpp/vi", "r");

        if (fp == NULL) {
            db_error("open file failed, %s", strerror(errno));
            break;
        }

        stringstream ss;
        ss << "FrameLost:         ";
        int vi_chn = 0;
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            buf[strlen(buf) - 1] = '\0';
            string line = buf;
            string::size_type pos = line.find("frame_lost_cnt");
            if (pos != string::npos) {
                int len = strlen("frame_lost_cnt: ");
                ss << "vipp" << vi_chn << "["<< line.substr(len, line.find(",") - len) << "] ";
                vi_chn++;
            }
        }
        self->preview_win_->InsertDebugInfo("7_FrameLost", ss.str());

        fclose(fp);
        fp = NULL;

        sleep(3);
    }
}
#endif

int PreviewPresenter::SwitchLayer(CameraPreviewType p_oldPreviewType,CameraPreviewType p_newPreviewType)
{
	if(p_oldPreviewType == p_newPreviewType)
	{
		return 0;
	}

	ViewInfo CameraFrontInfo,CameraBackInfo;
	switch(p_newPreviewType )
	{
		case FRONT_BACK:
		{
			if(p_oldPreviewType == ONLY_FRONT )
			{
				break;
			}
		}
		case BACK_FRONT:
		{
			camera_map_[CAM_NORMAL_0]->GetCameraDispRect(&CameraFrontInfo);
			camera_map_[CAM_UVC_1]->GetCameraDispRect(&CameraBackInfo);
			camera_map_[CAM_NORMAL_0]->StopPreview();
			camera_map_[CAM_UVC_1]->StopPreview();
			camera_map_[CAM_NORMAL_0]->SetCameraDispRect(CameraBackInfo);
			camera_map_[CAM_UVC_1]->SetCameraDispRect(CameraFrontInfo);
			camera_map_[CAM_NORMAL_0]->StartPreview();
			camera_map_[CAM_UVC_1]->StartPreview();
			break;
		}
		case ONLY_FRONT:
		{
			if(p_oldPreviewType == FRONT_BACK )
				break;

			camera_map_[CAM_UVC_1]->GetCameraDispRect(&CameraBackInfo);
			camera_map_[CAM_NORMAL_0]->GetCameraDispRect(&CameraFrontInfo);
			camera_map_[CAM_NORMAL_0]->StopPreview();
			camera_map_[CAM_UVC_1]->StopPreview();
			camera_map_[CAM_NORMAL_0]->SetCameraDispRect(CameraBackInfo);
			camera_map_[CAM_NORMAL_0]->StartPreview();
			break;
		}
		default:
			break;
	}

	return 0;
}


