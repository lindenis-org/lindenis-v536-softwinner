#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "vi_venc_rtsp.h"

static void sig_handler(int signal_num)
{
    deinit_venc_module();
}

static int loadVirvi2VencConfig(Virvi2VencConfig *pConfig)
{
    int ret;
    memset(pConfig, 0, sizeof(Virvi2VencConfig));
    strncpy(pConfig->OutputFilePath, "test_out.h264", MAX_FILE_PATH_SIZE-1);
    pConfig->OutputFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    pConfig->DevNum = 1;
    pConfig->SrcFrameRate = 25;
    pConfig->SrcWidth = 1280;
    pConfig->SrcHeight = 720;
    pConfig->DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pConfig->EncoderType = PT_H264;
    pConfig->DestWidth = 640;
    pConfig->DestHeight = 480;
    pConfig->DestFrameRate = 25;
    pConfig->DestBitRate = 4 * (1 << 20);
    printf("dev_num=%d, src_width=%d, src_height=%d, src_frame_rate=%d",
            pConfig->DevNum,pConfig->SrcWidth,pConfig->SrcHeight,pConfig->SrcFrameRate);
    printf("dest_width=%d, dest_height=%d, dest_frame_rate=%d, dest_bit_rate=%d",
            pConfig->DestWidth,pConfig->DestHeight,pConfig->SrcFrameRate,pConfig->DestBitRate);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    Virvi2VencConfig mConfigPara;

    printf("===========================================================.\r\n");
    printf("\tsample_virvi2venc buile time = %s, %s.\r\n", __DATE__, __TIME__);
    printf("===========================================================.\r\n");

    signal(SIGINT, sig_handler);

    loadVirvi2VencConfig(&mConfigPara);
    ret = init_venc_module(&mConfigPara);
    if (ret < 0) {
        printf("init venc module failed!");
    } else {
        wait_venc_module_stop();
    }

_exit:
    printf("======================================.\r\n");
    printf("\tsample_virvi2venc exit!\n");
    printf("======================================.\r\n");

    return 0;
}
