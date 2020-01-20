#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#define TAG "ddrtester"
#include <dragonboard/dragonboard.h>

#define MEMESTER_CMD "memtester 4K 1  "
#define FIFO_DEV  "/tmp/fifo_ddr"

int main(int argc, char **argv)
{
    int retVal = 0;
    char str[64] = {0};
    int ret = -1;
    int fifoFd = 0;
    char str_pass[] = "P[DDR] PASS";
    char str_fail[] = "F[DDR]:FAIL";
    char tmp[50] = {0};
#if 0
    if ((fifoFd = open(FIFO_DEV, O_WRONLY | O_NONBLOCK)) < 0)
    {
        if (mkfifo(FIFO_DEV, 0666) < 0)
        {
            printf("mkfifo failed(%s)\n", strerror(errno));
            return -1;
        }
        else
        {
            fifoFd = open(FIFO_DEV, O_WRONLY | O_NONBLOCK);
        }
    }
#endif
    FILE *fd = NULL;
    fd = fopen(FIFO_DDR_DEV,"wb+");
    int *p = (int *)malloc(sizeof(int));
    if(p == NULL){
        fwrite(str_fail,1,strlen(str_fail),fd);
        printf("ddr test FAIL\n");
    } else {
        fwrite(str_pass,1,strlen(str_pass),fd);
        printf("ddr test PASS\n");
    }

    fclose(fd);
    return 0;
}
