/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file bind_window.cpp
 * @brief 绑定界面
 * @author id:007
 * @version v0.1
 * @date 2018-05-12
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "common/posix_timer.h"

class ListView;
class Dialog;
class TextView;
class BindWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(BindWindow, Runtime)

    public:

        BindWindow(IComponent *parent);

        ~BindWindow();

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
	 void keyProc(int keyCode, int isLongPress);
	 static void HideBindWindowTimerProc(union sigval sigval);
	 void DoHideBindWindow();
    private:
		Dialog *dialog_;
		TextView *title_window;
		GraphicView *bind_image;
		timer_t b_hide_timer_id;
		BITMAP *qrbmp ;
		pthread_mutex_t paint_lock;
};
