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
#include "device_model/download_4g_manager.h"
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
#include "device_model/system/event_manager.h"


#undef LOG_TAG
#define LOG_TAG "DownLoad4GManager"
#define TIME_OUT  600
using namespace EyeseeLinux;
using namespace std;

DownLoad4GManager::DownLoad4GManager()
	:local_file_len(0)
	,m_fp(NULL)
	,curl(NULL)
{
	ClearPackData();
	dl_file.clear();
}

DownLoad4GManager::~DownLoad4GManager()
{

}


int DownLoad4GManager::getFileLen(const char* url, double * filelen)
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

		//设置超时时间60s
		res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIME_OUT);                    
		if (res != CURLE_OK)  
		{     
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);    
			return -1;  
		}  

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

int DownLoad4GManager::getLocalFileAllLen(char *file_path,long long *filelen)
{

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
}

/*libcurl write callback function */ 
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {  
	//db_msg("debug_zhb--> write_data    stream = %ld",ftell(stream));
        size_t written = fwrite(ptr, size, nmemb, stream);  
        return written;  
    } 

int DownLoad4GManager::downLoadFile(const char* url, const std::string outFile)
{  
#ifdef AES_SUPPORT
	curl = NULL; 
	m_fp = NULL;  
	CURLcode res;  
	char * progress_data = NULL;
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
		res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIME_OUT);                    
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
	db_warn("download 4G version  file ok--------------");
	
	return 0;  
#else
    return 0;
#endif
}  

void DownLoad4GManager::ColseFb()
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

int DownLoad4GManager::getVersionUpdateInfo(const char * url,string &info_str)
{
#ifdef AES_SUPPORT
	curl = NULL;
	CURLcode code;
	string error; 
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
        db_msg("debug_zhb---> retcode  :%d",retcode);
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
#endif
}
/*{"message":"test","id":643,"md5Code":"80f2543ff524c9dcb57d8ad44f1f365f","downloadTime":0,"fileName":"V-1.00.09d26_CN.img",
"downloadPath":"http://download.xiaoyi.com/smarthomecam/1.00.09","forceUpdate":"false","code":20000,"version":"1.00.09","needUpdate":"true"};*/
int DownLoad4GManager::ParsPackets(string &data_str)
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
	pack_4g_data.needUpdate  = strncmp(temp.c_str(),"false",strlen("false")) == 0 ?0:1; 
	db_msg("debug_zhb--> pack_data.needUpdate  = %d",pack_4g_data.needUpdate);

	if(pack_4g_data.needUpdate == 0){
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
	pack_4g_data.message  = data_str.substr(start_x,len_ );
	db_msg("debug_zhb--> pack_4g_data.message  = %s",pack_4g_data.message.c_str());
	
	start_x = rc2 +strlen("id")+2;
	len_ = rc3-1 - start_x;
        pack_4g_data.id= atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_4g_data.id  = %d",pack_4g_data.id);

	start_x = rc3 +strlen("md5Code")+3;
	len_ = rc4-3 - start_x;
	pack_4g_data.md5Code  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_4g_data.md5Code  = %s",pack_4g_data.md5Code.c_str());

	start_x = rc4 +strlen("downloadTime")+3;
	len_ = rc5-3 - start_x;
	pack_4g_data.downloadTime=atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_4g_data.downloadTime  = %d",pack_4g_data.downloadTime);

	start_x = rc5 +strlen("fileName")+3;
	len_ = rc6-3 - start_x;
	pack_4g_data.fileName  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.fileName  = %s",pack_4g_data.fileName.c_str());
       dl_file.clear();
	dl_file = pack_4g_data.fileName;
	
	start_x = rc6 +strlen("downloadPath")+3;
	len_ = rc7-3 - start_x;
	pack_4g_data.downloadPath  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_data.downloadPath  = %s",pack_4g_data.downloadPath.c_str());

	start_x = rc7 +strlen("forceUpdate")+3;
	len_ = rc8-3 - start_x;
	temp = data_str.substr(start_x,len_); 
	pack_4g_data.forceUpdate  = strncmp(temp.c_str(),"false",strlen("false")) == 0 ?0:1;
	db_msg("debug_zhb--> pack_4g_data.forceUpdate  = %d",pack_4g_data.forceUpdate);

	start_x = rc8 +strlen("code")+2;
	len_ = rc9-1 - start_x;
	pack_4g_data.code= atoi((data_str.substr(start_x,len_)).c_str());
	db_msg("debug_zhb--> pack_4g_data.code  = %d",pack_4g_data.code);

	start_x = rc9 +strlen("version")+3;
	len_ = rc10-3 - start_x;
	pack_4g_data.version  = data_str.substr(start_x,len_);
	db_msg("debug_zhb--> pack_4g_data.version  = %s",pack_4g_data.version.c_str());

	

	//filelen
	string dl_url_str;
	double fileLen;
	getDownLoadFileUrl(dl_url_str);
	getFileLen(dl_url_str.c_str(),&fileLen);
	pack_4g_data.fileLen = fileLen;

	db_msg("debug_zhb--> pack_4g_data.fileLen  = %f ",pack_4g_data.fileLen);
	return 0;
#else
    return 0;
#endif
}

int DownLoad4GManager::CreateTempDir()
{
	DIR* dirptr = opendir(DIR_2CAT(DL_PATH, VERSION_4G_DIR_NET_));
	if(dirptr == NULL)
	{
	    int ret = create_dir(DIR_2CAT(DL_PATH, VERSION_4G_DIR_NET_));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	return 0;
}

int DownLoad4GManager::RemoveTempDir()
{
	pid_t status_;
	char buf[128]={0};
	snprintf(buf,sizeof(buf),"rm -rf %s/%s/",DL_PATH,VERSION_4G_DIR_NET_);
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

int DownLoad4GManager::MvVersion2Sdcard()
{
	 StorageManager *sm = StorageManager::GetInstance();
	 sm->setMOccupyDir(true);
	 if (sm->GetStorageStatus() == MOUNTED){
	 	char temp[128]={0};
		snprintf(temp,sizeof(temp),"cp /tmp/net_4g_version/*.bin %s/%s/",MOUNT_PATH,VERSION_4G_DIR_NET);
		pid_t status_;
		status_ =  system(temp);
		//status_ =  system("cp /tmp/net_4g_version/*.bin /mnt/extsd/net_4g_version/");//copy the version to sdcard
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

int DownLoad4GManager::getLoadFileOutPath(string &path_str,bool m_tf)
{
	char buf[128]={0};
	if(m_tf){
		snprintf(buf,sizeof(buf),"%s/%s/%s",MOUNT_PATH,VERSION_4G_DIR_NET,pack_4g_data.fileName.c_str());
	}else
		snprintf(buf,sizeof(buf),"%s%s","/tmp/net_4g_version/",pack_4g_data.fileName.c_str());
	path_str = buf;
	db_msg("debug_zhb--->4G  getLoadFileOutPath : %s",buf);
	return 0;
}
void DownLoad4GManager::getDlFile(std::string & str)
{
	char buf[128]={0};
	sprintf(buf,"%s/%s/%s",MOUNT_PATH,VERSION_4G_DIR_NET,dl_file.c_str());
	str = buf;
	db_msg("debug_zhb--->4G  getDlFile : %s",buf);
}

int  DownLoad4GManager::getDownLoadFileUrl(string &path_str)
{
	char buf[128]={0};
	snprintf(buf,sizeof(buf),"%s%s",pack_4g_data.downloadPath.c_str(),pack_4g_data.fileName.c_str());
	path_str = buf;
	db_msg("debug_zhb--->4G  getDownLoadFileUrl : %s",buf);
	return 0;
}

bool DownLoad4GManager::getUpdateStatus()
{
	return pack_4g_data.needUpdate;
}

void DownLoad4GManager::getRequestUrl(string & str)
{
	EventManager* ev = EventManager::GetInstance();
	string h_str;
	h_str.clear();
	if(access(URL_TEST_4G, F_OK) != 0)
		h_str = "https://fleet-ota.xiaoyi.com/vmanager/upgrade?uid=xxxxxxxxxxxxxxxxxx&protocol=micn&sname=didi-c26-4G&version=";
	else
		h_str = "https://fleet-ota.xiaoyi.com/vmanager/upgrade?uid=xxxxxxxxxxxxxxxxxx&protocol=micn&sname=didi-c26-4G-test&version=";
	if(ev->m_4g_version.empty())
		ev->m_4g_version = "1.00.15";
		h_str+=ev->m_4g_version.c_str();
	str = h_str;
	db_warn("Url:%s",str.c_str());
}

double DownLoad4GManager::getVersionPacketLen()
{
	return pack_4g_data.fileLen;
}


const std::string  DownLoad4GManager::getVersionPacketMd5Code()
{
	return pack_4g_data.md5Code;
}

const std::string  DownLoad4GManager::getVersionFileName()
{
	return pack_4g_data.fileName;
}
bool DownLoad4GManager::Md5CheckVersionPacket(bool m_tf)
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


void DownLoad4GManager::ClearPackData()
{
	pack_4g_data.message.clear();
	pack_4g_data.id = -1;
	pack_4g_data.md5Code.clear();
	pack_4g_data.downloadTime = -1;
	pack_4g_data.fileName.clear();
	pack_4g_data.downloadPath.clear();
	pack_4g_data.forceUpdate = false;
	pack_4g_data.code = -1;
	pack_4g_data.version.clear();
	pack_4g_data.needUpdate = false;
	pack_4g_data.fileLen = 0.0;
}
