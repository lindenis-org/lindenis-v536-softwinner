/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file remote_connector.h
 * @brief 第三方SDK连接适配器接口类
 * @author id:826
 * @version v0.3
 * @date 2016-08-31
 */

#pragma once


#include <string>

/** Stream Sender Type */
typedef enum {
    STREAM_SENDER_RTSP = 1 << 0,
    STREAM_SENDER_TUTK = 1 << 1,
    STREAM_SENDER_NONE = 1 << 2,
} StreamSenderType;

/** Connector Controller Type */
typedef enum {
    CTRL_TYPE_TUTK   = 0x01,
    CTRL_TYPE_ONVIF  = 0x02,
    CTRL_TYPE_HTTP   = 0x03,
} ControllerType;

class DeviceAdapter;

class RemoteConnector {
    public:
        struct InitParam {
            std::string arg1;
            std::string arg2;
            void *reversed;
        };

        virtual ~RemoteConnector() {}
        virtual void SetAdapter(DeviceAdapter *adapter) {};
        virtual int Init(const InitParam &param) { return 0; }
        virtual int Start() { return 0;}
        virtual int Stop() {return 0;}
        virtual int SendVideoData(const char *data, int size, void *data_info, int info_size) {return 0;}
        virtual int SendCmdData(int type,int chn ,int value ,void *data){return 0;}

};
