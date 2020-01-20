/* *******************************************************************************
 * Copyright (c), 2017-2027, Sunchip Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    setting_handler.h
 * @brief   设置控制窗口presenter
 * @author  id:fangjj
 * @version v1.0
 * @date    2017-04-21
 */

#pragma once

#include "common/subject.h"
#include "bll_presenter/presenter.h"
#include "bll_presenter/gui_presenter_base.h"

class SettingHandlerWindow;
class WindowManager;

namespace EyeseeLinux {

class SettingHandlerPresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(SettingHandlerPresenter)
    , public IObserverWrap(SettingHandlerPresenter)
{
    public:
        SettingHandlerPresenter();

        virtual ~SettingHandlerPresenter();

        int DeviceModelInit();

        int DeviceModelDeInit();

        void PrepareExit() {};

        int RemoteSwitchRecord(){return 0;};

        int RemoteTakePhoto(){return 0;};
        int GetRemotePhotoInitState(bool & init_state){return 0;};
        int RemoteSwitchSlowRecord(){return 0;};

        void OnWindowLoaded();

        void OnWindowDetached();

        int HandleGUIMessage(int msg, int val);

        void BindGUIWindow(::Window *win);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    private:
        WindowManager *win_mg_;
        SettingHandlerWindow *shw_win_;
}; // class SettingHandlerPresenter

} // namespace EyeseeLinux

