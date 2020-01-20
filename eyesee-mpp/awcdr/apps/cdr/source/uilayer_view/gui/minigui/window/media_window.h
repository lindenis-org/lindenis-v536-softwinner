/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: media_window.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _MEDIA_WINDOW_H_
#define _MEDIA_WINDOW_H_

#include "window/window.h"

class MediaWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(MediaWindow, Runtime)
public:
    MediaWindow(IComponent *parent);
    virtual ~MediaWindow();
    std::string GetResourceName();
    void ViewClickProc(View *control);
    void onChange(View *control);
};

#endif //_MEDIA_WINDOW_H_


