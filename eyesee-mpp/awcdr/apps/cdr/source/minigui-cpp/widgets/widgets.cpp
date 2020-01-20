/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: widgets.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    widget will in actual set its visibility, background color and caption etc.
 History:
*****************************************************************************/

#define NDEBUG

#include "widgets/widgets.h"

#include "debug/app_log.h"

Widget::Widget(View* parent)
    : View(parent)
    , handle_(HWND_INVALID)
    , option_style_(0)
{
}

Widget::~Widget()
{
    DestroyWidget();
}

/*****************************************************************************
 Function: Widget::GetCreateParams
 Description: call the child in actual
        @descendant
 Parameter:
    CommonCreateParams - fill it for creating widget
 Return: -
*****************************************************************************/
void Widget::GetCreateParams(CommonCreateParams& params)
{

}

void Widget::SetOptionStyle(const DWORD &style)
{
    option_style_ = style;
}

void Widget::GetOptionStyle(DWORD &style)
{
    style = option_style_;
}


void Widget::DestroyWidget()
{
    ::DestroyWindow(handle_);
    handle_ = HWND_INVALID;
}

/*****************************************************************************
 Function: Widget::HandleMessage
 Description: @called by Widget::WindowProc for further processing
    @descendant
    @override
 Parameter:
    #hwnd - the widget itself
 Return:
    the result of View::HandleMessage
*****************************************************************************/
int Widget::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                LPARAM lparam)
{
    switch (message)
    {
//      case MSG_CHAR:
//          return OnKeyPress((DWORD)wparam);
//      case MSG_KEYUP:
//          return OnKeyUp((DWORD)wparam);
//      case MSG_KEYDOWN:
//          return OnKeyDown((DWORD)wparam);
//      case MSG_SETFOCUS:
//          DoSetFocus(true);
//          if( OnEnter )
//              OnEnter(this);
//          return HELP_ME_OUT;
//      case MSG_KILLFOCUS:
//          if( hwnd == GetHandle() )
//          {
//              DoSetFocus(false);
//              if( OnLevel )
//                  OnLevel(this);
//          }
//          return HELP_ME_OUT;
     default:
         break;
    }

    return View::HandleMessage(hwnd, message, wparam, lparam);
}

/*****************************************************************************
 Function: Widget::WindowProc
 Description:
    Default process is essential for the creation of Widget.
    The function HandleMessage will be called if widget is existed.
    @attention: HandleMessage can be override
 Parameter:
    #hwnd - the widget itself
 Return:
    the result of HandleMessage
*****************************************************************************/
long int Widget::WindowProc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    Widget *widget = NULL;
    int ret = 0;
    if ( message == MSG_CREATE) {
        widget = (Widget*)(lparam);
        /* @attention: do not set addtional data again otherwhere */
        ::SetWindowAdditionalData2(hwnd, (DWORD)widget);
    } else {
        widget = (Widget*)(::GetWindowAdditionalData2(hwnd));
    }
    if (widget) {
        ret = widget->HandleMessage( hwnd, message, wparam, lparam);
    }

    if (ret) {
        return (DefaultControlProc( hwnd, message, wparam, lparam));
    } else {
        return 0;
    }
}

/*****************************************************************************
 Function: Widget::Refresh
 Description: refresh the widget completely
 Parameter: -
 Return: -
*****************************************************************************/
void Widget::Refresh()
{
    ::InvalidateRect(GetHandle(), 0, TRUE);
}

/*****************************************************************************
 Function: Widget::SetBackColor
 Description: set background color of the widget
    @override
 Parameter:
    #new_value - new background color, the format is 32bit ARGB
 Return: -
*****************************************************************************/
void Widget::SetBackColor(DWORD new_value)
{
    SetWindowBkColor(GetHandle(), new_value);
    back_color_ = new_value;
}
/*****************************************************************************
 Function: Widget::SetBackColor(HWND hWnd,DWORD new_value)
 Description: set background color of the widget
    @override
 Parameter:
    #new_value - new background color, the format is 32bit ARGB
 Return: -
*****************************************************************************/

void Widget::SetBackColor(HWND hWnd,DWORD new_value)
{
    SetWindowBkColor(hWnd, new_value);
    back_color_ = new_value;
}

/*****************************************************************************
 Function: Widget::SetVisible
 Description: set the visibility of the widget
    @override
 Parameter:
    #new_val - true:  show the widget
               false: hide the widget
 Return: -
*****************************************************************************/
void Widget::SetVisible(bool new_val)
{
    if (new_val) {
        if (!parent_ || parent_->GetVisible())
        Show();
    } else {
        Hide();
    }
}

/*****************************************************************************
 Function: Widget::Show
 Description:
    @override
 Parameter: -
 Return: -
*****************************************************************************/
void Widget::Show()
{
    ::ShowWindow(GetHandle(), SW_SHOWNORMAL);
    View::Show();
}

/*****************************************************************************
 Function: Widget::Hide
 Description:
    @override
 Parameter: -
 Return: -
*****************************************************************************/
void Widget::Hide()
{
    ::ShowWindow(GetHandle(), SW_HIDE);
    View::Hide();
}

/*****************************************************************************
 Function: Widget::SetCaption
 Description: replacing the caption will refresh the widget entirely
 Parameter:
    #new_caption - the text must be ecoded as UTF8
 Return:
*****************************************************************************/
void Widget::SetCaption(const char* new_caption)
{
    ::SetWindowText(GetHandle(), new_caption);
}

void Widget::GetCaption(char *pString, int pStringLen)
{
    ::GetWindowText(GetHandle(),pString,pStringLen);
}

/*****************************************************************************
 Function: Widget::CreateWidget
 Description: According to the obtained parameters to create a new widget
 Parameter: -
 Return: -
*****************************************************************************/
void Widget::CreateWidget()
{
    HWND parent_handle;
    CommonCreateParams params;
    WNDCLASS wc;

    memset((void*)(&params), 0, sizeof(params));

    GetCreateParams(params);
    db_msg("create widget: %s", params.alias);

    if (parent_)
        parent_handle = parent_->GetHandle();
    else
        parent_handle = HWND_DESKTOP;

    wc.spClassName = const_cast<char*>(params.class_name);
    wc.dwStyle     = WS_CHILD;
    wc.dwExStyle   = WS_EX_NONE;
    wc.dwAddData   = 0;
    wc.hCursor     = GetSystemCursor(IDC_ARROW);//0
    wc.iBkColor    = GetWindowElementColor (WE_BGC_WINDOW);
    wc.opMask      = 0;
    wc.WinProc     = WindowProc;

    if (::GetWindowClassInfo(&wc) == false)
    {
        ::RegisterWindowClass(&wc);
    }

    handle_ = CreateWindowEx(
    params.class_name,
    "",
    WS_CHILD | params.style,
    WS_EX_NONE | params.exstyle,
    params.id,
    params.x,
    params.y,
    params.w,
    params.h,
    parent_handle, (DWORD)this);
    SetBackColor(0xFF000000);
    ::GetWindowRect( handle_, &bound_rect_);
}

/*****************************************************************************
 Function: Widget::GetHandle
 Description: get widget's handler
    @override
 Parameter: -
 Return: the handler of the widget
*****************************************************************************/
HWND Widget::GetHandle()
{
    if (handle_ == HWND_INVALID) {
        CreateWidget();
    }
    return  handle_;
}

CustomWidget::CustomWidget(View *parent)
    : Widget(parent)
{

}

CustomWidget::~CustomWidget()
{

}


/*****************************************************************************
 Function: CustomWidget::HandleMessage
 Description: process the message if be necessary
    @override
 Parameter:
    #hwnd - widget's handler
 Return:
    the result of Widget::HandleMessage
*****************************************************************************/
int CustomWidget::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    HDC hdc;
    RECT rc;
    switch (message)
    {
//      case MSG_PAINT:
//          hdc = ::BeginPaint( m_hHandle );
//          ::GetClientRect( m_hHandle, &rc );
//          Paint( hdc, &rc );
//          ::EndPaint( m_hHandle, hdc );
//          return DO_IT_MYSELF;
        default:
            return Widget::HandleMessage(hwnd, message, wparam, lparam );
    }
}


