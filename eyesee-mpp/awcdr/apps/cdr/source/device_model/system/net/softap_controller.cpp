/**********************************************************
* Copyright (C), 2016, AllwinnerTech. Co., Ltd.  *
***********************************************************/

/**
 * @file softap_controller.cpp
 * @author
 * @date 2016-7-12
 * @version v0.3
 * @brief wifi连接控制模块
 * @see softap_controller.h
 * @verbatim
 *  History:
 * @endverbatim
 */

#include "device_model/system/net/softap_controller.h"
#include "device_model/system/net/net_manager.h"
#include "common/app_log.h"

#include "wifi/wifi_ap.h"
#include <string.h>

#undef LOG_TAG
#define LOG_TAG "SoftApController"

using namespace EyeseeLinux;
using namespace std;


SoftApController::SoftApController()
{
    mAp_state = SOFTAP_DISABLE;
    memset(&mAp_cfg, 0, sizeof(mAp_cfg));
}

SoftApController::~SoftApController()
{
}


int SoftApController::DefaultSoftApConfig()
{
    return 0;
}


int SoftApController::InitSoftAp(const std::string &wifi_name)
{
    int ret = 0;

    ret = wifi_ap_init();
    if (ret < 0) {
        db_error("Do wifi_ap_init fail! ret:%d", ret);
        return -1;
    }

    mWifi_name = wifi_name;
    mAp_state = SOFTAP_INIT;
    return 0;
}


int SoftApController::ExitSoftAp()
{
    int ret = 0;

    ret = wifi_ap_exit();
    if (ret < 0) {
        db_error("Do wifi_ap_exit fail! ret:%d", ret);
        return -1;
    }

    mAp_state = SOFTAP_UNINIT;
    return 0;
}


int SoftApController::EnableSoftAp(SoftApConfig &ap_cfg)
{
    int           ret = 0;
    WIFI_AP_CFG_S wifi_cfg;

//    this->Notify(MSG_SOFTAP_ENABLE);

    ret = wifi_ap_open(mWifi_name.c_str());
    if (ret < 0) {
        db_error("Do wifi_ap_open fail! ret:%d", ret);
        return -1;
    }

    wifi_cfg.channel     = ap_cfg.channel;
    wifi_cfg.hidden_ssid = ap_cfg.hidden_ssid;
    wifi_cfg.frequency   = ap_cfg.frequency;
    memcpy(wifi_cfg.ssid, ap_cfg.ssid,  sizeof(wifi_cfg.ssid));
    memcpy(wifi_cfg.pswd, ap_cfg.pswd, sizeof(wifi_cfg.pswd));
    switch (ap_cfg.security) {
        case SOFTAP_SECURITY_OPEN:
            wifi_cfg.security = WIFI_AP_SECURITY_OPEN;
            break;
        case SOFTAP_SECURITY_WEP:
            wifi_cfg.security = WIFI_AP_SECURITY_WEP;
            break;
        case SOFTAP_SECURITY_WPA_WPA2_EAP:
            wifi_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_EAP;
            break;
        case SOFTAP_SECURITY_WPA_WPA2_PSK:
            wifi_cfg.security = WIFI_AP_SECURITY_WPA_WPA2_PSK;
            break;
        case SOFTAP_SECURITY_WAPI_CERT:
            wifi_cfg.security = WIFI_AP_SECURITY_WAPI_CERT;
            break;
        case SOFTAP_SECURITY_WAPI_PSK:
            wifi_cfg.security = WIFI_AP_SECURITY_WAPI_PSK;
            break;
        default:
            db_error("Input security:%d error!", wifi_cfg.security);
            return -1;
            break;
    }

    usleep(500*1000);

    ret = wifi_ap_start(mWifi_name.c_str(), &wifi_cfg);
    if (ret < 0) {
        db_error("Do wifi_ap_start fail! ret:%d", ret);
        return -1;
    }

//    this->Notify(MSG_SOFTAP_ENABLED);
    mAp_state = SOFTAP_ENABLE;
    mAp_cfg   = ap_cfg;
    return 0;
}


int SoftApController::DisableSoftAp()
{
    int ret = 0;

    ret = wifi_ap_stop(mWifi_name.c_str());
    if (ret < 0) {
        db_error("Do wifi_ap_stop fail! ret:%d", ret);
        return -1;
    }

    ret = wifi_ap_close(mWifi_name.c_str());
    if (ret < 0) {
        db_error("Do wifi_ap_close fail! ret:%d", ret);
        return -1;
    }

    mAp_state = SOFTAP_DISABLE;
    this->Notify(MSG_SOFTAP_DISABLED);
    return 0;
}


int SoftApController::GetSoftApConfig(SoftApConfig & ap_cfg) const
{
    ap_cfg = mAp_cfg;
    return 0;
}


int SoftApController::GetSoftApWorkStatus(SoftApWorkStatus &ap_state) const
{
    ap_state = mAp_state;
    return 0;
}
