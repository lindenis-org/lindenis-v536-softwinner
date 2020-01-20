/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    launcher_window.cpp
 * @brief   launcher界面
 * @author  id:826
 * @version v0.3
 * @date    2016-10-09
 */

#include "window/launcher_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "widgets/view_container.h"
#include "widgets/button.h"
#include "resource/resource_manager.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "window/dialog.h"
#include "widgets/menu_items.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "LauncherWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(LauncherWindow)

int LauncherWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        default:
            return SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
    }
}

LauncherWindow::LauncherWindow(IComponent *parent)
        : SystemWindow(parent)
{
    wname = "LauncherWindow";
    Load();

    string bkgnd_bmp = R::get()->GetImagePath("bg");
    db_msg("set back ground pic: %s", bkgnd_bmp.c_str());
    SetWindowBackImage(bkgnd_bmp.c_str());

    //init shortcut window UI pic
    GraphicView::LoadImage(GetControl("gv_twocam"), "gv_twocam");
    GraphicView::LoadImage(GetControl("gv_setting"), "gv_setting");
    GraphicView::LoadImage(GetControl("gv_autotest"), "gv_autotest");
    GraphicView::LoadImage(GetControl("gv_videotalk"), "gv_videotalk");
    GraphicView::LoadImage(GetControl("gv_shutdown"), "gv_shutdown");
    GraphicView::LoadImage(GetControl("gv_mosaic"), "gv_mosaic");

    GraphicView *view;

    view = reinterpret_cast<GraphicView *>(GetControl("gv_twocam"));
    view->SetTag(WINDOWID_IPCMODE);
    view->OnClick.bind(this, &LauncherWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_setting"));
    view->SetTag(WINDOWID_SETTING);
    view->OnClick.bind(this, &LauncherWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_videotalk"));
    view->SetTag(WINDOWID_PREVIEW);
    view->OnClick.bind(this, &LauncherWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_shutdown"));
    view->OnClick.bind(this, &LauncherWindow::ShutdownProc);

    // init dialog
    dialog_ = new Dialog(this);
    dialog_->SetBackColor(0xFFFFFFFF);

    string title_str;
    R::get()->GetString("fs_error_dialog", title_str);
    TextView *title = reinterpret_cast<TextView*>(dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    // confirm button of dialog
    Button *confirm_button = reinterpret_cast<Button*>(dialog_->GetControl("confirm_button"));
    confirm_button->SetTag(CONFIRM_BUTTON);
    confirm_button->OnPushed.bind(this, &LauncherWindow::DialogProc);

    // // cancel button of dialog
    Button *cancel_button = reinterpret_cast<Button*>(dialog_->GetControl("cancel_button"));
    cancel_button->SetTag(CANCEL_BUTTON);
    cancel_button->OnPushed.bind(this, &LauncherWindow::DialogProc);
}

LauncherWindow::~LauncherWindow()
{
}

void LauncherWindow::ViewClickProc(View *control)
{
    db_msg("%d clicked!", control->GetTag());
    listener_->notify(this, WM_WINDOW_CHANGE, control->GetTag());
}

void LauncherWindow::ShutdownProc(View *control)
{
    // TODO: let presenter to sync config and stop background job
    system("poweroff");
}

void LauncherWindow::DialogProc(View *control)
{
    switch (control->GetTag()) {
        case CONFIRM_BUTTON: {
#if 0
            string title_str;
            TextView *title = reinterpret_cast<TextView *>(dialog_->GetControl("dialog_title"));

            if (listener_->sendmsg(this, CONFIRM_FORMAT, 1) == 0)
                R::get()->GetString("success", title_str);
            else
                R::get()->GetString("failed", title_str);

            title->SetCaption(title_str.c_str());
            title->Refresh();

            sleep(1);

            R::get()->GetString("fs_error_dialog", title_str);
            title->SetCaption(title_str.c_str());
#endif
            listener_->sendmsg(this, CONFIRM_FORMAT, 1);
        }
            break;
        case CANCEL_BUTTON:
            listener_->sendmsg(this, CANCEL_FORMAT, 1);
            break;
        default:
            break;
    }

    dialog_->DoHide();
}

string LauncherWindow::GetResourceName()
{
    return string(GetClassName());
}

void LauncherWindow::Update(MSG_TYPE msg, int p_CamID)
{
    db_msg("handle msg:%d", msg);

    switch (msg) {
        case MSG_STORAGE_FS_ERROR:
            // show dialog
            dialog_->DoShow();
            break;
        default:
            break;
    }
}
