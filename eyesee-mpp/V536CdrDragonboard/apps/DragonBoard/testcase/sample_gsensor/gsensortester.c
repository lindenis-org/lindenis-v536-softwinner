#define TAG "keytester"

#define FIFO_DEV  "/tmp/fifo_key"
#include <dragonboard/dragonboard.h>
#include "gsensortester.h"

int timeout_; // min:1s, max:16s
int data_fd;
int mWritePipeFd;
struct pollfd mPollFds[11];
pthread_t gsensor_tid_;
pthread_t gsensor_impact_tid_;
pthread_t gsensor_axis_data_tid_;
struct sensor_info gsensorInfo;
struct g_status seStatus[SUPPORT_SENSORS_NUMBER];
struct sensor_t sSensorList[SUPPORT_SENSORS_NUMBER];
int flag0;
int flag1;
int found;
int sNumber;
struct input_event *mBuffer;
struct input_event *mBufferEnd;
struct input_event *mHead;
struct input_event *mCurr;
ssize_t mFreeSpace;

int mEnabled;
int mPendingMask;
float convert;
float direct_x;
float direct_y;
float direct_z;
int direct_xy;
struct sensors_event_t mPendingEvent;
struct sensors_event_t mAccData;
struct sensors_event_t sensor_data;
int sensor_count;
bool m_bImpactStatus;
int m_ParkingModeIntImpact;
int m_gsensorIntEnable;
int mGsensorOccurAxis;
float mGsensorImpactTarget;
int mGsensorParkingImpactSlope;

void setSensorData(int number,struct sensor_t *sensorList)
{
    sSensorList[number].name = sensorList->name;
    sSensorList[number].vendor = sensorList->vendor;
    sSensorList[number].version = sensorList->version;
    sSensorList[number].handle = sensorList->handle;
    sSensorList[number].type = sensorList->type;
    sSensorList[number].maxRange = sensorList->maxRange;
    sSensorList[number].resolution = sensorList->resolution;
    sSensorList[number].power = sensorList->power;
    sSensorList[number].minDelay = sensorList->minDelay;
    memcpy(sSensorList[number].reserved,sensorList->reserved,
            sizeof(sSensorList[number].reserved));
}


int getDevice(struct sensor_extend_t list[], struct sensor_info *info,
        char buf[], char classPath[], int number)
{
    int ret = 0;

    if ((!strlen(buf)) || (list == NULL)) {
        return 0;
    }

    while(ret < number){
        if (!strncmp(buf, list[ret].sensors.name, strlen(buf))) {

            info->priData = list[ret].sensors.lsg;
            setSensorData(sNumber, &list[ret].sList);

            strncpy(info->sensorName, buf,strlen(buf));
            strncpy(info->classPath, classPath, strlen(classPath));
#ifdef DEBUG_SENSOR
            db_msg("sensorName:%s,classPath:%s,lsg:%f\n",
                    info->sensorName,info->classPath, info->priData);
#endif
            return 1 ;
        }
        ret++;
    }

    return 0;

}

int otherDeviceDetect(char buf[])
{
    int number0 = 0, number1 = 0;
    int re_value = 0;
    number0 = ARRAY_SIZE(otherDevice);
    number1 = ARRAY_SIZE(ctpDevice);
#ifdef DEBUG_SENSOR
    db_msg("buf:%s, flag0:%d, flag1:%d\n", buf, flag0, flag1);
    db_msg("otherDevice[0].isFind:%d,otherDevice[1].isFind:%d\n",
            otherDevice[0].isFind,
            otherDevice[1].isFind);
#endif

    if(!flag0) {
        while(number0--) {
            if(!otherDevice[number0].isFind){
                if (!strncmp(buf, otherDevice[number0].name,
                            strlen(otherDevice[number0].name))) {

                    otherDevice[number0].isFind = 1;
                    re_value = 1;
                }
            }
        }
    }
    if(!flag1) {
        while(number1--) {
            if(!ctpDevice[number1].isFind){
                if (!strncmp(buf, ctpDevice[number1].name,
                            strlen(ctpDevice[number1].name))) {

                    ctpDevice[number1].isFind = 1;
                    flag1 = 1;
                    re_value = 1;

                }
            }
        }
    }

    if((otherDevice[0].isFind == 1) && (otherDevice[1].isFind == 1))
        flag0 = 1;

    if((flag0 == 1) && (flag1 == 1)) {
        return 2;
    }

    if(re_value == 1)
        return 1;

    return 0;
}


int searchDevice(char buf[], char classPath[])
{
    int ret;
    if(!found) {
        found = otherDeviceDetect(buf);

#ifdef DEBUG_SENSOR
        db_msg("found:%d\n", found);
#endif

        if(found == 1) {
            found = 0;
            return 1;
        }else if(found == 2) {
            return 1;
        }
    }

    if((seStatus[ID_A].isUsed == true) && (seStatus[ID_A].isFound == false)) {
        ret = getDevice(gsensorList, &gsensorInfo, buf, classPath, ARRAY_SIZE(gsensorList));
        db_msg("ret:%d\n", ret);
        if(ret == 1){
            seStatus[ID_A].isFound = true;
            sNumber++;
            return 0;
        }
    }
    return 1;
}

void statusInit(void)
{
    seStatus[ID_A].isUsed  = true;
    seStatus[ID_A].isFound = false;

    seStatus[ID_M].isUsed  = true;
    seStatus[ID_M].isFound = false;

    seStatus[ID_GY].isUsed  = true;
    seStatus[ID_GY].isFound = false;

    seStatus[ID_L].isUsed  = true;
    seStatus[ID_L].isFound = false;

    seStatus[ID_PX].isUsed  = true;
    seStatus[ID_PX].isFound = false;

    seStatus[ID_O].isUsed  = false;
    seStatus[ID_O].isFound = false;

    seStatus[ID_T].isUsed  = true;
    seStatus[ID_T].isFound = false;

    seStatus[ID_P].isUsed  = false;
    seStatus[ID_P].isFound = false;

}


int sensorsDetect(void)
{
    char *inputsysfs =(char *) "/sys/class/input";
    char *dirname = (char *)"/dev/input";
    char buf[256];
    char classPath[256];
    int res;
    DIR *sysfs_dir;
    DIR *input_dir;
    struct dirent *de;
    struct dirent *input_de;
    int fd = -1;
    int ret = 0;
    char devname[256];
    char *filename;

    strncpy(devname, dirname, sizeof(devname));
    filename = devname + strlen(devname);
    *filename++ = '/';

    memset(&buf,0,sizeof(buf));
    statusInit();

    sysfs_dir = opendir(inputsysfs);
    if (sysfs_dir == NULL)
        return -1;

    input_dir = opendir(dirname);
    if (input_dir == NULL)
    {
        closedir(sysfs_dir);
        return -1;
    }

    while((de = readdir(sysfs_dir))) {
        if (strncmp(de->d_name, "input", strlen("input")) != 0) {
            continue;
        }

        snprintf(classPath, sizeof(classPath),"%s/%s", inputsysfs, de->d_name);
        snprintf(buf, sizeof(buf), "%s/name", classPath);

        fd = open(buf, O_RDONLY);
        if (fd < 0) {
            continue;
        }

        if ((res = read(fd, buf, sizeof(buf))) < 0) {
            close(fd);
            continue;
        }
        buf[res - 1] = '\0';

#ifdef DEBUG_SENSOR
        db_msg("buf:%s\n", buf);
#endif

        ret = searchDevice(buf, classPath);
        if(ret == 0){
            while((input_de = readdir(input_dir))) {
                if(input_de->d_name[0] == '.' &&
                  (input_de->d_name[1] == '\0' ||
                  (input_de->d_name[1] == '.' && input_de->d_name[2] == '\0')))
                  continue;
                strcpy(filename, input_de->d_name);
                data_fd = open(devname, O_RDONLY);
                if (data_fd >= 0) {
                    char name[80];
                    if (ioctl(data_fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                        name[0] = '\0';
                    }
                    db_msg("name is %s \n", name);
                    db_msg("devname is %s \n", devname);
                    strcpy(gsensorInfo.devname, devname);
                    if (!strcmp(name, buf)) {
                        convert = (GRAVITY_EARTH/gsensorInfo.priData);
#ifdef DEBUG_SENSOR
                        db_msg("lsg: %f,convert:%f", gsensorInfo.priData, convert);
#endif
                close(data_fd);
                data_fd = -1;
                        break;
                    } else {
                        close(data_fd);
                        data_fd = -1;
                    }
                }
            }
            close(fd);
            fd = -1;
            break;
        }
        close(fd);
        fd = -1;
    }

    closedir(sysfs_dir);
    closedir(input_dir);
    return 0;
}

ssize_t IPReader_fill(int fd)
{
    size_t numEventsRead = 0;
    if (mFreeSpace) {
         ssize_t nread = read(fd, mHead, mFreeSpace * sizeof(struct input_event));
        if (nread<0 || nread % sizeof(struct input_event)) {
            // we got a partial event!!
            return nread<0 ? -errno : -EINVAL;
        }
        numEventsRead = nread / sizeof(struct input_event);
        if (numEventsRead) {
            mHead += numEventsRead;
            mFreeSpace -= numEventsRead;

            if (mHead > mBufferEnd) {
                size_t s = mHead - mBufferEnd;
                memcpy(mBuffer, mBufferEnd, s * sizeof(struct input_event));
                mHead = mBuffer + s;
            }
        }
    }
    return numEventsRead;
}
#if 0
int64_t timevalToNano(timeval & t)
{
        return t.tv_sec*1000000000LL + t.tv_usec*1000;
}
#endif
ssize_t IPReader_readEvent(struct input_event **events)
{
    *events = mCurr;
    ssize_t available = (mBufferEnd - mBuffer) - mFreeSpace;
    return available ? 1 : 0;
}

void IPReader_next()
{
    mCurr++;
    mFreeSpace++;

    if (mCurr >= mBufferEnd) {
        mCurr = mBuffer;
    }
}

int readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;
    //data_fd = open("/dev/input/event2", O_RDONLY);
    ssize_t n = IPReader_fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    struct input_event *event;

    while (count && IPReader_readEvent(&event)) {
        int type = event->type;

        if ((type == EV_ABS) || (type == EV_REL) || (type == EV_KEY)) {
            processEvent(event->code, event->value);
            IPReader_next();
        } else if (type == EV_SYN) {
//            int64_t time = timevalToNano(event->time);

            if (mPendingMask) {
                mPendingMask = 0;
                mPendingEvent.timestamp = time;
#ifdef DEBUG_SENSOR
    db_msg("readEvents:  x,y,z:  %f, %f, %f, time : %d\n", mPendingEvent.acceleration.x,
                            mPendingEvent.acceleration.y,
                            mPendingEvent.acceleration.z, time);
#endif
                if (mEnabled) {
                    *data++ = mPendingEvent;
                    mAccData = mPendingEvent;
                    count--;
                    numEventReceived++;
                }
            }

            if (!mPendingMask) {
                IPReader_next();
            }
        } else {
            IPReader_next();
        }
    }
    return numEventReceived;
}

void processEvent(int code, int value)
{
    switch (code) {
        case ABS_X :
            mPendingMask = 1;
            if(direct_xy) {
                mPendingEvent.acceleration.y= value * direct_y * convert;
            }else {
                mPendingEvent.acceleration.x = value * direct_x * convert;
            }
            break;
        case ABS_Y :
            mPendingMask = 1;
            if(direct_xy) {
                mPendingEvent.acceleration.x = value * direct_x * convert;
            }else {
                mPendingEvent.acceleration.y = value * direct_y * convert;
            }
            break;
        case ABS_Z :
            mPendingMask = 1;
            mPendingEvent.acceleration.z = value * direct_z * convert;
            break;
    }

#ifdef DEBUG_SENSOR
    db_msg("Sensor data:  x,y,z:  %f, %f, %f\n", mPendingEvent.acceleration.x,
                            mPendingEvent.acceleration.y,
                            mPendingEvent.acceleration.z);
#endif

}

int writeGsensorIntEnable(int isEnable)
{
    db_warn("writeGsensorIntEnable %d",isEnable);
    m_gsensorIntEnable = isEnable;

    if(mGsensorParkingImpactSlope==G_SENSOR_SENSI_PARKING_OFF_VAL)
    {
        db_error("by hero  GsensorIntEnable is close, do not set intEnable \n");
        isEnable=0;
    }

    char buf[2];
    int err = -1 ;

    int bytes = snprintf(buf, sizeof(buf),"%d", isEnable);
    err = set_sysfs_input_attr(gsensorInfo.classPath,"int2_enable",buf,bytes);

    db_error("by hero writeGsensorIntEnable isEnable=%d \n", isEnable);

    return err;
}

//停车监控的唤醒灵敏度
int writeParkingSensibility(int value)
{
    char buf[4];

    if(gsensorInfo.classPath[0] == ICHAR)
        db_msg("by hero writeSensibility classpath err\n");

    switch(value)
    {
        case 3:
            mGsensorParkingImpactSlope=G_SENSOR_SENSI_PARKING_OFF_VAL;
            break;
        case 2:
            mGsensorParkingImpactSlope=G_SENSOR_SENSI_PARKING_L_VAL/7.81;
            break;
        case 1:
            mGsensorParkingImpactSlope=G_SENSOR_SENSI_PARKING_M_VAL/7.81;
            break;
        case 0:
            mGsensorParkingImpactSlope=G_SENSOR_SENSI_PARKING_H_VAL/7.81;
            break;
        default:
            db_msg("Invalid gsensor writeParkingSensibility:%d", value);
            return -1;
    }

    if(mGsensorParkingImpactSlope!=G_SENSOR_SENSI_PARKING_OFF_VAL)
    {
        db_warn("writeParkingSensibility set mGsensorParkingImpactSlope\n");
        int bytes = snprintf(buf, sizeof(buf),"%x", mGsensorParkingImpactSlope);
        set_sysfs_input_attr(gsensorInfo.classPath,"slope_th",buf, bytes);
        writeGsensorIntEnable(1);
    }else{
        db_warn("writeGsensorIntEnable false");
        writeGsensorIntEnable(0);
    }


    db_error("by hero writeParkingSensibility value=%d mGsensorParkingImpactSlope=%d \n", value, mGsensorParkingImpactSlope);

    return 0;
}

int readParkingSensibility(void)
{
    int value = 0;

    if(gsensorInfo.classPath[0] == ICHAR)
        db_msg("by hero writeSensibility classpath err\n");

    get_sysfs_input_attr(gsensorInfo.classPath,"slope_th", &value);

    db_msg("by hero readParkingSensibility value=%d \n", value);

    return value;
}

int writeDelay(int64_t ns)
{
    if(gsensorInfo.classPath[0] == ICHAR)
        db_msg("by hero writeDelay classpath err\n");
        //return -1;

    if (ns > 10240000000LL) {
        ns = 10240000000LL; /* maximum delay in nano second. */
    }
    if (ns < 312500LL) {
        ns = 312500LL; /* minimum delay in nano second. */
    }

    char buf[80];
    int bytes = snprintf(buf, sizeof(buf),"%lld", ns/1000 / 1000);

    if(!strcmp(gsensorInfo.sensorName, "lsm303d_acc")) {
        int err = set_sysfs_input_attr(gsensorInfo.classPath,"pollrate_us",buf,bytes);
    } else {
        int err = set_sysfs_input_attr(gsensorInfo.classPath,"delay",buf,bytes);
    }

    return 0;
}

int set_sysfs_input_attr(char *class_path, char *attr, char *value, int len)
{
    char path[256];
    int fd;
    db_msg("by hero set_sysfs_input_attr class_path = %s", class_path);
    if (class_path == NULL || *class_path == '\0'
        || attr == NULL || value == NULL || len < 1) {
        return -EINVAL;
    }

    snprintf(path, sizeof(path), "%s/%s", class_path, attr);
    path[sizeof(path) - 1] = '\0';
    fd = open(path, O_RDWR);
    db_msg("by hero set_sysfs_input_attr path = %s", path);
    if (fd < 0) {
        db_msg("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        close(fd);
        return -errno;
    }

    if (write(fd, value, len) < 0) {
            db_msg("path:%s", path);
            db_msg("Could not write SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        close(fd);
        return -errno;
    }

    close(fd);

    return 0;
}

int get_sysfs_input_attr(char *class_path, char *attr, int *ivalue)
{
    char path[256];
    char value[64];
    int len = sizeof(value);
    int fd;
    db_msg("by hero set_sysfs_input_attr class_path = %s", class_path);
    if (class_path == NULL || *class_path == '\0'
        || attr == NULL || value == NULL || len < 1) {
        return -EINVAL;
    }

    snprintf(path, sizeof(path), "%s/%s", class_path, attr);
    path[sizeof(path) - 1] = '\0';
    fd = open(path, O_RDWR);
    db_msg("by hero set_sysfs_input_attr path = %s", path);
    if (fd < 0) {
        db_msg("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        close(fd);
        return -errno;
    }

    if (read(fd, value, len) < 0) {
            db_msg("path:%s", path);
            db_msg("Could not read SysFs attribute \"%s\" (%s).", attr, strerror(errno));
        close(fd);
        return -errno;
    }

    close(fd);

    *ivalue = atoi(value);

    return 0;
}

//正常工作模式下，紧急录像灵敏度
int writeImpactHappenLevel(int level)
{
    //gsensor 正常工作模式碰撞数据处理，放到应用层做，提高精度 by owl
    db_error("GsensorManager level %d",level);
    switch(level)
    {
        case 3:
            mGsensorImpactTarget=G_SENSOR_SENSI_OFF_VAL;
            break;
        case 2:
            mGsensorImpactTarget=G_SENSOR_SENSI_L_VAL;
            break;
        case 1:
            mGsensorImpactTarget=G_SENSOR_SENSI_M_VAL;
            break;
        case 0:
            mGsensorImpactTarget=G_SENSOR_SENSI_H_VAL;
            break;
        default:
            db_msg("Invalid gsensor impact_happen_level:%d", level);
            return -1;
    }
    db_msg("by owl writeImpactHappenLevel impact_happen_level mGsensorImpactTarget=%d \n", mGsensorImpactTarget);
    return 0;
}

int readImpactHappenLevel(void)
{
    int value = 0;

    if(gsensorInfo.classPath[0] == ICHAR)
        db_msg("by owl readImpactHappenLevel classpath err\n");

    get_sysfs_input_attr(gsensorInfo.classPath,"impact_happen_level", &value);

    db_msg("by owl readImpactHappenLevel value=%d \n", value);
    return value;
}


int8_t StopGsensorManager()
{
    if( data_fd >= 0 )
        close(data_fd);
    return 0;
}

void *GsensorManagerThread(void *context)
{
    int res;
    char buf[10] = {0};
    char path[256] = {0};
    snprintf(path, sizeof(path), "%s/%s", gsensorInfo.classPath, "int2_start_status");

    db_error("by hero GsensorManagerThread path = %s", path);
    while(1)
    {
        //sleep(1);
        usleep(100*1000);
        if(m_gsensorIntEnable==0)
        {
            sleep(1);
            continue;
        }

        if(mGsensorParkingImpactSlope==G_SENSOR_SENSI_PARKING_OFF_VAL)
        {
            sleep(1);
            continue;
        }

        memset(buf, 0, sizeof(buf));
        int fd = open(path, O_RDWR);

        if (fd < 0)
        {
            continue;
        }
        else
        {
            if ((res = read(fd, buf, sizeof(buf))) < 0)
                continue;

            if(!strncmp(buf, "1", 1))
            {
//                Mutex::Autolock _l(mLock);
                m_bImpactStatus = true;
                m_ParkingModeIntImpact = 1;
                db_error("GsensorManagerThread read Int impact status=1 !!!!!!! \n");
                write(fd, "0", 1);
            }
            close(fd);
            fd = -1;
        }
    }

    return NULL;
}

bool getImpactStatus()
{
//    Mutex::Autolock _l(mLock);

    return m_bImpactStatus;
}

int setHandleFinish()
{
//    Mutex::Autolock _l(mLock);

    m_bImpactStatus = false;

    return 0;
}

int resetParkingImpactMode()
{
//    Mutex::Autolock _l(mLock);
    m_ParkingModeIntImpact = 0;

    return 0;
}

void *NormalModeImpactEventThread(void *context)
{
    //读取正常使用情况下出现碰撞，3轴加速度数据由驱动处理
    //和gsensor触发中断(int2_start_status)区分，为了后续停车监控方便处理 by owl
    int res;
    char buf[10] = {0};
    char path[256] = {0};
    snprintf(path, sizeof(path), "%s/%s", gsensorInfo.classPath, "impact_status");

    //std::string nodeGsensor = gsensorInfo.classPath;
    //string_replace(nodeGsensor, "/sys/class/input/input", "/dev/input/event");

    db_msg("by hero NormalModeImpactEventThread gsensor_fd node = %s", gsensorInfo.devname);
    int gsensor_fd = open(gsensorInfo.devname, O_RDONLY);
    if (gsensor_fd < 0)
    {
        db_error("open gsensor input error!!!!!!! \n");
        return NULL;
    }

    db_msg("by hero NormalModeImpactEventThread path = %s", path);
    while(1)
    {
        sleep(1);

        if(m_gsensorIntEnable==1)
        {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        int fd = open(path, O_RDWR);
        if (fd < 0)
            continue;
        else
        {
            if ((res = read(fd, buf, sizeof(buf))) < 0)
                continue;

            if(!strncmp(buf, "1", 1))
            {
//                Mutex::Autolock _l(mLock);
                m_bImpactStatus = true;
                db_error("NormalModeImpactEventThread read impact status=1 !!!!!!! \n");
                write(fd, "0", 1);
            }
            close(fd);
            fd = -1;
        }
    }

    close(gsensor_fd);
    gsensor_fd = -1;

    return NULL;
}

void *DealNormalAxisDataThread(void *context)
{
    //读取正常使用情况下出现碰撞，3轴加速度数据处理 by owl
    short x_value, y_value, z_value;
    float gx, gy, gz;

    int result;
    char buf[256] = {0};
    char axis_x_key[256] = {0};
    char axis_y_key[256] = {0};
    char axis_z_key[256] = {0};
    snprintf(axis_x_key, sizeof(axis_x_key), "%s/%s", gsensorInfo.classPath, "axis_x_value");
    snprintf(axis_y_key, sizeof(axis_y_key), "%s/%s", gsensorInfo.classPath, "axis_y_value");
    snprintf(axis_z_key, sizeof(axis_z_key), "%s/%s", gsensorInfo.classPath, "axis_z_value");
    float old_x_value = 0,old_y_value = 0,old_z_value = 0;
    db_error("[OWL_DEBUG] 00 DealNormalAxisDataThread axis_x_key = %s", axis_x_key);
    int count = 0,test = 0;
    char str_pass[] = "P[GSENSOR] PASS";
    char str_fail[] = "F[GSENSOR]:FAIL";
    FILE *fd = fopen(FIFO_GSENSOR_DEV,"wb+");
    int x_fd = -1,y_fd = -1,z_fd = -1;
    while(1)
    {
        usleep(500 * 1000);
//        sleep(1);
/*
        if(m_gsensorIntEnable==1)
        {
            sleep(1);
            continue;
        }
*/
        if(mGsensorImpactTarget==G_SENSOR_SENSI_OFF_VAL)
        {
            sleep(1);
            continue;
        }

        x_fd = open(axis_x_key, O_RDONLY);
        y_fd = open(axis_y_key, O_RDONLY);
        z_fd = open(axis_z_key, O_RDONLY);

        if (x_fd<0 || y_fd<0 || z_fd<0)
        {
            //db_error("[OWL_DEBUG] gsensor axis fd NULL ");
            continue;
        }
        else
        {
            memset(buf, 0, sizeof(buf));
            if ((result = read(x_fd, buf, sizeof(buf))) < 0)
            {
                continue;
            }
            x_value = atoi(buf);

            memset(buf, 0, sizeof(buf));
            if ((result = read(y_fd, buf, sizeof(buf))) < 0)
            {
                continue;
            }
            y_value = atoi(buf);

            memset(buf, 0, sizeof(buf));
            if ((result = read(z_fd, buf, sizeof(buf))) < 0)
            {
                continue;
            }
            z_value = atoi(buf);

            calc_gvalue(x_value, &gx);
            calc_gvalue(y_value, &gy);
            calc_gvalue(z_value, &gz);

            db_error("========new:gx %f gy %f gz %f,old:gx %f gy %f gz %f========",
                    gx,gy,gz,old_x_value,old_y_value,old_z_value);
//          db_info("[OWL_DEBUG] DealNormalAxisDataThread x=%d y=%d z=%d ", x_value, y_value, z_value);
//          db_info("[OWL_DEBUG] DealNormalAxisDataThread gx=%f gy=%f gz=%f ", gx, gy, gz);
            if(gx != old_x_value || gy != old_y_value || gz != old_z_value){
                old_x_value = gx;
                old_y_value = gy;
                old_z_value = gz;
                count = 0;
                test++;
            } else
                count++;
            if(count >= 5)
            {
                fwrite(str_fail,1,strlen(str_fail),fd);
                db_error("gsensor test:gsensor fail");
                goto exit;
            } else if(test >= 5){
                fwrite(str_pass,1,strlen(str_pass),fd);
                db_error("gsensor test:gsensor pass");
                goto exit;
            }
            close(x_fd);
            close(y_fd);
            close(z_fd);
            x_fd = -1;
            y_fd = -1;
            z_fd = -1;
        }
    }
exit:
    close(x_fd);
    close(y_fd);
    close(z_fd);
    fclose(fd);
    return NULL;
}

void calc_gvalue(short g, float *fg)
{
    float MaxSense = 8.0f; //+-4g
    int AdcResolution = 1<<12;   //12bit

    *fg = (MaxSense*g)/AdcResolution;
}

void Max3(float *dtmax, float a, float b, float c)
{
    float temp = 0.0;
    if (temp<a)
    {
        mGsensorOccurAxis=0x78;
        temp = a;
    }
    if (temp<b)
    {
        mGsensorOccurAxis=0x79;
        temp = b;
    }
    if(temp<c)
    {
        mGsensorOccurAxis=0x7A;
        temp = c;
    }
    *dtmax =  temp;
}

void Gfabs(float *a, float b)
{
    *a = (((b) < 0) ? -(b) : (b));
}

static float g_data[5][3] = {0.0};
static int gc =0;
static float avg[3] = {0.0};
static float dtmax, dtx, dty, dtz;
void Check_event(float x, float y, float z)
{
    int i = 0, j = 0;

    if (gc<5)
    {
        g_data[gc][0] = x;
        g_data[gc][1] = y;
        g_data[gc][2] = z;
        gc++;
        avg[0] = (g_data[0][0] + g_data[1][0] + g_data[2][0] + g_data[3][0] + g_data[4][0])/gc;
        avg[1] = (g_data[0][1] + g_data[1][1] + g_data[2][1] + g_data[3][1] + g_data[4][1])/gc;
        avg[2] = (g_data[0][2] + g_data[1][2] + g_data[2][2] + g_data[3][2] + g_data[4][2])/gc;
        return;
    }
    else
    {
        //  printk("x = %.2f  avg[0] = %.2f", x, avg[0]);
        //  printk("y = %.2f  avg[1] = %.2f", y, avg[1]);
        //  printk("z = %.2f  avg[2] = %.2f", z, avg[2]);

        Gfabs(&dtx, (x-avg[0]));
        Gfabs(&dty, (y-avg[1]));
        Gfabs(&dtz, (z-avg[2]));
        Max3(&dtmax, dtx, dty, dtz);
        //db_error("dtmax = %.2f  mGsensorImpactTarget=%f axis=<%c>", dtmax, mGsensorImpactTarget, mGsensorOccurAxis);
        if (dtmax > mGsensorImpactTarget)
        {
//            Mutex::Autolock _l(mLock);
            m_bImpactStatus = true;
            db_error("G-sensor Normal impact dtmax=%f mGsensorImpactTarget=<%f> gc=%d axis=<%c> impact_status=1  !!!!!!  \n\n",dtmax, mGsensorImpactTarget, gc, mGsensorOccurAxis);
            gc = 0;
            return;
        }

        for(i=0; i<4; i++)
        {
            for(j=0; j<3; j++)
            {
                g_data[i][j] = g_data[i+1][j];
            }
        }

        g_data[4][0] = x;
        g_data[4][1] = y;
        g_data[4][2] = z;

        avg[0] = (g_data[0][0] + g_data[1][0] + g_data[2][0] + g_data[3][0] + g_data[4][0])/5.0;
        avg[1] = (g_data[0][1] + g_data[1][1] + g_data[2][1] + g_data[3][1] + g_data[4][1])/5.0;
        avg[2] = (g_data[0][2] + g_data[1][2] + g_data[2][2] + g_data[3][2] + g_data[4][2])/5.0;
    }

}

int main(int argc, char **argv)
{
    char str[50] = {0};
    char str_pass[] = "P[KEY] PASS";
    char str_fail[] = "F[KEY]:FAIL";
    pthread_t KeyThreadID1, keyThreadID2,keyThreadID3;
    printf("run gsensor test...\r\n");
#if 0
    pthread_create(&KeyThreadID1, NULL, KeyFunction1, NULL); //ok_key
    pthread_create(&keyThreadID2, NULL, KeyFunction2, NULL); //power_key
    pthread_create(&keyThreadID3, NULL, KeyFunction3, NULL); //up_down_key
    usleep(20 * 1000);
    while(1)
    {
        if (g_key_code_1 > 0 || g_key_code_2 > 0) {
#ifdef FOX_PRO
            sprintf(str, "P[KEY] %d-%d  %d-%d", g_key_code_1, g_key_val_1, g_key_code_2, g_key_val_2);
            write(fifoFd, str, 50);
#else
            write(fifoFd,str_pass, strlen(str_pass));
#endif
        }
        usleep(20 * 1000);
    }
    close(fifoFd);
#endif
    sensorsDetect();/*detect device,filling sensor_t structure */
    writeGsensorIntEnable(1);

//    pthread_create(&gsensor_tid_, NULL, GsensorManagerThread,NULL);
    //ThreadCreate(&gsensor_impact_tid_, NULL, NormalModeImpactEventThread, this);
    pthread_create(&gsensor_axis_data_tid_, NULL, DealNormalAxisDataThread,NULL);
    pthread_join(gsensor_axis_data_tid_,NULL);
    printf("keytester pass\r\n");
    return 0;
}
