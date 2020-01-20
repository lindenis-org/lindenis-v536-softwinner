/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: view_container.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    to contain different views
 History:
*****************************************************************************/

#include "widgets/view_container.h"
#include "debug/app_log.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ViewContainer"

IMPLEMENT_DYNCRT_CLASS(ViewContainer)

ViewContainer::ViewContainer(View *parent)
    :  ContainerWidget(parent)
{
    db_msg("Create ViewContainer");
}

ViewContainer::~ViewContainer()
{

}

/*****************************************************************************
 Function: ViewContainer::GetCreateParams
 Description: set the ViewContainer's initial parameter
 Parameter:
 	#CommonCreateParams - parameter for creation
 Return: -
*****************************************************************************/
void ViewContainer::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_STATIC;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | SS_NOTIFY;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

/*****************************************************************************
 Function: ViewContainer::HandleMessage
 Description: handle the messages that ViewContainer concerned about.
 Parameter:
 Return:
*****************************************************************************/
int ViewContainer::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message )
    {
        default:
            return ContainerWidget::HandleMessage(hwnd, 
                                        message, wparam, lparam);
    }
}

