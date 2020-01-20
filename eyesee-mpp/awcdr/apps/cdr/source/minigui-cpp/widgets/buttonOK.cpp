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

#include "widgets/buttonOK.h"
#include "debug/app_log.h"
#include "window/user_msg.h"
#include "resource/resource_manager.h"

#undef LOG_TAG
#define LOG_TAG "ButtonOK"


IMPLEMENT_DYNCRT_CLASS(ButtonOK)

ButtonOK::ButtonOK(View *parent)
    :  SystemWidget(parent)
{
    memset(&image_, 0, sizeof(BITMAP));
}

ButtonOK::~ButtonOK()
{
    if (image_.bmBits) {
        UnloadBitmap(&image_);
    }
}

void ButtonOK::LoadImage(View *ctrl, const char *alias)
{
    ButtonOK* view = reinterpret_cast<ButtonOK *>(ctrl);
    if (view) {
        std::string image = R::get()->GetImagePath(alias);
        view->SetImage(image.c_str());
    }
}

void ButtonOK::UnloadImage(View *ctrl)
{
    ButtonOK* view = reinterpret_cast<ButtonOK *>(ctrl);

    if (view->image_.bmBits) {
        UnloadBitmap(&view->image_);
    }
}

void ButtonOK::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_BUTTON;
    params.alias      = GetClassName();
    // params.style      = WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP
                        // | BS_REALSIZEIMAGE | BS_CENTER | BS_VCENTER;
    params.style      = WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.id         = IDOK;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int ButtonOK::OnMouseUp(unsigned int button_status, int x, int y)
{
    db_msg(" ");
    if (OnPushed)
        OnPushed(this);
    return HELP_ME_OUT;;
}

int ButtonOK::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    switch ( message )
    {
        default:
            return SystemWidget::HandleMessage(hwnd, message, wparam, lparam);
    }
}

void ButtonOK::SetImage(const char *path)
{
    if (path == NULL)
        return;

    if (image_.bmBits) {
        UnloadBitmap(&image_);
    }
    ::LoadBitmapFromFile(HDC_SCREEN, &image_, path);
    SendMessage(GetHandle(), STM_SETIMAGE, (WPARAM)&image_, 0);
}
