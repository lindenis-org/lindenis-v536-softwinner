/*
*版权声明:暂无
*文件名称:aes_ctrl.h
*创建者:陈振华
*创建日期:2018-5-30
*文件描述:本文件主要负责对文件进行加密
*历史记录:无
*/
#ifndef __AES_CTRL_H__
#define __AES_CTRL_H__
	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/subject.h"
#include <utils/Mutex.h>
#include <openssl/aes.h>

using namespace EyeseeLinux;

#define AES_BITS 256
#define BLOCK_SIZE 32
#define CHUNK_SIZE 32
#define IV_SIZE 16
#define FIXED_KEY_ENCODE (0)

class AesCtrl
{
	public:
		AesCtrl();
		~AesCtrl();

	public:
		static AesCtrl* GetInstance(void);
		
		/*
		*名称: int aes_decrtpt(std::string in_file_path, std::string out_file_path)
		*功能: 对输入的文件进行解密，输出解决后的文件
		*参数:
		*	in_file_path: 需要加密的文件名(包含绝对路径)
		*   out_file_path: 加密后的文件(包含绝对路径)
		*返回值:
		*	1:成功
		*	0:失败
		*修改: 创建2018/5/30
		*/
		int aes_decrtpt(std::string in_file_path, std::string out_file_path);

		/*
		*名称: int aes_encrypt(std::string in_file_path, std::string out_file_path)
		*功能: 对输入的文件进行加密，输出加密后的文件
		*参数:
		*	in_file_path: 需要加密的文件名(包含绝对路径)
		*   out_file_path: 加密后的文件(包含绝对路径)
		*返回值:
		*	1:成功
		*	0:失败
		*修改: 创建2018/5/30
		*/
		int aes_encrypt(std::string in_file_path, std::string out_file_path);

		/*
		*名称: int setUserKey(std::string Key)
		*功能: 输入用户key,key包括32位随机码+imei组成。
		*参数:
		*	Key: 用户key
		*返回值:
		*	0:成功
		*	1:失败
		*修改: 创建2018/5/30
		*/
		int setUserKey(std::string Key);

		/*
		*名称: int getKey(std::string &Key)
		*功能: 获取文件加密key
		*参数:
		*	Key: 用户key
		*返回值:
		*	1:成功
		*	0:失败
		*修改: 创建2018/5/30
		*/
		int getKey(std::string &Key);

	private:
		int generateAesKey();

	private:
		std::string m_user_key;
		unsigned char m_ivec[IV_SIZE*2+1];
		unsigned char m_ivec_use[IV_SIZE*2+1];
		unsigned char m_ivec_peer[IV_SIZE*2+1];
		char m_aes_key[CHUNK_SIZE+1];
		char m_aes_key_peer[CHUNK_SIZE+1];
		Mutex m_mutex;		
};

#endif
