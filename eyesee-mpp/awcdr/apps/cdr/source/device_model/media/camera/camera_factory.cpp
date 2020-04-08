#include "device_model/media/camera/camera_factory.h"
#include "device_model/media/media_definition.h"
#include "device_model/display.h"
#include "device_model/menu_config_lua.h"
#include "common/setting_menu_id.h"
#include "common/app_log.h"
#include "device_model/media/osd_manager.h"

#include <media/mm_comm_sys.h>
#include <media/mpi_sys.h>

#undef LOG_TAG
#define LOG_TAG "camera_factory.cpp"

using namespace EyeseeLinux;
using namespace std;

Camera *CameraFactory::CreateCamera(CameraID cam_id)
{
    db_msg("create camera: %d", cam_id);
    Camera *camera = NULL;
    switch (cam_id) {
        case CAM_IPC_0:
            camera = this->CreateIPCCamera(CAM_A);
            break;
        case CAM_IPC_1:
            camera = this->CreateIPCCamera(CAM_B);
            break;
        case CAM_STITCH_0:
            camera = this->CreateStitchCamera(CAM_A);
            break;
        case CAM_STITCH_1:
            camera = this->CreateStitchCamera(CAM_B);
            break;
        case CAM_NORMAL_0:
            camera = this->CreateNormalCamera(CAM_A);
            break;
        case CAM_NORMAL_1:
            camera = this->CreateNormalCamera(CAM_B);
            break;
        case CAM_UVC_0:
            camera = this->CreateUVCCamera(CAM_A);
            break;
        case CAM_UVC_1:
            camera = this->CreateUVCCamera(CAM_B);
            break;
        default:
            break;
    }
    assert(camera != NULL);
    return camera;
}

Camera *CameraFactory::CreateCamera(PhysicalCameraID phy_cam_id, CameraInitParam &param)
{
    int ret = -1;
    Camera *camera = new Camera(phy_cam_id);
    assert(camera != NULL);
    // add ConfigCamera
	camera->ConfigCamera(phy_cam_id,ISP_0,VIPP_0,VIPP_1);

    ret = camera->Open();
    if (ret < 0) {
        db_error("open camera failed");
        goto out;
    }
    camera->SetVflip(0);//set playback rolation
    ret = camera->InitCamera(param);
    if (ret < 0) {
        db_error("camera init failed");
        goto out;
    }
//    camera->StartPreview();

    cam_cnt_++;

    return camera;

out:
    delete camera;
    return NULL;
}

uint8_t CameraFactory::GetCameraCnt()
{
    return cam_cnt_;
}

CameraFactory::CameraFactory()
        : cam_cnt_(0)
{
    /*
    int ret;
    MPP_SYS_CONF_S sys_conf;
    sys_conf.nAlignWidth = 32;
    ret = ::AW_MPI_SYS_SetConf(&sys_conf);
    if (ret != SUCCESS) {
        db_error("AW_MPI_SYS_SetConf failed");
    }

    ret = ::AW_MPI_SYS_Init();
    if (ret != SUCCESS) {
        db_error("AW_MPI_SYS_Init failed");
        _exit(-1);
    }
    */
}


CameraFactory::~CameraFactory()
{
    /*
    db_msg("destruct");
    ::AW_MPI_SYS_Exit();
    */
}

Camera *CameraFactory::CreateIPCCamera(PhysicalCameraID phy_cam_id)
{
    CameraInitParam param;

    SIZE_S size = {1920, 1080};
    param.cam_param_.setVideoSize(size);

#ifdef GUI_SUPPORT
    if (phy_cam_id == CAM_B) {
        param.sur_.x = 158;
        param.sur_.y = 180;
        param.sur_.w = 443;
        param.sur_.h = 400;
    } else if (phy_cam_id == CAM_A) {
        param.sur_.x = 666;
        param.sur_.y = 180;
        param.sur_.w = 443;
        param.sur_.h = 400;
    }
#endif

    return CreateCamera(phy_cam_id, param);
}

Camera *CameraFactory::CreateStitchCamera(PhysicalCameraID phy_cam_id)
{
    CameraInitParam param;

    SIZE_S size = {1280, 720};
    param.cam_param_.setVideoSize(size);

    return CreateCamera(phy_cam_id, param);
}

Camera *CameraFactory::CreateNormalCamera(PhysicalCameraID phy_cam_id)
{
    CameraInitParam param;

    param.main_penc_size_ = {3840,2160};
	#ifdef USE_IMX335
    param.main_venc_size_ = {2560,1440};
	#else
	param.main_venc_size_ = {3840,2160};
	#endif
    param.sub_venc_size_  = {640,360};
    param.sub_penc_size_  = {640,360};
    param.framate = 25;
    param.buffernumber = 4;
	#ifdef USE_IMX335
	param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
	#else
    param.pixel_fmt_ = MM_PIXEL_FORMAT_YUV_AW_LBC_2_5X;
	#endif
	if( phy_cam_id == CAM_B)
		param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    uint32_t vi_freq = 300000000;

    MenuConfigLua *menu_config = MenuConfigLua::GetInstance();
    int val = menu_config->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
	db_error("RECORD_RESOLUTION idx: %d",val);
    switch(val)
    {
        case VIDEO_QUALITY_4K30FPS:
			#ifdef USE_IMX335
			param.main_venc_size_ = {2560,1440};
            param.framate = 25;//25;
			#else
            param.main_venc_size_ = {3840,2160};
            param.framate = 25;
			#endif
            param.buffernumber = 5;
			OsdManager::get()->setTimeOsdPostion(3056-96, 2032);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(3056-96, 2032 - 96);
			OsdManager::get()->setCaridOsdPosition(3696, 2032 - 96);
            break;
        case VIDEO_QUALITY_2_7K30FPS:
			#ifdef USE_IMX335
            param.main_venc_size_ = {2560,1440};
            param.framate = 25;//25;
			#else
			param.main_venc_size_ = {2688,1520};
            param.framate = 25;
			#endif
            param.buffernumber = 5;
			OsdManager::get()->setTimeOsdPostion(1904-96, 1408);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(1904-96, 1408 - 96);
			OsdManager::get()->setCaridOsdPosition(2528, 1408 - 96);
            break;
        case VIDEO_QUALITY_1080P120FPS:
            param.main_venc_size_ = {1920,1080};
            param.framate = 120;
            param.buffernumber = 20;
            vi_freq = 480000000;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_1080P60FPS:
            param.main_venc_size_ = {1920,1080};
            param.framate = 60;
            param.buffernumber = 10;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_1080P30FPS:
            param.main_venc_size_ = {1920,1080};
            param.framate = 25;//25;
            param.buffernumber = 5;
			OsdManager::get()->setTimeOsdPostion(1542-48, 1007);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(1542-48, 1007 - 64);
			OsdManager::get()->setCaridOsdPosition(1856, 1007 - 64);
            break;
        case VIDEO_QUALITY_720P240FPS:
            param.main_venc_size_ = {1280,536};
            param.framate = 240;
            param.buffernumber = 40;
            vi_freq = 480000000;
            param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
			OsdManager::get()->setTimeOsdPostion(560, 472);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 472 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 472 - 64-64);
            break;
        case VIDEO_QUALITY_720P120FPS:
            param.main_venc_size_ = {1280,720};
            param.framate = 120;
            param.buffernumber = 30;
            vi_freq = 480000000;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 640 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 640 - 64-64);
            break;
        case VIDEO_QUALITY_720P60FPS:
            param.main_venc_size_ = {1280,720};
            param.framate = 60;
            param.buffernumber = 10;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 640 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 640 - 64-64);
            break;
        case VIDEO_QUALITY_720P30FPS:
            param.main_venc_size_ = {1280,720};
            param.framate = 30;
            param.buffernumber = 5;
			OsdManager::get()->setTimeOsdPostion(560, 640);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_32);
			OsdManager::get()->setGpsOsdPosition(560, 640 - 64);
			OsdManager::get()->setCaridOsdPosition(560, 640 - 64-64);
            break;
        default:
            db_error("not support image_quality:%d, use 4K30FPS as default", val);
			#ifdef USE_IMX335
			param.main_venc_size_ = {2560,1440};
            param.framate = 25;//25;
			#else
            param.main_venc_size_ = {3840,2160};
            param.framate = 25;
			#endif
            param.buffernumber = 5;
			OsdManager::get()->setTimeOsdPostion(3056-96, 2032);
			OsdManager::get()->SetTimeOsdFontSize(FONT_SIZE_64);
			OsdManager::get()->setGpsOsdPosition(3056-96, 2032 - 96);
			OsdManager::get()->setCaridOsdPosition(3696, 2032 - 96);
    }

	if (SCREEN_HEIGHT > SCREEN_WIDTH) {
	    if(param.framate == 240){
	        param.sub_venc_size_  = {360,640};
	    }else{
	        param.sub_venc_size_  = {SCREEN_HEIGHT,SCREEN_WIDTH};
	    }
	} else {
		if(param.framate == 240){
	        param.sub_venc_size_  = {640,360};
	    }else{
	        param.sub_venc_size_  = {SCREEN_WIDTH,SCREEN_HEIGHT};
	    }
	}

    val = menu_config->GetMenuIndexConfig(SETTING_PHOTO_RESOLUTION);
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
    }

    //int vf = menu_config->GetMenuIndexConfig(SETTING_CAMERA_IMAGEROTATION);
    param.mirror = 0;//vf ? 0 : 1;
    param.vflip  = 0;//vf ? 0 : 1;
    if (phy_cam_id == CAM_B) {
        param.main_chn_ = phy_cam_id + 1;
    } else if (phy_cam_id == CAM_A) {
        param.main_chn_ = phy_cam_id;
    }

    param.sub_chn_ = param.main_chn_ + 1;
#ifdef GUI_SUPPORT
    if (phy_cam_id == CAM_B) {
        param.sur_.x = SCREEN_WIDTH/2;
        param.sur_.y = SCREEN_HEIGHT/2;
        param.sur_.w = SCREEN_WIDTH/2;
        param.sur_.h = SCREEN_HEIGHT/2;
    } else if (phy_cam_id == CAM_A) {
        param.sur_.x = 0;
        param.sur_.y = 0;
        param.sur_.w = SCREEN_WIDTH;
        param.sur_.h = SCREEN_HEIGHT;
    }
#endif
    //AW_MPI_VI_SetVIFreq(0, vi_freq);

    return CreateCamera(phy_cam_id, param);
}

Camera *CameraFactory::CreateUVCCamera(PhysicalCameraID phy_cam_id)
{
    int ret,FlipVal=0;
    CameraInitParam param;

    Camera *camera = new Camera(phy_cam_id);
    assert(camera != NULL);
    camera->ConfigCamera(phy_cam_id,ISP_1,VIPP_2,VIPP_3);
    ret = camera->Open();
    if (ret < 0)
    {
        db_error("open camera failed");
        goto out;
    }

    param.main_penc_size_ = {1920,1080};
    param.main_venc_size_ = {1920,1080};
    param.sub_venc_size_  = {320,240};
    param.sub_penc_size_  = {320,240};
    param.framate = 25;
    param.buffernumber = 4;
    param.main_chn_ = phy_cam_id;
    param.sub_chn_ = param.main_chn_ + 1;
	if(phy_cam_id == CAM_B )
	{
		param.main_chn_ = phy_cam_id+1;
		param.sub_chn_ = param.main_chn_ + 1;
	}

#ifdef GUI_SUPPORT
		if (phy_cam_id == CAM_B)
		{
			param.sur_.x = 0;
			param.sur_.y = 0;
			param.sur_.w = 240;
			param.sur_.h = 320;
		}
#endif

    param.pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    if(!MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_CAMERA_IMAGEROTATION));
    {
        FlipVal = 1;
    }
    param.mirror = 1;//FlipVal;
    param.vflip = 0;//FlipVal;
    ret = camera->InitCamera(param);
    if (ret < 0)
    {
        db_error("camera init failed");
        goto out;
    }
	printf("@@CAM_B InitCamera ok!\n");
    cam_cnt_++;

    return camera;

    out:
        delete camera;
        return NULL;
}
