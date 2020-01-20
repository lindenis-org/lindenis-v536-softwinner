/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file tutk_wrapper.cpp
 * @brief tutk sdk包装类
 *
 *  用于提供sdk服务启动、停止及流发送接口
 *
 * @author id:826
 * @version v0.3
 * @date 2016-09-01
 */

#include <stdio.h>
#include "tutk_wrapper.h"
#include "av_server.h"
#include "av_ctrl.h"

#include "interface/dev_ctrl_adapter.h"

using namespace tutk;
using namespace std;

#if 0
#define pthread_mutex_lock(lock)  do { \
    printf("%s, lock\n", __FUNCTION__); \
    pthread_mutex_lock(lock); \
    printf("%s, locked\n", __FUNCTION__); \
} while(0)

#define pthread_mutex_unlock(lock)  do { \
    printf("%s, unlock\n", __FUNCTION__); \
    pthread_mutex_unlock(lock); \
    printf("%s, unlocked\n", __FUNCTION__); \
} while(0)
#endif

TutkWrapper::TutkWrapper()
{
}

TutkWrapper::TutkWrapper(DeviceAdapter *adapter)
    : adapter_(adapter)
    , av_server_(NULL)
    , av_ctrl_(NULL)
{
    av_ctrl_ = NULL;
    av_server_ = NULL;
    status_ = SERVER_UNINIT;
    pthread_mutex_init(&lock_, NULL);

}


TutkWrapper::~TutkWrapper()
{
    this->Stop();

    pthread_mutex_lock(&lock_);
    av_server_->DeInitAV();
    // FIXME: dead lock
    av_server_->DeInitIOTC();
    pthread_mutex_unlock(&lock_);

    delete av_ctrl_;
    delete av_server_;
}

int TutkWrapper::Init(const InitParam &param)
{
    int ret = 0;

    av_ctrl_ = new AVCtrl(adapter_);
    av_server_ = new AVServer(param.arg1, av_ctrl_);

    if ( (ret = av_server_->InitIOTC()) < 0) {
        fprintf(stderr, "init iotc failed\n");
        return ret;
    }

    if ( (ret = av_server_->InitAV()) < 0) {
        fprintf(stderr, "init av failed\n");
        return ret;
    }

    status_ = SERVER_INITED;

    return ret;
}

void TutkWrapper::SetAdapter(DeviceAdapter *adapter)
{
    av_ctrl_->SetAdapter(adapter);
}

int TutkWrapper::Start()
{
    av_server_->RunLoginThread();
    av_server_->RunIOTCListenThread();
    status_ = SERVER_START;

    return 0;
}

int TutkWrapper::Stop()
{
    pthread_mutex_lock(&lock_);
    status_ = SERVER_STOP;
    pthread_mutex_unlock(&lock_);

    // TODO: must stop thread first !!!
    av_server_->ExitAVServerThread();
    av_server_->ExitIOTCListenThread();
    av_server_->ExitLoginThread();

    return 0;
}
int TutkWrapper::SendVideoData(const char *data, int size, void *data_info, int info_size)
{
    int ret = -1;

    pthread_mutex_lock(&lock_);
    if (status_ == SERVER_STOP) {
        pthread_mutex_unlock(&lock_);
        return ret;
    }

    // 如果有多个客户端连接，向多个session发送
    const std::list<ConnectContext> context_list = av_server_->GetConnectContext();
    std::list<ConnectContext>::const_iterator c_iter;
    for (c_iter = context_list.begin(); c_iter != context_list.end(); c_iter++) {
        ret = av_server_->AVSendFrameData((*c_iter), data, size, data_info, info_size);

#if 0
        if (ret < 0) {
            fprintf(stderr, "session: %d, av channel: %d send video data failed, ret[%d]\n",
                    c_iter->iotc_sid, c_iter->av_ch_id, ret);
        }
#endif
    }
    pthread_mutex_unlock(&lock_);

    return ret;
}

int TutkWrapper::SendCmdData(int type,int chn ,int value ,void *data)
{
    int ret = -1;
    pthread_mutex_lock(&lock_);
    if (status_ == SERVER_STOP) {
        pthread_mutex_unlock(&lock_);
        return ret;
    }
    av_ctrl_->SendCmdData(type,chn,value,data);
    pthread_mutex_unlock(&lock_);
    return 0;
}
