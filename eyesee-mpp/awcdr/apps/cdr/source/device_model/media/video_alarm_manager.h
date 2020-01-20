/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file video_alarm_manager.h
 * @brief 视频报警管理,包括移动侦测,遮盖报警.
 *
 * @author id:
 * @date 2016-12-09
 *
 * @verbatim
    History:
   @endverbatim
 */


#pragma once

#include "common/message.h"
#include "common/subject.h"
#include "common/singleton.h"
#include "common/common_inc.h"
#include "rgb_ctrl.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"
#include "device_model/media/camera/camera.h"
#include "device_model/media/camera/camera_factory.h"


#include <list>

#define MAX_VI_CHN  2

#define VIDEO_DETECT_WIDTH   22
#define VIDEO_DETECT_HEIGTH  18
#define VIDEO_DETECT_AREA    (((VIDEO_DETECT_WIDTH)*(VIDEO_DETECT_HEIGTH) + 7) / 8)

typedef struct tag_VideoDetectArea {
    char detect_area[VIDEO_DETECT_AREA];
}VideoDetectArea;

typedef struct tag_RectangleArea {
    int left;
    int top;
    int width;
    int height;
}RectangleArea;

namespace EyeseeLinux {

class EyeseeCamera;
class Mutex;

class VideoAlarmManager
    : public ISubjectWrap(VideoAlarmManager),
      public Singleton<VideoAlarmManager>,
      public EyeseeCamera::MODDataCallback
{
    friend class Singleton<VideoAlarmManager>;
    public:
        int InitVideoAlarm(std::map<CameraID, Camera*> &camera_map);

        int ExitVideoAlarm(void);

        /**
         * @brief Video motion detection.
         */
        int EnableMdAlarm(int vi_chn);

        int DisableMdAlarm(int vi_chn);

        int GetMdAlarmStatus(int vi_chn, int &status);

        int SetMdAlarmSensitive(int vi_chn, int sensitive);

        int GetMdAlarmSensitive(int vi_chn, int &sensitive);

        int SetMdAlarmDelayTime(int vi_chn, int delay_time);

        int GetMdAlarmDelayTime(int vi_chn, int &delay_time);

        int SetMdAlarmArea(int vi_chn, const VideoDetectArea &detect_area);

        int GetMdAlarmArea(int vi_chn, VideoDetectArea &detect_area);

        /**
         * @brief Video cover detection.
         */
        int EnableCoverAlarm(int vi_chn);

        int DisableCoverAlarm(int vi_chn);

        int GetCoverAlarmStatus(int vi_chn, int &Status);

        int SetCoverAlarmSensitive(int vi_chn, int sensitive);

        int GetCoverAlarmSensitive(int vi_chn, int &sensitive);

        int SetCoverAlarmDelayTime(int vi_chn, int delay_time);

        int GetCoverAlarmDelayTime(int vi_chn, int &delay_time);

        int SetCoverAlarmArea(int vi_chn, const VideoDetectArea &detect_area);

        int GetCoverAlarmArea(int vi_chn, VideoDetectArea &detect_area);

        int SaveVideoAlarmConfig(void);

        int DefaultVideoAlarmConfig(void);

        void onMODData(int chnId, MotionDetectResult *p_sResult, EyeseeCamera* pCamera);
        static void *VideoDetectThread(void *context);

    private:
        int   chn_num_;
        bool  init_flag_;
        Mutex alarm_lock_;
        LuaConfig    *lua_cfg_;
        EyeseeCamera *eyesee_camera_[MAX_VI_CHN];
        int channel_id_[MAX_VI_CHN];
        pthread_t vda_thread_id_;

        int md_enable_[MAX_VI_CHN];
        int cover_enable_[MAX_VI_CHN];

        int md_sensitive_[MAX_VI_CHN];
        int cover_sensitive_[MAX_VI_CHN];

        int md_delay_time_[MAX_VI_CHN];
        int cover_delay_time_[MAX_VI_CHN];

        VideoDetectArea md_detect_area_[MAX_VI_CHN];
        VideoDetectArea cover_detect_area_[MAX_VI_CHN];

        VideoAlarmManager();
        VideoAlarmManager(const VideoAlarmManager &o);
        VideoAlarmManager &operator=(const VideoAlarmManager &o);
        ~VideoAlarmManager();

        int LoadVideoAlarmConfig(void);
        int CheckRect(RectangleArea &rect);
        int CheckRectInRect(RectangleArea &rect1, RectangleArea &rect2);
        int JudgeRectInRect(int src_width, int src_height, RectangleArea &alarm_rect,
                            VideoDetectArea &alarm_val, int &alarm_area_num);
};

} /* EyeseeLinux */
