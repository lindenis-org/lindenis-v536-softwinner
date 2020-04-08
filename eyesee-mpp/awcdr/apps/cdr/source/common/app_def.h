#pragma once

#include "common/app_platform.h"

// #define DEBUG_UI
#define EXIT_APP    0
#define POWEROFF    1
#define REBOOT      2

#if FOX_V7
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   376
#define SCREEN_INFO     "240x376-32bpp"
#elif FOX_V8
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   432
#define SCREEN_INFO     "240x432-32bpp"
#elif FOX_V9
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   320
#define SCREEN_INFO     "240x320-32bpp"
#elif FOX_V10
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   640
#define SCREEN_INFO     "480x640-32bpp"
#elif FOX_V11
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   854
#define SCREEN_INFO     "480x854-32bpp"
#elif FOX_C26A
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   640
#define SCREEN_INFO     "480x640-32bpp"
#elif LD720P
#define SCREEN_WIDTH    720
#define SCREEN_HEIGHT   1280
#define SCREEN_INFO     "720x1280-32bpp"
#define DIALOG_WIDTH    640
#define GUI_SCN_WIDTH    SCREEN_HEIGHT
#define GUI_SCN_HEIGHT    SCREEN_WIDTH
#elif LD1080P
#define SCREEN_WIDTH    1920
#define SCREEN_HEIGHT   1080
#define SCREEN_INFO     "1920x1080-32bpp"
#define DIALOG_WIDTH    640
#define GUI_SCN_WIDTH    SCREEN_WIDTH
#define GUI_SCN_HEIGHT    SCREEN_HEIGHT
#else
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   376
#define SCREEN_INFO     "240x376-32bpp"
#endif

#define MODEL_PREFIX	"TF700"		// "V536-CDR"

#define USE_CAMA

#define USE_CAMB
#define CAMB_PREVIEW

//#define USE_IMX335				//使用imx335打开宏定义
#define LONGPRESSTIME_FOR_PWR	250		//unit: 10ms

//#define SUPPORT_AUTOHIDE_STATUSBOTTOMBAR

#define USEICONTHUMB
#define PHOTO_MODE

#define LCD_BRIGHTNESSLEVEL		1	// 0 最亮 ~ 6 最暗
//#define SUPPORT_RECTIMELAPS			// 缩时录影
#define VIDEOTYPE_MP4				// if def = mp4 otherwise ts	
#define SUPPORT_CHANGEPARKFILE

//#define MEM_DEBUG

//#define SUPPORT_TFCARDIN_RECORD	// 插卡自动录像
#define INFO_ZD55

#define RTCASUTC		// 使用rtc时间为utc时间
//#define TF653_KEY       //使用tf653按键定义

//#define GSENSOR_SUSPEND_ENABLE	// 允许GSENSOR 进入挂起模式(0.7uA最省电)

#define PROMPT_POWEROFF_TIME	5	// 5
#define SUPPORT_MODEBUTTON_TOP		// 模式切换键在左上角(公版),否则在底部状态栏(致君)

//#define SUPPOTR_SHOWSPEEDUI		// 在预览界面显示当前速度
//#define SUPPORT_RECORDTYPEPART		// 录像文件分区

#define STOPCAMERA_TO_PLAYBACK      // 进入playback关闭camera

//#define SUPPORT_PSKIP_ENABLE		// 打开 允许插空帧, 关闭 不插空帧

#define RECORDER_FIXSIZE      // 固定录像文件尺寸
