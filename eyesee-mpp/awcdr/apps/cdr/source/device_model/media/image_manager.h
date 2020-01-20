/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file  image_manager.h
 * @brief overlay模块控制
 * @author id:
 * @version v0.3
 * @date 2016-11-25
 * @verbatim
    History:
    - 2016-11-25, Create.
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


namespace EyeseeLinux {


/* 日夜模式为定时，该时间段为白天模式，此外为夜间模式 */
typedef struct tag_plan_time {
    int start_hour;     // 开始时间
    int start_minute;
    int end_hour;       // 结束时间
    int end_minute;
} plan_time;

class EyeseeCamera;
class Mutex;

class ImageManager
    : public ISubjectWrap(ImageManager),
      public Singleton<ImageManager>
{
    friend class Singleton<ImageManager>;
    public:
        /**
         * @brief 初始化Image模块。
         */
        int InitImage(std::map<CameraID, Camera*> &camera_map);

        /**
         * @brief 去初始化Image模块。
         */
        int ExitImage(void);

        /**
         * @brief 设置/获取 VI 亮度调节
         * @param vi_chn VI通道号
         * - >= 0, vi_chn表示VI通道号
         * @param brightness ，亮度值
         * - 0 <= val =< 100
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetBrightness(int vi_ch, int brightness);
        int GetBrightness(int vi_ch, int &brightness);

        /**
         * @brief 设置/获取 VI 饱和度调节
         * @param vi_chn VI通道号
         * - >= 0, vi_chn表示VI通道号
         * @param contrast 饱和度值
         * - 0 <= val =< 100
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetContrast(int vi_ch, int contrast);
        int GetContrast(int vi_ch, int &contrast);

        /**
         * @brief 设置/获取 VI 对比度调节
         * @param vi_chn VI通道号
         * - >= 0, vi_chn表示VI通道号
         * @param saturation 对比度值
         * - 0 <= val =< 100
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetSaturation(int vi_ch, int saturation);
        int GetSaturation(int vi_ch, int &saturation);

        /**
         * @brief 设置/获取 VI 锐度调节
         * @param vi_chn VI通道号
         * - >= 0, vi_chn表示VI通道号
         * @param sharpness 锐度值
         * - 0 <= val =< 100
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetSharpness(int vi_ch, int sharpness);
        int GetSharpness(int vi_ch, int &sharpness);

        /**
         * @brief 设置/获取 VI 色度调节
         * @param vi_chn VI通道号
         * - >= 0, vi_chn表示VI通道号
         * @param hue 色度值
         * - 0 <= val =< 100
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetHue(int vi_ch, int hue);
        int GetHue(int vi_ch, int &hue);

        /**
         * @brief 设置/获取 VI 通道白平衡
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param wb_type 白平衡类型
         * - 0 关闭白平衡功能
         * - 1 自动白平衡
         * - 2 白炽灯
         * - 3 暖光灯
         * - 4 自然光
         * - 5 白日光
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetWhiteBalance(int vi_ch, int wb_type);
        int GetWhiteBalance(int vi_ch, int &wb_type);

        /**
         * @brief 设置/获取 VI 通道曝光调节
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param exp_type 曝光类型
         * - 0 自动曝光
         * - 1 手动设置
         * @param exp_time 曝光时间，该值为倒数，如exp_time=5，即1/（5+1）
         * - >0
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetExposure(int vi_ch, int exp_type, int exp_time);
        int GetExposure(int vi_ch, int &exp_type, int &exp_time);

        /**
         * @brief 设置/获取 VI 通道宽动态
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param wd_leve 曝光类型
         * - 0 关闭宽动态
         * - 1 宽动态1级
         * - n 宽动态n级
         * - 6 宽动态6级
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetWideDynamic(int vi_ch, int wd_leve);
        int GetWideDynamic(int vi_ch, int &wd_leve);

        /**
         * @brief 设置/获取 VI 通道的降噪功能
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param dn_leve 降噪级别
         * - 0 关闭降噪功能
         * - 1 降噪1级
         * - n 降噪n级
         * - 6 降噪1级
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetDenoise(int vi_ch, int dn_leve);
        int GetDenoise(int vi_ch, int &dn_leve);

        /**
         * @brief 设置/获取  VI 通道的曝光频率
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param freq 曝光频率 0-自动，1-50Hz, 2-60Hz
         * - >= 0
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetFlickerFreq(int vi_ch, int freq);
        int GetFlickerFreq(int vi_ch, int &freq);

        /**
         * @brief 设置/获取  VI 通道的图像翻转
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param flip_enable 图像翻转使能标志
         * - 0 停止图像翻转
         * - 1 使能图像翻转
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetFlip(int vi_ch, int flip_enable);
        int GetFlip(int vi_ch, int &flip_enable);

        /**
         * @brief 设置/获取  VI 通道的图像镜像
         * @param vi_ch VI通道号
         * - >= 0, vi_ch 表示VI通道号
         * @param mirror_enable 图像镜像使能标志
         * - 0 停止图像镜像
         * - 1 使能图像镜像
         * @return
         *  - 成功 0
         *  - 操作失败 -1
         */
        int SetMirror(int vi_ch, int mirror_enable);
        int GetMirror(int vi_ch, int &mirror_enable);

        /**
         * @brief 设置/获取 Day Night mode 参数
         */
        int SetIrcutDetect(int vi_ch, int detect_src);
        int GetIrcutDetect(int vi_ch, int &detect_src);

        int SetDayNightMode(int vi_ch, int daynight_mode);
        int GetDayNightMode(int vi_ch, int &daynight_mode);

        int SetTriggerSensitive(int vi_ch, int sensitive);
        int GetTriggerSensitive(int vi_ch, int &sensitive);

        int SetTriggerDelayTime(int vi_ch, int delay_time);
        int GetTriggerDelayTime(int vi_ch, int &delay_time);

        int SetPlanTime(int vi_ch, plan_time time);
        int GetPlanTime(int vi_ch, plan_time &time);

        int SaveImageConfig();
        int DefaultImageConfig();

    private:
        bool  init_flag_;
        Mutex image_lock_;
        LuaConfig *lua_cfg_;
        std::vector<EyeseeCamera *> camera_list_;

        int max_chn_;

        /* image config */
        std::vector<int> white_balance_;
        std::vector<int> exposure_type_;
        std::vector<int> exposure_time_;
        std::vector<int> wide_dynamic_;
        std::vector<int> denoise_;
        std::vector<int> flicker_freq_;
        std::vector<int> flip_;
        std::vector<int> mirror_;

        /* Color Config */
        std::vector<int> brightness_;
        std::vector<int> contrast_;
        std::vector<int> saturation_;
        std::vector<int> sharpness_;
        std::vector<int> hue_;

        /* Day Night Mode config */
        std::vector<int> detect_src_;
        std::vector<int> day_night_mode_;
        std::vector<int> trigger_leve_;
        std::vector<int> trigger_delay_time_;
        std::vector<plan_time> daymode_time_;

        ImageManager();
        ImageManager(const ImageManager &o);
        ImageManager &operator=(const ImageManager &o);
        ~ImageManager();

        int LoadImageConfig();

};  /* class ImageManager */
}   /* namespace EyeseeLinux */
