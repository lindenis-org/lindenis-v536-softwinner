/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file ethernet_controller.h
 * @brief 以太网连接控制模块
 *
 *  该模块提供上层应用对以太网的控制管理，状态查询等。如以太网
 *  的使能与禁用。查询网络连接状态等等。
 *
 * @author id:
 * @date 2016-7-14
 *
 * @verbatim
    History:
   @endverbatim
 */


#pragma once

#include "common/subject.h"

#include <string>
#include <list>
#include <stdio.h>

namespace EyeseeLinux {


/**
 * @brief 以太网状态
 */
typedef enum {
    ETHER_DISCONNECT = 0,   /**< 以太网断开连接 */
    ETHER_CONNECT_LAN,      /**< 以太网连接局域网 */
    ETHER_CONNECT_INTERNET, /**< 以太网连接internet网 */
    ETHER_ENABLE,           /**< 使能以太网 */
    ETHER_DISABLE,          /**< 禁用以太网 */
} EthConnectStat;


class EtherController
    : public ISubjectWrap(EtherController)
{
    public:
        EtherController();

        ~EtherController();

        int GetEtherConnectStatus(EthConnectStat &ether_state) const;
        static void *EtherLoopThread(void *context);

    private:
        pthread_t ether_loop_thread_id_;
};

} /* EyeseeLinux */
