/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file device_setting.h
 * @brief 设置界面presenter
 * @author id:826
 * @version v0.3
 * @date 2016-09-19
 */
#pragma once

#include "common/subject.h"
#include "common/observer.h"
#include "bll_presenter/presenter.h"
#include "bll_presenter/gui_presenter_base.h"
#include "window/setting_window.h"
#include "window/window_manager.h"
#include "main.h"


#include <map>

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup RecPlay
 * @{
 */

class Window;
class NetManager;
class DeviceSettingPresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(DeviceSettingPresenter)
    , public IObserverWrap(DeviceSettingPresenter)
{
    public:
        DeviceSettingPresenter(MainModule *mm);
        ~DeviceSettingPresenter();

        void OnWindowLoaded();
        void OnWindowDetached();
        int HandleGUIMessage(int msg, int val,int id=0);
        void BindGUIWindow(::Window *win);
        int DeviceModelInit();
        int DeviceModelDeInit();
        void PrepareExit() {};
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
	void HandleButtonDialogMsg(int val);
        int WifiSwitch(int val);
        int SoftAPSwitch(int val);
        int FormatStorage();
        int GetDeviceInfo(int msgid);
	 int GetWifiInfo(int msgid);
	 int GetWifiAppDownloadQRcode(int msgid);
	 int GetTrafficInfo(int msgid);
	 int GetFlowRechargeQRcode(int msgid);
	 int GetSimInfo(int msgid);
	 void ShowLevelBar(int levelbar_id);
        int GetSubMenuData(int index);
        void SetVideoResoulation(int p_CamId, int val);
        int GetVideoResolution(int p_CamId, Size &p_size);
        void SetEncoderType(int p_CamId, int val);
        void SetPicResolution(int p_CamId, int val);
        int GetPicResolution(int p_CamId, Size &p_size);
        void SetRecordAudioOnOff(int p_CamId, int val);
        Recorder* getRecorder(int p_CamId, int p_recorder_id= 0);
		Camera* getCamera(int p_CamId);
	 int GetMenuConfig(int msg);
	 int SetMenuConfig(int msg, int val);
	 void SetAutoScreensaver(int val);
	 void SetTimedShutdown(int val);
     int RemoteSwitchRecord(){return 0;};
     int RemoteTakePhoto(){return 0;};
     int GetRemotePhotoInitState(bool & init_state){return 0;};
     int RemoteSwitchSlowRecord(){return 0;};
     void SetWhiteBalance(int p_CamId, int val);
     void SetExposureValue(int p_CamId, int val);
     void SetLedSwitch(int val);
     void SetLightFreq(int p_CamId, int val);
     void SetTimeWaterMark(int p_CamId, int val);
     void ResetDevice();
     void SetDeviceDateTime();
     void SwitchDistortionCalibration(int val);
     void SwitchDevicesEIS(int p_CamId, int val);
     void SetEncodeSize(int p_CamId, int val);
     void SettingHandlerWindowUpdateLabel();
     void NotifyAll();
     int SetIspModuleOnOff(int p_CamId, int val);
     int SetSlowCameraResloution(int p_CamId, int val);
     void UpdateAllSetting(bool update_video_setting);
     void SetAdasSwitch(int val);
	 void SetAccSwitch(int val);
     void SetAdasLaneShiftReminding(int val);
     void SetAdasForwardCollisionWaring(int val);
     void SetWatchDogSwitch(int val);
     void SetWatchProbePrompt(int val);
     void SetWatchSpeedPrompt(int val);
     void SetEmerRecordSwitch(int val);
     void SetEmerRecordSensitivity(int p_CamId, int val);
     void SetParkingWarnLampStatus(int val);
     void SetParkingAbnormalMonitoryStatus(int val);
     void SetParkingAbnormalNoticeStatus(int val);
     void SetParkingLoopRecordStatus(int val);
     void SetParkingLoopResolution(int val);
     void SetRearVideoResoulation(int p_CamId, int val);
     int SetScreenBrightnessLevel(int val);
     void SetScreenDisturbMode(int val);
     void SetVoiceTakePhotoSwitch(int val);
     void SetVolumeSelection(int val);
     void SetPowerOnVoiceSwitch(int val);
     void SetKeyPressVoiceSwitch(int val);
     void SetDriveringReportSwitch(int val);
     void Set4GNetWorkSwitch(int val);
     void SetStandbyClockSwitch(int val);
     void showButtonDialog();
     void VideoRecordDetect(bool start_);
     void HideInfoDialog();
     int stopSystemRecords(void);
     int exeOtaUpdate(int update_type);
     void SetParkingImpactSensitivity(int p_CamId, int val);
     void SetVideoRecordTime(int val);
	 void SetVideoDelayTime(int val);
     void SetAutoStatusBarSaver(bool enable_flag);     
	 void SetPicQuality(int val);
	 void SetContinuousPictureMode(int val);
	 void SetCurrentPicMode(int val);
     int EnableMotionDetect(int p_nCamid,bool p_bEnable);
	private:
		int StartCamBRecord(int p_nEnable);
    private:
        int status_;
        pthread_mutex_t   model_lock_;
        WindowManager    *win_mg_;
        NetManager       *net_manager_;
        MainModule       *mainModule_;
        CamRecMap        m_CamRecMap;
        CameraMap      m_CamMap;
        bool softap_on_;
        bool isStreamRecodFlag;
}; /* class DeviceSettingPresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
