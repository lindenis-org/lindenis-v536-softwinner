/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file softap_controller.h
 * @brief softap连接控制模块
 *
 *  该模块提供上层应用对SoftAP的控制管理，状态查询等。
 *
 * @author id:
 * @date 2016-7-15
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

namespace EyeseeLinux {

#define AP_SSID_LEN    128
#define AP_PSWD_LEN    64

/**
 * @brief SoftAp使能状态
 */
typedef enum {
    SOFTAP_INIT,
    SOFTAP_UNINIT,
    SOFTAP_ENABLE,       /**< SoftAp启动 */
    SOFTAP_DISABLE,      /**< SoftAp关闭 */
} SoftApWorkStatus;


typedef enum {
    SOFTAP_SECURITY_OPEN = 0,
    SOFTAP_SECURITY_WEP,
    SOFTAP_SECURITY_WPA_WPA2_EAP,
    SOFTAP_SECURITY_WPA_WPA2_PSK,
    SOFTAP_SECURITY_WAPI_CERT,
    SOFTAP_SECURITY_WAPI_PSK,
    SOFTAP_SECURITY_BOTTON
} SoftApSecurity;


typedef struct {
    char  ssid[AP_SSID_LEN];
    char  pswd[AP_PSWD_LEN+1];
    int   channel;
    int   hidden_ssid;
    int   frequency;    //0-2.4GHz, 1-5GHz
    SoftApSecurity security;
} SoftApConfig;


class SoftApController
    : public ISubjectWrap(SoftApController)
{
    public:
        SoftApController();

        ~SoftApController();

        int InitSoftAp(const std::string &wifi_name);
        int ExitSoftAp();
        int EnableSoftAp(SoftApConfig &ap_cfg);
        int DisableSoftAp();
        int GetSoftApConfig(SoftApConfig &ap_cfg) const;
        int GetSoftApWorkStatus(SoftApWorkStatus &ap_state) const;
        int DefaultSoftApConfig();

    private:
        SoftApWorkStatus  mAp_state;
        SoftApConfig      mAp_cfg;
        std::string       mWifi_name;
};

} /* EyeseeLinux */
