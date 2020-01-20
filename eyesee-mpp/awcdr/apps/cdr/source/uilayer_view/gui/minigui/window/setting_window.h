/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: menu_window.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _MENU_WINDOW_H_
#define _MENU_WINDOW_H_

#include "window/window.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "widgets/text_view.h"


#define SETTING_BUTTON_DIALOG       (USER_MSG_BASE+100)
#define SETTING_LEVELBAR_HIDE   (USER_MSG_BASE+101)
#define SETTING_TIME_UPDATE_HIDE   (USER_MSG_BASE+102)
#define SETTING_LEVELBAR_BRIGHTNESS (USER_MSG_BASE+103)
#define SETTING_BUTTON_DIALOG_INFO      (USER_MSG_BASE+104)

enum{
	SET_BUTTON_UPDOWN_SHOW = 0,
	SET_BUTTON_UPDOWN_HIDE,
	SET_BUTTON_LEFTRIGHT_SHOW,
	SET_BUTTON_UP_DOWN_CHOICE_HIDE,
	SET_BUTTON_UP_DOWN_CHOICE_SHOW,
};

enum{
	BUTTON_DIALOG_REBOOT = 0,//
	BUTTON_DIALOG_REFRESH_NETWORK_TIME,//
	BUTTON_DIALOG_MANUAL_UPDATE_TIME,//
	BUTTON_DIALOG_RESETFACTORY,//
	BUTTON_DIALOG_FORMAT_SDCARD,//
	BUTTON_DIALOG_4G_NETWORK_OPEN_VERSION,//
	BUTTON_DIALOG_4G_NETWORK_OPEN_WATCHDOG,//
	BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN,//
	BUTTON_DIALOG_PACKING_RECORD_LOOP_OPEN_FAIL,//
	BUTTON_DIALOG_ADAS_OPEN_FAILE,//
	BUTTON_DIALOG_WATCHDOG_OPEN_FAILE,//
	BUTTON_DIALOG_DOWNLOAD_PACKET_VERSION,//ing
	BUTTON_DIALOG_DOWNLOAD_PACKET_WATCHDOG,//ing
	BUTTON_DIALOG_INSTALL_PACKET_VERSION,//
	BUTTON_DIALOG_INSTALL_PACKET_WATCHDOG,//
	BUTTON_DIALOG_INSTALL_CURRENT_CITY_PACKET_FINISH,//
	BUTTON_DIALOG_INSTALL_NATIONAL_CITY_PACKET_FINISH,//
	BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_NETWORK_ARNORMAL,//
	BUTTON_DIALOG_DATA_DOWNLOAD_FAIL_INSUFFICIENT_FLOW,//
	BUTTON_DIALOG_INSTALL_CURRENT_VERSION_PACKET_FINISH,//
};

typedef struct {
	const char *info_text;
	const char *info_title;
	const char *button_left;//ok /open_now /retry / recharge_now/know
	const char *button_right;//cancel
}button_dialog_item;

//Here the arry initalization sequence is consistent with the above enum definition order
static button_dialog_item button_dialog_item_[]={//
	{"ml_confirm_reboot","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_refresh_network_time","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_manual_update_time","","ml_confirm_ok","ml_confirm_no"},
	{"ml_device_resetselection","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_format_sdcard","","ml_confirm_ok","ml_confirm_no"},
	{"ml_button_dialog_4g_network_open","","ml_button_dialog_open_now","ml_confirm_no"},//version update need open the 4g
	{"ml_button_dialog_4g_network_open","","ml_button_dialog_open_now","ml_confirm_no"},//watchdog update packet need open the 4g
	{"ml_button_dialog_record_loop_open","","ml_button_dialog_open_now","ml_confirm_no"},
	{"ml_button_dialog_record_loop_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_adas_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_watchdog_open_fail","ml_button_dialog_open_fail","ml_button_dialog_know",""},
	{"ml_button_dialog_download_packet_version","","ml_button_dialog_retry","ml_confirm_no"},//version packet download
	{"ml_button_dialog_download_packet_watchdog","","ml_button_dialog_retry","ml_confirm_no"},//watchdog packet download
	{"ml_button_dialog_install_packet_version","","ml_confirm_no",""},//install the version packet
	{"ml_button_dialog_install_packet_watchdog","","ml_confirm_no",""},//install watchdog version packet
	{"ml_button_dialog_install_current_city_packet_finished","","ml_button_dialog_know",""},
	{"ml_button_dialog_install_national_city_packet_finished","","ml_button_dialog_know",""},
	{"ml_button_dialog_download_packet_fail_network_abnormal","","ml_button_dialog_retry","ml_confirm_no"},
	{"ml_button_dialog_download_packet_fail_insufficient_flow","","ml_button_dialog_recharge_now","ml_confirm_no"},
	{"ml_button_dialog_install_version_finish","","ml_button_dialog_know",""},
};

typedef struct {
    RECT rect_;//the item want to showlevelbar current rect
    int index_;//the item want to showlevelbar current index
    int msgid_;//the item want to showlevelbar current msgid
    int selectitem;//for showlevelbar
}leveldata_t;


class WindowManager;
class MenuItems;
class ItemData;
class Dialog;
class Button;
class GraphicView;
class MagicBlock;
class TimeSettingWindow;
class LevelBar;
class PromptBox;
class BulletCollection;

class SettingWindow : public SystemWindow {
    DECLARE_DYNCRT_CLASS(SettingWindow, Runtime)

    public:
        SettingWindow(IComponent *parent);
        virtual ~SettingWindow();
        std::string GetResourceName();
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        void InitMenuItem(const char *item_name,const char *sub_head,const char *item_tips,
                const char *unhilight_icon, const char *hilight_icon,const char *unselect_icon, const char *select_icon,
                int type,int subcnt, int value,int msgid);
        void InitMenuItem(const menu_win &menu, int index,int msgid);

        void MenuItemClickProc(View *control);
        void DialogClickProc(View *control);
        void UpdateItemString(int index, const std::string &name);
        void ShowFirmwareDownloadDialog();
        void ShowInfoDialog(const std::vector<std::string> &info, const std::string &tilteStr,int msgid);
        void HideInfoDialog();
        void ShowSubmenu(int height,const std::vector<std::string> &info);
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        void keyProc(int keyCode, int isLongPress);
        void MenultemkeyEventProc();
        void SetIndexToSubmenu(int index);
        int GetMenuIndex();
        void InitMenuWinGview(int menustatu);
        void InitMenuWinStatu(int menustatu);
        void DeInitMenuItem(int height);
        void DeInitMenuWinStatu(int menustatu);
        int   GetMenuConfig(int msg);
        void Update();
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        void ResetUpdate();
        void ShowSettingWindow();
        void HideSettingWindow();
        int   HandlerSettingWindow(int keyCode,int isLongPress);
        void SettingWindowButtonClickProc(View *control);
        int  ShowTimeSettingWindow();
        int  HideDialog();
        void SystemVersion(bool fset);
        void ShowListItem();//add by zhb
        void ShowListItemWindow(bool value);
        MenuItems * GetMenuItemsHandle(){return menu_items_;}
        int GetMenu_statu(){ return menu_statu;}
        void SetQuitFlag(bool flag);
        void InitGetMsgIdMap();
        int GetMsgIdMap(std::string mstring);
        //submenu handle
        void submenuitemkeyproc();
        void SetDefaultValue(int select_item);
        int GetNotifyMessage();
        void ShowMenuItem();
        void InitItemData(ItemData &data,int msgid,StringIntMap &_map,StringIntMap &value_map,StringVector &resultString,int len,int rsubcnt);
        void updataSubmenuItemChoiced(bool fDialog_);
        int HandleSubmenuSelectItem(int selectItem,int msg,bool fDialog);
        void ShowLevelBar(int levelbar_id);
        void GenBindQRcode(const char *src, int length);
        void setLevelData(int msgindex,int msgid,int select);
        void HideLeverWindow();
        void setLevelBrightness();
        int changeSubmenuItemLevelValue(int selectItem,int index);
        void initDeviceInfoItemData(ItemData &infodata,ItemData*data,int msgid);
        void initInfoDialog();
        void UpdateTimeString();
        void MenuToSubmenuSettingButtonStatus(int msgid);
        void ShowPromptBox();
        void GetSystemTimeToMenuItem(int offset);
        void UpdateSDcardCap();// window is in setting_window when tf in or out ,should update the cap string
        void UpdateSDcardCapEx(int p_IndexId);
        void AccountBindingCap(int bing_flag);
        void LanuageStringHeadUpdate();
        std::string getVersionStr(){return version_str;}
        void BcMsgSend(bool m_buttonDialog,int val);
        void GetCurrentTime();
        void ClearOldOperation();
        void DoClearOldOperation();
        static void ClearOldOperationTimerProc(union sigval sigval);
        static void HandleUpdate_download_Process(union sigval sigval);
        bool GetButtonDialogShowFlag(){return m_show_dialog_;}
        void setNewVersionUpdateFlag(bool flag){new_version_flag = flag;}
        void setVersionPacketLen(double flag){m_packet_len= flag;}
        void SetUpdateVersionFlag(int val);
        LevelBar* getLevelBarHandle(){return levelbar_;}
        void ForceCloseSettingWindowAllDialog();
	    int CheckPrepareUpdateVersion(std::string file_path,bool md5sum_ret,bool f_system,bool f_reset);
		int HandleOnOffSelectItem(int selectItem,int msg);
		int changeFirstMenuSwitchValue(ItemData *data,int msg_id);
		int setSubMenuDefaultItem(int p_nIndex);
		int HandleFristSubMenuItemEvent(int x,int y);
		int clcSettingWindowMenuItemPos(int x,int y);
		void updateAccountBindStatus(int p_index);
        void updateVersionFirmwareInfo(int p_nIndex);
        //void getSdcardInfo(stringstream &info_str);
        int getDownloadProcess();
        void setNewVersionFlag(bool flag);
        void HideUpdateProceccInfo(bool is_fail);
        int GetDownloadStatus();
        int SetDownloadStatus(int status);
        bool IsDownloadStatusReady();
        bool getSystemVersionDLFlag();
        void updateItemData();
        void account_bind_status_update();
    private:
        MenuItems *menu_items_;
        WindowManager *win_mg_;
        //info show
        Dialog *dialog_;
        TextView *dialog_title_;
        TextView *dialog_info_text;
        bool m_show_dialog_;
        //button dialog
        Dialog *button_dialog_;
        GraphicView *button_dialog_title;
        TextView *button_dialog_text;
        Button *button_dialog_confirm;
        Button *button_dialog_cancel;
        Button *button_dialog_progress_bar_background;
        Button *button_dialog_progress_bar_top_color;
        int button_dialog_current_id_;
        std::vector<std::string> item_list_;
        int index_flag;//msg id
        int menuIndex_;
        int menu_statu;
        int sw_flag;
        int m_nhightLightIndex;
        MagicBlock *magic_block_;
        TimeSettingWindow *m_TimeSettingObj;
        bool m_bIsformatting;
        StringIntMap getMsgId_map_;
        LevelBar * levelbar_;
        leveldata_t leveldata;//remember the item rect and index
        std::string version_str;
        PromptBox * s_PromptBox_;
        bool account_bind_flag_;
        bool new_version_flag;
        bool detect_version_manual;//manually detect the version if has new
        bool m_sdcard_info_ui;//second level and is sdcard info ui show
        timer_t timer_download_process_;
        timer_t clearOldOperation_timer_id;
        double m_packet_len;//MB
        int first_hight_id;
		bool m_abnormal_update;
		bool m_formart_dialog_display;
    public:
        BulletCollection * s_BulletCollection;
        pthread_mutex_t setwindow_proc_lock_;
        int installer_time_;
};

#endif //_MENU_WINDOW_H_
