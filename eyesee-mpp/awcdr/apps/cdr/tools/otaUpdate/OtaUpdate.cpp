#include "OtaUpdate.h"
#include <unistd.h>
#include "app_log.h"
#include <string.h>
#include "ota_private.h"
#include<iostream>
#include<stdio.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<cstring>
#include<stdlib.h>
#include<vector>
#include"ota_common.h"

static HWND MainWnd;
int  getSdcardVersion(std::vector<std::string> &p_FileNameList,std::string &path_str)
{
	int ret = -1;
	std::string filepath = path_str;
	DIR *sdcard_dir = NULL;
	struct dirent *dirp;

	if((sdcard_dir = opendir(filepath.c_str())) == NULL)
	{
	   db_msg("opendir fail");
	   return ret;
	}
	while((dirp = readdir(sdcard_dir)) != NULL)
	{
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;
		int size = strlen(dirp->d_name);
		if(strcmp((dirp->d_name+( size - 4)),".img")!=0)
			continue;
	   p_FileNameList.push_back(dirp->d_name);
	   ret = 0;
	}
	    closedir(sdcard_dir);
	return ret;
}

bool Md5CheckVersionPacket(std::string p_path,std::string md5Code,std::string path_str)
{
    FILE *ptr = NULL;
    char buf_ps[128]={0};
    char md5str[128]= {0};
    char temp[128]={0};;
    sprintf(temp,"md5sum %s%s",path_str.c_str(),p_path.c_str());
    db_warn("md5check command : %s",temp);
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
	 db_msg("debug_zhb--->md5Code = %s ",md5Code.c_str());
	if(strncmp(md5Code.c_str(),md5str,strlen(md5Code.c_str())) == 0)
		return true;

    	return false;
}
bool CheckVersionMd5IsOk(std::vector<std::string> &p_FileNameList,std::string path_v)
{
	bool ret_flag = false;
	//check md5
	for (unsigned int i=0; i<p_FileNameList.size() && p_FileNameList.size() < 2; i++)
	{
		std::string::size_type rc_cn_ = p_FileNameList[i].rfind("CN_");
		if( rc_cn_ == std::string::npos)
		{
            rc_cn_ = p_FileNameList[0].rfind("EN_");//
            if( rc_cn_ == std::string::npos)
            {
			    db_warn("invalid fileName:%s",p_FileNameList[i].c_str());
                continue;
            }
			
			
		}
		std::string md5_str;
        std::string::size_type rc_cn_debug = p_FileNameList[0].rfind("debug_");
        if(rc_cn_debug == std::string::npos)
        {
            //no debug str
           md5_str = p_FileNameList[0].substr(rc_cn_+strlen("CN_"));
        }
        else
        {
            //_debug 
            md5_str = p_FileNameList[0].substr(rc_cn_+strlen("CN_debug_"));
        }
		char temp[128]={0};
		if(strncpy(temp,md5_str.c_str(),32) == NULL)
		{
			db_warn("strncpy md5 str fail");
			continue;
		}
		db_warn("get the md5 str form version name : %s",temp);
		md5_str.clear();
		md5_str = temp;
		//check md5
		if(Md5CheckVersionPacket(p_FileNameList[i],md5_str,path_v) == false)
		{
			db_warn("check the version	md5  fail");
			continue;
		}
		ret_flag = true;
	}

	return ret_flag;
}

extern int ota_main(char *image_path, update_part_flag ota_partition_flag);

static void *otaUpdatePkgThread(void *ptx)
{
    std::string path_str =  (char*)ptx;
    update_part_flag part_flg;
    memset(&part_flg, 0, sizeof(update_part_flag));
    part_flg.update_boot_logo_flag = 1;
    part_flg.update_env_flag       = 1;
    part_flg.update_kernel_flag    = 1;
    part_flg.update_rootfs_flag    = 1;
    part_flg.update_uboot_flag     = 1;
    part_flg.update_overlay_flag   = 0;
    part_flg.update_custom_flag    = 1;
    //add by zhb
	std::vector<std::string> p_FileNameList;
	p_FileNameList.clear();
	if(getSdcardVersion(p_FileNameList,path_str) < 0){
		db_msg("get sdcard version info fail");
        	SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_FOUND_IMG_FAIL,0);
		return NULL;
	}
#if 0
	if(CheckVersionMd5IsOk(p_FileNameList,path_str) == false)
	{
		db_msg("CheckVersionMd5IsOk fail");
        	SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_MD5_CHECK_FAIL,0);
		return NULL;
	}
#endif
	//kill sdvcam
	system("kill -9 $(pidof sdvcam)");
	char filepath[128]={0};
	for (unsigned int i=0; i<p_FileNameList.size(); i++)
	{
		sprintf(filepath,"%s%s",path_str.c_str(),p_FileNameList[i].c_str());
	}
	//end add by zhb
    //int ret = ota_main((char *)UPDATE_FILE_PATH, part_flg);

    int ret = ota_main(filepath, part_flg);
    if (ret < 0)
    {
        db_error("error upgrade fail exit");
        SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_UPDATE_FAIL,0);
       // kill(getpid(),SIGKILL);
       return NULL;
    }

    SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_UPDATE_SUCCESS,0);

    return NULL;
}


OtaUpdate::OtaUpdate(HWND hwnd){
    mHwnd = hwnd;
    MainWnd = mHwnd;
}

int OtaUpdate::startUpdatePkg(char *path_str)
{
	db_msg("debug_zhb---->OtaUpdate-- str = %s",path_str);

    pthread_t thread_id = 0;
    pthread_create(&thread_id, NULL, otaUpdatePkgThread, path_str);

    return 0;
}
