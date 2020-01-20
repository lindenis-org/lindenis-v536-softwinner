/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file dialog.h
 * @brief 对话框窗口
 * @author id:690
 * @version v0.3
 * @date 2017-02-08
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "window/setting_window.h"
class Dialog;
class Submenu
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(Submenu, Runtime)

    public:
        NotifyEvent OnClick;

        Submenu(IComponent *parent);
        virtual ~Submenu();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void DoShow();

        void DoHide();
        void close();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        void keyProc(int keyCode, int isLongPress);
        void submenuitemkeyproc();
        void SetIndexToSubmenu(int index);
        int GetNotifyMessage();
	 void SetDefaultValue(int select_item);
	 void ShowSaveDialog();
	 void HideSaveDialog();
	 void SaveDialogProc(View *control);
	 void SaveSubMenuChanged(int val);
	 void SaveDefaultHighLight();
	 void SetDefaultHighLight();
	 bool IfDefaultHighLightChanged();
    private:
        IComponent *_Parent;
        int select_index;
	Dialog *save_dialog_;
	int default_hilight_idx_;
};
