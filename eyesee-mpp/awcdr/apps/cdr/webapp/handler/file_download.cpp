/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file file download.cpp
 * @brief file download api handler
 * @author id:826
 * @version v0.9
 * @date 2017-10-10
 */
#include "webserver/webserver.h"
#include "common/app_log.h"

#include <cstring>
#include <unistd.h>

using namespace std;

namespace EyeseeLinux {

class DownloadHandler : public UrlHandler
{
    public:
        bool handleGet(CivetServer *server, struct mg_connection *conn)
        {
            /* Handler may access the request info using mg_get_request_info */
            const struct mg_request_info *req_info = mg_get_request_info(conn);

            db_debug("url: %s", req_info->local_uri);

            string filepath = req_info->local_uri;
            string basename = filepath.substr(filepath.rfind("/") + 1);

            string::size_type pos;
            pos = basename.rfind(".");
            if (pos == string::npos) {
                db_error("Error: File not found");
                mg_send_http_error(conn, 404, "%s", "Error: File not found");
                return true;
            }
            string suffix = basename.substr(pos);

            if (suffix == ".db") {
                mg_send_mime_file(conn, string("/tmp/sqlite/" + basename).c_str(), NULL);
            } else if (suffix == ".jpg" || suffix == ".mp4") {
                mg_send_mime_file(conn, string("/mnt/extsd/" + basename).c_str(), NULL);
            }

            db_debug("basename: %s, suffix: %s", basename.c_str(), suffix.c_str());

            return true;
        }
};

}

