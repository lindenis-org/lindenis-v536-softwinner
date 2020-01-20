/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: system_widget.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    System Widget
 History:
*****************************************************************************/
#define NDEBUG

#include "type/types.h"
#include "widgets/system_widget.h"
#include "debug/app_log.h"

#undef LOG_TAG
#define LOG_TAG "SystemWidget"

using namespace std;

/* A global static KeyMap variable.
 * Through the handler to find out class pointer, so as to call the function
 * HandleMessage.
 * @where: SystemWidget::WindowProc
 */
KeyMap SystemWidget::g_controls_maps_;

SystemWidget::SystemWidget(View *parent)
    : Widget(parent)
{

}

SystemWidget::~SystemWidget()
{
    DestroyWidget();
}

/*****************************************************************************
 Function: SystemWidget::WindowProc
 Description: The old process function of the system control is substituted
    by this WindowProc. To begin with, HandleMessage is called, and later, when
    the value from that equals to HELP_ME_OUT, old window process function will
    be called.
    @attention: there is no HandleMessage here.
    @descendant
 Parameter:
    #hwnd - system widget's handler
 Return:
*****************************************************************************/
long int SystemWidget::WindowProc(HWND hwnd, unsigned int message, WPARAM wparam,
                            LPARAM lparam)
{
    int ret = 0;

    View *result = KeyMapSearch(g_controls_maps_, hwnd);

    if (result != NULL) {
        SystemWidget* ctrl = reinterpret_cast<SystemWidget*>(result) ;
        if (ctrl) {
            ret = ctrl->HandleMessage(hwnd, message, wparam, lparam);
            if (ret == HELP_ME_OUT) {
                return ctrl->old_window_proc_(hwnd, message, wparam, lparam);
            }
        }
    }
    return DefaultControlProc(hwnd, message, wparam, lparam);
}

void SystemWidget::DestroyWidget()
{
    g_controls_maps_.clear();
    ::DestroyWindow(handle_);
}

/*****************************************************************************
 Function: SystemWidget::CreateWidget
 Description: create the system widget and replace its window process function
 Parameter: -
 Return: -
*****************************************************************************/
void SystemWidget::CreateWidget()
{
    CommonCreateParams params;
    memset( (void*)(&params), 0, sizeof(params));
    GetCreateParams(params);
    HWND parent_handle = parent_->GetHandle();
    handle_ = ::CreateWindowEx(params.class_name,
        " ",
        WS_CHILD | params.style,
        WS_EX_NONE | params.exstyle,
        params.id,
        params.x, params.y, params.w, params.h,
        parent_handle,
        0);
    old_window_proc_= SetWindowCallbackProc(handle_, WindowProc);
    db_msg("create system widget: %s, %s, %p", params.alias, params.class_name, handle_);
    g_controls_maps_.insert(make_pair(handle_, this));
    ::GetWindowRect( handle_, &bound_rect_ );
}
