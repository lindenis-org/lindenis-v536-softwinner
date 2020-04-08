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

#include "window/setting_window.h"
#include "window/newSettingWindow.h"
#include "debug/app_log.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "widgets/magic_block.h"
#include "window/window_manager.h"
#include "window/dialog.h"
#include "widgets/menu_items.h"
#include "device_model/menu_config_lua.h"
#include "resource/resource_manager.h"
#include "window/time_setting_window_new.h"
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
#include "bll_presenter/device_setting.h"
#include "dd_serv/common_define.h"
#include "widgets/list_view.h"
#include "sublist.h"
#include "info_dialog.h"
#include "bll_presenter/statusbarsaver.h"
#include "window/carid_setting_window.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "NewSettingWindow"

using namespace EyeseeLinux;


IMPLEMENT_DYNCRT_CLASS(NewSettingWindow)
void NewSettingWindow::keyProc(int keyCode, int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_POWER:
			{
			db_warn("[debug_jaosn]:short MSG_TIMER");
			if (isLongPress == LONG_PRESS){
			 	PreviewWindow *p_win  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			 	p_win->ShutDown();
			  }
			}
            break;
        case SDV_KEY_OK:
			db_warn("[debug_jaosn]:short SDV_KEY_OK");
			//ListViewClickProc();
				if (sub_list_->GetVisible()){
					db_warn("[debug_jaosn]:this is  ok code ListItemClickProc");
					//ListViewClickProc();
					subListViewClickProcOKButton();
				}else if(m_TimeSetting->GetVisible()){
					//if(m_TimeSetting->cur_flag >= 6){
						//subListViewClickProcOKButton();
				//	}
					m_TimeSetting->keyProc(SDV_KEY_OK, SHORT_PRESS);
					db_warn("[debug_jaosn]:this is  ok code m_TimeSetting");
				}else if(m_info_dialog_->GetVisible()){
				m_info_dialog_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
				}else if(m_carid_window_->GetVisible()){
					m_carid_window_->keyProc(SDV_KEY_OK, SHORT_PRESS);
				}else if(s_BulletCollection->m_button_dialog_flag_){
					s_BulletCollection->keyProc(SDV_KEY_OK, SHORT_PRESS);
				}else{
					db_warn("[debug_jaosn]:this is  ok code subListViewClickProcOKButton");
					//subListViewClickProcOKButton();
					ListViewClickProc();
				}
            break;
		case SDV_KEY_LEFT:
		//case SCANCODE_SUNXIUP:	
		{	
			if (m_TimeSetting->GetVisible())
			{
				db_warn("[debug_jaosn]:sub list item  SDV_KEY_LEFT");
				m_TimeSetting->keyProc(SDV_KEY_LEFT, SHORT_PRESS);
		   		
			}else if(m_carid_window_->GetVisible()){
					m_carid_window_->keyProc(SDV_KEY_LEFT, SHORT_PRESS);
			}else if(m_info_dialog_->GetVisible()){
				m_info_dialog_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}else if(s_BulletCollection->m_button_dialog_flag_)
			{
				db_warn("[debug_jaosn]:!m_TimeSetting->GetVisible");
				s_BulletCollection->keyProc(SDV_KEY_LEFT, SHORT_PRESS);
			}
			else
			{	
				db_warn("[debug_jaosn]:sub list item  SDV_KEY_LEFT");
				//m_TimeSetting->keyProc(SDV_KEY_LEFT, SHORT_PRESS);
				//SubListItemClickProc(1);
			}
		}
		break;
		case SDV_KEY_RIGHT:
			if (m_TimeSetting->GetVisible())
			{
				db_warn("[debug_jaosn]:!m_TimeSetting->GetVisible");
				m_TimeSetting->keyProc(SDV_KEY_RIGHT, SHORT_PRESS);
		   		
			}else if(m_info_dialog_->GetVisible()){
				m_info_dialog_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}else if(s_BulletCollection->m_button_dialog_flag_){
				s_BulletCollection->keyProc(SDV_KEY_RIGHT, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}
			else if(m_carid_window_->GetVisible()){
					m_carid_window_->keyProc(SDV_KEY_RIGHT, SHORT_PRESS);
			}else
			{	
				db_warn("[debug_jaosn]:sub list item  SDV_KEY_LEFT");

				//SubListItemClickProc(1);
			}
		break;
		case SDV_KEY_RETURN:
			if(m_info_dialog_->GetVisible()){
				m_info_dialog_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}
		break;
        case SDV_KEY_MENU:
		case SDV_KEY_MODE:
			if (m_TimeSetting->GetVisible()){
				m_TimeSetting->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}else if(m_info_dialog_->GetVisible()){
				m_info_dialog_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}else if(m_carid_window_->GetVisible()){
					m_carid_window_->keyProc(SDV_KEY_MENU, SHORT_PRESS);
			}else if(s_BulletCollection->m_button_dialog_flag_){
				s_BulletCollection->keyProc(SDV_KEY_MENU, SHORT_PRESS);
				//subListViewClickProcOKButton();
			}
			else if(sub_list_->GetVisible()){
				sub_list_->DoHide();
                db_error("sub_list_->DoHide");
                // update title
        		r->GetString("ml_setting_menu", str_menu);
        	    m_list_view_item_title->SetCaption(str_menu.c_str()); 
                db_error("str_menu.c_str(): %s",str_menu.c_str());
                m_list_view_item_title->Refresh();
			}
			else
			{
			#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
            EyeseeLinux::StatusBarSaver::GetInstance()->pause(false);
			#endif
            listener_->sendmsg(this, MSG_SETTING_TO_PREVIEW, 0);			// 切换到 预览界面
			usleep(500*1000);
            listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
			}
            break;
        default:
            db_msg("[debug_zhb]:invild keycode");
            break;
    }  
}

int NewSettingWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{

      switch (message) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        case MSG_COMMAND:
            {
                int id = LOSWORD(wparam);
                int code = HISWORD(wparam);
                if (code == LVN_CLICKED) {
                    ListViewClickProc();                    
                }
            }
            break;
		case MSG_LBUTTONDOWN:
			break;
        case MSG_LBUTTONUP:
            break;
        case MSG_MOUSEMOVE:
			break;
        default:
            break;
    }
    return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}


NewSettingWindow::NewSettingWindow(IComponent *parent)
    : SystemWindow(parent)
    , list_view_(NULL)
    , sub_list_view_(NULL)
    , win_mg_(::WindowManager::GetInstance())
    ,sub_list_(NULL)
    ,r(R::get())
    ,m_main_row_index_(-1)
    ,m_list_view_item_title(NULL)
    ,m_TimeSetting(NULL)
    ,m_info_dialog_(NULL)
    ,s_BulletCollection(NULL)
    ,m_carid_window_(NULL)
    ,return_button_view(NULL)
    ,listview_top_bg(0xFF313747)
    ,listview_bg(0xFFFFFFFF)
    ,listview_first_str_color(0x313747)
    ,listview_second_str_color(0xA4A3A3)
    ,firstinit(true)
{
    wname = "NewSettingWindow";
    Load();
    SetBackColor(0xFF000000);
    str_menu.clear();
    version_str.clear();
	db_error("NewSettingWindow create");
    return_button_view = reinterpret_cast<GraphicView *>(GetControl("list_view_item_return"));		// 菜单窗口的返回按钮
    GraphicView::LoadImage(return_button_view, "sw_return", "sw_return_h",  GraphicView::NORMAL);
    return_button_view->OnClick.bind(this, &NewSettingWindow::ButtonClickProc);
    return_button_view->SetBackColor(listview_top_bg);
    return_button_view->Show();

    r->GetString("ml_setting_menu", str_menu);
    m_list_view_item_title = reinterpret_cast<TextView *>(GetControl("list_view_item_title"));
    m_list_view_item_title->SetCaptionColor(0xFFFFFFFF);
    m_list_view_item_title->SetCaption(str_menu.c_str());
    m_list_view_item_title->SetBackColor(listview_top_bg);

    TextView* m_list_view_item_title_right_bg = reinterpret_cast<TextView *>(GetControl("list_view_item_title_right_bg"));
    m_list_view_item_title_right_bg->SetBackColor(listview_top_bg);
    
    list_view_ = reinterpret_cast<ListView *> (GetControl("list_view_item_info"));
    list_view_->SetBackColor(listview_bg);
    SetWindowElementAttr(list_view_->GetHandle(), WE_BGC_HIGHLIGHT_ITEM,
                    Pixel2DWORD(HDC_SCREEN, 0xFF36EFED)); //主菜单选中显示


    //init the first listitem cols
    LVCOLUMN col[4];
    col[0].pfnCompare = NULL;
    col[0].width = FIRST_COL_W;
    col[0].colFlags = LVHF_CENTERALIGN;
    col[0].colmFlags = LVCF_CENTERALIGN_IMAGE;
    col[0].pszHeadText = "image";
    col[0].nCols = FIRST_COL;
    columns.push_back(col[0]);
    col[1].pfnCompare = NULL;
    col[1].width = SECOND_COL_W;
    col[1].colFlags = LVCF_LEFTALIGN;
    col[1].colmFlags = LVCF_CENTERALIGN_IMAGE;
    col[1].pszHeadText = "str";
    col[1].nCols =SECOND_COL;
    columns.push_back(col[1]);
    col[2].pfnCompare = NULL;
    col[2].width = THIRD_COL_W;
    col[2].colFlags = LVCF_RIGHTALIGN;
    col[2].colmFlags = LVCF_CENTERALIGN_IMAGE;
    col[2].pszHeadText = "str";
    col[2].nCols = THIRD_COL;
    columns.push_back(col[2]);
    col[3].pfnCompare = NULL;
    col[3].width = FOURTH_COL_W;
    col[3].colFlags = LVCF_RIGHTALIGN;
    col[3].colmFlags = LVCF_CENTERALIGN_IMAGE;
    col[3].pszHeadText = "image";
    col[3].nCols = FOURTH_COL;
    columns.push_back(col[3]);
    list_view_->SetColumns(columns,false);
	// load all menu icon  ListBoxItemData ListBoxItemDataPhoto
	listviewbmp.clear();
	InitListviewbmpMap();
	
    InitListViewItem();
    list_view_->Show();
    //sub list view
    sub_list_ = new Sublist(this);
    sub_list_->OnListItemClick.bind(this, &NewSettingWindow::subListViewClickProc);
    sub_list_view_ = static_cast<ListView*>(sub_list_->GetControl("sublist_view"));
    sub_list_view_ ->SetBackColor(listview_bg);
    SetWindowElementAttr(sub_list_view_->GetHandle(), WE_BGC_HIGHLIGHT_ITEM,
                    Pixel2DWORD(HDC_SCREEN, 0xFF36EFED));  //子菜单选中显示 

    //init the time setting windown 
    m_TimeSetting = new TimeSettingWindowNew(this);
    m_TimeSetting->OnConfirmClick.bind(this, &NewSettingWindow::DateTimeSettingConfirm);

    //init info dialog
    m_info_dialog_ = new InfoDialog(this);

    //init the button dialog
    s_BulletCollection = new BulletCollection();
	s_BulletCollection->initButtonDialog(this);

	/***********set the CarId setting window*******************/
    m_carid_window_ = new CaridSettingWindow(this);
    m_carid_window_->OnConfirmClick.bind(this, &NewSettingWindow::CaridSettingConfirm);
    SystemVersion(false);

	firstinit = false;
}

NewSettingWindow::~NewSettingWindow()
{
    if( sub_list_ != NULL)
	{
	    delete sub_list_;
	    sub_list_ = NULL;
	}

    if( m_TimeSetting != NULL)
	{
	    delete m_TimeSetting;
	    m_TimeSetting = NULL;

	}
	
    if( m_info_dialog_ != NULL)
	{
	    delete m_info_dialog_;
	    m_info_dialog_ = NULL;
	}
    if( s_BulletCollection != NULL)
	{
	    delete s_BulletCollection;
	    s_BulletCollection = NULL;
	}
	if( m_carid_window_ != NULL)
	{
	    delete m_carid_window_;
	    m_carid_window_ = NULL;
	}
	ReleaseListviewbmpMap();
	db_error("~NewSettingWindow delete");
}

void NewSettingWindow::InitListviewbmpMap()
{
	int i;
	const char* p;
	BITMAP* pbmp;
	std::string ss;	
	for (i=0; i<SETTING_LISTVIEW_NUM; i++) 
	{
		p = ListBoxItemData[i].first_icon_path;
		
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			//db_error("ss: %s",ss.c_str());
			listviewbmp.insert(make_pair(ss,pbmp));
		}
		p = ListBoxItemData[i].second_icon_path0;
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			listviewbmp.insert(make_pair(ss,pbmp));
		}
		p = ListBoxItemData[i].second_icon_path1;
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			listviewbmp.insert(make_pair(ss,pbmp));
		}	
	}
	for (i=0; i<SETTING_LISTVIEW_NUM_PHOTO; i++) 
	{
		p = ListBoxItemDataPhoto[i].first_icon_path;
		
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			listviewbmp.insert(make_pair(ss,pbmp));
		}
		p = ListBoxItemDataPhoto[i].second_icon_path0;
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			listviewbmp.insert(make_pair(ss,pbmp));
		}
		p = ListBoxItemDataPhoto[i].second_icon_path1;
		if (p) {
			pbmp = AllocListViewImage((const char*)((r->GetImagePath(p)).c_str()));
			ss = p;
			listviewbmp.insert(make_pair(ss,pbmp));
		}	
	}
	db_error("listviewbmp size: %d",listviewbmp.size());
}
BITMAP* NewSettingWindow::GetListviewbmpMap(std::string &strkey)
{
	std::map<std::string, BITMAP*> ::iterator iter;
	iter = listviewbmp.find(strkey);
	
	if (iter !=listviewbmp.end()) {
		//db_error("find in listviewbmp : %s",strkey.c_str());
		return iter->second;
	} else {
		return NULL;
	}
}

void  NewSettingWindow::ReleaseListviewbmpMap()
{
	std::map<std::string, BITMAP*> ::iterator iter;
	BITMAP* pbmp;
	for (iter = listviewbmp.begin(); iter != listviewbmp.end(); iter++)
	{
		pbmp = iter->second;
		if (pbmp) {
			if (pbmp->bmBits) {
				free(pbmp->bmBits);
			}
			free(pbmp);
			pbmp = NULL;
		}
	}
	listviewbmp.clear();
}

void NewSettingWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    //db_msg("ListItemInfoWindow:handle msg:%d", msg);
    switch(msg)
    {
    case MSG_STORAGE_UMOUNT:
		{
			if(s_BulletCollection->getButtonDialogShowFlag() && s_BulletCollection->getButtonDialogCurrentId() == BC_BUTTON_DIALOG_FORMAT_SDCARD && StorageManager::GetInstance()->getFormatFlag() == false)//close setting window button dialog
				s_BulletCollection->BCDoHide();
			 if (StorageManager::GetInstance()->GetStorageStatus() != UMOUNT)
			 	break;
			 UpdateSDcardCap();
		}
		break;
	case MSG_STORAGE_MOUNTED:
		{
			UpdateSDcardCap();
		}break;
   case MSG_PREVIEW_TO_NEWSETTING_WINDOW:
       {
			db_warn("MSG_PREVIEW_TO_NEWSETTING_WINDOW");
			StorageManager *sm = StorageManager::GetInstance();
    	    if(sm->GetStorageStatus() != MOUNTED)
    	    {
                db_warn("current sdcard is umount !!!");
                //break;
    	    }
            UpdateSDcardCap();
			InitListViewItem();
        }
        break;
    case MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION:
        {
		    db_msg("debug_zhb-----new setting------MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION");
		    listener_->sendmsg(this,MSG_SYSTEM_UPDATE,0);
        }
		break;
    case MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION:
		listener_->sendmsg(this,MSG_4G_UPDATE,0);
		break;
	case MSG_GPSON_HAPPEN:
	case MSG_GPSOFF_HAPPEN:
		InitListViewItem();
		break;
    default:
        break;

    }

}

void NewSettingWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE;// | WS_EX_TRANSPARENT;
    params.class_name = " ";
    params.alias      = GetClassName();
}

std::string NewSettingWindow::GetResourceName()
{
    return std::string(GetClassName());
}

void NewSettingWindow::PreInitCtrl(View *ctrl, std::string &ctrl_name)
{
    
    if(ctrl_name == std::string("list_view_item_title")
        ||ctrl_name == std::string("list_view_item_return")
        ||ctrl_name == std::string("list_view_item_title_right_bg") ) 
    {
        ctrl->SetCtrlTransparentStyle(false);
    }
}

void NewSettingWindow::ListViewClickProc()
{
    //need to get current the select item 
    LVITEM item;
    int ret = -1;
    ret = list_view_->GetSelectedItem(item);
    if(ret < 0)
    {
        db_error("[Habo]---> get selected item fail !!!");
        m_main_row_index_ = -1;
        return;
    
    }
    if(item.nItem >= SETTING_LISTVIEW_NUM || item.nItem < 0)
     {
        db_error("[Habo]---> get selected item invalid !!!");
        m_main_row_index_ = -1;
        return;
    }

    m_main_row_index_ = item.nItem;
    list_view_->SetHilight(m_main_row_index_);
    switch(mylistview_item_ids[item.nItem])
    {
        case SETTING_RECORD_RESOLUTION:
//        case SETTING_REAR_RECORD_RESOLUTION:
        case SETTING_RECORD_LOOP:
		case SETTING_RECORD_TIMELAPSE:	
        case SETTING_EMER_RECORD_SENSITIVITY:
        case SETTING_CAMERA_AUTOSCREENSAVER:
        case SETTING_DEVICE_LANGUAGE:
        case SETTING_CAMERA_LIGHTSOURCEFREQUENCY:
        case SETTING_RECORD_ENCODINGTYPE:
        case SETTING_VOLUME_SELECTION:
        case SETTING_CAMERA_EXPOSURE:
		case SETTING_PHOTO_RESOLUTION:
		case SETTING_PHOTO_TIMED:
		case SETTING_PHOTO_AUTO:
		case SETTING_PHOTO_DRAMASHOT:
		case SETTING_PHOTO_QUALITY:	
		case SETTING_SPEED_UNIT:	
		case SETTING_TIMEZONE:
        {
            ShowSubList(item.nItem);
        }
        break;
		case SETTING_MOTION_DETECT:
        case SETTING_RECORD_VOLUME:
        case SETTING_PARKING_MONITORY:
		case SETTING_GPS_SWITCH:
        case SETTING_WIFI_SWITCH:
        {
		#if 0
			//change the text 关闭(O ) 或者 打开( O) 的字符串
            StringVector sub_value_list;
			db_error("1111-----------");
            int current_val = GetMenuConfig(listview_item_ids[m_main_row_index_]);
            r->GetStringArray(std::string(ListBoxItemData[m_main_row_index_].first_text), sub_value_list);
            list_view_->SetItemText(sub_value_list[!current_val], m_main_row_index_, THIRD_COL);
            list_view_->Refresh();
          
            //change the button img 关闭/打开的图片
             LVSUBITEM pic_item;
            pic_item.pszText = NULL;
            pic_item.flags = LVFLAG_BITMAP;
            if(current_val == 0)
                pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(ListBoxItemData[m_main_row_index_].second_icon_path1)).c_str()));
            else
                pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(ListBoxItemData[m_main_row_index_].second_icon_path0)).c_str()));
            list_view_->UpdateItemData(pic_item, m_main_row_index_, FOURTH_COL);
            //reset the current value 
           SetMenuConfig(GetNotifyMessage(listview_item_ids[m_main_row_index_]),!current_val);
           listener_->sendmsg(this,GetNotifyMessage(listview_item_ids[m_main_row_index_]),!current_val);
		#else
		 	PingpangListViewItem(m_main_row_index_, true);
		#endif
        }
        break;
       case SETTING_DEVICE_DATETIME:
       {
            ShowTimeSettingWindow();
       }break;
       case SETTING_DEVICE_FORMAT:
       {
            ShowFormatScardDialog();
       }break;
       case SETTING_DEVICE_RESETFACTORY:
       {
            showResetFactoryDialog();
       }break;
       case SETTING_DEVICE_DEVICEINFO:
       {
            ShowDeviceInfoDialog();
       }break;
       case SETTING_DEVICE_CARID:
       {
            ShowCaridSettingWindow();
       }break;

    }

    
}

void NewSettingWindow::subListViewClickProc(View *control)
{
    LVITEM main_select_item, sub_select_item;
    int main_row_index = 0 ,sub_row_index = 0 ;
    int ret = -1;
	int tmp_row_index_ = -1 ;
	StringVector sub_value_list;
	int current_val;
    //reset the item head
    m_list_view_item_title->SetCaption(str_menu.c_str());
    //hide the sublistview
     usleep(400*1000);
    sub_list_->DoHide();
    
    ret = sub_list_view_->GetSelectedItem(sub_select_item);

    if (ret < 0) {
        db_error("GetSelectedItem failed, ret[%d]", ret);
        return;
    }

    if (m_main_row_index_ < 0) {
        ret = list_view_->GetSelectedItem(main_select_item);
        if (ret < 0) {
            db_error("GetSelectedItem failed, ret[%d]", ret);
            return;
        }
        main_row_index = main_select_item.nItem;
    } else {
        
        main_row_index = m_main_row_index_;
    }
    sub_row_index = sub_select_item.nItem;

    //判断这次选择的是否和进来的时候是一致的，如果是则不用刷新一级菜单
    int old_hilight = sub_list_view_->GetHilight();
 //   if(sub_row_index != old_hilight)
    {
        std::string value_str = sub_list_view_->GetItemText(sub_row_index, 0);//获取当前选择的item的字符串
        list_view_->SetItemText(value_str, main_row_index, THIRD_COL);
        list_view_->Refresh();


		StatusBarWindow *win = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
		PreviewWindow   *pwin = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));

		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_RECORD_DELAY_TIME){

			if(sub_row_index == 0){				
				StatusBarWindow *win = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
				win->SetWinStatus(STATU_PREVIEW);

				SetMenuConfig(MSG_SET_RECORD_TIME,2);
        		listener_->sendmsg(this,MSG_SET_RECORD_TIME,2);

				//change the text
				m_main_row_index_ = 1;
	            current_val = GetMenuConfig(mylistview_item_ids[m_main_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[m_main_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[2]).c_str(), m_main_row_index_, THIRD_COL);
	            list_view_->Refresh();
				
				return ;
			}else{
				win->SetWinStatus(STATU_DELAYRECIRD);	
			}				
		}
		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_RECORD_TIME){
			win->SetWinStatus(STATU_PREVIEW);
		}
#ifdef SUPPORT_PHOTOMODE
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_TIME_TAKE_PIC)
		{
			db_warn("set time tack pic index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close auto time take pic
				SetMenuConfig(MSG_SET_AUTO_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_AUTO_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_PIC_CONTINOUS,0);
        		listener_->sendmsg(this,MSG_SET_PIC_CONTINOUS,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 2 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();
				
				//win->UpdatePhotoMode(MODE_PIC_TIME,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_TIME,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);
		}

		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_AUTO_TIME_TAKE_PIC)
		{
			db_warn("set auto time tack pic index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close time take pic
				SetMenuConfig(MSG_SET_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_PIC_CONTINOUS,0);
        		listener_->sendmsg(this,MSG_SET_PIC_CONTINOUS,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

			//	win->UpdatePhotoMode(MODE_PIC_AUTO,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_AUTO,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);	
		}
		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_PIC_CONTINOUS)
		{
			db_warn("set  tack pic contious index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close time take pic
				SetMenuConfig(MSG_SET_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 2 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_AUTO_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_AUTO_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

			//	win->UpdatePhotoMode(MODE_PIC_CONTINUE,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_CONTINUE,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);
		}
#endif
        //reset the current value
        // SETTING_RECORD_RESOLUTION:
        //  ;
        db_error("main_row_index: %d, sub_row_index:%d",main_row_index,sub_row_index);
        db_error("msg: %d",GetNotifyMessage(mylistview_item_ids[main_row_index]));
        SetMenuConfig(GetNotifyMessage(mylistview_item_ids[main_row_index]),sub_row_index);
        listener_->sendmsg(this,GetNotifyMessage(mylistview_item_ids[main_row_index]),sub_row_index);	// MSG_SET_VIDEO_RESOULATION
    }
 
}

void NewSettingWindow::subListViewClickProcOKButton()
{
    LVITEM main_select_item, sub_select_item;
    int main_row_index = 0 ,sub_row_index = 0 ;
    int ret = -1;
	int tmp_row_index_ = -1 ;
	StringVector sub_value_list;
	int current_val;
    //reset the item head
    m_list_view_item_title->SetCaption(str_menu.c_str());
    //hide the sublistview
     usleep(400*1000);
    sub_list_->DoHide();
    
    ret = sub_list_view_->GetSelectedItem(sub_select_item);

    if (ret < 0) {
        db_error("GetSelectedItem failed, ret[%d]", ret);
        return;
    }

    if (m_main_row_index_ < 0) {
        ret = list_view_->GetSelectedItem(main_select_item);
        if (ret < 0) {
            db_error("GetSelectedItem failed, ret[%d]", ret);
            return;
        }
        main_row_index = main_select_item.nItem;
    } else {
        
        main_row_index = m_main_row_index_;
    }
    sub_row_index = sub_select_item.nItem;

    //判断这次选择的是否和进来的时候是一致的，如果是则不用刷新一级菜单
    int old_hilight = sub_list_view_->GetHilight();
 //   if(sub_row_index != old_hilight)
    {
        std::string value_str = sub_list_view_->GetItemText(sub_row_index, 0);//获取当前选择的item的字符串
        list_view_->SetItemText(value_str, main_row_index, THIRD_COL);
        list_view_->Refresh();


		StatusBarWindow *win = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
		PreviewWindow   *pwin = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));

		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_RECORD_DELAY_TIME){

			if(sub_row_index == 0){				
				StatusBarWindow *win = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
				win->SetWinStatus(STATU_PREVIEW);

				SetMenuConfig(MSG_SET_RECORD_TIME,2);
        		listener_->sendmsg(this,MSG_SET_RECORD_TIME,2);

				//change the text
				m_main_row_index_ = 1;
	            current_val = GetMenuConfig(mylistview_item_ids[m_main_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[m_main_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[2]).c_str(), m_main_row_index_, THIRD_COL);
	            list_view_->Refresh();
				
				return ;
			}else{
				win->SetWinStatus(STATU_DELAYRECIRD);	
			}				
		}
		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_RECORD_TIME){
			win->SetWinStatus(STATU_PREVIEW);
		}
#ifdef SUPPORT_PHOTOMODE
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_TIME_TAKE_PIC)
		{
			db_warn("set time tack pic index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close auto time take pic
				SetMenuConfig(MSG_SET_AUTO_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_AUTO_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_PIC_CONTINOUS,0);
        		listener_->sendmsg(this,MSG_SET_PIC_CONTINOUS,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 2 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();
				
				//win->UpdatePhotoMode(MODE_PIC_TIME,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_TIME,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);
		}

		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_AUTO_TIME_TAKE_PIC)
		{
			db_warn("set auto time tack pic index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close time take pic
				SetMenuConfig(MSG_SET_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_PIC_CONTINOUS,0);
        		listener_->sendmsg(this,MSG_SET_PIC_CONTINOUS,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ + 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

			//	win->UpdatePhotoMode(MODE_PIC_AUTO,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_AUTO,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);	
		}
		
		if(GetNotifyMessage(mylistview_item_ids[main_row_index]) == MSG_SET_PIC_CONTINOUS)
		{
			db_warn("set  tack pic contious index = %d",sub_row_index);
			if(sub_row_index != 0)
			{
				//close time take pic
				SetMenuConfig(MSG_SET_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 2 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

				//close pic continous
				SetMenuConfig(MSG_SET_AUTO_TIME_TAKE_PIC,0);
        		listener_->sendmsg(this,MSG_SET_AUTO_TIME_TAKE_PIC,0);

				//change the text
				tmp_row_index_ = m_main_row_index_ - 1 ;
	            current_val = GetMenuConfig(mylistview_item_ids[tmp_row_index_]);
	            r->GetStringArray(std::string(myListBoxItemData[tmp_row_index_].first_text), sub_value_list);
				list_view_->SetItemText((sub_value_list[0]).c_str(), tmp_row_index_, THIRD_COL);
	            list_view_->Refresh();

			//	win->UpdatePhotoMode(MODE_PIC_CONTINUE,sub_row_index);
				pwin->UpdatePhotoMode(MODE_PIC_CONTINUE,sub_row_index);
			}
			else
				pwin->UpdatePhotoMode(MODE_PIC_NORMAL,sub_row_index);
		}
#endif
        //reset the current value
        // SETTING_RECORD_RESOLUTION:
        //  ;
        SetMenuConfig(GetNotifyMessage(mylistview_item_ids[main_row_index]),sub_row_index);
        listener_->sendmsg(this,GetNotifyMessage(mylistview_item_ids[main_row_index]),sub_row_index);	// MSG_SET_VIDEO_RESOULATION
    }
 
}


// 菜单窗口的返回按钮
void NewSettingWindow::ButtonClickProc(View *control)
{
    if(!sub_list_->GetSubListWindowActiveStatus())
    {
        sub_list_->DoHide();
        AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
		// update title
		r->GetString("ml_setting_menu", str_menu);
	    m_list_view_item_title->SetCaption(str_menu.c_str()); 
        db_error("str_menu.c_str(): %s",str_menu.c_str());
        m_list_view_item_title->Refresh();
    }
    else
    {
        this->keyProc(SDV_KEY_MENU, SHORT_PRESS);
        AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
    }
}

void NewSettingWindow::InitListViewItem()
{
    list_view_->RemoveAllItems();
	int winstatus = STATU_PREVIEW;
	if (firstinit == false) {
		db_error("NewSettingWindow firstinit false ");
		PreviewWindow *p_win  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
		if (p_win) {
			winstatus = p_win->Get_win_statu_save();
		} else {
			db_error("WINDOWID_PREVIEW not create ?");
		}
	} else {
		db_error("NewSettingWindow firstinit true ");
	}
	int menucount = 0;
	db_error("InitListViewItem winstatus: %d",winstatus);
	for (int i=0; i<SETTING_LISTVIEW_NUM+2; i++ ) {
		memset(&myListBoxItemData[i] ,0,sizeof(ListViewItem));
	}
		
	if (winstatus == STATU_PREVIEW) 
	{
	
		if (EventManager::GetInstance()->CheckGpsOnline()) {
			menucount = SETTING_LISTVIEW_NUM;
		} else {
			menucount = SETTING_LISTVIEW_NUM - 2;	// 去掉 GPS和 speed
		}
		
		if (menucount == SETTING_LISTVIEW_NUM) {
	    	for(int i = 0 ; i < SETTING_LISTVIEW_NUM ; i++) {
				memcpy(&myListBoxItemData[i] ,&ListBoxItemData[i],sizeof(ListViewItem));
				mylistview_item_ids[i] = listview_item_ids[i];
				InitListViewItem(myListBoxItemData[i],i);
	    	}
			
		} else {
			int k = 0;
			for(int i = 0 ; i < SETTING_LISTVIEW_NUM ; i++) {
				if ((listview_item_ids[i] == SETTING_GPS_SWITCH ) || (listview_item_ids[i] == SETTING_SPEED_UNIT)) continue;

				mylistview_item_ids[k] = listview_item_ids[i];
				memcpy(&myListBoxItemData[k] ,&ListBoxItemData[i],sizeof(ListViewItem));
				
				k++;
				
	    	}
			for (k =0; k <menucount; k++ )
				InitListViewItem(myListBoxItemData[k],k);
		}
	    list_view_->SetHilight(0);
		list_view_->SetCurItem(0);
	}
	else {
		// STATU_PHOTO
		if (EventManager::GetInstance()->CheckGpsOnline()) {
			menucount = SETTING_LISTVIEW_NUM_PHOTO;
		} else {
			menucount = SETTING_LISTVIEW_NUM_PHOTO - 2;	// 去掉 GPS和 speed
		}
		
		if (menucount == SETTING_LISTVIEW_NUM_PHOTO) {
	    	for(int i = 0 ; i < SETTING_LISTVIEW_NUM_PHOTO ; i++) {
				memcpy(&myListBoxItemData[i] ,&ListBoxItemDataPhoto[i],sizeof(ListViewItem));
				mylistview_item_ids[i] = listview_item_ids_photo[i];
				InitListViewItem(myListBoxItemData[i],i);
	    	}
			
		} else {
			int k = 0;
			for(int i = 0 ; i < SETTING_LISTVIEW_NUM_PHOTO ; i++) {
				if ((listview_item_ids_photo[i] == SETTING_GPS_SWITCH ) || (listview_item_ids_photo[i] == SETTING_SPEED_UNIT)) continue;

				mylistview_item_ids[k] = listview_item_ids_photo[i];
				memcpy(&myListBoxItemData[k] ,&ListBoxItemDataPhoto[i],sizeof(ListViewItem));
				
				k++;
				
	    	}
			for (k =0; k <menucount; k++ )
				InitListViewItem(myListBoxItemData[k],k);
		}
	    list_view_->SetHilight(0);
		list_view_->SetCurItem(0);
	}
}


void NewSettingWindow::InitListViewItem(const ListViewItem &list,int index_)
{
        InitListViewItem(list.first_icon_path, list.first_text, list.type,list.second_icon_path0,list.second_icon_path1,index_);
    
}

int NewSettingWindow::InitListViewItem(const char *first_icon_path, const char *first_text,
            const int  type, const char *second_icon_path0, const char *second_icon_path1,int index_)
{
    std::string str_data;
    int index_n = 0;
    int current_val = 0;
    //set list item
    LVITEM item;
    item.dwFlags &= ~LVIF_FOLD;
    item.nItemHeight = LISTVIEW_ITEM_H;

    //set list sub item
    LVSUBITEM subdata;

    item.nItem = index_;
    list_view_->AddItem(item);
     StringVector str_text;

    //0 image
    subdata.pszText = const_cast<char*>("");
    subdata.nItem = index_;
    subdata.subItem=FIRST_COL;
    subdata.flags = LVFLAG_BITMAP;
	subdata.nTextColor = listview_first_str_color;
    //subdata.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(first_icon_path)).c_str()));
    std::string ss = first_icon_path;
    subdata.image = (DWORD)GetListviewbmpMap(ss);
	//db_error("xxxx: %s",ss.c_str());
    list_view_->FillSubItem(subdata);

    //1 str
    r->GetString(std::string(first_text), str_data);
    subdata.pszText = subdata.pszText = const_cast<char*>(str_data.c_str());
    subdata.nItem = index_;
    subdata.subItem=SECOND_COL;
    subdata.flags = 0;
    subdata.image = 0;
	subdata.nTextColor = listview_first_str_color;
    list_view_->FillSubItem(subdata);
    
    //2 str
    if(type == TYPE_DIALOG)
        subdata.pszText = const_cast<char*>("");
    else if(type == TYPE_DIALOG_STR){
        std::stringstream info_str;
        getSdcardInfo(info_str);
        subdata.pszText = const_cast<char*>(info_str.str().c_str());
    }
    else
    {
        current_val = GetMenuConfig(mylistview_item_ids[index_]);
        str_text.clear();
        r->GetStringArray(std::string(first_text), str_text);  
        subdata.pszText = const_cast<char*>((str_text[current_val]).c_str());
    }
    subdata.nItem = index_;
    subdata.subItem=THIRD_COL;
    subdata.flags = 0;
    subdata.image = 0;
	subdata.nTextColor = listview_second_str_color;
    list_view_->FillSubItem(subdata);

    //3 image
    subdata.pszText = const_cast<char*>("");
    subdata.nItem = index_;
    subdata.subItem=FOURTH_COL;
    subdata.flags = LVFLAG_BITMAP;
    if(type == TYPE_SWITCH && current_val == 1) {
        //subdata.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(second_icon_path1)).c_str()));
        ss = second_icon_path1;
		subdata.image = (DWORD)GetListviewbmpMap(ss);
    }
    else {
        //subdata.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(second_icon_path0)).c_str()));
        ss = second_icon_path0;
		subdata.image = (DWORD)GetListviewbmpMap(ss);
    }
	subdata.nTextColor = listview_first_str_color;
    list_view_->FillSubItem(subdata);
    return 0;
}

void NewSettingWindow::ShowSubList(int index_msg)
{
    int hi_idx = 0;
    sub_list_view_->RemoveAllItems();

    std::vector<LVCOLUMN> subcolumns;
    LVCOLUMN subcol;
    RECT rect;
    sub_list_view_->GetRect(&rect);
    subcol.width       = RECTW(rect);
    subcol.pfnCompare  = NULL;
    subcol.pszHeadText = NULL;
    subcol.colFlags    = LVCF_CENTERALIGN;
    subcolumns.push_back(subcol);
    sub_list_view_->SetColumns(subcolumns, false);


    /*********set the sublist item***********/
    LVITEM item_sub;
    item_sub.dwFlags &= ~LVIF_FOLD;
    item_sub.nItemHeight = LISTVIEW_ITEM_H;
    LVSUBITEM subdata;
    StringVector sub_value_list;
    sub_value_list.clear();

    //set the sub menu head
    std::string str_sub_head;
    r->GetString(std::string(myListBoxItemData[index_msg].first_text), str_sub_head);
    m_list_view_item_title->SetCaption(str_sub_head.c_str());
    //get the sub item data
    r->GetStringArray(std::string(myListBoxItemData[index_msg].first_text), sub_value_list);

#if 0
    std::vector<std::string>::const_iterator it;
    for(it = sub_value_list.begin(); it != sub_value_list.end(); it++)
    {
        db_msg("[debug_zhb]----sublistviewitem data:---sub_value_list = %s ",it->c_str());
    }
#endif

        hi_idx = GetMenuConfig(mylistview_item_ids[index_msg]);
	sub_list_view_->SetCurItem(hi_idx);
    for(int i = 0; i < sub_value_list.size(); ++i)
    {
        item_sub.nItem = i;
        sub_list_view_->AddItem(item_sub);
        subdata.nTextColor = listview_first_str_color;
        subdata.pszText = const_cast<char*>((sub_value_list[i]).c_str());
        subdata.flags   = 0;
        subdata.image   = 0;
        subdata.nItem = i;
        subdata.subItem=0;
        sub_list_view_->FillSubItem(subdata);
    }
    if( sub_list_ != NULL)
        sub_list_->DoShow();
    sub_list_view_->SelectItem(hi_idx);
}


void NewSettingWindow::OnLanguageChanged()
{
    r->GetString("ml_setting_menu", str_menu);
    m_list_view_item_title = reinterpret_cast<TextView *>(GetControl("list_view_item_title"));
    m_list_view_item_title->SetCaption(str_menu.c_str()); 
}


int NewSettingWindow::GetMenuConfig(int msg)
{
    int val=0;
    MenuConfigLua *menuconfiglua = MenuConfigLua::GetInstance();
    val = menuconfiglua->GetMenuIndexConfig(msg);
	return val;
}

void NewSettingWindow::SetMenuConfig(int msg,int val)
{
    MenuConfigLua *menuconfiglua = MenuConfigLua::GetInstance();
    menuconfiglua->SetMenuIndexConfig(msg,val);
}

BITMAP* NewSettingWindow::AllocListViewImage(const char *image_path)
{
    int ret = 0;

    if (image_path == NULL)
		return NULL;

	if( !strncmp(image_path, "", 1) )
		return NULL;
    BITMAP *data =NULL;
    data = (BITMAP*)malloc(sizeof(BITMAP));
    if(data == NULL)
    {
        db_warn("malloc data fail");
        return NULL;
    }
    ret = LoadBitmapFromFile(HDC_SCREEN, data, image_path);
    if (ret != 0)
	{
        free(data);
		data = NULL;
    }

    return data;
}

void NewSettingWindow::FreeListViewImage(BITMAP *data)
{
    UnloadBitmap(data);
}

int NewSettingWindow::GetNotifyMessage(int msgid)
{
    int ret = 0;
    switch(msgid){
        case SETTING_RECORD_RESOLUTION:
            ret = MSG_SET_VIDEO_RESOULATION;
			db_error("get MSG_SET_VIDEO_RESOULATION");
            break;
        case SETTING_REAR_RECORD_RESOLUTION:
            ret = MSG_SET_REAR_RECORD_RESOLUTION;
            break;
        case SETTING_RECORD_VOLUME:
            ret = MSG_SET_RECORD_VOLUME;
            break;
        case SETTING_SCREEN_BRIGHTNESS:
            ret = MSG_SET_SCREEN_BRIGHTNESS;
            break;
        case SETTING_SCREEN_NOT_DISTURB_MODE:
            ret = MSG_SET_SCREEN_NOT_DISTURB_MODE;
            break;
        case SETTING_VOICE_TAKE_PHOTO:
            ret = MSG_SET_VOICE_TAKE_PHOTO;
            break;
        case SETTING_WIFI_SWITCH:
            ret = MSG_SET_WIFI_SWITCH;
            break;
        case SETTING_4G_NET_WORK:
            ret = MSG_SET_4G_NET_WORK;
            break;
        case SETTING_VOLUME_SELECTION:
            ret = MSG_SET_VOLUME_SELECTION;
            break;
        case SETTING_SYSTEM_SOUND:
            ret = MSG_SET_SYSTEM_SOUND;
            break;
        case SETTING_POWERON_SOUND_SWITCH:
            ret = MSG_SET_POWERON_SOUND_SWITCH;
            break;
        case SETTING_KEY_SOUND_SWITCH:
            ret = MSG_SET_KEY_SOUND_SWITCH;
            break;
        case SETTING_ACC_SWITCH:
            ret = MSG_SET_ACC_SWITCH;
            break;
        case SETTING_DRIVERING_REPORT_SWITCH:
            ret = MSG_SET_DRIVERING_REPORT_SWITCH;
            break;
        case SETTING_ADAS_SWITCH:
            ret = MSG_SET_ADAS_SWITCH;
            break;
        case SETTING_STANDBY_CLOCK:
            ret = MSG_SET_STANDBY_CLOCK;
            break;
        case SETTING_FORWARD_COLLISION_WARNING:
            ret = MSG_SET_FORWARD_COLLISION_WARNING;
            break;
        case SETTING_LANE_SHIFT_REMINDING:
            ret = MSG_SET_LANE_SHIFT_REMINDING;
            break;
        case SETTING_WATCH_DOG_SWITCH:
            ret = MSG_SET_WATCH_DOG_SWITCH;
            break;
        case SETTING_PROBE_PROMPT:
            ret = MSG_SET_PROBE_PROMPT;
            break;
        case SETTING_SPEED_PROMPT:
            ret = MSG_SET_SPEED_PROMPT;
            break;
        case SETTING_TIMEWATERMARK:
            ret = MSG_SET_TIEM_WATER_MARK;
            break;
        case SETTING_EMER_RECORD_SWITCH:
            ret = MSG_SET_EMER_RECORD_SWITCH;
            break;
        case SETTING_EMER_RECORD_SENSITIVITY:
            ret = MSG_SET_EMER_RECORD_SENSITIVITY;
            break;
        case SETTING_PARKING_MONITORY:
            ret = MSG_SET_PARKING_MONITORY;
            break;
        case SETTING_PARKING_WARN_LAMP_SWITCH:
            ret = MSG_SET_PARKING_WARN_LAMP_SWITCH;
            break;
        case SETTING_PARKING_ABNORMAL_MONITORY_SWITCH:
            ret = MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH;
            break;
        case SETTING_PARKING_ABNORMAL_NOTICE_SWITCH:
            ret = MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH;
            break;
        case SETTING_PARKING_RECORD_LOOP_SWITCH:
            ret = MSG_SET_PARKING_RECORD_LOOP_SWITCH;
            break;
        case SETTING_PARKING_RECORD_LOOP_RESOLUTION:
            ret = MSG_SET_PARKING_RECORD_LOOP_RESOLUTION;
            break;
#ifdef ENABLE_ENC_TYPE_SELECT
        case SETTING_RECORD_ENCODINGTYPE:
            ret = MSG_SET_RECORD_ENCODE_TYPE;
            break;
#endif
        case SETTING_RECORD_LOOP:
            ret = MSG_SET_RECORD_TIME;
            break;
        case SETTING_RECORD_TIMELAPSE:
            ret = MSG_SET_RECORD_DELAY_TIME;
            break;
        case SETTING_RECORD_SLOWMOTION:
            ret = MSG_SET_RECORD_SLOW_TIME;
            break;
        case SETTING_PHOTO_RESOLUTION:
            ret = MSG_SET_PIC_RESOULATION;
            break;
        case SETTING_PHOTO_TIMED:
            ret = MSG_SET_TIME_TAKE_PIC;
            break;
        case SETTING_PHOTO_AUTO:
            ret = MSG_SET_AUTO_TIME_TAKE_PIC;
            break;
        case SETTING_PHOTO_DRAMASHOT:
            ret =MSG_SET_PIC_CONTINOUS;
            break;
		case SETTING_PHOTO_QUALITY:
			ret =MSG_SET_PIC_QUALITY;
			break;
        case SETTING_CAMERA_EXPOSURE:
            ret = MSG_SET_CAMERA_EXPOSURE;
            break;
        case SETTING_CAMERA_WHITEBALANCE:
            ret = MSG_SET_CAMERA_WHITEBALANCE;
            break;
        case SETTING_CAMERA_LIGHTSOURCEFREQUENCY:
            ret = MSG_SET_CAMERA_LIGHTSOURCEFREQUENCY;
            break;
        case SETTING_CAMERA_AUTOSCREENSAVER:
            ret = MSG_SET_AUTO_TIME_SCREENSAVER;
            break;
        case SETTING_CAMERA_TIMEDSHUTDOWN:
            ret = MSG_SET_AUTO_TIME_SHUTDOWN;
            break;
        case SETTING_DEVICE_LANGUAGE:
            ret = MSG_RM_LANG_CHANGED;
            break;
        case SETTING_DEVICE_TIME:
            ret = MSG_DEVICE_TIME;
            break;
        case SETTING_DEVICE_FORMAT:
            ret = MSG_DEVICE_FORMAT;
            break;
        case SETTING_ACCOUNT_UNBIND:
            ret = MSG_ACCOUNT_UNBIND;
            break;
        case SETTING_DEVICE_RESETFACTORY:
            ret = MSG_DEVICE_RESET_FACTORY;
            break;
        case SETTING_DEVICE_DEVICEINFO:
            //ret = MSG_SYSTEM_UPDATE;
            ret = MSG_SET_DEVICE_DEVICEINFO;
            break;
        case SETTING_DEVICE_SDCARDINFO:
            ret = MSG_SET_DEVICE_SDCARDINFO;
            break;
        case SETTING_DEVICE_VERSIONINFO:
            ret = MSG_SET_DEVICE_VERSIONINFO;
            break;
		case SETTING_MOTION_DETECT:
			ret = MSG_SET_MOTION_DETECT;
			break;
		case SETTING_GPS_SWITCH:
			ret = MSG_SET_GPS_SWITCH;
			break;
		case SETTING_SPEED_UNIT:
			ret = MSG_SET_SPEEDUNIT;
			break;
		case SETTING_TIMEZONE:
			ret = MSG_SET_TIMEZONE;
			break;
		case SETTING_DEVICE_CARID:
			ret = MSG_SET_DEVICE_CARID;
			break;
        default:
            ret = -1;
            break;
    }
    return ret;
}


void NewSettingWindow::DoShow()
{
    Window::DoShow();
}

void NewSettingWindow::DoHide()
{
    SetVisible(false);
}


int NewSettingWindow::ShowTimeSettingWindow()
{
    if( m_TimeSetting != NULL)
    {
        m_TimeSetting->ShowCurrentDateTime();
        m_TimeSetting->DoShow();
    }
    return 0;
}
int NewSettingWindow::ShowCaridSettingWindow()
{
    if( m_carid_window_ != NULL)
    {
        m_carid_window_->SetContentType();
        m_carid_window_->DoShow();
    }
    return 0;
}

void NewSettingWindow::DateTimeSettingConfirm(View *view, int value)
{
    if( m_TimeSetting != NULL)
        m_TimeSetting->DoHide();
}
void NewSettingWindow::CaridSettingConfirm(View *view, int value)
{
    if( m_carid_window_ != NULL)
        m_carid_window_->DoHide();
}

void NewSettingWindow::showResetFactoryDialog()
{
    if( s_BulletCollection != NULL)
    {
        s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_RESETFACTORY);
        s_BulletCollection->ShowButtonDialog();
    }
}

int NewSettingWindow::ShowFormatScardDialog()
{
    if(!(StorageManager::GetInstance()->GetStorageStatus() != UMOUNT))
    {
        PreviewWindow *p_win  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
        p_win->ShowPromptInfo(PROMPT_TF_NO_EXIST,2);
        return -1;
    }
    if( s_BulletCollection != NULL)
    {
        s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_FORMAT_SDCARD);
        s_BulletCollection->ShowButtonDialog();
    }
    return 0;
}

void NewSettingWindow::ShowDeviceInfoDialog()
{
    std::string str_title;
    R::get()->GetString("ml_device_deviceinfo", str_title);
    if( m_info_dialog_ != NULL)
        m_info_dialog_->SetInfoTitle(str_title);
    
    std::string str_name,str_make,str_version;
    R::get()->GetString("ml_device_deviceinfo_name", str_name);
    R::get()->GetString("ml_device_deviceinfo_make", str_make);
    R::get()->GetString("ml_device_version_current", str_version);
    ::LuaConfig config;
    config.LoadFromFile("/tmp/data/menu_config.lua");
	std::string version_str = config.GetStringValue("menu.device.sysversion.version");

    std::stringstream ss;
	#ifdef INFO_ZD55
    ss << "\n" << " " << "\n";
    ss << "\n" << str_version << " "<< version_str << "\n";
	#else
	ss << "\n" << str_name << "\n";
    ss << "\n" << str_make << "\n";
    ss << "\n" << str_version << " "<< version_str << "\n";
	#endif
    std::string str;
    str = ss.str();
    if( m_info_dialog_ != NULL)
    {
        m_info_dialog_->SetInfoText(str);
        m_info_dialog_->DoShow();
    }

}

void NewSettingWindow::ResetUpdate()
{
    db_error("ResetUpdate()");
    InitListViewItem();
}


void NewSettingWindow::getSdcardInfo(std::stringstream &info_str)
{
	uint32_t free_, total_;
	StorageManager *sm = StorageManager::GetInstance();
	int status = sm->GetStorageStatus();
	if(status == UMOUNT || status == STORAGE_FS_ERROR  || (status == FORMATTING))
	{
		total_ = 0;
		free_ = 0;
        std::string str_tf;
        r->GetString("ml_no_tf", str_tf);
        db_warn("habo---> str_tf = %s",str_tf.c_str());
        info_str<<str_tf.c_str();
	}
	else
	{
		sm->GetStorageCapacity(&free_, &total_);
		info_str<<free_<<" MB";
	}

    
	db_warn("[debug_zhb]-----getSdcardInfo-----info_str = %s",info_str.str().c_str());
}
void NewSettingWindow::UpdateSDcardCap()
{
    if(list_view_ == NULL)
    {
    	db_error("ItemData data is null");
    	return;
    }
    std::stringstream info_str;
    getSdcardInfo(info_str);
    for(int i = 0; i < SETTING_LISTVIEW_NUM ; i++)
        if(mylistview_item_ids[i] == SETTING_DEVICE_FORMAT)
            list_view_->SetItemText(info_str.str().c_str(), i, THIRD_COL);
    list_view_->Refresh();
	
}
void NewSettingWindow::SystemVersion(bool fset)
{
	::LuaConfig config;
        config.LoadFromFile("/tmp/data/menu_config.lua");
	if(!fset)
	 	version_str = config.GetStringValue("menu.device.sysversion.version");
	else
		config.SetStringValue("menu.device.sysversion.version",version_str);
}

void NewSettingWindow::ForceCloseSettingWindowAllDialog()
{
	if(s_BulletCollection->getButtonDialogShowFlag())//close setting window button dialog
		s_BulletCollection->BCDoHide();
}

int NewSettingWindow::GetPosInlistview_item_ids(int index)
{
	int k = -1;
	//db_error("sizeof(mylistview_item_ids) : %d",sizeof(mylistview_item_ids)/sizeof(SETTING_RECORD_RESOLUTION));
	for (int i=0; i<(sizeof(mylistview_item_ids)/sizeof(SETTING_RECORD_RESOLUTION)); i++)
	{
		if (index == mylistview_item_ids[i]){
			k=i;
			break;
		}
	}
	return k;
}


void NewSettingWindow::PingpangListViewItem(int index, bool update)
{
	StringVector sub_value_list;
	
	int current_val = GetMenuConfig(mylistview_item_ids[index]);
	db_error("1111----------- %d current: %d",index, current_val );
	r->GetStringArray(std::string(myListBoxItemData[index].first_text), sub_value_list);
	list_view_->SetItemText(sub_value_list[!current_val], index, THIRD_COL);
	if (update)
		list_view_->Refresh();
	db_error("3333----------- ");		  
	//change the button img 关闭/打开的图片
	LVSUBITEM pic_item;
	pic_item.pszText = NULL;
	pic_item.flags = LVFLAG_BITMAP;
	std::string ss;
	if(current_val == 0) {
		//pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(myListBoxItemData[index].second_icon_path1)).c_str()));
		ss = myListBoxItemData[index].second_icon_path1;
		pic_item.image = (DWORD)GetListviewbmpMap(ss);
		
	}
	else {
		//pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(myListBoxItemData[index].second_icon_path0)).c_str()));
		ss = myListBoxItemData[index].second_icon_path0;
		pic_item.image = (DWORD)GetListviewbmpMap(ss);
	}
	list_view_->UpdateItemData(pic_item, index, FOURTH_COL);
	//reset the current value 
	db_error("3333----------- set to: %d", !current_val);
	SetMenuConfig(GetNotifyMessage(mylistview_item_ids[index]),!current_val);
	listener_->sendmsg(this,GetNotifyMessage(mylistview_item_ids[index]),!current_val);

}

void NewSettingWindow::SetListViewItem(int index, int update)
{
	#if 0
	StringVector sub_value_list;
	db_error("1111----------- %d",index);
	int current_val = GetMenuConfig(listview_item_ids[index]);
	r->GetStringArray(std::string(ListBoxItemData[index].first_text), sub_value_list);
	list_view_->SetItemText(sub_value_list[!current_val], index, THIRD_COL);
	if (update)
		list_view_->Refresh();
	db_error("3333----------- ");		  
	//change the button img 关闭/打开的图片
	LVSUBITEM pic_item;
	pic_item.pszText = NULL;
	pic_item.flags = LVFLAG_BITMAP;
	if(current_val == 0)
		pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(ListBoxItemData[index].second_icon_path1)).c_str()));
	else
		pic_item.image = (DWORD)AllocListViewImage((const char*)((r->GetImagePath(ListBoxItemData[index].second_icon_path0)).c_str()));
	list_view_->UpdateItemData(pic_item, index, FOURTH_COL);
	//reset the current value 
	db_error("3333----------- ");
	#endif
	db_error("index: %d mylistview_item_ids[index]: %d ",index, mylistview_item_ids[index]);
	SetMenuConfig(GetNotifyMessage(mylistview_item_ids[index]),update);	// SETTING_RECORD_RESOLUTION
	listener_->sendmsg(this,GetNotifyMessage(mylistview_item_ids[index]),update);	// MSG_SET_VIDEO_RESOULATION

}

void NewSettingWindow::SetListViewItemEx(int index, int update, int type)
{
	if (type==0) {	// for record
		db_error("index: %d listview_item_ids[index]: %d ",index, listview_item_ids[index]);
		SetMenuConfig(GetNotifyMessage(listview_item_ids[index]),update);	// SETTING_RECORD_RESOLUTION
		listener_->sendmsg(this,GetNotifyMessage(listview_item_ids[index]),update);	// MSG_SET_VIDEO_RESOULATION
	} else {	// for photo
		db_error("index: %d listview_item_ids_photo[index]: %d ",index, listview_item_ids_photo[index]);
		SetMenuConfig(GetNotifyMessage(listview_item_ids_photo[index]),update);
		listener_->sendmsg(this,GetNotifyMessage(listview_item_ids_photo[index]),update);
	}

}

