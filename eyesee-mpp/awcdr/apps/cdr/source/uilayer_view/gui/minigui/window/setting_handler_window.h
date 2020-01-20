/* *******************************************************************************
 * Copyright (C), 2017-2027, Sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file setting_handler_window.h
 * @brief 设置控制窗口
 * @author id:fangjj
 * @version v1.0
 * @date 2017-04-21
 */

#pragma once

#include "window/window.h"
#include "window/user_msg.h"


class GraphicView;


class SettingHandlerWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(SettingHandlerWindow, Runtime)

    public:
        NotifyEvent OnClick;

        SettingHandlerWindow(IComponent *parent);
        virtual ~SettingHandlerWindow();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
		
        void keyProc(int keyCode, int isLongPress);
		
        void WindowSwitchonClickProc(View *control);

	 void UpdateLabel();

    private:
        IComponent *_Parent;
        GraphicView  *view_setting_handler;
        GraphicView  *view_window_switch;
		
};
