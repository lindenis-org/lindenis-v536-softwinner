/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file status_bar_window.h
 * @brief 状态栏窗口
 * @author id:690
 * @version v0.3
 * @date 2017-01-17
 */
#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"
#include "common/buttonPos.h"
#include <time.h>
#include <signal.h>
#define ZOOM_LOOP 0
class TextView;

typedef enum {
	SETTING_WINDOW, //0
	PREVIEW_WINDOW,
	PLAYBACK_WINDOW,
}WINDOWID;
typedef enum {
LEFT_RIGHT_SHOW = 0,
LEFT_SHOW_ONLY,
RIGHT_SHOW_ONLY,
LEFT_RIGHT_HIDE,
}LEFT_RIGHT_ICON_STATUS;

class StatusBarBottomWindow : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(StatusBarBottomWindow, Runtime)
public:
    StatusBarBottomWindow(IComponent *parent);
    virtual ~StatusBarBottomWindow();
    std::string GetResourceName();
    void GetCreateParams(CommonCreateParams& params);
    void PreInitCtrl(View *ctrl, std::string &ctrl_name);
    int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam);
    void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    void DoHide();
    void PreviewWindownButtonStatus(bool on_off, bool reload_all, int flag=0);
	void PreviewWindownModeButtonStatus(int flag);
    void initPreviewButtonPos();
    void GetPreviewButtonPos(struct buttonPos *bps,int len);
    void initPlayBackButtonPos();
    void GetPlayBackButtonPos(struct buttonPos *bps,int len);
    void PlaybackWindownButtonStatus(bool on_off,bool l_hilight,bool r_hilight);
    void PlayingWindownButtonStatus(bool on_off,bool p_pause, bool reload_all,bool jpeg_play_flag);
	void hideStatusBarBottomWindow();
	void showStatusBarBottomWindow();
	void updateRecordIcon(bool record_flag);
	void updatePhotoIcon(bool photo_flag);
	void updateLockIcon(bool lock_flag);

private:
    buttonPos_ PreviewButtonPos[PrviewButtonPosLen];
    buttonPos_ PlayBackButtonPos[PlayBackButtonPosLen];
private:


};

