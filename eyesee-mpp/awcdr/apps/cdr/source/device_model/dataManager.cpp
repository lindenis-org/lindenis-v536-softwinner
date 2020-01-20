#include "dataManager.h"
#include "common/app_log.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <json/json.h> 
#include "device_model/aes_ctrl.h"
#include "device_model/system/event_manager.h"
#include "bll_presenter/AdapterLayer.h"
#include "common/utils/utils.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "dataManager.cpp"
#endif
using namespace std;
static Mutex dataManager_mutex;
static DataManager *m_DataManager = NULL;

queue<GPS_INFO> GPSInfoQue;
queue<TrafficDataMsg> TrafficCMDQue; 

#define TIMEOUT_H 	(1 * 60 * 60)
#define TRAFFIC_GPS_FILE_PATH		"/mnt/extsd/.tmp/.log/gps/"
#define TRAFFIC_ROOT_PATH			"/mnt/extsd/.tmp/.log/"
#define TRAFFIC_UPLOAD_TMPFILE		"/mnt/extsd/.tmp/.log/log_tmp.tar"


#define GPSFILETYPE			"gps"
#define PICFILETYPE			"shoot"
#define ABNORMALFILETYPE	"abnormal"

#define ACTION_LOG_NAME "/mnt/extsd/.tmp/.log/Action.txt"
#define ACTION_LOG_BIN_NAME "/mnt/extsd/.tmp/.log_bin/Action.bin"

#define FACE_LOG_NAME  "/mnt/extsd/.tmp/.log/Face.txt"
#define FACE_LOG_BIN_NAME  "/mnt/extsd/.tmp/.log_bin/Face.bin"

#define FILES_LOG_NAME  "/mnt/extsd/.tmp/.log/Files.txt"
#define FILES_LOG_BIN_NAME  "/mnt/extsd/.tmp/.log_bin/Files.bin"

#define GPS_LOG_NAME   "/mnt/extsd/.tmp/.log/GPS.txt"
#define GPS_LOG_BIN_NAME   "/mnt/extsd/.tmp/.log_bin/GPS.bin"

#define USER_LOG_NAME  "/mnt/extsd/.tmp/.log/User.txt"
#define USER_LOG_BIN_NAME  "/mnt/extsd/.tmp/.log_bin/User.bin"

#define COMPRESS_LOG_NAME  "/mnt/extsd/log.tar"
#define COMPRESS_LOG_TEMP_NAME "/mnt/extsd/log_temp.tar"
#define COMPRESS_FILES_LOG_NAME "/mnt/extsd/Files.tar"
#define COMPRESS_FILES_TEMP_LOG_NAME "/mnt/extsd/Files_temp.tar"

DataManager::DataManager()
{
	m_ThreadExit = false;
	m_UploadFile = false;
	m_querybyorder_flag = false;
	m_uploadLog = false;
	m_fileKey.clear();
	m_logKey.clear();
	m_logName.clear();
	m_fileListName.clear();
	m_startTime = 0;
	m_stopTime = 0;
	m_creat_time = 0;
	m_OrderId.clear();
	m_FileMap.clear();
	mStorage = StorageManager::GetInstance();
#ifdef DDSERVER_SUPPORT
	pthread_create(&ComPressThreadId, NULL,  DataManager::CompressFileThread, this);
	pthread_create(&GPSDataOpThreadId, NULL, DataManager::GPSDataToFileThread, this);
	pthread_create(&GetTrafficCmdThreadId, NULL, DataManager::GetTrafficCmdThread, this);
#endif
}

DataManager::~DataManager()
{
	m_ThreadExit = true;
	if( ComPressThreadId > 0 )
		pthread_cancel(ComPressThreadId);

	if( GPSDataOpThreadId > 0 )
		pthread_cancel(GPSDataOpThreadId);

	if( GetTrafficCmdThreadId > 0 )
		pthread_cancel(GetTrafficCmdThreadId);
}

DataManager * DataManager::GetInstance(void)
{
	Mutex::Autolock _l(dataManager_mutex);
	if( m_DataManager != NULL )
		return m_DataManager;

	m_DataManager = new DataManager();
	if( m_DataManager != NULL)
		return m_DataManager;
	else
	{
		return NULL;
	}
}

int DataManager::WriteActionData(std::vector<ACTION_INFO> p_actionInfo)
{
#ifdef DDSERVER_SUPPORT
	int ret = 0;
	string out;
	Json::Value root;
	//Json::FastWriter writer;
	
	FILE *fp = fopen(ACTION_LOG_NAME,"w+");
	if( fp != NULL )
	{
		for(std::vector<ACTION_INFO>::iterator iter= p_actionInfo.begin(); iter != p_actionInfo.end(); iter++)
		{
			root["driving_mode"] = iter->driving_mode;
			root["after_camera_state"] = iter->after_camera_state;
			root["before_camera_state"] = iter->before_camera_state;
			root["microphone_state"] = iter->microphone_state;
			root["timestamp"] = iter->timestamp;
			out = root.toStyledString();
			fwrite("action:", 1, 7, fp);
			if(fwrite(out.c_str(), 1, out.size(), fp) <= 0)
			{
				printf("[%s] fwrite fail!\n", __FUNCTION__);
				ret = -1;
			}
		}
		fclose(fp);
	}
	else
	{
		ret = -1;
	}
	return ret;	
#else
    return 0;
#endif
}

int DataManager::WriteActionBinData(ACTION_INFO p_actionInfo)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	FILE *fp = fopen(ACTION_LOG_BIN_NAME,"a+");
	if( fp != NULL )
	{
		fwrite(&p_actionInfo, 1, sizeof(ACTION_INFO), fp);
		fclose(fp);
	}

	return 0;
#else
    return 0;
#endif
}

int DataManager::WriteFaceData(std::vector<ALL_FACE_INFO> p_faceInfo)
{
#ifdef DDSERVER_SUPPORT
	int ret = 0;
	string out;
	Json::Value root;
	//Json::FastWriter writer;
	Json::Value arrayObj;
    Json::Value item;
	
	FILE *fp = fopen(FACE_LOG_NAME,"a+");
	if( fp != NULL )
	{
		root["order_id"] = p_faceInfo->order_id;
		root["timestamp"] = p_faceInfo->timestamp;

		for(vector<FACE_INFO>::iterator iter = p_faceInfo->face_list.begin(); iter != p_faceInfo->face_list.end(); iter++)
        {
            item["age"] = iter->age;
            item["sec"] = iter->sex;
			item["face_pig_name"] = iter->face_pig_name;
			item["seat_location"] = iter->seat_location;
			item["md5"] = iter->md5;
            arrayObj.append(item);
        }
		root["face_list"] = arrayObj;
		fwrite("face:", 1, 5, fp);
		if(fwrite(out.c_str(), 1, out.size(), fp) <=0)
		{
			printf("[%s] fwrite fail!\n", __FUNCTION__);
			ret = -1;
		}
		fclose(fp);
	}
	else
	{
		ret = -1;
	}
	return ret;	
#else
    return 0;
#endif
}

int DataManager::WriteFaceBinData(ALL_FACE_INFO p_faceInfo)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	FILE *fp = fopen(FACE_LOG_BIN_NAME,"a+");
	if( fp != NULL )
	{
		fwrite(&p_faceInfo, 1, sizeof(ALL_FACE_INFO), fp);
		fclose(fp);
	}

	return 0;
#else
    return 0;
#endif
}


int DataManager::WriteFileData(FileMap p_FileMap)
{
#ifdef DDSERVER_SUPPORT
	int ret = 0;
	string out;
	Json::Value root;
	//Json::FastWriter writer;
	Json::Value arrayObj;
    Json::Value item;
	
	FILE *fp = fopen(FILES_LOG_NAME,"w+");
	if( fp != NULL )
	{
		for(FileMap::iterator iter= p_FileMap.begin(); iter != p_FileMap.end(); iter++)
		{
            if(!m_querybyorder_flag)
            {
                if(m_startTime <= iter->second.creat_time && m_stopTime >= iter->second.creat_time)
                {
                    item["file_name"] = iter->second.file_name;
                    item["file_path"] = iter->second.file_path;
                    item["file_size"] = iter->second.file_size;
                    item["file_type"] = iter->second.file_type;
                    item["creat_time"] = iter->second.creat_time;
                    item["shoot_type"] = iter->second.shoot_type;
                    item["order_id"] = iter->second.orderId;
                    arrayObj.append(item);
                }
            }
            else{
                if(m_OrderId == iter->second.orderId)
                {
                    item["file_name"] = iter->second.file_name;
                    item["file_path"] = iter->second.file_path;
                    item["file_size"] = iter->second.file_size;
                    item["file_type"] = iter->second.file_type;
                    item["creat_time"] = iter->second.creat_time;
                    item["shoot_type"] = iter->second.shoot_type;
                    item["order_id"] = iter->second.orderId;
                    arrayObj.append(item);
                }

            }
		}
		root["file_list"] = arrayObj;
		out = root.toStyledString();
		if(fwrite(out.c_str(), 1, out.size(), fp) <=0)
		{
			printf("[%s] fwrite fail!\n", __FUNCTION__);
			ret = -1;
		}
		fclose(fp);
	}
	else
	{
		ret = -1;
	}

	return ret;
#else
    return 0;
#endif
}

int DataManager::WriteGpsData(std::vector<GPS_INFO> p_gpsInfo)
{
#ifdef DDSERVER_SUPPORT
	int ret = 0;
	string out;
	Json::Value root;
	//Json::FastWriter writer;
	
	FILE *fp = fopen(GPS_LOG_NAME,"w+");
	if( fp != NULL )
	{
		for(std::vector<GPS_INFO>::iterator iter= p_gpsInfo.begin(); iter != p_gpsInfo.end(); iter++)
		{
			root["longitude"] = iter->longitude;
			root["latitude"] = iter->latitude;
			root["altitude"] = iter->altitude;
			//root["car_speed"] = (unsigned int)iter->car_speed;
			root["net_state"] = iter->net_state;
			root["timestamp"] = iter->timestamp;
			out = root.toStyledString();
			fwrite("gps:", 1, 4, fp);
			if(fwrite(out.c_str(), 1, out.size(), fp) <= 0)
			{
				printf("[%s] fwrite fail!\n", __FUNCTION__);
				ret = -1;
			};
		}
		fclose(fp);
	}
	else
	{
		ret = -1;
	}

	return ret;	
#else
    return 0;
#endif
}

int DataManager::WriteGpsBinData(GPS_INFO p_gpsInfo)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	FILE *fp = fopen(GPS_LOG_BIN_NAME,"a+");
	if( fp != NULL )
	{
		fwrite(&p_gpsInfo, 1, sizeof(GPS_INFO), fp);
		fclose(fp);
	}

	return 0;
#else
    return 0;
#endif
}

int DataManager::WriteUserData(std::vector<USER_INFO> p_userData)
{
#ifdef DDSERVER_SUPPORT
	int ret = 0;
	string out;
	Json::Value root;
	//Json::FastWriter writer;
	
	FILE *fp = fopen(USER_LOG_NAME,"w+");
	if( fp != NULL )
	{
		for(std::vector<USER_INFO>::iterator iter= p_userData.begin(); iter != p_userData.end(); iter++)
		{
			root["driver_id"] = iter->driver_id;
			root["imei"] = iter->imei;
			root["sn"] = iter->svn;
			root["sim_serial"] = iter->sim_serial;
			root["license_plate"] = iter->license_plate;
			root["timestamp"] = iter->timestamp;
			out = root.toStyledString();
			fwrite("user:", 1, 5, fp);
			if(fwrite(out.c_str(), 1, out.size(), fp) <= 0)
			{
				printf("[%s] fwrite fail!\n", __FUNCTION__);
				ret = -1;
			};
		}
		fclose(fp);
	}
	else
	{
		ret = -1;
	}

	return ret;	
#else
    return 0;
#endif
}

int DataManager::WriteUserBinData(USER_INFO p_userData)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	FILE *fp = fopen(USER_LOG_BIN_NAME,"a+");
	if( fp != NULL )
	{
		fwrite(&p_userData, 1, sizeof(USER_INFO), fp);
		fclose(fp);
	}

	return 0;
#else
    return 0;
#endif
}

int DataManager::upLoadFileLog(std::string &p_FileName, std::string &p_Key)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	p_Key = m_fileKey;
	p_FileName = m_fileListName;

	m_UploadFile = false;

	return 0;
#else
    return 0;
#endif
}

int DataManager::upLoadCompressLogFile(std::string &p_FileName, std::string &p_Key)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);

	p_Key = m_logKey;
	p_FileName = m_logName;

	m_uploadLog = false;

	return 0;
#else
    return 0;
#endif
}

int DataManager::syncData(int p_FileID)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	int ret = -1;
	switch(p_FileID )
	{
		case 0:
		{
			WriteFileData(m_FileMap);
			ret = 0;
			break;
		}
		case 1:
		{
			WriteFileData(m_FileMap);
			FILE *fin = fopen(ACTION_LOG_BIN_NAME, "r+");
			if(fin != NULL )
			{
				std::vector<ACTION_INFO> actionInfoVec;
				actionInfoVec.clear();
				ACTION_INFO actionInfo;
				while( !feof(fin) )
				{
					memset(&actionInfo, 0, sizeof(actionInfo));
					int nread = fread(&actionInfo, 1, sizeof(actionInfo), fin);
					if( nread == sizeof(actionInfo) )
						actionInfoVec.push_back(actionInfo);
				}
				fclose(fin);
				WriteActionData(actionInfoVec);
			}
			
			fin = fopen(GPS_LOG_BIN_NAME, "r+");
			if(fin != NULL )
			{
				std::vector<GPS_INFO> gpsInfoVec;
				gpsInfoVec.clear();
				GPS_INFO gpsInfo;
				while( !feof(fin) )
				{
					memset(&gpsInfo, 0, sizeof(GPS_INFO));
					int nread = fread(&gpsInfo, 1, sizeof(GPS_INFO), fin);
					if( nread == sizeof(GPS_INFO) )
						gpsInfoVec.push_back(gpsInfo);
				}
				fclose(fin);
				WriteGpsData(gpsInfoVec);
			}

			fin = fopen(USER_LOG_BIN_NAME, "r+");
			if(fin != NULL )
			{
				std::vector<USER_INFO> userInfoVec;
				userInfoVec.clear();
				USER_INFO userInfo;
				while( !feof(fin) )
				{
					memset(&userInfo, 0, sizeof(USER_INFO));
					int nread = fread(&userInfo, 1, sizeof(USER_INFO), fin);
					if( nread == sizeof(USER_INFO) )
						userInfoVec.push_back(userInfo);
				}
				fclose(fin);
				WriteUserData(userInfoVec);
			}			

			ret = 0;
			break;
		}

		default:
			ret = -1;
			break;
	}

	return ret;
#else
    return 0;
#endif
}

void *DataManager::CompressFileThread(void *context)
{
#ifdef DDSERVER_SUPPORT
	DataManager *DM = reinterpret_cast<DataManager*>(context);
	prctl(PR_SET_NAME, "CompressFileThread", 0, 0, 0);

	while(!DM->m_ThreadExit)
	{
		if(DM->m_UploadFile )
		{
			//write data to file
			DM->syncData(0);
			char buf[128] = {0};
			snprintf(buf, sizeof(buf),"tar -cf %s %s",COMPRESS_FILES_TEMP_LOG_NAME, FILES_LOG_NAME);
			system(buf);

			time_t now;
			struct tm *tm_now;			
			time(&now);
			tm_now = localtime(&now);
			char compressName[64] = {0};		
			snprintf(compressName, sizeof(compressName),"/mnt/extsd/%04d%02d%02d_%02d%02d%02d_filelist.tar",tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
			DM->m_fileListName = compressName;

			string Md5;
			DM->getFileMd5(COMPRESS_FILES_TEMP_LOG_NAME, Md5);
			string Imei;
			EventManager::GetInstance()->getIMEI(Imei);
			AesCtrl::GetInstance()->setUserKey(Md5+Imei);
			
			AesCtrl::GetInstance()->aes_encrypt(COMPRESS_FILES_TEMP_LOG_NAME, DM->m_fileListName);
			AesCtrl::GetInstance()->getKey(DM->m_fileKey);
			EventReportMsg event;
            event.err_no = 0;
			event.event_type = EVENT_REQ_POST_FILELISTS;
			event.file_name = DM->m_fileListName;
			AdapterLayer::GetInstance()->notifyMessage(event);
			DM->m_UploadFile = false;
		}
		else if( DM->m_uploadLog )
		{
			DM->syncData(1);
			string Md5;
			char buf[128] = {0};
			snprintf(buf, sizeof(buf),"tar -cf %s /mnt/extsd/.tmp/.log",COMPRESS_LOG_TEMP_NAME);
			system(buf);
			time_t now;
			struct tm *tm_now;			
			time(&now);
			tm_now = localtime(&now);
			char compressName[64] = {0};		
			snprintf(compressName, sizeof(compressName),"/mnt/extsd/%04d%02d%02d_%02d%02d%02d_log.tar",tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

			DM->m_logName = compressName;
			DM->getFileMd5(COMPRESS_LOG_TEMP_NAME, Md5);
			string Imei;
			EventManager::GetInstance()->getIMEI(Imei);
			AesCtrl::GetInstance()->setUserKey(Md5+Imei);
			AesCtrl::GetInstance()->aes_encrypt(COMPRESS_LOG_TEMP_NAME, DM->m_logName);
			AesCtrl::GetInstance()->getKey(DM->m_logKey);
			EventReportMsg event;
            event.err_no = 0;
			event.event_type = EVENT_REQ_POST_TRAFFICDATA;
			event.file_name = DM->m_logName;
			AdapterLayer::GetInstance()->notifyMessage(event);
			DM->m_uploadLog = false;
		}
		sleep(2);
	}

	return NULL;
#else
    return NULL;
#endif
}

static int GenGpsJsonData(const GPS_INFO *gpsInfo, std::string &out)
{
#if 0
	Json::Value root;
	Json::FastWriter writer;

	if(gpsInfo != NULL)
	{
		root["longitude"] = gpsInfo->longitude;
		root["latitude"] = gpsInfo->latitude;
		root["altitude"] = gpsInfo->altitude;
		root["car_speed"] = gpsInfo->car_speed;
		root["net_state"] = gpsInfo->net_state;
		root["timestamp"] = gpsInfo->timestamp;
	}else
	{
		return -1;
	}

	out = writer.write(root);

	return 0;
#else
    return 0;
#endif
}

static void GenTimeNow(std::string &filename)
{
#ifdef DDSERVER_SUPPORT
	time_t timep = 0;
	char str[32] = {0};
	
	time(&timep);
	strftime(str, sizeof(str) - 1, "%Y%m%d-%H%M%S", localtime(&timep));
	filename += str;
	return;
#else
    return;
#endif
}

static int getCurtimeOut(time_t start_time)
{
#if 0
	time_t tim_cur;
	time(&tim_cur);

	return tim_cur - start_time;
#else
    return 0;
#endif
}

static void GenGpsDataFileName(std::string &filename)
{
#ifdef DDSERVER_SUPPORT
	filename = TRAFFIC_GPS_FILE_PATH;
	GenTimeNow(filename);
	filename += "_gps.txt";
#endif
}

static int WriteGpsDataToFile(const char *filename, const char *data)
{
#ifdef DDSERVER_SUPPORT
	int ret = -1;
	FILE *fp = NULL;

	fp = fopen(filename, "a+");
	if(fp == NULL)
	{
		//db_error("open file %s failed\n", filename);
		return -1;
	}

	ret = fputs(data, fp);
	if(ret < 0)
	{
		//db_error("write date to %s failed\n", filename);
		fclose(fp);
		return -1;
	}

	sync();
	fclose(fp);
	return 0;
#else
    return 0;
#endif
}

void *DataManager::GPSDataToFileThread(void *context)
{
#ifdef DDSERVER_SUPPORT
	int ret = -1;
	static int first_run = true;
	std::string filename;
	DataManager *DM = reinterpret_cast<DataManager*>(context);
	prctl(PR_SET_NAME, "GPSDataToFileThread", 0, 0, 0);

	while(!DM->m_ThreadExit)
	{
		if(getCurtimeOut(DM->m_creat_time) > TIMEOUT_H)
		{
			time(&DM->m_creat_time);
			GenGpsDataFileName(filename);
		}
		int status = StorageManager::GetInstance()->GetStorageStatus();
		if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
		{
			if(!GPSInfoQue.empty())
			{
				GPS_INFO p_gpsInfo;
				std::string strjson;
				p_gpsInfo = GPSInfoQue.front();
				ret = GenGpsJsonData(&p_gpsInfo, strjson);
				if(ret < 0)
				{
					usleep(100 * 1000);
					continue;
				}

				ret = WriteGpsDataToFile(filename.c_str(), strjson.c_str());
				if(ret < 0)
				{
					usleep(100 * 1000);
					continue;
				}

				GPSInfoQue.pop();
			}
			usleep(200 * 1000);
		}else
		{
			if(!GPSInfoQue.empty())
			{
				GPSInfoQue.pop();
			}
			usleep(200 * 1000);
		}
	}

	return (void*)0;
#else
    return (void*)0;
#endif
}

static int Is_find_file(const char *filename, std::string time_start, std::string time_end)
{
#ifdef DDSERVER_SUPPORT
	long start = 0;
	long end = 0;
	time_t t = 0;
	struct tm timep;

	start = atol(time_start.c_str());
	end = atol(time_end.c_str());
	strptime(filename, "%Y%m%d-%H%M%S", &timep);
	t = mktime(&timep);
	if(t >= start && t <= end)
	{
		return true;
	}

	return false;
#else
    return 0;
#endif
}

static int FindGpsUploadFile(const char *basePath, const char *filetype, const TrafficDataMsg *Traffic)
{
#ifdef DDSERVER_SUPPORT
	char cmd[256] = {0};
	DIR *dir = NULL;
	bool Is_found = false;
	struct dirent *entry = NULL;

	dir = opendir(basePath);
	if(dir == NULL)
	{
		db_error("opendir %s err:%s", basePath, strerror(errno));
		return false;
	}

	while((entry = readdir(dir)) != NULL)
	{
		if(strstr(entry->d_name, filetype) != NULL)
		{
			if(Is_find_file(entry->d_name, Traffic->start_time, Traffic->end_time) == true)
			{
				snprintf(cmd, sizeof(cmd), "tar -uf %s -C %s %s", TRAFFIC_UPLOAD_TMPFILE, basePath, entry->d_name);
				//snprintf(cmd, sizeof(cmd), "cp %s%s %s", basePath, entry->d_name, TRAFFIC_TMP_PATH);
				system(cmd);
				Is_found = true;
			}
		}
		usleep(20 * 1000);
	}

	closedir(dir);

	return Is_found;
#else
    return 0;
#endif
}

#if 0
static int GenGpsCompressFile(const char *basePath)
{
	int ret = -1;
	char cmd[256] = {0};
	char filename[32] = {0};
	int is_found = false;
	DIR *dir = NULL;
	struct dirent *entry = NULL;

	dir = opendir(basePath);
	if(dir == NULL)
	{
		db_error("opendir %s err:%s", basePath, strerror(errno));
		return -1;
	}

	while((entry = readdir(dir)) != NULL)
	{
		if(strstr(entry->d_name, ".txt") != NULL)
		{
			is_found = true;
			break;
		}
	}

	if(is_found == false)
	{
		db_error("can't found upload log file");
		closedir(dir);
		return -1;
	}else
	{
		snprintf(filename, sizeof(filename), "%s*.txt", TRAFFIC_TMP_PATH);
		snprintf(cmd, sizeof(cmd), "tar -cf %s %s", TRAFFIC_UPLOAD_TMPFILE, filename);
		system(cmd);
		closedir(dir);
		return 0;
	}
}
#endif

void DataManager::GenEncCompressFile_Notify(const TrafficDataMsg *trafficMsg, DataManager *DM, bool Is_sucess)
{
#ifdef DDSERVER_SUPPORT
	std::string Md5;
	std::string Imei;
	EventReportMsg event;
    event.err_no = 0;

	if(Is_sucess == true)
	{
		DM->m_logName = TRAFFIC_ROOT_PATH;
		GenTimeNow(DM->m_logName);
		DM->m_logName += "_log.tar";

		DM->getFileMd5(TRAFFIC_UPLOAD_TMPFILE, Md5);
		EventManager::GetInstance()->getIMEI(Imei);
		AesCtrl::GetInstance()->setUserKey(Md5+Imei);
		AesCtrl::GetInstance()->aes_encrypt(TRAFFIC_UPLOAD_TMPFILE, DM->m_logName);
		AesCtrl::GetInstance()->getKey(DM->m_logKey);
		DM->getFileMd5(DM->m_logName, Md5);

		event.err_no = 0;
		event.event_type = EVENT_REQ_POST_TRAFFICDATA;
		event.file_name = DM->m_logName;
		event.file_content_key = DM->m_logKey;
		event.md5 = Md5;
		event.serialcode = trafficMsg->comMsg.serialcode;
	    event.chunk_size = trafficMsg->chunk_size;
	    event.callbackurl = trafficMsg->callbackurl;
		remove(TRAFFIC_UPLOAD_TMPFILE);
	}else
	{
		event.event_type = EVENT_REQ_POST_TRAFFICDATA;
		event.err_no = -1;
	}
	AdapterLayer::GetInstance()->notifyMessage(event);

	return;
#else
    return;
#endif
}

int DataManager::PaserCmdTypeOperation(const TrafficDataMsg *trafficMsg, DataManager *Dm)
{
#ifdef DDSERVER_SUPPORT
	bool Is_sucess = false;

	switch(trafficMsg->file_type)
	{
		case SHOOT_TYPE:
		case ABNORMAL_TYPE:
			GenEncCompressFile_Notify(trafficMsg, Dm, false);
			break;
		case GPS_TYPE:
			Is_sucess = FindGpsUploadFile(TRAFFIC_GPS_FILE_PATH, GPSFILETYPE, trafficMsg);
			if(Is_sucess < 0)
			{
				db_error("FindGpsCompressFile failed Is_sucess = %d", Is_sucess);
			}

			GenEncCompressFile_Notify(trafficMsg, Dm, Is_sucess);
			break;
		case ALL_TYPE:
		default:
			GenEncCompressFile_Notify(trafficMsg, Dm, false);
			db_error("this is unspported cmd_type:%d", trafficMsg->file_type);
			break;
	}

	return 0;
#else
    return 0;
#endif
}

void *DataManager::GetTrafficCmdThread(void *context)
{
#ifdef DDSERVER_SUPPORT
	int ret = -1;
	DataManager *DM = reinterpret_cast<DataManager*>(context);
	prctl(PR_SET_NAME, "GetTrafficCmdThread", 0, 0, 0);

	while(!DM->m_ThreadExit)
	{
		if(!TrafficCMDQue.empty())
		{
			TrafficDataMsg p_traffic = TrafficCMDQue.front();
			db_warn("[ghy], cmd_type:%d.....", p_traffic.file_type);
			ret = PaserCmdTypeOperation(&p_traffic, DM);
			TrafficCMDQue.pop();
		}

		usleep(50 * 1000);
	}

	return (void*)0;
#else
    return (void*)0;
#endif
}

int DataManager::getFileMd5(const std::string p_FileName,string & p_Md5)
{
#ifdef DDSERVER_SUPPORT
	char name[128] = {0};
	snprintf(name, sizeof(name), "md5sum %s | cut -d ' ' -f1 > /tmp/dataManager_md5.txt", p_FileName.c_str());
	system(name);
	int fd = open("/tmp/dataManager_md5.txt", O_RDONLY, 444);
	if( fd < 0 )
	{
		system("rm /tmp/dataManager_md5.txt");
		return -1;
	}
	char buf[128] = {0};

	int ret = read(fd, buf, 32);
	if( ret < 0 )
	{
		close(fd);
		system("rm /tmp/dataManager_md5.txt");
		return -1;
	}

	p_Md5 = buf;

	close(fd);
	system("rm /tmp/dataManager_md5.txt");

	return 0;
#else
    return 0;
#endif
}

int DataManager::setUploadFileList()
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);

	if( m_UploadFile )
	{
		db_warn("tar file list is busying");
		return -1;
	}

	m_UploadFile = true;

	return 0;
#else
    return 0;
#endif
}

int DataManager::setUploadLogList(const TrafficDataMsg *trafficMsg)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);

	TrafficCMDQue.push(*trafficMsg);
	// if( m_uploadLog )
	// {
	// 	db_warn("tar log list is busying");
	// 	return -1;
	// }

	// m_uploadLog = true;

	return 0;
#else
    return 0;
#endif
}

int DataManager::setQueryInfo(std::string p_startTime, std::string p_stopTime)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);

	m_startTime = (unsigned int)strtol(p_startTime.c_str(), NULL, 10);
	m_stopTime = (unsigned int)strtol(p_stopTime.c_str(), NULL, 10);

	if( m_startTime == m_stopTime && m_startTime != 0)
	{
		db_warn("time p_startTime:%s p_stopTime:%s",p_startTime.c_str(), p_stopTime.c_str());
		return -1;
	}
    m_querybyorder_flag = false;

	return 0;
#else
    return 0;
#endif
}

int DataManager::setQueryRollOrderId(std::string p_OrderId)
{	
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);

	if( m_UploadFile )
	{
		db_warn("tar file list is busying");
		return -1;
	}

	m_UploadFile = true;
    m_querybyorder_flag = true;

	m_OrderId = p_OrderId;

	return 0;
#else
    return 0;
#endif
}

int DataManager::ClearMap()
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
	m_FileMap.clear();
	
	return 0;
#else
    return 0;
#endif
}

int DataManager::pushToFileMap(FILE_INFO p_FileInfo)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
    int status = mStorage->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
		m_FileMap.insert(make_pair(p_FileInfo.file_name, p_FileInfo));
	}

	return 0;
#else
    return 0;
#endif
}

int DataManager::popFromFileMap(FILE_INFO p_FileInfo)
{
#ifdef DDSERVER_SUPPORT
	Mutex::Autolock _l(m_mutex);
    int status = mStorage->GetStorageStatus();
    if( (status != UMOUNT) && (status != STORAGE_FS_ERROR) && (status != FORMATTING))
    {
    	FileMap::iterator it;
		for( it = m_FileMap.begin(); it != m_FileMap.end(); it++)
		{
			if( strncmp(it->first.c_str(), p_FileInfo.file_name,sizeof(p_FileInfo.file_name)) == 0)
			{
				m_FileMap.erase(it);
				break;
			}
		}
	}

	return 0;
#else
    return 0;
#endif
}

int DataManager::dumpFileMap()
{
#ifdef DDSERVER_SUPPORT
	db_msg("mFileMap has %d file\n",m_FileMap.size());
	for(FileMap::iterator iter= m_FileMap.begin(); iter != m_FileMap.end(); iter++)
	{
		db_msg("file_name:%s\n",iter->second.file_name);
	}

	return 0;
#else
    return 0;
#endif
}
