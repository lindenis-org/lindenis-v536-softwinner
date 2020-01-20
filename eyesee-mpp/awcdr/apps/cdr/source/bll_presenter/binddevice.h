/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file binddevice.h
 * @brief ????¡±?presenter
 * @author id:826
 * @version v0.3
 * @date 2016-08-01
 */
#pragma once

#include "common/subject.h"
#include "common/observer.h"
#include "bll_presenter/presenter.h"
#include "bll_presenter/gui_presenter_base.h"
#include "window/window_manager.h"

#include <map>

class BindWindow;

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup RecPlay
 * @{
 */

class Window;
class BindPresenter
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(BindPresenter)
    , public IObserverWrap(BindPresenter)
{
    public:
        
        BindPresenter();
        ~BindPresenter();

        void OnWindowLoaded();
        void OnWindowDetached();
        int HandleGUIMessage(int msg, int val);
        void BindGUIWindow(::Window *win);
        int DeviceModelInit();
        int DeviceModelDeInit();
        void PrepareExit() {}
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        int RemoteSwitchRecord(){return 0;};
        int RemoteTakePhoto(){return 0;};
        int GetRemotePhotoInitState(bool & init_state){return 0;};
        int RemoteSwitchSlowRecord(){return 0;};
    private:
        int status_;
        pthread_mutex_t model_lock_;
        WindowManager *win_mg_;
}; /* class BindPresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
