#pragma once

#include "common/message.h"
#include "common/subject.h"
#include "common/singleton.h"
#include "common/common_inc.h"
#include <curl/curl.h> 
#include <list>
#include <thread>
#define DL_PATH     "/tmp"
#define VERSION_4G_DIR_NET_ "net_4g_version"
#define URL_TEST_4G       "/mnt/extsd/.tmp/.t/.4GuRlTeSt"

namespace EyeseeLinux {
typedef struct _pack_4g_data_{
	std::string message;
	int id;
	std::string md5Code;
	int downloadTime;
	std::string fileName;
	std::string downloadPath;
	bool forceUpdate;
	int code;
	std::string version;
	bool needUpdate;
	double fileLen;
}pack_4g_data_;

class DownLoad4GManager
    : public Singleton<DownLoad4GManager>
    , public ISubjectWrap(DownLoad4GManager)
{
    friend class Singleton<DownLoad4GManager>;
    public:
	DownLoad4GManager();
	~DownLoad4GManager();
	int downLoadFile(const char* url, const std::string outFile);
	int getVersionUpdateInfo(const char *url,std::string &info_str);
	int ParsPackets(std::string &data_str);
	int getLoadFileOutPath(std::string &path_str,bool m_tf);
	int getDownLoadFileUrl(std::string &path_str);
	void getRequestUrl(std::string & str);
	bool getUpdateStatus();
	int getFileLen(const char* url, double * filelen);
	double getVersionPacketLen();
	const std::string getVersionPacketMd5Code();
	const std::string getVersionFileName();
	bool Md5CheckVersionPacket(bool m_tf);
	void ClearPackData();
	int getLocalFileAllLen(char *file_path,long long *filelen);
	void setLocalFileLen(long long val){local_file_len = val;}
	long long getLocalFileLen(){return local_file_len;}
	void ColseFb();
	void getDlFile(std::string & str);
	int CreateTempDir();
	int RemoveTempDir();
	int MvVersion2Sdcard();
	 
private:
	pack_4g_data_ pack_4g_data;
	long long local_file_len;
	FILE *m_fp;  
	std::string dl_file;
	CURL *curl; 
	
}; 

}
