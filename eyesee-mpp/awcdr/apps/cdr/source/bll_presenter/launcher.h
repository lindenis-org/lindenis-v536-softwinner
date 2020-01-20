/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    launcher.h
 * @brief   launcher界面presenter
 * @author  id:826
 * @version v0.3
 * @date    2016-10-08
 *
 *  用于响应其它模式窗口的切换，及处理一些全局事件
 */

#pragma once

#include "bll_presenter/gui_presenter_base.h"
#include "window/window_manager.h"
#include "window/launcher_window.h"
#include "common_type.h"

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup Launcher
 * @{
 */

class Window;
class Launcher
    : public GUIPresenterBase
    , public IPresenter
    , public ISubject
    , public IObserver
{
    public:
        Launcher();

        ~Launcher();

        void Update(MSG_TYPE msg, int p_CamID=0);

        void OnWindowLoaded();

        void OnWindowDetached();

        virtual int HandleGUIMessage(int msg, int val);

        virtual int DeviceModelInit();

        virtual int DeviceModelDeInit();

        void BindGUIWindow(::Window *win);
}; // class Launcher

/**  @} */
/**  @} */
} // namespace EyeseeLinux
