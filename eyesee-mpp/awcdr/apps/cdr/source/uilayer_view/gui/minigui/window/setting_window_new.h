/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: menu_window.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _SETTING_WINDOW_NEW_H_
#define _SETTING_WINDOW_NEW_H_

#include "window/window.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "widgets/text_view.h"
#include "widgets/listbox_view.h"
#include "widgets/item_set_view.h"
class WindowManager;
class ListBoxView;
class ItemData;
class Dialog;
class Button;
class GraphicView;
class MagicBlock;
class TimeSettingWindow;
class LevelBar;
class PromptBox;
class BulletCollection;
class SettingWindowNew : public SystemWindow {
    DECLARE_DYNCRT_CLASS(SettingWindowNew, Runtime)

    public:
        SettingWindowNew(IComponent *parent);
        virtual ~SettingWindowNew();
        std::string GetResourceName();
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void keyProc(int keyCode, int isLongPress);
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        ListboxView *ListboxDataInit(ListboxView *listboxView);
        void ListBoxViewClickProc(View *control);
    private:
        void InitListBoxView();
        int InitListBoxView(ListboxView *listboxView);
        int GetMenuConfig(int msg);
    private:
        //MenuItems *menu_items_;
        WindowManager *win_mg_;
        ListBoxView *lbv;
    public:
        pthread_mutex_t setwindow_proc_lock_;
        ListboxView *listboxView;
};

#endif //_SETTING_WINDOW_NEW_H_