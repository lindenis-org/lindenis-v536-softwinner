/**********************************************************
* Copyright (C), 2015, AllwinnerTech. Co., Ltd.  *
***********************************************************/

/**
 * @file event_manager.cpp
 * @author yinzh@allwinnertech.com
 * @date 2016-4-28
 * @version v0.3
 * @brief 事件管理模块源文件
 * @see event_manager.h
 * @verbatim
 *  History:
 * @endverbatim
 */
 
#include "common/setting_menu_id.h" 
#include "device_model/menu_config_lua.h"
#include "device_model/system/power_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/partitionManager.h"
#include "device_model/storage_manager.h"
#include "device_model/dataManager.h"
#include "bll_presenter/AdapterLayer.h"
#include "bll_presenter/camRecCtrl.h"
#include "device_model/system/obd2_manager.h"
#include "window/window_manager.h"
#include <edog/EdogGlobal.h>
#include "common/app_log.h"
#include "common/app_def.h"
#include "common/thread.h"
#include "common/message.h"
#include "common/utils/utils.h"
#include "gsensor_manager.h"
#include "tools/udev_message.h"
#include <sys/resource.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <asm/types.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/reboot.h>

//#include <cutils/android_reboot.h>
#include "dd_serv/common_define.h"
#include "dd_serv/globalInfo.h"

#undef LOG_TAG
#define LOG_TAG "EventManager"

//#define ENABLE_4G_POWER_RESET
#define ENABLE_ONLY_SUPPORT_DC
//#define DEBUG_GPS
//#define WAKEUP_REBOOT
#define SEND_GPSDATA
//#define DEBUG_IR_LED
//#define DEBUG_NTP 1
//#define TEST_ACC
#define UNIX_DOMAIN     "/tmp/event/unix.domain"

//#define USE_OBD2
#define SDCARD_NODE     "/sys/bus/mmc/drivers/mmcblk/uevent"
#define SDCARD_NODE1    "/sys/bus/mmc/drivers/mmcblk/mmc0:0001/uevent"
#define SDCARD_NODEX    "/sys/bus/mmc/drivers/mmcblk/*/uevent"
#define USBDISK_NODE    "/sys/bus/usb/drivers/usb-storage/1-1:1.0/uevent"
#define HDMI_NODE       "/sys/devices/virtual/switch/hdmi/state"
#define CVBS_NODE       "/sys/devices/virtual/switch/cvbs/state"
#ifdef TEST_ACC
#define ACC_NODE       "/tmp/acc_state"
#else
#define ACC_NODE        "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/acc_state"
#endif
#define ACC_ENABLE        "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/enable_acc"
char const*const M_WAKEUP_AP =    "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/m_wakeup_ap";
char const*const AP_4G_RST   =    "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/ap_4g_rst";
char const*const AP_4G_PWR	 =	  "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/ap_4g_pwr";
char const*const AP_WAKEUP_M =    "/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/ap_wakeup_m";
#define TEST_GSENSOR       "/sys/devices/platform/gpio_sw.4/gpio_sw/PH13/data"
#define LIGHT_VALUE 	"/sys/devices/virtual/misc/light-value/light-value/light_value"
#define FILE_ENABLE_ACC_SWITCH  "/mnt/extsd/.tmp/.t/.dIsAbLeAcC"

char const*const IR_LED_FILE = "/sys/devices/platform/soc/gpioled/leds/ir_led/brightness";


#define POWER_RESET_INVAILD     (100)
#define POWER_RESET_FIRST       (10)

#define TIMES_RETRY     (1000)

#define GPS_TIMEOUT     10
//#define ZTE_4G_MODULE
//#define SIMCOM_4G_MODULE
//#define G457_GPS_MODULE

using namespace EyeseeLinux;
using namespace std;

//#define DEBUG_4G
#ifdef DEBUG_GPS
int print_count = 50;
#endif
#ifdef DEBUG_4G
int reinit_4g_count = 0;
char ip_4g[32] = {0};
int gotip_count = 0;
int repower_4g_count = 0;
int is_4g_need_reboot = 0;
#endif
#ifdef ENABLE_4G_POWER_RESET
int reset4GModuleCount = 0;
#endif

extern queue<GPS_INFO> GPSInfoQue;
int gpsFd;

EventManager::EventManager()
{
	mNetTime_flag = 1;
	m_4GWakeUp_state = 0;
	mStandbyStatus = false;
	mDcConnect_flag = false;
	mAccStatus = 0;
	mAccIn_flag = false;
	low_battery_flag = false;
	mDcStatus = 1;
	mGpsStatus = 0;
	mNetStatus = 0;
	gps_signal_info = 0;
	IrisOpen = false;
	mTimer_thread_flag = 0;
	mNotify_acc_on_flag = false;
	mNotify_acc_off_flag = false;
	mStandbybreak_flag = false;
	standby_try_count = 0;
	m_4GModule_flag = 0;
	m_NetType = "";
	m_NetSignal = "100";
	imei = "";
	sim = "";
	m_facturer = OTHER;
	mGpsSignal = 0;
	gps_signal_info = 0;
	is_enable_acc = 1;
	gpsFd = -1;
	usb4g_fd = -1;
	m_4g_version.clear();
	mNeedSwitchPid = true;
	mOpenSimcomGpsDone = false;
	GPSModuleStatus = 0;
	synctimeflag = 1;
	memset(TFCard_Cid, 0, sizeof(TFCard_Cid));
	listen_fd_ = -1;
    listen_fd_ = UdevMonitorInit();
    RemoteActionDone = 2; //test
    setIrLedBrightness(IRLedOff);
	sdcardFlag = false;
    db_error("=============close ir led=============");
	#if 0
    netlink_fd_ = InitNetLinkMonitor();
    if (netlink_fd_ > 0)
        ThreadCreate(&netlink_monitor_thread_id_, NULL, EventManager::NetLinkMonitorThread, this);	
	#endif
	#ifdef DEBUG_GPSSAVELOG
	gpslogfd = -1;
	#endif
}

EventManager::~EventManager()
{
    if (event_loop_thread_id_ > 0)
        pthread_cancel(event_loop_thread_id_);

	if( m_ImapactEvent_thread_id > 0 )
		pthread_cancel(m_ImapactEvent_thread_id);

    if (m_IrCheckFun_thread_id > 0)
        pthread_cancel(m_IrCheckFun_thread_id);

    if (listen_fd_ >= 0) close(listen_fd_);
#if 0
    if (netlink_fd_ > 0) close(netlink_fd_);
    if( m_GpsEvent_thread_id > 0 )
		pthread_cancel(m_GpsEvent_thread_id);
	
	if( m_BindFlagEvent_thread_id > 0 )
		pthread_cancel(m_BindFlagEvent_thread_id);

	if( m_AccEvent_thread_id > 0 )
		pthread_cancel(m_AccEvent_thread_id);

	if( m_Usb4G_thread_id > 0 )
		pthread_cancel(m_Usb4G_thread_id);

	if (usb4g_fd >= 0) close(usb4g_fd);

    if (m_NetEvent_thread_id > 0)
        pthread_cancel(m_NetEvent_thread_id);
    
    if (netlink_monitor_thread_id_ > 0)
        pthread_cancel(netlink_monitor_thread_id_);
#endif	
	#ifdef DEBUG_GPSSAVELOG
	if (gpslogfd>=0) {
		close(gpslogfd);
		gpslogfd = -1;
	}
	if (gpsfdorg>=0)
		close(gpsfdorg);
	#endif
	
}

int EventManager::WriteFileInt(char const* path, int value)
{
		int fd;	
		fd = open(path, O_RDWR);
		db_msg("write_int open fd=%d\n", fd);
		if (fd >= 0) {
			char buffer[4];
			snprintf(buffer,sizeof(buffer), "%d\n", value);
			int amt = write(fd, buffer, sizeof(buffer));
			close(fd);
			return amt == -1 ? -errno : 0;
		} else {
			return -errno;
		}
}

int EventManager::setIrLedBrightness(enum IRLedState IrLedState)
{
	int ret = 0;
    if(access(IR_LED_FILE, F_OK) < 0)
        return -1;
        
	ret = WriteFileInt(IR_LED_FILE,IrLedState);
	if(ret < 0)
		db_error("write %s fail ,err = %d \n",IR_LED_FILE, ret);
	return ret;
}

char *getStrBehindComma(char * str, int which)
{
	char *tmp = str;
	while(which)
	{
		if(tmp == NULL )
			return NULL;

		tmp = strchr(tmp,',');
		if(tmp == NULL )
			return NULL;
		tmp++;
		which--;
	}
	return tmp;
}

double get_double_number(char *s)
{
	if(s != NULL){
		return atof(s);
	}else{
		return 0.0;
	}
}

double get_locate(double temp)
{
    int m;
    double  n;
    m=(int)temp/100;
    n=(temp-m*100)/60;
    n=n+m;
    return n;

}

void EventManager::RunAccEvent()
{
    if (listen_fd_ >= 0)
        ThreadCreate(&event_loop_thread_id_, NULL, EventManager::EventLoopThread, this);
    ThreadCreate(&m_ImapactEvent_thread_id,NULL,EventManager::CheckImpackEvent,this);
    ThreadCreate(&m_AccEvent_thread_id,NULL,EventManager::CheckAccEvent,this);
    #if 0
    ThreadCreate(&m_Usb4G_thread_id,NULL,EventManager::USB4G_Init,this);
	ThreadCreate(&m_NetEvent_thread_id,NULL,EventManager::CheckNetEvent,this);
	
	ThreadCreate(&m_GpsEvent_thread_id,NULL,EventManager::startLocationReport,this);
	RunBindFlagDetect();
	RunIrCheckFun();
    #ifdef	USE_OBD2
	    Obd2Manager::GetInstance()->RunObd2();
    #endif
    #else
    #ifdef G457_GPS_MODULE
    ThreadCreate(&m_GpsEvent_thread_id,NULL,EventManager::startLocationReport,this);
    #endif
    #endif
    
}

void EventManager::RunBindFlagDetect()
{
	ThreadCreate(&m_BindFlagEvent_thread_id,NULL,EventManager::CheckBindFlagEvent,this);	
}

void EventManager::RunIrCheckFun()
{
	ThreadCreate(&m_IrCheckFun_thread_id,NULL,EventManager::CheckIrCheckFunEvent,this);	
}



int EventManager::configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) 
    { 
        db_msg("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD; 
    newtio.c_cflag &= ~CSIZE; 

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':                    
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':                     
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':                    
        newtio.c_cflag &= ~PARENB;
        break;
    }

	switch( nSpeed )
	{
	    case 2400:
	        cfsetispeed(&newtio, B2400);
	        cfsetospeed(&newtio, B2400);
	        break;
	    case 4800:
	        cfsetispeed(&newtio, B4800);
	        cfsetospeed(&newtio, B4800);
	        break;
	    case 9600:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
	    case 115200:
	        cfsetispeed(&newtio, B115200);
	        cfsetospeed(&newtio, B115200);
	        break;
	    default:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
    }
    if( nStop == 1 )
    {
        newtio.c_cflag &=  ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    newtio.c_oflag &= ~OPOST;
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    newtio.c_cc[VTIME]  = 15;	// 15*100ms timeout
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        db_msg("com set error");
        return -1;
    }
    db_msg("set done!\n");
    return 0;
}
//sometimes USB port may different.
//maybe ttyUSB0 ttyUSB1 ttyUSB2 ttyUSB3 ;maybe ttyUSB0 ttyUSB3 ttyUSB4 ttyUSB5
//first port is diag port ; second port is gps port; third port is AT commond port;
int EventManager::FindSerialPort(enum USBPort port_index)
{
		int loop_j ,loop_i= 0;
		char tty_usb_port[13] = {0};

		for(loop_j = 0 ; loop_j < 10 ; loop_j++)
			{
				snprintf(tty_usb_port, sizeof(tty_usb_port), "/dev/ttyUSB%d", loop_j);
				if((access(tty_usb_port, F_OK)) == 0){
					loop_i++;
					if(loop_i == port_index){
					break;
					}
				}
			}
		if(loop_i< port_index)
			{
			db_error("FindSerialPort not found the %d port!\n",port_index);
			return -1;
			}
		db_msg("FindSerialPort %d  %s !\n",port_index,tty_usb_port);
		return openSerialPort(tty_usb_port);
		
}
int EventManager::openSerialPort(char *ttys)
{
	int fd;
    fd = open(ttys, O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd <= 0)
    {
        db_msg("Can't Open Serial Port");
        return -1;
    }   
    if(fcntl(fd, F_SETFL, 0) < 0)
    {
        db_msg("fcntl failed!\n");
    }
    if(isatty(STDIN_FILENO) == 0)
    {
        db_msg("standard input is not a terminal device\n");
    }
    return fd;
}

int EventManager::LocationReportThreadInit()
{
    if(gpsFd > 0) {
        close(gpsFd);
        gpsFd = -1;
    }
#ifdef SIMCOM_4G_MODULE
while(1)
	{
	if(access("/dev/ttyUSB0", F_OK) == 0)
		break;
	else
		usleep(500*1000);
	}

	if((gpsFd=FindSerialPort(USB_GPS)) < 0)
	{
		db_error("open gps port error");
		return -1;
	}

#elif defined(G457_GPS_MODULE) 
    if((gpsFd=openSerialPort("/dev/ttyS4")) < 0)
    {
        db_error("open_port error");
        return -1;
    }
#else 
    if((gpsFd=openSerialPort("/dev/ttyS3")) < 0)
    {
        db_error("open_port error");
        return -1;
    }
#endif
    if(configureSerialPort(gpsFd,9600,8,'N',1) < 0)
    {
        db_error("set_opt error");
		close(gpsFd);
		gpsFd = -1;
        return -1;
    }

	return 0;
}

int EventManager::CheckttyUsb()
{
#ifdef SIMCOM_4G_MODULE
	if(access("/dev/ttyUSB0", F_OK) == 0)
#elif defined(G457_GPS_MODULE)
        if(access("/dev/ttyS4", F_OK) == 0)
#else
	if((access("/dev/ttyUSB0", F_OK) == 0) && (access("/dev/ttyUSB1", F_OK) == 0) && (access("/dev/ttyUSB2", F_OK) == 0))
#endif
		return 1;
	else
		return 0;
}

int EventManager::CheckEvent(EventType type, int &value)
{
    int v = 0;

    switch (type) {
        case EVENT_USB:
            v = CheckUSBDisk();
            break;
        case EVENT_SD:
            v = CheckSDCard();
            break;
        case EVENT_HDMI:
            v = CheckHDMI();
            break;
        case EVENT_TVOUT:
            v = CheckTVOut();
            break;
        case EVENT_BT:
            v = CheckBluetooth();
            break;
		case EVENT_ACC:
            v = CheckACCStatus();
            break;	
		case EVENT_4GWAKE:
            v = Check4GWakeUpStatus();
            break;				
        case EVENT_ETH:
            v = CheckNetLink("eth0");
            break;
        case EVENT_WIFI:
            v = CheckNetLink("wlan0");
            break;;
        default:
            db_warn("wrong event type: %d", type);
            break;
    }

    value = v;

    return v;
}

int EventManager::CheckSDCard()
{
    // "/sys/bus/mmc/drivers/mmcblk/mmc0:0001/uevent"
    // "/sys/bus/mmc/drivers/mmcblk/*/uevent"
    // "/sys/bus/mmc/drivers/mmcblk/uevent"

    struct stat st;
    int status = 0xAAAA;

    status = system("test -e " SDCARD_NODEX);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            return 1;
        } else if (WEXITSTATUS(status) == 1) {
            stat(SDCARD_NODE, &st);
            if ( (st.st_mode & S_IRUSR) == S_IRUSR)
                return 1;
        }
    }

    return 0;
}

int EventManager::CheckUSBDisk()
{
    // "/sys/bus/usb/drivers/usb-storage/1-1:1.0/uevent"

    struct stat st;
    stat(USBDISK_NODE, &st);

    if ( (st.st_mode & S_IRUSR) == S_IRUSR) {
        return 1;
    }

    return 0;
}

int EventManager::CheckHDMI()
{
    // "/sys/devices/virtual/switch/hdmi/state"

    char buf[16] = {0};

    int ret = getoneline(HDMI_NODE, buf, sizeof(buf));

    if (ret < 0) return 0;

    db_msg("buf: %s", buf);

    int stat = atoi(buf);

    if (stat) {
        this->Notify(MSG_HDMI_PLUGIN);
        return 1;
    }

    this->Notify(MSG_HDMI_PLUGOUT);

    return 0;
}

int EventManager::CheckTVOut()
{
    // "/sys/devices/virtual/switch/cvbs/state"

    char buf[16] = {0};

    int ret  = getoneline(CVBS_NODE, buf, sizeof(buf));

    if (ret < 0) return 0;

    int stat = atoi(buf);

    if (stat) {
        this->Notify(MSG_TVOUT_PLUG_IN);
        return 1;
    }

    this->Notify(MSG_TVOUT_PLUG_OUT);

    return 0;
}

int EventManager::CheckACCStatus()
{
    // "/sys/devices/virtual/acc/acc/state"

    char buf[16] = {0};

    int ret = getoneline(ACC_NODE, buf, sizeof(buf));

    if (ret < 0) return 0;

    int stat = atoi(buf);
    return stat;
}

int EventManager::Check4GWakeUpStatus()
{
    //"/sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/m_wakeup_ap"

    char buf[16] = {0};

    int ret = getoneline(M_WAKEUP_AP, buf, sizeof(buf));

    if (ret < 0) return 0;

    int stat = atoi(buf);

    return stat;
}


int EventManager::CheckBluetooth()
{
    // not enable temparery

    return 0;
}

int EventManager::CheckSDCard_Cid()
{
	char *dirname = (char *)"/sys/devices/platform/soc/sdc0/mmc_host/mmc0";
    char buf[64] = {0};	
	char classPath[256] = {0};	
	char cidPath[256] = {0};	
	DIR *mmc0_dir = NULL;
	DIR *cid_dir = NULL;
	int res;
    int ret = -1;
	int fd = -1;
    struct dirent *mmc0_de;
    struct dirent *cid_de;
	mmc0_dir = opendir(dirname);
	if(mmc0_dir == NULL){
		return TF_ERROR;
	}
	while((mmc0_de = readdir(mmc0_dir))){
		if (strncmp(mmc0_de->d_name, "mmc0", strlen("mmc0")) != 0) {
            continue;
        }
		snprintf(classPath, sizeof(classPath),"%s/%s", dirname, mmc0_de->d_name);
		cid_dir = opendir(classPath);
		if(cid_dir == NULL){
			closedir(mmc0_dir);
			return TF_ERROR;
		}
		while((cid_de = readdir(cid_dir))){
	        if (strncmp(cid_de->d_name, "cid", strlen("cid")) != 0) {
	            continue;
	        }		
			
			db_msg("by hero *** cid_de = %s", cid_de->d_name);
	        snprintf(cidPath, sizeof(cidPath),"%s/%s", classPath, cid_de->d_name);

	        fd = open(cidPath, O_RDONLY);
	        if (fd < 0) {
	            continue;
	        }

            memset(buf, 0, sizeof(buf));
	        if ((res = read(fd, buf, sizeof(buf))) < 0) {
	            close(fd);
	            continue;
	        }
	        buf[res - 1] = '\0';
		}
	}

    if(TFCard_Cid[strlen(TFCard_Cid)] == '\n') {
        TFCard_Cid[strlen(TFCard_Cid)] = '\0';
    }
    if(buf[strlen(buf)] == '\n') {
        buf[strlen(buf)] = '\0';
    }

	if(TFCard_Cid[0] != '\0')
	{
		if(strcmp(buf, "") != 0)
		{
			if(strcmp(TFCard_Cid, buf))
				ret = TF_DIFF;
	        else
	            ret = TF_NORMAL;
		}
		else
			ret = TF_ERROR;
    }
	else
	{
		if(	strcmp(buf, "") != 0 )
		{
			strncpy(TFCard_Cid, buf, sizeof(TFCard_Cid));
    	    ret = TF_NORMAL;
		}
		else
			ret = TF_ERROR;
	}

	if(NULL != cid_dir)
		closedir(cid_dir);	
	if(NULL != mmc0_dir)
		closedir(mmc0_dir);

	if(fd >= 0)
		close(fd);

    return ret;
}


int EventManager::CheckNetLink(const char *interface)
{
    int sockfd = 0;
    struct ifreq ifr;

    if (interface == NULL) {
        db_warn("interface is NULL");
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        db_warn("open socket error, interface[%s], %s", interface, strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        db_debug("ioctl SIOCGIFFLAGS error, interface[%s], %s", interface, strerror(errno));
        close(sockfd);
        return -1;
    }

	close(sockfd);
    if (ifr.ifr_flags & IFF_RUNNING)
        return 1;
    else
        return 0;
}

int EventManager::UdevMonitorInit()
{
    int listen_fd;
    int ret;

    struct sockaddr_un s_addr;

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        db_error("create socket failed: %s", strerror(errno));
        return -1;
    }

    s_addr.sun_family = AF_UNIX;
    strncpy(s_addr.sun_path, UNIX_DOMAIN, sizeof(s_addr.sun_path) - 1);

    unlink(UNIX_DOMAIN);
    ret = bind(listen_fd, (struct sockaddr*)&s_addr, sizeof(s_addr));
    if (ret < 0) {
        db_error("bind socket failed: %s", strerror(errno));
        close(listen_fd);
        return -1;
    }

    ret = listen(listen_fd, 5);
    if (ret < 0) {
        db_error("listen failed: %s", strerror(errno));
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int EventManager::TryEvent(int sockfd, int &event)
{
	if( sockfd < 0 )
		return -1;

    int listen_fd;
    int apt_fd;

    char recv_buf[128] = {0};

    struct sockaddr_un recv_addr;
    socklen_t recv_len = sizeof(recv_addr);

    listen_fd = sockfd;

    apt_fd = accept(listen_fd, (struct sockaddr*)&recv_addr, &recv_len);
    if (apt_fd < 0) {
        db_error("can not accept client connect request: %s", strerror(errno));
        return -1;
    }

    int count;
    while ( (count = read(apt_fd, recv_buf, sizeof(recv_buf))) > 0) {
        db_msg("udev msg:0x%02x", recv_buf[0]);
        event = recv_buf[0];
    }

    if (count < 0) {
        db_msg("read msg failed:%s", strerror(errno));
        close(apt_fd);
        return -1;
    }

    close(apt_fd);
    return 0;
}

int EventManager::InitNetLinkMonitor()
{
    int sockfd = 0;
    struct sockaddr_nl addr;

    memset((void *)&addr, 0, sizeof(addr));

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        db_error("create socket failed: %s", strerror(errno));
        return -1;
    }

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = RTMGRP_LINK|RTMGRP_IPV4_IFADDR|RTMGRP_IPV6_IFADDR;

    if ( bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        db_error("bind socket failed: %s", strerror(errno));
        return -1;
    }

    return sockfd;
}

int EventManager::NetLinkMsgHandler(struct sockaddr_nl *sockaddr, struct nlmsghdr *nlmsg)
{
    struct ifinfomsg *ifi = (struct ifinfomsg*)NLMSG_DATA(nlmsg);
    struct ifaddrmsg *ifa = (struct ifaddrmsg*)NLMSG_DATA(nlmsg);
    char ifname[1024] = {0};

    switch (nlmsg->nlmsg_type) {
        case RTM_NEWADDR:
            if_indextoname(ifi->ifi_index, ifname);

            if (strcmp(ifname, "wlan0") == 0) {
                this->Notify(MSG_WIFI_ENABLED);
            } else if (strcmp(ifname, "eth0") == 0) {
                this->Notify(MSG_ETH_CONNECT_LAN);
                printf("------------------------NetLinkMsgHandler\n");
				get_ntptime();
            }
			
            db_msg("RTM_NEWADDR : %s", ifname);
            break;
        case RTM_DELADDR:
            if_indextoname(ifi->ifi_index, ifname);

            if (strcmp(ifname, "wlan0") == 0) {
                this->Notify(MSG_WIFI_DISABLED);
            } else if (strcmp(ifname, "eth0") == 0) {
                this->Notify(MSG_ETH_DISCONNECT);
            }

            db_msg("RTM_DELADDR : %s", ifname);
            break;
        case RTM_NEWLINK: {
            if_indextoname(ifi->ifi_index, ifname);

            if (strcmp(ifname, "wlan0") == 0) {
                if (ifi->ifi_flags & IFF_RUNNING)
                    this->Notify(MSG_WIFI_ENABLED);
                else
                    this->Notify(MSG_WIFI_DISABLED);
            } else if (strcmp(ifname, "eth0") == 0) {
                if (ifi->ifi_flags & IFF_RUNNING)
                    this->Notify(MSG_ETH_CONNECT_LAN);
                else
                    this->Notify(MSG_ETH_DISCONNECT);
            }

            db_msg("Link %s %s", ifname, (ifi->ifi_flags & IFF_RUNNING)?"Up":"Down");
        }
            break;
        case RTM_DELLINK:
            if_indextoname(ifa->ifa_index, ifname);
            db_msg("RTM_DELLINK : %s", ifname);
            break;
        default:
            db_msg("Unknown netlink nlmsg_type %d", nlmsg->nlmsg_type);
            break;
    }

    return 0;
}

int EventManager::ReadNetLinkEvent(int sockfd)
{
    int status;
    int ret = 0;
    char buf[4096] = {0};
    struct iovec iov = { buf, sizeof buf };
    struct sockaddr_nl snl;
    struct msghdr msg = { (void*)&snl, sizeof snl, &iov, 1, NULL, 0, 0};
    struct nlmsghdr *nlmsg;

    status = recvmsg(sockfd, &msg, 0);
    if (status < 0) {
        /* Socket non-blocking so bail out once we have read everything */
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return ret;

        /* Anything else is an error */
        db_error("recvmsg faild, %s", strerror(errno));

        return status;
    }

    if (status == 0) {
        db_info("read netlink event: EOF");
    }

    /* We need to handle more than one message per 'recvmsg' */
    for (nlmsg = (struct nlmsghdr *) buf; NLMSG_OK (nlmsg, (unsigned int)status);
            nlmsg = NLMSG_NEXT (nlmsg, status))
    {
        /* Finish reading */
        if (nlmsg->nlmsg_type == NLMSG_DONE)
            return ret;

        /* Message is some kind of error */
        if (nlmsg->nlmsg_type == NLMSG_ERROR) {
            db_error("Message is an error - decode TBD");
            return -1; // Error
        }

        /* Call message handler */
        ret = NetLinkMsgHandler(&snl, nlmsg);
        if(ret < 0) {
            db_error("Message hander error %d", ret);
            return ret;
        }
    }

    return ret;
}

void *EventManager::EventLoopThread(void *context)
{
    int event;

    EventManager *em = reinterpret_cast<EventManager*>(context);

    db_msg("event loop");
    prctl(PR_SET_NAME, "EventLoopThread", 0, 0, 0);

    while(1) {
        if (em->TryEvent(em->listen_fd_, event) < 0) {
			if( em->listen_fd_ >= 0)
            {
            	close(em->listen_fd_);
	            em->listen_fd_ = -1;
			}
            em->listen_fd_ = em->UdevMonitorInit();
            continue;
        }
        switch (event) {
            case SD_MOUNTED: {
					em->sdcardFlag = true;
                    em->Notify(MSG_STORAGE_MOUNTED);
                }
                break;
            case SD_REMOVE: {
            // case SD_UMOUNT: {
                    em->Notify(MSG_STORAGE_UMOUNT);
					em->sdcardFlag = false;
                }
                break;
            case SD_FS_ERROR: {
                    db_error("SD_FS_ERROR happen\n");
                    //em->Notify(MSG_STORAGE_FS_ERROR);
                }
                break;

            case UDISK_INSERT: {
                    em->Notify(MSG_USB_PLUG_IN);
                }
                break;
            case UDISK_REMOVE: {
                    em->Notify(MSG_USB_PLUG_OUT);
                }
                break;
            case UDISK_FS_ERROR: {
                    db_error("UDISK_FS_ERROR happen\n");
                    em->Notify(MSG_STORAGE_FS_ERROR);
                }
                break;
            case HDMI_INSERT: {
                    em->Notify(MSG_HDMI_PLUGIN);
                }
                break;
            case HDMI_REMOVE: {
                    em->Notify(MSG_HDMI_PLUGOUT);
                }
                break;
            case TVOUT_INSERT: {
                    em->Notify(MSG_TVOUT_PLUG_IN);
                }
            break;
            case TVOUT_REMOVE: {
                    em->Notify(MSG_TVOUT_PLUG_OUT);
                }
            break;
            case USB_HOST_CONNECTED:
            {
            	db_error("USB_HOST_CONNECTED");
#ifdef USB_MODE_WINDOW
                em->Notify(MSG_USB_HOST_CONNECTED);
#endif
            }
            break;
            case USB_HOST_DETACHED:
            {
#ifdef USB_MODE_WINDOW
                em->Notify(MSG_USB_HOST_DETACHED);
#endif
            }
            break;
            case AHD_CONNECTED:
            {
                db_warn("[debug_jaosn]: this AHD is AHD_CONNECTED\n");
                em->Notify(MSG_AHD_CONNECT);
            }
            break;
            case AHD_DETACHED:
            {
                db_warn("[debug_jaosn]: this AHD is AHD_DETACHED\n");
                em->Notify(MSG_AHD_REMOVE);
            }
            break;
            default:
                break;
        }
    }

    return NULL;
}

void *EventManager::NetLinkMonitorThread(void *context)
{
    EventManager *em = reinterpret_cast<EventManager*>(context);

    prctl(PR_SET_NAME, "NetLinkMonitor", 0, 0, 0);

    db_msg("net link monitor");

    while (1) {
		if(!em->m_4GModule_flag){
			sleep(1);
			continue;
		}
        em->ReadNetLinkEvent(em->netlink_fd_);
    }

    return NULL;
}

void edog_resource_rd_cb_t(INT8U *buffer,INT32U offset,INT32U count)
{
	FILE *fp = fopen("/mnt/extsd/cn_180501_encrypt.bin","rb");
	size_t ret;
	if( fp != NULL )
	{
		fseek(fp, offset, SEEK_CUR);
		ret = fread(buffer, 1, count, fp);
		//db_msg("by hero *** ret = %d, offset = %d\n", ret, count);
		fclose(fp);
	}
}

void edog_notify_cb_t(INT8U alarm,INT32U event,INT32U speed,INT16U distance)
{
	db_msg("by hero *** edog_notify_cb_t");
}

void edog_location_notify_cb_t(PST_GPSRADAR_GPSINFO gpsinfo)
{
#if 0

	db_msg("by hero *** edog_location_notify_cb_t latitude = %d", gpsinfo->latitude);
	
	db_msg("by hero *** edog_location_notify_cb_t longitude = %d", gpsinfo->longitude);

	db_msg("by hero *** edog_location_notify_cb_t speed = %d", gpsinfo->speed);

	db_msg("by hero *** edog_location_notify_cb_t validSatNum = %d", gpsinfo->validSatNum);

	db_msg("by hero *** edog_location_notify_cb_t year = %d", gpsinfo->year);

	db_msg("by hero *** edog_location_notify_cb_t month = %d", gpsinfo->month);

	db_msg("by hero *** edog_location_notify_cb_t day = %d", gpsinfo->day);

	db_msg("by hero *** edog_location_notify_cb_t hour = %d", gpsinfo->hour);

	db_msg("by hero *** edog_location_notify_cb_t minute = %d", gpsinfo->minute);

	db_msg("by hero *** edog_location_notify_cb_t second = %d", gpsinfo->second);	

	db_msg("by hero *** edog_location_notify_cb_t course = %d", gpsinfo->course);
	#endif
	
}

static int send_packet(int sockfd)
{
    unsigned int ntp_head[12];
    memset(ntp_head, 0, sizeof(ntp_head));

    ntp_head[0] = htonl(DEF_NTP_HEAD_LI | DEF_NTP_HEAD_VN | DEF_NTP_HEAD_MODE |
                DEF_NTP_HEAD_STARTUM | DEF_NTP_HEAD_POLL | DEF_NTP_HEAD_PRECISION);
    ntp_head[1] = htonl(1<<16);  /* Root Delay (seconds) */
    ntp_head[2] = htonl(1<<16);  /* Root Dispersion (seconds) */

    struct timeval now;
    gettimeofday(&now, NULL);

    /* 将当前时间写到transmit timestamp */
    ntp_head[10] = htonl(now.tv_sec + OFFSET_1900_TO_1970); /* 高32位 */
    ntp_head[11] = htonl(NTPFRAC(now.tv_usec));             /* 低32位 */

    if (send(sockfd, ntp_head, sizeof(ntp_head), 0) == -1) {
        db_msg("send data failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int connect_to_server(const char *ntp_server)
{
    int sockfd;
    const char *server;

    if (ntp_server != NULL) {
        server = ntp_server;
    } else {
        server = DEF_NTP_SERVER;
    }

    /* struct sockaddr_in addr_src; */
    struct sockaddr_in addr_dst;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ) {
        db_msg("create socket failed: %s\n", strerror(errno));
        return -1;
    }

    struct hostent *host;
    host = gethostbyname(server);
    //host = gethostbyaddr("61.216.153.104", strlen("61.216.153.104"), AF_INET);
    if (host == NULL) {
		close(sockfd);
        herror(server);
        return -1;
    }

    memset(&addr_dst, 0, sizeof(addr_dst));
    memcpy(&(addr_dst.sin_addr.s_addr), host->h_addr_list[0], sizeof(addr_dst.sin_addr));
   	//memcpy(&(addr_dst.sin_addr.s_addr), "61.216.153.104", strlen("61.216.153.104"));
    addr_dst.sin_family = AF_INET;
    addr_dst.sin_port = htons(DEF_NTP_PORT);
#ifdef DEBUG_NTP
    db_msg("ntp server %s address:%s, port:%d\n", \
         server, inet_ntoa(addr_dst.sin_addr), DEF_NTP_PORT);
#endif  

    if (connect(sockfd, (struct sockaddr*)&addr_dst, sizeof(addr_dst)) == -1) {
        db_msg("connect to server failed:%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int get_time_from_server(int sockfd, struct timeval *time)
{
    ntp_timestamp_t origtime, recvtime, transtime, desttime;
    int64_t origus, recvus, transus, destus, offus, delayus;
    uint32_t ntp_data[12];

    memset(ntp_data, 0, sizeof(ntp_data));
    if (recv(sockfd, ntp_data, sizeof(ntp_data), 0) == -1) {
        printf("recv failed:%s\n", strerror(errno));
        return -1;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    /* 目的时间戳， 表示客户端接收到数据的时间 */
    desttime.integer   = now.tv_sec + OFFSET_1900_TO_1970;
    desttime.fraction  = NTPFRAC(now.tv_usec);

    /* 原始时间戳， 表示客户端请求数据的时间 */
    origtime.integer   = ntohl(ntp_data[6]);
    origtime.fraction  = ntohl(ntp_data[7]);

    /* 接收时间戳， 表示服务器接收到数据的时间 */
    recvtime.integer   = ntohl(ntp_data[8]);
    recvtime.fraction  = ntohl(ntp_data[9]);

    /* 发送时间戳， 表示服务器发送数据的时间 */
    transtime.integer  = ntohl(ntp_data[10]);
    transtime.fraction = ntohl(ntp_data[11]);


    origus  = NTPTIME_TO_USEC(origtime);
    recvus  = NTPTIME_TO_USEC(recvtime);
    transus = NTPTIME_TO_USEC(transtime);
    destus  = NTPTIME_TO_USEC(desttime);

    /* 总的网络延迟，包括发送和接收过程 */
    offus = ((recvus - origus) + (transus - destus))/2;
    /* 这个公式的前提是，假设发送过程和接收过程的网络延迟相同 */
    delayus = (recvus - origus) + (destus - transus);
    printf("offus: %lld, delayus: %lld\n", offus, delayus);

    struct timeval newtime;

    uint64_t new_usec = (uint64_t)now.tv_sec * 1000000 + now.tv_usec + offus;
    printf("new_usec: %llu\n", new_usec);

    newtime.tv_sec    = new_usec / 1000000;
    newtime.tv_usec   = new_usec % 1000000;

    printf("nowtime.tv_sec:%ld, nowtime.tv_usec:%ld\n", now.tv_sec, now.tv_usec);
    printf("newtime.tv_sec:%ld, newtime.tv_usec:%ld\n", newtime.tv_sec, newtime.tv_usec);

    memcpy(time, &newtime, sizeof(struct timeval));

    return 0;
}	

int EventManager::GetNetIp(const std::string &netdev_name, unsigned int &ip)
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        close(fd);
        return -1;
    }

    ip = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);

    close(fd);
    return 0;
}


int EventManager::GetNetIp(const std::string &netdev_name, std::string &ip)
{
    int            ret   = 0;
    unsigned int   u32ip = 0;
    struct in_addr sin_addr;

    ret = GetNetIp(netdev_name, u32ip);
    if (ret) {
        db_msg("Do GetNetDevIp error! ret:%d  \n", ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u32ip);
    ip              = inet_ntoa(sin_addr);

    return 0;
}


void *EventManager::NetTimeThread(void *context)
{
    EventManager *em = reinterpret_cast<EventManager*>(context);

    prctl(PR_SET_NAME, "NetTimeThread", 0, 0, 0);
	
	if(em->mTimer_thread_flag){
		db_msg("by hero *** is over\n");
		return NULL;
	}
	printf("NetTimeThread step 1\n");
    //pthread_mutex_lock(&em->net_lock);
	em->mTimer_thread_flag = 1;
	em->mNetTime_flag = 0;
	db_msg("NetTimeThread step 2\n");
	int sockfd;
	int resolv_fd;
	int ret;
	string ip;
	char need_set = 1;
	char *server = NULL;
	char buf[32] = {0};
	int cnt = 0;
	while(1) {
		if(!em->m_4GModule_flag){
			db_msg("NetTimeThread m_4GModule_flag fails\n");
			sleep(1);
			continue;
		}		
        if (em->GetNetIp("usb0", ip) < 0) {
           	db_msg("no activity net device found, set rtsp server ip to '%s\n", ip.c_str());
			em->mNetTime_flag = 1;
			break;
        }

		#ifdef DEBUG_NTP
		else{
			db_msg("activity net device found, set rtsp server ip to '%s\n", ip.c_str());
        }
		#endif
		//query_server("0.cn.poll.ntp.org");		
		if ( (sockfd = connect_to_server(server)) == -1) {
			printf("try again\n");
			usleep(100*1000);
			cnt++;
			if(cnt == 10){
				em->mNetTime_flag = 1;
				break;
			}
			continue;
		}
		send_packet(sockfd);
		struct pollfd pfd;		  
		pfd.fd = sockfd;		
		pfd.events = POLLIN;		
		ret = poll(&pfd, 1, 300);		 
		if (ret == 1) {
			printf("try setting time on host");
			/* Looks good, try setting time on host ... */
			struct timeval time;
			if (get_time_from_server(sockfd, &time) == -1) {
				close(sockfd);
				usleep(100*1000);
				continue;		
			}
			if (need_set) {
				struct timezone zone;
				zone.tz_minuteswest = timezone/60 - 60*daylight;
				zone.tz_dsttime = 8;
				if (settimeofday(&time, &zone) < 0) {
					fprintf(stderr, "set time failed, %s, try again\n", strerror(errno));
					close(sockfd);
				}
				system("hwclock -w");
				MenuConfigLua::GetInstance()->UpdateSystemTime(true);
				em->mNetStatus = 1;
				em->mNetTime_flag = 0;				
				em->Notify(MSG_UPDATED_SYSTEM_TIEM_BY_4G);
			}
			close(sockfd);	
			printf("by hero *** set time ok!\n");
			break;			
		}else if (ret == 0) {			
			printf("timeout retry!\n"); 	
			close(sockfd);			  
			usleep(100*1000);	
			cnt++;
			if(cnt == 50){
				em->mNetTime_flag = 1;
				break;
			}			
			continue;		 
		}
	}
	
	printf("NetTimeThread step 3\n");

	em->mTimer_thread_flag = 0;
	
    //pthread_mutex_unlock(&em->net_lock);
    return NULL;

}


int EventManager::get_ntptime()
{
        printf("------------------------get_ntptime\n");
	ThreadCreate(&m_NetTimer_thread_id,NULL,EventManager::NetTimeThread,this);
	
	return 0;          /* All done! :) */	
}

int EventManager::USB4GSerial_init()
{
    if(usb4g_fd > 0) {
        close(usb4g_fd);
        usb4g_fd = -1;
    }
	db_error("%s enter \n",__func__);
#ifdef SIMCOM_4G_MODULE
	if((usb4g_fd=FindSerialPort(USB_AT)) < 0)
	{
		db_error("open at commond port error");
		goto err;
	}


#else
    if((usb4g_fd = openSerialPort("/dev/ttyUSB0")) < 0)
    {
        db_msg("open_port ttyUSB0 error");
		if((usb4g_fd = openSerialPort("/dev/ttyUSB1")) < 0){
			 db_msg("open_port ttyUSB1 error");
			 if((usb4g_fd = openSerialPort("/dev/ttyUSB2")) < 0){
				 db_msg("open_port ttyUSB2 error");
				 goto err;
			}
		}
		
    }
#endif
    if(configureSerialPort(usb4g_fd,115200,8,'N',1) < 0)
    {
        db_msg("set_opt error");
		close(usb4g_fd);
		usb4g_fd = -1;
		goto err;
    }	
db_error("%s exit \n",__func__);
	return 0;
	
err:
	return -1;
}

int EventManager::USB4GSerial_exit()
{
    if(usb4g_fd >= 0)
    {
		close(usb4g_fd);
		usb4g_fd = -1;
    }
	return 0;
}


int EventManager::USB4GModules_init()
{	
	char buf[512] = {0};
	int ret = 0;
	char *ptr, *ptr1;
	char data[128] = {0};
	int net4g_flag = 0;
    int retry = TIMES_RETRY;

    //check tty
	while(1){
		if(CheckttyUsb()){
			ret = USB4GSerial_init();
			if(ret == 0){
				break;
			}
		}
		usleep(500*1000);
	}
	
	// write AT
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
			USB4G_ComWrite(usb4g_fd, "ATE1", strlen("ATE1"));
			usleep(5*1000); 
			USB4G_ComWrite(usb4g_fd, "\r\n", 2);
			usleep(5*1000); 		
			ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
	}
	ret = 0;
    retry = TIMES_RETRY;
	bzero(buf, sizeof(buf));
	#ifdef SIMCOM_4G_MODULE


	while(1){		//wait for sim ready
		USB4G_ComWrite(usb4g_fd, "AT+CPIN?", strlen("AT+CPIN?"));
		usleep(5*1000);	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+CPIN: ")) != NULL){
			if((ptr1 = strstr(ptr, "READY")) != NULL){
				break;
			}
		}
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	

	while(ret <= 0){		//open gps
		USB4G_ComWrite(usb4g_fd, "AT+CGPS=1", strlen("AT+CGPS=1"));
		usleep(5*1000);	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));
	
		while(ret <= 0){		//open gps
		USB4G_ComWrite(usb4g_fd, "ATS0=1", strlen("ATS0=1"));
		usleep(5*1000);	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));
#if 0
	// write CGMI
	while(ret <= 0){		
		USB4G_ComWrite(usb4g_fd, "AT+CGMI", strlen("AT+CGMI"));
		usleep(5*1000);	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "AT+CGMI\r\r\n")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				strncpy(data, ptr+strlen("AT+CGMI\r\r\n"), ptr1-ptr-strlen("AT+CGMI\r\r\n"));
				facturer = data;
			}
		}		
	}
	ret = 0;
	bzero(buf, sizeof(buf));	
	bzero(data, sizeof(data));
	db_error("facturer :%s",facturer.c_str());

	
	while(ret <= 0){		
		USB4G_ComWrite(usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
		usleep(5*1000); 	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+ICCID: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
				sim = data;
			}
		}
		if(!strcmp(sim.c_str(), "")){
			ret = -1;
			db_msg("USB4GModules_init sim is null");
		}

	}
	db_error("sim :%s",sim.c_str());

	ret = 0;	
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));		
	
	db_msg("by hero *** USB4G_Init step2");
	while(1){		
		USB4G_ComWrite(usb4g_fd, "AT+CGSN", strlen("AT+CGSN")); 
		usleep(5*1000);		
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "AT+CGSN\r\r\n")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				if(ptr1>ptr)
					{
						strncpy(data, ptr+strlen("AT+CGSN\r\r\n"), ptr1-ptr-strlen("AT+CGSN\r\r\n"));
						imei = data;
						break;
					}
			}
		}
	}
	ret = 0;
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));

	db_error("imei :%s",imei.c_str());

	while(ret <= 0){		
		USB4G_ComWrite(usb4g_fd, "AT+CGACT=1,1", strlen("AT+CGACT=1,1"));
		usleep(5*1000);	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));		
#endif

#else
	// write CGMI
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+CGMI", strlen("AT+CGMI"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 			
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+CGMI:")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+CGMI:"), ptr1-ptr-strlen("+CGMI:"));
				facturer = data;
			}
		}		
	}
	ret = 0;
	bzero(buf, sizeof(buf));	
	bzero(data, sizeof(data));
	#ifdef ZTE_4G_MODULE
	if(!strncmp(facturer.c_str(), " Thinkwill", strlen(" Thinkwill"))) {
		db_msg("by hero *** THINKWILL facturer = %s\n", facturer.c_str());
		m_facturer = THINKWILL;
        retry = TIMES_RETRY;
		while(ret <= 0){		
            if(retry-- <= 0) {
                db_error("retry failed!");
                return -1;
            }
			USB4G_ComWrite(usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
			usleep(5*1000); 	
			USB4G_ComWrite(usb4g_fd, "\r\n", 2);
			usleep(5*1000); 				
			ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
			#if 0
			if((ptr = strstr(buf, "+ICCID: ")) != NULL){
				if((ptr1 = strstr(ptr, "\r\n")) != NULL){
					strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
					sim = data;
				}
			}
			#endif
		}		
	} else {
		db_msg("by hero *** other facturer = %s\n", facturer.c_str());
		if(!strncmp(facturer.c_str(), " ZTE", strlen(" ZTE"))) {
			m_facturer = ZTE;
            retry = TIMES_RETRY;
			while(ret <= 0){		
                if(retry-- <= 0) {
                    db_error("retry failed!");
                    return -1;
                }
				USB4G_ComWrite(usb4g_fd, "AT+ZICCID?", strlen("AT+ZICCID?")); 
				usleep(5*1000); 	
				USB4G_ComWrite(usb4g_fd, "\r\n", 2);
				usleep(5*1000); 				
				ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
				usleep(5*1000);
				#if 0
				if((ptr = strstr(buf, "+ZICCID:")) != NULL){
					if((ptr1 = strstr(ptr, "\r\n")) != NULL){
						strncpy(data, ptr+strlen("+ZICCID:"), ptr1-ptr-strlen("+ZICCID:"));
						sim = data;
					}
				}
				#endif
			}		
		}
	}
	#else
    retry = TIMES_RETRY;
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
		usleep(5*1000); 	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+ICCID: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
				sim = data;
			}
		}	
		if(!strcmp(sim.c_str(), "")){
			ret = -1;
			db_msg("USB4GModules_init sim is null");
		}
	}

	#endif
	ret = 0;	
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));		
    retry = TIMES_RETRY;
	
	db_msg("by hero *** USB4G_Init step2");
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+CGSN", strlen("AT+CGSN")); 
		usleep(5*1000); 	
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		#if 0
		if((ptr = strstr(buf, "+CGSN: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+CGSN: "), ptr1-ptr-strlen("+CGSN: "));
				imei = data;
			}
		}
		#endif
	}
	ret = 0;
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));
    retry = TIMES_RETRY;
	
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT^SYSCONFIG=19,7,1,2", strlen("AT^SYSCONFIG=19,7,1,2"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 			
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	
    retry = TIMES_RETRY;
	
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+CPIN?", strlen("AT+CPIN?"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	
	#if 0
LOOP:
	while(ret <= 0){		
		USB4G_ComWrite(usb4g_fd, "AT^SYSINFO", strlen("AT^SYSINFO"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 		
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}

	if((ptr = strstr(buf, "^SYSINFO: ")) != NULL) {
		ptr1 = getStrBehindComma(ptr, 1);
		if(ptr1 != NULL){
			strncpy(data, ptr+strlen("^SYSINFO: "), ptr1-ptr-strlen("^SYSINFO: ")-strlen(","));
			db_msg("by hero *** SYSINFO = %s\n", data);
			if(!strcmp(data, "2")){
				net4g_flag = 1;
			}else{
				net4g_flag = 0;
			}
		}
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	
	bzero(data, sizeof(data));

	if(net4g_flag == 0){
		while(ret <= 0){		
			USB4G_ComWrite(usb4g_fd, "AT+CREG?", strlen("AT+CREG?"));
			usleep(5*1000); 
			USB4G_ComWrite(usb4g_fd, "\r\n", 2);
			usleep(5*1000); 		
			ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
		}	
		if((ptr = strstr(buf, "+CREG: 1")) != NULL) {
			strncpy(data, ptr+strlen("+CREG: 1,"), 1);
			if(!strcmp(data, "0")){
				ret = 0;	
				bzero(buf, sizeof(buf));	
				bzero(data, sizeof(data));
				goto LOOP;
			}
		}else{
			goto LOOP;
		}
		net4g_flag = 1;
		ret = 0;	
		bzero(buf, sizeof(buf));	
		bzero(data, sizeof(data));
	}
	#endif

    retry = TIMES_RETRY;
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+ZGACT=0,1", strlen("AT+ZGACT=0,1"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));
    retry = TIMES_RETRY;
	
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+CGACT=1,1", strlen("AT+CGACT=1,1"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 			
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));	
    retry = TIMES_RETRY;
	
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT+ZGACT=1,1", strlen("AT+ZGACT=1,1"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}

	ret = 0;
	bzero(buf, sizeof(buf));		
    retry = TIMES_RETRY;
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT^WAKESMSHEAD=1,12345678", strlen("AT^WAKESMSHEAD=1,12345678"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}	

	ret = 0;
	bzero(buf, sizeof(buf));		
    retry = TIMES_RETRY;
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT^WAKECONFIG=1,0,30,iothub.xiaojukeji.com,1884", strlen("AT^WAKECONFIG=1,0,30,iothub.xiaojukeji.com,1884"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}	
	
	ret = 0;
	bzero(buf, sizeof(buf));		
    retry = TIMES_RETRY;
	while(ret <= 0){		
        if(retry-- <= 0) {
            db_error("retry failed!");
            return -1;
        }
		USB4G_ComWrite(usb4g_fd, "AT^HEARTBEAT=12345678", strlen("AT^HEARTBEAT=12345678"));
		usleep(5*1000); 
		USB4G_ComWrite(usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = USB4G_ComRead(usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
#endif
	return 0;
}

#if 1
bool EventManager::IsGotIP(const char *interface, char *IP)
{
    int sockfd = -1;
    struct ifreq ifr;
    struct sockaddr_in *addr = NULL;
    
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        db_error("create socket error!\n");
        return false;
    }
    
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
        strcpy(IP, inet_ntoa(addr->sin_addr));
        close(sockfd);
#ifdef DEBUG_4G
        //db_error("IP:%s\n", IP);
        memset(ip_4g, 0, sizeof(ip_4g));
        strncpy(ip_4g, IP, sizeof(ip_4g));
        gotip_count++;
        if(gotip_count > 2) {
            gotip_count = 20;
#if 0
            db_error("4G net performance perfect, retest it , now reboot system");
            is_4g_need_reboot = 1;
#endif
        }
#endif
        return true;
    }
    
    close(sockfd);
    return false;
}

bool EventManager::IsNetReady(const char *interface, char *IP)
{
    int ret = 0;
    char buf[8] = {0};
    FILE *fp = NULL;
    
    if(! IsGotIP(interface,IP)){
        return false;
    }
    
    fp = popen("ping -c 1 iothub.xiaojukeji.com > /dev/null ; echo $?","r");
    if(fp == NULL) {
        db_error("open ping failed!");
        return false;
    }
    if(fgets(buf, sizeof(buf), fp) == NULL) {
        //db_error("fgets failed!");
        pclose(fp);
        return false;
    }
    if(strncmp(buf, "0",1) == 0) {
        //db_error("net is  ok buf %s !",buf);
        pclose(fp);
        return true;
    }
    else {
        //db_error("net is not ok buf %s !",buf);
        pclose(fp);
        return false;
    }
    
    pclose(fp);
    return false;
}

#endif

void *EventManager::USB4G_Init(void * context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	prctl(PR_SET_NAME, "USB4G_Init", 0, 0, 0);
	
	//sleep(10); //test
	char buf[512] = {0};
	int ret = 0,i;
	char *ptr, *ptr1;
	char data[128] = {0};
	int net4g_flag = 0;
	bool need_reset_apn = 0;
	
	//em->SetUSB4GModuleReset();
	//check tty
	while(1){
		if(em->CheckttyUsb()){
			ret = em->USB4GSerial_init();
			if(ret == 0){
				break;
			}
		}
		usleep(500*1000);
	}
		

#ifdef SIMCOM_4G_MODULE
	ret = 0;
	bzero(buf, sizeof(buf));	
	while(ret <= 0){		//open gps
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGPS=1", strlen("AT+CGPS=1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
    usleep(500*1000);
	ret = 0;	
	bzero(buf, sizeof(buf));	
	
	db_msg("by hero *** USB4G_Init step2");
	while(1){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGSN", strlen("AT+CGSN")); 
		usleep(5*1000);		
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "AT+CGSN\r\r\n")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				if(ptr1>ptr)
					{
						strncpy(data, ptr+strlen("AT+CGSN\r\r\n"), ptr1-ptr-strlen("AT+CGSN\r\r\n"));
						em->imei = data;
						break;
					}
			}
		}
	}
	db_error("em->imei :%s",em->imei.c_str());

	ret = 0;	
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));	

	// write CGMI
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGMI", strlen("AT+CGMI"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "AT+CGMI\r\r\n")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				strncpy(data, ptr+strlen("AT+CGMI\r\r\n"), ptr1-ptr-strlen("AT+CGMI\r\r\n"));
				em->facturer = data;
			}
		}		
	}
	db_error("em->facturer :%s",em->facturer.c_str());
	ret = 0;	
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));	

    while(1){		//check apn
        em->USB4G_ComWrite(em->usb4g_fd, "AT+CGDCONT?", strlen("AT+CGDCONT?"));
        usleep(5*1000); 
        em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
        usleep(5*1000); 				
        ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
        usleep(5*1000);
		if((ptr = strstr(buf, "+CGDCONT: 1")) != NULL){
	        if((ptr = strstr(buf, "+CGDCONT: 6")) != NULL){
	            if((ptr1 = strstr(ptr, "+CGDCONT: 6,\"IPV4V6\",\"\"")) != NULL){
	                db_error("not need reset AT+CGDCONT=6  %s \n",ptr);
	            }
	            else
	            {
	                need_reset_apn = true;
	                db_error("need reset AT+CGDCONT=6 %s \n",ptr);
	            }      
	        }else{
				db_error("have no CGDCONT: 6, not need care \n");
			}
		break;
		}
    }
	
	ret = 0;	
	bzero(buf, sizeof(buf));	

	if(need_reset_apn)
	{
		while(1){		//clean apn
			em->USB4G_ComWrite(em->usb4g_fd, "AT+CGDCONT=6,\"IPV4V6\",\"\"", strlen("AT+CGDCONT=6,\"IPV4V6\",\"\""));
			usleep(5*1000); 
			em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
			usleep(5*1000); 				
			ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
			
			if((ptr = strstr(buf, "OK")) != NULL){
				db_error("AT+CGDCONT=6 ok \n");
				break;
			}
		}
	}

	ret = 0;	
	bzero(buf, sizeof(buf));	

	while(1){		//SWITCH PID
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CUSBPIDSWITCH?", strlen("AT+CUSBPIDSWITCH?"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		
		if((ptr = strstr(buf, "+CUSBPIDSWITCH: ")) != NULL){
			if((ptr1 = strstr(ptr, "9011")) != NULL){
				em->mNeedSwitchPid = false;
				db_error("NOT mNeedSwitchPid %s\n",ptr);
			}
			break;
		}
	}
	
    ret = 0;	
	bzero(buf, sizeof(buf));	
	
	if(em->mNeedSwitchPid)
	{
		db_error("mNeedSwitchPid write start \n");
		while(ret <= 0){		//SWITCH PID
			em->USB4G_ComWrite(em->usb4g_fd, "AT+CUSBPIDSWITCH=9011,1,1", strlen("AT+CUSBPIDSWITCH=9011,1,1"));
			usleep(5*1000); 
			em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
			usleep(5*1000); 				
			ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
			usleep(500*1000);
			db_error("mNeedSwitchPid write done \n");
		}
	}

	if(need_reset_apn || em->mNeedSwitchPid)
	{
		reboot(RB_AUTOBOOT);
		return NULL;
	}
	
    ret = 0;	
	bzero(buf, sizeof(buf));	
	// write AT
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CSCLK=1", strlen("AT+CSCLK=1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);			
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));

	// write AT
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "ATE1", strlen("ATE1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);			
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));
		
	em->mOpenSimcomGpsDone = true;
	ret = 0;	
	bzero(buf, sizeof(buf));

	while(1){		//wait for sim ready
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CPIN?", strlen("AT+CPIN?"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+CPIN: ")) != NULL){
			if((ptr1 = strstr(ptr, "READY")) != NULL){
				break;
			}
		}
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	

	while(ret <= 0){		//open ATS0
		em->USB4G_ComWrite(em->usb4g_fd, "ATS0=1", strlen("ATS0=1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}

	ret = 0;
	bzero(buf, sizeof(buf));	
	
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
		usleep(5*1000); 	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+ICCID: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n\r\n")) != NULL){
				strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
				em->sim = data;
			}
		}
		if(!strcmp(em->sim.c_str(), "")){
			ret = -1;
			db_msg("USB4GModules_init sim is null");
		}

	}
	db_error("em->sim :%s",em->sim.c_str());

	ret = 0;
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));
		
	usleep(1000*1000);
	em->USB4G_OpenUdhcp();
	em->m_4GModule_flag = 1;

#else
	// write CGMI
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGMI", strlen("AT+CGMI"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+CGMI:")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+CGMI:"), ptr1-ptr-strlen("+CGMI:"));
				em->facturer = data;
			}
		}		
	}
	ret = 0;
	bzero(buf, sizeof(buf));	
	bzero(data, sizeof(data));

	#ifdef ZTE_4G_MODULE
	if(!strncmp(em->facturer.c_str(), " Thinkwill", strlen(" Thinkwill"))) {
		db_msg("by hero *** THINKWILL facturer = %s\n", em->facturer.c_str());
		em->m_facturer = THINKWILL;
		while(ret <= 0){		
			em->USB4G_ComWrite(em->usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
			usleep(5*1000);		
			em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
			usleep(5*1000);					
			ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
			if((ptr = strstr(buf, "+ICCID: ")) != NULL){
				if((ptr1 = strstr(ptr, "\r\n")) != NULL){
					strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
					em->sim = data;
				}
			}
		}		
	} else {
		db_msg("by hero *** other facturer = %s\n", em->facturer.c_str());
		if(!strncmp(em->facturer.c_str(), " ZTE", strlen(" ZTE"))) {
			em->m_facturer = ZTE;
			while(ret <= 0){		
				em->USB4G_ComWrite(em->usb4g_fd, "AT+ZICCID?", strlen("AT+ZICCID?")); 
				usleep(5*1000);		
				em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
				usleep(5*1000);					
				ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
				usleep(5*1000);
				if((ptr = strstr(buf, "+ZICCID:")) != NULL){
					if((ptr1 = strstr(ptr, "\r\n")) != NULL){
						strncpy(data, ptr+strlen("+ZICCID:"), ptr1-ptr-strlen("+ZICCID:"));
						em->sim = data;
					}
				}
			}	
		}
	}
	#else
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+ICCID", strlen("AT+ICCID")); 
		usleep(5*1000); 	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+ICCID: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+ICCID: "), ptr1-ptr-strlen("+ICCID: "));
				em->sim = data;
			}
		}
		if(!strcmp(em->sim.c_str(), "")){
			ret = -1;
			db_msg("USB4GModules_init sim is null");
		}

	}
	#endif
	ret = 0;	
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));		
	
	db_msg("by hero *** USB4G_Init step2");
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGSN", strlen("AT+CGSN")); 
		usleep(5*1000);		
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
		if((ptr = strstr(buf, "+CGSN: ")) != NULL){
			if((ptr1 = strstr(ptr, "\r\n")) != NULL){
				strncpy(data, ptr+strlen("+CGSN: "), ptr1-ptr-strlen("+CGSN: "));
				em->imei = data;
			}
		}
		if(!strcmp(em->imei.c_str(), "")){
			ret = AdapterLayer::GetInstance()->getProductInfo("imei", em->imei);
		}
	}
	ret = 0;
	bzero(buf, sizeof(buf));
	bzero(data, sizeof(data));

	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT^SYSCONFIG=19,7,1,2", strlen("AT^SYSCONFIG=19,7,1,2"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	

	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CPIN?", strlen("AT+CPIN?"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);					
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;	
	bzero(buf, sizeof(buf));
	#if 0
LOOP:
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT^SYSINFO", strlen("AT^SYSINFO"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);			
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}

	if((ptr = strstr(buf, "^SYSINFO: ")) != NULL) {
		ptr1 =  getStrBehindComma(ptr, 1);
		if(ptr1 != NULL){
			strncpy(data, ptr+strlen("^SYSINFO: "), ptr1-ptr-strlen("^SYSINFO: ")-strlen(","));
			db_msg("by hero *** SYSINFO = %s\n", data);
			if(!strcmp(data, "2")){
				net4g_flag = 1;
			}else{
				net4g_flag = 0;
			}
		}
	}
	ret = 0;	
	bzero(buf, sizeof(buf));	
	bzero(data, sizeof(data));

	if(net4g_flag == 0){
		while(ret <= 0){		
			em->USB4G_ComWrite(em->usb4g_fd, "AT+CREG?", strlen("AT+CREG?"));
			usleep(5*1000);	
			em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
			usleep(5*1000);			
			ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
			usleep(5*1000);
		}	
		if((ptr = strstr(buf, "+CREG: 1")) != NULL) {
			strncpy(data, ptr+strlen("+CREG: 1,"), 1);
			if(!strcmp(data, "0")){
				ret = 0;	
				bzero(buf, sizeof(buf));	
				bzero(data, sizeof(data));
				goto LOOP;
			}
		}else{
			goto LOOP;
		}
		net4g_flag = 1;
		ret = 0;	
		bzero(buf, sizeof(buf));	
		bzero(data, sizeof(data));
	}
	#endif
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+ZGACT=0,1", strlen("AT+ZGACT=0,1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);		
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));

	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+CGACT=1,1", strlen("AT+CGACT=1,1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);				
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	ret = 0;
	bzero(buf, sizeof(buf));	

	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT+ZGACT=1,1", strlen("AT+ZGACT=1,1"));
		usleep(5*1000);	
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000);		
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	
	ret = 0;
	bzero(buf, sizeof(buf));		

	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT^WAKESMSHEAD=1,12345678", strlen("AT^WAKESMSHEAD=1,12345678"));
		usleep(5*1000); 
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}	

	ret = 0;
	bzero(buf, sizeof(buf));		
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT^WAKECONFIG=1,0,30,iothub.xiaojukeji.com,1884", strlen("AT^WAKECONFIG=1,0,30,iothub.xiaojukeji.com,1884"));
		usleep(5*1000); 
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}	
	
	ret = 0;
	bzero(buf, sizeof(buf));		
	while(ret <= 0){		
        std::string username;
        std::string secret;
        std::string decode_username;
        std::string decode_secret;
        char atcmd[256] = {0};
        AdapterLayer::GetInstance()->getUserInfo(username, secret);
        if(strcmp(username.c_str(), "") != 0 && strcmp(secret.c_str(), "") != 0) {
            decode_username.clear();
            decode_secret.clear();
            decode_username = AdapterLayer::GetInstance()->base64_decode(username);
            decode_secret = AdapterLayer::GetInstance()->base64_decode(secret);
            snprintf(atcmd, sizeof(atcmd),"AT+MQTTCONFIG=%s,%s", decode_username.c_str(), decode_secret.c_str());
            db_msg("atcmd:%s\n", atcmd);
		    em->USB4G_ComWrite(em->usb4g_fd, atcmd, strlen(atcmd));
        }
        else {
            db_msg("atcmd not send\n" );
            break;  //not send AT+MQTTCONFIG,else 4G will connect with this invalid user
            //send Invaild user secret
		    em->USB4G_ComWrite(em->usb4g_fd, "AT+MQTTCONFIG=12345678,12345678", strlen("AT+MQTTCONFIG=12345678,12345678"));
        }
		usleep(5*1000); 
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}
	
	ret = 0;
	bzero(buf, sizeof(buf));		
	while(ret <= 0){		
		em->USB4G_ComWrite(em->usb4g_fd, "AT^HEARTBEAT=12345678", strlen("AT^HEARTBEAT=12345678"));
		usleep(5*1000); 
		em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
		usleep(5*1000); 	
		ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
		usleep(5*1000);
	}

	usleep(1000*1000);
	em->USB4G_OpenUdhcp();
	em->m_4GModule_flag = 1;
#endif

	return NULL;
}

void EventManager::USB4G_OpenUdhcp()
{
    usleep(500*1000);   //need to sleep
	StorageManager::GetInstance()->my_system("killall udhcpc &");
	StorageManager::GetInstance()->my_system("udhcpc -i usb0 &");
	db_error("udhcpc -i usb0 \n");

#ifdef DEBUG_4G
    reinit_4g_count++;
#endif
#ifdef ENABLE_4G_POWER_RESET
#ifdef DEBUG_4G
    db_error("reset4GModuleCount:%d", reset4GModuleCount);
#endif
    reset4GModuleCount++;
    if(reset4GModuleCount == POWER_RESET_FIRST) {
#ifdef DEBUG_4G
        db_error("4G power reset...");
        repower_4g_count++;
#endif
        if(usb4g_fd > 0) {
            close(usb4g_fd);
        }
		WriteFileInt(AP_4G_PWR,1);
        usleep(500*1000);
		WriteFileInt(AP_4G_PWR,0);
        usleep(10*1000*1000);
    }
    if(reset4GModuleCount > POWER_RESET_FIRST) {
        reset4GModuleCount = POWER_RESET_INVAILD;
    }
#endif
}

int EventManager::USB4G_ComRead(int fd, char *buf, int len)
{
	int ret;
	if(fd < 0)
		return -1;	
	if((ret = read(fd, buf, len)) < 0){
		#ifdef DEBUG_4G
		//db_msg("by hero *** USB4G_ComRead error ret = %d", ret);
		#endif
		return -1;
	}
	#ifdef DEBUG_4G
	//db_msg("by hero *** read buf = %s, ret = %d", buf, ret);	
	#endif
	return ret;
}

int EventManager::USB4G_ComWrite(int fd, char *buf, int len)
{
	int ret;
	if(fd < 0)
		return -1;
	if ((ret = write(fd, buf, len)) < 0) {  
		#ifdef DEBUG_4G
		//db_msg("by hero *** USB4G_ComWrite error ret = %d", ret);
		#endif
		return -1;
	}
	#ifdef DEBUG_4G
	//db_msg("by hero *** read buf = %s, ret = %d", buf, ret);	
	#endif
	return 0;
}

#if 0
void *EventManager::CheckWakeUpEvent(void * context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	prctl(PR_SET_NAME, "CheckWakeUpEvent", 0, 0, 0);
}
#endif
void *EventManager::CheckImpackEvent(void * context)
{
        db_error("CheckImpackEvent Load gsensor driver after IOVDD-CSI ON \n");
        system("insmod /lib/modules/4.9.118/da380.ko");
        usleep(500*1000);

        GsensorManager::GetInstance()->RunGsensorManager();
        EventManager *em = reinterpret_cast<EventManager*>(context);
        prctl(PR_SET_NAME, "CheckImpackEvent", 0, 0, 0);
        bool OverFlag = true;
        while(1)
        {
            bool status = GsensorManager::GetInstance()->getImpactStatus();
            //printf("GsensorManager::GetInstance()->getImpactStatus() :%d OverFlag:%d\n", status, OverFlag);
            if(status && OverFlag)
            {
                int curWinId=WindowManager::GetInstance()->GetCurrentWinID();
                int sd_status = 0;
                em->CheckEvent(EventManager::EVENT_SD, sd_status);
                db_error("CheckImpackEvent MSG_IMPACT_HAPPEN++++++++++++++ curWinId=%d sd_status=%d \n", curWinId, sd_status);
                OverFlag = false;
                
                if(curWinId==WINDOWID_PREVIEW)
                {
                    if(sd_status>0)
                    {
                        em->Notify(MSG_IMPACT_HAPPEN);
                    }
                    else
                    {
                        db_error("CheckImpackEvent MSG_IMPACT_HAPPEN++++++++++++++ NO SD Card!!!");
                    }
                }
                else
                {
                    db_error("CheckImpackEvent is not WINDOWID_PREVIEW,  not deal impact video \n");
                }
                GsensorManager::GetInstance()->setHandleFinish();
                OverFlag = true;
            }
            usleep(500*1000);
        }
        return NULL;
}

int EventManager::Get4gSignalLevel()
{
	//date ZTE 100~199, THINKWILL 0~31 -->4 level
	if(!mNetStatus)
		return 0;
	
	#ifdef ZTE_4G_MODULE
	if(m_facturer == THINKWILL){
		return (atoi(m_NetSignal.c_str()) / 8); 
	}else if(m_facturer == ZTE){
		return ((atoi(m_NetSignal.c_str()) - 100) / 25);
	}
	return 1;
	#else
	return (atoi(m_NetSignal.c_str()) / 8); 
	#endif
}

void *EventManager::CheckNetEvent(void *context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	prctl(PR_SET_NAME, "CheckNetEvent", 0, 0, 0);

	int ret = 0;
	int write_cnt = 5;
	int ip_cnt = 0;
	int ntp_cnt = 0;
	char buf[128] = {0};
	char *ptr, *ptr1, *ptr2;
	char data[32] = {0};
	string ip;
#ifdef DEBUG_4G
    int mNetStatus_old = 0;
    int debug_4g_count = 0;
#endif

	while(1){
#ifdef DEBUG_4G
        if(mNetStatus_old != em->mNetStatus) {
            db_error("mNetStatus:%d", em->mNetStatus);
            mNetStatus_old = em->mNetStatus;
        }
#endif
		//check 4g signal
		if(!em->m_4GModule_flag){
			usleep(50*1000);	
			continue;
		}

		if(!em->CheckttyUsb()){	
			db_msg("by hero *** CheckNetEvent usb4g_fd close");
			em->mNetStatus = 0;
			em->mNetTime_flag = 1;			
			em->USB4GSerial_exit();
			//not use
			//em->SetUSB4GModuleReset();
			while(1){
#ifdef DEBUG_4G
                db_error("CheckttyUsb");
#endif
				if(em->CheckttyUsb()){
					ret = em->USB4GSerial_init();
					if(ret == 0){
						break;
					}
				}
				usleep(1000*1000);
			}			
		}else{
	        if (em->GetNetIp("usb0", ip) < 0) {
#ifdef DEBUG_4G
                db_error("GetNetIp");
#endif
				write_cnt = 5;
				while(write_cnt--){
					bzero(buf, sizeof(buf));
					bzero(data, sizeof(data));
					em->USB4G_ComWrite(em->usb4g_fd, "AT+CGREG?", strlen("AT+CGREG?"));
					usleep(5*1000); 
					em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
					usleep(5*1000); 			
					ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
					if(ret > 0)
						break;
					usleep(5*1000);
				}		
				if((ptr = strstr(buf, "+CGREG: 0")) != NULL) {
					strncpy(data, ptr+strlen("+CGREG: 0,"), 1);
					#ifdef DEBUG_4G
					db_msg("by hero ***ptr = %s, +CGREG: 0, = %s\n",ptr, data);
					#endif
					if(strcmp(data, "1") && strcmp(data, "5")){
						em->mNetStatus = 0;
						em->mNetTime_flag = 1;
						//em->SetUSB4GModuleReset();
			            if(em->USB4GModules_init() < 0) {
			                usleep(3000*1000);
                            continue;
                        }
						usleep(1000*1000);
						em->USB4G_OpenUdhcp();							
						usleep(5000*1000);
					}
				}
				//em->USB4G_OpenUdhcp();
				ip_cnt++;
				if(ip_cnt == 5){				
			        if(em->USB4GModules_init() < 0) {
			            usleep(3000*1000);
                        continue;
                    }
					usleep(1000*1000);
					em->USB4G_OpenUdhcp();
					usleep(5000*1000);
					ip_cnt = 0;
				}
				usleep(2000*1000);
				continue;
		    }else if(em->mNetTime_flag){
				em->get_ntptime();
				usleep(1000*1000);
				#if 0
				ntp_cnt++;
				if(ntp_cnt == 5){
					em->SetUSB4GModuleReset();
					ntp_cnt = 0;
				}	
				#endif
			}

			while(write_cnt--){
				bzero(buf, sizeof(buf));
				bzero(data, sizeof(data));
				em->USB4G_ComWrite(em->usb4g_fd, "AT+CSQ", strlen("AT+CSQ"));
				usleep(5*1000); 
				em->USB4G_ComWrite(em->usb4g_fd, "\r\n", 2);
				usleep(5*1000); 			
				ret = em->USB4G_ComRead(em->usb4g_fd, buf, sizeof(buf));		
				if(ret > 0)
					break;
				usleep(5*1000);
			}		
			if((ptr = strstr(buf, "+CSQ: ")) != NULL) {
				ptr1 = getStrBehindComma(ptr, 1);
				if(ptr1 != NULL){
					strncpy(data, ptr+strlen("+CSQ: "), ptr1-ptr-strlen("+CSQ: ")-strlen(","));
                    if(strncmp(data, "99", 2) == 0) {
					    em->m_NetSignal = "0";
                    }
                    else {
					    em->m_NetSignal = data;
                    }
				}
			}
		
			//check 4g signal
		
			//check 4g state
			write_cnt = 5;	
		
			//check 4g state		

            char ip[32] = {0};
            if(! em->IsGotIP("usb0", ip)) {
                db_error("to init 4G and open udhcpc to get ip...");
				em->mNetStatus = 0;
			    if(em->USB4GModules_init() < 0) {
			        usleep(3000*1000);
                    continue;
                }
			    usleep(1000*1000);
			    em->USB4G_OpenUdhcp();
			    usleep(3000*1000);
            }
            else {
                if(em->mNetStatus != 1) {
				    em->mNetStatus = 1;
                }
            }
			usleep(5000*1000);
		}
	}
	return NULL;
}

void EventManager::SetUSB4GModuleStandby(bool flag)
{
	int dc_outstatus = 0;
	if(flag)
	{
		db_msg("by hero *** SetUSB4GModuleStandby true");
		m_4GModule_flag = 0;
		mStandbyStatus = true;
		USB4GSerial_exit();
		WriteFileInt(AP_WAKEUP_M,0);
		//system("echo 1 > /sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/enable_acc");
//		CheckEvent(EventManager::EVENT_ACC, dc_outstatus);
//		dc_outstatus = GetDCconnetStatus();
		if(mAccIn_flag == true && dc_outstatus == 0) {
			mAccIn_flag = false;
			db_error("========reset mAccIn_flag========");
		}
		if(mNotify_acc_off_flag == true)
		{
			db_error("==========reset mNotify_acc_off flag==========");
			mNotify_acc_off_flag = false;
		}
		if(mNotify_acc_on_flag == true)
		{
			db_error("==========reset mNotify_acc_on_flag flag==========");
			mNotify_acc_on_flag = false;
		}
	}else{
		db_msg("by hero *** SetUSB4GModuleStandby false");
		m_4GModule_flag = 1;
		WriteFileInt(AP_WAKEUP_M,1);
		//system("echo 0 > /sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/enable_acc");
		ResetAccStatus();
		//SetUSB4GModuleReset();

	}
}

void EventManager::SetUSB4GModuleReset()
{
	WriteFileInt(AP_4G_RST,1);
	usleep(200*1000);
	WriteFileInt(AP_4G_RST,0);
}

int EventManager::getBindFlagString(char *buffer)
{
    std::string bindflag = DD_GLOBALINFO::GetInstance()->getBindflag();
	snprintf(buffer, 8, "%s", bindflag.c_str());
	return 0;
	
}

int EventManager::IRledCtrl(bool flag)
{
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	Recorder *loop_rec = NULL,*short_rec = NULL;
	int CamId = 1;
	static bool IrisOpen_flag = false;
	if( NULL == m_CamRecCtrl )
	{
		db_error("m_CamRecCtrl is null\n");
		return -1;
	}
	loop_rec = m_CamRecCtrl->GetRecorder(CamId, 2);
		if( loop_rec == NULL ){
		db_error("loop_rec is NULL!!");
		return -1;
	}

	short_rec = m_CamRecCtrl->GetRecorder(CamId, 3);
	if( short_rec == NULL ){
			db_error("short_rec is NULL!!");
			return -1;
	}
	if((!loop_rec->RecorderIsBusy()) && (!short_rec->RecorderIsBusy())) //后台录像关闭
	{
		if(IrisOpen) {
			db_error("==============black recorder is stop,close ir_led==============");
			setIrLedBrightness(IRLedOff);
			IrisOpen = false;
			IrisOpen_flag = false;
		}
		return -1;
	}
	if(IrisOpen_flag == true)
		return 0;
	if(flag)
	{
		setIrLedBrightness(IRLedOn);
		IrisOpen = true;
		IrisOpen_flag = true;
#ifdef DEBUG_IR_LED
		db_error("==============open ir_led==============");
#endif
	}else{
		setIrLedBrightness(IRLedOff);
		IrisOpen = false;
#ifdef		DEBUG_IR_LED
		db_error("==============close ir_led==============");
#endif
	}
	return 0;
}

void *EventManager::CheckIrCheckFunEvent(void *context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	char buf[16] = {0};
	while(1){
		int ret = getoneline(LIGHT_VALUE, buf, sizeof(buf));
		if (ret < 0)
			break;
		int stat = atoi(buf);
		//db_warn("[debug_jason]: the is value is %d\n",stat);
#ifdef DEBUG_IR_LED
		db_error("============light_value %d============",stat);
#endif
#if 0
		if(stat <= 50){
#ifdef DEBUG_IR_LED
			db_error("=====stat <= 50=====");
#endif
			em->IRledCtrl(true);
		} else if(stat >= 120) {
#ifdef DEBUG_IR_LED
			db_error("=====stat >= 120=====");
#endif
			em->IRledCtrl(false);
		}
#endif
		em->IRledCtrl(true);
		usleep(1000*1000);
	}
	return NULL;
}


void *EventManager::CheckBindFlagEvent(void *context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	prctl(PR_SET_NAME, "CheckBindFlagEvent", 0, 0, 0);
	//db_msg("[debug_jason]: CheckBindFlagEvent 111");
	char tmp_buf[64] = {0};
	static bool ubind_flag = false;
	PartitionManager *PtM = PartitionManager::GetInstance();
	while(1){
        std::string bindflag = DD_GLOBALINFO::GetInstance()->getBindflag();
		if( ((!strcmp(bindflag.c_str(), "false"))) && ubind_flag )
		{
			//db_msg("debug_zhb----> ready to ubind the window");
			em->Notify(MSG_UNBIND_SUCCESS);
			ubind_flag = false;
		}
		else if( ((!strcmp(bindflag.c_str(),"true"))) && (!ubind_flag) )
		{
			//db_msg("debug_zhb----> ready to bind the window");
			em->Notify(MSG_BIND_SUCCESS);
			ubind_flag= true;
		}

	#if 0
		if(em->getBindFlagString(tmp_buf) == 1)
		{	
			em->Notify(MSG_BIND_SUCCESS);
		}
		//db_msg("tmp_buf = %s\n",tmp_buf);
		if(!strcmp(tmp_buf,"true")){
			em->Notify(MSG_BIND_SUCCESS);
		}else if(!strcmp(tmp_buf,"false")){
			em->Notify(MSG_UNBIND_SUCCESS);
		}
	#endif
		usleep(1000*1000);
	}
	em->m_AccEvent_thread_id = 0;
}

bool EventManager::GetDCconnetStatus()
{
	PowerManager *pm = PowerManager::GetInstance(); 
	return pm->getACconnectStatus();
}

int EventManager::GetAccResumeStatus()
{
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
	db_msg("by hero GetAccResumeStatus = %d", menuconfiglua->GetMenuIndexConfig(SETTING_ACC_RESUME));
	return menuconfiglua->GetMenuIndexConfig(SETTING_ACC_RESUME);
}

void EventManager::ResetAccStatus()
{
	int wakeup_event_id = 0;
	int dc_state = 0;
	int acc_state = 0;
	int menu_config_value = 0;
	PowerManager *pm = PowerManager::GetInstance(); 
	wakeup_event_id = pm->GetWakeUpSource();
	if(wakeup_event_id & CPUS_WAKEUP_LOWBATT){
		db_error("ResetAccStatus *** MSG_BATTERY_LOW");
		Notify(MSG_BATTERY_LOW);
		low_battery_flag = true;
		db_error("========set low_battery_flag true========");
#ifdef WAKEUP_REBOOT
		return;
#endif
	}else if(wakeup_event_id & CPUS_WAKEUP_ALM0){
		db_msg("check battery status");
	}

	if(is_enable_acc == 1) {
		MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
		menu_config_value = menuconfiglua->GetMenuIndexConfig(SETTING_ACC_SWITCH); //更新配置选项
		dc_state = GetDCconnetStatus(); //从dc status获取状态
		CheckEvent(EventManager::EVENT_ACC, acc_state); //从acc status获取状态
		if(((wakeup_event_id & CPUS_WAKEUP_AC) || (wakeup_event_id & CPUS_WAKEUP_USB)) && (dc_state == 0)){ //dc模式下dc电源被拔除时系统唤醒
			db_error("===========wake up id is ac,but ac is no in===========");
#if 0
			Notify(MSG_ACCOFF_HAPPEN);
			db_error("send msg MSG_ACCOFF_HAPPEN");
			mStandbyStatus = true;
			mWakeup_event_flag = true;
			db_error("set standby status true,set mWakeup_event_flag true");
			return;
#endif
#ifdef WAKEUP_REBOOT
			//system("reboot -f");
			db_error("============reboot============");
			reboot(RB_AUTOBOOT);
			return;
#endif
		} else if(((wakeup_event_id & CPUS_WAKEUP_AC) || (wakeup_event_id & CPUS_WAKEUP_USB)) //acc模式下接入dc时系统唤醒
				&& (menu_config_value == 0) && (acc_state == 0)){
			db_error("===========wake up id is ac,menu config is acc,but acc is no in===========");
#if 0
			Notify(MSG_ACCOFF_HAPPEN);
			db_error("send msg MSG_ACCOFF_HAPPEN");
			mStandbyStatus = true;
			mWakeup_event_flag = true;
			db_error("set standby status true,set mWakeup_event_flag true");
			return;
#endif
#ifdef WAKEUP_REBOOT
			//system("reboot -f");
			db_error("============reboot============");
			reboot(RB_AUTOBOOT);
			return;
#endif
		} else {
#ifdef WAKEUP_REBOOT
			//system("reboot -f");
			db_error("============reboot============");
			reboot(RB_AUTOBOOT);
			return;
#endif
		}
	} else if(is_enable_acc == 0) {
#ifdef WAKEUP_REBOOT
			//system("reboot -f");
			db_error("============reboot============");
			reboot(RB_AUTOBOOT);
			return;
#endif
	}


	if((wakeup_event_id & CPUS_WAKEUP_AC) || (wakeup_event_id & CPUS_WAKEUP_USB)){ //DC OFF
		db_error("ResetAccStatus *** CPUS_WAKEUP_AC");
		if(low_battery_flag) {
			db_error("low_battery_flag is true and ac in,set low_battery_flag false");
			low_battery_flag = false;
		}
	} else if((wakeup_event_id & CPUS_WAKEUP_GPIO)) {
		db_msg("ResetAccStatus *** CPUS_WAKEUP_GPIO");
 	}
	
	mStandbyStatus = false;
}

#ifdef MEM_DEBUG
#define CHECK_PATH "/proc/meminfo"
#define CHECK_NODE "MemFree"

int EventManager::debug_get_info()
{
       char line[128];
       FILE *fp = fopen(CHECK_PATH, "r");
       char num[32]="0";
       if(fp==NULL){
        //printf("open file failed.");
        return 0;
       }
       while(fgets(line, sizeof(line), fp)) {	// read one line
               if (strncmp(line, CHECK_NODE, strlen(CHECK_NODE)) == 0) {
                        //puts(line);
                        //char *str = strstr(line, ":");
                        //str += 1;
                        //sscanf(str, "%[^a-z]", num);
            			db_error("%s",line);            
                        break;
               }
       }
       fclose(fp);
       return atoi(num);

}
#endif

void *EventManager::CheckAccEvent(void *context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	
	prctl(PR_SET_NAME, "CheckAccEvent", 0, 0, 0);
	int acc_state = 0;
	int wake_state;
	int dc_count = 0;
	int acc_count = 0;
	int rst_count = 0;
	int dc_state = 0;
	int remote_try_count = 0;
	int menu_config_value = 0;
	int acc_check_flag = 0;
	int gps_state = 0;
//    int is_enable_acc = 1;  //check FILE_ENABLE_ACC_SWITCH to change this status
    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
#ifdef MEM_DEBUG
	int count = 0;
#endif
	while(1)
	{
		if(em->mStandbyStatus){
			usleep(1000*1000);
			continue;
		}
		// 借用
		#ifdef MEM_DEBUG
		em->debug_count++;
		if ((em->debug_count & 3) == 0) {
			em->debug_get_info();
		}
		#endif
		if (em->CheckGpsOnline()) {	// online
			if (!gps_state) {
				db_error("MSG_GPSON_HAPPEN");
				em->Notify(MSG_GPSON_HAPPEN);
				//::SendMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
				gps_state = 1;
			}
		} else {
			if (gps_state) {
				db_error("MSG_GPSOFF_HAPPEN");
				em->Notify(MSG_GPSOFF_HAPPEN);
				gps_state = 0;
			}
		}
#ifdef ENABLE_ONLY_SUPPORT_DC

       em->is_enable_acc = 0;
#endif

        if(em->mStandbybreak_flag == true) {
        	if(em->mNotify_acc_off_flag == true)
        	{
        		db_error("standby try time out,reset mNotify_acc_off_flag");
        		em->mNotify_acc_off_flag = false;
        	}
        	em->standby_try_count++;
        	if(em->standby_try_count >= 10) {
        		db_error("time out,check acc event again,reset standby_try_count");
        		em->mStandbybreak_flag = false;
        		em->standby_try_count  = 0;
        	} else {
        		usleep(1000*1000);
        		db_msg("standby_try_count++");
        		continue;
        	}
        }
        menu_config_value = menuconfiglua->GetMenuIndexConfig(SETTING_ACC_SWITCH); //更新配置选项

		em->CheckEvent(EventManager::EVENT_ACC, acc_state); //从acc status获取状态
		dc_state = em->GetDCconnetStatus(); //从dc status获取状态

		if (acc_check_flag) {
			db_error("AccEvent DC in");
			em->Notify(MSG_ACCON_HAPPEN);
			acc_check_flag = 0;
			em->mNotify_acc_on_flag = true;
		}
		
		if(em->is_enable_acc == 1) {
			if(menu_config_value == 1){
				if(dc_state == 1) {
//					db_msg("CheckAccEvent DC in");
					if(em->mNotify_acc_on_flag == false){
						db_error("CheckAccEvent DC in");
						//em->Notify(MSG_ACCON_STARTCHECK);
						acc_check_flag = 1;
						db_msg("notify MSG_ACCON_STARTCHECK");
					}
					remote_try_count = 0;
				} else if(dc_state == 0) {
					remote_try_count++;
					if(em->RemoteActionDone == 2){
//						db_msg("CheckAccEvent DC out");
						if(em->mNotify_acc_off_flag == false) {
							db_msg("CheckAccEvent DC out");
							em->Notify(MSG_ACCOFF_HAPPEN);
							db_msg("notify MSG_ACCOFF_HAPPEN");
							em->mNotify_acc_off_flag = true;
//				       	    em->RemoteActionDone = 0;//test
							remote_try_count = 0;
						}
					} else if(remote_try_count >= 20){
//		     			db_msg("Remote Action Done  set value time out,notify MSG_ACCOFF_HAPPEN!!!!");
						if(em->mNotify_acc_off_flag == false){
							db_msg("CheckAccEvent DC out");
							em->Notify(MSG_ACCOFF_HAPPEN);
							db_msg("notify MSG_ACCOFF_HAPPEN");
							em->mNotify_acc_off_flag = true;
//		         			em->RemoteActionDone = 0;//test
							remote_try_count = 0;
						}
					}
				}
			} else if(menu_config_value == 0){
				if(acc_state == 1) {
//					db_msg("CheckAccEvent ACC in");
					if(em->mNotify_acc_on_flag == false){
						db_error("CheckAccEvent ACC in");
						//em->Notify(MSG_ACCON_STARTCHECK);
						acc_check_flag = 1;
						db_msg("notify MSG_ACCON_STARTCHECK");
						//em->mNotify_acc_on_flag = true;
					}
					remote_try_count = 0;
				} else if(acc_state == 0) {
					remote_try_count++;
					if(em->RemoteActionDone == 2){
//						db_msg("CheckAccEvent ACC out");
						if(em->mNotify_acc_off_flag == false) {
							db_msg("CheckAccEvent ACC out");
							em->Notify(MSG_ACCOFF_HAPPEN);
							db_msg("notify MSG_ACCOFF_HAPPEN");
							em->mNotify_acc_off_flag = true;
//			    	        em->RemoteActionDone = 0;//test
							remote_try_count = 0;
						}
					} else if(remote_try_count >= 20){
//			     		db_msg("Remote Action Done  set value time out,notify MSG_ACCOFF_HAPPEN!!!!");
						if(em->mNotify_acc_off_flag == false){
							db_msg("CheckAccEvent ACC out");
							em->Notify(MSG_ACCOFF_HAPPEN);
							db_msg("notify MSG_ACCOFF_HAPPEN");
							em->mNotify_acc_off_flag = true;
//		         			em->RemoteActionDone = 0;//test
							remote_try_count = 0;
						}
					}
				}
			}
		}
#ifdef ENABLE_ONLY_SUPPORT_DC
		else if(em->is_enable_acc == 0) 
        {
			if(dc_state == 1) {
			//	db_msg("CheckAccEvent DC in");
				if(em->mNotify_acc_on_flag == false)
                {
					db_error("CheckAccEvent DC in");
					//em->Notify(MSG_ACCON_STARTCHECK);
					acc_check_flag = 1;
					db_msg("notify MSG_ACCON_STARTCHECK");
					//em->mNotify_acc_on_flag = true;
				}
			} else if(dc_state == 0) 
			{
			//	db_warn("CheckAccEvent DC out");
			    if(em->mNotify_acc_off_flag == false)
                {
						db_msg("CheckAccEvent DC out");
						em->Notify(MSG_ACCOFF_HAPPEN);
						db_msg("notify MSG_ACCOFF_HAPPEN");
						em->mNotify_acc_off_flag = true;
						remote_try_count = 0;
				}
			} 
		}
		sleep(1);
	}
#endif
#if 0
		em->CheckEvent(EventManager::EVENT_4GWAKE, wake_state);
		if(em->m_4GWakeUp_state != wake_state){
			rst_count++;
			if(rst_count == 3){
				if(wake_state){
					em->WriteFileInt(AP_WAKEUP_M,1);
				}
				rst_count = 0;
				em->m_4GWakeUp_state = wake_state;
			}
		}else{
			rst_count = 0;
		}
		
		usleep(1000*1000);
	}
    #endif
	em->m_AccEvent_thread_id = 0;
	return NULL;
}

static double cal_clongitude_latitude_value(const char *data)
{	// ddmm.mmmm or dddmm.mmmm ->dd.dddddd ddd.dddddd
    int value_high;
    double value ,value_low;
	
    value_high = atoi(data)/100;
    value_low =   (atof(data) - value_high*100) / 60;
	value = value_high + value_low;
	
    return value;
}

void EventManager::processgngga( char *buf)
{
	char *ptr,*ptr2;
	char temp[512] = {0};
    char tmpstr[16];
	int ret;
	int nQ;
    float fX,fY,fH;
    char cX,cY;
    double tmp_value = 0.0;
	char mGGAData[512] = {0};

	float car_speed_tmp;
#ifdef SIMCOM_4G_MODULE
	if((ptr=strstr(buf, "$GPGGA"))!= NULL) {
#else
        ptr = buf;
        if(1/*(ptr=strstr(buf, "$GNGGA"))!= NULL*/)  {
#endif
				//if((ptr2 = strstr(ptr, "\r\n")) != NULL)
				{				
					//strncpy(mGGAData,ptr,(ptr2-ptr));
					strncpy(mGGAData,buf,strlen(buf));
					GNGGA_string = mGGAData;
					//                          1          2         3 4          5 6 7  8    9
					//strncpy(mGGAData, "$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,19.7,M,,,,0000*1F", sizeof(mGGAData));	
					//                          UTC        ddmm.mmmm   dddmm.mmmm 
					#ifdef DEBUG_GPS
					db_msg("mGGAData ==== %s",mGGAData);
					#endif
					if((ptr = getStrBehindComma(mGGAData, 6)) != NULL){//check GPS STATUS
						if(ptr[0] != '0'){
							mGpsStatus = 1;
							ptr = getStrBehindComma(mGGAData, 1);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, 6);						
									sscanf(temp, "%2d%2d%2d.%3d", &mLocationInfo.utc.hour, &mLocationInfo.utc.min, &mLocationInfo.utc.sec, &mLocationInfo.utc.msec);
								}		
							}
							
							ptr = getStrBehindComma(mGGAData, 2);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									tmp_value = cal_clongitude_latitude_value(temp);	// ddmm.mmmm -> dd.dddddd
								}
							}

							ptr = getStrBehindComma(mGGAData, 3);
							if(ptr != NULL){
								if(ptr[0] != ','){
									mLocationInfo.NS = ptr[0];
                                    tmp_value = mLocationInfo.NS == 'N' ? tmp_value : 0 - tmp_value;
                                    //snprintf(tmpstr, sizeof(tmpstr), "%.6f", tmp_value);
                                    //mLocationInfo.latitude = atof(tmpstr);		// double
                                    mLocationInfo.latitude = tmp_value;
								}		
							}		

							ptr = getStrBehindComma(mGGAData, 4);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									tmp_value = cal_clongitude_latitude_value(temp);
								}		
							}

							ptr = getStrBehindComma(mGGAData, 5);
							if(ptr != NULL){
								if(ptr[0] != ','){
                                    mLocationInfo.EW = ptr[0];
                                    tmp_value = mLocationInfo.EW == 'E' ? tmp_value : 0 - tmp_value;
                                    //snprintf(tmpstr, sizeof(tmpstr), "%.6f", tmp_value); 
                                    //mLocationInfo.longitude = atof(tmpstr);
                                    mLocationInfo.longitude = tmp_value;
								}		
							}	

							ptr = getStrBehindComma(mGGAData, 7);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									sscanf(temp, "%d", &mLocationInfo.GpsSignal);							
								}		
							}	
							gps_signal_info = mLocationInfo.GpsSignal;
#ifdef DEBUG_GPS
							db_error("==========em->mLocationInfo.GpsSignal %d==========",mLocationInfo.GpsSignal);
#endif
							if(mLocationInfo.GpsSignal <= 4)
							{
								mGpsSignal = 1;
							}else if((mLocationInfo.GpsSignal > 4) && (mLocationInfo.GpsSignal <= 6)){
								mGpsSignal = 2;
							}else if(mLocationInfo.GpsSignal > 6){
								mGpsSignal = 3;
							}
							
							ptr = getStrBehindComma(mGGAData, 9);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									mLocationInfo.altitude = get_double_number(temp);
									//sscanf(temp, "%f", &em->mLocationInfo.altitude);							
								}		
							}
                            #ifdef DEBUG_GPS
							db_msg("GPS utc is %d:%d:%d,%d-%d-%d\n", \
									mLocationInfo.utc.hour, mLocationInfo.utc.min, mLocationInfo.utc.sec \
									,mLocationInfo.utc.year, mLocationInfo.utc.mon, mLocationInfo.utc.day);
							db_msg("GPS location is %c %f,%c %f, altitude %f\n", \
									mLocationInfo.NS, mLocationInfo.latitude, mLocationInfo.EW, mLocationInfo.longitude, mLocationInfo.altitude);
                            #endif
							if (mGpsSignal) {
								if (rmcflag && synctimeflag) {
									int timezone = MenuConfigLua::GetInstance()->GetTimezone();
									UTCsetSystemTime(mLocationInfo.gtm.tm_year, mLocationInfo.gtm.tm_mon, mLocationInfo.gtm.tm_mday,\
										mLocationInfo.utc.hour, mLocationInfo.utc.min, mLocationInfo.utc.sec,timezone);
									db_error("set local time by GPS: timezone %d",timezone);
									synctimeflag = 0;
								}
							}
						}else{
							#ifdef DEBUG_GPS
							db_msg("no $GNGGA in GPS data");
							#endif
							mGpsSignal = 0;
							mGpsStatus = 0;
							synctimeflag = 1;
						}
					}
				}
			}
}
/*
TZ 环境变量 GMT+0	表示UTC
GMT-8	表示 东8区(北京)	这里表示方法不是通常见到的 用GMT+8表示北京时区
简单理解为: 负表示比GMT早8个小时, 正表示比GMT晚8个小时
*/
void EventManager::set_tz(int tz)
{	
	char tzstr[256] = {0};		
	int tzhour = -tz;
		
	snprintf(tzstr,sizeof(tzstr),"GMT%+02d",tzhour);
	if(setenv("TZ", tzstr, 1)!=0)
	{
		db_error("setenv failed \n");
	}
	tzset();
	//db_error("set timezone: %d write TZ: %s",tz, tzstr);
}

// 设置新的时区(菜单里用)
void EventManager::set_tz_ex(int tzx)
{	
	char tzstr[256] = {0};	
	char buf[32] = {0};
	struct timeval tv;
	struct timezone tz;
	int tmzone = -tzx;		// +8 -> -8
	int ozx = 0;
	struct tm *p;
	//db_error("---------------before set new timezone-----------------");
	char *penv = getenv("TZ");	// "GMT-8" 实际表示 GMT+8
	//db_error("old penv: %s",penv);
	#if 1
	if (penv) strcpy(tzstr,penv);
	char *pp = tzstr;
	if (strlen(pp)>4) {
		
		while (*pp) {
			if ((*pp == '+') || (*pp == '-')) {		// "GMT-8"
				//db_error("pp: %s",pp);
				ozx = -atoi(pp);					// 得到 -(-8) +8
				break;
			}
			pp++;
		}
	}
	#endif
	//db_error("old zone: %d",ozx);
	#ifdef RTCASUTC
	ozx = MenuConfigLua::GetInstance()->GetOldTimezone();
	#endif
	if (ozx == tzx) return;
	#ifndef RTCASUTC
	set_tz(tzx);
	#else
	time_t now;
	time(&now);			// now 秒数
	now -= (ozx * 3600);
	now += (tzx * 3600);
	p = gmtime(&now);		// 转成系统UTC时间,
	db_error("------- ozx: %d tzx: %d",ozx,tzx);
	struct tm tm_time;
	tm_time.tm_sec = p->tm_sec;
	tm_time.tm_min = p->tm_min;
	tm_time.tm_hour = p->tm_hour;
	tm_time.tm_mday = p->tm_mday;
	tm_time.tm_mon = p->tm_mon;
	tm_time.tm_year = p->tm_year;
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;
	
	tv.tv_sec = mktime(&tm_time);
	tv.tv_usec = 0;
	tz.tz_minuteswest = 0;	// 和Greenwich 时间差了多少分钟
	tz.tz_dsttime = 0;					// 日光节约时间的状态
	if (settimeofday(&tv, NULL)<0) {
		db_error("settimeofday error!");
	}
		
	
	system("hwclock -w");
	MenuConfigLua::GetInstance()->UpdateSystemTime(true);
	#endif
}


void EventManager::UTCsetSystemTime(int year, int month, int day, int hour, int minute, int second, int tmzone)
{
	struct timeval tv;
	struct timezone tz;
	struct tm tm_time;
	struct tm *p;
	//tmzone = 8;
	time_t now;
	time(&now);
	p = gmtime(&now);
	
//	system UTC:  119/5/13 23:23:23			2019/
//     GPS UTC: 2019/6/14  7:23:24 8		15:23:24
	db_error("GPS UTC:    %d/%d/%d %d:%d:%d %d",year,month,day, hour, minute, second, tmzone);	// 2019/6/13 10:56:7 0
	tm_time.tm_sec = second;
	tm_time.tm_min = minute;
	tm_time.tm_hour = hour;
	tm_time.tm_mday = day;
	tm_time.tm_mon = month - 1;
	tm_time.tm_year = year - 1900;		// UTC时间
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;
	db_error("system  UTC: %d/%d/%d %d:%d:%d",p->tm_year,p->tm_mon,p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);	// 70/0/1 0:0:0 表示1970/1/01 00:00:00
	db_error("tm_time UTC: %d/%d/%d %d:%d:%d",tm_time.tm_year,tm_time.tm_mon,tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);	// 70/0/1 0:0:0 表示1970/1/01 00:00:00
	#if 0
	if ((p->tm_year != tm_time.tm_year) || (p->tm_mon != tm_time.tm_mon) || (p->tm_mday != tm_time.tm_mday) \
		|| (p->tm_hour != tm_time.tm_hour) || (p->tm_min != tm_time.tm_min) ) 
	#endif
	{
		#ifndef RTCASUTC
		set_tz(tmzone);
		#endif
		db_error("-------------------------settimeofday !");
		// 这里我们要把GPS的UTC设为系统UTC时间
		tv.tv_sec = mktime(&tm_time) + tmzone * 3600;	 // 因为mktime()已经根据当前时区进行计算, 如果是GMT+8则减去了288000, 加回来得到GMT+0的时间
		tv.tv_usec = 0;
		tz.tz_minuteswest = 0;	// 和Greenwich 时间差了多少分钟
		tz.tz_dsttime = 0;					// 日光节约时间的状态
			
		if (settimeofday(&tv, NULL)<0) {
			db_error("settimeofday error!");
		}
		
		
		system("hwclock -w");
		MenuConfigLua::GetInstance()->UpdateSystemTime(true);
	}
#if 1
	//struct timeval tv;
    //struct timezone tz;
    //gettimeofday(&tv, &tz);
    //db_error("tv_sec = %d, tv_usec = %d, tz_minuteswest = %d, tz_dsttime = %d\n", 
    //        tv.tv_sec, tv.tv_usec, tz.tz_minuteswest, tz.tz_dsttime) ;

	struct tm * tm=NULL;
    time_t timer;
    timer = time(NULL);
    tm = localtime(&timer);
	char buf[32] = {0};
	snprintf(buf, sizeof(buf),"%04d-%02d-%02d  %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	db_error("localtime: %s",buf);
	char *penv = getenv("TZ");
	db_error("TZ env: %s",penv);	// GMT-8
	time(&now);
	p = gmtime(&now);
	
	db_error("new system UTC: %d/%d/%d %d:%d:%d",p->tm_year,p->tm_mon,p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);	// 70/0/1 0:0:0 表示1970/1/01 00:00:00
#endif
}
void EventManager::processgnrmc( char *buf)
{
	char *ptr,*ptr2;
	char temp[512] = {0};
    char tmpstr[16];
	int ret;
	int nQ;
    float fX,fY,fH;
    char cX,cY;
    float tmp_value;
	char mRMCData[512] = {0};

	float car_speed_tmp;
#ifdef SIMCOM_4G_MODULE
	if((ptr=strstr(buf, "$GPRMC"))!= NULL)
#else
	ptr = buf;
        if(1/*(ptr=strstr(buf, "$GNRMC"))!= NULL*/)
#endif
			{
				//if((ptr2 = strstr(ptr, "\r\n")) != NULL)   
				{			
					//strncpy(mRMCData,ptr,(ptr2-ptr));
					strncpy(mRMCData,buf,strlen(buf));
					GPGSV_string = mRMCData;
					//strncpy(mRMCData,"$GPRMC,100539.000,A,2232.8557,N,11355.7438,E,0.13,309.62,031209,,*10", sizeof(mRMCData));
					//snprintf(mRMCData,sizeof(mRMCData),"$GPRMC,100539.000,A,%.4f,N,%.4f,E,0.13,309.62,031209,,*10",alt,longi);
					#ifdef DEBUG_GPS
					db_msg("mRMCData ==== %s",mRMCData);
					#endif
					if((ptr = getStrBehindComma(mRMCData, 2)) != NULL){
						//db_msg("GPRMC data valiable status:%c",ptr[0]);
						if(ptr[0] == 'A')
						{
                            //get time hour minute second
                            mLocationInfo.status = ptr[0];
                            char *tpt = getStrBehindComma(mRMCData, 1);
							if(*tpt != ','){
							    bzero(temp,sizeof(temp));
								if( strchr(tpt, ',') != NULL) {
                                    strncpy(temp, tpt, (strchr(tpt, ',') - tpt - 3));
                                    char hms[8] = {0};
                                    memcpy(hms, temp, 2);
                                    mLocationInfo.gtm.tm_hour = atoi(hms);
                                    memcpy(hms, temp + 2, 2);
                                    mLocationInfo.gtm.tm_min = atoi(hms);
                                    memcpy(hms, temp + 4, 2);
                                    mLocationInfo.gtm.tm_sec = atoi(hms);
                                    #ifdef DEBUG_GPS
                                    db_msg("hour:%d min:%d sec:%d\n", 
                                            mLocationInfo.gtm.tm_hour, mLocationInfo.gtm.tm_min, mLocationInfo.gtm.tm_sec);
                                    #endif
                                }
                            }

                            //get time year month day
                            tpt = getStrBehindComma(mRMCData, 9);
							if(*tpt != ','){
							    bzero(temp,sizeof(temp));
								if( strchr(tpt, ',') != NULL) {
                                    strncpy(temp, tpt, (strchr(tpt, ',') - tpt));
                                    char ymd[8] = {0};
                                    memcpy(ymd, temp, 2);
                                    mLocationInfo.gtm.tm_mday = atoi(ymd);
                                    memcpy(ymd, temp + 2, 2);
                                    mLocationInfo.gtm.tm_mon = atoi(ymd);
                                    memcpy(ymd, temp + 4, 2);
                                    mLocationInfo.gtm.tm_year =  2000 + atoi(ymd);
                                    #ifdef DEBUG_GPS
                                    db_msg("year:%d mon:%d mday:%d\n", 
                                            mLocationInfo.gtm.tm_year, mLocationInfo.gtm.tm_mon, mLocationInfo.gtm.tm_mday);
                                    #endif
                                    rmcflag = 1;
                                }
                            }
							ptr = getStrBehindComma(mRMCData, 7);
							if( ptr != NULL)
							{
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									if( strchr(ptr, ',') != NULL)
									{
										strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									}
									#if 0
									car_speed_tmp = atof(temp) * 1.852;//convert knots to km
									bzero(temp,sizeof(temp));
									snprintf(temp, sizeof(temp),"%.3f", car_speed_tmp);
									mLocationInfo.speed = car_speed_tmp;
									//sscanf(temp, "%f", &em->mLocationInfo.speed);
									//em->mLocationInfo.speed = em->mLocationInfo.speed * 1.852;
									#else
									car_speed_tmp = atof(temp);
									mLocationInfo.speed = car_speed_tmp;	// knot
									#endif
								}
							}
						} else{ 
							rmcflag = 0;
							synctimeflag = 1;
							mLocationInfo.status = 'V';
						}
					} 
				}
			}
}
void EventManager::processgpgsv( EventManager * em ,char *buf)
{
	char *ptr,*ptr2,*temp_ptr;
	char temp[512] = {0};
    char tmpstr[16];
	int ret;
	char mGSVData[128];
	int total_sa = 0;
	int total_gpgsv = 0;
	int current_gpgsv = 0;
	int loop_iii = 0;
	char gpgsv_log[256] = {0};
	char tmpsa_number[3];
	char tmpsa_nr[3];
	char *maohao = ":";
	char *douhao = ",";
	temp_ptr = buf;

	
	   memset(mGSVData, 0, sizeof(mGSVData));
	   bzero(gpgsv_log,sizeof(gpgsv_log));
	   bzero(tmpsa_number,sizeof(tmpsa_number));
	   bzero(tmpsa_nr,sizeof(tmpsa_nr));
	total_sa = 0;
	total_gpgsv = 0;
	current_gpgsv = 0;


while(1){
				//db_error("gpgsv_log 2  %s\n",gpgsv_log);
		if((ptr=strstr(temp_ptr, "$GPGSV"))!= NULL)
			{
				if((ptr2 = strstr(ptr, "\r\n")) != NULL)   
				{			
				    temp_ptr = ptr2;
					strncpy(mGSVData,ptr,(ptr2-ptr));
					//db_error("%s\n",mGSVData);
					if((ptr = getStrBehindComma(mGSVData, 1)) != NULL){
						//db_msg("GPRMC data valiable status:%c",ptr[0]);
					if(ptr != NULL){
						if(ptr[0] != ','){
							bzero(temp,sizeof(temp));
							strncpy(temp, ptr, (strchr(ptr,',')-ptr));
							sscanf(temp, "%d", &total_gpgsv);							
						}		
					}

					}

					if((ptr = getStrBehindComma(mGSVData, 2)) != NULL){
						//db_msg("GPRMC data valiable status:%c",ptr[0]);
					if(ptr != NULL){
						if(ptr[0] != ','){
							bzero(temp,sizeof(temp));
							strncpy(temp, ptr, (strchr(ptr,',')-ptr));
							sscanf(temp, "%d", &current_gpgsv);							
						}		
					}

					}
										
					if((ptr = getStrBehindComma(mGSVData, 3)) != NULL){
						//db_msg("GPRMC data valiable status:%c",ptr[0]);
					if(ptr != NULL){
						if(ptr[0] != ','){
							bzero(temp,sizeof(temp));
							strncpy(temp, ptr, (strchr(ptr,',')-ptr));
							sscanf(temp, "%d", &total_sa);							
						}		
					}

					}
					for(loop_iii=0;loop_iii < (current_gpgsv < total_gpgsv ? 4 : (total_sa % 4)==0 ? 4 :(total_sa % 4)) ;loop_iii++)
						{
						
							//db_msg("GPRMC data valiable status:%c",ptr[0]);
							if((ptr = getStrBehindComma(mGSVData, 4+loop_iii*4)) != NULL)
								{
								if(ptr != NULL){
									if(ptr[0] != ','){
										bzero(tmpsa_number,sizeof(tmpsa_number));
										strncpy(tmpsa_number, ptr, (strchr(ptr,',')-ptr));

										
								if((ptr = getStrBehindComma(mGSVData, 7+loop_iii*4)) != NULL)
									{
										if(ptr != NULL){
											if(ptr[0] != ','){
												bzero(tmpsa_nr,sizeof(tmpsa_nr));
												strncpy(tmpsa_nr, ptr, (strchr(ptr,',')-ptr));
												strcat(gpgsv_log,tmpsa_number);
												strcat(gpgsv_log,maohao);
												strcat(gpgsv_log,tmpsa_nr);
												strcat(gpgsv_log,douhao);
											}		
										}
									}
											}		
										}

								

						}



					}
				 if(current_gpgsv >= total_gpgsv)
						break;
					}
				else
					break;
				//db_error("gpgsv_log 1 %s\n",gpgsv_log);
			}
		else
			break;
				}
em->mGpgsvlog = gpgsv_log;

}
void EventManager::processgpsbuf( char *buf)
{
#ifdef SIMCOM_4G_MODULE
	if(strstr(buf, "$GPGGA")!= NULL)
	{
		processgngga(buf);
	}
	else if(strstr(buf, "$GPRMC")!= NULL)
	{		
		processgnrmc(buf);
	}
#else
	//if(strstr(buf, "$GNGGA")!= NULL)
        char *ptr;
        ptr = strstr(buf, "$");
        if ((ptr == NULL) || strlen(buf) < 8) return; 
        if (((ptr[1] == 'B') && (ptr[2] == 'D')) ||			/*BD 北斗*/
        	 ((ptr[1] == 'G') && (ptr[2] == 'N')) ||		/*GN 格洛纳斯*/
        	 ((ptr[1] == 'G') && (ptr[2] == 'P')) )			/*GP 美国GPS*/
	{
                if (strncmp(ptr+3, "GGA", 3) == 0 ) {
                #ifdef DEBUG_GPS
                db_msg("[debug_690]: the gps buf 00= %s\n",buf);
                #endif
				
		        processgngga(buf);
				
                } else if (strncmp(ptr+3, "RMC", 3) == 0 ) {
                #ifdef DEBUG_GPS
                db_msg("[debug_690]: the gps buf 11= %s\n",buf);
                #endif
				
			    processgnrmc(buf);
		} else if (strncmp(ptr+3, "GSV", 3) == 0 ) {
                }
	}
	
#endif
	
}

#ifdef DEBUG_GPSSAVELOG

void EventManager::dosavegpslog(char *temp)
{
	if (gpslogfd<0) return;
	char gpsbuff[512];
	time_t now;
	time(&now);
	struct tm *p;
	struct tm tm_time;
	tm_time.tm_sec = p->tm_sec;
	tm_time.tm_min = p->tm_min;
	tm_time.tm_hour = p->tm_hour;
	tm_time.tm_mday = p->tm_mday;
	tm_time.tm_mon = p->tm_mon;
	tm_time.tm_year = p->tm_year;
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;
	
	p = localtime(&now);	// 经过时区转换
	sprintf(gpsbuff,"%04d-%02d-%02d  %02d:%02d:%02d   %s", p->tm_year+1900, p->tm_mon+1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, temp);
	write(gpslogfd, gpsbuff, strlen(gpsbuff));
			
}			
#endif


#define GPSBUF_SIZE 1024

#define RINGBUFF_SUPPORT

#ifdef RINGBUFF_SUPPORT
#define DOCHECKSUM
// 取出缓冲区中的一条GPS记录
/*
+ wr = rd 说明 没有新的数据需要处理
+ wr > rd 说明 写指针在前
+ wr < rd 说明 写指针在后(已经回环)
+ */
int EventManager::GetGPSrecord(char* buff, char* pwr, char* &prd, char *bufret)
{
#if 1
	*bufret = 0;
	if (pwr == prd) {
		return 0;
	}
	char *pwx = pwr;
	char *prx = prd;
	char *tmpx= bufret;
	char ch;
	int bitflag = 0;
	#ifdef DOCHECKSUM
	char chusum = 0;
	char chusumtmp = 0;
	char checksum = 0;
	char chkbuf[8];
	int chk = 0;
	#endif
	//char gpsdata[256] = {0};
	//int gk =0;
	while (1) {
		ch = *prx;
		prx++;
		if (prx >= buff + GPSBUF_SIZE) prx = buff;	// 读指针超出环形缓冲区末尾,返回头部
		switch (ch) {
			case '$':
				bitflag = 1;
				*tmpx++ = ch;
				//gk =0;
				//gpsdata[gk++] = ch;
				break;
			case '*':
				if (bitflag & 1) {
					*tmpx++ = ch;
					bitflag |= 8;
					#ifdef DOCHECKSUM
					chusum = chusumtmp;
					#endif
				}
				//gpsdata[gk++] = ch;
				break;
			case '\r': {		// 0x0d
					if ((bitflag & 1) == 1) {
						bitflag |= 2;
						*tmpx++ = ch;
					}
					if (bitflag & 8) {	//
						chkbuf[chk] = '\0';
					}
				}
				//gpsdata[gk++] = ch;
				break;
			case '\n': {		// 0x0a
					if ((bitflag & 3) == 3) {
						bitflag |= 4;
						*tmpx++ = ch;
					}
					if (bitflag & 8) {	//
						chkbuf[chk] = '\0';
					}
					//gpsdata[gk++] = ch;
				}
				break;
			default:
				{
					if (bitflag & 1) {
						*tmpx++ = ch;
						#ifdef DOCHECKSUM
						chusumtmp ^= ch;
						#endif
					}
					if (bitflag & 8) {	//
						chkbuf[chk++] = ch;
					}
					//gpsdata[gk++] = ch;
				}
				break;
		}
		if (prx == pwr) {
			break;
		}
		if (bitflag & 4) {
			break; 
		}
	}
	if ((bitflag & 0xf) == 0xf) {
		#ifdef DOCHECKSUM
		checksum = (unsigned char)strtol(chkbuf,NULL,16);
		//db_error("%s",gpsdata);
		//db_error("checksum: (calc)%02x (org)%02x",chusum, checksum);
		if (checksum == chusum) {
			prd = prx;
			*tmpx++ = 0;
			return 1;
		} 
		else 
		#endif
		{
			prd = prx;
			*tmpx++ = 0;
			return -1;
		}
		
	} else {
		return 0;
	}
#else
	char *pwx = pwr;
	char *prx = prd;
	char gpsdata[1024] = {0};
	char ch;
	int k = 0;
	while (1) {
		ch = *prx;
		gpsdata[k++] = ch;
		prx++;
		if (prx >= buff + GPSBUF_SIZE) prx = buff;
		if (prx == pwr) {
			gpsdata[k++] = '\0';
			break;
		}
	}
	db_error("%s",gpsdata);
	prd = prx;
	
	return 0;
#endif
}
#endif

//#define GPS_SIMULATION
#ifdef GPS_SIMULATION
char* gps_sim_data[16] = {
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPRMC,075301.305,A,2237.3387,N,11404.3147,E,0.13,309.62,031209,,*10\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPRMC,075301.305,A,2237.3387,N,11404.3147,E,0.13,309.62,031209,,*10\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPRMC,075301.305,A,2237.3387,N,11404.3147,E,0.13,309.62,031209,,*10\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n",
	"$GPGGA,075301.305,2237.3387,N,11404.3147,E,1,7,1.04,126.7,M,-2.1,M,,*4E\r\n"
};
#endif

void *EventManager::startLocationReport(void * context)
{
	EventManager *em = reinterpret_cast<EventManager*>(context);
	prctl(PR_SET_NAME, "startLocationReport", 0, 0, 0);
#if 0
	unsigned int ret;
	em->LocationReportThreadInit();
	unsigned int ret_init;
	ret_init = Edog_Init();
	while(1){
		usleep(200*1000);
		Edog_Run();
		//ret = GPSRadar_GetBaseDatabase_Ver();
	}
#else
	char buf[GPSBUF_SIZE] , *ptr,*ptr2,*temp_ptr;
    char tmpstr[16];
    static int time_count = 0;
	int ret;
	int nQ;
    float fX,fY,fH;
    char cX,cY;
    float tmp_value;
	char mGGAData[512];
	char mRMCData[512];
	char temp[512] = {0};
	int iTmpcnt = 0;
	char *strindexstart = NULL;
	char *strindexend = NULL;
	char tmpbuf[512] = {0};
	#ifdef RINGBUFF_SUPPORT
	char *pw;	/*写指针*/
	char *pr;	/*读指针*/
	char *tmpx;
	char *prx;	
	pw = buf;
	pr = buf;
	int fs_sel,bytescanread;
	int bytesread,bytewrite;
	fd_set fs_read;
	int bitflag = 0;
	struct timeval time;
	#endif
	em->mLocationInfo.GpsSignal= 0;
	em->mLocationInfo.status = 'V';
	em->GetGpsswitchConfig();
	em->rmcflag = 0;

    //wait for open simcom gps
//	while(!em->mOpenSimcomGpsDone)
//		{
//		sleep(1);
//		}
#ifndef GPS_SIMULATION 	
loop:
	if(em->gpsFd <= 0)
	{
		if( em->LocationReportThreadInit() < 0 )
		{
			db_warn("[debug_jason]: open startLocationReport is failed try again");
			sleep(1);
			goto loop;
		}
	}
#else
	int simreadindex = 0;
#endif
#ifdef DEBUG_GPSSAVELOG
	char *sss = "...........loop write\n";
	if (em->gpslogfd<0) {
		if ((em->gpslogfd = open("/mnt/extsd/gpslog.txt",O_RDWR | O_CREAT,0666))<0) {\
			db_error("create gpslog.txt fail!!!");
		}
		lseek(em->gpslogfd,0,SEEK_END);	// seek to eof
		char *stx = "----------------------New Record---------------------\n";
		write(em->gpslogfd,stx,strlen(stx));

		em->gpsfdorg = open("/mnt/extsd/gpsorg.txt",O_RDWR | O_CREAT,0666);
		lseek(em->gpsfdorg,0,SEEK_END);	// seek to eof
	}
	
#endif
	while(1){
        //db_error("hds->gps while mGpsSignal:%d", em->mGpsSignal);
        memset(tmpbuf, 0, sizeof(tmpbuf));
		usleep(100*1000);

		//bzero(buf,512);
		//bzero(mRMCData,100);
		//bzero(mGGAData,100);
#ifndef GPS_SIMULATION		
		#ifdef RINGBUFF_SUPPORT
		if((ret = read(em->gpsFd, tmpbuf, 512)) > 0) 
		#else
		memset(buf, 0, sizeof(buf));	/* 100ms */		
		if((ret = read(em->gpsFd, buf, 1023)) > 1) 
        #endif
#else
		// GPS 模拟
		usleep(400*1000);
		ret = strlen(gps_sim_data[simreadindex]);
		memcpy(tmpbuf, gps_sim_data[simreadindex],ret);
		simreadindex++;
		if (simreadindex >= 16) simreadindex = 0;
		//db_error("--- %d  %s",ret, tmpbuf);
#endif
		{ 
			//printf("[%d] ==== %s\n",ret, tmpbuf);
			// GPS module online
			//db_error("gps on line");
			em->GPSModuleStatus = 1;	// online
#ifdef DEBUG_GPS
            if(print_count-- <= 0) {
                print_count = 50;
			    printf("[%d] ==== %s\n",ret, buf);
                printf("mGpsSignal:%d\n", em->mGpsSignal);
            }
#endif
			
			if(ret > 0)
			{
				// read data from serial fifi buff and save to buf
				#ifdef DEBUG_GPSSAVELOG
				write(em->gpsfdorg,tmpbuf,ret);
				#endif
				#ifdef RINGBUFF_SUPPORT
				//tmpbuf[bytescanread] = 0;
				bytesread = ret;  // 返回读到的字节数
				//bytesread = strlen(tmpbuf);
				tmpx = tmpbuf;
				//printf("[ret %d len: %d] |%s|\n",ret, bytesread, tmpbuf);
				while (bytesread) {
					if (pw + bytesread >= buf + GPSBUF_SIZE) {	// 超出环形缓冲区末尾
						bytewrite = buf + GPSBUF_SIZE - pw;		// 
						memcpy(pw, tmpx, bytewrite);
						tmpx += bytewrite;
						bytesread -= bytewrite;				// 
						pw = buf;							// 指回环形缓冲区开始
						//db_warn("...........loop write\n");
						#ifdef DEBUG_GPSSAVELOG
						write(em->gpsfdorg,sss,strlen(sss));
						#endif
						//db_warn("x buf:%s\n",buf);
					} else {
						bytewrite = bytesread;
						memcpy(pw, tmpx, bytewrite);
						tmpx += bytewrite;
						pw += bytewrite;
						bytesread = 0;
						//db_warn("v buf:%s\n",buf);
					}		
				}
				#endif
			temp_ptr = buf;
			//em->processgpgsv(em,temp_ptr);
                                #if 0
				strindexstart = strstr(buf,"$");
				if(!strindexstart)
				{
					strindexend = strstr(buf, "\r\n");
					if(strindexend)
					{
						if(iTmpcnt)
						{
							memset(tmpbuf, 0,sizeof(tmpbuf));
							strncpy(tmpbuf,temp, sizeof(tmpbuf));
							if(iTmpcnt + (strindexend - buf) < 512)
							{
								strncpy(tmpbuf+iTmpcnt, buf, strindexend - buf);
							}
							
							em->processgpsbuf(tmpbuf);
							
							iTmpcnt = 0;
						}
						
					}
					else
					{
						
						strncpy(temp+iTmpcnt, buf, sizeof(temp)-iTmpcnt);
						iTmpcnt += strlen(buf);
					}
					
					
				}
				else
				{
					while(strindexstart)
	                {
	                	if(iTmpcnt)
	            		{
	            			strncpy(tmpbuf,temp, sizeof(tmpbuf));
							strncpy(tmpbuf+iTmpcnt, buf, strindexstart - buf);
							em->processgpsbuf(tmpbuf);
							
							iTmpcnt = 0;
	            		}
	                    strindexend = strstr(strindexstart, "\r\n");
	                    if(strindexend)
	                    {
	                            strncpy(tmpbuf, strindexstart, strindexend - strindexstart);
								tmpbuf[strindexend - strindexstart]='\0';
								#ifdef DEBUG_GPS
								db_msg("by hero *** --- tmpbuf[%d] ==== %s\n",strlen(tmpbuf), tmpbuf);
								#endif
	                           // printf("buf =%s\n",buf);
	                            em->processgpsbuf(tmpbuf);
	                            strindexstart = strstr(strindexend,"$");

	                    }
	                    else
	                    {
	                    		
	                            strncpy(temp,strindexstart, sizeof(temp));		
	                            iTmpcnt = strlen(strindexstart);
								
	                            break;
	                    }

	                }
				}
                	#endif
				
			}
			//$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,19.7,M,,,,0000*1F
			//$GPRMC,100539.000,A,2232.8557,N,11355.7438,E,0.13,309.62,031209,,*10
			#if 0
			if((ptr=strstr(buf, "$GNGGA"))!= NULL) {
				if((ptr2 = strstr(ptr, "\r\n")) != NULL){				
					strncpy(mGGAData,ptr,(ptr2-ptr));
					em->GNGGA_string = mGGAData;
					//strcpy(mGGAData, "$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,19.7,M,,,,0000*1F");
					if((ptr = getStrBehindComma(mGGAData, 6)) != NULL){//check GPS STATUS
						if(ptr[0] == '1'){
							db_warn("by hero *** --- 2222222 ==== \n");
							em->mGpsStatus = 1;
							ptr = getStrBehindComma(mGGAData, 1);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, 6);
									sscanf(temp, "%2d%2d%2d.%3d", &em->mLocationInfo.utc.hour, &em->mLocationInfo.utc.min, &em->mLocationInfo.utc.sec, &em->mLocationInfo.utc.msec);
								}		
							}
							
							ptr = getStrBehindComma(mGGAData, 2);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									tmp_value = cal_clongitude_latitude_value(temp);
								}
							}

							ptr = getStrBehindComma(mGGAData, 3);
							if(ptr != NULL){
								if(ptr[0] != ','){
									em->mLocationInfo.NS = ptr[0];
                                    tmp_value = em->mLocationInfo.NS == 'N' ? tmp_value : 0 - tmp_value;
                                    snprintf(tmpstr, sizeof(tmpstr), "%.6f", tmp_value);
                                    em->mLocationInfo.latitude = tmpstr;
								}		
							}		

							ptr = getStrBehindComma(mGGAData, 4);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									tmp_value = cal_clongitude_latitude_value(temp);
								}		
							}

							ptr = getStrBehindComma(mGGAData, 5);
							if(ptr != NULL){
								if(ptr[0] != ','){
                                    em->mLocationInfo.EW = ptr[0];
                                    tmp_value = em->mLocationInfo.EW == 'E' ? tmp_value : 0 - tmp_value;
                                    snprintf(tmpstr, sizeof(tmpstr), "%.6f", tmp_value); 
                                    em->mLocationInfo.longitude = tmpstr;
								}		
							}	

							ptr = getStrBehindComma(mGGAData, 7);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									sscanf(temp, "%d", &em->mLocationInfo.GpsSignal);							
								}		
							}	
							em->gps_signal_info = em->mLocationInfo.GpsSignal;
//							em->mGpsSignal = em->mLocationInfo.GpsSignal / 2;
							/*if(em->mGpsSignal == 0)
								em->mGpsSignal = 1;*/
							db_error("==========em->mLocationInfo.GpsSignal %d==========",em->mLocationInfo.GpsSignal);
							if(em->mLocationInfo.GpsSignal <= 4)
							{
								em->mGpsSignal = 1;
							}else if((em->mLocationInfo.GpsSignal > 4) && (em->mLocationInfo.GpsSignal <= 6)){
								em->mGpsSignal = 2;
							}else if(em->mLocationInfo.GpsSignal > 6){
								em->mGpsSignal = 3;
							}

							ptr = getStrBehindComma(mGGAData, 9);
							if(ptr != NULL){
								if(ptr[0] != ','){
									bzero(temp,sizeof(temp));
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
									em->mLocationInfo.altitude = get_double_number(temp);
									//sscanf(temp, "%f", &em->mLocationInfo.altitude);							
								}		
							}
							#ifdef DEBUG_GPS
							db_msg("GPS utc is %d:%d:%d,%d-%d-%d\n", \
									em->mLocationInfo.utc.hour, em->mLocationInfo.utc.min, em->mLocationInfo.utc.sec \
									,em->mLocationInfo.utc.year, em->mLocationInfo.utc.mon, em->mLocationInfo.utc.day);
							db_msg("GPS location is %c %s,%c %s, altitude %f\n", \
									em->mLocationInfo.NS, em->mLocationInfo.latitude.c_str(), em->mLocationInfo.EW, em->mLocationInfo.longitude.c_str(), em->mLocationInfo.altitude);							
							#endif
							db_error("========em->mGpsSignal %d========",em->mGpsSignal);
						}else{
							#ifdef DEBUG_GPS
							db_error("no $GNGGA in GPS data");
							#endif
							em->mGpsSignal = 0;
							em->mGpsStatus = 0;
						}
					}
				}
			}
			
			if((ptr=strstr(buf, "$GNRMC"))!= NULL)
			{
				if((ptr2 = strstr(ptr, "\r\n")) != NULL)   
				{			
					strncpy(mRMCData,ptr,(ptr2-ptr));
					em->GPGSV_string = mRMCData;
					//strcpy(mRMCData,"$GPRMC,100539.000,A,2232.8557,N,11355.7438,E,0.13,309.62,031209,,*10");
					//snprintf(mRMCData,sizeof(mRMCData),"$GPRMC,100539.000,A,%.4f,N,%.4f,E,0.13,309.62,031209,,*10",alt,longi);
					//db_msg("mRMCData ==== %s",mRMCData);
					if((ptr = getStrBehindComma(mRMCData, 2)) != NULL){
						//db_msg("GPRMC data valiable status:%c",ptr[0]);
						if(ptr[0] == 'A')
						{
							ptr = getStrBehindComma(mRMCData, 7);
							if(ptr != NULL)
							{
								if(ptr[0] != ','){
								bzero(temp,sizeof(temp));
								if(strchr(ptr,',') != NULL)
									strncpy(temp, ptr, (strchr(ptr,',')-ptr));
								float car_speed_tmp = atof(temp) ;//* 1.852;//convert knots to km
								bzero(temp,sizeof(temp));
								snprintf(temp, sizeof(temp),"%.3f", car_speed_tmp);
								em->mLocationInfo.speed = car_speed_tmp;//ysc
								//sscanf(temp, "%f", &em->mLocationInfo.speed);
								//em->mLocationInfo.speed = em->mLocationInfo.speed * 1.852;
								}
							}
						}
					}
				}
			}
			#endif
		} else {
			//read(em->gpsFd, tmpbuf, 512) < 0
			//db_error("gps off line");
			em->GPSModuleStatus = 0;	// offline
			em->synctimeflag = 1;
		}

		#ifdef RINGBUFF_SUPPORT
		// 处理环形缓冲区的数据
		//printf("1>>>> %p %p\n",pw, pr);
		//printf("buff:|%s|\n",buf);
		while (pr != pw) {
			ret = em->GetGPSrecord(buf,pw,pr,temp);	// 如果有有效的数据会改变pr的值
		//	db_error("[debug_jaosn]: the gps is recive buf %s\n",temp);
			if (ret == 0) break;	// 没有要处理的数据
			if (ret < 0) {
				db_error(">>>>>>>>>>>>>>>>>>>GPS data Checksum Error!<<<<<<<<<<<<<<<<<<<<<<<<<");
				break;
			}
			//pr = prx;	
			//printf("xx: |%s|\n",temp);
			#ifdef DEBUG_GPSSAVELOG
			em->dosavegpslog(temp);
			#endif
			if (em->gpsswitch) {
				//db_error("do process GPS data");
				em->processgpsbuf(temp);
			} else {
				em->mGpsSignal = 0;
			}
		}
		#endif
#ifdef SEND_GPSDATA
    //   db_error("[debug_jaosn]: the Gps mGpsSignal = %d\n",em->mGpsSignal); 
        if(em->mGpsSignal)
        {
            time_count++;
            if(time_count >= GPS_TIMEOUT)
            {
                em->setGPSData();
                time_count = 0;
            }
			
        }
#endif
	}
#endif
	
	return NULL;
}

int EventManager::setGPSData()
{
	GPS_INFO p_gpsInfo;
	p_gpsInfo.altitude = mLocationInfo.altitude;
	// sscanf(mLocationInfo.speed.c_str(), "%ld", &p_gpsInfo.car_speed);
	// strncpy(p_gpsInfo.latitude,mLocationInfo.latitude.c_str(), sizeof(p_gpsInfo.latitude));
	// strncpy(p_gpsInfo.longitude,mLocationInfo.longitude.c_str(), sizeof(p_gpsInfo.longitude));
//    strncpy(p_gpsInfo.car_speed, mLocationInfo.speed.c_str(), sizeof(p_gpsInfo.car_speed));
//    strncpy(p_gpsInfo.latitude, mLocationInfo.latitude.c_str(), sizeof(p_gpsInfo.latitude));
 //   strncpy(p_gpsInfo.longitude, mLocationInfo.longitude.c_str(), sizeof(p_gpsInfo.longitude));
	p_gpsInfo.net_state = GetNetStatus();
	strncpy(p_gpsInfo.timestamp, std::to_string(time(0)).c_str(), sizeof(p_gpsInfo.timestamp));
    GPSInfoQue.push(p_gpsInfo);
	//dm->WriteGpsBinData(p_gpsInfo);
	//dm->syncData(1);
	return 0;
}

int EventManager::getIMEI(std::string &p_Imei)
{
	p_Imei = imei;
	return 0;
}

int EventManager::getLocationInfo(LocationInfo_t &p_LocationInfo)
{
	p_LocationInfo = mLocationInfo;

	return 0;
}

//int EventManager::getLocationInfoEx(void* data)
//{
	//memcpy(data, &mLocationInfo, sizeof(LocationInfo_t));

//	return 0;
//}

int EventManager::SetRemoteActionDone(int value)
{
	db_error("==========set remote action done,value %d==========",value);
	RemoteActionDone = value;
	return 0;
}

void EventManager::GetGpsswitchConfig()
{
	int val = MenuConfigLua::GetInstance()->GetGpsSwith();
	SetGpsswitch(val);
}
void EventManager::SetGpsswitch(int val)
{ 
	gpsswitch = val; 
	//db_error("SetGpsswitch %d", val);
}



