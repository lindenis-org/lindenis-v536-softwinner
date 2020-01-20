
#ifndef _AMICRECORD_H_
#define _AMICRECORD_H_

#include <pthread.h>

#include <plat_type.h>
#include <tsemaphore.h>
#include <mpi_sys.h>
#include <mpi_ai.h>

#include "SavePcmFile.h"

#define MAX_FILE_PATH_SIZE (256)

typedef struct AMicRecordCmdLineParam
{
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
}AMicRecordCmdLineParam;

typedef struct AMicRecordConfig
{
    char mDirPath[MAX_FILE_PATH_SIZE];
    char mFileName[MAX_FILE_PATH_SIZE];
    int mSampleRate;
    int mChannelCount;
    int mBitWidth;
    int mCapureDuration; //unit:s, 0:infinite
    int mbSaveWav;
}AMicRecordConfig;

typedef struct AMicRecordContext
{
    AMicRecordCmdLineParam mCmdLinePara;
    AMicRecordConfig mConfigPara;

    message_queue_t mMessageQueue;

    MPP_SYS_CONF_S mSysConf;
    AUDIO_DEV mAIDev;
    AI_CHN mAIChn;
    AIO_ATTR_S mAIOAttr;
    int mPcmSize;   //unit: byte
    int64_t mPcmDurationMs;   //unit:ms

    SavePcmFile *mpSavePcmFile;
}AMicRecordContext;
AMicRecordContext* constructAMicRecordContext();
void destructAMicRecordContext();

#endif  /* _AMICRECORD_H_ */

