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
#pragma once

#include "device_model/media/recorder/recorder.h"
#include <device_model/media/media_file.h>
#include <EyeseeThumbRetriever.h>


namespace EyeseeLinux {

class BackCameraRecorder
    : public Recorder
{
    public:
        BackCameraRecorder(Camera *camera);
        ~BackCameraRecorder();
        int InitRecorder(const RecorderParam &param);
        void DumpRecorderParm();
        int StopRecord();

        void SetNextFile(const std::string &file, int64_t filesize, int muxer_id);
        void SwitchFileNormal();
        void onInfo(EyeseeRecorder *pMr, int what, int extra);
        int addOutputFormatAndOutputSink();
        int removeOutputFormatAndOutputSink();
        void setWifiFlag(bool flag);
        void Update(MSG_TYPE msg, int p_CamID, int p_recordId=0);
		int GetSoSRecorderFileName(char *p_FileName);		
	    int setSosHappened(SosEventType p_EventType);
        int CheckStorage(int p_index, char *p_ReNameFile = "");
   private:
	    int takeSosThumbPic();
		int startSosRecord(SosEventType p_EventType);
		int stopSosRecord();
	//	int CheckStorage(void);
		int getFileMd5(const std::string p_FileName, std::string & p_Md5);
		int timeString2Time_t(std::string &timestr);

   private:
        int stream_muxer_id;
        bool isWifiEnabled;
		MediaFile *m_newFile, *m_DbFile;
		SosEventType m_bSosStatus;
		int  m_SosMuxerId;
		MediaFile *m_SosFile;
		EyeseeLinux::EyeseeThumbRetriever *m_ThumbRetriever;
		bool m_bRecordOnce;
		std::string m_fileName;
		std::string m_fileName2;
		std::string m_SosFileName;
        bool Back_SetNetFd_flag;
};

}/* EyeseeLinux */
