/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: menu_items.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/
#include "widgets.h"
#include "type/types.h"
#include <vector>
#include "common/setting_menu_id.h"

#define HILIGHT_IMAGE_NUM   4

#define ITEMNUMBER SETTING_RECOR_NUMBER
typedef struct __sub_first_icon_patch
{
    const char * sub_first_icon_w;
    const char * sub_first_icon;
}sub_first_icon_patch;
#ifdef SETTING_ITEM_DD
static sub_first_icon_patch sub_first_icon_patch_[]= {
	//this index must reference to	IPCLinuxPlatform/custom_aw/apps/sdv/source/common/setting_menu_id.h:SETTING_ITEM_DD_ID index

    {"/usr/share/minigui/res/images/set_front_resolution_g.png", 		"/usr/share/minigui/res/images/set_front_resolution.png"},
//    {"/usr/share/minigui/res/images/set_screen_brightness_g.png",      "/usr/share/minigui/res/images/set_screen_brightness.png"},
    {"/usr/share/minigui/res/images/set_auto_screensaver_g.png",       "/usr/share/minigui/res/images/set_auto_screensaver.png"},
    {"/usr/share/minigui/res/images/set_system_voice_g.png", 		"/usr/share/minigui/res/images/set_system_voice.png"},  	
    {"/usr/share/minigui/res/images/set_adas_w.png",  				"/usr/share/minigui/res/images/set_adas.png"},
    {"/usr/share/minigui/res/images/set_account_w.png",                 "/usr/share/minigui/res/images/set_account.png"},
    {"/usr/share/minigui/res/images/set_device_info_g.png", 			"/usr/share/minigui/res/images/set_device_info.png"},
    //  {"/usr/share/minigui/res/images/set_sdcard_g.png", 				"/usr/share/minigui/res/images/set_sdcard.png"},
    {"/usr/share/minigui/res/images/set_language_g.png", 			"/usr/share/minigui/res/images/set_language.png"},
    {"/usr/share/minigui/res/images/set_version_g.png", 			"/usr/share/minigui/res/images/set_version.png"},
    {"/usr/share/minigui/res/images/set_wifi_g.png", 				"/usr/share/minigui/res/images/set_wifi.png"},
    {"/usr/share/minigui/res/images/set_update_time_g.png",				"/usr/share/minigui/res/images/set_update_time.png"},
    {"/usr/share/minigui/res/images/set_system_acc_g.png",			"/usr/share/minigui/res/images/set_system_acc.png"},

};
#else
static sub_first_icon_patch sub_first_icon_patch_[]= {
    {"/usr/share/minigui/res/images/set_front_resolution_w.png", 		"/usr/share/minigui/res/images/set_front_resolution.png"},
    {"/usr/share/minigui/res/images/set_rear_resolution_w.png", 		"/usr/share/minigui/res/images/set_rear_resolution.png"},
    {"/usr/share/minigui/res/images/set_mic_w.png", 				"/usr/share/minigui/res/images/set_mic.png"},
	{"/usr/share/minigui/res/images/set_data_watermark_w.png", 		"/usr/share/minigui/res/images/set_data_watermark.png"},
	{"/usr/share/minigui/res/images/set_screen_brightness_w.png",     "/usr/share/minigui/res/images/set_screen_brightness.png"},
	{"/usr/share/minigui/res/images/set_auto_screensaver_w.png",      "/usr/share/minigui/res/images/set_auto_screensaver.png"},
	{"/usr/share/minigui/res/images/set_emer_record_w.png", 		"/usr/share/minigui/res/images/set_emer_record.png"},
	{"/usr/share/minigui/res/images/set_voice_photo_w.png", 		"/usr/share/minigui/res/images/set_voice_photo.png"},
	{"/usr/share/minigui/res/images/set_wifi_w.png", 				"/usr/share/minigui/res/images/set_wifi.png"},
	{"/usr/share/minigui/res/images/set_4g_network_w.png",	       		"/usr/share/minigui/res/images/set_4g_network.png"},
	{"/usr/share/minigui/res/images/set_update_time_w.png",	       "/usr/share/minigui/res/images/set_update_time.png"},
    	{"/usr/share/minigui/res/images/set_adas_w.png",  				"/usr/share/minigui/res/images/set_adas.png"},
    	{"/usr/share/minigui/res/images/set_watchdog_w.png", 			"/usr/share/minigui/res/images/set_watchdog.png"},
    	{"/usr/share/minigui/res/images/set_packing_monitory_w.png", 	"/usr/share/minigui/res/images/set_packing_monitory.png"},
    	{"/usr/share/minigui/res/images/set_standby_clock_w.png", 		"/usr/share/minigui/res/images/set_standby_clock.png"},
    	{"/usr/share/minigui/res/images/set_system_voice_w.png", 		"/usr/share/minigui/res/images/set_system_voice.png"},
    	{"/usr/share/minigui/res/images/set_driver_report_w.png", 		"/usr/share/minigui/res/images/set_driver_report.png"},
    	{"/usr/share/minigui/res/images/set_device_info_w.png", 			"/usr/share/minigui/res/images/set_device_info.png"},
    	{"/usr/share/minigui/res/images/set_sdcard_w.png", 			"/usr/share/minigui/res/images/set_sdcard.png"},
    	{"/usr/share/minigui/res/images/set_version_w.png", 			"/usr/share/minigui/res/images/set_version.png"},
};

#endif
enum {
    UNHILIGHTED_ICON = 0,
    HILIGHTED_ICON = 1,
};

typedef enum
{
    TYPE_STRING =0,
    TYPE_NULL,
    TYPE_IMAGE,
    TYPE_IMAGE_SELECT,
    TYPE_IMAGE_STRING,//add by zhb
    TYPE_IMAGE_BUTTON,
    TYPE_IMAGE_DATATIME,//6
    TYPE_IMAGE_STRING_ONLY,//7
    TYPE_IMAGE_NEED_BIND_BUTTON,
}SubmenuType;
typedef enum
{
    TYPE_SUBITEM_CHOICE=0,
    TYPE_SUBITEM_SWITCH,//the same level item type is switch
    TYPE_SUBITEM_ENLARGE,//the same level item type is view
    TYPE_SUBITEM_SET,//the same level  item type is set
    TYPE_SUBITEM_UPDATE,//the same level  item type is update
    TYPE_SUBMENU_SUBITEM_SWITCH,//the subme  item type is switch
    TYPE_SUBMENU_SUBITEM_ENLARGE,////the subme  item type is view
    TYPE_SUBMENU_SUBITEM_CHOICE,
}SubItemType;

class ItemData
{
public:
    ItemData();
    ~ItemData();
    std::string first_icon_path[2];
    BITMAP *first_icon[2];//menu icon
    std::string second_icon_path[2];
    BITMAP *second_icon[2];//menu_submenu icon
    std::string third_icon_path[14];
    BITMAP *third_icon[14];//submenu icon
    std::string item_string;//menu head string
    std::string subitem_string;//submenu head string
    StringVector result_string;//submenu string verctor
    StringVector menuitem_string;//menu string verctor
    BITMAP *result_image;
    int type;//the type of paint
    DWORD dwFlags;
    int value;//hilight flag
    int result_cnt;//the number of submenu string arry
    int submenuflag;//msg id 
    int submenuflag_second;//flag is the submenu status 
    int sub_hilight;
    StringIntMap subMenuStringType;
    StringIntMap subMenuStringvalue;
    int subitemcnt;//submenu realy cnt
    std::string on_string;//string on
    std::string off_string;//string off
    std::string button_string1; //button string
    std::string button_string2;
    std::string time_string;//time string
    std::string item_tips;//tips string
    int fupdate;//if has detected new version can be update
    bool devIsNeedBind;
};

class MenuItems : public CustomWidget
{
    DECLARE_DYNCRT_CLASS(MenuItems, Runtime)
public:
	MenuItems(View *parent);
	virtual ~MenuItems();
	virtual void GetCreateParams(CommonCreateParams &params);
	virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
	virtual void add(ItemData &data);
	void remove(unsigned int pos);
	void removeAll();
	BITMAP* AllocImage(const char *image_path);
	void FreeImage(BITMAP *data);
	int GetHilight();
	void SetHilight(int value);
	int GetCount();
	int SetTop();
	int CancleAllHilight();
	NotifyEvent OnItemClick;
	ItemData *GetItemData(int item_index);
	void SetWindowBackImage(const char *bmp);
	void SetQuitFlag(bool flag);
	bool getSubmenuStatusFlag();
	void SetSubHilightValue(int value);
	void addOrDelete(ItemData &data,int dex);
	void changeItemData(ItemData *itemdatadest,ItemData *itemdatasrc);
	void GetHilightItemRect(RECT * rect);
	void SaveHilightItemRect(RECT & rect);
	void SetTimeString(const std::string& time_string);
	void setLevelStr(std::string & str){level_str = str;}
	std::string getLevelStr(){return level_str;}
	void setCurrentTimeStr(std::string & str){current_time = str;}
	std::string getCurrentTimeStr(){return current_time;}
	void SetLanuageStr(const std::string & str);
	void FreedBeforeRemoveAll();
	void SetUpdateVersionFlag(int val,const std::string str,const std::string str2,int type);
	bool getAddDataFlag(){return m_add_data_finish;}
	bool getPaintSecondLevelFlag(){return m_paint_second_;}
	void setPaintSecondLevelFlag(bool flag){m_paint_second_ = flag;}
	void setSwitchValue(int value);
	int getSwitchValue();
	int setNeedPrint(bool Print_flag);
	int setSubMenuItemPos(int value);
protected:
	void GetItemsRect (int start, int end, RECT* prc);
	void InvalidateItem (HWND hwnd, int pos);
	bool InvalidateUnderMultiItem (HWND hwnd, int start, int end);
	bool InvalidateUnderItem (HWND hwnd, int pos);
	void CalcParams (const RECT* rcClient);
	void DrawItems (HWND hwnd, HDC hdc, int width);
	int SelectItem (HWND hwnd, unsigned int newSel);
	int CancleSelected (HWND hwnd);
	void DrawFocusRect (HWND hwnd, HDC hdc);
	void ChangeScrollbar (HWND hwnd, BOOL bShow, int iSBar);
	void SetVScrollbar (HWND hwnd, BOOL fRedraw);
	void SetHScrollbar (HWND hwnd, BOOL fRedraw);
	void SetScrollbar(HWND hwnd, BOOL fRedraw);
	void GetSubmenuRect (RECT* prc,bool submenuflag);
	void DrawFirstLevelMenu(ItemData * data,HDC hdc,int width,int offset);
	void DrawSecondLevelMenu(ItemData *data,HDC hdc, int width,int offset);
	int getSubMenuStringType(ItemData *data,std::string &str);
	int getSubMenuStringValue(ItemData *data,std::string &str);
    
    int first_MenuItem_Message(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
    int second_MenuItem_Message(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
	int GetFirstHightItem();
	

public:
    int main_item;
	int subSwitch_value;
	bool need_print_flag;
private:
    BITMAP* bg_image_;
    BITMAP* hilight_image_[HILIGHT_IMAGE_NUM];//menu hilight and unhilight image
    BITMAP* subhilight_image_[HILIGHT_IMAGE_NUM];//menu hilight and unhilight image
    BITMAP* submenu_first_image_w[ITEMNUMBER];
    BITMAP* submenu_first_image_[ITEMNUMBER];
    BITMAP* button_image_[HILIGHT_IMAGE_NUM];
	BITMAP* frist_bg_image_;
    DWORD dwFlags_;          // listbox flags
    int left_;           // display left of item
    int width_;          // display width of item
    int max_width_;       // max width of all items
    int count_;          // items count
    int top_;            // start display item
    int visible_cnt_;       // number of visible items
    int hilighted_idx_;      // current hilighted item(submenu or menu)
    int height_;         // item height
    int last_selected_idx_; // record last selected item when use LBS_MOUSEFOLLOW style
    SBPolicyType scrollbar_policy_;  // scrollbar policy type
    PLOGFONT lst_font;       // logic font for bold display
    std::vector<ItemData*> data_;
    int icon_restore_idx_;
    bool fling_flag_;
    int index_flag;
    bool exit_flag;
    bool submenu_status;//submenu or menu status
    int offset_submenu,offset_menu;
    RECT rect_;//remember the hilightitem rect
    int s_width,s_height;
    std::string level_str;
    std::string current_time;//save current time string
    bool m_add_data_finish;
    bool m_paint_second_;
	int first_ts_current_id;
};
