/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    event_monitor.h
 * @brief   处理event_manager事件
 * @author  id:007
 * @version v0.1
 * @date    2018-05-18
 *
 *  用于响应其它模式窗口的切换，及处理一些全局事件
 */

#pragma once

#include "bll_presenter/gui_presenter_base.h"
#include "device_model/system/event_manager.h"
#include "common_type.h"

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup EventMonitor
 * @{
 */

//class Window;
class EventMonitor
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(EventMonitor)
    , public IObserverWrap(EventMonitor)
{
    public:
        EventMonitor();

        ~EventMonitor();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

        void OnWindowLoaded();

        void OnWindowDetached();

        virtual int HandleGUIMessage(int msg, int val);

        virtual int DeviceModelInit();

        virtual int DeviceModelDeInit();

        void BindGUIWindow(::Window *win);
}; // class EventMonitor

/**  @} */
/**  @} */
} // namespace EyeseeLinux
