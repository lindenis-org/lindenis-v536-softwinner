/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    wizard.h
 * @brief   开机向导程序
 * @author  id:826
 * @version v0.3
 * @date    2016-10-28
 */

#pragma once

#include "window/window.h"
#include "bll_presenter/gui_presenter_base.h"

#include <minigui/common.h>
#include <minigui/minigui.h>

class Application;
class LuaConfig;
class WizardWindow;
class Wizard
    : public WindowListener
    , public IGUIPresenter
{
    public:
        Wizard();

        ~Wizard();

        void UIInit();

        void CreateWizardWindow();

        void ShowWindow();

        void RunGUIEventLoop();

        void notify(Window *form, int message, int val);

        int sendmsg(Window *form, int message, int val);

        virtual void OnWindowLoaded();

        virtual void OnWindowDetached();

        virtual int HandleGUIMessage(int msg, int val, int id=0);

        static void *HandlePostMessage(void *context);

        void BindGUIWindow(::Window *win) {}

    private:
        Application *app_;
        WizardWindow *wizard_win_;
        pthread_t notify_thread_id_;
        pthread_mutex_t post_msg_lock_;
        LuaConfig *wizard_config_;
}; // class Wizard

typedef struct PostMessageData {
    public:
        int message;
        int value;
        Wizard *context;
} PostMessageData;
