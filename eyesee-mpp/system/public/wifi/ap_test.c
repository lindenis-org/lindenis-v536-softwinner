/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file wifi_ap.c
 * @brief HAL for wifi ap control
 * @author id: KP0356
 * @version v0.1
 * @date 2016-08-28
 */

/******************************************************************************/
/*                             Include Files                                  */
/******************************************************************************/
#include "wpa_ctrl.h"
#include "wifi_ap.h"

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

int main(int argc, char* argv[])
{
    int ret = 0;
    int cnt = 0;
    int i   = 0;
    WIFI_AP_CFG_S ap_cfg;

    AP_ERR_PRT("argc is : %d\n", argc);
    for (i = 0; i < argc; i++){
	AP_ERR_PRT("argc[%d] is : %s\n", i, argv[i]);
    }

    if(argc <= 2){
        AP_ERR_PRT ("Set argv fail!\n");
	return -1;
    }
    ret = wifi_ap_init();
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_init fail! ret:%d\n", ret);
        return -1;
    }

    ret = wifi_ap_open("wlan0");
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_open fail! ret:%d\n", ret);
        return -1;
    }

    memset(&ap_cfg, 0, sizeof(ap_cfg));
    strncpy(ap_cfg.ssid,  argv[1], sizeof(ap_cfg.ssid)-1);
    strncpy(ap_cfg.bssid, "a0:0b:ba:b4:af:3e", sizeof(ap_cfg.bssid)-1);
    strncpy(ap_cfg.pswd,  argv[2], sizeof(ap_cfg.pswd)-1);
    ap_cfg.channel  = 8;
    ap_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_PSK;
    //ap_cfg.security = WIFI_AP_SECURITY_WEP;
    //ap_cfg.security = WIFI_AP_SECURITY_OPEN;
    //ap_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_EAP;
    ap_cfg.hidden_ssid = 0;

    ret = wifi_ap_start("wlan0", &ap_cfg);
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_start fail! ret:%d\n", ret);
        return -1;
    }

    system("ifconfig wlan0 192.168.10.99");
    system("udhcpd /etc/udhcpd.conf");
    sleep(120);

    ret = wifi_ap_stop("wlan0");
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_stop fail! ret:%d\n", ret);
        return -1;
    }
    usleep(80);

    ret = wifi_ap_close("wlan0");
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_close fail! ret:%d\n", ret);
    }
    usleep(80);

    ret = wifi_ap_exit();
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_exit fail! ret:%d\n", ret);
    }
    usleep(80);

    return 0;
}
