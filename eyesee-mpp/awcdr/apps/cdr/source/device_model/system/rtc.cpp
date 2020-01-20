#define LOG_NDEBUG 1
#undef LOG_TAG
#define LOG_TAG "rtc.cpp"

#include "common/app_log.h"
#include "common/common_inc.h"
#include "device_model/system/rtc.h"

int rtc_set_time(const struct tm *tm_time);

int set_date_time(struct tm *ptm)
{
    time_t timep;
    struct timeval tv;
    ptm->tm_wday = 0;
    ptm->tm_yday = 0;
    ptm->tm_isdst = 0;
    timep = mktime(ptm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) < 0) {
        db_error("Set system date and time error, errno(%d)", errno);
        return -1;
    }

    time_t t = time(NULL);
    struct tm *local = localtime(&t);

    db_debug("  datetime->tm_year=%d", local->tm_year + 1900);
    db_debug("  datetime->tm_mon=%d", local->tm_mon);
    db_debug("  datetime->tm_mday=%d", local->tm_mday);
    db_debug("  datetime->tm_hour=%d", local->tm_hour);
    db_debug("  datetime->tm_min=%d", local->tm_min);
    db_debug("  datetime->tm_sec=%d", local->tm_sec);

    if(rtc_set_time(local) < 0)
		return -1;
    return 0;
}

void reset_date_time(void)
{
    struct tm tm;

    tm.tm_year = (2014 - 1900);
    tm.tm_mon = (11 - 1);
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_wday = 0;
    tm.tm_yday = 0;
    tm.tm_isdst = 0;

    set_date_time(&tm);
}

time_t get_date_time(struct tm **local_time)
{
    time_t timer;
    timer = time(NULL);
    *local_time = localtime(&timer);
    return timer;
}

int rtc_set_time(const struct tm *tm_time)
{
    int rtc_handle = -1;
    int ret = 0;
    struct rtc_time rtc_tm;
    if (tm_time == NULL) {
        return -1;
    }
    rtc_handle = open("/dev/rtc0", O_RDWR);
    if (rtc_handle < 0) {
        db_error("open /dev/rtc0 fail");
        return -1;
    }
    memset(&rtc_tm, 0, sizeof(rtc_tm));
    rtc_tm.tm_sec = tm_time->tm_sec;
    rtc_tm.tm_min = tm_time->tm_min;
    rtc_tm.tm_hour = tm_time->tm_hour;
    rtc_tm.tm_mday = tm_time->tm_mday;
    rtc_tm.tm_mon = tm_time->tm_mon;
    rtc_tm.tm_year = tm_time->tm_year;
    rtc_tm.tm_wday = tm_time->tm_wday;
    rtc_tm.tm_yday = tm_time->tm_yday;
    rtc_tm.tm_isdst = tm_time->tm_isdst;
    ret = ioctl(rtc_handle, RTC_SET_TIME, &rtc_tm);
    if (ret < 0) {
        db_error("rtc_set_time fail");
        close(rtc_handle);
        return -1;
    }
    ALOGV("rtc_set_time ok");
    close(rtc_handle);
    return 0;
}

int rtc_get_time(struct tm *tm_time)
{
    int rtc_handle;
    int ret = 0;
    struct rtc_time rtc_tm;

    rtc_handle = open("/dev/rtc0", O_RDWR);
    if (rtc_handle < 0) {
//        db_error("open /dev/rtc0 fail");
        return -1;
    }
    memset(&rtc_tm, 0, sizeof(rtc_tm));
    ret = ioctl(rtc_handle, RTC_RD_TIME, &rtc_tm);
    if (ret < 0) {
        db_error("rtc_get_time fail");
        close(rtc_handle);
        return -1;
    }
    tm_time->tm_sec = rtc_tm.tm_sec;
    tm_time->tm_min = rtc_tm.tm_min;
    tm_time->tm_hour = rtc_tm.tm_hour;
    tm_time->tm_mday = rtc_tm.tm_mday;
    tm_time->tm_mon = rtc_tm.tm_mon;
    tm_time->tm_year = rtc_tm.tm_year;
    tm_time->tm_wday = rtc_tm.tm_wday;
    tm_time->tm_yday = rtc_tm.tm_yday;
    tm_time->tm_isdst = rtc_tm.tm_isdst;
    close(rtc_handle);
    return 0;
}
