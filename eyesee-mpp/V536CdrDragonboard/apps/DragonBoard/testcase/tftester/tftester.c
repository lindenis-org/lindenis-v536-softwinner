// judge tf card by detect file /dev/block/mmcblk0 not by /mnt/extsd

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#define TAG "tftester"
#include <dragonboard/dragonboard.h>

int main(int argc, char **argv)
{
	int fifoFd = 0, tfFd = -1;
    char str_pass[] = "P[TF] PASS";
    char str_fail[] = "F[TF]:FAIL";
	if ((fifoFd = open(FIFO_TF_DEV, O_WRONLY)) < 0)
	{ 
		if (mkfifo(FIFO_TF_DEV, 0666) < 0)
		{
			printf("mkfifo failed(%s)\n", strerror(errno));
			return -1;
		}
		else
		{
			fifoFd = open(FIFO_TF_DEV, O_WRONLY);
		}
	}
	while (1)
	{
		
		tfFd = open("/dev/mmcblk0", O_RDWR);
		if (tfFd < 0)
		{
			write(fifoFd,str_fail, strlen(str_fail));
		}
		else
		{
			int BufCount = write(tfFd, str_pass, strlen(str_pass));
			if( BufCount > 0 )
			    write(fifoFd,str_pass, strlen(str_pass));
			else
			    write(fifoFd,str_fail, strlen(str_fail));
			close(tfFd);
			tfFd = -1;
		}
		sleep(1);
	}

	return 0;
}
