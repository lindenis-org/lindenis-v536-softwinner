#pragma once

#include "common/message.h"
#include "common/subject.h"
#include "common/singleton.h"
#include "common/common_inc.h"
#include "dd_serv/dd_common.h"
#include <curl/curl.h>
#include <list>
#include <thread>
#define DL_PATH     "/tmp"
#define VERSION_DIR_NET_ "net_version"
#define URL_TEST       "/mnt/extsd/.tmp/.t/.uRlTeSt"
#define FLAG_FORCEUPDATE    ".forceupdate"


namespace EyeseeLinux {
typedef struct _pack_data_{
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
}pack_data_;

typedef struct _progress_data_
{
	double dltotal;
	double dlnow;
    int percent;
}progress_data_;

class VersionUpdateManager
    : public Singleton<VersionUpdateManager>
    , public ISubjectWrap(VersionUpdateManager)
{
    friend class Singleton<VersionUpdateManager>;
    public:
	VersionUpdateManager();
	~VersionUpdateManager();
	int downLoadFile(const char* url, const std::string outFile);
	int getVersionUpdateInfo(const char *url,std::string &info_str);
	int ParsPackets(std::string &data_str);
	int getLoadFileOutPath(std::string &path_str,bool m_tf);
	int getDownLoadFileUrl(std::string &path_str);
	void getRequestUrl(std::string & str);
	bool getUpdateStatus();
	void getProgressData(double * dltotal , double * dlNow);
    int clearProgressPercent(void);
    int getProgressPercent(void);
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
	int CurlGlobalInit();
    bool getForceUpdate(void);
    int setForceUpdateFlag(bool flag);
private:
	 static int  progress_func(char * progress_data,double t, double d,double ultotal,double ulnow);
	 int getVersion();

private:
	pack_data_ pack_data;
	progress_data_ p_data;
	std::string m_version_str;
	long long local_file_len;
	FILE *m_fp;
	std::string dl_file;
	CURL *curl;

};

}
