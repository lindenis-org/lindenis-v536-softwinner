/* *******************************************************************************
 * Copyright (C), 2017-2027, sunchip Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file prompt.h
 * @brief 提示窗口
 * @author id:fangjj
 * @version v10.
 * @date 2017-04-24
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"

#include <time.h>
#include <signal.h>

#define NUMBER_LEVELBAR 7

enum{
	LEVELBAR_VOLUME=0,
	LEVELBAR_SCREEN_BRIGHTNESS,
	LEVELBAR_EMER_SEN,
};


typedef struct {
    const char *image_icon;
    const char *image_png;
    const char *image_B_icon;
    const char *image_B_png;
}levelbar_t;

static levelbar_t  levelbar_[] = {
  	{"LevelBar_icon1","level1","LevelBar_B_icon1","level_b_1"},
	{"LevelBar_icon2","level2","LevelBar_B_icon2","level_b_2"},
	{"LevelBar_icon3","level3","LevelBar_B_icon3","level_b_3"},
	{"LevelBar_icon4","level4","LevelBar_B_icon4","level_b_4"},
	{"LevelBar_icon5","level5","LevelBar_B_icon5","level_b_5"},
	{"LevelBar_icon6","level6","LevelBar_B_icon6","level_b_6"},
	{"LevelBar_icon7","level7","LevelBar_B_icon7","level_b_7"},
};
class TextView;
class LevelBar
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(LevelBar, Runtime)

    public:
        NotifyEvent OnClick;

        LevelBar(IComponent *parent);

        virtual ~LevelBar();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);
        void ShowLevelBar(int prompt_id,RECT & rect_,int index_);

        void DoShow();

        void DoHide();

        void Update(MSG_TYPE msg, int p_CamID=0);

        void keyProc(int keyCode,int isLongPress);
        void HideLevelBar();
        void UpdateLevelBar(int &index_show,int &index_hide);
        int getCurrentIdex();
        void SetLevelBrightness();
        bool getLevelBarFlag(){return m_levelbar_flag;}
    private:
        int last_index;
        int current_index;
        int image_icon_y;
        int image_png_y;
        bool m_finsh;
        bool m_levelbar_flag;//this window show flag
        pthread_mutex_t l_lock_;
};
