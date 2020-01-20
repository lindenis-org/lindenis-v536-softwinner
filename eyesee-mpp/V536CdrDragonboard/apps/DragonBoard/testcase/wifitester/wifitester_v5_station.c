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
#define SCAN_TIMEOUT        5

int main(int argc, char** argv)
{
	int  retVal    = 0;
	char buf[1024] = {0};

#if 0
	int  fifoFd    = 0;
    if ((fifoFd = open(FIFO_DEV, O_WRONLY  | O_NONBLOCK)) < 0) {
        if (mkfifo(FIFO_DEV, 0666) < 0) {
            printf("mkfifo failed(%s)\n", strerror(errno));
            //return -1;
        } else {
            fifoFd = open(FIFO_DEV, O_WRONLY | O_NONBLOCK);
        }
    }
#endif
    FILE *fd = NULL;
    fd = fopen("/tmp/wifitest","wb+");

    int ret = 0;
    int cnt = 0;
    int i   = 0;
    WIFI_STA_STATUS_E   sta_status;
    WIFI_STA_STATUS_E   scan_status;
    WIFI_STA_AP_LIST_S  ap_list;
    WIFI_STA_EVENT_E    wpa_event;
    
   /* printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);
    printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);*/
    STA_ERR_PRT("=============\nwifi test 1!!!=============\n");
    ret = wifi_sta_init();
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_init fail! ret:%d\n", ret);
        ret = -1;
        goto wifi_test_exit;
    }
    STA_ERR_PRT("=============\nwifi test 2!!!=============\n");
    ret = wifi_sta_open("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_open fail! ret:%d\n", ret);
        ret = -1;
        goto wifi_test_exit;
    }
    STA_ERR_PRT("=============\nwifi test 3!!!=============\n");
    ret = wifi_sta_start("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_start fail! ret:%d\n", ret);
        ret = -1;
        goto wifi_test_exit;
    }

   /* printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);
    printf(" [FUN]:%s  [LINE]:%d  =======>>   \n",__func__,__LINE__);*/

    usleep(600 * 1000);

    ret = wifi_sta_start_scan("wlan0");
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_start_scan fail! ret:%d\n", ret);
        ret = -1;
        goto wifi_test_exit;
    }
    STA_ERR_PRT("=============\nwifi test 4!!!=============\n");
    cnt = 0;
    while (cnt < SCAN_TIMEOUT) {
        ret = wifi_sta_get_scan_status("wlan0", &scan_status);
        if (ret) {
            STA_ERR_PRT ("Do wifi_sta_get_scan_status fail! ret:%d\n", ret);
            sleep(1);
            cnt++;
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
    STA_ERR_PRT("=============\nwifi test 5!!!=============\n");
    if (cnt >= SCAN_TIMEOUT) {
        ret = -1;
        goto wifi_test_exit;
    }

    ret = wifi_sta_get_scan_results("wlan0", &ap_list);
    if (ret) {
        STA_ERR_PRT ("Do wifi_sta_get_scan_results fail! ret:%d\n", ret);
        ret = -1;
        goto wifi_test_exit;
    }
    STA_ERR_PRT("=============\nwifi test 6!!!=============\n");
    ret = 0;
wifi_test_exit:
    if (ret || ap_list.ap_list_num < 1)
	{
		STA_ERR_PRT ("\nDo wifi_sta_get_scan_results fail! ret:%d\n", ret);
		fwrite("F[WIFI]:FAIL",15,1,fd);
		STA_ERR_PRT("=============\nwifi test failed!!!=============\n");
    }
   else
    {
        if (ap_list.ap_list_num >= 1) {
            ret = snprintf(buf, sizeof(buf)-1, "P[WIFI] DB(%d) %s", ap_list.ap_list[0].db, ap_list.ap_list[0].ssid);
            if (ret <= 0) {
                STA_ERR_PRT ("Do snprintf CMD_WPA_RUN fail! ret:%d\n", ret);
            }
            fwrite(buf,50,1,fd);
            STA_DB_PRT ("\n buf:%s \n", buf);
        }
        STA_ERR_PRT("\n=============wifi test success!!!=============\n");
    }
    fclose(fd);
    wifi_sta_stop("wlan0");

    wifi_sta_close("wlan0");

    wifi_sta_exit();
    return 0;
}


