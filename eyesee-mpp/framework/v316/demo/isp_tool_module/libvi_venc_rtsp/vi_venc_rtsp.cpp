/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
 File Name     : sample_virvi2venc.c
Version       : Initial Draft
Author        : Allwinner BU3-PD2 Team
Created       : 2017/1/5
Last Modified :
Description   : mpp component implement
Function List :
History       :
 ******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "vi_venc_rtsp"

#include <utils/plat_log.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"
#include "media/mpi_venc.h"
#include "media/mpi_sys.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"

#include "vi_venc_rtsp.h"

#include <rtsp/TinyServer.h>

typedef struct awVI2Venc_PrivCap_S {
    pthread_t thid;
    VI_DEV Dev;
    VI_CHN Chn;
    AW_S32 s32MilliSec;
    PAYLOAD_TYPE_E EncoderType;
} VI2Venc_Cap_S;

VENC_CHN mVeChn;
VI_DEV mViDev;
VI_CHN mViChn;
int vipp_dev;
int isp_dev;

volatile int stop_enc = 0;

VI2Venc_Cap_S privCap;

FILE* OutputFile_Fd;

// rtsp var
static TinyServer *rtsp_server;
static MediaStream *stream_sender;
unsigned char *frame_data = NULL;
int frame_data_size = 0;

static int deinit_rtsp_module()
{
    rtsp_server->stop();

    if (stream_sender) {
        delete stream_sender;
        stream_sender= NULL;
    }

    if (rtsp_server) {
        delete rtsp_server;
        rtsp_server = NULL;
    }

    return 0;
}

static int init_rtsp_module(const Virvi2VencConfig *configPara)
{
    rtsp_server = TinyServer::createServer();
    if (rtsp_server == NULL) {
        printf("rtsp server create failed!");
        return -1;
    }

    MediaStream::MediaStreamAttr::VideoType video_type = MediaStream::MediaStreamAttr::VIDEO_TYPE_H264;
    if(PT_H264 == configPara->EncoderType)
    {
        video_type = MediaStream::MediaStreamAttr::VIDEO_TYPE_H264;
    }
    else if(PT_H265 == configPara->EncoderType)
    {
        video_type = MediaStream::MediaStreamAttr::VIDEO_TYPE_H265;
    }
    else if(PT_MJPEG == configPara->EncoderType)
    {
        printf("rtsp server not support send mjpeg now!\n");
        goto _exit;
    }

    MediaStream::MediaStreamAttr stream_attr;
    stream_attr.videoType = video_type;
    stream_attr.audioType = MediaStream::MediaStreamAttr::AUDIO_TYPE_AAC;
    stream_attr.streamType = MediaStream::MediaStreamAttr::STREAM_TYPE_UNICAST;
    stream_sender = rtsp_server->createMediaStream(configPara->OutputFilePath, stream_attr);
    if (stream_sender == NULL) {
        printf("rtsp stream sender create failed!\n");
        goto _exit;
    }

    rtsp_server->runWithNewThread();

    printf("================================================================\n");
    printf("\trtsp url[ %s ]\n", stream_sender->streamURL().c_str());
    printf("================================================================\n");

    return 0;

_exit:
    deinit_rtsp_module();
    return -1;
}

static int frame_data_consumer(unsigned char *data, int size, uint64_t pts)
{
    stream_sender->appendVideoData(data, size, pts, MediaStream::FRAME_DATA_TYPE_UNKNOW);

#ifdef SAVE_FILE
    fwrite(data, size, 1, OutputFile_Fd);
#endif

    return 0;
}

static int process_frame(const VENC_STREAM_S *VencFrame)
{
    VENC_DATA_TYPE_U nalu_type_i;
    bool have_vps = false;

    if (privCap.EncoderType == PT_H264) {
        nalu_type_i.enH264EType = H264E_NALU_ISLICE;                         /*ISLICE types*/
        have_vps = false;
    } else if (privCap.EncoderType == PT_H265) {
        nalu_type_i.enH265EType = H265E_NALU_ISLICE;                         /*ISLICE types*/
        have_vps = true;
    } else {
        printf("unknown encoder type!\n");
        nalu_type_i.enH264EType = H264E_NALU_ISLICE;                         /*ISLICE types*/
        have_vps = false;
    }

    if ((VencFrame->mpPack->mDataType.enH264EType == nalu_type_i.enH264EType)
            || (VencFrame->mpPack->mDataType.enH265EType == nalu_type_i.enH265EType)) {
        VencHeaderData vencheader;
        AW_MPI_VENC_GetH264SpsPpsInfo(mVeChn, &vencheader);
        if(vencheader.nLength)
        {
            frame_data_consumer(vencheader.pBuffer,vencheader.nLength, VencFrame->mpPack->mPTS);
        }

        if (have_vps)
        {
            AW_MPI_VENC_GetH265SpsPpsInfo(mVeChn, &vencheader);
            if(vencheader.nLength)
            {
                frame_data_consumer(vencheader.pBuffer,vencheader.nLength, VencFrame->mpPack->mPTS);
            }
        }
    }

    unsigned char *data_ptr = NULL;
    int data_size = 0;
    if(VencFrame->mpPack != NULL && VencFrame->mpPack->mLen1)
    {
        int sum_size = (VencFrame->mpPack->mLen0 + VencFrame->mpPack->mLen1);
        if (frame_data_size < sum_size)
        {
            free(frame_data);
            frame_data = (unsigned char*)malloc(sum_size);
            if (frame_data == NULL) {
               printf("malloc frame data failed!\n");
               frame_data_size = 0;
               return -1;
            }
            frame_data_size = sum_size;
        }

        memcpy(frame_data, VencFrame->mpPack->mpAddr0, VencFrame->mpPack->mLen0);
        memcpy(frame_data + VencFrame->mpPack->mLen0, VencFrame->mpPack->mpAddr1, VencFrame->mpPack->mLen1);
        data_ptr = frame_data;
        data_size = sum_size;
    } else {
        data_ptr = VencFrame->mpPack->mpAddr0;
        data_size = VencFrame->mpPack->mLen0;
    }

    frame_data_consumer(data_ptr, data_size, VencFrame->mpPack->mPTS);

    return 0;
}

static int hal_vipp_start(VI_DEV ViDev, VI_ATTR_S *pstAttr)
{
    AW_MPI_VI_CreateVipp(ViDev);
    AW_MPI_VI_SetVippAttr(ViDev, pstAttr);
    AW_MPI_VI_EnableVipp(ViDev);
    return 0;
}

static int hal_vipp_end(VI_DEV ViDev)
{
    AW_MPI_VI_DisableVipp(ViDev);
    AW_MPI_VI_DestoryVipp(ViDev);
    return 0;
}

static int hal_virvi_start(VI_DEV ViDev, VI_CHN ViCh, void *pAttr)
{
    int ret = -1;

    ret = AW_MPI_VI_CreateVirChn(ViDev, ViCh, pAttr);
    if(ret < 0)
    {
        aloge("Create VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
    ret = AW_MPI_VI_SetVirChnAttr(ViDev, ViCh, pAttr);
    if(ret < 0)
    {
        aloge("Set VI ChnAttr failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }

    return 0;
}

static int hal_virvi_end(VI_DEV ViDev, VI_CHN ViCh)
{
    int ret = -1;
#if 0
    /* better be invoked after AW_MPI_VENC_StopRecvPic */
    ret = AW_MPI_VI_DisableVirChn(ViDev, ViCh);
    if(ret < 0)
    {
        aloge("Disable VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
#endif
    ret = AW_MPI_VI_DestoryVirChn(ViDev, ViCh);
    if(ret < 0)
    {
        aloge("Destory VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
    return 0;
}

static void *GetEncoderFrameThread(void *pArg)
{
    VI_DEV ViDev;
    VI_CHN ViCh;
    int ret = 0;
    int count = 0;

    VI2Venc_Cap_S *pCap = (VI2Venc_Cap_S *)pArg;
    ViDev = pCap->Dev;
    ViCh = pCap->Chn;
    mViDev = ViDev;
    mViChn = ViCh;
    VENC_STREAM_S VencFrame;
    VENC_PACK_S venc_pack;
    VencFrame.mPackCount = 1;
    VencFrame.mpPack = &venc_pack;
    printf("Cap threadid=0x%lx, ViDev = %d, ViCh = %d\n", pCap->thid, ViDev, ViCh);

    if (mVeChn >= 0 && mViChn >= 0)
    {
        MPP_CHN_S ViChn = {MOD_ID_VIU, mViDev, mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVeChn};
        ret = AW_MPI_SYS_Bind(&ViChn,&VeChn);
        if(ret !=SUCCESS)
        {
            printf("error!!! vi can not bind venc!!!\n");
            return (void*)FAILURE;
        }
    }
    //printf("start start recv success!\n");
    ret = AW_MPI_VI_EnableVirChn(ViDev, ViCh);
    if (ret != SUCCESS)
    {
        printf("VI enable error!");
        return (void*)FAILURE;
    }
    ret = AW_MPI_VENC_StartRecvPic(mVeChn);
    if (ret != SUCCESS)
    {
        printf("VENC Start RecvPic error!");
        return (void*)FAILURE;
    }
	stop_enc = 0;
	//printf("**********************Enter GetEncoderFrameThread*******************\n");
    while(!stop_enc)
    {
		//count++;
        if((ret = AW_MPI_VENC_GetStream(mVeChn,&VencFrame, 1000)) < 0) //6000(25fps) 4000(30fps)
        {
            printf("get first frmae failed! stop_enc = %d, mVeChn=%d, ViDev=%d, ViCh=%d\n", stop_enc, mVeChn, ViDev, ViCh);
            continue;
        }
        else
        {
            process_frame(&VencFrame);

            ret = AW_MPI_VENC_ReleaseStream(mVeChn,&VencFrame);
            if(ret < 0)
            {
                printf("falied error,release failed!!!\n");
            }
        }
    }
	printf("*********************Exit GetEncoderFrameThread*******************\n");
    return NULL;
}

extern "C" int init_venc_module(const Virvi2VencConfig *configPara)
{
    init_rtsp_module(configPara);

    int ret, result = 0;
    int virvi_chn = 0;

#ifdef SAVE_FILE
    //open output file
    OutputFile_Fd = fopen(stContext.mConfigPara.OutputFilePath, "wb+");
    if(!OutputFile_Fd)
    {
        aloge("fatal error! can't open yuv file[%s]", stContext.mConfigPara.OutputFilePath);
        result = -1;
        goto _exit;
    }
#endif

    MPP_SYS_CONF_S mSysConf;
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    ret = AW_MPI_SYS_Init();
    if (ret < 0)
    {
        aloge("sys Init failed!");
        return -1;
    }
    VI_ATTR_S stAttr;
    vipp_dev = configPara->DevNum;
    /* dev:0, chn:0,1,2,3,4...16 */
    /* dev:1, chn:0,1,2,3,4...16 */
    /* dev:2, chn:0,1,2,3,4...16 */
    /* dev:3, chn:0,1,2,3,4...16 */
    /*Set VI Channel Attribute*/
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
    stAttr.format.field = V4L2_FIELD_NONE;
    stAttr.format.width = configPara->SrcWidth;
    stAttr.format.height = configPara->SrcHeight;
    stAttr.fps = configPara->SrcFrameRate;
    /* update configuration anyway, do not use current configuration */
    stAttr.use_current_win = 0;
    stAttr.nbufs = 5;
    stAttr.nplanes = 2;

    /* MPP components */
    mVeChn = 0;

    /* venc chn attr */
    int VIFrameRate = configPara->SrcFrameRate;
    int VencFrameRate = configPara->DestFrameRate;
    VENC_CHN_ATTR_S mVEncChnAttr;
    memset(&mVEncChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    SIZE_S wantedVideoSize = {configPara->DestWidth, configPara->DestHeight};
    SIZE_S videosize = {configPara->SrcWidth, configPara->SrcHeight};
    PAYLOAD_TYPE_E videoCodec = configPara->EncoderType;
    PIXEL_FORMAT_E wantedPreviewFormat = configPara->DestPicFormat;
    int wantedFrameRate = configPara->DestFrameRate;
    mVEncChnAttr.VeAttr.Type = videoCodec;
    mVEncChnAttr.VeAttr.SrcPicWidth = videosize.Width;
    mVEncChnAttr.VeAttr.SrcPicHeight = videosize.Height;
    mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    mVEncChnAttr.VeAttr.PixelFormat = wantedPreviewFormat;
    int wantedVideoBitRate = configPara->DestBitRate;
    if(PT_H264 == mVEncChnAttr.VeAttr.Type)
    {
        mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrH264e.Profile = 2;
        mVEncChnAttr.VeAttr.AttrH264e.PicWidth = wantedVideoSize.Width;
        mVEncChnAttr.VeAttr.AttrH264e.PicHeight = wantedVideoSize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
        mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = wantedFrameRate;
        if(configPara->mTimeLapseEnable)
        {
            mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = 1000 + (configPara->mTimeBetweenFrameCapture<<16);
        }
        else
        {
            mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = wantedFrameRate;
        }
        mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = wantedVideoBitRate;
    }
    else if(PT_H265 == mVEncChnAttr.VeAttr.Type)
    {
        mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = wantedVideoSize.Width;
        mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = wantedVideoSize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265CBR;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.fr32DstFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.mBitRate = wantedVideoBitRate;
    }
    else if(PT_MJPEG == mVEncChnAttr.VeAttr.Type)
    {
        mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= videosize.Width;
        mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = videosize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.fr32DstFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = wantedVideoBitRate;
    }

#if 0
    /* has invoked in AW_MPI_SYS_Init() */
    result = VENC_Construct();
    if (result != SUCCESS)
    {
        printf("VENC Construct error!");
        result = -1;
        goto _exit;
    }
#endif

    hal_vipp_start(vipp_dev, &stAttr);
    memset(&privCap, 0, sizeof(VI2Venc_Cap_S));
    privCap.Dev = vipp_dev;
    privCap.s32MilliSec = 5000;  // 2000;
    privCap.EncoderType = mVEncChnAttr.VeAttr.Type;

    /* open isp */
    if (vipp_dev == 0 || vipp_dev == 2) {
        isp_dev = 1;
    } else if (vipp_dev == 1 || vipp_dev == 3) {
        isp_dev = 0;
    }

#ifdef USE_ISP
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(isp_dev);
#endif

    result = hal_virvi_start(vipp_dev, 0, NULL);
    if(result < 0)
    {
        printf("VI start failed!\n");
        result = -1;
        goto _init_exit;
    }

    privCap.thid = 0;
    result = AW_MPI_VENC_CreateChn(mVeChn, &mVEncChnAttr);
    if(result < 0)
    {
        printf("create venc channel[%d] falied!\n", mVeChn);
        result = -1;
        goto _init_exit;
    }

    VENC_FRAME_RATE_S stFrameRate;
    stFrameRate.SrcFrmRate = VIFrameRate;
    stFrameRate.DstFrmRate = VencFrameRate;
    AW_MPI_VENC_SetFrameRate(mVeChn, &stFrameRate);

    result = pthread_create(&privCap.thid, NULL, GetEncoderFrameThread, (void *)&privCap);
    if (result < 0)
    {
        alogd("pthread_create failed, Dev[%d], Chn[%d].\n", privCap.Dev, privCap.Chn);
        goto _init_exit;
    }

    return 0;

_init_exit:
    return result;
}

extern "C" int wait_venc_module_stop()
{
    pthread_join(privCap.thid, NULL);

    return 0;
}

extern "C" int deinit_venc_module()
{
    int result = 0;
    int ret = 0;

    stop_enc = 1;

    result = AW_MPI_VENC_StopRecvPic(mViChn);
    if (result != SUCCESS)
    {
        printf("VENC Stop Receive Picture error!");
        result = -1;
        goto _exit;
    }

    /* better call AW_MPI_VI_DisableVirChn immediately after AW_MPI_VENC_StopRecvPic was invoked */
    ret = AW_MPI_VI_DisableVirChn(privCap.Dev, 0);
    if(ret < 0)
    {
        aloge("Disable VI Chn failed,VIDev = %d,VIChn = %d", vipp_dev, 0);
        return ret ;
    }

    result = AW_MPI_VENC_ResetChn(mVeChn);
    if (result != SUCCESS)
    {
        printf("VENC Reset Chn error!");
        result = -1;
        goto _exit;
    }

    AW_MPI_VENC_DestroyChn(mVeChn);
    if (result != SUCCESS)
    {
        printf("VENC Destroy Chn error!");
        result = -1;
        goto _exit;
    }

    result = hal_virvi_end(vipp_dev, 0);
    if(result < 0)
    {
        printf("VI end failed!\n");
        result = -1;
        goto _exit;
    }

#ifdef USE_ISP
    AW_MPI_ISP_Stop(isp_dev);
    AW_MPI_ISP_Exit();
#endif

    hal_vipp_end(vipp_dev);
    /* exit mpp systerm */
    ret = AW_MPI_SYS_Exit();
    if (ret < 0)
    {
        aloge("sys exit failed!");
        return -1;
    }

    if (frame_data) {
        free(frame_data);
        frame_data = NULL;
        frame_data_size = 0;
    }

    deinit_rtsp_module();

#ifdef SAVE_FILE
    fclose(OutputFile_Fd);
#endif

    return 0;

_exit:
    return result;
}

extern "C" int venc_stop(int VeChn)
{
#if 1
	int ret, count = 0,result = 0;
	int vipp_dev, virvi_chn = 0;
	int isp_dev;

	vipp_dev = 1;
	if (vipp_dev == 0 || vipp_dev == 2) {
        isp_dev = 1;
    } else if (vipp_dev == 1 || vipp_dev == 3) {
        isp_dev = 0;
    }

	result = AW_MPI_VENC_StopRecvPic(VeChn);
	if (result != SUCCESS)
	{
		printf("VENC Stop Receive Picture error!");
		result = -1;
		goto _exit;
	}
	stop_enc=1;
	if(privCap.thid != 0)
		pthread_join(privCap.thid, NULL);
	printf("*******************pthread_join privCap.thid=0x%lx*******************\n", privCap.thid);
	
	/* better call AW_MPI_VI_DisableVirChn immediately after AW_MPI_VENC_StopRecvPic was invoked */
	ret = AW_MPI_VI_DisableVirChn(vipp_dev, 0);
	if(ret < 0)
	{
		printf("Disable VI Chn failed,VIDev = %d,VIChn = %d", vipp_dev, virvi_chn);
		return ret ;
	}

	result = AW_MPI_VENC_ResetChn(VeChn);
	if (result != SUCCESS)
	{
		printf("VENC Reset Chn error!");
		result = -1;
		goto _exit;
	}
	AW_MPI_VENC_DestroyChn(VeChn);
	if (result != SUCCESS)
	{
		printf("VENC Destroy Chn error!");
		result = -1;
		goto _exit;
	}
	for (virvi_chn = 0; virvi_chn < 1; virvi_chn++)
	{
		result = hal_virvi_end(vipp_dev, virvi_chn);
		if(result < 0)
		{
			printf("VI end failed!\n");
			result = -1;
			goto _exit;
		}
	}
	AW_MPI_ISP_Stop(isp_dev);
	AW_MPI_ISP_Exit();
	hal_vipp_end(vipp_dev);
	/* exit mpp systerm */
	ret = AW_MPI_SYS_Exit();
	if (ret < 0)
	{
		printf("sys exit failed!");
		return -1;
	}
_exit:
		return result;
#else
	int isp_dev = 0, vipp_dev = 1, virvi_chn = 0;
	int result = -1;
	result = AW_MPI_VENC_StopRecvPic(VeChn);
	if (vipp_dev == 0 || vipp_dev == 2) {
        isp_dev = 1;
    } else if (vipp_dev == 1 || vipp_dev == 3) {
        isp_dev = 0;
    }
    if (result != SUCCESS)
    {
        printf("VENC Stop Receive Picture error!\n");
        result = -1;
        return result ;
    }

    /* better call AW_MPI_VI_DisableVirChn immediately after AW_MPI_VENC_StopRecvPic was invoked */
    result = AW_MPI_VI_DisableVirChn(vipp_dev, virvi_chn);
    if(result < 0)
    {
        printf("Disable VI Chn failed,VIDev = %d,VIChn = %d\n", vipp_dev, virvi_chn);
		result = -1;
		return result ;
    }
	
    result = AW_MPI_VENC_ResetChn(VeChn);
    if (result != SUCCESS)
    {
        printf("VENC Reset Chn error!\n");
        result = -1;
		return result ;
    }
    result = AW_MPI_VENC_DestroyChn(VeChn);
    if (result != SUCCESS)
    {
        printf("VENC Destroy Chn error!\n");
        result = -1;
		return result ;
    }
	result = hal_virvi_end(vipp_dev, virvi_chn);
    if(result < 0)
    {
        printf("VI end failed!\n");
        result = -1;
		return result ;
    }
	AW_MPI_ISP_Stop(isp_dev);
    AW_MPI_ISP_Exit();

    hal_vipp_end(vipp_dev);
    /* exit mpp systerm */
    result = AW_MPI_SYS_Exit();
    if (result < 0)
    {
        printf("sys exit failed!\n");
		result = -1;
        return result;
    }
	return result;
#endif
}
extern "C" int venc_start(int VeChn, Virvi2VencConfig *configPara, VENC_CHN_ATTR_S *VencAttr)
{
#if 1
		int ret, count = 0,result = 0;
		int vipp_dev, virvi_chn;
		int isp_dev;
		//stop_enc = 0;
		MPP_SYS_CONF_S mSysConf;
        memset(&mSysConf, 0, sizeof(MPP_SYS_CONF_S));
        mSysConf.nAlignWidth = 32;
        AW_MPI_SYS_SetConf(&mSysConf);
        ret = AW_MPI_SYS_Init();
        if (ret < 0)
        {
            printf("sys Init failed!");
            return -1;
        }
        VI_ATTR_S stAttr;
        vipp_dev = 1;
        /* dev:0, chn:0,1,2,3,4...16 */
        /* dev:1, chn:0,1,2,3,4...16 */
        /* dev:2, chn:0,1,2,3,4...16 */
        /* dev:3, chn:0,1,2,3,4...16 */
        /*Set VI Channel Attribute*/
        stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        stAttr.memtype = V4L2_MEMORY_MMAP;
        stAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
        stAttr.format.field = V4L2_FIELD_NONE;
        stAttr.format.width = configPara->SrcWidth;
        stAttr.format.height = configPara->SrcHeight;
        stAttr.fps = configPara->SrcFrameRate;
        /* update configuration anyway, do not use current configuration */
        stAttr.use_current_win = 0;
        stAttr.nbufs = 5;
        stAttr.nplanes = 2;

        /* venc chn attr */
        int VIFrameRate = configPara->SrcFrameRate;
        int VencFrameRate = configPara->DestFrameRate;

        hal_vipp_start(vipp_dev, &stAttr);
        // for (virvi_chn = 0; virvi_chn < MAX_VIR_CHN_NUM; virvi_chn++)
        for (virvi_chn = 0; virvi_chn < 1; virvi_chn++)
        {
            memset(&privCap, 0, sizeof(VI2Venc_Cap_S));
            privCap.Dev = vipp_dev;
            privCap.Chn = virvi_chn;
            privCap.s32MilliSec = 5000;  // 2000;
            privCap.EncoderType = VencAttr->VeAttr.Type;
            if (0 == virvi_chn) /* H264, H265, MJPG, Preview(LCD or HDMI), VDA, ISE, AIE, CVBS */
            {
                /* open isp */
                if (vipp_dev == 0 || vipp_dev == 2) {
                    isp_dev = 1;
                } else if (vipp_dev == 1 || vipp_dev == 3) {
                    isp_dev = 0;
                }
                AW_MPI_ISP_Init();
                AW_MPI_ISP_Run(isp_dev);

                result = hal_virvi_start(vipp_dev, virvi_chn, NULL);
                if(result < 0)
                {
                    printf("VI start failed!\n");
                    result = -1;
                    goto _exit;
                }
                privCap.thid = 0;
                result = AW_MPI_VENC_CreateChn(VeChn, VencAttr);
                if(result < 0)
                {
                    printf("create venc channel[%d] falied!\n", VeChn);
                    result = -1;
                    goto _exit;
                }
                VENC_FRAME_RATE_S stFrameRate;
                stFrameRate.SrcFrmRate = VIFrameRate;
                stFrameRate.DstFrmRate = VencFrameRate;
                AW_MPI_VENC_SetFrameRate(VeChn, &stFrameRate);
                VencHeaderData vencheader;
                if(PT_H264 == VencAttr->VeAttr.Type)
                {
                    AW_MPI_VENC_GetH264SpsPpsInfo(VeChn, &vencheader);
                }
                else if(PT_H265 == VencAttr->VeAttr.Type)
                {
                    AW_MPI_VENC_GetH265SpsPpsInfo(VeChn, &vencheader);
                }
                result = pthread_create(&privCap.thid, NULL, GetEncoderFrameThread, (void *)&privCap);
                if (result < 0)
                {
                    printf("pthread_create failed, Dev[%d], Chn[%d].\n", privCap.Dev, privCap.Chn);
                    continue;
                }
            }
        }
		/*for (virvi_chn = 0; virvi_chn < 1; virvi_chn++)
        {
            pthread_join(privCap.thid, NULL);
        }*/
		//printf("**********************Exit venc_start*************************\n");
_exit:
	return result;

#else
	int isp_dev = 0, vipp_dev = 1, virvi_chn = 0;
	int result = -1;
	if (vipp_dev == 0 || vipp_dev == 2) {
        isp_dev = 1;
    } else if (vipp_dev == 1 || vipp_dev == 3) {
        isp_dev = 0;
    }
	MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    result = AW_MPI_SYS_Init();
    if (result < 0)
    {
        printf("sys Init failed!");
        return -1;
    }
	VI_ATTR_S stAttr;
	/* dev:0, chn:0,1,2,3,4...16 */
	/* dev:1, chn:0,1,2,3,4...16 */
	/* dev:2, chn:0,1,2,3,4...16 */
	/* dev:3, chn:0,1,2,3,4...16 */
	/*Set VI Channel Attribute*/
	stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	stAttr.memtype = V4L2_MEMORY_MMAP;
	stAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
	stAttr.format.field = V4L2_FIELD_NONE;
	stAttr.format.width = 1280;
	stAttr.format.height = 720;
	stAttr.fps = 25;
	/* update configuration anyway, do not use current configuration */
	stAttr.use_current_win = 0;
	stAttr.nbufs = 5;
	stAttr.nplanes = 2;
	hal_vipp_start(VeChn,&stAttr);
	if (0 == virvi_chn) /* H264, H265, MJPG, Preview(LCD or HDMI), VDA, ISE, AIE, CVBS */
    {
        AW_MPI_ISP_Init();
        AW_MPI_ISP_Run(isp_dev);

        result = hal_virvi_start(vipp_dev, virvi_chn, NULL);
        if(result < 0)
        {
            printf("VI start failed!\n");
            result = -1;
            return result;
        }
		result = AW_MPI_VENC_CreateChn(VeChn, VencAttr);
	    if(result < 0)
	    {
	        printf("create venc channel[%d] falied!\n", VeChn);
	        result = -1;
	        return result;
	    }

		result = AW_MPI_VENC_StartRecvPic(VeChn);
		if(result < 0)
		{
			printf("startrecvpic failed!\n");
			result = -1;
			return result;
		}
	}
	return result;
#endif
}
extern "C" int venc_set_cfg(int VeChn, unsigned char group_id, unsigned int cfg_ids, void *cfg_data)
{
	int ret = 0;
	unsigned char *data_ptr = (unsigned char *)cfg_data;
	switch (group_id)
	{
	case HW_VENC_CFG_COMMON: /* venc_common */
		if (cfg_ids & HW_VENC_CFG_COMMON_PROC) /* proc */
		{
			venc_proc_cfg *venc_proc_data= (venc_proc_cfg *)data_ptr;
			struct VeProcSet data_set;
			data_set.bProcEnable = (unsigned char)venc_proc_data->bProcEnable;
			data_set.nProcFreq = venc_proc_data->nProcFreq;
			data_set.nStatisBitRateTime = venc_proc_data->nStatisBitRateTime;
			data_set.nStatisFrRateTime = venc_proc_data->nStatisFrRateTime;
			printf("PROC: Enable:%d Freq:%d BitRareTime:%d FrRateTime:%d \n",data_set.bProcEnable, data_set.nProcFreq, 
				    data_set.nStatisBitRateTime, data_set.nStatisFrRateTime);
			ret = AW_MPI_VENC_SetProcSet(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_proc_cfg);
		}

		if (cfg_ids & HW_VENC_CFG_COMMON_SAVEBSFILE) /* saveBSfile */
		{
			venc_savebsfile_cfg *venc_savebsfile_data= (venc_savebsfile_cfg *)data_ptr;
			struct VencSaveBSFile data_set;
			memcpy(&(data_set.filename[0]), &(venc_savebsfile_data->filename[0]), sizeof(data_set.filename));
			data_set.save_bsfile_flag = venc_savebsfile_data->save_bsfile_flag;
			data_set.save_start_time = venc_savebsfile_data->save_start_time;
			data_set.save_end_time = venc_savebsfile_data->save_end_time;
			printf("SAVEBSFILE: filename:%s  bsfile_flag:%d  save_start_time:%d data_set.save_end_time:%d \n",data_set.filename, data_set.save_bsfile_flag,
				   data_set.save_start_time, data_set.save_end_time);
			ret = AW_MPI_VENC_SaveBsFile(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_savebsfile_cfg);
		}

		if (cfg_ids & HW_VENC_CFG_COMMON_3DNR) /* 3DNR */
		{
			venc_3DNR_cfg *venc_3DNR_data= (venc_3DNR_cfg *)data_ptr;
			ret = AW_MPI_VENC_Set3DNR(VeChn, venc_3DNR_data->flag_3DNR);

			/* offset */
			data_ptr += sizeof(struct venc_savebsfile_cfg);
		}
		break;
	case HW_VENC_CFG_H264:
		if (cfg_ids & HW_VENC_CFG_H264_ATTR_CBR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			data_set.VeAttr.Type = PT_H264;
			data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
			data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
			data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
			data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
			data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
			data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
			data_set.VeAttr.AttrH264e.bByFrame = venc_attr_H264_H265_data->bByFrame;
			data_set.VeAttr.AttrH264e.BFrameNum = venc_attr_H264_H265_data->BFrameNum;
			data_set.VeAttr.AttrH264e.BufSize = venc_attr_H264_H265_data->BufSize;
			data_set.VeAttr.AttrH264e.FastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
			data_set.VeAttr.AttrH264e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
			data_set.VeAttr.AttrH264e.MaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
			data_set.VeAttr.AttrH264e.MaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
			data_set.VeAttr.AttrH264e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
			data_set.VeAttr.AttrH264e.PicHeight = venc_attr_H264_H265_data->PicHeight;
			data_set.VeAttr.AttrH264e.PicWidth = venc_attr_H264_H265_data->PicWidth;
			data_set.VeAttr.AttrH264e.Profile = venc_attr_H264_H265_data->Profile;
			data_set.VeAttr.AttrH264e.RefNum = venc_attr_H264_H265_data->RefNum;
			data_set.RcAttr.mAttrH264Cbr.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			data_set.RcAttr.mAttrH264Cbr.mBitRate = venc_attr_H264_H265_data->mBitRate;
			data_set.RcAttr.mAttrH264Cbr.mFluctuateLevel = venc_attr_H264_H265_data->mFluctuateLevel;
			data_set.RcAttr.mAttrH264Cbr.mGop = venc_attr_H264_H265_data->mGop;
			data_set.RcAttr.mAttrH264Cbr.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
			data_set.RcAttr.mAttrH264Cbr.mStatTime = venc_attr_H264_H265_data->mStatTime;
			data_set.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
			if(venc_attr_H264_H265_data->enGopMode == 0)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
				data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 1)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
				data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
				data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 2)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
				data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
				data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 3)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
				data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
				data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			
			Virvi2VencConfig configPara;
			memset(&configPara, 0, sizeof(Virvi2VencConfig));
			configPara.DevNum = 1;
			configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
			configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
			configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
			configPara.EncoderType = PT_H264;
			configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
			configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
			configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
			configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

			printf("first stop venc.......\n");
			ret = venc_stop(VeChn);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_stop failed!\n");
				return ret;
			}
			printf("start venc...........\n");
			ret = venc_start(VeChn, &configPara, &data_set);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_start failed!\n");
				return ret;
			}
	
			//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
		}
		
		if (cfg_ids & HW_VENC_CFG_H264_ATTR_VBR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			data_set.VeAttr.Type = PT_H264;
			data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
			data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
			data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
			data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
			data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
			data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
			data_set.VeAttr.AttrH264e.bByFrame = venc_attr_H264_H265_data->bByFrame;
			data_set.VeAttr.AttrH264e.BFrameNum = venc_attr_H264_H265_data->BFrameNum;
			data_set.VeAttr.AttrH264e.BufSize = venc_attr_H264_H265_data->BufSize;
			data_set.VeAttr.AttrH264e.FastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
			data_set.VeAttr.AttrH264e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
			data_set.VeAttr.AttrH264e.MaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
			data_set.VeAttr.AttrH264e.MaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
			data_set.VeAttr.AttrH264e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
			data_set.VeAttr.AttrH264e.PicHeight = venc_attr_H264_H265_data->PicHeight;
			data_set.VeAttr.AttrH264e.PicWidth = venc_attr_H264_H265_data->PicWidth;
			data_set.VeAttr.AttrH264e.Profile = venc_attr_H264_H265_data->Profile;
			data_set.VeAttr.AttrH264e.RefNum = venc_attr_H264_H265_data->RefNum;
			data_set.RcAttr.mAttrH264Vbr.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			data_set.RcAttr.mAttrH264Vbr.mGop = venc_attr_H264_H265_data->mGop;
			data_set.RcAttr.mAttrH264Vbr.mMaxBitRate = venc_attr_H264_H265_data->mMaxBitRate;
			data_set.RcAttr.mAttrH264Vbr.mMaxQp = venc_attr_H264_H265_data->mMaxQp;
			data_set.RcAttr.mAttrH264Vbr.mMinQp = venc_attr_H264_H265_data->mMinQp;
			data_set.RcAttr.mAttrH264Vbr.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
			data_set.RcAttr.mAttrH264Vbr.mStatTime  = venc_attr_H264_H265_data->mStatTime;
			data_set.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
			if(venc_attr_H264_H265_data->enGopMode == 0)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
				data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 1)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
				data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
				data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 2)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
				data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
				data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 3)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
				data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
				data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}

			Virvi2VencConfig configPara;
			memset(&configPara, 0, sizeof(Virvi2VencConfig));
			configPara.DevNum = 1;
			configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
			configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
			configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
			configPara.EncoderType = PT_H264;
			configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
			configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
			configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
			configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

			printf("first stop venc.......\n");
			ret = venc_stop(VeChn);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_stop failed!\n");
				return ret;
			}
			printf("start venc...........\n");
			ret = venc_start(VeChn, &configPara, &data_set);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_start failed!\n");
				return ret;
			}

			//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
		}

		if (cfg_ids & HW_VENC_CFG_H264_ATTR_FIXQP) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			data_set.VeAttr.Type = PT_H264;
			data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
			data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
			data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
			data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
			data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
			data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
			data_set.VeAttr.AttrH264e.bByFrame = venc_attr_H264_H265_data->bByFrame;
			data_set.VeAttr.AttrH264e.BFrameNum = venc_attr_H264_H265_data->BFrameNum;
			data_set.VeAttr.AttrH264e.BufSize = venc_attr_H264_H265_data->BufSize;
			data_set.VeAttr.AttrH264e.FastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
			data_set.VeAttr.AttrH264e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
			data_set.VeAttr.AttrH264e.MaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
			data_set.VeAttr.AttrH264e.MaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
			data_set.VeAttr.AttrH264e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
			data_set.VeAttr.AttrH264e.PicHeight = venc_attr_H264_H265_data->PicHeight;
			data_set.VeAttr.AttrH264e.PicWidth = venc_attr_H264_H265_data->PicWidth;
			data_set.VeAttr.AttrH264e.Profile = venc_attr_H264_H265_data->Profile;
			data_set.VeAttr.AttrH264e.RefNum = venc_attr_H264_H265_data->RefNum;
			data_set.RcAttr.mAttrH264FixQp.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			data_set.RcAttr.mAttrH264FixQp.mGop = venc_attr_H264_H265_data->mGop;
			data_set.RcAttr.mAttrH264FixQp.mIQp = venc_attr_H264_H265_data->mIQp;
			data_set.RcAttr.mAttrH264FixQp.mPQp = venc_attr_H264_H265_data->mPQp;
			data_set.RcAttr.mAttrH264FixQp.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
			data_set.RcAttr.mRcMode = VENC_RC_MODE_H264FIXQP;
			if(venc_attr_H264_H265_data->enGopMode == 0)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
				data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 1)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
				data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
				data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 2)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
				data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
				data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 3)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
				data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
				data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}

			Virvi2VencConfig configPara;
			memset(&configPara, 0, sizeof(Virvi2VencConfig));
			configPara.DevNum = 1;
			configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
			configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
			configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
			configPara.EncoderType = PT_H264;
			configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
			configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
			configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
			configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

			printf("first stop venc.......\n");
			ret = venc_stop(VeChn);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_stop failed!\n");
				return ret;
			}
			printf("start venc...........\n");
			ret = venc_start(VeChn, &configPara, &data_set);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_start failed!\n");
				return ret;
			}
			//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
		}	

		if(cfg_ids & HW_VENC_CFG_H264_ATTR_ABR)
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			data_set.VeAttr.Type = PT_H264;
			data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
			data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
			data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
			data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
			data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
			data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
			data_set.VeAttr.AttrH264e.bByFrame = venc_attr_H264_H265_data->bByFrame;
			data_set.VeAttr.AttrH264e.BFrameNum = venc_attr_H264_H265_data->BFrameNum;
			data_set.VeAttr.AttrH264e.BufSize = venc_attr_H264_H265_data->BufSize;
			data_set.VeAttr.AttrH264e.FastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
			data_set.VeAttr.AttrH264e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
			data_set.VeAttr.AttrH264e.MaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
			data_set.VeAttr.AttrH264e.MaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
			data_set.VeAttr.AttrH264e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
			data_set.VeAttr.AttrH264e.PicHeight = venc_attr_H264_H265_data->PicHeight;
			data_set.VeAttr.AttrH264e.PicWidth = venc_attr_H264_H265_data->PicWidth;
			data_set.VeAttr.AttrH264e.Profile = venc_attr_H264_H265_data->Profile;
			data_set.VeAttr.AttrH264e.RefNum = venc_attr_H264_H265_data->RefNum;
			data_set.RcAttr.mAttrH264Abr.mMaxBitRate = venc_attr_H264_H265_data->mMaxBitRate;
			//data_set.RcAttr.mAttrH264Abr.mRatioChangeQp= venc_attr_H264_H265_data->mRatioChangeQp;
			//data_set.RcAttr.mAttrH264Abr.mQuality = venc_attr_H264_H265_data->mQuality;
			data_set.RcAttr.mRcMode = VENC_RC_MODE_H264ABR;
			if(venc_attr_H264_H265_data->enGopMode == 0)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
				data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 1)
			{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
				data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
				data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
			}
			if(venc_attr_H264_H265_data->enGopMode == 2)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
				data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
				data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
				data_set.GopAttr.stSmartP.mVirtualIInterval = venc_attr_H264_H265_data->mVirtualIInterval;
			}
			if(venc_attr_H264_H265_data->enGopMode == 3)
			{
				data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
				data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
				data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
				data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
			}
			
			Virvi2VencConfig configPara;
			memset(&configPara, 0, sizeof(Virvi2VencConfig));
			configPara.DevNum = 1;
			configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
			configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
			configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
			configPara.EncoderType = PT_H264;
			configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
			configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
			configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
			configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
			configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

			printf("first stop venc.......\n");
			ret = venc_stop(VeChn);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_stop failed!\n");
				return ret;
			}
			printf("start venc...........\n");
			ret = venc_start(VeChn, &configPara, &data_set);
			if(ret < 0)
			{
				ret =-1;
				printf("venc_start failed!\n");
				return ret;
			}
			
			//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
		}
#if 0
		if (cfg_ids & HW_VENC_CFG_H264_RC_CBR) /* ATTR */
		{
			venc_rcparam_H264_H265_cfg *venc_rc_H264_H265_data= (venc_rcparam_H264_H265_cfg *)data_ptr;
			struct VENC_RC_PARAM_S data_set;
			memcpy(data_set.ThrdI, venc_rc_H264_H265_data->ThrdI, sizeof(data_set.ThrdI));
			memcpy(data_set.ThrdP, venc_rc_H264_H265_data->ThrdP, sizeof(data_set.ThrdP));
			data_set.RowQpDelta = venc_rc_H264_H265_data->RowQpDelta;
			data_set.pRcParam = 0;
			data_set.ParamH264Cbr.IPQPDelta = venc_rc_H264_H265_data->IPQPDelta;
			data_set.ParamH264Cbr.MaxIprop = venc_rc_H264_H265_data->MaxIprop;
			data_set.ParamH264Cbr.MaxQp = venc_rc_H264_H265_data->MaxQp;
			data_set.ParamH264Cbr.MaxReEncodeTimes = venc_rc_H264_H265_data->MaxReEncodeTimes;
			data_set.ParamH264Cbr.MinIprop = venc_rc_H264_H265_data->MinIprop;
			data_set.ParamH264Cbr.MinIQp = venc_rc_H264_H265_data->MinIQp;
			data_set.ParamH264Cbr.MinQp = venc_rc_H264_H265_data->MinQp;
			data_set.ParamH264Cbr.QualityLevel = venc_rc_H264_H265_data->QualityLevel;
			ret = AW_MPI_VENC_SetRcParam(VeChn, &data_set);
		}
		
		if (cfg_ids & HW_VENC_CFG_H264_RC_VBR) /* ATTR */
		{
			venc_rcparam_H264_H265_cfg *venc_rc_H264_H265_data= (venc_rcparam_H264_H265_cfg *)data_ptr;
			struct VENC_RC_PARAM_S data_set;
			memcpy(data_set.ThrdI, venc_rc_H264_H265_data->ThrdI, sizeof(data_set.ThrdI));
			memcpy(data_set.ThrdP, venc_rc_H264_H265_data->ThrdP, sizeof(data_set.ThrdP));
			data_set.RowQpDelta = venc_rc_H264_H265_data->RowQpDelta;
			data_set.pRcParam = 0;
			data_set.ParamH264VBR.s32ChangePos = venc_rc_H264_H265_data->s32ChangePos;
			data_set.ParamH264VBR.s32IPQPDelta =  = venc_rc_H264_H265_data->IPQPDelta;
			data_set.ParamH264VBR.u32MaxIprop  = venc_rc_H264_H265_data->MaxIprop;
			data_set.ParamH264VBR.u32MinIprop = venc_rc_H264_H265_data->MinIprop;
			data_set.ParamH264VBR.u32MinIQP = venc_rc_H264_H265_data->MinIQp;
			ret = AW_MPI_VENC_SetRcParam(VeChn, &data_set);
		}
#endif
		break;
	case HW_VENC_CFG_H265:
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_CBR) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				data_set.VeAttr.Type = PT_H265;
				data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
				data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
				data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
				data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
				data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
				data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
				data_set.VeAttr.AttrH265e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
				data_set.VeAttr.AttrH265e.mbByFrame = venc_attr_H264_H265_data->bByFrame;
				data_set.VeAttr.AttrH265e.mBFrameNum = venc_attr_H264_H265_data->BFrameNum;
				data_set.VeAttr.AttrH265e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
				data_set.VeAttr.AttrH265e.mBufSize = venc_attr_H264_H265_data->BufSize;
				data_set.VeAttr.AttrH265e.mFastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
				data_set.VeAttr.AttrH265e.mMaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
				data_set.VeAttr.AttrH265e.mMaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
				data_set.VeAttr.AttrH265e.mPicHeight = venc_attr_H264_H265_data->PicHeight;
				data_set.VeAttr.AttrH265e.mPicWidth = venc_attr_H264_H265_data->PicWidth;
				data_set.VeAttr.AttrH265e.mProfile = venc_attr_H264_H265_data->Profile;
				data_set.VeAttr.AttrH265e.mRefNum = venc_attr_H264_H265_data->RefNum;
				data_set.RcAttr.mAttrH265Cbr.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				data_set.RcAttr.mAttrH265Cbr.mBitRate = venc_attr_H264_H265_data->mBitRate;
				data_set.RcAttr.mAttrH265Cbr.mFluctuateLevel = venc_attr_H264_H265_data->mFluctuateLevel;
				data_set.RcAttr.mAttrH265Cbr.mGop = venc_attr_H264_H265_data->mGop;
				data_set.RcAttr.mAttrH265Cbr.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
				data_set.RcAttr.mAttrH265Cbr.mStatTime = venc_attr_H264_H265_data->mStatTime;
				data_set.RcAttr.mRcMode = VENC_RC_MODE_H265CBR;
				if(venc_attr_H264_H265_data->enGopMode == 0)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
					data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 1)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
					data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
					data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
					data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 2)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
					data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
					data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 3)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
					data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
					data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
		
				Virvi2VencConfig configPara;
				memset(&configPara, 0, sizeof(Virvi2VencConfig));
				configPara.DevNum = 1;
				configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
				configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
				configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
				configPara.EncoderType = PT_H265;
				configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
				configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
				configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
				configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

				printf("first stop venc.......\n");
				ret = venc_stop(VeChn);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_stop failed!\n");
					return ret;
				}
				printf("start venc...........\n");
				ret = venc_start(VeChn, &configPara, &data_set);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_start failed!\n");
					return ret;
				}
		
				//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);
		
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			}
			
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_VBR) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				data_set.VeAttr.Type = PT_H265;
				data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
				data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
				data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
				data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
				data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
				data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
				data_set.VeAttr.AttrH265e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
				data_set.VeAttr.AttrH265e.mbByFrame = venc_attr_H264_H265_data->bByFrame;
				data_set.VeAttr.AttrH265e.mBFrameNum = venc_attr_H264_H265_data->BFrameNum;
				data_set.VeAttr.AttrH265e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
				data_set.VeAttr.AttrH265e.mBufSize = venc_attr_H264_H265_data->BufSize;
				data_set.VeAttr.AttrH265e.mFastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
				data_set.VeAttr.AttrH265e.mMaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
				data_set.VeAttr.AttrH265e.mMaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
				data_set.VeAttr.AttrH265e.mPicHeight = venc_attr_H264_H265_data->PicHeight;
				data_set.VeAttr.AttrH265e.mPicWidth = venc_attr_H264_H265_data->PicWidth;
				data_set.VeAttr.AttrH265e.mProfile = venc_attr_H264_H265_data->Profile;
				data_set.VeAttr.AttrH265e.mRefNum = venc_attr_H264_H265_data->RefNum;
				data_set.RcAttr.mAttrH265Vbr.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				data_set.RcAttr.mAttrH265Vbr.mGop = venc_attr_H264_H265_data->mGop;
				data_set.RcAttr.mAttrH265Vbr.mMaxBitRate = venc_attr_H264_H265_data->mMaxBitRate;
				data_set.RcAttr.mAttrH265Vbr.mMaxQp = venc_attr_H264_H265_data->mMaxQp;
				data_set.RcAttr.mAttrH265Vbr.mMinQp = venc_attr_H264_H265_data->mMinQp;
				data_set.RcAttr.mAttrH265Vbr.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
				data_set.RcAttr.mAttrH265Vbr.mStatTime = venc_attr_H264_H265_data->mStatTime;
				data_set.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
				if(venc_attr_H264_H265_data->enGopMode == 0)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
					data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 1)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
					data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
					data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
					data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 2)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
					data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
					data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 3)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
					data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
					data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
		
				Virvi2VencConfig configPara;
				memset(&configPara, 0, sizeof(Virvi2VencConfig));
				configPara.DevNum = 1;
				configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
				configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
				configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
				configPara.EncoderType = PT_H265;
				configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
				configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
				configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
				configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

				printf("first stop venc.......\n");
				ret = venc_stop(VeChn);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_stop failed!\n");
					return ret;
				}
				printf("start venc...........\n");
				ret = venc_start(VeChn, &configPara, &data_set);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_start failed!\n");
					return ret;
				}
				
				//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);
		
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			}
		
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_FIXQP) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				data_set.VeAttr.Type = PT_H265;
				data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
				data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
				data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
				data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
				data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
				data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
				data_set.VeAttr.AttrH265e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
				data_set.VeAttr.AttrH265e.mbByFrame = venc_attr_H264_H265_data->bByFrame;
				data_set.VeAttr.AttrH265e.mBFrameNum = venc_attr_H264_H265_data->BFrameNum;
				data_set.VeAttr.AttrH265e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
				data_set.VeAttr.AttrH265e.mBufSize = venc_attr_H264_H265_data->BufSize;
				data_set.VeAttr.AttrH265e.mFastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
				data_set.VeAttr.AttrH265e.mMaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
				data_set.VeAttr.AttrH265e.mMaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
				data_set.VeAttr.AttrH265e.mPicHeight = venc_attr_H264_H265_data->PicHeight;
				data_set.VeAttr.AttrH265e.mPicWidth = venc_attr_H264_H265_data->PicWidth;
				data_set.VeAttr.AttrH265e.mProfile = venc_attr_H264_H265_data->Profile;
				data_set.VeAttr.AttrH265e.mRefNum = venc_attr_H264_H265_data->RefNum;
				data_set.RcAttr.mAttrH265FixQp.fr32DstFrmRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				data_set.RcAttr.mAttrH265FixQp.mGop = venc_attr_H264_H265_data->mGop;
				data_set.RcAttr.mAttrH265FixQp.mIQp = venc_attr_H264_H265_data->mIQp;
				data_set.RcAttr.mAttrH265FixQp.mPQp = venc_attr_H264_H265_data->mPQp;
				data_set.RcAttr.mAttrH265FixQp.mSrcFrmRate = venc_attr_H264_H265_data->mSrcFrmRate;
				data_set.RcAttr.mRcMode = VENC_RC_MODE_H265FIXQP;
				if(venc_attr_H264_H265_data->enGopMode == 0)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
					data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 1)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
					data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
					data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
					data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 2)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
					data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
					data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 3)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
					data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
					data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
		
				Virvi2VencConfig configPara;
				memset(&configPara, 0, sizeof(Virvi2VencConfig));
				configPara.DevNum = 1;
				configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
				configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
				configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
				configPara.EncoderType = PT_H265;
				configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
				configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
				configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
				configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

				printf("first stop venc.......\n");
				ret = venc_stop(VeChn);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_stop failed!\n");
					return ret;
				}
				printf("start venc...........\n");
				ret = venc_start(VeChn, &configPara, &data_set);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_start failed!\n");
					return ret;
				}
				
				//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);
		
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			}	
			if(cfg_ids & HW_VENC_CFG_H265_ATTR_ABR)
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				data_set.VeAttr.Type = PT_H265;
				data_set.VeAttr.MaxKeyInterval = venc_attr_H264_H265_data->MaxKeyInterval;
				data_set.VeAttr.SrcPicHeight = venc_attr_H264_H265_data->SrcPicHeight;
				data_set.VeAttr.SrcPicWidth = venc_attr_H264_H265_data->SrcPicWidth;
				data_set.VeAttr.Field =(VIDEO_FIELD_E)venc_attr_H264_H265_data->Field;
				data_set.VeAttr.PixelFormat = (PIXEL_FORMAT_E)venc_attr_H264_H265_data->PixelFormat;
				data_set.VeAttr.Rotate = (ROTATE_E)venc_attr_H264_H265_data->Rotate;
				data_set.VeAttr.AttrH264e.bByFrame = venc_attr_H264_H265_data->bByFrame;
				data_set.VeAttr.AttrH264e.BFrameNum = venc_attr_H264_H265_data->BFrameNum;
				data_set.VeAttr.AttrH264e.BufSize = venc_attr_H264_H265_data->BufSize;
				data_set.VeAttr.AttrH264e.FastEncFlag = venc_attr_H264_H265_data->FastEncFlag;
				data_set.VeAttr.AttrH264e.IQpOffset = venc_attr_H264_H265_data->IQpOffset;
				data_set.VeAttr.AttrH264e.MaxPicHeight = venc_attr_H264_H265_data->MaxPicHeight;
				data_set.VeAttr.AttrH264e.MaxPicWidth = venc_attr_H264_H265_data->MaxPicWidth;
				data_set.VeAttr.AttrH264e.mbPIntraEnable = venc_attr_H264_H265_data->mbPIntraEnable;
				data_set.VeAttr.AttrH264e.PicHeight = venc_attr_H264_H265_data->PicHeight;
				data_set.VeAttr.AttrH264e.PicWidth = venc_attr_H264_H265_data->PicWidth;
				data_set.VeAttr.AttrH264e.Profile = venc_attr_H264_H265_data->Profile;
				data_set.VeAttr.AttrH264e.RefNum = venc_attr_H264_H265_data->RefNum;
				data_set.RcAttr.mAttrH264Abr.mMaxBitRate = venc_attr_H264_H265_data->mMaxBitRate;
				//data_set.RcAttr.mAttrH264Abr.mRatioChangeQp= venc_attr_H264_H265_data->mRatioChangeQp;
				//data_set.RcAttr.mAttrH264Abr.mQuality = venc_attr_H264_H265_data->mQuality;
				data_set.RcAttr.mRcMode = VENC_RC_MODE_H265ABR;
				if(venc_attr_H264_H265_data->enGopMode == 0)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
					data_set.GopAttr.stNormalP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 1)
				{	data_set.GopAttr.enGopMode = VENC_GOPMODE_DUALP;
					data_set.GopAttr.stDualP.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
					data_set.GopAttr.stDualP.mSPInterval = venc_attr_H264_H265_data->mSPInterval;
					data_set.GopAttr.stDualP.mSPQpDelta = venc_attr_H264_H265_data->mSPQpDelta;
				}
				if(venc_attr_H264_H265_data->enGopMode == 2)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_SMARTP;
					data_set.GopAttr.stSmartP.mBgInterval = venc_attr_H264_H265_data->mBgInterval;
					data_set.GopAttr.stSmartP.mBgQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stSmartP.mViQpDelta = venc_attr_H264_H265_data->mViQpDelta;
					data_set.GopAttr.stSmartP.mVirtualIInterval = venc_attr_H264_H265_data->mVirtualIInterval;
				}
				if(venc_attr_H264_H265_data->enGopMode == 3)
				{
					data_set.GopAttr.enGopMode = VENC_GOPMODE_BIPREDB;
					data_set.GopAttr.stBipredB.mBFrmNum = venc_attr_H264_H265_data->mBFrmNum;
					data_set.GopAttr.stBipredB.mBQpDelta = venc_attr_H264_H265_data->mBgQpDelta;
					data_set.GopAttr.stBipredB.mIPQpDelta = venc_attr_H264_H265_data->mIPQpDelta;
				}
				
				Virvi2VencConfig configPara;
				memset(&configPara, 0, sizeof(Virvi2VencConfig));
				configPara.DevNum = 1;
				configPara.SrcWidth = venc_attr_H264_H265_data->SrcPicWidth;
				configPara.SrcHeight = venc_attr_H264_H265_data->SrcPicHeight;
				configPara.SrcFrameRate = venc_attr_H264_H265_data->mSrcFrmRate;
				configPara.EncoderType = PT_H265;
				configPara.DestWidth = venc_attr_H264_H265_data->PicWidth;
				configPara.DestHeight = venc_attr_H264_H265_data->PicHeight;
				configPara.DestFrameRate = venc_attr_H264_H265_data->fr32DstFrmRate;
				configPara.DestBitRate = venc_attr_H264_H265_data->mBitRate;
				configPara.DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

				printf("first stop venc.......\n");
				ret = venc_stop(VeChn);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_stop failed!\n");
					return ret;
				}
				printf("start venc...........\n");
				ret = venc_start(VeChn, &configPara, &data_set);
				if(ret < 0)
				{
					ret =-1;
					printf("venc_start failed!\n");
					return ret;
				}
				
				//ret = AW_MPI_VENC_SetChnAttr(VeChn, &data_set);
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			}	
		
#if 0		
		if (cfg_ids & HW_VENC_CFG_H265_RC_CBR) /* ATTR */
		{
			venc_rcparam_H264_H265_cfg *venc_rc_H264_H265_data= (venc_rcparam_H264_H265_cfg *)data_ptr;
			struct VENC_RC_PARAM_S data_set;
			memcpy(data_set.ThrdI, venc_rc_H264_H265_data->ThrdI, sizeof(data_set.ThrdI));
			memcpy(data_set.ThrdP, venc_rc_H264_H265_data->ThrdP, sizeof(data_set.ThrdP));
			data_set.RowQpDelta = venc_rc_H264_H265_data->RowQpDelta;
			data_set.pRcParam = 0;
			data_set.ParamH265Cbr.IPQPDelta = venc_rc_H264_H265_data->IPQPDelta;
			data_set.ParamH265Cbr.MaxIprop = venc_rc_H264_H265_data->MaxIprop;
			data_set.ParamH265Cbr.MaxQp = venc_rc_H264_H265_data->MaxQp;
			data_set.ParamH265Cbr.MaxReEncodeTimes = venc_rc_H264_H265_data->MaxReEncodeTimes;
			data_set.ParamH265Cbr.MinIprop = venc_rc_H264_H265_data->MinIprop;
			data_set.ParamH265Cbr.MinIQp = venc_rc_H264_H265_data->MinIQp;
			data_set.ParamH265Cbr.MinQp = venc_rc_H264_H265_data->MinQp;
			data_set.ParamH265Cbr.QualityLevel = venc_rc_H264_H265_data->QualityLevel;
			ret = AW_MPI_VENC_SetRcParam(VeChn, &data_set);
		}
		
		if (cfg_ids & HW_VENC_CFG_H265_RC_VBR) /* ATTR */
		{
			venc_rcparam_H264_H265_cfg *venc_rc_H264_H265_data= (venc_rcparam_H264_H265_cfg *)data_ptr;
			struct VENC_RC_PARAM_S data_set;
			memcpy(data_set.ThrdI, venc_rc_H264_H265_data->ThrdI, sizeof(data_set.ThrdI));
			memcpy(data_set.ThrdP, venc_rc_H264_H265_data->ThrdP, sizeof(data_set.ThrdP));
			data_set.RowQpDelta = venc_rc_H264_H265_data->RowQpDelta;
			data_set.pRcParam = 0;
			data_set.ParamH265Vbr.s32ChangePos = venc_rc_H264_H265_data->s32ChangePos;
			data_set.ParamH265Vbr.s32IPQPDelta =  = venc_rc_H264_H265_data->IPQPDelta;
			data_set.ParamH265Vbr.u32MaxIprop  = venc_rc_H264_H265_data->MaxIprop;
			data_set.ParamH265Vbr.u32MinIprop = venc_rc_H264_H265_data->MinIprop;
			data_set.ParamH265Vbr.u32MinIQP = venc_rc_H264_H265_data->MinIQp;
			ret = AW_MPI_VENC_SetRcParam(VeChn, &data_set);
		}
#endif
		break;
	}
	return ret;
}

extern "C" int venc_get_cfg(int VeChn, unsigned char group_id, unsigned int cfg_ids, void *cfg_data)
{
	int ret = 0;
	int data_length = 0;
	unsigned char *data_ptr = (unsigned char *)cfg_data;
	switch (group_id)
	{
	case HW_VENC_CFG_COMMON: /* venc_common */


		if (cfg_ids & HW_VENC_CFG_COMMON_3DNR) /* 3DNR */
		{
			venc_3DNR_cfg *venc_3DNR_data= (venc_3DNR_cfg *)data_ptr;
			ret = AW_MPI_VENC_Get3DNR(VeChn, &venc_3DNR_data->flag_3DNR);

			/* offset */
			data_ptr += sizeof(struct venc_3DNR_cfg);
			data_length += sizeof(struct venc_3DNR_cfg);
		}
		break;
	case HW_VENC_CFG_H264:
		if (cfg_ids & HW_VENC_CFG_H264_ATTR_CBR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
			
			venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
			venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
			venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
			venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
			venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
			venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
			venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH264e.bByFrame;
			venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH264e.BFrameNum;
			venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH264e.BufSize;
			venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH264e.FastEncFlag;
			venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH264e.IQpOffset;
			venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH264e.MaxPicHeight;
			venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH264e.MaxPicWidth;
			venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH264e.mbPIntraEnable;
			venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH264e.PicHeight;
			venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH264e.PicWidth;
			venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH264e.Profile;
			venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH264e.RefNum;
			venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH264Cbr.fr32DstFrmRate;
			venc_attr_H264_H265_data->mBitRate = data_set.RcAttr.mAttrH264Cbr.mBitRate ;
			venc_attr_H264_H265_data->mFluctuateLevel = data_set.RcAttr.mAttrH264Cbr.mFluctuateLevel;
			venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH264Cbr.mGop ;
			venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH264Cbr.mSrcFrmRate;
			venc_attr_H264_H265_data->mStatTime = data_set.RcAttr.mAttrH264Cbr.mStatTime;
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 0;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 1;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
				venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
				venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
			{
				venc_attr_H264_H265_data->enGopMode = 2;
				venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
				venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
				venc_attr_H264_H265_data->mVirtualIInterval = 0;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
			{
				venc_attr_H264_H265_data->enGopMode = 3;
				venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
			}


			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			data_length += sizeof(struct venc_attr_H264_H265_cfg);
		}
		
		if (cfg_ids & HW_VENC_CFG_H264_ATTR_VBR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
			
			venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
			venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
			venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
			venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
			venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
			venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
			venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH264e.bByFrame;
			venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH264e.BFrameNum;
			venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH264e.BufSize;
			venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH264e.FastEncFlag;
			venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH264e.IQpOffset;
			venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH264e.MaxPicHeight;
			venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH264e.MaxPicWidth;
			venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH264e.mbPIntraEnable;
			venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH264e.PicHeight;
			venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH264e.PicWidth;
			venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH264e.Profile;
			venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH264e.RefNum;

			venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH264Vbr.fr32DstFrmRate;
			venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH264Vbr.mGop;
			venc_attr_H264_H265_data->mMaxBitRate = data_set.RcAttr.mAttrH264Vbr.mMaxBitRate;
			venc_attr_H264_H265_data->mMaxQp = data_set.RcAttr.mAttrH264Vbr.mMaxQp;
			venc_attr_H264_H265_data->mMinQp = data_set.RcAttr.mAttrH264Vbr.mMinQp;
			venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH264Vbr.mSrcFrmRate ;
			venc_attr_H264_H265_data->mStatTime = data_set.RcAttr.mAttrH264Vbr.mStatTime;
			 
			 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
			 {	 
				 venc_attr_H264_H265_data->enGopMode = 0;
				 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
			 }
			 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
			 {	 
				 venc_attr_H264_H265_data->enGopMode = 1;
				 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
				 venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
				 venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
			 }
			 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
			 {
				 venc_attr_H264_H265_data->enGopMode = 2;
				 venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
				 venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
				 venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
			 }
			 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
			 {
				 venc_attr_H264_H265_data->enGopMode = 3;
				 venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
				 venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
				 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
			 }


			

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			data_length += sizeof(struct venc_attr_H264_H265_cfg);
		}

		if (cfg_ids & HW_VENC_CFG_H264_ATTR_FIXQP) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
			
			venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
			venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
			venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
			venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
			venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
			venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
			venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH264e.bByFrame;
			venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH264e.BFrameNum;
			venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH264e.BufSize;
			venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH264e.FastEncFlag;
			venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH264e.IQpOffset;
			venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH264e.MaxPicHeight;
			venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH264e.MaxPicWidth;
			venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH264e.mbPIntraEnable;
			venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH264e.PicHeight;
			venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH264e.PicWidth;
			venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH264e.Profile;
			venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH264e.RefNum;

			venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH264FixQp.fr32DstFrmRate;
			venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH264FixQp.mGop;
			venc_attr_H264_H265_data->mIQp = data_set.RcAttr.mAttrH264FixQp.mIQp;
			venc_attr_H264_H265_data->mPQp = data_set.RcAttr.mAttrH264FixQp.mPQp;
			venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH264FixQp.mSrcFrmRate;
	
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 0;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 1;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
				venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
				venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
			{
				venc_attr_H264_H265_data->enGopMode = 2;
				venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
				venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
			{
				venc_attr_H264_H265_data->enGopMode = 3;
				venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
			}

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			data_length += sizeof(struct venc_attr_H264_H265_cfg);
		}	
		
		if (cfg_ids & HW_VENC_CFG_H264_ATTR_ABR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
			
			venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
			venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
			venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
			venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
			venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
			venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
			venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH264e.bByFrame;
			venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH264e.BFrameNum;
			venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH264e.BufSize;
			venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH264e.FastEncFlag;
			venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH264e.IQpOffset;
			venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH264e.MaxPicHeight;
			venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH264e.MaxPicWidth;
			venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH264e.mbPIntraEnable;
			venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH264e.PicHeight;
			venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH264e.PicWidth;
			venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH264e.Profile;
			venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH264e.RefNum;

			venc_attr_H264_H265_data->mMaxBitRate = data_set.RcAttr.mAttrH264Abr.mMaxBitRate;
			//venc_attr_H264_H265_data->mRatioChangeQp = data_set.RcAttr.mAttrH264Abr.mRatioChangeQp;
			//venc_attr_H264_H265_data->mQuality = data_set.RcAttr.mAttrH264Abr.mQuality;
	
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 0;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 1;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
				venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
				venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
			{
				venc_attr_H264_H265_data->enGopMode = 2;
				venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
				venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
				venc_attr_H264_H265_data->mVirtualIInterval = data_set.GopAttr.stSmartP.mVirtualIInterval;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
			{
				venc_attr_H264_H265_data->enGopMode = 3;
				venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
			}

			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			data_length += sizeof(struct venc_attr_H264_H265_cfg);
		}

		break;
	case HW_VENC_CFG_H265:
		if (cfg_ids & HW_VENC_CFG_H265_ATTR_CBR) /* ATTR */
		{
			venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
			struct VENC_CHN_ATTR_S data_set;
			ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
			
			venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
			venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
			venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
			venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
			venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
			venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
			venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH265e.mbByFrame;
			venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH265e.mBFrameNum;
			venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH265e.mBufSize;
			venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH265e.mFastEncFlag;
			venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH265e.IQpOffset;
			venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH265e.mMaxPicHeight;
			venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH265e.mMaxPicWidth;
			venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH265e.mbPIntraEnable;
			venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH265e.mPicHeight;
			venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH265e.mPicWidth;
			venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH265e.mProfile;
			venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH265e.mRefNum;
			venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH265Cbr.fr32DstFrmRate;
			venc_attr_H264_H265_data->mBitRate = data_set.RcAttr.mAttrH265Cbr.mBitRate;
			venc_attr_H264_H265_data->mFluctuateLevel = data_set.RcAttr.mAttrH265Cbr.mFluctuateLevel;
			venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH265Cbr.mGop;
			venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH265Cbr.mSrcFrmRate;
			venc_attr_H264_H265_data->mStatTime = data_set.RcAttr.mAttrH265Cbr.mStatTime;
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 0;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
			{	
				venc_attr_H264_H265_data->enGopMode = 1;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
				venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
				venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
			{
				venc_attr_H264_H265_data->enGopMode = 2;
				venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
				venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
			}
			if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
			{
				venc_attr_H264_H265_data->enGopMode = 3;
				venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
				venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
				venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
			}


			/* offset */
			data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
			data_length += sizeof(struct venc_attr_H264_H265_cfg);
		}
			
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_VBR) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
				
				venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
				venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
				venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
				venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
				venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
				venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
				venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH265e.mbByFrame;
				venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH265e.mBFrameNum;
				venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH265e.mBufSize;
				venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH265e.mFastEncFlag;
				venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH265e.IQpOffset;
				venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH265e.mMaxPicHeight;
				venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH265e.mMaxPicWidth;
				venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH265e.mbPIntraEnable;
				venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH265e.mPicHeight;
				venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH265e.mPicWidth;
				venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH265e.mProfile;
				venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH265e.mRefNum;
			
				venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH265Vbr.fr32DstFrmRate;
				venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH265Vbr.mGop;
				venc_attr_H264_H265_data->mMaxBitRate = data_set.RcAttr.mAttrH265Vbr.mMaxBitRate;
				venc_attr_H264_H265_data->mMaxQp = data_set.RcAttr.mAttrH265Vbr.mMaxQp;
				venc_attr_H264_H265_data->mMinQp = data_set.RcAttr.mAttrH265Vbr.mMinQp;
				venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH265Vbr.mSrcFrmRate ;
				venc_attr_H264_H265_data->mStatTime = data_set.RcAttr.mAttrH265Vbr.mStatTime;
				 
				 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
				 {	 
					 venc_attr_H264_H265_data->enGopMode = 0;
					 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
				 }
				 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
				 {	 
					 venc_attr_H264_H265_data->enGopMode = 1;
					 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
					 venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
					 venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
				 }
				 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
				 {
					 venc_attr_H264_H265_data->enGopMode = 2;
					 venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
					 venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
					 venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
				 }
				 if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
				 {
					 venc_attr_H264_H265_data->enGopMode = 3;
					 venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
					 venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
					 venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
				 }
			
			
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
				data_length += sizeof(struct venc_attr_H264_H265_cfg);
			}
			
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_FIXQP) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
				
				venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
				venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
				venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
				venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
				venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
				venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
				venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH265e.mbByFrame;
				venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH265e.mBFrameNum;
				venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH265e.mBufSize;
				venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH265e.mFastEncFlag;
				venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH265e.IQpOffset;
				venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH265e.mMaxPicHeight;
				venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH265e.mMaxPicWidth;
				venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH265e.mbPIntraEnable;
				venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH265e.mPicHeight;
				venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH265e.mPicWidth;
				venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH265e.mProfile;
				venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH265e.mRefNum;
			
				venc_attr_H264_H265_data->fr32DstFrmRate = data_set.RcAttr.mAttrH265FixQp.fr32DstFrmRate;
				venc_attr_H264_H265_data->mGop = data_set.RcAttr.mAttrH265FixQp.mGop;
				venc_attr_H264_H265_data->mIQp = data_set.RcAttr.mAttrH265FixQp.mIQp;
				venc_attr_H264_H265_data->mPQp = data_set.RcAttr.mAttrH265FixQp.mPQp;
				venc_attr_H264_H265_data->mSrcFrmRate = data_set.RcAttr.mAttrH265FixQp.mSrcFrmRate;
			
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
				{	
					venc_attr_H264_H265_data->enGopMode = 0;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
				{	
					venc_attr_H264_H265_data->enGopMode = 1;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
					venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
					venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
				{
					venc_attr_H264_H265_data->enGopMode = 2;
					venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
					venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
					venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
				{
					venc_attr_H264_H265_data->enGopMode = 3;
					venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
					venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
				}
			
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
				data_length += sizeof(struct venc_attr_H264_H265_cfg);
			}	
			
			if (cfg_ids & HW_VENC_CFG_H265_ATTR_ABR) /* ATTR */
			{
				venc_attr_H264_H265_cfg *venc_attr_H264_H265_data= (venc_attr_H264_H265_cfg *)data_ptr;
				struct VENC_CHN_ATTR_S data_set;
				ret = AW_MPI_VENC_GetChnAttr(VeChn, &data_set);
				
				venc_attr_H264_H265_data->MaxKeyInterval= data_set.VeAttr.MaxKeyInterval;
				venc_attr_H264_H265_data->SrcPicHeight= data_set.VeAttr.SrcPicHeight;
				venc_attr_H264_H265_data->SrcPicWidth = data_set.VeAttr.SrcPicWidth;
				venc_attr_H264_H265_data->Field = data_set.VeAttr.Field;
				venc_attr_H264_H265_data->PixelFormat = data_set.VeAttr.PixelFormat;
				venc_attr_H264_H265_data->Rotate = data_set.VeAttr.Rotate;
				venc_attr_H264_H265_data->bByFrame = data_set.VeAttr.AttrH265e.mbByFrame;
				venc_attr_H264_H265_data->BFrameNum = data_set.VeAttr.AttrH265e.mBFrameNum;
				venc_attr_H264_H265_data->BufSize = data_set.VeAttr.AttrH265e.mBufSize;
				venc_attr_H264_H265_data->FastEncFlag = data_set.VeAttr.AttrH265e.mFastEncFlag;
				venc_attr_H264_H265_data->IQpOffset = data_set.VeAttr.AttrH265e.IQpOffset;
				venc_attr_H264_H265_data->MaxPicHeight = data_set.VeAttr.AttrH265e.mMaxPicHeight;
				venc_attr_H264_H265_data->MaxPicWidth = data_set.VeAttr.AttrH265e.mMaxPicWidth;
				venc_attr_H264_H265_data->mbPIntraEnable = data_set.VeAttr.AttrH265e.mbPIntraEnable;
				venc_attr_H264_H265_data->PicHeight = data_set.VeAttr.AttrH265e.mPicHeight;
				venc_attr_H264_H265_data->PicWidth = data_set.VeAttr.AttrH265e.mPicWidth;
				venc_attr_H264_H265_data->Profile = data_set.VeAttr.AttrH265e.mProfile;
				venc_attr_H264_H265_data->RefNum = data_set.VeAttr.AttrH265e.mRefNum;
			
				venc_attr_H264_H265_data->mMaxBitRate = data_set.RcAttr.mAttrH264Abr.mMaxBitRate;
				//venc_attr_H264_H265_data->mRatioChangeQp = data_set.RcAttr.mAttrH264Abr.mRatioChangeQp;
				//venc_attr_H264_H265_data->mQuality = data_set.RcAttr.mAttrH264Abr.mQuality;
			
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_NORMALP)
				{	
					venc_attr_H264_H265_data->enGopMode = 0;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stNormalP.mIPQpDelta;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_DUALP)
				{	
					venc_attr_H264_H265_data->enGopMode = 1;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stDualP.mIPQpDelta;
					venc_attr_H264_H265_data->mSPInterval = data_set.GopAttr.stDualP.mSPInterval;
					venc_attr_H264_H265_data->mSPQpDelta = data_set.GopAttr.stDualP.mSPQpDelta;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_SMARTP)
				{
					venc_attr_H264_H265_data->enGopMode = 2;
					venc_attr_H264_H265_data->mBgInterval = data_set.GopAttr.stSmartP.mBgInterval;
					venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stSmartP.mBgQpDelta;
					venc_attr_H264_H265_data->mViQpDelta = data_set.GopAttr.stSmartP.mViQpDelta;
					venc_attr_H264_H265_data->mVirtualIInterval = data_set.GopAttr.stSmartP.mVirtualIInterval;
				}
				if(data_set.GopAttr.enGopMode == VENC_GOPMODE_BIPREDB)
				{
					venc_attr_H264_H265_data->enGopMode = 3;
					venc_attr_H264_H265_data->mBFrmNum = data_set.GopAttr.stBipredB.mBFrmNum;
					venc_attr_H264_H265_data->mBgQpDelta = data_set.GopAttr.stBipredB.mBQpDelta;
					venc_attr_H264_H265_data->mIPQpDelta = data_set.GopAttr.stBipredB.mIPQpDelta;
				}
			
				/* offset */
				data_ptr += sizeof(struct venc_attr_H264_H265_cfg);
				data_length += sizeof(struct venc_attr_H264_H265_cfg);
			}
		
		break;
	}
	return data_length;
}






