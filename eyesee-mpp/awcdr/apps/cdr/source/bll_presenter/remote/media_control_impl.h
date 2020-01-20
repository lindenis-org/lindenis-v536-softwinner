/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file media_control_impl.h
 * @brief 多媒体相关控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */
#pragma once

#include "interface/dev_ctrl_adapter.h"
#include "bll_presenter/common_type.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/media/recorder/recorder_factory.h"
#include "device_model/menu_config_lua.h"
#include "onvif/onvif_param.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"
#include "window/window_manager.h"



typedef struct {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
}date;



#include <map>

namespace EyeseeLinux {


/**
 * @brief 设备多媒体相关的控制接口
 */
class MediaControlImpl
    : public DeviceAdapter::MediaControl
{
    public:
        MediaControlImpl(IPresenter *presenter);

        ~MediaControlImpl();

        void SetMediaCtrlInstance(const CamRecMap &cam_rec_map, const RecorderGroup &rec_grp ,const CameraGroup &cam_map);

        void SetStreamSenderInstance(const StreamSenderMap &stream_sender_map);

        int GetVideoChannelCnt(int &count);
        #if 0

        int GetOnvifSimpleProfile(int ch, OnvifSimpleProfile &profile);

        int GetOnvifVideoEncodingConfigOptions(int ch, OnvifVideoEncodingConfigOptions &options);
        #endif
        int SetVideoQuality(ControllerType ctrl_type, int ch, int width, int height, int bitrate, int framerate);

        int GetStreamUrl(int ch, char *url, int size);

        int GetMediaConfig(AWEnCodeConfig &media_cfg);

        int SaveMediaConfig(void);

        int DefaultMediaConfig(void);

        int SnapShot(ControllerType ctrl_type, int ch);

        int SetVencSize(ControllerType ctrl_type, int ch, int width, int height);

        int SetVencQuality(ControllerType ctrl_type, int ch, int quality);

        int SetVencFrameRate(ControllerType ctrl_type, int ch, int framerate);

        int SetVencBitRate(ControllerType ctrl_type, int ch, int bitrate);

        int SetVencGop(ControllerType ctrl_type, int ch, int gop);

        int SetVencType(ControllerType ctrl_type, int ch, int type);

        int SetVencFormat(ControllerType ctrl_type, int ch, int format);

        int SetVideoResoulation(ControllerType ctrl_type, int ch, int index);

        int SetRecordTime(ControllerType ctrl_type, int ch, int index);

        int SetRecordDelayTime(ControllerType ctrl_type, int ch, int index);

        int SetRecordAudioOnOff(ControllerType ctrl_type, int ch, int index);
        int SetRecordEisOnOff(ControllerType ctrl_type, int ch, int index);
        int SetSlowRecord(ControllerType ctrl_type, int ch, int index);
        int SetPicResolution(ControllerType ctrl_type, int ch, int index);
        int SetTimeTakePic(ControllerType ctrl_type, int ch, int index);
        int SetAutoTimeTakePic(ControllerType ctrl_type,int ch,int index);
        int SetContinuousPictureMode(ControllerType ctrl_type,int ch,int index);
        int SetExposureValue(ControllerType ctrl_type,int ch,int index);
        int SetWhiteBalance(ControllerType ctrl_type,int ch,int index);
        int SetLightFreq(ControllerType ctrl_type,int ch,int index);
        int SetPreviewFlip(ControllerType ctrl_type,int ch,int index);
        int SetTimeWaterMark(ControllerType ctrl_type,int ch,int index);
        int FormatStorage(ControllerType ctrl_type,int ch,int index);
        int SetCamerAutoScreenSaver(ControllerType ctrl_type,int ch,int index);
        int SetCameraTimeShutDown(ControllerType ctrl_type,int ch,int index);
        int SetDeviceLanguage(ControllerType ctrl_type,int ch,int index);
        int SetDeviceDateTime(ControllerType ctrl_type,int ch,int index);
        int SetDeviceReset(ControllerType ctrl_type,int ch,int index);
        int SetDeviceLedSwitch(ControllerType ctrl_type,int ch,int index);
        int SetDeviceDistortioncalibrationSwitch(ControllerType ctrl_type,int ch,int index);
        int GetDeviceConfig(ControllerType ctrl_type,int ch,int index,SMsgAVIoctrSDVDevConfigResp &resp);
        int GetDeviceMenuConfig(SMsgAVIoctrSDVDevConfigResp &resp);
        int RemoteClientDisconnect(void);
        int RemotePreviewChange(ControllerType ctrl_type,int ch,int index) ;
        int RemoteSaveMenuconfig(ControllerType ctrl_type,int ch,int index);

        int RemoteSendCmdToSettingPresenter(int msg,int index);
        int RemoteSendCmdToPreviewPresenter(int msg,int index);
        
        int SetParkingMonitor(ControllerType ctrl_type,int ch,int index);
        int SetParkingMonitorLevel(ControllerType ctrl_type,int ch,int index);
        int SetSystemAudioLevel(ControllerType ctrl_type,int ch,int index);
        int SetMotionDetect(ControllerType ctrl_type,int ch,int index);
        int SetRecordType(ControllerType ctrl_type,int ch,int index);
        int SetDeviceDate(ControllerType ctrl_type, int ch, std::string str);
        int SetDeviceTime(ControllerType ctrl_type, int ch, std::string str);        
        int SetWifiSsid(ControllerType ctrl_type, int ch, std::string str);
        int SetWifiPassword(ControllerType ctrl_type, int ch, std::string str);
        int CheckStatus();

        /**
         * @brief camera相关控制接口
         */
        class CameraControl {

        };

    private:
        CamRecMap cam_rec_map_;
        RecorderGroup rec_group_;
        CameraGroup cam_map_group_;
        StreamSenderMap stream_sender_map_;
        AWEnCodeConfig media_cfg_;
        LuaConfig *lua_cfg_;
        WindowManager *wm;
        std::map<WindowID, IGUIPresenter*> win_presenter_map_;

         /** 绑定控制器类型与RecorderID */
        std::map<ControllerType, std::vector<RecorderID> > ctrl_rec_map_;

        int LoadMediaConfig(void);
};

} // namespace EyeseeLinux
