/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    launcher_window.h
 * @brief   launcher界面
 * @author  id:826
 * @version v0.3
 * @date    2016-10-09
 */
#pragma once

#include "window/window.h"
#include "window/user_msg.h"

/**
 * 定义每个窗体内部的button/msg
 */
#define CONFIRM_FORMAT         (USER_MSG_BASE+1)
#define CANCEL_FORMAT          (USER_MSG_BASE+2)

class Dialog;
class LauncherWindow
        : public SystemWindow {
    DECLARE_DYNCRT_CLASS(LauncherWindow, Runtime)

    public:
        LauncherWindow(IComponent *parent);

        virtual ~LauncherWindow();

        std::string GetResourceName();

        void ViewClickProc(View *control);

        void ShutdownProc(View *control);

        void DialogProc(View *control);

        /* default window process function */
        static int WindowProc(HWND hwnd, int msg, WPARAM wparam, LPARAM lparam);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

    protected:
        virtual void Update(MSG_TYPE msg, int p_CamID=0);

    private:
        Dialog *dialog_;
};
