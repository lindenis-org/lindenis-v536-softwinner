#ifndef _VIRVI2VENC_H_
#define _VIRVI2VENC_H_

#include <plat_type.h>
#include <plat_defines.h>
#include <tsemaphore.h>
#include <media/mm_common.h>
#include <media/mm_comm_video.h>

//#define SAVE_FILE

#define MAX_FILE_PATH_SIZE  (256)
#define RC_THRD_SIZE 12

typedef struct Virvi2VencConfig
{
    int DevNum;
    unsigned int SrcWidth;
    unsigned int SrcHeight;
    int SrcFrameRate;
    PAYLOAD_TYPE_E EncoderType;
    int mTimeLapseEnable;
    int mTimeBetweenFrameCapture;
    unsigned int DestWidth;
    unsigned int DestHeight;
    int DestFrameRate;
    int DestBitRate;
    PIXEL_FORMAT_E DestPicFormat; //MM_PIXEL_FORMAT_YUV_PLANAR_420
    char OutputFilePath[MAX_FILE_PATH_SIZE];
} Virvi2VencConfig;

typedef enum {
	HW_VENC_CFG_COMMON					= 0x10,
	HW_VENC_CFG_H264					= 0x11,
	HW_VENC_CFG_H265					= 0x12,
} hw_venc_cfg_groups;

typedef enum {
	HW_VENC_CFG_COMMON_PROC				= 0x00000001,
	HW_VENC_CFG_COMMON_SAVEBSFILE			= 0x00000002,
	HW_VENC_CFG_COMMON_3DNR				= 0x00000004,
} hw_venc_cfg_common_ids;

typedef enum {
	HW_VENC_CFG_H264_ATTR_CBR			= 0x00000001,
	HW_VENC_CFG_H264_ATTR_VBR			= 0x00000002,
	HW_VENC_CFG_H264_ATTR_FIXQP			= 0x00000004,
	HW_VENC_CFG_H264_ATTR_ABR			= 0x00000008,
	HW_VENC_CFG_H264_RC_CBR				= 0x00000010,
	HW_VENC_CFG_H264_RC_VBR				= 0x00000020,
} hw_venc_cfg_H264_ids;

typedef enum {
	HW_VENC_CFG_H265_ATTR_CBR			= 0x00000001,
	HW_VENC_CFG_H265_ATTR_VBR			= 0x00000002,
	HW_VENC_CFG_H265_ATTR_FIXQP			= 0x00000004,
	HW_VENC_CFG_H265_ATTR_ABR			= 0x00000008,
	HW_VENC_CFG_H265_RC_CBR				= 0x00000010,
	HW_VENC_CFG_H265_RC_VBR				= 0x00000020,
	
} hw_venc_cfg_H265_ids;

typedef struct venc_attr_H264_H265_cfg
{
	//VENC_ATTR_S
	int MaxKeyInterval;                           /* wanted key frame interval, dynamic param*/
	unsigned int  SrcPicWidth;                    /* source width of a picture sent to venc channel, in pixel*/
	int  SrcPicHeight;                   /* source height of a picture sent to venc channel, in pixel*/
	int  Field;
	int PixelFormat;
	int Rotate;    /*encoder rotate angle.*/	
	    
		/*VENC_ATTR_H264_S*/
	unsigned int  MaxPicWidth;                         /*maximum width of a picture to be encoded, in pixel*/
	unsigned int  MaxPicHeight;                        /*maximum height of a picture to be encoded, in pixel*/
	unsigned int  BufSize;                             /*stream buffer size*/
	unsigned int  Profile;                             /*0: baseline; 1:MP; 2:HP; 3: SVC-T [0,3]; */          
	int bByFrame;                               /*get stream mode is slice mode or frame mode*/	
	unsigned int  PicWidth;                            /*width of a picture to be encoded, in pixel*/
	unsigned int  PicHeight;                           /*height of a picture to be encoded, in pixel*/    
	unsigned int  BFrameNum;                           /* 0: not support B frame; >=1: number of B frames */
	unsigned int  RefNum;                              /* 0: default; number of refrence frame*/
	unsigned int FastEncFlag;   //for fast video encoder
	unsigned int IQpOffset;  //IQp offset value to offset I frame Qp to decrease I frame size.
	unsigned int mbPIntraEnable;  //enalbe p frame intra

	  //for CBR VBR FIXQP
	unsigned int	mGop; 
	unsigned int	mSrcFrmRate;
	unsigned int	fr32DstFrmRate ;			 /* the target frame rate of the venc chnnel */  
	 //for FIXQP
	unsigned int	mIQp;				 /* qp of the i frame */
	unsigned int	mPQp;				 /* qp of the p frame */
	//for  CBR (StatTime also for VBR)
	unsigned int   mStatTime;                            /* the rate statistic time, the unit is senconds(s) */
	unsigned int   mBitRate;                             /* average bitrate */
	unsigned int  mFluctuateLevel;                      /* level [0..5].scope of bitrate fluctuate. 1-5: 10%-50%. 0: SDK optimized, recommended; */
	//for VBR
	unsigned int	mMaxBitRate;				/* the max bitrate */			   
	unsigned int	mMaxQp;				/* the max qp */
	unsigned int	mMinQp;				/* the min qp */

	//for ABR
	unsigned int mRatioChangeQp;
	int mQuality;
	int	enGopMode;
	 //for NORMALP and DUALP
	int  mIPQpDelta;
	 //DUALP
	unsigned int	mSPInterval;
	int	mSPQpDelta;
	//SMARTP
	unsigned int   mBgInterval;
	int  mBgQpDelta;
	int  mViQpDelta;									    /**/
	unsigned int	mVirtualIInterval;
	//BIPREDB
	unsigned int  mBFrmNum;

}venc_attr_H264_H265_cfg;

typedef struct venc_rcparam_H264_265_cfg{
		//for both CBR and VBR
	unsigned int ThrdI[RC_THRD_SIZE];                     /* just useful for h264/h265 and mpeg4 for now */
	unsigned int ThrdP[RC_THRD_SIZE];
	unsigned int RowQpDelta;
	unsigned int  MinIprop;                                /* the min ratio of i frame and p frame */           
	unsigned int  MaxIprop;                                /* the max ratio of i frame and p frame */
	unsigned int  MinQp;                                   /* the min QP value */
	unsigned int IPQPDelta;                               /* the qp difference between the i frame and the before gop avarage qp; == Qp(P) - Qp(I) */
		//for CBR
	unsigned int  MaxQp;                                   /* the max QP value */
	unsigned int  QualityLevel;                            /* quality of picture [1, 5] */
	unsigned int  MaxReEncodeTimes;                        /* max number of re-encode times [0, 3]*/ 
	unsigned int  MinIQp;                                  /* min qp for i frame */
		//for VBR
	int s32ChangePos;
}venc_rcparam_H264_H265_cfg;



typedef struct venc_proc_cfg {
   	unsigned int              bProcEnable;
    	unsigned int                nProcFreq;
	unsigned int				nStatisBitRateTime;
	unsigned int				nStatisFrRateTime;
}venc_proc_cfg;

typedef struct venc_savebsfile_cfg {
   char filename[256];
   unsigned int save_bsfile_flag;
    unsigned int save_start_time;
    unsigned int save_end_time;
}venc_savebsfile_cfg;

typedef struct venc_3DNR_cfg {
	int flag_3DNR;
}venc_3DNR_cfg;


#ifdef __cplusplus
extern "C" {
#endif

int init_venc_module(const Virvi2VencConfig *configPara);
int wait_venc_module_stop();
int deinit_venc_module();
int venc_set_cfg(int VeChn, unsigned char group_id, unsigned int cfg_ids, void *cfg_data);
int venc_get_cfg(int VeChn, unsigned char group_id, unsigned int cfg_ids, void *cfg_data);
//int venc_start(int VeChn, Virvi2VencConfig *configPara, VENC_CHN_ATTR_S *VencAttr);
//int venc_stop(int VeChn);


#ifdef __cplusplus
}
#endif

#endif  /* _VIRVI2VENC_H_ */

