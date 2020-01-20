/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file model_select_window.cpp
 * @brief 模式选择窗口
 * @author sh
 * @version v2.0
 * @date 2018-01-10
 */
#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "widgets/icon_view.h"

#include <time.h>
#include <signal.h>
#include <string>
#include <map>

struct ModeIconViewBmp {
    std::string name;
    BITMAP bmp;
};

class TextView;
class ModelSelectWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(ModelSelectWindow, Runtime)
    public:
        fastdelegate::FastDelegate2<View*, int> OnModeChangeEvent;
        ModelSelectWindow(IComponent *parent);
        virtual ~ModelSelectWindow();
        std::string GetResourceName();
        void GetCreateParams(CommonCreateParams& params);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                            LPARAM lparam);
        void keyProc(int keyCode, int isLongPress);
        void QuitViewClickProc(View * control);
        void IconItemClickProc(View * control, int index);
        void SetSelectedIcon(int index);
        void DoShow();
        void DoHide();
        void Update(MSG_TYPE msg);
        void UpdateWindowLang();
        void LoadIconViewResource();
        int OnMouseDown(unsigned int button_status, int x, int y);
    private:
         GraphicView  *view_quit_;
         IconView *icon_view_;
         TextView *title_;
         std::map<int, ModeIconViewBmp> mode_icon_views_;
         StringVector mode_list_;
         int current_index_;
         bool mouse_fling_;
};
