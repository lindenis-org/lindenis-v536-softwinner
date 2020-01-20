/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: graphic_view.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "widgets/graphic_view.h"
#include "debug/app_log.h"
#include "window/user_msg.h"
#include "resource/resource_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "GraphicView"


IMPLEMENT_DYNCRT_CLASS(GraphicView)

GraphicView::GraphicView(View *parent)
    :  SystemWidget(parent)
    , auto_hi_(true)
    , button_mode_(true)
    , timeout_(50)
{
    memset(&normal_image_, 0, sizeof(BITMAP));
    memset(&highlight_image_, 0, sizeof(BITMAP));
    option_style_ = SS_CENTER | SS_VCENTER | SS_REALSIZEIMAGE;
}

GraphicView::~GraphicView()
{
    if (normal_image_.bmBits) {
        UnloadBitmap(&normal_image_);
    }

    if (highlight_image_.bmBits) {
        UnloadBitmap(&highlight_image_);
    }
}
void GraphicView::LoadImage(View *ctrl, const char *alias)
{
    GraphicView* view = reinterpret_cast<GraphicView *>(ctrl);
    if (view) {
        std::string image = R::get()->GetImagePath(alias);
        view->SetImage(image);		// normal_image_
    }
}

void GraphicView::LoadImageFromAbsolutePath(View *ctrl, const std::string &path)
{
    GraphicView* view = reinterpret_cast<GraphicView *>(ctrl);
    if (view) {
        view->SetImage(path);
    }
}

void GraphicView::LoadImage(View *ctrl, const char *normal, const char *highlight, enum GraphicViewState state)
{
    GraphicView* view = reinterpret_cast<GraphicView *>(ctrl);
    if (view) {
        if (view->normal_image_.bmBits) {
            UnloadBitmap(&view->normal_image_);
        }

        if (view->highlight_image_.bmBits) {
            UnloadBitmap(&view->highlight_image_);
        }

        std::string normal_path = R::get()->GetImagePath(normal);
        ::LoadBitmapFromFile(HDC_SCREEN, &(view->normal_image_), normal_path.c_str());

        std::string highlight_path = R::get()->GetImagePath(highlight);
        ::LoadBitmapFromFile(HDC_SCREEN, &(view->highlight_image_), highlight_path.c_str());

        view->SetState(NORMAL,true);
    }
}

void GraphicView::GetImageInfo(View *ctrl,int *w,int *h)
{
    GraphicView* view = reinterpret_cast<GraphicView *>(ctrl);
    if (view->normal_image_.bmBits) {
        *w = view->normal_image_.bmWidth;
		*h = view->normal_image_.bmHeight;
    }
}

void GraphicView::UnloadImage(View *ctrl)
{
    GraphicView* view = reinterpret_cast<GraphicView *>(ctrl);

    if (view->normal_image_.bmBits) {
        UnloadBitmap(&view->normal_image_);
    }

    if (view->highlight_image_.bmBits) {
        UnloadBitmap(&view->highlight_image_);
    }
}

void GraphicView::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_STATIC;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | SS_NOTIFY | SS_BITMAP | SS_CENTERIMAGE
                        | option_style_;
    params.exstyle    = WS_EX_USEPARENTFONT | transparent_style_;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int GraphicView::OnMouseDown(unsigned int button_status, int x, int y)
{
    db_msg("this: %p", this);

    if (button_mode_) {
        int ret = SetState(HIGHLIGHT, true);	// set hight image and refresh
        if (ret != 0) return HELP_ME_OUT;
        if (::IsTimerInstalled(GetHandle(), ID_AUTO_HIGHLIGHT_TIMER)) {
            ::ResetTimer(GetHandle(), ID_AUTO_HIGHLIGHT_TIMER, timeout_);
        } else {
            ::SetTimer(GetHandle(), ID_AUTO_HIGHLIGHT_TIMER, timeout_);
        }
    }

    return HELP_ME_OUT;
}

int GraphicView::OnMouseUp(unsigned int button_status, int x, int y)
{
    db_msg("this: %p", this);

    //if (auto_hi_)
    //    SetState((state_ == NORMAL)?HIGHLIGHT:NORMAL, true);
	SetState(NORMAL, true);
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;;
}

int GraphicView::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    switch ( message )
    {
        case MSG_TIMER:
			db_msg("GraphicView: timeout");
            SetState(NORMAL, true);
            ::KillTimer(hwnd, ID_AUTO_HIGHLIGHT_TIMER);
            break;
        default:
            break;
    }
    return SystemWidget::HandleMessage(hwnd, message, wparam, lparam);
}

void GraphicView::SetImage(const std::string &path)
{
    if (path.empty())
        return;

    if (normal_image_.bmBits) {
        UnloadBitmap(&normal_image_);
    }

    ::LoadBitmapFromFile(HDC_SCREEN, &normal_image_, path.c_str());
    SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&normal_image_, 0);
}

void GraphicView::SetPlayBackImage(const std::string &path)
{
    if (path.empty())
        return;

    if (normal_image_.bmBits) {
        UnloadBitmap(&normal_image_);
    }

    ::LoadBitmapFromFile(HDC_SCREEN, &normal_image_, path.c_str());
    SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&normal_image_, 0);
}

int GraphicView::SetImage(const BITMAP &image)
{
    if (image.bmBits == NULL) {
        db_warn("image is empty");
        return -1;
    }

    SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&image, 0);

    return 0;
}

void GraphicView::SetCaptionColor(DWORD new_color)
{
    ::SetWindowElementAttr(handle_, WE_FGC_WINDOW, new_color);
}

int GraphicView::SetState(enum GraphicViewState state, bool refresh)
{
    state_ = state;
    if (state_ == HIGHLIGHT) {
        if (highlight_image_.bmBits != NULL)
            SetImage(highlight_image_);
        else
            return -1;
    } else {
        SetImage(normal_image_);
    }

    if (refresh) {
        Refresh();
    }

    return 0;
}

enum GraphicView::GraphicViewState GraphicView::GetState()
{
    return state_;
}

void GraphicView::AutoHightLight(bool enable)
{
    auto_hi_ = enable;
}

void GraphicView::SetButtonMode(bool enable)
{
    button_mode_ = enable;
}

void GraphicView::SetTimeOut(int msec)
{
    timeout_ = msec / 10;
}
