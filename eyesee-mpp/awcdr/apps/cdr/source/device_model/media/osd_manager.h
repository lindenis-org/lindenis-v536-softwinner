/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         osd_manager.h
 Version:             1.0
 Author:              KPA362
 Created:            2017/6/16
 Description:       osd setting
 * *******************************************************************************/

#pragma once
#include <utils/Thread.h>
#include <utils/Mutex.h>
#include <map>
#include "bll_presenter/common_type.h"
#include "rgb_ctrl.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"
#include <mm_comm_video.h>
#include <time.h>
#include <signal.h>

#include "device_model/media/recorder/recorder.h"
#include "device_model/system/event_manager.h"


#define OSDTHRREAD_USE_TIMER

typedef std::map<int, int> RegionID;


namespace EyeseeLinux
{
    class OsdManager
    {
        private:
            #define OVERLAY_CONFIG_FILE "/tmp/data/overlay_config.lua"
            #define MAX_RECODER_NUM 4
            #define MAX_NAME_LEN 128
            #define MAX_CHH_NUM 4
            #define THREAD_SLEEP_TIME 1
            #define MAX_CHAR_LEN 256

            typedef struct tOSDRECT
            {
                int nLeft;
                int nTop;
                int nWidth;
                int nHeight;
            }RECT;
            
            typedef struct tTIMEOSD
            {
               int nEnable;
               int nTimefmt;
               int nDatefmt;
               RECT rcTime;
            }TIMEOSD;

			typedef struct tGPSOSD
            {
                int nEnable;
                RECT rcgps;
                char szgps[MAX_NAME_LEN];
            }GPSOSD;
			
            typedef struct tDEVNAMEOSD
            {
                int nEnable;
                RECT rcDevName;
                char szDevName[MAX_NAME_LEN];
            }DEVNAMEOSD;

            typedef struct tCHNNAMEOSD
            {
                int nEnable;
                RECT rcChnName;
                char szChnName[MAX_NAME_LEN];
            }CHNNAMEOSD;

            typedef struct tRECORDER
            {
                EyeseeRecorder *m_recorder[MAX_RECODER_NUM];
                int  m_arrCamID[MAX_RECODER_NUM];
                int  m_arrRecordId[MAX_RECODER_NUM];
                int  m_arrEnable[MAX_RECODER_NUM];
            }RECORDER;
            typedef struct gps_info_s
            {        
                unsigned int Hour;       //gps信息里输出的小时 
                unsigned int Minute;     //gps信息里输出的分 
                unsigned int Second;     //gps信息里输出的秒 
                unsigned int Year;       //gps信息里输出的年 
                unsigned int Month;      //gps信息里输出的月 
                unsigned int Day;        //gps信息里输出的天 
                
                char Status;         //gps信息里输出的状态 
                char NSInd;          //gps信息里输出的NSInd 
                char EWInd;          //gps信息里输出的EWInd 
                char reserved;       //gps信息里输出的reserved
                
                double Latitude;      //gps信息里输出的纬度,注意这是double型 
                double Longitude;     //gps信息里输出的经度，注意这是double型 
                float Speed;         //gps信息里输出的速度 
                float Angle;         //gps信息里输出的方向 
                
                char ID[20];         //这个ID是固化在每个相机设备的主控里的，一旦烧录就不永远不会变，每台设备的ID是不一样的。 
                
                unsigned int GsensorX;   //gps信息里输出的重力加速X 
                unsigned int GsensorY;   //gps信息里输出的重力加速Y 
                unsigned int GsensorZ;   //gps信息里输出的重力加速Z 
                
                unsigned int MHour;     //主控的时间小时 ，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
                unsigned int MMinute;   //主控的时间分，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
                unsigned int MSecond;   //主控的时间秒，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
                unsigned int MYear;     //主控的时间年，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
                unsigned int MMonth;    //主控的时间月，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
                unsigned int MDay;      //主控的时间天，这个主要是用于显示在app上的相机当前时间，与gps时间不一样 
            }GPSINFO;   
        public:
            OsdManager();
            ~OsdManager();

        public:

           /****************************************************
                    * Name:
                    *     get()
                    * Function:
                    *     get OsdManager single object
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     OsdManager single object
                   *****************************************************/
            static OsdManager* get();

           /****************************************************
                    * Name:
                    *     initTimeOsd()
                    * Function:
                    *     init time osd
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     0: success
                    *     -1: failed
                   *****************************************************/
            int initTimeOsd(CamRecMap &p_cam_rec_map);

           /****************************************************
                    * Name:
                    *     unInitTimeOsd()
                    * Function:
                    *     uninit time osd
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     0: success
                    *     -1: failed
                   *****************************************************/
            int unInitTimeOsd();

           /****************************************************
                    * Name:
                    *     initCoverOsd()
                    * Function:
                    *     init cover osd
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     0: success
                    *     -1: failed
                   *****************************************************/
            int initCoverOsd();

           /****************************************************
                    * Name:
                    *     unInitCoverOsd()
                    * Function:
                    *     uninit cover osd
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     0: success
                    *     -1: failed
                   *****************************************************/
            int unInitCoverOsd();

           /****************************************************
                     * Name:
                     *     startTimeOsd()
                     * Function:
                     *     according to p_nCamID & p_nRecType to select which recorder will start
                     * Parameter:
                     *     input:
                     *         p_nCamID:    camera id (refer to enum CameraID)
                     *         p_RecordId:  record id
                     *     output:
                     *         none
                     * return:
                     *     0: success
                     *     -1: failed
                    *****************************************************/
            int startTimeOsd(int p_nCamID, int p_RecordId);

            /****************************************************
                      * Name:
                      *     stopTimeOsd()
                      * Function:
                      *     according to p_nCamID & p_nRecType to select which recorder will stop
                      * Parameter:
                      *     input:
                      *         p_nCamID:    camera id (refer to enum CameraID)
                      *         p_RecordId:  record id
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int stopTimeOsd(int p_nCamID, int p_RecordId);

            /****************************************************
                      * Name:
                      *     enableTimeOsd()
                      * Function:
                      *     enable time osd
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int enableTimeOsd();

            /****************************************************
                      * Name:
                      *     disableTimeOsd()
                      * Function:
                      *     disable time osd
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int disableTimeOsd();

            /****************************************************
                      * Name:
                      *     setTimeOsdPostion()
                      * Function:
                      *     set time osd position
                      * Parameter:
                      *     input:
                      *         p_nLeft: left postion
                      *         p_nTop: top postion
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setTimeOsdPostion(int p_nLeft, int p_nTop);

            /****************************************************
                      * Name:
                      *     getTimeOsdPostion()
                      * Function:
                      *     get time osd position
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nLeft: left postion
                      *         p_nTop: top postion
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getTimeOsdPostion(int &p_nLeft, int &p_nTop);

            /****************************************************
                      * Name:
                      *     setTimeOsdFormat()
                      * Function:
                      *     set time osd format
                      * Parameter:
                      *     input:
                      *         p_nDateFmt: date format
                      *         p_nTimeFmt: time format(12hour or 24hour)
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setTimeOsdFormat(int p_nDateFmt, int p_nTimeFmt);

            /****************************************************
                      * Name:
                      *     getTimeOsdFormat()
                      * Function:
                      *     get time osd format
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nDateFmt: date format
                      *         p_nTimeFmt: time format(12hour or 24hour)
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getTimeOsdFormat(int &p_nDateFmt, int &p_nTimeFmt);

            /****************************************************
                      * Name:
                      *     getTimeOsdStatus()
                      * Function:
                      *     get time osd enable status
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nStatus: time osd enable status
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getTimeOsdStatus(int &p_nStatus);

            /****************************************************
                      * Name:
                      *     disableChannelNameOsd()
                      * Function:
                      *     disable appointed channel name osd show
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int disableChannelNameOsd(int p_nChannel);

            /****************************************************
                      * Name:
                      *     enableChannelNameOsd()
                      * Function:
                      *     enable appointed channel name osd show
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int enableChannelNameOsd(int p_nChannel);

            /****************************************************
                      * Name:
                      *     setChannelNameOsdPosition()
                      * Function:
                      *     set channel name osd postion
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *         p_nLeft:   left postion
                      *         p_nTop:   top postion
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setChannelNameOsdPosition(int p_nChannel, int p_nLeft, int p_nTop);

            /****************************************************
                      * Name:
                      *     getChannelNameOsdPosition()
                      * Function:
                      *     get channel name osd postion
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nChannel: channel id
                      *         p_nLeft:   left postion
                      *         p_nTop:   top postion
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getChannelNameOsdPosition(int p_nChannel, int &p_nLeft, int &p_nTop);

            /****************************************************
                      * Name:
                      *     setChannelNameOsdString()
                      * Function:
                      *     set channel name
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *         p_chnName:   channel name
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setChannelNameOsdString(int p_nChannel, const char *p_chnName);

            /****************************************************
                      * Name:
                      *     getChannelNameOsdString()
                      * Function:
                      *     get channel name
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *     output:
                      *         p_chnName:   channel name
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getChannelNameOsdString(int p_nChannel, char *p_chnName);

            /****************************************************
                      * Name:
                      *     getChannelNameOsdStatus()
                      * Function:
                      *     get channel name enable status
                      * Parameter:
                      *     input:
                      *         p_nChannel: channel id
                      *     output:
                      *         p_nStatus:   channel name enable status
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getChannelNameOsdStatus(int p_nChannel, int &p_nStatus);

            /****************************************************
                      * Name:
                      *     disableDeviceNameOsd()
                      * Function:
                      *     disable  device  name osd show
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int disableDeviceNameOsd();

            /****************************************************
                      * Name:
                      *     enableDeviceNameOsd()
                      * Function:
                      *     enable  device  name osd show
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int enableDeviceNameOsd();

            /****************************************************
                      * Name:
                      *     setDeviceNameOsdPosition()
                      * Function:
                      *     set  device  name osd postion
                      * Parameter:
                      *     input:
                      *         p_nLeft:  left postion
                      *         p_nTop:  top postion
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setDeviceNameOsdPosition(int p_nLeft, int p_nTop);

            /****************************************************
                      * Name:
                      *     getDeviceNameOsdPosition()
                      * Function:
                      *     get  device  name osd postion
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nLeft:  left postion
                      *         p_nTop:  top postion
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getDeviceNameOsdPosition(int &p_nLeft, int &p_nTop);

            /****************************************************
                      * Name:
                      *     setDeviceNameOsdString()
                      * Function:
                      *     set  device  name
                      * Parameter:
                      *     input:
                      *         p_devname:  device name
                      *     output:
                      *         none
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int setDeviceNameOsdString(const char *p_devname);

            /****************************************************
                      * Name:
                      *     getDeviceNameOsdString()
                      * Function:
                      *     get  device  name
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_devname:  device name
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getDeviceNameOsdString(char *p_devName);

            /****************************************************
                      * Name:
                      *     getDeviceNameOsdStatus()
                      * Function:
                      *     get  device  name enable status
                      * Parameter:
                      *     input:
                      *         none
                      *     output:
                      *         p_nStatus:  device name enable status
                      * return:
                      *     0: success
                      *     -1: failed
                     *****************************************************/
            int getDeviceNameOsdStatus(int &p_nStatus);

           /****************************************************
                    * Name:
                    *     loadTimeOsdConfig()
                    * Function:
                    *     initial time osd config according to lua config
                    * Parameter:
                    *     input:
                    *         none
                    *     output:
                    *         none
                    * return:
                    *     0: success
                    *     -1: failed
                   *****************************************************/
          int loadOsdConfig();

          /****************************************************
                   * Name:
                   *     saveTimeOsdConfig()
                   * Function:
                   *     save time osd config  to lua config
                   * Parameter:
                   *     input:
                   *         none
                   *     output:
                   *         none
                   * return:
                   *     0: success
                   *     -1: failed
                  *****************************************************/
          int saveOsdConfig();
          int AttchVencRegion(int val,int region_id);
          int DettchVencRegion(int val,int region_Id);
		  int AttchCarIdRegion(int val,int region_id);
		  #if 0
          int DettchCarIdRegion(int val,int region_Id);
		  #endif
		  int setCaridOsdPosition(int p_nLeft, int p_nTop);
		  
		  int AttchGpsRegion(int val,int region_id);
          int DettchGpsRegion(int val,int region_Id);
		  
          int GetOsdBitMap(BITMAP_S &stBmp);
          int addCamRecordMap(int cam_id,int rec_id,Recorder *rec);
          int setlogoOsdRect();
          int AttchlogoVencRegion(int val,int region_id);
          int setGPSinfoToRecord();

		  int startGpsOsd(int p_nCamID, int p_RecordId);
		  int stopGpsOsd(int p_nCamID, int p_RecordId);
		  int enableGpsOsd();
		  int disableGpsOsd();
		      
		  int setGpsOsdPosition(int p_nLeft, int p_nTop);
		  
		  void SetTimeOsdFontSize(FONT_SIZE_TYPE sz) { m_FontSize = sz; }
		  FONT_SIZE_TYPE GetTimeOsdFontSize() { return m_FontSize; }
     private:
          /****************************************************
                   * Name:
                   *     OsdUpdateThread()
                   * Function:
                   *     update time osd
                   * Parameter:
                   *     input:
                   *         arg: this object
                   *     output:
                   *         none
                   * return:
                  *****************************************************/
          static void *OsdUpdateThread(void *arg);

          /****************************************************
                   * Name:
                   *     createPicTimeOsdRgb()
                   * Function:
                   *     create pic time osd rgb
                   * Parameter:
                   *     input:
                   *         none
                   *     output:
                   *         none
                   * return:
                   *     0: success
                   *     -1: failed
                  *****************************************************/
          int createPicTimeOsdRgb();

          /****************************************************
                   * Name:
                   *     createTimeOsdRgb()
                   * Function:
                   *     create time osd rgb
                   * Parameter:
                   *     input:
                   *         none
                   *     output:
                   *         none
                   * return:
                   *     0: success
                   *     -1: failed
                  *****************************************************/
          int createTimeOsdRgb();

          /****************************************************
                   * Name:
                   *     deleteTimeOsdRgb()
                   * Function:
                   *     delete the time osd rgb
                   * Parameter:
                   *     input:
                   *         none
                   *     output:
                   *         none
                   * return:
                   *     0: success
                   *     -1: failed
                  *****************************************************/
          int deleteTimeOsdRgb();

          /****************************************************
                   * Name:
                   *     updateTimeOsd()
                   * Function:
                   *     update  time osd
                   * Parameter:
                   *     input:
                   *         none
                   *     output:
                   *         none
                   * return:
                   *     0: success
                   *     -1: failed
                  *****************************************************/
          int updateTimeOsd();
          int updatelogoOsd();
         // int initOverylayRegion();

          /****************************************************
                   * Name:
                   *     loadBitmapFromFile()
                   * Function:
                   *     load bmp picture
                   * Parameter:
                   *     input:
                   *         path: bmp picture file
                   *     output:
                   *         bitmap
                   * return:
                   *     0: success
                   *     -1: failed
                   * notice:
                   *     1:bmp pixel must be divide by 16
                   *     2: alpha should not set 0
         *****************************************************/
        int loadBitmapFromFile(const std::string &path);
        int createispdebugRgb();
        int updateispDebugOsd();
        int deleteispdebugRgb();

		int createCarIdOsdRgb();
        int updateCarIdOsd();
        int deleteCarIdOsdRgb();

		int creategpsOsdRgb();
        int updategpsOsd();
        int deletegpsOsdRgb();
		
        void bmp_filp(int lineBytes, char *src_bmp, char *dst_bmp);
        static void OsdTimerProc(union sigval sigval);
	private:
        static pthread_mutex_t m_mutex;
        static OsdManager* m_instance;
        pthread_mutex_t m_osdLock;
        int m_EncChnNum;
        LuaConfig *m_LuaCfg;
        TIMEOSD   m_tTimeOsd;
		GPSOSD    m_tgpsOsd;
        DEVNAMEOSD  m_tDevNameOsd;
        CHNNAMEOSD  m_tChnNameOsd[MAX_CHH_NUM];
        bool m_bThreadExit;
        pthread_t m_nThreadId;
        bool m_bTimeOsdInit;
        RGB_PIC_S m_tTimeRgb;		// datetime watermark for video file
		RGB_PIC_S m_tCarIdRgb;		// carid watermark for video file
        RGB_PIC_S m_tPicTimeRgb;	// datetime watermark for photo file
        RGB_PIC_S m_ispDebugRgb;        
        RGB_PIC_S m_logoRgb;		// logo watermark for video file
        RGB_PIC_S m_tgpsRgb;		// gps watermark for video file
        RECORDER m_tRecorder;
        RGN_HANDLE mVencOverlayHandle;
        RGN_HANDLE mVencOverlayHandle_isp;

		int mLogoOsdRgnId;
		int mCaridOsdRgnId;
		int mTimeOsdRgnId;
		int mGpsOsdRgnid;
		
        RegionID m_RegionId;
        RECT m_logoRect;
        GPSINFO m_gpsinfo;
        char *pic_addr1;

		FONT_SIZE_TYPE m_FontSize;

        timer_t osd_timer_id_;
    };
}
