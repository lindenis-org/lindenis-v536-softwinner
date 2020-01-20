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
#include "widgets/graphic_view.h"


#define CARID_MAXLEN	8
#define CARIDSEL_MAXLEN	8
#define CARIDSELHIGHLIGHT_MAXLEN	8
#define CARID_HIGHTTAG	0x0800000

class TextView;

class CaridSettingWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(CaridSettingWindow, Runtime)

    public:
        enum ViewTag {
            CARID_SETTING_CONFIRM_BUTTON = 0x1000,
            CARID_SETTING_CARNUM		 = 0x2000,
            CARID_SETTING_CARNUMSEL		 = 0x4000,
            CARID_SETTING_CARNUMSELLEFT	 = 0x8001,
            CARID_SETTING_CARNUMSELRIGHT = 0x8002,
        };

        ClickEvent OnConfirmClick;

        CaridSettingWindow(IComponent *parent);

        virtual ~CaridSettingWindow();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);
		void keyProc(int keyCode, int isLongPress);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();

        enum ContentType {
            CONTENT_TYPE_CARID,
        };
        void SetContentType();
		void SetDiffNumContentType();
		void SetDiffLetterContentType();

        void SetTitle(const std::string& l, const std::string& m, const std::string& r);

        void SetText(int l, int m, int r);

        void SetActiveCol(int col);

		std::string Caridstring;
		std::string CaridSelstring;
		int selhighilghtid;
		std::string CaridSelstringtmp;
		int highlightid;
		int caridselstartindex;
		int caridselstrmax;
    private:
        void SetColColor(int col, int color);

        void UpdateNumRange();

        void CalcNum(int col, int num);

        void ButtonProc(View *control);

        void SetTextViewCaption(const std::string &view_id, const std::string &text);

        void DisplayCurrentCaridNum(std::string &str);
		void DisplayCurrentCarSelchar(std::string &str, int index);
        void SetSystemNewDevname(std::string &devname);

        std::string l_title_;
        std::string m_title_;
        std::string r_title_;

        int l_num_;
        int l_num_max_;
        int l_num_min_;

        int m_num_;
        int m_num_max_;
        int m_num_min_;

        int r_num_;
        int r_num_max_;
        int r_num_min_;

        int active_col_;
        ContentType content_type_;

		TextView *carid_label_[CARID_MAXLEN];
		TextView *caridsel_label_[CARIDSEL_MAXLEN];
		
};
