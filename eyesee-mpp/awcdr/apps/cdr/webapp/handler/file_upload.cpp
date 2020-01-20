/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file file_upload.cpp
 * @brief webserver file upload api handler
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

class UploadHandler : public UrlHandler
{
    public:
        bool handleGet(CivetServer *server, struct mg_connection *conn)
        {
            /* Handler may access the request info using mg_get_request_info */
            const struct mg_request_info *req_info = mg_get_request_info(conn);

            db_debug("url: %s", req_info->local_uri);

            mg_printf(conn,
                    "HTTP/1.1 200 OK\r\nContent-Type: "
                    "text/html\r\nConnection: close\r\n\r\n");

            mg_printf(conn, "<html><body>\n");
            mg_printf(conn, "<h2>You should send a PUT requst to upload a file!</h2>\n");
            mg_printf(conn,
                    "<p>The request was:<br><pre>%s %s HTTP/%s</pre></p>\n",
                    req_info->request_method,
                    req_info->request_uri,
                    req_info->http_version);
            mg_printf(conn, "</body></html>\n");

            return true;
        }

        bool handlePut(CivetServer *server, struct mg_connection *conn)
        {
            /* Handler may access the request info using mg_get_request_info */
            const struct mg_request_info *req_info = mg_get_request_info(conn);
            long long rlen, wlen;
            long long nlen = 0;
            long long tlen = req_info->content_length;
            FILE * f;
            char path[1024] = {0};
            char buf[1024] = {0};
            int fail = 0;

            // TODO: must check size, forbid upload large file

            std::string filename = "";
            if (CivetServer::getParam(req_info->query_string, "file", filename) < 0) {
                db_error("getparam failed");
                fail = 1;
                goto out;
            }

            // TODO: check file type, only bmp file be accept
            snprintf(path, sizeof(path), "/usr/share/osd/%s", filename.c_str());
            if (strlen(path)>1020) {
                /* The string is too long and probably truncated. Make sure an
                 * UTF-8 string is never truncated between the UTF-8 code bytes.
                 * This example code must be adapted to the specific needs. */
                fail = 1;
                f = NULL;
            } else {
                f = fopen(path, "w");
            }

            db_debug("write to file: %s", path);

            if (!f) {
                fail = 1;
            } else {
                while (nlen < tlen) {
                    rlen = tlen - nlen;
                    if (rlen > sizeof(buf)) {
                        rlen = sizeof(buf);
                    }
                    rlen = mg_read(conn, buf, (size_t)rlen);
                    if (rlen <= 0) {
                        fail = 1;
                        break;
                    }
                    wlen = fwrite(buf, 1, (size_t)rlen, f);
                    if (wlen != rlen) {
                        fail = 1;
                        break;
                    }
                    nlen += wlen;
                }
                fclose(f);
            }

            db_debug("recv file size: %lld, write file size: %lld\n", tlen, nlen);

out:
            if (fail) {
                mg_printf(conn,
                        "HTTP/1.1 409 Conflict\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n\r\n");
            } else {
                mg_printf(conn,
                        "HTTP/1.1 201 Created\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n\r\n");
            }

            return true;
        }
};
}
