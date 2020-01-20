/* *******************************************************************************
 * Copyright (C), 2001-2018, xiaoyi Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file dd_globalInfo.cpp
 * @brief global info
 *
 * @version v0.1
 * @date 2018-09-06
 */
#include <iostream> 
#include <sstream>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <json/json.h>
#include "device_model/system/event_manager.h"
#include "device_model/menu_config_lua.h"
#include "device_model/version_update_manager.h"
#include "dd_serv/globalInfo.h"
#include "dd_serv/dd_common.h"

using namespace std;
static DD_GLOBALINFO *_ginfo = NULL;

#define DEFAULT_UPDATE_CONF "{\"old_version\":\"\",\"time_upgrade\":\"\",\"isReport\":\"\"}"

#define DEFAULT_COLLECT_INTERVAL    (0) //seconds
#define DEFAULT_REPORT_INTERVAL     (0) //seconds

DD_GLOBALINFO::DD_GLOBALINFO()
{
    gInfo.imei = "";
    gInfo.sim = "";
    gInfo.sn= "";
    gInfo.bindflag = "";
    gInfo.systemVersion = "";

    gInfo.gps.gpscfg.collect_interval = DEFAULT_COLLECT_INTERVAL;
    gInfo.gps.gpscfg.report_interval = DEFAULT_REPORT_INTERVAL;

    gInfo.upgradeconf.old_version = "";
    gInfo.upgradeconf.time_upgrade = "";
    gInfo.upgradeconf.isReport = "";

    gInfo.timestamp_poweron = 0;
    gInfo.timestamp_poweroff = 0;
}

DD_GLOBALINFO::~DD_GLOBALINFO()
{
}

DD_GLOBALINFO* DD_GLOBALINFO::GetInstance(void)
{
    if(_ginfo != NULL) {
        return _ginfo;
    }

    _ginfo = new DD_GLOBALINFO();

    return _ginfo;
}

int DD_GLOBALINFO::getDevInfoReport(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.isReport;
}

void DD_GLOBALINFO::setDevInfoReport(int report)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.isReport = report;
}

std::string DD_GLOBALINFO::getIMEI(void)
{
    Mutex::Autolock _l(m_mutex);
    if(strcmp(gInfo.imei.c_str(), "") == 0) {
        std::string t_imei;
        AdapterLayer::GetInstance()->getProductInfo("imei", t_imei);
        if(strcmp(t_imei.c_str(), "") != 0 && t_imei.length() == 15) {
            gInfo.imei = t_imei;
            return gInfo.imei;
        }
        else {
			EventManager::GetInstance()->getIMEI(gInfo.imei);
            return gInfo.imei;
        }
    }
    else {
        return gInfo.imei;
    }

    return "";
}

int DD_GLOBALINFO::setIMEI(std::string _imei)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.imei = _imei;
    return 0;
}

std::string DD_GLOBALINFO::getSIM(void)
{
    Mutex::Autolock _l(m_mutex);
    if(strcmp(gInfo.sim.c_str(), "") == 0) {
        std::string t_sim = EventManager::GetInstance()->sim;
        if(strcmp(t_sim.c_str(), "") != 0) {
            gInfo.sim = t_sim;
            return gInfo.sim;
        }
        else {
            return "";
        }
    }
    else {
        return gInfo.sim;
    }

    return "";
}

int DD_GLOBALINFO::setSIM(std::string _sim)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.sim = _sim;
    return 0;
}

std::string DD_GLOBALINFO::getSN(void)
{
    Mutex::Autolock _l(m_mutex);
    if(strcmp(gInfo.sn.c_str(), "") == 0) {
        std::string t_sn;
        AdapterLayer::GetInstance()->getProductInfo("sn", t_sn);
        if(strcmp(t_sn.c_str(), "") != 0) {
            gInfo.sn = t_sn;
            return gInfo.sn;
        }
        else {
            return "";
        }
    }
    else {
        return gInfo.sn;
    }

    return "";
}

int DD_GLOBALINFO::setSN(std::string _sn)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.sn = _sn;
    return 0;
}

std::string DD_GLOBALINFO::getUserName(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.username;
}

int DD_GLOBALINFO::setUserName(std::string username)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.username = username;
    return 0;
}

std::string DD_GLOBALINFO::getMqttStatus(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.mqttstatus;
}

int DD_GLOBALINFO::setMqttStatus(std::string status)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.mqttstatus = status;
    return 0;
}

std::string DD_GLOBALINFO::getNetStatus(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.netstatus;
}

int DD_GLOBALINFO::setNetStatus(std::string status)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.netstatus = status;
    return 0;
}

std::string DD_GLOBALINFO::getSystemVersion(void)
{
    Mutex::Autolock _l(m_mutex);
    if(strcmp(gInfo.systemVersion.c_str(), "") == 0) {
        ::LuaConfig config;
        config.LoadFromFile("/data/menu_config.lua");
        gInfo.systemVersion = config.GetStringValue("menu.device.sysversion.version");
    }
    return gInfo.systemVersion;
}

std::string DD_GLOBALINFO::getBindflag(void)
{
#ifdef DDSERVER_SUPPORT
    Mutex::Autolock _l(m_mutex);
    int ret = -1;
    std::string bindstr;

    if(strcmp(gInfo.bindflag.c_str(), "") != 0) {
        return gInfo.bindflag;
    }
    else {
        ret = AdapterLayer::GetInstance()->getBindFlagInfo(bindstr);
        if(ret < 0){
            DBG_ERROR("getBindFlagInfo failed!\n");
            return "";
        }
        else {
            gInfo.bindflag = bindstr;
        }
    }

    return gInfo.bindflag;
#else
    return 0;
#endif
}

int DD_GLOBALINFO::setBindflag(std::string flag)
{
#ifdef DDSERVER_SUPPORT
    Mutex::Autolock _l(m_mutex);
    int ret = -1;
    ret = AdapterLayer::GetInstance()->setBindFlagInfo(flag);
    if(ret < 0){
        DBG_ERROR("setBindFlagInfo failed!\n");
        return -1;
    }
    gInfo.bindflag = flag;

    return 0;
#else
    return 0;
#endif
}

InfoGPS DD_GLOBALINFO::getGPS(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.gps;
}

int DD_GLOBALINFO::setGPSCfg(GpsConfigNode cfg)
{
    Mutex::Autolock _l(m_mutex);
    if(gInfo.gps.gpscfg.collect_interval != cfg.collect_interval) {
        gInfo.gps.gpscfg.collect_interval = cfg.collect_interval;
    }
    if(gInfo.gps.gpscfg.report_interval != cfg.report_interval) {
        gInfo.gps.gpscfg.report_interval = cfg.report_interval;
    }
    return 0;
}

UpgradeConf DD_GLOBALINFO::getUpgradeConf(void)
{
    Mutex::Autolock _l(m_mutex);
    int ret = -1;
    UpgradeConf cfg;
    if(strcmp(gInfo.upgradeconf.old_version.c_str(), "") == 0 ||
            strcmp(gInfo.upgradeconf.time_upgrade.c_str(), "") == 0) {
        ret = LoadUpdateConf(cfg);
        if(strcmp(cfg.old_version.c_str(), "") == 0) {   //default system old_version is "" in /data/upgrade.conf if hasnot upgraded
            ::LuaConfig config;
            config.LoadFromFile("/data/menu_config.lua");
            cfg.old_version = config.GetStringValue("menu.device.sysversion.version");

            //sync current version to /data/upgrade.conf
            ret = SaveLoadUpdateConf(cfg);
            if(ret < 0) {
                db_error("SaveLoadUpdateConf failed!");
            }
        }
        if(ret == 0) {
            gInfo.upgradeconf = cfg;
        }
    }

    return gInfo.upgradeconf;
}

int DD_GLOBALINFO::setUpgradeConf(UpgradeConf cfg)
{
    Mutex::Autolock _l(m_mutex);
    int ret = -1;
    ret = SaveLoadUpdateConf(cfg);
    if(ret == 0) {
        gInfo.upgradeconf = cfg;
    }

    return ret;
}

int DD_GLOBALINFO::setTimestampPowerOn(int timestamp)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.timestamp_poweron = timestamp;
    return 0;
}

int DD_GLOBALINFO::getTimestampPowerOn(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.timestamp_poweron;
}

int DD_GLOBALINFO::setTimestampPowerOff(int timestamp)
{
    Mutex::Autolock _l(m_mutex);
    gInfo.timestamp_poweroff = timestamp;
    return 0;
}

int DD_GLOBALINFO::getTimestampPowerOff(void)
{
    Mutex::Autolock _l(m_mutex);
    return gInfo.timestamp_poweroff;
}

int DD_GLOBALINFO::getRuntimes(int ts_poweroff)
{
#ifdef DDSERVER_SUPPORT
    Mutex::Autolock _l(m_mutex);
    if(gInfo.timestamp_poweron > 0) {
        if(ts_poweroff > 0 && ts_poweroff > gInfo.timestamp_poweron) {
            return ts_poweroff - gInfo.timestamp_poweron;
        }
        else {
            if(ts_poweroff <= 0 && gInfo.timestamp_poweroff > gInfo.timestamp_poweron) {
                return gInfo.timestamp_poweroff - gInfo.timestamp_poweron;
            }
        }
    }
    return 0;
#else
    return 0;
#endif
}

/*********************************************************/
/*********************************************************/
int DD_GLOBALINFO::LoadUpdateConf(UpgradeConf &cfg)
{
#ifdef DDSERVER_SUPPORT
    Json::Reader reader;
    Json::Value value;
    int ret = 0;
    std::string info;

    ret = AdapterLayer::GetInstance()->LoadUpgradeInfo(info);
    if(ret < 0){
        db_error("LoadUpgradeInfo failed!\n");
        return -1;
    }
    if(strcmp(info.c_str(), "") == 0) {
        //nothing had wrote, write defulat value
        ret = AdapterLayer::GetInstance()->SaveUpgradeInfo(DEFAULT_UPDATE_CONF);
        if(ret < 0){
            db_error("SaveUpgradeInfo failed!\n");
            return -1;
        }
        info = DEFAULT_UPDATE_CONF;
    }

    if (reader.parse(info, value)) {
        cfg.old_version = value["old_version"].asString();
        cfg.time_upgrade = value["time_upgrade"].asString();
        cfg.isReport = value["isReport"].asString();
        db_error("old_version:%s time_upgrade:%s isReport:%s", cfg.old_version.c_str(), cfg.time_upgrade.c_str(), cfg.isReport.c_str());
    }
    else {
        db_error("parse update config failed!");
        return -1;
    }

    return 0;
#else
    return 0;
#endif
}

int DD_GLOBALINFO::SaveLoadUpdateConf(UpgradeConf cfg)
{
#ifdef DDSERVER_SUPPORT
    int ret = 0;
    Json::Value root;
    Json::FastWriter writer;
    std::string jsoncfg;

    root["old_version"] = cfg.old_version;
    root["time_upgrade"] = cfg.time_upgrade;
    root["isReport"] = cfg.isReport;
    jsoncfg = writer.write(root);

    ret = AdapterLayer::GetInstance()->SaveUpgradeInfo(jsoncfg);
    if(ret < 0){
        db_error("SaveUpgradeInfo failed!\n");
        return -1;
    }

    return 0;
#else
    return 0;
#endif
}


