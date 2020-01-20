/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: recorder.h
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2015-11-10
Description:
    - IRecorder中定义recorder对外的公共接口
    - Recorder定义为不同类型Recorder的基类,
      CSI, UVC, NET_STREAM继承该基类实现一些自定义的方法
History:
*************************************************/
#pragma once

#include "device_model/media/media_definition.h"
#include "device_model/media/camera/camera.h"
#include "common/subject.h"
#include "common/observer.h"
#include "common/utils/utils.h"

#include <media/mm_common.h>
#include <EyeseeRecorder.h>
#include <vencoder.h>
#include <string>
#include <list>
#include <map>
#include <utils/Thread.h>

//#define WRITE_RAW_H264_FILE

namespace EyeseeLinux {
#define MAX_PACK_SIZE           ((1<<30) * 1.9)


typedef enum {
    NORMAL_RECORD_DIR        = 0,
    PARK_RECORD_DIR,
    SOS_RECORD_DIR,
} CheckDirID;


typedef enum {
    RECORDER_ERROR        = 0,
    RECORDER_IDLE         = 1 << 0,
    RECORDER_CONFIGURED   = 1 << 1,
    RECORDER_PREPARED     = 1 << 2,
    RECORDER_RECORDING    = 1 << 3,
    RECORDER_STOPPED      = 1 << 4,
    RECORDER_IMPACT_OCCUR = 1 << 5,
    RECORDER_IMPACT_DONE  = 1 << 6
} RecorderStatus;

/** 录像模式 */
typedef enum {
    REC_NORMAL = 0, /**< 普通类型, 用于写卡录像 */
    REC_STREAM      /**< 流类型, 用于流录制 */
} RecorderModel;


/** 视频编码类型 */
typedef enum {
    VENC_H264 = 0,
    VENC_H265 = 1,
    VENC_MJPEG = 2,
} VideoEncodeType;

/** 打包策略 */
typedef enum {
    PACK_BY_DURATION = 0, /**< 以录像时长设定文件打包间隔 */
    PACK_BY_FILESIZE,     /**< 以录像文件大小设定文件打包间隔 */
} RecorderPackStrategy;

typedef enum
{
	SOSEvent_Stop = 0,
	SOSEvent_Normal,
	SOSEvent_Attention,
	SOSEvent_NonAttention,
}SosEventType;
/** Recorder参数 */
typedef struct RecorderParam
{
    int audio_record;
    /**
     * 0: no stream send
     * 1: udp_raw
     * 2: rtsp
     */
    uint8_t stream_type;
    /**
     * 0: h254
     * 1: h265
     * 2: mjpeg
     */
    uint8_t enc_type;
    uint8_t framerate;  /**< 帧率 */
    uint32_t bitrate;   /**< 码率 */
    uint32_t gop;
    /**
     * the cache time about the impact file, unit: ms
     */
    uint16_t cache_time;
    /**
     * the time to continue to record  about impact file, unit: ms
     */
    uint16_t continue_time;
    RecorderPackStrategy pack_strategy;
    union {
        uint32_t cycle_time;    /**< 循环录像周期时间 */
        uint32_t file_size;     /**< 循环录像文件大小 */
    };

    Size video_size;    /**< 录像尺寸 */
    std::string file;   /**< 录像文件名 */
    double delay_fps ;   /*set delay record fps*/

    MediaFile *MF;

    int vi_chn; 
	int mux_mode; /*0: others 1: mux 1fps  */
	bool m_fileVisible;
    int bitrate_ctrl;
    int recorder_mode;	/*0:normal 1: park 2: sos */
    bool lock_file_flag;
	int timelaps;
    RecorderParam() {
        audio_record = 1;
        stream_type = 0;
        framerate = 30;
        pack_strategy = PACK_BY_DURATION;
        MF = NULL;
        lock_file_flag = false;
		timelaps = 0;
    }

    RecorderParam(RecorderModel type) {
        if (type == REC_NORMAL) {
            audio_record = 1;
            stream_type = 0;          /**< 写卡录制 */
            framerate = 30;
            bitrate = (1 << 20) * 10; /**< 10Mbps */
            pack_strategy = PACK_BY_DURATION;
            cycle_time = 60*1000;     /**< 1min */
            delay_fps = 0.0;
            MF = NULL;
            lock_file_flag = false;
			timelaps = 0;

        } else if(type == REC_STREAM) {
            audio_record = 0;
            stream_type = 2;          /**< 流录制 */
            framerate = 30;
            bitrate = (1 << 20) * 2;  /**< 2Mbps */
            pack_strategy = PACK_BY_DURATION;
            cycle_time = 0;           /**< 无循环录制 */
            delay_fps = 0.0;
            MF = NULL;
            lock_file_flag = false;
			timelaps = 0;
        }
    }

} RecorderParam;

struct EncodeDataCallbackParam {
    class Recorder *rec_;
    int what_;
    int extra_;
    void *context_;
    bool need_exit_;
};

typedef void (*EncodeDataCallback)(EncodeDataCallbackParam *param);

class Recorder
    : public EyeseeRecorder::OnInfoListener
    , public EyeseeRecorder::OnErrorListener
    , public EyeseeRecorder::OnDataListener
    , public ISubjectWrap(Recorder)
    , public IObserverWrap(Recorder)
{
    public:
        Recorder(Camera *camera);
        virtual ~Recorder();

        virtual int InitRecorder(const RecorderParam &param) = 0;
        virtual int StartRecord(bool p_FileVisible=true);
        virtual int StopRecord();
        virtual int CheckStorage(void);
        virtual void DumpRecorderParm() = 0;

        virtual void setWifiFlag(bool flag)= 0;
        int ResetRecorder();
        uint16_t GetStatus();
        void GetParam(RecorderParam &param) const;
        void SetParam(const RecorderParam &param);
        void SetFileName(const std::string &filename);
        Camera *GetCamera(void) const;
        EyeseeRecorder *GetEyeseeRecorder(void) const;
        int SetMute(bool mute);

        // NOTE: 支持动态参数配置的接口，
        // 后续增加的可动态配置的参数均以接口的形式提供，
        // 不可动态配置的参数通过预设参数接口SetParam/GetParam设置
        int SetVideoEncBitrate(uint32_t bitrate);

        int SetVideoEncQPRange(int min, int max);

        int SetVideoEncIFrameInterval(int val);

        int ForceIFrame();

        bool GetVoiceStatus();

        virtual void onInfo(EyeseeRecorder *pMr, int what, int extra){}
        virtual void onError(EyeseeRecorder *pMr, int what, int extra);
        virtual void onData(EyeseeRecorder *pMr, int what, int extra);
        virtual int addOutputFormatAndOutputSink();
        virtual int removeOutputFormatAndOutputSink();
        virtual void SwitchFileNormal() {}

        void SetEncodeDataCallback(EncodeDataCallback func, void *context);

        int GetVencHeaderData(VencHeaderData &info);

        void GetVencHeaderData(VencHeaderData &sps_info, VencHeaderData &pps_info);

        virtual void Update(MSG_TYPE msg, int p_CamID, int p_recordId) = 0;

        unsigned int GetID() const;

        void SetID(unsigned int id);

        char GetStreamSenderType() const;

        void SetStreamSenderType(char stream_sender_type);
        int SetDelayRecordTime(int val);
        int SetVideoClcRecordTime(int val);
        int SetRecordAudioOnOff(int val);
        int SetRecordEncodeSize(int val);
        int SetVideoEncoderType(int val);
		int SetMuxEncodeMode(int val);
        int GetClcRecordTime(int &val);
		int SetRecordTime(int p_Time);
		int SetRemoteAudioRecordType(int p_AudioType);
		virtual int setSosHappened(SosEventType p_EventType) { return 0;}
		bool RecorderIsBusy();
		virtual int GetSoSRecorderFileName(char *p_FileName) = 0;
		int GetRecordingFileName(std::string &p_RecFileName);
		int SetSosFileMaxDuration(int p_nTimeVal) { m_MaxDurationMs = p_nTimeVal; return 0;};//ms
		int GetSosFileMaxDuration() { return m_MaxDurationMs; };
        void checkRecordModeInParking();
        int GetCurrentClcRecordTime();
        int setLockFileFlag(bool flag);
        bool GetRecordStartFlag();
        int allocRecorderFile(char *p_fileName, int64_t p_fileSize, char *p_ReNameFile);        
        void setRecordMotionFlag(bool flag);
		int getRecordMode();
#ifdef WRITE_RAW_H264_FILE
        FILE *file_;
#endif

    protected:
        RecorderParam param_;
        EyeseeRecorder* recorder_;
        Camera *camera_;
        std::map<int, int> muxer_id_map_;
        pthread_mutex_t lock_;
        pthread_mutex_t enc_data_lock_;
        pthread_mutex_t alloc_file_lock;
        pthread_cond_t enc_data_cond_;
        int storage_status_;
        uint16_t current_status_;
        VencHeaderData pps_info_;
        VencHeaderData sps_info_;
        EncodeDataCallback enc_data_cb_;
        pthread_t cb_thread_id_;
        EncodeDataCallbackParam enc_data_cb_param_;
        bool enable_raw_data_;
        bool raw_data_readable_;
        int64_t file_allocate_size_;
        int64_t last_file_allocate_size_;
        static uint32_t record_time_;
        static bool delay_record_flag;		// 0-普通录影 1-缩时录影
        static std::map<int, std::string> file_list_;
        static uint32_t main_file_cnt_;
        static uint32_t sub_file_cnt_;
		int m_MaxDurationMs;
      //  bool lock_file_flag;

    private:
        bool is_silent_;
        unsigned int id_;
        char stream_sender_type;
        int ParseHeadData();
        bool m_startRecord;
        bool m_mtion_flag;
}; // class Recorder

} //namespace EyeseeLinux
