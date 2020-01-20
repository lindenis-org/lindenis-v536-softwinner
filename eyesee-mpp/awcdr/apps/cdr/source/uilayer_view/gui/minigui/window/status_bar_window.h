/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.h
 * @brief 状态栏窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-01
 */
#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include <time.h>
#include <signal.h>
#include "widgets/text_view.h"
#include "window/preview_window.h"
#include "window/window_manager.h"
//#include "device_model/system/event_manager.h"
#include "window/status_bar_middle_window.h"

#if 0
typedef struct
{
	struct
	{
		unsigned int year;
		unsigned int mon;
		unsigned int day;
		unsigned int hour;
		unsigned int min;
		unsigned int sec;	
		unsigned int msec;	
	} utc;
    struct tm gtm;
	char status;
	double latitude;
	char NS;
	double longitude;
	char EW;
	float speed;
	int GpsSignal;
	float altitude;
	
}GpsInfo_t;
#endif


class PreviewWindow;
class StatusBarMiddleWindow;

//#define PHOTOTIMEUSE
//#define DATETIMEUSE

class StatusBarWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(StatusBarWindow, Runtime)
    public:
        StatusBarWindow(IComponent *parent);
        virtual ~StatusBarWindow();
        std::string GetResourceName();
        void GetCreateParams(CommonCreateParams& params);
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
		#ifdef DATETIMEUSE
        static void DateUpdateProc(union sigval sigval);
		#endif
        void GetIndexStringArray(std::string array_name,  std::string &result, int index);
        void GetString(std::string array_name, std::string &result);
        int GetModeConfigIndex(int msg);
        //void ModeIconHandler(int winno);
        void initRecordTimeUi();
        void RecordStatusTimeUi(bool mstart);
		#ifdef PHOTOTIMEUSE
		void PhotoStatusTimeUi(int time,int mode);
		void HidePhotoStatusTimeUi();
		#endif
        int GetStringArrayIndex(int msg);
        static void GetTFCapacity(union sigval sigval);
		void UpdateBatteryStatus(bool show);
        void UpdateBatteryStatus(bool sb,int levelval);
		void UpdatePhotoMode(int mode,int index);
        void UpdateGpsStatus(bool sb,int levelval);
        void SetWinStatus(int status);
        void OnLanguageChanged();
        static void* BatteryDetectThread(void *context);
        static void* GpsSignalDetectThread(void *context);
        void VoiceIconHandler(bool sv);
        void ReturnStatusBarBottomWindowHwnd();
        HWND GetSBBWHwnd();
        void TimeStartStopCtrl(bool flag);
        void WifiIconHander(bool sw);
		void LockIconHander(bool sw);
        void AdasIconHander(bool sw);
        void ParkIconHander(bool sw);
		void AwmdIconHander(bool sw);
		
        void SdCardIconHander(bool sw);
		void ResolutionIconHander(bool sw);
		void LooprecIconHander(bool sw);
        
        void AppConnectIconHander(bool value);
		
		//void PreviewIconHander(bool sw);	
        void UpdatePlaybackFileInfo(int index,int count,bool flag = true);
        void UpdatePlaybackFileTime(std::string &fileTime,bool m_show);
        static void RecHintTimerProc(union sigval sigval);
        void SetRecordStatusFlag(bool flag){record_status_flag_ = flag;}
        void HidePlaybackBarIcon();
        static void RecordingTimerProc(union sigval sigval);
		static void PhotoingTimerProc(union sigval sigval);
		static void ChangemodetimerProc(union sigval sigval);
       void ResetRecordTime();
        int GetCurrentRecordTime();
		void hideStatusBarWindow();
		void showStatusBarWindow();
		void WinstatusIconHander(int state);
		NotifyEvent OnClick;
		void StatusBarWindowProc(View *control);
		void UpdateGpsStatusEx(bool sg,int index, int flash=0);
		int GetTFCapacity(uint32_t *free, uint32_t *total);
		// for phototimer
		void Display_photocountdown(int onoff);
		void Set_photocountdown_caption(char *text);

		void SetStatusPreview();
		void SetStatusPhoto();
		void SetStatusPlayback();
		void SetStatusTimelaps();
		void SetStatusSetting();

		void RecordTimeUiOnoff(int val);
		#ifdef DATETIMEUSE
		void DateTimeUiOnoff(int val);
		#endif
		//void UpdateGpsInfo(void* data);
		void SetStatusRecordMode(int val);
		bool m_changemode_enable;
		void SetStatusRecordStatus(int val) {RecordStatus= val;}
    private:
        timer_t timer_id_;
		#ifdef DATETIMEUSE
        timer_t timer_id_data;
		#endif
        timer_t rechint_timer_id_;
        timer_t recording_timer_id_;	
		#ifdef PHOTOTIMEUSE
		timer_t photoing_timer_id_;
		int PhotoTime;
		int phoeoMdoe;
		int current_photo_time;
		TextView 	*m_photocountdown_lb;
		GraphicView *m_photocountdown_bg;
		#endif
        int win_status_;
		int RecordStatus;
        pthread_t m_battery_detect_thread_id;
        pthread_t m_gps_signal_thread_id;
        int m_current_battery_level;
        int m_current_gps_signal_level;
        int m_current_4g_signal_level;
        HWND hwndel_; 
        bool m_time_update_4g;
        bool record_status_flag_;
        TextView* s_fileinfo_total;
        TextView* m_file_create_time;
        TextView* m_rec_file_time;
        TextView* time_label;
		//TextView* photo_time_label;
		GraphicView* win_status_icon;
        int m_RecordTime;
		int m_RecordTimeSave;
		timer_t changemode_timer_id_;
        unsigned long m_bg_color;
		
		StatusBarMiddleWindow* m_sbmiddle;

		//TextView 	*m_speed_lb;
		//GpsInfo_t 	GpsInfo;
		//int GpsSwitch;
		//int GpsSpeedUnit;
		int RecordMode;
        bool m_appConnected;
};
