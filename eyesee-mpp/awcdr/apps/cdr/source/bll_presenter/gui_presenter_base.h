/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file gui_presenter_base.h
 * @brief gui的presenter基类
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#pragma once

class Window;

class IGUIPresenter
{
public:
    virtual ~IGUIPresenter(){}
    virtual void OnWindowLoaded()=0;
    virtual void OnWindowDetached()=0;
    virtual void OnUILoaded()=0;
    virtual int HandleGUIMessage(int msg, int val,int id=0)=0;
    virtual void BindGUIWindow(::Window *win)=0;
    virtual void NotifyAll()=0;
    virtual void sendUsbConnectMessage()=0;
};

class GUIPresenterBase : public IGUIPresenter
{
public:
    virtual ~GUIPresenterBase(){};

    /** 窗口加载 */
    virtual void OnWindowLoaded(){};

    /** 窗口隐藏 */
    virtual void OnWindowDetached(){};

    virtual void OnUILoaded(){};

    /**
     * @brief 按钮被按下
     * @param msg 绑定窗口传递的message
     */
    virtual int HandleGUIMessage(int msg, int val,int id=0) { return 0; };

    /** 绑定window到presenter */
    virtual void BindGUIWindow(::Window *win){};

    virtual void NotifyAll(){};

    virtual void sendUsbConnectMessage() {};
};

