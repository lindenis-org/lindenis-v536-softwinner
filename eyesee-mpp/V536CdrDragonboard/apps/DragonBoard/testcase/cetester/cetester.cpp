#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



#include <openssl/engine.h>
#include <openssl/sha.h>
#include "crypto/aw_cipher.h"
#include "ss_test.h"


#define TAG "cetester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_ce"

//#define SS_TEST_BUF_SIZE    (8*1024)

#define CE_TEST_PREFIX "CE_SHA:"
char g_buf[SS_TEST_BUF_SIZE] = {0};
#define SHA_METHOD_ITEM 8
#define SHA_METHOD_ITEM_NAME 10

#define DBG(string, args...) \
    do { \
        printf("%s, %u - ", __FILE__, __LINE__); \
        printf(string, ##args); \
   } while (0)
#define ENGINE_load_builtin_engines ENGINE_load_af_alg

unsigned char g_sha_methed[SHA_METHOD_ITEM][SHA_METHOD_ITEM_NAME] =
{
//	"md5",
//	"sha1",
	"sha224",
//	"sha256",
//	"sha384",
//	"sha512",
	""
};
char sha1[1024]={0x73,0xe1,0x8e,0xf5,0x20,0x7a,0x09,0x1a,0xf8,
				 0x14,0xd2,0x24,0x80,0x43,0x3b,0xb2,0x72,0x0e,
				 0x3d,0xc2,0x9d,0xd7,0x05,0x37,0x23,0x48,0x0a,0x8f};


int cmp_md(unsigned char *md, int len)
{
	int i;
	for (i=0; i<len; i++)
	{
		if(sha1[i]!= md[i])
		{
			printf("sha1:%02x,md:%02x", sha1[i],md[i]);
			printf("\n");
			return -1;
		}
		printf("%02x", md[i]);

	}
	printf("\n");
	return 0;
}

void print_md(unsigned char *md, int len)
{
	int i;
	for (i=0; i<len; i++)
		printf("%02x", md[i]);
	printf("\n");
}



ENGINE *openssl_engine_init(char *type)
{
	ENGINE *e = NULL;
	const char *name = "af_alg";
	OpenSSL_add_all_algorithms();
	ENGINE_load_builtin_engines();

	e = ENGINE_by_id(name);
	if (!e) {
		DBG("find engine %s error\n", name);
		return NULL;
	}

	ENGINE_ctrl_cmd_string(e, "DIGESTS", type, 0);
	return e;
}


static void openssl_engine_free(ENGINE *e)
{
	if (e != NULL)
		ENGINE_free(e);

	ENGINE_cleanup();
	EVP_cleanup();
}

static const char g_test_string[17] = "0123456789abcdef";

int main(int argc, char *argv[])
{
    int ret = 0;
	int nid = NID_sha256;
	ENGINE *e = NULL;
	int hash_len = 32;
	EVP_MD_CTX ctx = {0};
	const EVP_MD *e_md = NULL;
	unsigned int md_size = 0;
	unsigned char md[128] = {0};
	char* pt[SHA_METHOD_ITEM];
	unsigned int i = 1;
	size_t wt_len;

	int retVal = 0;
	char str[64] = {0};
	int fifoFd = 0;
	char tmp[50] = {0};


//	system("ln -s /usr/lib/libaf_alg_aw_v31.so /usr/lib/libaf_alg.so");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/af_alg.ko");
	sleep(1);
	system("insmod /lib/modules/4.4.55/algif_hash.ko");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/algif_skcipher.ko");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/algif_rng.ko");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/sha256_generic.ko");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/ss.ko");

	printf("==============open %s==================\n",FIFO_DEV);
#if 1
	if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0)
	{
	printf("====open %s error :%s====\n",FIFO_DEV ,strerror(errno));

		if (mkfifo(FIFO_DEV, 0666) < 0)
		{
		    printf("mkfifo failed(%s)\n", strerror(errno));

		} else
		{
		   fifoFd = open(FIFO_DEV, O_WRONLY);
		}
	}
#endif

	for(i = 0; i < sizeof(g_sha_methed)/sizeof(g_sha_methed[0]); i++)
	{
		pt[i] = (char*)g_sha_methed[i];
		printf("\n%s%s test begin:\n",CE_TEST_PREFIX,pt[i]);
		if(strlen(pt[i]) == 0)
			{
				ret = 0;
				break;
			}
			e = openssl_engine_init(pt[i]);
			if (e == NULL) {
				ret = -1;
				break;
			}
			if (strcmp(pt[i], "sha1") == 0)
			{
				nid = NID_sha1;
				hash_len = 20;
			}
			else if (strcmp(pt[i], "md5") == 0)
			{
				nid = NID_md5;
				hash_len = 16;
			}
			else if (strcmp(pt[i], "sha224") == 0)
			{
				nid = NID_sha224;
				hash_len = 28;
			}
			else if (strcmp(pt[i], "sha384") == 0)
			{
				nid = NID_sha384;
				hash_len = 48;
			}
			else if (strcmp(pt[i], "sha512") == 0)
			{
				nid = NID_sha512;
				hash_len = 64;
			}

		e_md = ENGINE_get_digest(e, nid);
		if (e_md == NULL) {
			DBG("ENGINE_get_digest() failed! e_af_alg not support \n");
			ret = 0;
			break;
		}

		EVP_DigestInit(&ctx, e_md);
		EVP_DigestUpdate(&ctx, g_test_string, sizeof(g_test_string));
		EVP_DigestFinal(&ctx, md, &md_size);


		if(NID_sha224 == nid)
		{
			if(cmp_md(md, hash_len))
			{
				printf("cmp sha224 fail \n");
				sprintf(tmp, "F[ce]:FAIL");
				write(fifoFd, tmp, 50);

			}
			else
			{
				printf("cmp sha224 ok \n");
				sprintf(tmp, "P[CE]:PASS sha224");
				write(fifoFd, tmp, 50);
			}
		}

		print_md(md, hash_len);
		EVP_MD_CTX_cleanup(&ctx);
		openssl_engine_free(e);
		printf("\n%s%s test end:\n",CE_TEST_PREFIX,pt[i]);
	}
	printf("\n%s test end result:%d\n",CE_TEST_PREFIX,ret);

	//close(fifoFd);
    return ret;
}

