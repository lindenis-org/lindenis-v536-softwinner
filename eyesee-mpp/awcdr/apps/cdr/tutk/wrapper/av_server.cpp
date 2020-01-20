/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file av_server.cpp
 * @brief tutk 媒体服务管理.负责创建、初始化、启动av和iotc服务
 * @author id:826
 * @version v0.3
 * @date 2016-08-25
 */
#include "av_server.h"
#include "av_ctrl.h"
#include "tutk_util.h"
#include "AVFRAMEINFO.h"
#include "AVIOCTRLDEFs.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

using namespace tutk;
using namespace std;

AVServer::AVServer(std::string uid, AVCtrl *av_ctrl)
    : iotc_login_tid_(0)
    , iotc_listen_tid_(0)
    , av_server_tid_(0)
    , uid_(uid)
    , client_cnt_(0)
    , av_ctrl_(av_ctrl)
{
    pthread_mutex_init(&server_lock_, NULL);
}

AVServer::~AVServer()
{
    pthread_mutex_destroy(&server_lock_);
}

int AVServer::InitIOTC()
{
    int ret;

    ::IOTC_Set_Max_Session_Number(MAX_CLIENT_NUMBER);

    ret = ::IOTC_Initialize2(0);
    if (ret != IOTC_ER_NoERROR) {
        PrintErrHandling (ret, __FUNCTION__);
        return -1;
    }

    this->GetIOTCVersion();

#ifdef DEBUG_LOG
    ::IOTC_Set_Log_Path("/tmp/log/log_tutk_iotc_server.txt", 0);
#endif

    ::IOTC_Setup_Session_Alive_Timeout(5);

    ::IOTC_Get_Login_Info_ByCallBackFn(AVServer::LoginInfoCallback);

    return 0;
}

int AVServer::InitAV()
{
    int ret;

    // alloc MAX_CLIENT_NUMBER*3 for every session av data/speaker/play back
    // only av data, so.
    ret = ::avInitialize(MAX_CLIENT_NUMBER*1);
    if (ret < 0) {
        fprintf(stderr, "AV Init failed, err[%d]", ret);
        return -1;
    }

    this->GetAVAPIVersion();

#ifdef DEBUG_LOG
    ::AV_Set_Log_Path("/tmp/log/log_tutk_av_server.txt", 0);
#endif

    tmp_context_.v_trigger = false;
    tmp_context_.a_trigger = false;

    return 0;
}

void AVServer::DeInitIOTC()
{
    list<ConnectContext>::iterator iter;
    for (iter = context_list_.begin(); iter != context_list_.end(); iter++) {
        ::IOTC_Session_Close(iter->iotc_sid);
    }

    ::IOTC_DeInitialize();
}

void AVServer::DeInitAV()
{
    list<ConnectContext>::iterator iter;
    for (iter = context_list_.begin(); iter != context_list_.end(); iter++) {
        ::avServStop(iter->av_ch_id);
    }

    ::avDeInitialize();
}

std::string AVServer::GetIOTCVersion()
{
    unsigned int iotc_version;
    ::IOTC_Get_Version(&iotc_version);
    unsigned char *p = (unsigned char *)&iotc_version;

    char iotc_version_buf[16] = {0};
    sprintf(iotc_version_buf, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
    printf("IOTCAPI version[%s]\n", iotc_version_buf);

    return std::string(iotc_version_buf);
}

std::string AVServer::GetAVAPIVersion()
{
    int av_version = ::avGetAVApiVer();
    unsigned char *p = (unsigned char *)&av_version;

    char av_version_buf[16] = {0};
    sprintf(av_version_buf, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
    printf("AVAPI version[%s]\n", av_version_buf);

    return std::string(av_version_buf);
}

int AVServer::HandleIOTCtrlCmd(int sid, int av_index, char *buf, int type)
{
    printf("Handle CMD: ");

    // find connection context
    std::list<ConnectContext>::iterator iter;
    for (iter = context_list_.begin(); iter != context_list_.end(); iter++) {
        if ( iter->iotc_sid == sid) break;
    }

    switch (type) {
        case IOTYPE_USER_IPCAM_START: {
            SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *) buf;
            printf("IOTYPE_USER_IPCAM_START, ch:%d, avIndex:%d\n\n", p->channel, av_index);
            pthread_mutex_lock(&server_lock_);
            iter->v_trigger = true;
            pthread_mutex_unlock(&server_lock_);
            printf("ipcam start OK\n");
        }
        break;
        case IOTYPE_USER_IPCAM_STOP: {
            SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *) buf;
            printf("IOTYPE_USER_IPCAM_STOP, ch:%d, avIndex:%d\n\n", p->channel, av_index);
            pthread_mutex_lock(&server_lock_);
            iter->v_trigger = false;
            pthread_mutex_unlock(&server_lock_);
            printf("ipcam stop OK\n");
        }
        break;
        case IOTYPE_USER_IPCAM_AUDIOSTART: {
            SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *) buf;
            printf("IOTYPE_USER_IPCAM_AUDIOSTART, ch:%d, avIndex:%d\n\n", p->channel, av_index);
            pthread_mutex_lock(&server_lock_);
            iter->a_trigger = true;
            pthread_mutex_unlock(&server_lock_);
            printf("audio start OK\n");
        }
        break;
        case IOTYPE_USER_IPCAM_AUDIOSTOP: {
            SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *) buf;
            printf("IOTYPE_USER_IPCAM_AUDIOSTOP, ch:%d, avIndex:%d\n\n", p->channel, av_index);
            pthread_mutex_lock(&server_lock_);
            iter->a_trigger = false;
            pthread_mutex_unlock(&server_lock_);
            printf("audio stop OK\n");
        }
        break;
        default:
            av_ctrl_->HandleIOTCtrlCmd(sid, av_index, buf, type);
            break;
    }

    return 0;
}

int AVServer::AVSendFrameData(const ConnectContext &context, const char *cab_frame_data, int frame_data_size,
                              void *cab_frame_info, int frame_info_size)
{
    int ret;

    // 是否停止发送
    if (!context.v_trigger) return 0;

    FRAMEINFO_t *frame_info = static_cast<FRAMEINFO_t*>(cab_frame_info);
    frame_info->onlineNum = client_cnt_;

    ret = ::avSendFrameData(context.av_ch_id, cab_frame_data, frame_data_size,
                             cab_frame_info, frame_info_size);

    if(ret == AV_ER_EXCEED_MAX_SIZE) { // means data not write to queue, send too slow, I want to skip it
        usleep(10 * 1000); //sleep 10 ms
        printf("AV_ER_EXCEED_MAX_SIZE SID[%d]  av_ch_id[%d]  frame_data_size:%d\n", context.iotc_sid, context.av_ch_id, frame_data_size);
    } else if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE) {
        printf("AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d]\n", context.iotc_sid);
    } else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT) {
        printf("AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d]\n", context.iotc_sid);
    } else if(ret == IOTC_ER_INVALID_SID) {
        printf("Session cant be used anymore\n");
    }

    return ret;
}

int AVServer::RunLoginThread()
{
    int ret;
    printf("%s\n", __FUNCTION__);
    // create thread to login because without WAN still can work on LAN
    ret = pthread_create(&iotc_login_tid_, NULL, &AVServer::LoginThread, this);
    if (ret < 0) {
        fprintf(stderr, "Login Thread create failed, ret=[%d]\n", ret);
        return -1;
    }
    pthread_detach(iotc_login_tid_);
    return 0;
}

int AVServer::RunIOTCListenThread()
{
    int ret;
    printf("%s\n", __FUNCTION__);
    ret = pthread_create(&iotc_listen_tid_, NULL, &AVServer::IOTCListenThread, this);
    if (ret < 0) {
        fprintf(stderr, "IOTC Listen Thread create failed, ret=[%d", ret);
        return -1;
    }
    pthread_detach(iotc_listen_tid_);
    return 0;
}

int AVServer::RunAVServerThread()
{
    int ret;
    printf("%s\n", __FUNCTION__);
    ret = pthread_create(&av_server_tid_, NULL, &AVServer::AVServerThread, this);
    if (ret < 0) {
        fprintf(stderr, "IOTC Server Thread create failed, ret=[%d]\n", ret);
        return -1;
    }
    pthread_detach(av_server_tid_);

    pthread_mutex_lock(&server_lock_);
    if( client_cnt_ == 0 )
    {
        AVServer::av_ctrl_->SetClientConnectStatus(true);
    }
    client_cnt_++;
    pthread_mutex_unlock(&server_lock_);

    return 0;
}

void *AVServer::LoginThread(void *arg)
{
    int ret = 0;
    AVServer *self = static_cast<AVServer*>(arg);

    for (;;) {
        ret = ::IOTC_Device_Login(self->uid_.c_str(), AVID, AVPWD);
        if (ret == IOTC_ER_NoERROR || ret == IOTC_ER_LOGIN_ALREADY_CALLED) {
            break;
        } else {
            // PrintErrHandling(ret, __FUNCTION__);
            sleep(30);
        }
    }

    printf("LoginThread exit!\n");

    pthread_exit(0);
}

void *AVServer::IOTCListenThread(void *arg)
{
    int sid = 0;
    static int last_error_code = 0;
    AVServer *self = static_cast<AVServer*>(arg);

    for (;;) {
        // Accept connection only when IOTC_Listen() calling
        if ( (sid = IOTC_Listen(0)) < 0) {
            if (sid == IOTC_ER_LISTEN_ALREADY_CALLED)
                break;
            else /*if (sid == IOTC_ER_EXCEED_MAX_SESSION || sid == IOTC_ER_TIMEOUT)*/ {
                if (sid == IOTC_ER_EXCEED_MAX_SESSION) {
                    if (sid != last_error_code)
                        PrintErrHandling (sid, __FUNCTION__);
                } else {
                    PrintErrHandling (sid, __FUNCTION__);
                }
                last_error_code = sid;
                sleep(5);
                continue;
            }
        }

        pthread_mutex_lock(&self->server_lock_);
        self->tmp_context_.iotc_sid = sid;
        pthread_mutex_unlock(&self->server_lock_);

        self->RunAVServerThread();
    }

    printf("IOTCListenThread exit!\n");

    pthread_exit(0);
}


void *AVServer::AVServerThread(void *arg)
{
    int ret;
    AVServer *self = static_cast<AVServer*>(arg);

    unsigned int io_type;
    int resend = -1;
    char io_ctrl_buf[MAX_SIZE_IOCTRL_BUF];
    struct st_SInfo Sinfo;

    pthread_mutex_lock(&self->server_lock_);
    int sid = self->tmp_context_.iotc_sid;
    pthread_mutex_unlock(&self->server_lock_);

    int av_index = ::avServStart3(sid, AVServer::AuthCallback, 0, SERVTYPE_STREAM_SERVER, 0, &resend);
    if(av_index < 0) {
        printf("avServStart3 failed!! sid[%d] code[%d]!!!\n", sid, av_index);
        ::IOTC_Session_Close(sid);
        self->client_cnt_--;
        pthread_exit(0);
    }

    // add iotc session id and av channel id into context list
    ConnectContext context = {sid, av_index};
    self->context_list_.push_back(context);

    if(IOTC_Session_Check(sid, &Sinfo) == IOTC_ER_NoERROR) {
        const char *mode[3] = {"P2P", "RLY", "LAN"};
        // print session information(not a must)
        if( isdigit( Sinfo.RemoteIP[0] ))
            printf("Client is from[IP:%s, Port:%d] Mode[%s] VPG[%d:%d:%d] VER[%X] NAT[%d] AES[%d]\n",
                   Sinfo.RemoteIP, Sinfo.RemotePort, mode[(int)Sinfo.Mode], Sinfo.VID, Sinfo.PID,
                   Sinfo.GID, Sinfo.IOTCVersion, Sinfo.NatType, Sinfo.isSecure);
    }

    printf("avServStart3 OK, avIndex[%d], Resend[%d]\n\n", av_index, resend);

    pthread_mutex_lock(&self->server_lock_);
    self->av_ctrl_->RegisterEventMsg(sid, av_index);
    pthread_mutex_unlock(&self->server_lock_);

    for (;;) {
        ret = avRecvIOCtrl(av_index, &io_type, (char *)&io_ctrl_buf, MAX_SIZE_IOCTRL_BUF, 1000);
        if(ret >= 0) {
            self->HandleIOTCtrlCmd(sid, av_index, io_ctrl_buf, io_type);
        } else if(ret != AV_ER_TIMEOUT) {
            fprintf(stderr, "avIndex[%d], avRecvIOCtrl error, code[%d]\n",av_index, ret);
            break;
        }
    }

    ::avSendIOCtrlExit(av_index);
    ::avServStop(av_index);
    ::IOTC_Session_Close(sid);

    pthread_mutex_lock(&self->server_lock_);
    self->client_cnt_--;
    std::list<ConnectContext>::iterator iter;
    for (iter = self->context_list_.begin(); iter != self->context_list_.end();) {
        if ( iter->iotc_sid == sid) {
            iter = self->context_list_.erase(iter);
        } else {
            iter++;
        }
    }
    self->av_ctrl_->UnRegisterEventMsg(sid, av_index);
    if( self->client_cnt_ == 0 )
    {
        self->av_ctrl_->SetClientConnectStatus(false);
    }

    pthread_mutex_unlock(&self->server_lock_);

    printf("SID[%d], avIndex[%d], thread_ForAVServerStart exit!!\n", sid, av_index);

    pthread_exit(0);
}

void AVServer::LoginInfoCallback(unsigned int login_info)
{
    if ((login_info & 0x04))
        printf("I can be connected via Internet\n");
    else if ((login_info & 0x08))
        printf("I am be banned by IOTC Server because UID multi-login\n");
}

int AVServer::AuthCallback(char *viewAcc, char *viewPwd)
{
    if(strcmp(viewAcc, AVID) == 0 && strcmp(viewPwd, AVPWD) == 0)
        return 1;

    return 0;
}

const string &AVServer::GetUID() const
{
    return uid_;
}

const list<ConnectContext> &AVServer::GetConnectContext() const
{
    return context_list_;
}

void AVServer::ExitLoginThread()
{
    if (iotc_login_tid_ > 0) {
        pthread_cancel(iotc_login_tid_);
    }
}

void AVServer::ExitIOTCListenThread()
{
    ::IOTC_Listen_Exit();

    if (iotc_listen_tid_ > 0) {
        pthread_cancel(iotc_listen_tid_);
    }
}

void AVServer::ExitAVServerThread()
{
    if (av_server_tid_ > 0) {
        pthread_cancel(av_server_tid_);
    }
}

