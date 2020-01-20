#ifndef _WINDOWS_4G_H
#define _WINDOWS_4G_H

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <signal.h>
#include<string>
#include<vector>

#define MSG_UPDATE_OVER  1

class Update4GModule{
public:
	Update4GModule();
	~Update4GModule();
	Update4GModule(HWND hwnd);
	int startUpdate4GModule();
private:
	int getSdcardVersion(std::vector<std::string> &p_FileNameList,std::string &path_str);
	void SetUSB4GModuleReset();
	int CheckttyUsb();
	int openSerialPort(char *ttys);
	int configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop);
	int USB4GSerial_init();
	int USB4GSerial_exit();
	int USB4G_ComRead(int fd, char *buf, int len);
	int USB4G_ComWrite(int fd, char *buf, int len);
	static void *startUpdate4GModuleThread(void * context);
	int startUpdate4G(std::string com);
private:
    HWND mHwnd;
    int m_usb4g_fd;
public:
    std::string m_path_version;
};
#endif
