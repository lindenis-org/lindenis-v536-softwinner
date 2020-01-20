/**********************************************************
* Copyright (C), 2016, AllwinnerTech. Co., Ltd.  *
***********************************************************/

/**
 * @file wifi_connector.cpp
 * @author
 * @date 2016-7-12
 * @version v0.3
 * @brief wifi连接控制模块
 * @see wifi_connector.h
 * @verbatim
 *  History:
 * @endverbatim
 */


#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/net_manager.h"
#include "common/app_log.h"

#include "wifi/wifi_sta.h"

#include <string.h>

#undef LOG_TAG
#define LOG_TAG "WifiController"

using namespace EyeseeLinux;
using namespace std;

#define WIFISTA_CONFIG_FILE "/tmp/data/net_config.lua"

void WifiStaEventProcess(WIFI_STA_EVENT_E event, void *pdata)
{
    int             ret = 0;
    WifiEventType   wifi_event;
    WifiController *wifi_ctrl = reinterpret_cast<WifiController*>(pdata);

    switch (event) {
        case WIFI_STA_EVENT_UP:
            wifi_event = WIFI_EVENT_ENABLE;
            break;
        case WIFI_STA_EVENT_DOWN:
            wifi_event = WIFI_EVENT_DISABLE;
            break;
        case WIFI_STA_EVENT_CONNECTED:
            wifi_event = WIFI_EVENT_CONNECTED;
            break;
        case WIFI_STA_EVENT_DISCONNECTED:
            wifi_event = WIFI_EVENT_DISCONNECTED;
            break;
        case WIFI_STA_EVENT_SCANING:
            wifi_event = WIFI_EVENT_SCANING;
            break;
        case WIFI_STA_EVENT_SCAN_END:
            wifi_event = WIFI_EVENT_SCAN_END;
            break;
        case WIFI_STA_EVENT_SUPP_STOPPED:
            wifi_event = WIFI_EVENT_WPA_STOPPED;
            break;
        default:
            db_msg ("Unkown wifi event:%d", event);
            return;
            break;
    }

    ret = wifi_ctrl->UpdateWifiSta(wifi_event);
    if (ret) {
        db_msg("[FUN]:%s [LINE]%d  Do UpdateWifiSta error! ret:%d", __func__, __LINE__, ret);
    }
}


WifiController::WifiController()
{
    lua_cfg_         = new LuaConfig();
    m_connect_status = WIFI_DISCONNECT;
    m_scan_state     = WIFI_SCANEND_BUTT;
    memset(&m_connect_info, 0, sizeof(m_connect_info));

    int ret = LoadWifiStaConfig();
    if (ret) {
        printf("[FUN]:%s [LINE]:%d  Do LoadSoftApConfig fail:%d !\n", __func__, __LINE__, ret);
    }

    //ThreadCreate(&wifi_loop_thread_id_, NULL, WifiController::WifiLoopThread, this);
}


WifiController::~WifiController()
{
    if (lua_cfg_) {
        delete lua_cfg_;
    }
}


int WifiController::LoadWifiStaConfig()
{
    int ret = 0, tmp = 0;
    std::string str;

    if (NULL == lua_cfg_) {
        printf("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(WIFISTA_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", WIFISTA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(WIFISTA_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again", WIFISTA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(WIFISTA_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!", WIFISTA_CONFIG_FILE);
            return -1;
        }
    }

    str = lua_cfg_->GetStringValue("network.wifi.ssid");
    strncpy(m_connect_info.ssid, str.c_str(), sizeof(m_connect_info.ssid) - 1);

    str = lua_cfg_->GetStringValue("network.wifi.pwd");
    strncpy(m_connect_info.psswd, str.c_str(), sizeof(m_connect_info.psswd) - 1);

    tmp = lua_cfg_->GetIntegerValue("network.wifi.alg_type");
    m_connect_info.alg_type = (WifiAlgType)tmp;

    tmp = lua_cfg_->GetIntegerValue("network.wifi.security");
    m_connect_info.security = (WifiSecurityType)tmp;

    return 0;
}


int WifiController::SaveWifiStaConfig()
{
    int ret = 0, tmp = 0;
    std::string str;

    if (NULL == lua_cfg_) {
        printf("The lua_cfg_ is NULL! error! \n");
        return -1;
    }

    if (!FILE_EXIST(WIFISTA_CONFIG_FILE)) {
        db_warn("config file %s not exist, copy default from /usr/share/app/sdv", WIFISTA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
    }

    ret = lua_cfg_->LoadFromFile(WIFISTA_CONFIG_FILE);
    if (ret) {
        db_warn("Load %s failed, copy backup and try again", WIFISTA_CONFIG_FILE);
        system("cp -f /usr/share/app/sdv/net_config.lua /tmp/data/");
        ret = lua_cfg_->LoadFromFile(WIFISTA_CONFIG_FILE);
        if (ret) {
            db_error("Load %s failed!", WIFISTA_CONFIG_FILE);
            return -1;
        }
    }

    lua_cfg_->SetStringValue("network.wifi.ssid",  m_connect_info.ssid);
    lua_cfg_->SetStringValue("network.wifi.pwd", m_connect_info.psswd);
    lua_cfg_->SetIntegerValue("network.wifi.alg_type", (int)m_connect_info.alg_type);
    lua_cfg_->SetIntegerValue("network.wifi.security", (int)m_connect_info.security);

    ret = lua_cfg_->SyncConfigToFile(WIFISTA_CONFIG_FILE, "network");
    if (ret) {
        printf("Do SyncConfigToFile error! file:%s\n", WIFISTA_CONFIG_FILE);
        return 0;
    }

    return 0;
}


int WifiController::DefaultWifiStaConfig()
{
    return 0;
}


void *WifiController::WifiLoopThread(void * context)
{
    WifiController *wifi_ctrl = reinterpret_cast<WifiController*>(context);
    prctl(PR_SET_NAME, "WIFIEventLoop", 0, 0, 0);

    while(1) {
        //wifi_ctrl->Notify(MSG_WIFI_DISABLED);
        //sleep(1);
        //wifi_ctrl->Notify(MSG_WIFI_ENABLE);
        //sleep(1);
        sleep(10);
    }

    return NULL;
}


int WifiController::InitWifiSta(const std::string &wifi_name)
{
    int ret = 0;

    m_wifi_name = wifi_name;

    ret = wifi_sta_init();
    if (ret) {
        db_msg ("Do wifi_sta_init fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_open(wifi_name.c_str());
    if (ret) {
        db_msg ("Do wifi_sta_open fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_start(m_wifi_name.c_str());
    if (ret) {
        db_msg ("Do wifi_sta_start fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_register_eventcall(m_wifi_name.c_str(), WifiStaEventProcess, (void *)this);
    if (ret) {
        db_msg ("Do wifi_sta_register_eventcall fail! ret:%d", ret);
        return -1;
    }

    this->Notify(MSG_WIFI_ENABLED);

    return ret;
}


int WifiController::ExitWifiSta()
{
    int ret = 0;

    ret = wifi_sta_unregister_eventcall(m_wifi_name.c_str());
    if (ret) {
        db_msg ("Do wifi_sta_register_eventcall fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_stop(m_wifi_name.c_str());
    if (ret) {
        db_msg ("Do wifi_sta_stop fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_close(m_wifi_name.c_str());
    if (ret) {
        db_msg ("Do wifi_sta_close fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_sta_exit();
    if (ret) {
        db_msg ("Do wifi_sta_exit fail! ret:%d", ret);
        return -1;
    }

    m_connect_status = WIFI_DISABLE;
    this->Notify(MSG_WIFI_DISABLED);

    return ret;
}


int WifiController::ConnectAp(WifiConnectInfo &connect_info)
{
    int ret = 0;
    WIFI_STA_AP_INFO_S ap_info;

    memcpy(ap_info.ssid,  connect_info.ssid,  sizeof(ap_info.ssid));
    memcpy(ap_info.psswd, connect_info.psswd, sizeof(ap_info.psswd));
    memcpy(ap_info.bssid, connect_info.bssid, sizeof(ap_info.bssid));

    switch (connect_info.security) {
        case WIFI_SECURITY_OPEN:
            ap_info.security = WIFI_STA_SECURITY_OPEN;
            break;
        case WIFI_SECURITY_WEP:
            ap_info.security = WIFI_STA_SECURITY_WEP;
            break;
        case WIFI_SECURITY_WPA_WPA2_EAP:
            ap_info.security = WIFI_STA_SECURITY_WPA_WPA2_EAP;
            break;
        case WIFI_SECURITY_WPA_WPA2_PSK:
            ap_info.security = WIFI_STA_SECURITY_WPA_WPA2_PSK;
            break;
        case WIFI_SECURITY_WAPI_CERT:
            ap_info.security = WIFI_STA_SECURITY_WAPI_CERT;
            break;
        case WIFI_SECURITY_WAPI_PSK:
            ap_info.security = WIFI_STA_SECURITY_WAPI_PSK;
            break;
        default:
            db_msg("[FUN]:%s [LINE]%d  Input security:%d error!", __func__, __LINE__, connect_info.security);
            return -1;
            break;
    }

    switch (connect_info.alg_type) {
        case WIFI_ALG_CCMP:
            ap_info.alg_type = WIFI_STA_ALG_CCMP;
            break;
        case WIFI_ALG_TKIP:
            ap_info.alg_type = WIFI_STA_ALG_TKIP;
            break;
        case WIFI_ALG_CCMP_TKIP:
            ap_info.alg_type = WIFI_STA_ALG_CCMP_TKIP;
            break;
        default:
            db_msg("[FUN]:%s [LINE]%d  Input alg_type:%d error!", __func__, __LINE__, connect_info.alg_type);
            return -1;
            break;
    }

    ret = wifi_sta_connect(m_wifi_name.c_str(), &ap_info);
    if (ret < 0) {
        db_msg("[FUN]:%s [LINE]%d  Do wifi_sta_connect error! ret:%d", __func__, __LINE__, ret);
        return ret;
    }

    memcpy(&m_connect_info, &connect_info, sizeof(m_connect_info));

    db_msg("connect finished");

    return ret;
}


int WifiController::GetConnectApInfo(WifiConnectInfo &connect_info)
{
    memcpy(&connect_info, &m_connect_info, sizeof(m_connect_info));
    return 0;
}


int WifiController::DisconnectAp()
{
    int ret = 0;

    ret = wifi_sta_disconnect(m_wifi_name.c_str());
    if (ret < 0) {
        db_msg("[FUN]:%s [LINE]%d  Do wifi_sta_disconnect error! ret:%d", __func__, __LINE__, ret);
        return ret;
    }

    return ret;
}


int WifiController::StartScanAp()
{
    int ret = 0;

    ret = wifi_sta_start_scan(m_wifi_name.c_str());
    if (ret < 0) {
        db_msg("[FUN]:%s [LINE]%d  Do wifi_sta_start_scan error! ret:%d", __func__, __LINE__, ret);
        return ret;
    }

    m_scan_state = WIFI_SCANING;

    return ret;
}


int WifiController::GetScanResults(WifiScanStatus &scan_state, std::vector<WifiApInfo> &ap_list)
{
    int ret = 0;
    int cnt = 0;
    WifiApInfo         ap_info;
    WIFI_STA_AP_LIST_S scan_list;

    if (WIFI_SCANING == m_scan_state) {
        scan_state = WIFI_SCANING;
        return 0;
    }

    memset(&scan_list, 0, sizeof(scan_list));
    ret = wifi_sta_get_scan_results(m_wifi_name.c_str(), &scan_list);
    if (ret < 0) {
        db_msg("[FUN]:%s [LINE]%d  Do wifi_sta_get_scan_results error! ret:%d", __func__, __LINE__, ret);
        return ret;
    }

    for (cnt = 0; cnt < scan_list.ap_list_num; cnt++) {
        memcpy(ap_info.ssid,  scan_list.ap_list[cnt].ssid,  sizeof(ap_info.ssid));
        memcpy(ap_info.bssid, scan_list.ap_list[cnt].bssid, sizeof(ap_info.bssid));
        ap_info.frequency   = scan_list.ap_list[cnt].frequency;
        ap_info.db          = scan_list.ap_list[cnt].db;
        ap_info.hidden_ssid = scan_list.ap_list[cnt].hidden_ssid;

        switch (scan_list.ap_list[cnt].security) {
            case WIFI_STA_SECURITY_OPEN:
                ap_info.security = WIFI_SECURITY_OPEN;
                break;
            case WIFI_STA_SECURITY_WEP:
                ap_info.security = WIFI_SECURITY_WEP;
                break;
            case WIFI_STA_SECURITY_WPA_WPA2_EAP:
                ap_info.security = WIFI_SECURITY_WPA_WPA2_EAP;
                break;
            case WIFI_STA_SECURITY_WPA_WPA2_PSK:
                ap_info.security = WIFI_SECURITY_WPA_WPA2_PSK;
                break;
            case WIFI_STA_SECURITY_WAPI_CERT:
                ap_info.security = WIFI_SECURITY_WAPI_CERT;
                break;
            case WIFI_STA_SECURITY_WAPI_PSK:
                ap_info.security = WIFI_SECURITY_WAPI_PSK;
                break;
            default:
                db_msg("[FUN]:%s [LINE]%d  Input security:%d error!", __func__, __LINE__, scan_list.ap_list[cnt].security);
                break;
        }

        switch (scan_list.ap_list[cnt].alg_type) {
            case WIFI_STA_ALG_CCMP:
                ap_info.alg_type = WIFI_ALG_CCMP;
                break;
            case WIFI_STA_ALG_TKIP:
                ap_info.alg_type = WIFI_ALG_TKIP;
                break;
            case WIFI_STA_ALG_CCMP_TKIP:
                ap_info.alg_type = WIFI_ALG_CCMP_TKIP;
                break;
            default:
                db_msg("[FUN]:%s [LINE]%d  Input alg_type:%d error!", __func__, __LINE__, scan_list.ap_list[cnt].alg_type);
                break;
        }

        ap_list.push_back(ap_info);
    }

    scan_state = WIFI_SCANEND;

    return ret;
}


int WifiController::UpdateWifiSta(WifiEventType &wifi_event)
{
    switch (wifi_event) {
        case WIFI_EVENT_SCANING:
            m_scan_state = WIFI_SCANING;
            break;

        case WIFI_EVENT_SCAN_END:
            m_scan_state = WIFI_SCANEND;
            this->Notify(MSG_WIFI_SCAN_END);
            break;

        case WIFI_EVENT_CONNECTED:
            m_connect_status = WIFI_CONNECT;
            this->Notify(MSG_WIFI_CONNECTED);
            break;

        case WIFI_EVENT_DISCONNECTED:
            m_connect_status = WIFI_DISCONNECT;
            this->Notify(MSG_WIFI_DISCONNECTED);
            break;

        case WIFI_EVENT_ENABLE:
            m_connect_status = WIFI_ENABLE;
            this->Notify(MSG_WIFI_ENABLED);
            break;

        case WIFI_EVENT_DISABLE:
            m_connect_status = WIFI_DISABLE;
            this->Notify(MSG_WIFI_DISABLED);
            break;

        default:
            break;
    }

    return 0;
}


int WifiController::GetWifiConnectorStatus(WifiConnectStatus &wifi_status) const
{
    wifi_status = m_connect_status;
    return 0;
}

