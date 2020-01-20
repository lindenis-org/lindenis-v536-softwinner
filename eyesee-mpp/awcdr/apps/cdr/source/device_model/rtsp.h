/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file rtsp.h
 * @brief rtsp接口类
 * @author id:826
 * @version v0.3
 * @date 2016-07-26
 */
#pragma once

#include "common/singleton.h"

#include <rtsp/TinyServer.h>
#include <rtsp/MediaStream.h>
#include <string>

namespace EyeseeLinux {

class RtspServer
    : public Singleton<RtspServer>
{
    friend class Singleton<RtspServer>;

    public:
        enum RtspServerStatus {
            SERVER_RUNNING = 0,
            SERVER_STOPED
        };

        class StreamSender
        {
            friend class RtspServer;
            public:
                ~StreamSender();
                void SendVideoData(unsigned char *data, unsigned int size, long    long pts, unsigned int type);
                void SendAudioData(unsigned char *data, unsigned int size);
                void SetPPS(unsigned char* from, unsigned size);
                void SetSPS(unsigned char* from, unsigned size);

                typedef void(OnClientConnected)(void *context);

                void SetSenderCallback(OnClientConnected *callback, void *context);
                std::string &GetUrl();
            private:
                ::MediaStream *stream_;
                std::string url_;
                StreamSender(::MediaStream *stream);
        };

        StreamSender* CreateStreamSender(std::string name);
        int CreateServer(const std::string &ip);
        void Run();
        void Stop();

        RtspServerStatus GetServerStatus();

    private:
        ::TinyServer *tiny_server_;
        RtspServerStatus server_status_;

        RtspServer();
        ~RtspServer();
        RtspServer(const RtspServer &o);
        RtspServer &operator=(const RtspServer &o);
};

}

