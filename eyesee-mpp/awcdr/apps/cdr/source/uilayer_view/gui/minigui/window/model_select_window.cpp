/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file model_select_window.cpp
 * @brief ģʽѡ�񴰿�
 * @author sh
 * @version v2.0
 * @date 2018-01-10
 */
#include "window/model_select_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "common/app_def.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ModelSelectWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(ModelSelectWindow)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int ModelSelectWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message ) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_MOUSE_FLING:
            IconItemClickProc(icon_view_, icon_view_->GetIconHighlight());
            mouse_fling_ = true;
            break;
        default:
            break;
    }

   return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

int ModelSelectWindow::OnMouseDown(unsigned int button_status, int x, int y)
{
    mouse_fling_ = false;
    return Window::OnMouseDown(button_status, x, y);
}

void ModelSelectWindow::keyProc(int keyCode, int isLongPress)
{
    if (parent_) {
        Window *parent_win = reinterpret_cast<Window*>(parent_);
        parent_win->keyProc(keyCode, isLongPress);
    }
}

ModelSelectWindow::ModelSelectWindow(IComponent *parent)
    : SystemWindow(parent)
    , icon_view_(NULL)
    , current_index_(0)
    , mouse_fling_(false)
{
    Load();
    //R *r = R::get();
    this->SetBackColor(0xff000000); //black

    /*load the quit graphics and title of window*/
    GraphicView::LoadImage(GetControl("quit_gview"), "button_mode_return");
    view_quit_ = reinterpret_cast<GraphicView *>(GetControl("quit_gview"));
    view_quit_->OnClick.bind(this, &ModelSelectWindow::QuitViewClickProc);
    LoadIconViewResource();
}

ModelSelectWindow::~ModelSelectWindow()
{
    db_msg("destruct");
    for (auto iter : mode_icon_views_) {
        if (iter.second.bmp.bmBits) {
            ::UnloadBitmap(&(iter.second.bmp));
        }
    }
}

void ModelSelectWindow::UpdateWindowLang()
{
    icon_view_->RemoveAllIconItem();
    LoadIconViewResource();
    icon_view_->SetIconHighlight(current_index_, true);
}

void ModelSelectWindow::LoadIconViewResource()
{
    R *r = R::get();
    string title_str;
    r->GetString("tl_model_select", title_str);
    title_ = reinterpret_cast<TextView*>(GetControl("model_title"));
    title_->SetTextStyle(DT_VCENTER | DT_LEFT | DT_SINGLELINE);
    title_->SetCaption(title_str.c_str());
    title_->SetCaptionColor(0xffffffff);

    mode_icon_views_.clear();
    mode_list_.clear();
    r->GetStringArray("mode_list", mode_list_);

    int index = 0;
    for (auto mode : mode_list_) {
        BITMAP bmp;
        memset(&bmp, 0, sizeof(BITMAP));
        string bmp_path = r->GetImagePath(mode.c_str());
        ::LoadBitmapFromFile(HDC_SCREEN, &bmp, bmp_path.c_str());
        ModeIconViewBmp icon_view_bmp;
        r->GetString(mode.c_str(), icon_view_bmp.name);
        memcpy(&icon_view_bmp.bmp, &bmp, sizeof(BITMAP));
        mode_icon_views_.emplace(index++, icon_view_bmp);
    }

    /*set model icon*/
    icon_view_ = reinterpret_cast<IconView *>(GetControl("model_view"));
    icon_view_->onIconClickEvent.bind(this, &ModelSelectWindow::IconItemClickProc);
    icon_view_->SetBackColor(0xff000000);
    icon_view_->SetIconMargins();
    icon_view_->SetCaptionColor(0xffffffff);

    int item_width = (SCREEN_HEIGHT) / 4;//432/4=108
    int item_height =360 / 2;//(240-60)/2=90
    icon_view_->SetIconSize(item_width, item_height);	// 

    std::vector<IVITEMINFO> iconview;
    for (auto iter : mode_icon_views_) {
        IVITEMINFO items;
        memset(&items, 0, sizeof(IVITEMINFO));
        items.bmp = &mode_icon_views_[iter.first].bmp;
        items.nItem = iter.first;
        items.label = const_cast<char*>(mode_icon_views_[iter.first].name.c_str());
        //items.addData = (DWORD)iconlabels[j];
        iconview.push_back(items);
    }
    icon_view_->AddIconViewItems(iconview);
}

void ModelSelectWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string ModelSelectWindow::GetResourceName()
{
    return string(GetClassName());
}

void ModelSelectWindow::QuitViewClickProc(View * control)
{
    db_msg("close the model select window");
    DoHide();
}

void ModelSelectWindow::IconItemClickProc(View * control, int index)
{
	if (mouse_fling_ == false && OnModeChangeEvent) {
        OnModeChangeEvent(this, index);
    }
}

void ModelSelectWindow::SetSelectedIcon(int index)
{
    icon_view_->SetIconHighlight(index, true);
    current_index_ = index;
	db_msg("set current_index_ = %d", current_index_ );
}

void ModelSelectWindow::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
	db_msg("current_index_ = %d", current_index_ );
}

void ModelSelectWindow::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
    if (parent_->GetVisible()) {
        ::SetActiveWindow(parent_->GetHandle());
    }
}

void ModelSelectWindow::Update(MSG_TYPE msg)
{
    //db_msg("msg received: %s", msg_str[GET_MSG_IDX(msg)]);
}


