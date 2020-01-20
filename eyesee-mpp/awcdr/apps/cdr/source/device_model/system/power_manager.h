/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file power_manager.cpp
 * @brief support power manager function for app
 * @author id:848
 * @version v0.3
 * @date 2016-10-24
 */

#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H
#include "common/message.h"
#include <utils/Thread.h>
#include <utils/Mutex.h>
#include <cutils/android_reboot.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include "common/subject.h"

using namespace EyeseeLinux;

namespace EyeseeLinux {

#define DE2

enum
{
    SCREEN_ON = 0,
    SCREEN_OFF,
};

#ifdef DE2
typedef enum
{
    DISP_OUTPUT_TYPE_NONE   = 0,
    DISP_OUTPUT_TYPE_LCD    = 1,
    DISP_OUTPUT_TYPE_TV     = 2,
    DISP_OUTPUT_TYPE_HDMI   = 4,
    DISP_OUTPUT_TYPE_VGA    = 8,
}disp_output_type_;
#endif

enum{
    BATTERY_CHARGING = 1,
    BATTERY_DISCHARGING,
    BATTERY_NOTCHARGING,
    BATTERY_FULL
};

#define DISP_DEV "/dev/disp"

#define BATTERY_HIGH_LEVEL 4
#define BATTERY_LOW_LEVEL 1
#define BATTERY_CHANGE_BASE 0

#define CPU_FREQ    "480000"
#define CPU_TEMPERATURE_FILE  "/sys/class/thermal/thermal_zone0/temp"

#define TIME_INFINITE 0xFFFFFF

#define SCALING_MAX_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define SCALING_GOVERNOR  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define SCALING_SETSPEED  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"

#define CPUINFO_CUR_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

#define BATTERY_STATUS_FILE         "/sys/class/power_supply/battery/status"
#define BATTERY_ONLINE_FILE     "/sys/class/power_supply/battery/online"
#define BATTERY_CAPACITY            "/sys/class/power_supply/battery/capacity"

#define USB_CONNECT_STATUS "/sys/devices/virtual/android_usb/android0/state"
#define AC_ONLINE_STATUS "/sys/class/power_supply/ac/online"
#define USB_ONLINE_STATUS "/sys/class/power_supply/usb/online"
#define UDC_CONFIG_NODE "/sys/kernel/config/usb_gadget/g1/UDC"
#define AHD_ONLINE_STATUS "/sys/class/csi/tp9950/online"
#define POWER_ON_STATUS "/sys/class/axp/axp_reg"


#define DISP_CMD_LCD_ON     0x140
#define DISP_CMD_LCD_OFF    0x141
#define DISP_PW_BLANK          0x0C

#define CPU_TEMP_HIGH_LEVEL 75
#define BAT_DETECT_TIME 1

#define CPUS_WAKEUP_LOWBATT     (1<<12)                                       
#define CPUS_WAKEUP_USB         (1<<13)                                       
#define CPUS_WAKEUP_AC          (1<<14)                                       
#define CPUS_WAKEUP_ASCEND      (1<<15)                                       
#define CPUS_WAKEUP_DESCEND     (1<<16)                                       
#define CPUS_WAKEUP_SHORT_KEY   (1<<17)                                       
#define CPUS_WAKEUP_LONG_KEY    (1<<18)                                       
#define CPUS_WAKEUP_IR          (1<<19)                                       
#define CPUS_WAKEUP_ALM0        (1<<20)                                       
#define CPUS_WAKEUP_ALM1        (1<<21)                                       
#define CPUS_WAKEUP_TIMEOUT     (1<<22)                                       
#define CPUS_WAKEUP_GPIO        (1<<23)                                       
#define CPUS_WAKEUP_USBMOUSE    (1<<24)                                       
#define CPUS_WAKEUP_LRADC       (1<<25)                                       
#define CPUS_WAKEUP_WLAN        (1<<26)                                       
#define CPUS_WAKEUP_CODEC       (1<<27)                                       
#define CPUS_WAKEUP_BAT_TEMP    (1<<28)                                       
#define CPUS_WAKEUP_FULLBATT    (1<<29)                                       
#define CPUS_WAKEUP_HMIC        (1<<30)                                       
#define CPUS_WAKEUP_POWER_EXP   (1<<31)                                       
#define CPUS_WAKEUP_KEY         (CPUS_WAKEUP_SHORT_KEY | CPUS_WAKEUP_LONG_KEY)

/* define cpus wakeup src */                            
#define CPUS_MEM_WAKEUP          (CPUS_WAKEUP_LOWBATT  \
                                | CPUS_WAKEUP_USB      \
                                | CPUS_WAKEUP_AC       \
                                | CPUS_WAKEUP_DESCEND  \
                                | CPUS_WAKEUP_ASCEND   \
                                | CPUS_WAKEUP_ALM0     \
                                | CPUS_WAKEUP_GPIO     \
                                | CPUS_WAKEUP_IR)  

#define WAKE_UP_EVENT_DEV  "/sys/power/sunxi/wakeup_event"								
								
enum scaling_mode
{
    PERFORMANCE = 0,
    USERSPACE,
    POWERSAVE,
    ONDEMAND,
    CONSERVATIVE,
};

class PowerManager
    : public ISubjectWrap(PowerManager)
{
public:
    PowerManager();
    ~PowerManager();
    static PowerManager* GetInstance();
    void SetOffTime(unsigned int time);
    void Pulse();
    void PowerOff();
    int  ScreenOff();
    int  ScreenOn();
    int  ReadyToOffScreen();
    void ScreenSwitch();
    int  GetBattery();
    int  GetBatteryLevel(void);
    int  GetBatteryStatus();
    void Reboot();
    bool IsScreenOn();
    void AdjustCpuFreq();
    int  getStandby_status();
    void setStandby_status(bool flag);
    void Connect2StandbyService();
    //void EnterStandby();
    void WatchDogRun();
    void WatchdogThread();
    void EnterStandby(int pid=-1);
    void enterStandbyModem();
    int  standbySuspend();
    int  standbyResume();
    void standbyOver();
    bool get_sd_standby();
    void set_sd_exit_standby();
    void SetEnterOTAService(bool flag);
    void ScreenSwitchThread();
    void PowerOffThread();
    void WatchDogThread();
    bool getStandbyFlag();
    void notifyResume();
    void waitResume();
    static void *EventLoopThread(void *context);
    void get_oneline(const char *pathname, std::string &val);
    bool getUsbconnectStatus();
    bool getACconnectStatus();
    int getcapacity();
    static void* BatteryDetectThread(void *context);
    int resetScreenStatus();
    void ResetUDC();
    int PrepareToSuspend();        
	int ReadyToStandy();        
	int StartResume();	
	static void *StandByDetechThread(void *context);
	int SetBrightnessLevel(int level);
	int GetWakeUpSource();
	int SetOperationAfterWakeUp();
	void setStandbyFlag(bool flag);	
	void CloseStandbyDialog();
    int getAhdInsertOnline();
    int getPowenOnType();
private:
	bool need_wakeup_;
    int mdisp_fd_;
    unsigned int moff_time_;
    int mstate_;
    int mbattery_level_;
    Mutex mlock_;
    //Condition mCon;
    int poweroff();
    bool is_enter_standby_;
    bool isEnterStandby;
    bool isEnterStandby2;
	bool standbyFlag_;
    bool standbyFlag;
    bool mEnterStandby;
    bool mRestoreScreenOn;
    bool m_sd_standby;
    Mutex mwLock;
    Condition mwCon;
    pthread_t power_event_loop_thread_id_;    
    pthread_t m_battery_thread_id;
	pthread_t mStandByDetechThreadId;
    bool m_bBatDetectThreadExit;
    bool m_bEventDetectThreadExit;
	int mAcIN;
	int mAcDetValue;
	bool mbEnterStandbyFlag;
	bool mbLoopRecordFlag;
	bool mbStandByDetechThreadExitFlag;
};
}
#endif
