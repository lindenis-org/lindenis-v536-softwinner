/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * ******************************************************************************
 */
/**
 * @file main_recorder.cpp
 * @brief 涓荤爜娴乺ecorder
 * @author id:826
 * @version v0.3
 * @date 2016-06-20
 */

#include "main_recorder.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/storage_manager.h"
#include "common/app_log.h"
#include "common/app_def.h"

#include "device_model/media/camera/camera_factory.h"
#include "bll_presenter/AdapterLayer.h"
//#include "uberflow.h"

#undef LOG_TAG
#define LOG_TAG "main_recorder.cpp"

using namespace EyeseeLinux;
using namespace std;

#define EXPAND_FILE_SIZE 1.2

MainRecorder::MainRecorder(Camera *camera)
    : Recorder(camera)
    , v_file_(NULL)
    , n_file_(NULL)
    , m_bSosStatus(SOSEvent_Stop)
    , m_SosMuxerId(-1)
    , m_SosFile(NULL)
    , m_bRecordOnce(false)
    , dynamic_bitrate_(0)
    , dynamic_minQp_(0)
    , enc_type_(0)
    , default_bitrate_(0)
{
    MediaFileManager *mfm = MediaFileManager::GetInstance();
    StorageManager *sm = StorageManager::GetInstance();
    int status = sm->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
        storage_status_ = STORAGE_OK;
    } else {
        storage_status_ = STORAGE_NOT_OK;
    }

	m_ThumbRetriever = NULL;
	m_MaxDurationMs = 10*1000;//10s
    // notify finish record msg
    this->Attach(mfm);
	sm->Attach(this);

    pthread_mutex_init(&file_lock_, NULL);
}

MainRecorder::~MainRecorder()
{
    this->StopRecord();

    pthread_mutex_lock(&file_lock_);
    if (v_file_)
        delete v_file_;
    if (n_file_)
        delete n_file_;
    pthread_mutex_unlock(&file_lock_);

    pthread_mutex_destroy(&file_lock_);
    MediaFileManager *mfm = MediaFileManager::GetInstance();
    this->Detach(mfm);
	StorageManager *sm = StorageManager::GetInstance();
	sm->Detach(this);
}

void MainRecorder::DumpRecorderParm()
{
    db_msg("######### This is MainRecorder param_ ############");
    db_msg("[debug_jaosn]:param_.audio_record = %d",param_.audio_record);
    db_msg("[debug_jaosn]:param_.bitrate = %d",param_.bitrate);
    db_msg("[debug_jaosn]:param_.enc_type = %d",param_.enc_type);
    db_msg("[debug_jaosn]:param_.cache_time = %d",param_.cache_time);
    db_msg("[debug_jaosn]:param_.pack_strategy = %d",param_.pack_strategy);
    db_msg("[debug_jaosn]:param_.cycle_time = %d",param_.cycle_time);
    db_msg("[debug_jaosn]:param_.file_size = %d",param_.file_size);
    db_msg("[debug_jaosn]:param_.delay_fps = %f",param_.delay_fps);
    db_msg("[debug_jaosn]:param_.framerate = %d",param_.framerate);
    db_msg("[debug_jaosn]:param_.stream_type = %d",param_.stream_type);
    db_msg("[debug_jaosn]:param_.recoer_mode = %d",param_.recorder_mode);
    db_msg("[debug_jaosn]:param_.video_size.width = %d,param_.video_size.height = %d",param_.video_size.width,param_.video_size.height);
    db_msg("delay_record_flag: %d", delay_record_flag);
}


int MainRecorder::CheckStorage(int p_index, char *p_ReNameFile)
{
    int ret = -1;
    int status = 0;
	int delete_flag = 0;
	bool not_rename = false;
    StorageManager *sm = StorageManager::GetInstance();
	db_error("need space:%lld", file_allocate_size_);
    // update storage status
    status = sm->GetStorageStatus();
    int try_count = 0;
    bool db_init_flag = false;
	int camid;
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
    	bool fullStatus;
		std::string QueryType;
		std::string file_name;
    	if(p_index == (int)NORMAL_RECORD_DIR)
	    {
	    	fullStatus = StorageManager::GetInstance()->CheckVideoRecordFDirFull();
			QueryType = "video_A";
            if(fullStatus)
            {
                int fileCountF = MediaFileManager::GetInstance()->GetMediaFileCnt("video_A", 0);
                if(fileCountF == 0)
                {
                    db_warn("Be careful Current video file is all event video should stop record");
                    ret = -2;
                    return ret;
                }
            }
    	}
		else if(p_index == (int)SOS_RECORD_DIR)
		{
			fullStatus = StorageManager::GetInstance()->CheckVideoRecordFDirFull();
			QueryType = "videoA_SOS";
			not_rename = true;
            if(fullStatus)
            {
                int fileCountF = MediaFileManager::GetInstance()->GetMediaFileCnt("video_A", 0);
                if(fileCountF == 0)
                {
                    db_warn("Be careful Current video file is all event video should stop record");
                    ret = -2;
                    return ret;
                }
            }
		}
		else if(p_index == (int)PARK_RECORD_DIR)
		{	// 占总容量 15%
			fullStatus = StorageManager::GetInstance()->CheckParkRecordDirFull();
			QueryType = "videoA_PARK";
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
		while(fullStatus)
		{
		    if( sm->GetStorageStatus() == UMOUNT) {
		        ret = -1;
		        db_error("sd umount!!");
		        return -1;
		    }
            last_file_allocate_size_ = MediaFileManager::GetInstance()->GetLastFileFallocateSizeByType(QueryType, 0);
		    db_error(" FULL the file_allocate_size_ = %lld ,last_file_allocate_size_ = %lld",file_allocate_size_,last_file_allocate_size_);
            if(file_allocate_size_ == last_file_allocate_size_){
                if(!not_rename){	// normal
                    MediaFileManager::GetInstance()->GetLastFileByType(QueryType, 0, file_name);
                    MediaFileManager::GetInstance()->DeleteLastFilesDatabaseIndex(file_name, 0);
                // 仅仅删除了最后一个文件, 如果这个文件很小而需要的空间很大呢?
                    if(p_ReNameFile != NULL && !file_name.empty()){
                        strncpy(p_ReNameFile, file_name.c_str(), sizeof(char)*64);
                        db_warn("sd card is full, get rename file name %s",p_ReNameFile);
                    }else{
                        db_error("file name is empty!!!");
                    }
                }else{
                   // db_warn("sd card is full, no need rename delete file");
                   // MediaFileManager::GetInstance()->DeleteLastFile(QueryType, 1, 0, 1);
                   // delete_flag = 1;
                }
            }else{
                //在满的情况下，当前文件与最前文件预分配大小不一样，就先删文件，在判断大小。
                 int count1 = 0;
                 delete_flag = 1;
                 uint32_t reserv1_size = 0;
                  while(delete_flag)
                  {
                      count1++;
                      MediaFileManager::GetInstance()->DeleteLastFile(QueryType, 1, 0, 1);
                      reserv1_size = StorageManager::GetInstance()->getvideoFDirReserveSize();
                      db_msg("reserv_size = %lld,file_allocate_size_ = %lld",reserv1_size,file_allocate_size_);
                      if(reserv1_size >= file_allocate_size_)
                      {
                          db_msg("recive need space to create video file");
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
			break;
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
		    db_error("F Dir not full the file_allocate_size_ = %lld ,last_file_allocate_size_ = %lld",file_allocate_size_,last_file_allocate_size_);
            if((file_allocate_size_ != last_file_allocate_size_) && (last_file_allocate_size_ != 0)){
                db_msg("F dir change record time or video resoulation");
                if(p_index == (int)PARK_RECORD_DIR)
                {
                    //待添加检测
                    reserv_size = StorageManager::GetInstance()->getvideoParkDirReserveSize();
                    db_error("F not full park dir the not full reserv_size = %lld",reserv_size);
                }else if(p_index == (int)NORMAL_RECORD_DIR || p_index == (int)SOS_RECORD_DIR)
                {
                    reserv_size = StorageManager::GetInstance()->getvideoFDirReserveSize(); 
                    db_error("F dir the not full reserv_size = %lld",reserv_size);
                }
                if(reserv_size > file_allocate_size_)
                {
                    p_ReNameFile = "";
                }else
                {
                    delete_flag = 1;
                    db_msg("F dir Be careful space is not enough should be delete file");
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
                            reserv_size = StorageManager::GetInstance()->getvideoFDirReserveSize();
                            db_msg("F dir reserv_size = %lld,file_allocate_size_ = %lld",reserv_size,file_allocate_size_);
                            if(reserv_size >= file_allocate_size_)
                            {
                                db_msg("F dir recive need space to create video file");
                                delete_flag = 0;
                                count = 0;
                                p_ReNameFile = "";
                                return ret;
                            }else{
                                if(count == 50)
                                {
                                    db_msg("F dir some this is wrong break while");
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
int MainRecorder::CheckStorage(void)
{
    int ret = -1;
    int status = 0;
	int delete_flag = 0;
    StorageManager *sm = StorageManager::GetInstance();
    // update storage status
    status = sm->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
		//????剩????量
		while( StorageManager::GetInstance()->IsFrontCameraFull() )
		{
			int fileCount = MediaFileManager::GetInstance()->GetMediaFileCnt("", 0);
			if(  fileCount > 0)
			{
				db_warn("sd card is full, delete last file fileCount:%d",fileCount);
				//if( MediaFileManager::GetInstance()->DeleteLastFile("", 1, 0) < 0 )
                    //FrontCamera is Full, force delete
				MediaFileManager::GetInstance()->DeleteLastFile("", 1, 0, 1);

                //delete a after camera last unlock file
				MediaFileManager::GetInstance()->DeleteLastFile("", 1, 1, false);
				delete_flag = 1;
			}
			else
			{
				sleep(1);
			}
			//send message to dd platform
		}
		if(delete_flag){
			db_warn("send msg MSG_DELETE_VIDEOFILE");
			delete_flag = 0;
			StorageManager::GetInstance()->Notify(MSG_DELETE_VIDEOFILE);
		}		
		ret = 0;
	}

    return ret;
}
#endif


int MainRecorder::CheckStorageThresholdPercent(uint32_t threshold)
{
    int ret = 0;
    uint32_t percent = 0;
    StorageManager *sm = StorageManager::GetInstance();
    ret = sm->GetStorageThreshold(threshold, &percent);
    if(ret == 0) {
        EventReportMsg event;
        event.err_no = 0;
        event.event_type = EVENT_SD_NO_SPACE_THRESHHOLD;
        event.file_name.clear();
        AdapterLayer::GetInstance()->notifyMessage(event);
    }

    return 0;
}


int MainRecorder::InitRecorder(const RecorderParam &param)
{
    int ret = -1;

    if (current_status_ != RECORDER_CONFIGURED && current_status_ != RECORDER_IDLE) {
        db_error("status: [%d], recorder is not idle, you can not init recorder", current_status_);
        return ret;
    }


    this->param_ = param;
	db_warn("[debug_jason]:Main recorder InitRecorder param.audio_record = %d\n",param.audio_record);
#if 1
	if (!param.timelaps) {	// 普通录像
	#if 0
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
	#else
			recorder_->setAudioSource(EyeseeRecorder::AudioSource::MIC);
	        recorder_->setAudioSamplingRate(44100);
	        recorder_->setAudioChannels(1);
	        recorder_->setAudioEncodingBitRate(12200);
	        recorder_->setAudioEncoder(PT_AAC);
	        recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_AUDIO);
			
			recorder_->setMuteMode(!param.audio_record);	// 菜单1=on(有声音) 0=off(无声音) 设置 0=有声 1=无声
			
	#endif
	} else {
		//param.audio_record = 0;
		recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_ONLY);
	}
#endif
    char ReNameFile[128] = {0};
    if(param_.recorder_mode == 1)
    {
        file_allocate_size_ = calc_record_size_by_time(param_.bitrate, 15*1000);
    }else{
        file_allocate_size_ = calc_record_size_by_time(param_.bitrate, param_.cycle_time);
    }
    db_msg("InitRecorder file_allocate_size_ = %lld ",file_allocate_size_);
    ret = CheckStorage(param_.recorder_mode,ReNameFile);
    if (ret < 0) {
        db_error("storage is not ok, can not do file record ret = %d",ret);
        return ret;
    }

	//C26a new add
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
    db_error("param_.recorder_mode: %d  enc type %d",param_.recorder_mode, enc_type);
    recorder_->setVideoEncoder(enc_type);


#if 1
    recorder_->setAudioSource(EyeseeRecorder::AudioSource::MIC);
	recorder_->setAudioSamplingRate(44100);
	recorder_->setAudioChannels(1);
	recorder_->setAudioEncodingBitRate(12200);
	recorder_->setAudioEncoder(PT_AAC);
	recorder_->setBsFrameRawDataType(CALLBACK_OUT_DATA_VIDEO_AUDIO);
	if (param.audio_record) {
		recorder_->setMuteMode(false);
	} else {
		recorder_->setMuteMode(true);
	}
#endif
	//recorder_->setMuxCacheDuration(m_MaxDurationMs/2);
    recorder_->setCameraProxy(camera_->GetEyeseeCamera()->getRecordingProxy(),param_.vi_chn);
    recorder_->setVideoSource(EyeseeRecorder::VideoSource::CAMERA);

    recorder_->setVideoFrameRate(param_.framerate);
    recorder_->setVideoSize(param_.video_size.width, param_.video_size.height);
    db_msg("recorder: framerate %d,video width %d,video height %d",
            param_.framerate,param_.video_size.width, param_.video_size.height);
    recorder_->setVideoEncodingRateControlMode((EyeseeRecorder::VideoEncodeRateControlMode)param_.bitrate_ctrl);
    if (param_.bitrate_ctrl == EyeseeRecorder::VideoRCMode_CBR){
        EyeseeRecorder::VEncBitRateControlAttr stRcAttr;
        stRcAttr.mVEncType = enc_type;
        stRcAttr.mRcMode = EyeseeRecorder::VideoRCMode_CBR;
        enc_type_ = enc_type;

        switch (stRcAttr.mVEncType)
               {
                   case PT_H264:
                   {
                       stRcAttr.mAttrH264Cbr.mBitRate = param_.bitrate;
                       stRcAttr.mAttrH264Cbr.mMaxQp = 51;
                       stRcAttr.mAttrH264Cbr.mMinQp = 10;
                       dynamic_bitrate_ = stRcAttr.mAttrH264Cbr.mBitRate;
                       dynamic_minQp_ = stRcAttr.mAttrH264Cbr.mMinQp;
                       default_bitrate_ = dynamic_bitrate_;
                       break;
                   }
                   case PT_H265:
                   {
                       stRcAttr.mAttrH265Cbr.mBitRate = param_.bitrate;
                       stRcAttr.mAttrH265Cbr.mMaxQp = 36;//51;
                       stRcAttr.mAttrH265Cbr.mMinQp = 16;//10;
                       dynamic_bitrate_ = stRcAttr.mAttrH265Cbr.mBitRate;
                       dynamic_minQp_ = stRcAttr.mAttrH265Cbr.mMinQp;
                       default_bitrate_ = dynamic_bitrate_;
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
#ifdef ADJUST_BITRATE
    db_error("enable DBCR");
    recorder_->enableDBRC(true);
#endif
    EyeseeRecorder::VEncAttr stVEncAttr;
    memset(&stVEncAttr, 0, sizeof(EyeseeRecorder::VEncAttr));
    stVEncAttr.mType = enc_type;
    stVEncAttr.mBufSize = 25 * 1024 * 1024;
    recorder_->setVEncAttr(&stVEncAttr);
	#if 0
    // 甯у唴棰勬祴
  //  recorder_->enableVideoEncodingPIntra(true);
    // 鑷�傚簲璋冩暣甯у唴棰勬祴寮哄害, 鍙湁H265鏀寔
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

//    file_allocate_size_ = 0;

    if (delay_record_flag == true)
	{
        // 鏇存柊record_time_鍊�, 浣垮瓙褰曞儚鏃堕棿鍚屾
        record_time_ = calc_record_time_by_size(param_.bitrate, MAX_ONE_FILE_SIZE);
        recorder_->setMaxDuration(record_time_ * 1000);
        if (delay_record_flag == true)
		{
            recorder_->setCaptureRate(param_.delay_fps);
        }
    }
	else
	{
        if (param_.pack_strategy == PACK_BY_DURATION)
		{
//            file_allocate_size_ = calc_record_size_by_time(param_.bitrate, param_.cycle_time)*EXPAND_FILE_SIZE;
            if(param_.recorder_mode == 1){
                db_warn(" Main revcord be careful Impact record is happen\n");
                record_time_ = 15*1000;
                m_bRecordOnce = true;
                recorder_->setMaxDuration(record_time_);
            }else{
                recorder_->setMaxDuration(param_.cycle_time);
                db_error("main cam rec:set max duration cycle_time %d",param_.cycle_time);
            }
        }
		else if (param_.pack_strategy == PACK_BY_FILESIZE)
		{
            recorder_->setMaxDuration(record_time_ * 1000);
        }
		else
		{
            db_error("should set pack strategy");
        }
    }

	//add by jason
//	recorder_->setMuteMode(false);
	int status = StorageManager::GetInstance()->GetStorageStatus();
	if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
	{
		if(param_.file.empty()){
		    switch(param_.recorder_mode)
            {
                        case 0:
                            v_file_ = new MediaFile(camera_->GetCameraID(), VIDEO_A);
                            file_allocate_size_ = calc_record_size_by_time(param_.bitrate, param_.cycle_time);
                            last_file_allocate_size_ = file_allocate_size_;
                            break;
                        case 1:
                        {
                            v_file_ = new MediaFile(camera_->GetCameraID(), VIDEO_A_PARK);
                            file_allocate_size_ = calc_record_size_by_time(param_.bitrate, 15*1000);// 3分钟
                            last_file_allocate_size_ = file_allocate_size_;
                            break;
                        }
                        case 2:
                            break;
            }
            db_warn("m_filename is %s",v_file_->GetMediaFileName().c_str());
		}
		else
		{
			v_file_ = new MediaFile(param_.file);
			m_bRecordOnce = true;
		}
	}
	else 
	{
		db_warn("sd card has been removed,did not record any more");
		system("rm /overlay/upperdir/mnt/* -rf");
		return -1;
	}
    current_status_ = RECORDER_PREPARED;

    char *file_name = const_cast<char*>(v_file_->GetMediaFileName().c_str());
    db_msg("file name: %s", file_name);
#ifdef FALLOCATE_SUPPORT   
    db_error("record time: %u, bitrate: %d,file allocate size %lld",
                param_.cycle_time, param_.bitrate,file_allocate_size_);
    allocRecorderFile(file_name,file_allocate_size_,ReNameFile);
#endif
    param_.MF = v_file_;
    int muxer_id;
    muxer_id = recorder_->addOutputFormatAndOutputSink(
            MEDIA_FILE_FORMAT_MP4, file_name, 0, false);



    muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_MP4, muxer_id));

    main_file_cnt_++;
    file_list_.emplace(main_file_cnt_, file_name);

    // NOTE: 鍥犱负瀛愬綍鍍忔殏鏃朵笉鏀寔寤舵椂鎽勫奖妯″紡
    // 鍦ㄨ妯″紡涓嬪瓙褰曞儚鐢盡ainRecord鐢熸垚, 閫氳繃娣诲姞棰濆鐨勪竴璺痬uxer
    #if 0
    if (delay_record_flag) {
        file_name = const_cast<char*>(param_.MF->GetVideoThumbFileName().c_str());
        muxer_id = recorder_->addOutputFormatAndOutputSink(
                MEDIA_FILE_FORMAT_MP4, file_name, file_allocate_size_, false);
        muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_MP4+1, muxer_id));
    }

    if (enable_raw_data_) {
        muxer_id = recorder_->addOutputFormatAndOutputSink(
                MEDIA_FILE_FORMAT_RAW, -1, 0, true);
        muxer_id_map_.insert(make_pair(MEDIA_FILE_FORMAT_RAW, muxer_id));
    }
	#endif

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
    if (ret != NO_ERROR) {
        db_error("prepare Failed(%d)", ret);
		current_status_ = RECORDER_IDLE;
        return ret;
    }

	//add by jason
	ret = recorder_->setSwitchFileDurationPolicy(muxer_id,RecordFileDurationPolicy_MinDuration);
	if(ret != 0)
	{
		db_error("setSwitchFileDurationPolicy filed \n");
	}

    return ret;
}

void MainRecorder::StopImpactRecord()
{
    pthread_mutex_lock(&lock_);
    current_status_ = RECORDER_IMPACT_DONE;
    pthread_mutex_unlock(&lock_);
}

int MainRecorder::timeString2Time_t(std::string &timestr)
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

void MainRecorder::onInfo(EyeseeRecorder *pMr, int what, int extra)
{
    MediaFileManager *mfm = MediaFileManager::GetInstance();
    StorageManager *sm = StorageManager::GetInstance();

    switch(what) {
        case EyeseeRecorder::MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD: {
                Main_setNext_fd_flag = true;
				if( m_bRecordOnce )
				{
					break;
				}
                db_warn("set next fd");
                char reNameFile[128] = "";
                this->CheckStorageThresholdPercent(89);
                if (this->CheckStorage(param_.recorder_mode, reNameFile) < 0) {
#ifdef SUB_RECORD_SUPPORT
                    this->Notify(MSG_NEED_STOP_SUB_RECORD);
#endif
                    //如果所有文件都被加锁则需要提示卡满，停掉录像
                    db_warn("Be Careful every clc record is all locked there is no Normal record video ");
                    StopRecord();
                    break;
                }
                int nMediatype = VIDEO_A;
			    if( param_.recorder_mode == 0){
				    nMediatype = VIDEO_A;
			    }
		    	else
                {
    				nMediatype = VIDEO_A_PARK;
                }
				if(m_SosMuxerId == extra)
				{
					break;
				}
#if 0
                if (delay_record_flag == true && extra == muxer_id_map_[MEDIA_FILE_FORMAT_TS+1]) {
                    string file_name;
                    sub_file_cnt_++;
                    auto it = file_list_.find(sub_file_cnt_);
                    if (it == file_list_.end()) {
                        MediaFile file(camera_->GetCameraID(), VIDEO_A);
                        file_name = file.GetMediaFileName();
                        file_list_.emplace(sub_file_cnt_, file_name);
                    } else {
                        file_name = file_list_[sub_file_cnt_];
                    }

					string::size_type rc = file_name.rfind(".");
					if( rc == string::npos)
					{
						db_warn("invalid fileName:%s",file_name.c_str());
						break;
					}
                    file_name.insert(rc, "_ths");

                    db_warn("onInfo: SetNextFile name is %s", file_name.c_str());
                    this->SetNextFile(file_name, file_allocate_size_, extra);
                } else {
#endif
					pthread_mutex_lock(&lock_);

                    delete n_file_;
                    n_file_ = NULL;
                    
                    string file_name;
                    main_file_cnt_++;
                    auto it = file_list_.find(main_file_cnt_);
                    if (it == file_list_.end()) {
                        n_file_  = new MediaFile(camera_->GetCameraID(), nMediatype);
                        file_name = n_file_->GetMediaFileName();
                        file_list_.emplace(main_file_cnt_, file_name);
                    } else {
                        file_name = file_list_[main_file_cnt_];
                        n_file_ = new MediaFile(file_name);
                    }

                    db_warn("new video file: %s current_status_:%d", file_name.c_str(), current_status_);
					if(current_status_ == RECORDER_RECORDING){
#ifdef FALLOCATE_SUPPORT
					    char *filename = const_cast<char*>(file_name.c_str());
					    db_error("file_allocate_size_ %lld",file_allocate_size_);
					    this->allocRecorderFile(filename,file_allocate_size_,reNameFile);
#endif
					    this->SetNextFile(file_name, 0, extra);
						param_.MF = n_file_;
					}else{
                        db_msg("[debug_jaosn]:mainRecord MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD#####");
						delete n_file_;
						n_file_ = NULL;
					}
	                 pthread_mutex_unlock(&lock_);
              //  }
            }
            break;
        case EyeseeRecorder::MEDIA_RECORDER_INFO_RECORD_FILE_DONE: {
                db_warn("recording finished");
				if(m_SosMuxerId == extra)
				{
					if( m_SosFile != NULL)
					{
						takeSosThumbPic();
						pthread_mutex_lock(&file_lock_);
						recorder_->removeOutputFormatAndOutputSink(m_SosMuxerId);
						mfm->AddFile(*m_SosFile);
						std::string file_name = m_SosFile->GetMediaFileName();
						std::string really_name;
						m_SosFile->getFileReallyName(really_name);
						m_SosFile = NULL;
						m_SosMuxerId = -1;
						pthread_mutex_unlock(&file_lock_);
					}
					m_bSosStatus = SOSEvent_Stop;
					break;
				}
		        db_msg("[debug_zhb]---> ready to send file done msg to update record time");
		        if(!m_bRecordOnce){
//		            db_error("notify MSG_RECORD_FILE_DONE");
		            this->Notify(MSG_RECORD_FILE_DONE);
		        }
                //if (delay_record_flag == true && extra == muxer_id_map_[MEDIA_FILE_FORMAT+1])
                //    break;
                pthread_mutex_lock(&file_lock_);
                    if (v_file_ == NULL)
					{
                        db_warn("v_file_ is NULL, will not add to database");
                    }
					else
					{
                        if(param_.lock_file_flag && param_.recorder_mode == 0){
                            db_warn("[debug_jaosn]:current is lock file,param_.lock_file_flag = %d",param_.lock_file_flag);
                            mfm->AddFile(*v_file_,0,file_allocate_size_,true);
                            param_.lock_file_flag = false;
                            std::string current_file_name = v_file_->GetMediaFileName();
                            std::string lock_file_name = mfm->getlockFileName();
                            std::string mv_buffer_sting = "mv -f " + current_file_name + " " + lock_file_name;
                         //   db_warn("mv_buffer_string is :%s",mv_buffer_sting.c_str());
                            //move video ths pic file
                            std::string current_thumb_file_name = v_file_->GetVideoThumbPicFileName();
						 	HideFile(current_thumb_file_name.c_str(),1);
                            std::string lock_thumb_file_name = mfm->getlocThumbPickFileName();
                            std::string mv_thumb_buffer_sting = "mv -f " + current_thumb_file_name + " " + lock_thumb_file_name;
                          //  db_warn("mv_thumb_buffer_sting is :%s",mv_thumb_buffer_sting.c_str());
                            char reNameFile[128] = "";
                            if(this->CheckStorage(2, reNameFile) < 0)
                            {
                                db_warn("sd card is full or some this is wrong");
                            }
                            system(mv_buffer_sting.c_str());
                            system(mv_thumb_buffer_sting.c_str());
                            //mv file to event dir
                        }else{
                        	db_error("add normal file %s",(*v_file_).GetMediaFileName().c_str());
                            mfm->AddFile(*v_file_,0,file_allocate_size_,false);
                        }

                        #if 0
						if(m_bRecordOnce)
						{
							EventReportMsg event;
                            event.err_no = 0;
							event.event_type = EVENT_REQ_POST_LITEVIDEO;
							event.file_name = v_file_->GetMediaFileName();
							AdapterLayer::GetInstance()->notifyMessage(event);
							record_time_ = 0;
						}
                        #endif
		            }
                    pthread_mutex_unlock(&file_lock_);

                    pthread_mutex_lock(&lock_);
                    if (current_status_ != RECORDER_STOPPED && current_status_ != RECORDER_IDLE) {
                        pthread_mutex_unlock(&lock_);
                        // 寰幆褰曞儚閫氱煡瀛愮爜娴佸垏鎹㈡枃浠讹紝 骞剁敓鎴愮缉鐣ュ浘
#ifdef SUB_RECORD_SUPPORT
                        this->Notify(MSG_NEED_SWITCH_SUB_FILE);
#endif
						if( !m_bRecordOnce ){
                            if(Main_setNext_fd_flag){
                                Main_setNext_fd_flag = false;
							    this->Notify(MSG_TAKE_THUMB_VIDEO, 0, GetID());
                            }
						}
                    } else {
                        //NOTE: 鎵嬪姩鍋滄褰曞儚锛屼笉闇�瑕佸湪缁撴潫鏃剁敓鎴愮缉鐣ュ浘
                    }
                    pthread_mutex_unlock(&lock_);

                pthread_mutex_lock(&file_lock_);
                delete v_file_;
                v_file_ = NULL;
                pthread_mutex_unlock(&file_lock_);
				if( m_bRecordOnce )
				{
//					m_bRecordOnce = false;
					db_error("current_status_ %d",current_status_);
					StopRecord();
				}

                pthread_mutex_lock(&lock_);
                // is on circulate recording
                if (n_file_ != NULL) {
					 db_warn("[debbug_jaosn]:remove invalid empty file: %s, current_status_ = %d",
                                n_file_->GetMediaFileName().c_str(),current_status_);
                    if ((current_status_ == RECORDER_STOPPED) || (current_status_ == RECORDER_IDLE)) {
                        db_warn("[debbug_jaosn]:remove invalid empty file: %s",
                                n_file_->GetMediaFileName().c_str());
                        sm->RemoveFile(n_file_->GetMediaFileName().c_str());
                        //sm->RemoveFile(n_file_->GetVideoThumbPicFileName().c_str());
                        current_status_ = RECORDER_IDLE;
                    } else {
                        v_file_ = n_file_;
                    }
                    n_file_ = NULL;
                } else {
                	db_warn("[debug_jaosn]: the file is NULL");
                    current_status_ = RECORDER_IDLE;
                }
                pthread_mutex_unlock(&lock_);

#ifdef WRITE_RAM_H264_FILE
                sync();
                fclose(file_);
#endif
                if(m_bRecordOnce == true)
                {
                    db_error("notify MSG_RECORD_FILE_DONE");
                    m_bRecordOnce = false;
                    this->Notify(MSG_RECORD_FILE_DONE);
                }
                db_msg("status: [%d]", current_status_);
            }
            break;
#ifdef ADJUST_BITRATE
        case EyeseeRecorder::MEDIA_RECORDER_INFO_VENC_BUFFER_USAGE:
           {
               if(current_status_ != RECORDER_RECORDING){
                   db_error("current status is not recording!!");
                   break;
               }
               EyeseeRecorder::BufferState bufstate;
               memset(&bufstate, 0, sizeof(EyeseeRecorder::BufferState));
               EyeseeRecorder::VEncBitRateControlAttr stRcAttr;
               memset(&stRcAttr, 0, sizeof(EyeseeRecorder::VEncBitRateControlAttr));
               int ret = recorder_->GetBufferState(bufstate);
               if(ret != 0){
                   db_error("GetBuffer state error:%d",ret);
                   return;
               }
               //db_error("\nmValidSizePercent:%d ,mValidSize:%d ,mTotalSize:%d",bufstate.mValidSizePercent,bufstate.mValidSize,bufstate.mTotalSize);
               recorder_->getVEncBitRateControlAttr(stRcAttr);
               if(ret != NO_ERROR){
                   db_error("get buffer state error:%d",ret);
                   break;
               }
               //db_error( " ----curbitrate:%u ,bitrate:%u ,mBitRate:%u",curbitrate,param.bitrate,stRcAttr.mAttrH264Cbr.mBitRate);
               if(bufstate.mValidSizePercent >= 50){
                   db_info("buffer size mValidSizePercent %d is exceed 50,will adjust bitrate.",bufstate.mValidSizePercent);
                   db_info("enc_type is %d bitrate is %d minQp is %d",
                           this->enc_type_, this->dynamic_bitrate_, this->dynamic_minQp_);
                   int tmp = this->dynamic_bitrate_ - DYNAMIC_BITRATE_DECREASE_ADJUSTMENT_INTERVAL;
                   if(tmp >= MINI_BITRATE_VALUE){
                       this->dynamic_bitrate_ = tmp;
                       if(this->enc_type_ == PT_H264){
                           stRcAttr.mAttrH264Cbr.mBitRate = this->dynamic_bitrate_;
//                           stRcAttr.mAttrH264Cbr.mMinQp = this->dynamic_minQp_;
                       }else if(this->enc_type_ == PT_H265){
                           stRcAttr.mAttrH265Cbr.mBitRate = this->dynamic_bitrate_;
//                           stRcAttr.mAttrH265Cbr.mMinQp = this->dynamic_minQp_;
                       }
                       recorder_->setVEncBitRateControlAttr(stRcAttr);
                       db_info("low write sd speed,adjust bitrate %d", this->dynamic_bitrate_);
                   }else{
                       db_info("adjust bitrate %d less than mini bitrate value,cat not adjust bitrate!!",this->dynamic_bitrate_);
                   }
               } else if(bufstate.mValidSizePercent <= 30){
                   //db_warn("buffer size mValidSizePercent %d is less 30,will adjust bitrate.",bufstate.mValidSizePercent);
                   //db_warn("enc_type is %d bitrate is %d minQp is %d",
                   //  this->enc_type_, this->dynamic_bitrate_, this->dynamic_minQp_);
                   int tmp = this->dynamic_bitrate_ + DYNAMIC_BITRATE_INCREASE_ADJUSTMENT_INTERVAL;
                   if(tmp >= this->default_bitrate_){
                      // db_error("adjust bitrate %d more than default bitrate,cat not adjust bitrate!!",this->dynamic_bitrate_);
                   }else{
                       this->dynamic_bitrate_ = tmp;
                       if(this->dynamic_bitrate_ <= 30*1024*1024)
                       {
                            db_info("[debug_jaosn]: be careful increase the bitRate to 30Mbit");
                            this->dynamic_bitrate_ = 30*1024*1024;
                       }					   
                       if(this->enc_type_ == PT_H264){
                           stRcAttr.mAttrH264Cbr.mBitRate = this->dynamic_bitrate_;
//                         stRcAttr.mAttrH264Cbr.mMinQp = this->dynamic_minQp_;
                       }else if(this->enc_type_ == PT_H265){
                           stRcAttr.mAttrH265Cbr.mBitRate = this->dynamic_bitrate_;
//                         stRcAttr.mAttrH265Cbr.mMinQp = this->dynamic_minQp_;
                       }
					   
                       recorder_->setVEncBitRateControlAttr(stRcAttr);
                       db_info("mValidSizePercent less 30,increase bitrate %d",this->dynamic_bitrate_);
                   }
               }
           }
           break;
#endif
        default:
            break;
    }
}


int MainRecorder::takeSosThumbPic()
{
	m_ThumbRetriever = new EyeseeThumbRetriever();
	if( m_ThumbRetriever != NULL )
	{
		m_ThumbRetriever->setDataSource(const_cast<char*>(m_SosFile->GetMediaFileName().c_str()));
		std::string FileName = m_SosFile->GetMediaFileName();
		string::size_type rc = FileName.rfind(".");
		if(rc == string::npos)
		{
			db_warn("invalid fileName:%s",FileName.c_str());
			delete m_ThumbRetriever;
			m_ThumbRetriever = NULL;
			return -1;
		}
		std::string PicturePath = FileName.substr(0, rc)+ "_ths.jpg";
		
		std::shared_ptr<CMediaMemory> pJpegThumb = m_ThumbRetriever->getJpegAtTime(0, 640, 360);//param_.video_size.width, param_.video_size.height);
		FILE *fp = fopen(PicturePath.c_str(), "wb");
		if( fp != NULL )
		{
			if( pJpegThumb != NULL &&  pJpegThumb->getPointer() != NULL)
				fwrite(pJpegThumb->getPointer(), 1, pJpegThumb->getSize(), fp);
			fclose(fp);
		}
		m_ThumbRetriever->reset();
		delete m_ThumbRetriever;
		m_ThumbRetriever = NULL;
	}	
	return 0;
}

void MainRecorder::SetNextFile(const string &file, int64_t filesize, int muxer_id)
{
    // if(muxer_id_map_[MEDIA_FILE_FORMAT_MP4] != muxer_id) {
        // db_error("fatal error! setNextFile muxerId[%d]!=[%d]", muxer_id_map_[MEDIA_FILE_FORMAT_MP4], muxer_id);
    // }
    recorder_->setOutputFileSync(const_cast<char*>(file.c_str()), filesize, muxer_id);
}

int MainRecorder::StopRecord()
{

	if( current_status_ == RECORDER_IDLE)
		return 0;

    int ret;
    db_error("stop rec...");
    int timeout = 0;
    ret = Recorder::StopRecord();
    while (ret == 0) {
        pthread_mutex_lock(&lock_);
        if (current_status_ == RECORDER_IDLE || current_status_ == RECORDER_STOPPED) {
            pthread_mutex_unlock(&lock_);
            break;
        } else if (timeout == 4) { // 5 seconds
            timeout = 0;
            pthread_mutex_unlock(&lock_);
            db_error("status: [%d], wait record file done timeout", current_status_);
            break;
        } else {
            db_error("status: [%d], wait record file done", current_status_);
            pthread_mutex_unlock(&lock_);
            usleep(500 * 1000);
        }
        pthread_mutex_unlock(&lock_);
        ++timeout;
    }
    dynamic_bitrate_ = 0;
    dynamic_minQp_ = 0;
    TimeTest *time_test = new TimeTest("sync record file");
    sync();
    delete time_test;

    pthread_mutex_lock(&lock_);
    current_status_ = RECORDER_IDLE;
    pthread_mutex_unlock(&lock_);

	if(GetID() == 0)
	    this->Notify(MSG_RECORD_STOP);

    return  ret;
}

int MainRecorder::addOutputFormatAndOutputSink()
{
    return 0;
}
int MainRecorder::removeOutputFormatAndOutputSink()
{
    return 0;
}

int MainRecorder::setSosHappened(SosEventType p_EventType)
{
    db_warn(" MainRecorder setSosHappened run here but nothing to do");
    #if 0
	if( m_bSosStatus == p_EventType )
		return 0;

	if (CheckStorage()<0)
		return -1;

	if( p_EventType )
	{
		if( startSosRecord(p_EventType) < 0 )
			return -1;
	}
	else
		stopSosRecord();

	m_bSosStatus = p_EventType;
#endif
	return 0;
}

int MainRecorder::startSosRecord(SosEventType p_EventType)
{
#if 0
    if( current_status_ != RECORDER_RECORDING )
	{
		db_warn("main recorder is not recording yet,so could not start sos record");
		return -1;
	}

	m_SosFile = new MediaFile(camera_->GetCameraID(), VIDEO_A_SOS);
	if( m_SosFile == NULL )
	{
		db_error("generate front sos file failed\n");
		return -1;
	}

    char *file_name = const_cast<char*>(m_SosFile->GetMediaFileName().c_str());
    db_msg("file name: %s", file_name);

	SinkParam sinkParam;
	memset(&sinkParam, 0, sizeof(SinkParam));
	sinkParam.mOutputFd 		= -1;
	sinkParam.mOutputPath		= file_name;
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

int MainRecorder::stopSosRecord()
{
	return 0;
}

void MainRecorder::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
	return ;
}

int MainRecorder::GetSoSRecorderFileName(char *p_FileName)
{
	if(m_SosFile != NULL)
	{	
		strncpy(p_FileName, m_SosFile->GetMediaFileName().c_str(), sizeof(char)*64);
	}

	return 0;
}
