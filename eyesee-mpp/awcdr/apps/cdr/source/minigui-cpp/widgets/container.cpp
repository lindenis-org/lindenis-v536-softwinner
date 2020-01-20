/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: container.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    all messages associated with children should be processed here
 History:
*****************************************************************************/

#include "widgets/container.h"
#include "widgets/view.h"
#include "debug/app_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ContainerWidget"


ContainerWidget::ContainerWidget(View *parent)
    : Widget(parent)
{

}

ContainerWidget::~ContainerWidget()
{

}

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
int ContainerWidget::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message ) {
    case MSG_FONTCHANGED: {
        HWND hChild;
        hChild = GetNextChild(hwnd, 0);
        while( hChild && (hChild != HWND_INVALID) )
        {
            SetWindowFont(hChild, GetWindowFont(hwnd));
            hChild = GetNextChild(hwnd, hChild);
        }
    }
    return HELP_ME_OUT;
    case MSG_MOUSE_FLING: {
        HWND hChild;
        hChild = GetNextChild(hwnd, 0);
        int scrollbar_direction = -1;
        while( hChild && (hChild != HWND_INVALID))
        {
            if (GetWindowStyle(hChild) & WS_VSCROLL) {
                int direction = LOSWORD (wparam);
                if (direction == MOUSE_UP) {
                    scrollbar_direction = SB_LINEDOWN;
                } else if (direction == MOUSE_DOWN) {
                    scrollbar_direction = SB_LINEUP;
                }
                SendMessage(hChild, MSG_VSCROLL, scrollbar_direction, 0);
            }
            hChild = GetNextChild(hwnd, hChild);
        }
    }
    return HELP_ME_OUT;
    case MSG_LBUTTONUP:
        return HELP_ME_OUT;
	case MSG_KEYDOWN:{
		HWND hChild;
        hChild = GetNextChild(hwnd, 0);
        while( hChild && (hChild != HWND_INVALID) )
        {
            SendMessage(hChild, message, wparam, lparam);
            hChild = GetNextChild(hwnd, hChild);
        }
		
		}
	return HELP_ME_OUT;
	case MSG_KEYUP:{
        
		HWND hChild;
		RECT* update_rect;
        hChild = GetNextChild(hwnd, 0);
        while( hChild && (hChild != HWND_INVALID) )
        {
          // InvalidateRect (hChild, NULL, TRUE);
            SendMessage(hChild, message, wparam, lparam);
            hChild = GetNextChild(hwnd, hChild);
        }
        
	}
	return HELP_ME_OUT;
    default:
        return Widget::HandleMessage( hwnd, message, wparam, lparam );
   }
}

/*****************************************************************************
 Function: ContainerWidget::NotifyChildren
 Description: @reserved
 Parameter:
 Return:
*****************************************************************************/
int ContainerWidget::NotifyChildren(int iMsg, WPARAM wparam, LPARAM lparam)
{
    return HELP_ME_OUT;
}

