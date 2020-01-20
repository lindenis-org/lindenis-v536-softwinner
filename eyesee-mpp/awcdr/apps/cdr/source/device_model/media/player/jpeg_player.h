/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    jpeg_player.h
 * @brief   解码jpeg文件并通过vo显示
 * @author  id:826
 * @version v0.3
 * @date    2017-11-27
 */
#pragma once

#include <mm_common.h>
#include <mpi_sys.h>
#include <mpi_vdec.h>
#include <mpi_vo.h>

#include <string>
#include <mutex>
#include "common/subject.h"
#include <tsemaphore.h>
#include "device_model/media/media_definition.h"
namespace EyeseeLinux {

enum JpegPlayerState {
    JPEG_PLAYER_ERROR    = -1,
    JPEG_PLAYER_IDLE     = 0,
    JPEG_PLAYER_PREPARED = 1,
    JPEG_PLAYER_STARTED  = 2,
};

class JpegPlayer
        : public ISubjectWrap(JpegPlayer)
         ,public IObserverWrap(JpegPlayer)
{
    public:
        JpegPlayer();

        ~JpegPlayer();

        int SetDisplay(int hlay);

        int PrepareFile(std::string filepath,std::string type);

        int ShowPic();

        int ReleasePic();

        int Reset();

        static ERRORTYPE MPPVDecCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);

        static ERRORTYPE MPPVOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);

        void SetJpegPicSize(SIZE_S &size);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
    private:
        JpegPlayerState state_;
        std::mutex mutex_;
        cdx_sem_t sem_wait_from_vo_;
        int hlay_;
        VDEC_CHN vdec_chn_;
        VO_CHN vo_chn_;
        int new_stream_size_;
        VDEC_STREAM_S stream_info_;
        VIDEO_FRAME_INFO_S frame_info_;
        SIZE_S picsize_;
}; /* JpegPlayer */

} /* EyseeLinux */
