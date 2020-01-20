/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: view_container.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    just a container to contain different views
 History:
*****************************************************************************/

#ifndef _VIEW_CONTAINER_H_
#define _VIEW_CONTAINER_H_

#include "container.h"

class ViewContainer : public ContainerWidget
{
    DECLARE_DYNCRT_CLASS(ViewContainer, Runtime)
public:
    ViewContainer(View *parent);
    ~ViewContainer();

    /* get the parameter for creating view container*/
    virtual void GetCreateParams(CommonCreateParams &params);

    /* handle the message that need to be processed in ViewContainer */
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam);
};

#endif //_VIEW_CONTAINER_H_
