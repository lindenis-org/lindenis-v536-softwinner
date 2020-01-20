/* *******************************************************************************
 * Copyright (C), 2017-2027, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file prompt.h
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
    //style 1: only has tips and text
    PROMPT_TF_FULL = 0,
    PROMPT_TF_CAP_NO_SUPPORT,
    PROMPT_TF_LOW_SPEED,
    PROMPT_BAT_FULL,
    PROMPT_BAT_LOW,
    PROMPT_DATABASE_FULL,
    PROMPT_CAMEAR_ERROR,
    PROMPT_PREPARE_UPDATE_VERSION,
    PROMPT_UNSUPPORTED_FUNC,
    PROMPT_NOTIC_UPDATE,
    //style 2: full show has tips /icon/text
    PROMPT_FULL_SDCARD_FORMAT,
    PROMPT_FULL_WIFI_CONNET,
    //style 3: has tips/icon/text (has countdown time)
	PROMPT_STANDBY_MODE,
    PROMPT_PWOER_DISCONNECTED,
    //style 4: only has icon and text (icon on the left)
    PROMPT_TF_NULL,
    PROMPT_VERSION_NO_EXIST,
    PROMPT_VERSION_INVALID,
    PROMPT_EMERGENCY_RECORDING,
    //style 5:only has icon and text (icon on the top)
    PROMPT_TF_FORMATTING,
    PROMPT_TF_FORMAT_FINISH,
    PROMPT_TF_FORMAT_FAILED,
    PROMPT_ACCOUNT_UNBIND,
    PROMPT_UPDATE_NET_TIME_FINISH,
    PROMPT_UPDATE_NET_TIME_FAILED,
    PROMPT_TF_NO_EXIST,
    PROMPT_OTA_FAILE,
    PROMPT_RESOLUTION_SW_ING,
    PROMPT_RESOLUTION_SW_FINISH,
    PROMPT_RESET_FACTORY_ING,
    PROMPT_RESET_FACTORY_FINISH,

	PROMPT_ADAS_LEFT_LDW,
	PROMPT_ADAS_RIGHT_LDW,
	PROMPT_ADAS_FCW,
	PROMPT_ADAS_DANGER_FATIGUE,

    PROMPT_INVALID,
};

/*** title label and info label***/
typedef struct {
    const char *TitleCmd;
    const char *image_icon;
    const char *InfoCmd;
}labelStringCmd_t;

static labelStringCmd_t  labelStringCmd[] = {
    //style 1
    {"ml_prompt",NULL,    "ml_prompt_tf_full"},
    {"ml_prompt",NULL,    "ml_prompt_tf_cap_no_support"},
    {"ml_prompt",NULL,    "ml_prompt_tf_low_speed"},
    {"ml_prompt",NULL,    "ml_prompt_bat_full"},
    {"ml_prompt",NULL,    "ml_prompt_bat_low"},
    {"ml_prompt",NULL,    "ml_prompt_database_full"},
    {"ml_prompt",NULL,    "ml_prompt_camera_error"},
    {"ml_prompt",NULL,    "ml_prompt_prepare_update"},
    {"ml_prompt",NULL,    "ml_prompt_unsupported_func"},
    {"ml_prompt",NULL,    "ml_prompt_update_count_down"},
//here is the icon style
    //style 2
    {"ml_device_sdcard_format","prompt_sdcard","ml_prompt_sdcard_format"},
    {"ml_wifi_connect","prompt_phone_connet","ml_prompt_wifi_connect_ok"},
    //style 3
    {"","power","ml_prompt_standbymode"},
    {"","power","ml_prompt_power_disconnected"},
    //styl 4
    {"","notice", "ml_prompt_tf_null"},
    {"","notice", "ml_prompt_version_no_exist"},
    {"","notice", "ml_prompt_version_invalid"},
    {"","notice","ml_prompt_emergency_recording"},
    //style 5
    {"","tips_tf","ml_prompt_tf_formating"},
    {"","tips_finish","ml_prompt_tf_format_finish"},
    {"","tips_fail","ml_prompt_tf_format_failed"},
    {"", "tips_finish", "ml_prompt_account_unbind_finish"},
    {"","tips_finish","ml_prompt_update_net_time"},
    {"","tips_fail","ml_prompt_update_net_time_fail"},
    {"","tips_tf","ml_playback_no_sdcard"},
    {"","tips_fail","ml_sdvcam_ota_fail"},
    {"","tips_finish","ml_prompt_resolution_ing"},
    {"","tips_finish","ml_prompt_resolution_finish"},
    {"","tips_reset","ml_prompt_reset_factory_ing"},
    {"","tips_finish","ml_prompt_reset_factory"},

    {"", "adas_left_ldw","ml_adas_left_ldw"},
	{"", "adas_right_ldw","ml_adas_right_ldw"},
    {"", "adas_pcw","ml_adas_pcw"},
    {"", "adas_danger_driver_fatigue","ml_adas_danger_driver_fatigue"},
};

class Prompt
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(Prompt, Runtime)

    public:
        NotifyEvent OnClick;

        Prompt(IComponent *parent);
        
        Prompt(int type, IComponent *parent);

        virtual ~Prompt();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
	     
	 void keyProc(int keyCode,int isLongPress);
	      
	 void ShowPromptInfo(unsigned int prompt_id, unsigned int showtimes = 2);

	  void HidePromptInfo();

	  inline bool GetStandbyFlagStatus() {return m_prompt_standby_flag_;}

	  inline void SetStandbyFlagStatus(bool standby_flag) {m_prompt_standby_flag_ = standby_flag;}

	  static void HandlePromptTime(union sigval sigval);
	 bool GetPromptShowFlag(){return m_bIsShowing;}
	  void SetPromptShowFlag(bool flag){m_bIsShowing = flag;}
	  int getPromptId(){return prompt_id_;}
	  void setPromptId(int val){prompt_id_ = val;}
    private:
	    timer_t timer_id_;
     	unsigned int showtimes_,timercount;
        bool m_bIsShowing;
        TextView *m_Title;
        TextView *m_Text,*m_Time;
        bool m_prompt_standby_flag_;
        int prompt_id_;
};
