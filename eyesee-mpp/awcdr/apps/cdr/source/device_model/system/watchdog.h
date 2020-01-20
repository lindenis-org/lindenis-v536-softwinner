/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file watchdog.h
 * @brief 看门狗用户程序
 *
 * 打开系统看门狗设备，定时喂狗，如应用出现异常，如卡死，崩溃，意外退出
 * 将触发看门狗导致系统自动重启
 *
 * @author id:826
 * @date 2017-02-04
 *
 * @verbatim
    History:
   @endverbatim
 */

#pragma once

#include "common/singleton.h"

#include <pthread.h>
#include <stdint.h>

namespace EyeseeLinux {

class WatchDog:
	 public Singleton<WatchDog>
{
	friend class Singleton<WatchDog>;
	public:
        WatchDog(int feed_time = 3);

        ~WatchDog();

        int8_t RunWatchDog();

        int8_t StopWatchDog();

		int SetFeedTime();

        static void *WatchDogThread(void *context);

	private:
        int8_t FeedDog();

    private:
        int dog_fd_;
        int feed_time_;
        int timeout_; // min:1s, max:16s
        pthread_t watchdog_tid_;
		unsigned int m_FeedCount0, m_FeedCount1;
		bool m_Reset;
		pthread_mutex_t m_lock;
};

}
