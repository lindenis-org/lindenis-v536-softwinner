// judge tf card by detect file /dev/block/mmcblk0 not by /mnt/extsd

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define TAG "tftester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_tf"

int main(int argc, char **argv)
{
	int fifoFd = 0, tfFd = -1;
	 printf("----fuck--------1\n");
    if (mkfifo(FIFO_DEV, 0666) < 0)
    {
         printf("mkfifo file\n");
         return -1;
    }
    printf("----fuck--------2\n");
    fifoFd = open(FIFO_DEV, O_WRONLY);
    if(fifoFd < 0)
    {
        printf("open fifo failed!!! %s",strerror(errno));
    }
    printf("----------------3\n");
	while (1)
	{
		
		tfFd = open("/dev/mmcblk0", O_RDWR);
		if (tfFd < 0)
		{
			write(fifoFd, "F[TF] FAIL", 11);
			printf("write failed");
		}
		else
		{
			int BufCount = write(tfFd, "miljie", 7);
			if( BufCount > 0 )
				write(fifoFd, "P[TF] PASS", 11);
			else
				write(fifoFd,  "F[TF] FAIL", 11);
			close(tfFd);
			tfFd = -1;
		}
		sleep(1);
	}

	return 0;
	}
