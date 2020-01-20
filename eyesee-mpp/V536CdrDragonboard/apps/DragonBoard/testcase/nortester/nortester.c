#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#define TAG "nortester"
#include <dragonboard/dragonboard.h>
#include <dragonboard/mtd-abi.h>

#define FIFO_DEV  "/tmp/fifo_nor"
#define NOR_FLASH "/dev/mtdblock0"
//#define NOR_FLASH "/dev/mtd0"
/**********file utils define***************/
#define FILE_EXIST(PATH)   (access(PATH, F_OK) == 0)

int main(int argc, char **argv)
{
    int fd;
    int ret = 0;
    char str[128] = {0};
    char str_pass[] = "P[NOR] PASS";
    char str_fail[] = "F[NOR]:FAIL";
    sleep(2);
    fd = open(FIFO_NOR_DEV, O_WRONLY | O_CREAT);

    if(FILE_EXIST(NOR_FLASH)){
        db_error("/dev/mtdblock0 exit");
        write(fd,str_pass,strlen(str_pass));
    }else{
        db_error("/dev/mtdblock0 no exit");
        write(fd,str_fail,strlen(str_fail));
    }


#if 0
    int nor_fd = open(NOR_FLASH,O_RDONLY);
    if(nor_fd < 0)
    {
        db_error("open mtd block failed");
        write(fd,str_fail,strlen(str_fail));
        close(fd);
        return -1;
    }
    struct mtd_info_user mtd_info_msg;
    memset(&mtd_info_msg, 0, sizeof(mtd_info_user));
    mtd_info_msg.size = 0;
    ret = ioctl(nor_fd, MEMGETINFO,&mtd_info_msg);
    if(ret < 0)
    {
        db_error("ioctl mtd block failed.");
    }
    mtd_info_msg.size = mtd_info_msg.size / (1 << 20);
    db_error("mtd_info_msg.size %ld",mtd_info_msg.size);

    while (1) {
#ifdef FOX_PRO
        sprintf(str, "P[NOR] PASS %dM",mtd_info_msg.size);
#else
        sprintf(str, "P[NOR] PASS");
#endif
        write(fd,str,strlen(str));
        sleep(2);
        break;
    }
    close(fd);

#endif
    return 0;
}
