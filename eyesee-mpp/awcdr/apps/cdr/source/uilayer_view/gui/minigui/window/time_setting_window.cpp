/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         time_settting_window.cpp
 Version:             1.0
 Author:              KPA362
 Created:            2017/4/10
 Description:       time settting  window show
 * *******************************************************************************/
#include "time_setting_window.h"
#include "resource/resource_manager.h"
#include "widgets/text_view.h"
#include "widgets/graphic_view.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "debug/app_log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "device_model/system/rtc.h"
#include "device_model/system/power_manager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"
#include <sys/time.h>
#include "window/setting_window.h"

using namespace std;
IMPLEMENT_DYNCRT_CLASS(TimeSettingWindow)

#define SELECT_COLOR   0xFF2772db //gold:0xFFFFD700 blue:0xFFFF0000
#define UNSELECT_COLOR 0xFF3c3f55
#define TEXT_COLOR 0xFFEBEBEE
#define NUM_TIMEITEM 5
TimeSettingWindow::TimeSettingWindow(IComponent *parent)
    :SystemWindow(parent)
    ,isKeyUp (true)
    ,downKey (0)
    ,m_currentKeyID(0)
{
    wname = "TimeSettingWindow";
    Load();
    listener_ = WindowManager::GetInstance();

    string bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
    SetWindowBackImage(bkgnd_bmp.c_str());

    date_t date;
    getSystemDate(&date);
    char dateString[6][5];
	snprintf(dateString[0],sizeof(dateString[0]),"%04u", date.year);
	snprintf(dateString[1],sizeof(dateString[0]),"%02u", date.month);
	snprintf(dateString[2],sizeof(dateString[0]),"%02u", date.day);
	snprintf(dateString[3],sizeof(dateString[0]),"%02u", date.hour);
	snprintf(dateString[4],sizeof(dateString[0]),"%02u", date.minute);
	snprintf(dateString[5],sizeof(dateString[0]),"%02u", date.second);

    m_TextArray[0] = reinterpret_cast<TextView *>(GetControl("year"));
    m_TextArray[0]->SetBackColor(SELECT_COLOR);
    m_TextArray[0]->SetCaption(dateString[0]);
    m_TextArray[0]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[0]->SetPosition(20, 10, 100, 40);
    m_date_fen1 = reinterpret_cast<TextView *>(GetControl("date_fen1"));
    m_date_fen1->SetCaption("-");
    m_date_fen1->SetCaptionColor(TEXT_COLOR);
    m_date_fen1->SetPosition(123, 10, 19, 40);

    m_TextArray[1] = reinterpret_cast<TextView *>(GetControl("mon"));
    m_TextArray[1]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[1]->SetCaption(dateString[1]);
    m_TextArray[1]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[1]->SetPosition(145, 10, 50, 40);
    m_date_fen2 = reinterpret_cast<TextView *>(GetControl("date_fen2"));
    m_date_fen2->SetCaption("-");
    m_date_fen2->SetCaptionColor(TEXT_COLOR);
    m_date_fen2->SetPosition(198, 10, 19, 40);
	
    m_TextArray[2] = reinterpret_cast<TextView *>(GetControl("day"));
    m_TextArray[2]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[2]->SetCaption(dateString[2]);
    m_TextArray[2]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[2]->SetPosition(220, 10, 50, 40);

    m_TextArray[3] = reinterpret_cast<TextView *>(GetControl("hour"));
    m_TextArray[3]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[3]->SetCaption(dateString[3]);
    m_TextArray[3]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[3]->SetPosition(80, 60, 50, 40);

    m_time_fen = reinterpret_cast<TextView *>(GetControl("time_fen"));
    m_time_fen->SetCaption(":");
    m_time_fen->SetCaptionColor(TEXT_COLOR);
    m_time_fen->SetPosition(133, 65, 19, 30);
    m_TextArray[4] = reinterpret_cast<TextView *>(GetControl("min"));
    m_TextArray[4]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[4]->SetCaption(dateString[4]);
    m_TextArray[4]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[4]->SetPosition(155, 60, 50, 40);

 
    R::get()->GetString(string("ml_device_time_save"), save_str);
    db_msg("[debug_zhb]------save_str = %s",save_str.c_str());
    m_TextArray[5] = reinterpret_cast<TextView *>(GetControl("save_string"));
    m_TextArray[5]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[5]->SetCaption(save_str.c_str());
    m_TextArray[5]->SetCaptionColor(TEXT_COLOR);
    m_TextArray[5]->SetPosition(80, 110, 125, 40);
#if 0
    m_time_fen2 = reinterpret_cast<TextView *>(GetControl("time_fen2"));
    m_time_fen2->SetCaption(":");
    m_time_fen2->SetCaptionColor(TEXT_COLOR);

    m_TextArray[5] = reinterpret_cast<TextView *>(GetControl("sec"));
    m_TextArray[5]->SetBackColor(UNSELECT_COLOR);
    m_TextArray[5]->SetCaption(dateString[5]);
    m_TextArray[5]->SetCaptionColor(TEXT_COLOR);
#endif
	
}

TimeSettingWindow::~TimeSettingWindow()
{

}
string TimeSettingWindow::GetResourceName()
{
    return string(GetClassName());
}

void TimeSettingWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    ctrl->SetCtrlTransparentStyle(false);
}

void TimeSettingWindow::keyProc(int keyCode, int isLongPress)
{
    switch( keyCode )
    {
        case SDV_KEY_OK:
          //  handleFocus();
            if(isLongPress == LONG_PRESS){
                handleText(-1);
            }else{
                handleText(1);
            }
            break;
        case SDV_KEY_POWER:
            handleFocus();
            break;
        case SDV_KEY_LEFT:
            handleText(1);
            break;
        case SDV_KEY_RIGHT:
            handleText(-1);
            break;
        default:
            break;
    }

    return ;
}

int TimeSettingWindow::HandleMessage(HWND hwnd,int message,WPARAM wparam,LPARAM lparam)
{
    switch (message)
    {
        case MSG_PAINT:
            return HELP_ME_OUT;
		case MSG_TIMER:
            break;
        default:
            break;
    }
    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}


int TimeSettingWindow::handleFocus()
{
    ++m_currentKeyID;
    if(m_currentKeyID > NUM_TIMEITEM )
    {
        m_TextArray[m_currentKeyID-1]->SetBackColor(UNSELECT_COLOR);
        m_TextArray[m_currentKeyID-1]->Refresh();
        m_currentKeyID = 0;

        date_t date;
        getCaptionDate(&date);
        setSystemDate(&date);
        
        this->Hide();
        static_cast<Window *>(parent_)->DoShow();
	Window *win = static_cast<Window *>(parent_);
	 listener_->sendmsg(win,SETTING_TIME_UPDATE_HIDE,0);
    }
    else 
    {
        m_TextArray[m_currentKeyID-1]->SetBackColor(UNSELECT_COLOR);
        m_TextArray[m_currentKeyID-1]->Refresh();
        m_TextArray[m_currentKeyID]->SetBackColor(SELECT_COLOR);
        m_TextArray[m_currentKeyID]->Refresh();
    }

    return 0;
}

int TimeSettingWindow::handleText(int p_value)
{
    if(m_currentKeyID > NUM_TIMEITEM-1){
		db_msg("[debug_zhb]---now m_currentKeyID[%d]  is save_string ",m_currentKeyID);
		return -1;
    	}
    int value;
    char timeStr[6] = {0};
    m_TextArray[m_currentKeyID]->GetCaption(timeStr,sizeof(timeStr));
    value = atoi(timeStr);

    value += p_value;
    switch(m_currentKeyID)
    {
        case 0:
            if( value < 1970 )
                value = 1970;
            break;

        case 1:
            if( value < 1 )
            {
                value = 12;
            }
            else if( value > 12 )
            {
                value = 1;
            }
            break;

        case 2:
        {
            int year, month, days;
            m_TextArray[0]->GetCaption(timeStr,sizeof(timeStr));
            year = atoi(timeStr);

            m_TextArray[1]->GetCaption(timeStr,sizeof(timeStr));
            month = atoi(timeStr);

            days = getMonthDays(year, month);
            if( value < 1 )
            {
                value = days;
            }
            else if( value > days )
            {
                value =1;
            }
         }
            break;

        case 3:
            if( value < 0 )
            {
                value = 23;
            }
            else if( value > 23 )
            {
                value = 0;
            }
            break;
            
        case 4:
        case 5:
            if( value < 0 )
            {
                value = 59;
            }
            else if( value > 59 )
            {
                value = 0;
            }
            break;
    }

    snprintf(timeStr, sizeof(timeStr),"%02d", value);
    m_TextArray[m_currentKeyID]->SetCaption(timeStr);

    return 0;
}

int TimeSettingWindow::getMonthDays(int year, int month)
{
	int days;
	switch (month){
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		days = 31;
		break;
        
	case 4:
	case 6:
	case 9:
	case 11:
		days = 30;
		break;
        
	case 2:
		if ((year % 400 == 0) || ((year % 4 == 0 && year % 100 != 0)))
        {
            days = 29;
        }
		else 
		{
		    days = 28;
        }
		break;
        
	default:
		days = -1;
		break;
	}

	return days;
}

int TimeSettingWindow::setSystemDate(date_t* date)
{
	struct tm tm;

	tm.tm_year = date->year - 1900;
	tm.tm_mon = date->month - 1;
	tm.tm_mday = date->day;
	tm.tm_hour = date->hour;
	tm.tm_min = date->minute;
	tm.tm_sec = date->second;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;

    set_date_time(&tm);

    return 0;
}

int TimeSettingWindow::getSystemDate(date_t * date)
{
	struct tm *ptm;
       time_t timer;
	timer = time(NULL);
       ptm = localtime(&timer);

	date->year = ptm->tm_year + 1900;
	date->month = ptm->tm_mon + 1;
	date->day = ptm->tm_mday;
	date->hour = ptm->tm_hour;
	date->minute = ptm->tm_min;
	date->second = ptm->tm_sec;

    return 0;
}

int TimeSettingWindow::updateDialog()
{
    string title_str;
    db_warn("[habo]---->     updata the dialog time setting windown");
    m_currentKeyID = 0;
    m_TextArray[0]->SetBackColor(SELECT_COLOR);
    m_TextArray[0]->SetCaptionColor(TEXT_COLOR);

    date_t date;
    getSystemDate(&date);
    char dateString[6][5];
	snprintf(dateString[0],sizeof(dateString[0]), "%04u", date.year);
	snprintf(dateString[1],sizeof(dateString[0]), "%02u", date.month);
	snprintf(dateString[2],sizeof(dateString[0]), "%02u", date.day);
	snprintf(dateString[3],sizeof(dateString[0]), "%02u", date.hour);
	snprintf(dateString[4],sizeof(dateString[0]), "%02u", date.minute);
       snprintf(dateString[5],sizeof(char)*5, "%02u", date.second);

    m_TextArray[0]->SetCaption(dateString[0]);
    m_TextArray[1]->SetCaption(dateString[1]);
    m_TextArray[2]->SetCaption(dateString[2]);
    m_TextArray[3]->SetCaption(dateString[3]);
    m_TextArray[4]->SetCaption(dateString[4]);
    m_TextArray[5]->SetCaption(save_str.c_str());

    return 0;
}

int TimeSettingWindow::getCaptionDate(date_t *date)
{
    char timeStr[6] = {0};
    m_TextArray[0]->GetCaption(timeStr,sizeof(timeStr));
    date->year = atoi(timeStr);

    m_TextArray[1]->GetCaption(timeStr,sizeof(timeStr));
    date->month= atoi(timeStr);

    m_TextArray[2]->GetCaption(timeStr,sizeof(timeStr));
    date->day= atoi(timeStr);

    m_TextArray[3]->GetCaption(timeStr,sizeof(timeStr));
    date->hour= atoi(timeStr);

    m_TextArray[4]->GetCaption(timeStr,sizeof(timeStr));
    date->minute= atoi(timeStr);

   // m_TextArray[5]->GetCaption(timeStr,sizeof(timeStr));
    //date->second = atoi(timeStr);

    return 0;
}
void TimeSettingWindow::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
}

void TimeSettingWindow::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
}

