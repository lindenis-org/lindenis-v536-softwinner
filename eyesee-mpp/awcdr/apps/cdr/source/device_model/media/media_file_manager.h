/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: media_file_manager.h
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2015-10-30
Description:
    1.通过调用数据库接口(DBCtrl)接口实现数据存储
    2.管理多媒体文件，包括，文件名的生成，文件加解锁
    3.实现拍照和录像完成的回调处理
History:
*************************************************/
#ifndef _MEDIA_FILE_MANAGER_H
#define _MEDIA_FILE_MANAGER_H

#include "common/subject.h"
#include "common/observer.h"
#include "common/message.h"
#include "common/common_inc.h"
#include "common/singleton.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <string>
#include <vector>

#include <thread>
#include <mutex>

namespace SQLite
{
    class Database;
}

namespace EyeseeLinux {

typedef struct MixFileInfo
{
	std::string file_name;
	long long time_value;
	std::string typeStr;
	std::string infoStr;
	long long sizeValue;
	long long LockValue;
	long long LockTimeValue;
	long long CheckStateValue;
	std::string KeyStr;
	std::string orderNumStr;
}MixFileInfo_;

class MediaFile;
class MediaFileManager
    : public IObserverWrap(MediaFileManager)
    , public Singleton<MediaFileManager>
    , public ISubjectWrap(MediaFileManager)
{
    friend class Singleton<MediaFileManager>;
    public:
        MediaFileManager();
        ~MediaFileManager();
        MediaFileManager(const MediaFileManager &o);
        MediaFileManager &operator=(const MediaFileManager &o);

        int8_t AddFile(const MediaFile &file, int p_CamId = 0, int64_t keyValue = 0,bool lock_flag = false);
        int8_t AddFile(const MediaFile &file, SQLite::Statement &stm, int count=-1, int p_CamId = 0, int64_t keyValue = 0,std::string order_Num = "",bool lock_flag = false);
        int8_t RemoveFile(const std::string &filename);
        int8_t GetMediaFileList(std::vector<std::string> &file_list,uint32_t idx_s, uint16_t list_cnt, bool order, const std::string &media_type,int p_CamId = 0);
        uint32_t GetMediaFileCnt(const std::string &media_type, int p_CamId = 0);
        int8_t DeleteFileByName(const std::string &file_name, int p_CamId = 0);
        int8_t DeleteLastFilesDatabaseIndex(const std::string &file_name, int p_CamId);
        int8_t DeleteLastFile(const std::string &media_type, int cnt, int p_CamId = 0, bool m_force=false);
        const std::string GetMediaFileType(const std::string &file_name, int p_CamId = 0);
        time_t GetFileTimestampByName(const std::string &file_name, int p_CamId = 0);
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        std::string GetThumbPicName(const std::string &file_name,int p_CamId);
        std::string GetThumbVideoName(const std::string &file_name);
        inline bool GetDataBaseStartUpInitFlag() {return db_start_up_init_;}
        inline void SetDataBaseStartUpInitFlag(bool flag) {db_start_up_init_ = flag;}
        int64_t GetLastFileFallocateSizeByType(const std::string &media_type,int p_CamId);
        uint32_t getFilefallocateSize(const std::string p_DirName);
       /*
        *名称: bool IsFileExist(char *p_FileName)
        *功能: 判断指定的文件在数据库中是否存在
        *参数:
        *   p_FileName: 需要查找的文件名称
        *      p_CamId:需要查找的哪个数据库
        *返回值:
        *   true:成功
        *   false:失败
        *修改: 创建2018/5/17
        */
		bool IsFileExist(const std::string &p_FileName,int p_CamId, int p_CountCapacity=0);

	   /*
        *名称: int  SetFileLockStatus(const std::string &p_FileName, int p_LockStatus,int p_CamId)
        *功能: 设置数据库中文件的锁定状态值
        *参数:
        *   p_FileName: 需要查找的文件名称
        *   p_LockStatus: 锁定值
        *      p_CamId:需要查找的哪个数据库
        *返回值:
        *   0:成功
        *   -1:失败
        *修改: 创建2018/5/17
        */
		int  SetFileLockStatus(const std::string &p_FileName, int p_LockStatus,int p_CamId);
	   /*
        *名称: int  GetFileLockStatus(const std::string &p_FileName, int p_LockStatus,int p_CamId)
        *功能: 设置数据库中文件的锁定状态值
        *参数:
        *   p_FileName: 需要查找的文件名称
        *   p_LockStatus: 锁定值
        *      p_CamId:需要查找的哪个数据库
        *返回值:
        *   0:成功
        *   -1:失败
        *修改: 创建2018/5/17
		*/
		int  GetFileLockStatus(const std::string &p_FileName, int *p_LockStatus,int p_CamId);
        /*
        *名称: int GetFileList(string p_startTime, string p_stopTime, std::vector<std::string> &p_FileNameList,int p_CamId)
        *功能: 获取指定时间段内的文件列表信息
        *参数:
        *   p_startTime: 开始时间
        *   p_stopTime: 结束时间
        *   p_FileNameList: 查询结果存储容器
        *      p_CamId:需要查找的哪个数据库
        *返回值:
        *   0:成功
        *   -1:失败
        *修改: 创建2018/5/17
        */
		int GetFileList(const std::string p_startTime, const std::string p_stopTime, std::vector<std::string> &p_FileNameList,int p_CamId, std::string p_OrderId);

        /*
        *名称: int setFileLockTime(const std::string p_FileName, const std::string p_LockTime, int p_CamId, int p_action)
        *功能: 设置指定文件的锁定时间
        *参数:
        *   p_FileName: 文件名
        *   p_LockTime: 锁定时间
        *   p_CamId:需要查找的哪个数据库
        *   p_action: 0(设置锁定)/1(设置解锁)
        *返回值:
        *   0:成功
        *   -1:失败
        *修改: 创建2018/5/17
        */
		int setFileLockTime(const std::string p_FileName, const std::string p_LockTime, int p_CamId, int p_action);
        /*
        *名称: int getFileLockInfo(std::map<std::string, std::string> &p_Info, int p_CamId)
        *功能: 获取数据库中的锁定文件信息,包括文件名与锁定时间
        *参数:
        *   p_Info: 文件信息
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   0:成功
        *   -1:失败
        *修改: 创建2018/5/24
        */
		int getFileLockInfo(std::map<std::string, std::string> &p_Info, int p_CamId);

        /*
        *名称: int getLockFileCount(int p_CamId)
        *功能: 获取数据库中的锁定文件个数
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   文件个数
        *修改: 创建2018/5/24
        */

		int getLockFileCount(int p_CamId);
        /*
        *名称: int GetFileKey(const string &p_FileName,string &keystring)
        *功能: 根据p_FileName 和p_CamId 获取对应数据库对应的keystring
        *参数:
        *
        *返回值:
        *   成功：0  失败-1
        *修改: 创建2018/06/05
        */
		int GetFileKey(const std::string &p_FileName, std::string &keystring);

        /*
        *名称: int GetFileList(std::map<std::string, std::string> &file_list, int p_CamId)
        *功能: 获取p_CamId对应数据库文件信息
        *参数:
        *  file_list: 文件信息
        *返回值:
        *   0: 成功
        *   -1: 失败
        *修改: 创建2018/06/05
        */
		int GetFileList(std::map<std::string, std::string> &file_list, int p_CamId) ;
        /*
        *名称: int GetAllInfoFileList(std::vector<MixFileInfo_>&file_list, int p_CamId)
        *功能: 获取p_CamId对应数据库文件信息
        *参数:
        *  file_list:数据库文件所有信息
        *返回值:
        *   0: 成功
        *   -1: 失败
        *修改: 创建2018/06/05
        */

		int GetAllInfoFileList(std::vector<MixFileInfo_>&file_list, int p_CamId);

        /*
        *名称: int setRollingOrder(const string p_OrderId, int p_Status)
        *功能: 设置顺风车指令
        *参数:
        *   p_OrderId: 顺风车订单号 输入
        *   p_Status: 订单结束(0)/订单开始(1)
        *返回值:
        *   0:成功
        *   1:失败
        *修改: 创建2018/6/30
        */
		int setOrderId(const std::string p_OrderId,int p_Status);
		
        /*
        *名称: int getRollingOrderId(string &p_OrderId)
        *功能: 获取顺风车订单号
        *参数:
        *   p_OrderId: 顺风车订单号 输出
        *返回值:
        *   0:成功
        *   1:失败
        *修改: 创建2018/6/30
        */
		int getOrderId(std::string &p_OrderId);

        /*
        *名称: bool DataBaseIsAlready()
        *功能: 获取数据库是否创建和刷新完成
        *参数:
        *返回值:
        *   true:可以读取
        *   false:未准备好
        *修改: 创建2018/6/30
        */
		bool DataBaseIsAlready();
        int getOrderIdByName(const std::string &p_FileName, std::string &p_OrderId);
		int  startCreateDataBase();

        /*
        *名称: int GetUpdateFlag(bool &p_flag)
        *功能:获取更新数据标识
        *参数:
        *   true:需要更新  false: 不需要更新
        *返回值:
        *修改: 创建2018/06/04
        */
		int GetUpdateFlag(bool &p_flag);

        /*
        *名称: int SetUpdateFlag(bool p_flag)
        *功能: 设置更新数据标识
        *参数:
        *   true:需要更新  false: 不需要更新
        *返回值:
        *修改: 创建2018/06/04
        */
		int SetUpdateFlag(bool p_flag);
		int GetDeleteFileList(std::map<std::string, std::string> &file_list, int p_CamId);
		std::string getFileReallyName(std::string p_FileName);
		std::string getFilePath(std::string p_FilePath);
		int getFileType(std::string fileType);	
        std::string getlockFileName();
        std::string getlocThumbPickFileName();
        int SetFileInfoByName(const std::string &fileName,const std::string &fileName_dts,const std::string typeStr,int lockStatus, int p_CamId);
        int GetThumbFromMainPic(const std::string& src_pic,const std::string& dest_pic);
        void GetLastFileByType(const std::string &media_type,int p_CamId, std::string &filename);
		int GetMediaFileNameTime(const std::string &fileName, std::string &fileNameTime);
private:
		static void *DBInitThread(void *arg);
		static void *DoDBUpdate(void *arg);
        int8_t DBUpdate();
        int8_t DeleteFilesByType(const std::string &media_type, int p_CamId = 0);

        /*
        *名称: bool getNewDbCreateFlag(int p_CamId)
        *功能: 根据p_CamId 获取对应的数据库是否创建标志位
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   数据库的创建标志位
        *修改: 创建2018/06/05
        */
		bool getNewDbCreateFlag(int P_CamID);

        /*
        *名称: int8_t clearCheckFlag(int p_CamId)
        *功能: 根据p_CamId 对应数据库设置对应的checkstate = 0
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   成功：0  失败-1
        *修改: 创建2018/06/05
        */
		int8_t clearCheckFlag(int p_CamId);
        /*
        *名称: int8_t setCheckFlag(int P_CamID,int checkF,const std::string & file_name);
        *功能: 根据p_CamId 对应数据库设置对应的checkstate 的值
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   成功：0  失败-1
        *修改: 创建2018/06/05
        */
		int8_t setCheckFlag(int P_CamID,int checkF,const std::string & file_name);

        /*
        *名称: SQLite::Database * getDBHandle(int p_CamId)
        *功能: 根据p_CamId 获取对应的数据库句柄
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   数据库的句柄
        *修改: 创建2018/06/05
        */
		SQLite::Database * getDBHandle(int P_CamID){return db_;};
        /*
        *名称: void DoDBUpdateByCamId(MediaFileManager *self,int P_CamID)
        *功能: 根据p_CamId 更新对应的数据库记录内容
        *参数:
        *   self:  MediaFileManager 类指针
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *修改: 创建2018/06/05
        */

		void DoDBUpdateByCamId(MediaFileManager *self,int P_CamID);
        /*
        *名称: int8_t deleteFileFromDB(int p_CamId)
        *功能: 根据p_CamId 删除对应数据库里面checkstatus =0 的不存在的文件记录信息
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   成功：0  失败-1
        *修改: 创建2018/06/05
        */

        void DoDBUpdateByCamIdEx(std::string p_scanPath,MediaFileManager *self,int P_CamID, int p_IsVideo);

        
		int8_t deleteFileFromDB(int p_CamId);

        /*
        *名称: int getFileCount(int p_CamId)
        *功能: 获取数据库中的对应Id 文件个数
        *参数:
        *   p_CamId:  摄像头ID 0:前置 1:后置
        *返回值:
        *   文件个数
        *修改: 创建2018/06/04
        */
		int getFileCount(int p_CamId);
        int8_t DBInit();
        int8_t DBReset();
        int8_t DeleteFileByOrder(const std::string &media_type, bool order, int p_CamId = 0,bool m_force=false);
		void timeString2Time_t(const std::string &timestr,time_t *time);
		int InsertLockInfoToMap(std::map<std::string, std::string> &p_Info,const std::string &file_name,const std::string &LockTime);

private:
		pthread_mutex_t m_lock, m_db_lock, m_update_lock;
        bool db_inited_; //记录db数据库是否创建完成
        bool db_stop_update_; //记录数据库文件扫描是否完成
        bool db_update_done_;
		bool db_new_create_;
		bool db_backCam_new_create_;
		bool db_start_up_init_;
        SQLite::Database *db_;
	 	SQLite::Database *m_BackCam_db;
		std::string m_orderId;
		int m_OrderStatus;
		bool m_needUpdate;
		pthread_t DB_updata_thread_id,DB_init_thread_id;
		bool m_IgnoreScan;
        std::string m_lockFileName;
        std::string m_lockthumbPicFileName;
        std::vector<uint32_t> m_fallocateSize;
}; // class MediaManager
} // namespace EyeseeLinux

#endif // media_file_manager.h
