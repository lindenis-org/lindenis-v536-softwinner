/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    wizard_window.h
 * @brief   开机向导界面
 * @author  id:826
 * @version v0.3
 * @date    2016-11-01
 */
#pragma once

#include "window/window.h"
#include "window/user_msg.h"

/**
 * 定义每个窗体内部的button/msg
 */
#define ONECAM_MODE         (USER_MSG_BASE+1)
#define DUALCAM_MODE        (USER_MSG_BASE+2)
#define FORMAT_DISK         (USER_MSG_BASE+3)
#define LAST_PAGE           (USER_MSG_BASE+4)
#define NEXT_PAGE           (USER_MSG_BASE+5)
#define CONFIRM_EXIT        (USER_MSG_BASE+6)
#define CANCEL_EXIT         (USER_MSG_BASE+7)
#define FEATURE_BASE        (USER_MSG_BASE+8)
#define TUTK_SUPPORT        (FEATURE_BASE+1)
#define RTSP_SUPPORT        (FEATURE_BASE+2)
#define ONVIF_SUPPORT       (FEATURE_BASE+3)

class Dialog;
class MenuItems;
class WizardWindow
        : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(WizardWindow, Runtime)

    public:
        WizardWindow(IComponent *parent);

        virtual ~WizardWindow();

        std::string GetResourceName();

        void InitMenuItem(const char *item_name,
                          const char *unhilight_icon, const char *hilight_icon,
                          int type, int value = 0);

        void MenuItemClickProc(View *control);

        void ViewClickProc(View *control);

        void ShutdownProc(View *control);

        void ExitDialogProc(View *control);

        void FormatDialogProc(View *control);

        void FormatDoneCallback(int status);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

    private:
        MenuItems *list_view_;
        Dialog *finish_dialog_;
        Dialog *format_dialog_;
        Dialog *info_dialog_;
};
