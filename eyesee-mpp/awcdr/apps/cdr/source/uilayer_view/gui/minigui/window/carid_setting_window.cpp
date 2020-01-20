/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ok_dialog.cpp
 * @brief 车牌设置窗口
 * @author id:826
 * @version v1.0
 * @date 2018-01-12
 */
#include "carid_setting_window.h"

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

#include "lua/lua_config_parser.h"

#include "window/setting_window.h"
#include "device_model/menu_config_lua.h"

#include "window/sublist.h"
#include "widgets/button.h"


using namespace std;
using namespace EyeseeLinux;
// AARRGGBB
#define gray_color 0xFFB5B5B5
#define black_color 0xFF363636
#define caridhight_color  0xFF4DFFFF
#define caridnormal_color 0xFFFFFFFF
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x00000000
#define COLOR_OK_DIALOG_BUTTON_BG gray_color
#define COLOR_BUTTON_FG gray_color

IMPLEMENT_DYNCRT_CLASS(CaridSettingWindow)

int getCarSelcharlen(const char *str)
{
	int ret = 0;
	const char *p = str;
	if (p == NULL) return ret;
	while (*p) {
		if (*p & 0x80) {
			p += 3;
		} else {
			p ++;
		}
		ret ++;
	}
	return ret;
}

int getCarSelcharPos(const char *str, int pos)
{
	int ret = 0;
	const char *p = str;
	
	if (p == NULL) return ret;
	while (pos) {
		if (*p & 0x80) {
			p += 3;
			ret +=3;
		} else {
			p ++;
			ret ++;
		}
		pos --;
	}
	return ret;
}

/*****************************************************************************
 Function: ContainerWidget::HandleMessage
 Description: process the messages and notify the children
    @override
 Parameter:
 Return:
*****************************************************************************/

void CaridSettingWindow::keyProc(int keyCode, int isLongPress)
{
	db_warn("[debug_jaosn]:CaridSettingWindow  keyCode = %d ",keyCode);
    switch(keyCode){
        case SDV_KEY_RIGHT:
        {
        	caridsel_label_[selhighilghtid]->SetTag(CARID_SETTING_CARNUMSEL + selhighilghtid);
     		caridsel_label_[selhighilghtid]->SetCaptionColor(caridnormal_color);
     		//carid_label_[highlightid]->SetCaption(buff);
     		caridsel_label_[selhighilghtid]->Refresh();
     		selhighilghtid--;
     		if (selhighilghtid <= 0) 
			{
				selhighilghtid = 0;
				if (caridselstartindex <= CARIDSEL_MAXLEN)
					caridselstartindex = CARIDSEL_MAXLEN;
				caridselstartindex -= CARIDSEL_MAXLEN;
				DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);	
			}
     		caridsel_label_[selhighilghtid]->SetTag(CARID_SETTING_CARNUMSEL + CARIDSELHIGHLIGHT_MAXLEN + selhighilghtid);	
     		caridsel_label_[selhighilghtid]->SetCaptionColor(caridhight_color);
     		caridsel_label_[selhighilghtid]->Refresh();
			db_warn("[debug_jaosn]:SDV_KEY_LEFT ");
            break;
        }
        case SDV_KEY_MODE:
		case SDV_KEY_MENU:
			highlightid = 0;
			selhighilghtid = 0;
			OnConfirmClick(this, 1);
            break;
        case SDV_KEY_OK:
			//TextView *text = nullptr;
			//text = static_cast<TextView*>(GetControl(buffcap));	// text0~text7
     		char buff[8];
     		caridsel_label_[selhighilghtid]->GetCaption(buff,8);
     		db_warn("----highlightid = %d,selhighilghtid = %d",highlightid,selhighilghtid);
     		#if 0
     		carid_label_[highlightid]->SetCaption(buff);
     		carid_label_[highlightid]->Refresh();
     		#else
     		// 焦点自动后移
     		carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + highlightid);
     		carid_label_[highlightid]->SetCaptionColor(caridnormal_color);
     		carid_label_[highlightid]->SetCaption(buff);
     		carid_label_[highlightid]->Refresh();
     		highlightid++;
     		if (highlightid >= CARIDSEL_MAXLEN) {
				highlightid = 0;
				selhighilghtid = 0;
				SetSystemNewDevname(Caridstring);
				MenuConfigLua::GetInstance()->SetDeviceCaiId(Caridstring);		// 保存到menu
				OnConfirmClick(this, 1);
			}else{
				 carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG + highlightid);	
     			carid_label_[highlightid]->SetCaptionColor(caridhight_color);
     			carid_label_[highlightid]->Refresh();
			}
			if(highlightid == 1){
				SetDiffLetterContentType();
			}else if(highlightid >= 2){
				SetDiffNumContentType();
			}
     		#endif
            break;
        case SDV_KEY_LEFT:
        {
             caridsel_label_[selhighilghtid]->SetTag(CARID_SETTING_CARNUMSEL + selhighilghtid);
     		caridsel_label_[selhighilghtid]->SetCaptionColor(caridnormal_color);
     		//carid_label_[highlightid]->SetCaption(buff);
     		caridsel_label_[selhighilghtid]->Refresh();
     		selhighilghtid++;
     		if (selhighilghtid >= CARIDSELHIGHLIGHT_MAXLEN) 
			{
				selhighilghtid = 0;
				if (caridselstartindex +CARIDSEL_MAXLEN > caridselstrmax)
				caridselstartindex = caridselstrmax-CARIDSEL_MAXLEN;
				caridselstartindex += CARIDSEL_MAXLEN;
				DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
				
			}
     		caridsel_label_[selhighilghtid]->SetTag(CARID_SETTING_CARNUMSEL + CARIDSELHIGHLIGHT_MAXLEN + selhighilghtid);	
     		caridsel_label_[selhighilghtid]->SetCaptionColor(caridhight_color);
     		caridsel_label_[selhighilghtid]->Refresh();
            break;
        }
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }
}

int CaridSettingWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch ( message ) {
        case MSG_PAINT:
    	{
			HDC hdc = ::BeginPaint(hwnd);
            RECT rect;
			int i;
			gal_pixel pencolor = ::SetPenColor(hdc, caridnormal_color);
            //::GetWindowRect(hwnd, &rect);
			for (i=0; i<CARID_MAXLEN; i++ ) {
				if (CARID_HIGHTTAG & (carid_label_[i]->GetTag()))
					::SetPenColor(hdc, caridhight_color);	// highlight
				else
					::SetPenColor(hdc, COLOR_WHITE);
				carid_label_[i]->GetRect(&rect);
				::Rectangle(hdc, rect.left+1, rect.top+1, rect.right-1, rect.bottom-1);
			
			}
			for (i=0; i<CARIDSEL_MAXLEN; i++ ) {
				if (CARID_HIGHTTAG & (caridsel_label_[i]->GetTag()))
					::SetPenColor(hdc, caridhight_color);	// highlight
				else
					::SetPenColor(hdc, COLOR_WHITE);
				//::SetPenColor(hdc, COLOR_WHITE);
				caridsel_label_[i]->GetRect(&rect);
				::Rectangle(hdc, rect.left+1, rect.top+1, rect.right-1, rect.bottom-1);
			
			}
			::SetPenColor(hdc,pencolor);
            ::EndPaint(hwnd, hdc);
            return HELP_ME_OUT;
        }
		
        case MSG_MOUSE_FLING:
        {
			db_msg("MSG_MOUSE_FLING");
			int direction = LOSWORD (wparam);
            if (direction == MOUSE_LEFT) {
				
            } else if (direction == MOUSE_RIGHT) {
            }
        }
            return HELP_ME_OUT;
		 case MSG_KEYDOWN:
            {
                db_warn("[debug_jaosn]:short MSG_KEYDOWN");
                keyProc(wparam, SHORT_PRESS);
            }
            break;
        default:
            break;
    }

    return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
}

CaridSettingWindow::CaridSettingWindow(IComponent *parent)
    : SystemWindow(parent)
{
	char buff[8];
	int i;
	Load();

    SetBackColor(COLOR_BLACK);

    GraphicView *title_bg = static_cast<GraphicView*>(GetControl("title_bg"));
    title_bg->SetBackColor(black_color);

    GraphicView *confirm = static_cast<GraphicView*>(GetControl("confirm_button"));
    GraphicView::LoadImage(confirm, "button_time_setting_return");
    confirm->SetTag(CARID_SETTING_CONFIRM_BUTTON);
    confirm->OnClick.bind(this, &CaridSettingWindow::ButtonProc);

	GraphicView *carselleft = static_cast<GraphicView*>(GetControl("left_button"));
    GraphicView::LoadImage(carselleft, "button_last_file");
    carselleft->SetTag(CARID_SETTING_CARNUMSELLEFT);
    carselleft->OnClick.bind(this, &CaridSettingWindow::ButtonProc);

	GraphicView *carselright = static_cast<GraphicView*>(GetControl("right_button"));
    GraphicView::LoadImage(carselright, "button_next_file");
    carselright->SetTag(CARID_SETTING_CARNUMSELRIGHT);
    carselright->OnClick.bind(this, &CaridSettingWindow::ButtonProc);
	
    TextView *text = static_cast<TextView*>(GetControl("title"));
    text->SetCaptionColor(COLOR_WHITE);
	// 注意: 字符串为 utf8编码,汉字为3个字节
	// Caridstring = "京1234567";
	
    std::string ss = "01234567";
    Caridstring = ss;//xx SettingCtrl::GetInstance()->GetCaridString(ss);
	#if 0
	const char *p = Caridstring.c_str();
	char buff[256];
	for (int i=0; i<Caridstring.length(); i++ )
		sprintf(buff+3*i,"%02x ",*(p+i) );
	db_msg("Caridstring: %s",buff);		// Caridstring: e4 ba ac 31 32 33 34 35 36 37	// utf8

	const wchar_t wtext[] = L"京1234567" ;
	for (int i=0; i<8; i++ )
		sprintf(buff+5*i,"%04x ",*(wtext+i) );
	db_msg("Caridstring: %s",buff);		// Caridstring: 4eac 0031 0032 0033 0034 0035 0036 0037		// ucs2l
	#endif
	for (i=0; i<CARID_MAXLEN; i++) {
		sprintf(buff,"text%d",i);
		carid_label_[i] = static_cast<TextView*>(GetControl(buff));	// carid_label_[i] -> text0~7
		carid_label_[i]->SetTag( CARID_SETTING_CARNUM + i );
		carid_label_[i]->TextOnClick.bind(this, &CaridSettingWindow::ButtonProc);
	}
	carid_label_[0]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG);
	carid_label_[0]->SetCaptionColor(caridhight_color);
	highlightid = 0;
	for (i=0; i<CARIDSEL_MAXLEN; i++) {
		sprintf(buff,"htext%d",i);
		caridsel_label_[i] = static_cast<TextView*>(GetControl(buff));	// caridsel_label_[i] -> htext0~7
		caridsel_label_[i]->TextOnClick.bind(this, &CaridSettingWindow::ButtonProc);
		caridsel_label_[i]->SetTag( CARID_SETTING_CARNUMSEL + i);
		caridsel_label_[i]->SetCaptionColor(COLOR_WHITE);
	}
	caridsel_label_[0]->SetTag(CARID_SETTING_CARNUMSEL + CARID_HIGHTTAG);
	caridsel_label_[0]->SetCaptionColor(caridhight_color);
	selhighilghtid = 0;
}

CaridSettingWindow::~CaridSettingWindow()
{
}

void CaridSettingWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string CaridSettingWindow::GetResourceName()
{
    return string(GetClassName());
}

void CaridSettingWindow::DoShow()
{
    Window::DoShow();
    ::EnableWindow(parent_->GetHandle(), false);
}

void CaridSettingWindow::DoHide()
{
    SetVisible(false);
    ::EnableWindow(parent_->GetHandle(), true);
    if (parent_->GetVisible()) {
        ::SetActiveWindow(parent_->GetHandle());
    }
}

void CaridSettingWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
#if 0
    if (ctrl_name == string("title_bg")
            || ctrl_name == string("lt_ind")
            || ctrl_name == string("ld_ind")
            || ctrl_name == string("mt_ind")
            || ctrl_name == string("md_ind")
            || ctrl_name == string("rt_ind")
            || ctrl_name == string("rd_ind")) {
         ctrl->SetCtrlTransparentStyle(false);
     }
#endif
}

int CaridSettingWindow::OnMouseUp(unsigned int button_status, int x, int y)
{
//    if (TextOnClick)
//        TextOnClick(this);

    return HELP_ME_OUT;;
}

void CaridSettingWindow::ButtonProc(View *control)
{
    int tag = control->GetTag();
	//db_msg("click tag: %08x",tag);
    if ((tag & CARID_SETTING_CONFIRM_BUTTON) && OnConfirmClick) {
        SetSystemNewDevname(Caridstring);
		MenuConfigLua::GetInstance()->SetDeviceCaiId(Caridstring);		// 保存到menu
		
        OnConfirmClick(this, 1);
		
    } else if (tag & CARID_SETTING_CARNUM) {
		// 切换焦点
		//for (int i=0; i<CARID_MAXLEN; i++) {
		//	if (CARID_HIGHTTAG & carid_label_[i]->GetTag()) {
		//		carid_label_[i]->SetTag(CARID_SETTING_CARNUM + i);
		//	}
		//}
		// old hight id
		carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + highlightid);
		carid_label_[highlightid]->SetCaptionColor(caridnormal_color);
		carid_label_[highlightid]->Refresh();
		// new hight id
		highlightid = tag & 0xff;
		//((TextView*)control)->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG);
		carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG + highlightid);	
		carid_label_[highlightid]->SetCaptionColor(caridhight_color);
		carid_label_[highlightid]->Refresh();
    } else if (tag & CARID_SETTING_CARNUMSEL) {
		// 把字符复制到有焦点的控件上
		char buff[8];
		((TextView*) control)->GetCaption(buff,8);
		db_msg("click %s",buff);
		#if 0
		carid_label_[highlightid]->SetCaption(buff);
		carid_label_[highlightid]->Refresh();
		#else
		// 焦点自动后移
		carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + highlightid);
		carid_label_[highlightid]->SetCaptionColor(caridnormal_color);
		carid_label_[highlightid]->SetCaption(buff);
		carid_label_[highlightid]->Refresh();
		highlightid++;
		if (highlightid >= CARIDSEL_MAXLEN) highlightid = 0;
		carid_label_[highlightid]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG + highlightid);	
		carid_label_[highlightid]->SetCaptionColor(caridhight_color);
		carid_label_[highlightid]->Refresh();
		#endif
    } else if (tag == CARID_SETTING_CARNUMSELLEFT) {
		if (caridselstartindex <= CARIDSEL_MAXLEN)
			caridselstartindex = CARIDSEL_MAXLEN;
		caridselstartindex -= CARIDSEL_MAXLEN;
		DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
    } else if (tag == CARID_SETTING_CARNUMSELRIGHT) {
		if (caridselstartindex +CARIDSEL_MAXLEN > caridselstrmax)
			caridselstartindex = caridselstrmax-CARIDSEL_MAXLEN;
		caridselstartindex += CARIDSEL_MAXLEN;
		DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
    }
}


void CaridSettingWindow::SetTextViewCaption(const std::string &view_id, const std::string &text)
{
    TextView *title = static_cast<TextView*>(GetControl(view_id.c_str()));
    title->SetBackColor(COLOR_OK_DIALOG_BUTTON_BG);
    title->SetCaptionColor(COLOR_BUTTON_FG);
    title->SetCaption(text.c_str());
}

void CaridSettingWindow::SetTitle(const std::string& l, const std::string& m, const std::string& r)
{
}

void CaridSettingWindow::SetText(int l, int m, int r)
{
}

void CaridSettingWindow::SetColColor(int col, int color)
{
}

void CaridSettingWindow::SetActiveCol(int col)
{
}

void CaridSettingWindow::UpdateNumRange()
{
}

void CaridSettingWindow::CalcNum(int col, int num)
{
}

void CaridSettingWindow::SetContentType()
{
	int i;
	content_type_ = CONTENT_TYPE_CARID;
    TextView *text = static_cast<TextView*>(GetControl("title"));
    string title_str;

    R::get()->GetString("ml_device_carid_config", title_str);

	std::string ss = "0123456701234567";
	Caridstring = ss; //xx SettingCtrl::GetInstance()->GetCaridString(ss);
    MenuConfigLua::GetInstance()->GetDeviceCaiId(Caridstring);		// 取出menu的值 UTF8
    db_error("Caridstring: %s",Caridstring.c_str());
	highlightid = 0;
	for (i=0; i<CARID_MAXLEN; i++) {
		carid_label_[i]->SetTag( CARID_SETTING_CARNUM + i );
		carid_label_[i]->SetCaptionColor(caridnormal_color);
	}
	carid_label_[0]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG);
	carid_label_[0]->SetCaptionColor(caridhight_color);
	
    DisplayCurrentCaridNum(Caridstring);

    text->SetCaption(title_str.c_str());
	int curlang = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);	
	R::get()->GetString("all_carchar", CaridSelstringtmp);
	caridselstartindex = 0;

	int ix = CaridSelstringtmp.find("1234567890",0);
	if (curlang == 0) {	// 简体中文
		CaridSelstring = CaridSelstringtmp;
	} else {
		CaridSelstring = CaridSelstringtmp.substr(ix);
	}
	
	caridselstrmax = getCarSelcharlen(CaridSelstring.c_str());
	db_msg("caridselstrmax = %d",caridselstrmax);
	DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
	
	
}

void CaridSettingWindow::SetDiffNumContentType()
{
	int i;
	#if 0
	content_type_ = CONTENT_TYPE_CARID;
    TextView *text = static_cast<TextView*>(GetControl("title"));
    string title_str;

    R::get()->GetString("ml_device_carid_config", title_str);

	std::string ss = "0123456701234567";
	Caridstring = ss; //xx SettingCtrl::GetInstance()->GetCaridString(ss);
    MenuConfigLua::GetInstance()->GetDeviceCaiId(Caridstring);		// 取出menu的值 UTF8
    db_error("Caridstring: %s",Caridstring.c_str());
	highlightid = 0;
	for (i=0; i<CARID_MAXLEN; i++) {
		carid_label_[i]->SetTag( CARID_SETTING_CARNUM + i );
		carid_label_[i]->SetCaptionColor(caridnormal_color);
	}
	carid_label_[0]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG);
	carid_label_[0]->SetCaptionColor(caridhight_color);
	
    DisplayCurrentCaridNum(Caridstring);

    text->SetCaption(title_str.c_str());
	int curlang = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);	
	R::get()->GetString("all_carchar", CaridSelstringtmp);
	#endif
	caridselstartindex = 0;

	int ix = CaridSelstringtmp.find("1234567890",0);
	//if (curlang == 0) {	// 简体中文
	//	CaridSelstring = CaridSelstringtmp;
//	} else {
		CaridSelstring = CaridSelstringtmp.substr(ix);
	//}
	
	caridselstrmax = getCarSelcharlen(CaridSelstring.c_str());
	db_warn("caridselstrmax = %d",caridselstrmax);
	DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
	
	
}

void CaridSettingWindow::SetDiffLetterContentType()
{
	int i;
	#if 0
	content_type_ = CONTENT_TYPE_CARID;
    TextView *text = static_cast<TextView*>(GetControl("title"));
    string title_str;

    R::get()->GetString("ml_device_carid_config", title_str);

	std::string ss = "0123456701234567";
	Caridstring = ss; //xx SettingCtrl::GetInstance()->GetCaridString(ss);
    MenuConfigLua::GetInstance()->GetDeviceCaiId(Caridstring);		// 取出menu的值 UTF8
    db_error("Caridstring: %s",Caridstring.c_str());
	highlightid = 0;
	for (i=0; i<CARID_MAXLEN; i++) {
		carid_label_[i]->SetTag( CARID_SETTING_CARNUM + i );
		carid_label_[i]->SetCaptionColor(caridnormal_color);
	}
	carid_label_[0]->SetTag(CARID_SETTING_CARNUM + CARID_HIGHTTAG);
	carid_label_[0]->SetCaptionColor(caridhight_color);
	
    DisplayCurrentCaridNum(Caridstring);

    text->SetCaption(title_str.c_str());
	int curlang = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);	
	R::get()->GetString("all_carchar", CaridSelstringtmp);
	#endif
	caridselstartindex = 0;

	int ix = CaridSelstringtmp.find("ABCDEFGHIJKLMNOPQRSTUVWXYZ",0);
	//if (curlang == 0) {	// 简体中文
	//	CaridSelstring = CaridSelstringtmp;
//	} else {
		CaridSelstring = CaridSelstringtmp.substr(ix);
	//}
	
	caridselstrmax = getCarSelcharlen(CaridSelstring.c_str());
	db_warn("caridselstrmax = %d",caridselstrmax);
	DisplayCurrentCarSelchar(CaridSelstring, caridselstartindex);
	
	
}


void CaridSettingWindow::DisplayCurrentCaridNum(std::string &str)
{
	TextView *text = nullptr;
	int i = 0;
	char buffcap[8];
	char buff[8];
	std::string s;
	int pos = 0;
	db_error("Carid: %s",str.c_str());
	const char *p = str.c_str();
	for (i=0; i<CARID_MAXLEN; i++) {
		sprintf(buffcap,"text%d",i);
		text = static_cast<TextView*>(GetControl(buffcap));	// text0~text7
		//s = str[i+1];
		//db_error("%s",s.c_str());		// 从1开始
		#if 1
		if (*p & 0x80) {
			// 取3个字节
			strncpy(buff,p,3);
			buff[3]=0;
			p+=3;
		} else {
			strncpy(buff,p,1);
			buff[1]=0;
			p++;
		}
		db_msg("%s",buff);
		text->SetCaption(buff);
		#else
		
		text->SetCaption(s.c_str());
		#endif
		
	}
	

}


void CaridSettingWindow::DisplayCurrentCarSelchar(std::string &str, int index)
{
	TextView *text = nullptr;
	int i = 0;
	char buff[8];
	std::string s;
	int len = 0;
	
	const char *p = str.c_str();
	len = getCarSelcharlen(p);
	if (index + CARIDSEL_MAXLEN >= len) {
		index = len - CARIDSEL_MAXLEN;
	}
	p += getCarSelcharPos(p, index);
	
	for (i=0; i<CARIDSEL_MAXLEN; i++) {
		sprintf(buff,"htext%d",i);
		text = static_cast<TextView*>(GetControl(buff));	// htext0~htext8
		#if 1
		if (*p & 0x80) {
			// 取3个字节
			strncpy(buff,p,3);
			buff[3]=0;
			p+=3;
		} else {
			strncpy(buff,p,1);
			buff[1]=0;
			p++;
		}
		//db_msg("%s",buff);
		text->SetCaption(buff);
		#else
		text->SetCaption(str[i+1]);
		#endif
		text->Refresh();
	}
	

}


void CaridSettingWindow::SetSystemNewDevname(std::string &devname)
{
	char buff[32],tmp[4];
	char *p;
	int pos = 0;
	for (int i=0; i<CARID_MAXLEN; i++) {
		carid_label_[i]->GetCaption(tmp, 3);
		p = tmp;
		if (*p & 0x80) {
			strncpy(buff+pos,p,3);
			pos+=3;
		} else {
			*(buff+pos) = *p;
			pos++;
		}
	}
	buff[pos] = 0;
	// 得到 utf8格式的字符串,需转成gb2312供水印代码使用
	//
	
	devname = (string)buff;
	//memcpy((char*)devname.c_str(),buff,pos+1);
	db_error("set carid: %s, length:%d",devname.c_str(),devname.length());
}
