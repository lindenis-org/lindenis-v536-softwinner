/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file sample_ipc.h
 * @brief 无UI，单IPC功能，本地录像存储+RTSP
 * @author id:826
 * @version v0.3
 * @date 2016-08-11
 */
#pragma once

#include "common_type.h"

/* tutk */
class DeviceAdapter;
class RemoteConnector;

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup IPCMode
 * @{
 */

/**
 * @brief 该presenter的操作命令定义
 */
typedef enum {
    CMD_RECORD = 0,
    CMD_TAKEPIC,
} SAMPLEIPC_CMD;

class NetManager;
class SampleIPCPresenter
    : public IPresenter
    , public ISubject
    , public IObserver
{
    public:
        SampleIPCPresenter();
        ~SampleIPCPresenter();

        int DeviceModelInit();
        int DeviceModelDeInit();

        void StartSampleIPC();
        void ExitSampleIPC();
        void SampleIPCCtrl(const GroupID &grp_id, int cmd, int val);

        void Update(MSG_TYPE msg, int p_CamID=0);

#ifdef ENABLE_RTSP
        static void* RtspThreadLoop(void *context);
#endif
        static void EncodeDataCallback(EncodeDataCallbackParam *param);
    private:
        std::map<CameraID, Camera*> camera_map_;
        CamRecMap cam_rec_map_;
        RecorderGroup rec_group_;

        DeviceAdapter *dev_adapter_;
        RemoteConnector *tutk_connector_;
        NetManager *nm_;

        int status_;
        pthread_mutex_t model_lock_;

#ifdef ENABLE_RTSP
        void PrintRtspUrl(const GroupID &grp_id);
        RtspServer *rtsp_server_;
        StreamSenderMap stream_sender_map_;
#endif

        void CameraOnOffHandler(const GroupID &grp_id, bool value);
        void TakePicHandler(const GroupID &grp_id, bool value);
        void MediaInit();
        int RemoteServerInit();
        int RemoteServerStart();
        int RemoteServerStop();
        void StartRecord(Recorder *recorder, CameraID cam_id);
        void StopRecord(Recorder *recorder);
        int CreateRtspServer();

        static void SendRtspData(const VEncBuffer *frame, Recorder *rec, SampleIPCPresenter *self);
        static void SendTutkData(const VEncBuffer *frame, Recorder *rec, SampleIPCPresenter *self);
}; /* class SampleIPCPresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
