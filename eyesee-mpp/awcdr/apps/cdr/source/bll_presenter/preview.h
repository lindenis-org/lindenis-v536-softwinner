/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview.h
 * @brief 单路预览录像presenter
 * @author id:826
 * @version v0.3
 * @date 2016-08-01
 */
#pragma once

#include "bll_presenter/gui_presenter_base.h"
#include "common_type.h"
#include "common/posix_timer.h"
#include "main.h"
#include <time.h>
#include <signal.h>
#include <thread>

#include <map>

/* tutk */
class DeviceAdapter;
class RemoteConnector;

class WindowManager;
class PreviewWindow;

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup RecPlay
 * @{
 */
typedef std::map<CameraID, std::map<RecorderType, Recorder*> > CamRecMap;

class LuaConfig;
class VideoPlayer;
class NetManager;

class PreviewPresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(PreviewPresenter)
    , public IObserverWrap(PreviewPresenter)

{
    public:
        PreviewPresenter(MainModule *mm);

        ~PreviewPresenter();

        void OnWindowLoaded();

        void OnUILoaded();

        void OnWindowDetached();

        void PrepareExit();

        int HandleGUIMessage(int msg, int val);

        void BindGUIWindow(::Window *win);

        void MediaInit();

		int BackCameraMediaInit();

        int DeviceModelInit();

        int DeviceModelDeInit();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

        static void *RtspThreadLoop(void *context);

        static void EncodeDataCallback(EncodeDataCallbackParam *param);
        //static void TimeTakePicTimerProc(union sigval sigval);
        void TakePicforVideothumb(Recorder *recorder,const CameraID &cam_id);
        void ClosePicforVideothumb(const CameraID &cam_id);
        static void CheckBatteryStatusTimerProc(union sigval sigval);
        int WifiSOftApEnable();
        int WifiSOftApDisable();
        int RemoteSwitchRecord();
        int RemoteTakePhoto();
        int GetRemotePhotoInitState(bool &init_state);
        int CheckStorage(void);
		void TimeTakePic();
		void RestartCountdown();
        int RemoteSwitchSlowRecord();
        int setWifiFlagtoRecorder(const CameraID &p_nCamId,bool flag);
#ifdef SHOW_DEBUG_INFO
        static void DebugInfoThread(PreviewPresenter *self);
#endif
    private:
        enum WorkingMode {
            NORMAL_MODE = 1,
            USB_MODE = 2,
            USB_MODE_MASS_STORAGE,
            USB_MODE_UVC,
            USB_MODE_CHARGE,
        };

        std::map<CameraID, Camera*> camera_map_;
        CamRecMap cam_rec_map_;
        RecorderGroup rec_group_;

        int status_;
        pthread_mutex_t model_lock_;
        WindowManager *win_mg_;
        PreviewWindow *preview_win_;
        NetManager *nm_;

        DeviceAdapter *dev_adapter_;
        RemoteConnector *tutk_connector_;
        RemoteConnector *onvif_connector_;
        MainModule *mainmodule;
        bool deinit_flag_;
        bool rtsp_flag_;
        bool onvif_flag_;
        bool tutk_flag_;
        bool audio_record_;
        bool wifi_statu_;
        bool isAllowTakePIC;
        bool isContinueTakePicMode;
        bool isTimeTakePicMode;
        bool isAutoTimeTakePicMode;
        bool isWifiSoftEnable;
        bool isRecordStart;
        timer_t time_take_pic_timer_id_;
        int last_win_statu_msg;
        int win_statu_msg;
        int storage_status_;
        WorkingMode mode_;
        bool m_usb_attach_status;
        cdx_sem_t mTakePhotoOver;
        bool re_init_camera_slowrecod;
        bool re_init_camera_photo;
        bool osd_flag;
	    bool m_HdmiDevConn;
        CameraChnConfigParam m_cam_Chnparam;
        bool isAutoTakeTimerStart;
        bool isTimeTakeTimerStart;
        std::thread debug_info_thread_;
        std::mutex disp_switch_mutex_;
		bool mFrontCamRecordStartStatus;
		bool mBackCamRecordStartStatus;
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
         * @param cam_id camera id
         * @param value 按键值
         * @param force 强制终止
         *  - true on
         *  - false off
         *
         *  当有多个camera开关按钮时就需要在OnButtonPushed中传入子id
         *
         * @see PreviewPresenter::OnButtonPushed
         */
        void TakePicButtonHandler(const CameraID &cam_id, bool value, bool force = false);

        int GetWifiInfo();

        void SetCountdownSec(int sec,bool loop);

        void StopAutoLoopCountdown(void);

        void SetDightZoomButtonHandler(const CameraID &cam_id, int value);

        void RecordAudioSwitch(const CameraID &cam_id);

        void AudioSwitch(Recorder *recorder);

        bool GetAudioStatus(const CameraID &cam_id);

        int RemoteServerInit();

        int RemoteServerStart();

        int RemoteServerStop();

        void RemoteServerDeInit();

        int StartRecord(Recorder *recorder, CameraID cam_id);

        int StopRecord(Recorder *recorder);

        static void SendRtspData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self);

        static void SendTutkData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self);

        static void PutUVCData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self);

        static void WriteRawData(const VEncBuffer *frame, Recorder *rec, PreviewPresenter *self);

        void MediaDeInit();

		int BackCameraMediaDeInit();

        int NormalInit();

		int BackCameraInit();

		int BackCameraDeInit();

        int NormalDeInit();

        int UVCModeInit();

        int UVCModeDeInit();

        int UVCModeStart();

        int UVCModeStop();
	
	void SetDightZoomValue(int zoom_val);

       void sendUsbConnectMessage();

	   int SwitchLayer(CameraPreviewType p_oldPreviewType, CameraPreviewType p_newPreviewType);
    private:
        typedef enum eRECRODTYPE
        {
            NORMAL_RECORD = 0,
            SLOW_RECORD = 1,
        }RecType;

        timer_t lowpower_shutdown_timer_id_;
        bool lowpower_shutdown_processing_;
    private:
        /****************************************************
                 * Name:
                 *     RecordHandle()
                 * Function:
                 *     set record parameter , camera parameter& start or stop record
                 * Parameter:
                 *     input:
                 *         p_nRecordType:  normal record or slow record
                 *         p_nCamId: camera id 
                 *         p_bVal: 
                 *               0:  stop record  
                 *               1:  start record
                 *     output:
                 *         none
                 * return:
                 *     0: success
                 *     -1: failed
       *****************************************************/
       int RecordHandle(const CameraID &p_nCamId, bool p_bVal);

	/****************************************************
		 * Name:
		 *	   ReConfigResolution()
		 * Function:
		 *	   check camera and recorder resolution, if device's  resolution is not equal to 
		 *	   config resolution,then chang it according to config resolution. 
		 * Parameter:
		 *	   input:
		 *		 p_Camera:  camera object
		 *		 p_Record:   recorder object
		 *	   output:
		 *		 none
		 * return:
		 *	   0: success
		 *	   -1: failed
	*****************************************************/
	int ReConfigResolution(RecType p_nRecordType, Camera *p_Camera, Recorder *p_Record);

       /****************************************************
                * Name:
                *     GetConfigVideoResolution()
                * Function:
                *     get normal record resolution id 
                * Parameter:
                *     input:
                *       none;
                *     output:
                *         p_nIndexID:  resolution id
                * return:
                *     0: success
                *     -1: failed
       *****************************************************/
       int GetConfigVideoResolution(int &p_nIndexID);

       /****************************************************
                * Name:
                *     GetSlowRecordVideoResolution()
                * Function:
                *     get slow record resolution id 
                * Parameter:
                *     input:
                *       none;
                *     output:
                *         p_nIndexID:  resolution id
                * return:
                *     0: success
                *     -1: failed
       *****************************************************/
       int GetSlowRecordVideoResolution(int &p_nIndexID);
       int GetThumbVideoFileName(const CameraID &p_nCamId);
       int StreamRecordHandle(int val);
	int PlugInHandle(int msg);
	int PlugOutHandle(int msg);

    /****************************************************
                * Name:
                *     remoteTakePicture()
                * Function:
                *     remote apk take picture
                * Parameter:
                *     input:
                *       none;
                *     output:
                *         none
                * return:
                *     0: success
                *     -1: failed
       *****************************************************/
    int  remoteTakePicture();

    void SetRecordMute(bool value);

    void DoSystemShutdown();

    void StopAllRecord();

    void CameraOnError();

    static void LowPowerShutdownTimerHandler(union sigval sigval);
}; /* class PreviewPresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
