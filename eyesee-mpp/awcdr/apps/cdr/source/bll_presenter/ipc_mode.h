/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ipc_mode.h
 * @brief ipc工作模式presenter
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#pragma once

#include "bll_presenter/gui_presenter_base.h"
#include "common_type.h"

/* tutk */
class DeviceAdapter;
class RemoteConnector;

/* GUI */
class Window;
class WindowManager;
class IPCModeWindow;

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup IPCMode
 * @{
 */

class NetManager;

class IPCModePresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubject
    , public IObserver
{
    public:
        IPCModePresenter();

        ~IPCModePresenter();

        void OnWindowLoaded();

        void OnWindowDetached();

        void PrepareExit();

        int HandleGUIMessage(int msg, int val);

        void BindGUIWindow(::Window *win);

        int DeviceModelInit();

        int DeviceModelDeInit();

        void Update(MSG_TYPE msg, int p_CamID=0);

        static void* RtspThreadLoop(void *context);

        static void EncodeDataCallback(EncodeDataCallbackParam *param);
    private:
        std::map<CameraID, Camera*> camera_map_;
        CamRecMap cam_rec_map_;
        RecorderGroup rec_group_;

        DeviceAdapter *dev_adapter_;
        RemoteConnector *tutk_connector_;
        RemoteConnector *onvif_connector_;

        int status_;
        pthread_mutex_t model_lock_;
        WindowManager *win_mg_;
        IPCModeWindow *ipcmode_win_;
        NetManager *nm_;
        CameraID cam_id_;
        bool rtsp_flag_;
        bool onvif_flag_;
        bool tutk_flag_;

#ifdef ENABLE_RTSP
        RtspServer *rtsp_server_;
        StreamSenderMap stream_sender_map_;

        std::string GetRtspUrl(const CameraID &grp_id);

        int CreateRtspServer();

        int CreateRtspStreamSender();

        int DestroyRtspServer();
#endif

        /**
         * @brief 具体的Camera开关按钮事件处理handler
         * @param grp_id 按键子id
         * @param value 按键值
         *  - true on
         *  - false off
         *
         *  当有多个camera开关按钮时就需要在OnButtonPushed中传入子id
         *
         * @see IPCModePresenter::OnButtonPushed
         */
        void CameraOnOffButtonHandler(const GroupID &grp_id, bool value);

        /**
         * @brief 具体的设置按钮事件处理handler
         * @param id 按键子id
         * @param value 按键值
         *  - true on
         *  - false off
         *
         *  当有多个设置按钮时就需要在OnButtonPushed中传入子id
         *
         * @see IPCModePresenter::OnButtonPushed
         */
        void SettingButtonHandler(uint8_t id, bool value);

        /**
         * @brief 拍照按钮事件处理handler
         * @param id 按键子id
         * @param value 按键值, 应该始终为On
         *  - true on
         *  - false off
         *
         *  当有多个拍照按钮时就需要在OnButtonPushed中传入子id
         *
         * @see IPCModePresenter::OnButtonPushed
         */
        void TakePicButtonHandler(const CameraID &id, bool value);

        void MediaInit();

        int RemoteServerInit();

        int RemoteServerStart();

        int RemoteServerStop();

        void RemoteServerDeInit();

        int StartRecord(Recorder *recorder, CameraID cam_id);

        int StopRecord(Recorder *recorder);

        static void SendRtspData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self);

        static void SendTutkData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self);

        static void WriteRawData(const VEncBuffer *frame, Recorder *rec, IPCModePresenter *self);
}; /* class IPCModePresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
