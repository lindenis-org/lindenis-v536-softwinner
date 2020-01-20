/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    wizard.cpp
 * @brief   开机向导程序
 * @author  id:826
 * @version v0.3
 * @date    2016-10-28
 */

#include "wizard.h"
#include "common/app_log.h"
#include "common/utils/utils.h"
#include "window/user_msg.h"
#include "window/wizard_window.h"
#include "common/utils/utils.h"
#include "application.h"

#include "lua/lua_config_parser.h"

#include <errno.h>
#include <signal.h>
#include <sys/mount.h>

using namespace EyeseeLinux;
using namespace std;

#undef LOG_TAG
#define LOG_TAG "Wizard"

const char *g_config_file = "/tmp/data/wizard_config.lua";

Wizard::Wizard()
    : app_(NULL)
    , wizard_win_(NULL)
    , wizard_config_(NULL)
{
    pthread_mutex_init(&post_msg_lock_, NULL);
}

Wizard::~Wizard()
{
    pthread_mutex_destroy(&post_msg_lock_);
    delete wizard_config_;
    wizard_config_ = NULL;
}

void Wizard::UIInit()
{
    app_ = Application::GetApp();
}

void Wizard::CreateWizardWindow()
{
    wizard_win_ = new WizardWindow(NULL);
    assert(wizard_win_ != NULL);

    wizard_win_->SetWindowCallback(this);

    this->OnWindowLoaded();
}

void Wizard::ShowWindow()
{
    if (wizard_win_)
        db_msg("");
        wizard_win_->DoShow();
}

void Wizard::RunGUIEventLoop()
{
    app_->Run();
}

void Wizard::notify(Window *form, int message, int val)
{
    db_msg("message %d val %d", message, val);
    int win_id = form->GetTag();

    PostMessageData *data = new PostMessageData();

    data->message = message;
    data->value = val;
    data->context = this;

    ThreadCreate(&notify_thread_id_, NULL, HandlePostMessage, data);

}

int Wizard::sendmsg(Window *form, int message, int val)
{
    int ret;
    int win_id = form->GetTag();

    db_msg("message %d val %d", message, val);

    PostMessageData *data = new PostMessageData();

    data->message = message;
    data->value = val;
    data->context = this;

    if (data->message > USER_MSG_BASE && data->message < WM_BASE) {
        ret = this->HandleGUIMessage(data->message, data->value);
    }

    delete data;
    data = NULL;

    return ret;
}

void *Wizard::HandlePostMessage(void *context)
{
    PostMessageData *data = reinterpret_cast<PostMessageData*>(context);
    prctl(PR_SET_NAME, "WinMsgPost", 0, 0, 0);

    if (data->message > USER_MSG_BASE && data->message < WM_BASE) {
        pthread_mutex_lock(&data->context->post_msg_lock_);
        data->context->HandleGUIMessage(data->message, data->value);
        pthread_mutex_unlock(&data->context->post_msg_lock_);
    }

    delete data;
    data = NULL;

    return NULL;
}

void Wizard::OnWindowLoaded()
{
    if (!wizard_config_) {
        wizard_config_ = new LuaConfig();

        if (!FILE_EXIST(g_config_file)) {
            db_warn("config file %s not exist, copy default from /usr/share/app/ipc", g_config_file);
            system("cp -f /tmp/data/wizard_config.lua /tmp/data/");
        }

        int ret = wizard_config_->LoadFromFile(g_config_file);
        if (ret != 0) {
            fprintf(stderr, "load %s failed, copy backup and try again\n", g_config_file);
            system("cp -f /tmp/data/wizard_config.lua /tmp/data/");
            ret = wizard_config_->LoadFromFile(g_config_file);
            if (ret != 0) {
                fprintf(stderr, "load %s failed\n", g_config_file);
                exit(-1);
            }
        }
    }
}

void Wizard::OnWindowDetached()
{
    int ret;
    pid_t pid;

    string exec_bin = wizard_config_->GetStringValue("wizard.working_mode");

    if ( exec_bin == string("onecam") || exec_bin == string("dualcam")) {
        wizard_config_->SetBoolValue("wizard.configured", true);
        pid = vfork();
        if (pid == 0) {
            ret = execl(string("/usr/bin/" + exec_bin).c_str(), string("/usr/bin/" + exec_bin).c_str(), NULL);
            if (ret < 0)
                perror("exec failed");
        } else if (pid < 0) {
            perror("fork failed");
        }
    } else {
        wizard_config_->SetBoolValue("wizard.configured", false);
    }

    wizard_config_->SyncConfigToFile(g_config_file, "wizard");
}

/** usage: newfs_msdos -F 32 -O allwinner -b 65536 -c 128 */
#define FORMAT_BIN      "/usr/bin/newfs_msdos"
#define MOUNT_PATH      "/mnt/extsd"
#define STORAGE_DEVICE  "/dev/mmcblk0"

static int format_storage(const char *dev)
{
    int ret = 0;
    int status = 0xAAAA;
    char cmd[128] = {0};
    const char *device;

    db_info("formatting...");

    device = (dev!=NULL)?dev:STORAGE_DEVICE;

    snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s", FORMAT_BIN,
             "-F 32", "-O allwinner", "-b 65536", "-c 128", device);
    status = system(cmd);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            ret = 0;
        } else {
            db_error("format failed");
            ret = -1;
        }
    } else {
        ret = -2;
    }

    return ret;
}

int Wizard::HandleGUIMessage(int msg, int val)
{
    switch (msg) {
        case ONECAM_MODE:
            wizard_config_->SetStringValue("wizard.working_mode", "onecam");
            system("echo onecam > /tmp/data/autostart");
            break;
        case DUALCAM_MODE:
            wizard_config_->SetStringValue("wizard.working_mode", "dualcam");
            system("echo dualcam > /tmp/data/autostart");
            break;
        case FORMAT_DISK: {
                umount(MOUNT_PATH);
                int ret = format_storage(NULL);
                wizard_win_->FormatDoneCallback(ret);
            }
            break;
        case TUTK_SUPPORT:
            wizard_config_->SetBoolValue("wizard.features.tutk", (bool)val);
            break;
        case RTSP_SUPPORT:
            wizard_config_->SetBoolValue("wizard.features.rtsp", (bool)val);
            break;
        case ONVIF_SUPPORT:
            wizard_config_->SetBoolValue("wizard.features.onvif", (bool)val);
            break;
        case CONFIRM_EXIT:
            this->OnWindowDetached();

            if (wizard_config_->GetBoolValue("wizard.configured")) {
                delete wizard_config_;
                wizard_config_ = NULL;
                app_->Terminate();
            } else {
                db_error("configured failed, maybe try again!");
            }

        default:
            break;
    }

    return 0;
}

static int MiniGUIAppMain (int args, const char* argv[])
{
    Wizard wizard;

    wizard.UIInit();

    wizard.CreateWizardWindow();
    wizard.ShowWindow();

    // will block the process
    wizard.RunGUIEventLoop();

    return 0;
}

int main_entry (int args, const char *argv[])
{
    int ret = 0;
    pid_t pid;

    if (!FILE_EXIST(g_config_file)) {
        system("cp /data/wizard_config.lua /tmp/data/wizard_config.lua");
    }

    LuaConfig config;
    config.LoadFromFile(g_config_file);

    string exec_bin = config.GetStringValue("wizard.working_mode");

    if ( exec_bin == string("onecam") || exec_bin == string("dualcam")) {
        pid = vfork();
        if (pid == 0) {
            ret = execl(string("/usr/bin/" + exec_bin).c_str(), string("/usr/bin/" + exec_bin).c_str(), NULL);
            if (ret < 0)
                perror("exec failed");
        } else if (pid < 0) {
            perror("fork failed");
        }

        return 0;
    }

    if (InitGUI (args, argv) != 0) {
        return 1;
    }

    ret = MiniGUIAppMain (args, argv);

    ExitGUISafely(ret);
    TerminateGUI (ret);

    return ret;
}

