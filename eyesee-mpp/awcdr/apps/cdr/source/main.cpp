/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file main.cpp
 * @brief 该文件为应用程序入口�? *
 * @author id:520
 * @date 2015-11-24
 */

#include "main.h"

#include <thread>

#include "common/thread.h"
#include "common/app_log.h"
#include "common/app_def.h"
#include "common/utils/utils.h"
#include "bll_presenter/sample_ipc.h"

#include "device_model/system/gsensor_manager.h"
#include "device_model/system/keyinput.h"
#include "device_model/system/watchdog.h"
#include "device_model/system/device_qrencode.h"
#include "device_model/system/event_manager.h"
#include "device_model/system/net/bluetooth_controller.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/ethernet_controller.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/system/power_manager.h"
#include "device_model/media/camera/camera.h"
#include "device_model/display.h"
#include "device_model/menu_config_lua.h"
#include "device_model/fbtools.h"
#include "device_model/partitionManager.h"
#include "device_model/version_update_manager.h"
#include "dd_serv/globalInfo.h"

//#include "window/window_manager.h"
#include "uilayer_view/gui/minigui/window/preview_window.h"
#include "uilayer_view/gui/minigui/window/prompt.h"
//#include <audio_hw.h>
//#include <alsa_interface.h>


#include "dd_serv/dd_mqtt.h"
#include "dd_serv/dd_AGPS.h"

#include "bll_presenter/screensaver.h"
#include "bll_presenter/closescreen.h"

#include "dd_serv/dd_https.h"
#ifdef GUI_SUPPORT
#include "bll_presenter/launcher.h"
#include "bll_presenter/ipc_mode.h"
#include "bll_presenter/newPreview.h"
#include "bll_presenter/event_monitor.h"
#include "bll_presenter/binddevice.h"
#include "bll_presenter/playback.h"
#include "bll_presenter/setting_handler.h"
#include "bll_presenter/device_setting.h"
#include "bll_presenter/status_bar.h"
#include "bll_presenter/status_bar_bottom.h"
#include "window/window_manager.h"
#include <vo/hwdisplay.h>
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/camRecCtrl.h"
#endif

#include <mpi_sys.h>
#include <mpi_venc.h>
#include <audio_hw.h>
#include "dd_serv/common_define.h"
#include "../source/device_model/system/factory_writesn.h"

#undef LOG_TAG
#define LOG_TAG "MainModule"

#define DEBUG_MEM 0
#define ENABLE_QRENCODE
//#define ENABLE_WATCHDOG // 给客户的固件要打开
//#define SHOW_BIND

using namespace EyeseeLinux;
using namespace std;
NewPreview *preview;
sem_t g_app_exit;
DeviceSettingPresenter *p_device_setting = NULL;

int g_exit_action = EXIT_APP;

void *MainModule::DeviceInitThread(void *context)
{
    db_msg("device model init");
    MainModule *mm = reinterpret_cast<MainModule*>(context);

    prctl(PR_SET_NAME, "DeviceInit", 0, 0, 0);

    mm->DeviceModelInit();
    return NULL;
}

#define KFCTMPDIR "/tmp"

#if 0
int CameraPreInit()
{

	struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", "start camera init", tv.tv_sec, tv.tv_usec);
#ifdef DEBUG_LOG_CREATE
    log_init("sdvcam", NULL);
#endif
    MPP_SYS_CONF_S sys_conf;
    memset(&sys_conf, 0, sizeof(MPP_SYS_CONF_S));
    sys_conf.nAlignWidth = 32;
    strncpy(sys_conf.mkfcTmpDir, KFCTMPDIR, sizeof(sys_conf.mkfcTmpDir));
    AW_MPI_SYS_SetConf(&sys_conf);
    int ret = AW_MPI_SYS_Init_S1();
    if (ret != SUCCESS) {
        _exit(-1);
    }
    // create camera
 //   Layer::GetInstance()->CloseLayer(LAYER_UI);
    ret = AW_MPI_SYS_Init_S2();
    if (ret != SUCCESS) {
        _exit(-1);
    }
    return 0;
}
#endif

int AW_MPI_SYS_S1_Init()
{
#ifdef DEBUG_LOG_CREATE
    GLogConfig stGLogConfig = 
    {
        .FLAGS_logtostderr = 1,
        .FLAGS_colorlogtostderr = 1,
        .FLAGS_stderrthreshold = _GLOG_INFO,
        .FLAGS_minloglevel = _GLOG_INFO,
        .FLAGS_logbuflevel = -1,
        .FLAGS_logbufsecs = 0,
        .FLAGS_max_log_size = 1,
        .FLAGS_stop_logging_if_full_disk = 1,
    };
    strcpy(stGLogConfig.LogDir, "/tmp/log");
    strcpy(stGLogConfig.InfoLogFileNameBase, "LOG-");
    strcpy(stGLogConfig.LogFileNameExtension, "CDR-");
    log_init("sdvcam", &stGLogConfig);
#endif
    MPP_SYS_CONF_S sys_conf;
    memset(&sys_conf, 0, sizeof(MPP_SYS_CONF_S));
    sys_conf.nAlignWidth = 32;
    strncpy(sys_conf.mkfcTmpDir, KFCTMPDIR, sizeof(sys_conf.mkfcTmpDir));
    AW_MPI_SYS_SetConf(&sys_conf);
    int ret = AW_MPI_SYS_Init_S1();
    if (ret != SUCCESS) {
       _exit(-1);
    }
    return 0;
}

int AW_MPI_SYS_S2_Init()
{
    int ret = AW_MPI_SYS_Init_S2();
    if (ret != SUCCESS) {
        _exit(-1);
    }
    return 0;
}

void MainModule::ShowPromptInfo()
{
    PreviewWindow*pw = static_cast<PreviewWindow *>(window_manager_->GetWindow(WINDOWID_PREVIEW));
    pw->ShowPromptInfo(PROMPT_NOTIC_UPDATE, 5, true);
}

#if 0
void* MainModule::thread_update(void* context)
{
    prctl(PR_SET_NAME, "thread_update", 0, 0, 0);
    sleep(5);
    std::string bindflag = DD_GLOBALINFO::GetInstance()->getBindflag();
    if(strcmp(bindflag.c_str(), "") != 0) {
        char filename[512] = {0};
        snprintf(filename,sizeof(filename), "%s/%s/%s",MOUNT_PATH,VERSION_DIR_NET, FLAG_FORCEUPDATE);
        if(access(filename, F_OK) == 0) {
            if(p_device_setting != NULL) {
                MainModule *MM = reinterpret_cast<MainModule*>(context);
                MM->ShowPromptInfo();        
                //save old_version time_upgrade isReport
                std::string old_version = DD_GLOBALINFO::GetInstance()->getSystemVersion();
                UpgradeConf cfg = DD_GLOBALINFO::GetInstance()->getUpgradeConf();
                cfg.old_version = old_version;
                cfg.time_upgrade = std::to_string(time(0));
                cfg.isReport = STRUE;
                //DD_GLOBALINFO::GetInstance()->setUpgradeConf(cfg);
                //set net update ota stage
                VersionUpdateManager::GetInstance()->setForceUpdateFlag(false);
                p_device_setting->exeOtaUpdate(TYPE_UPDATE_NET);
            }
        }
    }
}

#endif
int file_size(const char* filename)
{
    int size = -1;
    FILE *fp=fopen(filename,"r");
    if(!fp) {
        return -1;
    }
    fseek(fp,0L,SEEK_END);
    size=ftell(fp);
    fclose(fp);

    return size;
}

static int GetBeepToneConfig(int idx)
{
    int beep_map[] {100, 83, 60};
    return beep_map[idx];
}

void MiniGuiInit(int args, const char* argv[])
{
    int initguires = -1;
    if (InitGUI (args, argv) != 0) {
        initguires = -1;
    }
    else{
        initguires = 0;
    }
}

int MppInit(int args, const char* argv[])
{
    int ret = AW_MPI_SYS_S2_Init();
    if (ret != SUCCESS) {
        _exit(-1);
    }
    return 0;
}

int Main(int args, const char* argv[])
{
#ifdef ENABLE_WATCHDOG
    WatchDog *dog = WatchDog::GetInstance();
    dog->RunWatchDog();
#endif
    
    MainModule *main_module = new MainModule();
    AW_MPI_SYS_S1_Init();
    preview = new NewPreview(main_module);
    assert(preview != NULL);
    preview->Attach(main_module);
    preview->OnWindowLoaded();
    std::thread([&] {
        fb_clean();
    }).detach();
    std::thread GuiInitThread(MiniGuiInit,args,argv);
    std::thread MppInitThread(MppInit,args,argv);
    GuiInitThread.join();
    MppInitThread.join();
    main_module->DeviceModelInit();
    std::thread([&] {
       int ret = ::audioHw_Construct();
       if (ret) {
           db_warn("audioHw_Construct is failed");
       }
       AudioCtrl::GetInstance()->InitPlayer(0);
       int val = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_VOLUME_SELECTION);
       AudioCtrl::GetInstance()->SetBeepToneVolume(GetBeepToneConfig(val));
       AudioCtrl::GetInstance()->PlaySound(AudioCtrl::STARTUP_SOUND);
       verify_bin_md5();
    }).detach();
    preview->InitWindow();
    main_module->CreatePresenter();
    system("mount -t debugfs none /sys/kernel/debug/");
    sem_init(&g_app_exit, 0, 0);

    SetKeyLongPressTime(100);

#ifdef AES_SUPPORT
	db_warn("debug_zhb---> ready to init the curl global >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
	if( vum->CurlGlobalInit() < 0)
	{
		db_error("%s : %d  init the curl global failed ",__func__,__LINE__);
	}
#endif

    main_module->UIInit();

    sem_wait(&g_app_exit);
    sem_destroy(&g_app_exit);
    for (;;) {
        sleep(10);
    }

//    delete main_module;
#ifdef ENABLE_WATCHDOG
    dog->StopWatchDog();
    delete dog;
#endif

    if (g_exit_action == POWEROFF)
        system("poweroff");
    else if (g_exit_action == REBOOT)
        system("reboot");
    else
        exit(0);

    return 0;
}

MainModule::MainModule()
    : AsyncObserver(MsgHandler, this)
{
//    ThreadCreate(&init_thread_id_, NULL, MainModule::DeviceInitThread, this);
}

MainModule::~MainModule()
{
    window_manager_->Destroy();

    std::list<IPresenter*>::iterator iter;
    for (iter = presenter_list_.begin(); iter != presenter_list_.end();) {
        delete (*iter);
        iter = presenter_list_.erase(iter);
    }

    presenter_list_.clear();

    DeviceModelDeInit();
}

void MainModule::CreatePresenter()
{
    map<WindowID, IGUIPresenter*> win_presenter_map;
    // 1. create presenter
#ifdef GUI_SUPPORT
    BindPresenter *device_binding = new BindPresenter();
	assert(device_binding != NULL);
	device_binding->Attach(this);
	presenter_list_.push_back(device_binding);
	win_presenter_map.insert(make_pair(WINDOWID_BINDING, device_binding));

#ifdef ONE_CAM
    //PreviewPresenter *preview = new PreviewPresenter(this);
//	NewPreview *preview = new NewPreview(this);
//    assert(preview != NULL);
//    preview->Attach(this);
    presenter_list_.push_back(preview);
    win_presenter_map.insert(make_pair(WINDOWID_PREVIEW, preview));

    PlaybackPresenter *playback = new PlaybackPresenter();
    assert(playback != NULL);
    playback->Attach(this);
    presenter_list_.push_back(playback);
    win_presenter_map.insert(make_pair(WINDOWID_PLAYBACK, playback));
#endif

    SettingHandlerPresenter *setting_handler = new SettingHandlerPresenter();
    assert(setting_handler != NULL);
    presenter_list_.push_back(setting_handler);
    win_presenter_map.insert(make_pair(WINDOWID_SETTING_HANDLER, setting_handler));


    DeviceSettingPresenter *device_setting = new DeviceSettingPresenter(this);
    assert(device_setting != NULL);
    presenter_list_.push_back(device_setting);
    #ifdef SETTING_WIN_USE
    win_presenter_map.insert(make_pair(WINDOWID_SETTING, device_setting));
    #else
    win_presenter_map.insert(make_pair(WINDOWID_SETTING_NEW, device_setting));
    #endif
    p_device_setting = device_setting;

    StatusBarPresenter *status_bar = new StatusBarPresenter();
    assert(status_bar != NULL);
    presenter_list_.push_back(status_bar);
    win_presenter_map.insert(make_pair(WINDOWID_STATUSBAR, status_bar));


	StatusBarBottomPresenter *statu_bar_bottm = new StatusBarBottomPresenter();
	assert(statu_bar_bottm != NULL);
	presenter_list_.push_back(statu_bar_bottm);
	win_presenter_map.insert(make_pair(WINDOWID_STATUSBAR_BOTTOM, statu_bar_bottm));
    window_manager_ = WindowManager::GetInstance();
    window_manager_->SetGUIPresenter(win_presenter_map);
#endif

}

#ifdef GUI_SUPPORT
void MainModule::UIInit()
{
    db_msg("ui init");
    window_manager_ = WindowManager::GetInstance();
    // show main window
#ifdef ONE_CAM
#ifdef SHOW_BIND
    std::string bindflag = DD_GLOBALINFO::GetInstance()->getBindflag();
	if(strcmp(bindflag.c_str(),"true"))
	{
        if(access(FILE_BINDFLAG, F_OK) != 0) {
		    window_manager_->Init(WINDOWID_BINDING);
        }
        else {
		    window_manager_->Init(WINDOWID_PREVIEW);
        }
	}else
#endif
	{
		 window_manager_->Init(WINDOWID_PREVIEW);
	}

#endif

    // set ui framebuffer layer to top
    Layer::GetInstance()->SetLayerTop(LAYER_UI);
    MediaFileManager::GetInstance()->startCreateDataBase();
    int ret = StorageManager::GetInstance()->GetStorageStatus();
    if ( (ret != UMOUNT) && (ret != STORAGE_FS_ERROR) && (ret != FORMATTING))
    {
        db_error("sd mount set data base init flag true");
        MediaFileManager::GetInstance()->SetDataBaseStartUpInitFlag(true);
    }
	EventManager::GetInstance()->RunAccEvent();
    #if 0
    int ret = pthread_create(&thread_id_update, NULL, MainModule::thread_update, this);
    if (ret < 0) {
        printf("log thread failed!\n");
    }
    #endif
    //if park record here start
    if(PowerManager::GetInstance()->getPowenOnType()== 0)
    {	// gsensor
		int val = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_PARKING_MONITORY);
		if (val) {
			db_error("start park record ................");
			
        	window_manager_->sendmsg(window_manager_->GetWindow(WINDOWID_PREVIEW), PREVIEW_RECORD_BUTTON, 1);
		}
    }
    window_manager_->MsgLoop();
}
#endif

void MainModule::DeviceModelInit()
{
    int ret;
#if 0
    std::thread([&] {
        int ret = ::audioHw_Construct();
        if (ret) {
            db_warn("audioHw_Construct is failed");
        }
        AudioCtrl::GetInstance()->InitPlayer(0);
        int val = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_VOLUME_SELECTION);
        AudioCtrl::GetInstance()->SetBeepToneVolume(GetBeepToneConfig(val));
        AudioCtrl::GetInstance()->PlaySound(AudioCtrl::STARTUP_SOUND);
        verify_bin_md5();
        LoadDriver();
    }).detach();
#endif
    std::thread([&] {
        AW_MPI_VENC_SetVEFreq(MM_INVALID_CHN, 520);
    #ifdef GUI_SUPPORT
        Layer::GetInstance();
    #endif
        EventManager::GetInstance();
    //    NetManager::GetInstance();
        StorageManager::GetInstance();
        MediaFileManager::GetInstance();
        CameraFactory::GetInstance();
        RecorderFactory::GetInstance();
    #ifdef ENABLE_RTSP
      //  RtspServer::GetInstance();
    #endif
        MenuConfigLua::GetInstance();
        PowerManager::GetInstance()->SetBrightnessLevel(LCD_BRIGHTNESSLEVEL);
    }).detach();
}

int MainModule::LoadDriver()
{
    //system("insmod /lib/modules/4.4.55/hdmi.ko");

    return 0;
}

void MainModule::setCamerMap(const CameraMap &p_CamMap)
{
    m_CameraMap = p_CamMap;
	CamRecCtrl::GetInstance()->SetCamMap(m_CameraMap);
}

void MainModule::setRecoderMap(const CamRecMap &p_CamRecMap)
{
    m_CamRecMap = p_CamRecMap;
	CamRecCtrl::GetInstance()->setRecoderMap(m_CamRecMap);
}

const CamRecMap MainModule::getRecoderMap()
{
    return m_CamRecMap;
}

const CameraMap MainModule::getCamerMap()
{
    return m_CameraMap;
}

void MainModule::DeviceModelDeInit()
{
#ifdef ENABLE_RTSP
    RtspServer::Destroy();
#endif
    RecorderFactory::Destroy();
    CameraFactory::Destroy();

    MediaFileManager::Destroy();

	StorageManager::Destroy();
    NetManager::Destroy();
    //BlueToothController::Destroy();

    
#ifdef GUI_SUPPORT
    Layer::Destroy();
#endif

    EventManager::Destroy();

    AudioCtrl::Destroy();

    //AW_MPI_SYS_Exit();


}

void MainModule::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    if (msg == MSG_SHUTDOWN_SYSTEM)
        AsyncObserver::HandleMessage(msg);
	else if(msg == MSG_PREPARE_TO_SUSPEND)
		AsyncObserver::HandleMessage(msg);
	else if(msg == MSG_START_RESUME)
		AsyncObserver::HandleMessage(msg);
}

#ifndef GUI_SUPPORT
void MainModule::StartSampleIPC()
{
    sample_ipc_->StartSampleIPC();
    sample_ipc_->SampleIPCCtrl(CAM_IPC_0, CMD_RECORD, 1);
    sample_ipc_->SampleIPCCtrl(CAM_IPC_1, CMD_RECORD, 1);
}

void MainModule::ExitSampleIPC()
{
    sample_ipc_->SampleIPCCtrl(CAM_IPC_0, CMD_RECORD, 0);
    sample_ipc_->SampleIPCCtrl(CAM_IPC_1, CMD_RECORD, 0);
    sample_ipc_->ExitSampleIPC();
}
#endif

static int isshutdowning = 0;
void *MainModule::MsgHandler(void *context)
{
    MainModule *self = static_cast<MainModule *>(context);

    switch(self->msg_) {
        case MSG_SHUTDOWN_SYSTEM:
            {
				if (!isshutdowning) {	// 防止系统多次发送MSG_SHUTDOWN_SYSTEM引起的多次处理
					isshutdowning = 1;
					db_warn("cdr will shutdown,bye");
	                // wait info dialog draw finished
	                //usleep(500 * 1000);

	                std::map<WindowID, IGUIPresenter*> win_presenter_map =
	                WindowManager::GetInstance()->GetGuiPresenter();
	                win_presenter_map[WINDOWID_PLAYBACK]->OnWindowDetached();
	                AudioCtrl::GetInstance()->PlaySound(AudioCtrl::SHUTDOWN_SOUND);
	                #if 0
	                if (!verify_bin_md5()) {
	                    char cmd[128] = {0};
	                    snprintf(cmd, sizeof(cmd), "cp -v %s %s", SDVCAM_BIN_2, SDVCAM_BIN_1);
	                    system(cmd);
	                }
	                #endif
					sleep(1);
					#ifdef GSENSOR_SUSPEND_ENABLE
					// 0 -normal 1-lowpower mode 2- suspend  (1好像不行)
					system("echo 2 > /sys/class/input/input4/powermode");
					usleep(1 * 1000);
					system("cat /sys/class/input/input4/reg_data > /dev/console");
					#endif
	                system("hwclock -w");
	                system("poweroff -f");
					sem_post(&g_app_exit);
				}
                break;
            }
		case MSG_PREPARE_TO_SUSPEND:
			{
				db_msg("ready to MSG_PREPARE_TO_SUSPEND");
				//AudioCtrl::Destroy();
				AudioCtrl::GetInstance()->DeInitPlayer();
				#if 0
				int ret = AW_MPI_SYS_AudioHw_Destruct();
				if(ret){
					db_msg("audioHw_destruct is faild");
				}
				#endif
				EventManager::GetInstance()->SetUSB4GModuleStandby(true);
				//self->DeviceModelDeInit();
				break;
			}
		case MSG_START_RESUME:
			{
				db_msg("ready to MSG_START_RESUME");
				#if 0
                std::thread([=] {
                    prctl(PR_SET_NAME, "Resume_WakeUpScreen", 0, 0, 0);
                    Closescreen *cs = Closescreen::GetInstance();
                    if (cs->IsScreenClosed()) {
                        cs->Stop();
                    }
                    EyeseeLinux::Screensaver::GetInstance()->reStartCount();
                }).detach();	
				#endif
				#if 0
				int ret = AW_MPI_SYS_AudioHw_Construct();
				if(ret){
					db_msg("audioHw_destruct is faild");
				}	
				#endif
				AudioCtrl::GetInstance()->InitPlayer(0);
				EventManager::GetInstance()->SetUSB4GModuleStandby(false);
#if 0
                cdx_sem_up(&mSemGetAgpsStart);//for agps resume
#endif
                //self->DeviceModelInit();
				break;
			}		
        default:
            break;
    }

    return NULL;
}
