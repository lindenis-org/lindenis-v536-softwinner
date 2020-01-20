/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file av_server.h
 * @brief tutk 媒体服务管理.负责创建、初始化、启动av和iotc服务
 * @author id:826
 * @version v0.3
 * @date 2016-08-25
 */
#pragma once

#include <tutk/AVAPIs.h>
#include <tutk/IOTCAPIs.h>

#include <pthread.h>
#include <string>
#include <list>

namespace tutk {

#define SERVTYPE_STREAM_SERVER  0
#define MAX_CLIENT_NUMBER       1
#define MAX_SIZE_IOCTRL_BUF		1024

#define AVID                    "admin"
#define AVPWD                   "123456"

typedef int  (*TutkAVSendFrameDataFunc)(int nAVChannelID, const char *cabFrameData, int nFrameDataSize,
                                   const void *cabFrameInfo, int nFrameInfoSize);

typedef int  (*TutkAVSendAudioDataFunc)(int nAVChannelID, const char *cabAudioData, int nAudioDataSize,
                                   const void *cabFrameInfo, int nFrameInfoSize);

typedef struct ConnectContext {
    int     iotc_sid;     /**< IOTC Session ID */
    int     av_ch_id;     /**< AV Channel ID */
    bool    v_trigger;    /**< enable or disable video stream send */
    bool    a_trigger;    /**< enable or disable audio stream send */
    bool    stop_flag;    /**< the flag to stop conection */
} ConnectContext;

class AVCtrl;
class AVServer {
    public:
        AVServer(std::string uid, AVCtrl *av_ctrl);
        ~AVServer();
        int InitIOTC();
        int InitAV();
        void DeInitIOTC();
        void DeInitAV();
        std::string GetIOTCVersion();
        std::string GetAVAPIVersion();
        int AVSendFrameData(const ConnectContext &context, const char *cab_frame_data, int frame_data_size,
                            void *cab_frame_info, int frame_info_size);
        int RunLoginThread();
        int RunIOTCListenThread();
        int RunAVServerThread();
        void ExitLoginThread();
        void ExitIOTCListenThread();
        void ExitAVServerThread();
        static void *LoginThread(void *arg);
        static void *IOTCListenThread(void *arg);
        static void *AVServerThread(void *arg);
        const std::string &GetUID() const;
        const std::list<ConnectContext> &GetConnectContext() const;

    private:
        pthread_t iotc_login_tid_;
        pthread_t iotc_listen_tid_;
        pthread_t av_server_tid_;
        std::string uid_;

        ConnectContext tmp_context_;
        std::list<ConnectContext> context_list_;   /**< 保存连接上下文，IOTC Session ID及AV Channel ID */
        int client_cnt_;                           /**< online client count */
        AVCtrl *av_ctrl_;
        pthread_mutex_t server_lock_;

        int HandleIOTCtrlCmd(int sid, int av_index, char *buf, int type);
        static void LoginInfoCallback(unsigned int login_info);
        static int AuthCallback(char *viewAcc,char *viewPwd);
};

}
