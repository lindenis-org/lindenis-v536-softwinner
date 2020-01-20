/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file short_cut_window.cpp
 * @brief 下拉快捷菜单
 * @author id:826
 * @version v1.0
 * @date 2018-01-18
 */
#include "window/short_cut_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(ShortCutWindow)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int ShortCutWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_MOUSE_FLING:
            {
                int dir = LOSWORD(wparam);
                int speed = HISWORD(wparam);
                int x = LOSWORD(lparam);
                int y = HISWORD(lparam);
                if (dir == MOUSE_UP) {
                    DoHide();
                }
            }
            break;
        default:
            break;
    }
    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

void ShortCutWindow::keyProc(int keyCode, int isLongPress)
{
    switch (keyCode) {
        case SDV_KEY_OK:
            // need do hide
            break;
        case SDV_KEY_POWER: {
            if(isLongPress == LONG_PRESS){
                db_msg("power key long click");
                if (parent_) {
                    Window *pwin = reinterpret_cast<Window*>(parent_);
                    pwin->keyProc(keyCode, isLongPress);
                }
            } else {
                db_msg("power key click, hide the short cut window");
                DoHide();
            }
        }
            break;
        default:
            break;
    }
}

ShortCutWindow::ShortCutWindow(IComponent *parent)
    : SystemWindow(parent)
{
    Load();

    SetBackColor(0xff000000);
    GraphicView::LoadImage(GetControl("splite"), "orna_line2");
    GraphicView::LoadImage(GetControl("wifi"), "button_wifi", "button_wifi_hi", GraphicView::NORMAL);
    GraphicView::LoadImage(GetControl("rc"), "button_rc", "button_rc_hi", GraphicView::NORMAL);
    GraphicView::LoadImage(GetControl("audio"), "button_rec_audio", "button_rec_audio_hi", GraphicView::NORMAL);
    GraphicView::LoadImage(GetControl("lock"), "button_lock1", "button_lock1_hi", GraphicView::NORMAL);
    GraphicView::LoadImage(GetControl("shutdown"), "button_shutdown", "button_shutdown_hi", GraphicView::NORMAL);
    GraphicView::LoadImage(GetControl("drop"), "button_shortcut_menu");

    GraphicView *view = NULL;

    view = reinterpret_cast<GraphicView*>(GetControl("wifi"));
    view->SetTag(SHORTCUT_WIRELESS);
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
    view->AutoHightLight(true);
    short_cut_.emplace(SHORTCUT_WIRELESS, view);

    view = reinterpret_cast<GraphicView*>(GetControl("rc"));
    view->SetTag(SHORTCUT_RC);
    view->Hide();
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
    view->AutoHightLight(true);
    short_cut_.emplace(SHORTCUT_RC, view);

    view = reinterpret_cast<GraphicView*>(GetControl("audio"));
    view->SetTag(SHORTCUT_AUDIO);
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
    view->AutoHightLight(true);
    short_cut_.emplace(SHORTCUT_AUDIO, view);

    view = reinterpret_cast<GraphicView*>(GetControl("lock"));
    view->SetTag(SHORTCUT_LOCK);
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
    view->AutoHightLight(true);
    short_cut_.emplace(SHORTCUT_LOCK, view);

    view = reinterpret_cast<GraphicView*>(GetControl("shutdown"));
    view->SetTag(SHORTCUT_SHUTDOWN);
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
    view->AutoHightLight(true);
    short_cut_.emplace(SHORTCUT_SHUTDOWN, view);

    view = reinterpret_cast<GraphicView*>(GetControl("drop"));
    view->SetTag(SHORTCUT_DROP_OUT);
    view->OnClick.bind(this, &ShortCutWindow::ShortCutProc);
}

ShortCutWindow::~ShortCutWindow()
{
}

void ShortCutWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string ShortCutWindow::GetResourceName()
{
    return string(GetClassName());
}

void ShortCutWindow::Update(MSG_TYPE msg)
{
    //db_msg("msg received: %s", msg_str[GET_MSG_IDX(msg)]);
    switch (msg) {
        default:
            break;
    }
}

void ShortCutWindow::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
}

void ShortCutWindow::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
    if (parent_->GetVisible()) {
        ::SetActiveWindow(parent_->GetHandle());
    }
}

void ShortCutWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
}

void ShortCutWindow::SetShortCutState(enum ShortCutType type, bool state)
{
    auto it = short_cut_.find(type);
    if (it != short_cut_.end()) {
        short_cut_[type]->SetState(state?GraphicView::HIGHLIGHT:GraphicView::NORMAL);
    }
}

void ShortCutWindow::ShortCutProc(View *view)
{
    GraphicView *gv = reinterpret_cast<GraphicView*>(view);
    if (OnEvent) {
        OnEvent(this, gv->GetTag(), (int)gv->GetState());
    }
}
