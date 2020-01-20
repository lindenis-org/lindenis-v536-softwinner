#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define TAG "keytester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_key"

static int fifoFd;
static int g_key_code_1 = -3;
static int g_key_val_1  = -3;
static int g_key_code_2 = -3;
static int g_key_val_2  = -3;
static int g_key_code_3 = -3;
static int g_key_val_3  = -3;


void *KeyFunction1(void *argv)
{
    struct input_event buf;
    char str[50] = {0};
    int fd = 0;
    fd = open( "/dev/input/event2", O_RDONLY);

    printf("scan key input event....\r\n");
    while(1)
    {
        read(fd, &buf, sizeof(buf));
//      printf("time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
        sprintf(str, "P[KEY] (%d, %d)", buf.code, buf.value);
        //if((buf.code == 102)||(buf.code == 114)||(buf.code == 115)||(buf.code == 28)||(buf.code == 139))
        if(buf.code > 10 && buf.code < 255)
        {
            g_key_code_1 = buf.code;
            g_key_val_1  = buf.value;
        }
        sleep(2);
    }
    printf("g_key_code_1 %d,g_key_val_1 %d\r\n",g_key_code_1,g_key_val_1);
    close(fd);

    return NULL;
}

void *KeyFunction2(void *argv)
{
    struct input_event buf;
    char str[50] = {0};

    int fd = open( "/dev/input/event2", O_RDONLY);

    if(fd < 0)
    {
//      write(fifoFd, "F[KEY] FAIL", 50);
    }

    while(1)
    {
        read(fd, &buf, sizeof(buf));
//      printf("time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
//      printf("KeyFunction2 time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
        sprintf(str, "P[KEY] (%d, %d)", buf.code, buf.value);
        //if((buf.code == 116))
        if(buf.code > 10 && buf.code < 255)
        {
            #if 0
            if (g_key_code_2 < 0) {
                sprintf(str, "P[KEY] val(%d-%d)", buf.code, buf.value);
                write(fifoFd, str, 50);
            }
            #endif
            g_key_code_2 = buf.code;
            g_key_val_2  = buf.value;
        }
        sleep(2);
    }
    close(fd);

    return NULL;
}

void *KeyFunction3(void *argv)
{
    struct input_event buf;
    char str[50] = {0};

    int fd = open( "/dev/input/event1", O_RDONLY);

    if(fd < 0)
    {
//      write(fifoFd, "F[KEY] FAIL", 50);
    }

    while(1)
    {
        read(fd, &buf, sizeof(buf));
//      printf("time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
//        printf(" KeyFunction3 time %u:%u type:%d code:%d value:%d\n",buf.time.tv_sec,buf.time.tv_usec,buf.type,buf.code,buf.value);
        sprintf(str, "P[KEY] (%d, %d)", buf.code, buf.value);
        //if((buf.code == 116))
        if(buf.code > 10 && buf.code < 255)
        {
            #if 0
            if (g_key_code_2 < 0) {
                sprintf(str, "P[KEY] val(%d-%d)", buf.code, buf.value);
                write(fifoFd, str, 50);
            }
            #endif
            g_key_code_2 = buf.code;
            g_key_val_2  = buf.value;
        }
        sleep(2);
    }
    close(fd);

    return NULL;
}

int main(int argc, char **argv)
{
    char str[50] = {0};
    char str_pass[] = "P[KEY] PASS";
    char str_fail[] = "F[KEY]:FAIL";
#if 1
    if ((fifoFd = open(FIFO_KEY_DEV, O_WRONLY)) < 0)
    {
        if (mkfifo(FIFO_KEY_DEV, 0666) < 0)
        {
            printf("mkfifo failed(%s)\n", strerror(errno));
            return -1;
        }
        else
        {
            fifoFd = open(FIFO_KEY_DEV, O_WRONLY);
        }
    }
#endif
    pthread_t KeyThreadID1, keyThreadID2,keyThreadID3;
    printf("run keytest\r\n");
    pthread_create(&KeyThreadID1, NULL, KeyFunction1, NULL); //ok_key
#if 0
    pthread_create(&keyThreadID2, NULL, KeyFunction2, NULL); //power_key
    pthread_create(&keyThreadID3, NULL, KeyFunction3, NULL); //up_down_key
#endif
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
    printf("keytester pass\r\n");
    return 0;
}
