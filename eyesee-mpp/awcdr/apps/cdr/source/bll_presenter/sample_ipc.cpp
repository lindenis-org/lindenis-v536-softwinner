/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file sample_ipc.cpp
 * @brief 无UI，单IPC功能，本地录像存储+RTSP
 * @author id:826
 * @version v0.3
 * @date 2016-08-11
 */
#include "bll_presenter/sample_ipc.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/ethernet_controller.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/bluetooth_controller.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/media/recorder/recorder.h"
#include "device_model/media/recorder/stream_recorder.h"
#include "device_model/rtsp.h"
#include "common/app_log.h"

#include "tutk/wrapper/tutk_wrapper.h"
#include "bll_presenter/remote/interface/dev_ctrl_adapter.h"
#include "bll_presenter/remote/media_control_impl.h"
#include "lua/lua_config_parser.h"

#include <sstream>

#undef LOG_TAG
#define LOG_TAG "sample_ipc"

using namespace EyeseeLinux;
using namespace std;
using namespace tutk;

SampleIPCPresenter::SampleIPCPresenter()
    : dev_adapter_(NULL)
    , tutk_connector_(NULL)
    , nm_(NetManager::GetInstance())
{
    status_ = MODEL_UNINIT;
    pthread_mutex_init(&model_lock_, NULL);
#ifdef ENABLE_RTSP
    rtsp_server_ = RtspServer::GetInstance();
#endif

    // group 0 recorder id set
    RecorderIDSet grp0_rec_id_set;

    // 添加camera id和recorder type id到id group
    RecorderID grp0_rec0 = {CAM_IPC_0, REC_1080P30FPS, REC_STREAM, STREAM_SENDER_RTSP};
    grp0_rec_id_set.push_back(grp0_rec0);

    RecorderID grp0_rec1 = {CAM_IPC_0, REC_S_VGA30FPS, REC_STREAM, (StreamSenderType)(STREAM_SENDER_RTSP|STREAM_SENDER_TUTK)};
    grp0_rec_id_set.push_back(grp0_rec1);

    rec_group_.insert(make_pair(CAM_IPC_0, grp0_rec_id_set));
#if 1
    // group 1 recorder id set
    RecorderIDSet grp1_rec_id_set;

    RecorderID grp1_rec0 = {CAM_IPC_1, REC_S_VGA30FPS, REC_STREAM, STREAM_SENDER_RTSP};
    grp1_rec_id_set.push_back(grp1_rec0);

    rec_group_.insert(make_pair(CAM_IPC_1, grp1_rec_id_set));
#endif

    dev_adapter_ = new DeviceAdapter(this);

#ifdef TUTK_SUPPORT
    // TODO: tutk去初始化会阻塞在IOTC_Device_Login导致线程无法正常退出，故暂时不停止tutk service
    tutk_connector_ = new TutkWrapper(dev_adapter_);
    LuaConfig config;
    config.LoadFromFile("/usr/share/app/ipc/ipc_config.lua");
    string uuid = config.GetStringValue("system.uuid");
    if (uuid.empty())
        uuid = "A5H9X6ZEXT86GJKL111A";
    tutk_connector_->Init(uuid);
    tutk_connector_->Start();
#endif
}

SampleIPCPresenter::~SampleIPCPresenter()
{
    rec_group_.clear();
}

void SampleIPCPresenter::StartSampleIPC()
{
    db_msg("start sample ipc");
    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }
}

void SampleIPCPresenter::ExitSampleIPC()
{
    db_msg("exit sample ipc");
    if (status_ == MODEL_INITED) {
        this->DeviceModelDeInit();
    }
}

void SampleIPCPresenter::SampleIPCCtrl(const GroupID &grp_id, int cmd, int val)
{
    switch(cmd) {
        case CMD_RECORD: {
                db_msg("cam on, id:%d, val:%d", grp_id, val);
                this->CameraOnOffHandler(grp_id, val);
            }
            break;
        case CMD_TAKEPIC: {
                db_msg("take pic, id:%d, val:%d", grp_id, val);
                this->TakePicHandler(grp_id, val);
            }
            break;
        default:
            db_msg("there is no this button defined");
            break;
    }
}

#ifdef ENABLE_RTSP
void* SampleIPCPresenter::RtspThreadLoop(void *context)
{
    SampleIPCPresenter *self = reinterpret_cast<SampleIPCPresenter*>(context);

    prctl(PR_SET_NAME, "RtspThreadLoop", 0, 0, 0);

    self->rtsp_server_->Run();

    return NULL;
}
#endif

void SampleIPCPresenter::MediaInit()
{
#ifdef ENABLE_RTSP
    CreateRtspServer();
#endif
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

#ifdef ENABLE_RTSP
            // create rtsp stream sender
            if (set_iter->rec_model == REC_STREAM) {
                recorder->SetEncodeDataCallback(SampleIPCPresenter::EncodeDataCallback, this);

                std::stringstream ss;
                RtspServer::StreamSender *stream_sender = NULL;

                rtsp_server_ = RtspServer::GetInstance();
                ss << "ch" << cam->GetCameraID() << set_iter->rec_type;
                stream_sender = rtsp_server_->CreateStreamSender(ss.str());

                if (stream_sender != NULL) {
                    stream_sender_map_.insert(make_pair(recorder, stream_sender));
                }
            }
#endif
            rec_instance_grp.insert(make_pair(set_iter->rec_type, recorder));
        }

        cam_rec_map_.insert(make_pair(grp_iter->first, rec_instance_grp));
    }

#ifdef ENABLE_RTSP
    pthread_t rtsp_thread_id;
    ThreadCreate(&rtsp_thread_id, NULL, SampleIPCPresenter::RtspThreadLoop, this);
#endif

}

int SampleIPCPresenter::CreateRtspServer()
{
    rtsp_server_ = RtspServer::GetInstance();

    string ip;

    if (nm_->GetNetDevIp("eth0", ip) < 0) {
        if (nm_->GetNetDevIp("wlan0", ip) < 0) {
            ip = "0.0.0.0";
            db_warn("no activity net device found, set rtsp server ip to '%s", ip.c_str());
        }
    }

    return rtsp_server_->CreateServer(ip);
}

int SampleIPCPresenter::RemoteServerInit()
{
#if 0
#ifdef TUTK_SUPPORT
    tutk_connector_ = new TutkWrapper(dev_adapter_);
#else
    tutk_connector_ = new RemoteConnector();
#endif
#endif
    return 0;
}

int SampleIPCPresenter::DeviceModelInit()
{
    db_msg("ipc mode device model init");

    this->MediaInit();
#if defined (TUTK_SUPPORT) || defined (ONVIF_SUPPORT)
    MediaControlImpl *media_ctrl = static_cast<MediaControlImpl*>(dev_adapter_->media_ctrl_);
    media_ctrl->SetMediaCtrlInstance(cam_rec_map_, rec_group_);
#endif

    //EventManager::GetInstance()->Attach(this);
    EtherController::GetInstance()->Attach(this);
    WifiController::GetInstance()->Attach(this);
    SoftApController::GetInstance()->Attach(this);
#ifdef BT_SUPPORT
     BlueToothController *bt_ctrl = BlueToothController::GetInstance();
     bt_ctrl->Attach(this);
     bt_ctrl->InitBlueToothDevice();
     bt_ctrl->EnableBlueToothDevice();
#endif

    status_ = MODEL_INITED;

    return 0;
}

int SampleIPCPresenter::DeviceModelDeInit()
{
    // pthread_mutex_lock(&model_lock_);

    map<CameraID, Camera*>::iterator cam_iter;
    for (cam_iter = camera_map_.begin(); cam_iter != camera_map_.end(); cam_iter++) {
        // 1. delete Recorder
        map<RecorderType, Recorder*>::iterator rec_iter;
        for (rec_iter = cam_rec_map_[cam_iter->first].begin();
            rec_iter != cam_rec_map_[cam_iter->first].end(); rec_iter++) {
            delete (rec_iter->second);
            rec_iter->second = NULL;
        }

        // 2. delete camera;
        delete (cam_iter->second);
        cam_iter->second = NULL;
    }
    cam_rec_map_.clear();
    camera_map_.clear();

#ifdef ENABLE_RTSP
    rtsp_server_->Stop();

    map<Recorder*, RtspServer::StreamSender*>::iterator sender_iter;
    for (sender_iter = stream_sender_map_.begin(); sender_iter != stream_sender_map_.end(); sender_iter++) {
        delete (sender_iter->second);
    }
    stream_sender_map_.clear();

    usleep(500*1000);
    RtspServer::Destroy();
    rtsp_server_ = NULL;
#endif
    if (tutk_connector_) {
        delete tutk_connector_;
        tutk_connector_ = NULL;
    }

#if defined(TUTK_SUPPORT) || defined (ONVIF_SUPPORT)
    if (dev_adapter_) {
        delete dev_adapter_;
        dev_adapter_ = NULL;
    }
#endif

    //EventManager::GetInstance()->Detach(this);
    EtherController::GetInstance()->Detach(this);
    WifiController::GetInstance()->Detach(this);
    SoftApController::GetInstance()->Detach(this);
//    BlueToothController::GetInstance()->Detach(this);

    status_ = MODEL_UNINIT;

    // pthread_mutex_unlock(&model_lock_);

    return 0;
}

// 等待底层通知回调
void SampleIPCPresenter::Update(MSG_TYPE msg, int p_CamID)
{
    this->Notify(msg);
}

// camera开关按键handler
void SampleIPCPresenter::CameraOnOffHandler(const GroupID &grp_id, bool value)
{
    db_msg("group_id: %d, value: %d", grp_id, value);

    // TODO: make sure device has been inited
    CameraID cam_id = grp_id;
    map<CameraID, Camera*>::iterator iter = camera_map_.find(cam_id);
    if (iter == camera_map_.end()) {
        db_error("cam[%d] instance is not exist", cam_id);
        return;
    }

#ifdef ENABLE_RTSP
    this->PrintRtspUrl(grp_id);
#endif

    RecorderIDSet rec_id_set = rec_group_[grp_id];
    RecorderIDSet::iterator rec_id_iter;
    for (rec_id_iter = rec_id_set.begin(); rec_id_iter != rec_id_set.end(); rec_id_iter++) {
        if (value) {
            this->StartRecord(cam_rec_map_[grp_id][rec_id_iter->rec_type], grp_id);
        } else {
            this->StopRecord(cam_rec_map_[grp_id][rec_id_iter->rec_type]);
        }
    }

}

#ifdef ENABLE_RTSP
void SampleIPCPresenter::PrintRtspUrl(const GroupID &grp_id)
{
    RecorderIDSet rec_id_set = rec_group_[grp_id];
    RecorderIDSet::iterator iter;
    for (iter = rec_id_set.begin(); iter != rec_id_set.end(); iter++) {
        Recorder *recorder = cam_rec_map_[iter->cam_id][iter->rec_type];
        StreamSenderMap::iterator r_iter = stream_sender_map_.find(recorder);
        if (r_iter != stream_sender_map_.end()) {
            string url = r_iter->second->GetUrl();
            db_msg("%s", url.c_str());
        } else {
            db_error("can not found stream_sender by cam_id[%d], rec_type[%d]", iter->cam_id, iter->rec_type);
        }

    }
}
#endif

void SampleIPCPresenter::StartRecord(Recorder *recorder, CameraID cam_id)
{
    //uint16_t status = recorder->GetStatus();

    //if (status == RECORDER_IDLE) { // when restart record
    //    RecorderParam param;
    //    recorder->GetParam(param);
    //    recorder->SetParam(param);
    //}
    if (recorder != NULL)
        recorder->StartRecord();
}

void SampleIPCPresenter::StopRecord(Recorder *recorder)
{
    if (recorder != NULL)
        recorder->StopRecord();
}

void SampleIPCPresenter::EncodeDataCallback(EncodeDataCallbackParam *param)
{
    int ret = 0;
    int sender_type = 0;
    VEncBuffer* frame = NULL;
    Recorder *rec = param->rec_;

    switch (param->what_) {
        case MPP_EVENT_ERROR_ENCBUFFER_OVERFLOW:
            db_error("send data too slow!!!");
            break;
        default:
            break;
    }

    SampleIPCPresenter *self = static_cast<SampleIPCPresenter *>(param->context_);
    sender_type = rec->GetStreamSenderType();

    while (1) {

        frame = rec->GetEyeseeRecorder()->getOneBsFrame();
        if (frame == NULL)
            return;

        if ((sender_type & STREAM_SENDER_RTSP) == STREAM_SENDER_RTSP) {
            self->SendRtspData(frame, rec, self);
        }

        if ((sender_type & STREAM_SENDER_TUTK) == STREAM_SENDER_TUTK) {
            self->SendTutkData(frame, rec, self);
        }

        rec->GetEyeseeRecorder()->freeOneBsFrame(frame);
        usleep(5000);
    }
}

void SampleIPCPresenter::SendRtspData(const VEncBuffer *frame, Recorder *rec, SampleIPCPresenter *self)
{
#ifdef ENABLE_RTSP
    RtspServer::StreamSender *stream_sender = self->stream_sender_map_[rec];
    MediaStream::FrameDataType frame_type;

    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame

            // send sps/pps first
            VencHeaderData sps_info, pps_info;
            rec->GetVencHeaderData(sps_info, pps_info);

            stream_sender->SendVideoData(sps_info.pBuffer, sps_info.nLength, MediaStream::FRAME_DATA_TYPE_SPS);
            stream_sender->SendVideoData(pps_info.pBuffer, pps_info.nLength, MediaStream::FRAME_DATA_TYPE_PPS);

            frame_type = MediaStream::FRAME_DATA_TYPE_I;
        }
        else {
            frame_type = MediaStream::FRAME_DATA_TYPE_P;
        }

        stream_sender->SendVideoData((unsigned char *) frame->data, frame->data_size, frame_type);
    }
    //else {
    //    db_msg("stream type: %d", frame->stream_type);
    //}
#endif
}

void SampleIPCPresenter::SendTutkData(const VEncBuffer *frame, Recorder *rec, SampleIPCPresenter *self)
{
#ifdef TUTK_SUPPORT
    RemoteConnector *tutk_connector = self->tutk_connector_;

    FRAMEINFO_t frame_info;
    memset(&frame_info, 0, sizeof(FRAMEINFO_t));
    frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
    frame_info.timestamp = (unsigned int) frame->pts;

    if (frame->stream_type == 0x00) { // video
        if ((*(frame->data + 4) & 0x1F) == 5) { // if is I frame

            // send sps/pps first
            VencHeaderData sps_info, pps_info;
            rec->GetVencHeaderData(sps_info, pps_info);

            tutk_connector->SendVideoData((const char *) sps_info.pBuffer, sps_info.nLength, &frame_info, sizeof(frame_info));
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

void SampleIPCPresenter::TakePicHandler(const GroupID &grp_id, bool value)
{
   camera_map_[grp_id]->TakePicture(0);
}

int SampleIPCPresenter::RemoteServerStart()
{
    tutk_connector_->Start();
    return 0;
}

int SampleIPCPresenter::RemoteServerStop()
{
    tutk_connector_->Stop();
    return 0;
}
