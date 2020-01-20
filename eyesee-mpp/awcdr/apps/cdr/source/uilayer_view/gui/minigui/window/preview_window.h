				/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview_window.h
 * @brief 单路预览录像窗口
 * @author id:826
 * @version v0.3
 * @date 2016-11-03
 */
#pragma once

#include "window/window.h"
#include "window/user_msg.h"
#include <signal.h>
#include <pthread.h>
#include "device_model/system/led.h"
#include "window/status_bar_window.h"
#include "window/status_bar_middle_window.h"

#include "window/window_manager.h"
#include "window/promptBox.h"

/**
 * 定义每个窗体内部的button/msg
 */
#define PREVIEW_RECORD_BUTTON       (USER_MSG_BASE+1)
#define PREVIEW_SHOTCUT_BUTTON      (USER_MSG_BASE+2)
#define PREVIEW_GO_PLAYBACK_BUTTON  (USER_MSG_BASE+3)
#define PREVIEW_AUDIO_BUTTON        (USER_MSG_BASE+4)
#define PREVIEW_SWITCH_CAM_BUTTON   (USER_MSG_BASE+5)
#define PREVIEW_CONFIRM_FORMAT      (USER_MSG_BASE+6)
#define PREVIEW_CANCEL_FORMAT       (USER_MSG_BASE+7)
#define PREVIEW_SHUTDOWN_BUTTON     (USER_MSG_BASE+8)
#define PREVIEW_WIFI_SWITCH_BUTTON  (USER_MSG_BASE+9)
#define PREVIEW_SET_DIGHTZOOM_BUTTON (USER_MSG_BASE+10)
#define PREVIEW_SET_RECORD_MUTE     (USER_MSG_BASE+11)
#define PREVIEW_BUTTON_DIALOG_HIDE  (USER_MSG_BASE+12)
#define PREVIEW_WIFI_DIALOG_HIDE    (USER_MSG_BASE+13)
#define PREVIEW_USB_DIALOG_HIDE     (USER_MSG_BASE+14)
#define PREVIEW_LOWPOWER_SHUTDOWN   (USER_MSG_BASE+15)
#define PREVIEW_SWITCH_LAYER		(USER_MSG_BASE+16)
#define PREVIEW_TO_SETTING_BUTTON		(USER_MSG_BASE+17)
#define PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION		(USER_MSG_BASE+18)
#define PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION		(USER_MSG_BASE+19)
#define PREVIEW_CAMB_PREVIEW_CONTROL (USER_MSG_BASE+20)
#define PREVIEW_TAKE_PIC_CONTROL (USER_MSG_BASE+21)
#define PREVIEW_EMAGRE_RECORD_CONTROL (USER_MSG_BASE+22)
#define PREVIEW_TO_SETTING_NEW_WINDOW (USER_MSG_BASE+23)
#define PREVIEW_TO_SHUTDOWN (USER_MSG_BASE+24)
#define PREVIEW_GO_PHOTO_BUTTON  	(USER_MSG_BASE+25)
#define PREVIEW_GO_RECORD_BUTTON  	(USER_MSG_BASE+26)
#define PREVIEW_CHANGEWINDOWSTATUS  	(USER_MSG_BASE+27)
#define PREVIEW_ONCAMERA  	(USER_MSG_BASE+28)
#define PREVIEW_ONCAMERA_FROM_PLAYBACK  	(USER_MSG_BASE+28)
#define PREVIEW_ONCAMERA_USBMODE  	(USER_MSG_BASE+29)
#define PREVIEW_RESTARTMOD  	(USER_MSG_BASE+30)
#define PREVIEW_RESET_CAMERA_ON_ERROR  	(USER_MSG_BASE+31)




typedef enum {
    STATUS_DOWNLOAD_INIT,
    STATUS_DOWNLOAD_SUCC,
    STATUS_DOWNLOAD_FAIL,
    STATUS_DOWNLOAD_NETFAIL,
    STATUS_DOWNLOAD_FSUNMOUNT,
    STATUS_DOWNLOAD_FSFAIL
};

typedef enum {
    TAKE_PHOTO_NORMAL = 0,
    TAKE_PHOTO_THUMB,
    TAKE_PHOTO_SNAPSHOT
};

typedef enum {
    RECORDSTATS_IDLE = 0,
    RECORDSTATS_PREPARE,
    RECORDSTATS_RECORD,
    RECORDSTATS_FILEDONE,
    RECORDSTATS_STOP,
};

typedef enum {
    //WINDOWID_LAUNCHER = 1,
    WIN_DEFAULT = 0,
    WIN_WIFI,
    WIN_Dialog,
    WIN_USBMode,
    WIN_Format,
} CurWinStatus;
enum {
	PREVIEW_BUTTON_DIALOG_FORMAT_SDCARD= 0,
	PREVIEW_BUTTON_DIALOG_DD_NOTICE,
	PREVIEW_BUTTON_DIALOG_SDCARD_UPDATE_VERSION,
};
class Dialog;

class GraphicView;

class TextView;

class SwitchButton;

//class ProgressBar;

class ShutDownWindow;

class Prompt;

class PromptBox;

class BulletCollection;

class StatusBarWindow;

class USBModeWindow;
class PreviewWindow
        : public SystemWindow {
    DECLARE_DYNCRT_CLASS(PreviewWindow, Runtime)

    public:
        PreviewWindow(IComponent *parent);

        virtual ~PreviewWindow();

        std::string GetResourceName();

        void SwitchButtonProc(View *control);

        void GraphicViewButtonProc(View *control);
         void GetCreateParams(CommonCreateParams &params);

        void ChangeZoomTimes(int flag);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void keyProc(int keyCode, int isLongPress);
		void ShutDown();

        void ShowPromptInfo(unsigned int prompt_id,unsigned int showtimes=2,bool m_force = false);

        int HandlerPromptInfo(void);
        int GetRecordStatus();
        int GetWindowStatus();

		void showShutDownLogo();
        void SetRecordMute(bool value);
        bool IsUsbAttach();
#ifdef SHOW_DEBUG_INFO
        void ShowDebugInfo(bool value);

        void ClearDebugInfo();

        void InsertDebugInfo(const std::string &key, const std::string &value);

        void RemoveDebugInfo(const std::string &key);

        void UpdateDebugInfo();

        void UpdateDebugInfo(const std::string &info);
#endif
        void OnLanguageChanged();

        void HidePromptInfo();

	void ShowPromptBox(unsigned int promptbox_id,unsigned int showtimes=2);
	void PauseRecordCtrl(bool flag);
	void RecordStatusTimeUi(bool mstart);
    void ResetRecordTime();
	void SetPreviewButtonDialogIndex(int flag){m_preview_button_dialog_index = flag;}
	int  GetPreviewButtonDialogIndex(){return m_preview_button_dialog_index;}
	void VideoRecordDetect(bool start_);
	void initTopBottomBg(bool mdown,bool msbottom);
	int DetectSdcardNewVersion();
	void HandleButtonDialogMsg(int val);
    void SetStartDownload(bool flag);
    bool IsDownloadStatusReady(void);
    void SetDownloadStatus(int status);
    int GetDownloadStatus(void);
    int CheckVersionFromNet();
	int DownLoadVersionFromNet();
	static void *DownLoadVersionFromNetThread(void *arg);
	void DoCreateDLThread();
	//4g
	int DownLoad4GVersionFromNet();
	static void *DownLoad4GVersionFromNetThread(void *arg);
	void DoCreate4GDLThread();

	bool IsHighCalssCard();
	bool IsFileSystemError();
	static void DLTimerProc(union sigval sigval);	

	bool get4GVersionDLFlag(){return m_4g_dl_finish;}
	void set4GVersionDLFlag(bool flag){m_4g_dl_finish = flag;}
	bool getSystemVersionDLFlag(){return m_dl_finish;}
	void setSystemVersionDLFlag(bool flag){m_dl_finish = flag;}
	inline Prompt *GetPromptPoint(){return prompt_;}
	bool Md5CheckVersionPacket(std::string p_path,std::string md5Code);
	int ShowCamBRecordIcon();
	int HideCamBRecordIcon();
    int GetCurrentRecordTime();
    void showLockFileUiInfo(int value);
    bool getIsRecordStartFlag();
    void VideoStopRecordCtl();
	void UpdatePhotoMode(int mode,int index);
	void TakePicControl(int flag);
	void SetWindowStatus(int status);
	void ChangeWindowStatus(int newstatus);
	void mySetRecordval();
	void USBModeWindowLangChange();
	void SetPhotoTimeShot();

	int Get_win_statu_save() { return win_statu_save;}
	int Get_win_statu() { return win_statu;}
	void Set_win_statu_save(int val) { win_statu_save = val;}
	void Set_win_statu(int val) { win_statu = val;}
	int GetisRecordStart() {return isRecordStart;}

	bool isRecordStartFake;
    bool isMotionDetectHappen;

	/*
		-1  record start error
		0 = idle
		1 = prepare record
		2 = record start
		3 = record file done
		4 = record stop
		*/
	int RecordState;
	void SetRecordState(int val) {RecordState = val;}
	int GetRecordState() {return RecordState;}

	void ResetPhotoMode();

	void OnOffStatusBar();

	void OnOffCamera(int camid, int status);
	int RecordMode;
  private:
        /****************************************************
                * Name:
                *     ResumeWindow(int p_nWinId, CurWinStatus p_Status)
                * Function:
                *     resume window
                * Parameter:
                *     input:
                *         p_nWinId: resume window id;
                *         p_Status:  cur window status
                *     output:
                *         none
                * Return:
                *     0:    success
               *****************************************************/
        int ResumeWindow(int p_nWinId, CurWinStatus p_Status);
		  /****************************************************
                * Name:
                *     IsNewVersion(std::string external_version,std::string local_version)
                * Function:
                *     IsNewVersion
                * Parameter:
                *     input:
                *         p_nWinId: resume window id;
                *         p_Status:  cur window status
                *     output:
                *         pointer
                * Return:
                *     true:    success
                *     false:    fail
               *****************************************************/
	bool IsNewVersion(std::string external_version,std::string local_version);
	int current_count;
	
    private:
		SwitchButton *rec_switch_;
		SwitchButton *audio_switch_;
		SwitchButton *cam_switch_;
		int win_statu;
		int win_statu_save;
		int zoom_val_;
		// timer_t rechint_timer_id_;
		pthread_mutex_t proc_lock_;
		
		bool isRecordStart;
		
		bool m_isRecord;//if is recording status when change the settingwindow menu item
		bool isTakepicFinish;
		bool isWifiStarting;
		ShutDownWindow *m_shutwindowObj;
		Prompt *prompt_;
		PromptBox * PromptBox_;
		bool m_bAutoTakePic;
		bool m_bTimeTakePic;
		int  m_nCurrentWin;
		bool m_bUsbDialogShow;
		bool m_bHdmiConnect;
		timer_t recording_timer_id_;
		bool m_standby_flag;
		int m_preview_button_dialog_index;
		bool m_stop2down;
		std::map<std::string, std::string> debug_info_;
		BulletCollection * m_BulletCollection_;
		pthread_t version_dl_thread_id;
		bool m_dl_finish;//记录是否正常下载了数据包并且校验md5sum ok
		WindowManager *win_mg;
		timer_t dl_timer_id;
 		pthread_t version_4g_dl_thread_id;
		bool m_4g_dl_finish;//记录是否正常下载了数据包并且校验md5sum ok
        bool is_start_download;
        int m_download_status;
		TextView *m_record_info, *m_record_info1;
        USBModeWindow * usb_win_;
		int photo_mode;
		int photo_time;
		bool photoing_flag;		

		timer_t acc_timer_id;
		int AccUSBtype;
        
};
