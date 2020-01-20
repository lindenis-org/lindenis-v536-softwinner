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

namespace EyeseeLinux {

class Camera;
class StreamRecorder
    : public Recorder
{
    public:
        StreamRecorder(Camera *camera);
        ~StreamRecorder();
        int InitRecorder(const RecorderParam &param);

        int StopRecord();

        void Update(MSG_TYPE msg, int p_CamID, int p_recordId);
        void DumpRecorderParm();
        void setWifiFlag(bool flag);
        
        int GetSoSRecorderFileName(char *p_FileName);

};

}/* EyeseeLinux */
