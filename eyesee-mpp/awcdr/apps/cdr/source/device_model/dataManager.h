#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include <string>
#include <utils/Mutex.h>
#include <pthread.h>
#include <sys/time.h>
#include <queue>
#include "device_model/storage_manager.h"
#include "dd_serv/dd_common.h"
#include <map>

using namespace EyeseeLinux;

#define MAX_KEY_LEN 32
#define MAX_DATANAME_LEN 32

typedef enum
{
	SHOOT_TYPE = 0,
	ABNORMAL_TYPE,
	GPS_TYPE,
	ALL_TYPE
}cmd_t;

typedef enum
{
	ACTION_DATA = 0,
	FACE_DATA,
	FILE_DATA,
	GPS_DATA,
	USER_DATA,
}DataType;

typedef struct _ACTION_INFO_
{
	int driving_mode;	//行车模式	 0：私家车模式	 1: 顺风车模式
	int after_camera_state; // 后录摄像头开启状态	  0 关闭   1开启
	int before_camera_state; //前置摄像头开启状态	  0 关闭   1开启
	int microphone_state;//录音状态		  0 关闭   1开启
	int timestamp;	//时间戳
}ACTION_INFO;

typedef struct _FACE_INFO_
{
	int age;   //年龄
	int sex;   //性别
	char face_pig_name;	// 人脸截图名称
	int seat_location; // 1:司机 2：乘客
	char md5[64]; //识别图片MD5
}FACE_INFO;

typedef struct _ALL_FACE_INFO_
{
	std::string order_id;
	std::string timestamp;
	std::vector<FACE_INFO> face_list;
}ALL_FACE_INFO;

typedef struct _FILE_INFO_
{
	int file_type; // 文件类型	 0：图片   1：视频
	char file_name[64];	//文件名称
	int file_size; //文件大小
	char file_path[64]; //文件路径
	int creat_time; // 创建时间
	int shoot_type; // 0 : 车内摄像头产生的文件  1：前置摄像头产生的文件
	char orderId[64];
}FILE_INFO;

typedef struct _GPS_INFO_
{
	char longitude[64];	// 经度
	char latitude[64];    //纬度
	float altitude;   // 海拔
	char car_speed[64];	 // 车速   单位：m/s
	int net_state;	//网络状态	 0:4G 1：3G  2：2G	3：WiFi 4：无网络
	char timestamp[64];	//时间戳
}GPS_INFO;

//1469148274,GSENSOR,0.005615,-0.89917,9.89917
typedef struct _USER_INFO_
{
	int driver_id;	// 车主id
	char imei[64];	   // 设备imei
	char svn[64];	   //  设备svn
	char sim_serial[64];   //SIM卡序列号
	char license_plate[64];	// 车牌
	int timestamp;	//时间戳
}USER_INFO;

typedef std::map<std::string, FILE_INFO> FileMap;

class DataManager
{
public:
	DataManager();
	~DataManager();

public:
	static DataManager* GetInstance(void);

	int WriteActionBinData(ACTION_INFO p_actionInfo);

	int WriteFaceBinData(ALL_FACE_INFO p_faceInfo);

	int WriteGpsBinData(GPS_INFO p_gpsInfo);

	int WriteUserBinData(USER_INFO p_userData);

	/*
	*名称: int upLoadCompressLogFile(string &p_FileName, string &p_Key)
	*功能: 获取压缩日志
	*参数:  
	*	p_FileName: 输出压缩包文件名 输出
	*   p_Key: 输出压缩包的加密密钥 输出
	*返回值:
	*   0:成功
	*   -1:失败
	*修改: 创建2018/6/11
	*/
	int upLoadCompressLogFile(std::string &p_FileName, std::string &p_Key);

	/*
	*名称: int upLoadFileLog(string &p_FileName, string &p_Key)
	*功能: 获取文件列表
	*参数:	
	*	p_FileName: 输出文件列表 输出
	*	p_Key: 输出文件列表的加密密钥 输出
	*返回值:
	*	0:成功
	*	-1:失败
	*修改: 创建2018/6/11
	*/
	int upLoadFileLog(std::string &p_FileName, std::string &p_Key);
	int setUploadFileList();
	int setUploadLogList(const TrafficDataMsg *trafficMsg);
	int syncData(int p_FileID);
	int setQueryInfo(std::string p_startTime, std::string p_stopTime);
	/*
	*名称: int setQueryRollOrderId(std::string p_OrderId)
	*功能: 设置需要查询的订单号，查询结果通过异步发送
	*参数: 
	*	p_OrderId:	订单号
	*返回值:
	*	0:成功
	*	-1:失败
	*修改: 创建2018/7/17
	*/
	int setQueryRollOrderId(std::string p_OrderId);

	int ClearMap();
	int pushToFileMap(FILE_INFO p_FileInfo);
	int popFromFileMap(FILE_INFO p_FileInfo);
	int dumpFileMap();
private:
	int WriteActionData(std::vector<ACTION_INFO> p_actionInfo);
	int WriteFaceData(std::vector<ALL_FACE_INFO> p_faceInfo);
	int WriteFileData(FileMap p_FileMap);
	int WriteGpsData(std::vector<GPS_INFO> p_gpsInfo);
	int WriteUserData(std::vector<USER_INFO> p_userData);

	static void GenEncCompressFile();
	static int PaserCmdTypeOperation(const TrafficDataMsg *trafficMsg, DataManager *Dm);
	static void GenEncCompressFile_Notify(const TrafficDataMsg *trafficMsg, DataManager *Dm, bool Is_sucess);
	
	int getFileMd5(const std::string p_FileName, std::string & p_Md5);
	static void *CompressFileThread(void *context);
	static void *GPSDataToFileThread(void *context);
	static void *GetTrafficCmdThread(void *context);
private:
	time_t m_creat_time;
	bool m_ThreadExit;
	pthread_t ComPressThreadId;
	pthread_t GPSDataOpThreadId;
	pthread_t GetTrafficCmdThreadId;
	Mutex m_mutex;
	StorageManager *mStorage;
	bool m_UploadFile, m_uploadLog;
	bool m_querybyorder_flag;
	std::string m_fileKey, m_logKey;
	std::string m_logName, m_fileListName;
	int m_startTime, m_stopTime;
	std::string m_OrderId;
	FileMap m_FileMap;
};

#endif
