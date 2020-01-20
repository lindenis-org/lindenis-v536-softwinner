/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: view.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    the base window class
 History:
*****************************************************************************/

#ifndef _WINDOW_CONTROL_H_
#define _WINDOW_CONTROL_H_
#include "icomponent.h"
class Runtime;


/* describe the view's life state */
typedef enum
{
    STATE_CREATING,
    STATE_CREATED,
    STATE_CLICKED,
    STATE_NORMAL,
    STATE_FREE,         //read to destroy
    STATE_DESTROYING    //destroying
}ViewState;

class View : public IComponent
{
public:
    View(View* parent);
    virtual ~View();

    /* handle the message from other views */
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam);

    /* get the view's handle */
    virtual HWND GetHandle();

    /* set the member viriable about view's visibility */
    virtual void SetVisible(bool new_value);
    virtual bool GetVisible();
    virtual void Show();
    virtual void Hide();

    /* set the member viriable about view's background color */
    virtual void SetBackColor(DWORD new_value);
    DWORD GetBackColor();

    /* set the view's position */
    virtual void SetPosition(int x, int y, int w, int h);
    void SetPosition(PRECT new_rect);
    virtual void Resize(const PRECT new_rect);
    virtual void GetRect(const PRECT new_rect);
	void Refresh();
    /* an interface to set window's caption */
    virtual void SetCaption(const char* new_caption) =0;

    /* set the view's identification */
    virtual void SetTag(int new_value);
    int GetTag();

    /* process some mouse events */
    virtual int OnMouseDown(unsigned int button_status, int x, int y);
    virtual int OnMouseUp(unsigned int button_status, int x, int y);
    virtual int OnMouseClick(unsigned int button_status, int x, int y);

    /* pre init sub class */
    void PreInit(std::string &ctrl_name);

    /* implement by window class */
    virtual void PreInitCtrl(View *ctrl, std::string &ctrl_name) {}

    virtual void SetCtrlTransparentStyle(bool enable);

    virtual void SetCtrlStyle(DWORD style_);
protected:
    View *parent_;
    DWORD back_color_;
    bool is_visible_;       //whether the view is visible
    RECT bound_rect_;       //the valid rect size
    int tag_;               //identification
    ViewState view_state_;
    DWORD transparent_style_;
};
#endif //_WINDOW_CONTROL_H_
