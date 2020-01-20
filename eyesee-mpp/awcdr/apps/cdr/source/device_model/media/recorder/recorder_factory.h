/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file recorder_factory.h
 * @brief recorder工厂类，负责创建如下对象:
 *
 * - MainRecorder 主录像类，实现主录像的写卡录像
 * - StreamRecorder 流录像类，实现网络预览流的生成与发送
 *
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#pragma once

// #include <map>
#include "common/singleton.h"
#include "common/common_inc.h"
#include "device_model/media/camera/camera.h"

namespace EyeseeLinux {


/** 根据不同分辨率和帧率定义recorder类型, 不同recorder，其它初始化参数也将不同 */
typedef enum {
    REC_1080P30FPS = 0, /**< 1080P, 30帧/秒 */
    REC_720P30FPS,      /**< 720P, 30帧/秒 */
    REC_S_1080P30FPS,   /**< 1080P, 30帧/秒 流*/
    REC_S_720P30FPS,    /**< 720P, 30帧/秒 流*/
    REC_4K25FPS,
    REC_S_VGA30FPS,     /**< VGA, 30帧/秒 流*/
    REC_S_CIF30FPS,     /**< CIF, 30帧/秒 流*/
    REC_U_1080P30FPS,   /**< 1080P, 30帧/秒 UVC */
    REC_U_720P30FPS,    /**< 720P, 30帧/秒 UVC */
    REC_U_VGA30FPS,     /**< VGA, 30帧/秒 UVC */
    REC_MAIN_CHN,
    REC_M_SUB_CHN,
    REC_B_SUB_CHN,
    REC_CNT,
} RecorderType;


class Recorder;
class RecorderFactory
    :public Singleton<RecorderFactory>
{
    friend class Singleton<RecorderFactory>;
    public:
        /**
         * @brief 创建Recorder
         * @param rec_type 要创建的recorder type
         * @param camera 用于创建recorder的camera实例
         */
        Recorder *CreateRecorder(RecorderType rec_type, Camera *camera, int p_viChn);
    private:
        // std::map<uint8_t, Recorder*> recorder_map_;

        RecorderFactory();
        ~RecorderFactory();
        RecorderFactory(const RecorderFactory &o);
        RecorderFactory &operator=(const RecorderFactory &o);

        /**
         * @brief 创建主Recorder，仅供模块内部使用，外部应通过CreateRecorder接口
         * @param rec_type 要创建的recorder type
         * @param camera 用于创建recorder的camera实例
         */
        Recorder *CreateMainRecorder(RecorderType rec_type, Camera *camera, int p_viChn);

        /**
         * @brief 创建用于UVC/Webcam的Recorder，仅供模块内部使用，外部应通过CreateRecorder接口
         * @param rec_type 要创建的recorder type
         * @param camera 用于创建recorder的camera实例
         */
        Recorder *CreateUVCRecorder(RecorderType rec_type, Camera *camera, int p_viChn);

        
        Recorder *CreateStreamRecorder(RecorderType rec_type, Camera *camera, int p_viChn);
}; /* class RecorderFactory */
} /* EyeseeLinux */
