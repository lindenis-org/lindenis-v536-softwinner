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
#include "widgets/text_view.h"

class TextView;

class InfoDialog
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(InfoDialog, Runtime)

    public:

        InfoDialog(IComponent *parent);
        virtual ~InfoDialog();
        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        void keyProc(int keyCode, int isLongPress);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void DoShow();
        void DoHide();
        void SetInfoTitle(std::string &title_str);
        void SetInfoText(std::string &text_str);
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
    private:
        TextView* m_info_title;
        TextView* m_info_text;
        bool m_show;

};
