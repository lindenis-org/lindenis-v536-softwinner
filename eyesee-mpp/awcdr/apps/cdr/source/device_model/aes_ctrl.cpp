/*
 * *版权声明:暂无
 * *文件名称:aes_ctrl.h
 * *创建者:陈振华
 * *创建日期:2018-5-30
 * *文件描述:本文件主要负责对文件进行加密
 * *历史记录:无
 * */

#include "aes_ctrl.h"
#include "common/app_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "backCameraCheck.cpp"
#endif

using namespace EyeseeLinux;
using namespace std;

static Mutex AesCtrl_mutex;
static AesCtrl *m_AesCtrl = NULL;

AesCtrl::AesCtrl()
{
	m_user_key.clear();
	bzero(m_aes_key, sizeof(m_aes_key));
	bzero(m_aes_key_peer, sizeof(m_aes_key_peer));
	bzero(m_ivec, sizeof(m_ivec));
	bzero(m_ivec_use, sizeof(m_ivec_use));
	bzero(m_ivec_peer, sizeof(m_ivec_peer));

}

AesCtrl::~AesCtrl()
{

}

AesCtrl* AesCtrl::GetInstance(void)
{
	Mutex::Autolock _l(AesCtrl_mutex);
	if( m_AesCtrl != NULL )
		return m_AesCtrl;

	m_AesCtrl = new AesCtrl();
	if( m_AesCtrl != NULL)
		return m_AesCtrl;
	else
	{
		db_error("new AesCtrl failed\n");
		return NULL;
	}
}

int AesCtrl::aes_encrypt( std::string in_file_path,std::string out_file_path)
{
	Mutex::Autolock _l(m_mutex);

	FILE *fin, *fout;
	fin = fopen(in_file_path.c_str(), "r+");
	if(fin == NULL )
	{
		printf("fopen %s failed\n",in_file_path.c_str());
		return 0;
	}

	fout = fopen(out_file_path.c_str(), "w+");
	if( fout == NULL )
	{
		printf("fopen %s failed\n",out_file_path.c_str());
		fclose(fin);
		remove(in_file_path.c_str());
		return 0;
	}

	char command[1024] = {0};
#if FIXED_KEY_ENCODE
    snprintf(command, sizeof(command),"openssl enc -aes-256-cbc -in %s -out %s -K 3030303030303030303030303030303030303030303030303030303030303030 -iv 32323232323232323232323232323232", in_file_path.c_str(),out_file_path.c_str());
#else
	snprintf(command, sizeof(command),"openssl enc -aes-256-cbc -in %s -out %s -K %s -iv %s", in_file_path.c_str(),out_file_path.c_str(),m_aes_key, m_ivec_use);
#endif

	system(command);
#if 0
	AES_KEY aes;
	if(AES_set_encrypt_key((unsigned char *)m_aes_key, AES_BITS, &aes) < 0)
		return 0;

	char *in_data  = new char[CHUNK_SIZE + 1];
	char *out_data = new char[CHUNK_SIZE + 1];

	bzero(in_data, CHUNK_SIZE + 1);
	bzero(out_data, CHUNK_SIZE + 1);

	while(!feof(fin))
	{
		bzero(in_data, CHUNK_SIZE + 1);
		bzero(out_data, CHUNK_SIZE + 1);
		fread(in_data, 1, CHUNK_SIZE, fin);
		AES_cbc_encrypt((unsigned char *)in_data, (unsigned char *)out_data, CHUNK_SIZE, &aes, m_ivec, AES_ENCRYPT);
		fwrite(out_data, 1, CHUNK_SIZE, fout);
	}

	if(in_data != NULL )
	{
		delete in_data;
		in_data = NULL;
	}

	if( out_data != NULL )
	{
		delete out_data;
		out_data = NULL;
	}
#endif

	fclose(fin);
	fclose(fout);

	remove(in_file_path.c_str());

	return 1;
}

int AesCtrl::aes_decrtpt( std::string in_file_path,std::string out_file_path)
{
	return 0;
#if 0
	Mutex::Autolock _l(m_mutex);

	FILE *fin, *fout;
	fin = fopen(in_file_path.c_str(), "r+");
	if(fin == NULL )
	{
		printf("fopen %s failed\n",in_file_path.c_str());
		return 0;
	}

	fout = fopen(out_file_path.c_str(), "w+");
	if( fout == NULL )
	{
		printf("fopen %s failed\n",out_file_path.c_str());
		return 0;
	}
	bzero(m_ivec, sizeof(m_ivec));
	memset(m_ivec, 'b', sizeof(m_ivec)-1);

	AES_KEY aes;
	if(AES_set_decrypt_key((unsigned char *)m_aes_key, AES_BITS, &aes) < 0)
		return 0;

	char *in_data  = new char[CHUNK_SIZE + 1];
	char *out_data = new char[CHUNK_SIZE + 1];

	while(!feof(fin))
	{
		bzero(in_data, CHUNK_SIZE + 1);
		bzero(out_data, CHUNK_SIZE + 1);

		fread(in_data, 1, CHUNK_SIZE, fin);
		AES_cbc_encrypt((unsigned char *)in_data, (unsigned char *)out_data, CHUNK_SIZE, &aes, m_ivec, AES_DECRYPT);
		fwrite(out_data, 1, CHUNK_SIZE, fout);
	}

	if(in_data != NULL )
	{
		delete []in_data;
		in_data = NULL;
	}

	if( out_data != NULL )
	{
		delete []out_data;
		out_data = NULL;
	}
	fclose(fin);
	fclose(fout);

	remove(in_file_path.c_str());
	return 0;
#endif
}

int AesCtrl::setUserKey( std::string Key)
{
	Mutex::Autolock _l(m_mutex);

	m_user_key = Key;
	generateAesKey();

	return 0;
}

int AesCtrl::generateAesKey()
{
#ifdef AES_SUPPORT
	unsigned char iv[IV_SIZE];
	for(int i =0;i<IV_SIZE;i++)
	{
		iv[i] = rand()%9;
	}

	for(int i = 0; i< IV_SIZE; i++)
	{
#if FIXED_KEY_ENCODE
        m_ivec_peer[i] = '2';
#else
        m_ivec_peer[i] = 0x30 + (iv[i] % 10);
#endif

		sprintf((char *)(&m_ivec[2*i]), "%02x", m_ivec_peer[i]);
		sprintf((char *)(&m_ivec_use[2*i]), "%02x", m_ivec_peer[i]);
	}

	char source[AES_BLOCK_SIZE+1] = "1234567890123456";
	AES_KEY aes;
	bzero(m_aes_key, sizeof(m_aes_key));
	bzero(m_aes_key_peer, sizeof(m_aes_key_peer));
	if(AES_set_encrypt_key((unsigned char *)source, AES_BITS, &aes) < 0)
		return 0;

	char *out_data = (char *)malloc(CHUNK_SIZE);
	bzero(out_data, CHUNK_SIZE);

	AES_cbc_encrypt((unsigned char *)m_user_key.c_str(), (unsigned char *)out_data, CHUNK_SIZE, &aes, iv, AES_ENCRYPT);
	for(int i = 0; i< CHUNK_SIZE/2; i++)
	{
        if(out_data[i] < 0x61) {
            out_data[i] = 0x30 +  out_data[i] % 10;
        }
        else {
            out_data[i] = 0x61 +  out_data[i] % 26;
        }
#if FIXED_KEY_ENCODE
        m_aes_key_peer[i] = '0';
#else
        m_aes_key_peer[i] = out_data[i];
#endif
		sprintf(&m_aes_key[2*i], "%02x", m_aes_key_peer[i]);
	}

	if( out_data != NULL )
	{
		free(out_data);
		out_data = NULL;
	}

	return 1;
#else
    return 0;
#endif
}

int  AesCtrl::getKey( std::string &Key)
{
	Mutex::Autolock _l(m_mutex);
	Key.clear();

	char buf[128] = {0};
	strncpy(buf, m_aes_key_peer, strlen(m_aes_key_peer));
	buf[strlen(m_aes_key_peer)] = '_';
	strncpy(buf+strlen(m_aes_key_peer) + 1, (char *)m_ivec_peer, strlen((char *)m_ivec_peer));
	Key = buf;

	return 1;
}
