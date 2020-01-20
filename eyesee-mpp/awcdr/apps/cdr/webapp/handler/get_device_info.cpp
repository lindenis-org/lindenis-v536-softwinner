/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file get_device_info.cpp
 * @brief get device info api handler
 * @author id:826
 * @version v0.9
 * @date 2017-08-09
 */
#include "webserver/webserver.h"
#include "common/app_log.h"

#include <cstring>
#include <unistd.h>

using namespace std;

namespace EyeseeLinux {

class GetDeviceInfoHandler : public UrlHandler
{
    public:
        bool handleGet(CivetServer *server, struct mg_connection *conn)
        {
            /* Handler may access the request info using mg_get_request_info */
            const struct mg_request_info *req_info = mg_get_request_info(conn);

            db_debug("url: %s", req_info->local_uri);

            mg_printf(conn,
                    "HTTP/1.1 200 OK\r\nContent-Type: "
                    "application/json\r\nConnection: close\r\n\r\n");

            std::string result = "{"
                "\"id\":\"001\","
                "\"ip\": \"192.168.205.10\","
                "\"mac\": \"00:32:23:45:66:78\""
            "}";

            mg_printf(conn, result.c_str());

            return true;
        }
};
}

