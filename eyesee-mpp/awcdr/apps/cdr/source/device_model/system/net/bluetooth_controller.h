/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file bluetooth_controller.h
 * @brief bluetooth连接控制模块
 *
 *  该模块提供上层应用对bluetooth的控制管理，状态查询等。如bluetooth
 *  的使能与禁用。查询网络连接状态等等。
 *
 * @author id:
 * @date 2016-7-22
 *
 * @verbatim
    History:
   @endverbatim
 */


#pragma once

#include "common/singleton.h"
#include "common/subject.h"

#include <string>
#include <list>
#include <stdio.h>

namespace EyeseeLinux {

typedef enum {
    BT_UNINIT = 0,
    BT_INIT,
    BT_INITING,
    BT_ENABLE,
    BT_ENABLING,
    BT_DISABLE,
    BT_DISABLING,
} BtState;



class BlueToothController
    : public ISubjectWrap(BlueToothController)
    , public Singleton<BlueToothController>
{
    friend class Singleton<BlueToothController>;
    public:
        int InitBlueToothDevice();
        int UnInitBlueToothDevice();
        int EnableBlueToothDevice();
        int DisableBlueToothDevice();
        int GetBlueToothConnectStatus(BtState &bt_state) const;

        static void *BlueToothLoopThread(void *context);

    private:
        pthread_t bluetooth_loop_thread_id_;
        BtState m_bt_state;

        BlueToothController();
        BlueToothController(const BlueToothController &o);
        BlueToothController &operator=(const BlueToothController &o);
        ~BlueToothController() {};
};

} /* EyeseeLinux */
