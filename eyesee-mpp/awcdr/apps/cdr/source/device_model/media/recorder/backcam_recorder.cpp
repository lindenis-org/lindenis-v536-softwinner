/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: stream_recorder.cpp
Author: chenzhenhua@allwinnertech.com
Version: 0.1
Date: 2018-3-17
Description:
History:
*************************************************/

#include "device_model/media/recorder/backcam_recorder.h"
#include "common/app_log.h"
#include "common/app_def.h"

#include "device_model/media/media_file_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/aes_ctrl.h"
#include "device_model/system/event_manager.h"
#include "bll_presenter/AdapterLayer.h"

#include <stdlib.h>
#include <string.h>
#include <string>

#undef LOG_TAG
#define LOG_TAG "BackCameraRecorder.cpp"

using namespace EyeseeLinux;
using namespace std;

//#define tmp_test
#define EXPAND_FILE_SIZE 1.2

BackCameraRecorder::BackCameraRecorder(Camera *camera)
    : Recorder(camera)
{
     isWifiEnabled = false;
     stream_muxer_id = 0;
	 m_newFile = NULL;
	 m_DbFile = NULL;
	 m_bSosStatus = SOSEvent_Stop;
	 m_SosMuxerId = -1;
	 m_SosFile = NULL;
	 m_ThumbRetriever = NULL;
	 m_bRecordOnce = false;
	 m_fileName.clear();
	 m_fileName2.clear();
	 m_SosFileName.clear();
	 m_MaxDurationMs = 10*1000;//10s
     StorageManager::GetInstance()->Attach(this);
	 MediaFileManager *mfm = MediaFileManager::GetInstance();
     Back_SetNetFd_flag = false;
	 this->Attach(mfm);
}

BackCameraRecorder::~BackCameraRecorder()
{
	if(m_ThumbRetriever != NULL )
	{
		delete(m_ThumbRetriever);
		m_ThumbRetriever = NULL;
	}

    StopRecord();
    StorageManager::GetInstance()->Detach(this);
    MediaFileManager *mfm = MediaFileManager::GetInstance();
    this->Detach(mfm);
}

void BackCameraRecorder::DumpRecorderParm()
{
	return ;
}


int BackCameraRecorder::CheckStorage(int p_index, char *p_ReNameFile)
{
    int ret = -1;
    int status = 0;
	int delete_flag = 0;
    StorageManager *sm = StorageManager::GetInstance();
    // update storage status
    status = sm->GetStorageStatus();
    bool not_rename = false;
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
    	bool fullStatus;
		std::string QueryType;
		std::string file_name;
    	if(p_index == (int)NORMAL_RECORD_DIR )
	    {
	    	fullStatus = StorageManager::GetInstance()->CheckVideoRecordRDirFull();
			QueryType = "video_B";
            if(fullStatus)
            {
                int fileCountR = MediaFileManager::GetInstance()->GetMediaFileCnt("video_B", 0);
                if(fileCountR == 0)
                {
                    db_warn("Be careful Current R video file is all event video should stop record");
                    ret = -2;
                    return ret;
                }
            }
    	}
		else if(p_index == (int)SOS_RECORD_DIR)
		{
			fullStatus = StorageManager::GetInstance()->CheckVideoRecordRDirFull();
			QueryType = "videoB_SOS";
			not_rename = true;
            if(fullStatus)
            {
                int fileCountR = MediaFileManager::GetInstance()->GetMediaFileCnt("video_B", 0);
                if(fileCountR == 0)
                {
                    db_warn("Be careful Current R video file is all event video should stop record");
                    ret = -2;
                    return ret;
                }
            }
		}
		else if(p_index == (int)PARK_RECORD_DIR )
		{
			fullStatus = StorageManager::GetInstance()->CheckParkRecordDirFull();
			QueryType = "videoB_PARK";
			not_rename = true;
            if(fullStatus)
            {
                db_msg("Be Careful the park Dir is full!");
                ret = -3;
                return ret;
            }
		}
		ret = 0;
		db_error("fullStatus is: %d",fullStatus);
		while( fullStatus )
		{
            last_file_allocate_size_ = MediaFileManager::GetInstance()->GetLastFileFallocateSizeByType(QueryType, 0);
		    db_error(" R FULL the file_allocate_size_ = %lld ,last_file_allocate_size_ = %lld",file_allocate_size_,last_file_allocate_size_);
            if(file_allocate_size_ == last_file_allocate_size_){
                if(!not_rename){
                    MediaFileManager::GetInstance()->GetLastFileByType(QueryType, 1, file_name);
                    MediaFileManager::GetInstance()->DeleteLastFilesDatabaseIndex(file_name, 1);
                    if(p_ReNameFile != NULL && !file_name.empty()){
                        strncpy(p_ReNameFile, file_name.c_str(), sizeof(char)*64);
                        db_warn("sd card is full, get rename file name %s",p_ReNameFile);
                    }else{
                        db_error("file name is empty!!!");
                    }
                }else{
                   // db_warn("sd card is full, no need rename delete file");
                //    MediaFileManager::GetInstance()->DeleteLastFile(QueryType, 1, 1, 1);
                   // delete_flag = 1;
                }

                break;
            }else{
                 //在满的情况下，当前文件与最前文件预分配大小不一样，就先删文件，在判断大小。
                 int count1 = 0;
                 delete_flag = 1;
                 uint32_t reserv1_size = 0;
                  while(delete_flag)
                  {
                      count1++;
                      MediaFileManager::GetInstance()->DeleteLastFile(QueryType, 1, 0, 1);
                      reserv1_size = StorageManager::GetInstance()->getvideoRDirReserveSize();
                      db_msg("R dir reserv_size = %lld,file_allocate_size_ = %lld",reserv1_size,file_allocate_size_);
                      if(reserv1_size >= file_allocate_size_)
                      {
                          db_msg("R dir recive need space to create video file");
                          delete_flag = 0;
                          count1 = 0;
                          p_ReNameFile = "";
                          return ret;
                      }else{
                          if(count1 == 50)
                          {
                              db_msg("some this is wrong break while");
                              delete_flag = 0;
                              count1 = 0;
                              ret = -2;
                              return ret;
                          }
                      }
                  }
            }
		}
		if(delete_flag){
			db_warn("send msg MSG_DELETE_VIDEOFILE");
			delete_flag = 0;
			StorageManager::GetInstance()->Notify(MSG_DELETE_VIDEOFILE);
		}
		if(!fullStatus){
             //不满的情况，先判断剩余空间是否满足当前文件预分配大小，满足直接预分配，不满足则删除文件到满足预分配大小
             last_file_allocate_size_ = MediaFileManager::GetInstance()->GetLastFileFallocateSizeByType(QueryType, 0);
             unsigned long long  reserv_size = 0;
             db_error("R not full the file_allocate_size_ = %lld ,last_file_allocate_size_ = %lld",file_allocate_size_,last_file_allocate_size_);
             if((file_allocate_size_ != last_file_allocate_size_) && (last_file_allocate_size_ != 0)){
                 db_msg("R change record time or video resoulation");
                 if(p_index == (int)PARK_RECORD_DIR)
                 {
                     //待添加检测
                     reserv_size = StorageManager::GetInstance()->getvideoParkDirReserveSize();
                     db_error("R the not full check park dir reserv_size = %lld",reserv_size);
                 }else if(p_index == (int)NORMAL_RECORD_DIR || p_index == (int)SOS_RECORD_DIR)
                 {
                     reserv_size = StorageManager::GetInstance()->getvideoRDirReserveSize();
                     db_error("R the not full reserv_size = %lld",reserv_size);
                 }
                 if(reserv_size > file_allocate_size_)
                 {
                     p_ReNameFile = "";
                 }else
                 {
                     delete_flag = 1;
                     db_msg("Be careful R dir space is not enough should be delete file");
                     if(p_index == (int)PARK_RECORD_DIR)
                     {
                         ret = -3;
                         delete_flag = 0;
                         return ret;
                     }else if( p_index == (int)SOS_RECORD_DIR){
                         ret = -2;
                         delete_flag = 0;
                         return ret;
                     }else if(p_index == (int)NORMAL_RECORD_DIR)
                     {
                         int count = 0;
                         while(delete_flag)
                         {
                             count++;
                             MediaFileManager::GetInstance()->DeleteLastFile(QueryType, 1, 0, 1);
                             reserv_size = StorageManager::GetInstance()->getvideoRDirReserveSize();
                             db_msg(" R dir reserv_size = %lld,file_allocate_size_ = %lld",reserv_size,file_allocate_size_);
                             if(reserv_size >= file_allocate_size_)
                             {
                                 db_msg("recive need space to create video file");
                                 delete_flag = 0;
                                 count = 0;
                                 p_ReNameFile = "";
                                 return ret;
                             }else{
                                 if(count == 50)
                                 {
                                     db_msg("some this is wrong break while");
                                     delete_flag = 0;
                                     count = 0;
                                     ret = -2;
                                     return ret;
                                 }
                             }
                         }
                     }
                 }
             }else{
             db_error("set p_ReNameFile null");
             p_ReNameFile = "";
            }
        }
	}

    return ret;
}


#if 0

int BackCameraRecorder::CheckStorage(void)
{
    int ret = -1;
    int status = 0;
    StorageManager *sm = StorageManager::GetInstance();
    // update storage status
    status = sm->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR)  && (status != FORMATTING))
    {
Check:
		CapacityStatus status = StorageManager::GetInstance()->getBackCameraCapacityStatus();
		switch(status)
		{
			case CAPACITY_ENOUGH:
				return 0;
			case CAPACITY_WARN:
			{
				//send message to dd platform
				#if 0
				EventReportMsg event;
                event.err_no = 0;
				event.event_type = EVENT_SD_REQ_DELETE_FILES;
				event.file_name.clear();
				AdapterLayer::GetInstance()->notifyMessage(event);
                #endif
				return 0;
			}
			case CAPACITY_EMERGENCY:
			{
				unsigned int dir_size = sm->getDirSize("/mnt/extsd/video/R");//KB
				if( dir_size <= FRONT_REC_RESERVE_SIZE)
				{
					if(MediaFileManager::GetInstance()->GetMediaFileCnt("", 1) > 0 )
					{
						db_warn("sd card is full, delete last B folder file fileCount");
						if( MediaFileManager::GetInstance()->DeleteLastFile("", 1, 1) < 0 )
							MediaFileManager::GetInstance()->DeleteLastFile("", 1, 1, 1);
						goto Check;
					}
					else 
						sleep(1);
				}
				else
				{
					if(MediaFileManager::GetInstance()->GetMediaFileCnt("", 0) > 0 )
					{
						db_warn("sd card is full, delete last A folder file fileCount");
						MediaFileManager::GetInstance()->DeleteLastFile("", 1, 0);
						goto Check;
					}
					else
						sleep(1);
				}
			}
		}
	}

    return ret;
}
#endif

int BackCameraRecorder::InitRecorder(const RecorderParam &param)
{


    int ret = -1;

    if (current_status_ != RECORDER_CONFIGURED && current_status_ != RECORDER_IDLE) {
        db_error("status: [%d], recorder is not idle, you can not init recorder", current_status_);
        return ret;
    }
    char ReNameFile[128] = {0};
    if(param_.recorder_mode == 1)
    {
        file_allocate_size_ = calc_record_size_by_time(param_.bitrate, 15*1000);
    }else{
        file_allocate_size_ = calc_record_size_by_time(param_.bitrate, param_.cycle_time);
    }
    ret = CheckStorage(param_.recorder_mode,ReNameFile);
    this->param_ = param;
    if (ret < 0)
	{
		db_warn("storage is not ok, can not do file record");
		return ret;
	}

    PAYLOAD_TYPE_E enc_type = PT_H264;
    switch (param_.enc_type) {
        case VENC_H264:
            enc_type = PT_H264;
            break;
        case VENC_H265:
            enc_type = PT_H265;
            break;
        case VENC_MJPEG:
            enc_type = PT_MJPEG;
            break;
        default:
            db_warn("unsuppoort video encode type, use h264 as default");
            enc_type = PT_H264;
            break;

    }

    //C26a new add
    db_warn("[debug_jason]:Back cam recorder InitRecorder param.audio_record = %d\n",param.audio_record);
    if (param.audio_record) {
        recorder_->setAudioSource(EyeseeRecorder::AudioSource::MIC);
        recorder_->setAudioSamplingRate(44100);
        recorder_->setAudioChannels(1);
        recorder_->setAudioEncodingBitRate(12200);
        recorder_->setAudioEncoder(PT_AAC);
        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_AUDIO);
    } else {
        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_ONLY);
    }
#if 0
	if (param.audio_record) {
		recorder_->setMuteMode(false);
	} else {
		recorder_->setMuteMode(true);
	}
#endif


#if 0
    if (param.audio_record)
	{
        recorder_->setAudioSource(EyeseeRecorder::AudioSource::MIC);
        recorder_->setAudioSamplingRate(44100);
        recorder_->setAudioChannels(1);
        recorder_->setAudioEncodingBitRate(12200);
        recorder_->setAudioEncoder(PT_AAC);
		if( param.audio_record == 1)
	        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_AUDIO);
		else if( param.audio_record == 2)
		{
			recorder_->enableAttachAACHeader(false);
			m_bRecordOnce = true;
			record_time_ = param.cycle_time;

			if( record_time_  == 0)
			{
				m_bRecordOnce = false;
				record_time_ = 1*60*1000;
			}

			recorder_->setMaxDuration(record_time_);
			string file_name;

			if(param.file.empty())
			{
				m_DbFile = new MediaFile(camera_->GetCameraID(), AUDIO_B, param_.m_fileVisible);
				file_name = m_DbFile->GetMediaFileName();
				m_fileName.clear();
				string::size_type rc = file_name.rfind("/");
				if( rc == string::npos )
				{
					db_warn("invalid fileName:%s",file_name.c_str());
					return -1;
				}
				m_fileName = "/tmp"+file_name.substr(rc);
			}
			else
			{
				m_DbFile = new MediaFile(param.file, param_.m_fileVisible);
				m_fileName = param.file;
				m_fileName.clear();
				string::size_type rc = param.file.rfind("/");
				if( rc == string::npos )
				{
					db_warn("invalid fileName:%s",param.file.c_str());
					return -1;
				}
				m_fileName = "/tmp"+param.file.substr(rc);
			}
			param_.MF = m_DbFile;
			int muxer_id = recorder_->addOutputFormatAndOutputSink(MEDIA_FILE_FORMAT_AAC,(char *)m_fileName.c_str(), 0, false);
			muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_MP4, muxer_id));
			ret = recorder_->prepare();
			if (ret != NO_ERROR)
			{
				db_error("prepare Failed(%d)", ret);
				return ret;
			}
			
			current_status_ = RECORDER_PREPARED;
			return ret;
		}
    }
	else
	{
        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_ONLY);
    }
#endif
    recorder_->setCameraProxy(camera_->GetEyeseeCamera()->getRecordingProxy(),param_.vi_chn);

    ret = recorder_->setVideoSource(EyeseeRecorder::VideoSource::CAMERA);
    if (ret != NO_ERROR)
	{
        db_error("setVideoSource Failed(%d)", ret);
        return ret;
    }

    ret = recorder_->setVideoFrameRate(param_.framerate);
    if (ret != NO_ERROR)
	{
        db_error("setVideoFrameRate Failed(%d)", ret);
        return ret;
    }
#if 0
    ret = recorder_->setVideoEncodingBitRate(param_.bitrate);
    if (ret != NO_ERROR)
	{
        db_error("setVideoEncodingBitRate Failed(%d)", ret);
        return ret;
    }

    ret = recorder_->setVideoEncodingBufferTime(10*1000);
    if (ret != NO_ERROR)
	{
        db_error("setVideoEncodingBufferTimeFailed(%d)", ret);
    }
    recorder_->setMuxCacheDuration(m_MaxDurationMs/2);
#endif

    ret = recorder_->setVideoSize(param_.video_size.width, param_.video_size.height);
    if (ret != NO_ERROR)
	{
        db_error("setVideoSize Failed(%d)", ret);
        return ret;
    }

    recorder_->enableVideoEncodingPIntra(false);
    recorder_->setVideoEncoder(enc_type);
    recorder_->enable3DNR(4);

    #if 0
        // 帧内预测
      //  recorder_->enableVideoEncodingPIntra(true);
        // 自适应调整帧内预测强度, 只有H265支持
        recorder_->enableAdaptiveIntraInp(true);
    //    recorder_->enableVideoEncodingLongTermRef(false);
        recorder_->enableFastEncode(false);
        //recorder_->setVideoEncodingIFramesNumberInterval(150);
    //    recorder_->SetVideoEncodingQpRange(1, 51);
    //    recorder_->enable3DNR(4);
	#else
        // 插帧
        recorder_->enableAdaptiveIntraInp(false);
        recorder_->enableFastEncode(false);
        //recorder_->enablePSkip(true);
        #ifdef SUPPORT_PSKIP_ENABLE
		recorder_->enableNullSkip(true);    // true=插空帧 false=不插帧
		#else
		recorder_->enableNullSkip(false);
		#endif
        recorder_->enableIframeFilter(false);
	#endif


    recorder_->setVideoEncodingRateControlMode((EyeseeRecorder::VideoEncodeRateControlMode)param_.bitrate_ctrl);
    if (param_.bitrate_ctrl == EyeseeRecorder::VideoRCMode_CBR){
        EyeseeRecorder::VEncBitRateControlAttr stRcAttr;
        stRcAttr.mVEncType = enc_type;
        stRcAttr.mRcMode = EyeseeRecorder::VideoRCMode_CBR;
        
        switch (stRcAttr.mVEncType)
               {
                   case PT_H264:
                   {
                       stRcAttr.mAttrH264Cbr.mBitRate = param_.bitrate;
                       stRcAttr.mAttrH264Cbr.mMaxQp = 51;
                       stRcAttr.mAttrH264Cbr.mMinQp = 10;
                       break;
                   }
                   case PT_H265:
                   {
                       stRcAttr.mAttrH265Cbr.mBitRate = param_.bitrate;
                       stRcAttr.mAttrH265Cbr.mMaxQp = 51;
                       stRcAttr.mAttrH265Cbr.mMinQp = 10;
                       break;
                   }
                   case PT_MJPEG:
                   {
                       stRcAttr.mAttrMjpegCbr.mBitRate = param_.bitrate;
                       break;
                   }
                   default:
                   {
                       db_error("unsupport video encode type, check code!");
                       break;
                   }
               }
               ret = recorder_->setVEncBitRateControlAttr(stRcAttr);
               if (ret != NO_ERROR) {
                   db_error("setVideoEncodingBitRate Failed(%d)", ret);
                   return ret;
               }
    }

    EyeseeRecorder::VEncAttr stVEncAttr;
    memset(&stVEncAttr, 0, sizeof(EyeseeRecorder::VEncAttr));
    stVEncAttr.mType = enc_type;
    recorder_->setVEncAttr(&stVEncAttr);

    if (ret != NO_ERROR)
	{
        db_error("setVideoEncoder Failed(%d)", ret);
        return ret;
    }

	record_time_ = param.cycle_time;

	if(record_time_  == 0)
	{
		m_bRecordOnce = false;
		record_time_ = 1*60*1000;
	}

    if(param_.recorder_mode == 1){
        m_bRecordOnce = true;
        record_time_ = 15*1000;
        db_warn("be careful Impact record is happen\n");
    }

//    file_allocate_size_ = calc_record_size_by_time(param_.bitrate, record_time_)*EXPAND_FILE_SIZE;
    db_error("back cam rec:set max duration cycle_time %d",record_time_);
    recorder_->setMaxDuration(record_time_);

    db_msg("param.file:%s",param.file.c_str());
	int status = StorageManager::GetInstance()->GetStorageStatus();
	if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
	{
		if(param.file.empty())
		{
            switch(param_.recorder_mode)
            {
                        case 0:
                            m_DbFile = new MediaFile(camera_->GetCameraID(), VIDEO_B);
                            m_fileName = m_DbFile->GetMediaFileName();
                            file_allocate_size_ = calc_record_size_by_time(param_.bitrate, param_.cycle_time);
                            break;
                        case 1:
                        {
                            m_DbFile = new MediaFile(camera_->GetCameraID(), VIDEO_B_PARK);
                            m_fileName = m_DbFile->GetMediaFileName();
                            file_allocate_size_ = calc_record_size_by_time(param_.bitrate, 15*1000);
                            break;
                        }
                        case 2:
                            break;
            }
            db_warn("m_filename is %s",m_fileName.c_str());
		}
		else
		{
		}

	}
	else 
	{
		db_warn("sd card has been removed,did not record any more");
		system("rm /overlay/upperdir/mnt/* -rf");
		return -1;
	}


	param_.MF = m_DbFile;
    db_msg("file name: %s", m_fileName.c_str());
    db_error("record time: %u, bitrate: %d,file allocate size %lld",
                param_.cycle_time, param_.bitrate,file_allocate_size_);
    char *filename = const_cast<char*>(m_fileName.c_str());
    allocRecorderFile(filename,file_allocate_size_,ReNameFile);
   int muxer_id;
   // if( param_.recorder_mode == 2 || (param_.recorder_mode == 1 && m_MonitoryType == Ac_Impact_Monitor))
   //     muxer_id = recorder_->addOutputFormatAndOutputSink(MEDIA_FILE_FORMAT_TS, -1, 0, false);
  //  else
        muxer_id = recorder_->addOutputFormatAndOutputSink(MEDIA_FILE_FORMAT_MP4,(char *)m_fileName.c_str(), 0, false);

    muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_MP4, muxer_id));
    //open ve debug node
    VeProcSet mVeProcSet;
    memset(&mVeProcSet, 0, sizeof(mVeProcSet));
    mVeProcSet.bProcEnable = 1;
    mVeProcSet.nProcFreq = 30;
    mVeProcSet.nStatisBitRateTime = 1000;
    mVeProcSet.nStatisFrRateTime = 1000;
    recorder_->setProcSet(&mVeProcSet);

    //add enable gps package
    recorder_->gpsInfoEn(1);
    

    ret = recorder_->prepare();
    if (ret != NO_ERROR)
	{
        db_error("prepare Failed(%d)", ret);
        return ret;
    }

    current_status_ = RECORDER_PREPARED;

    return ret;
}


int BackCameraRecorder::addOutputFormatAndOutputSink()
{
    if(param_.MF != NULL)
	{
        if (delay_record_flag == false)
		{
            char *file_name = const_cast<char*>(param_.MF->GetVideoThumbFileName().c_str());
            db_msg("stram_recoder: thumb video file name is %s",file_name);
            stream_muxer_id = recorder_->addOutputFormatAndOutputSink(MEDIA_FILE_FORMAT_MP4,file_name, 0, false);
            if(stream_muxer_id < 0)
            {
                db_error("addOutputFormatAndOutputSink failed");
                return -1;
            }
            muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_MP4, stream_muxer_id));
        }
    }
    return 0;
}


int  BackCameraRecorder::removeOutputFormatAndOutputSink()
{
    if(stream_muxer_id < 0)
	{
      db_debug("removeOutputFormatAndOutputSink invild id %d",stream_muxer_id);
      return -1;
    }

    if(recorder_->removeOutputFormatAndOutputSink(stream_muxer_id) < 0)
    {
        db_error("removeOutputFormatAndOutputSink fialed");
        return -1;
    }

    return 0;
}

void BackCameraRecorder::setWifiFlag(bool flag)
{
    isWifiEnabled = flag;
    db_msg("setWifiFlag is %d",isWifiEnabled);
}

void BackCameraRecorder::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    switch (msg)
	{
        case MSG_STORAGE_FS_ERROR:
        case MSG_STORAGE_IS_FULL:
            storage_status_ = STORAGE_NOT_OK;
            if(isWifiEnabled)
			{
                db_debug("do nothing when apk is connect");
                if(stream_muxer_id >  0)
				{
                    recorder_->removeOutputFormatAndOutputSink(stream_muxer_id);
                    stream_muxer_id = 0;
                }
            }
			else
			{
                this->StopRecord();
            }
            break;
        default:
            break;
    }
}

int BackCameraRecorder::StopRecord()
{
    db_msg("StopRecord");
	if( current_status_ == RECORDER_IDLE)
		return 0;

    int ret = Recorder::StopRecord();
	if(ret == -1){
		db_msg("StopRecord record is not start");
	}else{
		sync();
	}
    current_status_ = RECORDER_IDLE;

    return ret;
}


int BackCameraRecorder::timeString2Time_t(std::string &timestr)
{	
	int year  = atoi(timestr.substr(0,  4).c_str());
	int month = atoi(timestr.substr(4,  2).c_str());
	int day   = atoi(timestr.substr(6,  2).c_str());
	int hour  = atoi(timestr.substr(9,  2).c_str());
	int min   = atoi(timestr.substr(11, 2).c_str());
	int sec   = atoi(timestr.substr(13, 2).c_str());

	db_msg("time: %d-%d-%d, %d-%d-%d", year, month, day, hour, min, sec);

	struct tm tm;
	tm.tm_year = year - 1900;
	tm.tm_mon  = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min  = min;
	tm.tm_sec  = sec;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;

	return (int)mktime(&tm);
}

void BackCameraRecorder::onInfo(EyeseeRecorder *pMr, int what, int extra)
{
    MediaFileManager *mfm = MediaFileManager::GetInstance();
    StorageManager *sm = StorageManager::GetInstance();

    switch(what)
	{
        case EyeseeRecorder::MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD:
		{
            db_warn("set next fd m_bRecordOnce:%d",m_bRecordOnce);
            Back_SetNetFd_flag = true;
			if( m_bRecordOnce )
				break;
            char ReNameFile[128] = "";
            if (this-> CheckStorage(param_.recorder_mode, ReNameFile) < 0){
					break;
            }
            #if 0
			if(m_SosMuxerId == extra)
			{
				break;
			}
            #endif
            
			int nMediatype = VIDEO_B;
			if( param_.recorder_mode == 0)
            {
				nMediatype = VIDEO_B;
			}
			else
            {
             //   if( m_MonitoryType == Normal_Monitor)
    			//	nMediatype = VIDEO_B;
            //    else
    			nMediatype = VIDEO_B_PARK;
            }
            delete m_newFile;
            m_newFile = NULL;
            
            string file_name;
			if( param_.audio_record == 2 )
				m_newFile = new MediaFile(camera_->GetCameraID(), AUDIO_B, param_.m_fileVisible);
			else
				m_newFile = new MediaFile(camera_->GetCameraID(), nMediatype, param_.m_fileVisible);
			file_name = m_newFile->GetMediaFileName();
            db_msg("onInfo: SetNextFile name is %s", file_name.c_str());
			if( param_.m_fileVisible )
				m_fileName2 = file_name;
			else
			{
                #if 0
				m_fileName2.clear();
				string::size_type rc = file_name.rfind("/");
				if( rc == string::npos )
				{
					db_warn("invalid fileName:%s",file_name.c_str());
					break;
				}
					m_fileName2 = "/tmp"+file_name.substr(rc);
                #endif
			}
#ifdef FALLOCATE_SUPPORT
            if(current_status_ == RECORDER_RECORDING){

			char *filename = const_cast<char*>(m_fileName2.c_str());
			db_error("file_allocate_size_ %lld",file_allocate_size_);
			this->allocRecorderFile(filename,file_allocate_size_,ReNameFile);
#endif
			if(param_.audio_record == 2)
				this->SetNextFile(m_fileName2, 0, extra);
			else
	            this->SetNextFile(m_fileName2, 0, extra);
			    param_.MF = m_newFile;
            }else{
                db_msg("[debug_jaosn]:backRecord MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD#####");
                delete m_newFile;
			    m_newFile = NULL;
            }
            
    	    break;
        }

        case EyeseeRecorder::MEDIA_RECORDER_INFO_RECORD_FILE_DONE:
		{
         #if 0
            if(m_SosMuxerId == extra)
			{
				if( m_SosFile != NULL )
				{
					//save sos thumb picture
					if( param_.m_fileVisible )
					{
						takeSosThumbPic();
						mfm->AddFile(*m_SosFile);
					}
					else
					{
                        #if 0
						string Md5, Key;
						getFileMd5(m_SosFileName, Md5);
						string Imei;
						EventManager::GetInstance()->getIMEI(Imei);
						AesCtrl::GetInstance()->setUserKey(Md5+Imei);
						AesCtrl::GetInstance()->aes_encrypt(m_SosFileName, m_SosFile->GetMediaFileName());
						AesCtrl::GetInstance()->getKey(Key);
						mfm->AddFile(*m_SosFile, 1, Key);
                        #endif
					}
					std::string file_name = m_SosFile->GetMediaFileName();
					std::string really_name;
					m_SosFile->getFileReallyName(really_name);
					//Uber_Control::GetInstance()->activate_Upload_Object(timeString2Time_t(really_name), file_name);
					recorder_->removeOutputFormatAndOutputSink(m_SosMuxerId);
					m_SosFile = NULL;
					m_SosMuxerId = -1;
					m_SosFileName.clear();
				}
				m_bSosStatus = SOSEvent_Stop;
				break;
			}
#endif
			if( m_DbFile != NULL )
			{
				db_msg("recording finished");
				string Key;
				if( !param_.m_fileVisible )
				{
                    #if 0
					string Md5;
					getFileMd5(m_fileName, Md5);
					string Imei;
					EventManager::GetInstance()->getIMEI(Imei);
					AesCtrl::GetInstance()->setUserKey(Md5+Imei);
					AesCtrl::GetInstance()->aes_encrypt(m_fileName, m_DbFile->GetMediaFileName());
					AesCtrl::GetInstance()->getKey(Key);
					mfm->AddFile(*m_DbFile, 1, Key);
                    #endif
				}
				else{
                    if(param_.lock_file_flag){
                      //  db_warn("[debug_jaosn]:current is lock file,param_.lock_file_flag = %d",param_.lock_file_flag);
                       mfm->AddFile(*m_DbFile,0,file_allocate_size_,true);
                       param_.lock_file_flag = false;
                       std::string current_file_name = m_DbFile->GetMediaFileName();
                       std::string lock_file_name = mfm->getlockFileName();
                       std::string mv_buffer_sting = "mv -f " + current_file_name + " " + lock_file_name;
                     //  db_warn("mv_buffer_string is :%s",mv_buffer_sting.c_str());
                       std::string current_thumb_file_name = m_DbFile->GetVideoThumbPicFileName();
                       std::string lock_thumb_file_name = mfm->getlocThumbPickFileName();
                       std::string mv_thumb_buffer_sting = "mv -f " + current_thumb_file_name + " " + lock_thumb_file_name;
                       char reNameFile[128] = "";
                       if(this->CheckStorage(2, reNameFile) < 0)
                       {
                          db_warn("sd card is full or some this is wrong");
                       }
                       system(mv_buffer_sting.c_str());
                       system(mv_thumb_buffer_sting.c_str());
                       //mv file to event dir
                    }else{
                       //db_warn("");
                       db_error("add back file %s",(*m_DbFile).GetMediaFileName().c_str());
                       mfm->AddFile(*m_DbFile,0,file_allocate_size_,false);
                    }
					//mfm->AddFile(*m_DbFile);
				}
				if(m_bRecordOnce)
				{
                    #if 0
					EventReportMsg event;
                    event.err_no = 0;
					RecorderParam param;
					GetParam(param);
					event.event_type = EVENT_REQ_POST_LITEVIDEO;
                    printf("param.audio_record:%d\n", param.audio_record);
					if(param.audio_record == 2) {
						event.event_type = EVENT_REQ_POST_SEPARATEDAUDIO;
                    }
					event.file_name = m_DbFile->GetMediaFileName();
					AdapterLayer::GetInstance()->notifyMessage(event);
					record_time_ = 0;
                    #endif
				}
				else
                {
                    if(Back_SetNetFd_flag){
                    Back_SetNetFd_flag = false;
					this->Notify(MSG_TAKE_THUMB_VIDEO, 1, GetID());
                    }
			    }
				m_DbFile->SetFileVisibleFlag(true);
				delete m_DbFile;
				m_DbFile = NULL;
				if(m_bRecordOnce)
				{
					m_bRecordOnce = false;
					StopRecord();
				}
			}


			if(m_newFile != NULL)
			{
				if(current_status_ == RECORDER_STOPPED)
				{
					remove(m_fileName2.c_str());
					sm->RemoveFile(m_newFile->GetMediaFileName().c_str());
					sm->RemoveFile(m_newFile->GetVideoThumbPicFileName().c_str());
					current_status_ = RECORDER_IDLE;
				}
				else
				{
					m_fileName = m_fileName2;
					m_DbFile = m_newFile;
				}
				m_newFile = NULL;
			}
			else
			{
				m_fileName2.clear();
				current_status_ = RECORDER_IDLE;
			}
            break;
		}
        default:
            break;
    }
}

int BackCameraRecorder::takeSosThumbPic()
{
	m_ThumbRetriever = new EyeseeThumbRetriever();
	if( m_ThumbRetriever != NULL )
	{
		m_ThumbRetriever->setDataSource(const_cast<char*>(m_SosFile->GetMediaFileName().c_str()));
		std::string FileName = m_SosFile->GetMediaFileName();
		string::size_type rc = FileName.rfind(".");
		if( rc == string::npos)
		{
			db_warn("invalid FileName:%s",FileName.c_str());
			delete m_ThumbRetriever;
			m_ThumbRetriever = NULL;
			return -1;
		}
		std::string PicturePath = FileName.substr(0, rc)+ "_ths.jpg";

		std::shared_ptr<CMediaMemory> pJpegThumb = m_ThumbRetriever->getJpegAtTime(0, 640, 360);
		FILE *fp = fopen(PicturePath.c_str(), "wb");
		if( fp != NULL )
		{
			fwrite(pJpegThumb->getPointer(), 1, pJpegThumb->getSize(), fp);
			fclose(fp);
		}
		m_ThumbRetriever->reset();
		delete m_ThumbRetriever;
		m_ThumbRetriever = NULL;
	}
	return 0;
}

void BackCameraRecorder::SetNextFile(const string &file, int64_t filesize, int muxer_id)
{
    if(muxer_id_map_[MEDIA_FILE_FORMAT_MP4] != muxer_id)
	{
        db_error("fatal error! setNextFile muxerId[%d]!=[%d]", muxer_id_map_[MEDIA_FILE_FORMAT_MP4], muxer_id);
    }
    recorder_->setOutputFileSync(const_cast<char*>(file.c_str()), filesize, muxer_id);
}

void BackCameraRecorder::SwitchFileNormal()
{
	return ;
}

int BackCameraRecorder::setSosHappened(SosEventType p_EventType)
{
    db_warn(" BackCameraRecorder setSosHappened run here but nothing to do");
    #if 0
	if( m_bSosStatus == p_EventType )
		return 0;

	if (CheckStorage()<0)
		return -1;

	if( param_.audio_record != 2 )//only audio
	{
		if( p_EventType )
		{
			if( startSosRecord(p_EventType) < 0 )
				return -1;
		}
		else
			stopSosRecord();

		m_bSosStatus = p_EventType;
	}
    #endif
	return 0;
}

int BackCameraRecorder::startSosRecord(SosEventType p_EventType)
{
#if 0
	if( current_status_ != RECORDER_RECORDING )
	{
		db_warn("main recorder is not recording yet,so could not start sos record");
		return -1;
	}

	m_SosFile = new MediaFile(camera_->GetCameraID(), VIDEO_B_SOS,param_.m_fileVisible);
	if( m_SosFile == NULL )
	{
		db_error("generate front sos file failed\n");
		return -1;
	}

	string file_name = m_SosFile->GetMediaFileName();
	if(param_.m_fileVisible)
		m_SosFileName = file_name;
	else
	{
		string::size_type rc = file_name.rfind("/");
		if( rc == string::npos )
		{
			db_warn("invalid fileName:%s",file_name.c_str());
			return -1;
		}
		m_SosFileName = "/tmp"+file_name.substr(rc);

	}

    db_msg("m_SosFileName file name: %s", m_SosFileName.c_str());

	SinkParam sinkParam;
	memset(&sinkParam, 0, sizeof(SinkParam));
	sinkParam.mOutputFd 		= -1;
	sinkParam.mOutputPath		= (char *)(m_SosFileName.c_str());
	switch(p_EventType )
	{
		case SOSEvent_Normal:
			sinkParam.mFallocateLen 	= 0;
			sinkParam.mMaxDurationMs	= m_MaxDurationMs/*30*1000*/;
			break;
		case SOSEvent_Attention:
			sinkParam.mFallocateLen 	= 0;
			sinkParam.mMaxDurationMs	= 4*1000;
			break;
		case SOSEvent_NonAttention:
			sinkParam.mFallocateLen 	= 0;
			sinkParam.mMaxDurationMs	= 10*1000;
			break;
	}
	sinkParam.bCallbackOutFlag	= false;
	sinkParam.bBufFromCacheFlag = true;
	m_SosMuxerId = recorder_->addOutputSink(&sinkParam);
	if (m_SosMuxerId < 0)
	{
		db_error("fatal error! Start front sos record fail!");
		return -1;
	}

	db_msg("start front Sos Record success\n");
#endif
	return 0;
}

int BackCameraRecorder::stopSosRecord()
{
	return 0;
}

int BackCameraRecorder::getFileMd5(const string p_FileName,string & p_Md5)
{
	char name[128] = {0};
	snprintf(name, sizeof(name), "md5sum %s | cut -d ' ' -f1 > /tmp/backcam_md5.txt", p_FileName.c_str());
	system(name);
	int fd = open("/tmp/backcam_md5.txt", O_RDONLY, 444);
	if( fd < 0 )
	{
		system("rm /tmp/backcam_md5.txt");
		return -1;
	}
	char buf[128] = {0};

	int ret = read(fd, buf, 32);
	if( ret < 0 )
	{
		close(fd);
		system("rm /tmp/backcam_md5.txt");
		return -1;
	}

	p_Md5 = buf;

	close(fd);
	system("rm /tmp/backcam_md5.txt");

	return 0;
}

int BackCameraRecorder::GetSoSRecorderFileName(char *p_FileName)
{
	if(m_SosFile != NULL)
	{
		strncpy(p_FileName, m_SosFile->GetMediaFileName().c_str(), sizeof(char)*64);
	}

	return 0;
}
