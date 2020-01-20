#include "handler/file_upload.cpp"
#include "handler/file_download.cpp"
#include "handler/do_ota.cpp"
#include "handler/get_device_info.cpp"
#include "handler/get_ota_progress.cpp"
#include "webserver/webserver.h"

#include <errno.h>
#include <string.h>
#include <signal.h>

using namespace EyeseeLinux;

bool g_exit = false;

void handle_exit(int signal)
{
    WebServer::Destroy();
    db_info("bye!!!");
    g_exit = true;
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, handle_exit) == SIG_ERR) {
        db_error("can't catch SIGINT ignal, %s", strerror(errno));
    }
    if (signal(SIGTERM, handle_exit) == SIG_ERR) {
        db_error("can't catch SIGTERM signal, %s", strerror(errno));
    }

    WebServer *web_server = WebServer::GetInstance();
    web_server->Start();

    WebServer::UrlHandlerMap url_handlers;
    url_handlers.emplace("/upload", new UploadHandler());
    url_handlers.emplace("/api/ota", new DoOTAHandler());
    url_handlers.emplace("/api/getdeviceinfo", new GetDeviceInfoHandler());
    url_handlers.emplace("/api/getotaprogress", new GetOtaProgressHandler());
    // url_handlers.emplace("/file", new GetOtaProgressHandler());
    url_handlers.emplace("/tmp/data/.data/sqlite", new DownloadHandler());
    url_handlers.emplace("/mnt/extsd/", new DownloadHandler());
    web_server->RegistUrlHandler(url_handlers);

    while (!g_exit) {
        sleep(1);
    }

    return 0;
}

