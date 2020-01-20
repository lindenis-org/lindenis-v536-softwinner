
/*
*版权声明:暂无
*文件名称:AdapterLayer.h
*创建者:陈振华
*创建日期:2018-5-11
*文件描述:本文件主要转发滴滴平台传过来的指令需求
*历史记录:无
*/

#ifndef __ADAPTERLAYER_H__
#define __ADAPTERLAYER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "dd_serv/dd_common.h"
#include "../source/device_model/system/factory_writesn.h"

class AdapterLayer
{
	public:
		AdapterLayer();
		~AdapterLayer();

        /*
		*name: int setTfMounted(int p_Status)
		*description: set tf card mounted flag
		*return:
		*   0:success
		*/
        int setTfMounted(int p_Status);

        /*
		*name: int getTfMounted(void)
		*description: get tf card mounted flag
		*return:
		*   1: mounted
        *   0: not mounted
		*/
        int getTfMounted(void);

	public:
		static AdapterLayer * GetInstance();
		void getDeviceInfo(Device_info &p_DevInfo);
        /*
		*名称: int getDeviceInfo(RemoteDeviceStatusInfo &p_DevInfo)
		*功能: 获取设备状态信息
		*参数: p_DevInfo 
		*	设备信息结果 输出
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int getDeviceInfo(RemoteDeviceStatusInfo &p_DevInfo);

		/*
		*名称: int getUnnormalDeviceInfo(RemoteDeviceAbnormalInfo &p_DevInfo)
		*功能: 获取设备异常时的设备状态信息
		*参数: p_DevInfo 
		*	设备异常时的设备信息结果 输出
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int getUnnormalDeviceInfo(RemoteDeviceAbnormalInfo &p_DevInfo);

		/*
		*名称: int setRollingOrder(const string p_OrderId, int p_Status)
		*功能: 设置顺风车指令
		*参数:  
		*	p_OrderId: 顺风车订单号 输入
		*   p_Status: 订单结束(0)/订单开始(1)
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int setRollingOrder(const std::string p_OrderId, int p_Status);

		/*
		*名称: int getRollingOrderId(string &p_OrderId)
		*功能: 获取顺风车订单号
		*参数:  
		*	p_OrderId: 顺风车订单号 输出
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int getRollingOrderId(std::string &p_OrderId);

        int getRollingOrderIdByName(const std::string &p_FileName, std::string &p_OrderId);

		/*
		*名称: int setDevAttr(int p_FrontRecResolutionIndex, int p_BackRecResolutionIndex, int p_gSensorValue)
		*功能: 设置设备属性
		*参数: 
		*	p_FrontRecResolutionIndex: 前置报像头分辨率
		*	p_BackRecResolutionIndex: 后置摄像头分辨率
		*   p_gSensorValue: gsensor灵敏度值 0:关 1:低 2:中 3:高
		*	设备属性信息 输入
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int setDevAttr(int p_FrontRecResolutionIndex,int p_BackRecResolutionIndex, int p_gSensorValue);

		/*
		*名称: int setDevAttrEx(int p_gSensorValue)
		*功能: 设置设备gsensor属性
		*参数: 
		*   p_gSensorValue: gsensor灵敏度值 0:关 1:低 2:中 3:高
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int setDevAttrEx(int p_gSensorValue);

		/*
		*名称: int setShootAttr(int p_FrontCamEnable, int p_BackCamEnable, int p_AudioType)
		*功能: 设置拍摄属性
		*参数: 
		*	p_FrontCamEnable: 前置摄像头开启(0) /关闭(1)/
		*	p_BackCamEnable:  后置摄像头开启(0) /关闭(1)
		*   p_AudioType: 开(0)/关(1)/单独录音(2)
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int setShootAttr(int p_FrontCamEnable, int p_BackCamEnable, int p_AudioType);

		/*
		*名称: int setSlientPhoto(int p_RecordType, const string p_picName)
		*功能: 设置静默拍照属性
		*参数: 
		*	p_RecordType: 后置摄像头拍摄(0)/前置摄像头拍摄(1)
		*   p_picName: 照片名称
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int setSlientPhoto(int p_RecordType, const std::string p_picName);

		/*
		*名称: int getSlientPic(FilePushInfo &p_Pic)
		*功能: 获取静默拍照的结果
		*参数: 
		*	p_Pic: 结果信息
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int getSlientPic(FilePushInfo &p_Pic);

		/*
		*名称: int getRecordFile(FilePushInfo &p_fileInfo)
		*功能: 获取文件信息
		*参数: 
		*	p_fileInfo: 文件信息结构体
		*返回值:
		*   0:成功
		*   1:失败
		*修改: 创建2018/5/11
		*/
		int getRecordFile(FilePushInfo &p_fileInfo);

		/*
		*名称: int setTriggerFaceOneTime()
		*功能: 设置触发人脸识别( 只触发一次)
		*参数: 无
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setTriggerFaceOneTime(int frame_cnt);

		/*
		*名称: int getFaceDetectResult(FilePushInfo *p_fileInfo)
		*功能: 获取人脸检测结果
		*参数:
		*	p_fileInfo 人脸检测结果结构体 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getFaceDetectResult(FilePushInfo *p_fileInfo);

		/*
		*名称: int setTriggerRecord(int p_CamSelect, int p_AudioSelect, int p_CamResolution, int p_RecTime, const string p_FileName)
		*功能: 设置触发小视频拍摄
		*参数: 
		*	p_CamSelect: 后路摄像头拍摄(0)/前路摄像头拍摄(1)
		*   p_RecTime: 录像时长
		*   p_FileName: 录像文件名称
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/11
		*/
		int setTriggerRecord(int p_CamSelect, int p_AudioSelect, int p_CamResolution, int p_RecTime, const std::string p_FileName);

		/*
		*名称: int setLockFileByTime(const string p_StartTime, const string p_StopTime, const string p_LockTime, int p_CamId)
		*功能: 设置指定时间段里面的文件锁定，并且设置锁定时间
		*参数: 
		*	p_StartTime: 起始时间
		*   p_StopTime: 结束时间
		*   p_LockTime: 锁定时间
		*   p_CamId: camera选择,0是前路，1是后路
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, const std::string p_LockTime, int p_CamId = 1);

		/*
		*名称: int setLockFileByTimeEx(const string p_StartTime, const string p_StopTime, const string p_LockTime, int p_CamId,std::string p_OrderId)
		*功能: 设置指定时间段里面的文件锁定，并且设置锁定时间
		*参数: 
		*	p_StartTime: 起始时间
		*   p_StopTime: 结束时间
		*   p_LockTime: 锁定时间
		*   p_CamId: camera选择,0是前路，1是后路
		*   p_OrderId: 订单号
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setLockFileByTimeEx(const std::string p_StartTime, const std::string p_StopTime, const std::string p_LockTime, int p_CamId = 1, std::string p_OrderId="");

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
		*   p_CamId: camera选择,0是前路，1是后路
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setUnLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, int p_CamId = 1);

		/*
		*名称: int setUnLockFileByTimeEx(const string p_StartTime, const string p_StopTime, int p_CamId, std::string p_OrderId)
		*功能: 解锁指定时间段里面的文件
		*参数: 
		*	p_StartTime: 起始时间
		*   p_StopTime: 结束时间
		*   p_CamId: camera选择,0是前路，1是后路
		*   p_OrderId: 订单号
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setUnLockFileByTimeEx(const std::string p_StartTime, const std::string p_StopTime, int p_CamId = 1, std::string p_OrderId="");

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
		*名称: int setLockFileByTime(const string p_FileName, const string p_LockTime, int p_CamId)
		*功能: 设置指定文件锁定，并且设置锁定时间
		*参数: 
		*	p_FileName: 锁定文件的文件名
		*   p_LockTime: 锁定时间
		*   p_CamId: camera选择,0是前路，1是后路
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setLockFileByName(const std::string p_FileName, const std::string p_LockTime, int p_CamId=1);


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
		*名称: int  setUnLockFileByName(const string p_FileName, int p_CamId)
		*功能: 指定文件解锁
		*参数: 
		*	p_FileName: 解锁文件的文件名
		*   p_CamId: camera选择,0是前路，1是后路
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int setUnLockFileByName(const std::string p_FileName, int p_CamId);

		/*
		*名称: int setRecordAudio(const string p_FileName, int p_RecTime)
		*功能: 设置单独录音
		*参数: 
		*	p_FileName: 录音文件名 输入
		*   p_RecTime:  录音时长
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/11
		*/
		int setRecordAudio(const std::string p_FileName, int p_RecTime);

		/*
		*名称: int getRecordAudioResult(FilePushInfo &p_fileInfo)
		*功能: 获取触发的音频文件结果
		*参数:
		*	p_fileInfo 音频文件结果 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getRecordAudioResult(FilePushInfo &p_fileInfo);

		/*
		*名称: int formatSdCard()
		*功能: 格式化SD卡
		*参数: 无
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/11
		*/
		int formatSdCard();

		/*
		*名称: int setDrivingData()
		*功能: 设置触发行车数据打包
		*参数:
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/6/27
		*/
		int setDrivingData(const TrafficDataMsg *log);

		/*
		*名称: int getDrivingDataResult(FilePushInfo &p_fileInfo)
		*功能: 获取需要上传的行车数据
		*参数:
		*	p_fileInfo 行车数据文件 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getDrivingDataResult(FilePushInfo &p_fileInfo);


		/*
		*名称: int getBasicHWinfo(RemoteBasicHWinfo *p_HWinfo)
		*功能: 获取基本的常用信息
		*参数:
		*	p_HWinfo 常用信息结构体 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getBasicHWinfo(RemoteBasicHWinfo *p_HWinfo);

		/*
		*名称: int getFileKey(const string p_FileName, string &p_key)
		*功能: 获取指定文件的加密秘钥
		*参数:
		*	p_FileName: 文件名 输入
		*   p_key:秘钥 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int getFileKey(const std::string p_FileName, std::string &p_key);

		/*
		*名称: int pushDevStatusInfo(RemoteDeviceStatusInfo *p_devInfo)
		*功能: 主动push设备状态信息到DD平台
		*参数:
		*	p_devInfo: 设备状态信息 输出
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int pushDevStatusInfo(RemoteDeviceStatusInfo *p_devInfo);

		/*
		*名称: int DevAbnormalInfo(RemoteDeviceAbnormalInfo *p_devInfo)
		*功能: 主动push设备异常状态信息到DD平台
		*参数:
		*	p_devInfo: 文件名 输入
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/17
		*/
		int DevAbnormalInfo(RemoteDeviceAbnormalInfo *p_devInfo);

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
		int removeSlientPic(int p_CamId=1);

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
		int removeSlientRecordFile(int p_CamId=1);

		/*
		*名称: int setFileList(std::string p_startTime="", std::string p_stopTime="")
		*功能: 触发打包文件列表，待打包完成发送打包完成通知，然后再调用getFileList获取打包的文件信息
		*参数: 
		*	p_startTime:开始时间
		*   p_stopTime:结束时间
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/7
		*/
		int setFileList(std::string p_startTime="0", std::string p_stopTime="0");

		/*
		*名称: int getFileList(FilePushInfo &p_fileInfoVec)
		*功能: 获取文件列表
		*参数:
		*   p_fileInfoVec:  文件列表信息
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/7
		*/
		int getFileList(FilePushInfo &p_fileInfoVec);

        bool is_base64(unsigned char c);
		/*
		*名称: std::string base64_decode(std::string const& encoded_string)
		*功能: base64 decode
		*参数: 
		*   encoded_string: the decode string
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/8/3
		*/
        std::string base64_decode(std::string const& encoded_string);

		/*
		*名称: int setUserInfo(std::string p_UserName, std::string p_Password)
		*功能: 设置绑定的用户名和密码
		*参数: 
		*   p_UserName:  用户名
		*   p_Password:  密码
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/7
		*/
		int setUserInfo(std::string p_UserName, std::string p_Password);
		
		/*
		*名称: int getUserInfo(std::string &p_UserName, std::string p_Password)
		*功能: 获取绑定的用户名与密码
		*参数: 
		*   p_UserName:  用户名
		*   p_Password:  密码
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/7
		*/
		int getUserInfo(std::string &p_UserName, std::string &p_Password);

		/*
		*名称: int setProductInfo(std::string p_item, std::string p_val)
		*功能: 设置生产信息
		*参数: 
		*   p_item:  名称
		*   p_val:  值
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int setProductInfo(std::string p_item, std::string p_val);
		int setProductInfo(std::string p_item, std::string p_val, int iSave);
		int saveProductInfo();

		/*
		*名称: int getProductInfo(std::string p_item, std::string &p_val)
		*功能: 获取生产信息值
		*参数: 
		*   p_item:  名称
		*   p_val:  值
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int getProductInfo(std::string p_item, std::string &p_val);
		int getProductInfofromflash(std::string p_item, std::string &p_val);

		/*
		*名称: int clearProductInfo()
		*功能: 清空flash
		*参数: 
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/

		int getFactoryInfo(std::string p_item, std::string &p_val);

		
		int clearProductInfo();

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
		*名称: setRecordFileName(const std::string p_FileName)
		*功能: 设置文件的名字
		*参数: 
		*   p_FileName:  指定文件名
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/22
		*/
        int setRecordFileName(const std::string p_FileName);

		/*
		*名称: int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId=1)
		*功能: 当存储空间不足时，通过此接口，获取即将删除的锁定文件列表
		*参数: 
		*   p_lockfileList:锁定文件列表
		*   p_CamId: 需要检索的摄像头ID号 0:前置 1:车内
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId=1);
		/*
		*名称: int setBindFlagInfo(std::string p_BindFlag)
		*功能: 设置绑定标致
		*参数: 
		*   p_BindFlag:绑定标志
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/
		int setBindFlagInfo(std::string p_BindFlag);
		/*
		*名称: int getBindFlagInfo(std::string &p_BindFlag)
		*功能: 设置绑定标致
		*参数: 
		*   p_BindFlag:绑定标志
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/6/27
		*/

		int getBindFlagInfo(std::string &p_BindFlag);

		int notifyMessage(EventReportMsg event);

		/*
		*名称: int getFileList(std::vector<FilePushInfo> &p_fileInfoVec, std::string p_orderId)
		*功能: 根据订单号，获取文件列表
		*参数: 
		*   p_fileInfoVec:  文件列表信息
		*   p_orderId: 订单号
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/7/13
		*/
		int getFileListByRollOrder(std::vector<FilePushInfo> &p_fileInfoVec, std::string p_orderId);

		/*
		*名称: int setQueryRollOrderId(std::string p_OrderId)
		*功能: 设置需要查询的订单号，查询结果通过异步发送，结果通过getFileListByRollOrder获取
		*参数: 
		*	p_OrderId:	订单号
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/7/13
		*/
		int setQueryRollOrderId(std::string p_OrderId);
		//远程唤醒后台指令执行完成设置标志
		int SetRemoteActionDone(int value);

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

        /*
         * Function name    : SaveUpgradeInfo
         * Description      : save upgrade info to secure flash second part
         * Argument         : info : the json string to save
         */
        int SaveUpgradeInfo(std::string info);

        /*
         * Function name    : LoadUpgradeInfo
         * Description      : load upgrade info from secure flash second part
         * Argument         : info : the json string to load
         */
        int LoadUpgradeInfo(std::string &info);

	private:

		/*
		*名称: int getCameraResolution(int p_CamId, std::string &p_Resolution)
		*功能: 获取分辨率
		*参数:
		*	p_CamId:	camdId
		*   p_Resolution: 分辨率
		*返回值:
		*	0:成功
		*	-1:失败
		*修改: 创建2018/8/1
		*/
		int getCameraResolution(int p_CamId, std::string &p_Resolution);

};

#endif
