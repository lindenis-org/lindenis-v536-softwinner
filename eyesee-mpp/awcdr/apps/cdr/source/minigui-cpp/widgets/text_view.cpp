/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: text_view.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "widgets/text_view.h"
#include "debug/app_log.h"

#undef LOG_TAG
#define LOG_TAG "TextView"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(TextView)


TextView::TextView(View *parent)
    :  SystemWidget(parent)
    , text_style_(DT_CENTER | DT_VCENTER | DT_SINGLELINE)
    , text_color_(PIXEL_red)
    ,time_text_bg_color_(0x66000000)
{
    option_style_ = SS_CENTER | SS_VCENTER;
	m_TimeUpdate = false;
}

TextView::~TextView()
{

}

void TextView::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_STATIC;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | SS_NOTIFY | option_style_;
    params.exstyle    = WS_EX_USEPARENTFONT | transparent_style_;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int TextView::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    switch ( message )
    {
        case MSG_PAINT:
            {
                HDC hdc;
                hdc = BeginPaint(hwnd);
                SetBkMode(hdc, BM_TRANSPARENT);
                SelectFont(hdc, GetWindowFont(hwnd));//new add
                SetTextColor(hdc, text_color_);
                RECT rect;
                GetClientRect(hwnd, &rect);
                RECT text_rect = {0, 0, RECTW(rect), RECTH(rect)};
                 //db_msg("draw text: %s, rect[%d, %d, %d, %d]",
                 //        text_.c_str(), text_rect.left, text_rect.top, text_rect.right, text_rect.bottom);
				if( m_TimeUpdate )
				{
					SetBrushColor(hdc, time_text_bg_color_);
					FillBox(hdc,0 ,0, RECTW(rect),RECTH(rect));
					m_TimeUpdate = false;
				}
                DrawText( hdc, text_.c_str(), -1, &text_rect, text_style_);
				EndPaint(hwnd, hdc);
            }
            return 0;
        default:
            break;
    }

    return SystemWidget::HandleMessage(hwnd, message, wparam, lparam);
}

void TextView::SetText(const string text)
{
    RECT rect;
    text_ = text;
    GetClientRect(GetHandle(), &rect);
    InvalidateRect(GetHandle(), &rect, true);
}

void TextView::GetText(string &text)
{
    text = text_;
}

void TextView::SetTimeCaption(const char* new_caption,DWORD bg_color_)
{
	m_TimeUpdate = true;
    time_text_bg_color_ = bg_color_;
    RECT rect;
    text_ = new_caption;
    GetClientRect(GetHandle(), &rect);
    UpdateWindow(GetHandle(),false);
}

void TextView::SetCaption(const char* new_caption)
{
    string text = new_caption;
    SetText(text);
}

void TextView::SetCaptionEx(const char* new_caption) //调用SetCaption会概率性出现对话框字符串没有显示出来
                                                     //直接调用minigui绘制字符串函数来实现
{
    text_ = new_caption;
    ::SetWindowText(GetHandle(), new_caption);
}

void TextView::GetCaption(char *pString, int pStringLen)
{
    if (pString == NULL) {
        db_warn("pString is NULL");
    } else {
        strncpy(pString, text_.c_str(), pStringLen);
    }
}

void TextView::SetCaptionColor(DWORD new_color)
{
    text_color_ = new_color;
}

void TextView::SetTextStyle(uint32_t style)
{
    text_style_ = style;
}

int TextView::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (TextOnClick)
        TextOnClick(this);

    return HELP_ME_OUT;;
}


