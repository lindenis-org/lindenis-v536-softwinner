/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview_window.h
 * @brief 单路预览录像窗口
 * @author id:826
 * @version v0.3
 * @date 2016-11-04
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"

#define PLAYLIST_CONFIRM_BUTTON     (USER_MSG_BASE+100)
#define PLAYLIST_CANCEL_BUTTON      (USER_MSG_BASE+101)
#define PLAYLIST_FILLLIST           (USER_MSG_BASE+102)
#define PLAYLIST_CONFIRM_PLAYFILE   (USER_MSG_BASE+103)

// must be the largest
#define PLAYLIST_BASE   (USER_MSG_BASE+104)

class ListView;
class PlaylistWindow
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(PlaylistWindow, Runtime)

    public:
        NotifyEvent OnClick;

        PlaylistWindow(IComponent *parent);

        ~PlaylistWindow();

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();

        void Update(MSG_TYPE msg, int p_CamID=0);

        void MenuItemClickProc(View *control);

        void ButtonClickProc(View *control);

        void AddFileToList(const std::vector<std::string> &file_list);
		void keyProc(int keyCode, int isLongPress);
		void Menultemkeyproc();
    private:
        ListView *list_view_;
        std::vector<std::string> playlist_;
        BITMAP video_type_;
        BITMAP pic_type_;
		bool isKeyUp;
		WPARAM downKey;
		bool isLongPress;
};
