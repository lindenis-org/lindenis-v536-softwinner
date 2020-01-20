/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file net_manager.cpp
 * @brief 网络管理模块
 * @author id:826
 * @version v0.3
 * @date 2016-07-28
 */
#include "device_model/system/net/net_manager.h"
#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/ethernet_controller.h"
#include "device_model/system/event_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/menu_config_lua.h"
#include "common/utils/utils.h"
#include "common/app_log.h"

#include <smartlink.h>

#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <net/route.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#undef LOG_TAG
#define LOG_TAG "NetworkManager"

using namespace std;
using namespace EyeseeLinux;

#define NET_DEVICE_NAME           20
#define PATH_PROCNET_DEV          "/proc/net/dev"
#define PATH_PROCNET_IFINET6      "/proc/net/if_inet6"
#define PATH_PROCNET_ROUTE        "/proc/net/route"
#define PATH_DNS_CONT             "/etc/resolv.conf"
#define PATH_NTP_CONT             "/etc/ntp.conf"
#define MTU_MINI_VAL              10

#define CMD_RUN_DHCPC             "/sbin/udhcpc -i %s -b -R"
#define CMD_KILL_DHCPC            "killall udhcpc"
#define CMD_RUN_DHCPD             "/usr/sbin/udhcpd /etc/udhcpd.conf"
#define CMD_KILL_DHCPD            "killall udhcpd"

#define NET_CONFIG_FILE "/tmp/data/net_config.lua"

#define MAX_NET_DEV_NUM 2

void get_if_addrs(const string &if_name, string &address)
{
    struct ifaddrs * if_addrs = NULL;
    void * src_addr = NULL;

    getifaddrs(&if_addrs);

    while (if_addrs != NULL) {
        if (strncmp(if_name.c_str(), if_addrs->ifa_name, if_name.size()) == 0) {
            if (if_addrs->ifa_addr->sa_family == AF_INET) { // check it is IP4
                // is a valid IP4 Address
                src_addr = &((struct sockaddr_in *)if_addrs->ifa_addr)->sin_addr;
                char addr_buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, src_addr, addr_buf, INET_ADDRSTRLEN);
                address = addr_buf;
                db_msg("%s IP Address %s\n", if_addrs->ifa_name, addr_buf);
                break;
            }
        }
        if_addrs = if_addrs->ifa_next;
    }
}

NetManager::NetManager()
{
    lua_cfg_ = new LuaConfig();

    //StorageManager::GetInstance()->Attach(this);

    softap_ctrl_ = new SoftApController();
    softap_ctrl_->Attach(this);

    wifi_ctrl_ = new WifiController();
    wifi_ctrl_->Attach(this);

    eth_ctrl_ = new EtherController();
    eth_ctrl_->Attach(this);

    dhcpc_running_ = true;
    netlink_type_ = NETLINK_ETH_DHCP;
}

NetManager::~NetManager()
{
    //StorageManager::GetInstance()->Detach(this);

    if (lua_cfg_) {
        delete lua_cfg_;
    }

    softap_ctrl_->Detach(this);
    wifi_ctrl_->Detach(this);
    eth_ctrl_->Detach(this);

    delete softap_ctrl_;
    delete wifi_ctrl_;
    delete eth_ctrl_;
}

int NetManager::SaveNetConfig(const Net_Attr_List &attr_list)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(NET_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", NET_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    for (cnt = 0; cnt < MAX_NET_DEV_NUM; cnt++) {
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].interface", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].interface);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].mac", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].mac);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].ip", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].ip);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].mask", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].mask);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].gateway", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].gateway);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dns1", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].dns1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dns2", cnt + 1);
        lua_cfg_->SetStringValue(tmp_str, attr_list.net_attr[cnt].dns2);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dhcp_enable", cnt + 1);
        lua_cfg_->SetIntegerValue(tmp_str, attr_list.net_attr[cnt].dhcp_enable);

        #if 0
        db_msg("cnt:%d  interface:%s \n",   cnt, attr_list.net_attr[cnt].interface);
        db_msg("cnt:%d  mac:%s \n",         cnt, attr_list.net_attr[cnt].mac);
        db_msg("cnt:%d  ip:%s \n",          cnt, attr_list.net_attr[cnt].ip);
        db_msg("cnt:%d  mask:%s \n",        cnt, attr_list.net_attr[cnt].mask);
        db_msg("cnt:%d  gateway:%s \n",     cnt, attr_list.net_attr[cnt].gateway);
        db_msg("cnt:%d  dns1:%s \n",        cnt, attr_list.net_attr[cnt].dns1);
        db_msg("cnt:%d  dns2:%s \n",        cnt, attr_list.net_attr[cnt].dns2);
        db_msg("cnt:%d  dhcp_enable:%d \n", cnt, attr_list.net_attr[cnt].dhcp_enable);
        #endif
    }

    return 0;
}


int NetManager::DefaultNetConfig()
{
    return 0;
}

int NetManager::LoadNetConfig(Net_Attr_List &attr_list)
{
    int ret = 0;
    int cnt = 0, i = 0;
    char tmp_str[256] = {0};
    std::string str;

    if (NULL == lua_cfg_) {
        db_msg("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(NET_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", NET_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(NET_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again", NET_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(NET_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!", NET_CONFIG_FILE);
            return -1;
        }
    }

    memset(&attr_list, 0, sizeof(attr_list));

    for (cnt = 0; cnt < MAX_NET_DEV_NUM; cnt++) {
        attr_list.attr_num++;
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].interface", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].interface, str.c_str(), sizeof(attr_list.net_attr[cnt].interface) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].mac", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].mac, str.c_str(), sizeof(attr_list.net_attr[cnt].mac) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].ip", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].ip, str.c_str(), sizeof(attr_list.net_attr[cnt].ip) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].mask", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].mask, str.c_str(), sizeof(attr_list.net_attr[cnt].mask) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].gateway", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].gateway, str.c_str(), sizeof(attr_list.net_attr[cnt].gateway) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dns1", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].dns1, str.c_str(), sizeof(attr_list.net_attr[cnt].dns1) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dns2", cnt + 1);
        str = lua_cfg_->GetStringValue(tmp_str);
        strncpy(attr_list.net_attr[cnt].dns2, str.c_str(), sizeof(attr_list.net_attr[cnt].dns2) - 1);

        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str) - 1, "network.net_attr[%d].dhcp_enable", cnt + 1);
        attr_list.net_attr[cnt].dhcp_enable = lua_cfg_->GetIntegerValue(tmp_str);

        #if 0
        db_msg("cnt:%d  interface:%s \n",   cnt, attr_list.net_attr[cnt].interface);
        db_msg("cnt:%d  mac:%s \n",         cnt, attr_list.net_attr[cnt].mac);
        db_msg("cnt:%d  ip:%s \n",          cnt, attr_list.net_attr[cnt].ip);
        db_msg("cnt:%d  mask:%s \n",        cnt, attr_list.net_attr[cnt].mask);
        db_msg("cnt:%d  gateway:%s \n",     cnt, attr_list.net_attr[cnt].gateway);
        db_msg("cnt:%d  dns1:%s \n",        cnt, attr_list.net_attr[cnt].dns1);
        db_msg("cnt:%d  dns2:%s \n",        cnt, attr_list.net_attr[cnt].dns2);
        db_msg("cnt:%d  dhcp_enable:%d \n", cnt, attr_list.net_attr[cnt].dhcp_enable);
        #endif
    }

    return 0;
}


int NetManager::InitNetManager()
{
    int ret = 0, cnt = 0;
    Net_Attr_List attr_list;
    do {
        ret = LoadNetConfig(attr_list);
        if (ret) {
            db_msg("Do LoadNetConfig fail!\n");
            break;
        }

        for (cnt = 0; cnt < MAX_NET_DEV_NUM; cnt++) {
            if (strstr(attr_list.net_attr[cnt].interface, "eth")) {
                /* Do DHCP or static IP setting */
                if (attr_list.net_attr[cnt].dhcp_enable) {
                    DisableNetDevDhcpd();
                    usleep(80);

                    ret = EnableNetDevDhcpc(attr_list.net_attr[cnt].interface);
                    usleep(500);
                } else {
                    DisableNetDevDhcpd();
                    usleep(20);

                    if ((ret = UpNetDev(attr_list.net_attr[cnt].interface)))
                        break;
                    usleep(20);

                    if ((ret = SetNetDevIp(attr_list.net_attr[cnt].interface, attr_list.net_attr[cnt].ip)))
                        break;

                    if ((ret = SetNetDevMask(attr_list.net_attr[cnt].interface, attr_list.net_attr[cnt].mask)))
                        break;

                    if ((ret = AddDefaultGateWay(attr_list.net_attr[cnt].interface, attr_list.net_attr[cnt].gateway)))
                        break;

                    if ((ret = SetNetDevDns(attr_list.net_attr[cnt].dns1, attr_list.net_attr[cnt].dns2)))
                        break;
                }
                break;
            }
        }
    } while (0);

    return ret;
}


int NetManager::ExitNetManager()
{
    return 0;
}


int NetManager::GetNetDevlist(std::vector<std::string> &netdev_list) const
{
    unsigned int  cnt = 0, i = 0;
    int  fd  = -1;
    char buf[768]     = {0};
    char devname[128] = {0};
    FILE *fp = NULL;

    fp = fopen(PATH_PROCNET_DEV, "r");
    if (NULL == fp) {
        db_msg("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_DEV, errno, strerror(errno));
    }

    /* Skip the two line. */
    fgets(buf, sizeof buf, fp); /* eat line */
    fgets(buf, sizeof buf, fp); /* eat line */

    while (fgets(buf, sizeof(buf) - 1, fp)) {
        memset(devname, 0, sizeof(devname));
        for (cnt = 0, i = 0; cnt < (sizeof(buf) - 1); cnt++) {
            if (':' == buf[cnt]) {
                break;
            }
            if (' ' == buf[cnt]) {
                continue;
            }
            devname[i] = buf[cnt];
            i++;
        }
        db_msg ("===>> devname:%s \n", devname);
        netdev_list.push_back(devname);
    }

    fclose(fp);
    return 0;
}


int NetManager::CheckIpAddr(const std::string &ip_addr) const
{
    int  val     = 0;
    int  val_cnt = 0;
    int  doc_cnt = 0;
    char tmp     = 0;

    for (unsigned int i = 0; i < ip_addr.length(); ++i)
    {
        tmp = ip_addr[i];

        if ('.' != tmp) {
            if (tmp < '0' || tmp > '9') {
                return -1;
            }

            val_cnt++;
            val = val * 10 + (tmp - '0');
            if (val > 255) {
                return -1;
            }
        }
        else {
            val = 0;
            doc_cnt++;
            if ((i + 1) >= ip_addr.length()) {
                return -1;
            }
            if (0 == val_cnt) {
                return -1;
            }
            val_cnt = 0;
        }
    }

    if (3 != doc_cnt) {
        return -1;
    }

    return 0;
}


int NetManager::GetNetDevIp(const std::string &netdev_name, unsigned int &ip) const
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    ip = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);

    close(fd);
    return 0;
}


int NetManager::GetNetDevIp(const std::string &netdev_name, std::string &ip) const
{
    int            ret   = 0;
    unsigned int   u32ip = 0;
    struct in_addr sin_addr;

    ret = this->GetNetDevIp(netdev_name, u32ip);
    if (ret) {
        db_msg("Do GetNetDevIp error! ret:%d  \n", ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u32ip);
    ip              = inet_ntoa(sin_addr);

    return 0;
}



int NetManager::SetNetDevIp(const std::string &netdev_name, const unsigned int &ip)
{
    int                 fd = -1;
    struct ifreq        ifr;
    struct sockaddr_in *sin;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    sin                  = (struct sockaddr_in*)&ifr.ifr_addr;
    sin->sin_addr.s_addr = htonl(ip);
    sin->sin_family      = AF_INET;
    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0)
    {
        db_msg("Fail to ioctl SIOCSIFADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);
    return 0;
}


int NetManager::SetNetDevIp(const std::string &netdev_name, const std::string &ip)
{
    int          ret   = 0;
    unsigned int u32ip = 0;

    u32ip = inet_addr(ip.c_str());
    u32ip = ntohl(u32ip);

    ret = this->SetNetDevIp(netdev_name, u32ip);
    if (ret) {
        db_msg("Do SetNetDevIp error! ret:%d  \n", ret);
        return -1;
    }

    return 0;
}

int NetManager::GetNetDevMask(const std::string &netdev_name, unsigned int &mask) const
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFNETMASK. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    mask = ntohl(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr.s_addr);

    close(fd);
    return 0;
}


int NetManager::GetNetDevMask(const std::string &netdev_name, std::string &mask) const
{
    int            ret     = 0;
    unsigned int   u32mask = 0;
    struct in_addr sin_addr;

    ret = this->GetNetDevMask(netdev_name, u32mask);
    if (ret) {
        db_msg("Do GetNetDevMask error! ret:%d  \n", ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u32mask);
    mask            = inet_ntoa(sin_addr);

    return 0;
}


int NetManager::SetNetDevMask(const std::string &netdev_name, const unsigned int &mask)
{
    int                 fd = -1;
    struct ifreq        ifr;
    struct sockaddr_in *sin;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    sin                  = (struct sockaddr_in*)&ifr.ifr_netmask;
    sin->sin_addr.s_addr = htonl(mask);
    sin->sin_family      = AF_INET;
    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        db_msg("Fail to ioctl SIOCSIFNETMASK. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);
    return 0;
}


int NetManager::SetNetDevMask(const std::string &netdev_name, const std::string &mask)
{
    int          ret     = 0;
    unsigned int u32mask = 0;

    u32mask = inet_addr(mask.c_str());
    u32mask = ntohl(u32mask);
    ret = this->SetNetDevMask(netdev_name, u32mask);
    if (ret) {
        db_msg("Do SetNetDevMask error! ret:%d  \n", ret);
        return -1;
    }

    return 0;
}

int NetManager::GetNetDevBroadCast(const std::string &netdev_name, unsigned int &broadcast) const
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFBRDADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    broadcast = ntohl(((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr);

    close(fd);
    return 0;
}


int NetManager::GetNetDevBroadCast(const std::string &netdev_name, std::string &broadcast) const
{
    int            ret         = 0;
    unsigned int   u3broadcast = 0;
    struct in_addr sin_addr;

    ret = this->GetNetDevBroadCast(netdev_name, u3broadcast);
    if (ret) {
        db_msg("Do GetNetDevBroadCast error! ret:%d  \n", ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u3broadcast);
    broadcast       = inet_ntoa(sin_addr);

    return 0;
}


int NetManager::SetNetDevBroadCast(const std::string &netdev_name, const unsigned int &broadcast)
{
    int                 fd = -1;
    struct ifreq        ifr;
    struct sockaddr_in *sin;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    sin                  = (struct sockaddr_in*)&ifr.ifr_broadaddr;
    sin->sin_addr.s_addr = htonl(broadcast);
    sin->sin_family      = AF_INET;
    if (ioctl(fd, SIOCSIFBRDADDR, &ifr) < 0)
    {
        db_msg("Fail to ioctl SIOCSIFBRDADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return  -1;
    }

    close(fd);
    return 0;
}


int NetManager::SetNetDevBroadCast(const std::string &netdev_name, const std::string &broadcast)
{
    int          ret          = 0;
    unsigned int u32broadcast = 0;

    u32broadcast = inet_addr(broadcast.c_str());
    u32broadcast = ntohl(u32broadcast);

    ret = this->SetNetDevBroadCast(netdev_name, u32broadcast);
    if (ret < 0) {
        db_msg("Do SetNetDevBroadCast error! ret:%d  \n", ret);
        return ret;
    }

    return ret;
}


int NetManager::GetNetDevMac(const std::string &netdev_name, unsigned char mac[]) const
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFHWADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    mac[0] = ifr.ifr_hwaddr.sa_data[0];
    mac[1] = ifr.ifr_hwaddr.sa_data[1];
    mac[2] = ifr.ifr_hwaddr.sa_data[2];
    mac[3] = ifr.ifr_hwaddr.sa_data[3];
    mac[4] = ifr.ifr_hwaddr.sa_data[4];
    mac[5] = ifr.ifr_hwaddr.sa_data[5];

    db_msg("%s, mac address: [%02x:%02x:%02x:%02x:%02x:%02x]\n", netdev_name.c_str(),
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    close(fd);
    return 0;
}



int NetManager::SetNetDevMac(const std::string &netdev_name, const unsigned char mac[])
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFHWADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    ifr.ifr_hwaddr.sa_data[0] = mac[0];
    ifr.ifr_hwaddr.sa_data[1] = mac[1];
    ifr.ifr_hwaddr.sa_data[2] = mac[2];
    ifr.ifr_hwaddr.sa_data[3] = mac[3];
    ifr.ifr_hwaddr.sa_data[4] = mac[4];
    ifr.ifr_hwaddr.sa_data[5] = mac[5];

    if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCSIFHWADDR. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int NetManager::GetNetDevGateWay(const std::string &netdev_name, unsigned int &gateway) const
{
    int  ret         = 0;
    char *pret       = NULL;
    char devname[64] = {0};
    char flags[16]   = {0};
    int  flgs, ref, use, metric, mtu, win, ir;
    unsigned long dst, gw, mask;
    FILE *fp = NULL;

    gateway = 0;

    fp = fopen(PATH_PROCNET_ROUTE, "r");
    if (NULL == fp) {
        db_msg("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_ROUTE, errno, strerror(errno));
        return -1;
    }

    /* Skip the first line. */
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        /* Empty or missing line, or read error. */
        db_msg("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_ROUTE, errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    while (1) {
        ret = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &dst, &gw, &flgs, &ref, &use, &metric, &mask,
                   &mtu, &win, &ir);
        if (ret != 11) {
            if ((ret < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
                ret = -1;
                db_msg("Don't find NetDev:%s gateway!\n",  netdev_name.c_str());
                break;
            }
        }

        if (!(flgs & RTF_UP)) { /* Skip interfaces that are down. */
            continue;
        }

        ret = strncmp(netdev_name.c_str(), devname, NET_DEVICE_NAME-1);
        if (0 == ret && 0 != gw) {
            gateway = ntohl(gw);
            ret     = 0;
            break;
        } else {
            continue;
        }
    }

    fclose(fp);
    return 0;
}


int NetManager::GetNetDevGateWay(const std::string &netdev_name, std::string &gateway) const
{
    int            ret = 0;
    unsigned int   u3gateway;
    struct in_addr sin_addr;

    ret = GetNetDevGateWay(netdev_name, u3gateway);
    if (ret) {
        db_msg("Do GetNetDevGateWay error! ret:%d  \n", ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u3gateway);
    gateway         = inet_ntoa(sin_addr);
    return 0;
}


int NetManager::AddDefaultGateWay(const std::string &netdev_name, const std::string &gateway)
{
    /* char buffer instead of bona-fide struct avoids aliasing warning */
    char rt_buf[sizeof(struct rtentry)] = {0};
    struct rtentry     *const rt = (struct rtentry *)rt_buf;
    struct sockaddr_in *sin;

    /* Clean out the RTREQ structure. */
    memset(rt, 0, sizeof(rt_buf));

    /* set  Genmask */
    sin                  = (struct sockaddr_in*)&(rt->rt_genmask);
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_family      = AF_INET;

    /* set Destination */
    sin                  = (struct sockaddr_in*)&(rt->rt_dst);
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_family      = AF_INET;
    sin->sin_port        = 0;

    /* set Gateway */
    sin                  = (struct sockaddr_in*)&(rt->rt_gateway);
    sin->sin_addr.s_addr = inet_addr(gateway.c_str());
    sin->sin_family      = AF_INET;

    /* set Iface */
    rt->rt_dev = (char*)netdev_name.c_str();

    /* set Flags */
    rt->rt_flags |= RTF_UP;
    rt->rt_flags |= RTF_GATEWAY;

    int fd = -1;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        return -1;
    }

    if (ioctl(fd, SIOCADDRT, rt) < 0) {
        db_msg("Fail to ioctl SIOCADDRT. errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int NetManager::DeleteAllDefaultGateWay()
{
    int  ret = 0;
    char *pret          = NULL;
    char devname[64]    = {0};
    char flags[16]      = {0};
    int  flgs, ref, use, metric, mtu, win, ir;
    unsigned long dst, gw, mask;
    int   fd = -1;
    FILE *fp = NULL;

    /* char buffer instead of bona-fide struct avoids aliasing warning */
    char rt_buf[sizeof(struct rtentry)] = {0};
    struct rtentry     *const rt = (struct rtentry *)rt_buf;
    struct sockaddr_in *sin;

    fp = fopen(PATH_PROCNET_ROUTE, "r");
    if (NULL == fp) {
        db_msg("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_ROUTE, errno, strerror(errno));
        return -1;
    }

    /* Skip the first line. */
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        /* Empty or missing line, or read error. */
        db_msg("Open file:%s fail! errno[%d] errinfo[%s]\n",
                PATH_PROCNET_ROUTE, errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    while (1) {
        ret = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &dst, &gw, &flgs, &ref, &use, &metric, &mask,
                   &mtu, &win, &ir);
        if (ret != 11) {
            if ((ret < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
                db_msg("Read file:%s end! Delete default Gate way end.\n",  PATH_PROCNET_ROUTE);
                ret = 0;
                break;
            }
        }

        //db_msg("--- devname:%s  dst:[0x%lx] flgs:[0x%x] gw:[0x%lx] mask:[0x%lx] \n",
        //        devname, dst, flgs, gw, mask);

        if (!(flgs & (RTF_UP | RTF_GATEWAY))) { /* Skip interfaces that are down. */
            continue;
        }

        if (0x0 == dst) {
            /* Clean out the RTREQ structure. */
            memset(rt, 0, sizeof(rt_buf));

            /* set  Genmask */
            sin                  = (struct sockaddr_in*)&(rt->rt_genmask);
            sin->sin_addr.s_addr = mask;
            sin->sin_family      = AF_INET;

            /* set Destination */
            sin                  = (struct sockaddr_in*)&(rt->rt_dst);
            sin->sin_addr.s_addr = dst;
            sin->sin_family      = AF_INET;
            sin->sin_port        = 0;

            /* set Gateway */
            sin                  = (struct sockaddr_in*)&(rt->rt_gateway);
            sin->sin_addr.s_addr = gw;
            sin->sin_family      = AF_INET;

            /* set Iface */
            rt->rt_dev = devname;

            /* set Flags */
            rt->rt_flags |= RTF_UP;
            rt->rt_flags |= RTF_GATEWAY;

            fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd <= 0) {
                db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                         errno, strerror(errno));
                ret = -1;
                break;
            }

            if (ioctl(fd, SIOCDELRT, rt) < 0) {
                db_msg("Fail to ioctl SIOCDELRT. errno[%d] errinfo[%s]\n",
                         errno, strerror(errno));
                ret = -1;
                close(fd);
                break;
            }

            close(fd);
        }
    }

    fclose(fp);
    return 0;
}


int NetManager::GetNetDevMtu(const std::string &netdev_name, unsigned int &mtu) const
{
    int          fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    mtu = 0;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFMTU. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    mtu = ifr.ifr_mtu;

    close(fd);
    return 0;
}


int NetManager::SetNetDevMtu(const std::string &netdev_name, const unsigned int mtu)
{
    int          fd = -1;
    struct ifreq ifr;

    if (mtu <= MTU_MINI_VAL) {
        db_msg("Input mtu:%d too min, error!\n",  mtu);
        return -1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    ifr.ifr_mtu = mtu;
    if (ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCSIFMTU. devname[%s] errno[%d] errinfo[%s]\n",
                netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int NetManager::GetNetDevDns(std::string &dns1, std::string &dns2) const
{

    int  i              = 0;
    char line_buf[1024] = {0};
    char tmp_buf[64]    = {0};
    char name[32]       = "nameserver";
    char *pret          = NULL;
    FILE *fp            = NULL;

    fp= fopen(PATH_DNS_CONT, "w+");
    if (fp == NULL) {
        db_msg("Open file:[%s] error! errno[%d] errinfo[%s]\n",
                PATH_DNS_CONT, errno, strerror(errno));
        return -1;
    }

    while(1)
    {
        memset(line_buf, 0, sizeof(line_buf));

        /* read  /etc/resolv.conf  file */
        pret = fgets(line_buf, sizeof(line_buf) - 1, fp);
        if(NULL == pret) {
            fclose(fp);
            return 0;
        }

        if (NULL != (pret = strstr(line_buf, name))) {
            if(i == 0) {
                sscanf(line_buf, "%*[^ ] %s", tmp_buf);
                dns1 = tmp_buf;
            }
            else if (i == 1) {
                sscanf(line_buf, "%*[^ ] %s", tmp_buf);
                dns2 = tmp_buf;
            }
            else if(i > 1) {
                break;
            }
            i++;
        }
    }

    fclose(fp);
    return 0;
}


int NetManager::SetNetDevDns(const std::string &dns1, const std::string &dns2)
{
    int  ret = 0;
    FILE *fp = NULL;

    fp= fopen(PATH_DNS_CONT, "w+");
    if (fp == NULL) {
        db_msg("Open file:[%s] error! errno[%d] errinfo[%s]\n",
                PATH_DNS_CONT, errno, strerror(errno));
        return -1;
    }

    ret = fprintf(fp, "nameserver %s\n", dns1.c_str());
    if (ret < 0) {
        db_msg("Write dns1 to file:[%s] error! errno[%d] errinfo[%s]\n",
                PATH_DNS_CONT, errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    ret = fprintf(fp, "nameserver %s\n", dns2.c_str());
    if (ret < 0) {
        db_msg("Write dns2 to file:[%s] error! errno[%d] errinfo[%s]\n",
                PATH_DNS_CONT, errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int NetManager::EnableNetDevStatic()
{
    netlink_type_ = NETLINK_ETH_STATIC;

    return 0;
}

int NetManager::EnableNetDevDhcpc(const std::string &netdev_name)
{
	return 0; //4G not use
    int  ret       = 0;
    char buf[1024] = {0};

    memset(buf, 0, sizeof(buf));
    ret = snprintf(buf, sizeof(buf)-1, CMD_RUN_DHCPC, netdev_name.c_str());
    if (ret <= 0) {
        db_msg("Do snprintf CMD_RUN_DHCPC fail! errno[%d] errinfo[%s]\n",
                         errno, strerror(errno));
        return -1;
    }

    string ip;
    GetNetDevIp(netdev_name, ip);

    FILE *fp = NULL;
    char result[128] = {0};
    const char *cmd = "pidof udhcpc";

    fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen failed");
        pclose(fp);
        return -errno;
    }

    int udhcpc_pid = 0;
    if (fgets(result, 128, fp) != NULL) {
        udhcpc_pid = atoi(result);
        db_info("udhcpc run with pid[%d]", udhcpc_pid);
    }

    pclose(fp);

    if (udhcpc_pid > 0 && ip == "") {
        db_warn("net link is broken, re-run udhcpc");
        system(CMD_KILL_DHCPC);
        system(buf);
    } else if (udhcpc_pid == 0 || ip == "") {
        db_info("run udhcpc for '%s'", netdev_name.c_str());
        set_all_fd_cloexec();
        system(buf);
    }

    db_msg("%s, dhcpc get ip: %s", netdev_name.c_str(), ip.c_str());

    if (netdev_name == "eth0")
        netlink_type_ = NETLINK_ETH_DHCP;
    else if (netdev_name == "wlan0")
        // TODO: should be NETLINK_WIFI_STA_DHCP
        netlink_type_ = NETLINK_WIFI_STA;

    dhcpc_running_ = true;

    return 0;
}

int NetManager::DisableNetDevDhcpc(void)
{
	return 0;
    db_info("kill udhcpc");

    system(CMD_KILL_DHCPC);

    dhcpc_running_ = false;

    return 0;
}

int NetManager::GetNetDevDhcpcStatus(const std::string &netdev_name) const
{
    return dhcpc_running_;
}


int NetManager::EnableNetDevDhcpd(const std::string &netdev_name)
{
    db_info("run udhcpd");

    set_all_fd_cloexec();
    system(CMD_RUN_DHCPD);

    dhcpd_running_ = true;

    return 0;
}

int NetManager::DisableNetDevDhcpd(void)
{
    db_info("kill udhcpd");

    system(CMD_KILL_DHCPD);
    dhcpd_running_ = false;

    return 0;
}

int NetManager::GetNetDevDhcpdStatus(const std::string &netdev_name) const
{
    return dhcpd_running_;
}


int NetManager::PingIp(unsigned int dstIp, int timeOut, bool &is_reach)
{
    return 0;
}

int NetManager::PingIp(const std::string &dstIp, int timeOut, bool &is_reach)
{
    return 0;
}


int NetManager::IsConnectInternet(int timeOut, bool &is_connetct)
{
    return 0;
}

int NetManager::GetNetDevConnectState(const std::string &netdev_name, bool &is_connetct)
{
    int fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_RUNNING) {
        is_connetct = true;
    } else {
        is_connetct = false;
    }

    close(fd);
    return 0;
}


int NetManager::UpNetDev(const std::string &netdev_name)
{
    int fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    ifr.ifr_flags |= IFF_UP;

    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCSIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int NetManager::DownNetDev(const std::string &netdev_name)
{
    int fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    ifr.ifr_flags &= (~IFF_UP);

    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCSIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


int NetManager::GetNetDevState(const std::string &netdev_name, NET_DEV_STATE &netdev_state)
{
    int fd = -1;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        db_msg("Fail to create socket! errno[%d] errinfo[%s]\n",
                 errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        db_msg("Fail to ioctl SIOCGIFFLAGS. devname[%s] errno[%d] errinfo[%s]\n",
                 netdev_name.c_str(), errno, strerror(errno));
        netdev_state = NET_DEV_STATE_BOTTON;
        close(fd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_UP) {
        netdev_state = NET_DEV_UP_STATE;
    } else {
        netdev_state = NET_DEV_DOWN_STATE;
    }

    close(fd);
    return 0;
}

int NetManager::SwitchToSoftAp(const char *ssid, const char *pwd, int mode, int encrypt, int freq)
{
    int ret = 0;

    if (NULL == ssid || NULL == pwd) {
        db_error("Input ssid or pwd is NULL!\n");
        return -1;
    }

    db_msg("ssid: %s, pwd: %s, mode: %d, encrypt: %d, freq: %d", ssid, pwd, mode, encrypt, freq);

    ret = this->DisableSoftap();
    if (ret < 0) {
        db_warn("reset net link failed, switch may not succesed");
    }

    netlink_type_ = NETLINK_NONE;

    /* Enable soft ap */
    SoftApConfig softap;
    softap.channel = 6;
    softap.hidden_ssid = 0;
    softap.frequency = freq;
    strncpy(softap.ssid, ssid, sizeof(softap.ssid));
    strncpy(softap.pswd, pwd, sizeof(softap.pswd));
    softap.security = SOFTAP_SECURITY_WPA_WPA2_PSK;

    if (softap_ctrl_->InitSoftAp(WIFI_INTERFACE_NAME) < 0) {
        db_error("Do InitSoftAp fail!\n");
        return -1;
    }
    usleep(200 * 1000);

    ret = softap_ctrl_->EnableSoftAp(softap);
    if (ret){
        db_error("Do EnableSoftAp fail:%d!\n", ret);
        return -1;
    }
    usleep(200 * 1000);

    ret = this->SetNetDevIp(WIFI_INTERFACE_NAME, "192.168.10.1");
    if (ret < 0) {
        db_warn("set ip address failed, dev[%s], wait 500ms and try again", WIFI_INTERFACE_NAME);
        system("ifconfig wlan0 down");
        usleep(500*1000);
        system("ifconfig wlan0 up");
        usleep(500*1000);
        ret = this->SetNetDevIp(WIFI_INTERFACE_NAME, "192.168.10.1");
        if (ret < 0) {
            db_error("set ip address failed, dev[%s]", WIFI_INTERFACE_NAME);
            return -1;
        }
    }

    netlink_type_ = NETLINK_WIFI_SOFTAP;

    this->EnableNetDevDhcpd(WIFI_INTERFACE_NAME);
    this->Notify(MSG_SOFTAP_ENABLED);
    /* Save Soft AP station config */
    std::string str_ssid = ssid;
    std::string str_pwd = pwd;
    SetWifiInfo(str_ssid, str_pwd);

    return 0;
}

int NetManager::DisableSoftap()
{
    if (softap_ctrl_->DisableSoftAp() < 0) {
        db_msg("DisableSoftap fialed");
        return -1;
    }

    usleep(20 * 1000);
    softap_ctrl_->ExitSoftAp();

    usleep(40 * 1000);

    this->DisableNetDevDhcpc();
    this->DisableNetDevDhcpd();

    return 0;
}
int NetManager::DisableAllNetLink()
{
    /* exit wifi mode, when softap is enabled */
    if (wifi_ctrl_->ExitWifiSta() < 0) {
        return -1;
    }

    /* exit softap mode, when wifi station mode is enable */
    if (softap_ctrl_->DisableSoftAp() < 0) {
        return -1;
    }

    usleep(20 * 1000);
    softap_ctrl_->ExitSoftAp();

    usleep(40 * 1000);

    this->DisableNetDevDhcpc();
    this->DisableNetDevDhcpd();

    return 0;
}

int NetManager::EnableMonitorMode(const string &interface)
{
    int ret = 0;

    ret = this->DisableAllNetLink();
    if (ret < 0) {
        db_warn("reset net link failed, switch may not succesed");
    }

    netlink_type_ = NETLINK_NONE;

    if (wifi_ctrl_->InitWifiSta(WIFI_INTERFACE_NAME) < 0) {
        db_error("InitWifiSta failed");
        return -1;
    }
    sleep(2);

    ret = set_wifi_monitor(interface.c_str(), 1);
    if (ret < 0) {
        db_error("Do set_wifi_monitor fail! ret:%d", ret);
        return -1;
    }

    return 0;
}

int NetManager::DisableMonitorMode(const string &interface)
{
    int ret = 0;

    ret = set_wifi_monitor(interface.c_str(), 0);
    if (ret < 0) {
        db_error("Do set_wifi_monitor fail! ret:%d", ret);
        return -1;
    }

    return 0;
}

int NetManager::SmartLinkLockChannel(const string &interface, int interval, int timeout)
{
    int ret = 0;

    ret = lock_smartlink_wifi_chn(interface.c_str(), interval, timeout);
    if (ret < 0) {
        db_error("Do lock_smartlink_wifi_chn fail! ret:%d", ret);
        return -1;
    }

    return 0;
}

int NetManager::SmartLinkParseAPInfo(const string &interface, string &ssid,
                                        string &pwd, unsigned char &note_num, int interval)
{
    int ret = 0;
    SMARTLINK_AP_S ap_info;
    memset(&ap_info, 0, sizeof(ap_info));

    ret = parser_smartlink_config(interface.c_str(), interval, &ap_info);
    if (ret < 0) {
        db_error("Do parser_smartlink_config fail! ret:%d", ret);
        return -1;
    }

    ssid = ap_info.ssid;
    pwd = ap_info.psswd;
    note_num = ap_info.random_num;

    db_info("parse ok, ssid: %s, pwd: %s", ssid.c_str(), pwd.c_str());

    return 0;
}

int NetManager::SmartLinkResultReturn(unsigned char note_num)
{
    int ret = 0;

    ret = send_broadcast_val(note_num);
    if (ret < 0) {
        db_error("Do send_broadcast_val fail! ret:%d", ret);
        return -1;
    }
    return 0;
}


void NetManager::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("msg received: %d", msg);

    switch (msg) {
        case MSG_SOFTAP_DISABLED:
			//db_warn("[debug_jaosn]: upadate MSG_SOFTAP_DISABLED\n");
            this->Notify(MSG_SOFTAP_DISABLED);
            break;
        case MSG_SOFTAP_ENABLED:
            netlink_type_ = NETLINK_WIFI_SOFTAP;
            this->Notify(MSG_SOFTAP_ENABLED);
            break;
        case MSG_WIFI_DISABLED:
            netlink_type_ = NETLINK_NONE;
            break;
        case MSG_WIFI_ENABLED: {
                static int times = 1;

                db_debug("times: %d", times);
                db_debug("netlink type: %d", netlink_type_);

                if (times == 1) db_msg("for count, so just ignore this msg");

                if (netlink_type_ == NETLINK_WIFI_SOFTAP) {
                    if (times++ == 2) {
                        this->Notify(MSG_SOFTAP_SWITCH_DONE);
                        times = 1;
                    }
                } else if (netlink_type_ == NETLINK_WIFI_STA) {
                    if (times++ == 2) {
                        this->Notify(MSG_WIFI_SWITCH_DONE);
                        times = 1;
                    }
                } else
                    times = 1;
            }
            break;
        case MSG_WIFI_CONNECTED:
            netlink_type_ = NETLINK_WIFI_STA;
            break;
        case MSG_WIFI_DISCONNECTED:
            netlink_type_ = NETLINK_NONE;
            break;
        case MSG_WIFI_SCAN_SCANING:
            break;
        case MSG_WIFI_SCAN_END:
            break;
        case MSG_ETH_DISCONNECT:
        {
            if (dhcpc_running_ && (netlink_type_ == NETLINK_ETH_DHCP)) {
                DisableNetDevDhcpc();
            }

            if (netlink_type_ != NETLINK_WIFI_STA
                && netlink_type_ != NETLINK_WIFI_SOFTAP
                && netlink_type_ != NETLINK_ETH_STATIC) {
                netlink_type_ = NETLINK_NONE;
            }

            this->Notify(MSG_ETH_DISCONNECT);

            break;
        }
        case MSG_ETH_CONNECT_LAN:
        {
            if (netlink_type_ == NETLINK_WIFI_STA || netlink_type_ == NETLINK_WIFI_SOFTAP)
                break;

            if (dhcpc_running_) {
                netlink_type_ = NETLINK_ETH_DHCP;
                sleep(2);
                this->Notify(MSG_ETH_SWITCH_DONE);
            } else if (netlink_type_ == NETLINK_ETH_STATIC) {
                this->Notify(MSG_ETH_SWITCH_DONE);
                break;
            } else {
                EnableNetDevDhcpc("eth0");
            }
            break;
        }
        case MSG_ETH_CONNECT_INTERNET:
            break;
        default:
            break;
    }
}

void NetManager::GetNetLinkType(NetLinkType &netlink_type) const
{
    netlink_type = netlink_type_;
}

int NetManager::GetWifiInfo(std::string &p_ssid, std::string &p_pwd)
{
     if (!FILE_EXIST("/tmp/data/menu_config.lua"))
        return -1;
	 char info_buf[10]={0};
	string sub_Ssid;
    LuaConfig luaCfg;
    luaCfg.LoadFromFile("/tmp/data/menu_config.lua");
    p_ssid = luaCfg.GetStringValue("menu.camera.wifiinfo.ssid");
    if(p_ssid.empty())
    {
	char info[10] = {0};
	memset(info,0,sizeof(info));
	StorageManager *sm = StorageManager::GetInstance();
	sm->readCpuInfo(info_buf);
	sub_Ssid = info_buf;
	p_ssid = "Yi_";
	p_ssid +=sub_Ssid;
	luaCfg.SetStringValue("menu.camera.wifiinfo.ssid",  p_ssid);
    }

    p_pwd = luaCfg.GetStringValue("menu.camera.wifiinfo.password");
    int ret = luaCfg.SyncConfigToFile("/tmp/data/menu_config.lua", "menu");
    if (ret < 0) {
        db_error("Do SyncConfigToFile error! file:/tmp/data/menu_config.lua\n");
        return -1;
    }

    return 0;
}

int NetManager::SetWifiInfo(std::string &p_ssid, std::string &p_pwd)
{
    if (!FILE_EXIST("/tmp/data/menu_config.lua"))
       return -1;

    LuaConfig luaCfg;
    luaCfg.LoadFromFile("/tmp/data/menu_config.lua");

    luaCfg.SetStringValue("menu.camera.wifiinfo.ssid",  p_ssid);
    luaCfg.SetStringValue("menu.camera.wifiinfo.password",  p_pwd);
    int ret = luaCfg.SyncConfigToFile("/tmp/data/menu_config.lua", "menu");
    if (ret < 0) {
        db_error("Do SyncConfigToFile error! file:/tmp/data/menu_config.lua\n");
        return -1;
    }

    MenuConfigLua::GetInstance()->UpdateWifiInfo();
    
    return 0;
}
