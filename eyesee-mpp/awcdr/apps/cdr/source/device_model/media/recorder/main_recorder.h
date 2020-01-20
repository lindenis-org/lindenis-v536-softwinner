/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: recorder_impl.h
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2015-11-11
Description:
      实现具体的recorder类，其中：
        - MainRecorder 主录像类，实现前后摄像头的循环录像
        - StreamRecorder 流录像类，实现网络预览流的生成与发送
        - SubRecorder 子录像类，实现前摄像头的小视频生成
History:
*************************************************/
#ifndef _RECORDER_IMPL_H
#define _RECORDER_IMPL_H

#include <device_model/media/recorder/recorder.h>
#include <device_model/media/media_file.h>
#include <EyeseeThumbRetriever.h>

#define ADJUST_BITRATE
#define MINI_BITRATE_VALUE  10 * 1024 * 1024
#define DYNAMIC_BITRATE_INCREASE_ADJUSTMENT_INTERVAL 10 * 1024 * 1024
#define DYNAMIC_BITRATE_DECREASE_ADJUSTMENT_INTERVAL 10 * 1024 * 1024

namespace EyeseeLinux {

class MainRecorder
    : public Recorder
{
    public:
        MainRecorder(Camera *camera);
        ~MainRecorder();
        int InitRecorder(const RecorderParam &param);
        void DumpRecorderParm();

        virtual int StopRecord();

        void StopImpactRecord();
        void SetNextFile(const std::string &file, int64_t filesize, int muxer_id);
        void onInfo(EyeseeRecorder *pMr, int what, int extra);

        void Update(MSG_TYPE msg, int p_CamID, int p_recordId);
        int addOutputFormatAndOutputSink();
        int removeOutputFormatAndOutputSink();

        void setWifiFlag(bool flag){};

        int CheckStorageThresholdPercent(uint32_t threshold);
//		int CheckStorage(void);
        int CheckStorage(int p_index, char *p_ReNameFile = "");
		int GetSoSRecorderFileName(char *p_FileName);
		int setSosHappened(SosEventType p_EventType);
	private:
		int takeSosThumbPic();
		int startSosRecord(SosEventType p_EventType);
		int stopSosRecord();
		int timeString2Time_t(std::string &timestr);

    private:
        bool is_silent_;
        pthread_mutex_t file_lock_;
        MediaFile *v_file_; /**< current video file */
        MediaFile *n_file_; /**< next video file */
		SosEventType m_bSosStatus;
		int  m_SosMuxerId;
		MediaFile *m_SosFile;
		EyeseeLinux::EyeseeThumbRetriever *m_ThumbRetriever;
		bool m_bRecordOnce;
        bool Main_setNext_fd_flag;
        int dynamic_bitrate_;
        int dynamic_minQp_;
        int enc_type_;
        int default_bitrate_;
};

}/* EyeseeLinux */

#endif
