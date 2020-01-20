#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define TAG "tptester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_tp"
#define TP_NODE   "/dev/input/event3"

int main(int argc, char **argv)
{
    int fifoFd;

    // this part works well since then, so no need to notify server if open fifo failed
    // if we want to notify server, use other ipc tools, such as message, shared memory
    if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {      // fifo's write-endian block until read-endian open
        if (mkfifo(FIFO_DEV, 0666) < 0) {
            db_error("mkfifo failed(%s)\n", strerror(errno));
            return -1;
        } else {
            fifoFd = open(FIFO_DEV, O_WRONLY);
        }
    }

    write(fifoFd, "W[TP] waiting.", 15);
    usleep(1000);

    int fd, xp, yp;
    struct input_event buf;
    char str[32];

    fd = open(TP_NODE, O_RDONLY);
	
	if(fd < 0)
	{
		write(fifoFd, "F[TP] FAIL", 50);
	}

    while(1) {
        read(fd, &buf, sizeof(buf));
  //      printf("time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
        if (buf.type == 3 && buf.code == 53) {
            xp = buf.value;
            read(fd, &buf, sizeof(buf));
            if (buf.type == 3 && buf.code == 54) {
                yp = buf.value;
                sprintf(str, "P[TP] (%d, %d)", xp, yp);
                write(fifoFd, str, 32);
            }
        }
    }
    close(fd);
    close(fifoFd);

    return 0;
}
