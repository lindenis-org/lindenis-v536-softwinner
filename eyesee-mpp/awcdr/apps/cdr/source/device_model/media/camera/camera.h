/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file camera.h
 * @brief 定义camera对外接口
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#pragma once

#include "device_model/media/media_definition.h"


#ifdef GUI_SUPPORT
#include "device_model/display.h"
#endif
#include "common/subject.h"
#include "common/observer.h"

#include "media/camera/EyeseeCamera.h"
#include "bll_presenter/AdapterLayer.h"

#if 0
#include "faceQuality/aw_macro.h"
#include "faceQuality/quality_assessment.h"
#include "faceQuality/QA.h"
#include "faceQuality/stb_image.h"
#endif

#include <media/mpi_vi.h>

#include <pthread.h>
#include <list>

#include <tsemaphore.h>

namespace EyeseeLinux {

/**
 * @brief 实际物理camera id
 */
typedef enum {
    CAM_A = 0,  /*前摄 */
    CAM_B,      /*后拉*/
    CAM_CNT,    /*camera 总数 */
    ERROR_CAM = -1,
} PhysicalCameraID;

typedef enum {
    ISP_0 = 0,
    ISP_1,
    ERROR_ISP = -1,
} CameraIspID;

typedef enum {
    VIPP_0 = 0,
    VIPP_1,
    VIPP_2,
    VIPP_3,
    ERROR_VIPP = -1,
} VippChnID;

/**
 * camera status
 */
typedef enum {
    CAM_OPENED = 0,
    CAM_INITED,
    CAM_PREVIEWING,
    CAM_CLOSED,
    CAM_PREVIEWING_PAUSE,
} CameraStatus;

typedef struct CameraConfigParam_
{
   int main_chn;
   int sub_chn;
   CameraParameters main_chn_cfgParam;
   CameraParameters sub_chn_cfgParam;   
}CameraChnConfigParam;

typedef struct CameraInitParam {
    CameraParameters cam_param_;
    SIZE_S main_penc_size_;         /**< 图片编码size */
    SIZE_S main_venc_size_;         /**< 主视频编码size */
    SIZE_S sub_venc_size_;          /**< 子视频编码size */
    SIZE_S sub_penc_size_;          /**< 子视频编码size */
    int main_chn_;                  /**< 主视频编码使用的数据通道 */
    int sub_chn_;                   /**< 子视频编码使用的数据通道, 预览也使用该通道 */
    int framate;
    int buffernumber;
    int vflip;
    int mirror;
    PIXEL_FORMAT_E pixel_fmt_;
#ifdef GUI_SUPPORT
    ViewInfo sur_;
#endif

    CameraInitParam() {
    main_penc_size_ = {DEFAULT_CAPTURE_WIDTH, DEFAULT_CAPTURE_HEIGHT};
    main_venc_size_ = {DEFAULT_CAPTURE_WIDTH, DEFAULT_CAPTURE_HEIGHT};
    sub_venc_size_ = {DEFAULT_CAPTURE_WIDTH, DEFAULT_CAPTURE_HEIGHT};
    pixel_fmt_ = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    cam_param_.setPreviewFrameRate(30);
    cam_param_.setVideoBufferNumber(7);
    cam_param_.setPreviewRotation(0);
    SIZE_S default_size = {DEFAULT_CAPTURE_WIDTH, DEFAULT_CAPTURE_HEIGHT};
    cam_param_.setVideoSize(default_size);
    cam_param_.setPictureSize(default_size);

    }

} CameraInitParam;
	
typedef struct face_name_info_t
{
	std::string filename;
	int roiType;
	int id;
	float score;
}face_name_info;
typedef struct face_rect_t
{
	short int left;
	short int top;
	short int right;
	short int buttom;
}face_rect;
typedef struct face_result_t
{
	int u32ID;
	face_rect stRect;
}face_result;

#define CAMERA_MAX_ZOOM 10
#define CAMERA_MIN_ZOOM 0
#define MAX_FACE_FRAME	(90)
class MediaFile;
class Camera
    : public ISubjectWrap(Camera)
    , public IObserverWrap(Camera)
    , public EyeseeCamera::PictureCallback
    , public EyeseeCamera::InfoCallback
    , public EyeseeCamera::ShutterCallback
    , public PictureRegionCallback
    , public EyeseeCamera::ErrorCallback    
    , public EyeseeCamera::MODDataCallback
{
    public:
        Camera(PhysicalCameraID phy_cam_id);
        ~Camera();
		int startMotionDetect();
		int stopMotionDetect();
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

        void onShutter(int chnId);
        bool onInfo(int chnId, CameraMsgInfoType info, int extra, EyeseeCamera *pCamera);
        void onError(int chnId, int error_tyoe, EyeseeCamera *pCamera);
        void onPictureTaken(int chnId, const void *data, int size, EyeseeCamera* pCamera);
		//void onFaceDetectData(int chnId, const AW_AI_EVE_EVENT_RESULT_S * data, EyeseeLinux ::EyeseeCamera * pCamera);
		void onMODData(int chnId, MotionDetectResult * data, EyeseeCamera * pCamera);
        void addPictureRegion(std::list<PictureRegionType> &rPictureRegionList);
        int Open();
        int Close();
        int InitCamera(CameraInitParam &init_param);
        int DeinitCamera();
        EyeseeCamera* GetEyeseeCamera(void) const;
        PhysicalCameraID GetCameraID() const;
        void GetCameraInitParam(CameraInitParam &init_param);
		int InitCameraFace(CameraInitParam &init_param);

#ifdef GUI_SUPPORT
        void ShowPreview(void);
        void HidePreview(void);
        void StartPreview(bool p_OnlyRestartChannel = false);
        void StopPreview(void);
#endif
//        void SetVideoCaptureBufferCnt(int cnt);
//        int GetVideoCaptureBufferCnt(void) const;
        void SetVideoResolution(int resolution);
        void SetVideoResolution(int width, int height);
        void GetVideoResolution(Size &size) const;
        void SetVideoFPS(int fps);
        int GetVideoFPS(void) const;

        int TakePicture(int chn);
		int TakePictureEx(int chn, int pictype);
        void SetContinuousPictureMode(int value);
        void SetPicResolution(int resolution);
        void SetPicResolution(int width, int height);
        void GetPicResolution(Size &size) const;
        void SetPicQuality(int percent);
        int GetPicQuality(void) const;
        int GetContinuousPictureNumber(void);
        int GetTakePicMode(void);
        void SetTimeTakePic(int val);
        const unsigned int GetTimeTakePicTime() const;
        const unsigned int GetAutoTimeTakePicTime() const;
        void SetAutoTimeTakePic(int val);
        void SetVideoFileForThumb(MediaFile *mf);
        
        /*
         * 0:normal mode
         * 1:fast mode
         * 2:continuous mode
         * 3:continuous fast mode
         */
        void SetPictureMode(int mode);
        /*
         * "auto",
         * "daylight",
         * "cloudy-daylight",
         * "incandescent",
         * "fluorescent"
         */
        void SetWhiteBalance(int value);
        const int GetWhiteBalance(void) const;
        void SetContrastValue(int value);
        int GetContrastValue(void) const;
        void SetExposureValue(int value);
        int GetExposureValue(void) const;
        void SetLightFreq(int index);
        int GetLightFreq(void) const;
        /*
         * 0:NoFlip
         * 1:LeftRightFlip
         * 2:TopBottomFlip
         */
        void SetPreviewFlip(int flip);
        int GetPreviewFlip(void) const;
        void SetVflip(int flip);
        void SetDightZoom(int value);
        void SetPreviewRotation(int degree);

        void SetOSDPos(int h);
        void EnableOSD();
        void DisableOSD();

        void EnableEIS(bool value);

        bool IsPreviewing(void);
        int GetChannelID(void) const;

        int CheckStorage(void);
        const std::string &GetPicFile() const;
        const std::string &GetPicFileName() const;
        const std::string GetThumbPicFileName();
        int GetMainEncChn(void) const;
        int GetSubEncChn(void) const;
        int ISPModuleOnOff(int value);
        void ConfigCamera(int CsiChn,int ispdev,int vippchnValue0,int vippchnValue1);
        void ReSetVideoResolutionForNormalphoto(PIXEL_FORMAT_E type);
        void SetSlowVideoResloution(int resolution);
    int ReInitCameraChn(CameraChnConfigParam &p_camChnPara);
    int GetCameraChnCfg(CameraChnConfigParam &p_camChnPara);
    void ReNameThumbFile();
    void CancelContinuousPicture();
	int getFileMd5(const std::string p_FileName, std::string & p_Md5);
#ifdef GUI_SUPPORT
	int GetCameraDispRect(ViewInfo &p_info);
	int SetCameraDispRect(ViewInfo p_info, int p_Zorder);
	void SetDispZorder(int zorder);
#endif
	int SetPicFileName(const std::string &p_filename);
	int StartFaceDetect(int detect_frm);
	int face_pic_pack(void);
	void onPictureTakenFace(int chnId, const void * data, int size, EyeseeLinux::EyeseeCamera * pCamera);
	int getFaceResult(FilePushInfo *p_fileInfo);
	int getFaceStatus(void);
	int DeinitCameraFace(void);
    void resetMotionResult(int value);
	//int RunFaceDetect(void);
	//static void *FaceDetectThread(void *arg);
    int releasePictureEncoder(int chid);
    
    int allocTakeJpegFile(char *p_fileName, int64_t p_fileSize, char *p_ReNameFile);
    int CheckStorage(char* reNamePhotofile);
private:
		int FQ_init(void);
		int FQ_destroy(void);
		int get_driver_id(void);
		int FQ_process(float face_score[][MAX_FACE_FRAME], int *f_inc);
		int pic_packet_by_FQ(float face_score[][MAX_FACE_FRAME], int *f_inc, char *dir_name);
        int notify_face_detect(void);
        EyeseeCamera *eyesee_camera_;
        PhysicalCameraID phy_cam_id_;
        int main_enc_chn_;  /**< 用于主码流及拍照 */
        int sub_enc_chn_;   /**< 用于子码流及预览 */
        int channel_id_;
        cdx_sem_t mSemRenderStart;
#ifdef GUI_SUPPORT
        Layer *layer_;
        int layer_handle_;
        int zorder_;
#endif
        int storage_status_;
        uint8_t cam_status_;
        CameraInitParam init_param_;
        pthread_mutex_t lock_;
        std::string pic_file_;
        int continue_pic_cont_;
        unsigned int time_take_pic_cont_;
        unsigned int auto_time_take_pic_cont_;
        bool is_thumb_pic_;
        MediaFile *video_thumb_pic_;
        ISP_MODULE_ONOFF _isp_value;
        CameraParameters m_main_cam_cfg,m_sub_cam_cfg;
        int vflip;
        int mirror;
	    int m_degree;
        int m_whiteBalance;
        int m_ExposureBias;
        int m_LightFreq;
        int osd_enable_;
        std::map<int, std::string> sub_pic_name_map_;
        std::map<int, std::string> main_pic_name_map_;
        int main_pic_cnt_;
        int sub_pic_cnt_;
        bool takepic_done_;
        bool eis_enable_;
		bool m_takeOnce;
		int face_snap_cnt;
		int face_pic_num;
		int max_face_frm;
		int driver_id;
		unsigned int max_face_id;
		int face_status;
        int face_flag;
        int mresult_cnt;
        struct timeval tm_start,tm_now;
		//FaceQuality* faceq;
		pthread_mutex_t face_lock;
		pthread_mutex_t thumb_pic_lock;
        pthread_mutex_t photo_file_lock;
		int  m_dispHandle;
#ifdef GUI_SUPPORT
        int InitDisplay(int p_CamId);
#endif  
        int GetModeConfigIndex(int msg);
		void InitTimeTakePic();
        void dumpinit_param_();
}; /* class Camera */
} /* namespace EyeseeLinux */

