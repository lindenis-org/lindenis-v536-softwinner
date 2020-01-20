// judge tf card by detect file /dev/block/mmcblk0 not by /mnt/extsd

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define TAG "usbtester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_usb"

int main(int argc, char **argv)
{
    int fifoFd = 0, usbFd = 0;


	printf("============USB START============\n");

    // this part works well since then, so no need to notify server if open fifo failed
    // if we want to notify server, use other ipc tools, such as message, shared memory
#if 1
	if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {      // fifo's write-endian block until read-endian open
        if (mkfifo(FIFO_DEV, 0666) < 0) {
            db_error("mkfifo failed(%s)\n", strerror(errno));
            return -1;
        } else {
            fifoFd = open(FIFO_DEV, O_WRONLY);
        }
    }
#endif
	printf("[usb] shart open /dev/sda\n");

    while (1) {
        if ((usbFd = open("/dev/sda", O_RDONLY)) < 0) {
			//printf("[usb] open /dev/sda fai\n");
            write(fifoFd, "F[USB] fail", 30);
			close(usbFd);
        } else {
           write(fifoFd, "P[USB] PASS", 30);
  			//printf("[usb] open /dev/sda ok\n");
            close(usbFd);
        }
        sleep(5);         // careful! detect TF card every 20ms
    }
   close(fifoFd);

    return 0;
}
