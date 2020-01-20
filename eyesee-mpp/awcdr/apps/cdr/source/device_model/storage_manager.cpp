/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file storage_manager.cpp
 * @brief 存储管理类源文件
 * @author id:826
 * @version v0.3
 * @date 2015-11-02
 * @verbatim
    History:
    - 2016-06-07, change for v40 ipc demo
   @endverbatim
 */
#include "device_model/storage_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/system/event_manager.h"
#include "device_model/system/usb_gadget.h"
#include "bll_presenter/AdapterLayer.h"
#include "bll_presenter/camRecCtrl.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "common/app_def.h"
#include "common/thread.h"
#include <sys/resource.h>

#include <sys/mount.h>
#include <string.h>
#include <dirent.h>
#include "bll_presenter/screensaver.h"

#undef LOG_TAG
#define LOG_TAG "StorageManager"

using namespace EyeseeLinux;
using namespace std;

void StorageManager::Init()
{
    int ret = 0;
    char cmd[128] = {0};

    db_debug("start init");

    if (IsMounted())
	{
        if( CheckFileSystemError()!= true)
		{
			db_error("checkFileSystemError---");
			return ;
		}

		if(DetectTotalCapacitySupported() != true)
			return ;

		CreateAllDirectory();

		pthread_mutex_lock(&m_lock);
		m_storageStatus = MOUNTED;
		pthread_mutex_unlock(&m_lock);
    }
	else
	{
		/*if the card was inserted, but it was not mount in system*
		 *maybe the filesystem in sd-card was not fat-32
		 *need to try mount and infor sdvcam by udev_handler
		 */
		if(!access(m_storageDevice.c_str(), F_OK))
		{
			/*call udev_handler after 2 sencond, if don't delay, this time sdvcam has not creat socket yet*/
			//runnig command "sleep 2 && ls /dev/mmcblk0 && udev_hander add sd /dev/mmcblk0"
			snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s","sleep 2 && ls ", 
					m_storageDevice.c_str(), " && ", UDEV_HANDLER, "add sd", m_storageDevice.c_str());
			m_handlerSD_fp = popen(cmd, "r");
			if(m_handlerSD_fp == NULL)
				db_error("runnig udev_handler erro \n");
		}
	}
}

StorageManager::StorageManager()
    : AsyncObserver(EventMsgHandler, this)
    , m_storageStatus(UMOUNT)
    , m_bDiskFullNotifyFlag(false)
    , m_bFormatSdExit(false)
    , m_bNeedFormat(false)
    , mLoopRecordFlag(true)
    ,m_format_flag(false)
    ,m_occupy_dir(false)
    ,m_ReadOnlyDiskFormatFinish(true)
    ,m_umount_flag(false)
    ,m_LockFileCapacity(0)
{
    pthread_mutex_init(&m_lock, NULL);
    pthread_mutex_init(&m_Notifylock, NULL);
    m_storageDevice = "/dev/mmcblk0";
    m_handlerSD_fp  = NULL;
    memset(m_fileSystemType,0,sizeof(m_fileSystemType));
    Init();

    m_EventManager = EventManager::GetInstance();
    m_EventManager->Attach(this);
    m_formatThread = thread(StorageManager::FormatSdThread, this);
	m_checkCardReadOnlyThread = thread(StorageManager::CheckCardReadOnlyThread, this);
    m_percentReported = 0;
}

StorageManager::~StorageManager()
{
	m_bFormatSdExit = true;
	if( m_formatThread.joinable())
	{
		pthread_cancel(m_formatThread.native_handle());
		m_formatThread.join();
	}

	if( m_checkCardReadOnlyThread.joinable())
	{
		pthread_cancel(m_checkCardReadOnlyThread.native_handle());
		m_checkCardReadOnlyThread.join();
	}
	if(m_handlerSD_fp != NULL)
	{
		pclose(m_handlerSD_fp);
	}
	//if(IsMounted())
    	//UMount(MOUNT_PATH);
    m_EventManager->Detach(this);
    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_Notifylock);
}

/* TODO: parase from config file */
int StorageManager::CreateAllDirectory()
{
	int ret = -1;

	DIR *dirptr = opendir(MOUNT_PATH);
	if( dirptr == NULL)
    {
    	ret = create_dir(MOUNT_PATH);
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	#ifdef USE_CAMB
	dirptr = opendir(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	#endif
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, PHOTO_DIR_A));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, PHOTO_DIR_A));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	#ifdef USE_CAMB
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, PHOTO_DIR_B));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, PHOTO_DIR_B));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
    #endif
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, EVENT_DIR_A));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, EVENT_DIR_A));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
    #ifdef USE_CAMB
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, EVENT_DIR_B));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, EVENT_DIR_B));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	#endif
   dirptr = opendir(DIR_2CAT(MOUNT_PATH, PARK_DIR_A));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, PARK_DIR_A));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
    #ifdef USE_CAMB
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, PARK_DIR_B));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, PARK_DIR_B));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
    #endif
	#if 0
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, VERSION_DIR));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, VERSION_DIR));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
	#endif
    #if 0

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, LOG_DIR));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, LOG_DIR));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, LOG_BIN));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, LOG_BIN));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, VERSION_DIR_NET));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, VERSION_DIR_NET));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
		
    dirptr = opendir(DIR_2CAT(MOUNT_PATH, VERSION_4G_DIR_NET));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, VERSION_4G_DIR_NET));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
		
	dirptr = opendir(DIR_2CAT(MOUNT_PATH, TRAFFICG_DIR_GPS));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, TRAFFICG_DIR_GPS));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, TMP_DIR));
	if(dirptr == NULL)
	{
	    ret = create_dir(DIR_2CAT(MOUNT_PATH, TMP_DIR));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);

	dirptr = opendir(DIR_2CAT(MOUNT_PATH, DD_SERV_LOG));
	if( dirptr == NULL)
	{
		ret = create_dir(DIR_2CAT(MOUNT_PATH, DD_SERV_LOG));
		if( ret < 0 )
			return -1;
	}
	else
		closedir(dirptr);
    #endif

	return 0;
}

int StorageManager::GetStorageStatus()
{
    return m_storageStatus;
}


int StorageManager::FreeSpace()
{
	return 0;
	pthread_mutex_lock(&m_lock);
    uint32_t total, free;
    MediaFileManager *mfm = MediaFileManager::GetInstance();

    if (m_storageStatus == STORAGE_LOOP_COVERAGE)
	{
		pthread_mutex_unlock(&m_lock);
        db_msg("storage is full , the oldest file will be deleted");
        if (mfm->DeleteLastFile("video_A", 1) < 0)
		{
            db_msg("delete last file failed");
            return -1;
        }
        this->GetStorageCapacity(&free, &total);
    }
	else if (m_storageStatus == STORAGE_DISK_FULL)
	{
        db_warn("will do not free space, file need be keeped");
		pthread_mutex_unlock(&m_lock);
        return -1;
    }

	pthread_mutex_unlock(&m_lock);

    return 0;
}

int StorageManager::Mount(const char *mount_point, const char *dev_node)
{
    if (dev_node == NULL)
        dev_node = m_storageDevice.c_str();

    int ret = mount(dev_node, mount_point, m_fileSystemType, 0, NULL);
    if (ret < 0)
	{
        db_error("mount '%s' to '%s' failed, fs type: '%s', %s",dev_node, mount_point, m_fileSystemType, strerror(errno));
        return -1;
    }

    return 0;
}

int StorageManager::UMount(const char *mount_point)
{
    int ret;
    db_info("umount from %s", mount_point);
    ret = umount(mount_point);
    if (ret < 0) {
        db_error("umount failed, %s, try force umount", strerror(errno));
        ret = umount2(mount_point, MNT_FORCE);
        if (ret < 0) {
		if(errno == EBUSY){
	              db_error("force umount failed, %s", strerror(errno));
			system("lsof /mnt/extsd");
		}else if(errno == EINVAL){
			db_error("force umount failed, %s", strerror(errno));
			}
            return -errno;
        }
    }

    memset(m_fileSystemType,0,sizeof(m_fileSystemType));
	pthread_mutex_lock(&m_lock);
    m_storageStatus = UMOUNT;
	pthread_mutex_unlock(&m_lock);

    return 0;
}

bool StorageManager::IsMounted()
{
    char line[255] = {0};
	FILE *fp = fopen("/proc/mounts", "r");

    if (NULL == fp)
        return false;

    while(fgets(line, sizeof(line), fp))
	{
		if (line[0] == '/' && (strstr(line, MOUNT_PATH) != NULL))
		{
			if( strstr(line,"rw,") == NULL)
			{
				db_warn("sd card is read only");
				fclose(fp);
				return false;
			}
		
			fclose(fp);
			fp = NULL;
			return true;
		}

        memset(line,'\0',sizeof(line));
        continue;
    }

    if(fp)
        fclose(fp);

    return false;
}

int StorageManager::DirectFormat()
{
    int ret = 0;
    int status = 0xAAAA;
    char cmd[128] = {0};

    /*checking sd-card whether if it was insert when Format sd-card*/
    /*if the card was gone,change m_storageStatus flage*/
    if(access(m_storageDevice.c_str(), F_OK))
    {
	pthread_mutex_lock(&m_lock);
        m_storageStatus = UMOUNT;
	pthread_mutex_unlock(&m_lock);
	db_error("can not find sd-card. do you insert it?\n");
	return -1;
    }

    db_info("formatting...");

    snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s %s %s", FORMAT_BIN, "-F 32", "-n 2", "-O allwinner", "-b 65536", "-c 128", "-L 'DASHCAM'", m_storageDevice.c_str());
    status = system(cmd);
	if(-1 == status)
	{
		db_error("ready fork fail");
		return -1;
	}
	else
	{
	    if (WIFEXITED(status))
		{
		    if (WEXITSTATUS(status) == 0)
			{
					pthread_mutex_lock(&m_lock);
			        m_storageStatus = FORMATTING;
					pthread_mutex_unlock(&m_lock);
			        strncpy(m_fileSystemType,"vfat",sizeof("vfat"));
			        ret = 0;
		   	 }
			else
			{
			    ret = -1;
			}
	    	}
		else
		{
		    ret = -1;
		}
	}
    return ret;
}

int StorageManager::Format()
{
    db_info("prepare format, umount disk first");
	int ret = -1;
	pthread_mutex_lock(&m_lock);
    if (m_storageStatus != UMOUNT)
	{
		MediaFileManager *mfm = MediaFileManager::GetInstance();
		pthread_mutex_unlock(&m_lock);
		mfm->Update(MSG_STORAGE_UMOUNT);
		setFormatFlag(true);
		Notify(MSG_STORAGE_UMOUNT);
		while(m_occupy_dir)  //预防正在往卡中拷贝数据中去格式化
		{
			db_warn("mnt/extsd/ has been Occupied ,wait ...");
			usleep(500*1000);
		}
		sleep(2);
		ret = UMount(MOUNT_PATH);
		if(ret == -EBUSY)
			return -1;
    }
		
	pthread_mutex_unlock(&m_lock);

    ret = DirectFormat();
    if (ret != 0)
	{
        db_error("format failed\n");
        this->Notify(MSG_STORAGE_FORMAT_FINISHED);
        return ret;
    }else{
		db_error("format successfully\n");
		/*
		  *  If format successfully, rescan  partitions by ioctl.
		  *  It will report the event to udevd.
		  */
    		int fd;
		fd = open(m_storageDevice.c_str(),O_RDWR);
		if (fd < 0) {
			return fd;
		}

		ioctl(fd, BLKRRPART);
		close(fd);
    	}
	
	if(!IsMounted() )
		Mount(MOUNT_PATH,m_storageDevice.c_str());

	if( CheckFileSystemError()!= true)
	{
		db_error("checkFileSystemError---");
		return ret;
	}
	
	if(DetectTotalCapacitySupported() != true)
		return ret;


	ret = CreateAllDirectory();
	if( ret < 0 )
	{
		db_warn("creat directory failed\n");
		return -1;
	}

    this->Notify(MSG_STORAGE_FORMAT_FINISHED);
	pthread_mutex_lock(&m_lock);
	m_storageStatus = MOUNTED;
	pthread_mutex_unlock(&m_lock);
	m_ReadOnlyDiskFormatFinish = true;

    this->Notify(MSG_STORAGE_MOUNTED);

    return ret;
}

inline static uint32_t kscale(uint32_t b, uint32_t bs)
{
    return (b * (uint64_t) bs + 1024/2) >> 10;
}

bool StorageManager::DetectTotalCapacitySupported()
{
	if(IsMounted())
	{
		struct statfs diskinfo;
		uint32_t total_size = 0;
		uint32_t total= 0;
		if(statfs(MOUNT_PATH, &diskinfo) != -1)
		{
			total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);
			total = (total_size >> 10); //MB
			db_msg("total_size:%u Bytes  total_size: %uMB", total_size,total);
            #if 0
			if(total <= 16*1024 || total > 64*1024)
			{
			   Notify(MSG_STORAGE_CAP_NO_SUPPORT);
			   UMount(MOUNT_PATH);
			   return false;
			}
            #endif
			return true;
		}
	}

	return false;
}

uint32_t StorageManager::getDirSize(const std::string p_DirName)
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_lock);


	char name[128] = {0};
	snprintf(name, sizeof(name), "du -s %s | cut -f1 > /tmp/size.txt", p_DirName.c_str());
	system(name);
	int fd = open("/tmp/size.txt", O_RDONLY, 444);
	if( fd < 0 )
	{
		system("rm /tmp/size.txt");
		return -1;
	}

	char buf[128] = {0};

	int ret = read(fd, buf, sizeof(buf));
	if( ret < 0 )
	{
		close(fd);
		system("rm /tmp/size.txt");
		return -1;
	}

	close(fd);
	system("rm /tmp/size.txt");
	return atol(buf);
}

uint32_t StorageManager::getLockFileCapacity()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_lock);

	return m_LockFileCapacity;
}

int StorageManager::setLockFileCapacity(int32_t p_value)
{
	if( p_value > 0 )
		m_LockFileCapacity += p_value/1024/1024;

	return m_LockFileCapacity;
}

bool StorageManager::IsFrontCameraFull()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;

	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		avail_size = kscale(diskinfo.f_bavail, diskinfo.f_bsize);
		if(avail_size <= LOOP_COVERAGE_SIZE)
		{
			db_info("storage capacity less than %dM, Loop Coverage",LOOP_COVERAGE_SIZE);
			return true;
		}
	}

	return false;
}

CapacityStatus StorageManager::getBackCameraCapacityStatus()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return CAPACITY_ERROR;
	}
	pthread_mutex_unlock(&m_lock);


	struct statfs diskinfo;
	uint32_t avail_size = 0;

	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		avail_size = kscale(diskinfo.f_bavail, diskinfo.f_bsize);
		if(avail_size <= STORAGE_RESERVE_SIZE)
		{
			db_warn("storage capacity less than %dM, Loop Coverage", STORAGE_RESERVE_SIZE / 1024);
			return CAPACITY_EMERGENCY;
		}

		if(avail_size <= FRONT_REC_RESERVE_SIZE + STORAGE_RESERVE_SIZE)
		{
			unsigned int dir_size = getDirSize("/mnt/extsd/video/R");//KB			
            if(dir_size <= FRONT_REC_RESERVE_SIZE) {
			    //if(avail_size - dir_size<= STORAGE_RESERVE_SIZE )
			    {
			        return CAPACITY_EMERGENCY;
			    }
            }
			return CAPACITY_WARN;
		}
#if 0
		else if(avail_size <=FRONT_REC_RESERVE_SIZE+STORAGE_RESERVE_SIZE+WARN_RESERVE_SIZE)
		{
            printf("hds->getBackCameraCapacityStatus avail_size <= %d\n", FRONT_REC_RESERVE_SIZE+STORAGE_RESERVE_SIZE+WARN_RESERVE_SIZE);
			unsigned int dir_size = getDirSize("/mnt/extsd/DCIM/100MEDIA/A");//KB			
            if(avail_size >= dir_size) {
			    if((avail_size - dir_size) <= (STORAGE_RESERVE_SIZE+WARN_RESERVE_SIZE))
			    {
                    printf("hds->getBackCameraCapacityStatus avail_size <= %d\n", FRONT_REC_RESERVE_SIZE+STORAGE_RESERVE_SIZE+WARN_RESERVE_SIZE);
			    	return CAPACITY_WARN;
			    }
            }
		}
#endif
	}

	return CAPACITY_ENOUGH;
}

int StorageManager::GetStorageThreshold(uint32_t threshold, uint32_t *percent)
{
    FILE *fp = NULL;
    char buf[128] = {0};
    if(threshold < 0 || threshold > 100 || !percent) {
        db_msg("GetStorageThreshold invalid argument!");
        return -1;
    }
    if((fp = popen("df | grep mmcblk0 | awk {'print $5'}","r")) != NULL) {
        if(fgets(buf, 128, fp) != NULL) {
            char tmp[4] = {0};
            if(buf[2] == '%') {
                strncpy(tmp, buf, 2);
            }
            else if(buf[1] == '%') {
                strncpy(tmp, buf, 1);
            }
            else {
                pclose(fp);
                return -1;
            }

            *percent = atoi(tmp);
            db_msg("GetStorageThreshold threshold:%d percent:%d", threshold, *percent);
            if(*percent > threshold && m_percentReported == 0) {
                pclose(fp);
                m_percentReported = 1;
                return 0;
            }
            else if(*percent > threshold && m_percentReported == 1) {
                //TODO have reported
            }
            else {
                m_percentReported = 0;
            }
        }
		pclose(fp);
    }

    return 1;
}

void StorageManager::GetStorageCapacity(uint32_t *avail, uint32_t *total)
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		*avail = 0;
		pthread_mutex_unlock(&m_lock);
		return ;
	}
	pthread_mutex_unlock(&m_lock);


    struct statfs diskinfo;
    uint32_t blocks_used;
    uint32_t blocks_percent_used;
    uint32_t total_size = 0;
    uint32_t avail_size = 0;

    if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
        if (total)
		{
            total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);
            *total = total_size >> 10; //MB
        }

        if (avail)
		{
            avail_size = kscale(diskinfo.f_bavail, diskinfo.f_bsize),
            *avail = avail_size >> 10; //MB
        }

        blocks_used = diskinfo.f_blocks - diskinfo.f_bfree;
        blocks_percent_used = 0;
        if (blocks_used + diskinfo.f_bavail)
		{
            blocks_percent_used = (blocks_used * 100ULL+(blocks_used + diskinfo.f_bavail)/2) / (blocks_used + diskinfo.f_bavail);
        }

        if (mLoopRecordFlag)
		{
            if(*avail <= LOOP_COVERAGE_SIZE)
			{
                db_info("storage capacity is %uM, less than %dM, Loop Coverage", *avail, LOOP_COVERAGE_SIZE);
                return;
            }
        }

        if (*avail <= RESERVED_SIZE)
		{
            if (mLoopRecordFlag)
			{
                db_error("fatal error, storage capacity is %uM, less than %dM", *avail, RESERVED_SIZE);
            }
			else
			{
				pthread_mutex_lock(&m_lock);
                m_storageStatus = STORAGE_DISK_FULL;
				pthread_mutex_unlock(&m_lock);
                pthread_mutex_lock(&m_Notifylock);
                if( m_bDiskFullNotifyFlag )
                {
                    db_warn("storage capacity is %uM, less than %dM", *avail, RESERVED_SIZE);
                    this->Notify(MSG_STORAGE_IS_FULL);
                    m_bDiskFullNotifyFlag = false;
                }
                pthread_mutex_unlock(&m_Notifylock);
            }
        } 
    }
}

int32_t StorageManager::GetFileSize(const char *file_name)
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_lock);

    struct stat buf;
    stat(file_name, &buf);
    if (buf.st_size == 0)
	{
        db_msg("%s size is 0", file_name);
    }

    return buf.st_size;
}

bool StorageManager::IsSupportSystemType()
{
    FILE *ptr = NULL;
    char buf_ps[128];
    if((ptr = popen("/usr/sbin/blkid","r"))!= NULL)
    {
        memset(m_fileSystemType,0,sizeof(m_fileSystemType));
        while(fgets(buf_ps, 128, ptr) !=NULL)
        {
            if(strstr(buf_ps,"/dev/mmcblk0")== NULL)
                continue;
            char *saveptr = strstr(buf_ps,":");
            char devname[128];
            memset(devname, 0, sizeof(devname));
            strncpy(devname, buf_ps, saveptr-buf_ps);
            char *tmp_result1 = strstr(buf_ps,"TYPE=\"");
            char *tmp_result2 = strstr(tmp_result1+sizeof("TYPE=\"")-1,"\"");
            memcpy(m_fileSystemType,tmp_result1+sizeof("TYPE=\"")-1,sizeof(tmp_result2-tmp_result1));
            break;
        }
        pclose(ptr);
        if( strlen(m_fileSystemType) != 0)
        {
            if( !strcmp(m_fileSystemType,"ext4") || !strcmp(m_fileSystemType,"vfat"))
            {            	
                return true;
            }
            else
            {
                db_error("unsupport filesystem: %s", m_fileSystemType);
                return false;
            }
        }
    }
    db_error("check IsSupportSystemType failed\n");
    return false;
}

bool StorageManager::CheckFileSystemError()
{
    int status = 0xAAAA;
    char cmd[128] = {0};
    if( IsSupportSystemType()!= true)
    {
	    pthread_mutex_lock(&m_lock);
		m_storageStatus = STORAGE_FS_ERROR;
		pthread_mutex_unlock(&m_lock);

        return false;
    }
#if 1
    snprintf(cmd, sizeof(cmd), "%s %s %s", CHECK_FS_BIN1, "-p", m_storageDevice.c_str());
    status = system(cmd);
    if (WIFEXITED(status))
	{
        if (WEXITSTATUS(status) == 0)
		{
            db_msg("No error found");
            return true;
        }
		else if (WEXITSTATUS(status) == 1)
		{
            db_msg("Error fixed");
            return true;
        }
		else if (WEXITSTATUS(status) == 2)
		{
            db_error("Usage error");
			pthread_mutex_lock(&m_lock);
			m_storageStatus = STORAGE_FS_ERROR;
			pthread_mutex_unlock(&m_lock);
            return false;
        }
    }
	else
	{
        db_msg("CHECK_FS_BIN failed");
	    pthread_mutex_lock(&m_lock);
		m_storageStatus = STORAGE_FS_ERROR;
		pthread_mutex_unlock(&m_lock);
        return false;
    }
#endif
    return true;
}

int StorageManager::RemoveFile(const char *file_name)
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_lock);

    int ret = -1;
    if (file_name == NULL)
	{
        db_error("invalid parameter, <NULL>");
        return ret;
    }

    if (FILE_EXIST(file_name))
	{
        if ((ret = remove(file_name)) < 0)
		{
            db_error("delete file failed:%s", strerror(errno));
        }
    }
	else
	{
        db_warn("file '%s' is not exist", file_name);
        ret = 0;
    }

    return ret;
}

int StorageManager::RemoveFiles(const char *path)
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_lock);

    int ret = -1;
    DIR *dir = NULL;
    struct dirent *dirent = NULL;
    struct stat buf;

    if (path == NULL)
	{
        db_error("invalid parameter, <NULL>");
        return ret;
    }

    stat(path, &buf);
    if (S_ISDIR(buf.st_mode))
	{
        if ((dir = opendir(path)) != NULL)
		{
            chdir(path);
            while ((dirent = readdir(dir)))
			{
                stat(dirent->d_name, &buf);
                if (!S_ISDIR(buf.st_mode))
				{
                    ret = remove(dirent->d_name);
                    db_msg("delete file %s", dirent->d_name);
                }
            }
            free(dirent);
            closedir(dir);
        }
    }
	else
	{
        db_error("invalid parameter, %s is not a directory", path);
        ret = -1;
    }

    return ret;
}

void StorageManager::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg)
	{
        case MSG_STORAGE_MOUNTED:
		{
            db_error("TO CheckFileSystemError");
            if(CheckFileSystemError()!= true)
            {
                this->Notify(MSG_STORAGE_FS_ERROR);
                db_error("CheckFileSystemError");
                return;
            }
            usleep(500 * 1000);
            if( DetectTotalCapacitySupported() )
            {
                pthread_mutex_lock(&m_lock);
                m_storageStatus = MOUNTED;
                pthread_mutex_unlock(&m_lock);
                if( CreateAllDirectory() < 0 )
                    return ;
            }
            m_umount_flag = false;
            db_error("send MSG_STORAGE_MOUNTED start");
            Notify(msg);
            db_error("send MSG_STORAGE_MOUNTED end");
			
//			break;
            return;
        }

        case MSG_STORAGE_UMOUNT:
            pthread_mutex_lock(&m_lock);
            m_storageStatus = UMOUNT;
            pthread_mutex_unlock(&m_lock);
            m_umount_flag = true;
            db_error("send MSG_STORAGE_UMOUNT start");
            Notify(msg);
            db_error("send MSG_STORAGE_UMOUNT end");
            m_ReadOnlyDiskFormatFinish = true;
            //break;
            return;
        case MSG_STORAGE_FS_ERROR:
		pthread_mutex_lock(&m_lock);
        	m_storageStatus = STORAGE_FS_ERROR;
		pthread_mutex_unlock(&m_lock);
            break;
        case MSG_ACCON_HAPPEN:
        	 db_msg("send MSG_ACCON_HAPPEN start");
        	 Notify(msg);
        	 db_msg("send MSG_ACCON_HAPPEN end");
        	 return;
        case MSG_ACCOFF_HAPPEN:
			m_ReadOnlyDiskFormatFinish = true;
			db_msg("send MSG_ACCOFF_HAPPEN start");
			Notify(msg);
			db_msg("send MSG_ACCOFF_HAPPEN end");
			return;
		case MSG_GPSON_HAPPEN:
        	 Notify(msg);
        	 return;
        case MSG_GPSOFF_HAPPEN:
			Notify(msg);
			return;
        case MSG_AHD_CONNECT:
        case MSG_AHD_REMOVE:
            Notify(msg);
            return;
        default:
            break;
    }
    db_msg("handle msg exit:%d", msg);
    AsyncObserver::HandleMessage(msg);
}

void *StorageManager::EventMsgHandler(void *context)
{
    StorageManager *sm = static_cast<StorageManager *>(context);
    prctl(PR_SET_NAME, "StorageManager", 0, 0, 0);
    pthread_mutex_lock(&sm->msg_lock_);
    db_msg("EventMsgHandler msg receive:%d", sm->msg_);
    switch (sm->msg_) {
        case MSG_STORAGE_FS_ERROR:
		case MSG_PM_RECORD_STOP:
		case MSG_PM_RECORD_START:
		case MSG_TO_PREVIEW_WINDOW:
		case MSG_UPDATED_SYSTEM_TIEM_BY_4G:
		case MSG_BATTERY_LOW:	
		case MSG_UNBIND_SUCCESS:
		case MSG_BIND_SUCCESS:
		case MSG_CLOSE_STANDBY_DIALOG:
        case MSG_USB_HOST_CONNECTED:
        case MSG_USB_HOST_DETACHED:
            sm->Notify(sm->msg_);
            break;
		case MSG_IMPACT_HAPPEN:
	        sm->Notify(sm->msg_);
			break;	
		case MSG_ACCOFF_HAPPEN:
			sm->m_ReadOnlyDiskFormatFinish = true;
			sm->Notify(sm->msg_);
			break;
#if 0
        case MSG_STORAGE_UMOUNT:
		{
			sm->m_umount_flag = true;
			sm->Notify(sm->msg_);
			//if( sm->IsMounted() )
			//{
			//	sm->UMount(MOUNT_PATH);
			//}
			break;
		}
	case MSG_STORAGE_MOUNTED:
		sm->m_umount_flag = false;
		sm->Notify(sm->msg_);
		break;
#endif
	default:
            break;
    }
    pthread_mutex_unlock(&sm->msg_lock_);
    return NULL;
}

int  StorageManager::readCpuInfo(char *info)
{
       char line[128];
       FILE *fp = fopen("/sys/class/sunxi_info/sys_info", "r");
       if(fp==NULL)
       {
               db_msg("open file failed.");
               return -1;
       }
       while(fgets(line, sizeof(line), fp))
       {
           if (strncmp(line, "sunxi_chipid", strlen("sunxi_chipid")) == 0)
           {
               char *str = strstr(line, ":");
               str += 5;
               if(strlen(str)>8)
               {
		     char buf[10]={0};
		     strncpy(buf,str,sizeof(char)*8);
                   strcpy(info,buf);
                   db_msg("buf=%s,ret=%s",buf,info);
               }
           }
       }
       fclose(fp);
       return 0;
}

int StorageManager::SetFormat()
{
	m_bNeedFormat = true;

	return 0;
}

bool StorageManager::GetFormat()
{
	return m_bNeedFormat;
}

void StorageManager::CheckCardReadOnlyThread(StorageManager *self)
{
	while(1)
	{
	    char line[255] = {0};
		FILE *fp = fopen("/proc/mounts", "r");
		static int reset_time = 0;
	    if (NULL == fp || !self->m_ReadOnlyDiskFormatFinish)
	    {
	    	reset_time++;
			if(fp)
			{
				fclose(fp);
				fp = NULL;
			}

			if(reset_time > 60 && !self->m_ReadOnlyDiskFormatFinish)
			{
				reset_time = 0;
				self->m_ReadOnlyDiskFormatFinish = true;
			}

	    	sleep(1);
	    	continue;
	    }
		reset_time = 0;

	    while(fgets(line, sizeof(line), fp))
		{
	        if (line[0] == '/' && (strstr(line, MOUNT_PATH) != NULL))
			{
				if( strstr(line,"rw,") == NULL)
				{
					pthread_mutex_lock(&self->m_lock);
					self->m_storageStatus = STORAGE_FS_ERROR;
					pthread_mutex_unlock(&self->m_lock);
					self->m_ReadOnlyDiskFormatFinish = false;
                    db_error("CheckCardReadOnlyThread MSG_STORAGE_FS_ERROR\n");
					self->Notify(MSG_STORAGE_FS_ERROR);
					break;
				}
	        }
	    }

	    if(fp)
	    {
	     	fclose(fp);
			fp = NULL;
	    }
		sleep(1);
	}
}

void StorageManager::FormatSdThread(StorageManager *self)
{
	while(!self->m_bFormatSdExit )
	{
		if(self->GetFormat())
		{
			//stop record
			CamRecCtrl::GetInstance()->StopAllRecord();
			self->Format();
			self->m_bNeedFormat = false;
			EventReportMsg event;
            event.err_no = 0;
			event.event_type = EVENT_SD_FORMAT_FINISH;
			event.file_name.clear();
			AdapterLayer::GetInstance()->notifyMessage(event);
		}
		sleep(1);
	}
}


void StorageManager::CapacityCheckThread(StorageManager *self)
{
    while (1)
	{
        if (self->GetStorageStatus() == STORAGE_LOOP_COVERAGE)
		{
            self->FreeSpace();
        }
        usleep(500*1000);
    }
}

bool StorageManager::CheckStorageIsOk()
{
    pthread_mutex_lock(&m_lock);
    if( m_storageStatus == STORAGE_DISK_FULL || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == UMOUNT || m_storageStatus == FORMATTING) {
        pthread_mutex_unlock(&m_lock);
        db_warn("storage status: %d", m_storageStatus);
        return false;
    }
    pthread_mutex_unlock(&m_lock);

    return true;
}

int StorageManager::my_system(const char *cmd)
{  
	pid_t	   pid;  
	int		   status;	
	struct	   sigaction ignore, saveintr, savequit;  
	sigset_t    childmask, savemask;

	if (cmd == NULL)	
		return (1);

	ignore.sa_handler = SIG_IGN;  
	ignore.sa_flags = 0;
	sigemptyset(&ignore.sa_mask);  
	if (sigaction(SIGINT, &ignore, &saveintr) < 0)	
		return (-1);  

	if (sigaction(SIGQUIT, &ignore, &savequit) < 0)	
		return (-1);  

	sigemptyset(&childmask);  
	sigaddset(&childmask, SIGCHLD);	
	if (sigprocmask(SIG_BLOCK, &childmask, &savemask) < 0)	
		return (-1);  

	if ((pid = fork()) < 0)
	{  
		status = -1;
	}
	else if (pid == 0)
	{
		struct rlimit rl;
		if( getrlimit(RLIMIT_NOFILE,&rl) < 0)
		{
			db_msg("getrlimit failed\n");
		}
		
		if( rl.rlim_max == RLIM_INFINITY)
		{
			rl.rlim_max = 1024;
		}
		for(unsigned int i =0; i< rl.rlim_max;i++)
		{
			close(i);
		}	

		sigaction(SIGINT, &saveintr, NULL);	
		sigaction(SIGQUIT, &savequit, NULL);  
		sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);	
		execl("/bin/sh", "sh", "-c", cmd, (char *)0);  
		_exit(127);	
	}
	else
	{
		while (waitpid(pid, &status, 0) < 0)
		{	
			if (errno != EINTR)
			{
				status = -1;
				break;  
			}  
		}  

		if (sigaction(SIGINT, &saveintr, NULL) < 0)  
			return (-1);  

		if (sigaction(SIGQUIT, &savequit, NULL) < 0)  
			return (-1);  

		if (sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL) < 0) /* */  
			return (-1);  		
	}  

	return (status);  
} 

int StorageManager::IsHighClassCard()
{
	if(IsMounted())
	{
		DIR *dir = opendir("/sys/devices/platform/soc/sdc0/mmc_host/mmc0");
		if(dir != NULL)
		{
			char fileName[128] = {0};
			struct dirent *dirp;
			while((dirp = readdir(dir)) != NULL)
			{
				if( strncmp(dirp->d_name, "mmc0", strlen("mmc0")) == 0)
				{
					snprintf(fileName, sizeof(fileName), "/sys/devices/platform/soc/sdc0/mmc_host/mmc0/%s/ssr", dirp->d_name);
					break;
				}
			}
			closedir(dir);
			FILE *ptr = fopen(fileName, "r");
			if(ptr != NULL)
			{
				char buf[128];
				fgets(buf, 128, ptr);
				unsigned int size = strlen(buf);
				if(size < 19)
				{
					fclose(ptr);
					return ERROR_SPEED;
				}
				if( (strncmp(buf+16, "00", strlen("00")) == 0) || (strncmp(buf+16, "01", strlen("00")) == 0) || (strncmp(buf+16, "02", strlen("00")) == 0) || (strncmp(buf+16, "03", strlen("00")) == 0))
				{
					db_warn("low speed sd card\n");
					fclose(ptr);
					return LOW_SPEED;
				}
				else
				{
					db_error("hight speed sd card\n");
					fclose(ptr);
					return HIGH_SPEED;
				}
			}		

		}
	}
	
	return ERROR_SPEED;
}

int StorageManager::ForceUmount()
{	
	int ret = -1;
	if(IsMounted()){
		ret = UMount(MOUNT_PATH);
		if( ret < 0 ){
			ret = UMount(MOUNT_PATH);
			if( ret < 0 )
				return -1;
		}
	}
	return 0;
}

bool StorageManager::CheckPhotoBDirFull()
{
    pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);
	unsigned int dir_size = getDirSize("/mnt/extsd/photo/R");//KB
	db_msg("CheckPhotoBDirFull the dir_size = %d",dir_size);
	if(dir_size > PHOTO_B_RESERVE_SIZE)	// total_size * 60 / 100
	{	// 目录文件大小+预留空间 > 总大小的60%
		db_warn("storage capacity less than %dM, Loop Coverage",LOOP_COVERAGE_SIZE);
		return true;
	}
    return false;
}


bool StorageManager::CheckPhotoADirFull()
{
    pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);
	unsigned int dir_size = getDirSize("/mnt/extsd/photo/F");//KB
	db_msg("CheckPhotoADirFull the dir_size = %d",dir_size);
	if(dir_size > PHOTO_A_RESERVE_SIZE)	// total_size * 60 / 100
	{	// 目录文件大小+预留空间 > 总大小的60%
		db_warn("storage capacity less than %dM, Loop Coverage",LOOP_COVERAGE_SIZE);
		return true;
	}
    return false;
}



bool StorageManager::CheckVideoRecordFDirFull()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;


	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{	// f_blocks 文件系统数据块总数           f_bsize 经过优化的传输块大小
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
		unsigned int dir_size_n_f = getDirSize("/mnt/extsd/video/F");//KB	
		unsigned int dir_size_e_f = getDirSize("/mnt/extsd/event/F");
        db_msg("F video dir the VF = %d ,EF = %d totalsize = %d",dir_size_n_f,dir_size_e_f,(60*total_size)/100);
		if((dir_size_n_f+dir_size_e_f + STORAGE_RESERVE_SIZE)*100 > 60*total_size)	// total_size * 60 / 100
		{	// 目录文件大小+预留空间 > 总大小的60%
			db_warn("F video dir storage capacity is revice full should clc write");
			return true;
		}

	}

	return false;
}

bool StorageManager::CheckVideoRecordRDirFull()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;


	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
		unsigned int dir_size_n_r = getDirSize("/mnt/extsd/video/R");//KB
		unsigned int dir_size_e_r = getDirSize("/mnt/extsd/event/R");//KB
        db_msg("R video the VR = %d ,ER = %d totalsize = %d",dir_size_n_r,dir_size_e_r,(20*total_size)/100);
		if((dir_size_n_r+dir_size_e_r)*100 > 20*total_size)
		{	// 目录文件大小+预留空间 > 总大小的20%
			db_warn("R video dir storage capacity is revice full should clc write");
			return true;
		}
	}

	return false;
}

uint32_t StorageManager::getvideoFDirReserveSize()
{
    uint32_t reserve_size = 0;
    pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;


	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
	    unsigned int dir_size_n_f = getDirSize("/mnt/extsd/video/F");//KB
		unsigned int dir_size_e_f = getDirSize("/mnt/extsd/event/F");
        reserve_size = ((60*total_size)/100) - dir_size_n_f -dir_size_e_f -STORAGE_RESERVE_SIZE;
		db_msg("F video dir total size = %d, reserve_size = %d",(60*total_size)/100,reserve_size);
        return reserve_size*1024;
	}
    return reserve_size;
}

uint32_t StorageManager::getvideoRDirReserveSize()
{
    uint32_t reserve_size = 0;
    pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;


	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
	    unsigned int dir_size_n_f = getDirSize("/mnt/extsd/video/R");//KB
		unsigned int dir_size_e_f = getDirSize("/mnt/extsd/event/R");
        reserve_size = ((20*total_size)/100) - dir_size_n_f -dir_size_e_f;
		db_msg("R video dir total size = %d, reserve_size = %d",(60*total_size)/100,reserve_size);
        return reserve_size*1024;
	}
    return reserve_size;
}

uint32_t StorageManager::getvideoParkDirReserveSize()
{
    uint32_t reserve_size = 0;
    pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;

	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
	    unsigned int park_dir_size = getDirSize("/mnt/extsd/park");//KB
        reserve_size = ((15*total_size)/100) - park_dir_size;
		db_msg("park video dir total size = %d, reserve_size = %d",(15*total_size)/100,reserve_size);
        return reserve_size*1024;
	}
    return reserve_size;
}



bool StorageManager::CheckParkRecordDirFull()
{
	pthread_mutex_lock(&m_lock);
	if (m_storageStatus == UMOUNT || m_storageStatus == STORAGE_FS_ERROR || m_storageStatus == FORMATTING)
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	pthread_mutex_unlock(&m_lock);

	struct statfs diskinfo;
	uint32_t avail_size = 0;


	if(statfs(MOUNT_PATH, &diskinfo) != -1)
	{
		uint32_t total_size = kscale(diskinfo.f_blocks, diskinfo.f_bsize);//KB
		unsigned int dir_size = getDirSize("/mnt/extsd/park");//KB
		db_msg("park video dir size = %d total size = %d",dir_size,(15*total_size)/100);
		if((dir_size*100)> 15*total_size)
		{	// 目录文件大小+预留空间 > 总大小的15%
			db_warn("Park video dir storage capacity is revice full should stop record");
			return true;
		}
	}

	return false;
}


int StorageManager::MountToPC()
{
    int ret = 0;
    if (UMount(MOUNT_PATH) < 0) {
        system("lsof " MOUNT_PATH);
        db_warn("can not mount to pc");
        return -1;
    }

    db_msg("mount to pc");
    USBGadget::GetInstance()->ActiveMassStorage(m_storageDevice);

    return ret;
}

int StorageManager::UMountFromPC()
{
    db_warn("habo---> UMountFromPC");
    int ret = 0;
    USBGadget::GetInstance()->DeactiveMassStorage();
    strncpy(m_fileSystemType,"vfat",sizeof("vfat"));
    if (Mount(MOUNT_PATH, m_storageDevice.c_str()) < 0) {
        db_warn("remount as device disk failed");
        return -1;
    }
    db_warn("habo---> UMountFromPC  ready to sendmsg has been mount the sdcard");
    pthread_mutex_lock(&m_lock);
	m_storageStatus = MOUNTED;
	pthread_mutex_unlock(&m_lock);
    this->Notify(MSG_STORAGE_MOUNTED);
    return ret;
}


