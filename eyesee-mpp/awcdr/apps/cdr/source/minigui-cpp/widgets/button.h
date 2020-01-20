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

class Button : public SystemWidget
{
    DECLARE_DYNCRT_CLASS(Button, Runtime)
public:
    Button(View *parent);
    virtual ~Button();
    virtual void GetCreateParams(CommonCreateParams &params);
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                            LPARAM lparam);
    virtual void SetImage(const char *path);
    virtual int OnMouseUp(unsigned int button_status, int x, int y);

    static void LoadImage(View *ctrl, const char *alias);
    static void UnloadImage(View *ctrl);
    void SetBackColor(DWORD color);
    void SetCaptionColor(DWORD color);
    NotifyEvent OnPushed;
protected:
    BITMAP image_;
};
