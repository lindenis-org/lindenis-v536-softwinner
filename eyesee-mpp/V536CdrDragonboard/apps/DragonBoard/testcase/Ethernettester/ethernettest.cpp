#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <iostream>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <unistd.h>
#include <stdlib.h>

//#include <iostream.h>


#define TAG "ethernettest"
#include <dragonboard/dragonboard.h>

#define MEMESTER_CMD "/mnt/extsd/DragonBoard/memtester 128M 1"
#define FIFO_DEV  "/tmp/fifo_ethernet"
#define PATH_PROCNET_ROUTE        "/proc/net/route"
#define NET_DEVICE_NAME           20
#define CMD_KILL_DHCPC            "killall udhcpc"
#define CMD_KILL_DHCPD            "killall udhcpd"


using namespace std;

int DisableAllNetLink(void)
{
	system(CMD_KILL_DHCPC);
	system(CMD_KILL_DHCPD);
	return 0;
}

int GetNetDevGateWay(const std::string &netdev_name, unsigned int &gateway)
{
    int  ret         = 0;
    char *pret       = NULL;
    char devname[64] = {0};
    char flags[16]   = {0};
    int  flgs, ref, use, metric, mtu, win, ir;
    unsigned long dst, gw, mask;
    FILE *fp = NULL;

    gateway = 0;

    fp = fopen(PATH_PROCNET_ROUTE, "r");
    if (NULL == fp) {
        printf("[FUN]:%s  [LINE]:%d  Open file:%s fail! errno[%d] errinfo[%s]\n", __func__, __LINE__,
                PATH_PROCNET_ROUTE, errno, strerror(errno));
    }

    /* Skip the first line. */
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        /* Empty or missing line, or read error. */
        printf("[FUN]:%s  [LINE]:%d  Open file:%s fail! errno[%d] errinfo[%s]\n", __func__, __LINE__,
                PATH_PROCNET_ROUTE, errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    while (1) {
        ret = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &dst, &gw, &flgs, &ref, &use, &metric, &mask,
                   &mtu, &win, &ir);
        if (ret != 11) {
            if ((ret < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
                ret = -1;
                printf("[FUN]:%s  [LINE]:%d  Don't find NetDev:%s gateway!\n", __func__, __LINE__, netdev_name.c_str());
                break;
            }
        }

        if (!(flgs & RTF_UP)) { /* Skip interfaces that are down. */
            continue;
        }

        ret = strncmp(netdev_name.c_str(), devname, NET_DEVICE_NAME-1);
        if (0 == ret && 0 != gw) {
            gateway = ntohl(gw);
            ret     = 0;
            break;
        } else {
            continue;
        }
    }

    fclose(fp);
    return ret;
}


int GetNetDevGateWay(const std::string &netdev_name, std::string &gateway)
{
    int            ret = 0;
    unsigned int   u3gateway;
    struct in_addr sin_addr;

    ret = GetNetDevGateWay(netdev_name, u3gateway);
    if (ret) {
        printf("[FUN]:%s  [LINE]:%d  Do GetNetDevGateWay error! ret:%d  \n",__func__,__LINE__, ret);
        return -1;
    }
    sin_addr.s_addr = htonl(u3gateway);
    gateway         = inet_ntoa(sin_addr);
    return 0;
}




int main(int argc, char **argv)
{
	int i = 0;
    int retVal = 0;
	int fifoFd = 0;
	std::string str;
	char tmp[50] = {0};
	char netdev_name[64] ="eth0";
	char netgatway[50]={0};
	char ping_log[20] = "/tmp/ping.log";
	int ping_fd  = 0;
	char ping_text[1024]={0};
	char *ms_p = NULL;
	char show_str[15]={0};

	printf(" mkfifo start\n");

//	sleep(1); //wait wifi close

#if 1

	if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0)
	{
		printf(" open %s fail\n",FIFO_DEV);
		if (mkfifo(FIFO_DEV, 0666) < 0)
		{
		    printf("mkfifo failed(%s)\n", strerror(errno));

		} else {

			printf(" mkfifo creat again\n");
		    fifoFd = open(FIFO_DEV, O_WRONLY);
		}
	}
#endif
	printf(" get route gateway start\n");

	DisableAllNetLink();
	usleep(20 * 1000);

	system("/sbin/udhcpc -i eth0 -b -R");

	sleep(2);

	retVal = GetNetDevGateWay(netdev_name, str);

	if(retVal)
	{
		 write(fifoFd, "F[Fail] get gateway fail", 30);
		 printf("error GetNetDevGateWay\n");
		// return 0;
	}

	if(!strcmp("127.0.0.1",netgatway))
	{
		// write(fifoFd, "W[waiting] not get route gateway ", 30);
		 printf("enot get route gateway\n");
		// return 0;
	}

    strcpy(netgatway, str.c_str());

	sprintf(tmp, "ping %s > %s &",netgatway,ping_log);

	printf("tmp is %s, \n", tmp);

	retVal = system(tmp);

	sleep(2);

	system("killall ping");

	retVal = access(ping_log, F_OK);

    if(retVal != 0)
    {
       printf("ping.log is exit!\n");

	 //   write(fifoFd, "F[Fail] fail.ping.log is exit", 30);
	   //return 0;

    }

	if ((ping_fd = open(ping_log, O_RDONLY)))
	{
		 write(fifoFd, "F[Fail] open fail", 30);
		printf("error open %s\n",ping_log);
	}

	read(ping_fd, ping_text, 1024);

	ms_p = strstr(ping_text, "time=");

	for(i = 0;i < 5; i++)
	{
		if(ms_p)
		{
			strncpy(show_str,ms_p,13);
			sprintf(tmp, "P[Ethernet]:PASS %s",show_str);
		    printf("----tmp--:%s\n",tmp);
			write(fifoFd, tmp, 50);
		}
		else
		{
			 write(fifoFd, "F[Ethernet]", 30);
			 printf("----ping_text--:%s\n",ping_text);
		}
		sleep(1);
	}
	
	close(ping_fd);
    return 0;
}
