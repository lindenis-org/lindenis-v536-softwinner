/* *******************************************************************************
 * Copyright (C), 2017-2027, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file promptBox.h
 * @brief 提示窗口
 * @author id:fangjj
 * @version v10.
 * @date 2017-04-24
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"

#include <time.h>
#include <signal.h>

class TextView;

/** prompt id **/
enum{
    PROMPT_BOX_RECORD_SOUND_OPEN =0,
    PROMPT_BOX_RECORD_SOUND_CLOSE,
    PROMPT_BOX_RECORDING_START,
    PROMPT_BOX_RECORDING_STOP,
    PROMPT_BOX_MOBILE_CONNECTED,
    PROMPT_BOX_MOBILE_DISCONNECTED,
    PROMPT_BOX_GPS_CONNECTED,
    PROMPT_BOX_GPS_DISCONNECTED,
    PROMPT_BOX_DEVICE_RECORDING,
    PROMPT_BOX_VIDEO_PLAYERR,
    PROMPT_BOX_TF_INSERT,
    PROMPT_BOX_TF_OUT,
    PROMPT_BOX_LAST_VIDEO,
    PROMPT_BOX_FIRST_VIDEO,
    PROMPT_BOX_LOCK_FILE,
    PROMPT_BOX_UNLOCK_FILE,
    PROMPT_BOX_FILE_LOCKED,
    PROMPT_BOX_LOCK_RECORD_TIP_FILE,
    PROMPT_BOX_PARKING_VIDEO,
    PROMPT_BOX_DEVICE_PHOTOING,
    PROMPT_BOX_MOTIONISON,
    PROMPT_BOX_R_EVENT_DIR_FULL,
    PROMPT_BOX_F_EVENT_DIR_FULL,
    PROMPT_BOX_PARK_DIR_FULL,
    PROMPT_BOX_INVALID,

};	

/*** title label and info label***/
typedef struct {
    const char *image_icon;
    const char *mLabel;
}promptboxinfo_t;

static promptboxinfo_t  promptboxinfo[] = {
    {"voice_on",   "ml_promptBox_record_voice_on"},
    {"voice_off",   "ml_promptBox_record_voice_off"},
    {"recording",   "ml_promptBox_recording_on"},
    {"recording", "ml_promptBox_recording_off"},
    {"mobile", "ml_promptBox_mobile_connected"},
    {"mobile", "ml_promptBox_mobile_disconnected"},
    {"gps_on", "ml_promptBox_gps_connected"},
    {"gps_off", "ml_promptBox_gps_disconnected"},
    {NULL, "ml_promptBox_device_recording"},
    {NULL, "ml_promptBox_video_playerr"},
    {NULL, "ml_promptBox_tf_insert"},
    {NULL, "ml_promptBox_tf_out"},
    {NULL, "ml_promptBox_last_video"},
    {NULL, "ml_promptBox_first_video"},
    {NULL, "ml_lock_current_file"},        
    {NULL, "ml_unlock_current_file"},
    {NULL, "ml_current_file_locked"},
    {NULL, "ml_promptBox_current_recording_off"},
    {NULL, "ml_promptBox_park_video"},
    {NULL, "ml_promptBox_device_photoing"},
    {NULL, "ml_motion_is_on"},
    {NULL, "ml_r_event_dir_full"},
    {NULL, "ml_f_event_dir_full"},
    {NULL, "ml_park_dir_full"},
};

class PromptBox
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(PromptBox, Runtime)

    public:
        NotifyEvent OnClick;

        PromptBox(IComponent *parent);

        virtual ~PromptBox();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
	     
	 void keyProc(int keyCode,int isLongPress);
	      
	 void ShowPromptBox(unsigned int promptbox_id, unsigned int showtimes = 2);

	  void HidePromptBox();
	  static void HandlePromptBoxTime(union sigval sigval);
	  int getPromptBoxLen(){return m_len;}
	  void setPromptBoxLen(int len){m_len = len;}
	  bool GetPromptBoxShowFlag(){return m_bIsShowing;}
	  void SetPromptBoxShowFlag(bool flag){m_bIsShowing = flag;}
    private:
     	unsigned int showtimes_;
		timer_t timer_id_;
        bool m_bIsShowing;
	TextView *m_Text;
	int first_icon_width,third_icon_width;
	int m_len;//promptbox all len
};
