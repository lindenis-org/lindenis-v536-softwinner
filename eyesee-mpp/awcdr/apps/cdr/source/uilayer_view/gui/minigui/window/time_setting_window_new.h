/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ok_dialog.h
 * @brief 确认对话框窗口
 * @author id:826
 * @version v1.0
 * @date 2018-01-12
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "type/types.h"


class TextView;
class GraphicView;

class TimeSettingWindowNew
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(TimeSettingWindowNew, Runtime)

    public:
        enum ViewTag {
            TIME_SETTING_CONFIRM_BUTTON,
            TIME_SETTING_YEAR_COL,
            TIME_SETTING_MONTH_COL,
            TIME_SETTING_DAY_COL,
            TIME_SETTING_HOUR_COL,
            TIME_SETTING_MINUTE_COL,
            TIME_SETTING_SECOND_COL,
        };

        ClickEvent OnConfirmClick;

        TimeSettingWindowNew(IComponent *parent);

        virtual ~TimeSettingWindowNew();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();


        void ShowCurrentDateTime();

        void SetTitle(const std::string& year_str, const std::string& month_str, const std::string& day_str, const std::string& hour_str, const std::string& minute_str, const std::string& second_str);

        void SetText(int year_, int month_, int day_, int hour_, int minute_, int second_);

        void SetActiveCol(int col);
		
		void keyProc(int keyCode, int isLongPress);

    private:
        void SetColColor(int col, int color);

        void UpdateNumRange();

        void CalcNum(int col, int num);

        void ButtonProc(View *control);

        void SetTextViewCaption(const std::string &view_id, const std::string &text);

        void DisplayCurrentDateTimeNum();

        void SetSystemDateTime();

        std::string l_title_;
        std::string m_title_;
        std::string r_title_;

		int cur_flag;
        int year_num_;
        int year_num_max_;
        int year_num_min_;

        int month_num_;
        int month_num_max_;
        int month_num_min_;

        int day_num_;
        int day_num_max_;
        int day_num_min_;

        int hour_num_;
        int hour_num_max_;
        int hour_num_min_;

        int minute_num_;
        int minute_num_max_;
        int minute_num_min_;

        int second_num_;
        int second_num_max_;
        int second_num_min_;

        int active_col_;

        TextView *m_title;
        TextView *m_year_label;
        TextView *m_year_text;
        TextView *m_month_label;
        TextView *m_month_text;
        TextView *m_day_label;
        TextView *m_day_text;
        TextView *m_hour_label;
        TextView *m_hour_text;
        TextView *m_minute_label;
        TextView *m_minute_text;
        TextView *m_second_label;
        TextView *m_second_text;
        
        TextView *m_year_top_ind;
        TextView *m_year_bottom_ind;
        TextView *m_month_top_ind;
        TextView *m_month_bottom_ind;
        TextView *m_day_top_ind;
        TextView *m_day_bottom_ind;
        TextView *m_hour_top_ind;
        TextView *m_hour_bottom_ind;
        TextView *m_minute_top_ind;
        TextView *m_minute_bottom_ind;
        TextView *m_second_top_ind;
        TextView *m_second_bottom_ind;

};
