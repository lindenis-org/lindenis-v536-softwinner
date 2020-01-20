/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file do_ota.cpp
 * @brief do ota update api handler
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

class DoOTAHandler : public UrlHandler
{
    public:
        bool handlePost(CivetServer *server, struct mg_connection *conn)
        {
            /* Handler may access the request info using mg_get_request_info */
            const struct mg_request_info *req_info = mg_get_request_info(conn);
            long long rlen;
            long long nlen = 0;
            long long tlen = req_info->content_length;
            char buf[1024] = {0};

            db_debug("url: %s", req_info->local_uri);

            while (nlen < tlen) {
                rlen = tlen - nlen;
                if (rlen > sizeof(buf)) {
                    rlen = sizeof(buf);
                }
                rlen = mg_read(conn, buf, (size_t)rlen);
                if (rlen <= 0) {
                    break;
                }
                std::string str = buf;
                db_debug("read post data: %s", str.c_str());
                nlen += rlen;
            }

            mg_printf(conn,
                    "HTTP/1.1 200 OK\r\nContent-Type: "
                    "application/json\r\nConnection: close\r\n\r\n");

            std::string result = "{"
                "\"status\":\"0\""
            "}";

            mg_printf(conn, result.c_str());

            return true;
        }
};
}


