/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file webserver.h
 * @brief webserver
 * @author id:826
 * @version v0.9
 * @date 2017-07-26
 */
#pragma once

#include "common/singleton.h"
#include "interface/remote_connector.h"

#include <string>
#include <mutex>
#include <map>

#include <CivetServer.h>

class CivetServer;

class DeviceAdapter;

namespace EyeseeLinux {

class UrlHandler : public CivetHandler
{
    public:
        UrlHandler() {}

        virtual ~UrlHandler() {}

        void SetAdapter(DeviceAdapter *adapter) {
            adapter_ = adapter;
        }
    private:
        DeviceAdapter *adapter_;
};

class WebServer
    : public Singleton<WebServer>
    , public RemoteConnector
{
    friend class Singleton<WebServer>;

    public:
        typedef std::map<std::string, UrlHandler*> UrlHandlerMap;

        void SetAdapter(DeviceAdapter *adapter);

        int Init(const InitParam &param);

        int Start();

        int RegistUrlHandler(const UrlHandlerMap &handlers);

        int Stop();

    private:
        std::string root_;
        std::string port_;
        DeviceAdapter *adapter_;
        CivetServer *server_;
        std::mutex server_mutex_;
        UrlHandlerMap handlers_;

        WebServer();
        ~WebServer();
        WebServer(const WebServer &o);
        WebServer &operator=(const WebServer &o);
};

}

