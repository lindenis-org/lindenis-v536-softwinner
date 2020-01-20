/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file storage_manager.cpp
 * @brief 存储管理类源文件
 * @author id:826
 * @version v0.3
 * @date 2015-11-02
 * @verbatim
    History:
    - 2016-06-07, change for v40 ipc demo
   @endverbatim
 */
 //#define NDEBUG
#include "device_model/version_update_manager.h"
#include "bll_presenter/AdapterLayer.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "common/thread.h"
#include <sys/resource.h>
#include <sys/mount.h>
#include <string.h>
#include <dirent.h>
#include <sstream>
#include "device_model/menu_config_lua.h"
#include "device_model/storage_manager.h"
#include <json/json.h>

#undef LOG_TAG
#define LOG_TAG "VersionUpdateManager"

using namespace EyeseeLinux;
using namespace std;

#define DOWNLOAD_TIMEOUT 600

VersionUpdateManager::VersionUpdateManager()
	:local_file_len(0)
	,m_fp(NULL)
	,curl(NULL)
{
	m_version_str.clear();
	ClearPackData();
	memset(&p_data,0,sizeof(progress_data_));
	dl_file.clear();
	getVersion();
}

VersionUpdateManager::~VersionUpdateManager()
{

}


int VersionUpdateManager::getFileLen(const char* url, double * filelen)
{
#ifdef AES_SUPPORT
	curl = NULL;
	CURLcode res;
	/*  调用curl_easy_init()函数得到 easy interface型指针  */
	curl = curl_easy_init();
	if (curl) {
		/*  调用curl_easy_setopt()设置传输选项 */
		res = curl_easy_setopt(curl, CURLOPT_URL, url);
		if (res != CURLE_OK)
		{
			db_error("set CURLOPT_URL failed.");
			curl_easy_cleanup(curl);
			return -1;
		}
		   struct curl_slist *headers = NULL;
		   headers = curl_slist_append(headers, "postman-token: e9ea62a0-c262-c019-4788-91d1c3e07190");
		   headers = curl_slist_append(headers, "cache-control: no-cache");
		   headers = curl_slist_append(headers, "content-type: application/x-www-form-urlencoded");
		   res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		     if (res != CURLE_OK)
		    {
		        db_error( "Failed to set http header");
			curl_slist_free_all(headers);
			 curl_easy_cleanup(curl);
		        return -1;
		    }
			 //关闭证书的校验
		   res = curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,false);
		    if (res != CURLE_OK)
		    {
		        db_error( "Failed to close ssl  ");
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
		        return -1;
		    }
		res = curl_easy_setopt(curl, CURLOPT_HEADER, true);
		if (res != CURLE_OK)
		{
			db_error("set CURLOPT_HEADER failed.");
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return -1;
		}
		res = curl_easy_setopt(curl, CURLOPT_NOBODY, true);
		if (res != CURLE_OK)
		{
			db_error("set CURLOPT_NOBODY failed.");
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return -1;
		}
		//设置链接超时
		res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
		if (res != CURLE_OK)
		{
			db_error("set CURLOPT_CONNECTTIMEOUT failed.");
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return -1;
		}
		#if 1
		//设置超时时间60s
		res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT);
		if (res != CURLE_OK)
		{
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return -1;
		}
#endif
		res = curl_easy_perform(curl);   // 调用curl_easy_perform()函数完成传输任务
		if(res != CURLE_OK){
			db_error("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			*filelen = -1;
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return -1;
		}else{
			curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD,filelen);
			//*filelen = *filelen /1024/1024;
			}

		/* always cleanup */
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);       // 调用curl_easy_cleanup()释放内存
	}
	return 0;
#else
	return 0;
#endif
}

int VersionUpdateManager::getLocalFileAllLen(char *file_path,long long *filelen)
{
#ifdef AES_SUPPORT
	if(access(file_path,F_OK) != 0){
		db_warn("file : %s  is not exist",file_path);
		*filelen = 0.0;
		return -1;
	}

	int ret = -1;
	FILE *fp_ = NULL;
	fp_ = fopen(file_path,"rb");
	if(fp_ == NULL){
		db_error("open %s  failed",file_path);
		return -1;
	}
	ret = fseek(fp_,0L,SEEK_END);
	if(ret != 0){
		db_error("fseek seekl_end  failed");
		fclose(fp_);
		return -1;
	}
	*filelen = ftell(fp_);
	fclose(fp_);
	return 0;
#else
    return 0;
#endif
}

/*libcurl write callback function */
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	//db_msg("debug_zhb--> write_data    stream = %ld",ftell(stream));
        size_t written = fwrite(ptr, size, nmemb, stream);
        return written;
    }

int VersionUpdateManager::progress_func(char * progress_data,double t, double d,double ultotal,double ulnow)
{
	VersionUpdateManager *m_version = VersionUpdateManager::GetInstance();
	//db_msg("debug_zhb---> progress function :   (%g %%)",d*100.0/t);
	//printf("progress function : d:%f t:%f  (%g %%)",d, t,d*100.0/t);
	m_version->p_data.dltotal = t;
	m_version->p_data.dlnow = d;
	m_version->p_data.percent = (int)d*100.0/t;
	return 0;
}

int VersionUpdateManager::CurlGlobalInit()
{
#ifdef AES_SUPPORT
	CURLcode res;
	 /*   调用curl_global_init()初始化libcurl  */
	res = curl_global_init(CURL_GLOBAL_ALL);
	if (CURLE_OK != res)
	{
		db_error("init libcurl failed.");
		curl_global_cleanup();
		return -1;
	}
	return 0;
#else
    return 0;
#endif
}
int VersionUpdateManager::downLoadFile(const char* url, const std::string outFile)
{
#ifdef AES_SUPPORT
	curl = NULL;
	m_fp = NULL;
	char * progress_data = NULL;
	CURLcode res;
	/*  调用curl_easy_init()函数得到 easy interface型指针  */
	curl = curl_easy_init();
	if (curl) {
  		m_fp = fopen(outFile.c_str(),"ab+");
		if(m_fp == NULL){
			db_error("open %s failed",outFile.c_str());
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		db_msg("debug_zhb---> open m_fp:%p",m_fp);
		/*  调用curl_easy_setopt()设置传输选项 */
		res = curl_easy_setopt(curl, CURLOPT_URL, url);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		//设置发起链接前等待时间
		res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}

		//设置超时时间60s
		res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}

		//设置查找次数
		res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		//设置断点下载
		res = curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, local_file_len);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		/*  根据curl_easy_setopt()设置的传输选项，实现回调函数以完成用户特定任务  */
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		 /*  根据curl_easy_setopt()设置的传输选项，实现回调函数以完成用户特定任务  */
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, m_fp);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
              //set progress bar
              curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
              /*  根据curl_easy_setopt()设置的传输选项，实现回调函数以完成用户特定任务  */
		res = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		 /*  根据curl_easy_setopt()设置的传输选项，实现回调函数以完成用户特定任务  */
		res = curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data);
		if (res != CURLE_OK)
		{
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}

		res = curl_easy_perform(curl);   // 调用curl_easy_perform()函数完成传输任务
		/* Check for errors */
		if(res != CURLE_OK){
			db_error("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			fclose(m_fp);
			m_fp = NULL;
			curl_easy_cleanup(curl);
			curl = NULL;
			return -1;
		}
		/* always cleanup */
		curl_easy_cleanup(curl);       // 调用curl_easy_cleanup()释放内存
		  curl = NULL;
	}
	if( m_fp != NULL)
	{
		db_msg("debug_zhb---> close m_fp:%p",m_fp);
		fclose(m_fp);
		m_fp = NULL;
	}
	db_warn("down version file ok--------------");

	return 0;
#else
    return 0;
#endif
}

void VersionUpdateManager::ColseFb()
{
	if(m_fp != NULL){
		db_msg("debug_zhb---> close m_fp:%p",m_fp);
		fclose(m_fp);
		m_fp = NULL;
	}
}

static long getInfowriter(void *data, int size, int nmemb, void *content)
{
   string temp((char*)data,size * nmemb);
    *((stringstream *)content) << temp <<endl;
    return size * nmemb;
}

int VersionUpdateManager::getVersionUpdateInfo(const char * url,string &info_str)
{
#ifdef AES_SUPPORT
	curl = NULL;
	CURLcode code;
	string error;

	if(url == NULL)
	{
		db_warn("[getVersionUpdateInfo]: the url is NULL return");
		return -1;
	}


	curl = curl_easy_init();
	if(curl == NULL){
		db_error("failed to create curl connection");
		return -1;
	}
   //设置
   code =  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
   if (code != CURLE_OK)
    {
        db_error("Failed to set post [%s]\n", &error);
	curl_easy_cleanup(curl);
        return -1;
    }
   //设置报告每一个意外的事情
   code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
     if (code != CURLE_OK)
    {
        db_error("Failed to set post [%s]\n", &error);
	curl_easy_cleanup(curl);
        return -1;
    }
  //设置PHP 取回url 地址
    code = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (code != CURLE_OK)
    {
        db_error("Failed to set URL [%s]\n", &error);
	curl_easy_cleanup(curl);
        return -1;
    }

   struct curl_slist *headers = NULL;
   headers = curl_slist_append(headers, "postman-token: e9ea62a0-c262-c019-4788-91d1c3e07190");
   headers = curl_slist_append(headers, "cache-control: no-cache");
   headers = curl_slist_append(headers, "content-type: application/x-www-form-urlencoded");
   code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
     if (code != CURLE_OK)
    {
        db_error( "Failed to set http header");
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
        return -1;
    }
	 //关闭证书的校验
   code = curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,false);
    if (code != CURLE_OK)
    {
        db_error( "Failed to close ssl  ");
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
        return -1;
    }
   //设置链接超时
	code = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
	if (code != CURLE_OK)
	{
		db_error("set CURLOPT_CONNECTTIMEOUT failed.");
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return -1;
	}
   //设置接受数据的回调函数
    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getInfowriter);
    if (code != CURLE_OK)
    {
        db_error( "Failed to set writer [%s]\n", &error);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
        return -1;
    }
   //设置接受数据的对象
     stringstream out_str;
    code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_str);
    if (code != CURLE_OK)
    {
        db_error( "Failed to set write data [%s]\n", &error );
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
        return -1;
    }

   //调用函数curl_easy_perform 完成传输
    code = curl_easy_perform(curl);
    if (code != CURLE_OK)
    {
        db_error("curl_easy_perform() failed:  %d   ---- %s\n",code,curl_easy_strerror(code));
	 curl_slist_free_all(headers);
	 curl_easy_cleanup(curl);
        return -1;
    }
    long retcode = 0;
    code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
    if ( (code == CURLE_OK) && retcode == 200 )
    {
        db_warn("%s : %d retcode  :%d",__func__,__LINE__,retcode);
	 info_str = out_str.str().c_str();
	  //db_msg("debug_zhb---> len  = %d  info_str :%s",strlen(info_str.c_str()),info_str.c_str());
        //return 0;
    }
    else
    {
    	db_error("get response code faile");
	  curl_slist_free_all(headers);
	  curl_easy_cleanup(curl);
        return -1;
    }
	curl_slist_free_all(headers);
    	curl_easy_cleanup(curl);
	return 0;
#else
    return 0;
#endif;
}
/*{"message":"test","id":643,"md5Code":"80f2543ff524c9dcb57d8ad44f1f365f","downloadTime":0,"fileName":"V-1.00.09d26_CN.img",
"downloadPath":"http://download.xiaoyi.com/smarthomecam/1.00.09","forceUpdate":"false","code":20000,"version":"1.00.09","needUpdate":"true"};*/
int VersionUpdateManager::ParsPackets(string &data_str)
{
#ifdef AES_SUPPORT
	int start_x = -1 , len_ = -1;
	string temp;
//first pars the needupdate
	string::size_type rc10 = data_str.rfind("needUpdate");///
	if( rc10 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc11 = data_str.rfind('}');
	if( rc11 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	start_x = rc10 +strlen("needUpdate")+3;
	len_ = rc11-1 - start_x;
	temp = data_str.substr(start_x,len_);
	pack_data.needUpdate  = strncmp(temp.c_str(),"false",strlen("false")) == 0 ?0:1;
	db_msg("debug_zhb--> pack_data.needUpdate  = %d",pack_data.needUpdate);

	if(pack_data.needUpdate == 0){
			db_warn("needUpate is false, no need to pars other string");
			return 0;
		}

	string::size_type rc1 = data_str.rfind("message");///
	if( rc1 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	 string::size_type rc2 = data_str.rfind("id");
	if( rc2 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc3 = data_str.rfind("md5Code");
	if( rc3 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc4 = data_str.rfind("downloadTime");
	if( rc4 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc5 = data_str.rfind("fileName");
	if( rc5 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc6 = data_str.rfind("downloadPath");
	if( rc6 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc7 = data_str.rfind("forceUpdate");///
	if( rc7 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc8 = data_str.rfind("code");///
	if( rc8 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc9 = data_str.rfind("version");
	if( rc9 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}



	start_x = rc1 +strlen("message")+3;
	len_ = rc2-3 - start_x;
	pack_data.message  = data_str.substr(start_x,len_ );
	db_msg("debug_zhb--> pack_data.message  = %s",pack_data.message.c_str());

	start_x = rc2 +strlen("id")+2;
	len_ = rc3-1 - start_x;
        pack_data.id= atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_data.id  = %d",pack_data.id);

	start_x = rc3 +strlen("md5Code")+3;
	len_ = rc4-3 - start_x;
	pack_data.md5Code  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.md5Code  = %s",pack_data.md5Code.c_str());

	start_x = rc4 +strlen("downloadTime")+3;
	len_ = rc5-3 - start_x;
	pack_data.downloadTime=atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_data.downloadTime  = %d",pack_data.downloadTime);

	start_x = rc5 +strlen("fileName")+3;
	len_ = rc6-3 - start_x;
	pack_data.fileName  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.fileName  = %s",pack_data.fileName.c_str());
       dl_file.clear();
	dl_file = pack_data.fileName;

	start_x = rc6 +strlen("downloadPath")+3;
	len_ = rc7-3 - start_x;
	pack_data.downloadPath  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.downloadPath  = %s",pack_data.downloadPath.c_str());

	start_x = rc7 +strlen("forceUpdate")+3;
	len_ = rc8-3 - start_x;
	temp = data_str.substr(start_x,len_);
	pack_data.forceUpdate  = strncmp(temp.c_str(),"false",strlen("false")) == 0 ?0:1;
	db_msg("debug_zhb--> pack_data.forceUpdate  = %d",pack_data.forceUpdate);

	start_x = rc8 +strlen("code")+2;
	len_ = rc9-1 - start_x;
	pack_data.code= atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_data.code  = %d",pack_data.code);

	start_x = rc9 +strlen("version")+3;
	len_ = rc10-3 - start_x;
	pack_data.version  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.version  = %s",pack_data.version.c_str());



	//filelen
	string dl_url_str;
	double fileLen;
	getDownLoadFileUrl(dl_url_str);
	getFileLen(dl_url_str.c_str(),&fileLen);
	pack_data.fileLen = fileLen;

	db_msg("debug_zhb--> pack_data.fileLen  = %f ",pack_data.fileLen);
	return 0;
#else
	return 0;
#endif
}

int VersionUpdateManager::CreateTempDir()
{
	DIR* dirptr = opendir(DIR_2CAT(DL_PATH, VERSION_DIR_NET_));
	if(dirptr == NULL)
	{
	    int ret = create_dir(DIR_2CAT(DL_PATH, VERSION_DIR_NET_));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	return 0;
}

int VersionUpdateManager::RemoveTempDir()
{
	pid_t status_;
	char buf[128]={0};
	snprintf(buf,sizeof(buf),"rm -rf %s/%s/",DL_PATH,VERSION_DIR_NET_);
	status_ = system(buf);
	if(-1 == status_){
		db_error("system error");
		return -1;
	}
	if(!WIFEXITED(status_)){
		db_error("error: exit status = [%d]",WIFEXITED(status_));
	 	return -1;
	}
	if(0 != WEXITSTATUS(status_)){
	 	db_error("run shell script  error");
	 	return -1;
	}

	return 0;
}

int VersionUpdateManager::MvVersion2Sdcard()
{
	 StorageManager *sm = StorageManager::GetInstance();
	 sm->setMOccupyDir(true);
	 if (sm->GetStorageStatus() == MOUNTED){
	 	char cmd[128]={0};
	 	char temp[128]={0};

        snprintf(cmd, sizeof(cmd), "rm -rf %s/%s/*", MOUNT_PATH, VERSION_DIR_NET);
        system(cmd);

		snprintf(temp,sizeof(temp),"cp /tmp/net_version/*.img %s/%s/",MOUNT_PATH,VERSION_DIR_NET);
		pid_t status_;
		status_ =  system(temp);
		//status_ =  system("cp /tmp/net_version/*.img /mnt/extsd/net_version/");//copy the version to sdcard
		if(-1 == status_){
			db_error("system error");
			sm->setMOccupyDir(false);
			return -1;
		}
		if(!WIFEXITED(status_)){
			db_error("error: exit status = [%d]",WIFEXITED(status_));
			sm->setMOccupyDir(false);
		 	return -1;
		}
		if(0 != WEXITSTATUS(status_)){
		 	db_error("run shell script  error");
			sm->setMOccupyDir(false);
		 	return -1;
		}
	 }else{
	 		 sm->setMOccupyDir(false);
			return -1;
	 	}
	 sm->setMOccupyDir(false);
	return 0;
}

int VersionUpdateManager::getLoadFileOutPath(string &path_str,bool m_tf)
{
	char buf[128]={0};
	if(m_tf){
		snprintf(buf,sizeof(buf),"%s/%s/%s",MOUNT_PATH,VERSION_DIR_NET,pack_data.fileName.c_str());
	}else
		snprintf(buf,sizeof(buf),"%s%s","/tmp/net_version/",pack_data.fileName.c_str());
	path_str = buf;
	return 0;
}
void VersionUpdateManager::getDlFile(std::string & str)
{
	char buf[128]={0};
	snprintf(buf,sizeof(buf),"%s/%s/%s",MOUNT_PATH,VERSION_DIR_NET,dl_file.c_str());
	str = buf;
}

int  VersionUpdateManager::getDownLoadFileUrl(string &path_str)
{
	char buf[128]={0};
	snprintf(buf,sizeof(buf),"%s%s",pack_data.downloadPath.c_str(),pack_data.fileName.c_str());
	path_str = buf;
	return 0;
}

bool  VersionUpdateManager::getForceUpdate(void)
{
    db_msg("getForceUpdate %d",pack_data.forceUpdate);
    return pack_data.forceUpdate;
}

int  VersionUpdateManager::setForceUpdateFlag(bool flag)
{
    char filename[512] = {0};
    snprintf(filename,sizeof(filename), "%s/%s/%s",MOUNT_PATH,VERSION_DIR_NET, FLAG_FORCEUPDATE);

    if(flag) {
        if(access(filename, F_OK) != 0){
            char tmp[64] = {0};
            int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC);
            if(fd == -1) {
                db_error("create force update flag failed!");
                return -1;
            }
            close(fd);
        }
    }
    else {
        if(access(filename, F_OK) == 0){
            if(remove(filename) == -1) {
                char tmpbuf[512] = {0};
                snprintf(tmpbuf, sizeof(tmpbuf), "rm -f %s", filename);
                system(tmpbuf);
                if(access(filename, F_OK) == 0) {
                    db_error("remove force update flag failed!");
                    return -1;
                }
            }
        }
    }

    return 0;
}

bool VersionUpdateManager::getUpdateStatus()
{
	return pack_data.needUpdate;
}

int  VersionUpdateManager::getVersion()
{
	string version_str;
	::LuaConfig config;
        config.LoadFromFile("/data/menu_config.lua");
	version_str = config.GetStringValue("menu.device.sysversion.version");//V-1.00.12d26_CN
	string::size_type rc = version_str.rfind('-');
	if( rc == string::npos)
	{
		db_error("rfind error");
		return -1;
	}

	string::size_type rc1 = version_str.rfind("u26");
	if( rc1 == string::npos)
	{
		db_error("rfind error");
		return -1;
	}
	m_version_str = version_str.substr(rc+1,rc1-(rc+1));
	db_msg("debug_zhb---> m_version_str = %s",m_version_str.c_str());
	return 0;
}

void VersionUpdateManager::getRequestUrl(string & str)
{
	string h_str;
	h_str.clear();
	if(m_version_str.empty())
		getVersion();
	if(access(URL_TEST, F_OK) != 0)
		h_str = "https://fleet-ota.xiaoyi.com/vmanager/upgrade?uid=xxxxxxxxxxxxxxxxxx&protocol=micn&sname=didi-c26a&version=";
	else
		h_str = "https://fleet-ota.xiaoyi.com/vmanager/upgrade?uid=xxxxxxxxxxxxxxxxxx&protocol=micn&sname=didi-c26a-test&version=";
	h_str+=m_version_str;
	str = h_str;
	db_warn("Url:%s",str.c_str());
}

void VersionUpdateManager::getProgressData(double * dltotal , double * dlNow)
{
	*dltotal = p_data.dltotal;
	*dlNow = p_data.dlnow;
}

int VersionUpdateManager::clearProgressPercent(void)
{
    p_data.dltotal = 0;
    p_data.dlnow = 0;
    return 0;
}

int VersionUpdateManager::getProgressPercent(void)
{
    if(p_data.dltotal <= 0 || p_data.dlnow <= 0) {
        return 0;
    }
    return (int)(p_data.dlnow * 100.0 / p_data.dltotal);
}

double VersionUpdateManager::getVersionPacketLen()
{

	return pack_data.fileLen;
}


const std::string  VersionUpdateManager::getVersionPacketMd5Code()
{
	return pack_data.md5Code;
}

const std::string  VersionUpdateManager::getVersionFileName()
{
	return pack_data.fileName;
}
bool VersionUpdateManager::Md5CheckVersionPacket(bool m_tf)
{
    string p_path;
    FILE *ptr = NULL;
    char buf_ps[128]={0};
    char md5str[128]= {0};
    char temp[128]={0};;
    getLoadFileOutPath(p_path,m_tf);
    snprintf(temp,sizeof(temp),"md5sum %s",p_path.c_str());
    if((ptr = popen(temp,"r"))!= NULL)
    {
        while(fgets(buf_ps, 128, ptr) !=NULL)
        {

            if(strstr(buf_ps,p_path.c_str())== NULL)
                continue;
            char *saveptr = strstr(buf_ps," ");
            memset(md5str, 0, sizeof(md5str));
            strncpy(md5str, buf_ps, saveptr-buf_ps);
	     db_msg("debug_zhb--->md5str = %s ",md5str);
            break;
        }
        pclose(ptr);
    }
	const string md5Code = getVersionPacketMd5Code();
	 db_msg("debug_zhb--->md5Code = %s ",md5Code.c_str());
	if(strncmp(md5Code.c_str(),md5str,strlen(md5Code.c_str())) == 0)
		return true;

    	return false;
}


void VersionUpdateManager::ClearPackData()
{
	pack_data.message.clear();
	pack_data.id = -1;
	pack_data.md5Code.clear();
	pack_data.downloadTime = -1;
	pack_data.fileName.clear();
	pack_data.downloadPath.clear();
	pack_data.forceUpdate = false;
	pack_data.code = -1;
	pack_data.version.clear();
	pack_data.needUpdate = false;
	pack_data.fileLen = 0.0;
}
