/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    EventMonitor.cpp
 * @brief   处理event_manager事件
 * @author  id:007
 * @version v0.1
 * @date    2018-05-18
 *
 *  用于响应其它模式窗口的切换，及处理一些全局事件
 */

#include <sys/times.h>
#include "event_monitor.h"

#include "device_model/system/event_manager.h"
#include "device_model/storage_manager.h"
#include "bll_presenter/AdapterLayer.h"
#include "dd_serv/globalInfo.h"

using namespace EyeseeLinux;

EventMonitor::EventMonitor()
{
    // TODO: should be Layer
    StorageManager::GetInstance()->Attach(this);
}

EventMonitor::~EventMonitor()
{
    StorageManager::GetInstance()->Detach(this);	
}

void EventMonitor::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    switch (msg) {
		case MSG_ACCON_HAPPEN:
			{
                #if 0
				RemoteDeviceStatusInfo p_devInfo;
				AdapterLayer::GetInstance()->getDeviceInfo(p_devInfo);
                p_devInfo.equipment_action = EVENT_POWER_ON;
				AdapterLayer::GetInstance()->pushDevStatusInfo(&p_devInfo);
                #endif
			}
			break;
		case MSG_ACCOFF_HAPPEN:
			{
                #if 0
				RemoteDeviceStatusInfo p_devInfo;
				AdapterLayer::GetInstance()->getDeviceInfo(p_devInfo);
                p_devInfo.equipment_action = EVETN_POWER_SUSPEND;
                p_devInfo.runtimes = std::to_string(DD_GLOBALINFO::GetInstance()->getRuntimes(time(0)));
				AdapterLayer::GetInstance()->pushDevStatusInfo(&p_devInfo);
                #endif
			}
			break;
		case MSG_IMPACT_HAPPEN:
			{
                #if 0
				RemoteDeviceAbnormalInfo p_devInfo;
				AdapterLayer::GetInstance()->getUnnormalDeviceInfo(p_devInfo);
				AdapterLayer::GetInstance()->DevAbnormalInfo(&p_devInfo);
                #endif
			}
			break;
	  	case MSG_USB_HOST_DETACHED:
			{
				RemoteDeviceAbnormalInfo p_devInfo;
				AdapterLayer::GetInstance()->getUnnormalDeviceInfo(p_devInfo);
				AdapterLayer::GetInstance()->DevAbnormalInfo(&p_devInfo);
			}
			break;
        case MSG_STORAGE_MOUNTED: 
			{
                #if 0
				AdapterLayer::GetInstance()->setTfMounted(1);
				RemoteDeviceAbnormalInfo p_devInfo;
				AdapterLayer::GetInstance()->getUnnormalDeviceInfo(p_devInfo);
				AdapterLayer::GetInstance()->DevAbnormalInfo(&p_devInfo);
                #endif
        	}
			break;
        case MSG_STORAGE_UMOUNT: 
            {
                #if 0
				AdapterLayer::GetInstance()->setTfMounted(0);
				RemoteDeviceAbnormalInfo p_devInfo;
				AdapterLayer::GetInstance()->getUnnormalDeviceInfo(p_devInfo);
				AdapterLayer::GetInstance()->DevAbnormalInfo(&p_devInfo);
                #endif
        	}
			break;			
        default:
            break;
    }
}

void EventMonitor::OnWindowLoaded()
{
}

void EventMonitor::OnWindowDetached()
{
}

void EventMonitor::BindGUIWindow(::Window *win)
{
}

int EventMonitor::DeviceModelInit()
{
    return 0;
}

int EventMonitor::DeviceModelDeInit()
{
    return 0;
}

int EventMonitor::HandleGUIMessage(int msg, int val)
{
    int ret = 0;

    switch (msg) {
        default:
            break;
    }

    return ret;
}
