#ifndef _DragonBoard_H_
#define _DragonBoard_H_

#include <linux/rtc.h>
#include <cutils/log.h>

struct fifo_param {
    char type;                  // A: autotester, H: handtester
    char name[10];              // tester-name
    char result;                // F: Fail, P: Pass, W: waiting
    union {
        float acc[3];           // acc[0-2]: x,y,z
        struct rtc_time rtc;        // rtc[0-5]: Y:M:D:H:M:S
        int MemTotal;           // DRAM capacity
    }val;
};
typedef struct fifo_param fifo_param_t;

//#define WIFI_TEST
#define GSENSOR_TEST
//#define GPS_TEST

/**************SYSTREM**************/
#define FIFO_TF_DEV   "/tmp/fifo_tf"
#define FIFO_CPU_DEV  "/tmp/fifo_cpu"
#ifdef WIFI_TEST
#define FIFO_WIFI_DEV "/tmp/wifitest"
#endif
#ifdef GPS_TEST
#define FIFO_GPS_DEV  "/tmp/gpstest"
#endif
#ifdef GSENSOR_TEST
#define FIFO_GSENSOR_DEV  "/tmp/gsensortest"
#endif
#define FIFO_KEY_DEV  "/tmp/keytest"
#define FIFO_RTC_DEV  "/tmp/rtctest"
#define FIFO_DDR_DEV  "/tmp/ddrtest"
#define FIFO_NOR_DEV  "/tmp/nortest"

/***************MPP*****************/
#define FIFO_SPK_MIC_DEV "/tmp/spk_mictest"
#define FIFO_CSI_DEV     "/tmp/csitest"

#include <log/log.h>
#define DB_ERROR
#define DB_WARN
#define SCREENT_W 640
#define SCREENT_H 480
#ifdef DB_FATAL
#define db_fatal(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_FATAL, fmt, ##arg); \
    } while(0)
#else
#define db_fatal(fmt, arg...)
#endif

#ifdef DB_ERROR
#define db_error(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_ERROR, fmt, ##arg); \
    } while(0)
#else
#define db_error(fmt, arg...)
#endif

#ifdef DB_WARN
#define db_warn(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_WARN, fmt, ##arg); \
    } while(0)
#else
#define db_warn(fmt, arg...)
#endif

#ifdef DB_INFO
#define db_info(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_INFO, fmt, ##arg); \
    } while(0)
#else
#define db_info(fmt, arg...)
#endif

#ifdef DB_DEBUG
#define db_debug(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_INFO, fmt, ##arg); \
    } while(0)
#else
#define db_debug(fmt, arg...)
#endif

#ifdef DB_MSG
#define db_msg(fmt, arg...) \
    do { \
        GLOG_PRINT(_GLOG_INFO, fmt, ##arg); \
    } while(0)
#else
#define db_msg(fmt, arg...)
#endif

#endif /* _DragonBoard_H_ */
