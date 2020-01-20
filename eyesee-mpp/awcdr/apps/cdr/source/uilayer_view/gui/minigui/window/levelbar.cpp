/* *******************************************************************************
 * Copyright (C), 2017-2027, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file prompt.cpp
 * @brief 提示窗口
 * @author id:fangjj
 * @version v10.
 * @date 2017-04-24
 */
 #define NDEBUG 
 #include "window/levelbar.h"
#include "window/setting_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/screensaver.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "LevelBar"
#define F_B_OFFSET_TOP 109 //the second level item string bg offset the top
using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(LevelBar)

int LevelBar::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch( message )
    {
        case MSG_PAINT:
			db_msg("debug_zhb-----MSG_PAINT-----");
            return HELP_ME_OUT;
        case MSG_TIMER:
            break;
        case MSG_ERASEBKGND:
            {
                db_msg("debug_zhb-----MSG_ERASEBKGND-----");
            }
            break;
        case MSG_LBUTTONDOWN:
            int mouseX, mouseY;
            
            mouseX = LOSWORD (lparam);
            mouseY = HISWORD (lparam);
            if(mouseY > 90 || mouseX > 620)
                break;
            current_index = mouseX/(620/7);
            if(current_index < 0)
                current_index = 7;
            break;

        case MSG_LBUTTONUP:
                UpdateLevelBar(current_index, last_index);
                SetLevelBrightness();
            break;
        case MSG_MOUSEMOVE:
            {
#if 0
                UpdateLevelBar(current_index, true);
                SetLevelBrightness();
                int mouseX, mouseY;

                mouseX = LOSWORD (lparam);
                mouseY = HISWORD (lparam);
                db_msg("x=%d, y=%d", mouseX, mouseY);

                if(mouseY > mouseY || mouseX > 620)
                {
                    break;
                }

                current_index = mouseX/(620/7);
                UpdateLevelBar(current_index, true);
                SetLevelBrightness();
#endif
            }
            break;
        default:
            break;
    }

    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}


void LevelBar::keyProc(int keyCode,int isLongPress)
{
    //pthread_mutex_lock(&l_lock_);
    switch(keyCode){
        case SDV_KEY_LEFT:
            db_msg("[debug_zhb]----LevelBar---SDV_KEY_LEFT");
            //	if(!m_finsh){
            //		db_msg("[debug_zhb]---> no do anything");
            //		break;
            //		}
            //	m_finsh = false;
            if(--current_index < 0){
                current_index = 0;
                //	m_finsh = true;
                return;
            }
            UpdateLevelBar(current_index, last_index);
            SetLevelBrightness();
            break;
        case SDV_KEY_RIGHT:
            db_msg("[debug_zhb]----LevelBar---SDV_KEY_RIGHT");
            //	if(!m_finsh){
            //		db_msg("[debug_zhb]---> no do anything");
            //		break;
            //		}
            //	m_finsh = false;
            if(++current_index > NUMBER_LEVELBAR-1){
                current_index = NUMBER_LEVELBAR-1;
                //		m_finsh = true;
                return;
            }
            UpdateLevelBar(current_index, last_index);
            SetLevelBrightness();
            break;
        case SDV_KEY_POWER:
        case SDV_KEY_OK:
            db_msg("[debug_zhb]----LevelBar---SDV_KEY_OK/SDV_KEY_POWER");
            //	if(!m_finsh){
            //		db_msg("[debug_zhb]---> no do anything");
            //		break;
            //		}
            //	m_finsh = false;
            HideLevelBar();
            break;
        default:
            break;
    }
    //pthread_mutex_unlock(&l_lock_);

    return;
}

LevelBar::LevelBar(IComponent *parent)
        : SystemWindow(parent)
        ,current_index(0)
        ,last_index(0)
        ,image_icon_y(-1)
        ,image_png_y(-1)
        ,m_finsh(false)
        ,m_levelbar_flag(false)
{
    wname = "levelBar";
    Load();
    listener_ = WindowManager::GetInstance();
    // SetBackColor(0x96000000);
    string bkgnd_bmp = R::get()->GetImagePath("bg_levelbar");
    SetWindowBackImage(bkgnd_bmp.c_str()); 
    for(int i = 0 ; i < NUMBER_LEVELBAR ; i++ ){
        GraphicView::LoadImage(GetControl(levelbar_[i].image_icon), levelbar_[i].image_png);
        GetControl(levelbar_[i].image_icon)->Hide();
        GraphicView::LoadImage(GetControl(levelbar_[i].image_B_icon), levelbar_[i].image_B_png);
        GetControl(levelbar_[i].image_B_icon)->Hide();
    }
    pthread_mutex_init(&l_lock_, NULL);
}

LevelBar::~LevelBar()
{
 	
}
int LevelBar::getCurrentIdex()
{
   return current_index;
}

void LevelBar::HideLevelBar()
{
	GetControl(levelbar_[current_index].image_icon)->Hide();
	GetControl(levelbar_[current_index].image_B_icon)->Hide();

    Window *win = static_cast<Window *>(parent_);
    listener_ = WindowManager::GetInstance();
    listener_->sendmsg(win,SETTING_LEVELBAR_HIDE,0);
    m_levelbar_flag = false;
}
void LevelBar::SetLevelBrightness()
{
    Window *win = static_cast<Window *>(parent_);
    listener_ = WindowManager::GetInstance();
    listener_->sendmsg(win, SETTING_LEVELBAR_BRIGHTNESS, 0);
}
void LevelBar::UpdateLevelBar(int &index_show, int &index_hide)
{
    db_msg("[ghy], index_show=%d, index_hide=%d", index_show, index_hide);

    //hide 
    GraphicView::LoadImage(GetControl(levelbar_[index_hide].image_icon), levelbar_[index_hide].image_png);
    GetControl(levelbar_[index_hide].image_icon)->Hide();
    GraphicView::LoadImage(GetControl(levelbar_[index_hide].image_B_icon), levelbar_[index_hide].image_B_png);
    GetControl(levelbar_[index_hide].image_B_icon)->Hide();

    //show
    RECT b_icon_rect,icon_rect;
    GetControl(levelbar_[index_show].image_icon)->GetRect(&icon_rect);
    //db_msg("[debug_zhb]--UpdateLevelBar----image_icon_y = %d-----x = %d y = %d w = %d h=%d  levelbar_[index_].image_icon = %s",image_png_y,icon_rect.left,image_png_y,icon_rect.right-icon_rect.left,(icon_rect.bottom-icon_rect.top),levelbar_[index_].image_icon);
    GetControl(levelbar_[index_show].image_B_icon)->GetRect(&b_icon_rect);
    //db_msg("[debug_zhb]--UpdateLevelBar----image_png_y = %d-----x = %d y = %d w = %d h=%d  levelbar_[index_].image_B_icon = %s",image_png_y,b_icon_rect.left,image_png_y,b_icon_rect.right-b_icon_rect.left,(b_icon_rect.bottom-b_icon_rect.top),levelbar_[index_].image_B_icon);
    GetControl(levelbar_[index_show].image_icon)->SetPosition(icon_rect.left, image_icon_y, icon_rect.right-icon_rect.left, (icon_rect.bottom - icon_rect.top));
    GetControl(levelbar_[index_show].image_B_icon)->SetPosition(b_icon_rect.left, image_png_y, b_icon_rect.right - b_icon_rect.left, (b_icon_rect.bottom - b_icon_rect.top));
    GraphicView::LoadImage(GetControl(levelbar_[index_show].image_icon), levelbar_[index_show].image_png);
    GetControl(levelbar_[index_show].image_icon)->Show();
    GraphicView::LoadImage(GetControl(levelbar_[index_show].image_B_icon), levelbar_[index_show].image_B_png);
    GetControl(levelbar_[index_show].image_B_icon)->Show();
    //	m_finsh = true;
    //usleep(300*1000);
    index_hide = index_show;
    db_msg("debug_zhb]---->updatelevelbar____finsh");
}

void LevelBar::ShowLevelBar(int levelbar_id, RECT & rect_, int index_)
{
	m_levelbar_flag = true;
	RECT b_icon_rect,icon_rect;
	current_index = index_;
	db_msg("debug_zhb---->ShowLevelBar---> index_ = %d");
	GraphicView::LoadImage(GetControl(levelbar_[index_].image_icon), levelbar_[index_].image_png);
	GraphicView::LoadImage(GetControl(levelbar_[index_].image_B_icon), levelbar_[index_].image_B_png);
	GetControl(levelbar_[index_].image_icon)->GetRect(&icon_rect);
	image_icon_y = rect_.top-F_B_OFFSET_TOP;
	//db_msg("[debug_zhb]--ShowLevelBar--image_icon_y = %d--x = %d y = %d w = %d h=%d  index = %d",image_icon_y,rect_.left,rect_.top,rect_.right-rect_.left,rect_.bottom-rect_.top,index_);
	if(LEVELBAR_SCREEN_BRIGHTNESS == levelbar_id)
		GetControl(levelbar_[index_].image_icon)->SetPosition(icon_rect.left,rect_.top-F_B_OFFSET_TOP,icon_rect.right-icon_rect.left,(icon_rect.bottom-icon_rect.top));
	else
		GetControl(levelbar_[index_].image_icon)->SetPosition(icon_rect.left,rect_.top-F_B_OFFSET_TOP,icon_rect.right-icon_rect.left,(icon_rect.bottom-icon_rect.top));
	GetControl(levelbar_[index_].image_B_icon)->GetRect(&b_icon_rect);
	image_png_y = rect_.top-F_B_OFFSET_TOP+(b_icon_rect.bottom-b_icon_rect.top)-10;
	//db_msg("[debug_zhb]--ShowLevelBar--111--image_png_y = %d-----x = %d y = %d w = %d h=%d  index = %d",image_png_y,b_icon_rect.left,rect_.top-F_B_OFFSET_TOP+(b_icon_rect.bottom-b_icon_rect.top),b_icon_rect.right-b_icon_rect.left,(b_icon_rect.bottom-b_icon_rect.top),index_);
	GetControl(levelbar_[index_].image_B_icon)->SetPosition(b_icon_rect.left,rect_.top-F_B_OFFSET_TOP+(b_icon_rect.bottom-b_icon_rect.top)-10,b_icon_rect.right-b_icon_rect.left,(b_icon_rect.bottom-b_icon_rect.top));
	GetControl(levelbar_[index_].image_icon)->Show();
	GetControl(levelbar_[index_].image_B_icon)->Show();
    last_index = index_;
	//m_finsh = true;
}
void LevelBar::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string LevelBar::GetResourceName()
{
    return string(GetClassName());
}

void LevelBar::Update(MSG_TYPE msg, int p_CamID)
{
    switch (msg) {
        default:
            break;
    }
}

void LevelBar::DoShow()
{
    WindowManager *wm = WindowManager::GetInstance();
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::EnableWindow(cur_win->GetHandle(), false);
    Widget::Show();
}

void LevelBar::DoHide()
{
    WindowManager *wm = WindowManager::GetInstance();
    Window *cur_win = wm->GetWindow(wm->GetCurrentWinID());
    ::SetActiveWindow(cur_win->GetHandle());
    ::EnableWindow(cur_win->GetHandle(), true);
    Widget::Hide();
}

void LevelBar::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("LevelBar_icon1")||
		ctrl_name == string("LevelBar_icon2")||
		ctrl_name == string("LevelBar_icon3")||
		ctrl_name == string("LevelBar_icon4")||
		ctrl_name == string("LevelBar_icon5")||
		ctrl_name == string("LevelBar_icon6")||
		ctrl_name == string("LevelBar_icon7")) {
        ctrl->SetCtrlTransparentStyle(false);
    }else if(ctrl_name == string("LevelBar_B_icon1")||
		ctrl_name == string("LevelBar_B_icon2")||
		ctrl_name == string("LevelBar_B_icon3")||
		ctrl_name == string("LevelBar_B_icon4")||
		ctrl_name == string("LevelBar_B_icon5")||
		ctrl_name == string("LevelBar_B_icon6")||
		ctrl_name == string("LevelBar_B_icon7")){
	ctrl->SetCtrlTransparentStyle(true);
    	}
}

int LevelBar::OnMouseUp(unsigned int button_status, int x, int y)
{
    if (OnClick)
        OnClick(this);

    return HELP_ME_OUT;;
}

