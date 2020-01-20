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
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "power_manager.cpp"
#endif
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common/app_log.h"
#include "power_manager.h"
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include "tools/udev_message.h"
#include "common/utils/utils.h"
#include "device_model/display.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/reboot.h>

#include <mutex>

using namespace std;
using namespace EyeseeLinux;
#define DEBUG_STANDBY 1
//#define WAKEUP_REBOOT
static Mutex pm_mutex;
static PowerManager *gpower_manager_ = NULL;
static mutex screen_mutex;

PowerManager::PowerManager()
    : mdisp_fd_(-1),
      moff_time_(TIME_INFINITE),
      mstate_(SCREEN_ON),
      mbattery_level_(BATTERY_HIGH_LEVEL),
      m_bBatDetectThreadExit(false),
      m_bEventDetectThreadExit(false)
{
    mdisp_fd_ = open(DISP_DEV, O_RDWR);
    if (mdisp_fd_ < 0) {
        db_msg("fail to open %s", DISP_DEV);
    }
	need_wakeup_ = false;
    is_enter_standby_=true;   //no use
	isEnterStandby=true;
	isEnterStandby2=true;
	standbyFlag_ = false;
	mAcIN = -1;
	mAcDetValue = -1;
	mbEnterStandbyFlag = false;
	mbLoopRecordFlag = false;
	mbStandByDetechThreadExitFlag = false;
    //ScreenSwitchThread(this);     /*creat screen switch thread,  close screen when timeout*/
    //WatchDogRun();
	//ThreadCreate(&power_event_loop_thread_id_, NULL, PowerManager::EventLoopThread, this);
	//system("echo normalstandby > /sys/power/wake_lock");  //acuire one wake_lock.
	system("echo standby > /sys/power/wake_lock");  //acuire one wake_lock.
    ThreadCreate(&m_battery_thread_id, NULL, PowerManager::BatteryDetectThread, this);

	ThreadCreate(&mStandByDetechThreadId,NULL,PowerManager::StandByDetechThread,this);
}

PowerManager::~PowerManager()
{
    //xxxx; /*close screen switch thread*/
    //yyyy; /*close watchdog thread*/
    mbStandByDetechThreadExitFlag = true;
	if( mStandByDetechThreadId > 0 )
		pthread_cancel(mStandByDetechThreadId);

    m_bEventDetectThreadExit = true;
    if( power_event_loop_thread_id_ > 0 )
        pthread_cancel(power_event_loop_thread_id_);

    m_bBatDetectThreadExit = true;
    if( m_battery_thread_id > 0 )
       pthread_cancel(m_battery_thread_id);
    close(mdisp_fd_);
}

PowerManager* PowerManager::GetInstance(void)
{
    Mutex::Autolock _l(pm_mutex);
    if(gpower_manager_ != NULL) {
        return gpower_manager_;
    }
    gpower_manager_ = new PowerManager();
    return gpower_manager_;
}

int PowerManager::SetOperationAfterWakeUp()
{
    int wakeup_event_id = 0;
    char buf[32] = {0};
    char *endptr = NULL;

	/* if the wakeup_event exist*/
    if (!FILE_EXIST(WAKE_UP_EVENT_DEV) || !FILE_READABLE(WAKE_UP_EVENT_DEV)) {
        db_error("file not exist or can not access");
        return -1;
    }

    FILE *fp = NULL;

    fp = fopen(WAKE_UP_EVENT_DEV, "r");

    if (fp == NULL) {
        db_error("open file failed, %s", strerror(errno));
        return -1;
    }
	/* get value ,value is string*/
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        buf[(sizeof(buf))-1] = '\0';
    } else {
        fclose(fp);
        return -1;
    }

    fclose(fp);
	/*string to hex*/
    wakeup_event_id = strtol(buf, &endptr, 16);

    db_msg("the buf is %s, wakeup_event_id is 0x%x", buf, wakeup_event_id);
	/* wakeup source whether if rtc timer*/
    if (wakeup_event_id & CPUS_WAKEUP_ALM0) {
        db_msg("wakeup by alarm, enter the shut down process!!!");
    } else {
        //system("echo +0 >/sys/class/rtc/rtc0/wakealarm");
		/* wakeup source whether batter power low*/
        if (wakeup_event_id & CPUS_WAKEUP_LOWBATT) {
            db_msg("wakeup by low power, enter the shut down process!!!");
		/* wakeup source whether pwer key*/
        } else if (wakeup_event_id & CPUS_WAKEUP_DESCEND) {
            db_msg("wakeup by power key, normal work");
		/* wakeup source whether */
        } else if (wakeup_event_id & CPUS_WAKEUP_KEY) {
            db_msg("wakeup by gpio ok key");
        } else {
            db_warn("the other wakeup source");
        }
    }

    return 0;
}

int PowerManager::GetWakeUpSource()
{
    int wakeup_event_id = 0;
    char buf[32] = {0};
    char *endptr = NULL;

	/* if the wakeup_event exist*/
    if (!FILE_EXIST(WAKE_UP_EVENT_DEV) || !FILE_READABLE(WAKE_UP_EVENT_DEV)) {
        db_error("file not exist or can not access");
        return -1;
    }

    FILE *fp = NULL;

    fp = fopen(WAKE_UP_EVENT_DEV, "r");

    if (fp == NULL) {
        db_error("open file failed, %s", strerror(errno));
        return -1;
    }
	/* get value ,value is string*/
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        buf[(sizeof(buf))-1] = '\0';
    } else {
        fclose(fp);
        return -1;
    }

    fclose(fp);
	/*string to hex*/
    wakeup_event_id = strtol(buf, &endptr, 16);


    return wakeup_event_id;
}


void *PowerManager::StandByDetechThread(void * context)
{
	PowerManager *pm = reinterpret_cast<PowerManager*>(context);
	while(!pm->mbStandByDetechThreadExitFlag)
	{
		sleep(1);
		//1:获取ACC电压值， 获取ACC Detect值
		pm->mAcIN = 1;
		pm->mAcDetValue = 1;
		if(!pm->mAcIN && !pm->mbEnterStandbyFlag)
		{
//			pm->EnterStandby();
			pm->Notify(MSG_PM_RECORD_STOP);
			pm->mbEnterStandbyFlag = true;
		}
		else if( pm->mAcIN )
		{
			//1:wakeup from standby
			if( pm->mbEnterStandbyFlag )
			{
//				pm->StartResume();
				pm->Notify(MSG_PM_RECORD_START);
				pm->mbEnterStandbyFlag = false;
			}
			//2:step in to loop record
			else if(!pm->mAcDetValue && !pm->mbLoopRecordFlag)
			{
				//start loop record
				pm->mbLoopRecordFlag = true;
			}
		}
	}

	return NULL;
}

static inline int capacity2Level(int capacity)
{
    return ((capacity - 1) / 20 + 1);
}

int PowerManager::PrepareToSuspend()
{    
	Notify(MSG_PREPARE_TO_SUSPEND);    
	return 0;
}

int PowerManager::ReadyToStandy()
{   
	// release wake_lock before enter standy mode    
	system("echo standby > /sys/power/wake_unlock");
	#if DEBUG_STANDBY    
	system("echo N > /sys/module/printk/parameters/console_suspend");    
	system("echo Y > /sys/module/kernel/parameters/initcall_debug");
	#endif    
	return 0;
}

int PowerManager::StartResume()
{    
	system("echo standby > /sys/power/wake_lock");    
	Notify(MSG_START_RESUME);
	//#if DEBUG_STANDBY    
	//system("cat /proc/meminfo | grep -E \"(Mem*|Commit*|Vmalloc*)\"");
	//#endif    
	need_wakeup_ = false;    
	return 0;
}

void PowerManager::EnterStandby(int pid)
{
    db_error("Notify MSG_SYSTEM_POWEROFF");
    Notify(MSG_SYSTEM_POWEROFF);//("poweroff");
    return;
	if(!need_wakeup_){
	    db_msg("****enterStandby****");
		need_wakeup_ = true;	
	    PrepareToSuspend();  
        if (mstate_ == SCREEN_ON) {
        	ScreenOff();
        }
//		usleep(2000*1000);
		ReadyToStandy();    
		db_msg("enter super standy mode start");   
		system("echo mem > /sys/power/state"); 
		db_msg("enter super standy mode temp 1"); 
#ifdef WAKEUP_REBOOT
		//system("reboot -f");
		db_error("============reboot============");
		reboot(RB_AUTOBOOT);
		return;
#endif
		StartResume();	
		db_msg("enter super standy mode end"); 
	}
    //mstandby_service_->enterStandby(pid);
}

void PowerManager::CloseStandbyDialog()
{        
	Notify(MSG_CLOSE_STANDBY_DIALOG);
}


void PowerManager::Connect2StandbyService()
{
    return ;
}

int PowerManager::GetBattery()
{
    string val = "";
    int battery_capacity = 0;
#if 0
    get_oneline(BATTERY_ONLINE_FILE, val);
   if (atoi(val.c_str()) != 1) {  //not use battery
       // db_msg("has not use battery");
        return -1;
    }
#endif
    get_oneline(BATTERY_CAPACITY, val);
    battery_capacity = atoi(val.c_str());
    return battery_capacity;
}

int PowerManager::getcapacity()
{
    string val = "";
    get_oneline(BATTERY_CAPACITY, val);
    return atoi(val.c_str());
}

int PowerManager::GetBatteryLevel()
{
    int cap = GetBattery();
    if(cap<=0){
       // mbattery_level_ = 4;
        return mbattery_level_;//直接返回上次的状态
    }else{
        int val = cap/25;
        switch(val){
            case 0:
                mbattery_level_ = 0;
                break;
            case 1:
                mbattery_level_ = 1;
                break;
            case 2:
                mbattery_level_ = 2;
                break;
            case 3:
                mbattery_level_ = 3;
                break;
            default:
                mbattery_level_ = 3;
        }
    }
    return mbattery_level_;
}

int PowerManager::GetBatteryStatus()
{
    char status[512]={0};
    FILE * fp;
    if(access(BATTERY_STATUS_FILE, F_OK) != 0) {
        return -1;
    }

    if(access(BATTERY_STATUS_FILE, R_OK) != 0) {
        db_msg("/sys/class/power_supply/battery/status can not read");
        return -1;
    }

    fp = fopen(BATTERY_STATUS_FILE, "r");
    if (fp == NULL) {
        return -1;
    }

    if(fgets(status, sizeof(status), fp) == NULL) {
        db_msg("/sys/class/power_supply/battery/status read failed");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    status[strlen(status)-1]='\0';
    if(strcmp(status,"Charging")==0){
        return BATTERY_CHARGING;
    }else if(strcmp(status,"Discharging")==0){
        return BATTERY_DISCHARGING;
    }else if(strcmp(status,"Not charging")==0){
        return BATTERY_NOTCHARGING;
    }else if(strcmp(status,"Full")==0){
        return BATTERY_FULL;
    }
    return 0;

}

void PowerManager::SetOffTime(unsigned int time)
{
    Mutex::Autolock _l(mlock_);

    moff_time_ = time;
    Pulse();
}

void PowerManager::Pulse()
{
    db_msg("pulse");
    //mcon_.signal();
}

void PowerManager::standbyOver()
{
	Mutex::Autolock _l(mlock_);
	standbyFlag = false;
	if (mRestoreScreenOn) {
		ScreenOn();
	}
}

bool  PowerManager::get_sd_standby()
{
	return m_sd_standby;
}

void  PowerManager::set_sd_exit_standby()
{
	m_sd_standby = false;
}

void PowerManager::ScreenSwitch()
{
    lock_guard<mutex> lock(screen_mutex);
    if (mstate_ == SCREEN_ON) {
        screen_mutex.unlock();
        ScreenOff();
    } else {
        screen_mutex.unlock();
        ScreenOn();
    }
    Pulse();
}

bool PowerManager::IsScreenOn()
{
    if (screen_mutex.try_lock() == false) {
        db_warn("get screen stat failed, just return screen off");
        return false;
    }

    screen_mutex.unlock();
    return (mstate_ == SCREEN_ON);
}

void PowerManager::PowerOff()
{
    //mpo_ = new PowerOffThread(this);  //power off thread
    //mpo_->start();
    db_msg("power off");
    system("poweroff");
}

int PowerManager::poweroff()
{
	db_error("power off");
//	screenOff();
	//android_reboot(ANDROID_RB_POWEROFF, 0, 0);
	db_error("power off!");

	return 0;
}

void PowerManager::Reboot()
{
    //android_reboot(ANDROID_RB_RESTART, 0, 0);
}


int PowerManager::getStandby_status()
{
	return isEnterStandby2;
}

void PowerManager::setStandby_status(bool flag)
{
	isEnterStandby2 = flag;
}

void PowerManager::get_oneline(const char *pathname, string &val)
{
	FILE * fp;
	char linebuf[1024];

	if(access(pathname, F_OK) != 0) {
		val = "";
		return ;
	}

	if(access(pathname, R_OK) != 0) {
		//db_msg("%s can not read", pathname);
		val = "";
		return ;
	}

	fp = fopen(pathname, "r");

	if (fp == NULL) {
		val = "";
		return;
	}

	if(fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		fclose(fp);
		val = linebuf;
		return ;
	}

	fclose(fp);
}

void PowerManager::waitResume(void)
{
	db_msg("wait for resume signal");

#ifdef AUTO_NORMAL_STANDBY
  	int ret = 0;
//	sleep(11);
	while(1) {
		string val;
		usleep(200 * 1000);
		get_oneline("/sys/module/pm_tmp/parameters/normal_standby_autotest", val);
		db_msg(" xxxxxx val:%s ", val.c_str());
		if(atoi(val.c_str()) == 1) {
			db_msg("BREAK!!!");
			break;
		}
	}
    ret = system("echo normalstandby > /sys/power/wake_lock");

	db_msg("[richard debug]step2 waitResume wake_lock ret = [ %d ]",ret);

	ret = system("echo 0 > /sys/module/pm_tmp/parameters/normal_standby_autotest");

	db_msg("[richard debug]step1 waitResume ret = [ %d ]",ret);

#else
    mwCon.wait(mwLock);
#endif
    db_msg("[richard debug] standbySuspend sleep step1 !!!!!!!");
}

void PowerManager::notifyResume()
{
	int ret = 0;

	ret = system("echo normalstandby > /sys/power/wake_lock");
#if  1

	db_msg("[richard debug]step notifyResume 1 ret = [ %d ]",ret);

    ret = system("echo on > /sys/power/state");   //richard debug
	db_msg("[richard debug]step 5  notifyResume ret = [ %d ]",ret);
#endif

#ifdef AUTO_NORMAL_STANDBY
    ret = system("echo 1 > /sys/module/pm_tmp/parameters/normal_standby_autotest");
	db_msg("[richard debug]step 1  notifyResume ret = [ %d ]",ret);

#endif

	db_msg("notify resume!!");
	mwCon.signal();
}

static int listdir(const char *path)
{
    struct dirent *entry;
    DIR *dp;

    dp = opendir(path);
    if (dp == NULL)
    {
        perror("opendir");
        return -1;
    }

    while((entry = readdir(dp)))
    {
        db_msg("listdir = %s",entry->d_name);
    }

    closedir(dp);
    return 0;
}

static int file_exist(char *path)
{
	if (access(path, F_OK) == 0) {
		return 1;
	} else
		return -1;
}


static int debug_get_info(){
	   int total = 0;
       char line[128];
       FILE *fp = fopen("/proc/meminfo", "r");
       char num[32]="0";
       if(fp==NULL){
        db_msg("open file failed.");
        return 0;
       }
       while(fgets(line, sizeof(line), fp)) {
               if (strncmp(line, "MemFree", strlen("MemFree")) == 0) {
                        puts(line);
                        char *str = strstr(line, ":");
                        str += 1;
                        sscanf(str, "%128[^a-z]", num);
                        break;
               }
       }
       fclose(fp);
	   total = atoi(num);
	   db_msg("[richard debug3]meminfo free memory =[%8d KB]",total);
       return total;
}


void PowerManager::enterStandbyModem()
{
	standbyFlag = true;
	standbySuspend();
	isEnterStandby2  = false;

    int ret = 0;
	int cnt = 0;
	m_sd_standby = true ;

    db_msg("[richard debug2] meminfo enter stanby start...");
	debug_get_info();

	db_msg("[richard debug] 2015_08_28");
	listdir("/sys/power/");

    char *msg_wanklock, *msg_normal_standby_autotest, *msg_state;
    msg_wanklock = (char *)"/sys/power/wake_unlock";
    msg_normal_standby_autotest = (char *)"/sys/module/pm_tmp/parameters/normal_standby_autotest";
    msg_state = (char *)"/sys/power/state";
    if( -1 == file_exist(msg_wanklock)) {

//   if( -1 == file_exist("/sys/power/wake_unlock")) {
	    do {
			if( (cnt == 0) || (cnt == 2) || (cnt == 7)) {
				listdir("/sys/power/");
				db_msg("cnt = [%d]",cnt);
			}
			cnt++;
			usleep(200*1000);
			if(-1 != file_exist(msg_normal_standby_autotest)) {
				break;
			}
		}while(cnt < 8);
		cnt = 0;
	}

	ret = system("echo normalstandby > /sys/power/wake_unlock");
	db_msg("[richard debug]step 1-1 => ret = [ %d ]",ret);
	if(ret == -1) {
		 usleep(20*1000);
		 system("echo normalstandby > /sys/power/wake_unlock");
	}
	db_msg("[richard debug]step 1 ret = [ %d ]",ret);

	if( -1 == file_exist(msg_normal_standby_autotest)) {
		do {
			if( (cnt == 0) || (cnt == 2) || (cnt == 7)) {
				listdir("/sys/module/pm_tmp/parameters/");
				db_msg("cnt = [%d]",cnt);
				}
			cnt++;
			usleep(200*1000);
			if(-1 != file_exist((char *)"msg_normal_standby_autotest")) {
				break;
			}
		}while(cnt < 8);
		cnt = 0;
	}

    ret = system("echo 0 > /sys/module/pm_tmp/parameters/normal_standby_autotest");
	if ( -1 == ret) {
		 usleep(20*1000);
		 system("echo 0 > /sys/module/pm_tmp/parameters/normal_standby_autotest");
	}

    db_msg("[richard debug]step 2 ret = [ %d ]",ret);

	ret = system("echo N > /sys/module/printk/parameters/console_suspend");
    db_msg(" [richard debug ]standbySuspend!!!!!!! ret =%d",ret);

	ret = system("echo Y > /sys/module/kernel/parameters/initcall_debug");
	db_msg(" [richard debug ]standbySuspend!!!!!!! ret =%d",ret);

#ifdef AUTO_NORMAL_STANDBY
	ret = system("echo 30000 > /sys/module/pm_tmp/parameters/time_to_wakeup"); // 30000 ===>> 30s
#endif

	if( -1 == file_exist(msg_state)) {
		do {
			if( (cnt == 0) || (cnt == 2) || (cnt == 7)) {
				listdir("/sys/power/");
				db_msg("cnt = [%d]",cnt);
			}
			cnt++;
			usleep(200*1000);
			if(-1 != file_exist(msg_state)) {
				break;
			}
		}while(cnt < 8);
		cnt = 0;
	}

    ret = system("echo mem > /sys/power/state");
	if(ret == -1) {

		do {
			db_msg("[richard debug]step 4-1 try ret = [ %d ]",ret);
			usleep(20*1000);
			ret = system("echo mem > /sys/power/state");
		}while(ret == -1 && (cnt++ < 8) );
		cnt = 0;
	}
	db_msg("[richard debug]step 4 ret = [ %d ]",ret);

	waitResume();
    db_msg(" [richard debug ]824 exit standby and enter resume now !!!!!!!");

	if( -1 == file_exist(msg_state)) {
			do {
				if( (cnt == 0) || (cnt == 2) || (cnt == 7)) {
					listdir("/sys/power/");
					db_msg("cnt = [%d]",cnt);
				}
				cnt++;
				usleep(200*1000);
				if(-1 != file_exist(msg_state)) {
					break;
				}
			}while(cnt < 8);
			cnt = 0;
		}
	ret = system("echo on > /sys/power/state");   //richard debug
	if(ret == -1) {
		db_msg("[richard debug]step 5 echo on try ret = [ %d ]",ret);
		do {
				usleep(20*1000);
				ret = system("echo on > /sys/power/state");
		}while((ret == -1)&& (cnt++ < 8));
	}

	db_msg("[richard debug]step 5 ret = [ %d ]",ret);
	db_msg(" standbyResume step111!!!!!!!");
    standbyResume();
	db_msg(" standbyResume!!!!!!!");
	db_msg("[richard debug2] meminfo standbyResume...");
	debug_get_info();
}

int PowerManager::standbySuspend()
{
	if (mEnterStandby) {
		mRestoreScreenOn = true;
	}
	int ret,args[4]={0};
	args[0] = 0;
	args[1] = 1;
	ret = ioctl(mdisp_fd_, DISP_PW_BLANK, args);
	mEnterStandby = true;
	return ret;
}

int PowerManager::standbyResume()
{
	int ret,args[4]={0};
	args[0] = 0;
	args[1] = 0;
	ret = ioctl(mdisp_fd_, DISP_PW_BLANK, args);
	mEnterStandby = false;
	return ret;
}

bool PowerManager::getStandbyFlag()
{
	return standbyFlag_;
	//return need_wakeup_;
}

void PowerManager::setStandbyFlag(bool flag)
{
	standbyFlag_ = flag;
}


int PowerManager::ScreenOff()
{
    lock_guard<mutex> lock(screen_mutex);
    db_msg("screenOff");
    int retval = 0;

#ifdef NORMAL_STANDBY
        standbySuspend();
        db_msg("standbySuspend```");
#else
        unsigned long args[4]={0};
        args[1] = DISP_OUTPUT_TYPE_NONE;
        retval = ioctl(mdisp_fd_, DISP_DEVICE_SWITCH, args);
#endif

    if (retval < 0) {
        db_msg("fail to set screen on");
        return -1;
    }
    mstate_ = SCREEN_OFF;
    return 0;
}

int PowerManager::ScreenOn()
{
    if (screen_mutex.try_lock() == false) {
        db_warn("screen on is in the response...");
        return -1;
    }
    screen_mutex.unlock();

    lock_guard<mutex> lock(screen_mutex);
    db_msg("screenOn");
    int retval = 0;

#ifdef NORMAL_STANDBY
        standbyResume();
        db_msg("standbyResume```");
#else
        unsigned long args[4]={0};
        args[1] = DISP_OUTPUT_TYPE_LCD;
        retval = ioctl(mdisp_fd_, DISP_DEVICE_SWITCH, args);
#endif

    if (retval < 0) {
        db_msg("fail to set screen on");
        return -1;
    }

    mstate_ = SCREEN_ON;
    return 0;
}

int PowerManager::ReadyToOffScreen()
{
    db_msg("readyToOffScreen");
    mlock_.lock();
    status_t result = 0;
    //unsigned int time = 0;
    db_msg("waitRelative 0x%x", moff_time_);
    //time = (mstate_ == SCREEN_ON) ? moff_time_ : TIME_INFINITE;
    //result  = mcon_.waitRelative(mlock_, seconds(time));
    if (result != NO_ERROR) { //timeout
        db_msg("timeout");
        if (mstate_ == SCREEN_ON) {
                ScreenOff();
        }
    } else {
        db_msg("signaled");
    }
    mlock_.unlock();
    return 0;
}

int set_scaling_governor(int mode)
{
    int fd = 0;
    int ret = -1;
    char scale_mode[20];

    fd =  open(SCALING_GOVERNOR, O_RDWR);
    if(fd < 0) {
        db_msg("open SCALING_GOVERNOR fail");
        return -1;
    }
    memset(scale_mode,0,sizeof(scale_mode));
    if(mode == PERFORMANCE)
    {
        strncpy(scale_mode,"performance",strlen("performance"));
    }
    else if(mode == USERSPACE) {
        strncpy(scale_mode,"userspace",strlen("userspace"));
//      db_msg("scale_mode=%s",scale_mode);
    }
    else {
        strncpy(scale_mode,"userspace",strlen("userspace"));
    }
    ret = write(fd, scale_mode, strlen(scale_mode)+1);
//  ret = write(fd, "userspace", strlen("userspace"));

    if(ret > 0)
        db_msg("write SCALING_GOVERNOR success!");
    else {
        db_msg("write SCALING_GOVERNOR fail");
        db_msg("Error (%s),ret=%d,fd=%d",strerror(errno),ret,fd);
    }
    close(fd);
    return (ret > 0) ? 0 : -1;
}

int set_scaling_speed(const char *freq)
{
    int fd = 0;
    int ret = -1;
//  char freq[10]={"640000"};
    fd =  open(SCALING_SETSPEED, O_RDWR);
    if(fd < 0) {
        db_msg("open SCALING_SETSPEED fail");
        return -1;
    }
    ret = write(fd, freq, strlen(freq));
    if(ret > 0)
        db_msg("write cpu_freq_value success!");
    else {
        db_msg(" write Error (%s),ret=%d,fd=%d ",strerror(errno),ret,fd);
    }
    close(fd);
    return (ret > 0) ? 0 : -1;
}

unsigned int get_scaling_speed( )
{
    int fd = 0;
    int ret = -1;
    char cpu_freq_value[10] ="1007000";;
    fd =  open(CPUINFO_CUR_FREQ, O_RDONLY);
    if(fd < 0) {
        db_msg("open CPUINFO_CUR_FREQ fail");
        return -1;
    }
    ret = read(fd, cpu_freq_value, strlen(cpu_freq_value));
    if(ret > 0)
        db_msg("read cpu_freq_value success!");
    else {
        db_msg(" read Error (%s),ret=%d,fd=%d",strerror(errno),ret,fd);
    }
    db_msg("cpu_freq_value =%s",cpu_freq_value);
    close(fd);
    return (ret > 0) ? 0 : -1;
}

int _adjustCpuFreq()
{
    //get_scaling_speed();

    set_scaling_governor(USERSPACE);
    db_msg("set new freq %s", CPU_FREQ);
    set_scaling_speed(CPU_FREQ);
    get_scaling_speed();
    return 0;
}

void PowerManager::AdjustCpuFreq()
{
    _adjustCpuFreq();
}

void PowerManager::WatchdogThread()
{
    int fd = open("/dev/watchdog", O_RDWR);
    if (fd < 0) {
        db_msg("Watch dog device open error.");
        return;
    }
    int watchdog_enable = WDIOS_DISABLECARD;
    int io_ret = ioctl(fd,WDIOC_SETOPTIONS, &watchdog_enable);
    db_msg("After WDIOC_SETOPTIONS disable, ret = %d", io_ret);
    int time_out = 10;      //8s, min:1, max:16
    io_ret = ioctl(fd, WDIOC_SETTIMEOUT, &time_out);
    db_msg("WDIOC_SETTIMEOUT, ret = %d, time_out = %d", io_ret, time_out);
    io_ret = ioctl(fd,WDIOC_GETTIMEOUT, &time_out);
    db_msg("WDIOC_GETTIMEOUT, ret = %d, time_out = %d", io_ret, time_out);

    watchdog_enable = WDIOS_ENABLECARD;
    io_ret = ioctl(fd, WDIOC_SETOPTIONS, &watchdog_enable);
    db_msg("WDIOC_SETOPTIONS enable, ret = %d", io_ret);

    while(1) {
        ioctl(fd, WDIOC_KEEPALIVE, NULL);
        usleep(3000 * 1000);
        if(!is_enter_standby_) {
            watchdog_enable = WDIOS_DISABLECARD;
            io_ret = ioctl(fd,WDIOC_SETOPTIONS, &watchdog_enable);
            db_msg("After WDIOC_SETOPTIONS disable, ret = %d", io_ret);
            break;
        }
    }
    close(fd);
    return;
}

void PowerManager::WatchDogRun()
{
    //WatchDogThread(this);  /*creat watchdog thread*/
}

void PowerManager::SetEnterOTAService(bool flag)
{
    is_enter_standby_ = flag;
}

void *PowerManager::EventLoopThread(void *context)
{
    int tempFd;
    char str[32] = {0};
    PowerManager *pm = reinterpret_cast<PowerManager*>(context);

    prctl(PR_SET_NAME, "PMEventLoopThread", 0, 0, 0);

    while(!pm->m_bEventDetectThreadExit) {
        int ret = getoneline(CPU_TEMPERATURE_FILE, str, 32);
        if (ret < 0) {
            db_warn("read cpu temp failed, %s", strerror(errno));
            sleep(10);
            continue;
        }
        int cur_temp = atoi(str);
        bool temp_is_high = (cur_temp >= CPU_TEMP_HIGH_LEVEL);
        static bool last_temp_state = temp_is_high;

        if (temp_is_high) {
            db_warn("cpu temperature is too high, temp: %d", cur_temp);
            pm->Notify(MSG_CPU_TEMP_HIGH);
        } else if (!temp_is_high) {
            if (last_temp_state != temp_is_high)
                pm->Notify(MSG_CPU_TEMP_NORMAL);
        }

        last_temp_state = temp_is_high;
        sleep(10);
    }
    return NULL;
}

bool PowerManager::getUsbconnectStatus()
{
    string connect_status = "";
    get_oneline(USB_CONNECT_STATUS, connect_status);

    return !strncmp(connect_status.c_str(),"CONFIGURED",strlen("CONFIGURED"));
}

int PowerManager::getAhdInsertOnline()
{
    string Ahd_Online;
    string isOnline = "0x3";
    string isOffline = "0x4";
    int online_value;
    get_oneline(AHD_ONLINE_STATUS, Ahd_Online);
    db_error("getAhdInsertOnline online = %s\n",Ahd_Online.c_str());
    if(!strncmp(Ahd_Online.c_str(),"0x3",3))
    {
        db_error("ahd is insert #####\n");
        return 1;
    }else if(!strncmp(Ahd_Online.c_str(),"0x4",3))
    {
        db_error("ahd is plgout #####\n");
        return 0;
    }else{
        db_error("getAhdInsertOnline othe value \n");
        return 0;   
    }
}

int PowerManager::getPowenOnType()
{
    string tmpString;
    string powerOnType;
    int ret_type = -1;
    system("echo 0x20 > /sys/class/axp/axp_reg");
    get_oneline(POWER_ON_STATUS, powerOnType);    
    db_warn("poweron type is %s\n",powerOnType.c_str());
    std::size_t pos =  powerOnType.find("=");
    tmpString = powerOnType.substr(pos);
    db_warn("tmpString type is  = %s\n",tmpString.c_str());
    
    if(!strncmp(tmpString.c_str(),"=0x2",4))
    {
        db_warn("getPowenOnType is gsensor \n");
        return 0;
    }else if(!strncmp(powerOnType.c_str(),"=0x1",4))
    {
        db_warn("getPowenOnType is batter on\n");
        return 1;
    }else if(!strncmp(powerOnType.c_str(),"=0x4",4)){
        db_error("getPowenOnType is  vbus on\n");
        return 2;   
    }
    return ret_type;
}

bool PowerManager::getACconnectStatus()
{
    string ac_online,usb_online;

    get_oneline(AC_ONLINE_STATUS, ac_online);
    get_oneline(USB_ONLINE_STATUS, usb_online);
    return (atoi(ac_online.c_str()) || atoi(usb_online.c_str()));
}

void* PowerManager::BatteryDetectThread(void *context)
{
    int level = 5;
    int batteryStatus;
    bool battery_full = false;
    PowerManager *self = reinterpret_cast<PowerManager*>(context);
    while(!self->m_bBatDetectThreadExit)
    {
        batteryStatus = self->GetBatteryStatus();
        if( batteryStatus == EyeseeLinux::BATTERY_FULL&&battery_full== false && self->getACconnectStatus())
        {
            if( 100 == self->getcapacity())
            {
                battery_full = true;
               // self->Notify(MSG_BATTERY_FULL);
            }
        }
        else 
        {            
            if( batteryStatus < 0)
            {
                sleep(BAT_DETECT_TIME);
                battery_full = false;
                continue;
            }
            if( batteryStatus != EyeseeLinux::BATTERY_FULL )
                battery_full = false;
                
            if( self->getcapacity() == 0)
            {
				//db_warn("[debug_jaosn]: BatteryDetectThread Current Battery is low need power off!\n");
				if (!self->getACconnectStatus()) {
                    //self->Notify(MSG_BATTERY_LOW);
                }
            }
        }

        sleep(BAT_DETECT_TIME);
    }
    return NULL;  
}

int PowerManager::resetScreenStatus()
{
    mstate_ = SCREEN_ON;

    return 0;
}

void PowerManager::ResetUDC()
{
    int pstat;

    system("killall adbd 2>/dev/null");
    set_all_fd_cloexec();

    if (0 == vfork()) {
	    umask(000);
	    chdir("/");
	    setsid();
	    signal(SIGCHLD, SIG_IGN);
	    signal(SIGHUP, SIG_IGN);

	    if (fork())
		    _exit(EXIT_SUCCESS);

	    execlp("adbd", "adbd", NULL);

	    /*
	     * If fail to run adbd, we need to remove this function,
	     * If not, we can not receive the state of UDC.
	     */
	    unlink("/sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
	    _exit(EXIT_FAILURE);
    }

    sleep(2);
    /*
     * Recycled the resources of child process, otherwise, it will be dead process
     */
    while ((-1 == waitpid(-1, &pstat, WNOHANG)) && (errno == EINTR));

    system("echo $(ls /sys/class/udc)> " UDC_CONFIG_NODE);
}

int PowerManager::SetBrightnessLevel(int level)
{
    lock_guard<mutex> lock(screen_mutex);
    db_error("SetBrightnessLevel --- level = %d",level);
    int retval = 0;
    unsigned long args[4]={0};
    switch(level){
	case 0:
		args[1]= 0;
		break;
	case 1:
		args[1]= 105;
		break;
	case 2:
		args[1]= 135;
		break;
	case 3:
		args[1]= 165;
		break;
	case 4:
		args[1]= 195;
		break;
	case 5:
		args[1]= 225;
		break;
	case 6:
		args[1]= 255;
		break;
    	}
	args[1] = 255 - args[1];
    retval = ioctl(mdisp_fd_, DISP_LCD_SET_BRIGHTNESS, args);
    if (retval < 0) {
        db_msg("fail to SetBrightnessLevel");
        return -1;
    }
    return 0;
}

