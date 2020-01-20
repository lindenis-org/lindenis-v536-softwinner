/**********************************************************
* Copyright (C), 2016, AllwinnerTech. Co., Ltd.  *
***********************************************************/

/**
 * @file ethernet_controller.cpp
 * @author
 * @date 2016-7-12
 * @version v0.3
 * @brief ether网连接控制模块
 * @see ethernet_controller.h
 * @verbatim
 *  History:
 * @endverbatim
 */



#include "device_model/system/net/ethernet_controller.h"

#include <unistd.h>

#undef LOG_TAG
#define LOG_TAG "ethernet_controller.cpp"

using namespace EyeseeLinux;
using namespace std;

EtherController::EtherController()
{
    ThreadCreate(&ether_loop_thread_id_, NULL, EtherController::EtherLoopThread, this);
}

EtherController::~EtherController()
{
    pthread_cancel(ether_loop_thread_id_);
}

void *EtherController::EtherLoopThread(void * context)
{
    EtherController *ether_ctrl = reinterpret_cast<EtherController*>(context);
    prctl(PR_SET_NAME, "EtherEventLoop", 0, 0, 0);

    while(1) {
        //ether_ctrl->Notify(MSG_ETH_DISCONNECT);
        //sleep(1);
        //ether_ctrl->Notify(MSG_ETH_CONNECT_LAN);
        //sleep(1);
        //ether_ctrl->Notify(MSG_ETH_CONNECT_INTERNET);
        //sleep(1);
        sleep(10);
    }

    return NULL;
}


int EtherController::GetEtherConnectStatus(EthConnectStat &ether_state) const
{
    return 0;
}

