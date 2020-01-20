#include "update4GModule.h"
#include <unistd.h>
#include "app_log.h"
#include <string.h>
#include<iostream>
#include<stdio.h>
#include<unistd.h>
#include<dirent.h>
#include<cstring>
#include<stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/prctl.h>
#include"ota_common.h"

static HWND MainWnd;
int Update4GModule::getSdcardVersion(std::vector<std::string> &p_FileNameList,std::string &path_str)
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
		if(strcmp((dirp->d_name+( size - strlen(".bin"))),".bin")!=0)
			continue;
	   p_FileNameList.push_back(dirp->d_name);
	   ret = 0;
	}
	    closedir(sdcard_dir);
	return ret;
}

Update4GModule::Update4GModule(HWND hwnd){
    mHwnd = hwnd;
    MainWnd = mHwnd;
    m_usb4g_fd = -1;
    char temp[128]={0};
    sprintf(temp,"%s/%s/",MOUNT_PATH,VERSION_4G_DIR_NET);
    m_path_version = temp;
    //m_path_version = "/mnt/extsd/net_4g_version/";
}

Update4GModule::~Update4GModule()
{
	m_usb4g_fd = -1;
}

int Update4GModule::startUpdate4GModule()
{
    pthread_t thread_id = 0;
    pthread_create(&thread_id, NULL, startUpdate4GModuleThread, this);
    return 0;
}
//4g module init 
void Update4GModule::SetUSB4GModuleReset()
{
	system("echo 1 > /sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/ap_4g_rst");
	usleep(200*1000);
	system("echo 0 > /sys/devices/virtual/misc/sunxi-usb4g/usb4g-ctrl/ap_4g_rst");
}
int Update4GModule::CheckttyUsb()
{
	if((access("/dev/ttyUSB0", F_OK) == 0) || (access("/dev/ttyUSB1", F_OK) == 0) || (access("/dev/ttyUSB2", F_OK) == 0))
		return 1;
	else
		return 0;
}
int Update4GModule::openSerialPort(char *ttys)
{
	int fd;
    fd = open(ttys, O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd <= 0)
    {
        db_msg("Can't Open Serial Port");
        return -1;
    }   
    db_msg("debug_zhb---> ttys : %s",ttys);
    if(fcntl(fd, F_SETFL, 0) < 0)
    {
        db_msg("fcntl failed!\n");
    }
    if(isatty(STDIN_FILENO) == 0)
    {
        db_msg("standard input is not a terminal device\n");
    }
    return fd;
}

int Update4GModule::configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) 
    { 
        db_msg("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD; 
    newtio.c_cflag &= ~CSIZE; 

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':                    
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':                     
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':                    
        newtio.c_cflag &= ~PARENB;
        break;
    }

	switch( nSpeed )
	{
	    case 2400:
	        cfsetispeed(&newtio, B2400);
	        cfsetospeed(&newtio, B2400);
	        break;
	    case 4800:
	        cfsetispeed(&newtio, B4800);
	        cfsetospeed(&newtio, B4800);
	        break;
	    case 9600:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
	    case 115200:
	        cfsetispeed(&newtio, B115200);
	        cfsetospeed(&newtio, B115200);
	        break;
	    default:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
    }
    if( nStop == 1 )
    {
        newtio.c_cflag &=  ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    newtio.c_cc[VTIME]  = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        db_msg("com set error");
        return -1;
    }
    db_msg("set done!\n");
    return 0;
}

int Update4GModule::USB4GSerial_init()
{
    if((m_usb4g_fd = openSerialPort((char*)"/dev/ttyUSB0")) < 0)
    {
        db_msg("open_port ttyUSB0 error");
		if((m_usb4g_fd = openSerialPort((char*)"/dev/ttyUSB1")) < 0){
			 db_msg("open_port ttyUSB1 error");
			 if((m_usb4g_fd = openSerialPort((char*)"/dev/ttyUSB2")) < 0){
				 db_msg("open_port ttyUSB2 error");
				 goto err;
			}
		}
		
    }
    if(configureSerialPort(m_usb4g_fd,115200,8,'N',1) < 0)
    {
        db_msg("set_opt error");
		close(m_usb4g_fd);
		m_usb4g_fd = -1;
		goto err;
    }	

	return 0;
	
err:
	return -1;
}
int Update4GModule::USB4GSerial_exit()
{
    if(m_usb4g_fd >= 0)
    {
		close(m_usb4g_fd);
		m_usb4g_fd = -1;
    }
	return 0;
}

int Update4GModule::USB4G_ComRead(int fd, char *buf, int len)
{
	int ret;
	if(fd < 0)
		return -1;	
	if((ret = read(fd, buf, len)) < 0){
		db_error("USB4G_ComRead error ret = %d", ret);
		return -1;
	}
	return ret;
}

int Update4GModule::USB4G_ComWrite(int fd, char *buf, int len)
{
	int ret;
	if(fd < 0)
		return -1;
	if ((ret = write(fd, buf, len)) < 0) {  
		db_error(" USB4G_ComWrite error ret = %d", ret);
		return -1;
	}
	return 0;
}

int Update4GModule::startUpdate4G(std::string com)
{
	pid_t status_;
	status_ = system(com.c_str());
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

void *Update4GModule::startUpdate4GModuleThread(void * context)
{
	char buf[512] = {0};
	int ret = 0;
	char *ptr, *ptr1;
	char data[128] = {0};
	int net4g_flag = 0;
	char update_buf [128] = {0};
	char filepath[128]={0};
	std::string com_str;
	std::string ret_str = "\"BOOT\",0,\"MUTI_AMT\",0";
	int loop_count = 3;

	
	Update4GModule *u4m = reinterpret_cast<Update4GModule*>(context);
	prctl(PR_SET_NAME, "Update4GModule", 0, 0, 0);

	
	//ready to scan the dir if has the new xxx.bin need to update
	std::vector<std::string> p_FileNameList;
	p_FileNameList.clear();
	if(u4m->getSdcardVersion(p_FileNameList,u4m->m_path_version) < 0){
		db_msg("get sdcard version info fail");
		SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_FOUND_IMG_FAIL,0);
		return NULL;
	}
	for (unsigned int i=0; i<p_FileNameList.size(); i++)
	{
		sprintf(filepath,"%s%s",u4m->m_path_version.c_str(),p_FileNameList[i].c_str());
	}
	
	//kill the sdvcam
	system("kill -9 $(pidof sdvcam)");
	sleep(1);
	
	//Make sure to check tty and init the tty ok
	while(1){
		if(u4m->CheckttyUsb()){
			ret = u4m->USB4GSerial_init();
			if(ret == 0){
				break;
			}
		}
		usleep(500*1000);
	}

	//Make sure to prepare for the upgrade condition ok
     while(1){
		ret = 0;
		bzero(buf, sizeof(buf));	
		bzero(data, sizeof(data));
		while(ret <= 0){
			u4m->USB4G_ComWrite(u4m->m_usb4g_fd, (char*)"AT+ZFLAG=\"BOOT\",0", strlen("AT+ZFLAG=\"BOOT\",0")); //17
			usleep(5*1000);		
			u4m->USB4G_ComWrite(u4m->m_usb4g_fd, (char*)"\r\n", 2);
			usleep(5*1000);					
			ret = u4m->USB4G_ComRead(u4m->m_usb4g_fd, buf, sizeof(buf));
			usleep(5*1000);
		}

		ret = 0;
		bzero(buf, sizeof(buf));	
		bzero(data, sizeof(data));
		while(ret <= 0){		
			u4m->USB4G_ComWrite(u4m->m_usb4g_fd, (char*)"AT+ZFLAG?", strlen("AT+ZFLAG?")); 
			usleep(5*1000);		
			u4m->USB4G_ComWrite(u4m->m_usb4g_fd, (char*)"\r\n", 2);
			usleep(5*1000);					
			ret = u4m->USB4G_ComRead(u4m->m_usb4g_fd, buf, sizeof(buf));	
			usleep(5*1000);
			if((ptr = strstr(buf, "+ZFLAG: ")) != NULL){
				if((ptr1 = strstr(ptr, "\r\n")) != NULL){
					strncpy(data, ptr+strlen("+ZFLAG: "), ptr1-ptr-strlen("+ZFLAG: "));
				}
			}
			
		}

		if(strncmp(data,ret_str.c_str(),sizeof(data)) == 0)
		{
			db_warn("4g module update condition has been ok : %s",data);
			break;
		}
		usleep(500*1000);
     	}
	 
	//ready to update the 4g module
	sprintf(update_buf,"sml -f %s",filepath);
	com_str = update_buf;
	//try update 3 times
	for(int i = 0 ; i < loop_count ; i++)
	{
		u4m->SetUSB4GModuleReset();
		sleep(1);//wait the tty* stable
		if(u4m->startUpdate4G(com_str) < 0)
		{
			if(i == loop_count-1)
			{
				db_error("try %d times update 4g module failed",loop_count);
				goto fail_out;
			}
		}
		else
		{
			db_warn("update 4g module successful ");
			goto done_out;
		}
	}

done_out:

	u4m->SetUSB4GModuleReset();
	u4m->USB4GSerial_exit();
	SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_UPDATE_SUCCESS,0);
	db_warn("4g module update done !!!");
	return NULL;

fail_out:
	u4m->SetUSB4GModuleReset();
	u4m->USB4GSerial_exit();
    	SendMessage(MainWnd,MSG_UPDATE_OVER,OTA_UPDATE_FAIL,0);
	db_warn("4g module update failed !!!");
    return NULL; 
}


