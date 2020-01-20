/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file rtsp.cpp
 * @brief rtsp接口类
 * @author id:826
 * @version v0.3
 * @date 2016-07-26
 */

#include "device_model/rtsp.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "RtspServer"

using namespace EyeseeLinux;
using namespace std;

RtspServer::RtspServer()
{
}

RtspServer::~RtspServer()
{
    delete tiny_server_;
    tiny_server_ = NULL;
}

int RtspServer::CreateServer(const string &ip)
{
    tiny_server_ = ::TinyServer::createServer(ip, 8554);
    return 0;
}

void RtspServer::Run()
{
    server_status_ = SERVER_RUNNING;
    tiny_server_->run();
}

void RtspServer::Stop()
{
    tiny_server_->stop();
    server_status_ = SERVER_STOPED;
}

RtspServer::RtspServerStatus RtspServer::GetServerStatus()
{
    return server_status_;
}

RtspServer::StreamSender *RtspServer::CreateStreamSender(string name)
{
    StreamSender *stream_sender;
    MediaStream::MediaStreamAttr ms_attr = {
        MediaStream::MediaStreamAttr::VIDEO_TYPE_H264,
        MediaStream::MediaStreamAttr::AUDIO_TYPE_AAC,
        MediaStream::MediaStreamAttr::STREAM_TYPE_UNICAST,
    };

    stream_sender = new StreamSender(tiny_server_->createMediaStream(name, ms_attr));

    if (stream_sender == NULL) {
        db_error("create stream sender filed");
    }
    return stream_sender;
}

RtspServer::StreamSender::StreamSender(::MediaStream *stream)
    : stream_(NULL)
{
    stream_ = stream;
}

RtspServer::StreamSender::~StreamSender()
{
    delete stream_;
}
void RtspServer::StreamSender::SendVideoData(unsigned char *data, unsigned int size, long long pts ,unsigned int type)
{
    stream_->appendVideoData(data, size,pts, (MediaStream::FrameDataType)type);
}

void RtspServer::StreamSender::SendAudioData(unsigned char *data, unsigned int size)
{
    stream_->appendAudioData(data,size,0);
}

void RtspServer::StreamSender::SetPPS(unsigned char* from, unsigned size)
{
}

void RtspServer::StreamSender::SetSPS(unsigned char* from, unsigned size)
{
}

void RtspServer::StreamSender::SetSenderCallback(OnClientConnected *callback, void *context)
{
    stream_->setNewClientCallback(callback, context);
}

string &RtspServer::StreamSender::GetUrl()
{
    url_ = stream_->streamURL();
    return url_;
}
