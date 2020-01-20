/**
* @file obd2_manager.cpp
* @author wang.guojun@xiaoyi.com
* @date 2018-10-23
* @version v0.1
* @brief 获取OBD数据
* @see obd2_manager.h
* @verbatim
*  History:
* @endverbatim
*/
#include "device_model/system/obd2_manager.h"
#include "common/app_log.h"
#include "common/thread.h"

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>

using namespace EyeseeLinux;
using namespace std;

#define OBD_PORT "/dev/ttyS3"

Obd2Manager::Obd2Manager()
{
	obd2_fd = -1;
	memset(&mObd2_rt,0,sizeof(mObd2_rt));
}

Obd2Manager::~Obd2Manager()
{
	if( m_Obd2_thread_id > 0 )
		pthread_cancel(m_Obd2_thread_id);
}
int Obd2Manager::OpenObd2Port(char *ttys)
{
	int fd;
	fd = open(ttys, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd <= 0)
	{
		db_msg("Can't Open Serial Port");
		return -1;
	}	
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
int Obd2Manager::configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop)
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

int Obd2Manager::Obd2MonitorThreadInit()
{
    if(obd2_fd > 0) {
        close(obd2_fd);
        obd2_fd = -1;
    }

    if((obd2_fd=OpenObd2Port(OBD_PORT)) < 0)
    {
        db_error("open_port error");
        return -1;
    }

    if(configureSerialPort(obd2_fd,9600,8,'N',1) < 0)
    {
        db_error("set_opt error");
		close(obd2_fd);
		obd2_fd = -1;
        return -1;
    }

	return 0;
}
int Obd2Manager::GetObd2Data(Obd2_RT_t &p_Obd2_RT)
{
	p_Obd2_RT = mObd2_rt;
	return 0;
}

void Obd2Manager::ProcessOBD2_RT( char *buf)
{
	char *ptr,*ptr2;
	int ret;
	if((ptr=strstr(buf, "$OBD-RT"))!= NULL)
			{
				if((ptr2 = strstr(ptr, "\r\n")) != NULL)
					{
					
					ret = sscanf(ptr+sizeof("$OBD-RT"), "%f,%d,%d,%f,%f,%d,%f,%f,%f,%f,%f,%f,%d,%d,%d", &mObd2_rt.battery_vol, 
																									    &mObd2_rt.engine_rpm,  
																										&mObd2_rt.speed,      
																										&mObd2_rt.throttle_position,   
																										&mObd2_rt.engine_load,     
																										&mObd2_rt.coolant_temperature,   
																										&mObd2_rt.dynamical_fuel ,   
																										&mObd2_rt.average_fuel,  
																										&mObd2_rt.road_mileage,    
																										&mObd2_rt.odograph,    
																										&mObd2_rt.instantaneous_fuel_consumption,   
																										&mObd2_rt.total_fuel_consumption,    
																										&mObd2_rt.DTC_Number,        
																										&mObd2_rt.rapid_acceleration,    
																										&mObd2_rt.emergency_deceleration);
					db_msg("%f,%d,%d,%f,%f,%d,%f,%f,%f,%f,%f,%f,%d,%d,%d \n", mObd2_rt.battery_vol, 
																		mObd2_rt.engine_rpm,  
																		mObd2_rt.speed,	  
																		mObd2_rt.throttle_position,   
																		mObd2_rt.engine_load,	   
																		mObd2_rt.coolant_temperature,	 
																		mObd2_rt.dynamical_fuel ,	 
																		mObd2_rt.average_fuel,  
																		mObd2_rt.road_mileage,    
																		mObd2_rt.odograph,    
																		mObd2_rt.instantaneous_fuel_consumption,	
																		mObd2_rt.total_fuel_consumption,	 
																		mObd2_rt.DTC_Number,		 
																		mObd2_rt.rapid_acceleration,	 
																		mObd2_rt.emergency_deceleration);

				}
				
			}
}

void Obd2Manager::RunObd2()
{
	ThreadCreate(&m_Obd2_thread_id,NULL,Obd2Manager::Obd2MonitorThread,this);
}
void *Obd2Manager::Obd2MonitorThread(void * context)
{
	char buf[1024] , *ptr,*ptr2,*temp_ptr;
	int ret ;

	Obd2Manager *om = reinterpret_cast<Obd2Manager*>(context);
	prctl(PR_SET_NAME, "Obd2MonitorThread", 0, 0, 0);
	db_msg("%s enter \n",__func__);

loop:
	if(om->obd2_fd <= 0)
	{
		if( om->Obd2MonitorThreadInit() < 0 )
		{
			db_warn("[debug_jason]: open startLocationReport is failed try again");
			sleep(1);
			goto loop;
		}
	}
	while(1)
		{
			if((ret = read(om->obd2_fd, buf, 1023)) > 1) 
				{
				om->ProcessOBD2_RT(buf);
				sleep(1);
				memset(buf, 0, sizeof(buf));
			}
		}

	return 0;
}


