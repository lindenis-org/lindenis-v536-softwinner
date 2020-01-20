/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file switch_button.cpp
 * @brief 使用minigui的checkbox实现switch样式的button
 * @author id:826
 * @version v0.3
 * @date 2016-06-27
 */
#pragma once

#include "graphic_view.h"

typedef enum {
  SWITCH_OFF = 0,
  SWITCH_ON = 1,
} SwitchState;

class SwitchButton : public GraphicView
{
    DECLARE_DYNCRT_CLASS(SwitchButton, Runtime)
public:
    NotifyEvent OnSwitchOn;
    NotifyEvent OnSwitchOff;

    SwitchButton(View *parent);
    virtual int OnMouseUp(unsigned int button_status, int x, int y);
    void SetSwitchState(SwitchState state);
    void SetSwitchImage(const char *on_image, const char *off_image);
    bool GetSwitchStatus() const;
private:
    bool switch_flag_;
    std::string on_image_;
    std::string off_image_;
};
