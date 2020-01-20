/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file camera_factory.h
 * @brief camera工厂类
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */

#pragma once

// #include "model/display/layer.h"
// #include <map>
#include "common/common_inc.h"
#include "common/singleton.h"
#include "device_model/media/camera/camera.h"

namespace EyeseeLinux {

/**
 * @brief camera类型
 */
typedef enum {
    CAM_IPC_0 = 0,  /**< IPC工作模式cam0 */
    CAM_IPC_1,      /**< IPC工作模式cam1 */
    CAM_STITCH_0,   /**< 拼接模式cam0 */
    CAM_STITCH_1,   /**< 拼接模式cam1 */
    CAM_NORMAL_0,   /**< 单路录像cam0 */
    CAM_NORMAL_1,  /**< 单路录像cam1 */
    CAM_UVC_0,      /**< UVC模式cam0 */
    CAM_UVC_1,      /**< UVC模式cam0 */    
} CameraID;


class Camera;
/**
 * @brief CameraFactory
 * this class contains preview, record, easy to control record,preview for each camId
 */
class CameraFactory
    :public Singleton<CameraFactory>
{
    friend class Singleton<CameraFactory>;
    public:
        Camera *CreateCamera(CameraID cam_id);
        // void DestoryCamera(uint8_t cam_id);
        uint8_t GetCameraCnt();
    private:
        // std::map<uint8_t, Camera*> cam_map_;
        // CDR_RECT csi_rect_, uvc_rect_;  //for show CSI or UVC size in lcd
        uint8_t cam_cnt_;

        CameraFactory();
        ~CameraFactory();
        CameraFactory(const CameraFactory &o);
        CameraFactory &operator=(const CameraFactory &o);

        Camera *CreateCamera(PhysicalCameraID phy_cam_id, CameraInitParam &param);
        Camera *CreateIPCCamera(PhysicalCameraID phy_cam_id);
        Camera *CreateStitchCamera(PhysicalCameraID phy_cam_id);
        Camera *CreateNormalCamera(PhysicalCameraID phy_cam_id);
        Camera *CreateUVCCamera(PhysicalCameraID phy_cam_id);
}; /* class CameraFactory */
} /* EyeseeLinux */
