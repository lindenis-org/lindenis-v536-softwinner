/* *******************************************************************************
 *
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file camera.cpp
 * @brief camera模块基类
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#include "device_model/media/media_file.h"
#include "device_model/media/camera/camera.h"
#include "common/app_log.h"
#include "device_model/display.h"
#include "device_model/storage_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/menu_config_lua.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "common/setting_menu_id.h"
#include "../../../bll_presenter/audioCtrl.h"
#include "device_model/media/osd_manager.h"
#include "device_model/aes_ctrl.h"
#include "device_model/system/event_manager.h"


#include <sstream>
#include <sys/stat.h>
#include <string>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/reboot.h>

#undef LOG_TAG
#define LOG_TAG "camera.cpp"
//#define FACE_DETECT
//#define SENSOR_DEBUG
using namespace EyeseeLinux;
using namespace std;
#define PIC_DIR					"/mnt/extsd/pic/"

#define PREVIEW_WAIT_TIME		2000 
#define EPSILON					(0.000001)
#ifdef  FACE_DETECT
#define FACE_CHN		3
#define FACE_TAR_RESULT			"/mnt/extsd/pic/facedata.tar"
#define FACE_TAR_TEMP_RESULT	"/mnt/extsd/pic/facedata_tmp.tar"
#endif



std::vector<face_result> FaceResult;
std::vector<face_name_info> FaceInfo;

#define SET_CAMERA_PARAMS(func,...) \
{ \
    CameraParameters params; \
    eyesee_camera_->getParameters(main_enc_chn_, params); \
    params.func(__VA_ARGS__); \
    eyesee_camera_->setParameters(main_enc_chn_, params); \
}

#define GET_CAMERA_PARAMS(func) \
{ \
    CameraParameters params; \
    eyesee_camera_->getParameters(main_enc_chn_, params); \
    return params.func(); \
}

#define GET_CAMERA_PARAMS_(func,...) \
{ \
    CameraParameters params; \
    eyesee_camera_->getParameters(main_enc_chn_, params); \
    params.func(__VA_ARGS__); \
}

#define SET_ISP_PARAMS(func,...) \
{ \
    CameraParameters params; \
    eyesee_camera_->getISPParameters(params); \
    params.func(__VA_ARGS__); \
    eyesee_camera_->setISPParameters(params); \
}

#define GET_ISP_PARAMS_(func, val) \
{ \
    CameraParameters params; \
    eyesee_camera_->getISPParameters(params); \
    val = params.func(); \
}

#define GET_ISP_PARAMS(func) \
{ \
    CameraParameters params; \
    eyesee_camera_->getISPParameters(params); \
    return params.func(); \
}


Camera::Camera(PhysicalCameraID phy_cam_id)
    : eyesee_camera_(NULL)
    , phy_cam_id_(phy_cam_id)
    , main_enc_chn_(-1)
    , sub_enc_chn_(-1)
#ifdef GUI_SUPPORT
    , layer_(NULL)
    , layer_handle_(-1)
    , zorder_(0)
#endif
    , cam_status_(CAM_CLOSED)
    , continue_pic_cont_(0)
    , time_take_pic_cont_(PTL_OFF)
    , auto_time_take_pic_cont_(PTL_OFF)
    , video_thumb_pic_(NULL)
    , m_degree(0)
    , m_whiteBalance(0)
    , m_ExposureBias(4)
    , m_LightFreq(3)
    , osd_enable_(false)
    , main_pic_cnt_(0)
    , sub_pic_cnt_(0)
    , takepic_done_(true)
    , eis_enable_(false)
    , m_takeOnce(false)
    , face_snap_cnt(0)
    , max_face_id(0)
    , face_status(0)
    , face_flag(0)
    , m_dispHandle(-1)
    , mresult_cnt(0)
{
    StorageManager *sm = StorageManager::GetInstance();
    int status = sm->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
        storage_status_ = STORAGE_OK;
    } else {
        storage_status_ = STORAGE_NOT_OK;
    }
	db_warn("1111 phy_cam_id = %d ",phy_cam_id);
	if(phy_cam_id == 0){
		zorder_ = 0;
	}else if(phy_cam_id == 1){
		zorder_ = 2;
	}
    sm->Attach(this);

    pthread_mutex_init(&lock_, NULL);
	pthread_mutex_init(&face_lock, NULL);
	pthread_mutex_init(&thumb_pic_lock, NULL);
    pthread_mutex_init(&photo_file_lock,NULL);
    cdx_sem_init(&mSemRenderStart, 0);
}

Camera::~Camera()
{
    db_msg("Camera Destructor");
    if (eyesee_camera_) {
        this->DeinitCamera();
        // TODO: do it in DeinitCamera
        if (cam_status_ == CAM_OPENED)
            this->Close();
    }

    StorageManager *sm = StorageManager::GetInstance();
    sm->Detach(this);
    pthread_mutex_destroy(&lock_);
	pthread_mutex_destroy(&face_lock);
	pthread_mutex_destroy(&thumb_pic_lock);
	pthread_mutex_destroy(&photo_file_lock);
    cdx_sem_deinit(&mSemRenderStart);
}

int Camera::startMotionDetect()
{
    MOTION_DETECT_ATTR_S par;
    par.nSensitivity = 1;		// 设置移动侦测灵敏度
	if( eyesee_camera_)
	{
		eyesee_camera_->setMODDataCallback(this);
        //set sensitiviity_level
        eyesee_camera_->setMODParams(init_param_.sub_chn_,par);
		eyesee_camera_->startMODDetect(init_param_.sub_chn_);
	}
	return 0;
}

int Camera::stopMotionDetect()
{
	if( eyesee_camera_)
		eyesee_camera_->stopMODDetect(init_param_.sub_chn_);

	return 0;
}

void Camera::ConfigCamera(int CsiChn,int ispdev,int vippchnValue0,int vippchnValue1)
{
    printf("ConfigCamera : CsiChn = %d,ispdev = %d,vippchnValue0 = %d,vippchnValue1 = %d\n",CsiChn,ispdev,vippchnValue0,vippchnValue1);
    EyeseeCamera::clearCamerasConfiguration();
    int cameraId;
    CameraInfo cameraInfo;
    VI_CHN chn;
    cameraId = CsiChn;
    cameraInfo.facing = CAMERA_FACING_BACK;
	if(CsiChn == 1 )
		cameraInfo.facing = CAMERA_FACING_FRONT;

    cameraInfo.orientation = 0;
    cameraInfo.canDisableShutterSound = true;
    cameraInfo.mCameraDeviceType = CameraInfo::CAMERA_CSI;
    cameraInfo.mMPPGeometry.mCSIChn = CsiChn;
    ISPGeometry isp_geometry;
    isp_geometry.mISPDev = ispdev;
    isp_geometry.mScalerOutChns.push_back(vippchnValue0);
    isp_geometry.mScalerOutChns.push_back(vippchnValue1);
    cameraInfo.mMPPGeometry.mISPGeometrys.push_back(isp_geometry);
    EyeseeCamera::configCameraWithMPPModules(cameraId, &cameraInfo);

	if (SCREEN_HEIGHT > SCREEN_WIDTH)
		m_degree = 270;

}
int Camera::Open()
{
    printf("open camera: %d\n", phy_cam_id_);
    eyesee_camera_ = EyeseeCamera::open(phy_cam_id_);
    if(eyesee_camera_ == NULL) {
        db_error("fail to open EyeseeCamera");
        return -1;
    }
    cam_status_ = CAM_OPENED;

    return 0;
}

int Camera::Close()
{
    if (eyesee_camera_ == NULL) return 0;

    EyeseeCamera::close(eyesee_camera_);
	eyesee_camera_ = NULL;
    cdx_sem_reset(&mSemRenderStart);
    cam_status_ = CAM_CLOSED;

    return 0;
}

#ifdef GUI_SUPPORT
int Camera::InitDisplay(int p_CamId)
{
    int hlay;
    ViewInfo sur;

    sur.x = init_param_.sur_.x;
    sur.y = init_param_.sur_.y;
    sur.w = init_param_.sur_.w;
    sur.h = init_param_.sur_.h;

    if ((sur.w == 0) || (sur.h == 0))
	{
        fprintf(stderr, "fun[%s:%d] xyhw[%d,%d,%d,%d]\n", __FUNCTION__, __LINE__, sur.x, sur.y, sur.w, sur.h);
    }

    layer_ = Layer::GetInstance();
    if(layer_ == NULL) {
        db_error("fail to new Display");
        return -1;
    }
    int layer_id = phy_cam_id_;
	db_msg("############request layer_id, layer_id: %d", layer_id);
	db_msg("init display zorder %d",zorder_);
	switch(p_CamId)
	{
		case CAM_A:
		{
			if(sur.w == SCREEN_WIDTH ){
				hlay = layer_->RequestLayer(LAYER_CAM0, 0, 0, zorder_, &sur);
			} else {
				sur.w = SCREEN_WIDTH;
				sur.h = SCREEN_HEIGHT;
				hlay = layer_->RequestLayer(LAYER_CAM0, 0, 0, zorder_, &sur);
			}
			break;
		}
		case CAM_B:
		{
			if(sur.w == SCREEN_WIDTH ){
				hlay = layer_->RequestLayer(LAYER_CAM1, 1, 0, zorder_, &sur);
			} else {
				sur.w = SCREEN_WIDTH;
				sur.h = SCREEN_HEIGHT;
				hlay = layer_->RequestLayer(LAYER_CAM1, 1, 0, zorder_, &sur);
			}
			db_msg("SetLayerAlpha LAYER_CAM1 255");
			Layer::GetInstance()->SetLayerAlpha(LAYER_CAM1, 255);
			break;
		}
		default:
			break;
	}
    if (hlay < 0)
	{
        db_error("init display failed");
        return -1;
    }
    db_msg("request layer, hlay: %d", hlay);

    layer_handle_ = hlay;
    return hlay;
}


int Camera::GetCameraDispRect(ViewInfo &p_info)
{
	p_info = init_param_.sur_;

	return 0;
}

int Camera::SetCameraDispRect(ViewInfo p_info, int p_Zorder)
{
	init_param_.sur_ = p_info;
	zorder_ = p_Zorder;
	if( IsPreviewing() )
	{
		if(phy_cam_id_)
		{
			layer_->SetLayerZorder(LAYER_CAM1, p_Zorder);
			layer_->SetLayerRect(LAYER_CAM1, &p_info);
		}
		else
		{
			layer_->SetLayerZorder(LAYER_CAM0, p_Zorder);
			layer_->SetLayerRect(LAYER_CAM0, &p_info);
		}
	}

	return 0;
}
#endif

void Camera::SetDispZorder(int zorder)
{
    zorder_ = zorder;
}

int Camera::InitCamera(CameraInitParam &init_param)
{
    int ret = -1;
    CameraParameters cam_param;
    init_param_ = init_param;

    
    // 0. set callback
    eyesee_camera_->setInfoCallback(this);
    eyesee_camera_->setErrorCallback(this);
    // 1. set device attr
    VI_DEV_ATTR_S pstDevAttr;
    memset(&pstDevAttr, 0, sizeof(pstDevAttr));
    //eyesee_camera_->setDeviceAttr(&pstDevAttr);

    // 2. prepare device
    ret = eyesee_camera_->prepareDevice();
    if (ret != 0) {
        db_error("prepare device error!");
        return -1;
    }

    // 3. start device
    ret = eyesee_camera_->startDevice();
    if (ret != 0) {
        db_error("startDevice error!");
        return -1;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", "open main channel", tv.tv_sec, tv.tv_usec);

    // create and init channel for sub stream encode and preview
    // 4. open and prepare channel
    ret = eyesee_camera_->openChannel(init_param_.main_chn_,true);
    if (ret != NO_ERROR) {
        db_error("open channel error! chn: %d",init_param_.main_chn_);
        return -1;
    }

    // 5. set camera parameters
    if(main_enc_chn_ == -1)
    {
        eyesee_camera_->getParameters(init_param_.main_chn_, m_main_cam_cfg);
        main_enc_chn_ = init_param_.main_chn_;
    }
	printf("[debug_jaosn]:main_enc_chn_ = %d,sub_enc_chn_ = %d, main_chn_ %d, sub_chn_  %d\n",main_enc_chn_,sub_enc_chn_, init_param_.main_chn_ , init_param_.sub_chn_);
    init_param_.cam_param_ = m_main_cam_cfg;
    init_param_.cam_param_.setPreviewFormat(init_param_.pixel_fmt_);

    init_param_.cam_param_.setVideoSize(init_param_.main_venc_size_);
    init_param_.cam_param_.setPictureSize(init_param_.main_penc_size_);
    init_param_.cam_param_.setVideoBufferNumber(init_param_.buffernumber);
    init_param_.cam_param_.setPreviewFrameRate(init_param_.framate);
	init_param_.cam_param_.setPreviewRotation(m_degree);
    init_param_.cam_param_.setPictureMode(TAKE_PICTURE_MODE_FAST);
    init_param_.cam_param_.setJpegThumbnailSize({320,240});  //320, 240
    init_param_.cam_param_.setJpegThumbnailQuality(60);
    init_param_.cam_param_.setColorSpace(V4L2_COLORSPACE_REC709);	// V4L2_COLORSPACE_JPEG
    db_msg("main_enc_chn_ : init_param_.vflip = %d,init_param_.mirror = %d",init_param_.vflip,init_param_.mirror);
    //init_param_.vflip= 0;
    //init_param_.mirror=0;
    init_param_.cam_param_.SetFlip(init_param_.vflip);
    init_param_.cam_param_.SetMirror(init_param_.mirror);
    db_msg("sensor fps %d",init_param_.framate);
    ret = eyesee_camera_->setParameters(main_enc_chn_, init_param_.cam_param_);
    if (ret != 0) {
        db_error("setParameters error!");
        return -1;
    }
	eyesee_camera_->KeepPictureEncoder(main_enc_chn_, false);
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", "prepare main channel", tv.tv_sec, tv.tv_usec);
    ret = eyesee_camera_->prepareChannel(main_enc_chn_);
    if (ret != 0) {
        db_error("prepareChannel error! chn: %d",main_enc_chn_);
        return -1;
    }

    ret = eyesee_camera_->startChannel(main_enc_chn_);
    if (ret != 0) {
        db_error("startChannel error! chn: %d",main_enc_chn_);
        return -1;
    }

 //   ISPModuleOnOff(1);

    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", "open sub channel", tv.tv_sec, tv.tv_usec);
    cam_status_ = CAM_INITED;
    SET_ISP_PARAMS(ChnIspAwb_SetMode, 0);
    SET_ISP_PARAMS(ChnIspAwb_SetMode, m_whiteBalance);
    int val = GetModeConfigIndex(SETTING_CAMERA_EXPOSURE);
    SetExposureValue(val);
    db_error("=======m_ExposureBias %d=======",m_ExposureBias);
    int freq = GetModeConfigIndex(SETTING_CAMERA_LIGHTSOURCEFREQUENCY);
    SetLightFreq(freq);
    db_error("=======m_LightFreq %d=======",m_LightFreq);
    #if 0
	if(init_param.sub_chn_ == FACE_CHN)
	{
		InitCameraFace(init_param);
	}
    #endif
    return ret;
}

#ifdef  FACE_DETECT
int Camera::InitCameraFace(CameraInitParam &init_param)
{
	int ret = -1;
    CameraParameters cam_param;
    init_param_ = init_param;
	printf("[%s]:enter init face!\n", __FUNCTION__);
	if(init_param_.sub_chn_ != FACE_CHN)
	{
		printf("[%s]: invalid sub_chn %d\n", __FUNCTION__, init_param_.sub_chn_);
		return ret;
	}
	if(FQ_init() < 0)
	{
		return -1;
	}

    // 4. open and prepare channel
    ret = eyesee_camera_->openChannel(init_param_.sub_chn_, false);
    if (ret != NO_ERROR) {
        db_error("open channel error! chn: %d",init_param_.sub_chn_);
        return -1;
    }

    // 5. set camera parameters

	eyesee_camera_->getParameters(init_param_.sub_chn_, cam_param);
	printf("[aaa]:main_enc_chn_ = %d,sub_enc_chn_ = %d, main_chn_ %d, sub_chn_  %d\n",main_enc_chn_,sub_enc_chn_, init_param_.main_chn_ , init_param_.sub_chn_);

    SIZE_S FaceDetectSize={1280, 720};
	cam_param.setVideoSize(FaceDetectSize);
	cam_param.setDisplayFrameRate(init_param_.framate);
	cam_param.setPreviewFormat(init_param_.pixel_fmt_);
	cam_param.setVideoBufferNumber(init_param_.buffernumber);

    ret = eyesee_camera_->setParameters(init_param_.sub_chn_, cam_param);
    if (ret != 0) {
        db_error("setParameters error!");
        return -1;
    }
	eyesee_camera_->prepareChannel(init_param_.sub_chn_);
    eyesee_camera_->setFaceDetectDataCallback(this);

	eyesee_camera_->getParameters(init_param_.sub_chn_, cam_param);
	cam_param.setPictureMode(TAKE_PICTURE_MODE_FAST);
	cam_param.setPictureSize(FaceDetectSize);
	cam_param.setJpegQuality(99);  
	eyesee_camera_->setParameters(init_param_.sub_chn_, cam_param);

	eyesee_camera_->enableFaceAutoTakePicture(init_param_.sub_chn_, this);
	
	eyesee_camera_->startChannel(init_param_.sub_chn_); 

    return ret;
}

int Camera::DeinitCameraFace()
{
	int ret=0;
	pthread_mutex_lock(&lock_);
	
	FQ_destroy();
	ret = eyesee_camera_->stopChannel(FACE_CHN);
	if (ret != NO_ERROR) 
	{
        db_error("stop channel[%d] failed", FACE_CHN);
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    ret = eyesee_camera_->releaseChannel(FACE_CHN);
	if (ret != NO_ERROR) 
	{
        db_error("releaseChannel[%d] failed", FACE_CHN);
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    ret = eyesee_camera_->closeChannel(FACE_CHN);
	if (ret != NO_ERROR) 
	{
		db_error("releaseChannel[%d] failed", FACE_CHN);
		pthread_mutex_unlock(&lock_);
		return -1;
	}
	pthread_mutex_unlock(&lock_);

	return ret;
}
#endif

int Camera::DeinitCamera()
{
    int ret = 0;
    if (eyesee_camera_ == NULL) return 0;

#ifdef GUI_SUPPORT
    StopPreview();
#endif

    pthread_mutex_lock(&lock_);
    // 1. deinit channel
    ret = eyesee_camera_->stopChannel(main_enc_chn_);
    if (ret != NO_ERROR) {
        db_error("stop channel[%d] failed", main_enc_chn_);
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    ret = eyesee_camera_->releaseChannel(main_enc_chn_);
    if (ret != NO_ERROR) {
        db_error("release channel[%d] failed", main_enc_chn_);
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    ret = eyesee_camera_->closeChannel(main_enc_chn_);
    if (ret != NO_ERROR) {
        db_error("close channel[%d] failed", main_enc_chn_);
        pthread_mutex_unlock(&lock_);
        return -1;
    }

    // 2. deinit device
    ret = eyesee_camera_->stopDevice();
    if (ret != NO_ERROR) {
        db_error("stop device failed");
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    ret = eyesee_camera_->releaseDevice();
    if (ret != NO_ERROR) {
        db_error("release device failed");
        pthread_mutex_unlock(&lock_);
        return -1;
    }
    pthread_mutex_unlock(&lock_);
    #ifdef FACE_DETECT
	if(sub_enc_chn_ == FACE_CHN)
		DeinitCameraFace();
	#endif
    pthread_mutex_lock(&lock_);

    cam_status_ = CAM_OPENED;
	m_dispHandle = -1;

    pthread_mutex_unlock(&lock_);

    return 0;
}

void Camera::GetCameraInitParam(CameraInitParam &init_param)
{
    init_param = init_param_;
}

EyeseeCamera* Camera::GetEyeseeCamera(void) const
{
    return eyesee_camera_;
}

PhysicalCameraID Camera::GetCameraID() const
{
    return phy_cam_id_;
}

#ifdef GUI_SUPPORT
void Camera::ShowPreview(void)
{
    db_msg("open layer to show preview m_dispHandle:%d cam_status_:%d phy_cam_id_:%d",m_dispHandle,cam_status_,phy_cam_id_);
	pthread_mutex_lock(&lock_);
    if (cam_status_ == CAM_PREVIEWING_PAUSE) {
        int layer_id = phy_cam_id_;
		if( m_dispHandle == -1 )
		{
			m_dispHandle = this->InitDisplay(phy_cam_id_);
			if( phy_cam_id_ == CAM_B ){
				eyesee_camera_->setChannelDisplay(sub_enc_chn_, m_dispHandle);
                layer_->SetLayerZorder(LAYER_CAM1, zorder_);
			} else {
				eyesee_camera_->setChannelDisplay(sub_enc_chn_, m_dispHandle);
                layer_->SetLayerZorder(LAYER_CAM0, zorder_);
			}
		}
		else
		{
			if( layer_ != NULL)
				layer_->SetLayerZorder(LAYER_CAM1, 2);
		}
		status_t renderRet;
		if( phy_cam_id_ == CAM_B )
	        renderRet = eyesee_camera_->startRender(sub_enc_chn_);
		else
	        renderRet = eyesee_camera_->startRender(sub_enc_chn_);
        if(NO_ERROR == renderRet)
        {
            int ret = cdx_sem_down_timedwait(&mSemRenderStart, PREVIEW_WAIT_TIME);
            if(ETIMEDOUT == ret)
            {
                db_error("fatal error! wait render start timeout");
            }
        }
        else if(ALREADY_EXISTS == renderRet)
        {
            db_msg("camera already show preview");
        }
        else
        {
            db_error("fatal error! start render fail!");
        }
        layer_->OpenLayer(layer_id);
        cam_status_ = CAM_PREVIEWING;
		pthread_mutex_unlock(&lock_);
    } else if (cam_status_ == CAM_INITED) {
        //db_msg("####################open layer to show video preview");
		pthread_mutex_unlock(&lock_);
        this->StartPreview();
    } else {
        db_warn("wrong status: %d", cam_status_);
		pthread_mutex_unlock(&lock_);
    }
}

void Camera::HidePreview(void)
{
    db_msg("close layer to hide video preview");
    pthread_mutex_lock(&lock_);
    if (cam_status_ == CAM_PREVIEWING) {
		int ret;
		if( phy_cam_id_ == CAM_B )
		{
			ret = eyesee_camera_->stopRender(sub_enc_chn_);
			if (ret != NO_ERROR)
			{
				db_error("stop render failed, chn[%d]", sub_enc_chn_);
			}
		}
		else
		{
	        ret = eyesee_camera_->stopRender(sub_enc_chn_);
			if (ret != NO_ERROR)
			{
				db_error("stop render failed, chn[%d]", sub_enc_chn_);
			}
		}
        int layer_id = phy_cam_id_;
     //   db_warn("close layer to hide video preview layer_id  = %d",layer_id);
        layer_->CloseLayer(layer_id);
        layer_->ReleaseLayer(layer_id);
        m_dispHandle = -1;
//        eyesee_camera_->setChannelDisplay(channel_id_, -1);
        cam_status_ = CAM_PREVIEWING_PAUSE;
    }
    pthread_mutex_unlock(&lock_);
}

void Camera::StartPreview(bool p_OnlyRestartChannel)
{
    int ret;
    struct timeval tv;
    pthread_mutex_lock(&lock_);
    #if 0
	if( phy_cam_id_ == CAM_B )
	{
		if( m_dispHandle == -1)
			m_dispHandle = this->InitDisplay(phy_cam_id_);
	    eyesee_camera_->setChannelDisplay(main_enc_chn_, m_dispHandle);

		ret = eyesee_camera_->stopRender(main_enc_chn_);
		if (ret != NO_ERROR) {
			db_error("stopRender error!");
			pthread_mutex_unlock(&lock_);
			return;
		}
		
		usleep(250*1000);
		
		ret = eyesee_camera_->startRender(main_enc_chn_);
		if (ret != NO_ERROR) {
			db_error("startRender error!");
			pthread_mutex_unlock(&lock_);
			return;
		}
		ret = cdx_sem_down_timedwait(&mSemRenderStart, PREVIEW_WAIT_TIME);
		if(ETIMEDOUT == ret)
		{
			db_error("fatal error! wait render start timeout");
		}
		int layer_id = phy_cam_id_;
		layer_->OpenLayer(layer_id);
		
		cam_status_ = CAM_PREVIEWING;

		pthread_mutex_unlock(&lock_);
		return ;
	}
#endif
    ret = eyesee_camera_->openChannel(init_param_.sub_chn_,false);
    if (ret != NO_ERROR) {
        db_error("open channel error!, chn: %d", init_param_.sub_chn_);
		pthread_mutex_unlock(&lock_);
        return ;
    }
    // 5. set new eyesee camera parameters
    if(sub_enc_chn_ == -1) {
        eyesee_camera_->getParameters(init_param_.sub_chn_, m_sub_cam_cfg);
        sub_enc_chn_ = init_param_.sub_chn_;
    }
    init_param_.cam_param_ = m_sub_cam_cfg;
    init_param_.cam_param_.setVideoSize(init_param_.sub_venc_size_);
    init_param_.cam_param_.setPictureSize(init_param_.sub_penc_size_);
    init_param_.cam_param_.setPreviewFrameRate(init_param_.framate);
    init_param_.cam_param_.setVideoBufferNumber(init_param_.buffernumber);
    init_param_.cam_param_.setPictureMode(TAKE_PICTURE_MODE_FAST);
    init_param_.cam_param_.setJpegThumbnailSize(SIZE_S{0, 0});
    init_param_.cam_param_.setPreviewFormat(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
    init_param_.cam_param_.setColorSpace(V4L2_COLORSPACE_REC709);	//V4L2_COLORSPACE_JPEG
    init_param_.cam_param_.setZoom(0);
    init_param_.cam_param_.setPreviewRotation(m_degree);
    init_param_.cam_param_.setDisplayFrameRate(30);
    db_msg("sub_enc_chn_ : init_param_.vflip = %d,init_param_.mirror = %d",init_param_.vflip,init_param_.mirror);
	//init_param_.vflip= 0;
    //init_param_.mirror=0;
    init_param_.cam_param_.SetFlip(init_param_.vflip);
    init_param_.cam_param_.SetMirror(init_param_.mirror);
    vflip = init_param_.vflip;
    mirror = init_param_.mirror;

    ret = eyesee_camera_->setParameters(sub_enc_chn_, init_param_.cam_param_);
    if (ret != 0) {
        db_error("setParameters error!");
		pthread_mutex_unlock(&lock_);
        return ;
    }
	eyesee_camera_->KeepPictureEncoder(sub_enc_chn_, false);
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", "prepare channel", tv.tv_sec, tv.tv_usec);
    ret = eyesee_camera_->prepareChannel(sub_enc_chn_);
    if (ret != 0) {
        db_error("prepareChannel error!, chn: %d", sub_enc_chn_);
		pthread_mutex_unlock(&lock_);
        return ;
    }
	if( !p_OnlyRestartChannel)
	{
		if( m_dispHandle == -1)
		    m_dispHandle = this->InitDisplay(phy_cam_id_);
	    eyesee_camera_->setChannelDisplay(sub_enc_chn_, m_dispHandle);
	    ret = eyesee_camera_->stopRender(sub_enc_chn_);
	    if (ret != NO_ERROR) {
	        db_error("stopRender error!");
	        pthread_mutex_unlock(&lock_);
	        return;
	    }
	}

    ret = eyesee_camera_->startChannel(sub_enc_chn_);
    if (ret != 0) {
        db_error("startChannel error!");
        pthread_mutex_unlock(&lock_);
        return;
    }

    usleep(250*1000);


	if( !p_OnlyRestartChannel)
	{
	    ret = eyesee_camera_->startRender(sub_enc_chn_);
	    if (ret != NO_ERROR) {
	        pthread_mutex_unlock(&lock_);
	        return;
	    }
	}

#if 0
    ret = cdx_sem_down_timedwait(&mSemRenderStart, PREVIEW_WAIT_TIME);
    if(ETIMEDOUT == ret)
    {
        db_warn("fatal error! wait render start timeout");
    }
#endif

	if( !p_OnlyRestartChannel)
	{
	    int layer_id = phy_cam_id_;
	    layer_->OpenLayer(layer_id);

	    gettimeofday(&tv, NULL);
	    fprintf(stderr, "%s time: %lds.%ldms\n", "show preview", tv.tv_sec, tv.tv_usec);

	    cam_status_ = CAM_PREVIEWING;
	    pthread_mutex_unlock(&lock_);

	    this->Notify(MSG_CAMERA_START_PREVIEW);
	}
	else
	{
		cam_status_ = CAM_PREVIEWING_PAUSE;
	}
	pthread_mutex_unlock(&lock_);
}

void Camera::StopPreview(void)
{
    int ret = 0;
    pthread_mutex_lock(&lock_);
    db_msg("stopPreview %d", phy_cam_id_);
    // if (cam_status_ != CAM_PREVIEWING && cam_status_ != CAM_PREVIEWING_PAUSE) {
        // db_msg("preview is already stoped");
        // pthread_mutex_unlock(&lock_);
        // return;
    // }

    if (cam_status_ == CAM_PREVIEWING || cam_status_ == CAM_PREVIEWING_PAUSE) {
        ret = eyesee_camera_->stopChannel(sub_enc_chn_);
        if (ret != NO_ERROR) {
            db_error("stop channel[%d] failed", sub_enc_chn_);
            pthread_mutex_unlock(&lock_);
            return;
        }
    }

    db_debug("current status: %d", cam_status_);
    ret = eyesee_camera_->releaseChannel(sub_enc_chn_);
    if (ret != NO_ERROR) {
        db_error("release channel[%d] failed", sub_enc_chn_);
        pthread_mutex_unlock(&lock_);
        return;
    }

    ret = eyesee_camera_->closeChannel(sub_enc_chn_);
    if (ret != NO_ERROR) {
        db_error("close channel[%d] failed", sub_enc_chn_);
        pthread_mutex_unlock(&lock_);
        return;
    }

    int layer_id = phy_cam_id_;
    //layer_->CleanLayer(layer_id);
    if (layer_){
        layer_->CloseLayer(layer_id);
        layer_->ReleaseLayer(layer_id);
    }
    m_dispHandle = -1;

    cam_status_ = CAM_INITED;
    pthread_mutex_unlock(&lock_);
    this->Notify(MSG_CAMERA_STOP_PREVIEW);
}
#endif

int Camera::GetMainEncChn(void) const
{
    return main_enc_chn_;
}

int Camera::GetSubEncChn(void) const
{
    return sub_enc_chn_;
}

/*void Camera::SetVideoCaptureBufferCnt(int cnt)
{
    SET_CAMERA_PARAMS(setVideoCaptureBufferNum, cnt);
}

int Camera::GetVideoCaptureBufferCnt(void) const
{
    GET_CAMERA_PARAMS(getVideoCaptureBufferNum);
}*/


void Camera::SetVideoFileForThumb(MediaFile *mf)
{
	pthread_mutex_lock(&thumb_pic_lock);
    video_thumb_pic_ = mf;

	pthread_mutex_unlock(&thumb_pic_lock);
}

void Camera::SetVideoResolution(int width, int height)
{
    SIZE_S size;

    size.Width = width;
    size.Height = height;

    SET_CAMERA_PARAMS(setVideoSize, size);
}

void Camera::ReSetVideoResolutionForNormalphoto(PIXEL_FORMAT_E type)
{
    eyesee_camera_->getParameters(main_enc_chn_, m_main_cam_cfg);
    eyesee_camera_->getParameters(sub_enc_chn_, m_sub_cam_cfg);
	db_error("--------------------------------ReSetVideoResolutionForNormalphoto %d", (int)type);
    DeinitCamera();
    usleep(250 * 1000);
    SIZE_S venc_size;
    SIZE_S penc_size;
    int fps;
    int buffercnt;
	#ifdef USE_IMX335
	venc_size.Width = 2560;
    venc_size.Height = 1440;
	#else
    venc_size.Width = 3840;
    venc_size.Height = 2160;
	#endif
    fps = 25;
    buffercnt = 5;
    m_main_cam_cfg.getPictureSize(penc_size);
    CameraInitParam &param = init_param_;
    param.main_penc_size_ = penc_size;
    param.main_venc_size_ = venc_size;
    param.sub_penc_size_  = {640, 360};

    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    layer_->GetDisplayDeviceType(&disp_type, &tv_mode);
    if (disp_type == VO_INTF_LCD)
    {
		if (SCREEN_HEIGHT > SCREEN_WIDTH)
			param.sub_venc_size_  = {SCREEN_HEIGHT,SCREEN_WIDTH};
		else
			param.sub_venc_size_  = {SCREEN_WIDTH,SCREEN_HEIGHT};

        param.sub_penc_size_  = {SCREEN_WIDTH,SCREEN_HEIGHT};
    }
    else if (disp_type == VO_INTF_HDMI)
    {
        switch (tv_mode) {
            case VO_OUTPUT_3840x2160_25:
            case VO_OUTPUT_3840x2160_30:
                {
                    if (param.main_venc_size_.Width > 2560 || param.main_venc_size_.Height > 1440) {
                        param.sub_venc_size_  = {2560, 1440};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            case VO_OUTPUT_1080P24:
            case VO_OUTPUT_1080P25:
            case VO_OUTPUT_1080P30:
            case VO_OUTPUT_1080P60:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            default:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
        }
    }
    param.main_chn_ = main_enc_chn_;
    param.sub_chn_ = sub_enc_chn_;
    param.framate = fps;
    param.buffernumber = buffercnt;
    param.mirror = mirror;
    param.vflip = vflip;
    param.pixel_fmt_ = type;
    InitCamera(param);
    // StartPreview();
    this->Notify(MSG_CAMERA_SET_PHOTO);
}

void Camera::SetSlowVideoResloution(int resolution)
{
    eyesee_camera_->getParameters(main_enc_chn_, m_main_cam_cfg);
    eyesee_camera_->getParameters(sub_enc_chn_, m_sub_cam_cfg);
    SIZE_S size;
    SIZE_S penc_size;
    int fps;
    int buffercnt;
    DeinitCamera();
    usleep(250 * 1000);
    switch(resolution)
    {
        case 0:
            size.Width = 1920;
            size.Height = 1080;
            fps = 120;
            buffercnt = 20;
            break;
        case 1:
            size.Width = 1920;
            size.Height = 1080;
            fps = 60;
            buffercnt = 10;
            break;
        case 2:
            size.Width = 1280;
            size.Height = 536;
            fps = 240;
            buffercnt = 40;
            break;
        case 3:
            size.Width = 1280;
            size.Height = 720;
            fps = 120;
            buffercnt = 30;
            break;
        default:
            db_error("not support image_quality:%d", resolution);
            return ;
    }
    m_main_cam_cfg.getPictureSize(penc_size);
    CameraInitParam &param = init_param_;
    param.main_penc_size_ = penc_size;
    param.main_venc_size_ = size;
    param.sub_penc_size_  = {640, 360};

    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    layer_->GetDisplayDeviceType(&disp_type, &tv_mode);
    if (disp_type == VO_INTF_LCD)
    {
		if (SCREEN_HEIGHT > SCREEN_WIDTH)
			param.sub_venc_size_  = {SCREEN_HEIGHT,SCREEN_WIDTH};
		else
			param.sub_venc_size_  = {SCREEN_WIDTH,SCREEN_HEIGHT};

        param.sub_penc_size_  = {SCREEN_WIDTH, SCREEN_HEIGHT};
    }
    else if (disp_type == VO_INTF_HDMI)
    {
        switch (tv_mode) {
            case VO_OUTPUT_3840x2160_25:
            case VO_OUTPUT_3840x2160_30:
                {
                    if (param.main_venc_size_.Width > 2560 || param.main_venc_size_.Height > 1440) {
                        param.sub_venc_size_  = {2560, 1440};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            case VO_OUTPUT_1080P24:
            case VO_OUTPUT_1080P25:
            case VO_OUTPUT_1080P30:
            case VO_OUTPUT_1080P60:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            default:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
        }
    }

    param.main_chn_ = main_enc_chn_;
    param.sub_chn_ = sub_enc_chn_;
    param.framate = fps;
    param.buffernumber = buffercnt;
    param.mirror = mirror;
    param.vflip = vflip;
    param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    if (param.framate > 60) {
        param.sub_venc_size_  = {640,360};
        param.sub_penc_size_  = {640,360};
    }
    InitCamera(param);
    // StartPreview();
    this->Notify(MSG_CAMERA_SLOW_RESOULATION);
}

void Camera::SetVideoResolution(int resolution)
{
    CameraInitParam &param = init_param_;
    eyesee_camera_->getParameters(main_enc_chn_, m_main_cam_cfg);
    eyesee_camera_->getParameters(sub_enc_chn_, m_sub_cam_cfg);
	int m_onlyRestartChannel = false;
	if(cam_status_ != CAM_PREVIEWING )
		m_onlyRestartChannel = true;
	db_error("--------------------------------SetVideoResolution %d", resolution);
   // eyesee_camera_->getParameters(main_enc_chn_, params);
    SIZE_S size;
    SIZE_S penc_size;
    int fps;
    int buffercnt;
    DeinitCamera();
    usleep(250 * 1000);
	#ifdef USE_IMX335
    param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
	#else
	param.pixel_fmt_ = MM_PIXEL_FORMAT_YUV_AW_LBC_2_5X;
	#endif
    switch(resolution)
    {
        case VIDEO_QUALITY_4K30FPS:
			#ifdef USE_IMX335
			size.Width = 2560/*2592*/;
            size.Height = 1440/*1944*/;
            fps = 25;//25;
			#else
            size.Width = 3840;
            size.Height = 2160;
            fps = 25;
			#endif
            buffercnt = 5;
			OsdManager::get()->setTimeOsdPostion(3056-96, 2032);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(3056-96, 2032 - 96);
			OsdManager::get()->setCaridOsdPosition(3696, 2032 - 96);	// right align
            break;
	    case VIDEO_QUALITY_2_7K30FPS:
			#ifdef USE_IMX335
            size.Width = 2560;
            size.Height = 1440;
			#else
			size.Width = 2688;
            size.Height = 1520;
			#endif
            fps = 25;//25;
            buffercnt = 5;
			OsdManager::get()->setTimeOsdPostion(1904-96, 1408);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(1904-96, 1408 - 96);
			OsdManager::get()->setCaridOsdPosition(2528, 1408 - 96);
            break;
        case VIDEO_QUALITY_1080P120FPS:
            size.Width = 1920;
            size.Height = 1080;
            fps = 120;
            buffercnt = 20;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_1080P60FPS:
            size.Width = 1920;
            size.Height = 1080;
            fps = 60;
            buffercnt = 10;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_1080P30FPS:
            size.Width = 1920;
            size.Height = 1080;
            fps = 25;//25;
            buffercnt = 5;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_720P240FPS:
            size.Width = 1280;
            size.Height = 536;
            fps = 240;
            buffercnt = 40;
            param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
			OsdManager::get()->setTimeOsdPostion(560, 472);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 472 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 472 - 64 -64);
            break;
        case VIDEO_QUALITY_720P120FPS:
            size.Width = 1280;
            size.Height = 720;
            fps = 120;
            buffercnt = 30;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 472 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 472 - 64 -64);
            break;
        case VIDEO_QUALITY_720P60FPS:
            size.Width = 1280;
            size.Height = 720;
            fps = 60;
            buffercnt = 10;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 472 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 472 - 64 -64);
            break;
        case VIDEO_QUALITY_720P30FPS:
            size.Width = 1280;
            size.Height = 720;
            fps = 30;
            buffercnt = 5;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 472 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 472 - 64 -64);
            break;
        default:
            db_error("not support image_quality:%d, use 4K30FPS as default", resolution);
			#ifdef USE_IMX335
			size.Width = 2560;
            size.Height = 1440;
            fps = 25;
			#else
            size.Width = 3840;
            size.Height = 2160;
            fps = 25;
			#endif
            buffercnt = 5;
			OsdManager::get()->setTimeOsdPostion(3056-96, 2032);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(3056-96, 2032 - 96);
			OsdManager::get()->setCaridOsdPosition(3696, 2032 - 96);
            return ;
    }


    db_info("SetVideoResolution %d %d %d",size.Width,size.Height,fps);
    #if 1
	MenuConfigLua *menu_config = MenuConfigLua::GetInstance();
	int val = menu_config->GetMenuIndexConfig(SETTING_PHOTO_RESOLUTION);
    switch(val) {
        case PIC_RESOLUTION_8M:
            {
                param.main_penc_size_ = {3840,2160};
            }
            break;
        case PIC_RESOLUTION_13M:
            {
                param.main_penc_size_ = {4800,2700};
            }
            break;
        case PIC_RESOLUTION_16M:
            {
                param.main_penc_size_ = {5440,3060};
            }
            break;
        default:
            db_error("not support PicResolution:%d", val);
			param.main_penc_size_ = {4800,2700};
			break;
    }
    m_main_cam_cfg.getPictureSize(penc_size);
    param.main_penc_size_ = penc_size;
    param.main_venc_size_ = size;
    param.sub_penc_size_  = {640,360};

    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    layer_->GetDisplayDeviceType(&disp_type, &tv_mode);
    if (disp_type == VO_INTF_LCD)
    {
		if (SCREEN_HEIGHT > SCREEN_WIDTH)
			param.sub_venc_size_  = {SCREEN_HEIGHT,SCREEN_WIDTH};
		else
			param.sub_venc_size_  = {SCREEN_WIDTH,SCREEN_HEIGHT};
    }
    else if (disp_type == VO_INTF_HDMI)
    {
        #if 1
        switch (tv_mode) {
            case VO_OUTPUT_3840x2160_25:
            case VO_OUTPUT_3840x2160_30:
                {
                    if (param.main_venc_size_.Width > 2560 || param.main_venc_size_.Height > 1440) {
                        param.sub_venc_size_  = {2560, 1440};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            case VO_OUTPUT_1080P24:
            case VO_OUTPUT_1080P25:
            case VO_OUTPUT_1080P30:
            case VO_OUTPUT_1080P60:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
            default:
                {
                    if (param.main_venc_size_.Width > 1920 || param.main_venc_size_.Height > 1080) {
                        param.sub_venc_size_  = {1920, 1080};
                    } else {
                        param.sub_venc_size_  = param.main_venc_size_;
                    }
                }
                break;
        }
        #endif
    }

    param.main_chn_ = main_enc_chn_;
    param.sub_chn_ = sub_enc_chn_;
    param.framate = fps;
    param.buffernumber = buffercnt;
    param.mirror = mirror;
    param.vflip = vflip;
    if (param.framate > 60) {
        param.sub_venc_size_  = {640,360};
        param.sub_penc_size_  = {640,360};
    }
    InitCamera(param);
    StartPreview(m_onlyRestartChannel);
#if 1
	val = GetModeConfigIndex(SETTING_MOTION_DETECT);
	if (!val) {
		stopMotionDetect();	
	} else {
		startMotionDetect();
	}
#endif
    #endif
    this->Notify(MSG_CAMERA_VIDEO_RESOLUTION);
}

void Camera::GetVideoResolution(Size &size) const
{
    SIZE_S _size;
    GET_CAMERA_PARAMS_(getVideoSize, _size);
    size.width = _size.Width;
    size.height = _size.Height;
}

void Camera::SetVideoFPS(int fps)
{
    SET_CAMERA_PARAMS(setPreviewFrameRate, fps);
    this->Notify(MSG_CAMERA_VIDEO_FPS);
}

int Camera::GetVideoFPS(void) const
{
    GET_CAMERA_PARAMS(getPreviewFrameRate);

    return 0;
}

int Camera::CheckStorage(char* reNamePhotofile)
{
	int delete_flag = 0;
	int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR)  && (status != FORMATTING))
	{
        bool fullStatus;
		std::string QueryType;
		std::string file_name;
        db_msg("Camera CheckStorage camid = %d",phy_cam_id_);
		if( phy_cam_id_ == CAM_B )
		{
            db_msg("Check photo B dir is full!");
            fullStatus = StorageManager::GetInstance()->CheckPhotoBDirFull();
            QueryType = "photo_B";
		}
        else
        {	// CAM_A        
            db_msg("Check photo A dir is full!");
            fullStatus = StorageManager::GetInstance()->CheckPhotoADirFull();
            QueryType = "photo_A";
        }        
		db_error("Camera Photo fullStatus is: %d",fullStatus);
        while(fullStatus)
        {
             
             MediaFileManager::GetInstance()->GetLastFileByType(QueryType, 0, file_name);
             MediaFileManager::GetInstance()->DeleteLastFilesDatabaseIndex(file_name, 0);
             db_msg("Camera CheckStorage get the last photo file name is：%s",file_name.c_str());
              if(reNamePhotofile != NULL && !file_name.empty()){
                  strncpy(reNamePhotofile, file_name.c_str(), sizeof(char)*64);
                  db_warn("sd card is full, get Photo rename file name %s",reNamePhotofile);
              }else{
                  db_error("file name is empty!!!");
              }
              break;
        }
		storage_status_ = STORAGE_OK;
		return 0;
	}
	else
	{
        storage_status_ = STORAGE_NOT_OK;
        db_error("sd card is not ok, can not start record");
		return -1;
	}		
}

int Camera::TakePicture(int chn)
{
    int ret = 0;
    takepic_done_ = false;
    #if 0
    ret = CheckStorage();
    if(ret < 0){
        db_msg("do take picture falied tf card");
        this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        return ret;
    }
#endif
    if (storage_status_ == STORAGE_OK) {
        db_warn("do take picture chn: %d main_enc_chn_:%d sub_enc_chn_:%d",chn, main_enc_chn_,sub_enc_chn_);
        int ret = NO_ERROR;
        if(chn == main_enc_chn_){
            int main_ret = eyesee_camera_->takePicture(main_enc_chn_, this, NULL, this, this);
            if (main_ret != NO_ERROR) ret = main_ret;
            //int sub_ret = eyesee_camera_->takePicture(sub_enc_chn_, this, NULL, this, NULL);
            //if (sub_ret != NO_ERROR) ret = sub_ret;
            #if 0
            ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
            eyesee_camera_->takePicture(main_enc_chn_, NULL, NULL, this);
            ret = cdx_sem_down_timedwait(&mTakePhotoOver, 1000);
            if(ETIMEDOUT == ret)
            {
                db_error("fatal error! wait render start timeout");
            }
            ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YUV_AW_AFBC);
            #endif
        }else if(chn == sub_enc_chn_){
            ret = eyesee_camera_->takePicture(sub_enc_chn_, this, NULL, this, NULL);
        }
        pthread_mutex_lock(&lock_);
        takepic_done_ = true;
        pthread_mutex_unlock(&lock_);

        if (ret != NO_ERROR) {
            db_warn("take pic failed! current cam status: %d chn:%d, main_enc_chn_:%d sub_enc_chn_:%d", cam_status_,chn,main_enc_chn_,sub_enc_chn_);
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        }
    } else {
        db_warn("current cam status: %d", cam_status_);
        this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
    }

    return ret;
}

/*
	pictype 0=Normal	1=thumb		2=snapshot
*/
#define _TAKE_PHOTO_NORMAL		0
#define _TAKE_PHOTO_THUMB		1
#define _TAKE_PHOTO_SNAPSHOT	2


int Camera::TakePictureEx(int chn, int pictype)
{
	int ret = 0;
	takepic_done_ = false;
    #if 0
	ret = CheckStorage();
	if(ret < 0){
		db_msg("do take picture falied tf card");
		this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
		return ret;
	}
    #endif
	if (storage_status_ == STORAGE_OK)
	{
		//db_warn("do take picture 11 chn = %d main_enc_chn_ = %d",chn, main_enc_chn_);
		int ret = NO_ERROR;
		db_warn("do take picture ex chn: %d main_enc_chn_:%d sub_enc_chn_:%d",chn, main_enc_chn_,sub_enc_chn_);
		if(chn == main_enc_chn_)
		{
			if( pictype==_TAKE_PHOTO_THUMB ) {		// TAKE_PHOTO_THUMB
				SetPicResolution(640, 360);
			}
			else if( pictype==_TAKE_PHOTO_NORMAL ) 	// TAKE_PHOTO_NORMAL
			{
				//ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
				//init_param_.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
				//init_param_.cam_param_.setPreviewFormat(init_param_.pixel_fmt_);
				int val = GetModeConfigIndex(SETTING_PHOTO_RESOLUTION);
				db_error("SETTING_PHOTO_RESOLUTION: %d",val);
				switch(val) {
			        case PIC_RESOLUTION_8M:
			            {
			                init_param_.main_penc_size_= {3840,2160};
			            }
			            break;
			        case PIC_RESOLUTION_13M:
			            {
			                init_param_.main_penc_size_ = {4800,2700};
			            }
			            break;
			        case PIC_RESOLUTION_16M:
			            {
			                init_param_.main_penc_size_ = {5440,3060};
			            }
			            break;
			        default:
			            db_error("not support PicResolution:%d", val);
						init_param_.main_penc_size_ = {4800,2700};
						break;
			    }
    			m_main_cam_cfg.setPictureSize(init_param_.main_penc_size_);
				SetPicResolution(init_param_.main_penc_size_.Width, init_param_.main_penc_size_.Height);
				// pic size 和 video size 一样就不用ReSetVideoResolutionForNormalphoto
				//SetPicResolution(5376, 3024);	// NG
				//SetPicResolution(3840, 2160);	// OK
				//SetPicResolution(1920, 1080);	// OK

				//SetVideoResolution(VIDEO_QUALITY_4K30FPS);
				db_error("set photo normal: %d %d",init_param_.main_penc_size_.Width, init_param_.main_penc_size_.Height);
			}
			else if( pictype==_TAKE_PHOTO_SNAPSHOT ) 	//TAKE_PHOTO_SNAPSHOT
			{
				SetPicResolution(init_param_.main_venc_size_.Width, init_param_.main_venc_size_.Height);
				//SetPicResolution(5440, 3060);
				db_error("set photo sanpshot: %d %d",init_param_.main_venc_size_.Width, init_param_.main_venc_size_.Height);
			}
			ShowPreview();	
            //db_warn("do take picture 22 width =%d   height =%d",init_param_.main_venc_size_.Width,init_param_.main_venc_size_.Height);
			int main_ret = eyesee_camera_->takePicture(main_enc_chn_, this, NULL, this, this);
			if (main_ret != NO_ERROR)
				ret = main_ret;
		}
		else if(chn == sub_enc_chn_)
		{
			ret = eyesee_camera_->takePicture(sub_enc_chn_, this, NULL, this, NULL);
		}
       // db_warn("do take picture 3333");
		pthread_mutex_lock(&lock_);
		takepic_done_ = true;
		pthread_mutex_unlock(&lock_);

		if (ret != NO_ERROR)
		{
			db_warn("take pic failed! current cam status: %d chn:%d, main_enc_chn_:%d sub_enc_chn_:%d", cam_status_,chn,main_enc_chn_,sub_enc_chn_);
			this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
		}
	}
	else
	{
		db_warn("current cam status: %d", cam_status_);
		this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
	}

	return ret;
}

void Camera::addPictureRegion(std::list<PictureRegionType> &rPictureRegionList)
{
    rPictureRegionList.clear();

    if (osd_enable_) {
        BITMAP_S stBmp;
        memset(&stBmp, 0, sizeof(BITMAP_S));
        OsdManager::get()->GetOsdBitMap(stBmp);

        PictureRegionType Region;
        Region.mType = OVERLAY_RGN;
        Region.mRect ={0, 0, stBmp.mWidth, stBmp.mHeight};
        Region.mInfo.mOverlay.mPixFmt = stBmp.mPixelFormat;
        Region.mInfo.mOverlay.mBitmap = stBmp.mpData;
        Region.mInfo.mOverlay.mbInvColEn = 0;
        Region.mInfo.mOverlay.mPriority = 5;
        rPictureRegionList.push_back(Region);
    } else {
        db_info("picture osd is not enable");
    }

    return ;
}

void Camera::SetContinuousPictureMode(int value)
{
    CameraParameters main_chn_params, sub_chn_params;
    eyesee_camera_->getParameters(main_enc_chn_, main_chn_params);
    eyesee_camera_->getParameters(sub_enc_chn_, sub_chn_params);
    switch(value) {
        case 0:
            main_chn_params.setPictureMode(TAKE_PICTURE_MODE_FAST);
            sub_chn_params.setPictureMode(TAKE_PICTURE_MODE_FAST);
            if (!takepic_done_) {
                eyesee_camera_->cancelContinuousPicture(main_enc_chn_);
                eyesee_camera_->cancelContinuousPicture(sub_enc_chn_);
            }
            db_msg("fast mode");
            break;
        case 1:
            main_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            sub_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            main_chn_params.setContinuousPictureNumber(3);
            sub_chn_params.setContinuousPictureNumber(3);
            db_msg("continuous mode:3p");
            break;
        case 2:
            main_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            sub_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            main_chn_params.setContinuousPictureNumber(5);
            sub_chn_params.setContinuousPictureNumber(5);
            db_msg("continuous mode:5p");
            break;
        case 3:
            main_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            sub_chn_params.setPictureMode(TAKE_PICTURE_MODE_CONTINUOUS);
            main_chn_params.setContinuousPictureNumber(10);
            sub_chn_params.setContinuousPictureNumber(10);
            main_chn_params.setContinuousPictureIntervalMs(300);
            sub_chn_params.setContinuousPictureIntervalMs(300);
            db_msg("continuous mode:10p");
            break;
        default:
            return;
    }
    eyesee_camera_->setParameters(main_enc_chn_, main_chn_params);
    eyesee_camera_->setParameters(sub_enc_chn_, sub_chn_params);
    if(GetTakePicMode() == TAKE_PICTURE_MODE_CONTINUOUS){
        this->Notify(MSG_CAMERA_CONTINOUS_ON);
    }else{
        this->Notify(MSG_CAMERA_CONTINOUS_OFF);
    }
}

int Camera::GetContinuousPictureNumber()
{
     CameraParameters params;
     eyesee_camera_->getParameters(main_enc_chn_, params);
     db_msg("[debug_joson]:the ContinuousPictureNumber is %d",params.getContinuousPictureNumber());
     return params.getContinuousPictureNumber();
}

int Camera::GetTakePicMode()
{
     CameraParameters params;
     eyesee_camera_->getParameters(main_enc_chn_, params);
     db_msg("[debug_joson]:the GetTakePicMode is %d",params.getPictureMode());
     return params.getPictureMode();
}
void Camera::SetPicResolution(int resolution)
{
    SIZE_S size;
	db_error("SetPicResolution: %d",resolution);
    if(eyesee_camera_ == NULL) {
        db_error("eyesee_camera_ is NULL");
        return;
    }

    switch(resolution) {
        case PIC_RESOLUTION_8M:
            {
                size.Width = 3840;
                size.Height = 2160;
            }
            break;
        case PIC_RESOLUTION_13M:
            {
                size.Width = 4800;//4800;
                size.Height = 2700;//2700;
            }
            break;
        case PIC_RESOLUTION_16M:
            {
                size.Width = 5440;
                size.Height = 3060;
            }
            break;
        default:
            db_error("not support PicResolution:%d", resolution);
            return ;
    }
    db_info("SetPicResolution %d %d",size.Width,size.Height);
    SET_CAMERA_PARAMS(setPictureSize, size);
    this->Notify(MSG_CAMERA_PIC_RESOLUTION);
}

const unsigned int Camera::GetTimeTakePicTime() const
{
    db_debug("time_take_pic_cont_: %d", time_take_pic_cont_);
    return time_take_pic_cont_;
}

const unsigned int Camera::GetAutoTimeTakePicTime() const
{
    db_debug("auto_time_take_pic_cont_: %d", auto_time_take_pic_cont_);
    return auto_time_take_pic_cont_;
}


void Camera::SetTimeTakePic(int val)
{
      db_info("SetTimeTakePic:%d",val);
     CameraParameters params;
     eyesee_camera_->getParameters(main_enc_chn_, params);
     if(params.getPictureMode() == TAKE_PICTURE_MODE_CONTINUOUS){
        db_msg("[debug_jason]: switch the take pic mode to TAKE_PICTURE_MODE_FAST");
        params.setPictureMode(TAKE_PICTURE_MODE_FAST);
     }
     switch(val)
     {
        case 0:
            time_take_pic_cont_ = PTL_OFF;
            break;
        case 1:
            time_take_pic_cont_ = PTL_3;
            break;
        case 2:
            time_take_pic_cont_ = PTL_5;
            break;
        case 3:
            time_take_pic_cont_ = PTL_10;
            break;
        case 4:
            time_take_pic_cont_ = PTL_20;
            break;
        default:
            db_msg("[debug_jaosn]:this is invild cont");
            time_take_pic_cont_ = PTL_OFF;
            break;
     }
     if(time_take_pic_cont_ == PTL_OFF)
     {
        this->Notify(MSG_TIME_TAKE_PIC_OFF);
     }else{
        this->Notify(MSG_TIME_TAKE_PIC_ON);
     }
}

void Camera::SetAutoTimeTakePic(int val)
{
     db_info("SetAutoTimeTakePic:%d",val);
     CameraParameters params;
     eyesee_camera_->getParameters(main_enc_chn_, params);
     if(params.getPictureMode() == TAKE_PICTURE_MODE_CONTINUOUS){
        db_msg("[debug_jason]: switch the take pic mode to TAKE_PICTURE_MODE_FAST");
        params.setPictureMode(TAKE_PICTURE_MODE_FAST);
     }
     switch(val)
     {
        case 0:
            auto_time_take_pic_cont_ = PTL_OFF;
            break;
        case 1:
            auto_time_take_pic_cont_ = PTL_3;
            break;
        case 2:
            auto_time_take_pic_cont_ = PTL_10;
            break;
        case 3:
            auto_time_take_pic_cont_ = PTL_15;
            break;
        case 4:
            auto_time_take_pic_cont_ = PTL_20;
            break;
		case 5:
            auto_time_take_pic_cont_ = PTL_30;
            break;
        default:
            db_msg("[debug_jaosn]:this is invild cont");
            auto_time_take_pic_cont_ = PTL_OFF;
            break;
     }
     if(auto_time_take_pic_cont_ == PTL_OFF){
        this->Notify(MSG_AUTO_TIME_TAKE_PIC_OFF);
     }else{
        this->Notify(MSG_AUTO_TIME_TAKE_PIC_ON);
     }

}
void Camera::SetPicResolution(int width, int height)
{
    SIZE_S size;
    size.Width = width;
    size.Height = height;
    SET_CAMERA_PARAMS(setPictureSize, size);
	#if 0
	#define SET_CAMERA_PARAMS(func,...) \
	{ \
    CameraParameters params; \
    eyesee_camera_->getParameters(main_enc_chn_, params); \
    params.func(__VA_ARGS__); \
    eyesee_camera_->setParameters(main_enc_chn_, params); \
	}
	展开
	{
	CameraParameters params;
    eyesee_camera_->getParameters(main_enc_chn_, params);
    params.setPictureSize(size);
    eyesee_camera_->setParameters(main_enc_chn_, params);
	}
	#endif
    this->Notify(MSG_CAMERA_PIC_RESOLUTION);
}

void Camera::GetPicResolution(Size &size) const
{
    SIZE_S _size;
    GET_CAMERA_PARAMS_(getPictureSize, _size);
    size.height = _size.Height;
    size.width = _size.Width;
}

void Camera::SetPicQuality(int percent)
{
    int value;
    value = 100-percent*20; //60 80 100
    SET_CAMERA_PARAMS(setJpegQuality, value);
    this->Notify(MSG_CAMERA_PIC_QUALITY);
}

int Camera::GetPicQuality() const
{
    GET_CAMERA_PARAMS(getJpegQuality);
    return 0;
}

void Camera::CancelContinuousPicture()
{
    CameraParameters main_chn_params, sub_chn_params;

    eyesee_camera_->getParameters(main_enc_chn_, main_chn_params);
    eyesee_camera_->getParameters(sub_enc_chn_, sub_chn_params);

    TAKE_PICTURE_MODE_E pre_mode;

    pre_mode = main_chn_params.getPictureMode();
    if (pre_mode == TAKE_PICTURE_MODE_CONTINUOUS) {
        if (!takepic_done_)
            eyesee_camera_->cancelContinuousPicture(main_enc_chn_);
    }

    pre_mode = sub_chn_params.getPictureMode();
    if (pre_mode == TAKE_PICTURE_MODE_CONTINUOUS) {
        if (!takepic_done_)
            eyesee_camera_->cancelContinuousPicture(sub_enc_chn_);
    }
}

void Camera::SetPictureMode(int mode)
{
    CameraParameters main_chn_params, sub_chn_params;

    eyesee_camera_->getParameters(main_enc_chn_, main_chn_params);
    eyesee_camera_->getParameters(sub_enc_chn_, sub_chn_params);

    TAKE_PICTURE_MODE_E pre_mode;

    pre_mode = main_chn_params.getPictureMode();
    if (pre_mode == TAKE_PICTURE_MODE_CONTINUOUS) {
        if (!takepic_done_)
            eyesee_camera_->cancelContinuousPicture(main_enc_chn_);
    }

    pre_mode = sub_chn_params.getPictureMode();
    if (pre_mode == TAKE_PICTURE_MODE_CONTINUOUS) {
        if (!takepic_done_)
            eyesee_camera_->cancelContinuousPicture(sub_enc_chn_);
    }

    sub_chn_params.setPictureMode((TAKE_PICTURE_MODE_E)mode);
    main_chn_params.setPictureMode((TAKE_PICTURE_MODE_E)mode);

    eyesee_camera_->setParameters(main_enc_chn_, main_chn_params);
    eyesee_camera_->setParameters(sub_enc_chn_, sub_chn_params);
}

void Camera::SetWhiteBalance(int value)
{
    db_info("SetWhiteBalance:%d", value);
    int balance_val = 0;
    switch(value)
    {
        case 0:
            balance_val = 0;
            break;
        case 1:
            balance_val = 6;
            break;
        case 2:
            balance_val = 9;
            break;
        case 3:
            balance_val = 2;
            break;
        case 4:
            balance_val = 3;
            break;
    }

    /*
     * balance_val
     * 1, V4L2_WHITE_BALANCE_MANUAL
     * 0, V4L2_WHITE_BALANCE_AUTO, 自动
     * 2, V4L2_WHITE_BALANCE_INCANDESCENT, 白炽   
     * 3, V4L2_WHITE_BALANCE_FLUORESCENT, 荧光
     * 4, V4L2_WHITE_BALANCE_FLUORESCENT_H, 荧光H
     * 5, V4L2_WHITE_BALANCE_HORIZON,
     * 6, V4L2_WHITE_BALANCE_DAYLIGHT, 晴天,日光
     * 7, V4L2_WHITE_BALANCE_FLASH, 闪光
     * 8, V4L2_WHITE_BALANCE_CLOUDY, 多云
     * 9, V4L2_WHITE_BALANCE_SHADE, 阴天
    */
    SET_ISP_PARAMS(ChnIspAwb_SetMode, balance_val);
    m_whiteBalance = balance_val;
    this->Notify(MSG_CAMERA_WHITE_BALANCE);
}

const int Camera::GetWhiteBalance(void) const
{
    db_info("SetWhiteBalance");
    int val = -1, index = -1;
    GET_ISP_PARAMS_(ChnIspAwb_GetMode, val);
    switch (val) {
        case 0:
            index = 0;
        case 6:
            index = 1;
        case 9:
            index = 2;
        case 2:
            index = 3;
        case 3:
            index = 4;
        default:
            index = -1;
    }

    return index;
}

void Camera::SetContrastValue(int value)
{
#if 0
    db_msg("contrast:%d", value);
    value = 25*value;
    SET_CAMERA_PARAMS(setContrastValue, value);
#endif
    this->Notify(MSG_CAMERA_CONTRAST);
}

int Camera::GetContrastValue(void) const
{
#if 0
    GET_CAMERA_PARAMS(getContrastValue);
#endif
    return 0;
}

void Camera::SetExposureValue(int value)
{
    int map[] {
       // 0, 1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7,
    };
    m_ExposureBias = map[value];
    db_msg("=====m_ExposureBias is %d=====",map[value]);
    SET_ISP_PARAMS(ChnIspAe_SetExposureBias, m_ExposureBias);
}

int Camera::GetExposureValue(void) const
{
    GET_ISP_PARAMS(ChnIspAe_GetExposureBias);
    return 0;
}

void Camera::SetLightFreq(int index)
{
    int freq = 0;

    switch (index) {
        case 0: //50Hz
            freq = 1;
            break;
        case 1: //60Hz
            freq = 2;
            break;
        default:
            freq = 1;
            break;
    }

    db_msg("Camera::SetLightFreq, index: %d, flcker value: %d", index, freq);

    SET_ISP_PARAMS(ChnIsp_SetFlicker,freq);
    m_LightFreq = freq;
}

int Camera::GetLightFreq(void) const
{
    int index = 0;
    CameraParameters params;

    eyesee_camera_->getISPParameters(params);
    int freq = params.ChnIsp_GetFlicker();

    switch (freq) {
        case 3:
            index = 0;
            break;
        case 1:
            index = 1;
            break;
        case 2:
            index = 2;
            break;
        default:
            break;
    }

    return index;
}

void Camera::SetPreviewFlip(int flip)
{
#if 0
    db_info("Camera::SetPreviewFlip");
    int isFlip;
    EyeseeCamera::Parameters params;
    eyesee_camera_->getParameters(&params);

    switch (flip) {
        case NO_FLIP:
            params.setPreviewFlip(EyeseeCamera::Parameters::PREVIEW_FLIP::NoFlip);
            break;
        case LEFT_RIGHT_FLIP:
            params.setPreviewFlip(EyeseeCamera::Parameters::PREVIEW_FLIP::LeftRightFlip);
            break;
        case TOP_BOTTOM_FLIP:
            params.setPreviewFlip(EyeseeCamera::Parameters::PREVIEW_FLIP::TopBottomFlip);
            break;
    }

    eyesee_camera_->setParameters(&params);
 #endif
}

int Camera::GetPreviewFlip(void) const
{
#if 0
    GET_CAMERA_PARAMS(getPreviewFlip);
#endif
    return 0;
}

void Camera::SetVflip(int flip)
{
    db_msg("vflip: %d, mirror: %d", flip, flip);
    vflip = flip;
    mirror = flip;
    if (flip == init_param_.vflip) {
        db_info("same as init param, no need do it again");
        return;
    }

    // NOTE: V5平台上由于硬件设计问???导致vflip动态设置概率性崩???有风???只能设置vflip标志后重启VI
    init_param_.vflip = flip;
    init_param_.mirror = flip;
}

void Camera::SetDightZoom(int value)
{
    if( value < CAMERA_MIN_ZOOM || value > CAMERA_MAX_ZOOM )
    {
        db_msg("zoom value[%d] invalid", value);
        return ;
    }
    CameraParameters params;
    eyesee_camera_->getParameters(sub_enc_chn_, params);
    params.setZoom(value);
    eyesee_camera_->setParameters(sub_enc_chn_, params);
}

void Camera::SetPreviewRotation(int degree)
{
#if 0
    SET_CAMERA_PARAMS(setPreviewRotation, degree);
#endif
}

void Camera::SetOSDPos(int h)
{
#if 0
//    mWMPos.x = (h > 1280) ? 1400 : 1400;
//    mWMPos.y = (h > 720) ? 970 : 970;
#endif
}

void Camera::EnableOSD()
{
    db_info("Camera::EnableOSD");
    osd_enable_ = true;
    this->Notify(MSG_CAMERA_ENABLE_OSD);
}

void Camera::DisableOSD()
{
    db_info("Camera::DisableOSD");
    osd_enable_ = false;
    this->Notify(MSG_CAMERA_DISABLE_OSD);
}

void Camera::EnableEIS(bool value)
{
    db_info("set eis: %d", value);
    eis_enable_ = value;
}

bool Camera::IsPreviewing(void)
{
    return (cam_status_ == CAM_PREVIEWING);
}

#if 0
void Camera::setUvcMode(int mode)
{
    eyesee_camera_->setUvcGadgetMode(mode);
}

void Camera::suspend()
{
    Release();
}

void Camera::resume()
{
    eyesee_camera_ = EyeseeCamera::open(cam_id_);
}
#endif

void Camera::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_info("handle msg:%d", msg);
    switch (msg) {
        case MSG_STORAGE_UMOUNT:
        case MSG_STORAGE_FS_ERROR:
        case MSG_STORAGE_IS_FULL:
            storage_status_ = STORAGE_NOT_OK;
            break;
        case MSG_STORAGE_MOUNTED:
            storage_status_ = STORAGE_OK;
            break;
        default:
            break;
    }
}

void Camera::onShutter(int chnId)
{
    db_info("onShutter, ch: %d", chnId);
}

void Camera::onError(int chnId, int error_type, EyeseeCamera *pCamera)
{
    switch(error_type)
    {
        case CAMERA_ERROR_SELECT_TIMEOUT:
            {
                db_error("CAMERA_ERROR_SELECT_TIMEOUT:info[0x%x] from channel[%d]", error_type, chnId);
                if(chnId == main_enc_chn_){
                    db_error("[debug_jaosn]: this camera is time out");
                    #if 0      
                     int val = GetModeConfigIndex(SETTING_RECORD_RESOLUTION);
                     SetVideoResolution(val);
                     ShowPreview();
                    #endif
                    Notify(MSG_CAMERA_ON_ERROR);
                }
            }
            break;
        case CAMERA_ERROR_UNKNOWN:
            {
                db_error("CAMERA_ERROR_UNKNOWN:info[0x%x] from channel[%d]", error_type, chnId);
            }
            break;
        case CAMERA_ERROR_RELEASED:
            {
                db_error("CAMERA_ERROR_RELEASED:info[0x%x] from channel[%d]", error_type,chnId);
            }
            break;
        case CAMERA_ERROR_PTS_CHANGE:
            {
                db_error("CAMERA_ERROR_PTS_CHANGE:info[0x%x] from channel[%d]", error_type, chnId);
            }
            break;
        default:

            break;
    }
    return ;
}


void Camera::resetMotionResult(int value)
{
   // mresult_cnt = value;
}

void Camera::onMODData(int chnId, MotionDetectResult * data, EyeseeCamera * pCamera)
{
	if( data == NULL)
	{
		db_error("result pointer is null!");
		return ;
	}
	switch(data->nResult)
	{
		case 0:
			//db_warn("normal");
			break;
		case 1:
            mresult_cnt ++;
            if(mresult_cnt == 1 /*2*/){ 
                db_warn("motion detect happen");
                Notify(MSG_CAMERA_MOTION_HAPPEN);
                mresult_cnt = 0;
            }
			break;
		case 2:
			db_warn("ROI cover happen");
			break;
		case 3:
			db_warn("motion detect and other something happened together");
			break;
		case 4:
			db_warn("camera cover happen");
			break;
		default:
			break;
	}
}

bool Camera::onInfo(int chnId, CameraMsgInfoType info, int extra, EyeseeCamera *pCamera)
{
    bool bHandleInfoFlag = true;
    switch(info)
    {
        case CAMERA_INFO_RENDERING_START:
        {
			if(phy_cam_id_ == CAM_B)
			{
				if(chnId == sub_enc_chn_)
					cdx_sem_up(&mSemRenderStart);
			}
			else if(phy_cam_id_ == CAM_A)
			{
				if(chnId == sub_enc_chn_)
					cdx_sem_up(&mSemRenderStart);
			}	
            else
            {
                db_error("channel[%d] notify render start, but main_chn[%d] sub_chn[%d] wait render start!", chnId, main_enc_chn_,sub_enc_chn_);
            }
            break;
        }
        default:
        {
            db_error("fatal error! unknown info[0x%x] from channel[%d]", info, chnId);
            bHandleInfoFlag = false;
            break;
        }
    }
    return bHandleInfoFlag;
}

void Camera::ReNameThumbFile()
{
    string main_pic_name, sub_pic_name;
    for (auto it : sub_pic_name_map_) {
        auto main_pic = main_pic_name_map_.find(it.first);
        if (main_pic != main_pic_name_map_.end()) {
            main_pic_name = main_pic_name_map_[it.first];
			string::size_type rc = main_pic_name.rfind(".");
			if( rc == string::npos)
			{
				db_warn("invalid picName:%s",main_pic_name.c_str());
				return ;
			}
            sub_pic_name = main_pic_name.insert(rc, "_ths");
            rename(it.second.c_str(), sub_pic_name.c_str());
        }
    }
    main_pic_name_map_.clear();
    sub_pic_name_map_.clear();
    main_pic_cnt_ = 0;
    sub_pic_cnt_ = 0;
}

int Camera::SetPicFileName(const std::string &p_filename)
{
	pic_file_ = p_filename;
	m_takeOnce = true;

	return 0;
}

void Camera::onPictureTaken(int chnId, const void *data, int size, EyeseeCamera *pCamera)
{
    int ret = -1;
    if(!StorageManager::GetInstance()->CheckStorageIsOk())
    {
        db_error("chnId[%d] storage device is not prepared yet!\n",chnId);
        if (chnId == main_enc_chn_)
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        return ;
    }
    //检查会否达到循环复写条件
    MediaFile *file = NULL;    
    char reNameFile[128] = "";
    if (chnId == main_enc_chn_) 
    {
            if(this->CheckStorage(reNameFile) < 0)
            {
                db_error("be careful the sd card is not ok");
                return;
            }
			pthread_mutex_lock(&thumb_pic_lock);
			if( video_thumb_pic_ != NULL)
			{
				pic_file_ = video_thumb_pic_->GetVideoThumbPicFileName();
			}
			else
		    {   
                if( phy_cam_id_ == CAM_A ){
                        db_warn("[debug_jaosn]:onPictureTaken is CAM_A　main_chn\n");
                        file = new MediaFile(phy_cam_id_, PHOTO_A);
               }
               else if(phy_cam_id_ == CAM_B){
                        db_warn("[debug_jaosn]:onPictureTaken is CAM_B　main_enc_chn_\n");
                        file = new MediaFile(phy_cam_id_, PHOTO_B);
               }
               pic_file_ = file->GetMediaFileName();
               db_warn("[debug_joasn]:onPictureTaken pic_file_ is : %s",pic_file_.c_str());
			}
			pthread_mutex_unlock(&thumb_pic_lock);
    }
    
	else if (chnId == sub_enc_chn_)
	{
		pthread_mutex_lock(&thumb_pic_lock);
        if(video_thumb_pic_ != NULL)
		{
            pic_file_ = video_thumb_pic_->GetVideoThumbPicFileName();
        }
		else if(video_thumb_pic_ == NULL)
		{
			if(pic_file_.empty())
			{
				stringstream ss;
				ss << "/mnt/extsd/video/." << sub_pic_cnt_ << ".jpg";
				pic_file_ = ss.str();
			}
			else
			{
				file = new MediaFile(pic_file_);
			}
		}
		pthread_mutex_unlock(&thumb_pic_lock);
	}

    db_warn("pic_file_ is : %s",pic_file_.c_str());
    //写照片文件区分缩略图还是正常照片文件
	int fd = -1;
    if(chnId == main_enc_chn_)
    {
        if(video_thumb_pic_ != NULL)
		{
            db_msg("############ @@@@@@filename is %s##########");
            fd = open(pic_file_.c_str(), O_RDWR | O_CREAT, 0666);
        }else{
            char *filename = const_cast<char*>(pic_file_.c_str());
            db_msg("############filename is %s##########",filename);
            fd = this->allocTakeJpegFile(filename, 3*1024*1024, reNameFile);
        }
    }else{
    fd = open(pic_file_.c_str(), O_RDWR | O_CREAT, 0666);
    }
    if (fd < 0) {
        db_error("open file %s failed(%s)!", pic_file_.c_str(), strerror(errno));
        pic_file_.clear();
        if (chnId == main_enc_chn_)
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        goto out2;
    }

    ret = write(fd, data, size);
    if (ret < 0) {
        db_error("write data filed(%s)", strerror(errno));
        pic_file_.clear();
        if (chnId == main_enc_chn_)
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        goto out1;
    }

    if (video_thumb_pic_ != NULL)
	{
        if (chnId == main_enc_chn_)
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
        goto out1;
    }
    if (chnId == main_enc_chn_) {
        main_pic_name_map_.emplace(main_pic_cnt_++, pic_file_);
        if (file != NULL) 
        {
			pthread_mutex_lock(&thumb_pic_lock);
			ret = MediaFileManager::GetInstance()->AddFile(*file);				
			pthread_mutex_unlock(&thumb_pic_lock);
            if (ret < 0)
            {
                db_error("store file into database failed!");
            }
        }else 
        {
            db_error("takepic file is NULL");
            this->Notify(MSG_CAMERA_TAKEPICTURE_ERROR);
            goto out1;
        }
        if (FILE_EXIST(pic_file_.c_str())) {
            string thumb_pic;
            MediaFile file(pic_file_);
            thumb_pic = file.GetBaseName() + THUMB_SUFFIX + PHOTO_SUFFIX;
            db_debug("pic file %s,thumb pic file %s",pic_file_.c_str(),thumb_pic.c_str());
            MediaFileManager::GetInstance()->GetThumbFromMainPic(pic_file_,thumb_pic);
        }
        //AudioCtrl::GetInstance()->PlaySound(AudioCtrl::AUTOPHOTO_SOUND);
    }else if (chnId == sub_enc_chn_){
        sub_pic_name_map_.emplace(sub_pic_cnt_++, pic_file_);
		pic_file_.clear();
    }

    if (GetTakePicMode() == TAKE_PICTURE_MODE_CONTINUOUS) {
        int pic_num = GetContinuousPictureNumber();
        if(main_pic_cnt_ == pic_num) {
            if (sub_pic_cnt_ == pic_num) {
                ReNameThumbFile();
                pthread_mutex_lock(&lock_);
                takepic_done_ = true;
                pthread_mutex_unlock(&lock_);
                this->Notify(MSG_CAMERA_TAKEPICTURE_FINISHED);
            } else {
                db_warn("low probability, main pic finish write disk first, main:[%d], sub:[%d], total:[%d]",
                        main_pic_cnt_, sub_pic_cnt_, pic_num);
            }
        }
    }else{
     //  ReNameThumbFile();
      pthread_mutex_lock(&lock_);
      takepic_done_ = true;
      pthread_mutex_unlock(&lock_);
      db_error("camera : MSG_CAMERA_TAKEPICTURE_FINISHED");
      this->Notify(MSG_CAMERA_TAKEPICTURE_FINISHED);
    }

out1:
    // NOTE: no need delete, create by recorder
	pthread_mutex_lock(&thumb_pic_lock);
	
    video_thumb_pic_ = NULL;
	pthread_mutex_unlock(&thumb_pic_lock);
	if( fd >= 0 )
    {
    	fsync(fd);
	    close(fd);
		fd = -1;
	}
out2:
	if( fd >= 0 )
	{
    	fsync(fd);
	    close(fd);
		fd = -1;
	}
    if (file != NULL) {
		string media_ths_file = file->GetBaseName() + THUMB_SUFFIX + PHOTO_SUFFIX;
		db_error("try hide file: %s",media_ths_file.c_str());
		if (HideFile(media_ths_file.c_str(),1)==0) {
			db_error("hide thumb file: %s OK",media_ths_file.c_str());
		} else {
			db_error("hide thumb file: %s fail",media_ths_file.c_str());
		}
        delete file;
    }
}
#if 0
void Camera::onFaceDetectData(int chnId, const AW_AI_EVE_EVENT_RESULT_S *data, EyeseeCamera *pCamera)
{

	int i,j;
	static int inc=0;
    face_result result_info;
    gettimeofday(&tm_now, NULL);
    if((tm_now.tv_sec - tm_start.tv_sec)*1000000 + (tm_now.tv_usec - tm_start.tv_usec) >= 30*1000000) // 30s timeout 
    {
        if(!getFaceStatus())
		{
			db_msg("### %s facedetect have stoped!\n", __FUNCTION__);
		}
		else
		{
			eyesee_camera_->disableFaceAutoTakePicture(FACE_CHN);
			eyesee_camera_->stopFaceDetect(FACE_CHN);
			if(face_pic_pack() < 0)
                face_flag = 0;
			pthread_mutex_lock(&face_lock);
			face_status = 0;
			pthread_mutex_unlock(&face_lock);
			//notify facedata finish
			notify_face_detect();
		}
        //printf("Detect Timeout is: %ld s %ld u\n", tm_now.tv_sec - tm_start.tv_sec, tm_now.tv_usec - tm_start.tv_usec);
		return;
    }
	if(data->sTarget.s32TargetNum > 0)
	{
		face_snap_cnt++;
		if(face_snap_cnt > max_face_frm)
		{
			return;
		}
		for(i=0; i<data->sEvent.s32EventNum; i++)
		{
			int iFindID = -1;
            for(j = 0; j < data->sTarget.s32TargetNum; j++)
            {
                if(data->sEvent.astEvents[i].u32TgtID == data->sTarget.astTargets[j].u32ID)
                {
                    iFindID = j;
                    break;
                }
            }
			if(iFindID>=0)
			{
				result_info.u32ID  = data->sTarget.astTargets[iFindID].u32ID;
				memcpy(&(result_info.stRect), &(data->sTarget.astTargets[iFindID].stRect), sizeof(data->sTarget.astTargets[iFindID].stRect));
				FaceResult.push_back(result_info);
			}
			else
			{
				db_msg("not find match ID!!!\n");
			}
		}
	}
	inc++;
	db_msg("@@@@@@@@@@@@ inc %d, face_snap_cnt %d\n",inc, face_snap_cnt);
}

#endif

#ifdef FACE_DETECT
void Camera::onPictureTakenFace(int chnId, const void * data, int size, EyeseeLinux::EyeseeCamera * pCamera)
{
    int ret = -1;
	time_t now;
	struct tm *tm_ptr=NULL;
	now = time(NULL);
	tm_ptr = localtime(&now);
	face_name_info name_info;

	if(chnId != FACE_CHN)
	{
		db_warn("[%s]: invalid chnId %d\n", __FUNCTION__, chnId);
		return;
	}
    
    void *p = const_cast<void *>(data) + size - 2 * sizeof(size_t) - sizeof(off_t);

    PictureROIType roitype = (PictureROIType)(*((off_t*)p));

    p += sizeof(off_t);
    size_t id = *((size_t*)p);

    p += sizeof(size_t);
    size_t fscore = *((size_t*)p);
    
	if(id > (unsigned int)max_face_id)
		max_face_id = id;
	//db_msg("channel [%d] roitype %d id %d\n", chnId, roitype, id);

	static char orgPicName[64];
    char picName[64];
	if(!roitype)	//ԭͼ
	{
		snprintf(orgPicName, sizeof(orgPicName),"%4d%02d%02d-%02d%02d%02d-%02d", tm_ptr->tm_year+1900, tm_ptr->tm_mon+1, tm_ptr->tm_mday, tm_ptr->tm_hour, 
			tm_ptr->tm_min, tm_ptr->tm_sec, ++face_pic_num);
		snprintf(picName, sizeof(picName),"P%s[0].jpg", orgPicName);
	}
	else	//Сͼ
	{
		snprintf(picName, sizeof(picName),"FR%s[%d].jpg", orgPicName, id);
	}
	if(face_pic_num > max_face_frm)
	{
		if(!getFaceStatus())
		{
			db_msg("### %s facedetect have stoped!\n", __FUNCTION__);
		}
		else
		{
			eyesee_camera_->disableFaceAutoTakePicture(FACE_CHN);
			eyesee_camera_->stopFaceDetect(FACE_CHN);
			if(face_pic_pack() < 0)
			{
                face_flag = 0;;
            }
			pthread_mutex_lock(&face_lock);
			face_status = 0;
			pthread_mutex_unlock(&face_lock);
			//notify facedata finish
			notify_face_detect();
		}
		return;
	}
	
	string PicFullPath(PIC_DIR);
	PicFullPath.append(picName);
    name_info.filename = PicFullPath;
	name_info.roiType = roitype;
	name_info.id  = id;
	FaceInfo.push_back(name_info);
    FILE *fp = fopen(PicFullPath.c_str(), "wb");
	if(fp != NULL)
	{
		if((ret = fwrite((char *)data, 1, size, fp)) < size)
	    {
	    	if(ret <= 0)
	    	{
	    		db_msg("%s:1 fwrite size %d, ret %d fail! @@@@\n", __FUNCTION__, size, ret);
			}
			else
			{
				int tmp;
				tmp = size - ret;
				ret = fwrite((char *)data+ret, 1, tmp, fp);
				if(ret < tmp)
				{
					db_msg("%s:2 fwrite size %d, ret %d fail! @@@@\n", __FUNCTION__, tmp, ret);
				}
			}
		}
		fflush(fp);
	    fclose(fp);
        face_flag = 1;
		db_msg(" fwrite %s success!\n", (char *)PicFullPath.c_str());
	}
	else
	{
		db_msg("%s fopen %s fail!\n", __FUNCTION__, PicFullPath.c_str());
	}
}



int compare(const void *a, const void *b)
{
    float fa,fb;
    fa = *(float *)a;
    fb = *(float*)b;
	
    return (fa > fb) ? -1 : 1;
}

int Camera::FQ_init(void)
{
	int quality_flag=0;

	faceq = (FaceQuality*)malloc(sizeof(FaceQuality));
	if(faceq == NULL)
	{
		db_msg("FQ_init fail!\n");
		return -1;
	}
#ifdef QA_LIGHTNESS
#ifdef LOAD_FLOAT_16
	faceq->mean_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile_fp16/patches_mean_plus_fp16.bin";
	faceq->covInv_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile_fp16/patches_covarianceInv_plus_fp16.bin";
	faceq->logCoeff_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile_fp16/patches_logCoeff_plus_fp16.bin";
	faceq->cos_csvFile = "./data/train_template/XiAnBenchmark_lightness_binFile_fp16/cos_value_fp16.bin";
#else
	faceq->mean_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile/facedata_patches_mean_lightness.bin";
	faceq->covInv_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile/facedata_patches_covarianceInv_lightness.bin";
	faceq->logCoeff_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile/facedata_patches_logCoeff_lightness.bin";
	faceq->cos_csvFile = "/data/train_template/XiAnBenchmark_lightness_binFile/cos_value.bin";
#endif // LOAD_FLOAT_16

#else
#ifdef LOAD_FLOAT_16
	faceq->mean_csvFile = "/data/train_template/XiAnBenchmark_binFile_fp16/patches_mean_fp16.bin";
	faceq->covInv_csvFile = "/data/train_template/XiAnBenchmark_binFile_fp16/patches_covarianceInv_fp16.bin";
	faceq->logCoeff_csvFile = "/data/train_template/XiAnBenchmark_binFile_fp16/patches_logCoeff_fp16.bin";
	faceq->cos_csvFile = "/data/train_template/XiAnBenchmark_binFile_fp16/cos_value_fp16.bin";
#else
	faceq->mean_csvFile = "/data/train_template/XiAnBenchmark_binFile/facedata_patches_mean.bin";
	faceq->covInv_csvFile = "/data/train_template/XiAnBenchmark_binFile/facedata_patches_covarianceInv.bin";
	faceq->logCoeff_csvFile = "/data/train_template/XiAnBenchmark_binFile/facedata_patches_logCoeff.bin";
	faceq->cos_csvFile = "/data/train_template/XiAnBenchmark_binFile/cos_value.bin";
#endif // LOAD_FLOAT_16
			
#endif // QA_LIGHTNESS
			
#ifdef QA_LIGHTNESS
	FQ_Init(faceq, FQ_top_d, FQ_patch_num, FQ_patch_size, FQ_feature_dim, "/usr/lib", &quality_flag);
#else
	FQ_Init(faceq, FQ_top_d, FQ_patch_num, FQ_patch_size);
#endif 
	if(quality_flag !=1)
	{
		FQ_Free(faceq);
		faceq = NULL;
		return -1;
	}

	return 0;
}

int Camera::FQ_destroy(void)
{
	if(faceq)
	{
		FQ_Free(faceq);
		faceq = NULL;
	}

	return 0;
}

int Camera::get_driver_id(void)
{
	int driver_id, max_x=0;
	for(vector<face_result>::iterator iter = FaceResult.begin(); iter != FaceResult.end(); iter++)
	{
		if(((iter->stRect.left + (iter->stRect.right - iter->stRect.left)/2 > 640)) && (iter->stRect.right - iter->stRect.left > 150) && (iter->stRect.left > max_x))
		{
			max_x = iter->stRect.left;
			driver_id = iter->u32ID;;
		}
	}
	if(driver_id == 0)	//no driver
	{
		driver_id = max_face_id+1;
	}
	return driver_id;
}

int Camera::FQ_process(float face_score[][MAX_FACE_FRAME], int *f_inc)
{
	int width = 0, height = 0, c = 0;
	unsigned char *pGray=NULL;
	float score;

	for(vector<face_name_info>::iterator iter = FaceInfo.begin(); iter != FaceInfo.end(); iter++)
	{
		if(iter->id == 0)
			continue;
		pGray = stbi_load(iter->filename.c_str(), &width, &height, &c, 1);
		//calculate  probability and quality score		
		if(faceq == NULL)
		{
			printf("faceq is null!\n");
			return -1;
		}
		score = FQ_Assessment(pGray, width, height, faceq);
		iter->score = score;
		face_score[iter->id][f_inc[iter->id]++] = score;
		db_msg("name:%s ID[%d] score[%.5f] f_inc %d\n", iter->filename.c_str(), iter->id, iter->score, f_inc[iter->id]);
		free(pGray);
	}
	return 0;
}

int Camera::pic_packet_by_FQ(float face_score[][MAX_FACE_FRAME], int *f_inc, char *dir_name)
{
	int i=0,ret=0;
	std::string PicPath;
	char *ptr = NULL, *ptr_tmp=NULL;
	char tmp_name[64], src_pic_name[100];
	char new_name[64];
	char pic_name[100];
	int p_inc[max_face_id+1] = {0};
	int org_flag[max_face_id+1] = {0};
	if(dir_name == NULL)
	{
		db_msg("[%s]dir_name is null\n", __FUNCTION__);
		return -1;
	}
	else
	{
		db_msg("dir_name:[%s]\n", dir_name);;
	}
	
	for(vector<face_name_info>::iterator iter = FaceInfo.begin(); iter != FaceInfo.end(); iter++)
	{
		if(iter->id == 0)
			continue;
		i++;
		if(iter->score >= face_score[iter->id][4]) // score top 5
		{
			p_inc[iter->id]++;
			if(p_inc[iter->id] > 5)
				continue;
			memset(pic_name, 0, 100);
			memset(tmp_name, 0, 64);
			memset(new_name, 0, 64);
			memset(src_pic_name, 0, 100);
			db_msg("pack id[%d] score %.5f\n", iter->id, iter->score);
			if(iter->id == driver_id)
			{
				PicPath = iter->filename;
				ptr_tmp = (char *)PicPath.c_str();
				ptr = strchr(ptr_tmp, (int)'F');
				strncpy(tmp_name, ptr,sizeof(tmp_name));
				ptr = strchr(tmp_name, (int)'.');
				ptr[0] ='\0';
				if(fabsf(iter->score- face_score[iter->id][0]) < EPSILON) //????score
				{
					snprintf(pic_name, sizeof(pic_name),"%s/D0/%s-1.jpg", dir_name, tmp_name);
				}
				else
				{
					snprintf(pic_name, sizeof(pic_name),"%s/D0/%s-0.jpg", dir_name, tmp_name);
				}
				ret = rename((char *)PicPath.c_str(), pic_name);
				if(ret<0)
				{
					db_error("rename :src:[%s] des: [%s] fail!\n", PicPath.c_str(), pic_name);
				}
				//ԭͼ
				if(org_flag[iter->id] == 0 && fabsf(iter->score- face_score[iter->id][0]) < EPSILON)
				{
					memset(tmp_name, 0, sizeof(tmp_name));
					memset(pic_name, 0, sizeof(pic_name));
					memset(new_name, 0, sizeof(new_name));
					memset(src_pic_name, 0, sizeof(src_pic_name));
					ptr = strchr(ptr_tmp, (int)'F');
					strncpy(tmp_name, ptr+2, sizeof(tmp_name));
					ptr = strchr(tmp_name, (int)'[');
					ptr[0] ='\0';
					snprintf(pic_name, sizeof(pic_name), "%s/D0/P%s[0].jpg", dir_name, tmp_name); //dest file name
					snprintf(new_name, sizeof(new_name), "P%s[0].jpg", tmp_name);
					snprintf(src_pic_name, sizeof(src_pic_name),"%s/%s", "/mnt/extsd/pic", new_name);//src file name
					ret = rename(src_pic_name, pic_name);
					if(ret<0)
					{
						db_error("rename :src:%s des:%s fail!\n", src_pic_name, pic_name);
					};
					org_flag[iter->id] = 1;
				}
				
			}
			else if(iter->id > driver_id)
			{
				PicPath = iter->filename;
				ptr_tmp = (char *)PicPath.c_str();
				ptr = strchr(ptr_tmp, (int)'F');
				strncpy(tmp_name, ptr, sizeof(tmp_name));
				ptr = strchr(tmp_name, (int)'.');
				ptr[0] ='\0';
				if(fabsf(iter->score- face_score[iter->id][0]) < EPSILON) //????score
				{
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/%s-1.jpg", dir_name, iter->id-1, tmp_name);;
				}
				else
				{
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/%s-0.jpg", dir_name, iter->id-1, tmp_name);;
				}
				if(rename((char *)PicPath.c_str(), pic_name)<0)
				{
					db_error("rename :src:%s des:%s fail!\n", PicPath.c_str(), pic_name);
				}
				//ԭͼ
				if(org_flag[iter->id] == 0 && fabsf(iter->score- face_score[iter->id][0]) < EPSILON)
				{
					memset(tmp_name, 0, sizeof(tmp_name));
					memset(pic_name, 0, sizeof(pic_name));
					memset(new_name, 0, sizeof(new_name));
					memset(src_pic_name, 0, sizeof(src_pic_name));
					ptr = strchr(ptr_tmp, (int)'F');
					strncpy(tmp_name, ptr+2, sizeof(tmp_name));
					ptr = strchr(tmp_name, (int)'[');
					ptr[0] ='\0';
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/P%s[0].jpg", dir_name, iter->id-1, tmp_name);//dest file name
					snprintf(new_name, sizeof(new_name),"P%s[0].jpg", tmp_name);
					snprintf(src_pic_name, sizeof(src_pic_name),"%s/%s", "/mnt/extsd/pic", new_name);//src file name
					ret = rename(src_pic_name, pic_name);
					if(ret<0)
					{
						db_error("rename :src:%s des:%s fail!\n", src_pic_name, pic_name);
					};
					org_flag[iter->id] = 1;
				}
				
			}
			else
			{
				PicPath = iter->filename;
				ptr_tmp = (char *)PicPath.c_str();
				ptr = strchr(ptr_tmp, (int)'F');
				strncpy(tmp_name, ptr, sizeof(tmp_name));
				ptr = strchr(tmp_name, (int)'.');
				ptr[0] ='\0';
				if(fabsf(iter->score- face_score[iter->id][0]) < EPSILON) //????score
				{
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/%s-1.jpg", dir_name, iter->id, tmp_name);;
				}
				else
				{
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/%s-0.jpg", dir_name, iter->id, tmp_name);;
				}
				ret = rename((char *)PicPath.c_str(), pic_name);
				if(ret<0)
				{
					printf("rename :src:%s des:%s fail!\n", PicPath.c_str(), pic_name);
				}
				//ԭͼ
				if(org_flag[iter->id] == 0 && fabsf(iter->score- face_score[iter->id][0]) < EPSILON)
				{
					memset(tmp_name, 0, sizeof(tmp_name));
					memset(pic_name, 0, sizeof(pic_name));
					memset(new_name, 0, sizeof(new_name));
					memset(src_pic_name, 0, sizeof(src_pic_name));
					ptr = strchr(ptr_tmp, (int)'F');
					strncpy(tmp_name, ptr+2, sizeof(tmp_name));
					ptr = strchr(tmp_name, (int)'[');
					ptr[0] ='\0';
					snprintf(pic_name, sizeof(pic_name),"%s/P%d/P%s[0].jpg", dir_name, iter->id, tmp_name);//dest file name
					snprintf(new_name, sizeof(new_name),"P%s[0].jpg", tmp_name);
					snprintf(src_pic_name, sizeof(src_pic_name),"%s/%s", "/mnt/extsd/pic", new_name);//src file name
					ret = rename(src_pic_name, pic_name);
					if(ret<0)
					{
						db_error("rename :src:%s des:%s fail!\n", src_pic_name, pic_name);
					};
					org_flag[iter->id] = 1;
				}
			};
		}
	}
    
	db_msg("%s: total pic is %d, ret %d pack finished!\n", __FUNCTION__, i, ret);
	return 0;
}

int Camera::notify_face_detect(void)
{
    EventReportMsg event;
    event.err_no = 0;
    if(face_flag)   //success
        event.file_name = FACE_TAR_RESULT;
    else
        event.file_name = "";
    event.event_type = EVENT_REQ_POST_FACEDETECT;
    //printf("[%s] notify name [%s] ###\n", __FUNCTION__, event.file_name.c_str());
    AdapterLayer::GetInstance()->notifyMessage(event);
    return 0;
}

int Camera::StartFaceDetect(int detect_frm)
{
	int ret = -1;

    max_face_id = 0;
	face_snap_cnt = 0;
	face_pic_num = 0;
	driver_id = 0;
    face_flag = 0;
    FaceResult.clear();
	FaceInfo.clear();
    if(!StorageManager::GetInstance()->CheckStorageIsOk())
	{
		db_warn("%s :sd card not ready yet, can't start faceDetect\n", __FUNCTION__);
        notify_face_detect();
		return -1;
	}
	if(detect_frm < 1)
	{
		db_warn("invalid detect_frm %d\n",detect_frm);
        detect_frm = 10;
	}
	eyesee_camera_->enableFaceAutoTakePicture(FACE_CHN, this);
	max_face_frm = detect_frm;	
	ret = eyesee_camera_->startFaceDetect(FACE_CHN);
	db_msg("StartFaceDetect!!!\n");
	pthread_mutex_lock(&face_lock);
	face_status = 1;
	pthread_mutex_unlock(&face_lock);
    gettimeofday(&tm_start, NULL);
	return ret;
}

int CreateFolder(const string& strFolderPath)
{
    int ret = -1;
	if(!StorageManager::GetInstance()->CheckStorageIsOk())
	{
		db_warn("sd card not ready yet\n");
		return -1;
	}

	if(strFolderPath.empty())
    {
        db_warn("jpeg path is not set!");
        return ret;
    }

	DIR *dir = opendir(strFolderPath.c_str());;
	if( NULL == dir )
	{
		char buf[128] = {0};
		snprintf(buf, sizeof(buf),"mkdir -p %s",strFolderPath.c_str());
		system(buf);
	}
	else
		closedir(dir);

	return 0;
}

int rm_result_dir(const char *dirname)
{
	if(dirname ==NULL)
	{
        db_error("invalid dirname!\n");
        return -1;
    }
	char tmp[100] = {0};
    DIR *dirp = opendir(dirname);
	if(dirp == NULL)
	{
		db_error("opendir %s fail!\n", dirname);
		return -1;
	}
	struct dirent *dir=NULL;
	struct stat st;
	while((dir = readdir(dirp)) != NULL)
	{
		if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0
		    || strcmp(dir->d_name,"facedata_tmp.tar")==0)
		{
			continue;
		}
		snprintf(tmp, sizeof(tmp),"%s/%s", dirname, dir->d_name);
		if(lstat(tmp, &st) == -1)
		{
			continue;
		}
		if(S_ISDIR(st.st_mode))	// is dir
		{
			if(rm_result_dir(tmp) < -1)
			{
				closedir(dirp);
				return -1;
			}
			rmdir(tmp);
		}
		else if(S_ISREG(st.st_mode)) // is normal file
		{
			unlink(tmp);
		}
		else
		{
			db_msg("st_mode [%d] ignore\n", st.st_mode);
			continue;
		}
	}
	
	return 0;
}

int Camera::face_pic_pack(void)
{
	int  ret=0, i;
	std::string PicPath;
	char pic_name[100];	
	int f_inc[max_face_id+1] = {0};
	float face_score[max_face_id+1][MAX_FACE_FRAME] = {0};
	memset(pic_name, 0, 100);
	//create related dir
	snprintf(pic_name, sizeof(pic_name),"/mnt/extsd/pic/result/");
	PicPath = pic_name;
	if(0 != CreateFolder(PicPath))
    {
        return -1;
    }
	
	FQ_process(face_score, f_inc);
	for(i=1; i<=(int)max_face_id; i++)
	{
		qsort(&face_score[i][0], f_inc[i], sizeof(float), compare);
		db_msg("Des Sort ID[%d], num %d\n", i, f_inc[i]);
	}
	//find driver ID
	driver_id = get_driver_id();
	db_msg("### driver_id [%d], max_id [%d] #####\n", driver_id, max_face_id);
	
	//create sub dir
	for(i=1; i<=(int)max_face_id; i++)
	{
		if(i == driver_id)
		{
			snprintf(pic_name, sizeof(pic_name),"%sresult/D0", PIC_DIR);
		}
		else if(i > driver_id)
		{
			snprintf(pic_name, sizeof(pic_name),"%sresult/P%d", PIC_DIR, i-1);
		}
		else
		{
			snprintf(pic_name, sizeof(pic_name),"%sresult/P%d", PIC_DIR, i);
		}
		PicPath = pic_name;
		if(SUCCESS != CreateFolder(PicPath))
	    {
	        return -1;
	    }
	}
	
	//JPG?ļ?by IDǰ5score ????
	pic_packet_by_FQ(face_score, f_inc, "/mnt/extsd/pic/result");	
	//ѹ??????
	snprintf(pic_name, sizeof(pic_name),"tar -cf %s /mnt/extsd/pic/result", FACE_TAR_TEMP_RESULT);
	ret = system(pic_name);
	db_msg("total pic is %d, ret %d pack finished!\n", i, ret);
	//rm_result_dir(PIC_DIR);
    
	return ret;
}

int Camera::getFaceResult(FilePushInfo *p_fileInfo)
{
	int ret;
	ret = getFaceStatus();
	if(!ret)
	{
		string Md5;
        if(access(FACE_TAR_TEMP_RESULT, 0)!= 0)
        {
            db_error("%s not exist!!!\n", FACE_TAR_TEMP_RESULT);
            return -1;
        }
		getFileMd5(FACE_TAR_TEMP_RESULT, Md5);
		string Imei;
		EventManager::GetInstance()->getIMEI(Imei);
		AesCtrl::GetInstance()->setUserKey(Md5+Imei);
		AesCtrl::GetInstance()->aes_encrypt(FACE_TAR_TEMP_RESULT, FACE_TAR_RESULT);
		string Key;
		AesCtrl::GetInstance()->getKey(Key);
		p_fileInfo->file_name = FACE_TAR_RESULT;
		p_fileInfo->key = Key;
		getFileMd5(FACE_TAR_RESULT, Md5);
		p_fileInfo->md5 = Md5;
		return 0;
	}

	return -1;
}

int Camera::getFaceStatus(void)
{
	int status;
	pthread_mutex_lock(&face_lock);
	status = face_status;
	pthread_mutex_unlock(&face_lock);

	return status;
}
#endif

const std::string &Camera::GetPicFile() const
{
    return pic_file_;
}

const std::string &Camera::GetPicFileName() const
{
    return pic_file_;
}

const std::string Camera::GetThumbPicFileName()
{
	string::size_type rc = pic_file_.rfind(".");
	if( rc == string::npos)
	{
		db_warn("invalid picName:%s",pic_file_.c_str());
		return "";
	}
    string sub_pic_name = pic_file_.insert(rc, "_ths");

    return sub_pic_name;
}

int Camera::GetModeConfigIndex(int msg)
{
    int index =-1;
    MenuConfigLua *mfl=MenuConfigLua::GetInstance();
    index = mfl->GetMenuIndexConfig(msg);
    db_msg("[fangjj]:GetModeConfigIndex  index:[%d]\n", index);
    return index;
}

void Camera::InitTimeTakePic(void)

{
	//init take pic time
	int index1 = GetModeConfigIndex(SETTING_PHOTO_TIMED);
	int index2 = GetModeConfigIndex(SETTING_PHOTO_AUTO);
	int index3 = GetModeConfigIndex(SETTING_PHOTO_DRAMASHOT);
	if(index1 != 0)
	{
        db_msg("SetTimeTakePic = %d=========zhanglm",index1);
        SetTimeTakePic(index1);
	}else if(index2 != 0){
        db_msg("SetAutoTimeTakePic = %d=========zhanglm",index2);
        SetAutoTimeTakePic(index2);
	}else if(index3 != 0){
	//
        db_msg("SetContinuousPictureMode = %d=========zhanglm",index3);
        SetContinuousPictureMode(index3);
	}else{
        time_take_pic_cont_ = 0;
	}
}


int Camera::ISPModuleOnOff(int value)
{
     CameraParameters params;
     eyesee_camera_->getISPParameters(params);
     _isp_value = params.getModuleOnOff();
        switch(value){
        case 1:
            _isp_value.tdf = value;
            break;
        case 0:
            _isp_value.tdf = value;
            break;
       default:
            break;
    }
    db_msg("ISPModuleOnOff = %d",_isp_value.tdf);
    params.setModuleOnOff(_isp_value);
    eyesee_camera_->setISPParameters(params);
    return 0;
}

int Camera::ReInitCameraChn(CameraChnConfigParam &p_camChnPara)
{
    db_info("reinit camera chn[%d,%d] xywh[%d,%d,%d,%d]"
            , p_camChnPara.main_chn
            , p_camChnPara.sub_chn
            , init_param_.sur_.x
            , init_param_.sur_.y
            , init_param_.sur_.w
            , init_param_.sur_.h);
    DeinitCamera();

    eyesee_camera_->setInfoCallback(this);
    eyesee_camera_->setErrorCallback(this);
    VI_DEV_ATTR_S pstDevAttr;
    memset(&pstDevAttr, 0, sizeof(pstDevAttr));

    int ret = eyesee_camera_->prepareDevice();
    if (ret != 0)
    {
        db_error("prepare device error!");
        return -1;
    }

    ret = eyesee_camera_->startDevice();
    if (ret != 0)
    {
        db_error("startDevice error!");
        return -1;
    }

    ret = eyesee_camera_->openChannel(p_camChnPara.main_chn,true);
    if (ret != NO_ERROR)
    {
        db_error("open channel error! chn: %d",p_camChnPara.main_chn);
        return -1;
    }

    ret = eyesee_camera_->setParameters(p_camChnPara.main_chn, p_camChnPara.main_chn_cfgParam);
    if (ret != 0)
    {
        db_error("setParameters error!");
        return -1;
    }
    ret = eyesee_camera_->prepareChannel(p_camChnPara.main_chn);
    if (ret != 0)
    {
        db_error("prepareChannel error! chn: %d",p_camChnPara.main_chn);
        return -1;
    }

    ret = eyesee_camera_->startChannel(p_camChnPara.main_chn);
    if (ret != 0)
    {
        db_error("startChannel error! chn: %d",p_camChnPara.main_chn);
        return -1;
    }

  //  ISPModuleOnOff(1);

    ret = eyesee_camera_->openChannel(p_camChnPara.sub_chn,false);
    if (ret != NO_ERROR)
    {
        db_error("open channel error!, chn: %d", p_camChnPara.sub_chn);
        return -1;
    }

    ret = eyesee_camera_->setParameters(p_camChnPara.sub_chn, p_camChnPara.sub_chn_cfgParam);
    if (ret != 0) {
        db_error("setParameters error!");
        return -1;
    }

    ret = eyesee_camera_->prepareChannel(p_camChnPara.sub_chn);
    if (ret != 0) {
        db_error("prepareChannel error!, chn: %d", p_camChnPara.sub_chn);
        return -1;
    }

    cam_status_ = CAM_INITED;

    mirror = p_camChnPara.main_chn_cfgParam.GetMirror();
    vflip = p_camChnPara.main_chn_cfgParam.GetFlip();
    SET_ISP_PARAMS(ChnIspAwb_SetMode, 0);
    SET_ISP_PARAMS(ChnIspAwb_SetMode, m_whiteBalance);
    int val = GetModeConfigIndex(SETTING_CAMERA_EXPOSURE);
    SetExposureValue(val);
    db_error("=======m_ExposureBias %d=======",m_ExposureBias);
    int freq = GetModeConfigIndex(SETTING_CAMERA_LIGHTSOURCEFREQUENCY);
    SetLightFreq(freq);
    db_error("=======m_LightFreq %d=======",m_LightFreq);
    return 0;
}


int Camera::GetCameraChnCfg(CameraChnConfigParam & p_camChnPara)
{
    p_camChnPara.main_chn = main_enc_chn_;
    p_camChnPara.sub_chn = sub_enc_chn_;

    eyesee_camera_->getParameters(main_enc_chn_, p_camChnPara.main_chn_cfgParam);
    eyesee_camera_->getParameters(sub_enc_chn_, p_camChnPara.sub_chn_cfgParam);

    return 0;
}

int Camera::getFileMd5(const std::string p_FileName, std::string & p_Md5)
{
	char name[128] = {0};
	snprintf(name, sizeof(name), "md5sum %s | cut -d ' ' -f1 > /tmp/camera_md5.txt", p_FileName.c_str());
	system(name);
	int fd = open("/tmp/camera_md5.txt", O_RDONLY, 444);
	if( fd < 0 )
	{
		system("rm /tmp/camera_md5.txt");
		return -1;
	}
	char buf[128] = {0};

	int ret = read(fd, buf, 32);
	if( ret < 0 )
	{
		close(fd);
		system("rm /tmp/camera_md5.txt");
		return -1;
	}

	p_Md5 = buf;

	close(fd);
	system("rm /tmp/camera_md5.txt");

	return 0;
}

void Camera::dumpinit_param_()
{
    db_error("---------------------------------");  
    db_error("init_param_.main_penc_size_: %d %d",init_param_.main_penc_size_.Width, init_param_.main_penc_size_.Height);
    db_error("init_param_.main_venc_size_: %d %d",init_param_.main_venc_size_.Width, init_param_.main_venc_size_.Height);
    db_error("init_param_.sub_venc_size_: %d %d",init_param_.sub_venc_size_.Width, init_param_.sub_venc_size_.Height);
    db_error("init_param_.sub_penc_size_: %d %d",init_param_.sub_penc_size_.Width, init_param_.sub_penc_size_.Height);
    db_error("init_param_.main_chn_: %d",init_param_.main_chn_);
    db_error("init_param_.sub_chn_: %d",init_param_.sub_chn_);
    db_error("init_param_.framate: %d",init_param_.framate);
    db_error("init_param_.buffernumber: %d",init_param_.buffernumber);
    db_error("init_param_.vflip: %d",init_param_.vflip);
    db_error("init_param_.mirror: %d",init_param_.mirror);
    db_error("init_param_.pixel_fmt_: %d",init_param_.pixel_fmt_);
    
    db_error("---------------------------------");  
}

int Camera::releasePictureEncoder(int chid)
{
    int ret = -1;
    ret = eyesee_camera_->releasePictureEncoder(chid);
    return ret;
}

int Camera::allocTakeJpegFile(char *p_fileName, int64_t p_fileSize, char *p_ReNameFile)
{
    pthread_mutex_lock(&photo_file_lock);
    db_error("allocTakeJpegFile rename file name %s,file name %s",
            p_ReNameFile, p_fileName);
    //1:预分配   2:判断文件大小   为0进行预分配  不为零只进行清除动作  
    int fd = -1;
    if(!p_fileName){
        db_error("fatal error! Invalid file name!!");
		pthread_mutex_unlock(&photo_file_lock);
        return -1;
    }
    if(p_ReNameFile != NULL)
    {
        db_error("ReNameFilelen %d,filenamelen %d",strlen(p_ReNameFile),strlen(p_fileName));
        if(strlen(p_ReNameFile))
        {
            db_warn("ReName:%s to %s",p_ReNameFile, p_fileName);
            fd = open(p_ReNameFile, O_RDWR | O_CREAT | O_TRUNC, 0666);		// O_TRUNC 若文件p_ReNameFile存在，则长度被截为0，属性不变
            if( fd != -1 ){
               // close(fd);
            rename(p_ReNameFile, p_fileName);
            }
        } else if(strlen(p_fileName)) {
            fd = open(p_fileName, O_RDWR | O_CREAT , 0666);
            if( fd != -1 )
            {
                if(fallocate(fd, 0x01, 0, p_fileSize) < 0)
                {
                    db_error("fatal error! Failed to fallocate size %lld, (%s)",p_fileSize, strerror(errno));
                }
              //  close(fd);
                db_error("allocate file success");
            }
        }
    }
    pthread_mutex_unlock(&photo_file_lock);
    return fd;
}


