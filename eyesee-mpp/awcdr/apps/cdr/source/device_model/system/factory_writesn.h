#ifndef __FACTORY_WRITESN_H__
#define __FACTORY_WRITESN_H__

#include <string>

enum cmd_type
{
    GET_INFO = 0,
    WR_SN,
    RD_SN,
    RD_TEST
};


typedef struct __RD_TEST_CMD_ST__
{
    std::string gsensor;
    std::string lte;
    std::string wifi;
    std::string gps;
}RD_TEST_CMD;


typedef struct __DEVICE_INFO_ST__
{
    std::string wifimac;
    std::string imei;
    std::string simID;
    std::string fwversion;
}Device_info;

typedef struct __WR_SN_CMD_ST__
{
    std::string sn;
    std::string lotid;
    std::string Key;
    std::string P2PID;
    std::string imei;
}WR_SN_CMD;

typedef struct __SERIAL_OPTION_ST__
{
    int speed;
    int flow_ctl;
    int databits;
    int stopbits;
    char parity;
}serial_option;


int creat_serial_command_thread();

#endif

