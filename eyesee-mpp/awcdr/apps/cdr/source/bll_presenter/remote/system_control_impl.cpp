/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file system_control_impl.cpp
 * @brief 系统控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */

#include "system_control_impl.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/system/net/softap_controller.h"
#include "bll_presenter/presenter.h"
#include "common/app_def.h"
#include "common/app_log.h"

#include <string>
#include <stdlib.h>
#include <sys/time.h>

#undef LOG_TAG
#define LOG_TAG "SystemControlImpl"

using namespace EyeseeLinux;
using namespace std;

extern int g_exit_action;

SystemControlImpl::SystemControlImpl(IPresenter *presenter)
    : presenter_(presenter)
    , net_manager_(NetManager::GetInstance())
{
}

SystemControlImpl::~SystemControlImpl()
{
}

int SystemControlImpl::GetIpAddress(char *interface, char *ipaddr, int size)
{
    string ip;
    net_manager_->GetNetDevIp(interface, ip);

    strncpy(ipaddr, ip.c_str(), size);

    return 0;
}

int SystemControlImpl::SetNetworkAttr(const AWNetAttr &net_attr)
{
    int ret = 0;

    // reset net link
    net_manager_->DisableAllNetLink();

    if (net_attr.dhcp_enable) {
        usleep(200 * 1000);
        ret = net_manager_->EnableNetDevDhcpc(net_attr.interface);
    } else {
        ret = net_manager_->EnableNetDevStatic();
        usleep(20 * 1000);
        ret = net_manager_->SetNetDevIp(net_attr.interface, net_attr.ip);
        ret = net_manager_->SetNetDevMask(net_attr.interface, net_attr.mask);
        ret = net_manager_->AddDefaultGateWay(net_attr.interface, net_attr.gateway);
        ret = net_manager_->SetNetDevDns(net_attr.dns1, net_attr.dns2);
    }

    Net_Attr_List attr_list;
    net_manager_->LoadNetConfig(attr_list);

    attr_list.net_attr[0].dhcp_enable = net_attr.dhcp_enable;
    strncpy(attr_list.net_attr[0].ip,   net_attr.ip, sizeof(attr_list.net_attr[0].ip));
    strncpy(attr_list.net_attr[0].mask, net_attr.mask, sizeof(attr_list.net_attr[0].mask));
    strncpy(attr_list.net_attr[0].gateway, net_attr.gateway, sizeof(attr_list.net_attr[0].gateway));
    strncpy(attr_list.net_attr[0].dns1, net_attr.dns1,sizeof(attr_list.net_attr[0].dns1));
    strncpy(attr_list.net_attr[0].dns2, net_attr.dns2, sizeof(attr_list.net_attr[0].dns2));

    net_manager_->SaveNetConfig(attr_list);

    return ret;
}

int SystemControlImpl::GetNetworkAttr(AWNetAttr &net_attr)
{
    Net_Attr_List attr_list;
    std::string str, tmp_str;
    char tmp[32] = {0};

    net_manager_->LoadNetConfig(attr_list);

    net_attr.dhcp_enable = attr_list.net_attr[0].dhcp_enable;
    strncpy(net_attr.interface, attr_list.net_attr[0].interface, sizeof(net_attr.interface));

    net_manager_->GetNetDevMac(net_attr.interface, (unsigned char *)tmp);
    snprintf(net_attr.mac, sizeof(net_attr.mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);

    net_manager_->GetNetDevIp(net_attr.interface, str);
    strncpy(net_attr.ip, str.c_str(), sizeof(net_attr.ip));

    net_manager_->GetNetDevGateWay(net_attr.interface, str);
    strncpy(net_attr.gateway, str.c_str(), sizeof(net_attr.gateway));

    net_manager_->GetNetDevMask(net_attr.interface, str);
    strncpy(net_attr.mask, str.c_str(),sizeof(net_attr.mask));

    strncpy(net_attr.dns1, net_attr.gateway, sizeof(net_attr.dns1));
    strncpy(net_attr.dns2, "0.0.0.0", sizeof(net_attr.dns2));

    return 0;
}

int SystemControlImpl::GetNetworkAttrList(AWNetAttrList &net_attr_list)
{
    db_msg("No complete!");
    return 0;
}

int SystemControlImpl::GetLocalDateTime(struct tm &tm)
{
    time_t now_time = time(NULL);

    memcpy(&tm, localtime(&now_time), sizeof(struct tm));

    db_info("%s, %s", tm.tm_zone, asctime(&tm));

    return 0;
}

int SystemControlImpl::SetLocalDateTime(struct tm &tm)
{
    int ret;

    time_t time = mktime(&tm);

    struct timeval tv;
    tv.tv_sec = time + tm.tm_gmtoff;
    tv.tv_usec = 0;

    ret = settimeofday(&tv, NULL);
    if (ret < 0) {
       db_error("set time failed");
    }

    db_info("%s, %s", tm.tm_zone, asctime(&tm));

    return ret;
}

int SystemControlImpl::GetUTCDateTime(struct tm &tm)
{
    time_t now_time = time(NULL);

    memcpy(&tm, gmtime(&now_time), sizeof(struct tm));

    db_info("%s, %s", tm.tm_zone, asctime(&tm));

    return 0;
}

int SystemControlImpl::SetNtpConfig(const AWNtpConfig &ntp_cfg)
{
    db_msg("No complete!");
    return 0;
}

int SystemControlImpl::GetNtpConfig(AWNtpConfig &ntp_cfg)
{
    db_msg("No complete!");
    return 0;
}

int SystemControlImpl::GetApList(AWWifiAp **wifi_ap, int cnt)
{
    db_msg("");
    return 0;
}

int SystemControlImpl::SetSoftAp(const char *ssid, const char *pwd, int mode, int enctype, int freq)
{
    int ret = 0;

    ret = net_manager_->SwitchToSoftAp(ssid, pwd, mode, enctype, freq);

    return ret;
}

int SystemControlImpl::GetSoftAp(char *ssid, char *pwd, int *mode, int *enctype, int *freq)
{
    if (NULL == ssid || NULL == pwd) {
        db_error("Input ssid or pwd is NULL!\n");
        return -1;
    }

    int ret = 0;
    LuaConfig lua_cfg;
    string str;

    const char *config_file = "/tmp/data/net_config.lua";
    if (!FILE_EXIST(config_file)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv/", config_file);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    ret = lua_cfg.LoadFromFile(config_file);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", config_file);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
        ret = lua_cfg.LoadFromFile(config_file);
        if (ret) {
            db_error("Load %s failed!\n", config_file);
            return -1;
        }
    }

    str = lua_cfg.GetStringValue("network.softap.ssid_prefix");
    strcpy(ssid, str.c_str());

    str = lua_cfg.GetStringValue("network.softap.pwd");
    strcpy(pwd, str.c_str());

    *mode = lua_cfg.GetIntegerValue("network.softap.hidden_ssid");
    *freq = lua_cfg.GetIntegerValue("network.softap.frequency");
    int security = lua_cfg.GetIntegerValue("network.softap.security");;

    switch (security) {
        case SOFTAP_SECURITY_OPEN:
        default:
            *enctype = 0;
            break;

        case SOFTAP_SECURITY_WEP:
        case SOFTAP_SECURITY_WPA_WPA2_PSK:
        case SOFTAP_SECURITY_WAPI_PSK:
            *enctype = 1;
            break;

        case SOFTAP_SECURITY_WPA_WPA2_EAP:
        case SOFTAP_SECURITY_WAPI_CERT:
            *enctype = 2;
            break;
    }

    return 0;
}

int SystemControlImpl::SetWifi(const char *ssid, const char *pwd)
{
    int ret = 0;

    ret = net_manager_->SwitchToSoftAp(ssid, pwd,0,0,0);

    return ret;
}

int SystemControlImpl::GetWifi(char *ssid, char *pwd, char *mode, char *enctype)
{
    if (NULL == ssid || NULL == pwd) {
        db_error("Input ssid or pwd is NULL!\n");
        return -1;
    }

    int ret = 0;
    LuaConfig lua_cfg;
    string str;

    const char *config_file = "/tmp/data/net_config.lua";
    if (!FILE_EXIST(config_file)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", config_file);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    ret = lua_cfg.LoadFromFile(config_file);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again\n", config_file);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
        ret = lua_cfg.LoadFromFile(config_file);
        if (ret) {
            db_error("Load %s failed!\n", config_file);
            return -1;
        }
    }

    str = lua_cfg.GetStringValue("network.wifi.ssid");
    strcpy(ssid, str.c_str());

    str = lua_cfg.GetStringValue("network.wifi.pwd");
    strcpy(pwd, str.c_str());

    *mode = (char)lua_cfg.GetIntegerValue("network.wifi.alg_type");
    *enctype = (char)lua_cfg.GetIntegerValue("network.wifi.security");

    return 0;
}


int SystemControlImpl::DefaultSystemConfig(void)
{
    unlink("/tmp/data/net_config.lua");
    unlink("/tmp/data/record_config.lua");
    unlink("/tmp/data/ipc_config.lua");
    unlink("/tmp/data/event_config.lua");
    unlink("/tmp/data/image_config.lua");
    unlink("/tmp/data/overlay_config.lua");
    unlink("/tmp/data/media_config.lua");
    system("sync");
    sleep(1);

    system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/record_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/ipc_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/event_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/image_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/overlay_config.lua /tmp/data/");
    system("cp -f /usr/share/app/sdv/media_config.lua /tmp/data/");
    system("sync");
    sleep(1);

    RebootSystem(0);

    return 0;
}

int SystemControlImpl::RebootSystem(int delay_time)
{
    int ret = 0;

    db_msg("");

    if (delay_time < 0) {
        db_msg("Input delay_time:%d error!\n", delay_time);
    }

    g_exit_action = REBOOT;

    presenter_->PrepareExit();

    return ret;
}

int SystemControlImpl::SetSystemMaintain(const AWSystemTime &maintain_time)
{
    db_msg("");
    return 0;
}
