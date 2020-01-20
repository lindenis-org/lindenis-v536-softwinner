/**
* @file obd2_manager.cpp
* @author wang.guojun@xiaoyi.com
* @date 2018-10-23
* @version v0.1
* @brief 获取OBD数据
* @verbatim
*  History:
* @endverbatim
*/


#pragma once

#include "common/singleton.h"

namespace EyeseeLinux {

typedef struct
{
	float battery_vol;  //电瓶电压
    int engine_rpm;  //发动机转速
	int speed;       //车速
	float throttle_position;   //节气门开度
	float engine_load;     //发动机负荷
	int coolant_temperature;   //冷却液温度
	float dynamical_fuel ;   //瞬时油耗
	float average_fuel;   //平均油耗
	float road_mileage;    //本地行驶里程
	float odograph;    //总里程
	float instantaneous_fuel_consumption;    //本次耗油量
	float total_fuel_consumption;    //累计耗油量
	int DTC_Number;        //当前故障码数量
	int rapid_acceleration;    //急加速次数
	int emergency_deceleration;  //急减速次数
	
}Obd2_RT_t;

class Obd2Manager
    : public Singleton<Obd2Manager>
{
    friend class Singleton<Obd2Manager>;
    public:
		int GetObd2Data(Obd2_RT_t &p_Obd2_RT);
		static void *Obd2MonitorThread(void * context);
		void RunObd2();

    private:

        Obd2Manager();
        ~Obd2Manager();
        Obd2Manager(const Obd2Manager &o);
        Obd2Manager &operator=(const Obd2Manager &o);
		int OpenObd2Port(char *ttys);
		int Obd2MonitorThreadInit();
		int configureSerialPort(int fd,int nSpeed, int nBits, char nEvent, int nStop);
		void ProcessOBD2_RT( char *buf);
		Obd2_RT_t mObd2_rt;
		int obd2_fd;
		pthread_t m_Obd2_thread_id;


}; 
} /* EyeseeLinux */

