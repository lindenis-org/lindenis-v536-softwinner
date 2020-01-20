/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file system_control_impl.h
 * @brief 系统控制接口
 * @author id:826
 * @version v0.3
 * @date 2016-08-29
 */
#pragma once

#include "interface/dev_ctrl_adapter.h"
#include "common/subject.h"

namespace EyeseeLinux
{

class NetManager;
class IPresenter;

/**
 * @brief 系统控制接口
 */
class SystemControlImpl
    : public DeviceAdapter::SystemControl
    , public ISubjectWrap(SystemControlImpl)
{
    public:
        SystemControlImpl(IPresenter *presenter);

        ~SystemControlImpl();

        int GetIpAddress(char *interface, char *ipaddr, int size);

        int SetNetworkAttr(const AWNetAttr &net_attr);

        int GetNetworkAttr(AWNetAttr &net_attr);

        int GetNetworkAttrList(AWNetAttrList &net_attr_list);

        int SetLocalDateTime(struct tm &tm);

        int GetLocalDateTime(struct tm &tm);

        int GetUTCDateTime(struct tm &tm);

        int SetNtpConfig(const AWNtpConfig &ntp_cfg);

        int GetNtpConfig(AWNtpConfig &ntp_cfg);

        int GetApList(AWWifiAp *wifi_ap[], int cnt);

        int SetSoftAp(const char *ssid, const char *pwd, int mode, int enctype, int freq);

        int GetSoftAp(char *ssid, char *pwd, int *mode, int *enctype, int *freq);

        int SetWifi(const char *ssid, const char *pwd);

        int GetWifi(char *ssid, char *pwd, char *mode, char *enctype);

        int DefaultSystemConfig(void);

        int RebootSystem(int delay_time);

        int SetSystemMaintain(const AWSystemTime &maintain_time);

    private:
        IPresenter *presenter_;
        NetManager *net_manager_;
};

} // namespace EyeseeLinux
