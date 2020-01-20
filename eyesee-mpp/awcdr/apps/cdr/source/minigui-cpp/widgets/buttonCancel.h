/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file button.h
 * @brief Push按钮控件
 * @author id:826
 * @version v0.3
 * @date 2016-07-06
 */
#pragma once

#include "system_widget.h"

class ButtonCancel : public SystemWidget
{
    DECLARE_DYNCRT_CLASS(ButtonCancel, Runtime)
public:
    ButtonCancel(View *parent);
    virtual ~ButtonCancel();
    virtual void GetCreateParams(CommonCreateParams &params);
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
    virtual void SetImage(const char *path);
    virtual int OnMouseUp(unsigned int button_status, int x, int y);

    static void LoadImage(View *ctrl, const char *alias);
    static void UnloadImage(View *ctrl);
    NotifyEvent OnPushed;
protected:
    BITMAP image_;
};
