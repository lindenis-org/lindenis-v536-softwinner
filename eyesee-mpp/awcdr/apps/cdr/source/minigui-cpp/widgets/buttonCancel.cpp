/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file button.cpp
 * @brief Push按钮控件
 * @author id:826
 * @version v0.3
 * @date 2016-07-06
 */

#include "widgets/buttonCancel.h"
#include "debug/app_log.h"
#include "window/user_msg.h"
#include "resource/resource_manager.h"

#undef LOG_TAG
#define LOG_TAG "ButtonCancel"


IMPLEMENT_DYNCRT_CLASS(ButtonCancel)

ButtonCancel::ButtonCancel(View *parent)
    :  SystemWidget(parent)
{
    memset(&image_, 0, sizeof(BITMAP));
}

ButtonCancel::~ButtonCancel()
{
    if (image_.bmBits) {
        UnloadBitmap(&image_);
    }
}

void ButtonCancel::LoadImage(View *ctrl, const char *alias)
{
    ButtonCancel* view = reinterpret_cast<ButtonCancel *>(ctrl);
    if (view) {
        std::string image = R::get()->GetImagePath(alias);
        view->SetImage(image.c_str());
    }
}

void ButtonCancel::UnloadImage(View *ctrl)
{
    ButtonCancel* view = reinterpret_cast<ButtonCancel *>(ctrl);

    if (view->image_.bmBits) {
        UnloadBitmap(&view->image_);
    }
}

void ButtonCancel::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_BUTTON;
    params.alias      = GetClassName();
    // params.style      = WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP
                        // | BS_REALSIZEIMAGE | BS_CENTER | BS_VCENTER;
    params.style      = WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.id         = IDCANCEL;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int ButtonCancel::OnMouseUp(unsigned int button_status, int x, int y)
{
    db_msg(" ");
    if (OnPushed)
        OnPushed(this);
    return HELP_ME_OUT;;
}

int ButtonCancel::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message )
    {
        default:
            return SystemWidget::HandleMessage(hwnd, message, wparam, lparam);
    }
}

void ButtonCancel::SetImage(const char *path)
{
    if (path == NULL)
        return;

    if (image_.bmBits) {
        UnloadBitmap(&image_);
    }
    ::LoadBitmapFromFile(HDC_SCREEN, &image_, path);
    SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&image_, 0);
}
