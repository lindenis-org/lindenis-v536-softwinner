/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file short_cut_window.h
 * @brief 下拉快捷菜单
 * @author id:826
 * @version v1.0
 * @date 2018-01-18
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"

#include <map>

typedef fastdelegate::FastDelegate3<View*, int, int> ShortCutEvent;

class ShortCutWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(ShortCutWindow, Runtime)

    public:
        enum ShortCutType {
            SHORTCUT_WIRELESS = 0,
            SHORTCUT_RC       = 1,
            SHORTCUT_AUDIO    = 2,
            SHORTCUT_LOCK     = 3,
            SHORTCUT_SHUTDOWN = 4,
            SHORTCUT_DROP_OUT = 5,
        };

        ShortCutEvent OnEvent;

        ShortCutWindow(IComponent *parent);

        virtual ~ShortCutWindow();

        virtual void keyProc(int keyCode, int isLongPress);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void ShortCutProc(View *view);

        void SetShortCutState(enum ShortCutType type, bool state);

        void DoShow();

        void DoHide();

        void Update(MSG_TYPE msg);
    private:
        std::map<enum ShortCutType, GraphicView*> short_cut_;
};
