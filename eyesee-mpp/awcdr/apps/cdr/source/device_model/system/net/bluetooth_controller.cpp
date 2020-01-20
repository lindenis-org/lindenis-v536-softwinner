/**********************************************************
* Copyright (C), 2016, AllwinnerTech. Co., Ltd.  *
***********************************************************/

/**
 * @file bluetooth_controller.cpp
 * @author 
 * @date 2016-7-22
 * @version v0.3
 * @brief bluetooth管理器
 * @see bluetooth_controller.h
 * @verbatim
 *  History:
 * @endverbatim
 */



#include "device_model/system/net/bluetooth_controller.h"
#include "device_model/system/event_manager.h"
#include "bt.h"

#include <string.h>



#undef LOG_TAG
#define LOG_TAG "bluetooth_controller.cpp"

using namespace EyeseeLinux;
using namespace std;

BlueToothController::BlueToothController()
    :m_bt_state(BT_UNINIT)
{
    ThreadCreate(&bluetooth_loop_thread_id_, NULL, BlueToothController::BlueToothLoopThread, this);
}

void *BlueToothController::BlueToothLoopThread(void * context)
{
    BlueToothController *bt_ctrl = reinterpret_cast<BlueToothController*>(context);
    prctl(PR_SET_NAME, "BlueToothLoopThread", 0, 0, 0);

    // TODO: ADD mutex lock to control for later . guixing !!!

    char bt_name[128];
    int  bt_connect_state = 0;
    bt_bdaddr_t address;

    #if 1
    while (1) {
        if (BT_ENABLE == bt_ctrl->m_bt_state) {
            // TODO: read bt client data. eg. wakeup or send wifi password.
        }
        else {
            sleep(10);
        }
    }
    #else
    while (1) {
        //adapter_get_adapter_name(bt_name);
        //bt_connect_state = adapter_get_adapter_conn_state();
        //bluetooth_create_bond(&address);
        printf("[FUN]:%s [LINE]%d  --->> bt_name:%s  connect_state:%d  address:%s \n", __func__, __LINE__, 
                                    bt_name, bt_connect_state, address.address);
        sleep(8);
        bt_ctrl->EnableBlueToothDevice();
        sleep(8);
        printf("[FUN]:%s [LINE]%d  --->>  DisableBlueToothDevice \n", __func__, __LINE__);
        bt_ctrl->DisableBlueToothDevice();
    }
    #endif
}

int BlueToothController::InitBlueToothDevice()
{
    // TODO: ADD mutex lock to control for later . guixing !!!
    if (BT_UNINIT == m_bt_state) {
        int ret = 0;
        /* Init the bluetooth device. */
        m_bt_state = BT_INITING;
        ret = bluetooth_init();
        if (ret) {
            printf ("[FUN]:%s  [LINE]:%d  bluetooth_init error! ret:%d \n",__func__,__LINE__, ret);
            m_bt_state = BT_UNINIT;
            return -1;
        }
        m_bt_state = BT_INIT;
    }
    else if(BT_INIT == m_bt_state || BT_INITING == m_bt_state) {
        printf ("[FUN]:%s  [LINE]:%d  bluetooth have init or initing! m_bt_state:%d\n",__func__,__LINE__, m_bt_state);
        return 0;
    }
    else {
        printf ("[FUN]:%s  [LINE]:%d  The bluetooth status Can't be init! m_bt_state:%d\n",__func__,__LINE__, m_bt_state);
        return -1;
    }
    return 0;
}


int BlueToothController::UnInitBlueToothDevice()
{
    // TODO: ADD mutex lock to control for later . guixing !!!
    int ret = 0;
    m_bt_state = BT_UNINIT;
    ret = bluetooth_cleanup();
    if (ret) {
        printf ("[FUN]:%s  [LINE]:%d  bluetooth_cleanup error! ret:%d \n",__func__,__LINE__, ret);
        return -1;
    }
    return 0;
}


int BlueToothController::EnableBlueToothDevice()
{
    // TODO: ADD mutex lock to control for later . guixing !!!
    int ret = 0;
    if (BT_INIT == m_bt_state || BT_DISABLE == m_bt_state) {
        /* Step 1.  Enable the bluetooth device */
        m_bt_state = BT_ENABLING;
        ret = bluetooth_enable();
        if (ret) {
            printf ("[FUN]:%s  [LINE]:%d  bluetooth_enable error! ret:%d \n",__func__,__LINE__, ret);
            m_bt_state = BT_INIT;
            return -1;
        }

        /* Step 2.  Waiting the bluetooth device enable success. */
        int cnt = 28;
        while(cnt--) {
            if (1 == adapter_get_adapter_state()) {
                break;
            }
            sleep(1);
        }
        if (cnt <= 0) {
            printf ("[FUN]:%s  [LINE]:%d  adapter_get_adapter_state Time out error!\n",__func__,__LINE__);
            m_bt_state = BT_INIT;
            return -1;
        }

        /* Step 3.  Set bluetooth device can be found by other bt device. */
        make_adapter_can_be_found(true);
        m_bt_state = BT_ENABLE;

        /* Step 4.  Update the Notify for Observer */
        this->Notify(MSG_BLUETOOTH_ENABLE);
    }
    else if (BT_ENABLE == m_bt_state || BT_ENABLING == m_bt_state) {
        printf ("[FUN]:%s  [LINE]:%d  bluetooth have enable or enabling! m_bt_state:%d\n",__func__,__LINE__, m_bt_state);
        return 0;
    }
    else {
        printf ("[FUN]:%s  [LINE]:%d  The bluetooth status Can't be enbale! m_bt_state:%d\n",__func__,__LINE__, m_bt_state);
        return -1;
    }

    return 0;
}


int BlueToothController::DisableBlueToothDevice()
{
    // TODO: ADD mutex lock to control for later . guixing !!!
    int ret = 0;
    m_bt_state = BT_DISABLING;
    make_adapter_can_be_found(false);
    ret = bluetooth_disable();
    if (ret) {
        printf ("[FUN]:%s  [LINE]:%d  DisableBlueToothDevice error! ret:%d \n",__func__,__LINE__, ret);
        return -1;
    }
    m_bt_state = BT_DISABLE;
    this->Notify(MSG_BLUETOOTH_DISABLE);
    return 0;
}


int BlueToothController::GetBlueToothConnectStatus(BtState &bt_state) const
{
    bt_state = m_bt_state;
    return 0;
}

