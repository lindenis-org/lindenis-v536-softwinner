#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "minigui/common.h"
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/ctrl/edit.h>
#include <mm_common.h>
#include "mm_comm_video.h"
#include <vo/hwdisplay.h>
#include <mpi_sys.h>
#include <mpi_vo.h>
#include "lua/lua_config_parser.h"
#include <cutils/memory.h>
#include <utils/plat_log.h>

#define TAG "MiniGUI"
#include <dragonboard/dragonboard.h>

using namespace std;

#define FBIO_CACHE_SYNC         0x4630

#define FILE_EXIST(PATH)   (access(PATH, F_OK) == 0)
#define FILE_READABLE(PATH)   (access(PATH, R_OK) == 0)
#define TEST_OK "Test OK"
#define TEST_FAIL "Test fail"

#define LCDR   107
#define LCDG   108
#define LCDB   109
#define REFRESH_TIMER  110

// whnd ID define
enum{
    ID_TF = 100,
    ID_CPU,
#ifdef WIFI_TEST
    ID_WIFI,
#endif
#ifdef GPS_TEST
    ID_GPS,
#endif
#ifdef GSENSOR_TEST
    ID_GSENSOR,
#endif
    ID_KEY,
    ID_RTC,
    ID_DDR,
    ID_NOR,
    ID_MIC_SPK,
    ID_CSI,
    ID_RESULT,
};


#define PATH_LEN 50
#define TEST_FOR_QC 1
#define X_start 0
#define Y_start 0

#define CHECK_OK 0
#define CHECK_FAIL 1

#define TEST_KEY_BIT	0xFFFFFFEF
#define TEST_TP_BIT		0xFFFFDFFF
#define TEST_ETHERNET_BIT	0xFFFFBFFF
#define TEST_RTC_BIT	0xFFFFFEFF
// content that read from fifo will display on LCD
static char tf_str[PATH_LEN];
static char cpu_str[PATH_LEN];
#ifdef WIFI_TEST
static char wifi_str[PATH_LEN];
#endif
#ifdef GPS_TEST
static char gps_str[PATH_LEN];
#endif
#ifdef GSENSOR_TEST
static char gsensor_str[PATH_LEN];
#endif
static char key_str[PATH_LEN];
static char rtc_str[PATH_LEN];
static char ddr_str[PATH_LEN];
static char nor_str[PATH_LEN];
static char spk_mic_str[PATH_LEN];
static char csi_str[PATH_LEN];

int  testTf = 1;
char testTfPath[PATH_LEN] = "/usr/bin/tftester";    // if it can NOT be found, use /system/bin/ by system()
int  testCPU = 1;
char testCPUPatth[PATH_LEN] = "/usr/bin/CPUtest";
#ifdef WIFI_TEST
int  testWifi = 1;
char testWifiPath[PATH_LEN] = "/usr/bin/wifitester";
#endif
#ifdef GPS_TEST
int  testGPS = 1;
char testGPSPath[PATH_LEN] = "/usr/bin/gpstester";
#endif
#ifdef GSENSOR_TEST
int  testGSENSOR = 1;
char testGSENSORPath[PATH_LEN] = "/usr/bin/gsensortester";
#endif
int  testKey = 1;
char testKeyPath[PATH_LEN] = "/usr/bin/keytester";
int  testRtc = 1;
char testRtcPath[PATH_LEN] = "/usr/bin/rtctester";
int  testDdr = 1;
char testDdrPath[PATH_LEN] = "/usr/bin/ddrtester";
int  testNor = 1;
char testNorPath[PATH_LEN] = "/usr/bin/nortester";
int  testSpk_MIC = 1;
char testSpk_MICPath[PATH_LEN] = "/usr/bin/mictester";
int  testCSI = 1;
char testCSIPath[PATH_LEN] = "/usr/bin/virvi2vo";
char testCSIConfPath[PATH_LEN] = "/usr/bin/sample_virvi2volcd.conf";

static int lcdWidth, lcdHeight;
static int chipType;            // 0-V3; 1-A20
PLOGFONT mLogFont;
static char MainWin_Title[200]={0};
static int IsResultOver = 0;
int textout_i = 0;

bool spk_micfinish_flag = false;
bool csifinish_flag = false;
bool run_csi_flag = false;

#define y_step  35
#define m_height 50

int getoneline(const char *path, char *buf, int size)
{
    if (buf == NULL || size <= 0) {
        db_error("buf == NULL or size <= 0");
        return -1;
    }

    if (!FILE_EXIST(path) || !FILE_READABLE(path)) {
        //db_error("file not exist or can not access");
        return -1;
    }

    FILE *fp = NULL;

    fp = fopen(path, "r");

    if (fp == NULL) {
        db_error("open file failed, %s", strerror(errno));
        return -1;
    }

    if (fgets(buf, size, fp) != NULL) {
        buf[size-1] = '\0';
    } else {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    return 0;
}

static long int  HelloWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int keyVal;
    static clock_t timestamp1, timestamp2;
    static HWND hHandTester, hAutoTester,hMicBar;
    static HWND hwnd_tf,hwnd_CPU,hwnd_key;
    static HWND hwnd_rtc,hwnd_ddr,hwnd_nor;
#ifdef WIFI_TEST
    static HWND hwnd_wifi;
#endif
#ifdef GPS_TEST
    static HWND hwnd_gps;
#endif
#ifdef GSENSOR_TEST
    static HWND hwnd_gsensor;
#endif
	static HWND hwnd_CSI,hwnd_mic_spk;
    static HWND hwnd_lcdr, hwnd_lcdg, hwnd_lcdb;
	static HWND hwnd_result;
    static HWND tmp;
	int index_x = 0;
	int index_y = 0;
	int index_y1 = 0;
	HDC hdc;
	char test_out[4][20]={"Test Waiting","Test Waiting.","Test Waiting..","Test Waiting..."};

    switch (message) {
		
        case MSG_CREATE:
            aloge("MSG_CREATE============");
            if (testTf) {
                hwnd_tf = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_TF,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_tf, COLOR_blue);
				index_y++;
				aloge("SetWindowBkColor tf");
            }
            if (testCPU) {
                hwnd_CPU = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_CPU,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_CPU, COLOR_blue);
				index_y++;
            }
#ifdef WIFI_TEST
            if (testWifi) {
                hwnd_wifi = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_WIFI,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_wifi, COLOR_blue);
				index_y++;
            }
#endif

#ifdef GPS_TEST
            if (testGPS) {
                hwnd_wifi = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_GPS,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_gps, COLOR_blue);
                index_y++;
            }
#endif

#if 0
            if (testGSENSOR) {
                hwnd_gsensor = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_GSENSOR,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_gsensor, COLOR_blue);
                index_y++;
            }
#endif

            if (testKey){
                hwnd_key = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_KEY,
                          X_start,  index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_key, COLOR_blue);
				index_y++;
            }

            if (testRtc) {
                hwnd_rtc = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_RTC,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_rtc, COLOR_blue);
				index_y++;
            }

            if (testDdr) {
                hwnd_ddr = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_DDR,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_ddr, COLOR_blue);
				index_y++;
            }

            if (testNor) {
                hwnd_nor = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_NOR,
                        X_start, index_y*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_nor, COLOR_blue);
				index_y++;
            }

			if(testSpk_MIC)
			{
			    hwnd_mic_spk= CreateWindow(CTRL_STATIC, NULL,
                       WS_CHILD | WS_VISIBLE, ID_MIC_SPK,
                       lcdWidth/2 + 5, index_y1*y_step, (lcdWidth - 10)/2, m_height,
                       hWnd, 0);
                SetWindowBkColor(hwnd_mic_spk, COLOR_blue);
                index_y1++;
			}

			if(testCSI)
			{
			     hwnd_CSI = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_CSI,
						lcdWidth/2 + 5, index_y1*y_step, (lcdWidth - 10)/2, m_height,
						hWnd, 0);
				SetWindowBkColor(hwnd_CSI, COLOR_blue);
				index_y1++;
			}

#ifdef GSENSOR_TEST
            if (testGSENSOR) {
                hwnd_gsensor = CreateWindow(CTRL_STATIC, NULL,
                        WS_CHILD | WS_VISIBLE, ID_GSENSOR,
                        lcdWidth/2 + 5, index_y1*y_step, (lcdWidth - 10)/2, m_height,
                        hWnd, 0);
                SetWindowBkColor(hwnd_gsensor, COLOR_blue);
                index_y1++;
            }
#endif

			index_y = index_y > index_y1?index_y:index_y1;
			db_error("index_y %d",index_y);
#if 1
			hwnd_lcdr = CreateWindow(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE, LCDR,
                   0, index_y*y_step+40, lcdWidth/3, m_height,     // make LCD_R/G/B height = 10
                    hWnd, 0);
            SetWindowBkColor(hwnd_lcdr, COLOR_red);
            hwnd_lcdg = CreateWindow(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE, LCDG,
                    lcdWidth/3, index_y*y_step+40, lcdWidth/3, m_height,
                    hWnd, 0);
            SetWindowBkColor(hwnd_lcdg, COLOR_green);
            hwnd_lcdb = CreateWindow(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE, LCDB,
                   lcdWidth*2/3-1, index_y*y_step+40, lcdWidth/3, m_height,
                    hWnd, 0);
            SetWindowBkColor(hwnd_lcdb, COLOR_blue);
#endif
#if 0
			hwnd_result = CreateWindow(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE |WS_TABSTOP, ID_RESULT,
                    lcdWidth/3+lcdWidth/12, index_y*y_step+100, lcdWidth/3, m_height,
                    hWnd, 0);
			
			hdc = GetDC(hwnd_result);
			SetWindowBkColor(hwnd_result, RGBA2Pixel(hdc, 0xaf, 0xaf, 0xff, 0x00));
#endif
            SetTimer(hWnd, REFRESH_TIMER, 100);       // set timer 0.2s(20), 1s(100)
            break;

        case MSG_TIMER:
            // HandTester
            if (testTf) {
                SetWindowFont(GetDlgItem(hWnd, ID_TF), mLogFont);
                if (*tf_str == 'P') {
                    SetWindowBkColor(hwnd_tf, PIXEL_green);     // green means pass
                    SetDlgItemText(hWnd, ID_TF, "[TF] PASS");
                } else if (*tf_str == 'F') {
                    SetWindowBkColor(hwnd_tf, PIXEL_red);
                    SetDlgItemText(hWnd, ID_TF, "[TF] FAIL");   // red means fail
                } else {
                    SetWindowBkColor(hwnd_tf, PIXEL_cyan);      // cyan means wait
                    SetDlgItemText(hWnd, ID_TF, "[TF] waiting");
                }
            }

            if (testCPU) {
				SetWindowFont(GetDlgItem(hWnd, ID_CPU), mLogFont);
                if (*cpu_str == 'P') {
                    SetWindowBkColor(hwnd_CPU, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_CPU, "[CPU] PASS");
                } else if (*cpu_str == 'F') {
                    SetWindowBkColor(hwnd_CPU, PIXEL_red);
                    SetDlgItemText(hWnd, ID_CPU, "[CPU] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_CPU, PIXEL_cyan);     // cyan means wait
                    SetDlgItemText(hWnd, ID_CPU, "[CPU] waiting");
                }
            }
#ifdef WIFI_TEST
            if (testWifi) {
				SetWindowFont(GetDlgItem(hWnd, ID_WIFI), mLogFont);
                if (*wifi_str == 'P') {
                    SetWindowBkColor(hwnd_wifi, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_WIFI, wifi_str+1);
                } else if (*wifi_str == 'F') {
                    SetWindowBkColor(hwnd_wifi, PIXEL_red);
                    SetDlgItemText(hWnd, ID_WIFI, "[WIFI] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_wifi, PIXEL_cyan);     // cyan means wait
                    SetDlgItemText(hWnd, ID_WIFI, "[WIFI] waiting");
                }
            }
#endif

#ifdef GPS_TEST
            if (testGPS) {
                SetWindowFont(GetDlgItem(hWnd, ID_GPS), mLogFont);
                if (*gps_str == 'P') {
                    SetWindowBkColor(hwnd_gps, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_GPS, gps_str+1);
                } else if (*gps_str == 'F') {
                    SetWindowBkColor(hwnd_gps, PIXEL_red);
                    SetDlgItemText(hWnd, ID_GPS, "[GPS] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_gps, PIXEL_cyan);     // cyan means wait
                    SetDlgItemText(hWnd, ID_GPS, "[GPS] waiting");
                }
            }
#endif

#ifdef GSENSOR_TEST
            if (testGSENSOR) {
                SetWindowFont(GetDlgItem(hWnd, ID_GSENSOR), mLogFont);
                if (*gsensor_str == 'P') {
                    SetWindowBkColor(hwnd_gsensor, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_GSENSOR, gsensor_str+1);
                } else if (*gsensor_str == 'F') {
                    SetWindowBkColor(hwnd_gsensor, PIXEL_red);
                    SetDlgItemText(hWnd, ID_GSENSOR, "[GSENSOR] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_gsensor, PIXEL_cyan);     // cyan means wait
                    SetDlgItemText(hWnd, ID_GSENSOR, "[GSENSOR] waiting");
                }
            }
#endif
            if (testKey) {
                SetWindowFont(GetDlgItem(hWnd, ID_KEY), mLogFont);
                    if (*key_str == 'P') {
                    SetWindowBkColor(hwnd_key, PIXEL_green);
                    SetDlgItemText(hWnd, ID_KEY, "[KEY] PASS");
                } else if (*key_str == 'F') {
                    SetWindowBkColor(hwnd_key, PIXEL_red);
                    SetDlgItemText(hWnd, ID_KEY, "[KEY] FAIL");
                } else {
                    SetWindowBkColor(hwnd_key, PIXEL_cyan);
                    SetDlgItemText(hWnd, ID_KEY, "[KEY] waiting");
                }
            }

            if (testRtc) {
				SetWindowFont(GetDlgItem(hWnd, ID_RTC), mLogFont);
                if (*rtc_str == 'P') {
                    SetWindowBkColor(hwnd_rtc, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_RTC, "[RTC] PASS");
                } else if (*rtc_str == 'F') {
                    SetWindowBkColor(hwnd_rtc, PIXEL_red);
                    SetDlgItemText(hWnd, ID_RTC, "[RTC] FAIL");    // red means fail
                } else {
                    SetWindowBkColor(hwnd_rtc, PIXEL_cyan);     // cyan means wait
                    SetDlgItemText(hWnd, ID_RTC, "[RTC] waiting");
                }
            }

            if (testDdr) {
                SetWindowFont(GetDlgItem(hWnd, ID_DDR), mLogFont);
                if (*ddr_str == 'P') {
                    SetWindowBkColor(hwnd_ddr, PIXEL_green);
                    SetDlgItemText(hWnd, ID_DDR, "[DDR] PASS");
                } else if (*ddr_str == 'F') {
                    SetWindowBkColor(hwnd_ddr, PIXEL_red);
                    SetDlgItemText(hWnd, ID_DDR, "[DDR] FAIL");
              	} else {
                    SetWindowBkColor(hwnd_ddr, PIXEL_cyan);
                    SetDlgItemText(hWnd, ID_DDR, "[DDR] waiting");
                }
            }

            if (testNor) {
                SetWindowFont(GetDlgItem(hWnd, ID_NOR), mLogFont);
                if (*nor_str == 'P') {
                    SetWindowBkColor(hwnd_nor, PIXEL_green);
                    SetDlgItemText(hWnd, ID_NOR, "[NOR] PASS");
                } else if (*nor_str == 'F') {
                    SetWindowBkColor(hwnd_nor, PIXEL_red);
                    SetDlgItemText(hWnd, ID_NOR, "[NOR] FAIL");
                } else {
                    SetWindowBkColor(hwnd_nor, PIXEL_cyan);
                    SetDlgItemText(hWnd, ID_NOR, "[NOR] waiting");
                }
            }

            if(testSpk_MIC){
                SetWindowFont(GetDlgItem(hWnd, ID_MIC_SPK), mLogFont);
                if (*spk_mic_str == 'P') {
                    SetWindowBkColor(hwnd_mic_spk, PIXEL_green);    // green means pass
                    SetDlgItemText(hWnd, ID_MIC_SPK, "[MIC_SPK] PASS");
                } else if (*spk_mic_str == 'F') {
                    SetWindowBkColor(hwnd_mic_spk, PIXEL_red);
                    SetDlgItemText(hWnd, ID_MIC_SPK, "[MIC_SPK] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_mic_spk, PIXEL_cyan);
                    SetDlgItemText(hWnd, ID_MIC_SPK, "[MIC_SPK] waiting");
                }
            }

            if (testCSI) {
                SetWindowFont(GetDlgItem(hWnd, ID_CSI), mLogFont);
                if (*csi_str == 'P') {
                    SetWindowBkColor(hwnd_CSI, PIXEL_green);	// green means pass
                    SetDlgItemText(hWnd, ID_CSI, "[CSI] PASS");
                } else if (*csi_str == 'F') {
                    SetWindowBkColor(hwnd_CSI, PIXEL_red);
                    SetDlgItemText(hWnd, ID_CSI, "[CSI] FAIL"); // red means fail
                } else {
                    SetWindowBkColor(hwnd_CSI, PIXEL_cyan); 	// cyan means wait
                    SetDlgItemText(hWnd, ID_CSI, "[CSI] waiting");
                }
            }

            if(IsResultOver)
            {
                SetWindowFont(GetDlgItem(hWnd, ID_RESULT), mLogFont);
                //hdc = GetDC(GetDlgItem(hWnd, ID_RESULT));
                //SetPenColor(hdc, COLOR_green);
                SetDlgItemText(hWnd, ID_RESULT, "Test Over!!!!");
            }
            else
            {
                SetWindowFont(GetDlgItem(hWnd, ID_RESULT), mLogFont);
                //hdc = GetDC(GetDlgItem(hWnd, ID_RESULT));
                //SetPenColor(hdc, COLOR_red);
                if(textout_i < 4)
                {
//                    SetDlgItemText(hWnd, ID_RESULT, test_out[textout_i++]);
                    //printf("Text out[%d]:%s\n",i,test_out[i]);
                }
                else{
                    textout_i = 0;
                }
            }

        break;
        case MSG_KEYDOWN:
            keyVal = LOWORD(wParam);
            break;
        case MSG_KEYUP:
            keyVal = LOWORD(wParam);
            switch (keyVal) {
                case 0x84:
                    timestamp2 = clock() / CLOCKS_PER_SEC;
                    if ( timestamp2 - timestamp1 > 2) { // long press >= 3s
                        // strlcopy(key_str, "P[KEY] reboot...", 20);
                        // sleep(5);                        // sleep 5s to display string "reboot..."
                        // system("reboot");        // NO cmd "poweroff"

                        //android_reboot(ANDROID_RB_POWEROFF, 0, 0);
                    }
            }
            break;
#if 1
		case MSG_PAINT:
		    aloge("MSG_PAINT");
            hdc = BeginPaint (hWnd);
            TextOut (hdc, 480, lcdHeight, "Hello AllwinnerTech!");
            EndPaint (hWnd, hdc);
            aloge("MSG_PAINT");
			break;
#endif
        case MSG_CLOSE:
            KillTimer(hWnd, REFRESH_TIMER);
            DestroyAllControls(hWnd);
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}


void* runTfthread(void *argv)
{
#ifdef DRAGONBOARD_TEST
    alogd("************runTfthread start************");
    system("/tmp/tftester");
    sleep(2);
    aloge("exit tf tester");
    alogd("************runTfthread end************");
    pthread_exit(NULL);
#else
    alogd("************runTfthread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testTfPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/tftester");
    sleep(2);
    aloge("exit tf tester");
    alogd("************runTfthread end************");
    pthread_exit(NULL);
#endif
}

void* tfThreadRead(void* argv)          // test tf card
{
    alogd("************tfThreadRead start************");
    int tfFd, retVal, fifoFd;
    static char m_strRes[128];
    if ((fifoFd = open(FIFO_TF_DEV, O_RDONLY)) < 0)
    {
       if ((retVal = mkfifo(FIFO_TF_DEV, 0666)) < 0)
       {
           strlcpy(tf_str, "F", 30);
           return NULL;
       }
       else
       {
           fifoFd = open(FIFO_TF_DEV, O_RDONLY);
       }
    }

    while (1)
    {
       read(fifoFd, tf_str, 30);
       sleep(2);
    }
    close(fifoFd);
    alogd("************tfThreadRead end************");
    pthread_exit(NULL);
}

void* CPUThreadRead(void* argv)
{
    alogd("************CPUThreadRead exit************");
    int  retVal, fifoFd,cpufd;
    char buf[4096] = {0};
    char cpuHardware[10];
    char *cpuHardwarePtr;
    cpuHardwarePtr = cpuHardware;
    if ((cpufd = open("/proc/cpuinfo", O_RDONLY)) > 0) {
        int cnt = read(cpufd, buf, 4096);
        if (cnt != -1) {
            char *cpuInfoPtr = strstr(buf, "sun");
            while ((*cpuHardwarePtr++ = *cpuInfoPtr++) != '\n');
            *--cpuHardwarePtr = '\0';
        }
        aloge("Hardware: %s\n", cpuHardware);
        sprintf(cpu_str, "P[CPU] PASS");
        aloge("cpu read success");
    } else {
        sprintf(cpu_str, "F[CPU]:FAIL");
        aloge("cpu read failed");
    }
    pthread_exit(NULL);
    alogd("************CPUThreadRead end************");
    return NULL;
}

#ifdef WIFI_TEST
void* runWifiThread(void* argv)
{
#ifdef DRAGONBOARD_TEST
    alogd("************runWifiThread start************");
    system("/tmp/wifitester");
    sleep(2);
    alogd("************runWifiThread end************");
    return NULL;
#else
    alogd("************runWifiThread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testWifiPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/wifitester");
    sleep(2);
    aloge("exit wifi tester");
    alogd("************runWifiThread end************");
    pthread_exit(NULL);
#endif
}

void* wifiThreadRead(void* argv)
{
    alogd("************wifiThreadRead start************kkk");
    int fifoFd,ret;
    char hotspot[256];

    FILE *fd = NULL;
    char buf[50];
    while (1) {
        sleep(2);
        if(access(FIFO_WIFI_DEV, F_OK) == 0 ){
            fd = fopen(FIFO_WIFI_DEV,"r");
            fread(buf, 50,1,fd);
            aloge("buf %s",buf);
            if (*buf == 'F') {
                wifi_str[0] = 'F';
                aloge("wifi_str %s",wifi_str);
                break;
            } else if (*buf == 'P') {
                strncpy(wifi_str, buf, 50);
                aloge("wifi_str %s",wifi_str);
                break;
            }
        }
    }
    fclose(fd);
    alogd("************wifiThreadRead end************");
    pthread_exit(NULL);
}
#endif

#ifdef GPS_TEST
void* runGPSThread(void* argv)
{
#ifdef DRAGONBOARD_TEST
    alogd("************runGPSThread start************");
    system("/tmp/gpstester");
    sleep(2);
    alogd("************runGPSThread end************");
    return NULL;
#else
    alogd("************runGPSThread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testGPSPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/gpstester");
    sleep(2);
    aloge("exit gps tester");
    alogd("************runGPSThread end************");
    pthread_exit(NULL);
#endif
}

void* GPSThreadRead(void* argv)
{
    alogd("************GPSThreadRead start************kkk");
    int fifoFd,ret;
    char hotspot[256];

    FILE *fd = NULL;
    char buf[50];
    while (1) {
        sleep(2);
        if(access(FIFO_GPS_DEV, F_OK) == 0 ){
            fd = fopen(FIFO_GPS_DEV,"r");
            fread(buf, 50,1,fd);
            aloge("buf %s",buf);
            if (*buf == 'F') {
                gps_str[0] = 'F';
                aloge("gps_str %s",gps_str);
                break;
            } else if (*buf == 'P') {
                strncpy(gps_str, buf, 50);
                aloge("gps_str %s",gps_str);
                break;
            }
        }
    }
    fclose(fd);
    alogd("************GPSThreadRead end************");
    pthread_exit(NULL);
}
#endif

#ifdef GSENSOR_TEST
void* runGSENSORThread(void* argv)
{
#ifdef DRAGONBOARD_TEST
    alogd("************runGSENSORThread start************");
    system("/tmp/gsensortester");
    sleep(2);
    alogd("************runGSENSORThread end************");
    return NULL;
#else
    alogd("************runGSENSORThread start************");
    while (1) {
       sleep(2);
       if(run_csi_flag){
            char tmpPath[50];
            sprintf(tmpPath, "cp %s /tmp", testGSENSORPath);
            system(tmpPath);
            sleep(3);
            system("/tmp/gsensortester");
            sleep(2);
            aloge("exit gsensor tester");
            break;
       }
    }
    alogd("************runGSENSORThread end************");
    pthread_exit(NULL);
#endif
}

void* GSENSORThreadRead(void* argv)
{
    alogd("************GSENSORThreadRead start************kkk");
    int fifoFd,ret;
    char hotspot[256];

    FILE *fd = NULL;
    char buf[50];
    while (1) {
        sleep(2);
        if(access(FIFO_GSENSOR_DEV, F_OK) == 0 ){
            fd = fopen(FIFO_GSENSOR_DEV,"r");
            fread(buf, 50,1,fd);
            aloge("buf %s",buf);
            if (*buf == 'F') {
                gsensor_str[0] = 'F';
                aloge("gsensor_str %s",gsensor_str);
                break;
            } else if (*buf == 'P') {
                strncpy(gsensor_str, buf, 50);
                aloge("gsensor_str %s",gsensor_str);
                break;
            }
        }
    }
    fclose(fd);
    alogd("************GSENSORThreadRead end************");
    pthread_exit(NULL);
}
#endif

void* runKeythread(void* argv)        // need add real detect program later
{
#ifdef DRAGONBOARD_TEST
    alogd("************runKeythread start************");
    system("/tmp/keytester");
    sleep(2);
    alogd("************runKeythread end************");
    return NULL;
#else
    alogd("************runKeythread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testKeyPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/keytester");
    sleep(2);
    aloge("exit key tester");
    alogd("************runKeythread end************");
    pthread_exit(NULL);
#endif
}

void* keyThreadRead(void* argv)
{
    alogd("************keyThreadRead start************");
    int fifoFd,ret;
    char buf[50];
    if ((fifoFd = open(FIFO_KEY_DEV, O_RDONLY)) < 0)
    {
       if ((ret = mkfifo(FIFO_KEY_DEV, 0666)) < 0)
       {
           strlcpy(key_str, "F", 30);
           return NULL;
       }
       else
       {
           fifoFd = open(FIFO_KEY_DEV, O_RDONLY);
       }
    }
    while (1) {
        read(fifoFd, buf, 50);
        if (*buf == 'F') {
            sprintf(key_str,"P[KEY] FAIL");
        } else {
            sprintf(key_str,"P[KEY] PASS");
        }
        sleep(2);           // time control is done in testcase => avoid read flood in case of write-endian shutdown
    }
    close(fifoFd);
    alogd("************keyThreadRead start************");
    pthread_exit(NULL);
}


void* runRtcthread(void* argv)        // need add real detect program later
{
#ifdef DRAGONBOARD_TEST
    alogd("************runRtcthread start************");
    system("/tmp/rtctester");
    sleep(2);
    alogd("************runRtcthread end************");
    return NULL;
#else
    alogd("************runRtcthread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testRtcPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/rtctester");
    sleep(2);
    aloge("exit rtc tester");
    alogd("************runRtcthread end************");
    pthread_exit(NULL);
#endif
}

void* rtcThreadRead(void* argv)
{
    alogd("************rtcThreadRead start************");
    FILE *fd = NULL;
    char buf[50];
    while (1) {
       sleep(2);
       if(access(FIFO_RTC_DEV, F_OK) == 0 ){
           fd = fopen(FIFO_RTC_DEV,"r");
           fread(buf, 50,1,fd);
           if (*buf == 'F') {
               sprintf(rtc_str,"P[RTC] FAIL");
               aloge("write rtc fail");
               break;
           } else if (*buf == 'P'){
               sprintf(rtc_str,"P[RTC] PASS");
               aloge("write rtc pass");
               break;
           }
       }
    }
    fclose(fd);
    alogd("************rtcThreadRead end************");
    pthread_exit(NULL);
}

void* runDDRthread(void* argv)        // need add real detect program later
{
#ifdef DRAGONBOARD_TEST
    alogd("************runDDRthread start************");
    system("/tmp/ddrtester");
    sleep(2);
    alogd("************runDDRthread end************");
    return NULL;
#else
    alogd("************runDDRthread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testDdrPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/ddrtester");
    sleep(2);
    aloge("exit ddr tester");
    alogd("************runDDRthread end************");
    pthread_exit(NULL);
#endif
}

void* DDRThreadRead(void* argv)
{
    alogd("************DDRThreadRead start************");
    FILE *fd = NULL;
    char buf[50];
    while (1) {
       sleep(2);
       if(access(FIFO_DDR_DEV, F_OK) == 0 ){
           fd = fopen(FIFO_DDR_DEV,"r");
           fread(buf, 50,1,fd);
           if (*buf == 'F') {
               sprintf(ddr_str,"P[DDR] FAIL");
               aloge("write ddr fail");
               break;
           } else if (*buf == 'P'){
               sprintf(ddr_str,"P[DDR] PASS");
               aloge("write ddr pass");
               break;
           }
       }
    }
    fclose(fd);
    alogd("************DDRThreadRead end************");
    pthread_exit(NULL);
}

void* runNORthread(void* argv)        // need add real detect program later
{
#ifdef DRAGONBOARD_TEST
    alogd("************runNORthread start************");
    system("/tmp/nortester");
    sleep(2);
    alogd("************runNORthread end************");
    return NULL;
#else
    alogd("************runNORthread start************");
    char tmpPath[50];
    sprintf(tmpPath, "cp %s /tmp", testNorPath);
    system(tmpPath);
    sleep(1);
    system("/tmp/nortester");
    sleep(2);
    aloge("exit nor tester");
    alogd("************runNORthread end************");
    pthread_exit(NULL);
#endif
}

void* NORThreadRead(void* argv)
{
    alogd("************NORThreadRead start************");
    FILE *fd = NULL;
    char buf[50];
    while (1) {
       sleep(2);
       if(access(FIFO_NOR_DEV, F_OK) == 0 ){
           fd = fopen(FIFO_NOR_DEV,"r");
           fread(buf, 50,1,fd);
           if (*buf == 'F') {
               sprintf(nor_str,"P[NOR] FAIL");
               aloge("write nor fail");
               break;
           } else if (*buf == 'P'){
               sprintf(nor_str,"P[NOR] PASS");
               aloge("write nor pass");
               break;
           }
       }
    }
    fclose(fd);
    alogd("************NORThreadRead end************");
    pthread_exit(NULL);
}

/*************************MPP**************************/
void* runSpk_MICthread(void* argv)
{
    alogd("************runSpk_MICthread start************");
    char tmpPath[50];
    while(1){
        sprintf(tmpPath, "cp %s /tmp", testSpk_MICPath);
        system(tmpPath);
        sleep(1);
        system("/tmp/mictester");
        aloge("exit ao tester");
        sleep(2);
        break;
    }
    alogd("************runSpk_MICthread end************");
    pthread_exit(NULL);
}

void* Spk_MICThreadRead(void* argv)
{
    alogd("************Spk_MICThreadRead start************");
    FILE *fd = NULL;
    char buf[128];
    while (1) {
        sleep(2);
        if(access(FIFO_SPK_MIC_DEV, F_OK) == 0 ){
            fd = fopen(FIFO_SPK_MIC_DEV,"r");
            fread(buf, 50,1,fd);
            if (*buf == 'F') {
                sprintf(spk_mic_str,"FAIL");
                aloge("write spk_mic fail");
                break;
            } else if (*buf == 'P'){
                sprintf(spk_mic_str,"PASS");
                aloge("write spk_mic pass");
                break;
            }
        }
    }
    fclose(fd);
    spk_micfinish_flag = true;
    alogd("************Spk_MICThreadRead end************");
    pthread_exit(NULL);
}

void* CSIThreadRead(void* argv)
{
    alogd("************CSIThreadRead start************");
    FILE *fd = NULL;
    char buf[50];
    while (1) {
        sleep(2);
        if(spk_micfinish_flag){
            if(access(FIFO_CSI_DEV, F_OK) == 0 ){
                fd = fopen(FIFO_CSI_DEV,"r");
                fread(buf, 50,1,fd);
                if (*buf == 'F') {
                    sprintf(csi_str,"FAIL");
                    aloge("write csi fail");
                    break;
                } else if (*buf == 'P'){
                    sprintf(csi_str,"PASS");
                    aloge("write csi pass");
                    break;
                }
            }
        }
    }
    fclose(fd);
    csifinish_flag = true;
    alogd("************CSIThreadRead end************");
    pthread_exit(NULL);
}

void* runCSIthread(void* argv)
{
   alogd("************runCSIthread start************");
   char tmpPath[50];
   while(1){
       if(spk_micfinish_flag){
           sprintf(tmpPath, "cp %s /tmp", testCSIPath);
           system(tmpPath);
           sprintf(tmpPath, "cp %s /tmp", testCSIConfPath);
           system(tmpPath);
           sleep(1);
           run_csi_flag = true;
           system("/tmp/virvi2vo -path /tmp/sample_virvi2volcd.conf");
           sleep(2);
           alogd("************run sample_virvi2vo************");
           break;
       }
       sleep(2);
   }
   alogd("************runCSIthread end************");
   return NULL;
}

static int InitLcd(void)
{
    /* lcd on/off */
    int mdisp_fd_ = -1;
    int retval = 0;
    unsigned long args[32]={0};

    printf(" FUN[%s] LINE[%d]  --->  \n", __func__, __LINE__);
    usleep(88 * 1000);
    mdisp_fd_ = open("/dev/disp", O_RDWR);
    if (mdisp_fd_ < 0) {
        printf(" FUN[%s] LINE[%d]  --->  \n", __func__, __LINE__);
        return -1;
    }

    printf(" FUN[%s] LINE[%d]  ---> mdisp_fd_:%d  \n", __func__, __LINE__, mdisp_fd_);

    args[1] = DISP_OUTPUT_TYPE_NONE;
    retval = ioctl(mdisp_fd_, DISP_DEVICE_SWITCH, args);
    if (retval < 0) {
        printf("fail to set screen off");
        close(mdisp_fd_);
        return -1;
    }

    usleep(88 * 100);

    memset(args, 0, sizeof(args));
    args[1] = DISP_OUTPUT_TYPE_LCD;
    retval = ioctl(mdisp_fd_, DISP_DEVICE_SWITCH, args);
    if (retval < 0) {
        printf("fail to set screen on");
        close(mdisp_fd_);
        return -1;
    }
    usleep(88 * 100);

    close(mdisp_fd_);
    return 0;
}

static void getScreenInfo(int *w, int *h)
{
    *w = 460;
    *h = 232;
}

int MiniGUIMain(int argc, const char **argv)
{
    HWND hMainWnd;
    MSG Msg;

    system("dd if=/dev/zero of=/dev/fb0 bs=614400 count=1");

	setenv("FB_SYNC", "1", 1);
    setenv("SCREEN_INFO", "480x640-32bpp", 1);
    int prompt_w = 640, prompt_h = 480;
    //show ui
    getScreenInfo(&prompt_w, &prompt_h);

    int y_pos = 60;//+60 是因为这个屏幕是480 高度，实际显示是360，上下都没有了60，所以y要向下偏移60
    int x_pos = 0;
    aloge("fucrrrrksdaaas....");
    lcdWidth = GetGDCapability(HDC_SCREEN, GDCAP_MAXX) + 1;
    lcdHeight = GetGDCapability(HDC_SCREEN, GDCAP_MAXY) + 1;
    aloge("LCD width & height: (%d, %d)\n", lcdWidth, lcdHeight);

#if 0
    MAINWINCREATE CreateInfo = {
        WS_VISIBLE | WS_CAPTION,
        WS_EX_NOCLOSEBOX,
        "=========== V536 cdr dragonboard ===========",
        0,
        GetSystemCursor(0),
        0,
        HWND_DESKTOP,
        HelloWinProc,
        x_pos, y_pos, x_pos+prompt_w, y_pos+prompt_h,
        COLOR_transparent,
        0, 0};
#endif

    MAINWINCREATE CreateInfo;
    CreateInfo.dwStyle= WS_VISIBLE | WS_CAPTION;
//    CreateInfo.dwExStyle = WS_EX_NOCLOSEBOX;
    CreateInfo.dwExStyle = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    CreateInfo.spCaption =  "=========================== V536 cdr dragonboard ===========================";
    CreateInfo.hMenu = 0;
     CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = HelloWinProc;
    CreateInfo.hHosting = HWND_DESKTOP;
    CreateInfo.lx = x_pos;
    CreateInfo.ty = y_pos;
    CreateInfo.rx = lcdWidth;
    CreateInfo.by = lcdHeight - y_pos;
    CreateInfo.iBkColor = COLOR_transparent;
    CreateInfo.dwAddData = 0;
    CreateInfo.dwReserved = 0;

    hMainWnd = CreateMainWindow(&CreateInfo);
    HDC hdc = GetDC(hMainWnd);
    SetWindowBkColor(hMainWnd, RGBA2Pixel(hdc, 0xaf, 0xaf, 0xff, 0x00));
    if (hMainWnd == HWND_INVALID) {
        db_msg("CreateMainWindow error");
        return -1;
    }

    ShowWindow (hMainWnd, SW_SHOWNORMAL);

    /**************SYSTREM**************/
    pthread_t tfTid,start_tfTid,cpuTid,start_cpuTid,keyTid,start_keyTid;
    pthread_t rtcTid,start_rtcTid,ddrTid,start_ddrTid,norTid,start_norTid;
#ifdef WIFI_TEST
    pthread_t wifiTid,start_wifiTid;
#endif
#ifdef GPS_TEST
    pthread_t gpsTid,start_gpsTid;
#endif
#ifdef GSENSOR_TEST
    pthread_t gsensorTid,start_gsensorTid;
#endif

    if(testTf){
        if (pthread_create(&tfTid, NULL, tfThreadRead, NULL) < 0) {
            aloge("pthread_create for tf failed");
        }
        if (pthread_create(&start_tfTid, NULL, runTfthread, NULL) < 0) {
            aloge("pthread_create for tf failed");
        }
    }

    if(testCPU){
        if (pthread_create(&cpuTid, NULL, CPUThreadRead, NULL) < 0) {
            aloge("pthread_create for CPU failed");
        }
    }

#ifdef WIFI_TEST
    if(testWifi){
        if (pthread_create(&wifiTid, NULL, wifiThreadRead, NULL) < 0) {
            aloge("pthread_create for wifi failed");
        }
        if (pthread_create(&start_wifiTid, NULL, runWifiThread, NULL) < 0) {
            aloge("pthread_create for wifi failed");
        }
    }
#endif

#ifdef GPS_TEST
    if(testGPS){
        if (pthread_create(&gpsTid, NULL, GPSThreadRead, NULL) < 0) {
            aloge("pthread_create for gps failed");
        }
        if (pthread_create(&start_gpsTid, NULL, runGPSThread, NULL) < 0) {
            aloge("pthread_create for gps failed");
        }
    }
#endif

#ifdef GSENSOR_TEST
    if(testGSENSOR){
        if (pthread_create(&gsensorTid, NULL, GSENSORThreadRead, NULL) < 0) {
            aloge("pthread_create for gsensor failed");
        }
        if (pthread_create(&start_gsensorTid, NULL, runGSENSORThread, NULL) < 0) {
            aloge("pthread_create for gsensor failed");
        }
    }
#endif

    if(testKey){
        if (pthread_create(&keyTid, NULL, keyThreadRead, NULL) < 0) {
            aloge("pthread_create for key failed");
        }
        if (pthread_create(&start_keyTid, NULL, runKeythread, NULL) < 0) {
            aloge("pthread_create for key failed");
        }
    }

    if(testRtc){
        if (pthread_create(&rtcTid, NULL, rtcThreadRead, NULL) < 0) {
            aloge("pthread_create for rtc failed");
        }
        if (pthread_create(&start_rtcTid, NULL, runRtcthread, NULL) < 0) {
            aloge("pthread_create for rtc failed");
        }
    }

    if(testDdr){
        if (pthread_create(&ddrTid, NULL, DDRThreadRead, NULL) < 0) {
            aloge("pthread_create for ddr failed");
        }
        if (pthread_create(&start_ddrTid, NULL, runDDRthread, NULL) < 0) {
            aloge("pthread_create for ddr failed");
        }
    }

    if(testNor){
        if (pthread_create(&ddrTid, NULL, NORThreadRead, NULL) < 0) {
            aloge("pthread_create for nor failed");
        }
        if (pthread_create(&start_ddrTid, NULL, runNORthread, NULL) < 0) {
            aloge("pthread_create for nor failed");
        }
    }

    /****************MPP****************/
    pthread_t spk_mic_Tid,start_spkmicTid,csiTid,start_csiTid;
    if(testSpk_MIC) {
        if (pthread_create(&spk_mic_Tid, NULL, Spk_MICThreadRead, NULL) < 0) {
            aloge("pthread_create for spk_mic failed");
        }
        if (pthread_create(&start_spkmicTid, NULL, runSpk_MICthread, NULL) < 0) {
            aloge("pthread_create for spk_mic failed");
        }
    }

    if(testCSI){
       if (pthread_create(&csiTid, NULL, CSIThreadRead, NULL) < 0) {
           aloge("pthread_create for csi failed");
       }
       if (pthread_create(&start_csiTid, NULL, runCSIthread, NULL) < 0) {
           aloge("pthread_create for csi failed");
       }
    }

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    MainWindowCleanup(hMainWnd);
    aloge("bye dragonboard");
    return 0;
}

#ifndef _MGRM_PROCESSES
#include <minigui/dti.c>
#endif
