/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file event_manager.h
 * @brief 浜嬩欢绠＄悊妯″潡澶存枃浠�
 *
 *  璇ユā鍧楁彁渚涘簲鐢ㄧ▼搴忓叧蹇冪殑涓�浜涚郴缁熶簨浠剁殑鏌ヨ鎺ュ彛鍙婇�氱煡鎺ュ彛锛�
 *  杩欎簺浜嬩欢鍖呮嫭浣嗕笉闄愪簬锛屽瓨鍌ㄨ澶囩殑鎸傝浇銆佹帴鍏ョ姸鎬侊紝HDMI鏄剧ず
 *  璁惧鐨勬帴鍏ワ紝USB 涓昏澶囩殑鎺ュ叆锛堢敤浜嶶VC鍙奙SG妯″紡鏀寔锛夌瓑銆�
 *
 * @author id:826
 * @date 2016-4-28
 *
 * @verbatim
    History:
   @endverbatim
 */

#pragma once

#include "common/singleton.h"
#include "common/subject.h"

#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <string>
#include <list>
#include <time.h>
#include "dd_serv/dd_common.h"


//#define DEBUG_GPSSAVELOG

#define DEF_NTP_HEAD_LI        (0  << 30)
#define DEF_NTP_HEAD_VN        (3  << 27)
#define DEF_NTP_HEAD_MODE      (3  << 24)
#define DEF_NTP_HEAD_STARTUM   (0  << 16)
#define DEF_NTP_HEAD_POLL      (4  << 8)
#define DEF_NTP_HEAD_PRECISION (-6 && 0xFF)

/**
 * 璇ュ畯鐢ㄤ簬杞崲寰鍒癗TP鏃堕棿鎴冲皬鏁伴儴鍒嗭紝
 * 鍗冲皢usec*4294.967296(usec*10^6/(2^32/10^12))锛岃浆鎹负闈炴诞鐐规暟杩愮畻
 */
#define NTPFRAC(x) (4294 * (x) + ((1981 * (x))>>11))
/* 涓婅堪瀹忕殑閫嗚繃绋� */
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

#define DEF_NTP_SERVER "pool.ntp.org"     //ntp瀹樻柟鏃堕棿
#define DEF_NTP_PORT   123

/**
 * 鍥犱负鍗忚瀹氫箟鐨凬TP timestamp format鏄粠1900骞�0H寮�濮嬭绠楃殑锛�
 * 浣哢TC鏄粠1970骞�0H寮�濮嬬畻鐨勶紝鎵�浠ヨ繖閲岄渶瑕佷竴涓亸绉婚噺
 *
 * 浠�1900骞�-1970骞寸殑绉掓暟锛�17涓烘鼎骞村鍑烘潵鐨勫ぉ鏁�
 */
const unsigned long OFFSET_1900_TO_1970 = ((365ul * 70ul + 17ul) * 24ul * 60ul * 60ul);

/* 杞崲ntp鏃堕棿鎴冲埌寰 */
#define NTPTIME_TO_USEC(ntptime) (((long long)ntptime.integer) * 1000000 + USEC(ntptime.fraction))

typedef struct ntp_timestamp {
    unsigned int integer;
    unsigned int fraction;
} ntp_timestamp_t;

namespace EyeseeLinux {

/**
 * @addtogroup DeviceModel
 * @{
 */

/**
 * @addtogroup System
 * @{
 */

/**
 * @addtogroup EventManager
 * @brief 浜嬩欢绠＄悊绫�
 * @{
 */


typedef struct
{
	struct
	{
		unsigned int year;
		unsigned int mon;
		unsigned int day;
		unsigned int hour;
		unsigned int min;
		unsigned int sec;	
		unsigned int msec;	
	} utc;
    struct tm gtm;
	char status;
	double latitude;
	char NS;
	double longitude;
	char EW;
	float speed;
	int GpsSignal;
	float altitude;
	
}LocationInfo_t;

class EventManager
    : public ISubjectWrap(EventManager)
    , public Singleton<EventManager>
{
    friend class Singleton<EventManager>;
    public:
        enum EventType {
            EVENT_USB = 0,
            EVENT_SD,
            EVENT_HDMI,
            EVENT_TVOUT,
            EVENT_ETH,
            EVENT_WIFI,
            EVENT_BT,
            EVENT_ACC,
            EVENT_4GWAKE,
        };

        enum TFState {
            TF_NORMAL = 0,
            TF_ERROR,
            TF_DIFF,
        };

        enum FACTURER_4G {
            THINKWILL = 0,
            ZTE,
            OTHER,
        };

        enum USBPort {
            USB_DIAG = 1,
            USB_GPS,
            USB_AT,
        };

        enum IRLedState {
            IRLedOff = 0,
            IRLedOn,
        };

        /**
         * @brief 浜嬩欢寰幆绾跨▼
         *
         *  鍩轰簬鏈湴socket閫氫俊浣滀负鏈嶅姟绔洃鍚郴缁焨event浜嬩欢
         *
         */
        static void *EventLoopThread(void *context);

        static void *NetLinkMonitorThread(void *context);
		int CheckttyUsb();
        int CheckEvent(EventType type, int &value);
		int CheckSDCard_Cid();
		void SetUSB4GModuleStandby(bool flag);
		void SetUSB4GModuleReset();
		static void *CheckImpackEvent(void *context);
		static void *CheckWakeUpEvent(void *context);
		static void *CheckAccEvent(void *context);
		static void *CheckBindFlagEvent(void *context);
		static void *CheckIrCheckFunEvent(void *context);
		static void *CheckNetEvent(void *context);
		static void *startLocationReport(void *context);
		static void * USB4G_Init(void *context);
		static void *NetTimeThread(void *context);
		int GetNetIp(const std::string &netdev_name, unsigned int &ip);
		int GetNetIp(const std::string &netdev_name, std::string &ip);	
		int SetRemoteActionDone(int value);
		int get_ntptime();
		bool GetDCconnetStatus();
		inline int GetGPSSignalInfo(){return gps_signal_info;};
		void ResetAccStatus();
		int GetAccResumeStatus();
		//add gps
		LocationInfo_t mLocationInfo;
		void RunAccEvent();
		void RunBindFlagDetect();
		void RunIrCheckFun();
	    int GetGpsSignalLevel(){return mGpsSignal;}
		int Get4gSignalLevel();
                int GetGPSrecord(char* buff, char* pwr, char* &prd, char *bufret);
		int  GetGpsStatus(){return mGpsStatus;}
		int  CheckGpsOnline(){return GPSModuleStatus;}
		void SetGpsswitch(int val);
		void GetGpsswitchConfig();
		int  GetAccStatus(){return mAccStatus;}
		int GetNetStatus(){return mNetStatus;}
		void SetNetStatus(int val){mNetStatus = val;}
		int getIMEI(std::string &p_Imei);
		const std::string getSimId(){return sim;}
		std::string get4GVersion(){return m_4g_version;}
		int getLocationInfo(LocationInfo_t &p_LocationInfo);
		//int getLocationInfoEx(void* data);
		int getBindFlagString(char *buffer);
		int IRledCtrl(bool flag);
		void processgngga( char *buf);
		void processgnrmc( char *buf);
		void processgpgsv( EventManager *em, char *buf);
		void processgpsbuf( char *buf);
		
		int setIrLedBrightness(enum IRLedState IrLedState);
		void UTCsetSystemTime(int year, int month, int day, int hour, int minute, int second, int timezone);
		void set_tz(int tz);
		void set_tz_ex(int tz);
#ifdef MEM_DEBUG
		int debug_get_info();
		int debug_count;
#endif			
#ifdef DEBUG_GPSSAVELOG
		int gpslogfd;
		void dosavegpslog(char *buff);
#endif
		bool mStandbyStatus;
		bool mDcConnect_flag;
		bool mNotify_acc_off_flag;
		bool mNotify_acc_on_flag;
		bool mStandbybreak_flag;
		int standby_try_count;
		bool mWakeup_event_flag;
		int is_enable_acc;
		int mDcStatus;
		int mGpsSignal;
		int gps_signal_info;
		int mGpsStatus;
		int mAccStatus;	
		int mAccIn_flag;
	    bool low_battery_flag;
		bool mNeedSwitchPid;
		bool mOpenSimcomGpsDone;
		int m_4GWakeUp_state;
		int mNetStatus;
		bool IrisOpen;
		int m_4GModule_flag;
		int m_facturer;
		int mNetTime_flag;
		std::string m_NetSignal;
		std::string m_NetType;	
		std::string imei;
		std::string sim;
		std::string facturer;
		std::string m_4g_version;
		char TFCard_Cid[64];
		int mTimer_thread_flag;
		int RemoteActionDone;
		std::string GNGGA_string;
		std::string GPGSV_string;
		std::string mGpgsvlog;
		int GPSModuleStatus;
		 int gpsswitch;
		 bool rmcflag;
		 bool synctimeflag;
		 bool sdcardFlag;
    private:
        pthread_t event_loop_thread_id_;
        pthread_t netlink_monitor_thread_id_;
		pthread_t m_ImapactEvent_thread_id;
		pthread_t m_GpsEvent_thread_id;		
		pthread_t m_Usb4G_thread_id;
		pthread_t m_AccEvent_thread_id;		
		pthread_t m_NetEvent_thread_id;
		pthread_t m_NetTimer_thread_id;
		pthread_t m_BindFlagEvent_thread_id;
		pthread_t m_IrCheckFun_thread_id;
		pthread_mutex_t net_lock;
        int listen_fd_;
        int netlink_fd_;
		int usb4g_fd;
		int gpsFd;
		int gpsfdorg;
        EventManager();
        ~EventManager();
        EventManager(const EventManager &o);
        EventManager &operator=(const EventManager &o);

		int configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop);
		int openSerialPort(char *ttys);			
		int FindSerialPort(enum USBPort port_index);
		int LocationReportThreadInit();
		int WriteFileInt(char const* path, int value);
        /**
         * @brief 鍒濆鍖栫洃鍚湇鍔＄
         * @return listen socketfd
         */
        int UdevMonitorInit();

        /**
         * @brief 鑾峰彇鏈嶅姟绔敹鍒扮殑浜嬩欢娑堟伅
         * @param sockfd 鏈嶅姟绔痵ocket fd
         * @param event 鎺ユ敹鑾峰彇鍒扮殑浜嬩欢
         * @return
         *  - -1 failed
         *  - 0 succeed
         */
        int TryEvent(int sockfd, int &event);

        int InitNetLinkMonitor();

        int ReadNetLinkEvent(int sockfd);
				
        int CheckSDCard();

        int CheckUSBDisk();

        int CheckHDMI();

        int CheckTVOut();
		
		int CheckACCStatus();

		int Check4GWakeUpStatus();
		
        int CheckBluetooth();

        int CheckNetLink(const char *interface);

        int NetLinkMsgHandler(struct sockaddr_nl *sockaddr, struct nlmsghdr *nlmsg);
				
		int USB4G_ComRead(int fd, char* buf, int len);

		int USB4G_ComWrite(int fd, char* buf, int len);

        bool IsGotIP(const char *interface, char *IP);
        bool IsNetReady(const char *interface, char *IP);
		void USB4G_OpenUdhcp();	
		int USB4GModules_init();
		int USB4GSerial_init();
		
		int USB4GSerial_exit();
		int setGPSData();
		void SetGpsSignalLevel(int level){mGpsSignal = level;}
		void SetGpsStatus(int flag){mGpsStatus = flag;}
		void SetAccStatus(int flag){mAccStatus = flag;}

		
}; /* EventManager */

/** @} */
/** @} */
/** @} */

} /* EyeseeLinux */

