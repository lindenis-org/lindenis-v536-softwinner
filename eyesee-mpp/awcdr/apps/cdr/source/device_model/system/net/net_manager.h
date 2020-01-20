/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file net_manager.h
 * @brief 网络管理模块
 * @author id:826
 * @version v0.3
 * @date 2016-07-28
 */

#pragma once

#include "common/subject.h"
#include "common/observer.h"
#include "common/singleton.h"
#include "common/utils/utils.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <list>
#include <mutex>


#define WIFI_INTERFACE_NAME   "wlan0"
#define ETH_INTERFACE_NAME    "eth0"
#define MAX_IFHWADDR_SIZE 20

typedef enum {
    NET_DEV_UP_STATE = 0,
    NET_DEV_DOWN_STATE,
    NET_DEV_STATE_BOTTON,
} NET_DEV_STATE;


typedef struct {
    char interface[32];
    char mac[32];
    char ip[32];
    char mask[32];
    char gateway[32];
    char dns1[32];
    char dns2[32];
    int dhcp_enable;
} Net_MANAGER_Attr;

typedef struct {
    int attr_num;
    Net_MANAGER_Attr net_attr[4];
} Net_Attr_List;


void get_if_addrs(const std::string &if_name, std::string &address);


namespace EyeseeLinux {

class SoftApController;
class WifiController;
class EtherController;

class NetManager
    : public ISubjectWrap(NetManager)
    , public IObserverWrap(NetManager)
    , public Singleton<NetManager>
{
    friend class Singleton<NetManager>;
    public:
        enum NetLinkType {
            NETLINK_ETH_STATIC = 0,
            NETLINK_ETH_DHCP,
            NETLINK_WIFI_STA,
            NETLINK_WIFI_SOFTAP,
            NETLINK_NONE,
        };
        
        int InitNetManager();
        int ExitNetManager();

        int CheckIpAddr(const std::string &ip_addr) const;
        int GetNetDevlist(std::vector<std::string> &netdev_list) const;
        int PingIp(unsigned int dstIp, int timeOut, bool &is_reach);
        int PingIp(const std::string &dstIp, int timeOut, bool &is_reach);
        int IsConnectInternet(int timeOut, bool &is_connetct);
        int GetNetDevConnectState(const std::string &netdev_name, bool &is_connetct);
        int UpNetDev(const std::string &netdev_name);
        int DownNetDev(const std::string &netdev_name);
        int GetNetDevState(const std::string &netdev_name, NET_DEV_STATE &netdev_state);

        int GetNetDevIp(const std::string &netdev_name, std::string &ip) const;
        int GetNetDevIp(const std::string &netdev_name, unsigned int &ip) const;
        int SetNetDevIp(const std::string &netdev_name, const std::string &ip);
        int SetNetDevIp(const std::string &netdev_name, const unsigned int &ip);

        int GetNetDevMask(const std::string &netdev_name, std::string &mask) const;
        int GetNetDevMask(const std::string &netdev_name, unsigned int &mask) const;
        int SetNetDevMask(const std::string &netdev_name, const std::string &mask);
        int SetNetDevMask(const std::string &netdev_name, const unsigned int &mask);

        int GetNetDevGateWay(const std::string &netdev_name, std::string &gateway) const;
        int GetNetDevGateWay(const std::string &netdev_name, unsigned int &gateway) const;
        int AddDefaultGateWay(const std::string &netdev_name, const std::string &gateway);
        int DeleteAllDefaultGateWay();

        int GetNetDevBroadCast(const std::string &netdev_name, std::string &broadcast) const;
        int GetNetDevBroadCast(const std::string &netdev_name, unsigned int &broadcast) const;
        int SetNetDevBroadCast(const std::string &netdev_name, const std::string &broadcast);
        int SetNetDevBroadCast(const std::string &netdev_name, const unsigned int &broadcast);

        int GetNetDevMtu(const std::string &netdev_name, unsigned int &mtu) const;
        int SetNetDevMtu(const std::string &netdev_name, const unsigned int mtu);

        int GetNetDevMac(const std::string &netdev_name, unsigned char mac[]) const;
        int SetNetDevMac(const std::string &netdev_name, const unsigned char mac[]);

        int GetNetDevDns(std::string &dns1, std::string &dns2) const;
        int SetNetDevDns(const std::string &dns1, const std::string &dns2);

        int EnableNetDevStatic();
        int EnableNetDevDhcpc(const std::string &netdev_name);
        int DisableNetDevDhcpc(void);
        int GetNetDevDhcpcStatus(const std::string &netdev_name) const;

        int EnableNetDevDhcpd(const std::string &netdev_name);
        int DisableNetDevDhcpd(void);
        int GetNetDevDhcpdStatus(const std::string &netdev_name) const;

        int SwitchToSoftAp(const char *ssid, const char *pwd, int mode, int encrypt, int freq);
        int DisableAllNetLink();

        int EnableMonitorMode(const std::string &interface);
        int DisableMonitorMode(const std::string &interface);
        int SmartLinkLockChannel(const std::string &interface, int interval, int timeout);
        int SmartLinkParseAPInfo(const std::string &interface, std::string &ssid,
                                        std::string &pwd, unsigned char &note_num, int interval);
        int SmartLinkResultReturn(unsigned char note_num);

        void GetNetLinkType(NetLinkType &netlink_type) const;

        int LoadNetConfig(Net_Attr_List &attr_list);
        int SaveNetConfig(const Net_Attr_List &attr_list);
        int DefaultNetConfig();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        int DisableSoftap();
	int GetWifiInfo(std::string &p_ssid, std::string &p_pwd);
	int SetWifiInfo(std::string &p_ssid, std::string &p_pwd);
    private:
        LuaConfig *lua_cfg_;
        SoftApController *softap_ctrl_;
        WifiController *wifi_ctrl_;
        EtherController *eth_ctrl_;
        bool dhcpc_running_;
        bool dhcpd_running_;
        NetLinkType netlink_type_;
        
        NetManager();
        ~NetManager();
        NetManager &operator=(const NetManager &o);

};

}
