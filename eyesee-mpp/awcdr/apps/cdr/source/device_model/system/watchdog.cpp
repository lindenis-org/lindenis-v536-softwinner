/**********************************************************
* Copyright (C), 2015, AllwinnerTech. Co., Ltd.  *
***********************************************************/
/**
 * @file watchdog.cpp
 * @brief 看门狗用户程序
 * @author id:826
 * @date 2017-02-04
 * @version v0.3
 * @see watchdog.h
 * @verbatim
 *  History:
 * @endverbatim
 */

#include "watchdog.h"
#include "common/app_log.h"
#include "common/thread.h"

#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#undef LOG_TAG
#define LOG_TAG "WatchDog"

using namespace EyeseeLinux;

WatchDog::WatchDog(int feed_time)
    : dog_fd_(-1)
    , feed_time_(feed_time)
    , timeout_(10)
    , watchdog_tid_(-1)
    , m_FeedCount0(0)
    , m_FeedCount1(0)
    , m_Reset(false)
{
	pthread_mutex_init(&m_lock, NULL);
}

WatchDog::~WatchDog()
{
    StopWatchDog();
	pthread_mutex_destroy(&m_lock);
}

int8_t WatchDog::RunWatchDog()
{
    int ret = -1;

    if (dog_fd_ > 0)
	{
		close(dog_fd_);
		dog_fd_ = -1;
    }
    dog_fd_ = open("/dev/watchdog", O_WRONLY);

    if (dog_fd_ < 0) {
        db_error("open watchdog failed, %s", strerror(errno));
        return -1;
    }

    int action = WDIOS_DISABLECARD;
    ret = ioctl(dog_fd_, WDIOC_SETOPTIONS, &action);
    if (ret < 0) {
        db_error("ioctl WDIOC_SETOPTIONS action:WDIOS_DISABLECARD failed, %s", strerror(errno));
		close(dog_fd_);
		dog_fd_ = -1;
        return -1;
    }

    ret = ioctl(dog_fd_, WDIOC_SETTIMEOUT, &timeout_);
    if (ret < 0) {
        db_error("ioctl WDIOC_SETTIMEOUT failed, %s", strerror(errno));
		close(dog_fd_);
		dog_fd_ = -1;
        return -1;
    }

    ret = ioctl(dog_fd_,WDIOC_GETTIMEOUT, &timeout_);
    if (ret < 0) {
        db_error("ioctl WDIOC_GETTIMEOUT failed, %s", strerror(errno));
		close(dog_fd_);
		dog_fd_ = -1;
        return -1;
    }
    db_info("recheck timeout: %d", timeout_);

    action = WDIOS_ENABLECARD;
    ret = ioctl(dog_fd_, WDIOC_SETOPTIONS, &action);
    if (ret < 0) {
        db_error("ioctl WDIOC_SETOPTIONS action:WDIOS_ENABLECARD failed, %s", strerror(errno));
		close(dog_fd_);
		dog_fd_ = -1;
        return -1;
    }

    ThreadCreate(&watchdog_tid_, NULL, WatchDog::WatchDogThread, this);

    return 0;
}

int8_t WatchDog::StopWatchDog()
{
    if (dog_fd_ < 0) {
        db_warn("watchdog is not opened, no need to stop");
        return 0;
    }

    int action = WDIOS_DISABLECARD;
    int ret = ioctl(dog_fd_, WDIOC_SETOPTIONS, &action);
    if (ret < 0) {
        db_error("ioctl WDIOC_SETOPTIONS action:WDIOS_DISABLECARD failed, %s", strerror(errno));
        return -1;
    }

    close(dog_fd_);
    dog_fd_ = -1;

    return 0;
}

int8_t WatchDog::FeedDog()
{
    if (dog_fd_ < 0) {
        db_error("feed dog failed, watchdog is not opened");
        return -1;
    }

    int ret = ioctl(dog_fd_, WDIOC_KEEPALIVE, NULL);
    if (ret < 0) {
        db_error("feed dog failed, %s", strerror(errno));
        return -1;
    }

    return 0;
}

int WatchDog::SetFeedTime()
{
	pthread_mutex_lock(&m_lock);
	if( m_FeedCount0 == 0)
	{
		m_FeedCount0 = 1;
		m_FeedCount1 = m_FeedCount0;
		m_Reset = true;
	}
	else
	{
		m_Reset = true;
	}

	pthread_mutex_unlock(&m_lock);
	return 0;
}

void *WatchDog::WatchDogThread(void *context)
{
    WatchDog *self = reinterpret_cast<WatchDog*>(context);
    bool stop_flag = false;
    int ret = 0, fd = -1;

    prctl(PR_SET_NAME, "WatchDogThread", 0, 0, 0);

    stop_flag = false;
    for (;;) {

        ret = access("/tmp/stop_watchdog", F_OK);
        if (0 == ret) {
            if (false == stop_flag) {
                if (!self->StopWatchDog()) {
                    stop_flag = true;
                    fd = open("/tmp/watchdog_exit", O_CREAT);
                    if (fd < 0) {
                        fprintf (stderr, "open /tmp/watchdog_exit file error<%d>, %s\n", errno, strerror(errno));
                    } else {
                        close(fd);
                        sync();
                    }
                }
            }

            sleep(1);
            continue;

        } else if (0 != ret && true == stop_flag) {
            if (!self->RunWatchDog()) {
                stop_flag = false;
                if (unlink("/tmp/watchdog_exit")) {
                    fprintf (stderr, "unlink /tmp/watchdog_exit file error<%d>, %s\n", errno, strerror(errno));
                }
            }
        }


		if( self->m_FeedCount0 > 0 )
		{
			pthread_mutex_lock(&self->m_lock);
			if( self->m_FeedCount1 - self->m_FeedCount0 < 10)
			{
				if(self->m_Reset)
				{
					self->m_FeedCount1 = self->m_FeedCount0;
					self->m_Reset = false;
				}
				else
				{
					self->m_FeedCount1++;
				}
				pthread_mutex_unlock(&self->m_lock);
				self->FeedDog();
				sleep(self->feed_time_);
				continue;
			}
			else
			{
				db_warn("oh no, minigui is dead, reboot system\n");
				pthread_mutex_unlock(&self->m_lock);
				sleep(self->feed_time_);
				continue;
			}
			pthread_mutex_unlock(&self->m_lock);
		}
        self->FeedDog();
        sleep(self->feed_time_); // 3s
#if 0 // TODO: handle standby mode
        if(isEnterStandby) {
            int action = WDIOS_DISABLECARD;
            ioctl(fd, WDIOC_SETOPTIONS, &action);
            break;
        }
#endif
    }

    return NULL;
}
