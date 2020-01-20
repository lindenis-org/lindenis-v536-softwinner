/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file wifi_connector.h
 * @brief wifi连接控制模块
 *
 *  该模块提供上层应用对wifi的控制管理，状态查询等。如wifi工
 *  作模式设置（AP/STATION）。SSID设置，连接AP设置。连接状态
 *  连接状态查询等。
 *
 * @author id:
 * @date 2016-7-12
 *
 * @verbatim
    History:
   @endverbatim
 */


#pragma once

#include "common/subject.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

#include <string>
#include <list>
#include <stdio.h>
#include <vector>


namespace EyeseeLinux {


#define MAX_SSID_LEN  128
#define MAX_BSSID_LEN 32
#define MAX_PSWD_LEN  64


/**
 * @brief wifi状态
 */
typedef enum {
    WIFI_CONNECT = 0,  /**< WIFI已经连接(station 模式下) */
    WIFI_DISCONNECT,   /**< WIFI断开连接(station 模式下) */
    WIFI_ENABLE,       /**< WIFI启动 */
    WIFI_DISABLE,      /**< WIFI关闭 */
} WifiConnectStatus;


typedef enum {
    WIFI_ALG_CCMP = 0,
    WIFI_ALG_TKIP,
    WIFI_ALG_CCMP_TKIP,
} WifiAlgType;


typedef enum {
    WIFI_SECURITY_OPEN = 0,
    WIFI_SECURITY_WEP,
    WIFI_SECURITY_WPA_WPA2_EAP,
    WIFI_SECURITY_WPA_WPA2_PSK,
    WIFI_SECURITY_WAPI_CERT,
    WIFI_SECURITY_WAPI_PSK
} WifiSecurityType;


typedef enum {
    WIFI_EVENT_ENABLE = 0,
    WIFI_EVENT_DISABLE,
    WIFI_EVENT_CONNECTED,
    WIFI_EVENT_DISCONNECTED,
    WIFI_EVENT_SCANING,
    WIFI_EVENT_SCAN_END,
    WIFI_EVENT_WPA_STOPPED,
} WifiEventType;


typedef enum {
    WIFI_SCANING = 0,
    WIFI_SCANEND,
    WIFI_SCANEND_BUTT,
} WifiScanStatus;


typedef struct {
    char             ssid[MAX_SSID_LEN];
    char             bssid[MAX_BSSID_LEN];
    unsigned int     frequency;
    unsigned int     db;
    unsigned int     hidden_ssid;
    WifiAlgType      alg_type;
    WifiSecurityType security;
} WifiApInfo;


typedef struct {
    char             ssid[MAX_SSID_LEN];
    char             bssid[MAX_BSSID_LEN];
    char             psswd[MAX_PSWD_LEN];
    WifiAlgType      alg_type;
    WifiSecurityType security;
} WifiConnectInfo;


class WifiController
    : public ISubjectWrap(WifiController)
{
    public:
        WifiController();

        ~WifiController();

        int InitWifiSta(const std::string &wifi_name);
        int ExitWifiSta();
        int ConnectAp(WifiConnectInfo &connect_info);
        int GetConnectApInfo(WifiConnectInfo &connect_info);
        int DisconnectAp();
        int StartScanAp();
        int GetScanResults(WifiScanStatus &scan_state, std::vector<WifiApInfo> &ap_list);
        int GetWifiConnectorStatus(WifiConnectStatus &wifi_status) const;
        int UpdateWifiSta(WifiEventType &wifi_event);   /* just for be call callback function */

        int SaveWifiStaConfig();
        int DefaultWifiStaConfig();

        static void *WifiLoopThread(void *context);

    private:
        pthread_t wifi_loop_thread_id_;
        LuaConfig        *lua_cfg_;
        WifiConnectStatus m_connect_status;
        std::string       m_wifi_name;
        WifiConnectInfo   m_connect_info;
        WifiScanStatus    m_scan_state;

        int LoadWifiStaConfig();
        //static void *WifiStaEventProcess(WIFI_STA_EVENT_E event, void *pdata);
};

} /* EyeseeLinux */
