/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: window.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    the base window class.
 History:
*****************************************************************************/

#ifndef _FORM_H_
#define _FORM_H_

#include "widgets/container.h"
#include "widgets/icomponent.h"
#include "type/types.h"
#include "common/observer.h"

#include <mutex>

#define WINDOW_NORMAL   0   //normal mode
#define WINDOW_MODAL    1   //modal mode

class ParserBase;

/* object who has interesting in the window can inheritted this interface */
class WindowListener {
    public:
        WindowListener() {};

        virtual ~WindowListener() {};

        /**
         * @brief 发送异步通知消息, 函数调用立即返回
         * @param form  发送消息的窗体对象指针
         * @param message  消息类型
         * @param val 消息的value
         */
        virtual void notify(class Window *form, int message, int val) = 0;

        /**
         * @brief 发送同步通知消息, 即消息处理完成才会返回
         * @param form  发送消息的窗体对象指针
         * @param message  消息类型
         * @param val 消息的value
         * @return 返回消息处理函数返回值
         */
        virtual int sendmsg(class Window *form, int message, int val) = 0;
        
};

class Window
: public ContainerWidget, public IObserverWrap(Window) {
    DECLARE_DYNCRT_CLASS(Window, Runtime)

    public:
        Window(IComponent *parent);

        virtual ~Window();

        /* show window in normal mode */
        virtual void DoShow();

        /* show window in modal mode*/
        int DoShowModal();

        /* create main window */
        virtual void CreateWidget();

        /* get the resource name "*.ui" */
        virtual std::string GetResourceName();

        /* match View by the widget_name */
        virtual View *GetControl(const char *widget_name);

        virtual Window *GetParentWindow();

        /* set the listener to the window */
        virtual void SetWindowCallback(WindowListener *listener)
        {
            listener_ = listener;
        }

        void SetVisible(bool new_value);

        /* the message looper */
        void run();

        /* default window process function */
        static long int WindowProc(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);

        /* get all the ctrls' map */
        CtrlMap &GetCtrlMap();

        /* get all the textview's text map*/
        StringMap &GetTextMap();

        /* set textviews' caption with text_map_ */
        virtual void FillTextView();

        virtual void OnLanguageChanged();

        /* tell the followers that window's current status */
        NotifyEvent OnCreate;
        NotifyEvent OnClose;
        NotifyEvent OnDestroy;

        virtual void keyProc(int keyCode, int isLongPress);

        class GlobalKeyBlocker {
            public:
                GlobalKeyBlocker() {
                    std::lock_guard<std::mutex> lock(global_key_mutex_);
                    db_debug("global key blocked");
                    global_key_blocked_ = true;
                }
                ~GlobalKeyBlocker() {
                    std::lock_guard<std::mutex> lock(global_key_mutex_);
                    db_debug("global key release");
                    global_key_blocked_ = false;
                }
                static bool IsGlobalKeyIgnored() {
                    std::lock_guard<std::mutex> lock(global_key_mutex_);
                    return global_key_blocked_;
                }
            private:
                static std::mutex global_key_mutex_;
                static bool global_key_blocked_;
        };

        std::string wname;
        static std::mutex key_proc_mutex_;
    protected:
        /* observer notify the window to update */
        virtual void Update(MSG_TYPE msg, int p_CamId, int p_recordId);

        /* load the resource requirements */
        virtual void Load();

        virtual void LoadRes(ParserBase *resParse);

        virtual void LoadResComplete();

        /* get the parser */
        virtual ParserBase *GetResParse();

        /* include all the ctrls in the window */
        CtrlMap ctrl_map_;

        /* the map stores all the textview's text */
        //@where MainParser::generate
        StringMap text_map_;

        IComponent *parent_;

        /* call SetWindowCallback to set listener */
        WindowListener *listener_;

        bool isKeyUp;

    private:
        int mode_;  //indicate the window diaplay mode
};

class SystemWindow
: public Window {
    DECLARE_DYNCRT_CLASS(SystemWindow, Runtime)

    public:
        SystemWindow(IComponent *parent);

        virtual ~SystemWindow();

        virtual void GetCreateParams(CommonCreateParams &Params);

        /* set window background image with bmp*/
        virtual void SetWindowBackImage(const char *bmp);

        virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

    protected:
        bool bkimg_used_;

    private:
        pthread_mutex_t bmp_lock_;
        BITMAP *win_bkgnd_bmp_;
};

#endif //_FORM_H_
