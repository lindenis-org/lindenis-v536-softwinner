/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    usb_mode_window.h
 * @brief   USB模式窗口, 当USB连接时进行模式选择
 *
 *  - 供电模式
 *  - 大容量存储模式
 *  - Webcam模式
 *
 * @author  id:826
 * @version v0.3
 * @date    2017-02-27
 */
#pragma once

#include "window/window.h"
#include "window/user_msg.h"

class ShutDownWindow
        : public SystemWindow {
    DECLARE_DYNCRT_CLASS(ShutDownWindow, Runtime)

    public:
        ShutDownWindow(IComponent *parent);

        virtual ~ShutDownWindow();

        std::string GetResourceName();

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void GetCreateParams(CommonCreateParams& params);
};
