/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         time_settting_window.h
 Version:             1.0
 Author:              KPA362
 Created:            2017/4/10
 Description:       time settting  window show
 * *******************************************************************************/
 #pragma once

#include "window/window.h"

class TextView;
class GraphicView;

typedef struct {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
}date_t;


class TimeSettingWindow:public SystemWindow
{
    DECLARE_DYNCRT_CLASS(TimeSettingWindow, Runtime)

    public:
        TimeSettingWindow(IComponent *parent);
        virtual ~TimeSettingWindow();
        std::string GetResourceName();
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);       

        /****************************************************
                * Name:
                *     keyProc(int keyCode, int isLongPress)
                * Function:
                *     handle window key message
                * Parameter:
                *     input:
                *         keyCode:  key id
                *         isLongPress: long press type
                *     output:
                *         none
                * return:
                *     none
               *****************************************************/
        void keyProc(int keyCode, int isLongPress);

        /****************************************************
                * Name:
                *     setSystemDate(date_t* date)
                * Function:
                *     set system time & set rtc time
                * Parameter:
                *     input:
                *         date:    time
                *     output:
                *         none
                * Return:
                *     0:    success
               *****************************************************/
        int setSystemDate(date_t* date);

        /****************************************************
                * Name:
                *     getSystemDate(date_t* date)
                * Function:
                *     get system time
                * Parameter:
                *     input:
                *         none
                *     output:
                *         date:    current system time
                * Return:
                *     0:    success
               *****************************************************/
        int getSystemDate(date_t* date);

        /****************************************************
                * Name:
                *     updateDialog()
                * Function:
                *     get system time and update dialog content
                * Parameter:
                *     input:
                *         none
                *     output:
                *         none
                * Return:
                *     0:    success
               *****************************************************/
        int updateDialog();

        /****************************************************
                * Name:
                *     PreInitCtrl(View *ctrl, std::string &ctrl_name)
                * Function:
                *     if use want to set View's background color,we should imcomplete  this function
                * Parameter:
                *     input:
                *         View:    view object
                *         ctrl_name:    control name
                *     output:
                *         none
                * Return:
                *     none
               *****************************************************/
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);  
	void DrawItem(HWND hwnd,HDC hdc);	
    private:
        /****************************************************
                * Name:
                *     handleFocus()
                * Function:
                *     set control caption color according  to focus status  
                * Parameter:
                *     input:
                *         none
                *     output:
                *         none
                * Return:
                *     0:    success
               *****************************************************/
        int handleFocus();
        
        /****************************************************
                * Name:
                *     handleText(int p_value)
                * Function:
                *     handle capture control   
                * Parameter:
                *     input:
                *         p_value: 
                *             1:    add 1 step
                *             -1:    reduce 1 step
                *     output:
                *         none
                * Return:
                *     0:    success
               *****************************************************/
        int handleText(int p_value);
        /****************************************************
                * Name: 
                *    getMonthDays(int year, int month)
                * Function:
                *     get days
                * Parameter:
                *     input:
                *         year:    year
                *         month:    month
                *     output:
                *         none
                * Return:
                *     days
               *****************************************************/
        int getMonthDays(int year, int month);

        /****************************************************
                * Name:
                *    getCaptionDate(date_t *date)
                * Function:
                *     get caption date
                * Parameter:
                *     input:
                *         none
                *     output:
                *         date:    caption date
                * Return:
                *     0:    success
               *****************************************************/
        int getCaptionDate(date_t *date);

        void DoShow();

        void DoHide();
	 
        TextView *m_TextArray[6];
        TextView *m_date_fen1;
        TextView *m_date_fen2;
        TextView *m_time_fen;
        TextView *m_time_fen2;
        GraphicView *m_view;
        bool isKeyUp;
        WPARAM downKey;
        bool isLongPress;
        int  m_currentKeyID;
	 std::string save_str;
};
