/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: icomponent.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-18
 Description:
    Interface
 History:
*****************************************************************************/

#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include "data/gui.h"
#include "runtime/runtime.h"

class IComponent : public Runtime
{
public:
    IComponent(){}
    virtual ~IComponent(){}

    /*************************************************
     Function: HandleMessage
     Description:
        An interface, Whether it wants the system's postprocessing
     Parameter:
        hwnd - the sender
        wparam, lparam - the additional parameter
     Return:
        @HELP_ME_OUT
            system will continue to process the message
        @DO_IT_MYSELF

    *************************************************/
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)=0;

    /*************************************************
     Function: GetHandle
     Description: An interface to get View's handle
     Parameter:
     Return: return View's handle
    *************************************************/
    virtual HWND GetHandle()=0;

    virtual bool GetVisible() = 0;
};
#endif //_COMPONENT_H_
