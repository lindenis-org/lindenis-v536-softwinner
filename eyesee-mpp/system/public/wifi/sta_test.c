/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file wifi_sta.c
 * @brief HAL for wifi control
 * @author id: KP0356
 * @version v0.1
 * @date 2016-08-28
 */

/******************************************************************************/
/*                             Include Files                                  */
/******************************************************************************/
#include "wpa_supplicant/wpa_ctrl.h"
#include "wifi_sta.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <net/route.h>
#include <stdio.h>

/******************************************************************************/
/*                           Macros & Typedefs                                */
/******************************************************************************/

static void wpa_event_callback(WIFI_STA_EVENT_E event, void *pdata)
{
    STA_DB_PRT("wpa_event_callback  event:%d \n", event);
}


int main(int argc, char* argv[])
{
    int ret = 0;
    int cnt = 0;
    int i   = 0;
    WIFI_STA_STATUS_E   sta_status;
    WIFI_STA_STATUS_E   scan_status;
    WIFI_STA_AP_LIST_S  ap_list;
    WIFI_STA_EVENT_E    wpa_event;
    WIFI_STA_CONNECT_STATUS_S con_status;

    STA_ERR_PRT("argc is : %d\n", argc);
    for (i = 0; i < argc; i++){
	STA_ERR_PRT("argc[%d] is : %s\n", i, argv[i]);
    }

    if(argc != 1){
	if(argc <= 2){
            STA_ERR_PRT ("Set argv fail!\n");
	    return -1;
        }
    }

    ret = wifi_sta_init();
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_init fail! ret:%d\n", ret);
        return -1;
    }

    ret = wifi_sta_open("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_open fail! ret:%d\n", ret);
        return -1;
    }

    ret = wifi_sta_start("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_start fail! ret:%d\n", ret);
        return -1;
    }

    sleep(1);

    ret = wifi_sta_register_eventcall("wlan0", wpa_event_callback, NULL);
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_register_enventcall fail! ret:%d\n", ret);
        return -1;
    }

    ret = wifi_sta_start_scan("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_start_scan fail! ret:%d\n", ret);
        return -1;
    }

    while (1) {
        ret = wifi_sta_get_scan_status("wlan0", &scan_status);
        if (ret) {
            STA_ERR_PRT ("Do wifi_sta_get_scan_status fail! ret:%d\n", ret);
            sleep(1);
            continue;
        }

        if (WIFI_STA_STATUS_SCANING == scan_status) {
            STA_DB_PRT ("Waitting scaning......\n");
            sleep(1);
            continue;
        } else if (WIFI_STA_STATUS_SCAN_END == scan_status) {
            STA_DB_PRT ("Scan AP list is end!\n");
            break;
        }
    }

    ret = wifi_sta_get_scan_results("wlan0", &ap_list);
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_get_scan_results fail! ret:%d\n", ret);
        return -1;
    }

    sleep(1);
    if(argc == 1)
	goto exit;

    WIFI_STA_AP_INFO_S ap_info;
    memset(&ap_info, 0, sizeof(ap_info));
    //strcpy(ap_info.ssid, "PD2-IPC-test");
    //ap_info.security = WIFI_STA_SECURITY_OPEN;
    strcpy(ap_info.ssid, argv[1]);
    strcpy(ap_info.psswd, argv[2]);
    ap_info.security = WIFI_STA_SECURITY_WPA_WPA2_PSK;

    ret = wifi_sta_connect("wlan0", &ap_info);
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_connect fail! ret:%d\n", ret);
        return -1;
    }

    ret = wifi_sta_do_dhcp("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_do_dhcp fail! ret:%d\n", ret);
        return -1;
    }

    cnt = 8;
    while (cnt--) {
        ret = wifi_sta_get_event("wlan0", &wpa_event);
        if (ret) {
            STA_ERR_PRT ("Do wifi_sta_get_event fail! ret:%d\n", ret);
        }
        STA_DB_PRT ("\n cnt:%d  Do wifi_sta_get_event :%d \n", cnt, wpa_event);
        sleep(8);
    }
    ret = wifi_sta_disconnect("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_disconnect fail! ret:%d\n", ret);
        return -1;
    }

exit:
    wifi_sta_unregister_eventcall("wlan0");

    wifi_sta_stop("wlan0");

    wifi_sta_close("wlan0");

    wifi_sta_exit();

    sleep(10);

    return 0;
}

