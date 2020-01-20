/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    status_bar.h
 * @brief   状态栏窗口presenter
 * @author  id:690
 * @version v0.3
 * @date    2017-01-17
 */

#pragma once

#include "common/subject.h"
#include "bll_presenter/presenter.h"
#include "bll_presenter/gui_presenter_base.h"

class StatusBarBottomWindow;
class WindowManager;

namespace EyeseeLinux {

class StatusBarBottomPresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(StatusBarBottomPresenter)
    , public IObserverWrap(StatusBarBottomPresenter)
{
    public:
        StatusBarBottomPresenter();

        virtual ~StatusBarBottomPresenter();

        int DeviceModelInit();

        int DeviceModelDeInit();

        void PrepareExit() {};

        int RemoteSwitchRecord(){return 0;};

        int RemoteTakePhoto(){return 0;};
        int GetRemotePhotoInitState(bool & init_state){return 0;};
        int RemoteSwitchSlowRecord(){return 0;};

        void OnWindowLoaded();

        void OnWindowDetached();

        int HandleGUIMessage(int msg, int val, int id=0);

        void BindGUIWindow(::Window *win);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    private:
        WindowManager *win_mg_;
        StatusBarBottomWindow *sb_win_;
}; // class StatusBarPresenter

} // namespace EyeseeLinux

