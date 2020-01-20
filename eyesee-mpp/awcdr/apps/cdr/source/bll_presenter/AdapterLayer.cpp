/*
*版权声明:暂无
*文件名称:AdapterLayer.cpp
*创建者:陈振华
*创建日期:2018-5-11
*文件描述:本文件主要转发滴滴平台传过来的指令需求
*历史记录:无
*/

#include "AdapterLayer.h"
#include "fileLockManager.h"
#include <sys/timeb.h>
#include "common/setting_menu_id.h"
#include "common/app_log.h"

#include "camRecCtrl.h"
#include "device_model/storage_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/system/gsensor_manager.h"
#include "device_model/system/led.h"
#include "device_model/partitionManager.h"
#include "device_model/menu_config_lua.h"
#include "dd_serv/dd_platform.h"
#include "dd_serv/common_define.h"
#include "dd_serv/globalInfo.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "AdapterLayer.cpp"
#endif


using namespace std;

static AdapterLayer *m_AdapterLayer = NULL;
static pthread_mutex_t m_staticMutex;
static int tfMountedStatus = -1;

#ifdef DEBUG_PART
extern queue<string> logRecordQue;
#endif

static const std::string base64_chars = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

AdapterLayer::AdapterLayer()
{
}

AdapterLayer::~AdapterLayer()
{

}

AdapterLayer * AdapterLayer::GetInstance()
{
	pthread_mutex_lock(&m_staticMutex);

	if( m_AdapterLayer != NULL )
	{
		pthread_mutex_unlock(&m_staticMutex);
		return m_AdapterLayer;
	}

	m_AdapterLayer = new AdapterLayer();
	if( m_AdapterLayer != NULL)
	{
		pthread_mutex_unlock(&m_staticMutex);
		return m_AdapterLayer;
	}

	pthread_mutex_unlock(&m_staticMutex);

	return NULL;
}

void AdapterLayer::getDeviceInfo(Device_info &p_DevInfo)
{
    EventManager *m_EventManager = EventManager::GetInstance();
    ::LuaConfig config;
    config.LoadFromFile("/data/menu_config.lua");
    p_DevInfo.imei = DD_GLOBALINFO::GetInstance()->getIMEI();
    p_DevInfo.simID = m_EventManager->getSimId();
    p_DevInfo.fwversion = config.GetStringValue("menu.device.sysversion.version");
}

int AdapterLayer::getDeviceInfo(RemoteDeviceStatusInfo &p_DevInfo)
{
	StorageManager *m_StorageManager = StorageManager::GetInstance();
	MenuConfigLua *m_menuconfiglua=MenuConfigLua::GetInstance();

	EventManager *m_EventManager = EventManager::GetInstance();
	uint32_t free = 0, total = 0, backcamCapacity = 0;
	char str[30] = {0};
	unsigned int t_;
	::LuaConfig config;
        config.LoadFromFile("/data/menu_config.lua");
	p_DevInfo.imei = DD_GLOBALINFO::GetInstance()->getIMEI();
    if(p_DevInfo.equipment_action != EVENT_TRIGGER_DEV_STATUS &&
            p_DevInfo.equipment_action != EVENT_SD_FORMAT_FINISH)  {
	    p_DevInfo.equipment_action = m_EventManager->mAccStatus;
    }
	p_DevInfo.systemVersion = config.GetStringValue("menu.device.sysversion.version");
	p_DevInfo.versionCode = 1; //test

	//----- sdcard_capacity start ------//
	m_StorageManager->GetStorageCapacity(&free, &total);
	memset(str, 0, sizeof(str));
	snprintf(str, sizeof(str),"%1.1f/%1.1f",(float)free, (float)total);
	p_DevInfo.sdcard_capacity = str;
	//----- sdcard_capacity end ------//

    p_DevInfo.bind_flag = DD_GLOBALINFO::GetInstance()->getBindflag();
	p_DevInfo.net_state = m_EventManager->m_NetType;
	p_DevInfo.gps_signal = m_EventManager->mGpsStatus;
	getCameraResolution(0, p_DevInfo.before_camera_resolution);
	getCameraResolution(1, p_DevInfo.after_camera_resolution);
	p_DevInfo.microphone = m_menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_VOLUME); 
	p_DevInfo.hardware_state = 0; //test 未提供
	p_DevInfo.serialcode = ""; //test
	p_DevInfo.latitude = m_EventManager->mLocationInfo.latitude;
	p_DevInfo.longitude = m_EventManager->mLocationInfo.longitude;	
	p_DevInfo.timestamp  = std::to_string(time(0));
	backcamCapacity = m_StorageManager->getLockFileCapacity();
    if(total == 0) {
	    p_DevInfo.lock_file_capacity = 0.0;
    }
    else {
	    p_DevInfo.lock_file_capacity = (float)(backcamCapacity/(float)total);
    }

	return 0;
}

int AdapterLayer::getUnnormalDeviceInfo(RemoteDeviceAbnormalInfo & p_DevInfo)
{
	EventManager *m_EventManager = EventManager::GetInstance();
	char str[30] = {0};
	unsigned int t_;
	::LuaConfig config;
        config.LoadFromFile("/data/menu_config.lua");
	p_DevInfo.imei = m_EventManager->imei; 
	FileLockManager::GetInstance()->getOrderId(p_DevInfo.order_id);
	p_DevInfo.systemVersion = config.GetStringValue("menu.device.sysversion.version");
	p_DevInfo.versionCode = 1; //test
	p_DevInfo.dismount = StorageManager::GetInstance()->GetStorageStatus(); 
	p_DevInfo.tf_state = m_EventManager->CheckSDCard_Cid();
//	p_DevInfo.g_sensor = GsensorManager::GetInstance()->readSensibility();
	char FileName[64] = {0};
	Recorder *rec = CamRecCtrl::GetInstance()->GetRecorder(0, 0);
	if( rec != NULL )
		rec->GetSoSRecorderFileName(FileName);
	p_DevInfo.video0_name = FileName;

	bzero(FileName, sizeof(FileName));
	rec = CamRecCtrl::GetInstance()->GetRecorder(1, 2);
		rec->GetSoSRecorderFileName(FileName);
	p_DevInfo.video1_name = FileName;

	p_DevInfo.latitude = m_EventManager->mLocationInfo.latitude;
	p_DevInfo.longitude = m_EventManager->mLocationInfo.longitude;
	p_DevInfo.timestamp  = std::to_string(time(0));

	return 0;
}

int AdapterLayer::setRollingOrder(const string p_OrderId,int p_Status)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setOrderId(p_OrderId, p_Status);
}

int AdapterLayer::setTfMounted(int p_Status)
{
    tfMountedStatus = p_Status;
    db_msg("tfMountedStatus:%d\n", tfMountedStatus);
	return 0;
}

int AdapterLayer::getTfMounted(void)
{
    int storageStatus = StorageManager::GetInstance()->GetStorageStatus();
    db_msg("getTfMounted tfMountedStatus:%d GetStorageStatus:%d\n", tfMountedStatus,storageStatus);
    if(tfMountedStatus == -1) {
        if(storageStatus == MOUNTED || storageStatus == STORAGE_LOOP_COVERAGE) {
            return 1;
        }
        else {
            return 0;
        }
    }
    return tfMountedStatus;
}

int AdapterLayer::getRollingOrderId(string &p_OrderId)
{
	return FileLockManager::GetInstance()->getOrderId(p_OrderId);
}

int AdapterLayer::getRollingOrderIdByName(const std::string &p_FileName, std::string &p_OrderId)
{
	return MediaFileManager::GetInstance()->getOrderIdByName(p_FileName,p_OrderId);
}

int AdapterLayer::setDevAttrEx(int p_gSensorValue)
{
    #if 0
	if((p_gSensorValue >= 0) && (p_gSensorValue <= 3))
    //	return GsensorManager::GetInstance()->writeSensibility(p_gSensorValue);
	else
		db_warn("invalid gsensor value %d",p_gSensorValue);
    #endif
	return -1;
}

int AdapterLayer::setDevAttr(int p_FrontRecResolutionIndex,int p_BackRecResolutionIndex,int p_gSensorValue)
{
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}

	Recorder *rec = m_CamRecCtrl->GetRecorder(0, 1);
	if( rec != NULL )
		rec->SetRecordEncodeSize(p_FrontRecResolutionIndex);

	rec = m_CamRecCtrl->GetRecorder(1, 2);
	if( rec != NULL )
		rec->SetRecordEncodeSize(p_BackRecResolutionIndex);

	if((p_gSensorValue >= 0) && (p_gSensorValue <= 3))
	{
		MenuConfigLua::GetInstance()->SetMenuIndexConfig(SETTING_EMER_RECORD_SENSITIVITY,abs(p_gSensorValue-3));
		//GsensorManager::GetInstance()->writeSensibility(p_gSensorValue);
	}

	return 0;
}

int AdapterLayer::setShootAttr(int p_FrontCamEnable,int p_BackCamEnable,int p_AudioType)
{
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();

	if( NULL == m_CamRecCtrl )
	{
		db_error("m_CamRecCtrl is null\n");
		return -1;
	}

    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }

	for(int CamId =0; CamId < 2; CamId++ )
	{
		Recorder *rec = NULL;
		if( 0 == CamId )
		{
			rec = m_CamRecCtrl->GetRecorder(CamId, 1);
			if(rec == NULL )
				continue;

			if( p_FrontCamEnable == 0)
			{
				if( rec->RecorderIsBusy() )
				{
					db_warn("front camera chn 1 is busy now, stop first, then start record");
					rec->StopRecord();
					sleep(2);
				}

				rec->SetRemoteAudioRecordType(p_AudioType);
				rec->StartRecord();
			}
			else if( p_FrontCamEnable == 1) 
			{
				rec->StopRecord();
			}
		}
		else
		{
			rec = m_CamRecCtrl->GetRecorder(CamId, 2);
			if( rec == NULL )
				continue;

			if( p_BackCamEnable == 0)
			{
				if( rec->RecorderIsBusy() )
				{
					db_warn("back camera chn 2 is busy now, stop first, then start record");
					rec->StopRecord();
					sleep(2);
				}

				rec->SetRemoteAudioRecordType(p_AudioType);
				if( rec->StartRecord(false) < 0 )
					return -1;

				if(p_AudioType != 2)//dont turn on led,when only record audio
					LedControl::get()->EnableLed(LedControl::DEV_LED, true, LedControl::LONG_LIGHT);
				else
					LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
			}
			else if(p_BackCamEnable == 1 )
			{
				rec->StopRecord();
				LedControl::get()->EnableLed(LedControl::DEV_LED, false, LedControl::LONG_LIGHT);
			}			
		}
	}

	return 0;
}

int AdapterLayer::setSlientPhoto(int p_RecordType,const string p_picName)
{
    int ret = -1;
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	EventManager *m_EventManager = EventManager::GetInstance();
	Recorder *loop_rec = NULL,*short_rec = NULL;
	bool ir_flag = false;
	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}

    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }

	Camera *cam = NULL;
	loop_rec = m_CamRecCtrl->GetRecorder(1, 2);
		if( loop_rec == NULL ){
		db_error("loop_rec is NULL!!");
		return -1;
	}

	short_rec = m_CamRecCtrl->GetRecorder(1, 3);
	if( short_rec == NULL ){
			db_error("short_rec is NULL!!");
			return -1;
	}
	if((!loop_rec->RecorderIsBusy()) && (!short_rec->RecorderIsBusy())) {
		db_error("recorder is close,need set  ir led");
		m_EventManager->setIrLedBrightness(EventManager::IRLedOn);
		ir_flag = true;
	} else {
		db_error("recorder is open,no need set ir led");
	}
	switch(p_RecordType)
	{
		case 0:
		{
			cam = m_CamRecCtrl->GetCamera(1);
			if(cam != NULL)
			{
				FileLockManager::GetInstance()->setSlientPicName(p_picName);
				cam->SetPicFileName(p_picName);
				ret = cam->TakePicture(2);
                if(ret < 0) {
                    db_error("TakePicture failed!");
                    return ret;
                }
				if(ir_flag) {
					db_error("cam 1 take pic finsh,close ir led");
					m_EventManager->setIrLedBrightness(EventManager::IRLedOff);
				}
				return 0;
			}
			break;
		}
		case 1:
		{
			cam = m_CamRecCtrl->GetCamera(0);
			if(cam != NULL)
			{
				FileLockManager::GetInstance()->setSlientPicName(p_picName);
				cam->SetPicFileName(p_picName);
				ret = cam->TakePicture(0);
                if(ret < 0) {
                    db_error("TakePicture failed!");
                    return ret;
                }
				return 0;
			}
			break;
		}
		default:
			db_warn("RecordType:%d is invalid",p_RecordType);
			return -1;
	}

	db_warn("camera:%d is not ready now",p_RecordType);

	return -1;
}

int AdapterLayer::getSlientPic(FilePushInfo & p_Pic)
{
	return FileLockManager::GetInstance()->getSlientPic(p_Pic);
}

int AdapterLayer::getRecordFile(FilePushInfo &p_fileInfo)
{
	return FileLockManager::GetInstance()->getRecordFile(p_fileInfo);
}

int AdapterLayer::setTriggerFaceOneTime(int frame_cnt)
{
	return 0;
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	Camera *cam = m_CamRecCtrl->GetCamera(1);
	if( cam != NULL )
	{
		int ret;
		if(frame_cnt<1 || frame_cnt >30)
		{
			db_msg("invalid frame_cnt %d",frame_cnt);
			frame_cnt = 30;
		}
		ret = cam->getFaceStatus();
		if(!ret)
		{
			cam->StartFaceDetect(frame_cnt);
			return 0;
		}
		else
		{
			db_warn("StartFaceDetect busy");
			return -1;
		}
	}

	return -1;
}

int AdapterLayer::getFaceDetectResult(FilePushInfo *p_fileInfo)
{
	return 0;
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}

	Camera *cam = m_CamRecCtrl->GetCamera(1);
	if( cam != NULL )
	{
		int ret;
		ret = cam->getFaceResult(p_fileInfo);
		return ret;
	}	

	return -1;
}

int AdapterLayer::setTriggerRecord(int p_CamSelect, int p_AudioSelect, int p_CamResolution, int p_RecTime, const string p_FileName)
{
    int ret = -1;
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();

	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }

	Recorder *rec;
	if( 0 == p_CamSelect )
		rec = m_CamRecCtrl->GetRecorder(1, 3);
	else
		rec = m_CamRecCtrl->GetRecorder(0, 1);

	if(rec != NULL )
	{
		if( !rec->RecorderIsBusy() )
		{
			FileLockManager::GetInstance()->setRecordFileName(p_FileName);
            rec->SetRemoteAudioRecordType(p_AudioSelect);   //0:on audio, 1:off audio
			rec->SetRecordTime(p_RecTime);
			rec->SetFileName(p_FileName);
			if( p_CamSelect == 0)
				ret = rec->StartRecord(false);
			else
				ret = rec->StartRecord();
            if(ret < 0) {
                db_error("StartRecord failed!");
                return ret;
            }
			return 0;
		}
		else
		{
			db_warn("cam[%d] rec is busy now",p_CamSelect);
			return -1;
		}
	}

	db_warn("cam[%d] rec is null",p_CamSelect);
	return -1;
}

int AdapterLayer::setLockFileByTime(const string p_StartTime,const string p_StopTime,const string p_LockTime, int p_CamId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setLockFileByTime(p_StartTime, p_StopTime, p_LockTime, p_CamId, "");
}

int AdapterLayer::setLockFileByTimeEx(const string p_StartTime,const string p_StopTime,const string p_LockTime, int p_CamId, std::string p_OrderId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setLockFileByTime(p_StartTime, p_StopTime, p_LockTime, p_CamId, p_OrderId);
}

int AdapterLayer::getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo)
{
	return FileLockManager::GetInstance()->getLockFileByTimeResult(p_fileLockInfo);
}

int AdapterLayer::setUnLockFileByTime(const string p_StartTime,const string p_StopTime, int p_CamId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setUnLockFileByTime(p_StartTime, p_StopTime, p_CamId, "");
}

int AdapterLayer::setUnLockFileByTimeEx(const string p_StartTime,const string p_StopTime, int p_CamId, std::string p_OrderId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setUnLockFileByTime(p_StartTime, p_StopTime, p_CamId, p_OrderId);
}

int AdapterLayer::getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo)
{
	return FileLockManager::GetInstance()->getUnLockFileByTimeResult(p_fileUnLockInfo);
}

int AdapterLayer::setLockFileByName(const string p_FileName,const string p_LockTime, int p_CamId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setLockFileByName(p_FileName, p_LockTime, p_CamId);
}

int AdapterLayer::setUnLockFileByName(const string p_FileName, int p_CamId)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setUnLockFileByName(p_FileName, p_CamId);
}

int AdapterLayer::setRecordAudio(const string p_FileName,int p_RecTime)
{
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();

	if( NULL == m_CamRecCtrl )
	{
		db_warn("m_CamRecCtrl is null\n");
		return -1;
	}
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }

	Recorder *rec = m_CamRecCtrl->GetRecorder(1, 3);
	if(rec != NULL )
	{
		FileLockManager::GetInstance()->setRecordFileName(p_FileName);
		rec->SetRemoteAudioRecordType(2);
		rec->SetRecordTime(p_RecTime);
		rec->SetFileName(p_FileName);
		rec->StartRecord(false);

		return 0;
	}

	return -1;
}

int AdapterLayer::getRecordAudioResult(FilePushInfo &p_fileInfo)
{
	return getRecordFile(p_fileInfo);
}

int AdapterLayer::formatSdCard()
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return StorageManager::GetInstance()->SetFormat();
}

int AdapterLayer::setDrivingData(const TrafficDataMsg *log)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setLogList(log);
}

int AdapterLayer::getDrivingDataResult(FilePushInfo &p_fileInfo)
{
	return FileLockManager::GetInstance()->getLogList(p_fileInfo);
}

int AdapterLayer::getBasicHWinfo(RemoteBasicHWinfo * p_HWinfo)
{
	EventManager *m_EventManager = EventManager::GetInstance();

	LocationInfo_t LocationInfo;

	m_EventManager->getLocationInfo(LocationInfo);
	p_HWinfo->latitude = LocationInfo.latitude;
	p_HWinfo->longitude = LocationInfo.longitude;

	m_EventManager->getIMEI(p_HWinfo->imei);

	return 0;
}

int AdapterLayer::getFileKey(const string p_FileName, string &p_key)
{
	return FileLockManager::GetInstance()->getFileKey(p_FileName, p_key);
}

int AdapterLayer::pushDevStatusInfo(RemoteDeviceStatusInfo *p_devInfo)
{
#ifdef DDSERVER_SUPPORT
	dd_Bg_Serv::GetInstance()->ddDevStatusInfoPost(p_devInfo);
	return 0;
#else
    return 0;
#endif
}

int AdapterLayer::DevAbnormalInfo(RemoteDeviceAbnormalInfo *p_devInfo)
{
#ifdef DDSERVER_SUPPORT
	dd_Bg_Serv::GetInstance()->ddDevAbnormalInfoPost(p_devInfo);
	return 0;
#else
    return 0;
#endif
}

int AdapterLayer::removeSlientPic(int p_CamId)
{
	return FileLockManager::GetInstance()->removeSlientPic(p_CamId);
}

int AdapterLayer::removeSlientRecordFile(int p_CamId)
{
	return FileLockManager::GetInstance()->removeSlientRecordFile(p_CamId);
}

int AdapterLayer::setFileList(std::string p_startTime, std::string p_stopTime)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setFileList(p_startTime, p_stopTime);
}

int AdapterLayer::getFileList(FilePushInfo &p_fileInfoVec)
{
	return FileLockManager::GetInstance()->getFileList(p_fileInfoVec);
}

bool AdapterLayer::is_base64(unsigned char c) 
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string AdapterLayer::base64_decode(std::string const& encoded_string)
{
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_];
        in_++;

        if (i ==4)
        {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++)
            ret += char_array_3[j];
    }

    return ret;
}

int AdapterLayer::setUserInfo(std::string p_UserName, std::string p_Password)
{
	PartitionManager::GetInstance()->sunxi_spinor_private_set("UserName", p_UserName.c_str());
	PartitionManager::GetInstance()->sunxi_spinor_private_set("PassWord", p_Password.c_str());
	PartitionManager::GetInstance()->sunxi_spinor_private_save();
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("set p_UserName:" + p_UserName + "p_Password:" + p_Password +  "\n");
    }
#endif

	return 0;
}

int AdapterLayer::setBindFlagInfo(std::string p_BindFlag)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("set p_BindFlag:" + p_BindFlag + "\n");
    }
#endif
	PartitionManager::GetInstance()->sunxi_spinor_private_sec_set((const char*)FKEY_BINDFLAG, (char*)p_BindFlag.c_str(), p_BindFlag.length());

	if(!strcmp(p_BindFlag.c_str(),"true"))
	{
		PartitionManager::GetInstance()->sunxi_spinor_private_set_flag(1);
	}else if(!strcmp(p_BindFlag.c_str(),"false"))
	{
		PartitionManager::GetInstance()->sunxi_spinor_private_set_flag(0);
	}
	return 0;
}

int AdapterLayer::getBindFlagInfo(std::string &p_BindFlag)
{
	char buf[64] = {0};
	PartitionManager::GetInstance()->sunxi_spinor_private_sec_get(FKEY_BINDFLAG, buf, sizeof(buf));
	p_BindFlag = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get p_BindFlag:" + p_BindFlag + "\n");
    }
#endif
	return 0;
}

int AdapterLayer::SaveUpgradeInfo(std::string info)
{
    int ret = -1;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("saveUpgradeInfo info:" + info + "\n");
    }
#endif
	ret = PartitionManager::GetInstance()->sunxi_spinor_private_sec_set((const char*)FKEY_UPGRADEINFO, (char*)info.c_str(), info.length());

	return ret;
}

int AdapterLayer::LoadUpgradeInfo(std::string &info)
{
	char buf[254] = {0};
	PartitionManager::GetInstance()->sunxi_spinor_private_sec_get(FKEY_UPGRADEINFO, buf, sizeof(buf));
	info = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("LoadUpgradeInfo info:" + info + "\n");
    }
#endif
	return 0;
}

int AdapterLayer::getUserInfo(std::string &p_UserName, std::string &p_Password)
{
	char buf[64] = {0};
	PartitionManager::GetInstance()->sunxi_spinor_private_get("UserName",buf,sizeof(buf));
	p_UserName = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get p_UserName:" + p_UserName + "\n");
    }
#endif
	memset(buf, 0, sizeof(buf));
	PartitionManager::GetInstance()->sunxi_spinor_private_get("PassWord",buf,sizeof(buf));
	p_Password = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get p_Password:" + p_Password + "\n");
    }
#endif

	return 0;
}

int AdapterLayer::getProductInfo(std::string p_item, std::string &p_val)
{
	int iRet = -1;
	char buf[256] = {0};
	iRet = PartitionManager::GetInstance()->sunxi_spinor_private_get(p_item.c_str(), buf, sizeof(buf));
	p_val = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get " + p_item + ":" + p_val + "\n");
    }
#endif

	return iRet;
}

int AdapterLayer::getFactoryInfo(std::string p_item, std::string &p_val)
{
	int iRet = -1;
	char buf[256] = {0};
	iRet = PartitionManager::GetInstance()->sunxi_spinor_factory_get(p_item.c_str(), buf, sizeof(buf));
	db_msg("p_item = %s buf = %s iRet = %d ",buf,p_item.c_str(),iRet);
	p_val = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get " + p_item + ":" + p_val + "\n");
    }
#endif

	return iRet;
}

int AdapterLayer::getProductInfofromflash(std::string p_item, std::string &p_val)
{
	int iRet = -1;
	char buf[256] = {0};
	iRet = PartitionManager::GetInstance()->sunxi_spinor_flash_get(p_item.c_str(), buf, sizeof(buf));
	p_val = buf;
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("get " + p_item + ":" + p_val + "\n");
    }
#endif

	return iRet;
}

int AdapterLayer::setProductInfo(std::string p_item, std::string p_val)
{
	PartitionManager::GetInstance()->sunxi_spinor_private_set(p_item.c_str(), p_val.c_str());
	PartitionManager::GetInstance()->sunxi_spinor_private_save();
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("set " + p_item + ":" + p_val + "\n");
    }
#endif

	return 0;
}


int AdapterLayer::setProductInfo(std::string p_item, std::string p_val, int iSave)
{
	PartitionManager::GetInstance()->sunxi_spinor_private_set(p_item.c_str(), p_val.c_str());
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("set " + p_item + ":" + p_val + "\n");
    }
#endif
	if(iSave)
	{	
		PartitionManager::GetInstance()->sunxi_spinor_private_save();
	}

	return 0;
}

int AdapterLayer::saveProductInfo()
{

	PartitionManager::GetInstance()->sunxi_spinor_private_save();
	return 0;
}

int AdapterLayer::clearProductInfo()
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("clear flash\n");
    }
#endif
	PartitionManager::GetInstance()->sunxi_spinor_private_clear();

	return 0;
}

int AdapterLayer::getLockFileBynameResult(LockFileInfo &p_fileLockInfo)
{
	return FileLockManager::GetInstance()->getLockFileBynameResult(p_fileLockInfo);
}

int AdapterLayer::getunLockFileBynameResult(LockFileInfo &p_fileLockInfo)
{
	return FileLockManager::GetInstance()->getunLockFileBynameResult(p_fileLockInfo);
}

int AdapterLayer::removeFile(const std::string p_FileName)
{
	return FileLockManager::GetInstance()->removeFile(p_FileName);
}

int AdapterLayer::setRecordFileName(const std::string p_FileName)
{
    if(getTfMounted() == 0) {
        db_error("tf card is not mounted ,return!!!");
        return -2;
    }
	return FileLockManager::GetInstance()->setRecordFileName(p_FileName);
}

int AdapterLayer::notifyMessage(EventReportMsg event)
{
#ifdef DDSERVER_SUPPORT
	dd_Bg_Serv::GetInstance()->ProcessEventReport(event);
	return 0;
#else
	return 0;
#endif
}

int AdapterLayer::getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId)
{	
	return FileLockManager::GetInstance()->getLockFileList(p_lockfileList,p_CamId);
}

int AdapterLayer::getFileListByRollOrder(std::vector<FilePushInfo> &p_fileInfoVec, std::string p_orderId)
{
	return 0;
}

int AdapterLayer::setQueryRollOrderId(std::string p_OrderId)
{
	return FileLockManager::GetInstance()->setQueryRollOrderId(p_OrderId);
}

int AdapterLayer::getCameraResolution(int p_CamId, std::string &p_Resolution)
{
	p_Resolution.clear();
	CamRecCtrl *m_CamRecCtrl = CamRecCtrl::GetInstance();
	if(m_CamRecCtrl!= NULL)
	{
		Size size;
		Camera *cam = m_CamRecCtrl->GetCamera(p_CamId);
		if( cam != NULL)
		{
			cam->GetVideoResolution(size);
			if( size.width == 1920 )
				p_Resolution = "1080P30FPS";
			else if( size.width == 1280 )
				p_Resolution = "720P30FPS";
		}
	}
	
	return 0;
}

int AdapterLayer::SetRemoteActionDone(int value)
{
	EventManager::GetInstance()->SetRemoteActionDone(value);
	return 0;
}

int AdapterLayer::getDeleteFileList(std::vector<LockFileInfo> &p_fileInfoVec, int p_CamId)
{
	return FileLockManager::GetInstance()->getDeleteFileList(p_fileInfoVec, p_CamId);
}
