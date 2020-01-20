#ifndef _RTC_H_
#define _RTC_H_

#undef LOG_TAG
#define LOG_TAG "RTC"
#include "common/app_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int set_date_time(struct tm* ptm);
time_t get_date_time(struct tm **local_time);
void reset_date_time(void);

#ifdef __cplusplus
}
#endif

namespace EyeseeLinux {
typedef struct RTCTimeTest
{
    public:
        RTCTimeTest(const char *msg) {
            this->msg = strdup(msg);
            clock_gettime(CLOCK_MONOTONIC, &measureTime);
            db_msg("start %s time is %ld secs, %ld nsecs", msg, measureTime.tv_sec, measureTime.tv_nsec);
        }
        ~RTCTimeTest() {
            clock_gettime(CLOCK_MONOTONIC, &measureTime);
            db_msg("end %s time is %ld secs, %ld nsecs", msg, measureTime.tv_sec, measureTime.tv_nsec);
            free(msg);
            msg = NULL;
        }
    private:
        char *msg;
        struct timespec measureTime;
} RTCTimeTest;
}
#endif  //_RTC_H_

