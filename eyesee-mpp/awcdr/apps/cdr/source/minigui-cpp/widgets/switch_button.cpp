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
#include "widgets/switch_button.h"
#include "resource/resource_manager.h"
#include "window/user_msg.h"
#include "debug/app_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "SwitchButton"


IMPLEMENT_DYNCRT_CLASS(SwitchButton)

SwitchButton::SwitchButton(View *parent)
    : GraphicView(parent)
    , switch_flag_(false)
{
}

void SwitchButton::SetSwitchState(SwitchState state)
{
    switch_flag_ = state;

    if ( (!on_image_.empty()) && (!off_image_.empty()) ) {
        SetImage(switch_flag_?on_image_:off_image_);
    }
}

void SwitchButton::SetSwitchImage(const char *on_image, const char *off_image)
{
    on_image_ = R::get()->GetImagePath(on_image);
    off_image_ = R::get()->GetImagePath(off_image);

    SetImage(switch_flag_?on_image_:off_image_);
}

int SwitchButton::OnMouseUp(unsigned int button_status, int x, int y)
{
    switch_flag_ = !switch_flag_;
    if (switch_flag_) {
        db_msg("switch on");
        SetImage(on_image_);
        if (OnSwitchOn)
            OnSwitchOn(this);
    } else {
        db_msg("switch off");
        SetImage(off_image_);
        if (OnSwitchOff)
            OnSwitchOff(this);
    }

    return HELP_ME_OUT;;
}

bool SwitchButton::GetSwitchStatus() const
{
    return switch_flag_;
}
