/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: user_msg.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _USER_MSG_H_
#define _USER_MSG_H_
#include "common/setting_menu_id.h"


enum {
    MSG_APP_CHANGE_ICON = (MSG_USER + 200),   //#define MSG_USER 0x0800(2048)
/* MENULIST */
    MLM_HILIGHTED_SPACE,
    MLM_NEW_SELECTED,
    MSG_MENULIST_INIT,
    MSG_MENULIST_SWITCH,
    MSG_MENULIST_SWITCH_VALUE,
/* ResourceManager */
    MSG_RM_LANG_CHANGED,
    MSG_MENULIST_SET_CURRENT_VALUE,
    MSG_LABEL_CLICKED,
    MSG_CARDIMAGEVIEW_GET,  //arg: card_idx, vector<ImageInfo*> **
/* ProgressBar */
    PGBM_SETTIME_RANGE,
    PGBM_SETCURTIME,
    PGBM_SETSTEP,
    PGBM_DELTAPOS,
    PGBM_STEPIT,
/*MenuSetting*/
    MSG_SET_VIDEO_RESOULATION,
    MSG_SET_REAR_RECORD_RESOLUTION,
    MSG_SET_RECORD_VOLUME,
    MSG_SET_MOTION_DETECT,
    MSG_SET_SCREEN_BRIGHTNESS,
    MSG_SET_SCREEN_BRIGHTNESS_LEVELBAR,
    MSG_SET_SCREEN_NOT_DISTURB_MODE,
    MSG_SET_VOICE_TAKE_PHOTO,
    MSG_SET_WIFI_SWITCH,
    MSG_SET_4G_NET_WORK,
    MSG_SET_VOLUME_SELECTION,
    MSG_SET_SYSTEM_SOUND,
    MSG_SET_POWERON_SOUND_SWITCH,
    MSG_SET_KEY_SOUND_SWITCH,
    MSG_SET_DRIVERING_REPORT_SWITCH,
    MSG_SET_ACC_SWITCH,
    MSG_SET_ADAS_SWITCH,
    MSG_SET_STANDBY_CLOCK,
    MSG_SET_FORWARD_COLLISION_WARNING,
    MSG_SET_LANE_SHIFT_REMINDING,
    MSG_SET_WATCH_DOG_SWITCH,
    MSG_SET_TIEM_WATER_MARK,
    MSG_SET_EMER_RECORD_SWITCH,
    MSG_SET_EMER_RECORD_SENSITIVITY,
    MSG_SET_PARKING_MONITORY,
    MSG_SET_PARKING_WARN_LAMP_SWITCH,
    MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH,
    MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH,
    MSG_SET_PARKING_RECORD_LOOP_SWITCH,
    MSG_SET_PARKING_RECORD_LOOP_RESOLUTION,
    MSG_SET_RECORD_ENCODE_TYPE,
    MSG_SET_RECORD_TIME,
    MSG_SET_RECORD_DELAY_TIME,
    MSG_SET_PIC_RESOULATION,
    MSG_SET_PIC_CONTINOUS,
    MSG_SET_PIC_QUALITY,
    MSG_SET_TIME_TAKE_PIC,
    MSG_SET_AUTO_TIME_TAKE_PIC,
    MSG_SET_AUTO_TIME_SCREENSAVER,
    MSG_SET_AUTO_TIME_SHUTDOWN,
    MSG_SET_RECORD_SLOW_TIME,
    MSG_SET_RECORD_AUIDO_CHANGE,
    MSG_SET_MENU_CONFIG_LUA,
    MSG_GET_MENU_CONFIG_LUA,
    MSG_SET_CAMERA_EXPOSURE,
    MSG_SET_CAMERA_WHITEBALANCE,
    MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY,
    MSG_SET_WIFI_ON,
    MSG_SET_WIFI_OFF,
/*preview*/
    MSG_CHANG_STATU_PHOTO,
    MSG_CHANG_STATU_PREVIEW,
    MSG_SET_STATUS_PREVIEW_REC_PLAY,
    MSG_SET_STATUS_PREVIEW_REC_PAUSE,
    MSG_CHANG_STATU_SLOWREC,
    MSG_CHANG_STATU_PLAYBACK,
    MSG_RECORD_AUDIO_ON,
    MSG_RECORD_AUDIO_OFF,
    MSG_PHOTO_TIMED_VALUE,
    MSG_PHOTO_AUTO_VALUE,
    MSG_PHOTO_DRAMASHOT_VALUE,
    MSG_RECORD_LOOP_VALUE,
    MSG_RECORD_TIMELAPSE_VALUE,
    MSG_SETTING_SHOWUPDATE_DIALOG,
    MSG_STREAM_RECORD_SWITCH,
    MSG_WIFI_CLOSE,
    MSG_REMOTE_CLIENT_DISCONNECT,
    MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR_BOTTOM,//add by zhb
    MSG_PREVIW_TO_SETTING_CHANGE_STATUS_BAR,
    MSG_CAMB_PREVIEW,
/*system update*/
    MSG_SYSTEM_UPDATE,
    MSG_SYSTEM_SHUTDOWN,
/**device**/
    MSG_DEVICE_FORMAT,
    MSG_DEVICE_RESET_FACTORY,
    MSG_FORMAT_START,
    MSG_FORMAT_FINISH,
    GUI_LANG_CHANGED,
    /**Account bind*/
    MSG_ACCOUNT_UNBIND,
    /*status bar bottom *///add by zhb
    /*setting */ //add by zhb
    MSG_SETTING_STATUS_BAR_BOTTOM,
    /*playback*/
    MSG_PLAYBACK_TO_PLAY_WINDOW,
    MSG_PLAY_TO_PLAYBACK_WINDOW,
    MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM,
   /*save dialog*/
   MSG_SET_BUTTON_STATUS,
   MSG_SET_SHOW_RIGHT_LEFT_BUTTON,
   MSG_SET_SHOW_UP_DWON_BUTTON,
   MSG_SET_HIDE_UP_DWON_BUTTON,
   MSG_SET_SHOW_UP_DWON_CHOICE_BUTTON,
   MSG_SET_HIDE_UP_DWON_CHOICE_BUTTON,
   MSG_SHOW_WIFI_INFO,
   MSG_SHOW_WIFI_APP_DOWNLAOD_LINK,
   MSG_SET_4G_TRAFFIC_INFO_QUERY,
   MSG_SET_4G_FLOW_RECHARGE,
   MSG_SET_4G_SIM_CARD_INFO,
   MSG_SET_PROBE_PROMPT,
   MSG_SET_SPEED_PROMPT,
   MSG_SET_UPDATE_DATA,
   MSG_SET_VOLUME_LEVELBAR,
   MSG_SET_DEVICE_DEVICEINFO,
   MSG_SET_DEVICE_SDCARDINFO,
   MSG_SET_DEVICE_VERSIONINFO,
   MSG_DEVICE_TIME,
   MSG_DEVICE_TIME_UPDATE_NETWORK,
   MSG_DEVICE_TIME_UPDATE_MANUAL,
   MSG_PROMPTBOX_STANDBY_BREAK,
   MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION,
   MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION,
   MSG_4G_UPDATE,
   MSG_SETTING_TO_PREVIEW,
   MSG_SHUTDOWN_SAVE_TIME,
   MSG_SET_DASH_CAMERA,
   /*save playback icon*/
   MSG_PLAYBACK_PAGE_UP_DOWN_ICON,
   MSG_CANCLE_DIALG,
   MSG_PLAYBACK_SHOW_GRAY_ICON,
   MSG_PLAYBACK_SHOW_HIGHLIGHT_ICON,
   //new setting window
   MSG_PREVIEW_TO_NEWSETTING_WINDOW,
   MSG_CHANGEWINDOWMODE,

   MSG_SET_GPS_SWITCH,
   MSG_SET_SPEEDUNIT,
   MSG_SET_TIMEZONE,
   MSG_SET_DEVICE_CARID,
   MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM,
   MSG_CAMERA_REINIT_RECORD,
};

#define WM_BASE 0x000F0000
#define VIEW_MOUSE_CLICK            (WM_BASE+103)
#define WM_WINDOW_CHANGE            (WM_BASE+200)
/** 定义 user message base id */
#define USER_MSG_BASE               0x80

/*
#define SETTING_VIDEO_RESOULT       (USER_MSG_BASE+1)
#define SETTING_RECORD_TIME         (USER_MSG_BASE+2)
#define SETTING_WIFI_SWITCH         (USER_MSG_BASE+3)
#define SETTING_SOFTAP_SWITCH       (USER_MSG_BASE+4)
#define SETTING_BLUETOOTH_SWITCH    (USER_MSG_BASE+5)
#define SETTING_VOLUME_CONTROL      (USER_MSG_BASE+6)
#define SETTING_FORMAT_STORAGE      (USER_MSG_BASE+7)
#define SETTING_GET_DEVICEINFO      (USER_MSG_BASE+8)
*/
    /*** [fangjj]:menu status ***/
#define MENU_SUB_COUNTS               5

enum {
  MENU_STATU_RECORD=0,
};

enum{
   AUTOSCREENSAVER_10SEC = 0,
   AUTOSCREENSAVER_30SEC,
   AUTOSCREENSAVER_60SEC,
   AUTOSCREENSAVER_OFF,
};

enum{
   AUTOSHUTDOWN_OFF=0,
   AUTOSHUTDOWN_3MIN,
   AUTOSHUTDOWN_5MIN,
   AUTOSHUTDOWN_10MIN,
};


static const char *menu_statu_win_[]={
   "menulist_record",
};

typedef struct __msg_map
{
    const char * item_name;
    int value;
}msg_map;
typedef struct __submenu_bind
{
    const char * item_name;
    int type;
    int msgid;
}submenu_bind;
typedef struct __item_info
{
    const char * item_name;
}iteminfo;


/******* {const char *item_name,  const char *unhilight_icon, const char *hilight_icon, int type, int value }*********/
typedef struct __menu_win
{
    const char *item_name;
    const char *sub_head;
    const char *item_tips;
    const char *unhilight_icon;
    const char *hilight_icon;
    const char *unselect_icon;
    const char *select_icon;
    int type;  /***    TYPE_STRING -->0,TYPE_NULL-->1,TYPE_IMAGE-->2, TYPE_IMAGE_SELECT-->3,**/
    int value;
}menu_win;


static menu_win menu_usb_win[] = {
    {"tl_usb_charge","","", "wifi_off", "wifi_on", "menu_unselect", "menu_select", 1,0},
    {"tl_usb_mass_storage","","", "wifi_off", "wifi_on", "menu_unselect", "menu_select", 1,0},
    {"tl_usb_webcam","","", "wifi_off", "wifi_on", "menu_unselect", "menu_select", 1,0},
};

#ifdef SETTING_ITEM_DD //add by zhb
static menu_win menu_record_win[]= {
	//this index must reference to	IPCLinuxPlatform/custom_aw/apps/sdv/source/common/setting_menu_id.h:SETTING_ITEM_DD_ID index

	{"ml_record_resolution", "ml_record_resolution_sub_head","ml_device_dash_camera_tips","set_front_resolution_g", "set_front_resolution_w", "select_off", "select_m",4,2},
    {"ml_camera_autoscreensaver", "ml_camera_autoscreensaver_sub_head","","set_auto_screensaver_g", "set_auto_screensaver_w", "select_off", "select_m",4,4},
    {"ml_system_voice", "ml_system_voice_sub_head","","set_system_voice_g", "set_system_voice_w", "select_off", "select_m",4,2},
    {"ml_record_adas","ml_record_adas_sub_head","ml_record_adas_tips", "set_adas_g", "set_adas_w", "select_off", "select_m",4,2},
    {"ml_account_bingding", "ml_account_binging_sub_head", "", "set_account_g", "set_account_w", "select_off", "select_m", 5, 0},
    {"ml_device_deviceinfo","ml_device_deviceinfo_sub_head","", "set_device_info_g", "set_device_info_w", "select_off", "select_m", 5,0},
	{"ml_device_version_update", "ml_device_version_update_sub_head","","set_version_g", "set_version_w", "select_off", "select_m", 6,0},
	//no use
	{"ml_device_language","ml_device_language_sub_head", "","set_language_g", "set_language_w", "select_off", "select_m", 4,2},////
	{"ml_wifi_connect","ml_wifi_connect_sub_head", "", "set_wifi_g", "set_wifi_w", "select_off", "select_m",4,2},////
	{"ml_screen_brightness","ml_screen_brightness_sub_head","", "set_screen_brightness_g", "set_screen_brightness_w", "select_off", "select_m",4,1},
	{"ml_device_sdcard_info","ml_device_sdcard_info_sub_head","", "set_sdcard_g", "set_sdcard_w", "select_off", "select_m", 5,0},
	{"ml_device_time","ml_device_time_sub_head", "ml_device_time_save_tips","set_update_time_g", "set_update_time_w", "select_off", "select_m", 6,1},//if has the manual adjust time is 2
	{"ml_system_acc", "ml_system_acc_sub_head","","set_system_acc_g", "set_system_acc_w", "select_off", "select_m",4,2},
	 
	{"ml_record_rear_resolution","ml_record_rear_resolution_sub_head", "ml_record_rear_resolution_tips","set_rear_resolution_w", "set_rear_resolution", "select_off", "select_m",4,2},
	{"ml_record_volume", "ml_record_volume_sub_head","","set_mic_w", "set_mic", "select_off", "select_m", 4,2},
	{"ml_record_timewatermark","ml_record_timewatermark_sub_head","ml_record_timewatermark_tips", "set_data_watermark_w", "set_data_watermark", "select_off", "select_m",4,2},
	{"ml_record_emerrecord", "ml_record_emerrecord_sub_head","ml_record_emerrecord_tips","set_emer_record_w", "set_emer_record", "select_off", "select_m", 4,2},
	{"ml_voice_photo","ml_voice_photo_sub_head","ml_voice_photo_tips", "set_voice_photo_w", "set_voice_photo", "select_off", "select_m",4,2},
	{"ml_4g_network", "ml_4g_network_sub_head","","set_4g_network_w", "set_4g_network", "select_off", "select_m",4,2},
    	{"ml_record_adas","ml_record_adas_sub_head","ml_record_adas_tips", "set_adas_w", "set_adas", "select_off", "select_m",4,2},
    	{"ml_watch_dog","ml_watch_dog_sub_head","", "set_watchdog_w", "set_watchdog", "select_off", "select_m",4,2},
    	{"ml_record_parkingmonitor", "ml_record_parkingmonitor_sub_head","","set_packing_monitory_w", "set_packing_monitory", "select_off", "select_m", 4,4},
    	{"ml_record_standby_clock", "ml_record_standby_clock_sub_head","ml_record_standby_clock_tips","set_standby_clock_w", "set_standby_clock", "select_off", "select_m",4,2},
    	{"ml_driver_report","ml_driver_report_sub_head","", "set_driver_report_w", "set_driver_report", "select_off", "select_m",4,2},
    	
    	//
    	{"ml_record_emerrecordsen", "ml_record_emerrecordsen","","wifi_off", "wifi_on", "select_off", "select_m", 4,4},//
    	{"ml_record_loop","ml_record_loop", "","wifi_off", "wifi_on", "select_off", "select_m", 4,4},//
    	{"ml_device_resetfactory", "ml_device_resetfactory", "","wifi_off", "wifi_on", "select_off", "select_m", 4,0},//
    	{"ml_screen_disturb_mode","ml_screen_disturb_mode", "","wifi_off", "wifi_on", "select_off", "select_m",4,2},
  	{"ml_record_adas_forward_collision_warning","ml_record_adas_forward_collision_warning","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
  	{"ml_record_adas_lane_shift_reminding","ml_record_adas_lane_shift_reminding","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
  	{"ml_volume_select","ml_volume_select", "","wifi_off", "wifi_on", "select_off", "select_m",4,7},
  	{"ml_power_on_voice","ml_power_on_voice","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
    	{"ml_key_voice","ml_key_voice", "","wifi_off", "wifi_on", "select_off", "select_m",4,2},
    	{"ml_packing_warn_lamp","ml_packing_warn_lamp","", "wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_packing_abnormal_monitor","ml_packing_abnormal_monitor", "","wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_packing_abnormal_notice","ml_packing_abnormal_notice","", "wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_record_parkingloop_sw", "ml_record_parkingloop_sw", "","wifi_off", "wifi_on", "select_off", "select_m", 4,2},
};

#else
static menu_win menu_record_win[]= {
    {"ml_record_resolution","ml_record_resolution_sub_head", "ml_record_resolution_tips","set_front_resolution_w", "set_front_resolution", "select_off", "select_m",4,2},
    {"ml_record_rear_resolution","ml_record_rear_resolution_sub_head", "ml_record_rear_resolution_tips","set_rear_resolution_w", "set_rear_resolution", "select_off", "select_m",4,2},
    {"ml_record_volume", "ml_record_volume_sub_head","","set_mic_w", "set_mic", "select_off", "select_m", 4,2},
	{"ml_record_timewatermark","ml_record_timewatermark_sub_head","ml_record_timewatermark_tips", "set_data_watermark_w", "set_data_watermark", "select_off", "select_m",4,2},
	{"ml_screen_brightness","ml_screen_brightness_sub_head", "", "set_screen_brightness_w", "set_screen_brightness", "select_off", "select_m",4,1},
	{"ml_camera_autoscreensaver", "ml_camera_autoscreensaver_sub_head","","set_auto_screensaver_w", "set_auto_screensaver", "select_off", "select_m",4,4},
	{"ml_record_emerrecord","ml_record_emerrecord_sub_head", "ml_record_emerrecord_tips","set_emer_record_w", "set_emer_record", "select_off", "select_m", 4,2},
	{"ml_voice_photo","ml_voice_photo_sub_head","ml_voice_photo_tips", "set_voice_photo_w", "set_voice_photo", "select_off", "select_m",4,2},
	{"ml_wifi_connect","ml_wifi_connect_sub_head","ml_wifi_connect_tips", "set_wifi_w", "set_wifi", "select_off", "select_m",4,2},
	{"ml_4g_network","ml_4g_network_sub_head", "","set_4g_network_w", "set_4g_network", "select_off", "select_m",4,2},
	{"ml_device_time","ml_device_time_sub_head","ml_device_time_save_tips","set_update_time_w", "set_update_time", "select_off", "select_m", 6,1},//if has the manual adjust time is 2
    	{"ml_record_adas","ml_record_adas_sub_head","ml_record_adas_tips", "set_adas_w", "set_adas", "select_off", "select_m",4,2},
    	{"ml_watch_dog","ml_watch_dog_sub_head","", "set_watchdog_w", "set_watchdog", "select_off", "select_m",4,2},
        {"ml_record_parkingmonitor", "ml_record_parkingmonitor_sub_head", "","set_packing_monitory_w", "set_packing_monitory", "select_off", "select_m", 4,4},
        {"ml_record_standby_clock", "ml_record_standby_clock_sub_head","ml_record_standby_clock_tips","set_standby_clock_w", "set_standby_clock", "select_off", "select_m",4,2},
        {"ml_system_voice","ml_system_voice_sub_head", "","set_system_voice_w", "set_system_voice", "select_off", "select_m",4,2},
        {"ml_driver_report","ml_driver_report_sub_head","", "set_driver_report_w", "set_driver_report", "select_off", "select_m",4,2},
        {"ml_device_language","ml_device_language_sub_head", "","set_language_w", "set_language", "select_off", "select_m", 4,2},
        {"ml_device_deviceinfo","ml_device_deviceinfo_sub_head","", "set_device_info_w", "set_device_info", "select_off", "select_m", 5,0},
        {"ml_device_sdcard_info","ml_device_sdcard_info_sub_head","", "set_sdcard_w", "set_sdcard", "select_off", "select_m", 5,0},
        {"ml_device_version_update", "ml_device_version_update_sub_head","","set_version_w", "set_version", "select_off", "select_m", 6,0},
        //
        {"ml_record_emerrecordsen", "ml_record_emerrecordsen","","wifi_off", "wifi_on", "select_off", "select_m", 4,4},//
        {"ml_record_loop","ml_record_loop", "","wifi_off", "wifi_on", "select_off", "select_m", 4,4},//
        {"ml_device_resetfactory", "ml_device_resetfactory", "","wifi_off", "wifi_on", "select_off", "select_m", 4,0},//
        {"ml_screen_disturb_mode","ml_screen_disturb_mode", "","wifi_off", "wifi_on", "select_off", "select_m",4,2},
        {"ml_record_adas_forward_collision_warning","ml_record_adas_forward_collision_warning","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
  	{"ml_record_adas_lane_shift_reminding","ml_record_adas_lane_shift_reminding","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
  	{"ml_volume_select","ml_volume_select", "","wifi_off", "wifi_on", "select_off", "select_m",4,7},
  	{"ml_power_on_voice","ml_power_on_voice","", "wifi_off", "wifi_on", "select_off", "select_m",4,2},
    	{"ml_key_voice","ml_key_voice", "","wifi_off", "wifi_on", "select_off", "select_m",4,2},
    	{"ml_packing_warn_lamp","ml_packing_warn_lamp","", "wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_packing_abnormal_monitor","ml_packing_abnormal_monitor", "","wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_packing_abnormal_notice","ml_packing_abnormal_notice","", "wifi_off", "wifi_on", "select_off", "select_m", 4,2},
    	{"ml_record_parkingloop_sw", "ml_record_parkingloop_sw", "","wifi_off", "wifi_on", "select_off", "select_m", 4,2},
};
#endif

#define PREVIEW_TIME_TO_TAKEPIC              (USER_MSG_BASE+91)
#define PLAYBACK_SHOW_UPDATE                 (USER_MSG_BASE+92)
#define MSG_CHANG_STATU_SETTING              (USER_MSG_BASE+93)
#define MSG_DATABASE_UPDATE_FINISHED         (USER_MSG_BASE+94)
#define MSG_PLAYBACK_DELETE_FILE             (USER_MSG_BASE+95)
#define MSG_PLAYBACK_SHOW_DELETE_DIALOG      (USER_MSG_BASE+96)
#define MSG_PREVIEW_SLOW_RECORD              (USER_MSG_BASE+103)
#define MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW (USER_MSG_BASE+104)
#define MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY (USER_MSG_BASE+105)
#define MSG_CHANG_STATU_PLAYBACK_NO_FILE_SDCARD (USER_MSG_BASE+106)
#define MSG_CHANG_STATU_PLYABACK_PLAY_DELETE_SH (USER_MSG_BASE+107)



#define MSGMAP_COUNT 50
static msg_map msg_map_[]= {
    {"ml_record_resolution",SETTING_RECORD_RESOLUTION},//0
    {"ml_record_rear_resolution",SETTING_REAR_RECORD_RESOLUTION},
    {"ml_record_volume",SETTING_RECORD_VOLUME},
    {"ml_record_timewatermark", SETTING_TIMEWATERMARK},
    {"ml_screen_brightness",SETTING_SCREEN_BRIGHTNESS},
    {"ml_camera_autoscreensaver",SETTING_CAMERA_AUTOSCREENSAVER},
    {"ml_record_emerrecord", SETTING_EMER_RECORD_SWITCH},
    {"ml_record_emerrecordsen", SETTING_EMER_RECORD_SENSITIVITY},
    {"ml_voice_photo", SETTING_VOICE_TAKE_PHOTO},
    {"ml_record_adas", SETTING_ADAS_SWITCH},
    {"ml_watch_dog", SETTING_WATCH_DOG_SWITCH},
    {"ml_volume_select", SETTING_VOLUME_SELECTION},
    {"ml_probe_prompt", SETTING_PROBE_PROMPT},
    {"ml_speed_prompt", SETTING_SPEED_PROMPT},
    {"ml_record_parkingmonitor",SETTING_PARKING_MONITORY},
    {"ml_packing_warn_lamp", SETTING_PARKING_WARN_LAMP_SWITCH},
    {"ml_packing_abnormal_monitor", SETTING_PARKING_ABNORMAL_MONITORY_SWITCH},
    {"ml_packing_abnormal_notice", SETTING_PARKING_ABNORMAL_NOTICE_SWITCH},
    {"ml_record_parkingloop_sw", SETTING_PARKING_RECORD_LOOP_SWITCH},
    {"ml_record_parkingloop_resolution", SETTING_PARKING_RECORD_LOOP_RESOLUTION},
    {"ml_record_loop", SETTING_RECORD_LOOP},
    {"ml_record_timelapse", SETTING_RECORD_TIMELAPSE},
    {"ml_system_voice", SETTING_SYSTEM_SOUND},
    {"ml_power_on_voice", SETTING_POWERON_SOUND_SWITCH},
    {"ml_key_voice", SETTING_KEY_SOUND_SWITCH},
    {"ml_driver_report", SETTING_DRIVERING_REPORT_SWITCH},
    {"ml_device_language", SETTING_DEVICE_LANGUAGE},
    {"ml_account_bingding", SETTING_ACCOUNT_BINDING},
    {"ml_device_deviceinfo",SETTING_DEVICE_DEVICEINFO},
    {"ml_device_sdcard_info",SETTING_DEVICE_SDCARDINFO},
    {"ml_device_version_update",SETTING_DEVICE_VERSIONINFO},
    {"ml_device_resetfactory",SETTING_DEVICE_RESETFACTORY},
    {"ml_screen_disturb_mode",SETTING_SCREEN_NOT_DISTURB_MODE},
    {"ml_record_adas_forward_collision_warning", SETTING_FORWARD_COLLISION_WARNING},
    {"ml_record_adas_lane_shift_reminding", SETTING_LANE_SHIFT_REMINDING},
    {"ml_wifi_select", SETTING_WIFI_SWITCH},
    {"ml_4g_network", SETTING_4G_NET_WORK},//28
    {"ml_record_standby_clock", SETTING_STANDBY_CLOCK},//
    {"ml_device_time", SETTING_DEVICE_TIME},//
    {"ml_system_acc", SETTING_ACC_SWITCH},
    {"ml_camB_Preview", SETTING_CAMB_PREVIEWING},
    {"ml_photo_resolution", SETTING_PHOTO_RESOLUTION},
	{"ml_photo_timed",SETTING_PHOTO_TIMED},
	{"ml_photo_auto",SETTING_PHOTO_AUTO},
	{"ml_photo_dramashot",SETTING_PHOTO_DRAMASHOT},
    {"ml_photo_Quality",SETTING_PHOTO_QUALITY},
    
	{"ml_gps_switch",SETTING_GPS_SWITCH},
	{"ml_speed_unit",SETTING_SPEED_UNIT},
	{"ml_timezone",SETTING_TIMEZONE},
	{"ml_device_carid_config",SETTING_DEVICE_CARID},
	
};
/*
typedef enum
{
    TYPE_SUBITEM_CHOICE=0,
    TYPE_SUBITEM_SWITCH,//the same level item type is switch
    TYPE_SUBITEM_ENLARGE,//the same level item type is view
    TYPE_SUBITEM_SET,//the same level  item type is set
    TYPE_SUBITEM_UPDATE,//the same level  item type is update
    TYPE_SUBITEM_SUBMENU_SWITCH,//the subme  item type is switch
    TYPE_SUBITEM_SUBMENU_ENLARGE,////the subme  item type is view
    TYPE_SUBITEM_SUBMENU_CHOICE,//7
}SubItemType;

*/
#define NUM_PACKING_MONITORY 5
static submenu_bind packingmonitory_[]={
    {"ml_packing_warn_lamp",1, SETTING_PARKING_WARN_LAMP_SWITCH},
    {"ml_packing_abnormal_monitor",1, SETTING_PARKING_ABNORMAL_MONITORY_SWITCH},
    {"ml_packing_abnormal_notice",1, SETTING_PARKING_ABNORMAL_NOTICE_SWITCH},
    {"ml_record_parkingloop_sw", 1,SETTING_PARKING_RECORD_LOOP_SWITCH},
    {"ml_record_parkingloop_resolution",7,SETTING_PARKING_RECORD_LOOP_RESOLUTION}
};
#define NUM_ADAS 2
static submenu_bind adas_[]={
    {"ml_record_adas_forward_collision_warning",1, SETTING_FORWARD_COLLISION_WARNING},
    {"ml_record_adas_lane_shift_reminding",1, SETTING_LANE_SHIFT_REMINDING},
};

#define NUM_DASH_CAMERA 2
static submenu_bind dash_camera_[]={
    {"ml_vioce_record_function",1, SETTING_RECORD_VOLUME},
    {"ml_vehicle_record_function",1, SETTING_REAR_RECORD_RESOLUTION},
};


#define NUM_4G_NETWORK 2
static submenu_bind network_4g_[]={
    {"ml_traffic_info_query",6, -1},
    {"ml_flow_recharge",6, -1},
};

#define NUM_WATCHDOG 2
static submenu_bind watch_dog_[]={
    {"ml_probe_prompt",5, SETTING_PROBE_PROMPT},
    {"ml_speed_prompt",5, SETTING_SPEED_PROMPT},
};

#define NUM_SYSTEM_SOUND 2
static submenu_bind system_sound_[]={
    {"ml_power_on_voice",1, SETTING_POWERON_SOUND_SWITCH},
    {"ml_key_voice",1, SETTING_KEY_SOUND_SWITCH},
};


//item info
#define NUM_DEVICE_INFO 6
static iteminfo device_info_[]={
	{"ml_device_deviceinfo_sn"},
	{"ml_device_deviceinfo_sim"},	
	{"ml_device_deviceinfo_account_id"},
	{"ml_device_deviceinfo_imei_id"},
	{"ml_device_deviceinfo_view"},
	{"ml_device_resetfactory"},	
};

#define NUM_ACCOUNT_INFO    4
static iteminfo account_info_[] = {
    {"ml_account_info_id"},
    {"ml_account_unbinding_info_1"},
    {"ml_account_unbinding_info_2"},
    {"ml_account_binding_button"},
    {"ml_account_unbind_button"},
};

#define NUM_SDCARD_INFO 4
static iteminfo sdcard_info_[]={
	{"ml_device_sdcard_capacity_used"},
	{"ml_device_sdcard_total_capacity"},
	{"ml_device_sdcard_capacity"},	
	{"ml_device_sdcard_format"},
};

#define NUM_VERSION_INFO 7
static iteminfo version_info_[]={
	{"ml_device_version_current"},
	{"ml_device_version_new"},
	{"ml_device_version_info"},	//2
	{"ml_device_version_update_now"},//3
	{"ml_device_version_new_can_upgraded"},//4
	{"ml_device_version_update_detect"},//5
	{"ml_device_4g_module_version_current"}//6
};

////////////////////////////////
/*
* for setting new window listboxview 
*/
typedef struct __ListViewItem
{
    const char *first_icon_path;
    const char *first_text;
    const int  type;
    const char *second_icon_path0;//如果第二个是button，这个保存unselect 图片,否则保存平常图片
    const char *second_icon_path1;//如果第二个是button，这个保存select 图片
    
}ListViewItem;
enum
{
    TYPE_NORMAL = 0,//switch to sublistview window
    TYPE_SWITCH,//button type
    TYPE_DIALOG,//show dialog window type
    TYPE_DIALOG_STR,//dialog + str

};
#define SUPPORT_PHOTOMODE
static ListViewItem ListBoxItemData[]= {
    {"sw_resolution","ml_record_resolution",TYPE_NORMAL,"sw_switch",NULL},
//    {"set_front_resolution_w","ml_record_rear_resolution",TYPE_NORMAL,"menu_select",NULL},
    {"sw_record_time","ml_record_loop",TYPE_NORMAL,"sw_switch",NULL},
#if 0
	{"sw_NormorPhoto","ml_photo_resolution",TYPE_NORMAL,"sw_switch",NULL}, 
    {"sw_TimePhoto","ml_photo_timed",TYPE_NORMAL,"sw_switch",NULL},  
	{"sw_AutoPhoto","ml_photo_auto",TYPE_NORMAL,"sw_switch",NULL},
	{"sw_quality","ml_photo_Quality",TYPE_NORMAL,"sw_switch",NULL},
#endif
#ifdef SUPPORT_RECTIMELAPS
    {"sw_record_timelaps","ml_record_timelapse",TYPE_NORMAL,"sw_switch",NULL},
#endif
    {"sw_encode_type","ml_record_encodingtype",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_exposure","ml_camera_exposure",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_park","ml_record_parkingmonitor",TYPE_SWITCH,"sw_button_off","sw_button_on"},
    {"sw_emerrecord_sen","ml_record_emerrecordsen",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_screensaver","ml_camera_autoscreensaver",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_language","ml_device_language",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_volume","ml_record_volume",TYPE_SWITCH,"sw_button_off","sw_button_on"},
    {"sw_volume","ml_volume_select",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_wifi","ml_wifi_select",TYPE_SWITCH,"sw_button_off","sw_button_on"},
    {"sw_awmd","ml_motion_detect", TYPE_SWITCH, "sw_button_off", "sw_button_on"},
    {"sw_light_fre","ml_camera_lightsourcefrequency",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_time","ml_device_time",TYPE_DIALOG,"sw_switch",NULL},
    {"sw_format","ml_device_format",TYPE_DIALOG_STR,"sw_switch",NULL},

	{"sw_GPS","ml_gps_switch",TYPE_SWITCH,"sw_button_off","sw_button_on"},
	{"sw_speed","ml_speed_unit",TYPE_NORMAL,"sw_switch",NULL},
	{"sw_timezone","ml_timezone",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_carid","ml_device_carid_config",TYPE_DIALOG,"sw_switch",NULL},
    
    {"sw_reset","ml_device_resetfactory",TYPE_DIALOG,"sw_switch",NULL},
    {"sw_about","ml_device_deviceinfo",TYPE_DIALOG,"sw_switch",NULL},
};

static ListViewItem ListBoxItemDataPhoto[]= {
    {"sw_NormorPhoto","ml_photo_resolution",TYPE_NORMAL,"sw_switch",NULL}, 
    {"sw_TimePhoto","ml_photo_timed",TYPE_NORMAL,"sw_switch",NULL},  
	{"sw_AutoPhoto","ml_photo_auto",TYPE_NORMAL,"sw_switch",NULL},
	{"sw_quality","ml_photo_Quality",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_exposure","ml_camera_exposure",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_screensaver","ml_camera_autoscreensaver",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_language","ml_device_language",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_volume","ml_volume_select",TYPE_NORMAL,"sw_switch",NULL},        
    {"sw_wifi","ml_wifi_select",TYPE_SWITCH,"sw_button_off","sw_button_on"},
    {"sw_light_fre","ml_camera_lightsourcefrequency",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_time","ml_device_time",TYPE_DIALOG,"sw_switch",NULL},
    {"sw_format","ml_device_format",TYPE_DIALOG_STR,"sw_switch",NULL},

	{"sw_GPS","ml_gps_switch",TYPE_SWITCH,"sw_button_off","sw_button_on"},
	{"sw_speed","ml_speed_unit",TYPE_NORMAL,"sw_switch",NULL},
	{"sw_timezone","ml_timezone",TYPE_NORMAL,"sw_switch",NULL},
    {"sw_carid","ml_device_carid_config",TYPE_DIALOG,"sw_switch",NULL},
    
    {"sw_reset","ml_device_resetfactory",TYPE_DIALOG,"sw_switch",NULL},
    {"sw_about","ml_device_deviceinfo",TYPE_DIALOG,"sw_switch",NULL},
};

static ListViewItem myListBoxItemData[SETTING_LISTVIEW_NUM+2] = {0};

#endif //_USER_MSG_H_
