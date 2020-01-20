/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: window_manager.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _WINDOW_MANAGER_H_
#define _WINDOW_MANAGER_H_

#include "window/window.h"
#include "type/types.h"
#include "common/observer.h"
#include "common/singleton.h"

#include "bll_presenter/gui_presenter_base.h"

#include <mutex>
#include <map>
#include <time.h>
#include <signal.h>

typedef enum {
   WINDOWID_INVALID =0,
    //WINDOWID_LAUNCHER = 1,
    WINDOWID_BINDING = 1,
    WINDOWID_SETTING,
    WINDOWID_PREVIEW,			// 3*
    WINDOWID_PLAYBACK,			// 4*
    
    WINDOWID_STATUSBAR_MIDDLE,	// 5*
    WINDOWID_STATUSBAR,			// 6*
    WINDOWID_STATUSBAR_BOTTOM,	// 7*
    WINDOWID_SETTING_HANDLER,
    WINDOWID_SETTING_NEW,		// 9*
    WINDOWID_SETTING_LISTBOX_VIEW,
    WINDOWID_IPCMODE,
    
} WindowID;


enum {
    STATU_PREVIEW = 0,
    STATU_PHOTO,
    STATU_SLOWRECOD,
    STATU_PLAYBACK,
    STATU_SETTING,
    STATU_BINDING,
    STATU_DELAYRECIRD,
};

enum PhotoingMode {
    MODE_PIC_NORMAL = 0,
    MODE_PIC_TIME,		
    MODE_PIC_AUTO,
    MODE_PIC_CONTINUE,
    MODE_EMPTY = -1,
};


class R;
class Application;

class WindowManager
    : public WindowListener
    , public EyeseeLinux::Singleton<WindowManager>
{
    friend class EyeseeLinux::Singleton<WindowManager>;
    public:
        void MsgLoop();
        void Init(WindowID entry_win);
        void notify(Window *form, int msg, int val);
        int sendmsg(Window *form, int msg, int val);
        Window* CreateWindowById(int window_id);
        Window* GetWindow(int window_id);
        void ChangeWindow(int src_window_id, int dst_window_id,bool preview_init = false);
        void SetGUIPresenter(const std::map<WindowID, IGUIPresenter*> &win_presenter_map);
        const std::map<WindowID, IGUIPresenter*> GetGuiPresenter();

        // close and destruct all window resource
        void DoExit();
        WindowID GetCurrentWinID();

        inline bool IsUILoaded() const { return ui_loaded_; }

        void ResetConfigLua();
        void sendUsbConnectMessage();

        // NOTE: 该接口会在所有窗口处理自己的按键事件之前调用,
        // 所以这里面禁止进行任何耗时的操作
        int CommonKeyProc(int win_id, int msg, int val);
	void DetectSdcardSpeedAndVersion();
	
		bool isWindowChanging;		/* 0=idle 1=changing */
		bool GetisWindowChanging() {return isWindowChanging;}
    private:
        R *r_;
        WindowMap window_map_;
        std::map<WindowID, IGUIPresenter*> win_presenter_map_;
        Application *app_;
        WindowID entry_win_;
        WindowID current_winid;
        std::mutex post_msg_mutex_;
        bool ui_loaded_;
	timer_t timer_id_;
        WindowManager();
        WindowManager(const WindowManager &o);
        WindowManager &operator=(const WindowManager &o);
        ~WindowManager() {};

        static void HandlePostMessage(Window *form, int msg, int val, WindowManager *self);
};

#endif //_WINDOW_MANAGER_H_
