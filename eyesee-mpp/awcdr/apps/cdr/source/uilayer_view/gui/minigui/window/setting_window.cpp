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

#include "window/setting_window.h"
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
#include "bll_presenter/device_setting.h"
#include "dd_serv/common_define.h"

#include "device_model/system/device_qrencode.h"
#define S_FORMAT
using namespace std;
using namespace EyeseeLinux;

#define MSG_SHOW_BIND_QR    110
#define MSG_NO_BIND_QR      111


#define S_F_LINE_X 380 //first level line x position
#define ITEM_HIGHT 59
#define S_ITEM_UPPER_CLEARANCE_H 11//tiem offset the top hight
#define S_ITEM_HIGHT 97
#define S_BUTTON_X 467
#define S_BUTTON_Y 303
#define S_BUTTON_WIGTH 467+269
#define S_BUTTON_HIGHT 303+66

#define SETTING_WINDOW_ITEM_HIGHT 59
#define ABOUT_ICON_INDEX 5

#define INSTALLER_TIMEOUT 5

extern DeviceSettingPresenter *p_device_setting;

IMPLEMENT_DYNCRT_CLASS(SettingWindow)

static void getSdcardInfo(stringstream &info_str)
{
    R *r = R::get();
    stringstream ss;
    string str;
	uint32_t free_, total_;
	int cap_pre=0;
	char buf[64]={0};
	StorageManager *sm = StorageManager::GetInstance();
	int status = sm->GetStorageStatus();
	if(status == UMOUNT || status == STORAGE_FS_ERROR  || (status == FORMATTING))
	{
		total_ = 0;
		free_ = 0;
		snprintf(buf,sizeof(buf),"%.1f",(float)0);
	}
	else
	{
		sm->GetStorageCapacity(&free_, &total_);
		snprintf(buf,sizeof(buf),"%.1f",(float)100*(total_-free_)/total_);
	}

    for(int i = 0 ; i < NUM_SDCARD_INFO-2 ; i++){
        r->GetString(string(sdcard_info_[i].item_name),str);
        if(i < 2){
            if(i == 0){
                ss << str.c_str()<< total_-free_<<"MB ("<<buf<<"%)";
                str = ss.str().c_str();
                ss.str("");
            }else{
                ss << str.c_str()<< total_<<"MB";
                str = ss.str().c_str();
                ss.str("");
            }
        }
        info_str<<str.c_str()<<"\n";
    }
	db_msg("[debug_zhb]-----getSdcardInfo----buf = %s---info_str = %s",buf,info_str.str().c_str());
}


void SettingWindow::keyProc(int keyCode, int isLongPress)
{
    pthread_mutex_lock(&setwindow_proc_lock_);
    if(m_bIsformatting || m_show_dialog_)
    {
        pthread_mutex_unlock(&setwindow_proc_lock_);
        return ;
    }
    switch(keyCode){
        case SDV_KEY_LEFT://button2
            db_msg("[debug_zhb]----setting----SDV_KEY_LEFT");
            GetSystemTimeToMenuItem(-1);
            break;
        case SDV_KEY_POWER://button1
            {
#if 0
                db_msg("[debug_zhb]----setting----SDV_KEY_POWER");
                if(menu_items_->getSubmenuStatusFlag()){
                    submenuitemkeyproc();//to menu
                }else{
#if 0
                    DeInitMenuWinStatu(menu_statu);
                    InitMenuWinStatu(menu_statu);
                    menu_items_->CancleAllHilight();
                    menu_items_->SetHilight(0);
#else
                    menu_items_->CancleAllHilight();
                    menu_items_->SetHilight(0);
                    menu_items_->SetTop();
#endif
                    listener_->sendmsg(this, MSG_SETTING_TO_PREVIEW, 0);
                    listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
                }
#endif
                break;
            }
        case SDV_KEY_MENU:
        {
			db_msg("[debug_zhb]----setting----SDV_KEY_POWER");
			if(menu_items_->getSubmenuStatusFlag()){
				submenuitemkeyproc();//to menu
			}else{
#if 0
				DeInitMenuWinStatu(menu_statu);
				InitMenuWinStatu(menu_statu);
				menu_items_->CancleAllHilight();
				menu_items_->SetHilight(0);
#else			
				//BroadcastMessage (MSG_CANCLE_DIALG, 0, 0);
				if(m_formart_dialog_display == true || m_bIsformatting == true || m_show_dialog_== true)
				{
					//db_error("ZZZZZZZZZZZZZZZZZ %d  %d",m_formart_dialog_display,m_bIsformatting);
					break;
				}
				menu_items_->CancleAllHilight();
				menu_items_->SetHilight(0);
				menu_items_->SetTop();
#endif
				listener_->sendmsg(this, MSG_SETTING_TO_PREVIEW, 0);
				listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
				WindowManager	 *win_mg_ = WindowManager::GetInstance();
				PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				p_win->ShowCamBRecordIcon();
			}
        	break;
    	}
        case SDV_KEY_OK://button4
            {
                db_msg("[debug_zhb]---setting-----SDV_KEY_OK");
				if(!menu_items_->getSubmenuStatusFlag() && !menu_items_->getAddDataFlag() && !menu_items_->getPaintSecondLevelFlag()){
					menu_items_->setPaintSecondLevelFlag(true);
                    MenultemkeyEventProc();//to submeu
                }else if(menu_items_->getSubmenuStatusFlag()){
                    updataSubmenuItemChoiced(true);
                }
            }
            break;
        case SDV_KEY_RIGHT://button3
            db_msg("[debug_zhb]---setting-----SDV_KEY_RIGHT");
            GetSystemTimeToMenuItem(1);
            break;
        case SDV_KEY_MODE:
            db_msg("[debug_zhb]----setting----SDV_KEY_MODE");
            break;
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }
    pthread_mutex_unlock(&setwindow_proc_lock_);
}

void SettingWindow::ShowPromptBox()
{
	db_msg("[debug_zhb]-----------setting window -----ShowPromptBox--");
	s_PromptBox_->DoShow();
	s_PromptBox_->ShowPromptBox(PROMPT_BOX_DEVICE_RECORDING, 3);
}

int SettingWindow::clcSettingWindowMenuItemPos(int x,int y)
{
	if((x > S_F_LINE_X)&&(y > S_ITEM_HIGHT)&&(y < ITEM_HIGHT+S_ITEM_HIGHT)){
		//db_error("[debug_jason]: clcSettingWindowMenuItemPos 11 @@@@@@");
		first_hight_id = 0;
	}else if((x > S_F_LINE_X) && (y >(ITEM_HIGHT+S_ITEM_HIGHT)) && (y < (ITEM_HIGHT*2+S_ITEM_HIGHT))){
		//db_error("[debug_jason]: clcSettingWindowMenuItemPos222222@@@@@@");
		first_hight_id = 1;
	}else if((x > S_F_LINE_X) && (y >(ITEM_HIGHT*2+S_ITEM_HIGHT)) && (y < (ITEM_HIGHT*3+S_ITEM_HIGHT))){
		//db_error("[debug_jason]: clcSettingWindowMenuItemPos333333@@@@@@");
		first_hight_id = 2;
	}else if((x > S_F_LINE_X) && (y >(ITEM_HIGHT*3+S_ITEM_HIGHT)) && (y < (ITEM_HIGHT*4+S_ITEM_HIGHT))){
		//db_error("[debug_jason]: clcSettingWindowMenuItemPos444444@@@@@@");
		first_hight_id = 3;
	}else if((x > S_F_LINE_X) && (y >(ITEM_HIGHT*4+S_ITEM_HIGHT)) && (y < (ITEM_HIGHT*5+S_ITEM_HIGHT))){
		//db_error("[debug_jason]: clcSettingWindowMenuItemPos555555@@@@@@");
		first_hight_id = 4;
	}
	menu_items_->setSubMenuItemPos(first_hight_id);
	return first_hight_id;
}

int SettingWindow::HandleFristSubMenuItemEvent(int x,int y)
{
	//db_error("[debug_jason]: x = %d ; y= %d",x,y);
	int msg_id;
	m_sdcard_info_ui = false;
	m_formart_dialog_display = false;
	int first_highlight_index = menu_items_->GetHilight();
	msg_id = USER_MSG_BASE +menu_statu+ first_highlight_index+1;
	SetIndexToSubmenu(msg_id);
	//db_error("[debug_jason]: the frist first_highlight_index id = %d  GetMenuIndex = %d",first_highlight_index,GetMenuIndex());
	switch(GetMenuIndex()){
		case SETTING_ADAS_SWITCH:
		case SETTING_SYSTEM_SOUND:
		case SETTING_RECORD_RESOLUTION:
		{
			clcSettingWindowMenuItemPos(x,y);
			if(y >(ITEM_HIGHT*2+S_ITEM_HIGHT)){
				menu_items_->setNeedPrint(false);
			}else{
				menu_items_->setNeedPrint(true);
				HandleOnOffSelectItem(first_hight_id,GetNotifyMessage());
			}

		}
		break;
		case SETTING_DEVICE_LANGUAGE:
		{
			db_msg("[debug_jason]:this is debug");
			clcSettingWindowMenuItemPos(x,y);
			if(y >(ITEM_HIGHT*2+S_ITEM_HIGHT)){
				menu_items_->setNeedPrint(false);
			}else{
				menu_items_->setNeedPrint(true);
				listener_->notify(this, GetNotifyMessage(), first_hight_id);
			}
		}
		break;
		case SETTING_CAMERA_AUTOSCREENSAVER:
		{
			clcSettingWindowMenuItemPos(x,y);
			if(y >(ITEM_HIGHT*4+S_ITEM_HIGHT)){
				menu_items_->setNeedPrint(false);
			}else{
				menu_items_->setNeedPrint(true);
			}
			listener_->sendmsg(this, GetNotifyMessage(), first_hight_id);
		}
		break;
        case SETTING_ACCOUNT_BINDING:
        {
            if((x > S_BUTTON_X) && (x < S_BUTTON_WIGTH) && (y > S_BUTTON_Y) && (y < S_BUTTON_HIGHT))
            {
                if(account_bind_flag_ == false)
                {
                    vector<string> wifi_app_download;
                    string info;
                    char src[128] = {0};
                    int length = 0;
                    memset(src, 0, sizeof(src));
                    //strcpy(src, "123456");
                    length = strlen(src);
                    if(length == 0)
                    {
                        //info = "Due to network problems.The device get bind code failed ! Please try again later!";
                        //wifi_app_download.push_back(info);
                        this->ShowInfoDialog(wifi_app_download, "", MSG_NO_BIND_QR);
                    }else
                    {
                        GenBindQRcode(src, length);
                        if(access("/data/UberBindimage.png", R_OK) == 0)
                        {
                            GraphicView::LoadImageFromAbsolutePath(dialog_->GetControl("dialog_icon4"), "/data/UberBindimage.png");
                            this->ShowInfoDialog(wifi_app_download, "", MSG_SHOW_BIND_QR);
                            system("rm -f /data/UberBindimage.png");
                        }else
                        {
                            info = "Due to network problems.The device get bind code failed ! Please try again later!";
                            wifi_app_download.push_back(info);
                            this->ShowInfoDialog(wifi_app_download, "", MSG_NO_BIND_QR);
                        }
                    }
                }else // unbind click
                {
                    s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_ACCOUNT_UNBIND);
                    s_BulletCollection->ShowButtonDialog();
                }
            }
        }
        break;
		case SETTING_DEVICE_DEVICEINFO:
		{
            if((x > S_BUTTON_X) && (x < S_BUTTON_WIGTH) && (y > S_BUTTON_Y) && (y < S_BUTTON_HIGHT))
            {
                //db_error("HandleFristSubMenuItemEvent : handle button event");
#ifdef S_FORMAT
                if(!(StorageManager::GetInstance()->GetStorageStatus() != UMOUNT))
                {
                    WindowManager    *win_mg_ = WindowManager::GetInstance();
                    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                    p_win->ShowPromptInfo(PROMPT_TF_NO_EXIST,2);
					return 0;
				}

				s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_FORMAT_SDCARD);
				s_BulletCollection->ShowButtonDialog();
				m_sdcard_info_ui = true;
				m_formart_dialog_display = true;
				UpdateSDcardCap();
				return 0;
				#endif
			}else{
				//db_error("HandleFristSubMenuItemEvent : not do anything");
			}
		}
		break;
		case SETTING_DEVICE_VERSIONINFO:
		{
			if(new_version_flag)
            {
				if((x > S_BUTTON_X) && (x < S_BUTTON_WIGTH) && (y > S_BUTTON_Y) && (y < S_BUTTON_HIGHT)){
				    WindowManager    *win_mg_ = WindowManager::GetInstance();
                    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                    if(IsDownloadStatusReady())
                    {
                        listener_->notify(this, GetNotifyMessage(), 3);
                        ShowFirmwareDownloadDialog();
                    }
                }else{
					db_msg("click SETTING_DEVICE_VERSIONINFO not do anything");
				}
			}
		}
		break;
		default:
			listener_->notify(this, GetNotifyMessage(), first_hight_id);
		break;
	}
	return 0;
}

void SettingWindow::account_bind_status_update()
{
    int first_highlight_index = menu_items_->GetHilight();
    updateAccountBindStatus(first_highlight_index);
}

int SettingWindow::setSubMenuDefaultItem(int p_nIndex)
{
	m_sdcard_info_ui = false;
	int default_id = USER_MSG_BASE +menu_statu+ p_nIndex+1;
	SetIndexToSubmenu(default_id);
	menu_items_->setSubMenuItemPos(GetMenuConfig(default_id));
    switch(default_id)
    {
        case SETTING_DEVICE_DEVICEINFO:
            {
                m_sdcard_info_ui = true;
                UpdateSDcardCapEx(p_nIndex);
            }
            break;
        case SETTING_ACCOUNT_BINDING:
            {
                updateAccountBindStatus(p_nIndex);
            }
            break;
        case SETTING_DEVICE_VERSIONINFO:
            updateVersionFirmwareInfo(p_nIndex);
            break;
        default:
            break;
    }

    return 0;
}

int SettingWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
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
//			 db_error("[debug_jason]:SettingWindow message = %d,mouseX = %u,mouseY = %lu",message,mouseX,mouseY);
			 if((mouseX > S_F_LINE_X) && (mouseY > S_ITEM_HIGHT)){
			 	HandleFristSubMenuItemEvent(mouseX,mouseY);
			 }else{
				setSubMenuDefaultItem(mouseY/SETTING_WINDOW_ITEM_HIGHT);
			 }
			// db_error("[debug_jason]:SettingWindow message = %d,mouseX = %u,mouseY = %lu",message,mouseX,mouseY);
			}
			break;
        default:
            break;
    }
    return ContainerWidget::HandleMessage(hwnd, message, wparam, lparam);
}

static int SetSubMenuStringValueOrType(StringIntMap *map,std::string &str,int tv)
{
    StringIntMap::iterator it;
    db_msg("debug_zhb-----SetSubMenuStringValueOrType----- it->c_str = %s  tv = %d",str.c_str(),tv);
    it = map->find(str.c_str());//?ҵ???ǰitem??value
    if(it!=map->end()){
        db_msg(" found the str");
        it->second = tv;
    }else{
        db_msg(" insert the str");
        map->insert(make_pair(str.c_str(),tv));
    }
	return 0;
}
static int updateSubMenuStringValue(StringIntMap *map,std::string &str)
{
    StringIntMap::iterator it;
    int ret = -1;
    it = map->find(str.c_str());//?ҵ???ǰitem??value
    if(it!=map->end()){
        db_msg("[debug_zhb]--------- it->second = %d",it->second);
        ret = !it->second;
        it->second = !it->second;
    }else{
        db_msg("[debug_zhb]--------- ");
    }
    return ret;
}

void SettingWindow::InitGetMsgIdMap()
{
    if( getMsgId_map_.size() != 0)
        getMsgId_map_.clear();
    for(int i=0; i<MSGMAP_COUNT;i++){
        getMsgId_map_.insert(make_pair(msg_map_[i].item_name, msg_map_[i].value));
    }
}
int SettingWindow::GetMsgIdMap(string mstring)
{
    int getMsgId_map_size = getMsgId_map_.size();
    int index = -1;
    StringIntMap::iterator s_iter = getMsgId_map_.find(mstring);
    if (s_iter != getMsgId_map_.end())
        index =  s_iter->second;
    db_msg("[debug_zhb]-----GetMsgIdMap---index = %d",index);
    return index;
}

void SettingWindow::InitItemData(ItemData &data,int msgid,StringIntMap &_map,StringIntMap &value_map,StringVector &resultString,int len,int rsubcnt)
{
	int type_ = -1,value = -1;
	int i=0;
	R *r = R::get();

	switch(msgid){
        case SETTING_CAMERA_AUTOSCREENSAVER:
            for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
                if(i<rsubcnt){
                    type_ = TYPE_SUBITEM_CHOICE;
                    value = -1;
                }else{
                    type_ = TYPE_SUBITEM_SWITCH;
                    value = GetMenuConfig(SETTING_SCREEN_NOT_DISTURB_MODE);
                }
                //db_msg("[debug_zhb]----------it->c_str() = %s type = %d  value = %d",it->c_str(),type_,value);
                _map.insert(make_pair(it->c_str(),type_));
                value_map.insert(make_pair(it->c_str(),value));
            }
            break;
		case SETTING_RECORD_RESOLUTION:
            {
                string str;
                for(int i = 0 ; i < NUM_DASH_CAMERA ; i++){
                    r->GetString(dash_camera_[i].item_name, str);
                    SetSubMenuStringValueOrType(&_map,str,dash_camera_[i].type);
                    SetSubMenuStringValueOrType(&value_map,str,GetMenuConfig(dash_camera_[i].msgid));
                    resultString.push_back(str.c_str());
                }
            }
            break;
		case SETTING_ADAS_SWITCH:
            {
				/*
				for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
                    type_ = TYPE_SUBITEM_CHOICE;
                    value = -1;
                    _map.insert(make_pair(it->c_str(),type_));
                    value_map.insert(make_pair(it->c_str(),value));
                }
				*/
                string str;
                for(int i = 0 ; i < NUM_ADAS ; i++){
                    r->GetString(adas_[i].item_name, str);
                    SetSubMenuStringValueOrType(&_map,str,adas_[i].type);
                    SetSubMenuStringValueOrType(&value_map,str,GetMenuConfig(adas_[i].msgid));
                    resultString.push_back(str.c_str());
                }
            }
			break;
		case SETTING_4G_NET_WORK:
			{
				for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
				if(i<rsubcnt){//handle turn off and turn on item
					type_ = TYPE_SUBITEM_CHOICE;
					value = -1;
				}else{
					type_ = TYPE_SUBITEM_ENLARGE;
					value = -1;//handle sim card information item
					}
				//db_msg("[debug_zhb]----SETTING_4G_NET_WORK------it->c_str() = %s type = %d  value = %d",it->c_str(),type_,value);
				_map.insert(make_pair(it->c_str(),type_));
				value_map.insert(make_pair(it->c_str(),value));
		             }
					string str;
					for(int i = 0 ; i < NUM_4G_NETWORK ; i++){
						r->GetString(network_4g_[i].item_name, str);
						SetSubMenuStringValueOrType(&_map,str,network_4g_[i].type);
						SetSubMenuStringValueOrType(&value_map,str,network_4g_[i].msgid);
						if(GetMenuConfig(msgid) == 1)
							resultString.insert(resultString.begin()+2+i,str.c_str());
					}

			}break;
		case SETTING_WATCH_DOG_SWITCH:
			{
				for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
				if(i<rsubcnt){//handle turn off and turn on item
					type_ = TYPE_SUBITEM_CHOICE;
					value = -1;
				}else if(i == rsubcnt){//handle beep volume
					type_ = TYPE_SUBITEM_SET;
					value = GetMenuConfig(SETTING_VOLUME_SELECTION);
					}else{//handle update data
						type_ = TYPE_SUBITEM_UPDATE;
						value = -1;
						}
				//db_msg("[debug_zhb]----SETTING_WATCH_DOG_SWITCH------it->c_str() = %s type = %d  value = %d",it->c_str(),type_,value);
				_map.insert(make_pair(it->c_str(),type_));
				value_map.insert(make_pair(it->c_str(),value));
		             }
					string str;
					for(int i = 0 ; i < NUM_WATCHDOG ; i++){
						r->GetString(watch_dog_[i].item_name, str);
						SetSubMenuStringValueOrType(&_map,str,watch_dog_[i].type);
						SetSubMenuStringValueOrType(&value_map,str,GetMenuConfig(watch_dog_[i].msgid));
						if(GetMenuConfig(msgid) == 1)
							resultString.insert(resultString.begin()+2+i,str.c_str());
					}

			}break;
		case SETTING_SCREEN_BRIGHTNESS:
			{
				for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
					type_ = TYPE_SUBITEM_SET;
					value = GetMenuConfig(msgid);
					db_msg("[debug_zhb]----SETTING_SCREEN_BRIGHTNESS------it->c_str() = %s type = %d  value = %d",it->c_str(),type_,value);
					_map.insert(make_pair(it->c_str(),type_));
					value_map.insert(make_pair(it->c_str(),value));
		             }
				string str;
				r->GetString("ml_screen_brightness_level", str);
				menu_items_->setLevelStr(str);
			}
			break;
		case SETTING_WIFI_SWITCH:
			{
				for(StringVector::iterator it = resultString.begin();it != resultString.end() || i < len;i++,it++){
				if(i<rsubcnt){
					type_ = TYPE_SUBITEM_CHOICE;//handle turn off and turn on item
					value = -1;
				}else {
					type_ = TYPE_SUBITEM_ENLARGE;//handle view wifi info and scan qr code download link item
					value = -1;
					}
				//db_msg("[debug_zhb]----SETTING_WIFI_SWITCH------it->c_str() = %s type = %d  value = %d",it->c_str(),type_,value);
				_map.insert(make_pair(it->c_str(),type_));
				value_map.insert(make_pair(it->c_str(),value));
		             }
			}
			break;
		case SETTING_SYSTEM_SOUND:
			{
				string str;
				for(int i = 0 ; i < NUM_SYSTEM_SOUND ; i++){
					r->GetString(system_sound_[i].item_name, str);
					SetSubMenuStringValueOrType(&_map,str,system_sound_[i].type);
					SetSubMenuStringValueOrType(&value_map,str,GetMenuConfig(system_sound_[i].msgid));
					resultString.push_back(str.c_str());
				}

			}break;
		case SETTING_PARKING_MONITORY:
			{
				string str;
				for(int i = 0 ; i < NUM_PACKING_MONITORY-1 ; i++){
					r->GetString(packingmonitory_[i].item_name, str);
					SetSubMenuStringValueOrType(&_map,str,packingmonitory_[i].type);
					SetSubMenuStringValueOrType(&value_map,str,GetMenuConfig(packingmonitory_[i].msgid));
					resultString.push_back(str.c_str());
				}
				StringVector loop_string;
				loop_string.clear();
    				r->GetStringArray(packingmonitory_[4].item_name, loop_string);
				vector<string>::const_iterator it;
				int i = 0,val = -1;
				val = GetMenuConfig(packingmonitory_[4].msgid);
				//db_msg("[debug_zhb]----SETTING_PARKING_MONITORY-----val = %d",val);
				  for(it = loop_string.begin(); it !=  loop_string.end(); it++,i++){//move the submenu string to the item_string
				  	str = it->c_str();
					SetSubMenuStringValueOrType(&_map,str,packingmonitory_[4].type);
					SetSubMenuStringValueOrType(&value_map,str,i == val?1:0);
					if(GetMenuConfig(packingmonitory_[3].msgid) == 1)
						resultString.push_back(it->c_str());
				    }

			}
			break;
        case SETTING_ACCOUNT_BINDING:
            {
				stringstream info_str;
				string str;
                info_str.clear();
                data.devIsNeedBind = account_bind_flag_;
                data.type = TYPE_IMAGE_NEED_BIND_BUTTON;

                r->GetString(string(account_info_[1].item_name), str);
                info_str<<str.c_str()<<"\n";
                r->GetString(string(account_info_[2].item_name), str);
                info_str<<str.c_str()<<"\n";
                r->GetString(account_info_[3].item_name, data.button_string2);
                data.button_string1 = info_str.str().c_str();
            }
            break;
		case SETTING_DEVICE_DEVICEINFO:
			{
                db_msg("[debug_zhb]----SETTING_DEVICE_DEVICEINFO--");
				char buf_sn[128] = {0};
				char buf_imei[128] = {0};
				stringstream info_str0;
				stringstream info_str1;
				string str;
				//PartitionManager::GetInstance()->sunxi_spinor_private_get("sn",buf_sn,sizeof(buf_sn));
				EventManager *ev = EventManager::GetInstance();
				data.type = TYPE_IMAGE_BUTTON;
				for(int i = 0 ; i < 2 ; i++){
					r->GetString(string(device_info_[i].item_name),str);
					if(i == 0){
						info_str0<<str.c_str()<<buf_sn << "\n";
					}else if(i == 1){
						info_str0<<str.c_str()<< ev->getSimId().c_str()<<"\n";
					}
				}
				getSdcardInfo(info_str1);
				info_str1<<info_str0.str().c_str()<<"\n";
				data.button_string1 = info_str1.str().c_str();
				r->GetString(sdcard_info_[3].item_name, data.button_string2);
			}
            break;
		case SETTING_DEVICE_SDCARDINFO:
			{
				db_msg("[debug_zhb]----SETTING_DEVICE_SDCARDINFO--");
				r->GetString(sdcard_info_[2].item_name, data.button_string1);
				r->GetString(sdcard_info_[3].item_name, data.button_string2);
				#ifndef S_FORMAT
				data.type =TYPE_IMAGE_STRING_ONLY;
				#endif
			}break;
		case SETTING_DEVICE_VERSIONINFO:
			{
                string str;
                stringstream info_str;
                data.fupdate = new_version_flag;
                if(!new_version_flag){//no version is new
                    data.type = TYPE_IMAGE_STRING_ONLY;
                    r->GetString(string(version_info_[0].item_name),str);
                    info_str<<str.c_str()<<version_str<<"\n\n";
                    r->GetString(string(version_info_[1].item_name),str);
                    info_str<<str.c_str()<<"\n";
					data.button_string1 = info_str.str().c_str();
                }else{//if detect has the new version can be update
                    data.type = TYPE_IMAGE_BUTTON;
                    r->GetString(version_info_[3].item_name, data.button_string2);
                    r->GetString(string(version_info_[0].item_name), str);
                    info_str<<str.c_str()<<version_str<<"\n\n";
                    r->GetString(version_info_[4].item_name, str);
                    info_str<<str.c_str()<<"\n";
                    data.button_string1 = info_str.str().c_str();
                }
			}break;
		case SETTING_DEVICE_TIME:
			{
				db_msg("[debug_zhb]----SETTING_DEVICE_TIME--");
				 string str;
				 date_t date;
				 char buf[128]={0};
				 m_TimeSettingObj->getSystemDate(&date);
				 r->GetString("ml_device_time_current",str);
				 snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d  %02d:%02d",str.c_str(),date.year,date.month,date.day,date.hour,date.minute);
				 data.time_string = buf;
				 db_msg("[debug_zhb]-----data.time_string = %s",data.time_string.c_str());
			}break;
		default:
            for(StringVector::iterator it = resultString.begin();it != resultString.end();it++){
                _map.insert(make_pair(it->c_str(), TYPE_SUBITEM_CHOICE));
                value_map.insert(make_pair(it->c_str(),value));
            }
            break;
	}
}

void SettingWindow::updateItemData()
{
    updateAccountBindStatus(4);
    updateVersionFirmwareInfo(6); 
}

void SettingWindow::InitMenuItem(const char *item_name,const char *sub_head,const char *item_tips,
                                const char *unhilight_icon, const char *hilight_icon,const char *unselect_icon, const char *select_icon,
                                int type,int subcnt, int value,int msgid)
{
    StringIntMap map_,map_vaule;
    char path[64];
    R *r = R::get();
    ItemData data;
    data.first_icon_path[UNHILIGHTED_ICON] += r->GetImagePath(unhilight_icon);
    data.first_icon_path[HILIGHTED_ICON] += r->GetImagePath(hilight_icon);
    data.second_icon_path[UNHILIGHTED_ICON] += r->GetImagePath(unselect_icon);
    data.second_icon_path[HILIGHTED_ICON] += r->GetImagePath(select_icon);
    for(int i = 0 ; i < THIRD_ICON_NUM ; i++)
        data.third_icon_path[i] += r->GetImagePath(third_icon_[i].icon_name);
    data.type = type;
    data.fupdate = 0;
    data.devIsNeedBind = true;

    r->GetString(string(item_name), data.item_string);
    r->GetString(string(sub_head), data.subitem_string);//sub head
    r->GetString(string(item_tips), data.item_tips);
	db_msg("debug_zhb----InitMenuItem------data.item_tips = %s",data.item_tips.c_str());
    data.result_string.clear();
    r->GetStringArray(string(item_name), data.result_string);

    InitItemData(data,msgid,map_, map_vaule, data.result_string,data.result_string.size(),subcnt);
    data.subMenuStringType = map_;
    data.subMenuStringvalue = map_vaule;
    data.subitemcnt = subcnt;
    //db_msg("debug_zhb---data.fupdate = %d-------subcnt = %d",data.fupdate,subcnt);
    data.value = value;
    //db_msg("debug_zhb------data.item_string = %s--------data.value = %d  data.result_string.size()= %d",data.item_string.c_str(),data.value,data.result_string.size());
    data.sub_hilight =GetMenuConfig(GetMsgIdMap(string(item_name)));
    data.submenuflag = 0;
    data.submenuflag_second= 0;
    data.result_cnt = data.result_string.size();
    r->GetString("ml_turn_on", data.on_string);
    r->GetString("ml_turn_off", data.off_string);

#if 0
   db_msg("[debug_zhb]----SettingWindow:---data.item_string = %s",data.item_string.c_str());
   vector<string>::const_iterator it;
  for(it = data.result_string.begin(); it !=  data.result_string.end(); it++)
		db_msg("[debug_zhb]----SettingWindow:---data.result_string = %s    result_cnt = %d",it->c_str(),data.result_cnt);
   db_msg("[debug_zhb]----SettingWindow:---data.value = %d",data.value);
   db_msg("[debug_zhb]----SettingWindow:---data.sub_hilight = %d",data.sub_hilight);
   db_msg("[debug_zhb]----SettingWindow:---data.type = %d",data.type);
   db_msg("[debug_zhb]----SettingWindow:---data.result_cnt = %d",data.result_cnt);
  #endif
    menu_items_->add(data);
}

void SettingWindow::InitMenuItem(const menu_win &menu, int index,int msgid)
{
    InitMenuItem(menu.item_name, menu.sub_head,menu.item_tips,menu.unhilight_icon, menu.hilight_icon,
            menu.unselect_icon, menu.select_icon, menu.type,menu.value,index,msgid);
}

void SettingWindow::InitMenuWinStatu(int menustatu)
{
    int index=0;

    db_msg("menu status: %d \n", menustatu);

    for(int i=0; i<SETTING_RECOR_NUMBER;i++){
        index = GetMenuConfig(USER_MSG_BASE +menustatu+ i+1);
        InitMenuItem(menu_record_win[i], index, USER_MSG_BASE + menustatu + i + 1);
    }
    menu_items_->SetHilight(0);
    menu_items_->SetTop();

}

void SettingWindow::DeInitMenuItem(int pos)
{
    db_msg(" [zhb]:----DeInitMenuItem ----height number: %d \n",pos);
    menu_items_->remove(pos);
}

void SettingWindow::DeInitMenuWinStatu(int menustatu)
{
    menu_items_->CancleAllHilight();
    menu_items_->FreedBeforeRemoveAll();
    menu_items_->removeAll();

    return;
}
int SettingWindow::GetMenuConfig(int msg)
{
       int val=0;
       MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
	val = menuconfiglua->GetMenuIndexConfig(msg);
       db_msg("[fangjj]:GetMenuConfig:msg[%d], val[%d]", msg, val);
	return val;
}
void SettingWindow::ResetUpdate()
{
      DeInitMenuWinStatu(menu_statu);
      for(int i=0; i<SETTING_RECOR_NUMBER;i++){
           InitMenuItem(menu_record_win[i], GetMenuConfig(USER_MSG_BASE +menu_statu+ i+1),USER_MSG_BASE +menu_statu+ i+1);
        }
      //menu_items_->SetHilight(0);
      //menu_items_->SetTop();
}


SettingWindow::SettingWindow(IComponent *parent)
    : SystemWindow(parent)
    ,win_mg_(WindowManager::GetInstance())
    ,m_show_dialog_(false)
    ,button_dialog_current_id_(BUTTON_DIALOG_REBOOT)
    ,index_flag(-1)
    ,menu_statu(MENU_STATU_RECORD)
    ,sw_flag(0)
    ,m_nhightLightIndex(0)
    ,m_bIsformatting(false)
    ,new_version_flag(false)
    ,detect_version_manual(false)
    ,m_sdcard_info_ui(false)
    ,m_packet_len(0.0)
    ,m_abnormal_update(false)
    ,m_formart_dialog_display(false)
{
    db_msg(" ");
    Load();

    wname = "SettingWindow";
    SetBackColor(0xFF1A1E38);//yellow
    SystemVersion(false);
    magic_block_ = new MagicBlock(this);

    menu_items_ = reinterpret_cast<MenuItems *> (GetControl(menu_statu_win_[menu_statu/20]));
    menu_items_->OnItemClick.bind(this, &SettingWindow::MenuItemClickProc);

    string menu_list_bmp = R::get()->GetImagePath("setting_bg");
    db_msg("set back ground pic:%s", menu_list_bmp.c_str());
    menu_items_->SetWindowBackImage(menu_list_bmp.c_str());
    menu_items_->SetBackColor(0xFF1A1E38);
    /*****   [fangjj]:InitMenuWinStatu *****/
    InitGetMsgIdMap();
    InitMenuWinStatu(menu_statu);
    initInfoDialog();
    //initFirmwareDownloadPrompt();
    m_TimeSettingObj = new TimeSettingWindow(this);
    levelbar_=new LevelBar(this);
	s_PromptBox_ = new PromptBox(this);
	s_BulletCollection = new BulletCollection();
	s_BulletCollection->initButtonDialog(this);

	create_timer(this, &clearOldOperation_timer_id,ClearOldOperationTimerProc);
	stop_timer(clearOldOperation_timer_id);
    pthread_mutex_init(&setwindow_proc_lock_,NULL);
}
void SettingWindow::BcMsgSend(bool m_buttonDialog,int val)
{
	if(m_buttonDialog)
		listener_->notify(this,SETTING_BUTTON_DIALOG,val);
//	else
//		listener_->sendmsg(this,MSG_SET_BUTTON_STATUS,val);
}

//void SettingWindow::initFirmwareDownloadPrompt()
//{
//    firmwareDowloadPrompt_ = new Prompt(0, this);
//    firmwareDowloadPrompt_->OnClick.bind(this, &SettingWindow::DownloadClickProc);
//}

void SettingWindow::initInfoDialog()
{
    dialog_ = new Dialog(INFO_DIALOG, this);
    // dialog_->SetBackColor(0xFFDDDDDD);
    dialog_->OnClick.bind(this, &SettingWindow::DialogClickProc);
    dialog_->SetPosition(0, 0, 854, 480);
    string dilaog_bg= R::get()->GetImagePath("dialog_bg");
    dialog_->SetWindowBackImage(dilaog_bg.c_str());

    dialog_title_ = reinterpret_cast<TextView*>(dialog_->GetControl("dialog_title"));
    dialog_title_->SetCaption("");
    dialog_title_->SetBackColor(0xFFFFCE42); // 灰色
    dialog_title_->SetTextStyle(DT_LEFT);
    //dialog_title_->SetPosition(0, 20, 290, 30);

    dialog_info_text = static_cast<TextView*>(dialog_->GetControl("info_text"));
    //dialog_info_text->SetPosition(20, 50, 250, 110);

    GraphicView::LoadImage(dialog_->GetControl("dialog_icon1"), "phone");
    dialog_->GetControl("dialog_icon1")->Hide();
    //GraphicView::LoadImage(dialog_->GetControl("dialog_icon3"), "wifi_rect");
    //dialog_->GetControl("dialog_icon3")->SetPosition(400, 62, 206, 206);
    //dialog_->GetControl("dialog_icon3")->Hide();

    if(access("/data/WifiAppQR.png", R_OK) == 0) {
        db_msg("[debug_zhb]----------use -data/WifiAppQR.png");
        GraphicView::LoadImageFromAbsolutePath(dialog_->GetControl("dialog_icon2"), "/data/WifiAppQR.png");
    }else{
        db_msg("[debug_zhb]----------no use -data/WifiAppQR.png");
        GraphicView::LoadImage(dialog_->GetControl("dialog_icon2"), "qrcode");
    }
    dialog_->GetControl("dialog_icon2")->SetPosition(400, 62, 206, 206);
    dialog_->GetControl("dialog_icon2")->Hide();

    create_timer(this, &timer_download_process_, HandleUpdate_download_Process);
    stop_timer(timer_download_process_);
    dialog_->GetControl("dialog_progress_bar_background")->Hide();
    dialog_->GetControl("dialog_progress_bar_top_color")->Hide();
}

void SettingWindow::HandleUpdate_download_Process(union sigval sigval)
{
    prctl(PR_SET_NAME, "SettingWindow", 0, 0, 0);
    SettingWindow *sw = reinterpret_cast<SettingWindow*>(sigval.sival_ptr);

    int process = sw->getDownloadProcess();
    char buf[8];
    snprintf(buf, sizeof(buf), "%d %%", process);
    sw->dialog_title_->SetCaption(buf);
    if(process >= 100)
    {
        if(sw->installer_time_ >= INSTALLER_TIMEOUT)
        {
            db_error("install firmware %d times failed", sw->installer_time_);
            sw->HideUpdateProceccInfo(false);
        }

        if(sw->getSystemVersionDLFlag() == true) // Is the download firmware is install ok
        {
            sw->HideUpdateProceccInfo(true);
        }else
        {
            sw->installer_time_++;
        }
    }else
    {
        if(sw->IsDownloadStatusReady() == false ||
                sw->GetDownloadStatus() == STATUS_DOWNLOAD_FAIL) // download firmware failed!
        {
            db_error("download failed!\n");
            sw->HideUpdateProceccInfo(false);
            sw->SetDownloadStatus(STATUS_DOWNLOAD_INIT);
        }
    }
}

void SettingWindow::HideUpdateProceccInfo(bool is_fail)
{
    stop_timer(timer_download_process_);
    dialog_->DoHide();
    m_show_dialog_ = false;
    if(is_fail) //download suceess
    {
       if(p_device_setting != NULL)
       {
           p_device_setting->exeOtaUpdate(TYPE_UPDATE_NET);
       }
       else
       {
           db_error("get pointer p_device_setting failed");
           return;
       }
    }else
    {

    }
}

bool SettingWindow::getSystemVersionDLFlag()
{
    WindowManager    *win_mg_ = WindowManager::GetInstance();
    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    return p_win->getSystemVersionDLFlag();
}

int SettingWindow::getDownloadProcess()
{
    return VersionUpdateManager::GetInstance()->getProgressPercent();
}

bool SettingWindow::IsDownloadStatusReady()
{
    WindowManager    *win_mg_ = WindowManager::GetInstance();
    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    return p_win->IsDownloadStatusReady();
}

int SettingWindow::GetDownloadStatus()
{
    WindowManager    *win_mg_ = WindowManager::GetInstance();
    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    return p_win->GetDownloadStatus();
}

int SettingWindow::SetDownloadStatus(int status)
{
    WindowManager    *win_mg_ = WindowManager::GetInstance();
    PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    p_win->SetDownloadStatus(status);
    return 0;
}

void SettingWindow::ShowFirmwareDownloadDialog()
{
    if(m_show_dialog_)
    {
        db_msg("this is show........");
        return;
    }
    string str;
    dialog_->SetPosition(197, 124, 460, 232);
    dialog_->SetWindowBackImage(R::get()->GetImagePath("dialog_bg").c_str());
    dialog_->SetDialogMsgType(SET_FIRMWARE_DOWNLOAD);

    dialog_->GetControl("dialog_icon4")->Hide();

    R::get()->GetString("ml_firmware_downloading_text", str);
    dialog_info_text->SetCaption(str.c_str());
    dialog_info_text->SetCaptionColor(0xFFFFFFFF);
    dialog_info_text->SetPosition(10, 50, 440, 50);
    dialog_info_text->SetTextStyle(DT_CENTER);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d %%", 0);
    dialog_title_->SetCaption(buf);
    dialog_title_->SetTextStyle(DT_CENTER);
    dialog_title_->SetCaptionColor(0xFFFFFFFF);
    dialog_title_->SetPosition(10, 100, 440, 50);

    WindowManager *win_mg_ = WindowManager::GetInstance();
    PreviewWindow *pw = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    pw->SetStartDownload(true);
    installer_time_ = 0;
    set_period_timer(1, 0, timer_download_process_);
    dialog_->DoShow();
    m_show_dialog_ = true;
}

void SettingWindow::ShowInfoDialog(const std::vector<std::string> &info,const std::string &tilteStr,int msgid)
{
    if(m_show_dialog_){
        db_msg("[debug_zhb]---> showinfodialog has been show");
        return;
    }
    dialog_->SetDialogMsgType(INFO_DIALOG);
    stringstream info_str;
    vector<string>::const_iterator it;
    for (it = info.begin(); it != info.end(); it++) {
        info_str << it->c_str() << "\n\n";
    }
    db_msg("[debug_zhb]------title = %s   info_str.str().c_str() = %s",tilteStr.c_str(),info_str.str().c_str());
   switch(msgid){
	case MSG_SHOW_WIFI_APP_DOWNLAOD_LINK:
	case MSG_SET_4G_FLOW_RECHARGE:
		{
			dialog_info_text->SetPosition(80, 184, 260, 100);
			//dialog_->GetControl("dialog_icon1")->SetPosition(80,20,60,100);
			//dialog_->GetControl("dialog_icon2")->SetPosition(220,125,150,150);
			//dialog_->GetControl("dialog_icon3")->Show();
			GraphicView::LoadImage(dialog_->GetControl("dialog_icon1"), "phone");
			dialog_->GetControl("dialog_icon1")->SetPosition(155,62,90,90);
			dialog_->GetControl("dialog_icon1")->Show();
			dialog_->GetControl("dialog_icon2")->Show();

		}
		break;
	case MSG_SHOW_WIFI_INFO:
		{
			dialog_title_->SetPosition(198, 78, 150, 42);
			dialog_info_text->SetPosition(198, 144, 400, 200);
			GraphicView::LoadImage(dialog_->GetControl("dialog_icon1"), "wifi_info");
			dialog_->GetControl("dialog_icon1")->SetPosition(54,120,90,90);
			dialog_->GetControl("dialog_icon1")->Show();
			dialog_->GetControl("dialog_icon2")->Hide();
			//dialog_->GetControl("dialog_icon3")->Hide();
		}
		break;
    case MSG_SHOW_BIND_QR:
        {
            string str;
            dialog_->SetPosition(0, 0, 854, 480);
            dialog_->SetWindowBackImage(R::get()->GetImagePath("dialog_bg").c_str());
            R::get()->GetString("ml_bindwindow_uber_text", str);
            dialog_info_text->SetTextStyle(DT_LEFT|DT_WORDBREAK);
            dialog_info_text->SetCaptionColor(0xFFFFFFFF);
            dialog_info_text->SetCaption(str.c_str());
            dialog_info_text->SetPosition(290, 100, 500, 300);
            dialog_->GetControl("dialog_icon4")->SetPosition(50, 90, 206, 206);
            dialog_->GetControl("dialog_icon4")->Show();
            dialog_title_->SetTextStyle(DT_CENTER);
            dialog_title_->SetCaption(tilteStr.c_str());
            dialog_title_->SetCaptionColor(0xFFFFFFFFFF);
        }
        break;
    case MSG_NO_BIND_QR:
        {
            string str;
            dialog_->SetPosition(0, 0, 854, 480);
            dialog_->SetWindowBackImage(R::get()->GetImagePath("dialog_bg").c_str());

            dialog_->GetControl("dialog_icon4")->Hide();
            dialog_title_->SetTextStyle(DT_CENTER);
            dialog_title_->SetCaption(tilteStr.c_str());
            dialog_title_->SetCaptionColor(0xFFFFFFFFFF);

            R::get()->GetString("ml_account_bind_failed_info", str);
            dialog_info_text->SetTextStyle(DT_LEFT|DT_WORDBREAK);
            dialog_info_text->SetCaptionColor(0xFFFFFFFF);
            dialog_info_text->SetCaption(str.c_str());
            dialog_info_text->SetPosition(80, 100, 690, 100);
        }
        break;
    default:
        dialog_->GetControl("dialog_icon1")->Hide();
        dialog_->GetControl("dialog_icon2")->Hide();
        //dialog_->GetControl("dialog_icon3")->Hide();
		break;
   	}

  // if(msgid != MSG_SHOW_BIND_QR)
  //     dialog_info_text->SetCaption(info_str.str().c_str());
   //dialog_info_text->SetCaptionColor(0xFFFFFFFF);
   //dialog_title_->SetTextStyle(DT_CENTER);
   //dialog_title_->SetCaption(tilteStr.c_str());
   //dialog_title_->SetCaptionColor(0xFFFFFFFFFF);
   dialog_->DoShow();
   m_show_dialog_ = true;
}
void SettingWindow::HideInfoDialog()
{
	db_msg("[debug_zhb]-----setting window-----HideInfoDialog");
	dialog_->DoHide();
	m_show_dialog_ = false;
}
void SettingWindow::MenuToSubmenuSettingButtonStatus(int msgid)
{
	return ;
	int status_index = -1;
	switch(msgid){
		case SETTING_SCREEN_BRIGHTNESS:
		case SETTING_DEVICE_DEVICEINFO:
		case SETTING_DEVICE_SDCARDINFO:
			status_index = SET_BUTTON_UPDOWN_HIDE;
			break;
		case SETTING_DEVICE_VERSIONINFO:
			if(new_version_flag)
				status_index = SET_BUTTON_UPDOWN_HIDE;
			else
				status_index = SET_BUTTON_UP_DOWN_CHOICE_HIDE;
			break;
		case SETTING_DEVICE_TIME:
			//status_index = SET_BUTTON_LEFTRIGHT_SHOW;
			status_index = SET_BUTTON_UPDOWN_HIDE;
			break;
		case MSG_SET_BUTTON_STATUS:
			status_index = SET_BUTTON_UP_DOWN_CHOICE_SHOW;//SET_BUTTON_UPDOWN_SHOW;
			break;
		default:
			return;
			break;
		}
	db_msg("[debug_zhb]-------------status_index = %d",status_index);
	listener_->sendmsg(this,MSG_SET_BUTTON_STATUS,status_index);
}

void SettingWindow::MenultemkeyEventProc()//to submenu
{
    db_msg("debug_zhb---> MenultemkeyEventProc --begine");
    int highlight_index = menu_items_->GetHilight();
    if(highlight_index == -1){
        db_msg("zhb------MenultemkeyEventProc");
        return;
    }
    int handle_id = highlight_index + 1;
    m_nhightLightIndex = highlight_index;
    db_msg("debug_zhb---> MenultemkeyEventProc --highlight_index = %d",highlight_index);
    ItemData * data = menu_items_->GetItemData(highlight_index);
    if(data == NULL)
    {
        db_error("MenultemkeyEventProc -- data == NULL");
        menu_items_->setPaintSecondLevelFlag(false);
        return ;
    }
    int &value = data->value;

    if(USER_MSG_BASE +menu_statu+ handle_id == SETTING_DEVICE_SDCARDINFO){
        m_sdcard_info_ui = true;
        StorageManager *sm = StorageManager::GetInstance();
        int status = sm->GetStorageStatus();
        if(status == UMOUNT)
        {
            WindowManager    *win_mg_ = WindowManager::GetInstance();
            PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
            p_win->ShowPromptInfo(PROMPT_TF_NO_EXIST,2);
            menu_items_->setPaintSecondLevelFlag(false);
            return ;
        }

    }else{
        m_sdcard_info_ui = false;
    }
    listener_->sendmsg(this, (USER_MSG_BASE +menu_statu+ handle_id), value);
    db_msg("debug_zhb---> MenultemkeyEventProc --end");
}

void SettingWindow::MenuItemClickProc(View *control)
{
    MenuItems *menu_item = (MenuItems *)control;
    static int last_index = 0;

    int highlight_index = menu_items_->GetHilight();
    int handle_id = highlight_index + 1;

    ItemData *c_item = menu_item->GetItemData(highlight_index);

	if(GetMenuConfig(SETTING_KEY_SOUND_SWITCH) == 1){
		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
 	}

    if(c_item == NULL)
    {
        db_error("MenuItemClickProc-- data == NULL");
        menu_items_->setPaintSecondLevelFlag(false);
        return;
    }
    int &value = c_item->value;
    if(c_item->submenuflag_second == 1)
    {
        //if (highlight_index - last_index != 0)
        //{
         //   last_index = highlight_index;
        //    return;
        //}
        updataSubmenuItemChoiced(true);
        //last_index = highlight_index;
    }else
    {
        if (highlight_index - last_index != 0)
        {
            db_msg("highlight_index:%d, last_index:%d", highlight_index, last_index);
            last_index = highlight_index;
            return;
        }

        db_msg("msg:%d, value=%d, handle_id=%d", USER_MSG_BASE +menu_statu+ handle_id, value, handle_id);

        listener_->sendmsg(this, (USER_MSG_BASE + menu_statu + handle_id), value);

        last_index = highlight_index;
        menu_items_->main_item = highlight_index;
    }
}

void SettingWindow::updateAccountBindStatus(int p_index)
{
    ItemData *data  = menu_items_->GetItemData(p_index);
    if(data == NULL)
    {
        db_error("ItemData data is null");
        return;
    }
    stringstream info_str;
    string str;
    string id_str;
    info_str.clear();

    data->type = TYPE_IMAGE_NEED_BIND_BUTTON;
    data->devIsNeedBind = account_bind_flag_;

    R::get()->GetString(string(account_info_[1].item_name), str);
    info_str<<str.c_str()<<"\n";
    R::get()->GetString(string(account_info_[2].item_name), str);
    info_str<<str.c_str()<<"\n";
    R::get()->GetString(account_info_[3].item_name, data->button_string2);

    data->button_string1 = info_str.str().c_str();
    ::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);
}

void SettingWindow::setNewVersionFlag(bool flag)
{
    new_version_flag = flag;
}

void SettingWindow::updateVersionFirmwareInfo(int p_index)
{
    ItemData *data  = menu_items_->GetItemData(p_index);
    if(data == NULL)
    {
        db_error("ItemData data is null");
        return;
    }
    stringstream info_str;
    string str;
    string id_str;
    info_str.clear();

    R *r = R::get();
    data->fupdate = new_version_flag;
    if(!new_version_flag) //no version is new
    {
        data->type = TYPE_IMAGE_STRING_ONLY;
        r->GetString(string(version_info_[0].item_name), str);
        info_str<<str.c_str()<<version_str<<"\n\n";
        r->GetString(version_info_[1].item_name, str);
        info_str<<str.c_str()<<"\n";
        data->button_string1 = info_str.str().c_str();
    }else
    {
        data->type = TYPE_IMAGE_BUTTON;
        r->GetString(version_info_[3].item_name, data->button_string2);
        r->GetString(string(version_info_[0].item_name), str);
        info_str<<str.c_str()<<version_str<<"\n\n";
        r->GetString(version_info_[4].item_name, str);
        info_str<<str.c_str()<<"\n";
        data->button_string1 = info_str.str().c_str();
    }

    ::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);

}

void SettingWindow::DialogClickProc(View *control)
{
    Dialog *dialog = static_cast<Dialog*>(control);
    dialog->DoHide();
    m_show_dialog_ = false;
    //updateAccountBindStatus(menu_items_->GetHilight());
}

SettingWindow::~SettingWindow()
{
	if (magic_block_) {
	    delete magic_block_;
	    magic_block_ = NULL;
	}
	if (menu_items_) {
	   delete menu_items_;
	   menu_items_ = NULL;
	}
	//if (win_mg_) {
	//	delete win_mg_;
	//	win_mg_ = NULL;
	//	}
	if (dialog_title_) {
		delete dialog_title_;
		dialog_title_ = NULL;
		}
	if (dialog_info_text) {
		delete dialog_info_text;
		dialog_info_text = NULL;
		}
	if (button_dialog_) {
		delete button_dialog_;
		button_dialog_ = NULL;
		}
	if (button_dialog_title) {
		delete button_dialog_title;
		button_dialog_title = NULL;
		}
	if (button_dialog_text) {
		delete button_dialog_text;
		button_dialog_text = NULL;
		}
	if (button_dialog_confirm) {
		delete button_dialog_confirm;
		button_dialog_confirm = NULL;
		}
	if (button_dialog_cancel) {
		delete button_dialog_cancel;
		button_dialog_cancel = NULL;
		}
	if (button_dialog_progress_bar_background) {
		delete button_dialog_progress_bar_background;
		button_dialog_progress_bar_background = NULL;
		}
	if (button_dialog_progress_bar_top_color) {
		delete button_dialog_progress_bar_top_color;
		button_dialog_progress_bar_top_color = NULL;
		}
	if (dialog_) {
	delete dialog_;
	dialog_ = NULL;
		}
	if (levelbar_) {
	delete levelbar_;
	levelbar_ = NULL;
		}
	if( m_TimeSettingObj != NULL)
	{
	    delete m_TimeSettingObj;
	    m_TimeSettingObj = NULL;
	}
	delete_timer(timer_download_process_);
    delete_timer(clearOldOperation_timer_id);
    pthread_mutex_unlock(&setwindow_proc_lock_);
}

string SettingWindow::GetResourceName()
{
    db_msg(" ");
    return string(GetClassName());
}

void SettingWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    ctrl->SetCtrlTransparentStyle(true);
}

void SettingWindow::UpdateItemString(int index, const std::string &str)
{
    ItemData *data = menu_items_->GetItemData(index);

    if(data != NULL)
    {
        data->item_string = str;
        ::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);
    }else
    {
         db_warn("UpdateItemValue:: the data is NULL be careful");
    }
}

void SettingWindow::ShowLevelBar(int levelbar_id)
{
	RECT rect;
	levelbar_->DoShow();
    	levelbar_->ShowLevelBar(levelbar_id,leveldata.rect_,leveldata.index_);
}
int SettingWindow::GetMenuIndex()
{
    return menuIndex_;
}

void SettingWindow::SetIndexToSubmenu(int index)
{
    menuIndex_= index;
}
void SettingWindow::SetQuitFlag(bool flag)
{
	menu_items_->SetQuitFlag(flag);
}

void SettingWindow::initDeviceInfoItemData(ItemData &infodata,ItemData*data,int msgid)
{
    R *r = R::get();
    int binding_flag = 1;
    infodata.submenuflag= msgid;
    infodata.submenuflag_second= 1;
    infodata.button_string2 = data->button_string2;
    stringstream info_str;
    string str;
    switch(msgid){
        case SETTING_DEVICE_DEVICEINFO:
            {
                char buf_sn[128] = {0};
                char buf_userName[128] = {0};
                char buf_imei[128] = {0};
                PartitionManager::GetInstance()->sunxi_spinor_private_get("sn",buf_sn,sizeof(buf_sn));
                PartitionManager::GetInstance()->sunxi_spinor_private_get("UserName",buf_userName,sizeof(buf_userName));
                std::string username = AdapterLayer::GetInstance()->base64_decode(buf_userName);
                PartitionManager::GetInstance()->sunxi_spinor_private_get("imei",buf_imei,sizeof(buf_imei));
				EventManager *ev = EventManager::GetInstance();
				infodata.type = TYPE_IMAGE_BUTTON;
                for(int i = 0 ; i < NUM_DEVICE_INFO ; i++){
                    r->GetString(string(device_info_[i].item_name),str);
                    if(i == 0){
                        info_str<<str.c_str()<<buf_sn << "\n";
                    }else if(i == 1){
                        info_str<<str.c_str()<< ev->getSimId().c_str()<<"\n";
                    }else if(i == 2){
                        info_str<<str.c_str()<< username.c_str()<<"\n";
                    }else if(i == 3){
                        info_str<<str.c_str()<< buf_imei<<"\n";
                    }
                }
			}
			break;
        case SETTING_ACCOUNT_BINDING:
            infodata.type = TYPE_IMAGE_NEED_BIND_BUTTON;
            infodata.devIsNeedBind = account_bind_flag_;
            if(account_bind_flag_ == true)
            {
                r->GetString(string(account_info_[0].item_name), str);
                info_str<<str.c_str()<<"\n";
                r->GetString(string(account_info_[4].item_name), str);
                info_str<<str.c_str()<<"\n";
            }else
            {
                r->GetString(string(account_info_[1].item_name), str);
                info_str<<str.c_str()<<"\n";
                r->GetString(string(account_info_[2].item_name), str);
                info_str<<str.c_str()<<"\n";
                r->GetString(string(account_info_[3].item_name), str);
                info_str<<str.c_str()<<"\n";
            }
            break;
		case SETTING_DEVICE_SDCARDINFO:
			{
#ifdef S_FORMAT
                infodata.type = TYPE_IMAGE_BUTTON;
#else
                infodata.type =TYPE_IMAGE_STRING_ONLY;
#endif
                getSdcardInfo(info_str);
            }
			break;
		case SETTING_DEVICE_VERSIONINFO:
            {
                //this need to get the packet total
                if(!infodata.fupdate){//if no detect has new version
                    infodata.type = TYPE_IMAGE_STRING_ONLY ;// ;TYPE_IMAGE_BUTTON
                    for(int i = 0; i < NUM_VERSION_INFO-4 ; i++){
                        r->GetString(string(version_info_[i].item_name),str);
                        if(i == 0)
                            info_str<<str.c_str()<<version_str<<"\n\n";
                        else
                            info_str<<str.c_str()<<"\n\n";
                    }
                }else{//if detect has the new version can be update
                    infodata.type = TYPE_IMAGE_BUTTON;
                    r->GetString(string(version_info_[0].item_name),str);
                    info_str<<str.c_str()<<version_str<<"\n\n";
                    r->GetString(string(version_info_[4].item_name),str);
                }
            }
            break;
        default:
            break;
    }
    infodata.button_string1 = info_str.str().c_str();
}
void SettingWindow::ShowSubmenu(int height,const std::vector<std::string> &info)
{
    index_flag = height;
    ItemData *data  = menu_items_->GetItemData(height);
    if(data == NULL)
    {
        db_error("GetItemData  data is null");
        menu_items_->setPaintSecondLevelFlag(false);
        return ;
    }
    ItemData submenudata;
    string temp_string;
    R *r = R::get();
    submenudata.value= height;
    //submenudata.subitem_string = data->item_string.c_str();//save the submenu head
    submenudata.subitem_string = data->subitem_string;//save the submenu head
    submenudata.first_icon_path[0] +="";
    submenudata.first_icon_path[1] += "";
    submenudata.second_icon_path[0] += "";
    submenudata.second_icon_path[1] +="";
    for(int i = 0 ; i < THIRD_ICON_NUM ; i++)
        submenudata.third_icon_path[i] += r->GetImagePath(third_icon_[i].icon_name);
    submenudata.subMenuStringType = data->subMenuStringType;
    submenudata.subMenuStringvalue = data->subMenuStringvalue;
    submenudata.subitemcnt = data->subitemcnt;
    submenudata.result_cnt = data->result_cnt;
    submenudata.on_string = data->on_string;
    submenudata.off_string = data->off_string;
    submenudata.item_tips = data->item_tips;
    db_msg("debug_zhb----------ShowSubmenu--------submenudata.item_tips = %s    result_cnt = %d",submenudata.item_tips.c_str(),submenudata.result_cnt);
    submenudata.fupdate= data->fupdate;
#if 0
	 db_msg("debug_zhb----------ShowSubmenu--------submenudata.subitemcnt = %d",submenudata.subitemcnt);
	for(StringIntMap::iterator it = submenudata.subMenuStringType.begin();it!=  submenudata.subMenuStringType.end();it++){
		if(it!=submenudata.subMenuStringType.end()){
			db_msg("debug_zhb----------ShowSubmenu---subMenuStringType----- it->c_str = %s  it->second = %d",it->first.c_str(),it->second);
		}
	}
	for(StringIntMap::iterator it = submenudata.subMenuStringvalue.begin();it!=  submenudata.subMenuStringvalue.end();it++){
		if(it!=submenudata.subMenuStringvalue.end()){
			db_msg("debug_zhb----------ShowSubmenu---subMenuStringvalue----- it->c_str = %s  it->second = %d",it->first.c_str(),it->second);
		}
	}
#endif
    submenudata.result_string.clear();
    vector<string>::const_iterator it_;
    for(it_ = data->result_string.begin(); it_ !=  data->result_string.end(); it_++){
        db_msg("debug_zhb----------ShowSubmenu--------result_string = %s",it_->c_str());
        submenudata.result_string.push_back(it_->c_str());
    }
    for(int i=0; i<SETTING_RECOR_NUMBER;i++){//load the item string head
        r->GetString(string(menu_record_win[i].item_name),temp_string);
        submenudata.menuitem_string.push_back(temp_string.c_str());
    }

    height = height+USER_MSG_BASE+menu_statu+1;
    int index = GetMenuConfig(height);
    if(height == SETTING_SCREEN_BRIGHTNESS)
        index = 0;
    submenudata.sub_hilight = index;
    db_msg("debug_zhb----------ShowSubmenu---submenudata.fupdate = %d--height = %d---index = %d",submenudata.fupdate,height,index);
    switch(height)
    {
        case SETTING_DEVICE_DEVICEINFO:
        case SETTING_DEVICE_VERSIONINFO:
        case SETTING_ACCOUNT_BINDING:
            {
                initDeviceInfoItemData(submenudata, data, height);
                menu_items_->FreedBeforeRemoveAll();
                menu_items_->add(submenudata);
            }
            break;
        default:
            {
                menu_items_->FreedBeforeRemoveAll();
                StringVector::iterator it;
                for(it = submenudata.result_string.begin(); it !=  submenudata.result_string.end(); it++)//move the submenu string to the item_string
                {
                    submenudata.item_string = *it;
                    if(height == SETTING_DEVICE_TIME){
                        submenudata.type = TYPE_IMAGE_DATATIME;
                        date_t date;
                        char buf[128]={0};
                        m_TimeSettingObj->getSystemDate(&date);
                        snprintf(buf,sizeof(buf),"%04d-%02d-%02d  %02d:%02d",date.year,date.month,date.day,date.hour,date.minute);
                        submenudata.time_string = buf;
                    }else{
                        submenudata.type = TYPE_IMAGE_SELECT;
                    }
                    submenudata.submenuflag= height;
                    submenudata.submenuflag_second= 1;
                    menu_items_->add(submenudata);
                }
            }
    }
    db_msg("debug_zhb---> showsubmenu end");
}
void SettingWindow::ShowMenuItem()
{
    StringIntMap map_,map_vaule;
    char path[64];
    R *r = R::get();
	if(menu_items_->GetItemData(0) == NULL)
	{
		db_error("data is null");
		return ;
	}
		int hilight_index = menu_items_->GetItemData(0)->value;

	menu_items_->FreedBeforeRemoveAll();

    for(int i=0; i<SETTING_RECOR_NUMBER;i++){
	    ItemData data;
	data.first_icon_path[UNHILIGHTED_ICON] += r->GetImagePath(menu_record_win[i].unhilight_icon);
	data.first_icon_path[HILIGHTED_ICON] += r->GetImagePath(menu_record_win[i].hilight_icon);
	data.second_icon_path[UNHILIGHTED_ICON] += r->GetImagePath(menu_record_win[i].unselect_icon);
	data.second_icon_path[HILIGHTED_ICON] += r->GetImagePath(menu_record_win[i].select_icon);

    for(int i = 0 ; i < THIRD_ICON_NUM ; i++)
        data.third_icon_path[i] += r->GetImagePath(third_icon_[i].icon_name);
    r->GetString(string(menu_record_win[i].item_name), data.item_string);
    r->GetString(string(menu_record_win[i].sub_head), data.subitem_string);
    r->GetString(string(menu_record_win[i].item_tips), data.item_tips);
    data.result_string.clear();
    r->GetStringArray(string(menu_record_win[i].item_name), data.result_string);
    data.value = hilight_index;// save first level menu higlight
    db_msg("debug_zhb----ShowMenuItem----------data.value = %d",data.value);
    data.sub_hilight =GetMenuConfig(GetMsgIdMap(string(menu_record_win[i].item_name)));
    data.type = menu_record_win[i].type;
    data.submenuflag = 0;
    data.submenuflag_second= 0;
    data.result_cnt = data.result_string.size();
    data.subitemcnt = menu_record_win[i].value;
    data.fupdate = 0;
    //db_msg("debug_zhb----------data.subitemcnt = %d",data.subitemcnt);
    InitItemData(data,USER_MSG_BASE +menu_statu+ i+1,map_,map_vaule,data.result_string,data.result_string.size(),data.subitemcnt);
    data.subMenuStringType = map_;
    data.subMenuStringvalue = map_vaule;
    r->GetString("ml_turn_on", data.on_string);
    r->GetString("ml_turn_off", data.off_string);

	   #if 0
	   db_msg("[debug_zhb]----ShowMenuItem:---data.first_icon_path[UNHILIGHTED_ICON] = %s",data.first_icon_path[UNHILIGHTED_ICON].c_str());
	   db_msg("[debug_zhb]----ShowMenuItem:---data.first_icon_path[HILIGHTED_ICON] = %s",data.first_icon_path[HILIGHTED_ICON].c_str());
	   db_msg("[debug_zhb]----ShowMenuItem:---data.second_icon_path[UNHILIGHTED_ICON] = %s",data.second_icon_path[UNHILIGHTED_ICON]);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.second_icon_path[HILIGHTED_ICON] = %s",data.second_icon_path[HILIGHTED_ICON]);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.third_icon_path[UNHILIGHTED_ICON] = %s",data.third_icon_path[UNHILIGHTED_ICON]);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.third_icon_path[HILIGHTED_ICON] = %s",data.third_icon_path[HILIGHTED_ICON]);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.item_string = %s",data.item_string.c_str());
	   vector<string>::const_iterator it;
	  for(it = data.result_string.begin(); it !=  data.result_string.end(); it++)
			db_msg("[debug_zhb]----ShowMenuItem:---data.result_string = %s",it->c_str());
	   db_msg("[debug_zhb]----ShowMenuItem:---data.value = %d",data.value);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.sub_hilight = %d",data.sub_hilight);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.type = %d",data.type);
	   db_msg("[debug_zhb]----ShowMenuItem:---data.result_cnt = %d",data.result_cnt);
	  #endif
	    menu_items_->add(data);
  }
	GetCurrentTime();
}


void SettingWindow::Update(void)
{
    db_msg("[debug_zhb]-----------Update---");
    index_flag = -1;
    string title_str;
    R::get()->GetString("ml_device_deviceinfo", title_str);
    TextView *title = reinterpret_cast<TextView*>(dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    m_TimeSettingObj->updateDialog();
}

int SettingWindow::ShowTimeSettingWindow()
{
    if( m_TimeSettingObj != NULL)
    {
        m_TimeSettingObj->Show();
        m_TimeSettingObj->updateDialog();
    }
    return 0;
}

int SettingWindow::HideDialog()
{
    dialog_->DoHide();
    if( m_TimeSettingObj != NULL)
    {
        m_TimeSettingObj->Hide();
    }
    return 0;
}
void SettingWindow::SystemVersion(bool fset)
{
	::LuaConfig config;
        config.LoadFromFile("/tmp/data/menu_config.lua");
	if(!fset)
	 	version_str = config.GetStringValue("menu.device.sysversion.version");
	else
		config.SetStringValue("menu.device.sysversion.version",version_str);
}
void SettingWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
	db_msg("msg:%d",msg);
    switch ((int)msg)
    {
        case MSG_FORMAT_START:
            m_bIsformatting = true;
            break;

        case MSG_FORMAT_FINISH:
            m_bIsformatting = false;
			m_formart_dialog_display = false;
            break;
		case MSG_CANCLE_DIALG:
			m_formart_dialog_display = false;
			break;
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
			db_msg("debug_zhb----setting_window--------MSG_STORAGE_MOUNTED");
			UpdateSDcardCap();
		}break;
	case MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION:
		db_msg("debug_zhb-----setting------MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION");
		listener_->sendmsg(this,MSG_SYSTEM_UPDATE,0);
		break;
	case MSG_PREVIEW_TO_SETTINGWINDOW_UPDATE_4G_VERSION:
		listener_->sendmsg(this,MSG_4G_UPDATE,0);
		break;
	default:
		break;
    }

    return ;
}
void SettingWindow::DoClearOldOperation()
{
	set_one_shot_timer(1,0,clearOldOperation_timer_id);
}

void SettingWindow::ClearOldOperationTimerProc(union sigval sigval)
{
    SettingWindow *sw = reinterpret_cast<SettingWindow *>(sigval.sival_ptr);
    sw->ClearOldOperation();
}

void SettingWindow::ClearOldOperation()
{
	db_msg(" [zhb]:----keyProc -ClearOldOperation--to preview---begin");
	if(menu_items_->getSubmenuStatusFlag()){
		ForceCloseSettingWindowAllDialog();
		submenuitemkeyproc();//to menu
		usleep(500*1000);
	}

	db_msg(" [zhb]:----keyProc -ClearOldOperation--to preview---finish");
	menu_items_->CancleAllHilight();
	menu_items_->SetHilight(0);
	menu_items_->SetTop();

	listener_->sendmsg(this, MSG_SETTING_TO_PREVIEW, 0);
	listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
	db_msg(" [zhb]:----keyProc -ClearOldOperation--to preview---finish");

}

void SettingWindow::GenBindQRcode(const char *src, int length)
{
    Qrencode *qrcode = new Qrencode();
    qrcode->QrencodeCreat((const unsigned char*)src, length);

    delete qrcode;
    return;
}

void SettingWindow::setLevelData(int msgindex,int msgid,int select)
{
	menu_items_->GetHilightItemRect(&leveldata.rect_);
	leveldata.index_ = GetMenuConfig(msgindex);
	leveldata.msgid_= msgid;
	leveldata.selectitem = select;
	db_msg("[debug_zhb]------x = %d y = %d w = %d h=%d	index = %d",leveldata.rect_.left,leveldata.rect_.top,leveldata.rect_.right-leveldata.rect_.left,leveldata.rect_.bottom-leveldata.rect_.top,leveldata.index_);
}
void SettingWindow::HideLeverWindow()
{
	db_msg("debug_zhb------------MSG_SET_LEVELBAR_CLOSE");
	levelbar_->DoHide();
}
void SettingWindow::setLevelBrightness()
{
	db_msg("debug_zhb------------MSG_SET_LEVELBAR_BRIGHTHNESS");
	changeSubmenuItemLevelValue(leveldata.selectitem,levelbar_->getCurrentIdex());
	listener_->sendmsg(this,leveldata.msgid_,levelbar_->getCurrentIdex());
}



static void copyItemData(ItemData *itemdatadest,ItemData *itemdatasrc)
{
	 R *r = R::get();
	 itemdatadest->first_icon_path[0] +="";
	 itemdatadest->first_icon_path[1] += "";
	 itemdatadest->second_icon_path[0] += "";
	 itemdatadest->second_icon_path[1] +="";
	 for(int i = 0 ; i < THIRD_ICON_NUM ; i++)
	 	itemdatadest->third_icon_path[i] += r->GetImagePath(third_icon_[i].icon_name);
	itemdatadest->item_tips        = itemdatasrc->item_tips;
	itemdatadest->subitem_string        = itemdatasrc->subitem_string;
	itemdatadest->result_string      = itemdatasrc->result_string;
	itemdatadest->menuitem_string= itemdatasrc->menuitem_string;
	itemdatadest->type               = itemdatasrc->type;
	itemdatadest->result_image       = itemdatasrc->result_image;
	itemdatadest->value              = itemdatasrc->value;//menu hilight
	itemdatadest->sub_hilight = itemdatasrc->sub_hilight;//submenu hilight
	itemdatadest->result_cnt         = itemdatasrc->result_cnt;
	itemdatadest->subMenuStringType = itemdatasrc->subMenuStringType;
	itemdatadest->subMenuStringvalue = itemdatasrc->subMenuStringvalue;
	itemdatadest->subitemcnt = itemdatasrc->subitemcnt;
	itemdatadest->submenuflag_second = itemdatasrc->submenuflag_second;
	itemdatadest->dwFlags=LBIF_NORMAL;
	itemdatadest->submenuflag = itemdatasrc->submenuflag;
	itemdatadest->fupdate= itemdatasrc->fupdate;
}

int SettingWindow::changeFirstMenuSwitchValue(ItemData *data,int msg_id)
{
	int select = -1;
	StringIntMap::iterator it;
	it = data->subMenuStringvalue.find(data->result_string[msg_id].c_str());//?ҵ???ǰitem??value
	if(it!=data->subMenuStringvalue.end()){
		//db_error("[debug_zhb]--------- it->second = %d",it->second);
		select = !it->second;
		it->second = !it->second;
	}
	return select;
}

static int changeSubmenuItemSwitchValue(ItemData *data)
{
	int select = -1;
	StringIntMap::iterator it;
	it = data->subMenuStringvalue.find(data->item_string.c_str());//?ҵ???ǰitem??value
	if(it!=data->subMenuStringvalue.end()){
		db_msg("[debug_zhb]--------- it->second = %d",it->second);
		select = !it->second;
		it->second = !it->second;
	}
	return select;
}
static void  changeSubmenuItemChioceValue(ItemData *data,StringVector &loop_string,int index_)
{
	vector<string>::const_iterator it;
	StringIntMap::iterator iter;
	int i = 0;
	  for(it = loop_string.begin(); it !=  loop_string.end(); it++,i++){
		iter = data->subMenuStringvalue.find(it->c_str());//?ҵ???ǰitem??value
		if(iter!=data->subMenuStringvalue.end()){
			if(index_ == i)
				iter->second = 1;
			else
				iter->second = 0;
			db_msg("[debug_zhb]--------it->c_str() = %s index_ = %d i = %d  iter->second = %d",it->c_str(),index_,i,iter->second);
		}
	}
}

int SettingWindow::changeSubmenuItemLevelValue(int selectItem,int index)
{
	ItemData *data  = menu_items_->GetItemData(selectItem);
	if(data == NULL)
	{
		db_error("data is null");
		return -1;
	}
	StringIntMap::iterator it;
	it = data->subMenuStringvalue.find(data->item_string.c_str());//?ҵ???ǰitem??value
	if(it!=data->subMenuStringvalue.end()){
		db_msg("[debug_zhb]--------- it->second = %d",it->second);
		it->second = index;
		return 0;
	}
	return -1;
}


int SettingWindow::HandleOnOffSelectItem(int selectItem,int msg)
{
	ItemData *data  = menu_items_->GetItemData(menu_items_->GetHilight());
	//string str;
	int ret = -1,select_ = -1;
	if(data == NULL)
	{
		db_error("data is null");
		return -1;
	}
	//r->GetString(system_sound_[i].item_name, str);
//	db_error("[debug_jason]:data->item_string.c_str() is = %s",data->item_string.c_str());
	switch(msg)
	{
		case MSG_SET_SYSTEM_SOUND:
			{
			//	db_error("[debug_zhb]---------MSG_SET_SYSTEM_SOUND");
				if(selectItem == 0){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_POWERON_SOUND_SWITCH;
				}else if(selectItem == 1){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_KEY_SOUND_SWITCH;
				}else{
					return 0;
				}

			}
			break;
		case MSG_SET_ADAS_SWITCH:
			{
				if(selectItem == 0){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_FORWARD_COLLISION_WARNING;
				}else if(selectItem == 1){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_LANE_SHIFT_REMINDING;
				}else{
					return 0;
				}
			}
			break;
		case MSG_SET_VIDEO_RESOULATION:
			{
				if(selectItem == 0){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_RECORD_VOLUME;
				}else if(selectItem == 1){
					select_ = changeFirstMenuSwitchValue(data,selectItem);
					//select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_REAR_RECORD_RESOLUTION;
				}else{
					return 0;
				}
			}
			break;
		default:
			break;
	}
	 listener_->sendmsg(this,ret,select_);
	return 0;
}

int SettingWindow::HandleSubmenuSelectItem(int selectItem,int msg,bool fDialog)
{
	EventManager *ev = EventManager::GetInstance();
	int ret = -1,select_ = -1;
	ItemData *data  = menu_items_->GetItemData(selectItem);
	if(data == NULL)
	{
		db_error("data is null");
		return -1;
	}
	ItemData addData;
	R *r = R::get();
	db_msg("[debug_zhb]---------selectItem = %d msg = %d menuIndex = %d",selectItem,msg);
	switch(msg){
		case MSG_SET_AUTO_TIME_SCREENSAVER:
			db_msg("[debug_zhb]---------select_ = %d data->subitemcnt = %d",selectItem,data->subitemcnt);
			if(selectItem >= data->subitemcnt){//????ֻ?п???ѡ??
				db_msg("[debug_zhb]---------select_ = %d current item string = %s ",selectItem,data->item_string.c_str());
				select_ = changeSubmenuItemSwitchValue(data);
				ret = MSG_SET_SCREEN_NOT_DISTURB_MODE;
			}else{
				select_ = selectItem;
			       ret = msg;
			       menu_items_->SetSubHilightValue(select_);//change the submenu hilight
				db_msg("[debug_zhb]---------select_ = %d ret = %d",select_,ret);
				}
			break;
		case MSG_SET_WIFI_SWITCH:
			{
				if(selectItem>= data->subitemcnt){//????ֻ?е???show ????ϸ??Ϣ
					if(selectItem==data->subitemcnt){//?????鿴WiFi infomation item
						db_msg("[debug_zhb]-------MSG_SET_WIFI_SWITCH----handle wifi info item");
						ret = MSG_SHOW_WIFI_INFO;

						}else{//????ɨ????ά??item
						ret = MSG_SHOW_WIFI_APP_DOWNLAOD_LINK;
						db_msg("[debug_zhb]-------MSG_SET_WIFI_SWITCH----handle scan qr code download app  ret = %d ",ret);
						}

				}else{
					select_ = selectItem;
			       		ret = msg;
			       		menu_items_->SetSubHilightValue(select_);//change the submenu hilight
					db_msg("[debug_zhb]----MSG_SET_WIFI_SWITCH-----select_ = %d ret = %d",select_,ret);
				}

			}
			break;
		case MSG_SET_WATCH_DOG_SWITCH:
			{
				db_msg("[debug_zhb]---------menu_items_->GetCount() = %d    data->subitemcnt = %d",menu_items_->GetCount(),data->subitemcnt);
				if(menu_items_->GetCount() > data->subitemcnt+2){
					db_msg("[debug_zhb]---------submenu probe prompt and speed prompt item has been showed");
					if(selectItem == 0 && (data->sub_hilight != selectItem)){//turn off item
							db_msg("[debug_zhb]----handle-----turn off item to delete submenu ");
							addData.submenuflag = data->submenuflag;
							menu_items_->addOrDelete(addData,DELETEITEM+2);//delete item  ??????λ??,Ҳ????data_[2]
							menu_items_->addOrDelete(addData,DELETEITEM+2);
		       					ret = msg;
						}else if(selectItem == 2){//handle probe prompt
								db_msg("[debug_zhb]----handle probe prompt ");
								select_ = changeSubmenuItemSwitchValue(data);
								ret =  MSG_SET_PROBE_PROMPT;
							}else if(selectItem == 3){//handle speed prompt
									db_msg("[debug_zhb]---------handle speed prompt ");
									select_ = changeSubmenuItemSwitchValue(data);
									ret =  MSG_SET_SPEED_PROMPT;
								}else if(selectItem ==4){//handle beep volume
										db_msg("[debug_zhb]-------handle beep volume ");
										setLevelData(SETTING_VOLUME_SELECTION,MSG_SET_VOLUME_SELECTION,selectItem);
										ret =  MSG_SET_VOLUME_LEVELBAR;
									}else if(selectItem ==5){//handle update data
										db_msg("[debug_zhb]--------handle update data ");
										ret =  MSG_SET_UPDATE_DATA;
									}
					if(selectItem <= 1){
						select_ = selectItem;
						menu_items_->SetSubHilightValue(select_);//change the submenu hilight
					}

				}else{
					db_msg("[debug_zhb]---------submenu probe prompt and speed prompt item don't  show");
					if(selectItem == 1 && (data->sub_hilight != selectItem)){//turn off on
						db_msg("[debug_zhb]-----MSG_SET_WATCH_DOG_SWITCH---- check the gps line if insure");
						//??????Ҫ?ж?gpsģ???Ƿ?????
						if(fDialog&& ev->GetGpsStatus()){
							s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_WATCHDOG_OPEN_FAILE);
							s_BulletCollection->ShowButtonDialog();
							return 0;
							}
						db_msg("[debug_zhb]----handle-----turn on item to add submenu ");
						copyItemData(&addData,data);
						for(int i = 0; i < NUM_WATCHDOG ; i++){
							r->GetString(watch_dog_[i].item_name, addData.item_string);
							menu_items_->addOrDelete(addData,INSERTITEM+2+i);//insert item to ??????λ??,Ҳ????data_[2]
						}

						ret = msg;
						}else if(selectItem ==2){//handle beep volume
								db_msg("[debug_zhb]-------handle beep volume ");
								setLevelData(SETTING_VOLUME_SELECTION,MSG_SET_VOLUME_SELECTION,selectItem);
								ret =  MSG_SET_VOLUME_LEVELBAR;
							}else if(selectItem ==3){//handle update data
									db_msg("[debug_zhb]--------handle update data ");
									ret =  MSG_SET_UPDATE_DATA;
								}
					if(selectItem <= 1){
						select_ = selectItem;
						menu_items_->SetSubHilightValue(select_);//change the submenu hilight
					}
				}

			}
			break;
		case MSG_SET_SCREEN_BRIGHTNESS:
			{
				db_msg("[debug_zhb]---------MSG_SET_SCREEN_BRIGHTNESS");
				setLevelData(SETTING_SCREEN_BRIGHTNESS,MSG_SET_SCREEN_BRIGHTNESS,selectItem);
				ret =  MSG_SET_SCREEN_BRIGHTNESS_LEVELBAR;
			}break;
		case MSG_SET_4G_NET_WORK:
			{
				if(menu_items_->GetCount() > data->subitemcnt+1){
					db_msg("[debug_zhb]---------submenu traffic info query and flow recharge item has been showed");
					if(selectItem == 0 && (data->sub_hilight != selectItem)){//turn off item
							db_msg("[debug_zhb]----handle-----turn off item to delete submenu ");
							addData.submenuflag = data->submenuflag;
							menu_items_->addOrDelete(addData,DELETEITEM+2);//delete item  ??????λ??,Ҳ????data_[2]
							menu_items_->addOrDelete(addData,DELETEITEM+2);
		       					ret = msg;
						}else if(selectItem == 2){//handle traffic info query
								db_msg("[debug_zhb]----handle traffic info query ");
								ret =  MSG_SET_4G_TRAFFIC_INFO_QUERY;
							}else if(selectItem == 3){//handle flow recharge
									db_msg("[debug_zhb]---------handle flow recharge ");
									ret =  MSG_SET_4G_FLOW_RECHARGE;
								}else if(selectItem ==4){//handle sim card info
										db_msg("[debug_zhb]---------handle sim card info ");
										ret =  MSG_SET_4G_SIM_CARD_INFO;
									}

				}else{
					db_msg("[debug_zhb]---------submenu traffic info query and flow recharge item don't  show");
					if(selectItem == 1 && (data->sub_hilight != selectItem)){//turn off on
						db_msg("[debug_zhb]----handle-----turn on item to add submenu ");
						copyItemData(&addData,data);
						for(int i = 0; i < NUM_4G_NETWORK ; i++){
							r->GetString(network_4g_[i].item_name, addData.item_string);
							menu_items_->addOrDelete(addData,INSERTITEM+2+i);//insert item to ??????λ??,Ҳ????data_[2]
						}

						ret = msg;
						}else if(selectItem == 2){//handle sim card info
								db_msg("[debug_zhb]---------handle sim card info ");
								ret =  MSG_SET_4G_SIM_CARD_INFO;
							}
				}
				if(selectItem <= 1){
					select_ = selectItem;
					menu_items_->SetSubHilightValue(select_);//change the submenu hilight
					}
			}
			break;
		case MSG_SET_ADAS_SWITCH:
			if(selectItem>= data->subitemcnt){
				select_=updateSubMenuStringValue(&data->subMenuStringvalue,data->item_string);
				db_msg("[debug_zhb]---------select_ = %d current item string = %s ",selectItem,data->item_string.c_str());
				if(selectItem == 2)
					ret = MSG_SET_FORWARD_COLLISION_WARNING;
				else
					ret = MSG_SET_LANE_SHIFT_REMINDING;
			}else{
				db_msg("[debug_zhb]~~~~~~~~~~~~~~~~~ data->sub_hilight = %d data->dwFlags = %d",data->sub_hilight,data->dwFlags&& LBIF_SELECTED);
				if(selectItem == 0 && (data->sub_hilight != selectItem)){
						addData.submenuflag = data->submenuflag;
						menu_items_->addOrDelete(addData,DELETEITEM+2);//delete item  ??????λ??,Ҳ????data_[2]
						menu_items_->addOrDelete(addData,DELETEITEM+2);
					}else if(selectItem == 1 && (data->sub_hilight != selectItem)){
						db_msg("[debug_zhb]-----MSG_SET_ADAS_SWITCH---- check the gps line if insure");
						//??????Ҫ?ж?gpsģ???Ƿ?????
						if(fDialog &&ev->GetGpsStatus()){
							s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_ADAS_OPEN_FAILE);
							s_BulletCollection->ShowButtonDialog();
							return 0;
							}
						db_msg("[debug_zhb]--------- add the item data");
						copyItemData(&addData,data);
						for(int i = 0; i < NUM_ADAS ; i++){
							r->GetString(adas_[i].item_name, addData.item_string);
							menu_items_->addOrDelete(addData,INSERTITEM+2+i);//insert item to ??????λ??,Ҳ????data_[2]
						}
					}
				if(selectItem <= 1){
					select_ = selectItem;
				       ret = msg;
				       menu_items_->SetSubHilightValue(select_);//change the submenu hilight
					db_msg("[debug_zhb]---------select_ = %d ret = %d",select_,ret);
					}
			}

			break;
		case MSG_SET_SYSTEM_SOUND:
			{
				db_msg("[debug_zhb]---------MSG_SET_SYSTEM_SOUND");
				if(selectItem == 0){
					select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_POWERON_SOUND_SWITCH;
				}else{
					select_ = changeSubmenuItemSwitchValue(data);
					ret = MSG_SET_KEY_SOUND_SWITCH;
				}

			}
			break;
		case MSG_SET_PARKING_MONITORY:
			{
				db_msg("[debug_zhb]--------MSG_SET_PARKING_MONITORY    data->subitemcnt = %d",data->subitemcnt);
				if(selectItem>= data->subitemcnt){
						db_msg("[debug_zhb]--------item has been showed");
						ret = MSG_SET_PARKING_RECORD_LOOP_RESOLUTION;
						select_ = selectItem-data->subitemcnt;
						StringVector loop_string;
						loop_string.clear();
			    			r->GetStringArray(packingmonitory_[4].item_name, loop_string);
						for(int i = 0-select_; i < 4-select_; i++){//update all the loop resolution submenu value
							data  = menu_items_->GetItemData(selectItem+i);
							if(data == NULL)
							{
								db_error("data is null");
								return -1;
							}
							changeSubmenuItemChioceValue(data,loop_string,select_);//change the loop resolution item value
						}
				}else{

					if(selectItem == 0){
						ret = MSG_SET_PARKING_WARN_LAMP_SWITCH;
					}else if(selectItem == 1){
							db_msg("[debug_zhb]-----MSG_SET_PARKING_MONITORY---- check ACC sure insert");
							if(fDialog && (!GetMenuConfig(SETTING_PARKING_ABNORMAL_MONITORY_SWITCH))&& ev->GetAccStatus()){//??Ҫ????ACC ?Ƿ???ͨ,?ж??Ƿ?????ͣ??????ģʽ
								s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL);
								s_BulletCollection->ShowButtonDialog();
								return 0;
							}
						ret = MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH;
                    }else if(selectItem == 2){
                        db_msg("[debug_zhb]-----MSG_SET_PARKING_MONITORY---- check ACC sure insert");
                        if(fDialog && (!GetMenuConfig(SETTING_PARKING_ABNORMAL_NOTICE_SWITCH)) &&
                                ev->GetAccStatus()){
                            s_BulletCollection->setButtonDialogCurrentId( BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL);
                            s_BulletCollection->ShowButtonDialog();
                            return 0;
                        }
						ret = MSG_SET_PARKING_ABNORMAL_NOTICE_SWITCH;
					}else if(selectItem == 3 && GetMenuConfig(SETTING_PARKING_RECORD_LOOP_SWITCH)){
						db_msg("[debug_zhb]--------item ready to close");
						addData.submenuflag = data->submenuflag;
						for(int i = 0 ; i < 4 ; i++)
							menu_items_->addOrDelete(addData,DELETEITEM+4);//delete item  ??????λ??,Ҳ????data_[2]
						ret = MSG_SET_PARKING_RECORD_LOOP_SWITCH;
					}else if(selectItem == 3 && (GetMenuConfig(SETTING_PARKING_RECORD_LOOP_SWITCH) == 0)){
							//??Ҫ????ACC ?Ƿ???ͨ,?ж??Ƿ?????ͣ??????ģʽ
							db_msg("[debug_zhb]-----MSG_SET_PARKING_MONITORY---- check ACC sure insert");
							if(fDialog && ev->GetAccStatus()){
								s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL);
								s_BulletCollection->ShowButtonDialog();
								return 0;
								}
							db_msg("[debug_zhb]-----MSG_SET_PARKING_MONITORY---- check the MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH if has been open");
							//??????Ҫ?ж?פ???쳣???ع????Ƿ?????
							if(fDialog  && (GetMenuConfig(SETTING_PARKING_ABNORMAL_MONITORY_SWITCH) ==0)){
								s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN);
								s_BulletCollection->ShowButtonDialog();
								return 0;
								}
							//?ȴ???פ???쳣????
							if(!GetMenuConfig(SETTING_PARKING_ABNORMAL_MONITORY_SWITCH) )
							{
								db_msg("[debug_zhb]-----ready to  the open packing monitory ");
								ItemData *tmpdata  = menu_items_->GetItemData(selectItem-2);
								if(tmpdata == NULL)
								{
									db_error("tmpdata is null");
									return -1;
								}
								select_ = changeSubmenuItemSwitchValue(tmpdata);
								ret = MSG_SET_PARKING_ABNORMAL_MONITORY_SWITCH;
								listener_->sendmsg(this,ret,select_);
								db_msg("[debug_zhb]-----finish the open packing monitory ");
							}
							db_msg("[debug_zhb]--------item ready to show ");
							ret = MSG_SET_PARKING_RECORD_LOOP_SWITCH;
							copyItemData(&addData,data);
							StringVector loop_string;
							loop_string.clear();
			    				r->GetStringArray(packingmonitory_[4].item_name, loop_string);
							vector<string>::const_iterator it;
							int i = 0,val = -1;
							val = GetMenuConfig(packingmonitory_[4].msgid);
							db_msg("[debug_zhb]----SETTING_PARKING_MONITORY-----val = %d",val);
							  for(it = loop_string.begin(); it !=  loop_string.end(); it++,i++){//move the submenu string to the item_string
								addData.item_string = it->c_str();
							  	SetSubMenuStringValueOrType(&addData.subMenuStringvalue,addData.item_string,val==i?1:0);//need to reinit the bind value
								menu_items_->addOrDelete(addData,INSERTITEM+4+i);//insert item to ??????λ??,Ҳ????data_[2]
							    }
						}
					select_ = changeSubmenuItemSwitchValue(data);
				}

            }
            break;
        case MSG_SET_DEVICE_DEVICEINFO:
            {
                db_msg("[debug_zhb]-----MSG_SET_DEVICE_DEVICEINFO----");
                if(fDialog){
                    s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_RESETFACTORY);
                    s_BulletCollection->ShowButtonDialog();
                    return 0;
                }
#if 0
                vector<string> wifi_app_download;
                string info;
                //R::get()->GetString("ml_wifi_app_download_qr_code", title_str);
                char src[128] = {0};
                int length = 0;
                memset(src, 0, sizeof(src));
                Uber_Control::GetInstance()->getUberBindKey(src);
                length = strlen(src);
                if(length == 0)
                {
                    info = "The device get bind code failed! please try again later!";
                    wifi_app_download.push_back(info);
                    this->ShowInfoDialog(wifi_app_download, "No Bind QRCode", MSG_NO_BIND_QR);
                }else
                {
                    GenBindQRcode(src, length);
                    if(access("/data/UberBindimage.png", R_OK) == 0)
                    {
                        info = "";
                        wifi_app_download.push_back(info);
                        GraphicView::LoadImageFromAbsolutePath(dialog_->GetControl("dialog_icon4"), "/data/UberBindimage.png");
                        this->ShowInfoDialog(wifi_app_download, "BIND QRcode Show", MSG_SHOW_BIND_QR);
                        system("rm -f /data/UberBindimage.png");
                    }else
                    {
                        info = "The device get bind code failed ! Please try again later!";
                        wifi_app_download.push_back(info);
                        this->ShowInfoDialog(wifi_app_download, "No Bind QRCode", MSG_NO_BIND_QR);
                    }
                }
#endif
                ret = msg;
                select_ = selectItem;
            }
            break;
        case MSG_SET_DEVICE_SDCARDINFO:
			{
				db_msg("[debug_zhb]-----MSG_SET_DEVICE_SDCARDINFO----");
				#ifdef S_FORMAT
				if(!(StorageManager::GetInstance()->GetStorageStatus() != UMOUNT))
					return 0;
				if(fDialog){
					s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_FORMAT_SDCARD);
					s_BulletCollection->ShowButtonDialog();

					return 0;
				}
				stringstream info_str;
                getSdcardInfo(info_str);
				data->button_string1 = info_str.str().c_str();
				::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//ˢ??????
				#endif
				ret = msg;
				select_ = selectItem;
			}
			break;
		case MSG_SET_DEVICE_VERSIONINFO:
			{
				VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
				DownLoad4GManager *dl4m = DownLoad4GManager::GetInstance();
				WindowManager    *win_mg_ = WindowManager::GetInstance();
				PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				string temp;
				r->GetString(string(version_info_[3].item_name),temp);
				if(strcmp(data->button_string2.c_str(),temp.c_str()) == 0)
				{
					string file_path;
					file_path.clear();
					//p_win->ShowPromptInfo(PROMPT_PREPARE_UPDATE_VERSION, 0);
					if(p_win->get4GVersionDLFlag() && (!p_win->getSystemVersionDLFlag()))
					{
						db_warn("only the 4g module version need to be update");
						dl4m->getDlFile(file_path);
						CheckPrepareUpdateVersion(file_path,dl4m->Md5CheckVersionPacket(true),false,true);

					}else if(p_win->getSystemVersionDLFlag() && (!p_win->get4GVersionDLFlag()))
					{
						db_warn("only the system version need to be update");
						vum->getDlFile(file_path);
						CheckPrepareUpdateVersion(file_path,vum->Md5CheckVersionPacket(true),true,true);

					}else if(p_win->get4GVersionDLFlag() && p_win->getSystemVersionDLFlag())
					{
						db_warn("4g module and system version need to be update");
						if(!m_abnormal_update)
						{
							db_warn("first handle the 4g version update");
							dl4m->getDlFile(file_path);
							CheckPrepareUpdateVersion(file_path,dl4m->Md5CheckVersionPacket(true),false,false);
							m_abnormal_update = true;
						}else{
								db_warn("second handle the 4g version update");
								vum->getDlFile(file_path);
								CheckPrepareUpdateVersion(file_path,vum->Md5CheckVersionPacket(true),true,true);
								m_abnormal_update = false;
							}
					}

					/*
						//first detect the tf and tf version whether exist
						if(StorageManager::GetInstance()->GetStorageStatus() == UMOUNT)
						{
							db_msg("debug_zhb---> no exist the sdcard");
							p_win->ShowPromptInfo(PROMPT_TF_NO_EXIST,2);
							return 0;
						}else {
								string file_path;
								vum->getDlFile(file_path);
								db_warn("debug_zhb---> file_path = %s",file_path.c_str());
								if(access(file_path.c_str(),F_OK) != 0){
									db_warn("debug_zhb---> no exist the version");
									setNewVersionUpdateFlag(false);
									p_win->ShowPromptInfo(PROMPT_VERSION_NO_EXIST,2);
									return 0;
								}else if(access(file_path.c_str(),F_OK) == 0 && (!vum->Md5CheckVersionPacket(true))){//·ÀÖ¹ÈÏÎªÌæ»»µô¹Ì¼þ
									db_warn("debug_zhb--->  exist the version and to check md5sum faile");
									setNewVersionUpdateFlag(false);
									p_win->ShowPromptInfo(PROMPT_VERSION_INVALID,2);
									return 0;
								}else{
										db_warn("debug_zhb---> ready to ota update");
										listener_->sendmsg(this,MSG_SYSTEM_UPDATE,1);//from net_version
									}

							}
					*/
				}
#if 0
				//if fupdate is 0 ,no need to run down
				if(!new_version_flag){
					db_msg("[debug_zhb]--------no detect the new version need to update");
					return 0;
				}else if(new_version_flag && data->fupdate == 0 && !detect_version_manual){
					db_msg("[debug_zhb]------detect_version_manual-- detect the new version need to update");
					stringstream info_str;
					string str;
					char buf[64]={0};
					sprintf(buf,"%0.2f",m_packet_len);
					r->GetString(string(version_info_[0].item_name),str);
					info_str<<str.c_str()<<version_str<<"\n\n";
					r->GetString(string(version_info_[4].item_name),str);
					info_str<<str.c_str()<<buf<<"MB"<<"\n\n";
					data->button_string1 = info_str.str().c_str();
					r->GetString(string(version_info_[3].item_name),str);
					data->button_string2 = str.c_str();
					detect_version_manual = true;
					::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//Ë¢??????
					return 0;
				}
#if 0
				//need to detect 4G network if has been open
				if(fDialog  && (GetMenuConfig(SETTING_4G_NET_WORK) ==0)){
					s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION);
					s_BulletCollection->ShowButtonDialog();
					return 0;
				}
				//need to open the 4G network
				if(!GetMenuConfig(SETTING_4G_NET_WORK)){
					db_msg("[debug_zhb]-----ready to  open the 4G network ");
					ret = MSG_SET_4G_NET_WORK;
					select_ = 1;
					listener_->sendmsg(this,ret,select_);
					db_msg("[debug_zhb]-----finish  to  open the 4G network ");
				}
#endif
				//download the packet
				if(!s_BulletCollection->getFdownload()){
					db_msg("[debug_zhb]-----ready to download the packet ");
					s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION);
					s_BulletCollection->ShowButtonDialog();
					return 0;
				}

				//install the packet by ota
				if(!s_BulletCollection->getFinstall()){
					s_BulletCollection->setFdownload(false);
					s_BulletCollection->setFinstall(false);
					detect_version_manual = false;
					new_version_flag = false;
					listener_->sendmsg(this,MSG_SYSTEM_UPDATE,0);
					return 0;
				}
#if 0
				//install the packet
				if(!finstall){
					db_msg("[debug_zhb]-----ready to install the packet ");
					s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_INSTALL_PACKET_VERSION);
					s_BulletCollection->ShowButtonDialog();
					return 0;
				}
				//finish the install
				if(fdownload && finstall){
					db_msg("[debug_zhb]----- finish the install ");
					fdownload = false;
					finstall = false;
					detect_version_manual = false;
					new_version_flag = false;
					//need to update the menuitem version string
						{
							stringstream info_str;
							string str;
							//data->type = TYPE_IMAGE_STRING_ONLY;
							data->fupdate = new_version_flag;
							for(int i = 0 ; i < NUM_VERSION_INFO-4-1 ; i++){
								r->GetString(string(version_info_[i].item_name),str);
								if(i == 0)
									info_str<<str.c_str()<<version_str<<"\n\n";
								else
									info_str<<str.c_str()<<"\n\n";
							}
							data->button_string1 = info_str.str().c_str();
							r->GetString(string(version_info_[5].item_name),str);
							data->button_string2 =str.c_str();
						}
					SystemVersion(true);
					::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//Ë¢??????
					s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH);
					s_BulletCollection->ShowButtonDialog();
					return 0;
				}
#endif
#endif
				ret = msg;
				select_ = selectItem;
			}
			break;
		case MSG_DEVICE_TIME:
			{
				if(selectItem == 0)
				s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_REFRESH_NETWORK_TIME);
				else
				s_BulletCollection->setButtonDialogCurrentId(BC_BUTTON_DIALOG_MANUAL_UPDATE_TIME);
				ret = msg;
				select_ = selectItem;
			}break;

		default:
			ret = msg;
			select_ = selectItem;
			menu_items_->SetSubHilightValue(select_);//change the submenu hilight
			db_msg("[debug_zhb]-----default----select_ = %d ret = %d",select_,ret);
			break;
		}
	db_msg("[debug_zhb]--------ready to sendmsg");
        listener_->sendmsg(this,ret,select_);
	return 0;
}
void SettingWindow::updataSubmenuItemChoiced(bool fDialog_)
{
	int select_item = 0;
	int menu_val = 0;
	int message = 0;
	int menu_index = 0;
	select_item = menu_items_->GetHilight();//get current submenu item choose
	message = GetNotifyMessage();
	menu_index = GetMenuIndex();
	//if((menu_index >= SETTING_DEVICE_TIME && menu_index < SETTING_DEVICE_LANGUAGE) || (menu_index >= SETTING_DEVICE_DEVICEINFO && menu_index <= SETTING_DEVICE_VERSIONINFO) || menu_index ==SETTING_SCREEN_BRIGHTNESS)
	if((menu_index >= SETTING_DEVICE_TIME && menu_index <= SETTING_SYSTEM_SOUND) || (menu_index >= SETTING_DEVICE_DEVICEINFO && menu_index <= SETTING_DEVICE_VERSIONINFO) || menu_index ==SETTING_SCREEN_BRIGHTNESS)
	{
		//
	}else{
		menu_val = GetMenuConfig(menu_index);
		db_msg("[debug_joasn]: this select_item is %d message is %d  menu_val = %d",select_item,message,menu_val);
		if(menu_val == select_item){
			db_warn("debug_zhb---> current select ==  menu_val  should not to do anything");
			return;
		}
	}
	HandleSubmenuSelectItem(select_item,message,fDialog_);
}
void SettingWindow::submenuitemkeyproc()
{
	MenuToSubmenuSettingButtonStatus(MSG_SET_BUTTON_STATUS);
	 ShowMenuItem();//change to first level menu

}

void SettingWindow::SetDefaultValue(int select_item)
{
      int count =0;
      count = menu_items_->GetCount();
      for(int i= 0; i<count; i++ ){

     ItemData * data = menu_items_->GetItemData(i);
	 if(data == NULL)
 	{
		db_error("data is null");
		return ;
 	}
	  int &value  = data->value;
      value =select_item;
      }
}

void SettingWindow::UpdateTimeString()
{
	date_t date;
	string str;
	 char buf[128]={0};
	 m_TimeSettingObj->getSystemDate(&date);
	 snprintf(buf,sizeof(buf),"%04d-%02d-%02d  %02d:%02d",date.year,date.month,date.day,date.hour,date.minute);
	 str = buf;
	 menu_items_->SetTimeString(str);
}
int SettingWindow::GetNotifyMessage()
{
    int ret = 0;
    switch(GetMenuIndex()){
        case SETTING_RECORD_RESOLUTION:
            ret = MSG_SET_VIDEO_RESOULATION;
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
			ret =MSG_SET_PIC_CONTINOUS;
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
        default:
            ret = -1;
            break;
    }
    return ret;
}
void SettingWindow::GetSystemTimeToMenuItem(int offset)//to submenu
{
        int highlight_index = menu_items_->GetHilight();
        if(highlight_index == -1){
            return;
        }
	//db_msg("debug_zhb-----SETTING_DEVICE_TIME = %d------highlight_index = %d  %d ",SETTING_DEVICE_TIME,highlight_index,USER_MSG_BASE + highlight_index + 1+offset);
	if( SETTING_DEVICE_TIME == (USER_MSG_BASE + highlight_index + 1+offset))
	{
		GetCurrentTime();
	}
}
void SettingWindow::GetCurrentTime()
{
	 R *r = R::get();
	 date_t date;
	 string str,str_;
	 char buf[128]={0};
	 m_TimeSettingObj->getSystemDate(&date);
	 r->GetString("ml_device_time_current",str_);
	 snprintf(buf,sizeof(buf),"%s %04d-%02d-%02d  %02d:%02d",str_.c_str(),date.year,date.month,date.day,date.hour,date.minute);
	 //db_msg("[debug_zhb]----------current time = %s",buf);
	 str = buf;
	 menu_items_->setCurrentTimeStr(str);
}
void SettingWindow::UpdateSDcardCap()
{
	if(m_sdcard_info_ui){
		ItemData *data  = menu_items_->GetItemData(menu_items_->GetHilight());
		if(data == NULL)
		{
			db_error("ItemData data is null");
			return;
		}
		char buf_sn[128] = {0};
		char buf_imei[128] = {0};
		stringstream info_str0;
		stringstream info_str1;
		string str;
		PartitionManager::GetInstance()->sunxi_spinor_private_get("sn",buf_sn,sizeof(buf_sn));
		EventManager *ev = EventManager::GetInstance();
		R::get()->GetString(string(device_info_[0].item_name),str);
		info_str0<<str.c_str()<<buf_sn << "\n";

		R::get()->GetString(string(device_info_[1].item_name),str);
		info_str0<<str.c_str()<< ev->getSimId().c_str()<<"\n";

		getSdcardInfo(info_str1);
		info_str1<<info_str0.str().c_str()<<"\n";
		data->button_string1 = info_str1.str().c_str();

		::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//ˢ??????
	}
}

void SettingWindow::UpdateSDcardCapEx(int p_IndexId)
{
	if(m_sdcard_info_ui)
	{
		ItemData *data  = menu_items_->GetItemData(p_IndexId);
		if(data == NULL)
		{
			db_error("ItemData data is null");
			return;
		}
		char buf_sn[128] = {0};
		char buf_imei[128] = {0};
		stringstream info_str0;
		stringstream info_str1;
		string str;
		PartitionManager::GetInstance()->sunxi_spinor_private_get("sn",buf_sn,sizeof(buf_sn));
		EventManager *ev = EventManager::GetInstance();
		R::get()->GetString(string(device_info_[0].item_name),str);
		info_str0<<str.c_str()<<buf_sn << "\n";

		R::get()->GetString(string(device_info_[1].item_name),str);
		info_str0<<str.c_str()<< ev->getSimId().c_str()<<"\n";

		getSdcardInfo(info_str1);
		info_str1<<info_str0.str().c_str()<<"\n";
		data->button_string1 = info_str1.str().c_str();

		::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//ˢ??????
	}
}
void SettingWindow::AccountBindingCap(int bind_flag)
{
    ItemData *data  = menu_items_->GetItemData(menu_items_->GetHilight());
    if(data == NULL)
    {
        db_error("ItemData data is null");
        return;
    }

    R *r = R::get();
    string str;
    stringstream info_str;
    if(bind_flag == 1)
    {
        r->GetString(string(account_info_[0].item_name), str);
        info_str<<str.c_str()<<"\n";
        r->GetString(account_info_[4].item_name, data->button_string2);
    }else
    {
        r->GetString(string(account_info_[1].item_name), str);
        info_str<<str.c_str()<<"\n";
        r->GetString(string(account_info_[2].item_name), str);
        info_str<<str.c_str()<<"\n";
        r->GetString(account_info_[3].item_name, data->button_string2);
    }
    data->button_string1 = info_str.str().c_str();
    ::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);
}

void SettingWindow::LanuageStringHeadUpdate()
{
	R *r = R::get();
	string str;
	r->GetString(string("ml_device_language_sub_head"),str);
	menu_items_->SetLanuageStr(str);
 	::InvalidateRect(menu_items_->GetHandle(), NULL, TRUE);//ˢ??????
}

void SettingWindow::SetUpdateVersionFlag(int val)
{
	string str1,str2;
	R *r = R::get();
	if(val == 1){
		r->GetString(string(version_info_[3].item_name),str1);
		r->GetString(version_info_[2].item_name, str2);
		menu_items_->SetUpdateVersionFlag(val,str1,str2,TYPE_IMAGE_BUTTON);
	}else{
		r->GetString(string(version_info_[5].item_name),str1);
		r->GetString(version_info_[1].item_name, str2);
		menu_items_->SetUpdateVersionFlag(val,str1,str2,TYPE_IMAGE_STRING_ONLY);
	}
}

void SettingWindow::ForceCloseSettingWindowAllDialog()
{
	if(s_BulletCollection->getButtonDialogShowFlag())//close setting window button dialog
		s_BulletCollection->BCDoHide();
	if(GetButtonDialogShowFlag()){//close the setting window info dialog
		dialog_->DoHideDialog();
		m_show_dialog_ = false;
		}
	if(getLevelBarHandle()->getLevelBarFlag())//close the setting brightness window
		getLevelBarHandle()->HideLevelBar();

}

int SettingWindow::CheckPrepareUpdateVersion(string file_path,bool md5sum_ret,bool f_system,bool f_reset)
{
	PreviewWindow *p_win = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
		//first detect the tf and tf version whether exist
	if(StorageManager::GetInstance()->GetStorageStatus() == UMOUNT)
	{
		p_win->ShowPromptInfo(PROMPT_TF_NO_EXIST,2,true);
		return 0;
	}else {
			if(access(file_path.c_str(),F_OK) != 0){
				if(f_reset)
					setNewVersionUpdateFlag(false);//new_version_flag
				p_win->ShowPromptInfo(PROMPT_VERSION_NO_EXIST,2,true);
				return 0;
			}else if(access(file_path.c_str(),F_OK) == 0 && (!md5sum_ret)){//·ÀÖ¹ÈÏÎªÌæ»»µô¹Ì¼þ
				if(f_reset)
					setNewVersionUpdateFlag(false);//new_version_flag
				p_win->ShowPromptInfo(PROMPT_VERSION_INVALID,2,true);
				return 0;
			}else{
					if(f_system)
						listener_->sendmsg(this,MSG_SYSTEM_UPDATE,1);//from net_version
					else
						listener_->sendmsg(this,MSG_4G_UPDATE,1);//from net_4g_version
				}

		}
	return 0;
}
