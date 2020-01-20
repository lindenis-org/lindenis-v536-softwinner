/*
*版权声明:暂无
*文件名称:fileLockManager.cpp
*创建者:陈振华
*创建日期:2018-5-23
*文件描述:本文件主要管理文件的锁定与解除锁定的逻辑实现
*历史记录:无
*/

#include "fileLockManager.h"
#include "common/app_log.h"
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "device_model/dataManager.h"
#include "device_model/storage_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "fileLockManager.cpp"
#endif

using namespace EyeseeLinux;
using namespace std;

static Mutex fileLockManager_mutex;
static FileLockManager *m_Manager = NULL;

FileLockManager *FileLockManager::GetInstance(void)
{
	Mutex::Autolock _l(fileLockManager_mutex);
	if( m_Manager != NULL )
		return m_Manager;

	m_Manager = new FileLockManager();
	if( m_Manager != NULL )
		return m_Manager;
	else
	{
		db_error("new FileLockManager failed\n");
		return NULL;
	}
}

FileLockManager::FileLockManager()
{
	mMediaFileManager = MediaFileManager::GetInstance();
	m_picName.clear();
	m_recordName.clear();
	m_unLockTime.clear();
	m_FileNameList.clear();
	m_LockFileVector.clear();
	m_unLockFileVector.clear();
	m_LockFileInfo.clear();
	m_threadId = -1;
	m_ThreadExit = false;
	m_ThreadPause = false;
	m_ThreadPauseFinish = true;
	m_FileCount = 0;
	m_LockFileName.clear();
	m_UnLockFileName.clear();
	m_LockFileList.clear();
	m_DeleteFileList.clear();
	ThreadCreate(&m_threadId, NULL, FileLockManager::CheckThread, this);
}

FileLockManager::~FileLockManager()
{
	m_ThreadExit = true;
}

int FileLockManager::setLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, const std::string p_LockTime, int p_CamId, std::string p_OrderId)
{
	Mutex::Autolock _l(m_mutex);

	//从数据库中找文件
	m_FileNameList.clear();
	m_LockFileVector.clear();
	int ret = mMediaFileManager->GetFileList(p_StartTime, p_StopTime, m_FileNameList,p_CamId, p_OrderId);
	if( ret < 0 )
	{
		db_warn("set lock file failed startTime:%s stopTime:%s camId:%d\n",p_StartTime.c_str(), p_StopTime.c_str(), p_CamId);
		return -1;
	}

	m_unLockTime = p_LockTime;
	for (unsigned int i=0; i<m_FileNameList.size(); i++)
	{
		mMediaFileManager->SetUpdateFlag(true);
		mMediaFileManager->setFileLockTime(m_FileNameList[i], p_LockTime, p_CamId, 0);
		mMediaFileManager->SetFileLockStatus(m_FileNameList[i], 1, p_CamId);
		LockFileInfo m_LockFileInfo;
		std::string::size_type rc = m_FileNameList[i].rfind("/");
		if( rc == std::string::npos )
		{
			db_warn("invalid fileName:%s",m_FileNameList[i].c_str());
			continue;
		}
		m_LockFileInfo.file_name = m_FileNameList[i].substr(rc+1);
		m_LockFileInfo.file_path = m_FileNameList[i].substr(0, rc);
		m_LockFileInfo.unlock_time = m_unLockTime;

		m_LockFileVector.push_back(m_LockFileInfo);
	}

	return 0;
}

int FileLockManager::getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo)
{
	Mutex::Autolock _l(m_mutex);

	p_fileLockInfo = m_LockFileVector;

	return 0;
}

int FileLockManager::setUnLockFileByTime(const std::string p_StartTime,const std::string p_StopTime, int p_CamId, std::string p_OrderId)
{
	Mutex::Autolock _l(m_mutex);

	m_FileNameList.clear();
	m_unLockFileVector.clear();
	int ret = mMediaFileManager->GetFileList(p_StartTime, p_StopTime, m_FileNameList,p_CamId,p_OrderId);
	if( ret < 0 )
	{
		db_warn("set unlock file failed startTime:%s stopTime:%s camId:%d\n",p_StartTime.c_str(), p_StopTime.c_str(), p_CamId);
		return -1;
	}

	for (unsigned int i=0; i<m_FileNameList.size(); i++)
	{
		mMediaFileManager->setFileLockTime(m_FileNameList[i], "0", p_CamId, 1);
		mMediaFileManager->SetFileLockStatus(m_FileNameList[i], 0, p_CamId);

		LockFileInfo m_LockFileInfo;
		std::string::size_type rc = m_FileNameList[i].rfind("/");
		if( rc == std::string::npos )
		{
			db_warn("invalid fileName:%s",m_FileNameList[i].c_str());
			continue;
		}

		m_LockFileInfo.file_name = m_FileNameList[i].substr(rc+1);
		m_LockFileInfo.file_path = m_FileNameList[i].substr(0, rc);
		m_LockFileInfo.unlock_time = "0";
		m_unLockFileVector.push_back(m_LockFileInfo);
	}

	return 0;
}

int FileLockManager::getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo)
{
	Mutex::Autolock _l(m_mutex);

	p_fileUnLockInfo = m_unLockFileVector;

	return 0;
}

int FileLockManager::setLockFileByName(const std::string p_FileName,const std::string p_LockTime, int p_CamId)
{
	Mutex::Autolock _l(m_mutex);
	int ret = mMediaFileManager->IsFileExist(p_FileName,p_CamId);
	if( ret < 0 )
		return -1;

	mMediaFileManager->setFileLockTime(p_FileName, p_LockTime, p_CamId, 0);
	mMediaFileManager->SetFileLockStatus(p_FileName, 1, p_CamId);
	m_LockFileName = p_FileName;
	m_unLockTime = p_LockTime;

	return 0;
}

int FileLockManager::setUnLockFileByName(const std::string p_FileName, int p_CamId)
{
	Mutex::Autolock _l(m_mutex);

	int ret = mMediaFileManager->IsFileExist(p_FileName,p_CamId);
	if( ret < 0 )
		return -1;

	mMediaFileManager->setFileLockTime(p_FileName, "0", p_CamId, 1);
	mMediaFileManager->SetFileLockStatus(p_FileName, 0, p_CamId);
	m_UnLockFileName = p_FileName;
	m_unLockTime.clear();

	return 0;
}

int FileLockManager::getSlientPic(FilePushInfo &p_Pic)
{
	Mutex::Autolock _l(m_mutex);

	p_Pic.file_name = m_picName;
	getFileMd5(m_picName, p_Pic.md5);
	mMediaFileManager->GetFileKey(m_picName,p_Pic.key);

	return 0;
}

int FileLockManager::getRecordFile(FilePushInfo &p_fileInfo)
{
	Mutex::Autolock _l(m_mutex);

	p_fileInfo.file_name = m_recordName;
	getFileMd5(m_recordName, p_fileInfo.md5);
	mMediaFileManager->GetFileKey(m_recordName,p_fileInfo.key);

	return 0;
}

int FileLockManager::getFileKey(const std::string p_FileName,std::string & p_key)
{
	Mutex::Autolock _l(m_mutex);

	return mMediaFileManager->GetFileKey(p_FileName, p_key);
}

int FileLockManager::setSlientPicName(const std::string p_picName)
{
	Mutex::Autolock _l(m_mutex);
	m_picName = p_picName;

	return 0;
}

int FileLockManager::setRecordFileName(const std::string p_recordName)
{
	Mutex::Autolock _l(m_mutex);
	m_recordName = p_recordName;

	return 0;
}

int FileLockManager::getFileMd5(const std::string p_FileName,std::string & p_Md5)
{
	char name[128] = {0};
	snprintf(name, sizeof(name), "md5sum %s | cut -d ' ' -f1 > /tmp/fileLock_md5.txt", p_FileName.c_str());
	system(name);
	int fd = open("/tmp/fileLock_md5.txt", O_RDONLY, 444);
	if( fd < 0 )
	{
		system("rm /tmp/fileLock_md5.txt");
		return -1;
	}
	char buf[128] = {0};

	int ret = read(fd, buf, 32);
	if( ret < 0 )
	{
		close(fd);
		system("rm /tmp/fileLock_md5.txt");
		return -1;
	}

	p_Md5 = buf;

	close(fd);
	system("rm /tmp/fileLock_md5.txt");

	return 0;
}

int FileLockManager::removeSlientPic(int p_CamId)
{
	Mutex::Autolock _l(m_mutex);

	return mMediaFileManager->DeleteFileByName(m_picName, p_CamId);
}

int FileLockManager::removeSlientRecordFile(int p_CamId)
{
	Mutex::Autolock _l(m_mutex);

	return mMediaFileManager->DeleteFileByName(m_recordName, p_CamId);
}

int FileLockManager::setFileList(std::string p_startTime, std::string p_stopTime)
{
	Mutex::Autolock _l(m_mutex);
	if( DataManager::GetInstance()->setQueryInfo(p_startTime, p_stopTime) < 0)
		return -1;

	return DataManager::GetInstance()->setUploadFileList();
}

int FileLockManager::getFileList(FilePushInfo &p_fileInfo)
{
	Mutex::Autolock _l(m_mutex);
	DataManager::GetInstance()->upLoadFileLog(p_fileInfo.file_name,p_fileInfo.key);
	getFileMd5(p_fileInfo.file_name ,p_fileInfo.md5);

	return 0;
}

void *FileLockManager::CheckThread(void *context)
{
	FileLockManager *LockManager = reinterpret_cast<FileLockManager*>(context);

	while(!LockManager->m_ThreadExit)
	{
		if(LockManager->m_ThreadPause)
		{
			LockManager->m_ThreadPauseFinish = true;
			sleep(1);
			continue;
		}

		if(!LockManager->mMediaFileManager->DataBaseIsAlready())
		{
			LockManager->m_LockFileInfo.clear();
			LockManager->m_FileCount = 0;
			sleep(2);
			continue;
		}

		if( LockManager->m_FileCount == 0 )
		{
			LockManager->mMediaFileManager->SetUpdateFlag(false);
			LockManager->m_LockFileInfo.clear();
			LockManager->m_FileCount = LockManager->mMediaFileManager->getLockFileCount(1);
			if( LockManager->m_FileCount != -1)
				LockManager->mMediaFileManager->getFileLockInfo(LockManager->m_LockFileInfo,1);
		}
		else if(LockManager->m_FileCount != LockManager->mMediaFileManager->getLockFileCount(1))
		{			
			LockManager->mMediaFileManager->SetUpdateFlag(false);
			LockManager->m_LockFileInfo.clear();
			LockManager->m_FileCount = LockManager->mMediaFileManager->getLockFileCount(1);
			if( LockManager->m_FileCount != -1)
				LockManager->mMediaFileManager->getFileLockInfo(LockManager->m_LockFileInfo,1);
		}
		else if(LockManager->m_FileCount == LockManager->mMediaFileManager->getLockFileCount(1) && LockManager->m_FileCount != 0)
		{
			bool updateFlag;
			LockManager->mMediaFileManager->GetUpdateFlag(updateFlag);
			if( updateFlag )
			{
				LockManager->mMediaFileManager->SetUpdateFlag(false);
				LockManager->m_LockFileInfo.clear();
				LockManager->m_FileCount = LockManager->mMediaFileManager->getLockFileCount(1);
				if( LockManager->m_FileCount != -1)
					LockManager->mMediaFileManager->getFileLockInfo(LockManager->m_LockFileInfo,1);
			}
		}

		long int m_startTime = time(NULL);
		std::map<std::string, std::string>::iterator iter;
		for(iter = LockManager->m_LockFileInfo.begin(); iter != LockManager->m_LockFileInfo.end(); iter++)
		{
			if(LockManager->m_ThreadPause)
			{
				LockManager->m_ThreadPauseFinish = true;
				break;
			}

			if( m_startTime >=strtol(iter->second.c_str(), NULL, 10))
			{
				CapacityStatus status = StorageManager::GetInstance()->getBackCameraCapacityStatus();
				if( status == CAPACITY_ERROR )
				{
					break;
				}
				else if(status == CAPACITY_EMERGENCY)
				{
					LockManager->removeFile(iter->first);
				}
			}
		}
		sleep(1);
	}

	return NULL;
}

int FileLockManager::getLockFileBynameResult(LockFileInfo &p_fileLockInfo)
{
	Mutex::Autolock _l(m_mutex);
	p_fileLockInfo.unlock_time = m_unLockTime;
	std::string::size_type rc = m_LockFileName.rfind("/");
	if( rc ==std::string::npos)
	{
		db_warn("invalid fileName:%s",m_LockFileName.c_str());
		return -1;
	}
	p_fileLockInfo.file_name = m_LockFileName.substr(rc+1);
	p_fileLockInfo.file_path = m_LockFileName.substr(0, rc);

	m_unLockTime.clear();
	m_LockFileName.clear();

	return 0;
}

int FileLockManager::getunLockFileBynameResult(LockFileInfo &p_fileLockInfo)
{
	Mutex::Autolock _l(m_mutex);
	p_fileLockInfo.unlock_time = "0";
	std::string::size_type rc = m_UnLockFileName.rfind("/");
	if( rc == std::string::npos)
	{
		db_warn("invalid fileName:%s",m_UnLockFileName.c_str());
		return -1;
	}
	p_fileLockInfo.file_name = m_UnLockFileName.substr(rc+1);
	p_fileLockInfo.file_path = m_UnLockFileName.substr(0, rc);

	m_unLockTime.clear();
	m_UnLockFileName.clear();

	return 0;
}

int FileLockManager::setLogList(const TrafficDataMsg *log)
{
	Mutex::Autolock _l(m_mutex);

	return DataManager::GetInstance()->setUploadLogList(log);
}

int FileLockManager::getLogList(FilePushInfo &p_fileInfo)
{
	Mutex::Autolock _l(m_mutex);

	DataManager::GetInstance()->upLoadCompressLogFile(p_fileInfo.file_name,p_fileInfo.key);
	getFileMd5(p_fileInfo.file_name ,p_fileInfo.md5);

	return 0;	
}

int FileLockManager::removeFile(const std::string p_FileName)
{
	Mutex::Autolock _l(m_mutex);
    int camid = 0;
    std::size_t found;
    std::string filename;

    if (p_FileName.empty()) {
        db_error("Invalid argument!");
        return -1;
    }
    found = p_FileName.rfind('/');
    if (found != std::string::npos)
    {
        filename = p_FileName.substr(found + 1);
        if(filename.at(15) == 'A') {
            camid = 0;
        }
        else if(filename.at(15) == 'B') {
            camid = 1;
        }
        else {
            db_error("parse camid form %s failed!", p_FileName.c_str());
            return -1;
        }
    }
	
	return  mMediaFileManager->DeleteFileByName(p_FileName, camid);
}

int FileLockManager::getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId)
{
	Mutex::Autolock _l(m_mutex);

	m_LockFileList.clear();
	mMediaFileManager->getFileLockInfo(m_LockFileList, p_CamId);

	std::map<std::string, std::string>::iterator iter;
	for(iter = m_LockFileList.begin(); iter != m_LockFileList.end(); iter++)
	{
		LockFileInfo lockInfo;
		std::string::size_type rc = iter->first.rfind("/");
		if( rc == std::string::npos)
		{
			db_warn("invalid fileName:%s",iter->first.c_str());
			continue;
		}

		lockInfo.file_name = iter->first.substr(rc+1);
		lockInfo.file_path = iter->first.substr(0, rc);
		lockInfo.unlock_time = iter->second;
		p_lockfileList.push_back(lockInfo);
	}

	return 0;
}

int FileLockManager::setQueryRollOrderId(std::string p_OrderId)
{
	Mutex::Autolock _l(m_mutex);

	return DataManager::GetInstance()->setQueryRollOrderId(p_OrderId);
}

int FileLockManager::setOrderId(const std::string p_OrderId,int p_Status)
{
	return mMediaFileManager->setOrderId(p_OrderId,p_Status);
}

int FileLockManager::getOrderId(std::string &p_OrderId)
{
	return mMediaFileManager->getOrderId(p_OrderId);
}

int FileLockManager::SetCheckThreadStatus(bool p_start)
{
	if( p_start)
	{
		m_ThreadPauseFinish = true;
		m_ThreadPause = false;
	}
	else
	{
		m_ThreadPauseFinish = false;
		m_ThreadPause = true;
	}

	return 0;
}

bool FileLockManager::WaitCheckThreadStop()
{
	if( m_ThreadPause )
	{
		while(!m_ThreadPauseFinish)
		{
			usleep(100*1000);
		}

		return m_ThreadPauseFinish;
	}

	return true;
}

int FileLockManager::getDeleteFileList(std::vector<LockFileInfo> &p_fileInfoVec, int p_CamId)
{
	Mutex::Autolock _l(m_mutex);
    int ret = 0;

	m_DeleteFileList.clear();
	ret = mMediaFileManager->GetDeleteFileList(m_DeleteFileList, p_CamId);
    if(ret == 1 || ret < 0) {
        return ret;
    }

	std::map<std::string, std::string>::iterator iter;
	for(iter = m_DeleteFileList.begin(); iter != m_DeleteFileList.end(); iter++)
	{
		LockFileInfo Info;

		Info.file_name = iter->first;
		Info.unlock_time = iter->second;
		
		p_fileInfoVec.push_back(Info);
	}

	return 0;
}
