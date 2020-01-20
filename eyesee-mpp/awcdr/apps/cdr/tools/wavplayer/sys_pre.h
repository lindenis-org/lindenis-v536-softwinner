
#ifndef _SYS_PRE_H_
#define _SYS_PRE_H_


//ref platform headers
#include <plat_defines.h>
#include <plat_errno.h>
#include <plat_math.h>
#include <plat_type.h>

//media api headers to app
#include "alsa_interface.h"
#include "BufferManager.h"
#include <mm_comm_aio.h>
#include <mm_common.h>
#include <mm_component.h>
#include <tmessage.h>
#include <tsemaphore.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define AUDIO_STARTUP_FILE "/usr/share/minigui/res/audio/startup.wav"

#define AO_DEV_0     0
#define AO_DEV_1     1

#define AO_CHN_0     0
#define AO_CHN_1     1

#define AIO_DEV_MAX_NUM (1)

#define AIO_DEV_MAX_NUM (1)

typedef enum AO_CHANNEL_PORT_DEFINE_E
{
    AO_CHN_PORT_INDEX_IN_CLK = 0,
    AO_CHN_PORT_INDEX_IN_PCM,
    AO_CHN_PORT_INDEX_OUT_PLAY,
    AO_CHN_PORT_INDEX_OUT_AI,
    AO_CHN_MAX_PORTS,
} AO_CHANNEL_PORT_DEFINE_E;

#define CHECK_AI_DEV_ID(id)                         \
    do {                                            \
        if (id < 0 || id > (AIO_DEV_MAX_NUM - 1)) { \
            aloge("Invalid AudioDevId %d!", id);    \
            return ERR_AI_INVALID_DEVID;            \
        }                                           \
    } while (0)

#define CHECK_AI_CHN_ID(id)                         \
    do {                                            \
        if (id < 0 || id > (AIO_MAX_CHN_NUM - 1)) { \
            aloge("Invalid AI channel ID %d!", id); \
            return ERR_AI_INVALID_CHNID;            \
        }                                           \
    } while (0)

#define CHECK_AO_DEV_ID(id)                         \
    do {                                            \
        if (id < 0 || id > (AIO_DEV_MAX_NUM - 1)) { \
            aloge("Invalid AudioDevId %d!", id);    \
            return ERR_AO_INVALID_DEVID;            \
        }                                           \
    } while (0)

#define CHECK_AO_CHN_ID(id)                         \
    do {                                            \
        if (id < 0 || id > (AIO_MAX_CHN_NUM - 1)) { \
            aloge("Invalid AI channel ID %d!", id); \
            return ERR_AO_INVALID_CHNID;            \
        }                                           \
    } while (0)


struct WaveHeader {
    int riff_id;
    int riff_sz;
    int riff_fmt;
    int fmt_id;
    int fmt_sz;
    short audio_fmt;
    short num_chn;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
    int data_id;
    int data_sz;
};

struct PcmWaveParam {
    int trackCnt;
    int sampleRate;
    int bitWidth;
    int aoCardType;
};

struct PcmWaveData {
    struct PcmWaveParam param;
    void *buffer;
    int size;
};


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _SYS_PRE_H_ */

