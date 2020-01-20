/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: system_widget.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    SystemWidget is the encapsulation of the system controls
 History:
*****************************************************************************/

#ifndef _SYSTEM_WIDGET_H_
#define _SYSTEM_WIDGET_H_

#include "widgets.h"
#include "type/types.h"

class SystemWidget : public Widget
{
public:
    SystemWidget(View *parent);
    virtual ~SystemWidget();
    static long int WindowProc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam);
    virtual void DestroyWidget();
    virtual void CreateWidget();
protected:
    WNDPROC old_window_proc_;   //the old window process function

    /* store all the system widget classes */
    static KeyMap g_controls_maps_;
};

#endif //_SYSTEM_WIDGET_H_
