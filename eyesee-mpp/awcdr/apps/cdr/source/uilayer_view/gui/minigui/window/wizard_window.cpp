/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    wizard_window.cpp
 * @brief   开机向导界面
 * @author  id:826
 * @version v0.3
 * @date    2016-11-01
 */

#include "window/wizard_window.h"
#include "debug/app_log.h"
#include "common/utils/utils.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "widgets/view_container.h"
#include "widgets/button.h"
#include "widgets/menu_items.h"
#include "resource/resource_manager.h"
#include "tools/wizard.h"
#include "window/user_msg.h"
#include "window/dialog.h"

#include "lua/lua_config_parser.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "WizardWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(WizardWindow)

void WizardWindow::InitMenuItem(const char *item_name,
                        const char *unhilight_icon, const char *hilight_icon,
                        int type, int value)
{
    char path[64];
    R *r = R::get();

    ItemData data;
    data.first_icon_path[UNHILIGHTED_ICON] += r->GetImagePath(unhilight_icon);

    data.first_icon_path[HILIGHTED_ICON] += r->GetImagePath(hilight_icon);

    r->GetString(string(item_name), data.item_string);

    data.result_string.clear();
    r->GetStringArray(string(item_name), data.result_string);

    data.value = value;
    data.type = type;

    if (type == TYPE_STRING)
        data.result_cnt = data.result_string.size();
    else if (type == TYPE_IMAGE)
        data.result_cnt = 2;

    list_view_->add(data);
}

int WizardWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        default:
            return SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
    }
}

WizardWindow::WizardWindow(IComponent *parent)
    : SystemWindow(parent)
    , list_view_(NULL)
    , finish_dialog_(NULL)
    , format_dialog_(NULL)
    , info_dialog_(NULL)
{
    wname = "WizardWindow";
    Load();

    string bkgnd_bmp = R::get()->GetImagePath("bg");
    db_msg("set back ground pic: %s", bkgnd_bmp.c_str());
    SetWindowBackImage(bkgnd_bmp.c_str());

    list_view_ = reinterpret_cast<MenuItems *> (GetControl("list_view"));
    list_view_->OnItemClick.bind(this, &WizardWindow::MenuItemClickProc);

    string menu_list_bmp = R::get()->GetImagePath("bg");
    db_msg("set back ground pic:%s", menu_list_bmp.c_str());
    list_view_->SetWindowBackImage(menu_list_bmp.c_str());
    list_view_->Hide();

    LuaConfig config;
    const char *config_file = "/tmp/data/wizard_config.lua";
    if (!FILE_EXIST(config_file)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", config_file);
        system("cp -f /usr/share/app/sdv/wizard_config.lua /tmp/data/");
    }

    int ret = config.LoadFromFile(config_file);
    if (ret < 0) {
        db_warn("Load %s failed, copy backup and try again", config_file);
        ret = config.LoadFromFile(config_file);
        if (ret < 0) {
            db_error("Load %s failed!", config_file);
        }
    }

    InitMenuItem("lv_tutk","info_icon", "info_icon", TYPE_STRING, config.GetBoolValue("wizard.features.tutk"));
    InitMenuItem("lv_rtsp","info_icon", "info_icon", TYPE_STRING, config.GetBoolValue("wizard.features.rtsp"));
    InitMenuItem("lv_onvif","info_icon", "info_icon", TYPE_STRING, config.GetBoolValue("wizard.features.onvif"));

    //init shortcut window UI pic
    GraphicView::LoadImage(GetControl("gv_onecam"), "gv_twocam");
    GraphicView::LoadImage(GetControl("gv_dualcam"), "gv_mosaic");
    GraphicView::LoadImage(GetControl("gv_shutdown"), "gv_shutdown");
    GraphicView::LoadImage(GetControl("gv_disk"), "gv_disk");

    TextView *text_view;
    text_view = reinterpret_cast<TextView *>(GetControl("tl_title"));
    text_view->SetBackColor(0xFF000000);
    text_view->SetCaptionColor(0xFFFFFFFF);

    text_view = reinterpret_cast<TextView *>(GetControl("tl_onecam"));
    text_view->SetCaptionColor(0xFFC0C0C0);

    text_view = reinterpret_cast<TextView *>(GetControl("tl_dualcam"));
    text_view->SetCaptionColor(0xFFC0C0C0);

    GetControl("tl_info")->SetCaption("模式选择");

    GraphicView *view;

    view = reinterpret_cast<GraphicView *>(GetControl("gv_onecam"));
    view->SetTag(ONECAM_MODE);
    view->OnClick.bind(this, &WizardWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_dualcam"));
    view->SetTag(DUALCAM_MODE);
    view->OnClick.bind(this, &WizardWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_disk"));
    view->SetTag(FORMAT_DISK);
    view->OnClick.bind(this, &WizardWindow::ViewClickProc);
    view->Hide();

    view = reinterpret_cast<GraphicView *>(GetControl("go_left"));
    view->SetTag(LAST_PAGE);
    view->OnClick.bind(this, &WizardWindow::ViewClickProc);
    view->SetCaption("上一页");
    view->Hide();

    view = reinterpret_cast<GraphicView *>(GetControl("go_right"));
    view->SetTag(NEXT_PAGE);
    view->SetCaption("下一页");
    view->OnClick.bind(this, &WizardWindow::ViewClickProc);

    view = reinterpret_cast<GraphicView *>(GetControl("gv_shutdown"));
    view->OnClick.bind(this, &WizardWindow::ShutdownProc);

    // init dialog
    finish_dialog_ = new Dialog(SELECTION_DIALOG, this);
    finish_dialog_->SetBackColor(0xFFFFFFFF);

    string title_str;
    R::get()->GetString("exit_wizard", title_str);
    TextView *title = reinterpret_cast<TextView*>(finish_dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    // confirm button of dialog
    Button *confirm_button = reinterpret_cast<Button*>(finish_dialog_->GetControl("confirm_button"));
    confirm_button->SetTag(CONFIRM_BUTTON);
    confirm_button->OnPushed.bind(this, &WizardWindow::ExitDialogProc);

    // // cancel button of dialog
    Button *cancel_button = reinterpret_cast<Button*>(finish_dialog_->GetControl("cancel_button"));
    cancel_button->SetTag(CANCEL_BUTTON);
    cancel_button->OnPushed.bind(this, &WizardWindow::ExitDialogProc);

    format_dialog_ = new Dialog(SELECTION_DIALOG, this);
    format_dialog_->SetBackColor(0xFFFFFFFF);

    R::get()->GetString("confirm_format", title_str);
    title = reinterpret_cast<TextView*>(format_dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    // confirm button of dialog
    confirm_button = reinterpret_cast<Button*>(format_dialog_->GetControl("confirm_button"));
    confirm_button->SetTag(CONFIRM_BUTTON);
    confirm_button->OnPushed.bind(this, &WizardWindow::FormatDialogProc);

    // // cancel button of dialog
    cancel_button = reinterpret_cast<Button*>(format_dialog_->GetControl("cancel_button"));
    cancel_button->SetTag(CANCEL_BUTTON);
    cancel_button->OnPushed.bind(this, &WizardWindow::FormatDialogProc);

    info_dialog_ = new Dialog(INFO_DIALOG, this);
    info_dialog_->SetBackColor(0xFFFFFFFF);

    R::get()->GetString("info_title", title_str);
    title = reinterpret_cast<TextView*>(info_dialog_->GetControl("dialog_title"));
    title->SetCaption(title_str.c_str());
    title->SetBackColor(0xFF999999); // 灰色

    TextView *info_text = static_cast<TextView*>(info_dialog_->GetControl("info_text"));
    info_text->SetPosition(60, 40, 400, 250);
}

WizardWindow::~WizardWindow()
{
    delete finish_dialog_;
    delete format_dialog_;
    delete info_dialog_;
}

void WizardWindow::MenuItemClickProc(View *control)
{
    MenuItems *menu_item = (MenuItems *)control;
    static int last_index = 0;

    int highlight_index = menu_item->GetHilight();
    int handle_id = highlight_index + 1;

    int &value = menu_item->GetItemData(highlight_index)->value;

    if (highlight_index - last_index != 0) {
        last_index = highlight_index;
        return;
    }

    if ( ++value == menu_item->GetItemData(highlight_index)->result_cnt ) {
        value = 0;//return to first
    }

    db_msg("click %d, value: %d", highlight_index, value);
    listener_->notify(this, (FEATURE_BASE + handle_id), value);

    last_index = highlight_index;
}

void WizardWindow::ViewClickProc(View *control)
{
    int tag = control->GetTag();
    db_msg("%d clicked!", tag);
    static int page_index = 0;

    switch (tag) {
        case ONECAM_MODE: {
            TextView *text_view;
            text_view = reinterpret_cast<TextView *>(GetControl("tl_onecam"));
            text_view->SetCaptionColor(0xFF000000);
            text_view->Refresh();

            text_view = reinterpret_cast<TextView *>(GetControl("tl_dualcam"));
            text_view->SetCaptionColor(0xFFC0C0C0);
            text_view->Refresh();
        }
            break;
        case DUALCAM_MODE: {
            TextView *text_view;
            text_view = reinterpret_cast<TextView *>(GetControl("tl_onecam"));
            text_view->SetCaptionColor(0xFFC0C0C0);
            text_view->Refresh();

            text_view = reinterpret_cast<TextView *>(GetControl("tl_dualcam"));
            text_view->SetCaptionColor(0xFF000000);
            text_view->Refresh();
        }
            break;
        case FORMAT_DISK:
            format_dialog_->DoShow();
            return;
        case LAST_PAGE: {
            page_index--;
            db_msg("go to page: %d", page_index);
            if (page_index == 0) {
                // hide page 1
                GetControl("tl_info")->SetCaption("模式选择");
                GetControl("list_view")->Hide();

                // show page 0
                GetControl("gv_onecam")->Show();
                GetControl("gv_dualcam")->Show();
                GetControl("tl_onecam")->Show();
                GetControl("tl_dualcam")->Show();

                // update foot
                GetControl("go_right")->SetCaption("下一页");
                GetControl("go_right")->Show();
                GetControl("go_left")->SetCaption("上一页");
                GetControl("go_left")->Hide();
            } else if (page_index == 1) {
                // hide page 1
                GetControl("gv_disk")->Hide();

                // show page 1
                GetControl("tl_info")->SetCaption("功能选择");
                GetControl("list_view")->Show();

                // update foot
                GetControl("go_right")->SetCaption("下一页");
                GetControl("go_right")->Show();
                GetControl("go_left")->SetCaption("上一页");
                GetControl("go_left")->Show();
            }
        }
            break;
        case NEXT_PAGE: {
            page_index++;
            db_msg("go to page: %d", page_index);
            if (page_index == 1) {
                // hide page 0
                GetControl("gv_onecam")->Hide();
                GetControl("gv_dualcam")->Hide();
                GetControl("tl_onecam")->Hide();
                GetControl("tl_dualcam")->Hide();

                // show page 1
                GetControl("tl_info")->SetCaption("功能选择");
                GetControl("list_view")->Show();

                // update foot
                GetControl("go_right")->SetCaption("下一页");
                GetControl("go_right")->Show();
                GetControl("go_left")->SetCaption("上一页");
                GetControl("go_left")->Show();
            } else if (page_index == 2) {
                // hide page 1
                GetControl("tl_info")->SetCaption("格式化SD卡");
                GetControl("list_view")->Hide();

                // show page 2
                GetControl("gv_disk")->Show();

                // update foot
                GetControl("go_right")->SetCaption("完成");
                GetControl("go_right")->Show();
                GetControl("go_left")->SetCaption("上一页");
                GetControl("go_left")->Show();
            } else if (page_index == 3) {
                page_index--;
                finish_dialog_->Show();
            }
        }
            break;
        default:
            break;
    }

    listener_->notify(this, tag, 1);
}

void WizardWindow::ShutdownProc(View *control)
{
    // TODO: let presenter to sync config and stop background job
    system("poweroff");
}

void WizardWindow::ExitDialogProc(View *control)
{
    switch (control->GetTag()) {
        case CONFIRM_BUTTON: {
            db_info("finish config");
            finish_dialog_->DoHide();
            listener_->notify(this, CONFIRM_EXIT, 1);
        }
            break;
        case CANCEL_BUTTON:
            //listener_->sendmsg(this, CANCEL_EXIT, 1);
            break;
        default:
            break;
    }

    finish_dialog_->DoHide();
}

void WizardWindow::FormatDialogProc(View *control)
{
    switch (control->GetTag()) {
        case CONFIRM_BUTTON: {
            format_dialog_->DoHide();

            listener_->notify(this, FORMAT_DISK, 1);

        }
            break;
        case CANCEL_BUTTON:
            break;
        default:
            break;
    }

    format_dialog_->DoHide();
}

void WizardWindow::FormatDoneCallback(int status)
{
      string info_str;

      TextView *info_text = static_cast<TextView*>(info_dialog_->GetControl("info_text"));
      info_text->SetPosition(150, 80, 400, 250);

      info_dialog_->DoShow();

      if (status == 0) {
          R::get()->GetString("format_success", info_str);
      } else {
          R::get()->GetString("format_failed", info_str);
      }

      info_text->SetCaption(info_str.c_str());

      sleep(2);

      info_dialog_->DoHide();
}

string WizardWindow::GetResourceName()
{
    return string(GetClassName());
}

void WizardWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("tl_title")) {
        ctrl->SetCtrlTransparentStyle(false);
    }
}
