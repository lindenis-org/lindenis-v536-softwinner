/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    sublist.h
 * @brief   子列表（三级列表）
 * @author  sh
 * @version v1.0
 * @date    2018-01-02
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "setting_window.h"

class Sublist
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(Sublist, Runtime)

    public:
        NotifyEvent OnListItemClick;

        Sublist(IComponent *parent);
        virtual ~Sublist();
        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        void keyProc(int keyCode, int isLongPress);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void DoShow();
        void DoHide();
        bool GetSubListWindowActiveStatus(){return m_hide;}
    private:
        bool m_hide;
};
