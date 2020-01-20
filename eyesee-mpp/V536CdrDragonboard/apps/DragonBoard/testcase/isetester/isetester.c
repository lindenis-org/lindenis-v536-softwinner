// ise_bi.c : 双目鱼眼用户态主程序
// 本工程用于调试双目鱼眼驱动
// 为接口函数添加句柄并加锁，实现驱动的多实例需求
// 改进内部库的封装，将NBBuffer/LUT_ROIP_Buffer
// LUTBuffer/ROIPBuffer数据封装至handle内部，实现不同实例间的数据隔离


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "ISE_lib_bi.h"
#include "ISE_proc_bi.h"

#define TAG "lsetester"
#include <dragonboard/dragonboard.h>

#define FIFO_DEV  "/tmp/fifo_ise"


#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#define TEST_TIMES          1           		// 测试次数
#define NUM_FRAM			1					// 处理图像数量

#define YUV_SOURCE_IMG "/usr/share/ISE/fisheye_asia_320.yuv420"

// 编码输入分辨率h及w配置为4的倍数，stride为32倍数，空间按32倍数留
#define TEST_SRC_HEIGHT					320
#define TEST_SRC_WIDTH					320
#define TEST_SRC_STRIDE					320     //luma&chroma 输入stride相等


// 输出分辨率w配置为8的倍数，h为4的倍数，stride为16或32倍数，空间按16或32倍数留
// 当输出plane模式时，CHROMA_STRIDE需要除2
#define TEST_PANO_HEIGHT				320
#define TEST_PANO_WIDTH					640
#define TEST_PANO_LUMA_STRIDE			640
#define TEST_PANO_CHROMA_STRIDE			640
#define TEST_PANO_FLIP					FALSE
#define TEST_PANO_MIRROR				FALSE

#define TEST_SCALAR_EN_CHN0             FALSE
#define TEST_SCALAR_HEIGHT0				160
#define TEST_SCALAR_WIDTH0				320
#define TEST_SCALAR_LUMA_STRIDE0		320
#define TEST_SCALAR_CHROMA_STRIDE0		320
#define TEST_SCALAR_FLIP0				FALSE
#define TEST_SCALAR_MIRROR0				FALSE


// chn1
#define TEST_SCALAR_EN_CHN1             FALSE
#define TEST_SCALAR_HEIGHT1				80
#define TEST_SCALAR_WIDTH1				160
#define TEST_SCALAR_LUMA_STRIDE1		160
#define TEST_SCALAR_CHROMA_STRIDE1		160
#define TEST_SCALAR_FLIP1				FALSE
#define TEST_SCALAR_MIRROR1				TRUE


// chn2
#define TEST_SCALAR_EN_CHN2             FALSE
#define TEST_SCALAR_HEIGHT2				40
#define TEST_SCALAR_WIDTH2				80
#define TEST_SCALAR_LUMA_STRIDE2		80
#define TEST_SCALAR_CHROMA_STRIDE2		80
#define TEST_SCALAR_FLIP2				FALSE
#define TEST_SCALAR_MIRROR2				FALSE

#define TEST_IN_YUV_TYPE     			YUV420
#define TEST_PANO_YUV_TYPE   			YUV420

// 相机参数
#define TEST_P0         (TEST_SRC_HEIGHT/3.1415)
#define TEST_CX0		(TEST_SRC_WIDTH/2)
#define TEST_CY0		(TEST_SRC_HEIGHT/2)

#define TEST_P1         (TEST_SRC_HEIGHT/3.1415)
#define TEST_CX1		(TEST_SRC_WIDTH/2)
#define TEST_CY1		(TEST_SRC_HEIGHT/2)


int main( )
{
	/**********************variable************************/
	ISE_HANDLE_BI 			*handle0 = NULL;
	FILE*					fp_src;

	char					filename[128];
	int 					i, j;
	int 					malloc_ret = 0;

	ISE_CFG_PARA_BI 		ise_cfg;
	ISE_PROCIN_PARA_BI		ise_procin;
	ISE_PROCOUT_PARA_BI 	ise_procout;
	unsigned int			block_size, n_readsize;
	unsigned int			hresult = -1;

	/* 源数据存放结构体 */
	static myAddr Y_Src_Buffer[2];
	static myAddr C_Src_Buffer[2];

	/* 硬件计算数据存放结构体 */
	static myAddr Y_Dst_Buffer_HW[4];
	static myAddr U_Dst_Buffer_HW[4];

	int 		 frm_num;

	/**************variable init*******************/
	// cfg para init
	ise_cfg.in_h = TEST_SRC_HEIGHT;
	ise_cfg.in_w = TEST_SRC_WIDTH;
	ise_cfg.out_h[0] = TEST_PANO_HEIGHT;
	ise_cfg.out_w[0] = TEST_PANO_WIDTH;
	ise_cfg.p0	= TEST_P0;
	ise_cfg.cx0 = TEST_CX0;
	ise_cfg.cy0 = TEST_CY0;
	ise_cfg.p1	= TEST_P1;
	ise_cfg.cx1 = TEST_CX1;
	ise_cfg.cy1 = TEST_CY1;
	ise_cfg.in_yuv_type = TEST_IN_YUV_TYPE;
	ise_cfg.out_flip[0]	= TEST_PANO_FLIP;
	ise_cfg.out_mirror[0] = TEST_PANO_MIRROR;
	ise_cfg.out_yuv_type = TEST_PANO_YUV_TYPE;
	ise_cfg.in_luma_pitch =TEST_SRC_STRIDE ;
	ise_cfg.in_chroma_pitch = TEST_SRC_STRIDE;
	ise_cfg.out_luma_pitch[0] = TEST_PANO_LUMA_STRIDE;
	ise_cfg.out_chroma_pitch[0] = TEST_PANO_CHROMA_STRIDE;

	//chn0
	ise_cfg.out_en[1+0] = TEST_SCALAR_EN_CHN0;
	if(ise_cfg.out_en[1+0])
	{
		printf("chn0 enable! \n");
		ise_cfg.out_h[1+0] = TEST_SCALAR_HEIGHT0;
		ise_cfg.out_w[1+0] = TEST_SCALAR_WIDTH0;
		ise_cfg.out_flip[1+0] = TEST_SCALAR_FLIP0;
		ise_cfg.out_mirror[1+0] = TEST_SCALAR_MIRROR0;
		ise_cfg.out_luma_pitch[1+0] = TEST_SCALAR_LUMA_STRIDE0;
		ise_cfg.out_chroma_pitch[1+0] = TEST_SCALAR_CHROMA_STRIDE0;
	}

	//chn1
	ise_cfg.out_en[1+1] = TEST_SCALAR_EN_CHN1;
	if(ise_cfg.out_en[1+1])
	{
		printf("chn1 enable! \n");
		ise_cfg.out_h[1+1] = TEST_SCALAR_HEIGHT1;
		ise_cfg.out_w[1+1] = TEST_SCALAR_WIDTH1;
		ise_cfg.out_flip[1+1]=TEST_SCALAR_FLIP1;
		ise_cfg.out_mirror[1+1]=TEST_SCALAR_MIRROR1;
		ise_cfg.out_luma_pitch[1+1] = TEST_SCALAR_LUMA_STRIDE1;
		ise_cfg.out_chroma_pitch[1+1]=TEST_SCALAR_CHROMA_STRIDE1;
	}

	//chn2
	ise_cfg.out_en[1+2] = TEST_SCALAR_EN_CHN2;
	if(ise_cfg.out_en[1+2])
	{
		printf("chn2 enable! \n");
		ise_cfg.out_h[1+2] = TEST_SCALAR_HEIGHT2;
		ise_cfg.out_w[1+2] = TEST_SCALAR_WIDTH2;
		ise_cfg.out_flip[1+2] = TEST_SCALAR_FLIP2;
		ise_cfg.out_mirror[1+2] = TEST_SCALAR_MIRROR2;
		ise_cfg.out_luma_pitch[1+2] = TEST_SCALAR_LUMA_STRIDE2;
		ise_cfg.out_chroma_pitch[1+2] = TEST_SCALAR_CHROMA_STRIDE2;
	}

	system("rmmod /lib/modules/4.4.55/sunxi_ise.ko");
	usleep(1000);
	system("insmod /lib/modules/4.4.55/sunxi_ise.ko");

    int fifoFd = 0;

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

	/*****************input&output mem alloc*****************/
	unsigned int test_time = 0;
	while(test_time < TEST_TIMES)
	{
		handle0 = ISE_Create_Bi(&ise_cfg);
		if (NULL == handle0)
		{
			printf("Create ISE fail\n");
			goto termi;
		}

		// initial LUT and write register
		hresult = ISE_SetAttr_Bi(handle0);
		if (0 != hresult)
		{
			printf("Set ISE fail\n");
			goto termi;
		}

		/**********************获取fion,为开辟空间使用**********************/
		int fion = -1;
		ISE_Context_BI *ise_ctx = (ISE_Context_BI*)handle0;
		fion = ise_ctx->fion;
		/**********************获取fion,为开辟空间使用**********************/
		// src memory malloc
		for (i = 0; i < 2; i++)
		{
			block_size = ise_cfg.in_luma_pitch * ise_cfg.in_h * sizeof(unsigned char);
			malloc_ret = myMalloc(fion, Y_Src_Buffer+i, block_size);
			if (malloc_ret == -1)
			{
				printf("malloc Y_Src_Buffer fail\n");
				goto termi;
			}
			myMemSet(fion, Y_Src_Buffer + i, 0);
			ise_procin.in_luma[i] = &(Y_Src_Buffer[i]);

			block_size = ise_cfg.in_chroma_pitch * ise_cfg.in_h * sizeof(unsigned char) / 2;
			if (ise_cfg.in_yuv_type == YUV422)
			{
				block_size *= 2;
			}

			malloc_ret = myMalloc(fion, C_Src_Buffer + i, block_size);
			if (malloc_ret == -1)
			{
				printf("malloc C_Src_Buffer fail\n");
				goto termi;
			}
			myMemSet(fion, C_Src_Buffer + i, 0);
			ise_procin.in_chroma[i] = &(C_Src_Buffer[i]);
		}

		// pano out memory malloc
		block_size = ise_cfg.out_h[0] * ise_cfg.out_luma_pitch[0] * sizeof(unsigned char);
		malloc_ret = myMalloc(fion, Y_Dst_Buffer_HW, block_size);
		if (malloc_ret == -1)
		{
			printf("malloc Y_Dst_Buffer_HW fail\n");
			goto termi;
		}
		myMemSet(fion, Y_Dst_Buffer_HW, 0);
		ise_procout.out_luma[0] = &(Y_Dst_Buffer_HW[0]);


		block_size = ise_cfg.out_h[0] * ise_cfg.out_chroma_pitch[0] * sizeof(unsigned char) / 2;
		if (ise_cfg.in_yuv_type == YUV422)
			block_size *= 2;

		malloc_ret = myMalloc(fion, U_Dst_Buffer_HW, block_size);
		if (malloc_ret == -1)
		{
			printf("malloc U_Dst_Buffer_HW fail\n");
			goto termi;
		}
		myMemSet(fion, U_Dst_Buffer_HW, 0);
		ise_procout.out_chroma_u[0] = &(U_Dst_Buffer_HW[0]);

		// scale chanel memory malloc
		for (j = 0; j < MAX_SCALAR_CHNL; j++)
		{
			if(ise_cfg.out_en[1+j])
			{
				ise_procout.out_luma[1+j] = NULL;
				ise_procout.out_chroma_u[1+j] = NULL;
				ise_procout.out_chroma_v[1+j] = NULL;

				block_size = ise_cfg.out_h[1+j] * ise_cfg.out_luma_pitch[1+j] * sizeof(unsigned char);
				malloc_ret = myMalloc(fion, Y_Dst_Buffer_HW+(j+1), block_size);
				if (malloc_ret == -1)
				{
					printf("malloc Y_Dst_Buffer_HW[%d] fail! \r\n",j);
					goto termi;
				}
				myMemSet(fion, Y_Dst_Buffer_HW+(j+1), 0);
				ise_procout.out_luma[1+j] = &(Y_Dst_Buffer_HW[j+1]);

				block_size = ise_cfg.out_h[1+j] * ise_cfg.out_chroma_pitch[1+j] * sizeof(unsigned char) / 2;
				if (ise_cfg.in_yuv_type == YUV422)
				{
					block_size *= 2;
				}

				malloc_ret = myMalloc(fion, U_Dst_Buffer_HW+(j+1), block_size);
				if (malloc_ret == -1)
				{
					printf("malloc U_Dst_Buffer_HW[%d] fail! \r\n", j);
					goto termi;
				}
				myMemSet(fion, U_Dst_Buffer_HW+(j+1), 0);
				ise_procout.out_chroma_u[1+j] = &(U_Dst_Buffer_HW[j+1]);

			}

		}

		// 读取SRC文件
		for (i = 0; i < 2; i++)
		{
			switch(i)
			{
			case 0:
				if (ise_cfg.in_yuv_type == YUV420)
				{
					//sprintf(filename, "fisheye_asia_320.yuv420");
					sprintf(filename, YUV_SOURCE_IMG);
				}
				else
				{
					goto termi;
				}
				fp_src = fopen(filename, "rb");
				break;
			case 1:
				if (ise_cfg.in_yuv_type == YUV420)
				{
					//sprintf(filename, "fisheye_asia_320.yuv420");
					sprintf(filename,YUV_SOURCE_IMG);
				}
				else
				{
					goto termi;
				}
				fp_src = fopen(filename, "rb");
				break;
			default:
				fp_src= NULL;
				break;
			}

			if(!fp_src)
			{
				printf("Src file does not exist\n");
				goto termi;
			}
			block_size = ise_cfg.in_luma_pitch * ise_cfg.in_h * sizeof(unsigned char);
			n_readsize = fread(ise_procin.in_luma[i]->mmu_Addr, 1, block_size, fp_src);
			if (n_readsize != block_size)
			{
				printf("read yuv file fail\n");
				fclose(fp_src);
				fp_src = NULL;
				goto termi;
			}

			block_size = ise_cfg.in_chroma_pitch * ise_cfg.in_h * sizeof(unsigned char) / 2;
			n_readsize = fread(ise_procin.in_chroma[i]->mmu_Addr, 1, block_size, fp_src);
			if (n_readsize != block_size)
			{
				printf("read yuv file fail\n");
				fclose(fp_src);
				fp_src = NULL;
				goto termi;
			}

			fclose(fp_src);
			fp_src = NULL;
		}

		// 循环测试TEST_NIMG次
		frm_num = 0;
		while(frm_num < NUM_FRAM)
		{
			ISE_Proc_Bi(handle0, &ise_procin, &ise_procout);

			hresult = ISE_CheckResult_Bi(handle0, &ise_procout);

			printf("frm_num:%d!\n", frm_num);
			frm_num++;

		}

#if 1
		for(i = 0;i < 3; i++)
		{
			if(0 == hresult)
			{
				printf("ISE Process success!\n");
				 write(fifoFd, "P[ISE] OK", 50);
			}
			else
			{	
				printf("ISE Process fail!\n");
				 write(fifoFd, "F[ISE] fail", 50);
			}
			sleep(1);
		}
#endif
		//close(fifoFd);

termi:
		ISE_Destroy_Bi(handle0);

		printf("%s %s(Line %d)\n\r",__FILE__,__FUNCTION__,__LINE__);
		//free SRC
		for (i = 0; i < 2; i++)
		{
			if (NULL != ise_procin.in_luma[i])
			{
				myFree(fion, ise_procin.in_luma[i]);
			}

			if (NULL != ise_procin.in_chroma[i])
			{
				myFree(fion, ise_procin.in_chroma[i]);
			}
		}

		// free DST
		if (NULL != ise_procout.out_luma[0])
		{
			myFree(fion, ise_procout.out_luma[0]);
		}
		if (NULL != ise_procout.out_chroma_u[0])
		{
			myFree(fion, ise_procout.out_chroma_u[0]);
		}
		if (NULL != ise_procout.out_chroma_v[0])
		{
			myFree(fion, ise_procout.out_chroma_v[0]);
		}

		// free SCA
		for (i = 0; i < MAX_SCALAR_CHNL; i++)
		{
			if(ise_cfg.out_en[1+i])
			{
				if(NULL != ise_procout.out_luma[1+i])
				{
					myFree(fion, ise_procout.out_luma[1+i]);
				}
				if(NULL != ise_procout.out_chroma_u[1+i])
				{
					myFree(fion, ise_procout.out_chroma_u[1+i]);
				}
				if(NULL != ise_procout.out_chroma_v[1+i])
				{
					myFree(fion, ise_procout.out_chroma_v[1+i]);
				}
			}
		}

		printf("------------->test_time:%d<-------------\n", test_time);
		test_time++;
	}

	return 0;
}
