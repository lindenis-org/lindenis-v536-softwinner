/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ipc_mode.cpp
 * @brief ipc工作模式presenter
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */

#include "bll_presenter/ipc_mode.h"
#include "bll_presenter/remote/interface/dev_ctrl_adapter.h"
#include "bll_presenter/remote/media_control_impl.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/ethernet_controller.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/bluetooth_controller.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/media/image_manager.h"
#include "device_model/media/overlay_manager.h"
#include "device_model/media/video_alarm_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/media/recorder/recorder.h"
#include "device_model/media/recorder/stream_recorder.h"
#include "device_model/rtsp.h"
#include "device_model/display.h"
#include "window/window_manager.h"
#include "window/ipc_mode_window.h"
#include "window/status_bar_window.h"
#include "application.h"
#include "common/app_log.h"

#include "media/recorder/EyeseeRecorder.h"
#include "tutk/wrapper/tutk_wrapper.h"
#include "onvif/OnvifConnector.h"
#include "lua/lua_config_parser.h"

#include <sstream>

#undef LOG_TAG
#define LOG_TAG "ipc_mode.cpp"

using namespace EyeseeLinux;
using namespace tutk;
using namespace std;

IPCModePresenter::IPCModePresenter()
    : onvif_connector_(NULL)
    , status_(MODEL_UNINIT)
    , win_mg_(::WindowManager::GetInstance())
    , ipcmode_win_(NULL)
    , nm_(NetManager::GetInstance())
    , rtsp_flag_(false)
    , onvif_flag_(false)
    , tutk_flag_(false)
{
    pthread_mutex_init(&model_lock_, NULL);
    // GUI need this msg
    StorageManager::GetInstance()->Attach(this);

#ifdef ENABLE_RTSP
    if (rtsp_flag_) {
        rtsp_server_ = RtspServer::GetInstance();
    }
#endif

    // group 0 recorder id set
    RecorderIDSet grp0_rec_id_set;

    // 添加camera id和recorder type id到id group
    RecorderID grp0_rec0 = {CAM_IPC_0, REC_720P30FPS, REC_NORMAL, STREAM_SENDER_NONE};
    grp0_rec_id_set.push_back(grp0_rec0);

    RecorderID grp0_rec1 = {CAM_IPC_0, REC_S_VGA30FPS, REC_STREAM, (StreamSenderType)(STREAM_SENDER_RTSP|STREAM_SENDER_TUTK)};
    grp0_rec_id_set.push_back(grp0_rec1);

    rec_group_.insert(make_pair(CAM_IPC_0, grp0_rec_id_set));
#if 1
    // group 1 recorder id set
    RecorderIDSet grp1_rec_id_set;

    RecorderID grp1_rec0 = {CAM_IPC_1, REC_720P30FPS, REC_NORMAL, STREAM_SENDER_NONE};
    grp1_rec_id_set.push_back(grp1_rec0);

    RecorderID grp1_rec1 = {CAM_IPC_1, REC_S_VGA30FPS, REC_STREAM, STREAM_SENDER_RTSP};
    grp1_rec_id_set.push_back(grp1_rec1);

    rec_group_.insert(make_pair(CAM_IPC_1, grp1_rec_id_set));
#endif
    dev_adapter_ = new DeviceAdapter(this);

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

#ifdef TUTK_SUPPORT
    if (tutk_flag_) {
        // TODO: tutk去初始化会阻塞在IOTC_Device_Login导致线程无法正常退出，故暂时不停止tutk service
        tutk_connector_ = new TutkWrapper(dev_adapter_);
        LuaConfig config;
        config.LoadFromFile("/data/ipc_config.lua");
        string uuid = config.GetStringValue("system.uuid");
        if (uuid.empty())
            uuid = "A5H9X6ZEXT86GJKL111A";
        tutk_connector_->Init(uuid);
        tutk_connector_->Start();
    }
#endif
}

IPCModePresenter::~IPCModePresenter()
{
    db_msg("destruct");
    rec_group_.clear();

    if (dev_adapter_) {
        delete dev_adapter_;
        dev_adapter_ = NULL;
    }

    pthread_mutex_destroy(&model_lock_);
}

void IPCModePresenter::OnWindowLoaded()
{
    db_msg("window load");

    ipcmode_win_ = reinterpret_cast<IPCModeWindow*>(win_mg_->GetWindow(WINDOWID_IPCMODE));

    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }

    // enable ui layer alpha to show preview, set 255 can disable it
    Layer::GetInstance()->SetLayerAlpha(LAYER_UI, 150);

    // if enable auto record on boot
    ipcmode_win_->CamAutoCtrl(CAM_A, true);
    ipcmode_win_->CamAutoCtrl(CAM_B, true);

    int status;
    EventManager::GetInstance()->CheckEvent(EventManager::EVENT_SD, status);

    if (status == 0) {
        ipcmode_win_->ShowDialogWithMsg("stream_only", 2);
    }
}

void IPCModePresenter::OnWindowDetached()
{
    db_msg("window detach");
    if (status_ == MODEL_INITED) {
        this->DeviceModelDeInit();
    }
}

void IPCModePresenter::PrepareExit()
{
    // sync record file data to disk
    sync();

    StatusBarWindow *stb_win_ = reinterpret_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    stb_win_->StopUpdateDateTime();

    this->Notify(MSG_SHUTDOWN_SYSTEM); // MainModule will handle this message
}

int IPCModePresenter::HandleGUIMessage(int msg, int val)
{
    int ret = 0;
    switch(msg) {
        case IPCMODE_CAM_0_BUTTON:
        case IPCMODE_CAM_1_BUTTON: {
                CameraID id = ((msg - IPCMODE_CAM_1_BUTTON) < 0)?CAM_IPC_0:CAM_IPC_1;
                db_msg("cam on, id:%d, val:%d", id, val);
                this->CameraOnOffButtonHandler(id, val);
            }
            break;
        case IPCMODE_CAM_0_TAKEPIC_BUTTON:
        case IPCMODE_CAM_1_TAKEPIC_BUTTON: {
                CameraID id = ((msg - IPCMODE_CAM_1_TAKEPIC_BUTTON) < 0)?CAM_IPC_0:CAM_IPC_1;
                db_msg("take pic, id:%d, val:%d", id, val);
                this->TakePicButtonHandler(id, val);
            }
            break;
        case IPCMODE_SETTING_BUTTON:
            db_msg("setting button pushed");
            this->SettingButtonHandler(0, true);
            break;
        case IPCMODE_SHUTDOWN_BUTTON:
            this->PrepareExit();
            break;
        default:
            db_msg("there is no this button defined");
            break;
    }

    return ret;
}

void IPCModePresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

#ifdef ENABLE_RTSP
void* IPCModePresenter::RtspThreadLoop(void *context)
{
    IPCModePresenter *self = reinterpret_cast<IPCModePresenter*>(context);

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

void IPCModePresenter::MediaInit()
{

    CameraFactory *camera_factory = CameraFactory::GetInstance();
    for (RecorderGroup::iterator grp_iter = rec_group_.begin(); grp_iter != rec_group_.end(); grp_iter++) {
        // create camera
        Camera *cam = camera_factory->CreateCamera(grp_iter->first);
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

            rec_instance_grp.insert(make_pair(set_iter->rec_type, recorder));
        }

        cam_rec_map_.insert(make_pair(grp_iter->first, rec_instance_grp));
    }
}

#ifdef ENABLE_RTSP
int IPCModePresenter::CreateRtspServer()
{
    rtsp_server_ = RtspServer::GetInstance();

    string ip;

    if (nm_->GetNetDevIp("eth0", ip) < 0) {
        if (nm_->GetNetDevIp("wlan0", ip) < 0) {
            ip = "0.0.0.0";
            db_warn("no activity net device found, set rtsp server ip to '%s", ip.c_str());
            ipcmode_win_->ShowDialogWithMsg("net_error", 2);
        }
    }

    db_debug("create rtsp server on: %s", ip.c_str());

    return rtsp_server_->CreateServer(ip);
}

int IPCModePresenter::CreateRtspStreamSender()
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
                    recorder->SetEncodeDataCallback(IPCModePresenter::EncodeDataCallback, this);

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

int IPCModePresenter::DestroyRtspServer()
{
    if (rtsp_flag_) {
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
    }

    return 0;
}
#endif

int IPCModePresenter::RemoteServerInit()
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

int IPCModePresenter::RemoteServerStart()
{
#ifdef ENABLE_RTSP
    pthread_t rtsp_thread_id;
    ThreadCreate(&rtsp_thread_id, NULL, IPCModePresenter::RtspThreadLoop, this);
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

int IPCModePresenter::RemoteServerStop()
{
#if 0
#ifdef TUTK_SUPPORT
    tutk_connector_->Stop();
#endif
#endif
#ifdef ONVIF_SUPPORT
    if (onvif_flag_) {
        onvif_connector_->Stop();
    }
#endif
    return 0;
}

void IPCModePresenter::RemoteServerDeInit()
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

int IPCModePresenter::DeviceModelInit()
{
    db_msg("ipc mode device model init");

    this->MediaInit();
    this->RemoteServerInit();
    this->RemoteServerStart();

    MediaControlImpl *media_ctrl = static_cast<MediaControlImpl *>(dev_adapter_->media_ctrl_);
    media_ctrl->SetMediaCtrlInstance(cam_rec_map_, rec_group_);
    media_ctrl->SetStreamSenderInstance(stream_sender_map_);

    Layer::GetInstance()->Attach(this);
    NetManager::GetInstance()->Attach(this);

    ImageManager::GetInstance()->InitImage(camera_map_);
    OverlayManager::GetInstance()->InitOverlay(camera_map_);
    VideoAlarmManager::GetInstance()->InitVideoAlarm(camera_map_);

#ifdef BT_SUPPORT
     BlueToothController *bt_ctrl = BlueToothController::GetInstance();
     bt_ctrl->Attach(this);
     bt_ctrl->InitBlueToothDevice();
     bt_ctrl->EnableBlueToothDevice();
#endif

    status_ = MODEL_INITED;

    return 0;
}

int IPCModePresenter::DeviceModelDeInit()
{
    // pthread_mutex_lock(&model_lock_);
    this->RemoteServerStop();
    this->RemoteServerDeInit();

    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map_.begin(); cam_iter != camera_map_.end(); cam_iter++) {
        // 1. delete Recorder
        map<RecorderType, Recorder*>::iterator rec_iter;
        for (rec_iter = cam_rec_map_[cam_iter->first].begin();
            rec_iter != cam_rec_map_[cam_iter->first].end(); rec_iter++) {
            delete (rec_iter->second);
            rec_iter->second = NULL;
        }

        // 2. Stop Preview
        cam_iter->second->StopPreview();

        // 3. delete camera;
        delete (cam_iter->second);
        cam_iter->second = NULL;
    }
    cam_rec_map_.clear();
    camera_map_.clear();

    ImageManager::GetInstance()->ExitImage();
    OverlayManager::GetInstance()->ExitOverlay();
    VideoAlarmManager::GetInstance()->ExitVideoAlarm();

    Layer::GetInstance()->Detach(this);
    NetManager::GetInstance()->Detach(this);
    //    BlueToothController::GetInstance()->Detach(this);

    status_ = MODEL_UNINIT;

    // pthread_mutex_unlock(&model_lock_);

    return 0;
}

// 等待底层通知回调
void IPCModePresenter::Update(MSG_TYPE msg, int p_CamID)
{
    db_msg("handle msg: %d", msg);

    switch (msg) {
        case MSG_CAMERA_TAKEPICTURE_FINISHED: {
            // TODO: do this in other thread
            string pic_file = camera_map_[cam_id_]->GetPicFile();
            db_msg("pic: %s", pic_file.c_str());
            ipcmode_win_->ShowSnapPic(camera_map_[cam_id_]->GetCameraID(), pic_file);
        }
            break;
        case MSG_HDMI_PLUGIN:
        case MSG_HDMI_PLUGOUT:
        case MSG_TVOUT_PLUG_IN:
        case MSG_TVOUT_PLUG_OUT: {
            Layer::GetInstance()->SetLayerRectByDispType(LAYER_CAM0, CAM_A);
            Layer::GetInstance()->SetLayerRectByDispType(LAYER_CAM1, CAM_B);
        }
            break;
        case MSG_STORAGE_MOUNTED:
        case MSG_STORAGE_UMOUNT:
        case MSG_RECORD_START:
        case MSG_RECORD_STOP:
            this->Notify(msg);
            break;
        case MSG_WIFI_SWITCH_DONE:
        case MSG_SOFTAP_SWITCH_DONE:
        case MSG_ETH_SWITCH_DONE:
            RemoteServerStop();
            RemoteServerDeInit();
            sleep(1);
            RemoteServerInit();
            RemoteServerStart();
            for (unsigned int i = 0; i < rec_group_.size(); i++) {
                string demo_url = GetRtspUrl((CameraID)i);
                ipcmode_win_->SetRtspUrl(i, demo_url);
            }
            this->Notify(msg);
            break;
        default:
            break;
    }
}

// camera开关按键handler
void IPCModePresenter::CameraOnOffButtonHandler(const GroupID &grp_id, bool value)
{
    db_msg("id: %d, value: %d", grp_id, value);

    // TODO: make sure device has been inited
    CameraID cam_id = grp_id;
    map<CameraID, Camera*>::iterator iter = camera_map_.find(cam_id);
    if (iter == camera_map_.end()) {
        db_error("no camera can be used");
        return;
    }

    Camera *camera = camera_map_[cam_id];
    if (value) {
        camera->ShowPreview();
        // camera->StartPreview();

#ifdef ENABLE_RTSP
        // 更新UI数据
        // 设置RTSP url， 必需在启动流录像之前
        if (rtsp_flag_) {
            string demo_url = GetRtspUrl(cam_id);

            ipcmode_win_->SetRtspUrl(grp_id, demo_url);
        }
#endif
    } else {
        camera->HidePreview();
        // camera->StopPreview();
    }

    RecorderIDSet rec_id_set = rec_group_[grp_id];
    RecorderIDSet::iterator rec_id_iter;
    for (rec_id_iter = rec_id_set.begin(); rec_id_iter != rec_id_set.end(); rec_id_iter++) {
        if (value) {
            int ret = this->StartRecord(cam_rec_map_[grp_id][rec_id_iter->rec_type], grp_id);

            if (ret < 0 && rec_id_iter->rec_model == REC_NORMAL) {
                int status;
                EventManager::GetInstance()->CheckEvent(EventManager::EVENT_SD, status);

                db_debug("status: %d", status);
                if (status == 0) {
                    db_debug("stream_only");
                    ipcmode_win_->ShowDialogWithMsg("stream_only", 2);
                }
            }
        } else {
            this->StopRecord(cam_rec_map_[grp_id][rec_id_iter->rec_type]);
        }
    }
}

string IPCModePresenter::GetRtspUrl(const GroupID &grp_id) {
    RecorderIDSet rec_id_set = rec_group_[grp_id];
    RecorderIDSet::iterator iter;
    string demo_url;
    for (iter = rec_id_set.begin(); iter != rec_id_set.end(); iter++) {
        Recorder *recorder = cam_rec_map_[iter->cam_id][iter->rec_type];
        StreamSenderMap::iterator r_iter = stream_sender_map_.find(recorder);
        if (r_iter != stream_sender_map_.end()) {
            string url = r_iter->second->GetUrl();
            db_msg("%s", url.c_str());
            if ( (iter->rec_type == REC_S_VGA30FPS && (iter->sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP)
                    || rec_id_set.size() == 1) {
                demo_url = url;
            }
        }
    }

    return demo_url;
}

void IPCModePresenter::SettingButtonHandler(uint8_t id, bool value)
{
}

int IPCModePresenter::StartRecord(Recorder *recorder, CameraID cam_id)
{
    int ret = 0;
    //uint16_t status = recorder->GetStatus();

    //if (status == RECORDER_IDLE) { // when restart record
    //RecorderParam param;
    //recorder->GetParam(param);
    //recorder->SetParam(param);
    // init and start recorder
    //}

    if (recorder != NULL)
        ret = recorder->StartRecord();

    return ret;
}

int IPCModePresenter::StopRecord(Recorder *recorder)
{
    int ret = 0;

    if (recorder != NULL)
        ret = recorder->StopRecord();

    return ret;
}

void IPCModePresenter::EncodeDataCallback(EncodeDataCallbackParam *param)
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

    IPCModePresenter *self = static_cast<IPCModePresenter *>(param->context_);
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

        if (self->rtsp_flag_) {
            if ((sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
                self->SendRtspData(frame, rec, self);
            }
        }

        if (self->tutk_flag_) {
            if ((sender_type & STREAM_SENDER_TUTK) == STREAM_SENDER_TUTK) {
                self->SendTutkData(frame, rec, self);
            }
        }

#ifdef WRITE_RAW_H264_FILE
        self->WriteRawData(frame, rec, self);
#endif

        rec->GetEyeseeRecorder()->freeOneBsFrame(frame);
    }
}

void IPCModePresenter::SendRtspData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self)
{
#ifdef ENABLE_RTSP
    if (self->stream_sender_map_.size() == 0) return;
    if (self->rtsp_server_->GetServerStatus() == RtspServer::SERVER_STOPED) return;

    RtspServer::StreamSender *stream_sender = self->stream_sender_map_[rec];
    MediaStream::FrameDataType frame_type;

    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame

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

void IPCModePresenter::SendTutkData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self)
{
#ifdef TUTK_SUPPORT
    if (self->tutk_connector_ == NULL) return;

    RemoteConnector *tutk_connector = self->tutk_connector_;

    FRAMEINFO_t frame_info;
    memset(&frame_info, 0, sizeof(FRAMEINFO_t));
    frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
    frame_info.timestamp = (unsigned int) frame->pts;

    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame

            // send sps/pps first
            VencHeaderData sps_info = {NULL, 0}, pps_info = {NULL, 0};
            rec->GetVencHeaderData(sps_info, pps_info);

            if (sps_info.pBuffer != NULL && sps_info.nLength != 0)
                tutk_connector->SendVideoData((const char *) sps_info.pBuffer, sps_info.nLength, &frame_info, sizeof(frame_info));
            if (pps_info.pBuffer != NULL && pps_info.nLength != 0)
                tutk_connector->SendVideoData((const char *) pps_info.pBuffer, pps_info.nLength, &frame_info, sizeof(frame_info));

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

void IPCModePresenter::WriteRawData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self)
{
    static int flag = 1;
#ifdef WRITE_RAW_H264_FILE
    if (frame->stream_type == 0x00) { // video
        if (flag == 1 || (*(frame->data + 4) & 0x1F) == 5) { // if is I frame
            VencHeaderData head_info = {NULL, 0};
            rec->GetVencHeaderData(head_info);

            // write sps/pps at the begining of the file and the front of I frame
            if (head_info.pBuffer != NULL && head_info.nLength != 0)
                fwrite(head_info.pBuffer, head_info.nLength, 1, rec->file_);
            flag = 0;
        }

        fwrite(frame->data, frame->data_size, 1, rec->file_);
    }
    //else {
    //    db_msg("stream type: %d", frame->stream_type);
    //}
#endif
}

void IPCModePresenter::TakePicButtonHandler(const CameraID &id, bool value)
{
    cam_id_ = id;
    camera_map_[id]->TakePicture(0);
}
