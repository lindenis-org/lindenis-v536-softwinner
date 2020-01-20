#include "common/utils/utils.h"
#include "common/common_inc.h"
#include "common/app_log.h"

#include <string.h>
#include <errno.h>

#undef LOG_TAG
#define LOG_TAG "Utils"

int8_t create_dir(const char *dirpath)
{
    struct stat st;
    if (dirpath == NULL) {
        return -1;
    }

    int i, len;
    len = strlen(dirpath);
    char dir[len+1];
    dir[len] = '\0';

    strncpy(dir, dirpath, len);

    for (i = 0; i < len; i++) {
        if (dir[i] == '/' && i > 0) {
            dir[i]='\0';
            if (stat(dir, &st) < 0) {
                int ret;
                if ( (ret = mkdir(dir, 0755)) < 0) {
                    db_error("mkdir '%s' filed:%s", dir, strerror(errno));
                    return -1;
                }
            }
            dir[i]='/';
        }
    }
    return 0;
}

int getoneline(const char *path, char *buf, int size)
{
    if (buf == NULL || size <= 0) {
        db_error("buf == NULL or size <= 0");
        return -1;
    }

    if (!FILE_EXIST(path) || !FILE_READABLE(path)) {
        return -1;
    }

    FILE *fp = NULL;

    fp = fopen(path, "r");

    if (fp == NULL) {
        db_error("open file failed, %s", strerror(errno));
        return -1;
    }

    if (fgets(buf, size, fp) != NULL) {
        buf[size-1] = '\0';
    }

    fclose(fp);

    return 0;
}

uint32_t calc_record_time_by_size(uint32_t bitrate, uint32_t size)
{
	uint32_t onehour_size, rec_time;
	float rate = 0.0f;

    rate = bitrate / (8.0f * 1024 * 1024);

	onehour_size = (uint32_t)(rate*3600);

	if (onehour_size < MAX_ONE_FILE_SIZE) {
		rec_time = 60*60;
	} else {
		rec_time = (int)MAX_ONE_FILE_SIZE/rate;
	}

    db_debug("bitrate: %u, size: %u, rec_time: %u", bitrate, size, rec_time);

	return rec_time;
}

int64_t calc_record_size_by_time(uint32_t bitrate, uint32_t ms)
{
    int64_t size;

    size = (ms / 1000) * (bitrate >> 3);

    db_debug("bitrate: %u, time: %u, size: %lld", bitrate, ms, size);

    // 预分配10%余量, 防止实际写入大小超过计算的预分配大小
#if 1
    int redundancy_size = size * 0.1;
    if (redundancy_size < (10 * (1 << 20)))
        redundancy_size  = 10 * (1 << 20);
    size += redundancy_size;
#endif
    return size;
}

int get_cmd_result(const char *cmd, char *buf, uint16_t size)
{
    FILE *fp = NULL;

    fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen failed");
        pclose(fp);
        return -errno;
    }

    while(fgets(buf, size, fp) != NULL) {
        buf[strlen(buf) - 1]= '\0';
    }

    pclose(fp);

    return 0;
}

bool verify_bin_md5()
{
    char md5_1[128] = {0};
    char md5_2[128] = {0};
    char cmd[128] = {0};

    if (access("/mnt/extsd/.sdvcam", F_OK) != 0) {
        db_info("run application from rootfs, no need verify md5, just check build version");
        return true;
    }

    snprintf(cmd, sizeof(cmd), "md5sum %s | awk '{print $1}'", SDVCAM_BIN_1);
    if (get_cmd_result(cmd, md5_1, sizeof(md5_1)) != 0) {
        db_warn("verify bin md5 failed, %s", cmd);
        return false;
    }
    db_info("'%s', md5: %s", SDVCAM_BIN_1, md5_1);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "md5sum %s | awk '{print $1}'", SDVCAM_BIN_2);
    if (get_cmd_result(cmd, md5_2, sizeof(md5_2)) != 0) {
        db_warn("verify bin md5 failed, %s", cmd);
        return false;
    }
    db_info("'%s', md5: %s", SDVCAM_BIN_2, md5_2);

    if (strcmp(md5_1, md5_2) != 0) {
        db_warn("md5 check failed");
        return false;
    }

    return true;
}
/*属性*/
enum
{
    EVATTR_NONE      = 0,
    EVATTR_RO        = 1,
    EVATTR_HIDDEN    = 2,
    EVATTR_SYS       = 4,
    EVATTR_VOLUME    = 8,
    EVATTR_DIR       = 16,
    EVATTR_ARCH      = 32,
};

int HideFile(const char *filename, bool flag)
{
	int ret = -1;
	if ((filename!=NULL) && (access(filename,F_OK)==0)) {
		int fd = open(filename,O_RDWR); 
		unsigned int attrx=0;
		if (fd >= 0) {
			if (ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &attrx)<0) {
				close(fd);
				return -3;
			}
			if (!flag) {
				attrx &= (~EVATTR_HIDDEN);
			} else {
				attrx |= ( EVATTR_HIDDEN);
			}
			if (ioctl(fd, FAT_IOCTL_SET_ATTRIBUTES, &attrx)<0) {
				close(fd);
				return -4;
			}
			close(fd);
			return 0;
		}
		ret = -2;
	}
	return ret;
}
int ispsave_file_filter(const struct dirent* dir)
{
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    stat(dir->d_name, &buf);
    if (!S_ISDIR(buf.st_mode))
	{	// isp0_3840_2160_25_ctx_saved.bin
		if (fnmatch("isp*ctx_saved.bin", dir->d_name, FNM_CASEFOLD) == 0) {
			return 1;
		}
    }
    return 0;
}

int HideIspSaveFile(void)
{
	struct dirent **namelist = NULL;
	int ret;
	const char *p_scanPath = "/mnt/extsd/";
	ret = scandir(p_scanPath, &namelist, ispsave_file_filter, alphasort);
	for (int i = 0; i < ret; i++){
		db_warn("namelist[%d] %s",i,namelist[i]); 
		char filepath[128];
		sprintf(filepath,"%s%s",p_scanPath,namelist[i]->d_name);
		HideFile(filepath,1);
	}
}


