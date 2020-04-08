/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview_window.h
 * @brief 单路预览录像窗口
 * @author id:826
 * @version v0.3
 * @date 2016-11-04
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "widgets/text_view.h"
#include <time.h>
#include <signal.h>

#define FIRST_COL_W  70
#define THIRD_COL_W 210
#define FOURTH_COL_W 80
#define SECOND_COL_W (GUI_SCN_WIDTH - FIRST_COL_W -\
						THIRD_COL_W -FOURTH_COL_W )

//预留32后面的宽度

typedef enum{
    FIRST_COL = 0,//image
    SECOND_COL,//str
    THIRD_COL,//str
    FOURTH_COL,//image
};

#define LISTVIEW_ITEM_H 100

class ListView;
class WindowManager;
class Dialog;
class Button;
class GraphicView;
class MagicBlock;
class TimeSettingWindowNew;
class PromptBox;
class Sublist;
class InfoDialog;
class BulletCollection;
class PreviewWindow;    
class CaridSettingWindow;

class NewSettingWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(NewSettingWindow, Runtime)

    public:

        NewSettingWindow(IComponent *parent);

        virtual ~NewSettingWindow();

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
        
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();

        void ListViewClickProc();

		void PingpangListViewItem(int index, bool update);
		
        void subListViewClickProc(View *control);
		
		void subListViewClickProcOKButton();

        void ButtonClickProc(View *control);

        void keyProc(int keyCode, int isLongPress);

        void OnLanguageChanged();
        
        BITMAP* AllocListViewImage(const char *image_path);
        
        void FreeListViewImage(BITMAP *data);
        
        int GetMenuConfig(int msg);
        
        void SetMenuConfig(int msg,int val);
        
        void InitListViewItem();
        
        void InitListViewItem(const ListViewItem &list,int index_);
        
        int InitListViewItem(const char *first_icon_path, const char *first_text,
            const int  type, const char *second_icon_path0, const char *second_icon_path1,int index_);

        void ShowSubList(int index_msg);

        int GetNotifyMessage(int msgid);

        int ShowTimeSettingWindow();

        void ShowDeviceInfoDialog();

        int ShowFormatScardDialog();

        void showResetFactoryDialog();

        void DateTimeSettingConfirm(View *view, int value);

        void ResetUpdate();
        std::string getVersionStr(){return version_str;}
        void SystemVersion(bool fset);

        void ForceCloseSettingWindowAllDialog();

		void SetListViewItem(int index, int update);
		void SetListViewItemEx(int index, int update, int type);
			
		int GetPosInlistview_item_ids(int index);

		int ShowCaridSettingWindow();

		void CaridSettingConfirm(View *view, int value);

		void InitListviewbmpMap();
		BITMAP* GetListviewbmpMap(std::string &strkey);
		void ReleaseListviewbmpMap();
		
private:
        void getSdcardInfo(std::stringstream &info_str);
        void UpdateSDcardCap();
public:

        BulletCollection *s_BulletCollection;

private:
    ListView *list_view_,*sub_list_view_;
    WindowManager    *win_mg_;
    Sublist *sub_list_;
    R* r;
    int m_main_row_index_;
    TextView* m_list_view_item_title;
    TimeSettingWindowNew *m_TimeSetting;
	CaridSettingWindow *m_carid_window_;
    InfoDialog *m_info_dialog_;
    GraphicView *return_button_view;
    std::string str_menu;
    std::vector<LVCOLUMN> columns;
    std::string version_str;
	DWORD listview_top_bg;
	DWORD listview_bg;
	DWORD listview_first_str_color;
	DWORD listview_second_str_color;
	bool firstinit;		// 避免第一次调用InitListViewItem时previewwindow还没创建的问题
	std::map<std::string, BITMAP*> listviewbmp;
};
