/* *******************************************************************************
 * Copyright (C), 2017-2025, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file menu_config_lua.cpp
 * @brief 璁剧疆鍙傛暟鎺у埗鎺ュ彛
 * @author :fangjianjun
 * @version v1.0
 * @date 2017-03-29
 */

#include "menu_config_lua.h"
#include "common/app_log.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "common/setting_menu_id.h"
#include "device_model/system/rtc.h"

#undef LOG_TAG
#define LOG_TAG "MenuConfigLua"
#define NO_DEBUG   0
#define DEFAULT_VALUE -1
using namespace EyeseeLinux;
using namespace std;

#define MENU_CONFIG_FILE "/tmp/data/menu_config.lua"
#define MENU_CONFIG_FILE_USR "/usr/share/app/sdv/menu_config.lua"
#define MENU_CONFIG_FILE_USR_TEMP "/usr/share/app/sdv/menu_config_temp.lua"


MenuConfigLua::MenuConfigLua()
{
    memset(&menu_cfg_, 0, sizeof(menu_cfg_));
    memset(&usr_temp_menu_cfg_, 0, sizeof(usr_temp_menu_cfg_));
    memset(&data_menu_cfg_, 0, sizeof(data_menu_cfg_));
	db_error("MenuConfigLua::MenuConfigLua");
    if(ChangeMenuConfig() < 0)
   	{
		db_msg("[zhb]:[FUN]:%s [LINE]:%d  Do ChangeMenuConfig !\n", __func__, __LINE__);
   	}
	
    lua_cfg_ = new LuaConfig();
    int ret = LoadMenuConfig();
    if (ret) {
        db_msg("[fangjj]:[FUN]:%s [LINE]:%d  Do LoadMenuConfig fail:%d !\n", __func__, __LINE__, ret);
    }
	char *penv = getenv("TZ");	// "GMT-8"
	if (penv == NULL) {
		char tzstr[256] = {0};	
		
		int tzhour = -GetTimezone();
		#ifndef RTCASUTC 	
		snprintf(tzstr,sizeof(tzstr),"GMT%+02d",tzhour);
		#else
		tzhour = 0;
		snprintf(tzstr,sizeof(tzstr),"GMT%+02d",tzhour);
		#endif
		if(setenv("TZ", tzstr, 1)!=0)
		{
			db_error("setenv failed \n");
		}
		tzset();
		db_error("set timezone at startup: %d",-tzhour);
	}
   //ready to set the system
   UpdateSystemTime(false);
}

MenuConfigLua::~MenuConfigLua()
{
    if (NULL != lua_cfg_) {
        delete lua_cfg_;
    }
}

int MenuConfigLua::SaveMenuAllConfig(void)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == this->lua_cfg_) {
        db_error("[fangjj]:The lua_cfg_ is NULL! error! \n");
        return -1;
    }
    if (!FILE_EXIST(MENU_CONFIG_FILE)) {
        db_warn("[fangjj]:config file %s not exist, copy default from /usr/share/app/sdv", MENU_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(MENU_CONFIG_FILE);
    if (ret) {
        db_warn("[fangjj]:Load %s failed, copy backup and try again", MENU_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
    
        ret = lua_cfg_->LoadFromFile(MENU_CONFIG_FILE);
        if (ret) {
            db_error("[fangjj]:Load %s failed!", MENU_CONFIG_FILE);
            return -1;
        }
    }
      db_msg("[fangjj]:Load %s success", MENU_CONFIG_FILE);		// 取回 "/tmp/data/menu_config.lua" 的设置数据

     	/*****switch******/
       lua_cfg_->SetIntegerValue("menu.switch.record_eis",  menu_cfg_.switch_record_eis);
       lua_cfg_->SetIntegerValue("menu.switch.record_awmd",  menu_cfg_.switch_record_awmd); 
       lua_cfg_->SetIntegerValue("menu.switch.record_drivingmode",  menu_cfg_.switch_record_drivingmode);
       lua_cfg_->SetIntegerValue("menu.switch.camera_imagerotation",  menu_cfg_.switch_camera_imagerotation);
       lua_cfg_->SetIntegerValue("menu.switch.camera_ledindicator",  menu_cfg_.switch_camera_ledindicator);
       lua_cfg_->SetIntegerValue("menu.switch.camera_timewatermark",  menu_cfg_.switch_camera_timewatermark);
       lua_cfg_->SetIntegerValue("menu.switch.camera_distortioncalibration",  menu_cfg_.switch_camera_distortioncalibration);
       lua_cfg_->SetIntegerValue("menu.switch.camera_keytone",  menu_cfg_.switch_camera_keytone);
	   lua_cfg_->SetIntegerValue("menu.switch.camB_preview",menu_cfg_.camB_preview);

	   //**** acc_resume *****//
	   lua_cfg_->SetIntegerValue("menu.switch.acc_resume",  menu_cfg_.switch_acc_resume);
	   lua_cfg_->SetIntegerValue("menu.record.acc.current",  menu_cfg_.record_switch_acc.current);
	   lua_cfg_->SetIntegerValue("menu.record.acc.count",	menu_cfg_.record_switch_acc.count);
       /*****record******/
      lua_cfg_->SetIntegerValue("menu.record.resolution.current",  menu_cfg_.record_resolution.current);
      lua_cfg_->SetIntegerValue("menu.record.resolution.count",  menu_cfg_.record_resolution.count);
      lua_cfg_->SetIntegerValue("menu.record.record_sound.current",  menu_cfg_.record_sound.current);
      lua_cfg_->SetIntegerValue("menu.record.record_sound.count",  menu_cfg_.record_sound.count);
      lua_cfg_->SetIntegerValue("menu.record.wifisw.current",  menu_cfg_.record_switch_wifi.current);
      lua_cfg_->SetIntegerValue("menu.record.wifisw.count",  menu_cfg_.record_switch_wifi.count);
      lua_cfg_->SetIntegerValue("menu.record.rear_resolution.current",  menu_cfg_.record_rear_resolution.current);
      lua_cfg_->SetIntegerValue("menu.record.rear_resolution.count",  menu_cfg_.record_rear_resolution.count);
      lua_cfg_->SetIntegerValue("menu.record.screenbrightness.current",  menu_cfg_.record_screen_brightness.current);
      lua_cfg_->SetIntegerValue("menu.record.screenbrightness.count",  menu_cfg_.record_screen_brightness.count);
      lua_cfg_->SetIntegerValue("menu.record.disturbmode.current",  menu_cfg_.record_screen_disturb_mode.current);
      lua_cfg_->SetIntegerValue("menu.record.disturbmode.count",  menu_cfg_.record_screen_disturb_mode.count);
      lua_cfg_->SetIntegerValue("menu.record.voicephoto.current",  menu_cfg_.record_voice_take_photo.current);
      lua_cfg_->SetIntegerValue("menu.record.voicephoto.count",  menu_cfg_.record_voice_take_photo.count);
      lua_cfg_->SetIntegerValue("menu.record.network4g.current",  menu_cfg_.record_4g_network_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.network4g.count",  menu_cfg_.record_4g_network_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.volumeselect.current",  menu_cfg_.record_volume_selection.current);
      lua_cfg_->SetIntegerValue("menu.record.volumeselect.count",  menu_cfg_.record_volume_selection.count);
      lua_cfg_->SetIntegerValue("menu.record.poweronvoice.current",  menu_cfg_.record_power_on_sound.current);
      lua_cfg_->SetIntegerValue("menu.record.poweronvoice.count",  menu_cfg_.record_power_on_sound.count);
      lua_cfg_->SetIntegerValue("menu.record.keyvoice.current",  menu_cfg_.record_key_sound.current);
      lua_cfg_->SetIntegerValue("menu.record.keyvoice.count",  menu_cfg_.record_key_sound.count);
      lua_cfg_->SetIntegerValue("menu.record.drivereport.current",  menu_cfg_.record_drivering_report.current);
      lua_cfg_->SetIntegerValue("menu.record.drivereport.count",  menu_cfg_.record_drivering_report.count);
      lua_cfg_->SetIntegerValue("menu.record.adas.current",  menu_cfg_.record_adas.current);
      lua_cfg_->SetIntegerValue("menu.record.adas.count",  menu_cfg_.record_adas.count);
      lua_cfg_->SetIntegerValue("menu.record.standbyclock.current",  menu_cfg_.record_standby_clock.current);
      lua_cfg_->SetIntegerValue("menu.record.standbyclock.count",  menu_cfg_.record_standby_clock.count);
      lua_cfg_->SetIntegerValue("menu.record.adasfcw.current",  menu_cfg_.record_adas_forward_collision_waring.current);
      lua_cfg_->SetIntegerValue("menu.record.adasfcw.count",  menu_cfg_.record_adas_forward_collision_waring.count);
      lua_cfg_->SetIntegerValue("menu.record.adaslsr.current",  menu_cfg_.record_adas_lane_shift_reminding.current);
      lua_cfg_->SetIntegerValue("menu.record.adaslsr.count",  menu_cfg_.record_adas_lane_shift_reminding.count);
      lua_cfg_->SetIntegerValue("menu.record.watchdog.current",  menu_cfg_.record_watchdog.current);
      lua_cfg_->SetIntegerValue("menu.record.watchdog.count",  menu_cfg_.record_watchdog.count);
      lua_cfg_->SetIntegerValue("menu.record.probeprompt.current",  menu_cfg_.record_probeprompt.current);
      lua_cfg_->SetIntegerValue("menu.record.probeprompt.count",  menu_cfg_.record_probeprompt.count);
      lua_cfg_->SetIntegerValue("menu.record.speedprompt.current",  menu_cfg_.record_speedprompt.current);
      lua_cfg_->SetIntegerValue("menu.record.speedprompt.count",  menu_cfg_.record_speedprompt.count);
      lua_cfg_->SetIntegerValue("menu.record.timewatermark.current",  menu_cfg_.record_timewatermark.current);
      lua_cfg_->SetIntegerValue("menu.record.timewatermark.count",  menu_cfg_.record_timewatermark.count);
      lua_cfg_->SetIntegerValue("menu.record.emerrecord.current",  menu_cfg_.record_emerrecord.current);
      lua_cfg_->SetIntegerValue("menu.record.emerrecord.count",  menu_cfg_.record_emerrecord.count);
      lua_cfg_->SetIntegerValue("menu.record.emerrecordsen.current",  menu_cfg_.record_emerrecordsen.current);
      lua_cfg_->SetIntegerValue("menu.record.emerrecordsen.count",  menu_cfg_.record_emerrecordsen.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingwarnlamp.current",  menu_cfg_.record_parkingwarnlamp_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingwarnlamp.count",  menu_cfg_.record_parkingwarnlamp_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingmonitor.current",  menu_cfg_.record_parkingmonitor_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingmonitor.count",  menu_cfg_.record_parkingmonitor_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingabnormalmonitor.current",  menu_cfg_.record_parkingabnormalmonitory_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingabnormalmonitor.count",  menu_cfg_.record_parkingabnormalmonitory_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingabnormalnotice.current",  menu_cfg_.record_parkingloopabnormalnotice_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingabnormalnotice.count",  menu_cfg_.record_parkingloopabnormalnotice_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingloopsw.current",  menu_cfg_.record_parkingloop_switch.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingloopsw.count",  menu_cfg_.record_parkingloop_switch.count);
      lua_cfg_->SetIntegerValue("menu.record.parkingloopresolution.current",  menu_cfg_.record_parkingloop_resolution.current);
      lua_cfg_->SetIntegerValue("menu.record.parkingloopresolution.count",  menu_cfg_.record_parkingloop_resolution.count);
      lua_cfg_->SetIntegerValue("menu.record.encodingtype.current",  menu_cfg_.record_encodingtype.current);
      lua_cfg_->SetIntegerValue("menu.record.encodingtype.count",  menu_cfg_.record_encodingtype.count);
      lua_cfg_->SetIntegerValue("menu.record.loop.current",  menu_cfg_.record_loop.current);
      lua_cfg_->SetIntegerValue("menu.record.loop.count",  menu_cfg_.record_loop.count);
      lua_cfg_->SetIntegerValue("menu.record.timelapse.current",  menu_cfg_.record_timelapse.current);
      lua_cfg_->SetIntegerValue("menu.record.timelapse.count",  menu_cfg_.record_timelapse.count);
      lua_cfg_->SetIntegerValue("menu.record.slowmotion.current",  menu_cfg_.record_slowmotion.current);
      lua_cfg_->SetIntegerValue("menu.record.slowmotion.count",  menu_cfg_.record_slowmotion.count);
     
       /*****photo******/
      lua_cfg_->SetIntegerValue("menu.photo.resolution.current",  menu_cfg_.photo_resolution.current);
      lua_cfg_->SetIntegerValue("menu.photo.resolution.count",  menu_cfg_.photo_resolution.count);
      lua_cfg_->SetIntegerValue("menu.photo.timed.current",  menu_cfg_.photo_timed.current);
      lua_cfg_->SetIntegerValue("menu.photo.timed.count",  menu_cfg_.photo_timed.count);
      lua_cfg_->SetIntegerValue("menu.photo.auto.current",  menu_cfg_.photo_auto.current);
      lua_cfg_->SetIntegerValue("menu.photo.auto.count",  menu_cfg_.photo_auto.count);
      lua_cfg_->SetIntegerValue("menu.photo.dramashot.current",  menu_cfg_.photo_dramashot.current);
      lua_cfg_->SetIntegerValue("menu.photo.dramashot.count",  menu_cfg_.photo_dramashot.count);
      lua_cfg_->SetIntegerValue("menu.photo.quality.current",  menu_cfg_.photo_quality.current);
      lua_cfg_->SetIntegerValue("menu.photo.quality.count",  menu_cfg_.photo_quality.count);     
     
       /*****camera******/
      lua_cfg_->SetIntegerValue("menu.camera.exposure.current",  menu_cfg_.camera_exposure.current);
      lua_cfg_->SetIntegerValue("menu.camera.exposure.count",  menu_cfg_.camera_exposure.count);
      lua_cfg_->SetIntegerValue("menu.camera.whitebalance.current",  menu_cfg_.camera_whitebalance.current);
      lua_cfg_->SetIntegerValue("menu.camera.whitebalance.count",  menu_cfg_.camera_whitebalance.count); 
      lua_cfg_->SetIntegerValue("menu.camera.lightfreq.current",  menu_cfg_.camera_lightfreq.current);
      lua_cfg_->SetIntegerValue("menu.camera.lightfreq.count",  menu_cfg_.camera_lightfreq.count); 
      lua_cfg_->SetIntegerValue("menu.camera.autoscreensaver.current",  menu_cfg_.camera_autoscreensaver.current);
      lua_cfg_->SetIntegerValue("menu.camera.autoscreensaver.count",  menu_cfg_.camera_autoscreensaver.count);
      lua_cfg_->SetIntegerValue("menu.camera.timedshutdown.current",  menu_cfg_.camera_timedshutdown.current);
      lua_cfg_->SetIntegerValue("menu.camera.timedshutdown.count",  menu_cfg_.camera_timedshutdown.count);
      
      lua_cfg_->SetStringValue("menu.camera.wifiinfo.ssid",  menu_cfg_.camera_wifiinfo.string1);
      lua_cfg_->SetStringValue("menu.camera.wifiinfo.password",  menu_cfg_.camera_wifiinfo.string2); 
       
             /*****device******/
	  lua_cfg_->SetStringValue("menu.device.systemTimeSave.timeStr",  menu_cfg_.device_systemTime.string1);
	  lua_cfg_->SetStringValue("menu.device.updatemenu.update",  menu_cfg_.device_update.string1);
      lua_cfg_->SetStringValue("menu.device.sysversion.version",  menu_cfg_.device_sysversion.string1);
      lua_cfg_->SetIntegerValue("menu.device.language.current",  menu_cfg_.device_language.current);
      lua_cfg_->SetIntegerValue("menu.device.language.count",  menu_cfg_.device_language.count);
      lua_cfg_->SetIntegerValue("menu.device.updatetime.current",  menu_cfg_.device_datatime.current);
      lua_cfg_->SetIntegerValue("menu.device.updatetime.count",  menu_cfg_.device_datatime.count);
      lua_cfg_->SetIntegerValue("menu.device.voicestatus.current",  menu_cfg_.device_voicestatus.current);
      lua_cfg_->SetIntegerValue("menu.device.voicestatus.count",  menu_cfg_.device_voicestatus.count);

	  lua_cfg_->SetIntegerValue("menu.device.gpsswitch.current",  menu_cfg_.device_gpswitch.current);
      lua_cfg_->SetIntegerValue("menu.device.gpsswitch.count",  menu_cfg_.device_gpswitch.count);
	  lua_cfg_->SetIntegerValue("menu.device.speedunit.current",  menu_cfg_.device_speedunit.current);
      lua_cfg_->SetIntegerValue("menu.device.speedunit.count",  menu_cfg_.device_speedunit.count);
	  lua_cfg_->SetIntegerValue("menu.device.timezone.current",  menu_cfg_.device_timezone.current);
      lua_cfg_->SetIntegerValue("menu.device.timezone.count",  menu_cfg_.device_timezone.count);

	  lua_cfg_->SetStringValue("menu.device.carid.idstr",  menu_cfg_.device_carid.string1);
    ret = lua_cfg_->SyncConfigToFile(MENU_CONFIG_FILE, "menu");
    if (ret < 0) {
        db_error("[fangjj]:Do SyncConfigToFile error! file:%s \n", MENU_CONFIG_FILE);
        return -1;
    }

    return 0;
}

int MenuConfigLua::DefaultMenuConfig(void)
{
    return 0;
}

int MenuConfigLua::SetMenuIndexConfig(int msg, int val)
{
    //db_error("[fangjj]:SetMenuIndexConfig:msg[%d], val[%d]", msg, val);
    switch(msg) {
               /*****switch******/
          case MSG_SET_RECORD_VOLUME:
          {
              menu_cfg_.record_sound.current=val;
        	  db_warn(" menu_cfg_.record_sound.current %d",menu_cfg_.record_sound.current);
          }
          break;
          case MSG_SET_MOTION_DETECT:
          menu_cfg_.switch_record_awmd=val;
          break;
//          case SETTING_RECORD_DRIVINGMODE:
//          menu_cfg_.switch_record_drivingmode=val;
//          break;			
          case SETTING_CAMERA_IMAGEROTATION:
          menu_cfg_.switch_camera_imagerotation	=val;	
          break;
          case SETTING_CAMERA_LEDINDICATOR:
          menu_cfg_.switch_camera_ledindicator =val;
          break;		
          case SETTING_CAMERA_TIMEWATERMARK:
          menu_cfg_.switch_camera_timewatermark=val;	
          break;	
		  //**** acc_resume *****//
          case SETTING_ACC_RESUME:
          menu_cfg_.switch_acc_resume=val;	
          break;		  
          case SETTING_CAMERA_DISTORTIONCALIBRATION:
          menu_cfg_.switch_camera_distortioncalibration	=val;
          break;
          case MSG_SET_WIFI_SWITCH:
          menu_cfg_.record_switch_wifi.current=val;		
          break;
	  case MSG_SET_4G_NET_WORK:
	   menu_cfg_.record_4g_network_switch.current = val;
	  break;
//          case SETTING_CAMERA_KEYTONE:
//          menu_cfg_.switch_camera_keytone=val;		
//          break;	
              /*****record******/
		  case MSG_CAMB_PREVIEW:
		  	menu_cfg_.camB_preview = val;
		  	break;
          case MSG_SET_VIDEO_RESOULATION:
          menu_cfg_.record_resolution.current   =val;
          break;
	  case MSG_SET_REAR_RECORD_RESOLUTION:
          menu_cfg_.record_rear_resolution.current   =val;
          break;
	   case MSG_SET_SCREEN_BRIGHTNESS:
          menu_cfg_.record_screen_brightness.current   =val;
          break;
	  case MSG_SET_SCREEN_NOT_DISTURB_MODE:
          menu_cfg_.record_screen_disturb_mode.current   =val;
          break;
	   case MSG_SET_VOICE_TAKE_PHOTO:
          menu_cfg_.record_voice_take_photo.current   =val;
          break;
	   case MSG_SET_VOLUME_SELECTION:
          menu_cfg_.record_volume_selection.current   =val;
          break;
	  case MSG_SET_POWERON_SOUND_SWITCH:
          menu_cfg_.record_power_on_sound.current   =val;
          break;
	  case MSG_SET_KEY_SOUND_SWITCH:
          menu_cfg_.record_key_sound.current   =val;
          break;
	  case MSG_SET_ACC_SWITCH:
          menu_cfg_.record_switch_acc.current   =val;
          break;		  
	   case MSG_SET_DRIVERING_REPORT_SWITCH:
          menu_cfg_.record_drivering_report.current   =val;
          break;
	  case MSG_SET_ADAS_SWITCH:
          menu_cfg_.record_adas.current   =val;
          break;
	 case MSG_SET_STANDBY_CLOCK:
          menu_cfg_.record_standby_clock.current   =val;
          break;
	  case MSG_SET_FORWARD_COLLISION_WARNING:
          menu_cfg_.record_adas_forward_collision_waring.current   =val;
          break;
         case MSG_SET_LANE_SHIFT_REMINDING:
          menu_cfg_.record_adas_lane_shift_reminding.current   =val;
          break;
	  case MSG_SET_WATCH_DOG_SWITCH:
          menu_cfg_.record_watchdog.current   =val;
          break;
	  case MSG_SET_PROBE_PROMPT:
          menu_cfg_.record_probeprompt.current   =val;
          break;
	  case MSG_SET_SPEED_PROMPT:
          menu_cfg_.record_speedprompt.current   =val;
          break;
	  case MSG_SET_TIEM_WATER_MARK:
          menu_cfg_.record_timewatermark.current   =val;
          break;
	  case MSG_SET_EMER_RECORD_SWITCH:
          menu_cfg_.record_emerrecord.current   =val;
          break;
	  case SETTING_EMER_RECORD_SENSITIVITY:
	  case MSG_SET_EMER_RECORD_SENSITIVITY:
          menu_cfg_.record_emerrecordsen.current   =val;
          break;
	  case MSG_SET_PARKING_WARN_LAMP_SWITCH:
          menu_cfg_.record_parkingwarnlamp_switch.current   =val;
          break;
      case MSG_SET_PARKING_MONITORY:
          menu_cfg_.record_parkingmonitor_switch.current   =val;
          break;
	  case MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH:
          menu_cfg_.record_parkingabnormalmonitory_switch.current   =val;
          break;
	  case MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH:
          menu_cfg_.record_parkingloopabnormalnotice_switch.current   =val;
          break;
	  case MSG_SET_PARKING_RECORD_LOOP_SWITCH:
          menu_cfg_.record_parkingloop_switch.current   =val;
          break;
         case MSG_SET_PARKING_RECORD_LOOP_RESOLUTION:
          menu_cfg_.record_parkingloop_resolution.current   =val;
          break;
          case MSG_SET_RECORD_ENCODE_TYPE:
          menu_cfg_.record_encodingtype.current =val;
          break;
          case MSG_SET_RECORD_TIME:
          menu_cfg_.record_loop.current	=val;		
          break;
          case MSG_SET_RECORD_DELAY_TIME:
          menu_cfg_.record_timelapse.current	=val;	
          break;
          case MSG_SET_RECORD_SLOW_TIME:
          menu_cfg_.record_slowmotion.current =val;		
          break;
           /*****photo******/		
          case MSG_SET_PIC_RESOULATION:
          menu_cfg_.photo_resolution.current =val;		
          break;
          case MSG_SET_PIC_CONTINOUS:
          menu_cfg_.photo_dramashot.current	=val;	
          break;
          case MSG_SET_TIME_TAKE_PIC:
          menu_cfg_.photo_timed.current	=val;	
          break;
          case MSG_SET_AUTO_TIME_TAKE_PIC:
          menu_cfg_.photo_auto.current	=val;
          break;
		  case MSG_SET_PIC_QUALITY:
          menu_cfg_.photo_quality.current	=val;
          break;
            /*****camera******/
          case MSG_SET_CAMERA_EXPOSURE:
          menu_cfg_.camera_exposure.current=val;		
          break;		  
          case MSG_SET_CAMERA_WHITEBALANCE:
          menu_cfg_.camera_whitebalance.current=val;	
          break;		  
          case MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY:
          menu_cfg_.camera_lightfreq.current=val;		
          break;		
          case MSG_SET_AUTO_TIME_SCREENSAVER:
          menu_cfg_.camera_autoscreensaver.current=val;	
          break;
          case MSG_SET_AUTO_TIME_SHUTDOWN:
          menu_cfg_.camera_timedshutdown.current=val;
          break;	
            /*****device******/	 
          case MSG_RM_LANG_CHANGED:
          menu_cfg_.device_language.current=val;	
          break;	
	   case MSG_DEVICE_TIME:
          menu_cfg_.device_datatime.current=val;	
          break;	
          case SETTING_DEVICE_VOICESTATUS:
          menu_cfg_.device_voicestatus.current=val;				
          break;
          case MSG_SET_MENU_CONFIG_LUA:
          case MSG_GET_MENU_CONFIG_LUA:
              break;
		  case MSG_SET_GPS_SWITCH:
		  		db_error("set val: %d",val);
		  		menu_cfg_.device_gpswitch.current = val;
              break;
		  case MSG_SET_SPEEDUNIT:
		  		menu_cfg_.device_speedunit.current = val;
              break;
		  case MSG_SET_TIMEZONE:
		  		OldTimezone = menu_cfg_.device_timezone.current;
		  		menu_cfg_.device_timezone.current = val;
              break;
		  case MSG_SET_DEVICE_CARID:
		  		break;
          default:
          db_msg("[fangjj]:SetMenuIndexConfig:unhandled message: msg[%d], val[%d]", msg, val);
          break;
      }    
      SaveMenuAllConfig();
      return 0; 
} 

 int MenuConfigLua::GetMenuConfig(SunChipMenuConfig &resp)
{
    /****switch*****/
	resp.switch_record_eis = menu_cfg_.switch_record_eis;
	resp.switch_camera_imagerotation = menu_cfg_.switch_camera_imagerotation;
	resp.switch_camera_ledindicator = menu_cfg_.switch_camera_ledindicator;
	resp.switch_camera_timewatermark = menu_cfg_.switch_camera_timewatermark;
	resp.switch_camera_distortioncalibration = menu_cfg_.switch_camera_distortioncalibration;

	/*****record******/
	resp.record_resolution.current = menu_cfg_.record_resolution.current;
	resp.record_sound.current = menu_cfg_.record_sound.current;
	resp.record_switch_wifi.current = menu_cfg_.record_switch_wifi.current;
	resp.record_rear_resolution.current = menu_cfg_.record_rear_resolution.current;
	resp.record_screen_brightness.current = menu_cfg_.record_screen_brightness.current;
	resp.record_screen_disturb_mode.current = menu_cfg_.record_screen_disturb_mode.current;
	resp.record_voice_take_photo.current = menu_cfg_.record_voice_take_photo.current;
	resp.record_4g_network_switch.current = menu_cfg_.record_4g_network_switch.current;
	resp.record_volume_selection.current = menu_cfg_.record_volume_selection.current;
	resp.record_power_on_sound.current = menu_cfg_.record_power_on_sound.current;
	resp.record_key_sound.current = menu_cfg_.record_key_sound.current;
	resp.record_drivering_report.current = menu_cfg_.record_drivering_report.current;
	resp.record_adas.current = menu_cfg_.record_adas.current;
	resp.record_standby_clock.current = menu_cfg_.record_standby_clock.current;
	resp.record_adas_forward_collision_waring.current = menu_cfg_.record_adas_forward_collision_waring.current;
	resp.record_adas_lane_shift_reminding.current = menu_cfg_.record_adas_lane_shift_reminding.current;
	resp.record_watchdog.current = menu_cfg_.record_watchdog.current;
	resp.record_probeprompt.current = menu_cfg_.record_probeprompt.current;
	resp.record_speedprompt.current = menu_cfg_.record_speedprompt.current;
	resp.record_timewatermark.current = menu_cfg_.record_timewatermark.current;
	resp.record_emerrecord.current = menu_cfg_.record_emerrecord.current;
	resp.record_emerrecordsen.current = menu_cfg_.record_emerrecordsen.current;
	resp.record_parkingwarnlamp_switch.current = menu_cfg_.record_parkingwarnlamp_switch.current;
    resp.record_parkingmonitor_switch.current = menu_cfg_.record_parkingmonitor_switch.current;
	resp.record_parkingabnormalmonitory_switch.current = menu_cfg_.record_parkingabnormalmonitory_switch.current;
	resp.record_parkingloopabnormalnotice_switch.current = menu_cfg_.record_parkingloopabnormalnotice_switch.current;
	resp.record_parkingloop_switch.current = menu_cfg_.record_parkingloop_switch.current;
	resp.record_parkingloop_resolution.current = menu_cfg_.record_parkingloop_resolution.current;
	resp.record_encodingtype.current=menu_cfg_.record_encodingtype.current;
	resp.record_loop.current = menu_cfg_.record_loop.current;
	resp.record_timelapse.current = menu_cfg_.record_timelapse.current;
	resp.record_slowmotion.current = menu_cfg_.record_slowmotion.current;
	//resp.record_switch_acc.current = menu_cfg_.record_switch_acc.current;
	/*photo*/
	resp.photo_resolution.current = menu_cfg_.photo_resolution.current;
	resp.photo_dramashot.current = menu_cfg_.photo_dramashot.current;
	resp.photo_timed.current = menu_cfg_.photo_timed.current;
	resp.photo_auto.current = menu_cfg_.photo_auto.current;
	resp.photo_quality.current = menu_cfg_.photo_quality.current;	

	/*****camera****/
	resp.camera_exposure.current = menu_cfg_.camera_exposure.current;
	resp.camera_whitebalance.current = menu_cfg_.camera_whitebalance.current;
	resp.camera_lightfreq.current = menu_cfg_.camera_lightfreq.current;
	resp.camera_autoscreensaver.current = menu_cfg_.camera_autoscreensaver.current;
	resp.camera_timedshutdown.current = menu_cfg_.camera_timedshutdown.current;

	/******devices*******/
	resp.device_language.current = menu_cfg_.device_language.current;
	resp.device_datatime.current = menu_cfg_.device_datatime.current;
	resp.device_voicestatus.current = menu_cfg_.device_voicestatus.current;
	resp.device_gpswitch.current = menu_cfg_.device_gpswitch.current;
	resp.device_speedunit.current = menu_cfg_.device_speedunit.current;
	resp.device_timezone.current = menu_cfg_.device_timezone.current;
	/*********wifiinfo**********/

	strncpy(resp.camera_wifiinfo.string1, menu_cfg_.camera_wifiinfo.string1, sizeof(menu_cfg_.camera_wifiinfo.string1) - 1);
	strncpy(resp.camera_wifiinfo.string2, menu_cfg_.camera_wifiinfo.string2, sizeof(menu_cfg_.camera_wifiinfo.string2) - 1);
	/*****system version*******/
	strncpy(resp.device_sysversion.string1, menu_cfg_.device_sysversion.string1, sizeof(menu_cfg_.device_sysversion.string1) - 1);
	strncpy(resp.device_update.string1, menu_cfg_.device_update.string1, sizeof(menu_cfg_.device_update.string1) - 1);
	strncpy(resp.device_systemTime.string1, menu_cfg_.device_systemTime.string1, sizeof(menu_cfg_.device_systemTime.string1) - 1);
	strncpy(resp.device_carid.string1, menu_cfg_.device_carid.string1, sizeof(menu_cfg_.device_carid.string1) - 1);
    return 0;
}
int MenuConfigLua::GetMenuIndexConfig(int msg)
{
       int val=0;
       switch(msg) {
             /*****switch******/
          case SETTING_MOTION_DETECT:
          val=  menu_cfg_.switch_record_awmd;
          break;
//          case SETTING_RECORD_DRIVINGMODE:
//          val=  menu_cfg_.switch_record_drivingmode;
//          break;			
          case SETTING_CAMERA_IMAGEROTATION:
          val=  menu_cfg_.switch_camera_imagerotation;	
          break;
          case SETTING_CAMERA_LEDINDICATOR:
          val= menu_cfg_.switch_camera_ledindicator;
          break;		
          case SETTING_CAMERA_TIMEWATERMARK:
          val= menu_cfg_.switch_camera_timewatermark;	
          break;
		   //**** acc_resume *****//
          case SETTING_ACC_RESUME:
          val= menu_cfg_.switch_acc_resume;	
          break;
          case SETTING_CAMERA_DISTORTIONCALIBRATION:
          val=  menu_cfg_.switch_camera_distortioncalibration;
          break;
//          case SETTING_CAMERA_KEYTONE:
//          val=  menu_cfg_.switch_camera_keytone;		
//          break;	
           /*****record******/
		  case SETTING_CAMB_PREVIEWING:
		  	val = menu_cfg_.camB_preview;
		  	break;
          case SETTING_RECORD_RESOLUTION:
          val=  menu_cfg_.record_resolution.current;
          break;
	   case SETTING_REAR_RECORD_RESOLUTION:
          val=  menu_cfg_.record_rear_resolution.current;
          break;
	   case SETTING_SCREEN_BRIGHTNESS:
          val=  menu_cfg_.record_screen_brightness.current;
          break;
	   case SETTING_SCREEN_NOT_DISTURB_MODE:
          val=  menu_cfg_.record_screen_disturb_mode.current;
          break;
	   case SETTING_VOICE_TAKE_PHOTO:
		val = menu_cfg_.record_voice_take_photo.current;
	    break;
	  case SETTING_RECORD_VOLUME:
          val=  menu_cfg_.record_sound.current;
          break;
	   case SETTING_WIFI_SWITCH:
          val=  menu_cfg_.record_switch_wifi.current;		
          break;
	   case SETTING_4G_NET_WORK:
          val=  menu_cfg_.record_4g_network_switch.current;		
          break;
	   case SETTING_VOLUME_SELECTION:
		val = menu_cfg_.record_volume_selection.current;
	    break;
	   case SETTING_POWERON_SOUND_SWITCH:
		val = menu_cfg_.record_power_on_sound.current;
	    break;
	   case SETTING_KEY_SOUND_SWITCH:
		val = menu_cfg_.record_key_sound.current;
	    break;
	   case SETTING_ACC_SWITCH:
		val = menu_cfg_.record_switch_acc.current;
	    break;		
	   case SETTING_DRIVERING_REPORT_SWITCH:
		val = menu_cfg_.record_drivering_report.current;
	    break;
	   case SETTING_ADAS_SWITCH:
		val = menu_cfg_.record_adas.current;
	    break;
	  case SETTING_STANDBY_CLOCK:
		val = menu_cfg_.record_standby_clock.current;
	    break;
	  case SETTING_FORWARD_COLLISION_WARNING:
		val = menu_cfg_.record_adas_forward_collision_waring.current;
	    break;
	  case SETTING_LANE_SHIFT_REMINDING:
		val = menu_cfg_.record_adas_lane_shift_reminding.current;
	    break;
	   case SETTING_WATCH_DOG_SWITCH:
		val = menu_cfg_.record_watchdog.current;
	    break;
	  case SETTING_PROBE_PROMPT:
		val = menu_cfg_.record_probeprompt.current;
	    break;
	 case SETTING_SPEED_PROMPT:
		val = menu_cfg_.record_speedprompt.current;
	    break;
	   case SETTING_TIMEWATERMARK:
		val = menu_cfg_.record_timewatermark.current;
	    break;
	   case SETTING_EMER_RECORD_SWITCH:
		val = menu_cfg_.record_emerrecord.current;
	    break;
	case SETTING_EMER_RECORD_SENSITIVITY:
		val = menu_cfg_.record_emerrecordsen.current;
	    break;
	case SETTING_PARKING_WARN_LAMP_SWITCH:
		val = menu_cfg_.record_parkingwarnlamp_switch.current;
	    break;
    case SETTING_PARKING_MONITORY:
		val = menu_cfg_.record_parkingmonitor_switch.current;
	    break;
	case SETTING_PARKING_ABNORMAL_MONITORY_SWITCH:
		val = menu_cfg_.record_parkingabnormalmonitory_switch.current;
	    break;
	case SETTING_PARKING_ABNORMAL_NOTICE_SWITCH:
		val = menu_cfg_.record_parkingloopabnormalnotice_switch.current;
	    break;
	case SETTING_PARKING_RECORD_LOOP_SWITCH:
		val = menu_cfg_.record_parkingloop_switch.current;
	    break;
	case SETTING_PARKING_RECORD_LOOP_RESOLUTION:
		val = menu_cfg_.record_parkingloop_resolution.current;
	    break;
#ifdef ENABLE_ENC_TYPE_SELECT
          case SETTING_RECORD_ENCODINGTYPE:
          val=  menu_cfg_.record_encodingtype.current;
          break;
#endif
          case SETTING_RECORD_LOOP:
          val=  menu_cfg_.record_loop.current;		
          break;
          case SETTING_RECORD_TIMELAPSE:
          val=   menu_cfg_.record_timelapse.current;	
          break;
          case SETTING_RECORD_SLOWMOTION:
          val=   menu_cfg_.record_slowmotion.current;		
          break;
           /*****photo******/ 	
          case SETTING_PHOTO_RESOLUTION:
          val=  menu_cfg_.photo_resolution.current;		
          break;
          case SETTING_PHOTO_DRAMASHOT:
          val=  menu_cfg_.photo_dramashot.current;	
          break;
          case SETTING_PHOTO_TIMED:
          val=  menu_cfg_.photo_timed.current;	
          break;
          case SETTING_PHOTO_AUTO:
          val=  menu_cfg_.photo_auto.current;
          break;
          case SETTING_PHOTO_QUALITY:
          val=  menu_cfg_.photo_quality.current;
          break;		  
           /*****camera******/
          case SETTING_CAMERA_EXPOSURE:
          val=  menu_cfg_.camera_exposure.current;		
          break; 	  
          case SETTING_CAMERA_WHITEBALANCE:
          val=  menu_cfg_.camera_whitebalance.current;	
          break; 	  
          case SETTING_CAMERA_LIGHTSOURCEFREQUENCY:
          val=  menu_cfg_.camera_lightfreq.current; 	
          break; 	
          case SETTING_CAMERA_AUTOSCREENSAVER:
          val=  menu_cfg_.camera_autoscreensaver.current;	
          break;
          case SETTING_CAMERA_TIMEDSHUTDOWN:
          val=  menu_cfg_.camera_timedshutdown.current;
          break; 
           /*****device******/	 
          case SETTING_DEVICE_LANGUAGE:
          val=  menu_cfg_.device_language.current;	
          break; 	
	   case SETTING_DEVICE_TIME:
          val=  menu_cfg_.device_datatime.current;	
          break; 
          case SETTING_DEVICE_VOICESTATUS:
          val=  menu_cfg_.device_voicestatus.current;				
          break;
	    case MSG_SET_RECORD_VOLUME:
		{
			  val = menu_cfg_.record_sound.current;
			  db_warn(" menu_cfg_.record_sound.current %d",menu_cfg_.record_sound.current);
		}
          break;
		case SETTING_GPS_SWITCH:
			
          val=  menu_cfg_.device_gpswitch.current;				
		  //db_error("get val:%d", val);
          break;
		case SETTING_SPEED_UNIT:
          val=  menu_cfg_.device_speedunit.current;				
          break;
		case SETTING_TIMEZONE:
          val=  menu_cfg_.device_timezone.current;				
          break;
		case SETTING_DEVICE_CARID:
			db_error("todo: SETTING_DEVICE_CARID");
			break;
          default:
          db_msg("[fangjj]:GetMenuIndexConfig:unhandled message: msg[%d], val[%d]", msg, val);
          break;
	}
         // db_msg("[fangjj]:GetMenuIndexConfig:msg[%d], val[%d]", msg, val);
	return val;
		
}	 

int MenuConfigLua::ResetMenuConfig(void)
{

    system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");

    db_msg("[fangjj]:config file %s  reset, copy default from /usr/share/app/sdv", MENU_CONFIG_FILE);
    int ret = LoadMenuConfig();
    if (ret) {
        db_msg("[fangjj]:[FUN]:%s [LINE]:%d  Do Reset LoadMenuConfig fail:%d !\n", __func__, __LINE__, ret);
    }

    return 0;
}
bool MenuConfigLua::IsVersionSame(std::string external_version,std::string local_version)
{
		//V-1.00.15d26_CN_debug
		//V-1.00.15d26_CN
	if(external_version.empty() || local_version.empty())
	{
		db_error("%s  external_version or local_version is empty",__func__);
		return false;
	}
	db_warn("IsNewVersion    external_version : %s  ---  local_version : %s",external_version.c_str(),local_version.c_str());
	//pars external_version
	string::size_type e_rc_start = external_version.rfind("V-");
	if( e_rc_start == string::npos)
	{
		db_warn("invalid fileName:%s",external_version.c_str());
		return false;
	}
	string::size_type e_rc_end = external_version.rfind("d26");
	if( e_rc_end == string::npos)
	{
		db_warn("invalid fileName:%s",external_version.c_str());
		return false;
	}
	string e_str = external_version.substr(e_rc_start+2,e_rc_end-(e_rc_start+2) );
	
	db_warn("pars external_version : %s",e_str.c_str());
	int e_num_first = 0, e_num_second = 0, e_num_third = 0;
	sscanf(e_str.c_str(),"%i.%i.%i",&e_num_first,&e_num_second,&e_num_third);
	db_warn("pars external version value : %d  %2d  %2d",e_num_first,e_num_second,e_num_third);
	
	//pars local version
	string::size_type local_rc_start = local_version.rfind("V-");
	if( local_rc_start == string::npos)
	{
		db_warn("invalid fileName:%s",local_version.c_str());
		return false;
	}
	string::size_type local_rc_end = local_version.rfind("d26");
	if( local_rc_end == string::npos)
	{
		db_warn("invalid fileName:%s",local_version.c_str());
		return false;
	}
	string local_str = local_version.substr(local_rc_start+2,local_rc_end-(local_rc_start+2) );

	db_warn("pars local_version : %s",local_str.c_str());
	int l_num_first = 0, l_num_second = 0, l_num_third = 0;
	sscanf(local_str.c_str(),"%i.%i.%i",&l_num_first,&l_num_second,&l_num_third);
	db_warn("pars local version value : %d  %2d  %2d",l_num_first,l_num_second,l_num_third);

	//cmp the version 
	if(e_num_first != l_num_first)
	{
		return false;
	}
	else //e_num_first == l_num_first
	{	 if(e_num_second !=  l_num_second)
		{
			return false;
		}
		else //e_num_second ==  l_num_second
		{
			if(e_num_third != l_num_third)
			{
				return false;
			}
			else //e_num_third == l_num_third
			{
				return true;
			}
		}

	}
	
	return true;
}

int MenuConfigLua::ChangeMenuConfig()
{
     db_warn("ready to ChangeMenuConfig-----------------------");
     LuaConfig usr_luacfg, data_luacfg;
	char temp[128]={0};
	int ret = -1;
	// 0. 检查是否存在原始的 "/usr/share/app/sdv/menu_config.lua"
	 if (!FILE_EXIST(MENU_CONFIG_FILE_USR)) 		// "/usr/share/app/sdv/menu_config.lua" 原始的lua
	 {
        db_warn("config file %s not exist", MENU_CONFIG_FILE_USR);
		return -1;
     }
	 // 1. 存在则 复制成 "/usr/share/app/sdv/menu_config_temp.lua"
	 // cp -f "/usr/share/app/sdv/menu_config.lua" "/usr/share/app/sdv/menu_config_temp.lua"
	 snprintf(temp,sizeof(temp),"cp -f %s %s",MENU_CONFIG_FILE_USR,MENU_CONFIG_FILE_USR_TEMP);
	 system(temp);
	
	 if (!FILE_EXIST(MENU_CONFIG_FILE_USR_TEMP)) // "/usr/share/app/sdv/menu_config_temp.lua"
	 {
        db_warn("config file %s not exist", MENU_CONFIG_FILE_USR_TEMP);
		return -1;
     }
	// 3. 加载"/usr/share/app/sdv/menu_config_temp.lua" 文件
	ret = usr_luacfg.LoadFromFile(MENU_CONFIG_FILE_USR_TEMP);
	if (ret)
	{
	    db_warn("Load %s failed", MENU_CONFIG_FILE_USR_TEMP);
	    //remove the menu_config_temp.lua
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
		system(temp);
	   	return -1;
	}

	std::string usr_update_str = usr_luacfg.GetStringValue("menu.device.updatemenu.update");
	std::string usr_ver_str = usr_luacfg.GetStringValue("menu.device.sysversion.version");
	db_warn("usr_update_str = %s   usr_ver_str = %s",usr_update_str.c_str(),usr_ver_str.c_str());
	// 4. 检查是否存在 "/tmp/data/menu_config.lua", 不存在则 复制原始的 "/usr/share/app/sdv/menu_config.lua"
	 if (!FILE_EXIST(MENU_CONFIG_FILE)) 
	 {
        db_warn("config file %s not exist, copy from data", MENU_CONFIG_FILE);
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"cp -f %s /tmp/data/",MENU_CONFIG_FILE_USR);
		system(temp);
		//remove the menu_config_temp.lua
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
		system(temp);
		return 0;
        }
	// 5. 加载 "/tmp/data/menu_config.lua"
	ret = data_luacfg.LoadFromFile(MENU_CONFIG_FILE);
	if (ret)
	{
		db_warn(" Load %s failed , ready to cp /usr/share/app/sdv/menu_config.lua /tmp/data/", MENU_CONFIG_FILE);
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"cp -f %s /tmp/data/",MENU_CONFIG_FILE_USR);
		system(temp);
		//remove the menu_config_temp.lua
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
		system(temp);
	    	return 0;
	}
	// 6.检查版本信息
	std::string data_ver_str = data_luacfg.GetStringValue("menu.device.sysversion.version");
	if(IsVersionSame(usr_ver_str,data_ver_str) == true)
	{
		db_warn("version is the same , no to do anything");
		//remove the menu_config_temp.lua
		memset(temp,0,sizeof(temp));
		snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
		system(temp);
		return 0;
	}
	else
	{	// 版本不同则覆盖/tmp/data/menu_config.lua
		if( strcmp(usr_update_str.c_str(), "true") == 0 )
		{
			db_warn("update is true ,need to force cover the /tmp/data/menu_config.lua");
			memset(temp,0,sizeof(temp));
			snprintf(temp,sizeof(temp),"cp -f %s /tmp/data/",MENU_CONFIG_FILE_USR);
			system(temp);
			//remove the menu_config_temp.lua
			memset(temp,0,sizeof(temp));
			snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
			system(temp);
			return 0;
		}
		else
		{
			#if 0
			db_warn("cover usr version to data version ");
			data_luacfg.SetStringValue("menu.device.sysversion.version",usr_ver_str);
			ret = data_luacfg.SyncConfigToFile(MENU_CONFIG_FILE, "menu");
			if (ret < 0) 
			{
				db_error("Do SyncConfigToFile error! file:%s \n", MENU_CONFIG_FILE);
				return -1;
			}
			#else
			if( ComparisonProfile(&usr_luacfg ,&data_luacfg) < 0)
			{
				db_error("error: ComparisonProfile failed");
				return -1;
			}
			//mv menu_config_temp.lua to data/menu_config.lua
			memset(temp,0,sizeof(temp));
			snprintf(temp,sizeof(temp),"mv %s %s",MENU_CONFIG_FILE_USR_TEMP,MENU_CONFIG_FILE);
			system(temp);
			sync();
			return 0;
			#endif

		}		
	}
	//remove the menu_config_temp.lua
	memset(temp,0,sizeof(temp));
	snprintf(temp,sizeof(temp),"rm -f %s",MENU_CONFIG_FILE_USR_TEMP);
	system(temp);
	sync();
	return 0;
}

/******************************************************************
*function : ComparisonProfile(LuaConfig * usr_luacfg , LuaConfig * data_luacfg)
*return : -1 失败, 0 成功
*比较规则如下:
*I.如果usr 下面的count 大于data 下面的current ,
*   就有两种情况:
*	1.usr and data count 相等 ,这种情况直接使用data current.
*	2.usr count > data count ,这种情况也是直接使用data current
*
*II.如果usr count <= data count,这种情况直接使用usr current
*注意事项:
*	如果有添加子选项，建议添加到后面；而且不
*	能顺便调换自选项的顺序
********************************************************************/

int MenuConfigLua::ComparisonProfile(LuaConfig * usr_luacfg , LuaConfig * data_luacfg)
{
	if(usr_luacfg == NULL || data_luacfg == NULL)
	{
		db_error("error: ComparisonProfile usr_luacfg = %p",usr_luacfg,data_luacfg);
		return -1;
	}
	
	string str;
	int ret = 0;
	//read the usr_luacfg
	db_warn("ready to read out the data menu_config.lua data");
	/*****switch******/
	usr_temp_menu_cfg_.switch_record_eis = usr_luacfg->GetIntegerValue("menu.switch.record_eis");
	usr_temp_menu_cfg_.switch_record_awmd =  usr_luacfg->GetIntegerValue("menu.switch.record_awmd"); 
	usr_temp_menu_cfg_.switch_record_drivingmode =  usr_luacfg->GetIntegerValue("menu.switch.record_drivingmode");
	usr_temp_menu_cfg_.switch_camera_imagerotation  =   usr_luacfg->GetIntegerValue("menu.switch.camera_imagerotation");
	usr_temp_menu_cfg_.switch_camera_ledindicator   =  usr_luacfg->GetIntegerValue("menu.switch.camera_ledindicator");
	usr_temp_menu_cfg_.switch_camera_timewatermark  =    usr_luacfg->GetIntegerValue("menu.switch.camera_timewatermark");
	usr_temp_menu_cfg_.switch_camera_distortioncalibration =    usr_luacfg->GetIntegerValue("menu.switch.camera_distortioncalibration");
	usr_temp_menu_cfg_.switch_camera_keytone   = usr_luacfg->GetIntegerValue("menu.switch.camera_keytone");
	usr_temp_menu_cfg_.switch_acc_resume   = usr_luacfg->GetIntegerValue("menu.switch.acc_resume");
	usr_temp_menu_cfg_.camB_preview = usr_luacfg->GetIntegerValue("menu.switch.camB_preview");
	/*****record******/
	usr_temp_menu_cfg_.record_resolution.current  =    usr_luacfg->GetIntegerValue("menu.record.resolution.current");
	usr_temp_menu_cfg_.record_resolution.count  =    usr_luacfg->GetIntegerValue("menu.record.resolution.count");
	usr_temp_menu_cfg_.record_sound.current  =    usr_luacfg->GetIntegerValue("menu.record.record_sound.current");
	usr_temp_menu_cfg_.record_sound.count  =    usr_luacfg->GetIntegerValue("menu.record.record_sound.count");
	usr_temp_menu_cfg_.record_switch_wifi.current  =    usr_luacfg->GetIntegerValue("menu.record.wifisw.current");
	usr_temp_menu_cfg_.record_switch_wifi.count  =    usr_luacfg->GetIntegerValue("menu.record.wifisw.count");
	usr_temp_menu_cfg_.record_rear_resolution.current  =    usr_luacfg->GetIntegerValue("menu.record.rear_resolution.current");
	usr_temp_menu_cfg_.record_rear_resolution.count  =    usr_luacfg->GetIntegerValue("menu.record.rear_resolution.count");
	usr_temp_menu_cfg_.record_screen_brightness.current  =    usr_luacfg->GetIntegerValue("menu.record.screenbrightness.current");
	usr_temp_menu_cfg_.record_screen_brightness.count  =    usr_luacfg->GetIntegerValue("menu.record.screenbrightness.count");
	usr_temp_menu_cfg_.record_screen_disturb_mode.current  =    usr_luacfg->GetIntegerValue("menu.record.disturbmode.current");
	usr_temp_menu_cfg_.record_screen_disturb_mode.count  =    usr_luacfg->GetIntegerValue("menu.record.disturbmode.count");
	usr_temp_menu_cfg_.record_voice_take_photo.current  =    usr_luacfg->GetIntegerValue("menu.record.voicephoto.current");
	usr_temp_menu_cfg_.record_voice_take_photo.count  =    usr_luacfg->GetIntegerValue("menu.record.voicephoto.count");
	usr_temp_menu_cfg_.record_4g_network_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.network4g.current");
	usr_temp_menu_cfg_.record_4g_network_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.network4g.count");
	usr_temp_menu_cfg_.record_volume_selection.current  =    usr_luacfg->GetIntegerValue("menu.record.volumeselect.current");
	usr_temp_menu_cfg_.record_volume_selection.count  =    usr_luacfg->GetIntegerValue("menu.record.volumeselect.count");
	usr_temp_menu_cfg_.record_power_on_sound.current  =    usr_luacfg->GetIntegerValue("menu.record.poweronvoice.current");
	usr_temp_menu_cfg_.record_power_on_sound.count  =    usr_luacfg->GetIntegerValue("menu.record.poweronvoice.count");
	usr_temp_menu_cfg_.record_key_sound.current  =    usr_luacfg->GetIntegerValue("menu.record.keyvoice.current");
	usr_temp_menu_cfg_.record_key_sound.count  =    usr_luacfg->GetIntegerValue("menu.record.keyvoice.count");
	usr_temp_menu_cfg_.record_drivering_report.current  =    usr_luacfg->GetIntegerValue("menu.record.drivereport.current");
	usr_temp_menu_cfg_.record_drivering_report.count  =    usr_luacfg->GetIntegerValue("menu.record.drivereport.count");
	usr_temp_menu_cfg_.record_adas.current  =    usr_luacfg->GetIntegerValue("menu.record.adas.current");
	usr_temp_menu_cfg_.record_adas.count  =    usr_luacfg->GetIntegerValue("menu.record.adas.count");
	usr_temp_menu_cfg_.record_standby_clock.current  =    usr_luacfg->GetIntegerValue("menu.record.standbyclock.current");
	usr_temp_menu_cfg_.record_standby_clock.count  =    usr_luacfg->GetIntegerValue("menu.record.standbyclock.count");
	usr_temp_menu_cfg_.record_adas_forward_collision_waring.current  =    usr_luacfg->GetIntegerValue("menu.record.adasfcw.current");
	usr_temp_menu_cfg_.record_adas_forward_collision_waring.count  =    usr_luacfg->GetIntegerValue("menu.record.adasfcw.count");
	usr_temp_menu_cfg_.record_adas_lane_shift_reminding.current  =    usr_luacfg->GetIntegerValue("menu.record.adaslsr.current");
	usr_temp_menu_cfg_.record_adas_lane_shift_reminding.count  =    usr_luacfg->GetIntegerValue("menu.record.adaslsr.count");
	usr_temp_menu_cfg_.record_watchdog.current  =    usr_luacfg->GetIntegerValue("menu.record.watchdog.current");
	usr_temp_menu_cfg_.record_watchdog.count  =    usr_luacfg->GetIntegerValue("menu.record.watchdog.count");
	usr_temp_menu_cfg_.record_probeprompt.current  =    usr_luacfg->GetIntegerValue("menu.record.probeprompt.current");
	usr_temp_menu_cfg_.record_probeprompt.count  =    usr_luacfg->GetIntegerValue("menu.record.probeprompt.count");
	usr_temp_menu_cfg_.record_speedprompt.current  =    usr_luacfg->GetIntegerValue("menu.record.speedprompt.current");
	usr_temp_menu_cfg_.record_speedprompt.count  =    usr_luacfg->GetIntegerValue("menu.record.speedprompt.count");
	usr_temp_menu_cfg_.record_timewatermark.current  =    usr_luacfg->GetIntegerValue("menu.record.timewatermark.current");
	usr_temp_menu_cfg_.record_timewatermark.count  =    usr_luacfg->GetIntegerValue("menu.record.timewatermark.count");
	usr_temp_menu_cfg_.record_emerrecord.current  =    usr_luacfg->GetIntegerValue("menu.record.emerrecord.current");
	usr_temp_menu_cfg_.record_emerrecord.count  =    usr_luacfg->GetIntegerValue("menu.record.emerrecord.count");
	usr_temp_menu_cfg_.record_emerrecordsen.current  =    usr_luacfg->GetIntegerValue("menu.record.emerrecordsen.current");
	usr_temp_menu_cfg_.record_emerrecordsen.count  =    usr_luacfg->GetIntegerValue("menu.record.emerrecordsen.count");
	usr_temp_menu_cfg_.record_parkingwarnlamp_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingwarnlamp.current");
	usr_temp_menu_cfg_.record_parkingwarnlamp_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingwarnlamp.count");
    usr_temp_menu_cfg_.record_parkingmonitor_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingmonitor.current");
	usr_temp_menu_cfg_.record_parkingmonitor_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingmonitor.count");
	usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingabnormalmonitor.current");
	usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingabnormalmonitor.count");
	usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingabnormalnotice.current");
	usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingabnormalnotice.count");
	usr_temp_menu_cfg_.record_parkingloop_switch.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingloopsw.current");
	usr_temp_menu_cfg_.record_parkingloop_switch.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingloopsw.count");
	usr_temp_menu_cfg_.record_parkingloop_resolution.current  =    usr_luacfg->GetIntegerValue("menu.record.parkingloopresolution.current");
	usr_temp_menu_cfg_.record_parkingloop_resolution.count  =    usr_luacfg->GetIntegerValue("menu.record.parkingloopresolution.count");
	usr_temp_menu_cfg_.record_encodingtype.current  =    usr_luacfg->GetIntegerValue("menu.record.encodingtype.current");
	usr_temp_menu_cfg_.record_encodingtype.count  =    usr_luacfg->GetIntegerValue("menu.record.encodingtype.count");
	usr_temp_menu_cfg_.record_loop.current  =    usr_luacfg->GetIntegerValue("menu.record.loop.current");
	usr_temp_menu_cfg_.record_loop.count   =   usr_luacfg->GetIntegerValue("menu.record.loop.count");
	usr_temp_menu_cfg_.record_timelapse.current  =    usr_luacfg->GetIntegerValue("menu.record.timelapse.current");
	usr_temp_menu_cfg_.record_timelapse.count   =   usr_luacfg->GetIntegerValue("menu.record.timelapse.count");
	usr_temp_menu_cfg_.record_slowmotion.current   =   usr_luacfg->GetIntegerValue("menu.record.slowmotion.current");
	usr_temp_menu_cfg_.record_slowmotion.count   =   usr_luacfg->GetIntegerValue("menu.record.slowmotion.count");
	usr_temp_menu_cfg_.record_switch_acc.current  =    usr_luacfg->GetIntegerValue("menu.record.acc.current");
	usr_temp_menu_cfg_.record_switch_acc.count  =    usr_luacfg->GetIntegerValue("menu.record.acc.count");     
	/*****photo******/
	usr_temp_menu_cfg_.photo_resolution.current =     usr_luacfg->GetIntegerValue("menu.photo.resolution.current");
	usr_temp_menu_cfg_.photo_resolution.count  =    usr_luacfg->GetIntegerValue("menu.photo.resolution.count");
	usr_temp_menu_cfg_.photo_timed.current   =   usr_luacfg->GetIntegerValue("menu.photo.timed.current");
	usr_temp_menu_cfg_.photo_timed.count    =  usr_luacfg->GetIntegerValue("menu.photo.timed.count");
	usr_temp_menu_cfg_.photo_auto.current   =   usr_luacfg->GetIntegerValue("menu.photo.auto.current");
	usr_temp_menu_cfg_.photo_auto.count  =    usr_luacfg->GetIntegerValue("menu.photo.auto.count");
	usr_temp_menu_cfg_.photo_dramashot.current  =    usr_luacfg->GetIntegerValue("menu.photo.dramashot.current");
	usr_temp_menu_cfg_.photo_dramashot.count  =    usr_luacfg->GetIntegerValue("menu.photo.dramashot.count");
	usr_temp_menu_cfg_.photo_quality.current  =    usr_luacfg->GetIntegerValue("menu.photo.quality.current");
	usr_temp_menu_cfg_.photo_quality.count  =    usr_luacfg->GetIntegerValue("menu.photo.quality.count");

	/*****camera******/
	usr_temp_menu_cfg_.camera_exposure.current   =   usr_luacfg->GetIntegerValue("menu.camera.exposure.current");
	usr_temp_menu_cfg_.camera_exposure.count    =  usr_luacfg->GetIntegerValue("menu.camera.exposure.count");
	usr_temp_menu_cfg_.camera_whitebalance.current  =    usr_luacfg->GetIntegerValue("menu.camera.whitebalance.current");
	usr_temp_menu_cfg_.camera_whitebalance.count  =    usr_luacfg->GetIntegerValue("menu.camera.whitebalance.count"); 
	usr_temp_menu_cfg_.camera_lightfreq.current   =   usr_luacfg->GetIntegerValue("menu.camera.lightfreq.current");
	usr_temp_menu_cfg_.camera_lightfreq.count   =   usr_luacfg->GetIntegerValue("menu.camera.lightfreq.count"); 
	usr_temp_menu_cfg_.camera_autoscreensaver.current   =   usr_luacfg->GetIntegerValue("menu.camera.autoscreensaver.current");
	usr_temp_menu_cfg_.camera_autoscreensaver.count    =  usr_luacfg->GetIntegerValue("menu.camera.autoscreensaver.count");
	usr_temp_menu_cfg_.camera_timedshutdown.current   =   usr_luacfg->GetIntegerValue("menu.camera.timedshutdown.current");
	usr_temp_menu_cfg_.camera_timedshutdown.count   =   usr_luacfg->GetIntegerValue("menu.camera.timedshutdown.count");

	str     =     usr_luacfg->GetStringValue("menu.camera.wifiinfo.ssid");
	strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string1) - 1);
	str     =     usr_luacfg->GetStringValue("menu.camera.wifiinfo.password"); 
	strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string2) - 1);
	str     =     usr_luacfg->GetStringValue("menu.device.sysversion.version");
	strncpy(usr_temp_menu_cfg_.device_sysversion.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_sysversion.string1) - 1); 
	str     =     usr_luacfg->GetStringValue("menu.device.updatemenu.update");
	strncpy(usr_temp_menu_cfg_.device_update.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_update.string1) - 1); 
	str     =     usr_luacfg->GetStringValue("menu.device.systemTimeSave.timeStr");
	strncpy(usr_temp_menu_cfg_.device_systemTime.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_systemTime.string1) - 1); 
	
	/*****device******/
	usr_temp_menu_cfg_.device_language.current  =    usr_luacfg->GetIntegerValue("menu.device.language.current");
	usr_temp_menu_cfg_.device_language.count  =    usr_luacfg->GetIntegerValue("menu.device.language.count");
	usr_temp_menu_cfg_.device_datatime.current  =    usr_luacfg->GetIntegerValue("menu.device.updatetime.current");
	usr_temp_menu_cfg_.device_datatime.count  =    usr_luacfg->GetIntegerValue("menu.device.updatetime.count");
	usr_temp_menu_cfg_.device_voicestatus.current   =   usr_luacfg->GetIntegerValue("menu.device.voicestatus.current");
	usr_temp_menu_cfg_.device_voicestatus.count   =   usr_luacfg->GetIntegerValue("menu.device.voicestatus.count");
	usr_temp_menu_cfg_.device_gpswitch.current   =   usr_luacfg->GetIntegerValue("menu.device.gpsswitch.current");
	usr_temp_menu_cfg_.device_gpswitch.count   =   usr_luacfg->GetIntegerValue("menu.device.gpsswitch.count");
	usr_temp_menu_cfg_.device_speedunit.current   =   usr_luacfg->GetIntegerValue("menu.device.speedunit.current");
	usr_temp_menu_cfg_.device_speedunit.count   =   usr_luacfg->GetIntegerValue("menu.device.speedunit.count");
	usr_temp_menu_cfg_.device_timezone.current   =   usr_luacfg->GetIntegerValue("menu.device.timezone.current");
	usr_temp_menu_cfg_.device_timezone.count   =   usr_luacfg->GetIntegerValue("menu.device.timezone.count");
	str     =     usr_luacfg->GetStringValue("menu.device.carid.idstr");
	strncpy(usr_temp_menu_cfg_.device_carid.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_carid.string1) - 1); 

/*----------------------------------------------------------------------------------------------------------*/
	//read the data_luacfg
	db_warn("ready to read out the data menu_config.lua data");
	/*****switch******/
	data_menu_cfg_.switch_record_eis = data_luacfg->GetIntegerValue("menu.switch.record_eis");
	data_menu_cfg_.switch_record_awmd =  data_luacfg->GetIntegerValue("menu.switch.record_awmd"); 
	data_menu_cfg_.switch_record_drivingmode =  data_luacfg->GetIntegerValue("menu.switch.record_drivingmode");
	data_menu_cfg_.switch_camera_imagerotation  =   data_luacfg->GetIntegerValue("menu.switch.camera_imagerotation");
	data_menu_cfg_.switch_camera_ledindicator   =  data_luacfg->GetIntegerValue("menu.switch.camera_ledindicator");
	data_menu_cfg_.switch_camera_timewatermark  =    data_luacfg->GetIntegerValue("menu.switch.camera_timewatermark");
	data_menu_cfg_.switch_camera_distortioncalibration =    data_luacfg->GetIntegerValue("menu.switch.camera_distortioncalibration");
	data_menu_cfg_.switch_camera_keytone   = data_luacfg->GetIntegerValue("menu.switch.camera_keytone");
	data_menu_cfg_.switch_acc_resume   = data_luacfg->GetIntegerValue("menu.switch.acc_resume");
	data_menu_cfg_.camB_preview = data_luacfg->GetIntegerValue("menu.switch.camB_preview");

	/*****record******/
	data_menu_cfg_.record_resolution.current  =    data_luacfg->GetIntegerValue("menu.record.resolution.current");
	data_menu_cfg_.record_resolution.count  =    data_luacfg->GetIntegerValue("menu.record.resolution.count");
	data_menu_cfg_.record_sound.current  =    data_luacfg->GetIntegerValue("menu.record.record_sound.current");
	data_menu_cfg_.record_sound.count  =    data_luacfg->GetIntegerValue("menu.record.record_sound.count");
	data_menu_cfg_.record_switch_wifi.current  =    data_luacfg->GetIntegerValue("menu.record.wifisw.current");
	data_menu_cfg_.record_switch_wifi.count  =    data_luacfg->GetIntegerValue("menu.record.wifisw.count");
	data_menu_cfg_.record_rear_resolution.current  =    data_luacfg->GetIntegerValue("menu.record.rear_resolution.current");
	data_menu_cfg_.record_rear_resolution.count  =    data_luacfg->GetIntegerValue("menu.record.rear_resolution.count");
	data_menu_cfg_.record_screen_brightness.current  =    data_luacfg->GetIntegerValue("menu.record.screenbrightness.current");
	data_menu_cfg_.record_screen_brightness.count  =    data_luacfg->GetIntegerValue("menu.record.screenbrightness.count");
	data_menu_cfg_.record_screen_disturb_mode.current  =    data_luacfg->GetIntegerValue("menu.record.disturbmode.current");
	data_menu_cfg_.record_screen_disturb_mode.count  =    data_luacfg->GetIntegerValue("menu.record.disturbmode.count");
	data_menu_cfg_.record_voice_take_photo.current  =    data_luacfg->GetIntegerValue("menu.record.voicephoto.current");
	data_menu_cfg_.record_voice_take_photo.count  =    data_luacfg->GetIntegerValue("menu.record.voicephoto.count");
	data_menu_cfg_.record_4g_network_switch.current  =    data_luacfg->GetIntegerValue("menu.record.network4g.current");
	data_menu_cfg_.record_4g_network_switch.count  =    data_luacfg->GetIntegerValue("menu.record.network4g.count");
	data_menu_cfg_.record_volume_selection.current  =    data_luacfg->GetIntegerValue("menu.record.volumeselect.current");
	data_menu_cfg_.record_volume_selection.count  =    data_luacfg->GetIntegerValue("menu.record.volumeselect.count");
	data_menu_cfg_.record_power_on_sound.current  =    data_luacfg->GetIntegerValue("menu.record.poweronvoice.current");
	data_menu_cfg_.record_power_on_sound.count  =    data_luacfg->GetIntegerValue("menu.record.poweronvoice.count");
	data_menu_cfg_.record_key_sound.current  =    data_luacfg->GetIntegerValue("menu.record.keyvoice.current");
	data_menu_cfg_.record_key_sound.count  =    data_luacfg->GetIntegerValue("menu.record.keyvoice.count");
	data_menu_cfg_.record_drivering_report.current  =    data_luacfg->GetIntegerValue("menu.record.drivereport.current");
	data_menu_cfg_.record_drivering_report.count  =    data_luacfg->GetIntegerValue("menu.record.drivereport.count");
	data_menu_cfg_.record_adas.current  =    data_luacfg->GetIntegerValue("menu.record.adas.current");
	data_menu_cfg_.record_adas.count  =    data_luacfg->GetIntegerValue("menu.record.adas.count");
	data_menu_cfg_.record_standby_clock.current  =    data_luacfg->GetIntegerValue("menu.record.standbyclock.current");
	data_menu_cfg_.record_standby_clock.count  =    data_luacfg->GetIntegerValue("menu.record.standbyclock.count");
	data_menu_cfg_.record_adas_forward_collision_waring.current  =    data_luacfg->GetIntegerValue("menu.record.adasfcw.current");
	data_menu_cfg_.record_adas_forward_collision_waring.count  =    data_luacfg->GetIntegerValue("menu.record.adasfcw.count");
	data_menu_cfg_.record_adas_lane_shift_reminding.current  =    data_luacfg->GetIntegerValue("menu.record.adaslsr.current");
	data_menu_cfg_.record_adas_lane_shift_reminding.count  =    data_luacfg->GetIntegerValue("menu.record.adaslsr.count");
	data_menu_cfg_.record_watchdog.current  =    data_luacfg->GetIntegerValue("menu.record.watchdog.current");
	data_menu_cfg_.record_watchdog.count  =    data_luacfg->GetIntegerValue("menu.record.watchdog.count");
	data_menu_cfg_.record_probeprompt.current  =    data_luacfg->GetIntegerValue("menu.record.probeprompt.current");
	data_menu_cfg_.record_probeprompt.count  =    data_luacfg->GetIntegerValue("menu.record.probeprompt.count");
	data_menu_cfg_.record_speedprompt.current  =    data_luacfg->GetIntegerValue("menu.record.speedprompt.current");
	data_menu_cfg_.record_speedprompt.count  =    data_luacfg->GetIntegerValue("menu.record.speedprompt.count");
	data_menu_cfg_.record_timewatermark.current  =    data_luacfg->GetIntegerValue("menu.record.timewatermark.current");
	data_menu_cfg_.record_timewatermark.count  =    data_luacfg->GetIntegerValue("menu.record.timewatermark.count");
	data_menu_cfg_.record_emerrecord.current  =    data_luacfg->GetIntegerValue("menu.record.emerrecord.current");
	data_menu_cfg_.record_emerrecord.count  =    data_luacfg->GetIntegerValue("menu.record.emerrecord.count");
	data_menu_cfg_.record_emerrecordsen.current  =    data_luacfg->GetIntegerValue("menu.record.emerrecordsen.current");
	data_menu_cfg_.record_emerrecordsen.count  =    data_luacfg->GetIntegerValue("menu.record.emerrecordsen.count");
	data_menu_cfg_.record_parkingwarnlamp_switch.current  =    data_luacfg->GetIntegerValue("menu.record.parkingwarnlamp.current");
	data_menu_cfg_.record_parkingwarnlamp_switch.count  =    data_luacfg->GetIntegerValue("menu.record.parkingwarnlamp.count");
    data_menu_cfg_.record_parkingmonitor_switch.current  =    data_luacfg->GetIntegerValue("menu.record.parkingmonitor.current");
	data_menu_cfg_.record_parkingmonitor_switch.count  =    data_luacfg->GetIntegerValue("menu.record.parkingmonitor.count");
	data_menu_cfg_.record_parkingabnormalmonitory_switch.current  =    data_luacfg->GetIntegerValue("menu.record.parkingabnormalmonitor.current");
	data_menu_cfg_.record_parkingabnormalmonitory_switch.count  =    data_luacfg->GetIntegerValue("menu.record.parkingabnormalmonitor.count");
	data_menu_cfg_.record_parkingloopabnormalnotice_switch.current  =    data_luacfg->GetIntegerValue("menu.record.parkingabnormalnotice.current");
	data_menu_cfg_.record_parkingloopabnormalnotice_switch.count  =    data_luacfg->GetIntegerValue("menu.record.parkingabnormalnotice.count");
	data_menu_cfg_.record_parkingloop_switch.current  =    data_luacfg->GetIntegerValue("menu.record.parkingloopsw.current");
	data_menu_cfg_.record_parkingloop_switch.count  =    data_luacfg->GetIntegerValue("menu.record.parkingloopsw.count");
	data_menu_cfg_.record_parkingloop_resolution.current  =    data_luacfg->GetIntegerValue("menu.record.parkingloopresolution.current");
	data_menu_cfg_.record_parkingloop_resolution.count  =    data_luacfg->GetIntegerValue("menu.record.parkingloopresolution.count");
	data_menu_cfg_.record_encodingtype.current  =    data_luacfg->GetIntegerValue("menu.record.encodingtype.current");
	data_menu_cfg_.record_encodingtype.count  =    data_luacfg->GetIntegerValue("menu.record.encodingtype.count");
	data_menu_cfg_.record_loop.current  =    data_luacfg->GetIntegerValue("menu.record.loop.current");
	data_menu_cfg_.record_loop.count   =   data_luacfg->GetIntegerValue("menu.record.loop.count");
	data_menu_cfg_.record_timelapse.current  =    data_luacfg->GetIntegerValue("menu.record.timelapse.current");
	data_menu_cfg_.record_timelapse.count   =   data_luacfg->GetIntegerValue("menu.record.timelapse.count");
	data_menu_cfg_.record_slowmotion.current   =   data_luacfg->GetIntegerValue("menu.record.slowmotion.current");
	data_menu_cfg_.record_slowmotion.count   =   data_luacfg->GetIntegerValue("menu.record.slowmotion.count");
	data_menu_cfg_.record_switch_acc.current  =    data_luacfg->GetIntegerValue("menu.record.acc.current");
	data_menu_cfg_.record_switch_acc.count  =    data_luacfg->GetIntegerValue("menu.record.acc.count");     
	/*****photo******/
	data_menu_cfg_.photo_resolution.current =     data_luacfg->GetIntegerValue("menu.photo.resolution.current");
	data_menu_cfg_.photo_resolution.count  =    data_luacfg->GetIntegerValue("menu.photo.resolution.count");
	data_menu_cfg_.photo_timed.current   =   data_luacfg->GetIntegerValue("menu.photo.timed.current");
	data_menu_cfg_.photo_timed.count    =  data_luacfg->GetIntegerValue("menu.photo.timed.count");
	data_menu_cfg_.photo_auto.current   =   data_luacfg->GetIntegerValue("menu.photo.auto.current");
	data_menu_cfg_.photo_auto.count  =    data_luacfg->GetIntegerValue("menu.photo.auto.count");
	data_menu_cfg_.photo_dramashot.current  =    data_luacfg->GetIntegerValue("menu.photo.dramashot.current");
	data_menu_cfg_.photo_dramashot.count  =    data_luacfg->GetIntegerValue("menu.photo.dramashot.count");
	data_menu_cfg_.photo_quality.current  =    data_luacfg->GetIntegerValue("menu.photo.quality.current");
	data_menu_cfg_.photo_quality.count  =    data_luacfg->GetIntegerValue("menu.photo.qualtiy.count");

	/*****camera******/
	data_menu_cfg_.camera_exposure.current   =   data_luacfg->GetIntegerValue("menu.camera.exposure.current");
	data_menu_cfg_.camera_exposure.count    =  data_luacfg->GetIntegerValue("menu.camera.exposure.count");
	data_menu_cfg_.camera_whitebalance.current  =    data_luacfg->GetIntegerValue("menu.camera.whitebalance.current");
	data_menu_cfg_.camera_whitebalance.count  =    data_luacfg->GetIntegerValue("menu.camera.whitebalance.count"); 
	data_menu_cfg_.camera_lightfreq.current   =   data_luacfg->GetIntegerValue("menu.camera.lightfreq.current");
	data_menu_cfg_.camera_lightfreq.count   =   data_luacfg->GetIntegerValue("menu.camera.lightfreq.count"); 
	data_menu_cfg_.camera_autoscreensaver.current   =   data_luacfg->GetIntegerValue("menu.camera.autoscreensaver.current");
	data_menu_cfg_.camera_autoscreensaver.count    =  data_luacfg->GetIntegerValue("menu.camera.autoscreensaver.count");
	data_menu_cfg_.camera_timedshutdown.current   =   data_luacfg->GetIntegerValue("menu.camera.timedshutdown.current");
	data_menu_cfg_.camera_timedshutdown.count   =   data_luacfg->GetIntegerValue("menu.camera.timedshutdown.count");

	str     =     data_luacfg->GetStringValue("menu.camera.wifiinfo.ssid");
	strncpy(data_menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(data_menu_cfg_.camera_wifiinfo.string1) - 1);
	str     =     data_luacfg->GetStringValue("menu.camera.wifiinfo.password"); 
	strncpy(data_menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(data_menu_cfg_.camera_wifiinfo.string2) - 1);
	str     =     data_luacfg->GetStringValue("menu.device.sysversion.version");
	strncpy(data_menu_cfg_.device_sysversion.string1, str.c_str(), sizeof(data_menu_cfg_.device_sysversion.string1) - 1); 
	str     =     data_luacfg->GetStringValue("menu.device.updatemenu.update");
	strncpy(data_menu_cfg_.device_update.string1, str.c_str(), sizeof(data_menu_cfg_.device_update.string1) - 1); 
	str     =     data_luacfg->GetStringValue("menu.device.systemTimeSave.timeStr");
	strncpy(data_menu_cfg_.device_systemTime.string1, str.c_str(), sizeof(data_menu_cfg_.device_systemTime.string1) - 1); 
	/*****device******/
	data_menu_cfg_.device_language.current  =    data_luacfg->GetIntegerValue("menu.device.language.current");
	data_menu_cfg_.device_language.count  =    data_luacfg->GetIntegerValue("menu.device.language.count");
	data_menu_cfg_.device_datatime.current  =    data_luacfg->GetIntegerValue("menu.device.updatetime.current");
	data_menu_cfg_.device_datatime.count  =    data_luacfg->GetIntegerValue("menu.device.updatetime.count");
	data_menu_cfg_.device_voicestatus.current   =   data_luacfg->GetIntegerValue("menu.device.voicestatus.current");
	data_menu_cfg_.device_voicestatus.count   =   data_luacfg->GetIntegerValue("menu.device.voicestatus.count");
	data_menu_cfg_.device_gpswitch.current   =   data_luacfg->GetIntegerValue("menu.device.gpsswitch.current");
	data_menu_cfg_.device_gpswitch.count   =   data_luacfg->GetIntegerValue("menu.device.gpsswitch.count");
	data_menu_cfg_.device_speedunit.current   =   data_luacfg->GetIntegerValue("menu.device.speedunit.current");
	data_menu_cfg_.device_speedunit.count   =   data_luacfg->GetIntegerValue("menu.device.speedunit.count");
	data_menu_cfg_.device_timezone.current   =   data_luacfg->GetIntegerValue("menu.device.timezone.current");
	data_menu_cfg_.device_timezone.count   =   data_luacfg->GetIntegerValue("menu.device.timezone.count");
	str     =     data_luacfg->GetStringValue("menu.device.carid.idstr");
	strncpy(data_menu_cfg_.device_carid.string1, str.c_str(), sizeof(data_menu_cfg_.device_carid.string1) - 1); 
/*=================================================================================*/

	db_warn("read to compare usr/--/menu_config.lua  and  data/menu_config.lua   one by one ");
	/*****switch******/
	if(data_menu_cfg_.switch_record_eis != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_record_eis = data_menu_cfg_.switch_record_eis;
	if(data_menu_cfg_.switch_record_awmd != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_record_awmd = data_menu_cfg_.switch_record_awmd;
	if(data_menu_cfg_.switch_record_drivingmode != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_record_drivingmode = data_menu_cfg_.switch_record_drivingmode;
	if(data_menu_cfg_.switch_camera_imagerotation != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_camera_imagerotation = data_menu_cfg_.switch_camera_imagerotation;
	if(data_menu_cfg_.switch_camera_ledindicator != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_camera_ledindicator = data_menu_cfg_.switch_camera_ledindicator;
	if(data_menu_cfg_.switch_camera_timewatermark != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_camera_timewatermark = data_menu_cfg_.switch_camera_timewatermark;
	if(data_menu_cfg_.switch_camera_distortioncalibration != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_camera_distortioncalibration = data_menu_cfg_.switch_camera_distortioncalibration;
	if(data_menu_cfg_.switch_camera_keytone != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_camera_keytone = data_menu_cfg_.switch_camera_keytone;
	if(data_menu_cfg_.switch_acc_resume != DEFAULT_VALUE)
		usr_temp_menu_cfg_.switch_acc_resume = data_menu_cfg_.switch_acc_resume;
	if(data_menu_cfg_.switch_acc_resume != DEFAULT_VALUE)
		usr_temp_menu_cfg_.camB_preview = data_menu_cfg_.camB_preview;
	/*****record******/
	/*比较规则如下:
	  *I.如果usr 下面的count 大于data 下面的current ,就有两种情况:
	  *1.usr and data count 相等 ,这种情况直接使用data current.
	  *2.usr count > data count ,这种情况也是直接使用data current
	  *
	  *II.如果usr count <= data count,这种情况直接使用usr current
	*/
	//说明data 存在这个配置选项
	if(data_menu_cfg_.record_resolution.count  != DEFAULT_VALUE  && data_menu_cfg_.record_resolution.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_resolution.count > data_menu_cfg_.record_resolution.current)
			usr_temp_menu_cfg_.record_resolution.current = data_menu_cfg_.record_resolution.current;
		
	if(data_menu_cfg_.record_sound.count  != DEFAULT_VALUE  && data_menu_cfg_.record_sound.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_sound.count > data_menu_cfg_.record_sound.current)
			usr_temp_menu_cfg_.record_sound.current = data_menu_cfg_.record_sound.current;
	
	if(data_menu_cfg_.record_switch_wifi.count  != DEFAULT_VALUE  && data_menu_cfg_.record_switch_wifi.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_switch_wifi.count > data_menu_cfg_.record_switch_wifi.current)
			usr_temp_menu_cfg_.record_switch_wifi.current = data_menu_cfg_.record_switch_wifi.current;
		
	if(data_menu_cfg_.record_rear_resolution.count  != DEFAULT_VALUE  && data_menu_cfg_.record_rear_resolution.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_rear_resolution.count > data_menu_cfg_.record_rear_resolution.current)
			usr_temp_menu_cfg_.record_rear_resolution.current = data_menu_cfg_.record_rear_resolution.current;
		
	if(data_menu_cfg_.record_screen_brightness.count  != DEFAULT_VALUE  && data_menu_cfg_.record_screen_brightness.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_screen_brightness.count > data_menu_cfg_.record_screen_brightness.current)
			usr_temp_menu_cfg_.record_screen_brightness.current = data_menu_cfg_.record_screen_brightness.current;
		
	if(data_menu_cfg_.record_screen_disturb_mode.count  != DEFAULT_VALUE  && data_menu_cfg_.record_screen_disturb_mode.current  != DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_screen_disturb_mode.count > data_menu_cfg_.record_screen_disturb_mode.current)
			usr_temp_menu_cfg_.record_screen_disturb_mode.current = data_menu_cfg_.record_screen_disturb_mode.current;
		
	if(data_menu_cfg_.record_voice_take_photo.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_voice_take_photo.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_voice_take_photo.count > data_menu_cfg_.record_voice_take_photo.current)
			usr_temp_menu_cfg_.record_voice_take_photo.current = data_menu_cfg_.record_voice_take_photo.current;

	if(data_menu_cfg_.record_4g_network_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_4g_network_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_4g_network_switch.count > data_menu_cfg_.record_4g_network_switch.current)
			usr_temp_menu_cfg_.record_4g_network_switch.current = data_menu_cfg_.record_4g_network_switch.current;
		
	if(data_menu_cfg_.record_volume_selection.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_volume_selection.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_volume_selection.count > data_menu_cfg_.record_volume_selection.current)
			usr_temp_menu_cfg_.record_volume_selection.current = data_menu_cfg_.record_volume_selection.current;
		
	if(data_menu_cfg_.record_power_on_sound.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_power_on_sound.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_power_on_sound.count > data_menu_cfg_.record_power_on_sound.current)
			usr_temp_menu_cfg_.record_power_on_sound.current = data_menu_cfg_.record_power_on_sound.current;
		
	if(data_menu_cfg_.record_key_sound.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_key_sound.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_key_sound.count > data_menu_cfg_.record_key_sound.current)
			usr_temp_menu_cfg_.record_key_sound.current = data_menu_cfg_.record_key_sound.current;
		
	if(data_menu_cfg_.record_drivering_report.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_drivering_report.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_drivering_report.count > data_menu_cfg_.record_drivering_report.current)
			usr_temp_menu_cfg_.record_drivering_report.current = data_menu_cfg_.record_drivering_report.current;
		
	if(data_menu_cfg_.record_adas.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_adas.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_adas.count > data_menu_cfg_.record_adas.current)
			usr_temp_menu_cfg_.record_adas.current = data_menu_cfg_.record_adas.current;

	if(data_menu_cfg_.record_standby_clock.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_standby_clock.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_standby_clock.count > data_menu_cfg_.record_standby_clock.current)
			usr_temp_menu_cfg_.record_standby_clock.current = data_menu_cfg_.record_standby_clock.current;

	if(data_menu_cfg_.record_adas_forward_collision_waring.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_adas_forward_collision_waring.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_adas_forward_collision_waring.count > data_menu_cfg_.record_adas_forward_collision_waring.current)
			usr_temp_menu_cfg_.record_adas_forward_collision_waring.current = data_menu_cfg_.record_adas_forward_collision_waring.current;
		
	if(data_menu_cfg_.record_adas_lane_shift_reminding.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_adas_lane_shift_reminding.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_adas_lane_shift_reminding.count > data_menu_cfg_.record_adas_lane_shift_reminding.current)
			usr_temp_menu_cfg_.record_adas_lane_shift_reminding.current = data_menu_cfg_.record_adas_lane_shift_reminding.current;
		
	if(data_menu_cfg_.record_watchdog.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_watchdog.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_watchdog.count > data_menu_cfg_.record_watchdog.current)
			usr_temp_menu_cfg_.record_watchdog.current = data_menu_cfg_.record_watchdog.current;

	if(data_menu_cfg_.record_probeprompt.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_probeprompt.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_probeprompt.count > data_menu_cfg_.record_probeprompt.current)
			usr_temp_menu_cfg_.record_probeprompt.current = data_menu_cfg_.record_probeprompt.current;
		
	if(data_menu_cfg_.record_speedprompt.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_speedprompt.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_speedprompt.count > data_menu_cfg_.record_speedprompt.current)
			usr_temp_menu_cfg_.record_speedprompt.current = data_menu_cfg_.record_speedprompt.current;

	if(data_menu_cfg_.record_timewatermark.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_timewatermark.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_timewatermark.count > data_menu_cfg_.record_timewatermark.current)
			usr_temp_menu_cfg_.record_timewatermark.current = data_menu_cfg_.record_timewatermark.current;

	if(data_menu_cfg_.record_emerrecord.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_emerrecord.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_emerrecord.count > data_menu_cfg_.record_emerrecord.current)
			usr_temp_menu_cfg_.record_emerrecord.current = data_menu_cfg_.record_emerrecord.current;

	if(data_menu_cfg_.record_emerrecordsen.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_emerrecordsen.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_emerrecordsen.count > data_menu_cfg_.record_emerrecordsen.current)
			usr_temp_menu_cfg_.record_emerrecordsen.current = data_menu_cfg_.record_emerrecordsen.current;

	if(data_menu_cfg_.record_parkingwarnlamp_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingwarnlamp_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingwarnlamp_switch.count > data_menu_cfg_.record_parkingwarnlamp_switch.current)
			usr_temp_menu_cfg_.record_parkingwarnlamp_switch.current = data_menu_cfg_.record_parkingwarnlamp_switch.current;

    if(data_menu_cfg_.record_parkingmonitor_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingmonitor_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingmonitor_switch.count > data_menu_cfg_.record_parkingmonitor_switch.current)
			usr_temp_menu_cfg_.record_parkingmonitor_switch.current = data_menu_cfg_.record_parkingmonitor_switch.current;
        
	if(data_menu_cfg_.record_parkingabnormalmonitory_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingabnormalmonitory_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.count > data_menu_cfg_.record_parkingabnormalmonitory_switch.current)
			usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.current = data_menu_cfg_.record_parkingabnormalmonitory_switch.current;

	if(data_menu_cfg_.record_parkingloopabnormalnotice_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingloopabnormalnotice_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.count > data_menu_cfg_.record_parkingloopabnormalnotice_switch.current)
			usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.current = data_menu_cfg_.record_parkingloopabnormalnotice_switch.current;

	if(data_menu_cfg_.record_parkingloop_switch.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingloop_switch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingloop_switch.count > data_menu_cfg_.record_parkingloop_switch.current)
			usr_temp_menu_cfg_.record_parkingloop_switch.current = data_menu_cfg_.record_parkingloop_switch.current;

	if(data_menu_cfg_.record_parkingloop_resolution.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_parkingloop_resolution.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_parkingloop_resolution.count > data_menu_cfg_.record_parkingloop_resolution.current)
			usr_temp_menu_cfg_.record_parkingloop_resolution.current = data_menu_cfg_.record_parkingloop_resolution.current;

	if(data_menu_cfg_.record_encodingtype.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_encodingtype.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_encodingtype.count > data_menu_cfg_.record_encodingtype.current)
			usr_temp_menu_cfg_.record_encodingtype.current = data_menu_cfg_.record_encodingtype.current;

	if(data_menu_cfg_.record_loop.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_loop.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_loop.count > data_menu_cfg_.record_loop.current)
			usr_temp_menu_cfg_.record_loop.current = data_menu_cfg_.record_loop.current;

	if(data_menu_cfg_.record_timelapse.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_timelapse.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_timelapse.count > data_menu_cfg_.record_timelapse.current)
			usr_temp_menu_cfg_.record_timelapse.current = data_menu_cfg_.record_timelapse.current;

	if(data_menu_cfg_.record_slowmotion.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_slowmotion.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_slowmotion.count > data_menu_cfg_.record_slowmotion.current)
			usr_temp_menu_cfg_.record_slowmotion.current = data_menu_cfg_.record_slowmotion.current;

	if(data_menu_cfg_.record_switch_acc.count	!= DEFAULT_VALUE  && data_menu_cfg_.record_switch_acc.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.record_switch_acc.count > data_menu_cfg_.record_switch_acc.current)
			usr_temp_menu_cfg_.record_switch_acc.current = data_menu_cfg_.record_switch_acc.current;
		
	/*****photo******/
	if(data_menu_cfg_.photo_resolution.count	!= DEFAULT_VALUE  && data_menu_cfg_.photo_resolution.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.photo_resolution.count > data_menu_cfg_.photo_resolution.current)
			usr_temp_menu_cfg_.photo_resolution.current = data_menu_cfg_.photo_resolution.current;

	if(data_menu_cfg_.photo_timed.count	!= DEFAULT_VALUE  && data_menu_cfg_.photo_timed.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.photo_timed.count > data_menu_cfg_.photo_timed.current)
			usr_temp_menu_cfg_.photo_timed.current = data_menu_cfg_.photo_timed.current;

	if(data_menu_cfg_.photo_auto.count	!= DEFAULT_VALUE  && data_menu_cfg_.photo_auto.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.photo_auto.count > data_menu_cfg_.photo_auto.current)
			usr_temp_menu_cfg_.photo_auto.current = data_menu_cfg_.photo_auto.current;

	if(data_menu_cfg_.photo_dramashot.count	!= DEFAULT_VALUE  && data_menu_cfg_.photo_dramashot.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.photo_dramashot.count > data_menu_cfg_.photo_dramashot.current)
			usr_temp_menu_cfg_.photo_dramashot.current = data_menu_cfg_.photo_dramashot.current;

	if(data_menu_cfg_.photo_quality.count	!= DEFAULT_VALUE  && data_menu_cfg_.photo_quality.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.photo_quality.count > data_menu_cfg_.photo_quality.current)
			usr_temp_menu_cfg_.photo_quality.current = data_menu_cfg_.photo_quality.current;
	/*****camera******/
	if(data_menu_cfg_.camera_exposure.count	!= DEFAULT_VALUE  && data_menu_cfg_.camera_exposure.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.camera_exposure.count > data_menu_cfg_.camera_exposure.current)
			usr_temp_menu_cfg_.camera_exposure.current = data_menu_cfg_.camera_exposure.current;

	if(data_menu_cfg_.camera_whitebalance.count	!= DEFAULT_VALUE  && data_menu_cfg_.camera_whitebalance.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.camera_whitebalance.count > data_menu_cfg_.camera_whitebalance.current)
			usr_temp_menu_cfg_.camera_whitebalance.current = data_menu_cfg_.camera_whitebalance.current;

	if(data_menu_cfg_.camera_lightfreq.count	!= DEFAULT_VALUE  && data_menu_cfg_.camera_lightfreq.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.camera_lightfreq.count > data_menu_cfg_.camera_lightfreq.current)
			usr_temp_menu_cfg_.camera_lightfreq.current = data_menu_cfg_.camera_lightfreq.current;

	if(data_menu_cfg_.camera_autoscreensaver.count	!= DEFAULT_VALUE  && data_menu_cfg_.camera_autoscreensaver.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.camera_autoscreensaver.count > data_menu_cfg_.camera_autoscreensaver.current)
			usr_temp_menu_cfg_.camera_autoscreensaver.current = data_menu_cfg_.camera_autoscreensaver.current;

	if(data_menu_cfg_.camera_timedshutdown.count	!= DEFAULT_VALUE  && data_menu_cfg_.camera_timedshutdown.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.camera_timedshutdown.count > data_menu_cfg_.camera_timedshutdown.current)
			usr_temp_menu_cfg_.camera_timedshutdown.current = data_menu_cfg_.camera_timedshutdown.current;
		
	/*****device******/
	if(data_menu_cfg_.device_language.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_language.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_language.count > data_menu_cfg_.device_language.current)
			usr_temp_menu_cfg_.device_language.current = data_menu_cfg_.device_language.current;

	if(data_menu_cfg_.device_datatime.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_datatime.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_datatime.count > data_menu_cfg_.device_datatime.current)
			usr_temp_menu_cfg_.device_datatime.current = data_menu_cfg_.device_datatime.current;

	if(data_menu_cfg_.device_voicestatus.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_voicestatus.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_voicestatus.count > data_menu_cfg_.device_voicestatus.current)
			usr_temp_menu_cfg_.device_voicestatus.current = data_menu_cfg_.device_voicestatus.current;
		
	if(data_menu_cfg_.device_gpswitch.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_gpswitch.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_gpswitch.count > data_menu_cfg_.device_gpswitch.current)
			usr_temp_menu_cfg_.device_gpswitch.current = data_menu_cfg_.device_gpswitch.current;

	if(data_menu_cfg_.device_speedunit.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_speedunit.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_speedunit.count > data_menu_cfg_.device_speedunit.current)
			usr_temp_menu_cfg_.device_speedunit.current = data_menu_cfg_.device_speedunit.current;

	if(data_menu_cfg_.device_timezone.count	!= DEFAULT_VALUE  && data_menu_cfg_.device_timezone.current	!= DEFAULT_VALUE)
		if(usr_temp_menu_cfg_.device_timezone.count > data_menu_cfg_.device_timezone.current)
			usr_temp_menu_cfg_.device_timezone.current = data_menu_cfg_.device_timezone.current;
	/*****string******/
	str.clear();//wifi ssid is "" the first time 
	str     =     data_luacfg->GetStringValue("menu.camera.wifiinfo.ssid");
	strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string1) - 1);
	str.clear();
	str     =     data_luacfg->GetStringValue("menu.camera.wifiinfo.password"); 
	if(!str.empty())
		strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string2) - 1);
	str.clear();
	str     =     data_luacfg->GetStringValue("menu.device.updatemenu.update");
	if(!str.empty())
		strncpy(usr_temp_menu_cfg_.device_update.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_update.string1) - 1); 
	str.clear();
	str     =     data_luacfg->GetStringValue("menu.device.systemTimeSave.timeStr");
	if(!str.empty())
		strncpy(usr_temp_menu_cfg_.device_systemTime.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_systemTime.string1) - 1); 
	str.clear();
	str     =     data_luacfg->GetStringValue("menu.device.carid.idstr");
	if(!str.empty())
		strncpy(usr_temp_menu_cfg_.device_carid.string1, str.c_str(), sizeof(usr_temp_menu_cfg_.device_carid.string1) - 1); 
	str.clear();
	db_warn("ready to set the data to the usr_luacfg");
	//set to the usr_luacfg
	/*****switch******/
	usr_luacfg->SetIntegerValue("menu.switch.record_eis",  usr_temp_menu_cfg_.switch_record_eis);
	usr_luacfg->SetIntegerValue("menu.switch.record_awmd",  usr_temp_menu_cfg_.switch_record_awmd); 
	usr_luacfg->SetIntegerValue("menu.switch.record_drivingmode",  usr_temp_menu_cfg_.switch_record_drivingmode);
	usr_luacfg->SetIntegerValue("menu.switch.camera_imagerotation",  usr_temp_menu_cfg_.switch_camera_imagerotation);
	usr_luacfg->SetIntegerValue("menu.switch.camera_ledindicator",  usr_temp_menu_cfg_.switch_camera_ledindicator);
	usr_luacfg->SetIntegerValue("menu.switch.camera_timewatermark",  usr_temp_menu_cfg_.switch_camera_timewatermark);
	usr_luacfg->SetIntegerValue("menu.switch.camera_distortioncalibration",  usr_temp_menu_cfg_.switch_camera_distortioncalibration);
	usr_luacfg->SetIntegerValue("menu.switch.camera_keytone",  usr_temp_menu_cfg_.switch_camera_keytone);
	usr_luacfg->SetIntegerValue("menu.switch.camB_preview", usr_temp_menu_cfg_.camB_preview);
	//**** acc_resume *****//
	usr_luacfg->SetIntegerValue("menu.switch.acc_resume",  usr_temp_menu_cfg_.switch_acc_resume);
	usr_luacfg->SetIntegerValue("menu.record.acc.current",  usr_temp_menu_cfg_.record_switch_acc.current);
	usr_luacfg->SetIntegerValue("menu.record.acc.count",	usr_temp_menu_cfg_.record_switch_acc.count);
	/*****record******/
	usr_luacfg->SetIntegerValue("menu.record.resolution.current",  usr_temp_menu_cfg_.record_resolution.current);
	usr_luacfg->SetIntegerValue("menu.record.resolution.count",  usr_temp_menu_cfg_.record_resolution.count);
	usr_luacfg->SetIntegerValue("menu.record.record_sound.current",  usr_temp_menu_cfg_.record_sound.current);
	usr_luacfg->SetIntegerValue("menu.record.record_sound.count",  usr_temp_menu_cfg_.record_sound.count);
	usr_luacfg->SetIntegerValue("menu.record.wifisw.current",  usr_temp_menu_cfg_.record_switch_wifi.current);
	usr_luacfg->SetIntegerValue("menu.record.wifisw.count",  usr_temp_menu_cfg_.record_switch_wifi.count);
	usr_luacfg->SetIntegerValue("menu.record.rear_resolution.current",  usr_temp_menu_cfg_.record_rear_resolution.current);
	usr_luacfg->SetIntegerValue("menu.record.rear_resolution.count",  usr_temp_menu_cfg_.record_rear_resolution.count);
	usr_luacfg->SetIntegerValue("menu.record.screenbrightness.current",  usr_temp_menu_cfg_.record_screen_brightness.current);
	usr_luacfg->SetIntegerValue("menu.record.screenbrightness.count",  usr_temp_menu_cfg_.record_screen_brightness.count);
	usr_luacfg->SetIntegerValue("menu.record.disturbmode.current",  usr_temp_menu_cfg_.record_screen_disturb_mode.current);
	usr_luacfg->SetIntegerValue("menu.record.disturbmode.count",  usr_temp_menu_cfg_.record_screen_disturb_mode.count);
	usr_luacfg->SetIntegerValue("menu.record.voicephoto.current",  usr_temp_menu_cfg_.record_voice_take_photo.current);
	usr_luacfg->SetIntegerValue("menu.record.voicephoto.count",  usr_temp_menu_cfg_.record_voice_take_photo.count);
	usr_luacfg->SetIntegerValue("menu.record.network4g.current",  usr_temp_menu_cfg_.record_4g_network_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.network4g.count",  usr_temp_menu_cfg_.record_4g_network_switch.count);
	usr_luacfg->SetIntegerValue("menu.record.volumeselect.current",  usr_temp_menu_cfg_.record_volume_selection.current);
	usr_luacfg->SetIntegerValue("menu.record.volumeselect.count",  usr_temp_menu_cfg_.record_volume_selection.count);
	usr_luacfg->SetIntegerValue("menu.record.poweronvoice.current",  usr_temp_menu_cfg_.record_power_on_sound.current);
	usr_luacfg->SetIntegerValue("menu.record.poweronvoice.count",  usr_temp_menu_cfg_.record_power_on_sound.count);
	usr_luacfg->SetIntegerValue("menu.record.keyvoice.current",  usr_temp_menu_cfg_.record_key_sound.current);
	usr_luacfg->SetIntegerValue("menu.record.keyvoice.count",  usr_temp_menu_cfg_.record_key_sound.count);
	usr_luacfg->SetIntegerValue("menu.record.drivereport.current",  usr_temp_menu_cfg_.record_drivering_report.current);
	usr_luacfg->SetIntegerValue("menu.record.drivereport.count",  usr_temp_menu_cfg_.record_drivering_report.count);
	usr_luacfg->SetIntegerValue("menu.record.adas.current",  usr_temp_menu_cfg_.record_adas.current);
	usr_luacfg->SetIntegerValue("menu.record.adas.count",  usr_temp_menu_cfg_.record_adas.count);
	usr_luacfg->SetIntegerValue("menu.record.standbyclock.current",  usr_temp_menu_cfg_.record_standby_clock.current);
	usr_luacfg->SetIntegerValue("menu.record.standbyclock.count",  usr_temp_menu_cfg_.record_standby_clock.count);
	usr_luacfg->SetIntegerValue("menu.record.adasfcw.current",  usr_temp_menu_cfg_.record_adas_forward_collision_waring.current);
	usr_luacfg->SetIntegerValue("menu.record.adasfcw.count",  usr_temp_menu_cfg_.record_adas_forward_collision_waring.count);
	usr_luacfg->SetIntegerValue("menu.record.adaslsr.current",  usr_temp_menu_cfg_.record_adas_lane_shift_reminding.current);
	usr_luacfg->SetIntegerValue("menu.record.adaslsr.count",  usr_temp_menu_cfg_.record_adas_lane_shift_reminding.count);
	usr_luacfg->SetIntegerValue("menu.record.watchdog.current",  usr_temp_menu_cfg_.record_watchdog.current);
	usr_luacfg->SetIntegerValue("menu.record.watchdog.count",  usr_temp_menu_cfg_.record_watchdog.count);
	usr_luacfg->SetIntegerValue("menu.record.probeprompt.current",  usr_temp_menu_cfg_.record_probeprompt.current);
	usr_luacfg->SetIntegerValue("menu.record.probeprompt.count",  usr_temp_menu_cfg_.record_probeprompt.count);
	usr_luacfg->SetIntegerValue("menu.record.speedprompt.current",  usr_temp_menu_cfg_.record_speedprompt.current);
	usr_luacfg->SetIntegerValue("menu.record.speedprompt.count",  usr_temp_menu_cfg_.record_speedprompt.count);
	usr_luacfg->SetIntegerValue("menu.record.timewatermark.current",  usr_temp_menu_cfg_.record_timewatermark.current);
	usr_luacfg->SetIntegerValue("menu.record.timewatermark.count",  usr_temp_menu_cfg_.record_timewatermark.count);
	usr_luacfg->SetIntegerValue("menu.record.emerrecord.current",  usr_temp_menu_cfg_.record_emerrecord.current);
	usr_luacfg->SetIntegerValue("menu.record.emerrecord.count",  usr_temp_menu_cfg_.record_emerrecord.count);
	usr_luacfg->SetIntegerValue("menu.record.emerrecordsen.current",  usr_temp_menu_cfg_.record_emerrecordsen.current);
	usr_luacfg->SetIntegerValue("menu.record.emerrecordsen.count",  usr_temp_menu_cfg_.record_emerrecordsen.count);
	usr_luacfg->SetIntegerValue("menu.record.parkingwarnlamp.current",  usr_temp_menu_cfg_.record_parkingwarnlamp_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingwarnlamp.count",  usr_temp_menu_cfg_.record_parkingwarnlamp_switch.count);
    usr_luacfg->SetIntegerValue("menu.record.parkingmonitor.current",  usr_temp_menu_cfg_.record_parkingmonitor_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingmonitor.count",  usr_temp_menu_cfg_.record_parkingmonitor_switch.count);
	usr_luacfg->SetIntegerValue("menu.record.parkingabnormalmonitor.current",  usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingabnormalmonitor.count",  usr_temp_menu_cfg_.record_parkingabnormalmonitory_switch.count);
	usr_luacfg->SetIntegerValue("menu.record.parkingabnormalnotice.current",  usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingabnormalnotice.count",  usr_temp_menu_cfg_.record_parkingloopabnormalnotice_switch.count);
	usr_luacfg->SetIntegerValue("menu.record.parkingloopsw.current",  usr_temp_menu_cfg_.record_parkingloop_switch.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingloopsw.count",  usr_temp_menu_cfg_.record_parkingloop_switch.count);
	usr_luacfg->SetIntegerValue("menu.record.parkingloopresolution.current",  usr_temp_menu_cfg_.record_parkingloop_resolution.current);
	usr_luacfg->SetIntegerValue("menu.record.parkingloopresolution.count",  usr_temp_menu_cfg_.record_parkingloop_resolution.count);
	usr_luacfg->SetIntegerValue("menu.record.encodingtype.current",  usr_temp_menu_cfg_.record_encodingtype.current);
	usr_luacfg->SetIntegerValue("menu.record.encodingtype.count",  usr_temp_menu_cfg_.record_encodingtype.count);
	usr_luacfg->SetIntegerValue("menu.record.loop.current",  usr_temp_menu_cfg_.record_loop.current);
	usr_luacfg->SetIntegerValue("menu.record.loop.count",  usr_temp_menu_cfg_.record_loop.count);
	usr_luacfg->SetIntegerValue("menu.record.timelapse.current",  usr_temp_menu_cfg_.record_timelapse.current);
	usr_luacfg->SetIntegerValue("menu.record.timelapse.count",  usr_temp_menu_cfg_.record_timelapse.count);
	usr_luacfg->SetIntegerValue("menu.record.slowmotion.current",  usr_temp_menu_cfg_.record_slowmotion.current);
	usr_luacfg->SetIntegerValue("menu.record.slowmotion.count",  usr_temp_menu_cfg_.record_slowmotion.count);

	/*****photo******/
	usr_luacfg->SetIntegerValue("menu.photo.resolution.current",  usr_temp_menu_cfg_.photo_resolution.current);
	usr_luacfg->SetIntegerValue("menu.photo.resolution.count",  usr_temp_menu_cfg_.photo_resolution.count);
	usr_luacfg->SetIntegerValue("menu.photo.timed.current",  usr_temp_menu_cfg_.photo_timed.current);
	usr_luacfg->SetIntegerValue("menu.photo.timed.count",  usr_temp_menu_cfg_.photo_timed.count);
	usr_luacfg->SetIntegerValue("menu.photo.auto.current",  usr_temp_menu_cfg_.photo_auto.current);
	usr_luacfg->SetIntegerValue("menu.photo.auto.count",  usr_temp_menu_cfg_.photo_auto.count);
	usr_luacfg->SetIntegerValue("menu.photo.dramashot.current",  usr_temp_menu_cfg_.photo_dramashot.current);
	usr_luacfg->SetIntegerValue("menu.photo.dramashot.count",  usr_temp_menu_cfg_.photo_dramashot.count);
	usr_luacfg->SetIntegerValue("menu.photo.quality.current",  usr_temp_menu_cfg_.photo_quality.current);
	usr_luacfg->SetIntegerValue("menu.photo.quality.count",  usr_temp_menu_cfg_.photo_quality.count);

	/*****camera******/
	usr_luacfg->SetIntegerValue("menu.camera.exposure.current",  usr_temp_menu_cfg_.camera_exposure.current);
	usr_luacfg->SetIntegerValue("menu.camera.exposure.count",  usr_temp_menu_cfg_.camera_exposure.count);
	usr_luacfg->SetIntegerValue("menu.camera.whitebalance.current",  usr_temp_menu_cfg_.camera_whitebalance.current);
	usr_luacfg->SetIntegerValue("menu.camera.whitebalance.count",  usr_temp_menu_cfg_.camera_whitebalance.count); 
	usr_luacfg->SetIntegerValue("menu.camera.lightfreq.current",  usr_temp_menu_cfg_.camera_lightfreq.current);
	usr_luacfg->SetIntegerValue("menu.camera.lightfreq.count",  usr_temp_menu_cfg_.camera_lightfreq.count); 
	usr_luacfg->SetIntegerValue("menu.camera.autoscreensaver.current",  usr_temp_menu_cfg_.camera_autoscreensaver.current);
	usr_luacfg->SetIntegerValue("menu.camera.autoscreensaver.count",  usr_temp_menu_cfg_.camera_autoscreensaver.count);
	usr_luacfg->SetIntegerValue("menu.camera.timedshutdown.current",  usr_temp_menu_cfg_.camera_timedshutdown.current);
	usr_luacfg->SetIntegerValue("menu.camera.timedshutdown.count",  usr_temp_menu_cfg_.camera_timedshutdown.count);

	usr_luacfg->SetStringValue("menu.camera.wifiinfo.ssid",  usr_temp_menu_cfg_.camera_wifiinfo.string1);
	usr_luacfg->SetStringValue("menu.camera.wifiinfo.password",  usr_temp_menu_cfg_.camera_wifiinfo.string2); 

	/*****device******/
	usr_luacfg->SetStringValue("menu.device.systemTimeSave.timeStr",  usr_temp_menu_cfg_.device_systemTime.string1);
	usr_luacfg->SetStringValue("menu.device.updatemenu.update",  usr_temp_menu_cfg_.device_update.string1);
	usr_luacfg->SetStringValue("menu.device.sysversion.version",  usr_temp_menu_cfg_.device_sysversion.string1);
	usr_luacfg->SetIntegerValue("menu.device.language.current",  usr_temp_menu_cfg_.device_language.current);
	usr_luacfg->SetIntegerValue("menu.device.language.count",  usr_temp_menu_cfg_.device_language.count);
	usr_luacfg->SetIntegerValue("menu.device.updatetime.current",  usr_temp_menu_cfg_.device_datatime.current);
	usr_luacfg->SetIntegerValue("menu.device.updatetime.count",  usr_temp_menu_cfg_.device_datatime.count);
	usr_luacfg->SetIntegerValue("menu.device.voicestatus.current",  usr_temp_menu_cfg_.device_voicestatus.current);
	usr_luacfg->SetIntegerValue("menu.device.voicestatus.count",  usr_temp_menu_cfg_.device_voicestatus.count);
	usr_luacfg->SetIntegerValue("menu.device.gpsswitch.current",  usr_temp_menu_cfg_.device_gpswitch.current);
	usr_luacfg->SetIntegerValue("menu.device.gpsswitch.count",  usr_temp_menu_cfg_.device_gpswitch.count);
	usr_luacfg->SetIntegerValue("menu.device.speedunit.current",  usr_temp_menu_cfg_.device_speedunit.current);
	usr_luacfg->SetIntegerValue("menu.device.speedunit.count",  usr_temp_menu_cfg_.device_speedunit.count);
	usr_luacfg->SetIntegerValue("menu.device.timezone.current",  usr_temp_menu_cfg_.device_timezone.current);
	usr_luacfg->SetIntegerValue("menu.device.timezone.count",  usr_temp_menu_cfg_.device_timezone.count);
	usr_luacfg->SetStringValue("menu.device.carid.idstr",  usr_temp_menu_cfg_.device_carid.string1);
#if 0
	/*****string******/
	strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string1,data_menu_cfg_.camera_wifiinfo.string1, sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string1) - 1);

	strncpy(usr_temp_menu_cfg_.camera_wifiinfo.string2, data_menu_cfg_.camera_wifiinfo.string2, sizeof(usr_temp_menu_cfg_.camera_wifiinfo.string2) - 1);

	strncpy(usr_temp_menu_cfg_.device_sysversion.string1, data_menu_cfg_.device_sysversion.string1, sizeof(usr_temp_menu_cfg_.device_sysversion.string1) - 1); 

	strncpy(usr_temp_menu_cfg_.device_update.string1, data_menu_cfg_.device_update.string1, sizeof(usr_temp_menu_cfg_.device_update.string1) - 1); 

	strncpy(usr_temp_menu_cfg_.device_systemTime.string1, data_menu_cfg_.device_systemTime.string1, sizeof(usr_temp_menu_cfg_.device_systemTime.string1) - 1); 
#endif
	db_warn("ready to save the compare result to the usr/--/menu_config_temp.lua");
	ret = usr_luacfg->SyncConfigToFile(MENU_CONFIG_FILE_USR_TEMP, "menu");
	if (ret < 0) 
	{
		db_error("Do SyncConfigToFile error! file:%s \n", MENU_CONFIG_FILE_USR_TEMP);
		return ret;
	}
	db_warn("Comparison is ok ");
	return ret;
}

int MenuConfigLua::LoadMenuConfig(void)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == this->lua_cfg_) {
        db_error("[fangjj]:The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(MENU_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", MENU_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(MENU_CONFIG_FILE);		// "/tmp/data/menu_config.lua"
    if (ret) {
        db_warn("Load %s failed, copy backup and try again", MENU_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/menu_config.lua /tmp/data/");

        ret = lua_cfg_->LoadFromFile(MENU_CONFIG_FILE);
        if (ret) {
            db_error("[fangjj]:Load %s failed!", MENU_CONFIG_FILE);
            return -1;
        }
    }
	db_error("read and set menu_cfg_");
       /*****switch******/
       menu_cfg_.switch_record_eis = lua_cfg_->GetIntegerValue("menu.switch.record_eis");
       menu_cfg_.switch_record_awmd =  lua_cfg_->GetIntegerValue("menu.switch.record_awmd"); 
       menu_cfg_.switch_record_drivingmode =  lua_cfg_->GetIntegerValue("menu.switch.record_drivingmode");
       menu_cfg_.switch_camera_imagerotation  =   lua_cfg_->GetIntegerValue("menu.switch.camera_imagerotation");
       menu_cfg_.switch_camera_ledindicator   =  lua_cfg_->GetIntegerValue("menu.switch.camera_ledindicator");
       menu_cfg_.switch_camera_timewatermark  =    lua_cfg_->GetIntegerValue("menu.switch.camera_timewatermark");
       menu_cfg_.switch_camera_distortioncalibration =    lua_cfg_->GetIntegerValue("menu.switch.camera_distortioncalibration");
       menu_cfg_.switch_camera_keytone   = lua_cfg_->GetIntegerValue("menu.switch.camera_keytone");
	   menu_cfg_.camB_preview = lua_cfg_->GetIntegerValue("menu.switch.camB_preview");
	  /*****acc_resume*****/
	  menu_cfg_.switch_acc_resume   = lua_cfg_->GetIntegerValue("menu.switch.acc_resume");
	  
       /*****record******/
       menu_cfg_.record_resolution.current  =    lua_cfg_->GetIntegerValue("menu.record.resolution.current");
       menu_cfg_.record_resolution.count  =    lua_cfg_->GetIntegerValue("menu.record.resolution.count");
	menu_cfg_.record_sound.current  =    lua_cfg_->GetIntegerValue("menu.record.record_sound.current");
       menu_cfg_.record_sound.count  =    lua_cfg_->GetIntegerValue("menu.record.record_sound.count");
	menu_cfg_.record_switch_wifi.current  =    lua_cfg_->GetIntegerValue("menu.record.wifisw.current");
       menu_cfg_.record_switch_wifi.count  =    lua_cfg_->GetIntegerValue("menu.record.wifisw.count");
       menu_cfg_.record_rear_resolution.current  =    lua_cfg_->GetIntegerValue("menu.record.rear_resolution.current");
       menu_cfg_.record_rear_resolution.count  =    lua_cfg_->GetIntegerValue("menu.record.rear_resolution.count");
	menu_cfg_.record_screen_brightness.current  =    lua_cfg_->GetIntegerValue("menu.record.screenbrightness.current");
       menu_cfg_.record_screen_brightness.count  =    lua_cfg_->GetIntegerValue("menu.record.screenbrightness.count");
	menu_cfg_.record_screen_disturb_mode.current  =    lua_cfg_->GetIntegerValue("menu.record.disturbmode.current");
       menu_cfg_.record_screen_disturb_mode.count  =    lua_cfg_->GetIntegerValue("menu.record.disturbmode.count");
	menu_cfg_.record_voice_take_photo.current  =    lua_cfg_->GetIntegerValue("menu.record.voicephoto.current");
       menu_cfg_.record_voice_take_photo.count  =    lua_cfg_->GetIntegerValue("menu.record.voicephoto.count");
	menu_cfg_.record_4g_network_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.network4g.current");
       menu_cfg_.record_4g_network_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.network4g.count");
	menu_cfg_.record_volume_selection.current  =    lua_cfg_->GetIntegerValue("menu.record.volumeselect.current");
       menu_cfg_.record_volume_selection.count  =    lua_cfg_->GetIntegerValue("menu.record.volumeselect.count");
	menu_cfg_.record_power_on_sound.current  =    lua_cfg_->GetIntegerValue("menu.record.poweronvoice.current");
       menu_cfg_.record_power_on_sound.count  =    lua_cfg_->GetIntegerValue("menu.record.poweronvoice.count");
	menu_cfg_.record_key_sound.current  =    lua_cfg_->GetIntegerValue("menu.record.keyvoice.current");
       menu_cfg_.record_key_sound.count  =    lua_cfg_->GetIntegerValue("menu.record.keyvoice.count");
	menu_cfg_.record_drivering_report.current  =    lua_cfg_->GetIntegerValue("menu.record.drivereport.current");
       menu_cfg_.record_drivering_report.count  =    lua_cfg_->GetIntegerValue("menu.record.drivereport.count");
	menu_cfg_.record_adas.current  =    lua_cfg_->GetIntegerValue("menu.record.adas.current");
       menu_cfg_.record_adas.count  =    lua_cfg_->GetIntegerValue("menu.record.adas.count");
	menu_cfg_.record_standby_clock.current  =    lua_cfg_->GetIntegerValue("menu.record.standbyclock.current");
       menu_cfg_.record_standby_clock.count  =    lua_cfg_->GetIntegerValue("menu.record.standbyclock.count");
	menu_cfg_.record_adas_forward_collision_waring.current  =    lua_cfg_->GetIntegerValue("menu.record.adasfcw.current");
       menu_cfg_.record_adas_forward_collision_waring.count  =    lua_cfg_->GetIntegerValue("menu.record.adasfcw.count");
	menu_cfg_.record_adas_lane_shift_reminding.current  =    lua_cfg_->GetIntegerValue("menu.record.adaslsr.current");
       menu_cfg_.record_adas_lane_shift_reminding.count  =    lua_cfg_->GetIntegerValue("menu.record.adaslsr.count");
	menu_cfg_.record_watchdog.current  =    lua_cfg_->GetIntegerValue("menu.record.watchdog.current");
       menu_cfg_.record_watchdog.count  =    lua_cfg_->GetIntegerValue("menu.record.watchdog.count");
	menu_cfg_.record_probeprompt.current  =    lua_cfg_->GetIntegerValue("menu.record.probeprompt.current");
       menu_cfg_.record_probeprompt.count  =    lua_cfg_->GetIntegerValue("menu.record.probeprompt.count");
	menu_cfg_.record_speedprompt.current  =    lua_cfg_->GetIntegerValue("menu.record.speedprompt.current");
       menu_cfg_.record_speedprompt.count  =    lua_cfg_->GetIntegerValue("menu.record.speedprompt.count");
	menu_cfg_.record_timewatermark.current  =    lua_cfg_->GetIntegerValue("menu.record.timewatermark.current");
       menu_cfg_.record_timewatermark.count  =    lua_cfg_->GetIntegerValue("menu.record.timewatermark.count");
	menu_cfg_.record_emerrecord.current  =    lua_cfg_->GetIntegerValue("menu.record.emerrecord.current");
       menu_cfg_.record_emerrecord.count  =    lua_cfg_->GetIntegerValue("menu.record.emerrecord.count");
	menu_cfg_.record_emerrecordsen.current  =    lua_cfg_->GetIntegerValue("menu.record.emerrecordsen.current");
       menu_cfg_.record_emerrecordsen.count  =    lua_cfg_->GetIntegerValue("menu.record.emerrecordsen.count");
	menu_cfg_.record_parkingwarnlamp_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingwarnlamp.current");
       menu_cfg_.record_parkingwarnlamp_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingwarnlamp.count");
       menu_cfg_.record_parkingmonitor_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingmonitor.current");
       menu_cfg_.record_parkingmonitor_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingmonitor.count");
       menu_cfg_.record_parkingabnormalmonitory_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingabnormalmonitor.current");
       menu_cfg_.record_parkingabnormalmonitory_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingabnormalmonitor.count");
       menu_cfg_.record_parkingloopabnormalnotice_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingabnormalnotice.current");
       menu_cfg_.record_parkingloopabnormalnotice_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingabnormalnotice.count");
       menu_cfg_.record_parkingloop_switch.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingloopsw.current");
       menu_cfg_.record_parkingloop_switch.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingloopsw.count");
	menu_cfg_.record_parkingloop_resolution.current  =    lua_cfg_->GetIntegerValue("menu.record.parkingloopresolution.current");
       menu_cfg_.record_parkingloop_resolution.count  =    lua_cfg_->GetIntegerValue("menu.record.parkingloopresolution.count");
       menu_cfg_.record_encodingtype.current  =    lua_cfg_->GetIntegerValue("menu.record.encodingtype.current");
       menu_cfg_.record_encodingtype.count  =    lua_cfg_->GetIntegerValue("menu.record.encodingtype.count");
       menu_cfg_.record_loop.current  =    lua_cfg_->GetIntegerValue("menu.record.loop.current");
       menu_cfg_.record_loop.count   =   lua_cfg_->GetIntegerValue("menu.record.loop.count");
       menu_cfg_.record_timelapse.current  =    lua_cfg_->GetIntegerValue("menu.record.timelapse.current");
       menu_cfg_.record_timelapse.count   =   lua_cfg_->GetIntegerValue("menu.record.timelapse.count");
       menu_cfg_.record_slowmotion.current   =   lua_cfg_->GetIntegerValue("menu.record.slowmotion.current");
       menu_cfg_.record_slowmotion.count   =   lua_cfg_->GetIntegerValue("menu.record.slowmotion.count");
	   menu_cfg_.record_switch_acc.current  =    lua_cfg_->GetIntegerValue("menu.record.acc.current");
       menu_cfg_.record_switch_acc.count  =    lua_cfg_->GetIntegerValue("menu.record.acc.count");     
       /*****photo******/
       menu_cfg_.photo_resolution.current =     lua_cfg_->GetIntegerValue("menu.photo.resolution.current");
       menu_cfg_.photo_resolution.count  =    lua_cfg_->GetIntegerValue("menu.photo.resolution.count");
       menu_cfg_.photo_timed.current   =   lua_cfg_->GetIntegerValue("menu.photo.timed.current");
       menu_cfg_.photo_timed.count    =  lua_cfg_->GetIntegerValue("menu.photo.timed.count");
       menu_cfg_.photo_auto.current   =   lua_cfg_->GetIntegerValue("menu.photo.auto.current");
       menu_cfg_.photo_auto.count  =    lua_cfg_->GetIntegerValue("menu.photo.auto.count");
       menu_cfg_.photo_dramashot.current  =    lua_cfg_->GetIntegerValue("menu.photo.dramashot.current");
       menu_cfg_.photo_dramashot.count  =    lua_cfg_->GetIntegerValue("menu.photo.dramashot.count");
       menu_cfg_.photo_quality.current  =    lua_cfg_->GetIntegerValue("menu.photo.quality.current");
       menu_cfg_.photo_quality.count  =    lua_cfg_->GetIntegerValue("menu.photo.quality.count");
         
       /*****camera******/
       menu_cfg_.camera_exposure.current   =   lua_cfg_->GetIntegerValue("menu.camera.exposure.current");
       menu_cfg_.camera_exposure.count    =  lua_cfg_->GetIntegerValue("menu.camera.exposure.count");
       menu_cfg_.camera_whitebalance.current  =    lua_cfg_->GetIntegerValue("menu.camera.whitebalance.current");
       menu_cfg_.camera_whitebalance.count  =    lua_cfg_->GetIntegerValue("menu.camera.whitebalance.count"); 
       menu_cfg_.camera_lightfreq.current   =   lua_cfg_->GetIntegerValue("menu.camera.lightfreq.current");
       menu_cfg_.camera_lightfreq.count   =   lua_cfg_->GetIntegerValue("menu.camera.lightfreq.count"); 
       menu_cfg_.camera_autoscreensaver.current   =   lua_cfg_->GetIntegerValue("menu.camera.autoscreensaver.current");
       menu_cfg_.camera_autoscreensaver.count    =  lua_cfg_->GetIntegerValue("menu.camera.autoscreensaver.count");
       menu_cfg_.camera_timedshutdown.current   =   lua_cfg_->GetIntegerValue("menu.camera.timedshutdown.current");
       menu_cfg_.camera_timedshutdown.count   =   lua_cfg_->GetIntegerValue("menu.camera.timedshutdown.count");
          
       str     =     lua_cfg_->GetStringValue("menu.camera.wifiinfo.ssid");
       strncpy(menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string1) - 1);
       str     =     lua_cfg_->GetStringValue("menu.camera.wifiinfo.password"); 
       strncpy(menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string2) - 1);
       str     =     lua_cfg_->GetStringValue("menu.device.sysversion.version");
       strncpy(menu_cfg_.device_sysversion.string1, str.c_str(), sizeof(menu_cfg_.device_sysversion.string1) - 1); 
	str     =     lua_cfg_->GetStringValue("menu.device.updatemenu.update");
       strncpy(menu_cfg_.device_update.string1, str.c_str(), sizeof(menu_cfg_.device_update.string1) - 1); 
	str     =     lua_cfg_->GetStringValue("menu.device.systemTimeSave.timeStr");
       strncpy(menu_cfg_.device_systemTime.string1, str.c_str(), sizeof(menu_cfg_.device_systemTime.string1) - 1); 
        /*****device******/
       menu_cfg_.device_language.current  =    lua_cfg_->GetIntegerValue("menu.device.language.current");
       menu_cfg_.device_language.count  =    lua_cfg_->GetIntegerValue("menu.device.language.count");
	menu_cfg_.device_datatime.current  =    lua_cfg_->GetIntegerValue("menu.device.updatetime.current");
       menu_cfg_.device_datatime.count  =    lua_cfg_->GetIntegerValue("menu.device.updatetime.count");
       menu_cfg_.device_voicestatus.current   =   lua_cfg_->GetIntegerValue("menu.device.voicestatus.current");
       menu_cfg_.device_voicestatus.count   =   lua_cfg_->GetIntegerValue("menu.device.voicestatus.count");

	   menu_cfg_.device_gpswitch.current   =   lua_cfg_->GetIntegerValue("menu.device.gpsswitch.current");
       menu_cfg_.device_gpswitch.count   =   lua_cfg_->GetIntegerValue("menu.device.gpsswitch.count");
	   menu_cfg_.device_speedunit.current   =   lua_cfg_->GetIntegerValue("menu.device.speedunit.current");
       menu_cfg_.device_speedunit.count   =   lua_cfg_->GetIntegerValue("menu.device.speedunit.count");
	   menu_cfg_.device_timezone.current   =   lua_cfg_->GetIntegerValue("menu.device.timezone.current");
       menu_cfg_.device_timezone.count   =   lua_cfg_->GetIntegerValue("menu.device.timezone.count");
db_error("menu_cfg_: timezone: %d %d",menu_cfg_.device_timezone.current,menu_cfg_.device_timezone.count);

	  str	   =	 lua_cfg_->GetStringValue("menu.device.carid.idstr");
	  strncpy(menu_cfg_.device_carid.string1, str.c_str(), sizeof(menu_cfg_.device_carid.string1) - 1); 
db_error("menu_cfg_: device_carid: %s",menu_cfg_.device_carid.string1);

       if(NO_DEBUG){
      db_msg("[fangjj]:menu.switch.record_eis:[%d]\n",                             menu_cfg_.switch_record_eis);
      db_msg("[fangjj]:menu.switch.record_awmd:[%d]\n",                         menu_cfg_.switch_record_awmd);
      db_msg("[fangjj]:menu.switch.record_drivingmode:[%d]\n",                menu_cfg_.switch_record_drivingmode);
      db_msg("[fangjj]:menu.switch.camera_imagerotation:[%d]\n",             menu_cfg_.switch_camera_imagerotation);	
      db_msg("[fangjj]:menu.switch.camera_ledindicator:[%d]\n",                menu_cfg_.switch_camera_ledindicator);    
      db_msg("[fangjj]:menu.switch.camera_timewatermark:[%d]\n",            menu_cfg_.switch_camera_timewatermark);	
      db_msg("[fangjj]:menu.switch.camera_distortioncalibration:[%d]\n",     menu_cfg_.switch_camera_distortioncalibration);
      db_msg("[fangjj]:menu.switch.camera_keytone:[%d]\n",                      menu_cfg_.switch_camera_keytone);	
      db_msg("[fangjj]:menu.switch.camB_preview:[%d]\n",                      menu_cfg_.camB_preview);	
      db_msg("[fangjj]:menu.switch.acc_resume:[%d]\n",                      menu_cfg_.switch_acc_resume);	
      db_msg("[fangjj]:menu.record.resolution.current:[%d], count=:[%d]\n",    menu_cfg_.record_resolution.current,menu_cfg_.record_resolution.count);
      db_msg("[fangjj]:menu.record.record_sound.current:[%d], count=:[%d]\n",    menu_cfg_.record_sound.current,menu_cfg_.record_sound.count);
      db_msg("[fangjj]:menu.record.wifisw.current:[%d], count=:[%d]\n",    menu_cfg_.record_switch_wifi.current,menu_cfg_.record_switch_wifi.count);
      db_msg("[fangjj]:menu.record.rear_resolution.current:[%d], count=:[%d]\n",    menu_cfg_.record_rear_resolution.current,menu_cfg_.record_rear_resolution.count);
      db_msg("[fangjj]:menu.record.screenbrightness.current:[%d], count=:[%d]\n",    menu_cfg_.record_screen_brightness.current,menu_cfg_.record_screen_brightness.count);
      db_msg("[fangjj]:menu.record.disturbmode.current:[%d], count=:[%d]\n",    menu_cfg_.record_screen_disturb_mode.current,menu_cfg_.record_screen_disturb_mode.count);
      db_msg("[zhb]:menu.record.voicephoto.current:[%d], count=:[%d]\n",    menu_cfg_.record_voice_take_photo.current,menu_cfg_.record_voice_take_photo.count); 
      db_msg("[zhb]:menu.record.network4g.current:[%d], count=:[%d]\n",    menu_cfg_.record_4g_network_switch.current,menu_cfg_.record_4g_network_switch.count);
      db_msg("[zhb]:menu.record.volumeselect.current:[%d], count=:[%d]\n",    menu_cfg_.record_volume_selection.current,menu_cfg_.record_volume_selection.count);
      db_msg("[zhb]:menu.record.poweronvoice.current:[%d], count=:[%d]\n",    menu_cfg_.record_power_on_sound.current,menu_cfg_.record_power_on_sound.count);
      db_msg("[zhb]:menu.record.keyvoice.current:[%d], count=:[%d]\n",    menu_cfg_.record_key_sound.current,menu_cfg_.record_key_sound.count);
      db_msg("[zhb]:menu.record.drivereport.current:[%d], count=:[%d]\n",    menu_cfg_.record_drivering_report.current,menu_cfg_.record_drivering_report.count);
      db_msg("[zhb]:menu.record.adas.current:[%d], count=:[%d]\n",    menu_cfg_.record_adas.current,menu_cfg_.record_adas.count);
      db_msg("[zhb]:menu.record.standbyclock.current:[%d], count=:[%d]\n",    menu_cfg_.record_standby_clock.current,menu_cfg_.record_standby_clock.count);
      db_msg("[zhb]:menu.record.adasfcw.current:[%d], count=:[%d]\n",    menu_cfg_.record_adas_forward_collision_waring.current,menu_cfg_.record_adas_forward_collision_waring.count);
      db_msg("[zhb]:menu.record.adaslsr.current:[%d], count=:[%d]\n",    menu_cfg_.record_adas_lane_shift_reminding.current,menu_cfg_.record_adas_lane_shift_reminding.count);
      db_msg("[zhb]:menu.record.watchdog.current:[%d], count=:[%d]\n",    menu_cfg_.record_watchdog.current,menu_cfg_.record_watchdog.count);
      db_msg("[zhb]:menu.record.probeprompt.current:[%d], count=:[%d]\n",    menu_cfg_.record_probeprompt.current,menu_cfg_.record_probeprompt.count);
      db_msg("[zhb]:menu.record.speedprompt.current:[%d], count=:[%d]\n",    menu_cfg_.record_speedprompt.current,menu_cfg_.record_speedprompt.count);
      db_msg("[zhb]:menu.record.timewatermark.current:[%d], count=:[%d]\n",    menu_cfg_.record_timewatermark.current,menu_cfg_.record_timewatermark.count);
      db_msg("[zhb]:menu.record.emerrecord.current:[%d], count=:[%d]\n",    menu_cfg_.record_emerrecord.current,menu_cfg_.record_emerrecord.count);
      db_msg("[zhb]:menu.record.emerrecordsen.current:[%d], count=:[%d]\n",    menu_cfg_.record_emerrecordsen.current,menu_cfg_.record_emerrecordsen.count);
      db_msg("[zhb]:menu.record.parkingwarnlamp.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingwarnlamp_switch.current,menu_cfg_.record_parkingwarnlamp_switch.count);
      db_msg("[zhb]:menu.record.parkingmonitor.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingmonitor_switch.current,menu_cfg_.record_parkingmonitor_switch.count);
      db_msg("[zhb]:menu.record.parkingabnormalmonitor.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingabnormalmonitory_switch.current,menu_cfg_.record_parkingabnormalmonitory_switch.count);
      db_msg("[zhb]:menu.record.parkingabnormalnotice.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingloopabnormalnotice_switch.current,menu_cfg_.record_parkingloopabnormalnotice_switch.count);
      db_msg("[zhb]:menu.record.parkingloopsw.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingloop_switch.current,menu_cfg_.record_parkingloop_switch.count);
      db_msg("[zhb]:menu.record.parkingloopresolution.current:[%d], count=:[%d]\n",    menu_cfg_.record_parkingloop_resolution.current,menu_cfg_.record_parkingloop_resolution.count);
      db_msg("[fangjj]:menu.record.encodingtype.current:[%d], count=:[%d]\n",    menu_cfg_.record_encodingtype.current,menu_cfg_.record_encodingtype.count);
      db_msg("[fangjj]:menu.record.loop.current:[%d], count=:[%d]\n",            menu_cfg_.record_loop.current,menu_cfg_.record_loop.count);
      db_msg("[fangjj]:menu.record.timelapse.current:[%d], count=:[%d]\n",     menu_cfg_.record_timelapse.current,menu_cfg_.record_timelapse.count);
      db_msg("[fangjj]:menu.record.slowmotion.current:[%d], count=:[%d]\n",  menu_cfg_.record_slowmotion.current,menu_cfg_.record_slowmotion.count);
      db_msg("[fangjj]:menu.photo.resolution.current:[%d], count=:[%d]\n",     menu_cfg_.photo_resolution.current,menu_cfg_.photo_resolution.count);
      db_msg("[fangjj]:menu.photo.timed.current:[%d], count=:[%d]\n",           menu_cfg_.photo_timed.current,menu_cfg_.photo_timed.count);    
      db_msg("[fangjj]:menu.photo.auto.current:[%d], count=:[%d]\n",             menu_cfg_.photo_auto.current,menu_cfg_.photo_auto.count);          
      db_msg("[fangjj]:menu.photo.dramashot.current:[%d], count=:[%d]\n",     menu_cfg_.photo_dramashot.current,menu_cfg_.photo_dramashot.count);     
      db_msg("[fangjj]:menu.photo.quality.current:[%d], count=:[%d]\n",     menu_cfg_.photo_quality.current,menu_cfg_.photo_quality.count);     
      db_msg("[fangjj]:menu.camera.exposure.current:[%d], count=:[%d]\n",               menu_cfg_.camera_exposure.current,menu_cfg_.camera_exposure.count); 
      db_msg("[fangjj]:menu.camera.whitebalance.current:[%d], count=:[%d]\n",          menu_cfg_.camera_whitebalance.current,menu_cfg_.camera_whitebalance.count);  
      db_msg("[fangjj]:menu.camera.lightfreq.current:[%d], count=:[%d]\n",                menu_cfg_.camera_lightfreq.current,menu_cfg_.camera_lightfreq.count);     
      db_msg("[fangjj]:menu.camera.autoscreensaver.current:[%d], count=:[%d]\n",      menu_cfg_.camera_autoscreensaver.current,menu_cfg_.camera_autoscreensaver.count);      
      db_msg("[fangjj]:menu.camera.timedshutdown.current:[%d], count=:[%d]\n",       menu_cfg_.camera_timedshutdown.current,menu_cfg_.camera_timedshutdown.count);           
      db_msg("[fangjj]:menu.camera.wifiinfo.ssid:%s\n",                                              menu_cfg_.camera_wifiinfo.string1);
      db_msg("[fangjj]:menu.camera.wifiinfo.password:%s\n",                                      menu_cfg_.camera_wifiinfo.string2);  
      db_msg("[fangjj]:menu.device.sysversion.version:%s\n",                                              menu_cfg_.device_sysversion.string1);
      db_msg("[fangjj]:menu.device.language.current:[%d], count=:[%d]\n",         menu_cfg_.device_language.current,menu_cfg_.device_language.count);    
      db_msg("[fangjj]:menu.device.updatetime.current:[%d], count=:[%d]\n",         menu_cfg_.device_datatime.current,menu_cfg_.device_datatime.count); 
      db_msg("[fangjj]:menu.device.voicestatus.current:[%d], count=:[%d]\n",      menu_cfg_.device_voicestatus.current,menu_cfg_.device_voicestatus.count);              
       }   
	  
  return 0;
  }

int MenuConfigLua::UpdateWifiInfo()
{
    if (NULL == this->lua_cfg_)
    {
        db_error("[fangjj]:The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    int ret = lua_cfg_->LoadFromFile(MENU_CONFIG_FILE);
    if (ret)
    {
        db_error("Load %s failed!", MENU_CONFIG_FILE);
        return -1;
    }

    std::string str = lua_cfg_->GetStringValue("menu.camera.wifiinfo.ssid");
    strncpy(menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string1) - 1);

    str = lua_cfg_->GetStringValue("menu.camera.wifiinfo.password"); 
    strncpy(menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string2) - 1);
    
    return 0;
}
int MenuConfigLua::SetMenuStringConfig(int msg, std::string & str)
{
    switch(msg) {
               /*****switch******/
          case MSG_SHUTDOWN_SAVE_TIME:
		strncpy(menu_cfg_.device_systemTime.string1, str.c_str(), sizeof(menu_cfg_.device_systemTime.string1) - 1); 
	   	break;
          default:
         	 break;
      }    
      SaveMenuAllConfig();
      return 0; 
} 

int MenuConfigLua::GetGpsSwith()
{
	int val = GetMenuIndexConfig(SETTING_GPS_SWITCH);
	return val;
}


int MenuConfigLua::GetTimezone()
{
	int val = GetMenuIndexConfig(SETTING_TIMEZONE);
	if (val > 12) val = val - 24;
	return val;
}
int MenuConfigLua::GetSpeedunit()
{
	int val = GetMenuIndexConfig(SETTING_SPEED_UNIT);
	return val;
}


int MenuConfigLua::UpdateSystemTime(bool flag)
{
    if(!flag)//刚开机的时候
    	system("hwclock -s");//将硬件的时间同步到系统时间
    char buf[32] = {0};
    struct tm * tm=NULL;
    time_t timer;
    timer = time(NULL);
    tm = localtime(&timer);
    snprintf(buf, sizeof(buf),"%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	//db_warn("debug_zhb---> buf = %s",buf);
    //get menu_config.lua time
    if(flag)//shutdown save timestr to menu_config.lua
   	{
		string temp;
		temp = buf;
		SetMenuStringConfig(MSG_SHUTDOWN_SAVE_TIME,temp);
   	}
	else
	{
	     string t_str;
		 t_str.clear();
   	     t_str = lua_cfg_->GetStringValue("menu.device.systemTimeSave.timeStr");
		 if( t_str.empty() )
		 {
		 	db_warn("get systemTimeSave is null");
			if(set_date_time(tm) < 0)
			{
				db_error("set system time fail");
				return -1;
	    		}
			system("hwclock -w");//将系统的时间同步到硬件时间
			return 0;
		 }
		
	     if(strncmp(t_str.c_str(),buf,strlen(t_str.c_str()))>=0)//menu_config.lua time >= system time
	    	{
			//2018-07-01 00:00:00
			struct tm tm_last;
			tm_last.tm_year = atoi((t_str.substr(0,4)).c_str()) - 1900;
			tm_last.tm_mon = atoi((t_str.substr(5,2)).c_str())-1;
			tm_last.tm_mday = atoi((t_str.substr(8,2)).c_str());
			tm_last.tm_hour = atoi((t_str.substr(11,2)).c_str());
			tm_last.tm_min = atoi((t_str.substr(14,2)).c_str());
			tm_last.tm_sec = atoi((t_str.substr(17,2)).c_str());
			tm_last.tm_wday = 0;
			tm_last.tm_yday = 0;
			tm_last.tm_isdst = 0;
			//db_warn("debug_zhb---> %04d-%02d-%02d %02d:%02d:%02d",tm_last.tm_year+1900, tm_last.tm_mon+1, tm_last.tm_mday, tm_last.tm_hour, tm_last.tm_min, tm_last.tm_sec);
			if(set_date_time(&tm_last) < 0)
			{
				db_error("set system time fail");
				return -1;
	    		}
			system("hwclock -w");//将系统的时间同步到硬件时间
	    	}
	 }
	
	return 0;
}

int MenuConfigLua::GetDeviceCaiId(std::string &name)
{
	db_error("--: %s length: %d",menu_cfg_.device_carid.string1, sizeof(menu_cfg_.device_carid.string1));	// 128
	//int len = strlen((char*)menu_cfg_.device_carid.string1);
	memcpy((char*)name.c_str(),(char*)menu_cfg_.device_carid.string1,32);
	if (name.length() == 0)
		return -1;
	//db_error("==: %s",name.c_str());
	return 0;
}

int MenuConfigLua::SetDeviceCaiId(std::string &name)
{
	//db_error("old: %s new: %s",menu_cfg_.device_carid.string1, name.c_str());
	int writelen = name.length();
	db_error("old: %s new(%d): %s",menu_cfg_.device_carid.string1, writelen, name.c_str());
	if (writelen > 32) writelen = 32;
	char tmp[32];
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,(char*)name.c_str(),name.length());
	
	memset((char*)menu_cfg_.device_carid.string1,0,128);
	memcpy((char*)menu_cfg_.device_carid.string1,(char*)tmp,32);
	if (name.length() == 0)
		return -1;
	db_error("==: %s",tmp);
	return 0;
}

int MenuConfigLua::SetWifiSsid(std::string str)
{
     strncpy(menu_cfg_.camera_wifiinfo.string1, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string1) - 1);
     lua_cfg_->SetStringValue("menu.camera.wifiinfo.ssid",  menu_cfg_.camera_wifiinfo.string1);
     return 0;
}

int MenuConfigLua::SetWifiPassword(std::string str)
{
     strncpy(menu_cfg_.camera_wifiinfo.string2, str.c_str(), sizeof(menu_cfg_.camera_wifiinfo.string2) - 1);
     lua_cfg_->SetStringValue("menu.camera.wifiinfo.password",  menu_cfg_.camera_wifiinfo.string2); 
     return 0;
}



