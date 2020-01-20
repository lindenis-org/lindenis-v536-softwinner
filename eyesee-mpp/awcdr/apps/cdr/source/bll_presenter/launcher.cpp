/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    launcher.cpp
 * @brief   launcher界面presenter
 * @author  id:826
 * @version v0.3
 * @date    2016-10-08
 *
 *  用于响应其它模式窗口的切换，及处理一些全局事件
 */

#include "launcher.h"

#include "device_model/system/event_manager.h"
#include "device_model/storage_manager.h"

using namespace EyeseeLinux;

Launcher::Launcher()
{
    // TODO: should be Layer
    //EventManager::GetInstance()->Attach(this);
    StorageManager::GetInstance()->Attach(this);
}

Launcher::~Launcher()
{
    //EventManager::GetInstance()->Detach(this);
    StorageManager::GetInstance()->Detach(this);
}

void Launcher::Update(MSG_TYPE msg, int p_CamID)
{
    Layer *layer = Layer::GetInstance();
    switch (msg) {
        case MSG_HDMI_PLUGIN:
            layer->SwitchDisplayDevice(DISPLAY_DEV_HDMI);
            break;
        case MSG_TVOUT_PLUG_IN:
            layer->SwitchDisplayDevice(DISPLAY_DEV_TV);
            break;
        case MSG_HDMI_PLUGOUT:
        case MSG_TVOUT_PLUG_OUT:
            layer->SwitchDisplayDevice(DISPLAY_DEV_LCD);
            break;
        case MSG_STORAGE_FS_ERROR:
            this->Notify(msg);
            break;
        default:
            break;
    }
}

void Launcher::OnWindowLoaded()
{
}

void Launcher::OnWindowDetached()
{
}

void Launcher::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

int Launcher::DeviceModelInit()
{
    return 0;
}

int Launcher::DeviceModelDeInit()
{
    return 0;
}

int Launcher::HandleGUIMessage(int msg, int val)
{
    int ret = 0;

    switch (msg) {
        case CONFIRM_FORMAT:
            ret = StorageManager::GetInstance()->Format();
            break;
        case CANCEL_FORMAT:
            break;
        default:
            break;
    }

    return ret;
}
