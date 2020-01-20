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

#define PHOTOTIMEUSEMIDDLE
class StatusBarMiddleWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(StatusBarMiddleWindow, Runtime)
    public:
        StatusBarMiddleWindow(IComponent *parent);
        virtual ~StatusBarMiddleWindow();
        std::string GetResourceName();
        void GetCreateParams(CommonCreateParams& params);
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        static void DateUpdateProc(union sigval sigval);
        void GetIndexStringArray(std::string array_name,  std::string &result, int index);
        void GetString(std::string array_name, std::string &result);
        int GetModeConfigIndex(int msg);

		int GetStringArrayIndex(int msg);
		#ifdef PHOTOTIMEUSEMIDDLE
		void PhotoStatusTimeUi(int time,int mode);
		void HidePhotoStatusTimeUi();
		#endif
        void SetWinStatus(int status);
        void OnLanguageChanged();
        void ReturnStatusBarBottomWindowHwnd();
        void TimeStartStopCtrl(bool flag);
		
		static void PhotoingTimerProc(union sigval sigval);

		// for phototimer
		void Display_photocountdown(int onoff);
		void Set_photocountdown_caption(char *text);

		void RecordTimeUiOnoff(int val);
		void DateTimeUiOnoff(int val);

		void hideStatusBarWindow();
		void showStatusBarWindow();
		//void UpdateGpsInfo(void* data);
		HWND GetSBBWHwnd();
        void SetPhotoTimeShot();
    private:
    	timer_t timer_id_data;
		#ifdef PHOTOTIMEUSEMIDDLE
		timer_t photoing_timer_id_;
		#endif
        int win_status_;
        TextView* time_label;
		int current_photo_time;
		int PhotoTime;
		int phoeoMdoe;
		TextView 	*m_photocountdown_lb;
		GraphicView *m_photocountdown_bg;

		TextView 	*m_speed_lb;

		HWND hwndel_; 
		unsigned long m_bg_color;
};
