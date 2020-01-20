/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file ipc_mode.cpp
 * @brief 双路预览录像presenter
 * @author id:826
 * @version v0.3
 * @date 2016-06-27
 */
#include "window/ipc_mode_window.h"
#include "window/dialog.h"
#include "resource/resource_manager.h"
#include "widgets/button.h"
#include "widgets/text_view.h"
#include "widgets/switch_button.h"
#include "window/window_manager.h"
#include "common/posix_timer.h"
#include "common/message.h"
#include "application.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "IPCModeWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(IPCModeWindow)

int IPCModeWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message ) {
    case MSG_PAINT:
        return HELP_ME_OUT;
    default:
        return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
   }
}

IPCModeWindow::IPCModeWindow(IComponent *parent)
    : SystemWindow(parent)
{
    db_msg(" ");
    Load();

    string bkgnd_bmp = R::get()->GetImagePath("bg");
    db_msg("set back ground pic:%s", bkgnd_bmp.c_str());
    SetWindowBackImage(bkgnd_bmp.c_str());

#if 0 // no launcher window any more, so no need back to launcher
    GraphicView::LoadImage(GetControl("return_btn"), "return_btn1");

    GraphicView* view = reinterpret_cast<GraphicView *>(GetControl("return_btn"));
    view->SetTag(WINDOWID_LAUNCHER);
    view->OnClick.bind(this, &IPCModeWindow::ReturnButtonProc);

    // init dialog
    info_dialog_ = new Dialog(this);
    info_dialog_->SetBackColor(0xFFFFFFFF);

    string title_str;
    R::get()->GetString("exit_dialog", title_str);
    GraphicView *title = reinterpret_cast<TextView*>(info_dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    // confirm button of dialog
    Button *confirm_button = reinterpret_cast<Button*>(info_dialog_->GetControl("confirm_button"));
    confirm_button->SetTag(CONFIRM_BUTTON);
    confirm_button->OnPushed.bind(this, &IPCModeWindow::DialogProc);

    // // cancel button of dialog
    Button *cancel_button = reinterpret_cast<Button*>(info_dialog_->GetControl("cancel_button"));
    cancel_button->SetTag(CANCEL_BUTTON);
    cancel_button->OnPushed.bind(this, &IPCModeWindow::DialogProc);
#endif

    // init dialog
    info_dialog_ = new Dialog(INFO_DIALOG, this);
    info_dialog_->SetBackColor(0xFFFFFFFF);

    string title_str;
    R::get()->GetString("info_title", title_str);
    TextView *title = reinterpret_cast<TextView*>(info_dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    TextView *info_text = static_cast<TextView*>(info_dialog_->GetControl("info_text"));
    info_text->SetPosition(100, 80, 420, 50);

    string info_str;
    R::get()->GetString("exit_info", info_str);
    info_text->SetCaption(info_str.c_str());

    GraphicView::LoadImage(GetControl("gv_shutdown"), "gv_shutdown");
    GraphicView *do_shutdown = reinterpret_cast<GraphicView*>(GetControl("gv_shutdown"));
    do_shutdown->SetTag(IPCMODE_SHUTDOWN_BUTTON);
    do_shutdown->OnClick.bind(this, &IPCModeWindow::GraphicViewButtonProc);

    cam_ctrl0_ = new CameraController(this, 0);
    cam_ctrl1_ = new CameraController(this, 1);

    ::create_timer(this, &dialog_timer_id_, &IPCModeWindow::DialogTimerProc);
}

IPCModeWindow::~IPCModeWindow()
{
    delete info_dialog_;
    info_dialog_ = NULL;
    delete cam_ctrl0_;
    cam_ctrl0_ = NULL;
    delete cam_ctrl1_;
    cam_ctrl1_ = NULL;

    ::delete_timer(dialog_timer_id_);
}

#if 0 // no launcher window any more, so no need handle dialog and back to launcher
void IPCModeWindow::ReturnButtonProc(View *control)
{
    info_dialog_->DoShow();
}

void IPCModeWindow::DialogProc(View *control)
{
    info_dialog_->DoHide();
    switch (control->GetTag()) {
        case CONFIRM_BUTTON: {
            cam_ctrl0_->DialogProc(control);
            cam_ctrl1_->DialogProc(control);

            listener_->notify(this, WM_WINDOW_CHANGE, WINDOWID_LAUNCHER);
        }
            break;
        case CANCEL_BUTTON:
            break;
        default:
            break;
    }
}
#endif

void IPCModeWindow::ShowDialogWithMsg(const char *res_name, uint8_t timeout)
{
    string info_str;
    R::get()->GetString(res_name, info_str);

    TextView *info_text = static_cast<TextView*>(info_dialog_->GetControl("info_text"));
    info_text->SetCaption(info_str.c_str());

    info_dialog_->DoShow();
    ::set_one_shot_timer(timeout, 0, dialog_timer_id_);
}

void IPCModeWindow::DialogTimerProc(union sigval sigval)
{
    static bool flag = true;
    IPCModeWindow *self = reinterpret_cast<IPCModeWindow*>(sigval.sival_ptr);

    self->info_dialog_->DoHide();
}

string IPCModeWindow::GetResourceName()
{
    return string(GetClassName());
}

void IPCModeWindow::Update(MSG_TYPE msg, int p_CamID)
{
    db_msg("handle msg:%d", msg);

    // TODO: 这里还无法区分是哪个recorder发送的消息
    cam_ctrl0_->Update(msg);
    cam_ctrl1_->Update(msg);
}

void IPCModeWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("gv_cam1") || ctrl_name == string("gv_cam0")
        || ctrl_name == string("cam0_switch") || ctrl_name == string("cam1_switch")) {
        ctrl->SetCtrlTransparentStyle(false);
    }

    if (ctrl_name == string("gv_cam0_snap") || ctrl_name == string("gv_cam1_snap")) {
        DWORD style;
        GraphicView *gv = reinterpret_cast<GraphicView *>(ctrl);
        gv->GetOptionStyle(style);
        style &= ~SS_REALSIZEIMAGE;
        gv->SetOptionStyle(style);
    }
}

void IPCModeWindow::SetRtspUrl(uint8_t id, const std::string &url)
{
    if (id == 0) {
        cam_ctrl0_->SetRtspUrl(url);
    } else if (id == 1) {
        cam_ctrl1_->SetRtspUrl(url);
    }
}

void IPCModeWindow::CamAutoCtrl(int id, bool flag)
{
    SwitchButton *cam_switch;
    IPCModeWindow::CameraController *cam_ctrl;

    if (id == 1) {
        cam_switch = reinterpret_cast<SwitchButton *>(GetControl("cam0_switch"));
        cam_ctrl = cam_ctrl0_;
    } else if (id == 0) {
        cam_switch = reinterpret_cast<SwitchButton *>(GetControl("cam1_switch"));
        cam_ctrl = cam_ctrl1_;
    } else {
        db_msg("only cam0, cam1 can be used");
        return;
    }

    if (flag) {
        cam_switch->SetSwitchState(SWITCH_ON);
        cam_ctrl->CamOnProc(cam_switch);
    } else {
        cam_switch->SetSwitchState(SWITCH_OFF);
        cam_ctrl->CamOffProc(cam_switch);
    }
}

void IPCModeWindow::GraphicViewButtonProc(View *control)
{
    int tag = control->GetTag();

    switch (tag) {
        case IPCMODE_SHUTDOWN_BUTTON: {
                string info_str;
                TextView *info_text = static_cast<TextView*>(info_dialog_->GetControl("info_text"));

                R::get()->GetString("exit_info", info_str);
                info_text->SetCaption(info_str.c_str());

                info_dialog_->Show();
                listener_->notify(this, tag, 1);
            }
            break;
        default:
            break;
    }
}

void IPCModeWindow::ShowSnapPic(int id, const std::string &pic_file)
{
    // change id order, because left cam use CAM_A and right cam use CAM_B
    if (id == 1) {
        cam_ctrl0_->ShowSnapPic(pic_file);
    } else if (id == 0) {
        cam_ctrl1_->ShowSnapPic(pic_file);
    }
}

/*=============================================================================================*
 *                            IPCModeWindow::CameraController                                  *
 *=============================================================================================*/
IPCModeWindow::CameraController::CameraController(IPCModeWindow *base, int id)
    : base_(base)
    , hint_flag_(true)
    , id_(id)
{
    if (id == 0) {
        cam_mask_ = reinterpret_cast<GraphicView *>(base_->GetControl("gv_cam0"));
        setting_btn_ = reinterpret_cast<GraphicView *>(base_->GetControl("cam0_set_btn"));
        rec_hint_ = reinterpret_cast<GraphicView *>(base_->GetControl("cam0_hint"));

        takepic_btn_ = reinterpret_cast<GraphicView *>(base_->GetControl("takepic_btn0"));
        cam_switch_ = reinterpret_cast<SwitchButton *>(base_->GetControl("cam0_switch"));
        tv_loading_ = reinterpret_cast<TextView*>(base_->GetControl("tl_loading_0"));
        tv_url_ = reinterpret_cast<TextView*>(base_->GetControl("rtsp_url0"));
        snap_ = reinterpret_cast<GraphicView *>(base_->GetControl("gv_cam0_snap"));

        takepic_btn_->SetTag(IPCMODE_CAM_0_TAKEPIC_BUTTON);
        cam_switch_->SetTag(IPCMODE_CAM_0_BUTTON);
    } else if (id == 1) {
        cam_mask_ = reinterpret_cast<GraphicView *>(base_->GetControl("gv_cam1"));
        setting_btn_ = reinterpret_cast<GraphicView *>(base_->GetControl("cam1_set_btn"));
        rec_hint_ = reinterpret_cast<GraphicView *>(base_->GetControl("cam1_hint"));
        takepic_btn_ = reinterpret_cast<GraphicView *>(base_->GetControl("takepic_btn1"));
        cam_switch_ = reinterpret_cast<SwitchButton *>(base_->GetControl("cam1_switch"));
        tv_loading_ = reinterpret_cast<TextView*>(base_->GetControl("tl_loading_1"));
        tv_url_ = reinterpret_cast<TextView*>(base_->GetControl("rtsp_url1"));
        snap_ = reinterpret_cast<GraphicView *>(base_->GetControl("gv_cam1_snap"));

        takepic_btn_->SetTag(IPCMODE_CAM_1_TAKEPIC_BUTTON);
        cam_switch_->SetTag(IPCMODE_CAM_1_BUTTON);
    }

    GraphicView::LoadImage(cam_mask_, "gv_twocam");
    GraphicView::LoadImage(setting_btn_, "setting_icon");
    GraphicView::LoadImage(takepic_btn_, "takepic_btn");
    GraphicView::LoadImage(rec_hint_, "rec_hint");

    cam_mask_->SetBackColor(0xFFAEC561);
    takepic_btn_->OnClick.bind(this, &IPCModeWindow::CameraController::TakePicProc);
    cam_switch_->OnSwitchOn.bind(this, &IPCModeWindow::CameraController::CamOnProc);
    cam_switch_->OnSwitchOff.bind(this, &IPCModeWindow::CameraController::CamOffProc);
    cam_switch_->SetSwitchImage("switch_on", "switch_off");
    cam_switch_->SetBackColor(0xFFFFFFFF);
    tv_loading_->SetCaptionColor(0xFFFFFFFF);
    tv_loading_->Hide();
    tv_url_->SetCaptionColor(0xFF000000);
    tv_url_->Hide();
    rec_hint_->Hide();

    ::create_timer(this, &rechint_timer_id_, &IPCModeWindow::CameraController::RecHintTimerProc);
    pthread_mutex_init(&proc_lock_, NULL);
}

IPCModeWindow::CameraController::~CameraController()
{
    ::stop_timer(rechint_timer_id_);
    ::delete_timer(rechint_timer_id_);
    pthread_mutex_destroy(&proc_lock_);
}

void IPCModeWindow::CameraController::TakePicProc(View *control)
{
    db_msg("take pic");
    base_->listener_->notify(base_, control->GetTag(), 1);
}

#if 0
void IPCModeWindow::CameraController::DialogProc(View *control)
{
    if (cam_switch_->GetSwitchStatus()) {
        cam_switch_->SetSwitchState(SWITCH_OFF);
        cam_switch_->SetSwitchImage("switch_on", "switch_off");
    }
}
#endif

void IPCModeWindow::CameraController::ShowSnapPic(const std::string &pic_file)
{
    snap_->SetImage(pic_file.c_str());
    snap_->Show();
    snap_->Refresh();
    sleep(1);
    snap_->Hide();
}

void IPCModeWindow::CameraController::CamOnProc(View *control)
{
    pthread_mutex_lock(&proc_lock_);
    DWORD color = 0x00AEC561;
    tv_loading_->Show();
    cam_mask_->SetBackColor(color);
    GraphicView::UnloadImage(cam_mask_);
    ::InvalidateRect(cam_mask_->GetHandle(), NULL, TRUE);
    base_->listener_->sendmsg(base_, control->GetTag(), SWITCH_ON);
    pthread_mutex_unlock(&proc_lock_);
}

void IPCModeWindow::CameraController::CamOffProc(View *control)
{
    pthread_mutex_lock(&proc_lock_);
    DWORD color = 0xFFAEC561;
    tv_loading_->Hide();
    tv_url_->Hide();
    cam_mask_->SetBackColor(color);
    GraphicView::LoadImage(cam_mask_, "gv_twocam");
    ::InvalidateRect(cam_mask_->GetHandle(), NULL, TRUE);
    base_->listener_->sendmsg(base_, control->GetTag(), SWITCH_OFF);
    pthread_mutex_unlock(&proc_lock_);
}

void IPCModeWindow::CameraController::Update(MSG_TYPE msg, int p_CamID)
{
    switch (msg) {
        case MSG_CAMERA_START_PREVIEW:
            break;
        case MSG_CAMERA_STOP_PREVIEW:
            break;
        case MSG_RECORD_START: {
            if (tv_loading_->GetVisible()) {
                db_msg("loading hide");
                tv_loading_->Hide();
            }

            if (cam_switch_->GetSwitchStatus()) {
                tv_url_->SetCaption(rtsp_url_.c_str());
                tv_url_->Show();
            }

            ::set_period_timer(1, 0, rechint_timer_id_);
        }
            break;
        case MSG_RECORD_STOP:
            rec_hint_->Hide();
            ::stop_timer(rechint_timer_id_);

            break;
        case MSG_STORAGE_UMOUNT:
            cam_switch_->SetSwitchState(SWITCH_OFF);
            break;
        case MSG_ETH_SWITCH_DONE:
        case MSG_WIFI_SWITCH_DONE:
        case MSG_SOFTAP_SWITCH_DONE:
            if (cam_switch_->GetSwitchStatus()) {
                tv_url_->Hide();
                tv_url_->SetCaption(rtsp_url_.c_str());
                tv_url_->Show();
            }
            break;
        default:
            break;
    }
}

void IPCModeWindow::CameraController::SetRtspUrl(const std::string &url)
{
    rtsp_url_ = url;
}

void IPCModeWindow::CameraController::RecHintTimerProc(union sigval sigval)
{
    IPCModeWindow::CameraController *self = reinterpret_cast<IPCModeWindow::CameraController *>(sigval.sival_ptr);

    if (self->hint_flag_) {
        self->rec_hint_->Hide();
    } else {
        self->rec_hint_->Show();
    }

    self->hint_flag_ = !self->hint_flag_;
}
