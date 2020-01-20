/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ipc_mode.h
 * @brief 双路预览录像presenter
 * @author id:826
 * @version v0.3
 * @date 2016-06-27
 */
#pragma once


#include "window/window.h"
#include "window/user_msg.h"

#include <signal.h>
#include <string>

/**
 * 定义每个窗体内部的button/msg
 */
#define IPCMODE_CAM_0_BUTTON         (USER_MSG_BASE+1)
#define IPCMODE_CAM_1_BUTTON         (USER_MSG_BASE+2)
#define IPCMODE_CAM_0_TAKEPIC_BUTTON (USER_MSG_BASE+3)
#define IPCMODE_CAM_1_TAKEPIC_BUTTON (USER_MSG_BASE+4)
#define IPCMODE_SETTING_BUTTON       (USER_MSG_BASE+5)
#define IPCMODE_SHUTDOWN_BUTTON      (USER_MSG_BASE+6)

class Dialog;
class GraphicView;
class TextView;
class SwitchButton;

class IPCModeWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(IPCModeWindow, Runtime)
public:
    IPCModeWindow(IComponent *parent);
    virtual ~IPCModeWindow();
    std::string GetResourceName();

    //void ReturnButtonProc(View *control);
    //void DialogProc(View *control);

    void Update(MSG_TYPE msg, int p_CamID=0);

    int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
    void PreInitCtrl(View *ctrl, std::string &ctrl_name);

    // be called by presenter
    void SetRtspUrl(uint8_t id, const std::string &url);

    void CamAutoCtrl(int id, bool flag);

    void ShowSnapPic(int id, const std::string &pic_file);

    void ShowDialogWithMsg(const char *res_name, uint8_t timeout);

    void GraphicViewButtonProc(View *control);

    static void DialogTimerProc(union sigval sigval);

private:
    class CameraController : public EyeseeLinux::IObserver {
    public:
        CameraController(IPCModeWindow *base, int id);

        ~CameraController();

        //void DialogProc(View *control);
        void TakePicProc(View *control);
        void CamOnProc(View *control);
        void CamOffProc(View *control);
        void SetRtspUrl(const std::string &url);
        void ShowSnapPic(const std::string &pic_file);
        void Update(MSG_TYPE msg, int p_CamID=0);

        static void RecHintTimerProc(union sigval sigval);
    private:
        IPCModeWindow *base_;
        GraphicView *cam_mask_;
        GraphicView *takepic_btn_;
        GraphicView *setting_btn_;
        GraphicView *rec_hint_;
        GraphicView *snap_;
        SwitchButton *cam_switch_;
        TextView *tv_loading_;
        TextView *tv_url_;
        std::string rtsp_url_;

        timer_t rechint_timer_id_;
        pthread_mutex_t proc_lock_;
        bool hint_flag_;
        int id_;
    };

    Dialog *info_dialog_;
    CameraController *cam_ctrl0_;
    CameraController *cam_ctrl1_;
    timer_t dialog_timer_id_;
};
