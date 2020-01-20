/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: view.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "widgets/view.h"
#include "window/user_msg.h"

#include "debug/app_log.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "View"

View::View(View *parent)
    : parent_(parent)
    , is_visible_(true)
    , tag_(0)
    , transparent_style_(WS_EX_TRANSPARENT)
{
    memset(&bound_rect_, 0, sizeof(RECT));
}

View::~View()
{

}

/*****************************************************************************
 Function: View::OnMouseDown
 Description: record the state when the mouse is preseed down
 Parameter: button_status - @reserved
 Return:
    HELP_ME_OUT - DefaultMainWinProc process
*****************************************************************************/
int View::OnMouseDown(unsigned int button_status, int x, int y)
{
    view_state_ = STATE_CLICKED;
    return HELP_ME_OUT;
}

/*****************************************************************************
 Function: View::OnMouseUp
 Description:
    record the state when the mouse is preseed up, and post CLICK message
 Parameter: button_status - @reserved
 Return:
    HELP_ME_OUT - DefaultMainWinProc process
*****************************************************************************/
int View::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (view_state_ == STATE_CLICKED)
    {
        db_msg("Touched...");
        view_state_ = STATE_NORMAL;
        ::PostMessage(GetHandle(), VIEW_MOUSE_CLICK, button_status,
            MAKELONG(x,y));
    }
    return HELP_ME_OUT;
}

/*****************************************************************************
 Function: View::OnMouseClick
 Description: @reserved
    record the state that mouse is clicked
 Parameter: button_status - @reserved
 Return:
    HELP_ME_OUT - DefaultMainWinProc process
*****************************************************************************/
int View::OnMouseClick(unsigned int button_status, int x, int y)
{
    return HELP_ME_OUT;
}

/*****************************************************************************
 Function: View::HandleMessage
 Description: handle the message what the view concerned
 Parameter:
    #hwnd - the message source
 Return:
    HELP_ME_OUT - DefaultMainWinProc process
*****************************************************************************/
int View::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                LPARAM lparam)
{
    switch ( message )
    {
        case MSG_LBUTTONDOWN:
            return OnMouseDown(wparam, LOSWORD(lparam), HISWORD(lparam));
        case MSG_LBUTTONUP:
            return OnMouseUp(wparam, LOSWORD(lparam), HISWORD(lparam));
        case VIEW_MOUSE_CLICK:
            return OnMouseClick(wparam, LOSWORD(lparam), HISWORD(lparam));
        default:
            break;
    }
    return HELP_ME_OUT;
}

/*****************************************************************************
 Function: View::SetPosition
 Description: reset the view's position and size
 Parameter: position and size
 Return: -
*****************************************************************************/
void View::SetPosition(int x, int y, int w, int h)
{
    RECT rc;
    ::SetRect( &rc, x, y, w+x, h+y ); //construct a rect
    SetPosition( &rc );
}

void View::SetPosition(PRECT new_rect)
{
    Resize( new_rect );
}

void View::Resize(const PRECT new_rect)
{
    NormalizeRect(new_rect);
    if (!EqualRect(&bound_rect_, new_rect))
    {
        bound_rect_ = *new_rect;
        ::MoveWindow(GetHandle(), new_rect->left, new_rect->top,
            RECTW((*new_rect)), RECTH((*new_rect)), false);
    }
}

/*****************************************************************************
 Function: View::GetRect
 Description: get a rect about the view's bound
 Parameter:
 Return:
*****************************************************************************/
void View::GetRect(const PRECT new_rect)
{
    *new_rect = bound_rect_;
}

HWND View::GetHandle()
{
    return  HWND_NULL;
}

/*****************************************************************************
 Function: View::SetVisible
 Description: just set a variable about view's visibility
 Parameter: new value
 Return: -
*****************************************************************************/
void View::SetVisible(bool new_value)
{
    if (new_value)
        Show();
    else
        Hide();
}

bool View::GetVisible()
{
    return is_visible_;
}

void View::Show()
{
    if (is_visible_ != true)
    {
        is_visible_ = true;
    }
}

void View::Hide()
{
    if ( is_visible_ != false)
    {
        is_visible_ = false;
    }
}

DWORD View::GetBackColor()
{
    return back_color_;
}

void View::SetBackColor(DWORD new_value)
{
    back_color_ = new_value;
}


/*****************************************************************************
 Function: View::SetTag
 Description: make up nickname.
    The tag will play a role in identifying different views.
 Parameter:
 Return:
*****************************************************************************/
void View::SetTag(int new_value)
{
    tag_ = new_value;
}

int View::GetTag()
{
    return tag_;
}

void View::PreInit(std::string &ctrl_name)
{
    parent_->PreInitCtrl(this, ctrl_name);
}

void View::SetCtrlTransparentStyle(bool enable)
{
    if (enable)
        transparent_style_ = WS_EX_TRANSPARENT;
    else
        transparent_style_ = WS_EX_NONE;
}

void View::SetCtrlStyle(DWORD style_)
{
	transparent_style_ = style_;
}

void View::Refresh()
{
	::InvalidateRect(GetHandle(), 0, TRUE);
}


