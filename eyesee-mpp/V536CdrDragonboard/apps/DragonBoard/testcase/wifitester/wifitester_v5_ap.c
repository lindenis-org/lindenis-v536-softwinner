/******************************************************************************/
/*                             Include Files                                  */
/******************************************************************************/
#include "wifi/wifi_ap.h"
#include "wifi/wifi_sta.h"

#include <unistd.h>
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

#include <fcntl.h>
#include <sys/stat.h>

#define TAG "wifitester"
#include <dragonboard/dragonboard.h>

#define IFNAME              "wlan0"
#define BUF_SIZE            256
#define EVENT_BUF_SIZE      2048
#define FIFO_DEV            "/tmp/fifo_wifi"
#define PATH_PROCNET_DEV    "/proc/net/dev"


static int check_net_dev(const char *net_dev_name)
{
    int  fd  = -1;
    char buf[768] = {0};
    FILE *fp = NULL;

    fp = fopen(PATH_PROCNET_DEV, "r");
    if (NULL == fp) {
        printf("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_DEV, errno, strerror(errno));
    }

    /* Skip the two line. */
    fgets(buf, sizeof buf, fp); /* eat line */
    fgets(buf, sizeof buf, fp); /* eat line */

    while (fgets(buf, sizeof(buf) - 1, fp)) {
        if (strstr (buf, net_dev_name)) {
            printf("check_net_dev buf[%s] \n", buf);
            fclose(fp);
            return 0;
        }
        memset(buf, 0, sizeof(buf) - 1);
    }

    fclose(fp);
    return -1;
}


int main(int argc, char** argv)
{
	int fifoFd = 0;
	char buf[50] = {0};
	char buf2[60] ={0};
	int retVal   = 0;
 
    if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {
        if (mkfifo(FIFO_DEV, 0666) < 0) {
            printf("mkfifo failed(%s)\n", strerror(errno));
            //return -1;
        } else {
            fifoFd = open(FIFO_DEV, O_WRONLY);
        }
    }

	int ret = 0;
	int cnt = 0;
	int i   = 0;
    WIFI_AP_CFG_S ap_cfg;

    ret = wifi_ap_init();
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_init fail! ret:%d\n", ret);
        goto wifi_ap_exit;
    }

    ret = wifi_ap_open("wlan0");
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_open fail! ret:%d\n", ret);
        goto wifi_ap_exit;
    }

    memset(&ap_cfg, 0, sizeof(ap_cfg));
    strncpy(ap_cfg.ssid,  "sdv_guixing", sizeof(ap_cfg.ssid)-1);
    strncpy(ap_cfg.bssid, "a0:0b:ba:b4:af:3e", sizeof(ap_cfg.bssid)-1);
    strncpy(ap_cfg.pswd,  "12345678", sizeof(ap_cfg.pswd)-1);
    ap_cfg.channel  = 8;
    ap_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_PSK;
    //ap_cfg.security = WIFI_AP_SECURITY_WEP;
    //ap_cfg.security = WIFI_AP_SECURITY_OPEN;
    //ap_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_EAP;
    ap_cfg.hidden_ssid = 0;

    ret = wifi_ap_start("wlan0", &ap_cfg);
    if (ret) {
        AP_ERR_PRT ("Do wifi_ap_start fail! ret:%d\n", ret);
        goto wifi_ap_exit;
    }

    sleep(2);

    ret = check_net_dev("wlan0");
    if (ret) {
        AP_ERR_PRT ("Do check_net_dev fail! ret:%d\n", ret);
        goto wifi_ap_exit;
    }

    ret = 0;
wifi_ap_exit: 
    if (ret)
	{
		AP_ERR_PRT ("Do wifi_sta_get_scan_results fail! ret:%d\n", ret);
		write(fifoFd, "F[WIFI]:FAIL", 50);
    }
   else
   {
		write(fifoFd, "P[WIFI]:PASS", 50);

   }
    return 0;
}

