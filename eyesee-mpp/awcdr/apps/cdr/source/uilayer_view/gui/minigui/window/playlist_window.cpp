/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file playlist_window.cpp
 * @brief 多媒体回放列表窗口
 * @author id:826
 * @version v0.3
 * @date 2016-11-04
 */
#include "window/playlist_window.h"
#include "resource/resource_manager.h"
#include "widgets/view_container.h"
#include "widgets/list_view.h"
#include "window/window_manager.h"
#include "window/playback_window.h"
#include "application.h"

#include "common/app_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "PlaylistWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(PlaylistWindow)


void PlaylistWindow::keyProc(int keyCode, int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_LEFT:
            break;
        case SDV_KEY_POWER:
            if( isLongPress == LONG_PRESS )
            {
                this->DoHide();
                WindowManager *win_mg = ::WindowManager::GetInstance();       
                listener_->notify(win_mg->GetWindow(WINDOWID_PREVIEW),MSG_SYSTEM_SHUTDOWN,1);
                break;
            }

            this->DoHide();
            static_cast<Window *>(parent_)->DoShow();
            break;
        case SDV_KEY_OK:
	     db_msg("[debug_zhb]------PlaylistWindow::keyProc---SDV_KEY_OK");
            Menultemkeyproc();
            break;
        case SDV_KEY_RIGHT:
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }  
}

int PlaylistWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    //db_msg("[debug_jaosn]:PlaylistWindow message = %d,wparam = %u,lparam = %lu",message,wparam,lparam);
    switch ( message ) {
        case MSG_PAINT: {
// this way will not erase bk ground and show transparent mask
#if 0
            HDC hdc = ::BeginPaint(hwnd);
            RECT rcTemp;

            GetClientRect (hwnd, &rcTemp);

            HDC mem_dc = CreateMemDC (RECTW(rcTemp), RECTH(rcTemp), 16, MEMDC_FLAG_HWSURFACE | MEMDC_FLAG_SRCALPHA,
                                  0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F);

            /* 设置一个半透明的刷子并填充矩形 */
            SetBrushColor (mem_dc, RGBA2Pixel (mem_dc, 0x00, 0x00, 0x00, 0x80));
            FillBox (mem_dc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));

            SetBkMode (mem_dc, BM_TRANSPARENT);
            BitBlt (mem_dc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp), hdc, rcTemp.left, rcTemp.top, 0);

            ::EndPaint(hwnd, hdc);
            return HELP_ME_OUT;
        }
        case MSG_ERASEBKGND:
            return 0;
#else
            return HELP_ME_OUT;
        }
        //case MSG_ERASEBKGND:
        //    return HELP_ME_OUT;
#endif
        case MSG_KEYUP:{
            isKeyUp = true;
            //db_msg("[debug_jaosn]:short MSG_KEYUP wparam = %u ; downKey = %d",wparam,downKey);
            if(wparam == downKey){
            KillTimer(hwnd, ID_PLYLIST_TIMER_KEY);
            //db_msg("[debug_jaosn]:short MSG_KEYUP");
            keyProc(wparam, SHORT_PRESS);
            }
        }
            break;
        case MSG_KEYDOWN:{
            //db_msg("[debug_jaosn]:short MSG_KEYDOWN");
            if (!GetVisible()) {
                db_warn("window is hiden, ignore key event");
                break;
            }
            if(isKeyUp == true) {
            downKey = wparam;
            SetTimer(hwnd, ID_PLYLIST_TIMER_KEY, LONG_PRESS_TIME);
            isKeyUp = false;
            }
            }
            break;
        case MSG_KEYLONGPRESS:
            //db_msg("[debug_jaosn]:long press\n");
            downKey = -1;
            keyProc(wparam, LONG_PRESS);
            break;
        case MSG_TIMER:
            if(wparam == ID_PLYLIST_TIMER_KEY) {
            db_msg("[debug_jaosn]:short MSG_TIMER");
            isKeyUp = true;       
            SendMessage(hwnd, MSG_KEYLONGPRESS, downKey, 0);
            KillTimer(hwnd, ID_PLYLIST_TIMER_KEY);
                }
            break;
    default:
        //return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
        return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
   }
    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}

PlaylistWindow::PlaylistWindow(IComponent *parent)
    : SystemWindow(parent)
    , list_view_(NULL)
    , isKeyUp(true)
    , downKey(0)
{
    Load();

    SetWindowBackImage(R::get()->GetImagePath("bg").c_str());

    listener_ = WindowManager::GetInstance();

    list_view_ = reinterpret_cast<ListView *> (GetControl("list_view"));
    list_view_->OnItemClick.bind(this, &PlaylistWindow::MenuItemClickProc);

    string menu_list_bmp = R::get()->GetImagePath("bg");
    db_msg("set back ground pic:%s", menu_list_bmp.c_str());
    list_view_->SetWindowBackImage(menu_list_bmp.c_str());

    std::vector<LVCOLUMN> columns;

    LVCOLUMN col;
    col.width = 0;
    col.pfnCompare = NULL;
    col.colFlags = 0;

    col.width = 80;
    col.pszHeadText = "type";
    columns.push_back(col);

    col.width = 400;
    col.pszHeadText = "name";
    columns.push_back(col);

    list_view_->SetColumns(columns, false);
	
    ::LoadBitmapFromFile(HDC_SCREEN, &video_type_, "/usr/share/minigui/res/images/video.png");
    ::LoadBitmapFromFile(HDC_SCREEN, &pic_type_, "/usr/share/minigui/res/images/null.png");

    GraphicView::LoadImage(GetControl("confirm"), "confirm_bt");
    GraphicView::LoadImage(GetControl("cancel"), "cancel_bt");

    GraphicView *view;

    view = reinterpret_cast<GraphicView *>(GetControl("confirm"));
    view->SetTag(PLAYLIST_CONFIRM_BUTTON);
    view->OnClick.bind(this, &PlaylistWindow::ButtonClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("cancel"));
    view->SetTag(PLAYLIST_CANCEL_BUTTON);
    view->OnClick.bind(this, &PlaylistWindow::ButtonClickProc);

    GetControl("tl_playlist")->SetCaption("Please choose a file to play");
}

PlaylistWindow::~PlaylistWindow()
{
    ::UnloadBitmap(&video_type_);
    ::UnloadBitmap(&pic_type_);
}

void PlaylistWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

string PlaylistWindow::GetResourceName()
{
    return string(GetClassName());
}

void PlaylistWindow::Update(MSG_TYPE msg, int p_CamID)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}

void PlaylistWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
}

void PlaylistWindow::MenuItemClickProc(View *control)
{
    LVITEM item;

    if (list_view_->GetSelectedItem(item) < 0 || item.itemData == 0) {
        db_warn("GetSelectedItem failed");
        return;
    }

    db_msg("ready to play file: %s", list_view_->GetItemText(item.nItem, 1).c_str());

    listener_->notify(this, (PLAYLIST_BASE + item.nItem + 1), 0);
}
void PlaylistWindow::Menultemkeyproc()
{
    LVITEM item;

    if (list_view_->GetSelectedItem(item) < 0 || item.itemData == 0) {
        db_warn("GetSelectedItem failed");
        return;
    }

    db_msg("ready to play file: %s", list_view_->GetItemText(item.nItem, 1).c_str());

    listener_->sendmsg(this, (PLAYLIST_BASE + item.nItem + 1), 0);
    this->DoHide();
    static_cast<Window *>(parent_)->DoShow();
    listener_->sendmsg(this, PLAYLIST_CONFIRM_PLAYFILE, 1);

}


void PlaylistWindow::ButtonClickProc(View *control)
{
    int tag = control->GetTag();

    if (tag == PLAYLIST_CONFIRM_BUTTON) {
        // TODO: start playback ths choiced file
        this->DoHide();
        listener_->notify(this, PLAYLIST_CONFIRM_PLAYFILE, 1);
    } else if (tag == PLAYLIST_CANCEL_BUTTON) {
        this->DoHide();
    }
}

void PlaylistWindow::AddFileToList(const vector<string> &file_list)
{
    // remove last list
    playlist_.clear();
    list_view_->RemoveAllItems();

    LVITEM item;
    item.dwFlags &= ~LVIF_FOLD;
    item.nItemHeight = 40;

    LVSUBITEM subdata;
    subdata.nTextColor = PIXEL_black;

    playlist_ = file_list;
    vector<string>::const_iterator c_it;
    for ( c_it = playlist_.begin(); c_it != playlist_.end(); ++c_it) {
        subdata.pszText = NULL;
        subdata.flags = LVFLAG_BITMAP;
        subdata.image = (DWORD)&video_type_;
        std::vector <LVSUBITEM> item_datas;
        item_datas.push_back(subdata);

        subdata.pszText = strdup(c_it->substr(c_it->rfind('/') + 1).c_str());
        subdata.flags = 0;
        subdata.image = 0;
        item_datas.push_back(subdata);

        item.nItem = c_it - playlist_.begin();
        list_view_->AddItemWithDatas(item, item_datas);

        free(subdata.pszText);
    }

}

void PlaylistWindow::DoShow()
{
    Window::DoShow();
}

void PlaylistWindow::DoHide()
{
    SetVisible(false);
}

