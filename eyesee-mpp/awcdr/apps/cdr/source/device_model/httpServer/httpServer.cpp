#include "device_model/httpServer/httpServer.h"
#include "common/app_log.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "bll_presenter/remote/interface/dev_ctrl_adapter.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <json/json.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include "common/thread.h"
#include "window/user_msg.h"



#undef LOG_TAG
#define LOG_TAG "httpServer"

using namespace EyeseeLinux;
using namespace std;

#define PORT_SOCKET 9999

httpServer::httpServer()
{
    db_error("coursurt httpserver\n");
    isConnected = false;
    listen_socket = -1;
    client_socket = -1;
    ClientheartBeatCount = 0;
    ServerheartBeatCount = 0;
    m_httpServer = WebServer::GetInstance();
    create_timer(this, &heartbeat_timer_id_,HeartBeatTimerProc);
    stop_timer(heartbeat_timer_id_);
    pthread_t socket_thread_id;
    ThreadCreate(&socket_thread_id, NULL, initSocketServer, this);
}

httpServer::~httpServer()
{
    db_msg("de consurt httpserver");
    stop_timer(heartbeat_timer_id_);
    delete_timer(heartbeat_timer_id_);
    //delete m_httpServer;
    //m_httpServer = NULL;
}

int httpServer::CreateSocket()
{
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        db_error("create socket error\n");
        return sockfd;
    }
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_SOCKET);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    if(ret == -1)
    {
        db_error("bind filed\n");
        close(sockfd);
        return ret;
    }

    ret = listen(sockfd,5);
    if(ret == -1)
    {
        db_error("listen error be carful\n");        
        close(sockfd);
        return ret;
    }
    return sockfd;
    
}

int httpServer::handSocketmassge(int cmd_id, int cmd_value)
{
    switch(cmd_id)
    {
       case IOTYPE_CDR_APP_TO_DEVICE_HEART_BEAT:
       {
            //启动定时器
            //赋值value
            ClientheartBeatCount ++;
       }
       break;
       default:
       {
           db_warn("handSocketmassge the cmd_id = %d",cmd_id);
           isConnected = false;
           ServerheartBeatCount = 0;
           ClientheartBeatCount = 0;
           stop_timer(heartbeat_timer_id_);
           Notify(MSG_APP_IS_DISCONNECTED);
       }
       break;
    }
    return 0;
}

bool httpServer::GetAppConnectedFlag()
{
    return isConnected;
}

void httpServer::HeartBeatTimerProc(union sigval sigval)
{
    httpServer *http = reinterpret_cast<httpServer*>(sigval.sival_ptr);
    http->ServerheartBeatCount ++;
    db_error("RecordingTimerProc time is now client = %d  server= %d \n",http->ClientheartBeatCount,http->ServerheartBeatCount);
    if((http->ServerheartBeatCount - http->ClientheartBeatCount)>= 1)
    {
        db_error("becareful cleant is disconnect\n");
        http->ServerheartBeatCount = 0;
        http->ClientheartBeatCount = 0;
        http->isConnected = false;
        stop_timer(http->heartbeat_timer_id_);
        http->Notify(MSG_APP_IS_DISCONNECTED);
    }else{
        
    }
}

void httpServer::startHeartBeatTimer()
{   
    db_error("startHeartBeatTimer\n");
    set_period_timer(30, 0, heartbeat_timer_id_);
}

void *httpServer::handleRecveMessage(void *context)
{
     httpServer *self = reinterpret_cast<httpServer*>(context);
     prctl(PR_SET_NAME, "handleRecveMessage", 0, 0, 0);
     char RecvBuf[1024] = {0};
     db_error("the client_socket is %d",self->client_socket);
     std::string cmd_id;
     int cmd_value = 0;
     while(true)
     {
        memset(&RecvBuf,'\0',sizeof(RecvBuf));
        int ret = recv(self->client_socket,RecvBuf,sizeof(RecvBuf)-1,0);
        if(ret < 0)
        {
             db_error("recive error\n");
        }
        Json::Reader reader;
        Json::Value root;
        if(reader.parse(RecvBuf, root))
        {
            cmd_id = root["cmd_id"].asString();
            
            //cmd_value = root["cmd_value"].asInt();
//            db_error("the cmd_value = %s  the cmd_value = %d",cmd_id.c_str(),cmd_value);
        }     
        self->handSocketmassge(atoi(cmd_id.c_str()),cmd_value);
        cmd_id = "";
//        db_msg("RecvBuf is %s\n",RecvBuf);
        sleep(1);
        if(self->isConnected != true)
        {
            db_error("the client is disconnected will exit this thread\n");
            break;
        }
     }
     close(self->client_socket);
     self->client_socket = -1;
}

int httpServer::sendCommdToServer(int cmd, int value,std::string str)
{
    char SendBuf[1024] = {0};
    std::string str1;
    db_error("sendCommdToServer cmd = %d, value = %d",cmd,value);
    if(isConnected)
    {
        switch(cmd)
        {
            case MSG_RECORD_START:
            case MSG_RECORD_STOP:
            {
                Json::Value item;
                item["event"] = IOTYE_CDR_RECORD_STATUS;
                item["status"] = value; //1:start 0:stop
                str1 = item.toStyledString();
                strncpy(SendBuf,str1.c_str(),strlen(str1.c_str()));
               // SendBuf = str1;
            }
            break;
            case MSG_STORAGE_MOUNTED:
            case MSG_STORAGE_UMOUNT:
            {
                Json::Value item;                
                item["event"] = IOTYE_CDR_TFCARD_STATUS;
                item["status"] = value;  //1:mounted 0:unmounted
                str1 = item.toStyledString();
                strncpy(SendBuf,str1.c_str(),strlen(str1.c_str()));
            }
            break;
        }
        db_error("sendCommdToServer: the send buffer is %s",SendBuf);
        int ret = send(client_socket,SendBuf,sizeof(SendBuf)-1,0);
    }else{
        db_error("snedCommdToServer is failed,the app is disconnected\n");
        return -1;
    }
    return 0;
}

void *httpServer::initSocketServer(void *context)
{
    httpServer *self = reinterpret_cast<httpServer*>(context);
    prctl(PR_SET_NAME, "initSocketServer", 0, 0, 0);
    self->listen_socket = self->CreateSocket();

    if(self->listen_socket == -1)
    {
        db_error("create socket server failed\n");
        close(self->listen_socket);
        return NULL;
    }
   while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        memset(&client_addr,0,sizeof(client_addr));
        db_error("initSocketServer wait client connect uu\n");
        self->client_socket = accept(self->listen_socket,(struct sockaddr *)&client_addr,&addrlen);
        if(self->client_socket == -1)
        {
            db_error("accept failed\n");
            close(self->client_socket);
            return NULL;
        }
      //  while(1){
        //一个客户端创建一个线程接受
        self->isConnected = true;
        self->ServerheartBeatCount = 0;
        self->ClientheartBeatCount = 0;
        self->startHeartBeatTimer();
        self->Notify(MSG_APP_IS_CONNECTED);
        //notify devices the app is connected
        pthread_t recive_thread_id;
        ThreadCreate(&recive_thread_id, NULL, handleRecveMessage, self);
        //write(self->client_socket,RecvBuf,ret);
      //  sleep(1);
      // }
   }
   close(self->listen_socket);
   close(self->client_socket);
   self->listen_socket = -1;
}

int httpServer::setAdapter(DeviceAdapter *adapter)
{
    mAdapterLayer = adapter;
    return 0;
}

void httpServer::start()
{
    db_error("http server is start\n");
    m_httpServer->Start();
}

void httpServer::stop()
{
    db_error("stop the http server\n");
    m_httpServer->Stop();
}

int httpServer::init()
{
    m_url_handlers.emplace("/api/setdeviceinfo", new httpServer::SetDeviceInfoHandler(this));
    m_url_handlers.emplace("/api/getdeviceinfo", new httpServer::GetDeviceInfoHandler(this));
    m_url_handlers.emplace("/tmp/sqlite/", new httpServer::DownloadHandler(this));
    m_url_handlers.emplace("/mnt/extsd/", new httpServer::DownloadHandler(this));
    m_httpServer->RegistUrlHandler(m_url_handlers);
    return 0;
}

bool httpServer::DownloadHandler::handleGet(CivetServer * server, struct mg_connection * conn)
{
    /* Handler may access the request info using mg_get_request_info */
    db_error("DownloadHandler::handleGet in\n");
    const struct mg_request_info *req_info = mg_get_request_info(conn);
  //  dumpURLinfo(conn);

    string filepath = req_info->local_uri;
    string basename;
    if(strstr(filepath.c_str(), "/mnt/extsd")!=NULL){
         string sd_name = "/mnt/extsd/";
         basename = filepath.substr(sd_name.length());
    }else{
         basename = filepath.substr(filepath.rfind("/") + 1);
    }

    string::size_type pos;
    pos = basename.rfind(".");
    if (pos == string::npos) {
        db_error("Error: File not found");
        mg_send_http_error(conn, 404, "%s", "Error: File not found");
        return true;
    }
    string suffix = basename.substr(pos);

    if (suffix == ".db") {
        mg_send_mime_file(conn, string("/tmp/sqlite/" + basename).c_str(), NULL);
    } else if (suffix == ".jpg" || suffix == ".mp4" || suffix == ".ts") {
        mg_send_mime_file(conn, string("/mnt/extsd/" + basename).c_str(), NULL);
    }

    db_error("filepath %s basename: %s, suffix: %s",
            filepath.c_str(),basename.c_str(), suffix.c_str());

    return true;

}

bool httpServer::SetDeviceInfoHandler::handlePost(CivetServer * server, struct mg_connection * conn)
{
  //  db_error("SetDeviceInfoHandler::handlePost in\n");
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    long long rlen;
    long long nlen = 0;
    long long tlen = req_info->content_length;
    char buf[1024] = {0};
    std::string str;
    std::string cmd_str;
    int pos;
    int ret = -1;

    db_debug("url: %s", req_info->local_uri);
    db_debug("query_string: %s", req_info->query_string);
  //  dumpURLinfo(conn);
    cmd_str = req_info->query_string;
    pos = cmd_str.find_last_of("&");
    std::string str1 = cmd_str.substr(13,pos);
    int cmd_value = atoi(str1.c_str());
//    db_error("handlePost pos = %d ; cmd_value = %d",pos,cmd_value);
    pos = cmd_str.find_last_of("=");
    std::string str2 = cmd_str.substr(pos+1);
    int par_value = atoi(str2.c_str());
  //  db_error("handlePost pos = %d ; cmd_value = %d",pos,par_value);
    ret = m_httpserver2->HandleSetIotCtrlCmd(cmd_value,par_value,str2);
    #if 0
    while (nlen < tlen) {
        rlen = tlen - nlen;
        if (rlen > sizeof(buf)) {
            rlen = sizeof(buf);
        }
        rlen = mg_read(conn, buf, (size_t)rlen);
        if (rlen <= 0) {
            break;
        }
        str = buf;
        db_error("read post data: %s", str.c_str());
        nlen += rlen;
    }
    Json::Reader reader;
    Json::Value root;
    if(reader.parse(buf, root))
    {
        std::string key_value = root["function"].asString();
        int value = root["value"].asInt();
        int io_type = root["id_cmd"].asInt();
        db_error("the key_value is %s  the root value = %d",key_value.c_str(),value);
        //can use a queue or map or vectot to store the value
        //m_httpserver2->HandleIotCtrlCmd(io_type,value,key_value.c_str());
        
    }else{
        db_error("Json reader parser is error\n");
    }
#endif
    mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: "
            "application/json\r\nConnection: close\r\n\r\n");
    
   
    Json::Value item;
    item["Cmd"] = cmd_value;
    item["Value"] = ret;
    std::string result = item.toStyledString();

    mg_printf(conn, result.c_str());

    return true; 
}

bool httpServer::GetDeviceInfoHandler::handleGet(CivetServer * server, struct mg_connection * conn)
{
    
//      db_error("GetDeviceInfoHandler::handleGet in\n");
      /* Handler may access the request info using mg_get_request_info */
      const struct mg_request_info *req_info = mg_get_request_info(conn);
      std::string str;
      std::string cmd_str;
      int pos;
      char buf[2048] = {0};

      db_debug("url: %s", req_info->local_uri);
      db_debug("query_string: %s", req_info->query_string);
      cmd_str = req_info->query_string;
      pos = cmd_str.find_last_of("=");
      std::string str2 = cmd_str.substr(pos+1);
      int cmd_value = atoi(str2.c_str());
   //   db_error("handlePost pos = %d ; cmd_value = %d",pos,cmd_value);
      m_httpserver1->HandleGetIotCtrlCmd(cmd_value, 0, buf,str);
//      db_error("handleGet: out is %s",str.c_str());
        
      mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: "
              "text/html\r\nConnection: close\r\n\r\n");
      #if 0
      mg_printf(conn, "<html><body>\n");
      mg_printf(conn, "<h2>You should send a PUT requst to upload a file!</h2>\n");
      mg_printf(conn,
              "<p>The request was:<br><pre>%s %s HTTP/%s</pre></p>\n",
              req_info->request_method,
              req_info->request_uri,
              req_info->http_version);
      mg_printf(conn, "</body></html>\n");
      #endif
      mg_printf(conn, str.c_str());

      return true;
}

int httpServer::GetDeviceStatus(int cmd)
{
    int ret = -1;
    AWDeviceInfo_ex dev_info;
    mAdapterLayer->dev_ctrl_->GetDeviceInfo_ex(dev_info);
    switch(cmd)
    {
        case IOTYPE_CDR_SETVIDEORESOULATION:
            ret = dev_info.menu_config_.resoultion_value;
        break;
        case IOTYPE_CDR_SETLOOPRECORD_TIME:
            ret = dev_info.menu_config_.loop_record_time;
        break;
        case IOTYPE_CDR_SETVIDEORECORD_TYPE:
            ret = dev_info.menu_config_.recod_type;
        break;
        case IOTYPE_CDR_SETCIDEOEXPROTION_VALUE:
            ret = dev_info.menu_config_.video_exp_value;
        break;
        case IOTYPE_CDR_SETAWD_VALUE:
            ret = dev_info.menu_config_.parking_value;
        break;
        case TOTYPE_CDR_SETGSENOR_SENVITY_VAUE:
            ret = dev_info.menu_config_.Gsensor_senstvity_value;
        break;
        case IOTYPE_CDR_AUTOSCREENSOFF_VALUE:
            ret = dev_info.menu_config_.screen_onoff_value;
        break;
        case IOTYPE_CDR_SETLANGUAGE_VALUE:
            ret =  dev_info.menu_config_.language_value;
        break;
        case IOTYPE_CDR_SETRECORDSOUND_VALUE:    
            ret =  dev_info.menu_config_.record_sound_value;
        break;
        case IOTYPE_CDR_SETRECORDSOUNDLEVEL_VALUE:    
            ret =  dev_info.menu_config_.record_sound_level_vaule;
        break;
        case IOTYPE_CDR_SETMOTIONDETECT_VALUE:    
            ret =  dev_info.menu_config_.moton_detect_value;;
        break;
        case IOTYPE_CDR_SETLIGHTFRQUENCR_VALUE:    
            ret =  dev_info.menu_config_.light_value;
        break;
    }
    return ret;
}

int httpServer::HandleGetIotCtrlCmd(int io_type,int result_value,char *buf,std::string &str)
{
//    db_error("HandleIotCtrlCmd : io_type = 0x%0x, result_value = %d, buf = %s",io_type,result_value,buf);
    switch(io_type)
    {
        case IOTYPE_CDR_GETVERSION:
        {
            
            db_error("IOTYPE_CDR_GETVERSION start");
            mAdapterLayer->dev_ctrl_->GetSystemVersion(buf);
         //   db_error("HandleGetIotCtrlCmd: the buf:%s",buf);
            Json::Value root;           
            root["Version"] = buf;
            str = root.toStyledString();
            db_error("HandleGetIotCtrlCmd: out is %s",str.c_str());
            
            db_error("IOTYPE_CDR_GETVERSION start");
	    }
        break;
        case IOTYPE_CDR_GETDEVICEINFO:
        {
            
            db_error("IOTYPE_CDR_GETDEVICEINFO start");
            AWDeviceInfo_ex dev_info;
            mAdapterLayer->dev_ctrl_->GetDeviceInfo_ex(dev_info);
            //change to json string            
            Json::Value root;
            Json::Value item;
            Json::Value arrayObj;
            root["device_id"] = dev_info.device_id;
            root["device_name"] = dev_info.device_name;
            root["software"] = dev_info.software;
            for(int i=1;i<=12;i++)
            {
                item["cmd"] = 1000+i;
                item["status"] = GetDeviceStatus(1000+i);
                arrayObj.append(item);    
            }
            root["info"] = arrayObj;
            str = root.toStyledString();            
            db_error("IOTYPE_CDR_GETDEVICEINFO end");
        }
        break;
        case IOTYPE_CDR_GETRECORDING_TIME:
        {
            int ret = 0;            
            Json::Value item;
            ret = mAdapterLayer->record_ctrl_->GetCurretRecordTime();
            item["RecodTime"] = ret;
            str = item.toStyledString(); 
        }
        break;
        case IOTYPE_CDR_GETCDRCARD_STATUS:
        {
            AWDiskInfo disk_info;            
            Json::Value item;
            mAdapterLayer->dev_ctrl_->GetDiskInfo(disk_info);
            item["disk_num"] = disk_info.disk_num;
            item["disk_id"]  = disk_info.disk_status[0].disk_id;
            item["disk_type"]= disk_info.disk_status[0].disk_type;
            item["capacity"] = disk_info.disk_status[0].capacity;
            item["free_space"] = disk_info.disk_status[0].free_space;
            item["disk_status"] = disk_info.disk_status[0].disk_status;
            str = item.toStyledString();
        }
        break;
        case IOTYPE_CDR_GET_MENU_ITEM:
        {
            
            db_error("IOTYPE_CDR_GET_MENU_ITEM start");
            Json::Value root_menu;            
            Json::Value item0;            
            Json::Value item1;
            Json::Value arrayObj0;            
            Json::Value arrayObj1;
            char buf[128];
            char headbuf[128];
            Res = R::get();
            int ret = 0;
            for(int i=1;i<=12;i++) //this may be 
            {
                memset(headbuf,0,sizeof(headbuf));
                item0["Cmd"] = 1000+i;
                GetMenuItemString(1000+i,-1,headbuf);                
                item0["Name"] = headbuf;                
                ret = GetMenuitemCount(1000+i);
               // db_error("get menu item count is %d",ret);
                arrayObj0.clear(); //clear all item string
                for(int j = 0;j < ret;j++)
                { 
                    memset(buf,0,sizeof(buf));
                    item1["Index"] = j;
                    GetMenuItemString(1000+i,j,buf);                    
               //     db_error("get menu item count is %s",buf);
                    item1["Id"] = buf;                    
                    arrayObj0.append(item1);
                    
                }
                item0["MenuList"] = arrayObj0;
                arrayObj1.append(item0);
            }
            root_menu["Menu"] = arrayObj1;
            str = root_menu.toStyledString();
            
            db_error("IOTYPE_CDR_GET_MENU_ITEM end");
        }
        break;
        case IOTYPE_CDR_GETRECORD_STATUS:
        {
            
            db_error("IOTYPE_CDR_GETRECORD_STATUS start");
            int ret = -1;
            Json::Value item;
           // ret = mAdapterLayer->record_ctrl_->GetRecordStatus();
            item["RecodStatus"] = 0;
            str = item.toStyledString();
            db_error("IOTYPE_CDR_GETRECORD_STATUS end");
        }
        break;
    }
}


int httpServer::GetMenuItemString(int cmd,int index,char *buf)
{
    StringVector sub_value_list;
    sub_value_list.clear();
    int index_msg = 0;
    switch(cmd)
    {
        case IOTYPE_CDR_SETVIDEORESOULATION:
            index_msg = 0;
        break;
        case IOTYPE_CDR_SETLOOPRECORD_TIME:
            index_msg = 1;
        break;
        case IOTYPE_CDR_SETVIDEORECORD_TYPE:
            index_msg = 2;
        break;
        case IOTYPE_CDR_SETCIDEOEXPROTION_VALUE:
            index_msg = 3;
        break;
        case IOTYPE_CDR_SETAWD_VALUE:
            index_msg = 4;
        break;
        case TOTYPE_CDR_SETGSENOR_SENVITY_VAUE:
            index_msg = 5;
        break;
        case IOTYPE_CDR_AUTOSCREENSOFF_VALUE:
            index_msg = 6;
        break;
        case IOTYPE_CDR_SETLANGUAGE_VALUE:
            index_msg = 7;
        break;
        case IOTYPE_CDR_SETRECORDSOUND_VALUE:    
            index_msg = 8;
        break;
        case IOTYPE_CDR_SETRECORDSOUNDLEVEL_VALUE:    
            index_msg = 9;
        break;
        case IOTYPE_CDR_SETMOTIONDETECT_VALUE:    
             index_msg = 11;
        break;
        case IOTYPE_CDR_SETLIGHTFRQUENCR_VALUE:    
             index_msg = 12;
        break;
    }
    if(index == -1){
        std::string str_sub_head;
        str_sub_head.clear();
        Res->GetString(std::string(ListBoxItemData[index_msg].first_text), str_sub_head);
        strncpy(buf,str_sub_head.c_str(),strlen(str_sub_head.c_str()));
    }else{
        Res->GetStringArray(std::string(ListBoxItemData[index_msg].first_text), sub_value_list);
        strncpy(buf,sub_value_list[index].c_str(),strlen(sub_value_list[index].c_str()));
    }
   // db_error("get menu item string is %s",sub_value_list[index].c_str());
   
    return 0;
}

int httpServer::GetMenuitemCount(int cmd)
{
    int ret = -1;
    AWDeviceInfo_ex dev_info;
    mAdapterLayer->dev_ctrl_->GetDeviceInfo_ex(dev_info);
    switch(cmd)
    {
        case IOTYPE_CDR_SETVIDEORESOULATION:
            ret = dev_info.menu_config_.resoultion_value_count;
        break;
        case IOTYPE_CDR_SETLOOPRECORD_TIME:
            ret = dev_info.menu_config_.loop_record_time_count;
        break;
        case IOTYPE_CDR_SETVIDEORECORD_TYPE:
            ret = dev_info.menu_config_.recod_type_count;
        break;
        case IOTYPE_CDR_SETCIDEOEXPROTION_VALUE:
            ret = dev_info.menu_config_.video_exp_value_count;
        break;
        case IOTYPE_CDR_SETAWD_VALUE:
            ret = dev_info.menu_config_.parking_value_count;
        break;
        case TOTYPE_CDR_SETGSENOR_SENVITY_VAUE:
            ret = dev_info.menu_config_.Gsensor_senstvity_value_count;
        break;
        case IOTYPE_CDR_AUTOSCREENSOFF_VALUE:
            ret = dev_info.menu_config_.screen_onoff_value_count;
        break;
        case IOTYPE_CDR_SETLANGUAGE_VALUE:
            ret =  dev_info.menu_config_.language_value_count;
        break;
        case IOTYPE_CDR_SETRECORDSOUND_VALUE:    
            ret =  dev_info.menu_config_.record_sound_value_count;
        break;
        case IOTYPE_CDR_SETRECORDSOUNDLEVEL_VALUE:    
            ret =  dev_info.menu_config_.record_sound_level_vaule_count;
        break;
        case IOTYPE_CDR_SETMOTIONDETECT_VALUE:    
            ret =  dev_info.menu_config_.moton_detect_value_count;
        break;
        case IOTYPE_CDR_SETLIGHTFRQUENCR_VALUE:    
            ret =  dev_info.menu_config_.light_value_count;
        break;
    }
    return ret;
    
}

int httpServer::HandleSetIotCtrlCmd(int io_type,int result_value, std::string str)
{
  db_error("HandleIotCtrlCmd : io_type = 0x%0x, result_value = %d, buf = %s",io_type,result_value,str.c_str());
   switch(io_type)
   {
        case IOTYPE_CDR_SETVIDEORESOULATION:
        {
            db_error("removte set IOTYPE_CDR_SETVIDEORESOULATION\n");
            mAdapterLayer->media_ctrl_->SetVideoResoulation(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETLOOPRECORD_TIME:
        {
           mAdapterLayer->media_ctrl_->SetRecordTime(CTRL_TYPE_HTTP,1,result_value);
        }
        break;
        case IOTYPE_CDR_SETVIDEORECORD_TYPE:
        {
           mAdapterLayer->media_ctrl_->SetRecordType(CTRL_TYPE_HTTP,1,result_value);
        }
        break;
        case IOTYPE_CDR_SETCIDEOEXPROTION_VALUE:
        {
           mAdapterLayer->media_ctrl_->SetExposureValue(CTRL_TYPE_HTTP,1,result_value);
        }
        break;
        case IOTYPE_CDR_SETAWD_VALUE:
        {
           mAdapterLayer->media_ctrl_->SetParkingMonitor(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case TOTYPE_CDR_SETGSENOR_SENVITY_VAUE:
        {
           mAdapterLayer->media_ctrl_->SetParkingMonitorLevel(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_AUTOSCREENSOFF_VALUE:
        {
           mAdapterLayer->media_ctrl_->SetCamerAutoScreenSaver(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETLANGUAGE_VALUE:
        {
            mAdapterLayer->media_ctrl_->SetDeviceLanguage(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETRECORDSOUND_VALUE:
        {
             mAdapterLayer->media_ctrl_->SetRecordAudioOnOff(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETRECORDSOUNDLEVEL_VALUE:
        {
             mAdapterLayer->media_ctrl_->SetSystemAudioLevel(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETMOTIONDETECT_VALUE:
        {
            mAdapterLayer->media_ctrl_->SetMotionDetect(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETLIGHTFRQUENCR_VALUE:
        {
            mAdapterLayer->media_ctrl_->SetLightFreq(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_SETDEVICETIME_VALUE:
        {
            //mAdapterLayer->media_ctrl_->SetLightFreq(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_FORMATSDCARD_VALUE:
        {
            mAdapterLayer->dev_ctrl_->FormatDisk(0);
        }
        break;
        case IOTYPE_CDR_FECTORYRESET_VALUE:
        {
            mAdapterLayer->media_ctrl_->SetDeviceReset(CTRL_TYPE_HTTP, 1, result_value);
        }
        break;
        case IOTYPE_CDR_REMOTERECORD_SWITCH:
        {
            mAdapterLayer->record_ctrl_->RemoteSwitchRecord(result_value);
        }
        break;
        case IOTYPE_CDR_REMOTETAKEPHOTO:
        {
            mAdapterLayer->record_ctrl_->RemoteTakePhoto();
        }
        break;
        case IOTYPE_CDR_SET_DATE:
        {
            mAdapterLayer->media_ctrl_->SetDeviceDate(CTRL_TYPE_HTTP, 1 , str);
        }
        break;
        case IOTYPE_CDR_SET_TIME:
        {
            mAdapterLayer->media_ctrl_->SetDeviceTime(CTRL_TYPE_HTTP, 1 , str);
        }
        break;
        case IOTYPE_CDR_SET_WIFI_SSID:
        {
            mAdapterLayer->media_ctrl_->SetWifiSsid(CTRL_TYPE_HTTP, 1 , str);
        }
        break;
        case IOTYPE_CDR_SET_WIFI_PASSPHRASE:
        {
            mAdapterLayer->media_ctrl_->SetWifiPassword(CTRL_TYPE_HTTP, 1 , str);
        }
        break;
        case IOTYPE_CDR_DELET_ONE_FILE:
        {
            mAdapterLayer->dev_ctrl_->RemoveFile(str.c_str());
        }
        break;
        default:
            break;
   }
   return 0; 
}
