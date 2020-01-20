/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file httpServer.h
 * @brief httpServer
 * @author id:690
 * @version v0.9
 * @date 2019-07-04
 */
#include "common/singleton.h"
#include "common/subject.h"

#include "webserver/webserver.h"
#include "common/singleton.h"
#include "resource/resource_manager.h"


//#include "AVIOCTRLDEFs.h"
#include <cstring>
#include <unistd.h>
#include <signal.h>


//ADD FOR V536 CDR
typedef enum{
    IOTYPE_CDR_SETVIDEORESOULATION = 1001,
    IOTYPE_CDR_SETLOOPRECORD_TIME  = 1002,
    IOTYPE_CDR_SETVIDEORECORD_TYPE = 1003,
    IOTYPE_CDR_SETCIDEOEXPROTION_VALUE = 1004,
    IOTYPE_CDR_SETAWD_VALUE         = 1005,
    TOTYPE_CDR_SETGSENOR_SENVITY_VAUE = 1006,
    IOTYPE_CDR_AUTOSCREENSOFF_VALUE   = 1007,
    IOTYPE_CDR_SETLANGUAGE_VALUE      = 1008,
    IOTYPE_CDR_SETRECORDSOUND_VALUE   = 1009,    
    IOTYPE_CDR_SETRECORDSOUNDLEVEL_VALUE   = 1010,
    IOTYPE_CDR_SETMOTIONDETECT_VALUE   = 1011,
    IOTYPE_CDR_SETLIGHTFRQUENCR_VALUE  = 1012,
    IOTYPE_CDR_FORMATSDCARD_VALUE      = 1013,
    IOTYPE_CDR_FECTORYRESET_VALUE      = 1014,
    IOTYPE_CDR_SETDEVICETIME_VALUE     = 1015,
    
    IOTYPE_CDR_REMOTERECORD_SWITCH     = 1100,    
    IOTYPE_CDR_REMOTETAKEPHOTO     = 1101,
    IOTYPE_CDR_SET_WIFI_SSID       = 3003,
    IOTYPE_CDR_SET_WIFI_PASSPHRASE = 3004,
    IOTYPE_CDR_SET_DATE            = 3005,
    IOTYPE_CDR_SET_TIME            = 3006,
    IOTYPE_CDR_HEART_BEAT          = 3016,
    IOTYPE_CDR_GET_MENU_ITEM       = 3031,

    IOTYPE_CDR_DELET_ONE_FILE      = 4003,
    
    IOTYPE_CDR_APP_TO_DEVICE_HEART_BEAT          = 4016,

    IOTYPE_CDR_GETVERSION  = 2001,
    IOTYPE_CDR_GETDEVICEINFO = 2002,
    IOTYPE_CDR_GETRECORDING_TIME = 2003,
    IOTYPE_CDR_GETCDRCARD_STATUS = 2004,
    IOTYPE_CDR_GETRECORD_STATUS = 2005,

    IOTYE_CDR_RECORD_STATUS  = 5001,
    IOTYE_CDR_TFCARD_STATUS  = 5002,
    IOTYPE_CDR_CMD_MAX,
}CDR_AVCTRL_IO_CMD;


class DeviceAdapter;

#pragma once
namespace EyeseeLinux {

class httpServer
    : public Singleton<httpServer>    
    , public ISubjectWrap(httpServer)
{
    friend class Singleton<httpServer>;
    public:
    class DownloadHandler : public UrlHandler
    {
       public:
            DownloadHandler(httpServer    *m_httpserver){m_httpserver0 = m_httpserver;};
            ~DownloadHandler(){};
            bool handleGet(CivetServer *server, struct mg_connection *conn);
       private:
            httpServer *m_httpserver0;
    };

    class GetDeviceInfoHandler : public UrlHandler
    {
        public:
            GetDeviceInfoHandler(httpServer     *m_httpserver){m_httpserver1 = m_httpserver;};
            ~GetDeviceInfoHandler(){};
            bool handleGet(CivetServer *server, struct mg_connection *conn);
        private:
             httpServer *m_httpserver1;
          //  bool handlePost(CivetServer *server, struct mg_connection *conn);
    };

    class SetDeviceInfoHandler:public UrlHandler
    {
        public:
            SetDeviceInfoHandler(httpServer     *m_httpserver){m_httpserver2 = m_httpserver;};
            ~SetDeviceInfoHandler(){};
       //     bool handleGet(CivetServer *server, struct mg_connection *conn);
            bool handlePost(CivetServer *server, struct mg_connection *conn);
         private:
             httpServer *m_httpserver2;

    };
    
    httpServer();
    ~httpServer();
    int init();
    void start();
    void stop();
    int setAdapter(DeviceAdapter *adapter);
    
    static void *initSocketServer(void *context);
    static void *handleRecveMessage(void *context);
    int CreateSocket();    
    int sendCommdToServer(int cmd, int value,std::string str);    
    static void HeartBeatTimerProc(union sigval sigval);
    void startHeartBeatTimer();    
    int handSocketmassge(int cmd_id, int cmd_value);    
    int GetDeviceStatus(int cmd);
    int GetMenuitemCount(int cmd);
    int GetMenuItemString(int cmd,int index,char *buf);
    bool GetAppConnectedFlag();
    
    private:
        int HandleSetIotCtrlCmd(int io_type,int result_value,std::string str);
        int HandleGetIotCtrlCmd(int io_type,int result_value,char *buf,std::string &str);
        WebServer *m_httpServer;
        WebServer::UrlHandlerMap m_url_handlers;
        DeviceAdapter *mAdapterLayer;
        int listen_socket;
        int client_socket;
        bool isConnected;
        int ClientheartBeatCount;        
        int ServerheartBeatCount;
        timer_t heartbeat_timer_id_;
        R* Res;
        
};
}
