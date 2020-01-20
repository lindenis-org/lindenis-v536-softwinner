/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file keyinput.cpp
 * @brief 系统按键检测
 *
 *  支持普通按键及电源键检测
 *
 * @author id:826
 * @version v0.3
 * @date 2017-02-17
 */
#define NDEBUG
#include "keyinput.h"
#include "common/app_log.h"

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#undef LOG_TAG
#define LOG_TAG "KeyInput"

using namespace EyeseeLinux;
using namespace std;

KeyInput::KeyInput()
    : key_fd_(-1)
    , power_key_fd_(-1)
{
    key_map_.clear();
    regkey_list_.clear();
}

KeyInput::~KeyInput()
{
    TermKeyInput();
}

int8_t KeyInput::InitKeyInput()
{
    int ret = 0;

    key_fd_ = open(NORMAL_KEY_DEV, O_RDONLY);
    if (key_fd_ < 0) {
        db_error("normal key device: %s open failed, %s", NORMAL_KEY_DEV, strerror(errno));
        goto failed;
    }

    power_key_fd_ = open(POWER_KEY_DEV, O_RDONLY);
    if (power_key_fd_ < 0) {
        db_error("power key device: %s open failed, %s", POWER_KEY_DEV, strerror(errno));
        goto failed;
    }

    ret = LoadKeyMap(KEY_MAP_FILE);
    if (ret < 0) {
        db_error("load key map file %s failed, %s", KEY_MAP_FILE, strerror(errno));
        goto failed;
    }

    ret = RunKeyEventThread();
    if (ret < 0) {
        goto failed;
    }

    return 0;

failed:
    db_error("init key input failed");
    return -1;
}

void KeyInput::TermKeyInput()
{
    if (key_fd_ > 0){
        close(key_fd_);
        key_fd_ = -1;
    }

    if (power_key_fd_ > 0){
        close(power_key_fd_);
         power_key_fd_ = -1;
        }

    if (key_event_thread_.joinable()) {
        pthread_cancel(key_event_thread_.native_handle());
        key_event_thread_.join();
    }

}

int8_t KeyInput::LoadKeyMap(const char *file)
{
    FILE *fp;
    char key[16];
    char name[32];
    int keycode;

    // define the default key map
    key_map_.push_back({"UP",    0, APP_KEY_UP});
    key_map_.push_back({"DOWN",  0, APP_KEY_DOWN});
    key_map_.push_back({"MENU",  0, APP_KEY_MENU});
    key_map_.push_back({"OK",    0, APP_KEY_OK});
    key_map_.push_back({"POWER", 0, APP_KEY_POWER});
    key_map_.push_back({"HOME",  0, APP_KEY_HOME});
    key_map_.push_back({"RESET", 0, APP_KEY_RESET});

    fp = fopen(KEY_MAP_FILE,"r");
    if (fp == NULL) {
        db_error("open key map file %s failed, %s", file, strerror(errno));
        return -1;
    }

    while (EOF != fscanf(fp, "%s%d%s", key, &keycode, name)) {
        auto it = FindMatchKey(key_map_, name);
        if (it != key_map_.end())
            it->key_code_ = keycode;
    }

    fclose(fp);

    return 0;
}

int8_t KeyInput::RunKeyEventThread()
{
    key_event_thread_ = thread(KeyInput::KeyEventThread, this);

    return 0;
}

void KeyInput::KeyEventThread(KeyInput *self)
{
    fd_set rfds;
    struct input_event key;

    prctl(PR_SET_NAME, "KeyEventThread", 0, 0, 0);

    int key_fd = self->key_fd_;
    int p_key_fd = self->power_key_fd_;

    for (;;) {
        FD_ZERO(&rfds);

        int max_fd = key_fd;

        FD_SET(key_fd, &rfds);

        if (p_key_fd > 0) {
            FD_SET(p_key_fd, &rfds);
            if (p_key_fd > key_fd)
                max_fd = p_key_fd;
        }

        int ret = select (max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            db_error("select failed, %s", strerror(errno));
            usleep(10 * 1000);
            continue;
        }

        if (FD_ISSET(key_fd, &rfds)) {
            FD_CLR(key_fd, &rfds);
            read(key_fd, &key, sizeof(key));
            if (key.code < 0) {
                db_warn("unknown key down");
                continue;
            } else if (key.code > 0){
                auto it = self->FindMatchKey(self->regkey_list_, key.code);
                if (it == self->regkey_list_.end()) {
                    db_msg("undefined key, code: %d", key.code);
                } else {
                    if (key.value) {
                        db_msg("normal key %s down, code: %d, id: 0x%x",
                                it->name_, key.code, it->key_id_);
                        self->DispatchKeyEvent(it->key_id_);
                    } else {
                        db_msg("normal key %s up, code: %d, id: 0x%x",
                                it->name_, key.code, it->key_id_);
                    }
                }
            }

        }

        if (p_key_fd < 0) {
            continue;
        }

        if(FD_ISSET(p_key_fd, &rfds))
        {
            FD_CLR(p_key_fd, &rfds);
            read(p_key_fd, &key, sizeof(key));
            if (key.code < 0) {
                db_warn("unknown key down");
                continue;
            } else if (key.code > 0){
                auto it = self->FindMatchKey(self->regkey_list_, key.code);
                if (it == self->regkey_list_.end()) {
                    db_msg("undefined key, code: %d", key.code);
                } else {
                    if (key.value) {
                        db_msg("normal key %s down, code: %d, id: 0x%x",
                                it->name_, key.code, it->key_id_);
                        self->DispatchKeyEvent(it->key_id_);
                    } else {
                        db_msg("normal key %s up, code: %d, id: 0x%x",
                                it->name_, key.code, it->key_id_);
                    }
                }
            }
        }
    }
}

int8_t KeyInput::RegistKey(const char *name)
{
    if (strncmp(name, "ALL", 3) == 0) {
        regkey_list_ = key_map_;
    } else {
        auto it = FindMatchKey(key_map_, name);
        if (it == key_map_.end()) {
            db_warn("key name: %s, no such key found", name);
            return -1;
        } else {
            regkey_list_.push_back(*it);
        }
    }

    return 0;
}

int8_t KeyInput::UnRegistKey(const char *name)
{
    if (strncmp(name, "ALL", 3) == 0) {
        regkey_list_.clear();
    } else {
        auto it = FindMatchKey(regkey_list_, name);

        if (it == regkey_list_.end()) {
            db_warn("key name: %s, no such key found", name);
            return -1;
        }

        regkey_list_.erase(it);
    }

    return 0;
}

void KeyInput::DispatchKeyEvent(int key_id)
{
    switch (key_id) {
        case APP_KEY_OK:
            Notify(MSG_OK_KEY_DOWN);
            break;
        case APP_KEY_UP:
            Notify(MSG_UP_KEY_DOWN);
            break;
        case APP_KEY_DOWN:
            Notify(MSG_DOWN_KEY_DOWN);
            break;
        case APP_KEY_MENU:
            Notify(MSG_MENU_KEY_DOWN);
            break;
        case APP_KEY_HOME:
            Notify(MSG_HOME_KEY_DOWN);
            break;
        case APP_KEY_POWER:
            Notify(MSG_POWER_KEY_DOWN);
            break;
        case APP_KEY_RESET:
            break;
        default:
            break;
    }
}
