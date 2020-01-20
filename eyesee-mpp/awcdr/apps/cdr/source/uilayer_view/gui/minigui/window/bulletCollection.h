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

enum{
	//setting_window
	BC_BUTTON_DIALOG_REBOOT = 0,//
	BC_BUTTON_DIALOG_REFRESH_NETWORK_TIME,//
	BC_BUTTON_DIALOG_MANUAL_UPDATE_TIME,//
	BC_BUTTON_DIALOG_RESETFACTORY,//
	BC_BUTTON_DIALOG_ACCOUNT_UNBIND,
    BC_BUTTON_DIALOG_FORMAT_SDCARD,//
    BC_BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION,//
	BC_BUTTON_DIALOG_4G_NETWORK_OPEN_WATCHDOG,//
	BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN,//
	BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL,//
	BC_BUTTON_DIALOG_ADAS_OPEN_FAILE,//
	BC_BUTTON_DIALOG_WATCHDOG_OPEN_FAILE,//
	BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION,//ing
	BC_BUTTON_DIALOG_DOWNLOAD_PACKET_WATCHDOG,//ing
	BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION,//
	BC_BUTTON_DIALOG_INSTALL_PACKET_WATCHDOG,//
	BC_BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH,//
	BC_BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH,//
	BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL,//
	BC_BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW,//
	BC_BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH,//
	//preview_window
	BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION,
	BC_BUTTON_DIALOG_DD_NOTICE,
	BC_BUTTON_DIALOG_DD_NOTICE_FULL,
	BC_BUTTON_DIALOG_DD_TF_FS_ERROR,
	//playback_window
	BC_BUTTON_DIALOG_DELETE_VIDEO,
	BC_BUTTON_DIALOG_DELETE_PHOTO,
	BC_BUTTON_DIALOG_DELETE_ERROR_VIDEO,
	BC_BUTTON_DIALOG_MORE_IMG,
	BC_BUTTON_DIALOG_CHECK_MD5_FAILE,
	BC_BUTTON_DIALOG_DELETE_ALLSELECTED,
	BC_BUTTON_DIALOG_DELETE_ERROR_PHOTO,
};

typedef struct {
	const char *info_text;
	const char *info_title;
	const char *button_left;//ok /open_now /retry / recharge_now/know 
	const char *button_right;//cancel 
}bc_button_dialog_item;

//Here the arry initalization sequence is consistent with the above enum definition order
static bc_button_dialog_item bc_button_dialog_item_[]={//
	{"ml_confirm_reboot","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_refresh_network_time","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_manual_update_time","","ml_confirm_ok","ml_confirm_no"},
	{"ml_device_resetselection","","ml_confirm_ok","ml_confirm_no"},
    {"ml_account_dialog_unbind", "", "ml_confirm_ok","ml_confirm_no"},
    {"ml_button_dialog_format_sdcard","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_4g_network_open","","ml_button_dialog_open_now","ml_confirm_no"},//version update need open the 4g
	{"ml_button_dialog_4g_network_open","","ml_button_dialog_open_now","ml_confirm_no"},//watchdog update packet need open the 4g
	{"ml_button_dialog_record_loop_open","","ml_button_dialog_open_now","ml_confirm_no"},
	{"ml_button_dialog_record_loop_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_adas_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_watchdog_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_download_packet_version","","ml_button_dialog_retry","ml_confirm_no"},//version packet download
	{"ml_button_dialog_download_packet_watchdog","","ml_button_dialog_retry","ml_confirm_no"},//watchdog packet download
	{"ml_button_dialog_install_packet_version","","ml_confirm_no",""},//install the version packet
	{"ml_button_dialog_install_packet_watchdog","","ml_confirm_no",""},//install watchdog version packet
	{"ml_button_dialog_install_current_city_packet_finished","","ml_button_dialog_know",""},
	{"ml_button_dialog_install_national_city_packet_finished","","ml_button_dialog_know",""},
	{"ml_button_dialog_download_packet_fail_network_abnormal","","ml_button_dialog_retry","ml_confirm_no"},
	{"ml_button_dialog_download_packet_fail_insufficient_flow","","ml_button_dialog_recharge_now","ml_confirm_no"},
	{"ml_button_dialog_install_version_finish","","ml_button_dialog_know",""},
	{"ml_sdcard_update_version_new","","ml_sdcard_update_version_now","ml_sdcard_update_version_no"},
	{"ml_preview_drip_notice","","ml_confirm_ok","ml_confirm_no"},//didi notice two button dialog
	{"ml_preview_drip_notice","ml_preview_drip_notice","ml_button_dialog_know",""},//DIDI notice full screen
	{"ml_prompt_tf_fs_error","","ml_confirm_ok","ml_confirm_no"},
	{"delete_info_video","","ml_confirm_ok","ml_confirm_no"},
	{"delete_info_photo","","ml_confirm_ok","ml_confirm_no"},
	{"delete_info_error_video","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_more_img","","ml_button_dialog_know",""},
	{"ml_button_dialog_check_md5_fail","","ml_button_dialog_know",""},
	{"delete_info_allselected","","ml_confirm_ok","ml_confirm_no"},
	{"delete_info_error_photo","","ml_confirm_ok","ml_confirm_no"},
};


class TextView;
class Dialog;
class Button;
class ButtonOK;
class ButtonCancel;
class GraphicView;

class BulletCollection
{
    public:
        BulletCollection();

        ~BulletCollection();
	void FreedP();
	void initButtonDialog(IComponent *parent);
	void updateProgressbarPercentage(int value);
	void StopProgressbarTime();
	void MsgToSettingWindow(bool m_buttonDialog,int val);
	static void HandleProgressbarTime(union sigval sigval);
	void buttonDialogFilling(const bc_button_dialog_item &_button_dialog_item_,bool button_right_show,bool progress_bar_show,bool m_full = false);
	int ShowButtonDialog();
	void ShowButtonDialogSettingButtonStatus(int current_id);
	void BCDoHide();
	int getButtonDialogCurrentId();
	void setButtonDialogCurrentId(int val);
	const std::string getButtonDialogTextStr(){return button_dialog_text_str;}
	void setButtonDialogShowFlag(bool flag){m_button_dialog_flag_ = flag;}
	bool getButtonDialogShowFlag(){return m_button_dialog_flag_;} 
	 bool getFdownload(){return fdownload;}
	 void setFdownload(bool flag ){fdownload = flag;}
	 bool getFinstall(){return finstall;}
	 void setFinstall(bool flag ){finstall = flag;}
	 float getTotalData(){return total_data;}
	 void setTotalData(float t_d){total_data = t_d;}
	 int getDialogCurrentId(){return button_dialog_current_id_;}
	 void keyProc(int keyCode, int isLongPress);
	 bool m_button_dialog_flag_;
    private:
        Dialog *button_dialog_;
        TextView *button_dialog_title;
        TextView *button_dialog_text;
        ButtonOK *button_dialog_confirm;
        ButtonCancel *button_dialog_cancel;
        Button *button_dialog_progress_bar_background;
        Button *button_dialog_progress_bar_top_color;
        int button_dialog_current_id_;
       // bool m_button_dialog_flag_;
        //
        timer_t progressbar_timer_id;
        double total_data;
        double current_data;
        bool fdownload,finstall;//remember the packet download and install status
        std::string button_dialog_text_str;
};
