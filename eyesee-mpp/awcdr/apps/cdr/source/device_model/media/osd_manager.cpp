/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         osd_manager.h
 Version:             1.0
 Author:              KPA362
 Created:            2017/6/16
 Description:       osd setting
 * *******************************************************************************/

#include "osd_manager.h"
#include <EyeseeRecorder.h>
#include "common/app_log.h"
#include "common/setting_menu_id.h"
#include "device_model/menu_config_lua.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "bll_presenter/camRecCtrl.h"
#include "common/posix_timer.h"


#include "device_model/media/camera/camera_factory.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/DxAZGPS.h"






using namespace EyeseeLinux;
using namespace std;

//#define ISP_DEBUG
#define GPS_INFO_DEBUG


#pragma pack(2)
typedef struct {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BMPFILEHEADER_T;

#pragma pack(2)
typedef struct{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BMPINFOHEADER_T;


OsdManager* OsdManager::m_instance = NULL;
pthread_mutex_t OsdManager::m_mutex = PTHREAD_MUTEX_INITIALIZER;

OsdManager *OsdManager::get()
{
    if( OsdManager::m_instance == NULL )
    {
        pthread_mutex_lock(&OsdManager::m_mutex);
        if( OsdManager::m_instance == NULL )
        {
            OsdManager::m_instance = new OsdManager();
        }
        pthread_mutex_unlock(&OsdManager::m_mutex);
    }

    return OsdManager::m_instance;
}

OsdManager::OsdManager()
{
    m_EncChnNum = 0;
    m_tTimeOsd.nEnable = 0;
    m_LuaCfg = new LuaConfig();
    memset(&m_tTimeOsd, 0, sizeof(m_tTimeOsd));
    memset(&m_tDevNameOsd, 0, sizeof(m_tDevNameOsd));
    memset(m_tChnNameOsd, 0, sizeof(m_tChnNameOsd));
    m_bThreadExit = false;
    m_bTimeOsdInit = false;
    memset(&m_tTimeRgb, 0, sizeof(m_tTimeRgb));
	memset(&m_tCarIdRgb, 0, sizeof(m_tCarIdRgb));
    memset(&m_ispDebugRgb, 0, sizeof(m_ispDebugRgb));
    memset(&m_tPicTimeRgb, 0, sizeof(m_tPicTimeRgb));
    memset(&m_tRecorder, 0, sizeof(m_tRecorder) );
    memset(&m_logoRect, 0, sizeof(m_logoRect));
    memset(&m_logoRgb,0,sizeof(m_logoRgb));
    #ifdef GPS_INFO_DEBUG
    memset(&m_gpsinfo,0,sizeof(m_gpsinfo));
    #endif
	m_FontSize = FONT_SIZE_32;
    pthread_mutex_init(&m_osdLock, NULL);
}

OsdManager::~OsdManager()
{
    if( m_LuaCfg != NULL )
    {
        delete m_LuaCfg;
    }
    pthread_mutex_destroy(&m_osdLock);
}

int OsdManager::addCamRecordMap(int cam_id,int rec_id,Recorder *rec)
{
     m_tRecorder.m_recorder[rec_id] = rec->GetEyeseeRecorder();
     m_tRecorder.m_arrCamID[rec_id] = cam_id;
     m_tRecorder.m_arrRecordId[rec_id] = rec_id;
     m_tRecorder.m_arrEnable[rec_id] = false;
     m_EncChnNum = MAX_RECODER_NUM;//rec_id;

}

#ifdef GPS_INFO_DEBUG

int OsdManager::setGPSinfoToRecord()
{
    //get GPS info 
    char id[21] = "1111111111111111QZKJ";
    LocationInfo_t info;
   // db_error("set gps info 111\n");
   EventManager::GetInstance()->getLocationInfo(info);
   // db_error("set gps info 2222\n");
    m_gpsinfo.Year = info.gtm.tm_year;
    m_gpsinfo.Month = info.gtm.tm_mon;
    m_gpsinfo.Day = info.gtm.tm_mday;
    m_gpsinfo.Hour = info.gtm.tm_hour;
    m_gpsinfo.Minute = info.gtm.tm_min;
    m_gpsinfo.Second = info.gtm.tm_sec;

    memcpy(m_gpsinfo.ID ,id,20);

    m_gpsinfo.Status = info.status;
    m_gpsinfo.NSInd = info.NS;
    m_gpsinfo.EWInd = info.EW;	
    m_gpsinfo.Latitude =  encryptLatitude(info.latitude,m_gpsinfo.Second,m_gpsinfo.ID);		// 加密
    m_gpsinfo.Longitude = encryptLongitude(info.longitude,m_gpsinfo.Minute,m_gpsinfo.ID);
	m_gpsinfo.Speed = info.speed;	// knot
    m_gpsinfo.Angle = info.altitude;
	

    m_gpsinfo.GsensorX = 0;
    m_gpsinfo.GsensorY = 0;
    m_gpsinfo.GsensorZ = 0;
    
    struct tm * tm=NULL;
    time_t timer;
    timer = time(NULL);	// 从公元1970年01月01日的UTC时间从0时0分0秒算起到现在所经过的秒数
    tm = localtime(&timer);
    m_gpsinfo.MHour = tm->tm_hour;
    m_gpsinfo.MMinute = tm->tm_min;
    m_gpsinfo.MSecond = tm->tm_sec;
    m_gpsinfo.MYear = tm->tm_year+1900;
    m_gpsinfo.MMonth = tm->tm_mon+1;
    m_gpsinfo.MDay = tm->tm_mday;
    #if 0
    db_msg("y:%d; M:%d; D:%d; h:%d; m:%d; s:%d N:%c ;E: %c ;L = %f ;L = %f; s = %f state = %c\n", \
    m_gpsinfo.Year, m_gpsinfo.Month, m_gpsinfo.Day, m_gpsinfo.Hour, m_gpsinfo.Minute, m_gpsinfo.Second, m_gpsinfo.NSInd, m_gpsinfo.EWInd \
    ,m_gpsinfo.Latitude,m_gpsinfo.Longitude, m_gpsinfo.Speed,m_gpsinfo.Status);
    #endif
    // set gpsinfo to record
    if(m_tRecorder.m_recorder[0] != NULL)
    {
      //  db_error("set gps info 333\n");
        m_tRecorder.m_recorder[0]->gpsInfoSend(&m_gpsinfo);	// -> EyeseeRecorder
    }
    
  //  db_error("set gps info 444\n");
    return 0;
}
#endif


int OsdManager::setlogoOsdRect()
{
    int value;
    MenuConfigLua *menu_config = MenuConfigLua::GetInstance();
    value = menu_config->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
    //db_error("[debug_jaosn]:setlogoOsdRect value  = %d\n",value);
    switch(value){
        case 0:
            m_logoRect.nLeft = 0;            
            m_logoRect.nTop = 0;            
            m_logoRect.nWidth = 3840;            
            m_logoRect.nHeight = 2160;
        break;
        case 1:
            m_logoRect.nLeft = 0;            
            m_logoRect.nTop = 0;            
            m_logoRect.nWidth = 2688;            
            m_logoRect.nHeight = 1520;
            break;
        case 2:
            m_logoRect.nLeft = 0;            
            m_logoRect.nTop = 0;            
            m_logoRect.nWidth = 1920;            
            m_logoRect.nHeight = 1080;
            break;
    }
    return 0;
}

int OsdManager::initTimeOsd(CamRecMap &p_cam_rec_map)
{
    #if 0
    m_EncChnNum = 0;

	map<int, std::map<int, Recorder *>>::iterator cam_iter;
    for(cam_iter = p_cam_rec_map.begin();cam_iter != p_cam_rec_map.end(); cam_iter++)
    {
        map<int, Recorder*>::iterator rec_iter;
        for( rec_iter = p_cam_rec_map[cam_iter->first].begin(); rec_iter != p_cam_rec_map[cam_iter->first].end();rec_iter++)
        {
           m_tRecorder.m_recorder[m_EncChnNum] = rec_iter->second->GetEyeseeRecorder();
           m_tRecorder.m_arrCamID[m_EncChnNum] = cam_iter->first;
           m_tRecorder.m_arrRecordId[m_EncChnNum] = rec_iter->first;
           m_tRecorder.m_arrEnable[m_EncChnNum] = false;
           m_EncChnNum++;
        }
    }

    if( 0 == m_EncChnNum )
    {
        db_error("no chn enable time osd");
        return 0;
    }
    #endif
	if( m_bTimeOsdInit )
	{
		return 0;
	}

    loadOsdConfig();
    setlogoOsdRect();    
    //loadBitmapFromFile("/usr/share/minigui/res/images/osdlogo.bmp");
    loadBitmapFromFile("");

    int ret = load_font_file(FONT_SIZE_32);
    if(ret < 0)
    {
        db_error("load_font_file FONT_SIZE_32 fail! ret:%d\n", ret);
        return -1;
    }

    ret = load_font_file(FONT_SIZE_64);
    if (ret < 0)
    {
        db_error("Do load_font_file FONT_SIZE_64 fail! ret:%d\n", ret);
        return -1;
    }

  //  initOverylayRegion();
	m_FontSize = FONT_SIZE_64;
    m_bThreadExit = false;
    #ifdef OSDTHRREAD_USE_TIMER
    create_timer(this, &osd_timer_id_, OsdTimerProc);
    stop_timer(osd_timer_id_);
    #else
    pthread_create(&m_nThreadId, NULL, OsdUpdateThread, this);
    #endif
    m_bTimeOsdInit = true;

    
    return 0;
}

int OsdManager::unInitTimeOsd()
{
    if( false == m_EncChnNum )
    {
        db_warn("Time Osd haved Exited or not init!\n");
        return 0;
    }

    m_EncChnNum = 0;
    #ifdef OSDTHRREAD_USE_TIMER
    ::stop_timer(osd_timer_id_);
    ::delete_timer(osd_timer_id_);
    #else
    m_bThreadExit = true;
    pthread_join(m_nThreadId, NULL);
    #endif
    m_bTimeOsdInit = false;
    memset(&m_tRecorder, 0, sizeof(m_tRecorder));
    saveOsdConfig();

    

    return 0;
}

int OsdManager::initCoverOsd()
{
    return 0;
}

int OsdManager::unInitCoverOsd()
{
    return 0;
}

int OsdManager::loadOsdConfig()
{
    pthread_mutex_lock(&m_osdLock);
    if( NULL == m_LuaCfg )
    {
        db_error("m_LuaCfg is null");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if (!FILE_EXIST(OVERLAY_CONFIG_FILE))
    {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", OVERLAY_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/overlay_config.lua /tmp/data/");
    }

    if( m_LuaCfg->LoadFromFile(OVERLAY_CONFIG_FILE) )
    {
        db_warn("Load %s failed, copy backup and try again\n", OVERLAY_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/overlay_config.lua /tmp/data/");
        if( m_LuaCfg->LoadFromFile(OVERLAY_CONFIG_FILE) )
        {
            db_error("Load %s fail!\n", OVERLAY_CONFIG_FILE);
            pthread_mutex_unlock(&m_osdLock);
            return -1;
        }
    }

    //m_tTimeOsd.nEnable = m_LuaCfg->GetIntegerValue("overlay.time_osd.osd_enable");
    m_tTimeOsd.nTimefmt = m_LuaCfg->GetIntegerValue("overlay.time_osd.time_format");
    m_tTimeOsd.nDatefmt = m_LuaCfg->GetIntegerValue("overlay.time_osd.date_format");
    m_tTimeOsd.rcTime.nLeft = m_LuaCfg->GetIntegerValue("overlay.time_osd.left");
    m_tTimeOsd.rcTime.nTop = m_LuaCfg->GetIntegerValue("overlay.time_osd.top");

    m_tDevNameOsd.nEnable = m_LuaCfg->GetIntegerValue("overlay.device_osd.osd_enable");
    m_tDevNameOsd.rcDevName.nLeft = m_LuaCfg->GetIntegerValue("overlay.device_osd.left");
    m_tDevNameOsd.rcDevName.nTop = m_LuaCfg->GetIntegerValue("overlay.device_osd.top");
    string str = m_LuaCfg->GetStringValue("overlay.device_osd.device_name");
    strncpy(m_tDevNameOsd.szDevName,str.c_str(), sizeof(m_tDevNameOsd.szDevName));

	m_tgpsOsd.rcgps.nLeft  = m_LuaCfg->GetIntegerValue("overlay.GPS_osd.osd_enable");
	m_tgpsOsd.rcgps.nLeft = m_LuaCfg->GetIntegerValue("overlay.GPS_osd.left");
    m_tgpsOsd.rcgps.nTop = m_LuaCfg->GetIntegerValue("overlay.GPS_osd.top");
#if 0
    char tmpStr[MAX_CHAR_LEN];
    for(int chnNo =0; chnNo< m_EncChnNum; chnNo++)
    {
        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr) - 1, "overlay.camera[%d].channel_osd.osd_enable", chnNo);
        m_tChnNameOsd[chnNo].nEnable = m_LuaCfg->GetIntegerValue(tmpStr);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr) - 1, "overlay.camera[%d].channel_osd.left", chnNo);
        m_tChnNameOsd[chnNo].rcChnName.nLeft = m_LuaCfg->GetIntegerValue(tmpStr);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr) - 1, "overlay.camera[%d].channel_osd.top", chnNo);
        m_tChnNameOsd[chnNo].rcChnName.nTop = m_LuaCfg->GetIntegerValue(tmpStr);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr) - 1, "overlay.camera[%d].channel_osd.channel_name", chnNo);
        str = m_LuaCfg->GetStringValue(tmpStr);
        strncpy(m_tChnNameOsd[chnNo].szChnName, str.c_str(), sizeof(m_tChnNameOsd[chnNo].szChnName));

        db_msg("ChnNameOsd[%d][enable %d rect(left %d top %d) devname %s]",chnNo,m_tDevNameOsd.nEnable,
            m_tDevNameOsd.rcDevName.nLeft,m_tDevNameOsd.rcDevName.nTop,m_tDevNameOsd.szDevName);
    }
#endif
    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::saveOsdConfig()
{
    pthread_mutex_lock(&m_osdLock);

    if( NULL == m_LuaCfg )
    {
        db_warn("invalid operation");
        pthread_mutex_unlock(&m_osdLock);
        return 0;
    }

    m_LuaCfg->SetIntegerValue("overlay.time_osd.osd_enable", m_tTimeOsd.nEnable);
    m_LuaCfg->SetIntegerValue("overlay.time_osd.time_format", m_tTimeOsd.nTimefmt);
    m_LuaCfg->SetIntegerValue("overlay.time_osd.date_format", m_tTimeOsd.nDatefmt);
    m_LuaCfg->SetIntegerValue("overlay.time_osd.left", m_tTimeOsd.rcTime.nLeft);
    m_LuaCfg->SetIntegerValue("overlay.time_osd.top", m_tTimeOsd.rcTime.nTop);

    m_LuaCfg->SetIntegerValue("overlay.device_osd.osd_enable", m_tDevNameOsd.nEnable);
    m_LuaCfg->SetIntegerValue("overlay.device_osd.left", m_tDevNameOsd.rcDevName.nLeft);
    m_LuaCfg->SetIntegerValue("overlay.device_osd.top", m_tDevNameOsd.rcDevName.nTop);
    m_LuaCfg->SetStringValue("overlay.device_osd.device_name", m_tDevNameOsd.szDevName);

    char tmpStr[MAX_CHAR_LEN];
    for(int chnNo =0; chnNo< m_EncChnNum; chnNo++)
    {
        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr)-1, "overlay.camera[%d].channel_osd.osd_enable", chnNo);
        m_LuaCfg->SetIntegerValue(tmpStr, m_tChnNameOsd[chnNo].nEnable);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr)-1, "overlay.camera[%d].channel_osd.left", chnNo);
        m_LuaCfg->SetIntegerValue(tmpStr, m_tChnNameOsd[chnNo].rcChnName.nLeft);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr)-1, "overlay.camera[%d].channel_osd.top", chnNo);
        m_LuaCfg->SetIntegerValue(tmpStr, m_tChnNameOsd[chnNo].rcChnName.nTop);

        memset(tmpStr, 0, sizeof(tmpStr));
        snprintf(tmpStr, sizeof(tmpStr)-1, "overlay.camera[%d].channel_osd.channel_name", chnNo);
        m_LuaCfg->SetStringValue(tmpStr, m_tChnNameOsd[chnNo].szChnName);
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

void *OsdManager::OsdUpdateThread(void *arg)
{
    OsdManager *osdManager = (OsdManager *)arg;
    prctl(PR_SET_NAME, "OsdUpdateThread", 0, 0, 0);
    while( !osdManager->m_bThreadExit )
    {
        if( !osdManager->m_tTimeOsd.nEnable )
        { 
            //sleep(THREAD_SLEEP_TIME);
            usleep(500*1000);
            continue;
        }

        int ret = pthread_mutex_trylock(&osdManager->m_osdLock);
        if( ret != 0 )
        {
            //sleep(THREAD_SLEEP_TIME);
            usleep(500*1000);
            continue;
        }

        osdManager->createTimeOsdRgb();
        osdManager->updateTimeOsd();
        osdManager->deleteTimeOsdRgb();
        #ifdef GPS_INFO_DEBUG
        osdManager->setGPSinfoToRecord();
        #endif
		
		osdManager->creategpsOsdRgb();
        osdManager->updategpsOsd();
        osdManager->deletegpsOsdRgb();
		
        #ifdef ISP_DEBUG
        osdManager->createispdebugRgb();
        osdManager->updateispDebugOsd();
        osdManager->deleteispdebugRgb();
        #endif
		
        pthread_mutex_unlock(&osdManager->m_osdLock);

        //sleep(THREAD_SLEEP_TIME);
        usleep(500*1000);
    }

    return NULL;
}

void OsdManager::OsdTimerProc(union sigval sigval)
{
    OsdManager *osdManager = (OsdManager *)(sigval.sival_ptr);
    prctl(PR_SET_NAME, "OsdTimerProc", 0, 0, 0);

    if( osdManager->m_tTimeOsd.nEnable )
    { 
        osdManager->createTimeOsdRgb();
        osdManager->updateTimeOsd();
        osdManager->deleteTimeOsdRgb();
        
        #ifdef GPS_INFO_DEBUG
        osdManager->setGPSinfoToRecord();
        #endif
		
		osdManager->creategpsOsdRgb();
        osdManager->updategpsOsd();
        osdManager->deletegpsOsdRgb();
		
        #ifdef ISP_DEBUG
        osdManager->createispdebugRgb();
        osdManager->updateispDebugOsd();
        osdManager->deleteispdebugRgb();
        #endif

    }
}

int OsdManager::createispdebugRgb()
{
    int  exp=-1,  exp_line=-1,   gain=-1,   lv_idx=-1,   color_temp=-1,   rgain=-1,   bgain=-1,grgain = -1, gbgain = -1;
    float R_Gr=0, B_Gb=0;
    char isp_debug[MAX_CHAR_LEN];
    Camera *cam = CamRecCtrl::GetInstance()->GetCamera(0);
    cam->GetEyeseeCamera()->getISPDMsg(&exp, &exp_line, &gain, &lv_idx, &color_temp, &rgain, &bgain, &grgain, &gbgain);
	if( (grgain != 0) || (gbgain != 0) ){
		
		R_Gr = (rgain/grgain)<<8;
		B_Gb = (bgain/gbgain)<<8;
		//printf("Rgain/Grgain*256=%.1f, Bgain/Gbgain*256=%.1f\r\n", R_Gr, B_Gb);
	}
  //  printf("Icekirin->exp=%d,exp_line=%d,gain=%d,lv_idx=%d,colorT=%d,Rgain=%d,Bgain=%d\r\n", exp/1000, exp_line, gain, lv_idx, color_temp, rgain, bgain);
    snprintf(isp_debug, sizeof(isp_debug)-1, "Icekirin->exp=%d, exp_line=%d, gain=%d, lv_idx=%d, colorT=%d, Rgain=%d, Bgain=%d,Rgain/Grgain*256=%.1f, Bgain/Gbgain*256=%.1f", 
                        exp/1000, exp_line, gain, lv_idx, color_temp, rgain, bgain,R_Gr, B_Gb);
    
    FONT_RGBPIC_S font_pic;
    font_pic.font_type     = FONT_SIZE_32;
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    font_pic.foreground[0] = 0x6;
    font_pic.foreground[1] = 0x1;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x21;
    font_pic.background[1] = 0x21;
    font_pic.background[2] = 0x21;
    font_pic.background[3] = 0x11;
    
    m_ispDebugRgb.enable_mosaic = 0;
    m_ispDebugRgb.rgb_type      = OSD_RGB_32;
    
    create_font_rectangle(isp_debug, &font_pic, &m_ispDebugRgb);
    //printf("m_ispDebugRgb.wide = %d,m_ispDebugRgb.height = %d",m_ispDebugRgb.wide,m_ispDebugRgb.high);
    return 0;
    
}


int OsdManager::createTimeOsdRgb()
{
#ifdef USE_AW_OSDPATCH
    deleteTimeOsdRgb();
#endif
	time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    char cDate[MAX_CHAR_LEN];
    memset( cDate, 0, sizeof(cDate) );
    switch (m_tTimeOsd.nDatefmt )
    {
        case 0:/*y/m/d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d/%02d/%02d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;

        case 1:/*m/d/y*/
            snprintf(cDate, sizeof(cDate)-1,"%02d/%02d/%4d",(1 + p->tm_mon), p->tm_mday,(1900 + p->tm_year));
            break;

        case 2:/*y-m-d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d-%02d-%02d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;

        case 3:/*m-d-y*/
            snprintf(cDate, sizeof(cDate)-1,"%4d-%02d-%02d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;

        default: /*y-m-d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d/%02d/%02d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;
    }

    char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    char cTime[MAX_CHAR_LEN];
    memset(cTime, 0, sizeof(cTime));
    switch(m_tTimeOsd.nTimefmt)
    {
        case 0:/*24 hour*/
            snprintf(cTime, sizeof(cTime)-1, "%s %s %02d:%02d:%02d", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            break;

        case 1:/*12 hour*/
            if (p->tm_hour == 0)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s 12:%02d:%02d AM", cDate, wday[p->tm_wday], p->tm_min, p->tm_sec);
            }
            else if (p->tm_hour < 12)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:%02d:%02d AM", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            }
            else if (p->tm_hour == 12)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:02%d:02%d PM", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            }
            else
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:02%d:02%d PM", cDate, wday[p->tm_wday], p->tm_hour - 12, p->tm_min, p->tm_sec);
            }
            break;

        default:
            snprintf(cTime, sizeof(cTime)-1, "%s %s %2d:%02d:%02d", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            break;
    }

    FONT_RGBPIC_S font_pic;
    font_pic.font_type     = m_FontSize;	
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x88;
    font_pic.background[1] = 0x88;
    font_pic.background[2] = 0x88;
    font_pic.background[3] = 0x33;

    m_tTimeRgb.enable_mosaic = 0;
    m_tTimeRgb.rgb_type      = OSD_RGB_32;

    create_font_rectangle(cTime, &font_pic, &m_tTimeRgb);	// -> system\public\rgb_ctrl\rgb_ctrl.c
	//m_tTimeRgb.wide 区域宽   	m_tTimeRgb.high 高
	//db_error("osdTime: %s",cTime);
    return 0;
}

#if 1
static void savebmp(char * pdata, char * bmp_file, int width, int height)
{
    //int size = width*height*3*sizeof(char);       // rgb888  - rgb24
    int size = width * height * 4 * sizeof(char);   // rgb8888 - rgb32

    /* RGB head */
    BMPFILEHEADER_T bfh;
    bfh.bfType = (WORD) 0x4d42;  //bm
    /* data size + first section size + second section size */
    bfh.bfSize = size + sizeof(BMPFILEHEADER_T) + sizeof(BMPINFOHEADER_T);
    bfh.bfReserved1 = 0; // reserved  
    bfh.bfReserved2 = 0; // reserved  
    bfh.bfOffBits = sizeof(BMPFILEHEADER_T) + sizeof(BMPINFOHEADER_T);

    // RGB info
    BMPINFOHEADER_T bih;
    bih.biSize = sizeof(BMPINFOHEADER_T);
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    //bih.biBitCount = 24;  // RGB888
    bih.biBitCount = 32;    // RGB8888
    bih.biCompression = 0;
    bih.biSizeImage = size;
    bih.biXPelsPerMeter = 2835;
    bih.biYPelsPerMeter = 2835;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;
    FILE * fp = NULL;
    fp = fopen(bmp_file, "wb");
    if (!fp) {
        return;
    }

    fwrite(&bfh, 8, 1, fp);
    fwrite(&bfh.bfReserved2, sizeof(bfh.bfReserved2), 1, fp);
    fwrite(&bfh.bfOffBits, sizeof(bfh.bfOffBits), 1, fp);
    fwrite(&bih, sizeof(BMPINFOHEADER_T), 1, fp);
    fwrite(pdata, size, 1, fp);
    fclose(fp);
}
#endif

void OsdManager::bmp_filp(int lineBytes, char *src_bmp, char *dst_bmp)
{
    int i = 0;
    int j = 0;
    int width   = m_logoRgb.wide;
    int height  = m_logoRgb.high;
    db_error("bmp_filp lineBytes = %d,width = %d, height = %d",lineBytes,width,height);
    for(i = 0; i< height ; i++)
    {
        for(j = 0 ; j< lineBytes ; j++)
        {
            dst_bmp[i*lineBytes+j] = src_bmp[(height -1 - i)*lineBytes + j];
        }
    }
}

int OsdManager::loadBitmapFromFile(const     string & path)
{
    int addr_cnt = 0;
	if( path.empty()){
        db_error("loadBitmapFromFile the path is NULL\n");
		return -1;
	}

	FILE *fp = fopen(path.c_str(), "r");
    if (fp == NULL){        
        db_error("loadBitmapFromFile open file is filed\n");
		return -1;
    }

	BMPFILEHEADER_T bmp_file_header = {0};
	fread(&bmp_file_header, sizeof(BMPFILEHEADER_T), 1, fp);
  //  db_error("00 the current pointer is %ld",ftell(fp));
	BMPINFOHEADER_T bmp_info_header = {0};
	fread(&bmp_info_header, sizeof(BMPINFOHEADER_T), 1, fp);
    
    //db_error("11 the current pointer is %ld",ftell(fp));

	if (bmp_info_header.biBitCount != 32)
	{
        db_error("loadBitmapFromFile the bmp file is not 32 bit\n");
		fclose(fp);
		return -1;
	}
    
    if(((bmp_info_header.biWidth % 16) != 0) || ((abs(bmp_info_header.biHeight) % 16)!= 0))
    {
        db_error("loadBitmapFromFile the bmp file is not 16 align\n");
        fclose(fp);
        return -1;
    }
    m_logoRgb.wide = bmp_info_header.biWidth;
    m_logoRgb.high = abs(bmp_info_header.biHeight);
    m_logoRgb.enable_mosaic = 0;
    m_logoRgb.rgb_type      = OSD_RGB_32;
    int data_size = bmp_file_header.bfSize - bmp_file_header.bfOffBits;
    m_logoRgb.pic_addr = (char *)malloc(data_size); 
    if(bmp_info_header.biHeight > 0)
    {
        db_msg("loadBitmapFromFile the bmp file need filp 00\n");
        pic_addr1 = (char *)malloc(data_size); 
        memset(pic_addr1,0xff,data_size);
    }
    fread(m_logoRgb.pic_addr, data_size, 1, fp);
    if(bmp_info_header.biHeight > 0)
    {
        db_msg("loadBitmapFromFile the bmp file need filp 22\n");
        bmp_filp(m_logoRgb.wide*4,m_logoRgb.pic_addr,pic_addr1);
        m_logoRgb.pic_addr = pic_addr1;
    }
    #if 1
     for (addr_cnt = 0; addr_cnt < data_size; addr_cnt++) 
     {
         m_logoRgb.pic_addr[addr_cnt] = m_logoRgb.pic_addr[addr_cnt];                
          if(addr_cnt == 3 || ((addr_cnt -3)%4 == 0))
         {
            m_logoRgb.pic_addr[addr_cnt] = 0xff;
         }
     }
    #endif
    #if 0
    savebmp(m_logoRgb.pic_addr,"/mnt/extsd/osdlogo1.bmp",256,96);
    savebmp(pic_addr1,"/mnt/extsd/osdlogo.bmp",256,96);
	bitmap.mWidth = bmp_info_header.biWidth;
	bitmap.mHeight = abs(bmp_info_header.biHeight);
	bitmap.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    int data_size = bmp_file_header.bfSize - bmp_file_header.bfOffBits;
	bitmap.mpData = malloc(data_size);
    fread(bitmap.mpData, data_size, 1, fp);
    #endif
	fclose(fp);
	
	return 0;
}

int OsdManager::createPicTimeOsdRgb()
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    char cDate[MAX_CHAR_LEN];
    memset( cDate, 0, sizeof(cDate) );
    switch (m_tTimeOsd.nDatefmt )
    {
        case 0:/*y/m/d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d/%2d/%2d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;

        case 1:/*m/d/y*/
            snprintf(cDate, sizeof(cDate)-1,"%2d/%2d/%4d",(1 + p->tm_mon), p->tm_mday,(1900 + p->tm_year));
            break;

        case 2:/*y-m-d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d-%2d-%2d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;

        case 3:/*m-d-y*/
            snprintf(cDate, sizeof(cDate)-1,"%2d-%2d-%4d",(1 + p->tm_mon), p->tm_mday, (1900 + p->tm_year));
            break;

        default: /*y-m-d*/
            snprintf(cDate, sizeof(cDate)-1,"%4d/%2d/%2d",(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
            break;
    }

    char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    char cTime[MAX_CHAR_LEN];
    memset(cTime, 0, sizeof(cTime));
    switch(m_tTimeOsd.nTimefmt)
    {
        case 0:/*24 hour*/
            snprintf(cTime, sizeof(cTime)-1, "%s %s %2d:%02d:%02d", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            break;

        case 1:/*12 hour*/
            if (p->tm_hour == 0)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s 12:%02d:%02d AM", cDate, wday[p->tm_wday], p->tm_min, p->tm_sec);
            }
            else if (p->tm_hour < 12)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:%02d:%02d AM", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            }
            else if (p->tm_hour == 12)
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:02%d:02%d PM", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            }
            else
            {
                snprintf(cTime, sizeof(cTime) - 1,"%s %s %2d:02%d:02%d PM", cDate, wday[p->tm_wday], p->tm_hour - 12, p->tm_min, p->tm_sec);
            }
            break;

        default:
            snprintf(cTime, sizeof(cTime)-1, "%s %s %2d:%02d:%02d", cDate, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
            break;
    }

    FONT_RGBPIC_S font_pic;
    font_pic.font_type     = FONT_SIZE_32;
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x88;
    font_pic.background[1] = 0x88;
    font_pic.background[2] = 0x88;
    font_pic.background[3] = 0x33;

    m_tPicTimeRgb.enable_mosaic = 0;
    m_tPicTimeRgb.rgb_type      = OSD_RGB_32;

    create_font_rectangle(cTime, &font_pic, &m_tPicTimeRgb);

    return 0;
}


int OsdManager::deleteTimeOsdRgb()
{
    if (release_rgb_picture(&m_tTimeRgb))
    {
        db_error("Release time rgb error!\n");
        return -1;
    }
    #if 0
    if (release_rgb_picture(&m_logoRgb))
    {
        db_error("Release time rgb error!\n");
        return -1;
    }
    #endif
    return 0;
}

int OsdManager::deleteispdebugRgb()
{
    if (release_rgb_picture(&m_ispDebugRgb))
    {
        db_error("Release time rgb error!\n");
        return -1;
    }

    return 0;
}

// 公司Logo水印
int OsdManager::AttchlogoVencRegion(int val,int region_id)
{
     pthread_mutex_lock(&m_osdLock);
     //db_error("[debug_jaosn]: stBmp.x = %d ,stBmp.y = %d",m_logoRgb.wide,m_logoRgb.high);
     RGN_ATTR_S region_attr;
     int ret;
     memset(&region_attr, 0, sizeof(region_attr));
     region_attr.enType = OVERLAY_RGN;
     region_attr.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
     region_attr.unAttr.stOverlay.mSize = {m_logoRgb.wide, m_logoRgb.high};
     mVencOverlayHandle = m_tRecorder.m_recorder[val]->createRegion(&region_attr);
     if(mVencOverlayHandle < 0)
     {
         db_error("createRegion falied");
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     BITMAP_S stBmp;
     memset(&stBmp, 0, sizeof(BITMAP_S));
     stBmp.mPixelFormat = region_attr.unAttr.stOverlay.mPixelFmt;
     stBmp.mWidth = region_attr.unAttr.stOverlay.mSize.Width;
     stBmp.mHeight = region_attr.unAttr.stOverlay.mSize.Height;
     stBmp.mpData = m_logoRgb.pic_addr;
     m_tRecorder.m_recorder[val]->setRegionBitmap(mVencOverlayHandle, &stBmp);
     RGN_CHN_ATTR_S stRgnChnAttr = {0};
     stRgnChnAttr.bShow = TRUE;
     stRgnChnAttr.enType = OVERLAY_RGN;
     if(val == 0){
        setlogoOsdRect();
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {m_logoRect.nWidth - stBmp.mWidth -64 ,32};
     }else{
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {1920 - stBmp.mWidth -64,32};
     }
     stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
     ret = m_tRecorder.m_recorder[val]->attachRegionToVenc(mVencOverlayHandle, &stRgnChnAttr);
     if(ret == UNKNOWN_ERROR || ret < 0)
     {
         db_error("record %d attachRegionToVenc failed");
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     m_RegionId.insert(make_pair(region_id,mVencOverlayHandle));
	 mLogoOsdRgnId = region_id;
     pthread_mutex_unlock(&m_osdLock);
	 return 0;
}

// 日期时间水印
int OsdManager::AttchVencRegion(int val,int region_id)
{
    pthread_mutex_lock(&m_osdLock);
#ifdef USE_AW_OSDPATCH
	deleteTimeOsdRgb();
#endif	
    createTimeOsdRgb();
    RGN_ATTR_S region_attr;
    int ret;
    //for(int i =0; i< m_EncChnNum; i++){
     memset(&region_attr, 0, sizeof(region_attr));
     region_attr.enType = OVERLAY_RGN;
     region_attr.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
     region_attr.unAttr.stOverlay.mSize = {m_tTimeRgb.wide, m_tTimeRgb.high};
     mVencOverlayHandle = m_tRecorder.m_recorder[val]->createRegion(&region_attr);
     if(mVencOverlayHandle < 0)
     {
         db_error("createRegion falied");
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     BITMAP_S stBmp;
     memset(&stBmp, 0, sizeof(BITMAP_S));
     stBmp.mPixelFormat = region_attr.unAttr.stOverlay.mPixelFmt;
     stBmp.mWidth = region_attr.unAttr.stOverlay.mSize.Width;
     stBmp.mHeight = region_attr.unAttr.stOverlay.mSize.Height;
     stBmp.mpData = m_tTimeRgb.pic_addr;
     m_tRecorder.m_recorder[val]->setRegionBitmap(mVencOverlayHandle, &stBmp);
     RGN_CHN_ATTR_S stRgnChnAttr = {0};
     stRgnChnAttr.bShow = TRUE;
     stRgnChnAttr.enType = OVERLAY_RGN;
     stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {m_tTimeOsd.rcTime.nLeft, m_tTimeOsd.rcTime.nTop};
     stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
     ret = m_tRecorder.m_recorder[val]->attachRegionToVenc(mVencOverlayHandle, &stRgnChnAttr);
     if(ret == UNKNOWN_ERROR || ret < 0)
     {
         db_error("record %d attachRegionToVenc failed");
		 #ifndef USE_AW_OSDPATCH
		 deleteTimeOsdRgb();
		 #endif
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     m_RegionId.insert(make_pair(region_id,mVencOverlayHandle));
	 mTimeOsdRgnId = region_id;
	 #ifndef USE_AW_OSDPATCH
     deleteTimeOsdRgb();
	 #endif
    #ifdef ISP_DEBUG
    if(val == 0)
   {
        createispdebugRgb();
        RGN_ATTR_S isp_region_attr;
        memset(&isp_region_attr, 0, sizeof(isp_region_attr));
        isp_region_attr.enType = OVERLAY_RGN;
        isp_region_attr.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
        isp_region_attr.unAttr.stOverlay.mSize = {m_ispDebugRgb.wide, m_ispDebugRgb.high};
        printf("[debug_jaosn]: m_ispDebugRgb.wide = %d,m_ispDebugRgb.high = %d",m_ispDebugRgb.wide,m_ispDebugRgb.high);
        mVencOverlayHandle_isp = m_tRecorder.m_recorder[val]->createRegion(&isp_region_attr);
        if(mVencOverlayHandle_isp < 0)
        {
             db_error("createRegion falied");
             pthread_mutex_unlock(&m_osdLock);
             return -1;
        }
         BITMAP_S stBmp_isp;
         memset(&stBmp_isp, 0, sizeof(BITMAP_S));
         stBmp_isp.mPixelFormat = isp_region_attr.unAttr.stOverlay.mPixelFmt;
         stBmp_isp.mWidth = isp_region_attr.unAttr.stOverlay.mSize.Width;
         stBmp_isp.mHeight = isp_region_attr.unAttr.stOverlay.mSize.Height;
          printf("[debug_jaosn]: stBmp_isp.mWidth = %d,stBmp_isp.mHeight = %d",stBmp_isp.mWidth,stBmp_isp.mHeight);
         stBmp_isp.mpData = m_ispDebugRgb.pic_addr;
         m_tRecorder.m_recorder[val]->setRegionBitmap(mVencOverlayHandle_isp, &stBmp_isp);

        RGN_CHN_ATTR_S stRgnChnAttr_isp = {0};
        stRgnChnAttr_isp.bShow = TRUE;
        stRgnChnAttr_isp.enType = OVERLAY_RGN;
        stRgnChnAttr_isp.unChnAttr.stOverlayChn.stPoint = {0, 2000};
        stRgnChnAttr_isp.unChnAttr.stOverlayChn.mLayer = 0;
        ret = m_tRecorder.m_recorder[val]->attachRegionToVenc(mVencOverlayHandle_isp, &stRgnChnAttr_isp);
        if(ret == UNKNOWN_ERROR || ret < 0)
        {
            db_error("record %d attachRegionToVenc failed");
			deleteispdebugRgb();
            pthread_mutex_unlock(&m_osdLock);
            return -1;
        }
        m_RegionId.insert(make_pair(4,mVencOverlayHandle_isp));
		deleteispdebugRgb();
    }
    #endif
    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

// 车牌水印
int OsdManager::AttchCarIdRegion(int val,int region_id)
{
    pthread_mutex_lock(&m_osdLock);
	#ifdef USE_AW_OSDPATCH
	deleteCarIdOsdRgb();
	#endif
    createCarIdOsdRgb();
    RGN_ATTR_S region_attr;
    int ret;
    //for(int i =0; i< m_EncChnNum; i++){
     memset(&region_attr, 0, sizeof(region_attr));
     region_attr.enType = OVERLAY_RGN;
     region_attr.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
     region_attr.unAttr.stOverlay.mSize = {m_tCarIdRgb.wide, m_tCarIdRgb.high};
     mVencOverlayHandle = m_tRecorder.m_recorder[val]->createRegion(&region_attr);
     if(mVencOverlayHandle < 0)
     {
         db_error("createRegion falied");
         pthread_mutex_unlock(&m_osdLock);
         deleteCarIdOsdRgb();
         return -1;
     }
     BITMAP_S stBmp;
     memset(&stBmp, 0, sizeof(BITMAP_S));
     stBmp.mPixelFormat = region_attr.unAttr.stOverlay.mPixelFmt;
     stBmp.mWidth = region_attr.unAttr.stOverlay.mSize.Width;
     stBmp.mHeight = region_attr.unAttr.stOverlay.mSize.Height;
     stBmp.mpData = m_tCarIdRgb.pic_addr;
     m_tRecorder.m_recorder[val]->setRegionBitmap(mVencOverlayHandle, &stBmp);
     RGN_CHN_ATTR_S stRgnChnAttr = {0};
     stRgnChnAttr.bShow = TRUE;
     stRgnChnAttr.enType = OVERLAY_RGN;
	 //m_tDevNameOsd.rcDevName.nLeft =64;
	 //m_tDevNameOsd.rcDevName.nTop = 64;
	 // 获取字符串长度

	 //calc_osd_width();
	 
	 int lx = m_tDevNameOsd.rcDevName.nLeft - m_tCarIdRgb.wide;
	 
     stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {lx/*m_tDevNameOsd.rcDevName.nLeft*/, m_tDevNameOsd.rcDevName.nTop};
     stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
     ret = m_tRecorder.m_recorder[val]->attachRegionToVenc(mVencOverlayHandle, &stRgnChnAttr);
     if(ret == UNKNOWN_ERROR || ret < 0)
     {
         db_error("record %d attachRegionToVenc failed");
		 #ifndef USE_AW_OSDPATCH
		 deleteCarIdOsdRgb();
		 #endif
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     m_RegionId.insert(make_pair(region_id,mVencOverlayHandle));
	 mCaridOsdRgnId = region_id;
	 #ifndef USE_AW_OSDPATCH
	 deleteCarIdOsdRgb();
	 #endif
    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

// GPS水印
int OsdManager::AttchGpsRegion(int val,int region_id)
{
    pthread_mutex_lock(&m_osdLock);
	#ifdef USE_AW_OSDPATCH
	deletegpsOsdRgb();
	#endif
    creategpsOsdRgb();
    RGN_ATTR_S region_attr;
    int ret;
    //for(int i =0; i< m_EncChnNum; i++){
     memset(&region_attr, 0, sizeof(region_attr));
     region_attr.enType = OVERLAY_RGN;
     region_attr.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
     region_attr.unAttr.stOverlay.mSize = {m_tgpsRgb.wide, m_tgpsRgb.high};
     mVencOverlayHandle = m_tRecorder.m_recorder[val]->createRegion(&region_attr);
	 //db_error("createRegion Gps mVencOverlayHandle: %x",mVencOverlayHandle);
     if(mVencOverlayHandle < 0)
     {
         db_error("createRegion falied");
         #ifdef USE_AW_OSDPATCH
	       deletegpsOsdRgb();
	     #endif
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     BITMAP_S stBmp;
     memset(&stBmp, 0, sizeof(BITMAP_S));
     stBmp.mPixelFormat = region_attr.unAttr.stOverlay.mPixelFmt;
     stBmp.mWidth = region_attr.unAttr.stOverlay.mSize.Width;
     stBmp.mHeight = region_attr.unAttr.stOverlay.mSize.Height;
     stBmp.mpData = m_tgpsRgb.pic_addr;
     m_tRecorder.m_recorder[val]->setRegionBitmap(mVencOverlayHandle, &stBmp);
     RGN_CHN_ATTR_S stRgnChnAttr = {0};
     stRgnChnAttr.bShow = TRUE;
     stRgnChnAttr.enType = OVERLAY_RGN;
	 //m_tgpsOsd.rcgps.nLeft = 320;
	 //m_tgpsOsd.rcgps.nTop = 320;
     stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {m_tgpsOsd.rcgps.nLeft, m_tgpsOsd.rcgps.nTop};
     stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
     ret = m_tRecorder.m_recorder[val]->attachRegionToVenc(mVencOverlayHandle, &stRgnChnAttr);
     if(ret == UNKNOWN_ERROR || ret < 0)
     {
         db_error("record %d attachRegionToVenc failed");
		 #ifndef USE_AW_OSDPATCH
		 deletegpsOsdRgb();
		 #endif
         pthread_mutex_unlock(&m_osdLock);
         return -1;
     }
     m_RegionId.insert(make_pair(region_id,mVencOverlayHandle));
	 
	 mGpsOsdRgnid = region_id;
	 #ifndef USE_AW_OSDPATCH
	 deletegpsOsdRgb();
	 #endif
    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::GetOsdBitMap(BITMAP_S &stBmp)
{
    pthread_mutex_lock(&m_osdLock);
    createPicTimeOsdRgb();
    stBmp.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    stBmp.mWidth = m_tPicTimeRgb.wide;
    stBmp.mHeight = m_tPicTimeRgb.high;
    stBmp.mpData = m_tPicTimeRgb.pic_addr;
    pthread_mutex_unlock(&m_osdLock);

    return 0;
}

int OsdManager::DettchVencRegion(int val,int region_id)
{
#ifdef USE_AW_OSDPATCH
	if(region_id == 0 || region_id == 2 ){		// 0 cam_a logo 2 cam_b logo
		RegionID::iterator it;
		it = m_RegionId.find(region_id);
		m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
		m_tRecorder.m_recorder[val]->destroyRegion(it->second);
		deleteTimeOsdRgb();
        return 0;
	}
	if(region_id == 5 || region_id == 6){		// 5 cam_a timeosd 6 cam_b timeosd
		RegionID::iterator it;
		it = m_RegionId.find(region_id);
		m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
		m_tRecorder.m_recorder[val]->destroyRegion(it->second);
		deleteTimeOsdRgb();
	}
#else
    RegionID::iterator it;
    it = m_RegionId.find(region_id);
	if (it !=m_RegionId.end()) {
    	m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
		m_tRecorder.m_recorder[val]->destroyRegion(it->second);
		m_RegionId.erase(it);
	}
#endif
    #ifdef ISP_DEBUG
    if(val == 0)
    {
        it = m_RegionId.find(4);
        m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
    }
    #endif
#ifdef USE_AW_OSDPATCH	
    m_RegionId.clear();
#endif
    return 0;
}
#if 0
int OsdManager::DettchCarIdRegion(int val,int region_id)
{
    RegionID::iterator it;
    it = m_RegionId.find(region_id);
    m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
    #ifdef ISP_DEBUG
    if(val == 0)
    {
        it = m_RegionId.find(4);
        m_tRecorder.m_recorder[val]->detachRegionFromVenc(it->second);
    }
    #endif
    m_RegionId.clear();
    return 0;
}
#endif

int OsdManager::updateispDebugOsd()
{
    BITMAP_S stBmp;
    RegionID::iterator it;
    memset(&stBmp, 0, sizeof(BITMAP_S));
    stBmp.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    stBmp.mWidth = m_ispDebugRgb.wide;
    stBmp.mHeight = m_ispDebugRgb.high;
    stBmp.mpData = m_ispDebugRgb.pic_addr;
    it = m_RegionId.find(4);
    m_tRecorder.m_recorder[0]->setRegionBitmap(it->second, &stBmp);
    return 0;
}



int OsdManager::updateTimeOsd()
{
    BITMAP_S stBmp;
    RegionID::iterator it;

    for(int i =0; i< m_EncChnNum; i++)
    {
        if( m_tRecorder.m_arrEnable[i] == false)
        {
            continue;
        }

        memset(&stBmp, 0, sizeof(BITMAP_S));
        stBmp.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
        stBmp.mWidth = m_tTimeRgb.wide;
        stBmp.mHeight = m_tTimeRgb.high;
        stBmp.mpData = m_tTimeRgb.pic_addr;
        it = m_RegionId.find(mTimeOsdRgnId);
        //db_error("i : %d updateTimeOsd the m_EncChnNum = %d  %d  %d mTimeOsdRgnId:%d",i, m_EncChnNum,it->first,it->second,mTimeOsdRgnId);
        if(it != m_RegionId.end()){
        m_tRecorder.m_recorder[i]->setRegionBitmap(it->second, &stBmp);
        }
    }

    return 0;
}

int OsdManager::startTimeOsd(int p_nCamID,int p_RecordId)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nCamID <0 || p_RecordId < 0 )
    {
        pthread_mutex_unlock(&m_osdLock);
        return 0;
    }

    bool tmp_found = false;
    int i=0;

    for(i =0; i<MAX_RECODER_NUM;i++)
    {
        if( m_tRecorder.m_arrCamID[i] == p_nCamID )
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    tmp_found = false;
    for( ; i< MAX_RECODER_NUM; i++)
    {
        if( m_tRecorder.m_arrRecordId[i] == p_RecordId)
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    m_tRecorder.m_arrEnable[i] = true;

    pthread_mutex_unlock(&m_osdLock);
    
    enableTimeOsd();
    #ifdef OSDTHRREAD_USE_TIMER
    set_period_timer(1, 0, osd_timer_id_);
    #endif
    return 0;
}

int OsdManager::stopTimeOsd(int p_nCamID,int p_RecordId)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nCamID <0 || p_RecordId < 0 )
    {
        db_warn("invalid input parameter");
        pthread_mutex_unlock(&m_osdLock);
        return 0;
    }

    bool tmp_found = false;
    int i=0;
    for(i =0; i<MAX_RECODER_NUM;i++)
    {
        if( m_tRecorder.m_arrCamID[i] == p_nCamID )
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    tmp_found = false;
    for( i =0; i< MAX_RECODER_NUM; i++)
    {
        if( m_tRecorder.m_arrRecordId[i] == p_RecordId)
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

	if( i < MAX_RECODER_NUM)
	    m_tRecorder.m_arrEnable[i] = false;
    
    pthread_mutex_unlock(&m_osdLock);
    disableTimeOsd();
    #ifdef OSDTHRREAD_USE_TIMER
    stop_timer(osd_timer_id_);
    #endif
    return 0;
}

int OsdManager::enableTimeOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tTimeOsd.nEnable = 1;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::disableTimeOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tTimeOsd.nEnable = 0;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}


int OsdManager::setCaridOsdPosition(int p_nLeft, int p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if(p_nLeft < 0 || p_nTop < 0 )
    {
        db_error("input parameter is invalid left %d top %d",p_nLeft, p_nTop);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( m_tDevNameOsd.rcDevName.nLeft != p_nLeft || m_tDevNameOsd.rcDevName.nTop != p_nTop )
    {
		p_nLeft = p_nLeft - p_nLeft % 16;
		m_tDevNameOsd.rcDevName.nLeft = p_nLeft;
		p_nTop = p_nTop - p_nTop % 16;
        m_tDevNameOsd.rcDevName.nTop = p_nTop;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setTimeOsdPostion(int p_nLeft, int p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if(p_nLeft < 0 || p_nTop < 0 )
    {
        db_error("input parameter is invalid left %d top %d",p_nLeft, p_nTop);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( m_tTimeOsd.rcTime.nLeft != p_nLeft || m_tTimeOsd.rcTime.nTop != p_nTop )
    {
		p_nLeft = p_nLeft - p_nLeft % 16;
		m_tTimeOsd.rcTime.nLeft = p_nLeft;
		p_nTop = p_nTop - p_nTop % 16;
        m_tTimeOsd.rcTime.nTop = p_nTop;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getTimeOsdPostion(int &p_nLeft, int &p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    p_nLeft = m_tTimeOsd.rcTime.nLeft;
    p_nTop = m_tTimeOsd.rcTime.nTop;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setTimeOsdFormat(int p_nDateFmt, int p_nTimeFmt)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( m_tTimeOsd.nTimefmt != p_nTimeFmt || m_tTimeOsd.nDatefmt != p_nDateFmt )
    {
        m_tTimeOsd.nDatefmt = p_nDateFmt;
        m_tTimeOsd.nTimefmt = p_nTimeFmt;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getTimeOsdFormat(int &p_nDateFmt, int &p_nTimeFmt)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    p_nDateFmt = m_tTimeOsd.nDatefmt;
    p_nTimeFmt = m_tTimeOsd.nTimefmt;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getTimeOsdStatus(int &p_nStatus)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    p_nStatus = m_tTimeOsd.nEnable;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::disableChannelNameOsd(int p_nChannel)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    m_tChnNameOsd[p_nChannel].nEnable = 0;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::enableChannelNameOsd(int p_nChannel)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    m_tChnNameOsd[p_nChannel].nEnable = 1;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setChannelNameOsdPosition(int p_nChannel, int p_nLeft, int p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if(p_nLeft < 0 || p_nTop < 0 )
    {
        db_error("input parameter is invalid left %d top %d",p_nLeft, p_nTop);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( m_tChnNameOsd[p_nChannel].rcChnName.nLeft != p_nLeft || m_tChnNameOsd[p_nChannel].rcChnName.nTop != p_nTop )
    {
        m_tChnNameOsd[p_nChannel].rcChnName.nLeft = p_nLeft;
        m_tChnNameOsd[p_nChannel].rcChnName.nTop = p_nTop;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getChannelNameOsdPosition(int p_nChannel, int &p_nLeft, int &p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    p_nLeft = m_tChnNameOsd[p_nChannel].rcChnName.nLeft;
    p_nTop = m_tChnNameOsd[p_nChannel].rcChnName.nTop;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setChannelNameOsdString(int p_nChannel, const char *p_chnName)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( strcmp(m_tChnNameOsd[p_nChannel].szChnName, p_chnName))
    {
        strncpy(m_tChnNameOsd[p_nChannel].szChnName, p_chnName, sizeof(m_tChnNameOsd[p_nChannel].szChnName)-1);
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getChannelNameOsdString(int p_nChannel, char *p_chnName)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    strncpy(p_chnName, m_tChnNameOsd[p_nChannel].szChnName, sizeof(m_tChnNameOsd[p_nChannel].szChnName)-1);

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getChannelNameOsdStatus(int p_nChannel, int &p_nStatus)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nChannel < 0 || p_nChannel >= MAX_CHH_NUM )
    {
        db_error("channel %d is invalid ",p_nChannel);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    p_nStatus = m_tChnNameOsd[p_nChannel].nEnable;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::disableDeviceNameOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tDevNameOsd.nEnable = 0;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::enableDeviceNameOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tDevNameOsd.nEnable = 1;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setDeviceNameOsdPosition(int p_nLeft, int p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if(p_nLeft < 0 || p_nTop < 0 )
    {
        db_error("input parameter is invalid left %d top %d",p_nLeft, p_nTop);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if(m_tDevNameOsd.rcDevName.nLeft != p_nLeft || m_tDevNameOsd.rcDevName.nTop != p_nTop)
    {
        m_tDevNameOsd.rcDevName.nLeft = p_nLeft;
        m_tDevNameOsd.rcDevName.nTop = p_nTop;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}
int OsdManager::getDeviceNameOsdPosition(int &p_nLeft, int &p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    p_nLeft = m_tDevNameOsd.rcDevName.nLeft;
    p_nTop = m_tDevNameOsd.rcDevName.nTop;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::setDeviceNameOsdString(const char *p_devname)
{
    pthread_mutex_lock(&m_osdLock);

    if( strcmp(m_tDevNameOsd.szDevName, p_devname))
    {
        strncpy(m_tDevNameOsd.szDevName, p_devname, sizeof(m_tDevNameOsd.szDevName));
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}
int OsdManager::getDeviceNameOsdString(char *p_devName)
{
    pthread_mutex_lock(&m_osdLock);

    strncpy(p_devName, m_tDevNameOsd.szDevName, sizeof(m_tDevNameOsd.szDevName));

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::getDeviceNameOsdStatus(int &p_nStatus)
{
    pthread_mutex_lock(&m_osdLock);

    p_nStatus = m_tDevNameOsd.nEnable;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

std::map<std::string, unsigned int> car_id_map_ = 
{
	{"京",0xbea9},
	{"湘",0xcfe6},
	{"津",0xbdf2},
	{"鄂",0xb6f5},
	{"沪",0xbba6},
	{"粤",0xd4c1},
	{"渝",0xd3e5},
	{"琼",0xc7ed},
	{"冀",0xbcbd},
	{"川",0xb4a8},
	{"晋",0xbdfa},
	{"贵",0xb9f3},
	{"辽",0xc1c9},
	{"云",0xd4c6},
	{"吉",0xbcaa},
	{"陕",0xc9c2},
	{"黑",0xbada},
	{"甘",0xb8ca},
	{"苏",0xcbd5},
	{"青",0xc7e0},
	{"浙",0xd5e3},
	{"台",0xcca8},
	{"皖",0xcdee},
	{"藏",0xb2d8},
	{"闽",0xc3f6},
	{"蒙",0xc3c9},
	{"赣",0xb8d3},
	{"桂",0xb9f0},
	{"鲁",0xc2b3},
	{"宁",0xc4fe},
	{"豫",0xd4a5},
	{"新",0xd0c2},
	{"港",0xb8db},
	{"澳",0xb0c4},	
	{"全",0xc8ab},
};
/* 注意: 最多转16个字符 */
std::string Utf8ToGbk(const std::string& strUtf8)
{
	if (strUtf8.empty()) return "";
	const char *p = strUtf8.c_str();
	
	char buff[256];
	#if 0
	for (int k=0; k<strlen(strUtf8.c_str()); k++) {
		sprintf(buff+k*3,"%02x ",*p);
		p++;
	}
	db_error("UTF in: %s",buff);	// 粤123ABC7  [e7 b2 a4 31 32 33 41 42 43 37]
	p = strUtf8.c_str();
	memset(buff,0,sizeof(buff));
	#endif
	int i;
	unsigned result = 0;
	char strtmp[8];
	char *pr = buff;
	std::map<std::string, unsigned int>::iterator it;
	for (i = 0; i<16; i++){
		if (0x80 & *p) {
			memcpy(strtmp,p,3);
			strtmp[3] = 0;
			std::string ss = string(strtmp);
			result = 0xa1a1;
			it = car_id_map_.find(ss);
			if(it != car_id_map_.end()) {
				result = it->second;
			}
			//db_error("result (%s): %x",ss.c_str(),result);
			p+=3;
			*(pr++) = (char)(result>>8);
			*(pr++) = (char)(result>>0);
			
		} else {
			result = *p;
			*(pr++) = (char)(result);
			p++;
		}	
	}
	//db_error("GBK out: %s",buff);	// 3ABC7
	#if 0
	char buff2[256];
	p = buff;
	for (int l=0; l<strlen(buff); l++) {
		sprintf(buff2+l*3,"%02x ",*p);
		p++;
	}
	db_error("GB2312 OUT: %s",buff2);	// d4 c1 31 32 33 41 42 43 37 
	#endif
	return string(buff);
}

int OsdManager::createCarIdOsdRgb()
{
    char cCarid[MAX_CHAR_LEN];
    memset(cCarid, 0, sizeof(cCarid));
#if 1
	string devname = "粤123ABC7";
	//snprintf(cCarid, sizeof(cCarid)-1, "%s", devname.c_str());

	MenuConfigLua::GetInstance()->GetDeviceCaiId(devname);
	//db_error("DeviceCaiId utf8: %s", devname.c_str());
	// need to convert UTF8 to GB2312
	std::string devname_gb2312 = Utf8ToGbk(devname);
	//devname_gb2312 = "A123ABC7";
	snprintf(cCarid, sizeof(cCarid)-1, "%s", devname_gb2312.c_str());
	//db_error("DeviceCaiId gb2312: %s", cCarid);
#else
	string devname = "A1234567";
	//snprintf(cCarid, sizeof(cCarid)-1, "%s", devname.c_str());

	MenuConfigLua::GetInstance()->GetDeviceCaiId(devname);
	db_error("DeviceCaiId : %s", devname.c_str());
	// need to convert UTF8 to GB2312
	std::string devname_gb2312 = Utf8ToGbk(devname);
	snprintf(cCarid, sizeof(cCarid)-1, "%s", devname_gb2312.c_str());
#endif
    FONT_RGBPIC_S font_pic;
    font_pic.font_type     = m_FontSize;	
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x88;
    font_pic.background[1] = 0x88;
    font_pic.background[2] = 0x88;
    font_pic.background[3] = 0x33;

    m_tCarIdRgb.enable_mosaic = 0;
    m_tCarIdRgb.rgb_type      = OSD_RGB_32;
#if 0
	char buff[256];
	char *p = cCarid;
	for (int k=0; k<strlen(cCarid); k++) {
		sprintf(buff+k*3,"%02x ",*p);
		p++;
	}
	db_error("cCarid: %s",buff);
#endif	
    create_font_rectangle(cCarid, &font_pic, &m_tCarIdRgb);	// -> system\public\rgb_ctrl\rgb_ctrl.c
	
    return 0;
}

int OsdManager::updateCarIdOsd()
{
    BITMAP_S stBmp;
    RegionID::iterator it;

    for(int i =0; i< m_EncChnNum; i++)
    {
        if( m_tRecorder.m_arrEnable[i] == false)
        {
            continue;
        }

        memset(&stBmp, 0, sizeof(BITMAP_S));
        stBmp.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
        stBmp.mWidth = m_tCarIdRgb.wide;
        stBmp.mHeight = m_tCarIdRgb.high;
        stBmp.mpData = m_tCarIdRgb.pic_addr;
        it = m_RegionId.find(mCaridOsdRgnId);
        //db_msg("updateTimeOsd the m_EncChnNum = %d  %d  %d",m_EncChnNum,it->first,it->second);
        if(it != m_RegionId.end()){
        m_tRecorder.m_recorder[i]->setRegionBitmap(it->second, &stBmp);
        }
    }

    return 0;
}
int OsdManager::deleteCarIdOsdRgb()
{
    if (release_rgb_picture(&m_tCarIdRgb))
    {
        db_error("Release CarId rgb error!\n");
        return -1;
    }
    #if 0
    if (release_rgb_picture(&m_logoRgb))
    {
        db_error("Release time rgb error!\n");
        return -1;
    }
    #endif
    return 0;
}

int OsdManager::creategpsOsdRgb()
{
    char cgps[64];
	int speedx;
    memset(cgps, 0, sizeof(cgps));
	
	LocationInfo_t LocationInfo;
	
	int gpssw = MenuConfigLua::GetInstance()->GetGpsSwith();
	//if (!gpssw) return -2;
	int GpsSignal = EventManager::GetInstance()->GetGpsSignalLevel();
	//if (!GpsSignal) return -1;
	EventManager::GetInstance()->getLocationInfo(LocationInfo);
	float speed = LocationInfo.speed;	// knot (海里/小时)
	//speed = 999.0;
	int unit = MenuConfigLua::GetInstance()->GetSpeedunit();
	if (!unit) {
		speed = speed * 1.852;				// km/h
		speedx = (int) (speed + 0.5);
		if (speedx>999) speedx = 999; 
		snprintf(cgps, sizeof(cgps)-1, "%dKM/H", speedx);
	} else {
		speed = speed * 1.150779448;		// mile/h
		speedx = (int) (speed + 0.5);
		if (speedx>999) speedx = 999; 
		snprintf(cgps, sizeof(cgps)-1, "%dMPH", speedx);
	}
	if (!gpssw || !GpsSignal) {
		snprintf(cgps, sizeof(cgps)-1, "       ");	// 999km/h
		//                              999km/h
	}
	
    FONT_RGBPIC_S font_pic;
    font_pic.font_type     = m_FontSize;	
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x88;
    font_pic.background[1] = 0x88;
    font_pic.background[2] = 0x88;
    font_pic.background[3] = 0x33;

    m_tgpsRgb.enable_mosaic = 0;
    m_tgpsRgb.rgb_type      = OSD_RGB_32;
    create_font_rectangle(cgps, &font_pic, &m_tgpsRgb);	// -> system\public\rgb_ctrl\rgb_ctrl.c

    return 0;
}

int OsdManager::updategpsOsd()
{
    BITMAP_S stBmp;
    RegionID::iterator it;

    for(int i =0; i< m_EncChnNum; i++)
    {
        if( m_tRecorder.m_arrEnable[i] == false)
        {
            continue;
        }

        memset(&stBmp, 0, sizeof(BITMAP_S));
        stBmp.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
        stBmp.mWidth = m_tgpsRgb.wide;
        stBmp.mHeight = m_tgpsRgb.high;
        stBmp.mpData = m_tgpsRgb.pic_addr;
        it = m_RegionId.find(mGpsOsdRgnid);
        //db_error("updategpsOsd the m_EncChnNum = %d  %d  %d",m_EncChnNum,it->first,it->second);
        if(it != m_RegionId.end()){
        m_tRecorder.m_recorder[i]->setRegionBitmap(it->second, &stBmp);
        }
    }

    return 0;
}
int OsdManager::deletegpsOsdRgb()
{
    if (release_rgb_picture(&m_tgpsRgb))
    {
        db_error("Release gps rgb error!\n");
        return -1;
    }
    #if 0
    if (release_rgb_picture(&m_logoRgb))
    {
        db_error("Release time rgb error!\n");
        return -1;
    }
    #endif
    return 0;
}

int OsdManager::startGpsOsd(int p_nCamID,int p_RecordId)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nCamID <0 || p_RecordId < 0 )
    {
        pthread_mutex_unlock(&m_osdLock);
        return 0;
    }

    bool tmp_found = false;
    int i=0;

    for(i =0; i<MAX_RECODER_NUM;i++)
    {
        if( m_tRecorder.m_arrCamID[i] == p_nCamID )
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    tmp_found = false;
    for( ; i< MAX_RECODER_NUM; i++)
    {
        if( m_tRecorder.m_arrRecordId[i] == p_RecordId)
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    m_tRecorder.m_arrEnable[i] = true;

    pthread_mutex_unlock(&m_osdLock);

    enableGpsOsd();

    return 0;
}

int OsdManager::stopGpsOsd(int p_nCamID,int p_RecordId)
{
    pthread_mutex_lock(&m_osdLock);

    if( p_nCamID <0 || p_RecordId < 0 )
    {
        db_warn("invalid input parameter");
        pthread_mutex_unlock(&m_osdLock);
        return 0;
    }

    bool tmp_found = false;
    int i=0;
    for(i =0; i<MAX_RECODER_NUM;i++)
    {
        if( m_tRecorder.m_arrCamID[i] == p_nCamID )
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    tmp_found = false;
    for( i =0; i< MAX_RECODER_NUM; i++)
    {
        if( m_tRecorder.m_arrRecordId[i] == p_RecordId)
        {
            tmp_found = true;
            break;
        }
    }

    if( tmp_found == false)
    {
        db_msg("camID %d not found",p_nCamID);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

	if( i < MAX_RECODER_NUM)
	    m_tRecorder.m_arrEnable[i] = false;
    
    pthread_mutex_unlock(&m_osdLock);
    disableGpsOsd();

    return 0;
}

int OsdManager::enableGpsOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tgpsOsd.nEnable = 1;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}

int OsdManager::disableGpsOsd()
{
    pthread_mutex_lock(&m_osdLock);

    m_tgpsOsd.nEnable = 0;

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}


int OsdManager::setGpsOsdPosition(int p_nLeft, int p_nTop)
{
    pthread_mutex_lock(&m_osdLock);

    if( false == m_bTimeOsdInit )
    {
        db_error("osd not init yet");
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if(p_nLeft < 0 || p_nTop < 0 )
    {
        db_error("input parameter is invalid left %d top %d",p_nLeft, p_nTop);
        pthread_mutex_unlock(&m_osdLock);
        return -1;
    }

    if( m_tgpsOsd.rcgps.nLeft != p_nLeft || m_tgpsOsd.rcgps.nTop != p_nTop )
    {
		p_nLeft = p_nLeft - p_nLeft % 16;
		m_tgpsOsd.rcgps.nLeft = p_nLeft;
		p_nTop = p_nTop - p_nTop % 16;
        m_tgpsOsd.rcgps.nTop = p_nTop;
    }

    pthread_mutex_unlock(&m_osdLock);
    return 0;
}
