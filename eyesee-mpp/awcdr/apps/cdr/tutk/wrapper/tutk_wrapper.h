/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file tutk_wrapper.h
 * @brief tutk sdk包装类
 *
 *  用于提供sdk服务启动、停止及流发送接口
 *
 * @author id:826
 * @version v0.3
 * @date 2016-09-01
 */

#pragma once

#include "interface/remote_connector.h"

#include "AVFRAMEINFO.h"
#include "AVIOCTRLDEFs.h"
#include <pthread.h>

class DeviceAdapter;
namespace tutk {

class AVServer;
class AVCtrl;

enum ServerStatus {
    SERVER_INITED = 0,
    SERVER_START,
    SERVER_STOP,
    SERVER_UNINIT,
};

class TutkWrapper
    : public RemoteConnector
{
    public:
        TutkWrapper();

        TutkWrapper(DeviceAdapter *adapter);

        ~TutkWrapper();

        int Init(const InitParam &param);

        void SetAdapter(DeviceAdapter *adapter);

        int Start();

        int Stop();

        int SendVideoData(const char *data, int size, void *data_info, int info_size);

        int SendCmdData(int type,int chn ,int value ,void *data);

    private:
        DeviceAdapter *adapter_;
        AVServer *av_server_;
        AVCtrl *av_ctrl_;
        int status_;
        pthread_mutex_t lock_;
};

}
