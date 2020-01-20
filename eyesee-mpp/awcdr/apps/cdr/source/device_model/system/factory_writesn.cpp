#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <termios.h>
#include <errno.h>
#include <json/json.h>

#include "dd_serv/dd_debug.h"

#include "dd_serv/dd_base64.h"
#include "../source/device_model/system/event_manager.h"
#include "../source/device_model/system/factory_writesn.h"
#include "../source/bll_presenter/AdapterLayer.h"

using namespace std;

extern std::string imeiname;
const char *netdevname = "wlan0";
#define SERIAL_PROT     "/dev/ttyS0"

#define UP_SERIAL_CONNECTION    0
#define READ_SN_FROM_FILE       0
#define SN_FILE         "/mnt/extsd/sn.txt"

serial_option default_s_op = 
{
    .speed = 115200,
    .flow_ctl = 0,
    .databits = 8,
    .stopbits = 1,
    .parity = 'N',
};

int tty_open(const char *port)
{
#ifdef AES_SUPPORT
    int ret = -1;
    int fd = -1;

    fd = open(port, O_RDWR| O_NOCTTY| O_NDELAY);
    if(fd < 0)
    {
        DBG_ERROR("can not open serial %s ,err:%s", port, strerror(errno));
        return -1;
    }

    return fd;
#else
    return 0;
#endif
}

int tty_setoption(int fd, serial_option *s_op)
{
#ifdef AES_SUPPORT
    int ret = -1;
    struct termios options, oldtio;

    ret = tcgetattr(fd, &oldtio);
    if(ret != 0)
    {
        DBG_ERROR("tcgetattr error\n");
        return -1;
    }

    bzero(&options, sizeof(options));

    options.c_cflag |= CLOCAL | CREAD;
    options.c_cflag &= ~CSIZE;
    switch(s_op->speed)
    {
        case 115200:
            cfsetispeed(&options, B115200);
            cfsetospeed(&options, B115200);
            break;
        case 9600:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
        default:
            DBG_ERROR("unsupport speed %d", s_op->speed);
            return -1; 
    }

    switch(s_op->flow_ctl)
    {
        case 0:
            options.c_cflag &= ~CRTSCTS;
            break;
        case 1:
            options.c_cflag |= CRTSCTS;
            break;
        case 2:
            options.c_cflag |= IXON | IXOFF | IXANY;
            break;
    }

    switch (s_op->databits)
    {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS6;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            DBG_ERROR("uspported databits:%d", s_op->databits);
            return -1;
    }

    switch(s_op->parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;
            break;
        case 'o':
        case 'O':
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            options.c_iflag |= ( INPCK | ISTRIP);;
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= ( INPCK | ISTRIP);
        default:
            DBG_ERROR("unsuported parity:%c", s_op->parity);
            return -1;
    }

    switch(s_op->stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB; 
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            DBG_ERROR("uspported stopbits:%d", s_op->stopbits);
            return -1;
    }

    //options.c_oflag &= ~OPOST;
    //options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 0;

    tcflush(fd, TCIFLUSH);

    ret = tcsetattr(fd, TCSANOW, &options);
    if (ret != 0)
    {
        DBG_ERROR("tcsetattr failed\n");
        return -1;
    }

    return 0;
#else
    return 0;
#endif
}

int tty_write(int fd, const char *data, int len)
{
#ifdef AES_SUPPORT
    int ret = -1;
    fd_set wfset;
    int sum = 0;
    struct timeval tv = {0, 0};

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&wfset);
    FD_SET(fd, &wfset);

    DBG_DEBUG("%s, send data:%s", __FUNCTION__, data);
    ret = select(fd + 1, NULL, &wfset, NULL, &tv);
    if(ret < 0)
    {
        return -2;
    }else if(ret == 0)
    {
        return -1;
    }else
    {
        while(sum < len)
        {
            ret = write(fd, data + sum, len - sum);
            if(ret < 0)
            {
                DBG_ERROR("write errno:%d, err:%s", errno, strerror(errno));
                return ret;
            }
            sum += ret;
        }
    }

    return ret;
#else
    return 0;
#endif
}

int tty_read(int fd, char *buff, int len)
{
#ifdef AES_SUPPORT
    int ret = -1;
    fd_set rfset;
    struct timeval tv = {0, 0};

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&rfset);
    FD_SET(fd, &rfset);

    ret = select(fd + 1, &rfset, NULL, NULL, &tv);
    if(ret < 0)
    {
        DBG_ERROR("select errno:%d, err:%s", errno, strerror(errno));
        return -2;
    }else if(ret == 0)
    {
        return -1;
    }else
    {
        ret = read(fd, buff, len);
        if(ret < 0)
        {
            DBG_ERROR("read errno:%d, err:%s", errno, strerror(errno));
            return ret;
        }
    }

    return ret;
#else
    return 0;
#endif
}

int Gen_dev_info_json(const Device_info *dev_info, std::string &str)
{
#ifdef AES_SUPPORT
    Json::Value root;
    Json::FastWriter writer;

    if(dev_info != NULL)
    {
        root["wifimac"] = dev_info->wifimac;
        root["imei"] = dev_info->imei;
        root["simID"] = dev_info->simID;
        root["fwversion"] = dev_info->fwversion;
    }else
    {
        DBG_ERROR("dev_info is NULL");
        return -1;
    }

    str = writer.write(root);

    return 0;
#else
    return 0;
#endif
}

int Gen_dev_value_json(const WR_SN_CMD *dev_id, std::string &out)
{
#ifdef AES_SUPPORT
    Json::Value root;
    Json::FastWriter writer;

    if(dev_id != NULL)
    {
        root["sn"] = dev_id->sn;
        root["lotid"] = dev_id->lotid;
    //    root["mqtt_pw"] = dev_id->passwd;
        root["key"] = dev_id->Key;
        root["imei"] = dev_id->imei;
		root["P2PID"] = dev_id->P2PID;
    }else
    {
        DBG_ERROR("dev_id is NULL");
        return -1;
    }

    out = writer.write(root);

    return 0;
#else
    return 0;
#endif
}

static int parse_command_type(const string strjson)
{
#ifdef AES_SUPPORT
    int push_type;
    Json::Reader reader;  
    Json::Value value;

    if (reader.parse(strjson, value))
    {
        push_type = value["cmd_type"].asInt();
    }else
    {
        DBG_DEBUG("get cmd_type error:%s", strjson.c_str());
        return -1;
    }

    return push_type;
#else
    return 0;
#endif
}

int parse_serial_value(const string data, WR_SN_CMD &wr_data)
{
#ifdef AES_SUPPORT
    int ret = -1;
    Json::Reader reader;  
    Json::Value value;

    if(reader.parse(data, value))
    {
        wr_data.sn = value["sn"].asString();
        wr_data.lotid = value["lotid"].asString();
      //  wr_data.passwd = value["mqtt_pw"].asString();
        wr_data.Key = value["key"].asString();
		wr_data.P2PID = value["P2PID"].asString();
    }else
    {
        DBG_ERROR("json:%s have some problem", data.c_str());
        return -1;
    }

  //  DBG_DEBUG("sn:%s, usr_name:%s, passwd:%s, Key:%s", 
//        wr_data.sn.c_str(), wr_data.usr_name.c_str(), wr_data.passwd.c_str(), wr_data.Key.c_str());
    return 0;
#else
    return 0;
#endif
}

static int get_wifi_mac(std::string &out)
{
#ifdef AES_SUPPORT
    int sfd = -1;
    int ret = -1;
    char mac[20] = {0};
    struct ifreq req;

    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sfd < 0)
    {
        DBG_ERROR("open socket failed\n");
        return -1;
    }
    
    bzero(&req, sizeof(req));
    strcpy(req.ifr_name, netdevname);
    
    ret = ioctl(sfd, SIOCGIFHWADDR, &req);
    if(ret < 0)
    {
        DBG_ERROR("ioctl SIOCGIFHWADDR failed\n");
        close(sfd);
        return -1;
    }
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", 
        (unsigned char)req.ifr_hwaddr.sa_data[0],
        (unsigned char)req.ifr_hwaddr.sa_data[1],
        (unsigned char)req.ifr_hwaddr.sa_data[2],
        (unsigned char)req.ifr_hwaddr.sa_data[3],
        (unsigned char)req.ifr_hwaddr.sa_data[4],
        (unsigned char)req.ifr_hwaddr.sa_data[5]);

    out = mac;

    return 0;
#else
    return 0;
#endif
}

int Get_dev_info(Device_info &dev_info)
{
#ifdef AES_SUPPORT
    int ret;

    ret = get_wifi_mac(dev_info.wifimac);
    if(ret < 0)
    {
        DBG_ERROR("get wifimac failed ret = %d\n ", ret);
        return ret;
    }
    AdapterLayer::GetInstance()->getDeviceInfo(dev_info);

    return 0;
#else
    return 0;
#endif
}

static int set_sn_key_to_mmc(std::string item, std::string value)
{
#ifdef AES_SUPPORT
    int ret = -1;
    ret = AdapterLayer::GetInstance()->setProductInfo(item, value,0);
	DBG_ERROR("[%s]:%d, setProductInfo item %s , value = %s", __FUNCTION__, __LINE__, item.c_str(),value.c_str());
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, setProductInfo failed, ret = %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}
static int save_sn_key_to_mmc()
{
#ifdef AES_SUPPORT
    int ret = -1;
    ret = AdapterLayer::GetInstance()->saveProductInfo();
    if(ret < 0)
    {
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}

static int clean_the_all_partion()
{
#ifdef AES_SUPPORT
    int ret = -1;
    ret = AdapterLayer::GetInstance()->clearProductInfo();
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, setProductInfo failed, ret = %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}

static int get_sn_key_to_mmc(std::string item, std::string &val)
{
#ifdef AES_SUPPORT
    int ret = -1;

    ret = AdapterLayer::GetInstance()->getProductInfofromflash(item, val);
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, getProductInfo failed ret =%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}

static int set_mqtt_to_mmc(const std::string usr, const std::string passwd)
{
#ifdef AES_SUPPORT
    int ret = -1;
    std::string userencode;
    std::string passwordencode;

    userencode = base64_encode((const unsigned char*)usr.data(), usr.length());
    passwordencode = base64_encode((const unsigned char*)passwd.data(), passwd.length());

    ret = AdapterLayer::GetInstance()->setUserInfo(userencode, passwordencode);
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, setUserInfo failed ret :%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}

static int get_mqtt_from_mmc(std::string &usr, std::string &passwd)
{
#ifdef AES_SUPPORT
    int ret = -1;
    ret = AdapterLayer::GetInstance()->getUserInfo(usr, passwd);
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, getUserInfo failed ret :%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    return 0;
#else
    return 0;
#endif
}

int set_value_to_mmc(WR_SN_CMD &dev_id)
{
#ifdef AES_SUPPORT
    int ret = -1;
    std::string item("sn");
    ret = set_sn_key_to_mmc(item, dev_id.sn);
    DBG_DEBUG("[%s]:%d, end of write sn to mmc", __FUNCTION__, __LINE__);

    item.clear();
    item = "key";
    ret = set_sn_key_to_mmc(item, dev_id.Key);

	item.clear();
    item = "p2pid";
    ret = set_sn_key_to_mmc(item, dev_id.P2PID);

    item.clear();
    item = "imei";
    DBG_DEBUG("[%s]:%d, 22imei:%s\n", __FUNCTION__, __LINE__, dev_id.imei.c_str());
    ret = set_sn_key_to_mmc(item, dev_id.imei);
	item.clear();
	item = "lotid";

	 ret = set_sn_key_to_mmc(item, dev_id.lotid);

	 save_sn_key_to_mmc();	
    DBG_DEBUG("[%s]:%d, end of write Key to mmc", __FUNCTION__, __LINE__);
    //ret = set_mqtt_to_mmc("", "");
    DBG_DEBUG("[%s]:%d, end of write usr and passwd to mmc", __FUNCTION__, __LINE__);

    return 0;
#else
    return 0;
#endif
}

int get_value_from_mmc(WR_SN_CMD *dev_id)
{
#ifdef AES_SUPPORT
    int ret = -1;
    std::string usr;
    std::string passwd;
    std::string item;

 //   ret = get_mqtt_from_mmc(usr, passwd);

//    dev_id->usr_name = base64_decode(usr);
 //   dev_id->passwd = base64_decode(passwd);
    item = "lotid";
    ret = get_sn_key_to_mmc(item, dev_id->lotid);
	
    DBG_DEBUG("[%s]:%d,get item %s value %s", __FUNCTION__, __LINE__, item.c_str(), dev_id->lotid.c_str());
    item = "sn";
    ret = get_sn_key_to_mmc(item, dev_id->sn);
    DBG_DEBUG("[%s]:%d,get item %s value %s", __FUNCTION__, __LINE__, item.c_str(), dev_id->sn.c_str());

    item = "key";
    ret = get_sn_key_to_mmc(item, dev_id->Key);
    DBG_DEBUG("[%s]:%d,get item %s value %s", __FUNCTION__, __LINE__, item.c_str(), dev_id->Key.c_str());

    item = "imei";
    ret = get_sn_key_to_mmc(item, dev_id->imei);
	
    DBG_DEBUG("[%s]:%d,get item %s value %s", __FUNCTION__, __LINE__, item.c_str(), dev_id->imei.c_str());
	item = "p2pid";
    ret = get_sn_key_to_mmc(item, dev_id->P2PID);
    DBG_DEBUG("[%s]:%d,get item %s value %s", __FUNCTION__, __LINE__, item.c_str(), dev_id->P2PID.c_str());

    return 0;
#else
    return 0;
#endif
}


static int get_factory_from_flash(std::string item, std::string &val)
{
#ifdef AES_SUPPORT
    int ret = -1;

    ret = AdapterLayer::GetInstance()->getFactoryInfo(item, val);
    if(ret < 0)
    {
        DBG_ERROR("[%s]:%d, getFactoryInfo failed ret =%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
#else
    return 0;
#endif
}

int get_factoryvalue_from_flash(RD_TEST_CMD *test_id)
{
#ifdef AES_SUPPORT
    int ret = -1;
    std::string item;



    item = "Gsensor test";
    ret = get_factory_from_flash(item, test_id->gsensor);
	
    item = "4G test";
    ret = get_factory_from_flash(item, test_id->lte);

    item = "WIFI test";
    ret = get_factory_from_flash(item, test_id->wifi);
	
	item = "GPS test";
    ret = get_factory_from_flash(item, test_id->gps);

    return 0;
#else
    return 0;
#endif
}

int Gen_test_value_json(const RD_TEST_CMD *test_id, std::string &out)
{
#ifdef AES_SUPPORT
    Json::Value root;
    Json::FastWriter writer;

    if(test_id != NULL)
    {
        root["Gsensor_test"] = test_id->gsensor;
        root["lte_test"] = test_id->lte;
        root["Wifi_test"] = test_id->wifi;
        root["Gps_test"] = test_id->gps;
       
    }else
    {
        DBG_ERROR("dev_id is NULL");
        return -1;
    }

    out = writer.write(root);

    return 0;
#else
    return 0;
#endif
}

int parse_and_set_cmd(int fd, const string cmdstr, int cmd)
{
#ifdef AES_SUPPORT
    int ret = -1;
    Device_info dev_info;
    WR_SN_CMD dev_id;
    WR_SN_CMD dev_id_r;
	
	RD_TEST_CMD test_id;
    std::string strjson;

    switch(cmd)
    {
        case GET_INFO:
            Get_dev_info(dev_info);
            imeiname = dev_info.imei;
            Gen_dev_info_json(&dev_info, strjson);
            strjson += "\n";
            DBG_DEBUG("send msg:%s\n", strjson.c_str());
            tty_write(fd, strjson.c_str(), strjson.length());
            break;
        case WR_SN:
            clean_the_all_partion();
            DBG_DEBUG("WR_SN:%s\n",cmdstr.c_str());
            //bzero(&dev_id, sizeof(dev_id));
            ret = parse_serial_value(cmdstr, dev_id);
            if(ret < 0)
            {
                return -1;
            }
            Get_dev_info(dev_info);
            dev_id.imei = dev_info.imei;
            DBG_DEBUG("[%s]:%d, imei:%s\n", __func__, __LINE__, dev_id.imei.c_str());
            set_value_to_mmc(dev_id);

            DBG_DEBUG("[%s]:%d, 11end of set_value_to_mmc", __FUNCTION__, __LINE__);
            get_value_from_mmc(&dev_id_r);

            DBG_DEBUG("[%s]:%d, 22end of get_value_from_mmc", __FUNCTION__, __LINE__);
            strjson.clear();
            ret = Gen_dev_value_json(&dev_id_r, strjson);
            strjson += "\n";
            DBG_DEBUG("send msg:%s\n", strjson.c_str());
            ret = tty_write(fd, strjson.c_str(), strjson.length());
            if(ret < 0)
            {
                DBG_ERROR("send msg %s failed\n", strjson.c_str());
            }
            break;
        case RD_SN:
            //bzero(&dev_id, sizeof(dev_id));
            get_value_from_mmc(&dev_id);
            ret = Gen_dev_value_json(&dev_id, strjson);
            strjson += "\n";
            DBG_DEBUG("[%s]:%d,send msg:%s\n", __FUNCTION__, __LINE__, strjson.c_str());
            ret = tty_write(fd, strjson.c_str(), strjson.length());
            if(ret < 0)
            {
                DBG_ERROR("send msg %s failed\n", strjson.c_str());
            }
            break;
		case RD_TEST:
			 get_factoryvalue_from_flash(&test_id);
            ret = Gen_test_value_json(&test_id, strjson);
            strjson += "\n";
            DBG_DEBUG("[%s]:%d,send msg:%s\n", __FUNCTION__, __LINE__, strjson.c_str());
            ret = tty_write(fd, strjson.c_str(), strjson.length());
            if(ret < 0)
            {
                DBG_ERROR("send msg %s failed\n", strjson.c_str());
            }
			break;
        default:
            DBG_ERROR("usupport cmd %d", cmd);
            return -1;
    }

    return 0;
#else
    return 0;
#endif;
}

void *serial_command_thread(void *nouse)
{
#ifdef AES_SUPPORT
    int ret = -1;
    int fd = -1;
    std::string cmdstr;
    char data[512] = {0};

    fd = tty_open(SERIAL_PROT);
    if(fd < 0)
    {
        DBG_ERROR("open %s failed\n", SERIAL_PROT);
        return NULL;
    }

    ret = tty_setoption(fd, &default_s_op);
    if(ret < 0)
    {
        DBG_ERROR("tty_setoption failed\n");
        close(fd);
        return NULL;
    }

    DBG_INFO("----------%s-----------", __FUNCTION__);
    while(1)
    {
        int command_type;
        memset(data, 0, sizeof(data));
        ret = tty_read(fd, data, sizeof(data) - 1);
        if(ret < 0)
        {
            usleep(100 * 1000); 
            continue;
        }else
        {
            DBG_DEBUG("read data:%s\n", data);
            std::string datavalue(data);
            cmdstr += datavalue;
        }

        std::size_t found = cmdstr.find_last_of('\n');
        if(found != std::string::npos)
        {
            //cmdstr = cmdstr.replace(found, 1, "\0");
            command_type = parse_command_type(cmdstr);
            ret = parse_and_set_cmd(fd, cmdstr, command_type);
            cmdstr.clear();
        }

        usleep(100*1000);
    }

    close(fd);
    return (void*)0;
#else
    return (void*)0;
#endif
}

static int read_the_key_file(const char *filename, std::string &str)
{
#ifdef AES_SUPPORT
    int ret = -1;
    FILE *fp = NULL;
    char buff[256] = {0};

    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        DBG_ERROR("open file %s failed\n", filename);
        return -1;
    }

    bzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff) - 1, fp);

    DBG_DEBUG("read data:[%s]\n", buff);
    str = buff;
    fclose(fp);
    return 0;
#else
    return 0;
#endif
}

void *file_command_thread(void *nouse)
{
#ifdef AES_SUPPORT
    int ret = -1;
    Device_info dev_info;
    WR_SN_CMD dev_id;
    std::string strbuff;

    while(1)
    {
        strbuff.clear();
        ret = read_the_key_file(SN_FILE, strbuff);
        if(ret != 0)
        {
            sleep(5);
            continue;
        }else
        {
            clean_the_all_partion();
            DBG_DEBUG("wrdata:%s\n",strbuff.c_str());
            ret = parse_serial_value(strbuff, dev_id);
            if(ret < 0)
            {
                sleep(5);
                continue;
            }
            Get_dev_info(dev_info);
            dev_id.imei = dev_info.imei;
            set_value_to_mmc(dev_id);
            get_value_from_mmc(&dev_id);
            strbuff.clear();
            ret = Gen_dev_value_json(&dev_id, strbuff);

            DBG_DEBUG("finsh write data to mmc!!");
            remove(SN_FILE);
            sleep(5);
        }
    }

    return (void*)0;
#else
    return (void*)0;
#endif
}

int creat_serial_command_thread()
{
#ifdef AES_SUPPORT
    int ret = -1;
    pthread_t pt;
    pthread_t pt2;

#if UP_SERIAL_CONNECTION
    ret = pthread_create(&pt, NULL, serial_command_thread, NULL);
    if(ret < 0)
    {
        DBG_ERROR("[%s] creat thread failed! ret:%d", __FUNCTION__, ret);
        return -1;
    }

    pthread_detach(pt);
#endif

#if READ_SN_FROM_FILE
    ret = pthread_create(&pt2, NULL, file_command_thread, NULL);
    if(ret < 0)
    {
        DBG_ERROR("[%s] creat thread failed! ret:%d", __FUNCTION__, ret);
        return -1;
    }

    pthread_detach(pt2);
#endif
    return 0;
#else
    return 0;
#endif
}


