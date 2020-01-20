/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ok_dialog.cpp
 * @brief 确认对话框窗口
 * @author id:826
 * @version v1.0
 * @date 2018-01-12
 */
#include "time_setting_window_new.h"

#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "resource/resource_manager.h"
#include "widgets/graphic_view.h"
#include "widgets/button.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <vector>

using namespace std;

#define gray_color 0xFFB5B5B5
#define black_color 0xFF363636
#define COLOR_BLACK                 0xFF000000
#define COLOR_WHITE                 0xFFFFFFFF
#define COLOR_GLOBAL_BG             0xFF00CDD1
#define COLOR_OK_DIALOG_BG          0xF0000000
#define COLOR_OK_DIALOG_BUTTON_BG   COLOR_GLOBAL_BG
#define COLOR_INFO_DIALOG_BG        0xF0000000
#define COLOR_BUTTON_FG             COLOR_WHITE


IMPLEMENT_DYNCRT_CLASS(TimeSettingWindowNew)

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/
void TimeSettingWindowNew::keyProc(int keyCode, int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_POWER:
			{
			db_warn("[debug_jaosn]:short MSG_TIMER");
			if (isLongPress == LONG_PRESS){
			 	//PreviewWindow *p_win  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			 	//p_win->ShutDown();
			  }
			}
            break;
        case SDV_KEY_OK:
			db_warn("[debug_jaosn]:short SDV_KEY_OK");
			if(cur_flag < 0/* || cur_flag >= 6*/){
				cur_flag = 1;
				SetActiveCol(cur_flag);
			}else if(cur_flag >= 6){
				cur_flag = 1;
				SetSystemDateTime();
        		OnConfirmClick(this,1);
			}else
			{
				cur_flag = cur_flag + 1;
				SetActiveCol(cur_flag);
				//SetActiveCol(cur_flag);
			}
            break;
		case SDV_KEY_LEFT:
		//case SCANCODE_SUNXIUP:	
		{	
		db_warn("[debug_jaosn]:short SDV_KEY_LEFT");
		CalcNum(active_col_, -1);
		}
		break;
		case SDV_KEY_RIGHT:
		case SCANCODE_SUNXIDOWN:	
			db_warn("[debug_jaosn]:short SDV_KEY_RIGHT");
			CalcNum(active_col_, 1);
		break;

        case SDV_KEY_MENU:
		case SDV_KEY_MODE:
			db_warn("[debug_jaosn]:short SDV_KEY_OK");
			//SetSystemDateTime();
			cur_flag = 1;
        	OnConfirmClick(this,1);
			//this->DoHide();
            break;
        default:
            db_msg("[debug_zhb]:invild keycode");
            break;
    }  
}


int TimeSettingWindowNew::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_MOUSE_FLING:
        {
            int direction = LOSWORD (wparam);
            if (direction == MOUSE_UP) {
                CalcNum(active_col_, 1);
                db_msg("time setting windown key  up.....");
            } else if (direction == MOUSE_DOWN) {
                CalcNum(active_col_, -1);
               db_msg("time setting windown key down.....");
            }
        }
            break;
//            return HELP_ME_OUT;
        default:
            break;
    }

    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

TimeSettingWindowNew::TimeSettingWindowNew(IComponent *parent)
    : SystemWindow(parent)
    , year_num_(-1)
    ,year_num_max_(-1)
    ,year_num_min_(-1)
    , month_num_(-1)
    ,month_num_max_(-1)
    ,month_num_min_(-1)
    , day_num_(-1)
    , day_num_max_(-1)
    , day_num_min_(-1)
    , hour_num_(-1)
    , hour_num_max_(-1)
    , hour_num_min_(-1)
    , minute_num_(-1)
    , minute_num_max_(-1)
    , minute_num_min_(-1)
    , second_num_(-1)
    , second_num_max_(-1)
    , second_num_min_(-1)
    , active_col_(-1)
    , cur_flag(-1)
    ,m_title(NULL)
    ,m_year_label(NULL)
    ,m_year_text(NULL)
    ,m_month_label(NULL)
    ,m_month_text(NULL)
    ,m_day_label(NULL)
    ,m_day_text(NULL)
    ,m_hour_label(NULL)
    ,m_hour_text(NULL)
    ,m_minute_label(NULL)
    ,m_minute_text(NULL)
    ,m_second_label(NULL)
    ,m_second_text(NULL)
    ,m_year_top_ind(NULL)
    ,m_year_bottom_ind(NULL)
    ,m_month_top_ind(NULL)
    ,m_month_bottom_ind(NULL)
    ,m_day_top_ind(NULL)
    ,m_day_bottom_ind(NULL)
    ,m_hour_top_ind(NULL)
    ,m_hour_bottom_ind(NULL)
    ,m_minute_top_ind(NULL)
    ,m_minute_bottom_ind(NULL)
    ,m_second_top_ind(NULL)
    ,m_second_bottom_ind(NULL)   
{
    wname = "TimeSettingWindowNew";
    Load();
    SetBackColor(COLOR_WHITE);
    GraphicView *title_bg = static_cast<GraphicView*>(GetControl("title_bg"));
    title_bg->SetBackColor(black_color);

    GraphicView *confirm = static_cast<GraphicView*>(GetControl("confirm_button"));
    GraphicView::LoadImage(confirm, "button_time_setting_return");
    confirm->SetTag(TIME_SETTING_CONFIRM_BUTTON);
    confirm->OnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    std::string title_str;
    m_title = static_cast<TextView*>(GetControl("title"));
    R::get()->GetString("ml_device_time", title_str);
    m_title->SetCaption(title_str.c_str());
    m_title->SetCaptionColor(COLOR_WHITE);

    m_year_label = static_cast<TextView*>(GetControl("year_label"));
    m_year_label->SetTag(TIME_SETTING_YEAR_COL);
    m_year_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_year_text = static_cast<TextView*>(GetControl("year_text"));
    m_year_text->SetTag(TIME_SETTING_YEAR_COL);
    m_year_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_month_label = static_cast<TextView*>(GetControl("month_label"));
    m_month_label->SetTag(TIME_SETTING_MONTH_COL);
    m_month_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_month_text = static_cast<TextView*>(GetControl("month_text"));
    m_month_text->SetTag(TIME_SETTING_MONTH_COL);
    m_month_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_day_label = static_cast<TextView*>(GetControl("day_label"));
    m_day_label->SetTag(TIME_SETTING_DAY_COL);
    m_day_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_day_text = static_cast<TextView*>(GetControl("day_text"));
    m_day_text->SetTag(TIME_SETTING_DAY_COL);
    m_day_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_hour_label = static_cast<TextView*>(GetControl("hour_label"));
    m_hour_label->SetTag(TIME_SETTING_HOUR_COL);
    m_hour_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_hour_text = static_cast<TextView*>(GetControl("hour_text"));
    m_hour_text->SetTag(TIME_SETTING_HOUR_COL);
    m_hour_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_minute_label = static_cast<TextView*>(GetControl("minute_label"));
    m_minute_label->SetTag(TIME_SETTING_MINUTE_COL);
    m_minute_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_minute_text = static_cast<TextView*>(GetControl("minute_text"));
    m_minute_text->SetTag(TIME_SETTING_MINUTE_COL);
    m_minute_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_second_label = static_cast<TextView*>(GetControl("second_label"));
    m_second_label->SetTag(TIME_SETTING_SECOND_COL);
    m_second_label->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);

    m_second_text = static_cast<TextView*>(GetControl("second_text"));
    m_second_text->SetTag(TIME_SETTING_SECOND_COL);
    m_second_text->TextOnClick.bind(this, &TimeSettingWindowNew::ButtonProc);


    m_year_top_ind = static_cast<TextView*>(GetControl("year_top_ind"));
    m_year_bottom_ind = static_cast<TextView*>(GetControl("year_bottom_ind"));
    m_month_top_ind = static_cast<TextView*>(GetControl("month_top_ind"));
    m_month_bottom_ind = static_cast<TextView*>(GetControl("month_bottom_ind"));
    m_day_top_ind = static_cast<TextView*>(GetControl("day_top_ind"));
    m_day_bottom_ind = static_cast<TextView*>(GetControl("day_bottom_ind"));
    m_hour_top_ind = static_cast<TextView*>(GetControl("hour_top_ind"));
    m_hour_bottom_ind = static_cast<TextView*>(GetControl("hour_bottom_ind"));
    m_minute_top_ind = static_cast<TextView*>(GetControl("minute_top_ind"));
    m_minute_bottom_ind = static_cast<TextView*>(GetControl("minute_bottom_ind"));
    m_second_top_ind = static_cast<TextView*>(GetControl("second_top_ind"));
    m_second_bottom_ind = static_cast<TextView*>(GetControl("second_bottom_ind"));
    SetColColor(1, gray_color);
    SetColColor(2, gray_color);
    SetColColor(3, gray_color);
    SetColColor(4, gray_color);
    SetColColor(5, gray_color);
    SetColColor(6, gray_color);
}

TimeSettingWindowNew::~TimeSettingWindowNew()
{
}

void TimeSettingWindowNew::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE|WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string TimeSettingWindowNew::GetResourceName()
{
    return string(GetClassName());
}

void TimeSettingWindowNew::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
}

void TimeSettingWindowNew::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
    #if 1
    if (parent_->GetVisible()) {
        ::SetActiveWindow(parent_->GetHandle());
    }
    #endif
}

void TimeSettingWindowNew::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("title_bg")
            || ctrl_name == string("year_top_ind")
            || ctrl_name == string("year_bottom_ind")
            || ctrl_name == string("month_top_ind")
            || ctrl_name == string("month_bottom_ind")
            || ctrl_name == string("day_top_ind")
            || ctrl_name == string("day_bottom_ind")
            || ctrl_name == string("hour_top_ind")
            || ctrl_name == string("hour_bottom_ind")
            || ctrl_name == string("minute_top_ind")
            || ctrl_name == string("minute_bottom_ind")
            || ctrl_name == string("second_top_ind")
            || ctrl_name == string("second_bottom_ind")) {
         ctrl->SetCtrlTransparentStyle(false);
     }
}

int TimeSettingWindowNew::OnMouseUp(unsigned int button_status, int x, int y)
{
//    if (OnClick)
//        OnClick(this);

    return HELP_ME_OUT;;
}

void TimeSettingWindowNew::ButtonProc(View *control)
{
    int tag = control->GetTag();

    if (tag == TIME_SETTING_CONFIRM_BUTTON && OnConfirmClick) {
        SetSystemDateTime();
        OnConfirmClick(this,1);
    } else if (tag == TIME_SETTING_YEAR_COL) {
        SetActiveCol(1);
    } else if (tag == TIME_SETTING_MONTH_COL) {
        SetActiveCol(2);
    } else if (tag == TIME_SETTING_DAY_COL) {
        SetActiveCol(3);
    }else if (tag == TIME_SETTING_HOUR_COL) {
        SetActiveCol(4);
    } else if (tag == TIME_SETTING_MINUTE_COL) {
        SetActiveCol(5);
    } else if (tag == TIME_SETTING_SECOND_COL) {
        SetActiveCol(6);
    }
}

void TimeSettingWindowNew::SetTextViewCaption(const std::string &view_id, const std::string &text)
{
    TextView *title = static_cast<TextView*>(GetControl(view_id.c_str()));
    title->SetBackColor(COLOR_OK_DIALOG_BUTTON_BG);
    title->SetCaptionColor(COLOR_BUTTON_FG);
    title->SetCaption(text.c_str());
}

void TimeSettingWindowNew::SetTitle(const std::string& year_str, const std::string& month_str, const std::string& day_str, const std::string& hour_str, const std::string& minute_str, const std::string& second_str)
{
    //TextView *text = nullptr;
    //text = static_cast<TextView*>(GetControl("year_label"));
    m_year_label->SetCaption(year_str.c_str());

    //text = static_cast<TextView*>(GetControl("month_label"));
    m_month_label->SetCaption(month_str.c_str());

    //text = static_cast<TextView*>(GetControl("day_label"));
    m_day_label->SetCaption(day_str.c_str());

    //text = static_cast<TextView*>(GetControl("hour_label"));
    m_hour_label->SetCaption(hour_str.c_str());

    //text = static_cast<TextView*>(GetControl("minute_label"));
    m_minute_label->SetCaption(minute_str.c_str());

    //text = static_cast<TextView*>(GetControl("second_label"));
    m_second_label->SetCaption(second_str.c_str());

}

void TimeSettingWindowNew::SetText(int year_, int month_, int day_, int hour_, int minute_, int second_)
{
    //TextView *text = nullptr;

    if (year_ != year_num_) {
        //text = static_cast<TextView*>(GetControl("year_text"));
        m_year_text->SetCaption(to_string(year_).c_str());
        year_num_ = year_;
    }

    if (month_ != month_num_) {
        //text = static_cast<TextView*>(GetControl("month_text"));
        m_month_text->SetCaption(to_string(month_).c_str());
        month_num_ = month_;
    }

    if (day_ != day_num_) {
        //text = static_cast<TextView*>(GetControl("day_text"));
        m_day_text->SetCaption(to_string(day_).c_str());
        day_num_ = day_;
    }
    if (hour_ != hour_num_) {
        //text = static_cast<TextView*>(GetControl("hour_text"));
        m_hour_text->SetCaption(to_string(hour_).c_str());
        hour_num_ = hour_;
    }

    if (minute_ != minute_num_) {
        //text = static_cast<TextView*>(GetControl("minute_text"));
        m_minute_text->SetCaption(to_string(minute_).c_str());
        minute_num_ = minute_;
    }

    if (second_ != second_num_) {
        //text = static_cast<TextView*>(GetControl("second_text"));
        m_second_text->SetCaption(to_string(second_).c_str());
        second_num_ = second_;
    }
}

void TimeSettingWindowNew::SetColColor(int col, int color)
{
    if (1 == col) {
       m_year_top_ind->SetCaptionColor(color);
       m_year_top_ind->Refresh();
       m_year_bottom_ind->SetCaptionColor(color);
       m_year_bottom_ind->Refresh();
       m_year_label->SetCaptionColor(color);
       m_year_label->Refresh();
       m_year_text->SetCaptionColor(color);
       m_year_text->Refresh();
    } else if (2 == col) {
       m_month_top_ind->SetCaptionColor(color);
       m_month_top_ind->Refresh();
       m_month_bottom_ind->SetCaptionColor(color);
       m_month_bottom_ind->Refresh();
       m_month_label->SetCaptionColor(color);
       m_month_label->Refresh();
       m_month_text->SetCaptionColor(color);
       m_month_text->Refresh();

    } else if (3 == col) {
       m_day_top_ind->SetCaptionColor(color);
       m_day_top_ind->Refresh();
       m_day_bottom_ind->SetCaptionColor(color);
       m_day_bottom_ind->Refresh();
       m_day_label->SetCaptionColor(color);
       m_day_label->Refresh();
       m_day_text->SetCaptionColor(color);
       m_day_text->Refresh();

    }else if (4 == col) {
       m_hour_top_ind->SetCaptionColor(color);
       m_hour_top_ind->Refresh();
       m_hour_bottom_ind->SetCaptionColor(color);
       m_hour_bottom_ind->Refresh();
       m_hour_label->SetCaptionColor(color);
       m_hour_label->Refresh();
       m_hour_text->SetCaptionColor(color);
       m_hour_text->Refresh();

    } else if (5 == col) {
       m_minute_top_ind->SetCaptionColor(color);
       m_minute_top_ind->Refresh();
       m_minute_bottom_ind->SetCaptionColor(color);
       m_minute_bottom_ind->Refresh();
       m_minute_label->SetCaptionColor(color);
       m_minute_label->Refresh();
       m_minute_text->SetCaptionColor(color);
       m_minute_text->Refresh();

    }else if (6 == col) {
       m_second_top_ind->SetCaptionColor(color);
       m_second_top_ind->Refresh();
       m_second_bottom_ind->SetCaptionColor(color);
       m_second_bottom_ind->Refresh();
       m_second_label->SetCaptionColor(color);
       m_second_label->Refresh();
       m_second_text->SetCaptionColor(color);
       m_second_text->Refresh();

    } 
}

void TimeSettingWindowNew::SetActiveCol(int col)
{
    if (active_col_ == col) return;

    if (active_col_ != -1) {
        SetColColor(active_col_, gray_color);
    }
    SetColColor(col, black_color);
    active_col_ = col;
}

void TimeSettingWindowNew::UpdateNumRange()
{

    int mon_map[] = {
            1, 3, 5, 7, 8, 10, 12
    };

    int size = sizeof(mon_map)/sizeof(int);
    int i = 0;
    for (; i < size; i++) {
        if (month_num_ == mon_map[i])
            break;
    }

    if (i == size) {
        day_num_max_ = 30;
    } else {
        day_num_max_ = 31;
    }

    if (2 == month_num_) {
        if (day_num_ % 4) {
            day_num_max_ = 28;
        } else {
            day_num_max_ = 29;
        }
    }
}

void TimeSettingWindowNew::CalcNum(int col, int num)
{
    UpdateNumRange();

    int year_res = year_num_;
    int month_res = month_num_;
    int day_res = day_num_;
    int hour_res = hour_num_;
    int minute_res = minute_num_;
    int second_res = second_num_;

    if (col == 1) {
        year_res = (year_num_ + num + year_num_max_ - year_num_min_);
        year_res = year_res % year_num_max_ + year_num_min_;
    } else if (col == 2) {
        month_res = (month_num_ + num + month_num_max_ - month_num_min_);
        month_res = month_res % month_num_max_ + month_num_min_;
    } else if (col == 3) {
        day_res = (day_num_ + num + day_num_max_ - day_num_min_);
        day_res = day_res % day_num_max_ + day_num_min_;
    }else if (col == 4) {
        hour_res = (hour_num_ + num + hour_num_max_ - hour_num_min_);
        hour_res = hour_res % hour_num_max_ + hour_num_min_;
    } else if (col == 5) {
        minute_res = (minute_num_ + num + minute_num_max_ - minute_num_min_);
        minute_res = minute_res % minute_num_max_ + minute_num_min_;
    } else if (col == 6) {
        second_res = (second_num_ + num + second_num_max_ - second_num_min_);
        second_res = second_res % second_num_max_ + second_num_min_;
    }  else {
        db_error("unknown col[%d]", col);
    }

    SetText(year_res, month_res, day_res,hour_res,minute_res,second_res);
    UpdateNumRange();
    if (day_res > day_num_max_) {
        day_res = day_num_max_;
    }
    SetText(year_res, month_res, day_res,hour_res,minute_res,second_res);
}

void TimeSettingWindowNew::ShowCurrentDateTime()
{
	TextView *text = static_cast<TextView*>(GetControl("title"));
    string title_str;
	R::get()->GetString("ml_device_time", title_str);
	text->SetCaption(title_str.c_str());
    year_num_min_ = 1970;
    year_num_max_ = 2999;

    month_num_min_ = 1;
    month_num_max_ = 12;

    day_num_min_ = 1;
    day_num_max_ = 31;

    hour_num_min_ = 0;
    hour_num_max_ = 24;

    minute_num_min_ = 0;
    minute_num_max_ = 60;

    second_num_min_ = 0;
    second_num_max_ = 60;

    DisplayCurrentDateTimeNum();


}

void TimeSettingWindowNew::DisplayCurrentDateTimeNum()
{
    int year, mon;
    time_t ct;
    struct tm *tp;
    time(&ct);
    tp = localtime(&ct);

    year = tp->tm_year + 1900;
    mon = tp->tm_mon + 1;
    db_debug("[%d, %d, %d, %d, %d, %d]", year, mon, tp->tm_mday,tp->tm_hour, tp->tm_min, tp->tm_sec);

    string year_s, month_s, day_s;
    R::get()->GetString("date_year", year_s);
    R::get()->GetString("date_month", month_s);
    R::get()->GetString("date_day", day_s);
    string hour_s, min_s, sec_s;
    R::get()->GetString("time_hour", hour_s);
    R::get()->GetString("time_minute", min_s);
    R::get()->GetString("time_second", sec_s);
    SetTitle(year_s, month_s, day_s,hour_s, min_s, sec_s);
    SetText(year, mon, tp->tm_mday,tp->tm_hour, tp->tm_min, tp->tm_sec);
    SetActiveCol(1);
}


void TimeSettingWindowNew::SetSystemDateTime()
{
    time_t ct;
    struct tm *tp;
    time(&ct);
    tp = localtime(&ct);

    tp->tm_year = year_num_ - 1900;
    tp->tm_mon = month_num_ - 1;
    tp->tm_mday = day_num_;

    tp->tm_hour = hour_num_;
    tp->tm_min = minute_num_;
    tp->tm_sec = second_num_;


    struct timeval tv;
    tv.tv_sec = mktime(tp);
    tv.tv_usec = 0;
    if (0 > settimeofday(&tv, NULL)) {
        db_error("set system date time failed! %s", strerror(errno));
    }
}
