/*
*版权声明:暂无
*文件名称:fileLockManager.h
*创建者:陈振华
*创建日期:2018-5-23
*文件描述:本文件主要管理文件的锁定与解除锁定的逻辑实现
*历史记录:无
*/

#ifndef __FILE_LOCK_MANAGER_H__
#define __FILE_LOCK_MANAGER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <utils/Mutex.h>
#include "AdapterLayer.h"
#include "device_model/media/media_file_manager.h"
#include <pthread.h>

using namespace EyeseeLinux;

class FileLockManager
{
	public:
		FileLockManager();
		~FileLockManager();
	public:
		static FileLockManager* GetInstance(void);

		/*
		*名称: int setLockFileByTime(const string p_StartTime, const string p_StopTime, const string p_LockTime, int p_CamId)
		*功能: 设置指定时间段里面的文件锁定，并且设置锁定时间
		*参数: 
		*	p_StartTime: 起始时间
		*   p_StopTime: 结束时间
		*   p_LockTime: 锁定时间
		*   p_CamId:指定前路还是后路摄像头
		*   p_OrderId: 订单号
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, const std::string p_LockTime, int p_CamId, std::string p_OrderId);

		/*
		*名称: int getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo)
		*功能: 获取锁定文件结果
		*参数:
		*	p_fileLockInfo 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo);

		/*
		*名称: int setUnLockFileByTime(const string p_StartTime, const string p_StopTime, int p_CamId)
		*功能: 解锁指定时间段里面的文件
		*参数: 
		*	p_StartTime: 起始时间
		*   p_StopTime: 结束时间
		*   p_CamId: 摄像头ID 0:前置 1:后置
		*   p_OrderId: 订单号
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setUnLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, int p_CamId, std::string p_OrderId);

		/*
		*名称: int getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo)
		*功能: 获取解锁文件结果
		*参数:
		*	p_fileUnLockInfo  输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo);


		/*
		*名称: int setLockFileByTime(const string p_FileName, const string p_LockTime, int p_CamId)
		*功能: 设置指定文件锁定，并且设置锁定时间
		*参数: 
		*	p_FileName: 锁定文件的文件名
		*   p_LockTime: 锁定时间
		*   p_CamId:  摄像头ID 0:前置 1:后置
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setLockFileByName(const std::string p_FileName, const std::string p_LockTime, int p_CamId);

		/*
		*名称: int  setUnLockFileByName(const string p_FileName, int p_CamId)
		*功能: 指定文件解锁
		*参数: 
		*	p_FileName: 解锁文件的文件名
		*   p_CamId:  摄像头ID 0:前置 1:后置
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setUnLockFileByName(const std::string p_FileName, int p_CamId);

		/*
		*名称: int getSlientPic(FilePushInfo &p_Pic)
		*功能: 获取静默拍照的结果
		*参数: 
		*	p_Pic: 结果信息
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getSlientPic(FilePushInfo &p_Pic);

		/*
		*名称: int getRecordFile(FilePushInfo &p_fileInfo)
		*功能: 获取文件信息
		*参数: 
		*	p_fileInfo: 文件信息结构体
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getRecordFile(FilePushInfo &p_fileInfo);

		/*
		*名称: int getFileKey(const string p_FileName, string &p_key)
		*功能: 获取指定文件的加密秘钥
		*参数:
		*	p_FileName: 文件名 输入
		*	p_key:秘钥 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getFileKey(const std::string p_FileName, std::string &p_key);

		/*
		*名称: int setSlientPicName(const string p_picName)
		*功能: 设置静默拍照文件的文件名
		*参数: 
		*	p_picName: 文件名
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/24
		*/
		int setSlientPicName(const std::string p_picName);

		/*
		*名称: int setRecordFileName(const string p_recordName)
		*功能: 设置录像文件的文件名
		*参数: 
		*	p_recordName: 文件名
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/24
		*/
		int setRecordFileName(const std::string p_recordName);

		/*
		*名称: int removeSlientPic(int p_CamId)
		*功能: 删除静默拍摄的图片
		*参数: 
		*   p_CamId:  摄像头ID 0:前置 1:后置
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/25
		*/
		int removeSlientPic(int p_CamId);

		/*
		*名称: int removeSlientRecordFile(int p_CamId)
		*功能: 删除静默拍摄的视频
		*参数: 
		*   p_CamId:  摄像头ID 0:前置 1:后置
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/25
		*/
		int removeSlientRecordFile(int p_CamId);
		/*
		*名称: int setFileList(std::string p_startTime, std::string p_stopTime)
		*功能: 触发打包文件列表，待打包完成发送打包完成通知，然后再调用getFileList获取打包的文件信息
		*参数: 
		*	p_startTime:开始时间
		*   p_stopTime:结束时间
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/7
		*/
		int setFileList(std::string p_startTime, std::string p_stopTime);


		/*
		*名称: int getFileList(FilePushInfo &p_fileInfoVec)
		*功能: 获取文件列表信息
		*参数: 
		*   p_fileInfo:  文件列表信息
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/11
		*/
		int getFileList(FilePushInfo &p_fileInfo);

		/*
		*名称: int getLockFileBynameResult(LockFileInfo &p_fileLockInfo)
		*功能: 获取锁定文件结果信息
		*参数: 
		*	p_fileLockInfo: 锁定文件信息
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/11
		*/
		int getLockFileBynameResult(LockFileInfo &p_fileLockInfo);


		/*
		*名称: int getunLockFileBynameResult(LockFileInfo &p_fileLockInfo)
		*功能: 获取非锁定文件结果信息
		*参数: 
		*	p_fileLockInfo: 锁定文件信息
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/11
		*/
		int getunLockFileBynameResult(LockFileInfo &p_fileLockInfo);

		/*
		*名称: int removeFile(const std::string p_FileName)
		*功能: 从数据库中删除指定文件
		*参数: 
		*   p_FileName:  指定文件名
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/19
		*/
		int removeFile(const std::string p_FileName);

		/*
		*名称: int setLogList()
		*功能: 触发行车数据打包
		*参数: 
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int setLogList(const TrafficDataMsg *log);

		/*
		*名称: int getLogList(FilePushInfo &p_fileInfo)
		*功能: 打包log日志
		*参数: 
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/26
		*/
		int getLogList(FilePushInfo &p_fileInfo);

		/*
		*名称: int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId)
		*功能: 获取锁定文件列表
		*参数: 
		*	p_lockfileList:锁定文件列表
		*	p_CamId: 需要检索的摄像头ID号 0:前置 1:车内
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId);

		/*
		*名称: int setRollingOrder(const string p_OrderId, int p_Status)
		*功能: 设置顺风车指令
		*参数:  
		*	p_OrderId: 顺风车订单号 输入
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
		*	p_OrderId: 顺风车订单号 输出
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/6/27
		*/
		int getOrderId(std::string &p_OrderId);

		/*
		*名称: int setQueryRollOrderId(std::string p_OrderId)
		*功能: 设置需要查询的订单号，查询结果通过异步发送
		*参数: 
		*	p_OrderId:	订单号
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/7/13
		*/
		int setQueryRollOrderId(std::string p_OrderId);
		int SetCheckThreadStatus(bool p_start);
		bool WaitCheckThreadStop();

		/*
		*名称: int getDeleteFileList(std::vector<FilePushInfo> &p_fileInfoVec, int p_CamId)
		*功能: 获取即将删除的文件列表
		*参数: 
		*	p_fileInfoVec:	删除的文件列表
		*   p_CamId: 需要检索的摄像头ID号 0:前置 1:车内
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/8/14
		*/
		int getDeleteFileList(std::vector<LockFileInfo> &p_fileInfoVec, int p_CamId);

	private:
		/*
		*名称: int getFileMd5(const string p_FileName, string &p_Md5)
		*功能: 获取文件md5
		*参数: 
		*	p_FileName: 文件名
		*   p_Md5: 文件md5
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/24
		*/
		int getFileMd5(const std::string p_FileName, std::string &p_Md5);

		/*
		*名称: static void *CheckThread(void *context)
		*功能: 解锁时间检测线程
		*参数: 
		*返回值:
		*修改: 创建2018/5/24
		*/
		static void *CheckThread(void *context);

	private:
		Mutex m_mutex;
		MediaFileManager *mMediaFileManager;
		std::string m_picName, m_recordName, m_unLockTime;
		std::vector<std::string> m_FileNameList;
		std::vector<LockFileInfo> m_LockFileVector,m_unLockFileVector;
		pthread_t m_threadId;
		bool m_ThreadExit;
		bool m_OrderIdThread_Exit;
		bool m_ThreadPause;
		bool m_ThreadPauseFinish;
		int m_FileCount;
		std::string m_LockFileName;
		std::string m_UnLockFileName;
		std::map<std::string, std::string> m_LockFileInfo;
		std::map<std::string, std::string> m_LockFileList;
		std::map<std::string, std::string> m_DeleteFileList;
};

#endif
