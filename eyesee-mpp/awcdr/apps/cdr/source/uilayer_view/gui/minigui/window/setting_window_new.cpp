/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: menu_window.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/
//#define NDEBUG
#include "window/setting_window_new.h"
#include "debug/app_log.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "widgets/magic_block.h"
#include "window/window_manager.h"
#include "window/dialog.h"
#include "widgets/menu_items.h"
#include "device_model/menu_config_lua.h"
//#include "widgets/button.h"
#include "resource/resource_manager.h"
#include "window/time_setting_window.h"
#include "device_model/system/power_manager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"
#include "common/setting_menu_id.h"
#include "widgets/listbox_impl.h"
#include "window/levelbar.h"
#include "device_model/storage_manager.h"
#include "device_model/system/event_manager.h"
#include "window/promptBox.h"
#include "window/bulletCollection.h"
#include <pthread.h>
#include <time.h>
#include <sstream>
#include "device_model/version_update_manager.h"
#include "uilayer_view/gui/minigui/window/preview_window.h"
#include "window/prompt.h"
#include "device_model/partitionManager.h"
#include "device_model/download_4g_manager.h"
#include "dd_serv/common_define.h"

#include "device_model/system/device_qrencode.h"

#include "widgets/listbox_view.h"
#include "widgets/item_set_view.h"


using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(SettingWindowNew)



void SettingWindowNew::keyProc(int keyCode, int isLongPress)
{
    pthread_mutex_lock(&setwindow_proc_lock_);
    switch(keyCode){
        case SDV_KEY_LEFT://button2
            db_msg("[debug_zhb]----SettingWindowNew----SDV_KEY_LEFT");
            break;
        case SDV_KEY_POWER://button1
            {
                db_msg("[debug_zhb]----SettingWindowNew----SDV_KEY_POWER");
                break;
            }
        case SDV_KEY_RETURN:
        {
			db_msg("[debug_zhb]----SettingWindowNew----SDV_KEY_POWER");
        	break;
    	}
        case SDV_KEY_OK://button4
            {
                db_msg("[debug_zhb]---SettingWindowNew-----SDV_KEY_OK");
            }
            break;
        case SDV_KEY_RIGHT://button3
            db_msg("[debug_zhb]---SettingWindowNew-----SDV_KEY_RIGHT");
            break;
        case SDV_KEY_MODE:
            db_msg("[debug_zhb]----SettingWindowNew----SDV_KEY_MODE");
            break;
        case SDV_KEY_MENU:
	    db_msg("[debug_zhb]------SettingWindowNew::keyProc---SDV_KEY_MENU");
            listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }
    pthread_mutex_unlock(&setwindow_proc_lock_);
}


int SettingWindowNew::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    //db_msg("[debug_jason]:SettingWindow message = %d,wparam = %u,lparam = %lu",message,wparam,lparam);
    switch (message) {
        case MSG_PAINT: {
			db_msg("[debug_zhb]-----------setting window -----PAINT--");
#if 0
            HDC hdc = ::BeginPaint(hwnd);
            HDC mem_dc = CreateMemDC (1280, 200, 16, MEMDC_FLAG_HWSURFACE | MEMDC_FLAG_SRCALPHA,
                                  0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F);

            /* 设置一个半透明的刷子并填充矩形 */
            SetBrushColor (mem_dc, RGBA2Pixel (mem_dc, 0x00, 0x00, 0x00, 0xA0));
            FillBox (mem_dc, 0, 0, 1280, 200);

            SetBkMode (mem_dc, BM_TRANSPARENT);
            BitBlt (mem_dc, 0, 0, 1280, 200, hdc, 0, 600, 0);

            ::EndPaint(hwnd, hdc);
#endif
        }
            return HELP_ME_OUT;
		case MSG_TIMER:
            break;
        case MSG_KEYUP:
            return HELP_ME_OUT;
		case MSG_LBUTTONDOWN:
			{
			 int mouseX, mouseY;
			 mouseX = LOSWORD (lparam);
   			 mouseY = HISWORD (lparam);
			 //db_error("[debug_jason]:SettingWindownew message = %d,mouseX = %u,mouseY = %lu",message,mouseX,mouseY);
               }
			break;
        default:
            break;
    }
    return ContainerWidget::HandleMessage(hwnd, message, wparam, lparam);
}


SettingWindowNew::SettingWindowNew(IComponent *parent)
    : SystemWindow(parent)
    ,win_mg_(WindowManager::GetInstance())
    ,lbv(NULL)
    ,listboxView(NULL)
{
    db_msg(" ");
    Load();
    //SetBackColor(0xFF1A1E38);//yellow
    SetBackColor(0xFFff0000);//yellow
    db_warn("[habo]---> SettingWindowNew  init \n");
     listboxView = ListboxDataInit(listboxView);
    // InitListBoxView(listboxView);
      db_warn("[habo]---> SettingWindowNew  init 11111\n");
    lbv = reinterpret_cast<ListBoxView *> (GetControl("listbox_view"));
     db_warn("[habo]---> SettingWindowNew  222222 \n");
    lbv->OnListBoxViewClick.bind(this, &SettingWindowNew::ListBoxViewClickProc);
    //lbv->SetBackColor(0xFF1A1E38);
    db_warn("[habo]---> SettingWindowNew  33333 \n");
    lbv->add(*listboxView);
    lbv->Show();
   
}

SettingWindowNew::~SettingWindowNew()
{
	
}

std::string SettingWindowNew::GetResourceName()
{
    db_msg(" ");
    return std::string(GetClassName());
}

void SettingWindowNew::PreInitCtrl(View *ctrl, std::string &ctrl_name)
{
    ctrl->SetCtrlTransparentStyle(true);
}


void SettingWindowNew::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
	db_msg("msg:%d",msg);
    switch ((int)msg)
    {
   
	default:
		break;
    }

    return ;
}

ListboxView *SettingWindowNew::ListboxDataInit(ListboxView *listboxView)
{
	int i;
	int err_code;
	char pBuf[NAME_MAX_SIZE];
	
	listboxView = (ListboxView*) malloc(sizeof(ListboxView));
	if (NULL == listboxView) 
    {
		db_warn("malloc ListboxView data error\n");
		return NULL;
	}
	memset((void *) listboxView, 0, sizeof(ListboxView));
    listboxView->winBg_path[0]="/usr/share/minigui/res/images/bg_transparent_old.png";
    listboxView->ltbBg_path[0]="/usr/share/minigui/res/images/bg_transparent_old.png";


    R* r = R::get();
	listboxView->lTextFt = r->GetFont();
	listboxView->rTextFt = r->GetFont();
	listboxView->lineColor = PIXEL_lightwhite;
	
	listboxView->itType = ITEM_LINE;

	listboxView->lTextColor = COLOR_lightwhite;
	listboxView->rTextColor = COLOR_lightwhite;
	listboxView->ht = 640;
	listboxView->ix = 640;			/**/
	listboxView->hfp = 0;			/**/
	listboxView->hbp = 0;			/**/
									/**/
	listboxView->vt = 640;			/**/
	listboxView->iy = 90;			/**/
	listboxView->vfp = 10;
	listboxView->vbp = 10;
	listboxView->vgap = 2;

	listboxView->itSz = 100;
	listboxView->Index = 0;
	listboxView->showSz = 4;
	listboxView->lthGap = 5;
	listboxView->ltvGap = 5;
	listboxView->moveGap = 0;
	//item data
	listboxView->itDt = (ItemDataBox*) malloc(sizeof(ItemDataBox)*listboxView->itSz);
	memset((void *) listboxView->itDt, 0, sizeof(ItemDataBox)*listboxView->itSz);
	for(i=0; i< listboxView->itSz; i++)
	{
		listboxView->itDt[i].rType = RITEM_TEXT;
		memset(pBuf, 0, sizeof(pBuf));
		sprintf(pBuf, "Habo%d", i);
		listboxView->itDt[i].lT = pBuf;
        sprintf(pBuf, "zhb%d", i);
        listboxView->itDt[i].rT = pBuf;
		listboxView->itDt[i].btn = NULL;
	}
	return listboxView;
}

#if 0
int SettingWindowNew::InitListBoxView(ListboxView *listboxView)
{
    int index=0;
    //init common data 
    listboxView = (ListboxView*) malloc(sizeof(ListboxView));
	if (NULL == listboxView) 
    {
		db_warn("malloc ListboxView data error\n");
		return -1;
	}
	memset((void *) listboxView, 0, sizeof(ListboxView));
    listboxView->winBg_path[0]="/usr/share/minigui/res/images/bg_transparent_old.png";
    listboxView->ltbBg_path[0]="/usr/share/minigui/res/images/bg_transparent_old.png";


    R* r = R::get();
	listboxView->lTextFt = r->GetFont();
	listboxView->rTextFt = r->GetFont();
	listboxView->lineColor = PIXEL_lightwhite;
	
	listboxView->itType = ITEM_LINE;

	listboxView->lTextColor = COLOR_lightwhite;
	listboxView->rTextColor = COLOR_lightwhite;
	listboxView->ht = 320;
	listboxView->ix = 320;			/**/
	listboxView->hfp = 0;			/**/
	listboxView->hbp = 0;			/**/
									/**/
	listboxView->vt = 240;			/**/
	listboxView->iy = 40;			/**/
	listboxView->vfp = 10;
	listboxView->vbp = 10;
	listboxView->vgap = 2;

	listboxView->itSz = SETTING_LISTBOXVIEW_NUM;
	listboxView->Index = 0;
	listboxView->showSz = 5;
	listboxView->lthGap = 5;
	listboxView->ltvGap = 5;
	listboxView->moveGap = 0;
	//item data
	listboxView->itDt = (ItemDataBox*) malloc(sizeof(ItemDataBox)*listboxView->itSz);
	memset((void *) listboxView->itDt, 0, sizeof(ItemDataBox)*listboxView->itSz);
    for(int i=0; i<SETTING_LISTBOXVIEW_NUM;i++)
    {
        index = GetMenuConfig(listboxview_item_ids[i]);
        db_msg("[habo]--->InitListBoxView  index = %d",index);
        //init first icon and first text gap
        listboxView->itDt[i].lrB_path[0]= r->GetImagePath(ListBoxViewItemData[i].first_icon_path); 
        r->GetString(std::string(ListBoxViewItemData[i].first_text), listboxView->itDt[i].lT);
        listboxView->itDt[i].l_icon_text_gap = ListBoxViewItemData[i].first_icon_text_gap;
        db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].lrB_path[0] = %s",i,listboxView->itDt[i].lrB_path[0].c_str());
        db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].lT = %s",i,listboxView->itDt[i].lT.c_str());
        db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].l_icon_text_gap = %d",i,listboxView->itDt[i].l_icon_text_gap);
        //init second icon and text gap
        StringVector string_text;
        string_text.clear();
        std::vector<std::string>::const_iterator it;
        for(int j = 0 ,it = string_text.begin(); it !=  string_text.end(); it++,j++)
        {
            
        }
        r->GetStringArray(std::string(ListBoxViewItemData[i].first_text), string_text);
        
        listboxView->itDt[i].r_icon_text_gap = ListBoxViewItemData[i].second_icon_text_gap;
        //db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].rT = %s",i,listboxView->itDt[i].rT.c_str());
        db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].r_icon_text_gap = %d",i,listboxView->itDt[i].r_icon_text_gap);
        if(ListBoxViewItemData[i].second_icon_button == 0)//is normal mode
        {
             listboxView->itDt[i].rType = RITEM_TEXT_BMP;
             listboxView->itDt[i].lrB_path[1]= r->GetImagePath(ListBoxViewItemData[i].second_icon_path0); 
             db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].lrB_path[0] = %s",i,listboxView->itDt[i].lrB_path[1].c_str());
        }
        else //is button mode
        {
            listboxView->itDt[i].rType = RITEM_TEXT_BUTTON;
            listboxView->itDt[i].btn->onOffB_path[0]= r->GetImagePath(ListBoxViewItemData[i].second_icon_path0); 
            listboxView->itDt[i].btn->onOffB_path[1]= r->GetImagePath(ListBoxViewItemData[i].second_icon_path1); 
            listboxView->itDt[i].btn->onStus = 0; //这里需要实际获取当前是什么状态
            listboxView->itDt[i].btn->sltItem = -1;
            listboxView->itDt[i].btn->text = "zhbtext";
            db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].btn->onOffB_path[0] = %s",i,listboxView->itDt[i].btn->onOffB_path[0].c_str());
            db_msg("[habo]--->InitListBoxView  listboxView->itDt[%d].btn->onOffB_path[1] = %s",i,listboxView->itDt[i].btn->onOffB_path[1].c_str());
        }
    }
}
#endif
int SettingWindowNew::GetMenuConfig(int msg)
{
       int val=0;
       MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
	val = menuconfiglua->GetMenuIndexConfig(msg);
       db_msg("[habo]:GetMenuConfig:msg[%d], val[%d]", msg, val);
	return val;
}


void SettingWindowNew::ListBoxViewClickProc(View *control)
{
    db_warn("[habo]---> ListBoxViewClickProc");
    
}

