/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: widgets.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    some common definitions about widget are here.
    widget is a base class and has simple functionalities, such as create,
    set visibility and set background color etc.
 History:
*****************************************************************************/

#ifndef _WIDGETS_H_
#define _WIDGETS_H_
#include "data/fast_delegate.h"
#include "data/fast_delegate_bind.h"

#include "data/gui.h"
#include "widgets/view.h"

/* ********** define static draw style ********** */
#define SS_VCENTER      0x00000040L

#define CTRL_MENULIST       "MenuList"
#define CTRL_CARD_IMAGEVIEW "CardImage"
#define CTRL_SEEKBAR        "SeekBar"
#define CTRL_MAGIC_BLOCK    "MagicBlock"
#define CTRL_LISTBOX_VIEW        "ListBoxView"

#define DEFAULT_CTRL_WIDTH  30
#define DEFAULT_CTRL_HEIGHT 30

typedef struct
{
    DWORD   style;          //window style
    DWORD   exstyle;        //extra style
    const char* class_name; //the minigui class name
    const char* alias;      // the alias name

    int id;
    /* the position, relative to the parent*/
    int     x;
    int     y;

    /* the size */
    int     w;
    int     h;
}CommonCreateParams;

/* Event delegation function */
typedef fastdelegate::FastDelegate1<View*> NotifyEvent;     //simple notify event
typedef fastdelegate::FastDelegate2<View*, int> ClickEvent;
typedef fastdelegate::FastDelegate2<View*, DWORD, int> KeyEvent;
typedef fastdelegate::FastDelegate2<View*, bool> ChangeEvent;


class Widget : public View
{
public:
    Widget(View *parent);
    virtual ~Widget();

    /* get the widget's param for creation */
    virtual void GetCreateParams(CommonCreateParams &params);

    virtual void SetOptionStyle(const DWORD &style);

    virtual void GetOptionStyle(DWORD &style);


    /* create the generic widget */
    virtual void CreateWidget();
    virtual void DestroyWidget();

    /* get the widget's handle */
    HWND GetHandle();

    /* process the message if be necessary */
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam);

    /* the default message handler */
    static int long WindowProc(HWND hwnd,unsigned int message, WPARAM wparam,
                                    LPARAM lparam);

    /* force to refresh the widget */
    virtual void Refresh();

    /* set the background color of the widget actually */
    virtual void SetBackColor(DWORD new_value);
    /* set the background color of the widget actually by window's hwnd*/
    virtual void SetBackColor(HWND hWnd,DWORD new_value);
    /* make the widget actually */
    virtual void SetVisible(bool new_val);
    virtual void Show();
    virtual void Hide();

    /* set the widget's caption actually */
    virtual void SetCaption(const char* new_caption);
    virtual void GetCaption(char *pString, int pStringLen);
protected:
    HWND handle_;   //the widget's handler
    DWORD option_style_;
private:
};

/* a reserved class for future customizability */
class CustomWidget : public Widget
{
public:
    CustomWidget(View *parent);
    ~CustomWidget();
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam);
};
#endif //_WIDGETS_H_
