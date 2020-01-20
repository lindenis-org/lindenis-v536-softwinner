/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: text_view.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _TEXT_LABEL_H_
#define _TEXT_LABEL_H_

#include "system_widget.h"
#include <string>

class TextView : public SystemWidget
{
    DECLARE_DYNCRT_CLASS(TextView, Runtime)
public:
    NotifyEvent TextOnClick;
    TextView(View *parent);
    ~TextView();
    virtual void GetCreateParams(CommonCreateParams &params);
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                            LPARAM lparam);
    void SetText(const std::string text);
    void GetText(std::string &text);
    void SetTextStyle(uint32_t style);
    void SetCaption(const char* new_caption);
	void SetTimeCaption(const char* new_caption,DWORD bg_color_= 0x66000000);
    void GetCaption(char *pString, int pStringLen);
    void SetCaptionEx(const char* new_caption);
    void SetCaptionColor(DWORD new_color);
    int OnMouseUp(unsigned int button_status, int x, int y);
private:
    uint32_t text_style_;
    DWORD text_color_;
    DWORD time_text_bg_color_;
    std::string text_;
	bool m_TimeUpdate;
};

#endif //_TEXT_LABEL_H_
