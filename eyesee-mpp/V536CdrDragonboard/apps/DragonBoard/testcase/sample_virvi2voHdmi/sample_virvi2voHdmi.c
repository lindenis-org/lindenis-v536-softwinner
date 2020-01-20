//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2vo"
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "vo/hwdisplay.h"
#include "log/log_wrapper.h"
#include <dragonboard/dragonboard.h>
#include <ClockCompPortIndex.h>
#include <mpi_videoformat_conversion.h>
#include <confparser.h>
#include "sample_virvi2voHdmi_config.h"
#include "sample_virvi2voHdmi.h"
#include "mpi_isp.h"

#define FIFO_DEV  "/tmp/fifo_hdmi"
BOOL HDMI_FLAG;
SampleVIRVI2VOContext *pSampleVIRVI2VOContext = NULL;

int initSampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    memset(pContext, 0, sizeof(SampleVIRVI2VOContext));
    pContext->mUILayer = HLAY(2, 0);
    cdx_sem_init(&pContext->mSemExit, 0);
    return 0;
}

int destroySampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    cdx_sem_deinit(&pContext->mSemExit);
    return 0;
}

static ERRORTYPE SampleVIRVI2VO_VOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleVIRVI2VOContext *pContext = (SampleVIRVI2VOContext*)cookie;
    if(MOD_ID_VOU == pChn->mModId)
    {
        if(pChn->mChnId != pContext->mVOChn)
        {
            aloge("fatal error! VO chnId[%d]!=[%d]", pChn->mChnId, pContext->mVOChn);
        }
        switch(event)
        {
            case MPP_EVENT_RELEASE_VIDEO_BUFFER:
            {
                aloge("fatal error! sample_virvi2vo use binding mode!");
                break;
            }
            case MPP_EVENT_SET_VIDEO_SIZE:
            {
                SIZE_S *pDisplaySize = (SIZE_S*)pEventData;
                alogd("vo report video display size[%dx%d]", pDisplaySize->Width, pDisplaySize->Height);
                break;
            }
            case MPP_EVENT_RENDERING_START:
            {
                alogd("vo report rendering start");
                break;
            }
            default:
            {
                //postEventFromNative(this, event, 0, 0, pEventData);
                aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                ret = ERR_VO_ILLEGAL_PARAM;
                break;
            }
        }
    }
    else
    {
        aloge("fatal error! why modId[0x%x]?", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

static ERRORTYPE SampleVIRVI2VO_CLOCKCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    alogw("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}

static int ParseCmdLine(int argc, char **argv, SampleVIRVI2VOCmdLineParam *pCmdLinePara)
{
    alogd("sample_virvi2vo path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleVIRVI2VOCmdLineParam));
    while(i < argc)
    {
        if(!strcmp(argv[i], "-path"))
        {
            if(++i >= argc)
            {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE)
            {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        }
        else if(!strcmp(argv[i], "-h"))
        {
            printf("CmdLine param example:\n"
                "\t run -path /home/sample_virvi2vo.conf\n");
            ret = 1;
            break;
        }
        else
        {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleVIRVI2VOConfig(SampleVIRVI2VOConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    if(NULL == conf_path)
    {
        alogd("user not set config file. use default test parameter!");
        pConfig->mDevNum = 3;
        pConfig->mCaptureWidth = 1920;
        pConfig->mCaptureHeight = 1080;
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pConfig->mFrameRate = 30;
        pConfig->mTestDuration = 0;
        return SUCCESS;
    }
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0)
    {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleVIRVI2VOConfig));
    pConfig->mDevNum        = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_DEVICE_NUMBER, 0);
    pConfig->mCaptureWidth  = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_WIDTH, 0);
    pConfig->mCaptureHeight = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_HEIGHT, 0);
    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_VIRVI2VO_KEY_PIC_FORMAT, NULL);
    if(!strcmp(pStrPixelFormat, "yu12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "yv12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "nv21"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "nv12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    }
    else
    {
        aloge("fatal error! conf file pic_format is [%s]?", pStrPixelFormat);
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    pConfig->mFrameRate = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_FRAME_RATE, 0);
    pConfig->mTestDuration = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_TEST_DURATION, 0);
    destroyConfParser(&stConfParser);
    return SUCCESS;
}

void handle_exit(int signo)
{
    alogd("user want to exit!");
    if(NULL != pSampleVIRVI2VOContext)
    {
        cdx_sem_up(&pSampleVIRVI2VOContext->mSemExit);
    }
}

static void MenuCtrl(int val)
{
    int fifoFd = 0;
    int ret = 0;
	if ((fifoFd = open(FIFO_DEV, O_WRONLY|O_NONBLOCK)) < 0)
    {      // fifo's write-endian block until read-endian open
        if (mkfifo(FIFO_DEV, 0666) < 0)
        {
            perror("mkfifo fifo failed");
            return ;
        }
        else
        {
            fifoFd = open(FIFO_DEV, O_WRONLY|O_NONBLOCK);
            if(fifoFd < 0)
            {
               perror("open fifo failed");
               return;
            }
        }
    }
	if(fifoFd < 0)
	{
	   perror("open fifo failed");
	   return;
	}
    if( val )
    {
        aloge("write hdmi true");
        ret = write(fifoFd, "P[Hdmi] PASS", 30);
        if(ret < 0)
        {
           perror("write fifo failed");
        }
    }
    else
    {
        aloge("write hdmi failed");
        ret = write(fifoFd, "F[Hdmi] FAIL", 30);
        if(ret < 0)
        {
           perror("write fifo failed");
        }

    }
}

void HdmiEventHandler(int iEvent)
{
    if (iEvent == 1) {
        alogd("detect hdmi plug in\n");
    } else if (iEvent == 0) {
        alogd("detect hdmi plug out\n");
    }
}

void *CheckHotplugEvent(void *pArg)
{
    SampleVIRVI2VOContext *pCap = (SampleVIRVI2VOContext *)pArg;
    int iNotifyFd;
    int iRet;
    struct inotify_event stEvent;
    int iCvbsState;
    FILE* iFdHdmi;
    FILE* iFdCvbs;
    char pcEventDesc[256];
    struct timeval tpstart,tpend;
    static long call_diff_ms = 0;
    gettimeofday(&tpstart, NULL);
    aloge("================ CheckHotplugEvent ================");
    while(1)
    {
        gettimeofday(&tpend,NULL);
        call_diff_ms = (tpend.tv_sec * 1000 + tpend.tv_usec/1000) - (tpstart.tv_sec * 1000 + tpstart.tv_usec/1000);
        if(call_diff_ms >= 4000)
        {
            aloge("================ Check Hotplug Event timeout ================");
            HDMI_FLAG = FALSE;
            break;
        }
        iFdHdmi = fopen("/sys/class/extcon/hdmi/state", "r");
        if(iFdHdmi < 0)
        {
            perror("open hdmi state failed");
            break;
        }
        fgets(pcEventDesc, sizeof(pcEventDesc), iFdHdmi);
        if (!strncmp("HDMI=1", pcEventDesc, 6) && pCap->HDMI_State == 0)
        {
            HdmiEventHandler(1);
            pCap->HDMI_State = 1;
            aloge("HDMI_State = %d",pCap->HDMI_State);
            AW_MPI_VO_GetVideoLayerAttr(pCap->mVoLayer, &pCap->mLayerAttr);
            pCap->mLayerAttr.stDispRect.X = 0;
            pCap->mLayerAttr.stDispRect.Y = 0;
            pCap->mLayerAttr.stDispRect.Width = 1920;
            pCap->mLayerAttr.stDispRect.Height = 1080;
            AW_MPI_VO_SetVideoLayerAttr(pCap->mVoLayer, &pCap->mLayerAttr);
            VO_PUB_ATTR_S spPubAttr;
            AW_MPI_VO_GetPubAttr(pCap->mVoDev, &spPubAttr);
            spPubAttr.enIntfType = VO_INTF_HDMI;
            spPubAttr.enIntfSync = VO_OUTPUT_1080P25;
            AW_MPI_VO_SetPubAttr(pCap->mVoDev, &spPubAttr);
            int mUILayer = HLAY(2, 0);
            AW_MPI_VO_CloseVideoLayer(mUILayer);
            fclose(iFdHdmi);
            HDMI_FLAG = TRUE;
            break;
        }
        fclose(iFdHdmi);
        usleep(1000);
    }
    aloge("================ exit CheckHotplugEvent ================");
    pthread_exit(NULL);
}

void *SwitchtoHDMI(void *pArg)
{
    SampleVIRVI2VOContext *pCap = (SampleVIRVI2VOContext *)pArg;
    int iNotifyFd;
    int iRet;
    struct inotify_event stEvent;
    int iHdmiState;
    int iCvbsState;
    FILE* iFdHdmi;
    FILE* iFdCvbs;
    char pcEventDesc[256];
    struct timeval tpstart,tpend;
    static long call_diff_ms = 0;
    gettimeofday(&tpstart, NULL);
    VO_PUB_ATTR_S spPubAttr;
    int mUILayer;
    BOOL thred_exit_flag = FALSE;
    int ret = 0;
    aloge("================ SwitchtoHDMI ================");
    aloge("pCap->HDMI_State = %d",pCap->HDMI_State);
    while(1)
    {
        if(thred_exit_flag == TRUE)
            break;
        iFdHdmi = fopen("/sys/class/extcon/hdmi/state", "r");
        if(iFdHdmi < 0)
        {
            perror("open hdmi state failed");
            break;
        }
        fgets(pcEventDesc, sizeof(pcEventDesc), iFdHdmi);
        if (!strncmp("HDMI=0", pcEventDesc, 6) && pCap->HDMI_State == 1) {
            HdmiEventHandler(0);
            pCap->HDMI_State = 0;
        }
        fclose(iFdHdmi);
        switch (pCap->HDMI_State)
        {
             case 1:
                    gettimeofday(&tpend,NULL);
                    call_diff_ms = (tpend.tv_sec * 1000 + tpend.tv_usec/1000) - (tpstart.tv_sec * 1000 + tpstart.tv_usec/1000);
                    if(call_diff_ms >= 3000)
                    {
                        ret = AW_MPI_VO_GetVideoLayerAttr(pCap->mVoLayer, &pCap->mLayerAttr);
                        pCap->mLayerAttr.stDispRect.X = 0;
                        pCap->mLayerAttr.stDispRect.Y = 0;
                        pCap->mLayerAttr.stDispRect.Width = 240;
                        pCap->mLayerAttr.stDispRect.Height = 376;
                        ret = AW_MPI_VO_SetVideoLayerAttr(pCap->mVoLayer, &pCap->mLayerAttr);
                        ret = AW_MPI_VO_GetPubAttr(pCap->mVoDev, &spPubAttr);
                        spPubAttr.enIntfType = VO_INTF_LCD;
                        spPubAttr.enIntfSync = VO_OUTPUT_NTSC;
                        ret = AW_MPI_VO_SetPubAttr(pCap->mVoDev, &spPubAttr);
                        mUILayer = HLAY(2, 0);
                        ret = AW_MPI_VO_GetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                        pCap->mLayerAttr.stDispRect.X = 0;
                        pCap->mLayerAttr.stDispRect.Y = 0;
                        pCap->mLayerAttr.stDispRect.Width = 240;
                        pCap->mLayerAttr.stDispRect.Height = 376;
                        ret = AW_MPI_VO_SetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                        thred_exit_flag = TRUE;
                    }
             break;
             case 0:
                    AW_MPI_VO_GetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                    pCap->mLayerAttr.stDispRect.X = 0;
                    pCap->mLayerAttr.stDispRect.Y = 0;
                    pCap->mLayerAttr.stDispRect.Width = 240;
                    pCap->mLayerAttr.stDispRect.Height = 376;
                    AW_MPI_VO_SetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                    AW_MPI_VO_GetPubAttr(pCap->mVoDev, &spPubAttr);
                    spPubAttr.enIntfType = VO_INTF_LCD;
                    spPubAttr.enIntfSync = VO_OUTPUT_NTSC;
                    AW_MPI_VO_SetPubAttr(pCap->mVoDev, &spPubAttr);
                    mUILayer = HLAY(2, 0);
                    ret = AW_MPI_VO_GetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                    pCap->mLayerAttr.stDispRect.X = 0;
                    pCap->mLayerAttr.stDispRect.Y = 0;
                    pCap->mLayerAttr.stDispRect.Width = 240;
                    pCap->mLayerAttr.stDispRect.Height = 376;
                    ret = AW_MPI_VO_SetVideoLayerAttr(mUILayer, &pCap->mLayerAttr);
                    thred_exit_flag = TRUE;
             break;
        }
        usleep(1000);
    }
    aloge("================ exit SwitchtoHDMI ================");
    pthread_exit(NULL);
}

int main(int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__)))
{
    int m_bTestSuccess = false;
    int result = 0;
    int iIspDev;
    FILE *fd = NULL;
    fd = fopen(FIFO_HDMI_DEV,"wb+");
    setenv("SCREEN_INFO","240x376-",1);
    aloge("set env SCREEN_INFO");
    SampleVIRVI2VOContext stContext;
	printf("sample_virvi2vo running!\n");
    initSampleVIRVI2VOContext(&stContext);
    pSampleVIRVI2VOContext = &stContext;

    /* parse command line param */
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0)
    {
        //aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        m_bTestSuccess = false;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0)
    {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    }
    else
    {
        pConfigFilePath = NULL;
    }

    /* parse config file. */
    if(loadSampleVIRVI2VOConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        m_bTestSuccess = false;
        goto _exit;
    }

    /* register process function for SIGINT, to exit program. */
    if (signal(SIGINT, handle_exit) == SIG_ERR)
        perror("can't catch SIGSEGV");

    /* init mpp system */
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    result = AW_MPI_SYS_Init();
    if (result) {
        aloge("fatal error! AW_MPI_SYS_Init failed");
        m_bTestSuccess = false;
        goto sys_init_err;
    }

    /* create vi channel */
    stContext.mVIDev = stContext.mConfigPara.mDevNum;
    stContext.mVIChn = 0;
    alogd("Vipp dev[%d] vir_chn[%d]", stContext.mVIDev, stContext.mVIChn);
    ERRORTYPE eRet = AW_MPI_VI_CreateVipp(stContext.mVIDev);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI CreateVipp failed");
        result = eRet;
        m_bTestSuccess = false;
        goto vi_create_vipp_err;
    }
    aloge("----set isp config----");
    AW_MPI_ISP_SetMirror(stContext.mVIDev,1);//0,1
    AW_MPI_ISP_SetFlip(stContext.mVIDev,1);//0,1
    VI_ATTR_S stAttr;
    eRet = AW_MPI_VI_GetVippAttr(stContext.mVIDev, &stAttr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI GetVippAttr failed");
    }
    memset(&stAttr, 0, sizeof(VI_ATTR_S));
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(stContext.mConfigPara.mPicFormat);
    stAttr.format.field = V4L2_FIELD_NONE;
    // stAttr.format.colorspace = V4L2_COLORSPACE_JPEG;
    stAttr.format.width = stContext.mConfigPara.mCaptureWidth;
    stAttr.format.height = stContext.mConfigPara.mCaptureHeight;
    stAttr.nbufs = 10;
    stAttr.nplanes = 2;
    /* do not use current param, if set to 1, all this configuration will
     * not be used.
     */
    stAttr.use_current_win = 0;
    stAttr.fps = stContext.mConfigPara.mFrameRate;
    eRet = AW_MPI_VI_SetVippAttr(stContext.mVIDev, &stAttr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI SetVippAttr failed");
    }
    eRet = AW_MPI_VI_EnableVipp(stContext.mVIDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! enableVipp fail!");
        result = eRet;
        m_bTestSuccess = false;
        goto vi_enable_vipp_err;
    }
#define ISP_RUN 1
#if ISP_RUN
    /* open isp */
    if (stContext.mVIDev == 0 || stContext.mVIDev == 1) {
           iIspDev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(iIspDev);
#endif
    eRet = AW_MPI_VI_CreateVirChn(stContext.mVIDev, stContext.mVIChn, NULL);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! createVirChn[%d] fail!", stContext.mVIChn);
    }
    /* enable vo dev */
    stContext.mVoDev = 0;
    AW_MPI_VO_Enable(stContext.mVoDev);
    AW_MPI_VO_AddOutsideVideoLayer(stContext.mUILayer);
    AW_MPI_VO_CloseVideoLayer(stContext.mUILayer); /* close ui layer. */
    /* enable vo layer */
    int hlay0 = 0;
    while(hlay0 < VO_MAX_LAYER_NUM)
    {
        if(SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay0))
        {
            break;
        }
        hlay0++;
    }
    if(hlay0 >= VO_MAX_LAYER_NUM)
    {
        aloge("fatal error! enable video layer fail!");
    }
    stContext.mVoLayer = hlay0;
    AW_MPI_VO_GetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
    stContext.mLayerAttr.stDispRect.X = 0;
    stContext.mLayerAttr.stDispRect.Y = 0;
    stContext.mLayerAttr.stDispRect.Width = 240;
    stContext.mLayerAttr.stDispRect.Height = 376;
    AW_MPI_VO_SetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);

/*    AW_MPI_VO_GetPubAttr(stContext.mVoDev, &spPubAttr);
    spPubAttr.enIntfType = VO_INTF_HDMI;
    spPubAttr.enIntfSync = VO_OUTPUT_1080P25;
    AW_MPI_VO_SetPubAttr(stContext.mVoDev, &spPubAttr);*/

    /* create vo channel and clock channel. 
    (because frame information has 'pts', there is no need clock channel now) 
    */
    BOOL bSuccessFlag = FALSE;
    stContext.mVOChn = 0;
    while(stContext.mVOChn < VO_MAX_CHN_NUM)
    {
        eRet = AW_MPI_VO_EnableChn(stContext.mVoLayer, stContext.mVOChn);
        if(SUCCESS == eRet)
        {
            bSuccessFlag = TRUE;
            alogd("create vo channel[%d] success!", stContext.mVOChn);
            break;
        }
        else if(ERR_VO_CHN_NOT_DISABLE == eRet)
        {
            alogd("vo channel[%d] is exist, find next!", stContext.mVOChn);
            stContext.mVOChn++;
        }
        else
        {
            aloge("fatal error! create vo channel[%d] ret[0x%x]!", stContext.mVOChn, eRet);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        stContext.mVOChn = MM_INVALID_CHN;
        aloge("fatal error! create vo channel fail!");
        result = -1;
        goto vo_create_chn_err;
    }
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)&stContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleVIRVI2VO_VOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(stContext.mVoLayer, stContext.mVOChn, &cbInfo);
    AW_MPI_VO_SetChnDispBufNum(stContext.mVoLayer, stContext.mVOChn, 0);

    /* bind clock,vo, viChn
    (because frame information has 'pts', there is no need to bind clock channel now)
    */
    MPP_CHN_S VOChn = {MOD_ID_VOU, stContext.mVoLayer, stContext.mVOChn};
    MPP_CHN_S VIChn = {MOD_ID_VIU, stContext.mVIDev, stContext.mVIChn};
    AW_MPI_SYS_Bind(&VIChn, &VOChn);

    /* start vo, vi_channel. */
    eRet = AW_MPI_VI_EnableVirChn(stContext.mVIDev, stContext.mVIChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! enableVirChn fail!");
        result = eRet;
        goto vi_enable_virchn_err;
    }
    AW_MPI_VO_StartChn(stContext.mVoLayer, stContext.mVOChn);

#if 0
    char str[256]  = {0};
    int num = 0, vl = 0;

    printf("\033[33m");
    printf("===========ISP test=========\n");
    printf("input <99> & ctrl+c to exit!\n");
    printf("input <1-12> to test\n");
    printf("============================\n");
    printf("\033[0m");
    while (1)
    {
        memset(str, 0, sizeof(str));
        fgets(str, 256, stdin);
        num = atoi(str);
        if (99 == num) {
			printf("break test.\n");
			printf("enter ctrl+c to exit this program.\n");
            break;
        }

        switch (num) {
            case 1:
                AW_MPI_ISP_AE_SetMode(iIspDev,0);//0 ,1---ok
                AW_MPI_ISP_AE_GetMode(iIspDev,&vl);
                printf("AE mode: current value = %d.\r\n", vl);
                break;
            case 2:
                AW_MPI_ISP_AE_SetExposureBias(iIspDev,4);//0~8---ok
                AW_MPI_ISP_AE_GetExposureBias(iIspDev,&vl);
                printf("AE exposure bias: current value = %d.\r\n", vl);
                break;
            case 3:
                AW_MPI_ISP_AE_SetExposure(iIspDev,1000);//
                AW_MPI_ISP_AE_GetExposure(iIspDev,&vl);//
                printf("AE exposure: current value = %d.\r\n", vl);
                break;
            case 4:
                AW_MPI_ISP_AE_SetGain(iIspDev,256);//256
                AW_MPI_ISP_AE_GetGain(iIspDev,&vl);//256
                printf("AE gain: current value = %d.\r\n", vl);
                break;
            case 5:
                AW_MPI_ISP_AWB_SetMode(iIspDev,0);//0,1---ok
                AW_MPI_ISP_AWB_GetMode(iIspDev,&vl);//0,1
                printf("AWB mode: current value = %d.\r\n", vl);
                break;
            case 6:
                AW_MPI_ISP_AWB_SetColorTemp(iIspDev,4);//0,1,2,3,4,5,6,7, ---ok
                AW_MPI_ISP_AWB_GetColorTemp(iIspDev,&vl);//0,1,2,3,4,5,6,7,
                printf("AWB clolor temperature: current value = %d.\r\n", vl);
                break;
            case 7:
            case 8:
            case 9:
                AW_MPI_ISP_SetBrightness(iIspDev,100);//0~256
                AW_MPI_ISP_GetBrightness(iIspDev,&vl);//0~256
                printf("brightness: current value = %d.\r\n", vl);
                break;
            case 10:
                AW_MPI_ISP_SetContrast(iIspDev,100);//0~256
                AW_MPI_ISP_GetContrast(iIspDev,&vl);//0~256
                printf("contrast: current value = %d.\r\n", vl);
                break;
            case 11:
                AW_MPI_ISP_SetSaturation(iIspDev,100);//0~256
                AW_MPI_ISP_GetSaturation(iIspDev,&vl);//0~256
                printf("saturation: current value = %d.\r\n", vl);
                break;
            case 12:
                AW_MPI_ISP_SetSharpness(iIspDev,100);//0~256
                AW_MPI_ISP_GetSharpness(iIspDev,&vl);//0~256
                printf("sharpness: current value = %d.\r\n", vl);
                break;
            default:
                printf("intput error.\r\n");
                break;
        }
    }

#endif
    pthread_t tIdHotplug;
    /* check the hotplug event */
    /*int mHotplug = 1;
    if (mHotplug) {
        eRet = pthread_create(&tIdHotplug, NULL, CheckHotplugEvent, &stContext);
        if (eRet < 0) {
            aloge("create CheckHotplugEvent thread failed!!, ret[%d]\n", eRet);
        }
    }
    pthread_join(tIdHotplug, NULL);*/

    /*if(HDMI_FLAG == TRUE)
    {
        pthread_t switchhdmi;
        if(stContext.HDMI_State == 1)
        {
            eRet = pthread_create(&switchhdmi, NULL, SwitchtoHDMI, &stContext);
            if (eRet < 0) {
                aloge("create CheckHotplugEvent thread failed!!, ret[%d]\n", eRet);
            }
        }
        pthread_join(tIdHotplug, NULL);
        m_bTestSuccess = true;
    }
    else
    {
        aloge("set m_bTestSuccess false");
        m_bTestSuccess = false;
        eRet = AW_MPI_VO_GetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
        stContext.mLayerAttr.stDispRect.X = 0;
        stContext.mLayerAttr.stDispRect.Y = 0;
        stContext.mLayerAttr.stDispRect.Width = 240;
        stContext.mLayerAttr.stDispRect.Height = 376;
        eRet = AW_MPI_VO_SetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
        eRet = AW_MPI_VO_GetPubAttr(stContext.mVoDev, &spPubAttr);
        spPubAttr.enIntfType = VO_INTF_LCD;
        spPubAttr.enIntfSync = VO_OUTPUT_NTSC;
        eRet = AW_MPI_VO_SetPubAttr(stContext.mVoDev, &spPubAttr);
        int mUILayer = HLAY(2, 0);
        eRet = AW_MPI_VO_GetVideoLayerAttr(mUILayer, &stContext.mLayerAttr);
        stContext.mLayerAttr.stDispRect.X = 0;
        stContext.mLayerAttr.stDispRect.Y = 0;
        stContext.mLayerAttr.stDispRect.Width = 240;
        stContext.mLayerAttr.stDispRect.Height = 376;
        eRet = AW_MPI_VO_SetVideoLayerAttr(mUILayer, &stContext.mLayerAttr);
        eRet = AW_MPI_VO_OpenVideoLayer(stContext.mUILayer);
    }*/

    AW_MPI_VO_GetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
    stContext.mLayerAttr.stDispRect.X = 0;
    stContext.mLayerAttr.stDispRect.Y = 0;
    stContext.mLayerAttr.stDispRect.Width = 1920;
    stContext.mLayerAttr.stDispRect.Height = 1080;
    AW_MPI_VO_SetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
    VO_PUB_ATTR_S spPubAttr;
    AW_MPI_VO_GetPubAttr(stContext.mVoDev, &spPubAttr);
    spPubAttr.enIntfType = VO_INTF_HDMI;
    spPubAttr.enIntfSync = VO_OUTPUT_1080P25;
    AW_MPI_VO_SetPubAttr(stContext.mVoDev, &spPubAttr);
    int mUILayer = HLAY(2, 0);
    AW_MPI_VO_CloseVideoLayer(mUILayer);
    aloge("HDMI test------mTestDuration = %d",stContext.mConfigPara.mTestDuration);
    if(stContext.mConfigPara.mTestDuration > 0)
    {
        cdx_sem_down_timedwait(&stContext.mSemExit, stContext.mConfigPara.mTestDuration*1000);
    }
    else
    {
        cdx_sem_down(&stContext.mSemExit);
    }
    m_bTestSuccess = true;

    /* stop vo channel, vi channel */
    AW_MPI_VO_StopChn(stContext.mVoLayer, stContext.mVOChn);
    AW_MPI_VI_DisableVirChn(stContext.mVIDev, stContext.mVIChn);

vi_enable_virchn_err:
    AW_MPI_VO_DisableChn(stContext.mVoLayer, stContext.mVOChn);
    stContext.mVOChn = MM_INVALID_CHN;
vo_create_chn_err:
    AW_MPI_VI_DestoryVirChn(stContext.mVIDev, stContext.mVIChn);
#if ISP_RUN
    AW_MPI_ISP_Stop(iIspDev);
    AW_MPI_ISP_Exit();
#endif
    AW_MPI_VI_DisableVipp(stContext.mVIDev);
    AW_MPI_VI_DestoryVipp(stContext.mVIDev);
    stContext.mVIDev = MM_INVALID_DEV;
    stContext.mVIChn = MM_INVALID_CHN;
    /* disable vo layer */
    AW_MPI_VO_DisableVideoLayer(stContext.mVoLayer);
    stContext.mVoLayer = -1;
    AW_MPI_VO_OpenVideoLayer(stContext.mUILayer); /* open ui layer. */
    AW_MPI_VO_AddOutsideVideoLayer(stContext.mUILayer);
    /* disalbe vo dev */
    AW_MPI_VO_Disable(stContext.mVoDev);
    stContext.mVoDev = -1;
vi_enable_vipp_err:
vi_create_vipp_err:
    /* exit mpp system */
    AW_MPI_SYS_Exit();
sys_init_err:
_exit:
//    MenuCtrl(m_bTestSuccess);
    destroySampleVIRVI2VOContext(&stContext);
    if (result == 0) {
        printf("sample_virvi2vo pass!\n");
        fwrite("P[HDMI] PASS",15,1,fd);
        aloge("hdmitest:hdmi pass");
    }else{
        fwrite("F[HDMI] FAIL",15,1,fd);
        aloge("hdmitest:hdmi fail");
    }

    return result;
}

