/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file udev_handler.c
 * @author id:826
 * @date 2016-5-4
 * @version v0.3
 * @brief udev handler，用于响应udev的ACTION
 * @verbatim
 *  History:
 * @endverbatim
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mount.h>
#include <fcntl.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "udev_message.h"

#define UNIX_DOMAIN "/tmp/event/unix.domain"
#define SEND_BUFFER_SIZE 8

extern char **environ;
int getInt(char *const *argv, int ret, int type);

static int send_to_server(char msg[], int count)
{
    int connect_fd;
    int ret;
    char send_buf[SEND_BUFFER_SIZE];
    struct sockaddr_un s_addr;

    bzero(send_buf, sizeof(send_buf));

    connect_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect_fd < 0) {
        perror("create connect socket failed");
        return -1;
    }

    /* set to no-block */
    int flags;
    if ( (flags = fcntl(connect_fd, F_GETFL, 0)) < 0 ) {
        perror("F_GETFL failed");
    }

//    if ( (fcntl(connect_fd, F_SETFL, flags | O_NONBLOCK)) < 0 ) {
//        perror("F_SETFL failed");
//    }

    s_addr.sun_family = AF_UNIX;
    strncpy(s_addr.sun_path, UNIX_DOMAIN, sizeof(s_addr.sun_path) - 1);

    ret = connect(connect_fd, (struct sockaddr*)&s_addr, sizeof(s_addr));
    if (ret < 0) {
        perror("connect to server failed");
        close(connect_fd);
        return -1;
    }

    int i;
    for(i = 0; i < count; i++) {
        printf("msg[%d]:0x%02x\n", i, msg[i]);
        strncpy(send_buf, &msg[i], sizeof(send_buf));
        write(connect_fd, send_buf, sizeof(send_buf));
    }

    close(connect_fd);
    return 0;
}

static int do_mount(const char *mount_point, const char *dev, const char *fs_type)
{
    int ret;
    assert(mount_point != NULL);
    assert(dev != NULL);

    FILE *fp = NULL;
    int try=0;

    if (!fs_type)
	return -EINVAL;

again:
    ret = mount(dev, mount_point, fs_type, 0, "errors=continue");
    if (ret < 0) {
	    printf("mount failed ret: %d, errno: %d, %s\n", ret, errno, strerror(errno));
	    usleep(500*1000);
	    if ((errno == EBUSY) && !try) {
		try = 1;
		goto again;
	    }
	    return -errno;
    }

    return 0;
}

static int do_umount(const char *mount_point)
{
    int ret;

    assert(mount_point != NULL);
    ret = umount2(mount_point, MNT_FORCE | MNT_DETACH);
    if (ret < 0) {
        perror("umount failed");
        return -errno;
    }
    return 0;
}

static int make_dir(const char *dirpath)
{
    struct stat st;

    if (stat(dirpath, &st) < 0) {
        if (mkdir(dirpath, 0755) < 0) {
            printf("mkdir '%s' failed: %s\n", dirpath, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static int storage_mount(int type, const char *argv)
{
    int ret = 0;
    char *fst = NULL;
    char *e;

    /*
     * parsing the environ, get ID_FS_TYPE & NPARTS.
     * Here, we do not support the multi partitions,
     * and only support the vfat filesystem type.
     *
     * If the sdcard can not match, we need notify the
     * application.
     */
    e = getenv("ID_FS_TYPE");
    if ((e && strncmp("vfat", e, 4)) || !e) {
	    //return UDEV_FS_ERROR;
		return UDEV_MOUNTED;
    } else {
	    fst = strdup(e);
    }
   #if 0
    if (type == SDCARD) {
	    e = getenv("NPARTS");
	    if ((e && strtoul(e, NULL, 10)) || !e){
            printf("[debug_jaosn]: NPARTS is mutile 33");
		    goto out;
	    }
    }
  #endif
    if (type == SDCARD) {
        make_dir("/mnt/extsd");
        ret = do_mount("/mnt/extsd", argv, fst);
    } else if (type == UDISK) {
        make_dir("/mnt/udisk");
        ret = do_mount("/mnt/udisk", argv, fst);
    }

out:
    if (fst)
	free(fst);

    if (ret == -EINVAL) {
        return UDEV_FS_ERROR;
    }

    return (ret == 0)?UDEV_MOUNTED:UKNOWN_ERROR;
}

static int storage_umount(int type)
{
    int ret = 0;

    if (type == SDCARD) {
        ret = do_umount("/mnt/extsd");
    } else if (type == UDISK) {
        ret = do_umount("/mnt/udisk");
    }

    return (ret == 0)?UDEV_UMOUNT:UKNOWN_ERROR;
}


/**
 * argv[1] - action
 * argv[2] - type
 * argv[3] - dev node
 *
 * udev will set properties in environ variable.
 * Such as: NPARTS, ID_FS_TYPE
 *
 */
int main(int argc, char *argv[])
{
    int type = 0, status = 0;
    char msg[2] = {0};

    if (argc < 3 || argc > 4) {
        printf("Usage: %s [type] [action]\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[2], "sd") == 0) type = SDCARD;
    else if (strcmp(argv[2], "udisk") == 0) type = UDISK;
    else if (strcmp(argv[2], "hdmi")  == 0) type = HDMI;
    else if (strcmp(argv[2], "cvbs")  == 0) type = CVBS;
    else if (strcmp(argv[2], "usb2host") == 0) type = USB2HOST;
    else if (strcmp(argv[2], "tp9950") == 0) type = TP9950;

    /* handle plug message */
    if (strcmp(argv[1], "add") == 0) {
        system("/bin/rm -rf /overlay/upperdir/mnt");
        sync();
        status = UDEV_INSERT;
        msg[0] = type|status;
        send_to_server(msg, 1);

        status = storage_mount(type, argv[3]);

    } else if (strcmp(argv[1], "remove") == 0) {
        system("/bin/rm -rf /overlay/upperdir/mnt");
        sync();
        status = UDEV_REMOVE;
        msg[0] = type|status;
        send_to_server(msg, 1);

        status = storage_umount(type);

    } else {
        type = UKNOWN_TYPE;
        status = UKNOWN_ERROR;
    }

out:
    // send extra message
    msg[0] = type|status;
    send_to_server(msg, 1);

    return 0;
}

