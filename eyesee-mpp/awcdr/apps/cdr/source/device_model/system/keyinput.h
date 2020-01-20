/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file keyinput.h
 * @brief 系统按键检测
 *
 *  支持普通按键及电源键检测
 *
 * @author id:826
 * @version v0.3
 * @date 2017-02-17
 */
#pragma once

#include "common/subject.h"

#include <vector>
#include <algorithm>
#include <string.h>

#include <thread>

namespace EyeseeLinux {

#define NORMAL_KEY_DEV  "/dev/input/event1"
#define POWER_KEY_DEV   "/dev/input/event2"
#define KEY_MAP_FILE    "/etc/sunxi-keyboard.kl"

#define APP_KEY_BASE    0xF0
#define APP_KEY_UP      (APP_KEY_BASE + 1)
#define APP_KEY_DOWN    (APP_KEY_BASE + 2)
#define APP_KEY_MENU    (APP_KEY_BASE + 3)
#define APP_KEY_OK      (APP_KEY_BASE + 4)
#define APP_KEY_POWER   (APP_KEY_BASE + 5)
#define APP_KEY_HOME    (APP_KEY_BASE + 6)
#define APP_KEY_RESET   (APP_KEY_BASE + 7)

class KeyInput
    : public ISubjectWrap(KeyInput)
{
    private:
        struct KeyInfo
        {
            const char *name_;  // user defined
            int key_code_;      // driver upload
            int key_id_;        // user defined
        };

    public:
        KeyInput();

        ~KeyInput();

        int8_t InitKeyInput();

        void TermKeyInput();

        int8_t RegistKey(const char *name);

        int8_t UnRegistKey(const char *name);

        void DispatchKeyEvent(int key_id);
    private:
        int key_fd_;
        int power_key_fd_;
        std::thread key_event_thread_;
        std::vector<KeyInfo> key_map_;
        std::vector<KeyInfo> regkey_list_;

        int8_t LoadKeyMap(const char *file);

        int8_t RunKeyEventThread();

        inline std::vector<KeyInfo>::iterator FindMatchKey(std::vector<KeyInfo> &key_list, const char *name) {
            return std::find_if(key_list.begin(), key_list.end(), [=](KeyInfo &k) {
                        return (strcmp(k.name_, name) == 0);});
        }

        inline std::vector<KeyInfo>::iterator FindMatchKey(std::vector<KeyInfo> &key_list, int key_code) {
            return std::find_if(key_list.begin(), key_list.end(), [=](KeyInfo &k) {
                        return k.key_code_ == key_code;});
        }

        static void KeyEventThread(KeyInput *self);


}; // class KeyInput

} // namespace EyeseeLinux
