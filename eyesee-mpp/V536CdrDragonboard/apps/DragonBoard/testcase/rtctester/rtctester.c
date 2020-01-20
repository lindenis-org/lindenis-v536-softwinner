#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#define TAG "rtctester"
#include <dragonboard/dragonboard.h>

#define RTC_DEV   "/dev/rtc0"

int GetRTCTime(int fd, struct rtc_time* prtc)
{
    int retVal = ioctl(fd, RTC_RD_TIME, prtc);
    if (retVal < 0)
    {
        db_error("get rtc time failed(%s)\n", strerror(errno));
        return retVal;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int retVal, rtcFd, fifoFd;
    char str_pass[] = "P[RTC] PASS";
    char str_fail[] = "F[RTC]:FAIL";
    char str_wait[] = "W[RTC] waiting";

    if ((fifoFd = open(FIFO_RTC_DEV, O_WRONLY)) < 0)
    {
        if (mkfifo(FIFO_RTC_DEV, 0666) < 0)
        {
            db_error("mkfifo failed(%s)\n", strerror(errno));
            return -1;
        }
        else
        {
            fifoFd = open(FIFO_RTC_DEV, O_WRONLY);
        }
    }

    if ((rtcFd = open(RTC_DEV, O_RDWR)) < 0)
    {
        write(fifoFd, str_fail, strlen(str_fail));
        return -1;
    }
    else
    {
        write(fifoFd, str_wait, strlen(str_wait));
    }
    db_msg("open rtc dev OK\n");            // use debug level [db_msg] when open dev success, use [db_debug] for get data

    static struct rtc_time last_rtc_tm;
    memset(&last_rtc_tm, 0, sizeof(last_rtc_tm));
    int test = 0;
    while (1)
    {
        char data_buf[30] = {0};
        struct rtc_time rtc_tm;
        memset(&rtc_tm, 0, sizeof(rtc_tm));
        static int count = 0;

        retVal = GetRTCTime(rtcFd, &rtc_tm);
        if (retVal == 0)
        {
            if(count > 5)
            {
                write(fifoFd, str_fail, strlen(str_fail));
                sleep(1);
//                goto exit;
            }

            if(last_rtc_tm.tm_hour != rtc_tm.tm_hour || last_rtc_tm.tm_min != rtc_tm.tm_min || last_rtc_tm.tm_sec != rtc_tm.tm_sec)
            {
                last_rtc_tm = rtc_tm;
                count = 0;
                test++;
            }
            else
                count++;
            db_error("last_rtc:tm_hour %d,tm_min %d,tm_sec %d",
                    last_rtc_tm.tm_hour,last_rtc_tm.tm_min,last_rtc_tm.tm_sec);
            db_error("rtc_tm:tm_hour %d,tm_min %d,tm_sec %d",
                    rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_sec);
            if(test >= 5){
                db_error("rtc test success");
                sprintf(data_buf, "P[RTC] %.2d:%.2d:%.2d", rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
//                write(fifoFd, data_buf, strlen(data_buf));
                write(fifoFd, str_pass, strlen(str_pass));
//                goto exit;
            }
        }
        else
        {
            db_error("read rtc fail");
            write(fifoFd, str_fail, strlen(str_fail));
        }
        sleep(1);
    }
exit:
    close(fifoFd);
    close(rtcFd);

    return 0;
}
