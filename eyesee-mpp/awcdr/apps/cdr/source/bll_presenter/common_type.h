/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file common_type.h
 * @brief 定义各presenter需要用到的公共数据结构
 * @author id:826
 * @version v0.3
 * @date 2016-09-07
 */

#pragma once

#include "common/subject.h"
#include "common/observer.h"
#include "common/utils/utils.h"
#include "bll_presenter/presenter.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/recorder/recorder.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/media/recorder/recorder_factory.h"
#include "device_model/rtsp.h"
#include "device_model/httpServer/httpServer.h"

#include "interface/remote_connector.h"

#include <map>

namespace EyeseeLinux {
/**
 * @brief 用于绑定camera与recorder的map
 */
typedef std::map<int, std::map<int, Recorder *> > CamRecMap;

/**
 * @brief 用于绑定recorder与流sender的map
 */
typedef std::map<Recorder *, RtspServer::StreamSender *> StreamSenderMap;

/**
 * @brief 用于描述recorder与camera的绑定关系及类型id的结构体
 * @note 与 CamRecMap 不同的是，这里的绑定内容是id， CamRecMap
 *       的绑定内容是具体对象的指针, 即该结构体只用于描述关系， CamRecMap
 *       是用于操作.
 */
typedef struct RecorderID {
    CameraID cam_id;                /**< 绑定的camera id */
    RecorderType rec_type;          /**< recorder类型id */
    RecorderModel rec_model;        /**< recorder工作模式 */
    int sender_type;                /**< 流发送平台类型, StreamSenderType */
} RecorderID;

/**
 * @brief recorder id集
 *
 * 上述recorder描述结构体的集合
 */
typedef std::vector<RecorderID> RecorderIDSet;

typedef CameraID GroupID;

/**
 * @brief recorder 组, 包含按组定义recorder id集
 *
 * 实际每个物理camera对应一个recorder组，所以在每个recorder id集中,
 * camera id与group id相等
 */
typedef std::map<int, Camera*> CameraMap;
typedef std::map<int, Recorder*> CamRecordMap;

typedef std::map<GroupID, RecorderIDSet> RecorderGroup;

typedef std::map<CameraID, Camera*> CameraGroup;

typedef std::map<RecorderType, Recorder *> RecTypeRecord;
} // namespace EyeseeLinux
