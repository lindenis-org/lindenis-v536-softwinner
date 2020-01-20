/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file webserver.cpp
 * @brief webserver
 * @author id:826
 * @version v0.9
 * @date 2017-07-26
 */
#include "webserver.h"
#include "common/app_log.h"

#include <CivetServer.h>

#include <cstring>
#include <unistd.h>

using namespace std;
using namespace EyeseeLinux;

WebServer::WebServer()
    : root_("/tmp")
    , port_("8082")
    , server_(NULL)
{
}

WebServer::~WebServer()
{
    Stop();

    for (auto e : handlers_) {
        delete e.second;
    }
}

void WebServer::SetAdapter(DeviceAdapter *adapter)
{
    adapter_ = adapter;
}

int WebServer::Init(const InitParam &param)
{
    root_ = param.arg1;
    port_ = param.arg2;

    return 0;
}

int WebServer::Start()
{
    lock_guard<mutex> lock(server_mutex_);
    if (server_ == NULL) {
        const char *opt[] = {
            "document_root", root_.c_str(), "listening_ports", port_.c_str(), 0};

        std::vector<std::string> options;
        for (unsigned int i=0; i<(sizeof(opt)/sizeof(opt[0])-1); i++) {
            options.push_back(opt[i]);
        }

        server_ = new CivetServer(options);
    }

    return server_?0:-1;

}

int WebServer::RegistUrlHandler(const UrlHandlerMap &handlers)
{
    if (server_ == NULL) {
        db_warn("web server is not running");
        return -1;
    }

    handlers_ = handlers;

    for (auto e : handlers_) {
        e.second->SetAdapter(adapter_);
        server_->addHandler(e.first, *(e.second));
    }

    return 0;
}

int WebServer::Stop()
{
    lock_guard<mutex> lock(server_mutex_);
    if (server_) {
        server_->close();
        delete server_;
        server_ = NULL;
    }

    return 0;
}
