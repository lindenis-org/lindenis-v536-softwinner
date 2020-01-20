/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: application.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-18
 Description:
    The root of the app, which dispatches the messages to the window.
 History:
*****************************************************************************/

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "data/gui.h"
#include "data/fast_delegate.h"
#include "data/fast_delegate_bind.h"
#include "widgets/icomponent.h"
#include "type/types.h"

#include <functional>

class Window;

class Application : public IComponent
{
public:
    Application();
    virtual ~Application();

    /* message looper to get message from the desktop */
    virtual void Run();
    virtual void Terminate();

    /* when message loop is running will call this callback */
    void RegistCallback(std::function<void(void)> func);

    /* get the unique app instance */
    static Application* GetApp();

    /* delete the only app instance */
    static void DeleteApp();

    /* the default window process to get the message sended by the desktop */
    static int WindowProc(HWND hwindow, int message, WPARAM wparam,
                                LPARAM lparam);

    /* initialize the mutex to assure generating instance safely */
    static void InitApplication();
    /* destroy the mutex */
    static void UninitApplication();

    /* create one vacuous window */
    void CreateHandle();
    void DestroyHandle();
    HWND GetHandle();

    /* handle all the messages if there is no existed window @reversed */
    int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                            LPARAM lparam);

    /* set the current modal window, which will receive the messages from
        the root app @reversed */
    Window* SetCurrentWindow(Window* form);
    Window* GetCurrentWindow();

    /* get the resource files path */
    std::string GetAppUIPath();
    void SetAppUIPath(std::string path);

protected:
    HWND handle_;   //the vacuous window's handle
    Window* current_form_;  //current modal window
private:
    static pthread_mutex_t mutex_;  //to assure generating the unique instance
    static Application* instance_;  //the unique instance
    std::string ui_path_; //the path of the resource files
    std::function<void(void)> callback_;
    bool is_running_;
};


/* Auxiliary class, initializes the instance mutex and
 * notifies some events.
 */
class AppObject
{
public:
    AppObject()
    {
        Application::InitApplication();
    }
    ~AppObject()
    {
        Application::UninitApplication();
    }
    fastdelegate::FastDelegate2<Application*, DWORD, int>   OnKeyPress;
    fastdelegate::FastDelegate1<Application*> OnCreate;
    fastdelegate::FastDelegate1<Application *> OnDestroy;
};

#endif //_APPLICATION_H_
