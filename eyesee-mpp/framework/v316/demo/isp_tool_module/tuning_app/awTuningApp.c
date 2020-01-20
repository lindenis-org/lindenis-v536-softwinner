#include "log_handle.h"
#include "./server/server_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SERVER_PORT          8848
#define BUILD_VERSION        "Build Version: 0.9.2"

//config
//TODO
static const char *logger = "/data/hawkview_server.log";
static int port = SERVER_PORT;
bool vi_venc_rtsp_en = false;

//TODO
static int parse(int argc, const char *argv[])
{
    int i = 1; // argc = 2;
    for(; i < argc; ++i)
    {
        if(strstr(argv[i], "-h"))
        {
            printf("-h                          awTuningApp help\n");
            printf("-port=[port]                awTuningApp server socket port\n");
            printf("-vi-venc-rtsp-en=[yes|no]   awTuningApp enable vi_venc_rtsp, default as no\n");
            printf("-logger=[log file path]     awTuningApp logger file path\n");
            return -1;
        }
        else if(strstr(argv[i], "-port="))
        {
            port = atoi(argv[i] + strlen("-port="));           
        }
        else if(strstr(argv[i], "-vi-venc-rtsp-en="))
        {
            if(!strcmp(argv[i] + strlen("-vi-venc-rtsp-en="), "yes"))
            {
                vi_venc_rtsp_en = true;           
            }
            
        }
        else if(strstr(argv[i], "-logger="))
        {
            logger = argv[i] + strlen("-logger=");
        }
        else
        {
            printf("fatal error, the config[%s] is ill", argv[i]);    
        }
    }

    return 0;
}

int main(int argc, const char *argv[])
{
	int ret = -1;
	//int port = SERVER_PORT;

    if(parse(argc, argv))
    {
        return 0;
    }
    printf("the port = %d\n", port);
    printf("the vi-venc-rtsp_en=%s\n", vi_venc_rtsp_en? "true":"false");
    printf("the logger=%s\n", logger);
	
#if SERVER_DEBUG_EN
	if (argc >= 2) {
		init_logger(argv[1], "wb");
	} else {
		init_logger("/data/hawkview_server.log", "wb");
	}
#endif

	LOG(                  "==================================================================================\n"
		"                  ==========   Welcome to Hawkview Tools Tuning Server  Venc:V0.0         ==========\n"
		"                  ==========   %s, %s %s                 ==========\n"
		"                  ==========   Copyright (c) 2017 by Allwinnertech Co., Ltd.              ==========\n"
		"                  ==========   http://www.allwinnertech.com                               ==========\n",
		BUILD_VERSION, __TIME__, __DATE__
		);

#if 0
	if (argc > 1) {
		port = atoi(argv[1]);
	}
#endif
    
	ret = init_server(port);
	if (!ret) {
		LOG(              "==========   Hawkview Tools Tuning Server Starts up, Enjoy tuning now!  ==========\n"
		//"                  ==================================================================================\n"
			);
		ret = run_server();
	}

	exit_server();
	LOG(                  "==========   Hawkview Tools Tuning Server Exits, Bye-bye!               ==========\n"
		"                  ==================================================================================\n"
		);

#if SERVER_DEBUG_EN
	close_logger();
#endif

	return 0;
}

