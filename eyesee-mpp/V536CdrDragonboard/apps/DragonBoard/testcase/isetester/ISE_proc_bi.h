#ifndef _ISE_PROC_BI_H_
#define _ISE_PROC_BI_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ISE_lib_bi.h"
#include "DataTypesNew.h"

typedef struct _ISE_Context_BI_
{
   	int 			filp ;		   // drive noder
   	int 			fion ;		   // ION noder

   	int				finishedFlag;

   	MAPTIHeader	   	table_header0;
   	ROIP_params	   	*roip0;
   	uint32_t 	   	*lut0;

   	MAPTIHeader	   	table_header1;
   	ROIP_params	   	*roip1;
   	uint32_t 	   	*lut1;

   	MAPTIHeader	   	table_header2;
   	ROIP_params	   	*roip2;
   	uint32_t 	   	*lut2;

	myAddr 			LUT_ROIP_Buffer[3];
	myAddr 			LUTBuffer[3];
	myAddr 			ROIPBuffer[3];
	myAddr 			NBBuffer[3];

   	//uint32_t 	   	*NB_Buffer[MAX_SCALAR_CHNL];

   	int32_t		   	feather_radius;
   	int32_t		   	blk_width;
   	int32_t		   	blk_height;
   	int32_t		   	mid0_width;
   	int32_t		  	mid1_width;
   	int32_t		   	mid2_width;

   	ISE_CFG_PARA_BI 	*ise_cfg;
}ISE_Context_BI;


#if 0
/* LUT、ROIP混合存放结构体 */
myAddr LUT_ROIP_Buffer[3] = {
	{ .phy_Addr		=	NULL,
	.mmu_Addr		=	NULL,
	.ion_handle	=	NULL,
	.fd			=	-1,
	.length		=	0	}
};


/* LUT、ROIP存放结构体 */
myAddr LUTBuffer[3] = {
	{ .phy_Addr		=	NULL,
	  .mmu_Addr		=	NULL,
	  .ion_handle	=	NULL,
	  .fd			=	-1,
	  .length		=	0	}
};


myAddr ROIPBuffer[3] = {
	{ .phy_Addr		=	NULL,
	  .mmu_Addr		=	NULL,
	  .ion_handle	=	NULL,
	  .fd			=	-1,
	  .length		=	0	}
};


/* 软件计算中间数据暂存结构体 */
myAddr NBBuffer[3] = {
	{ .phy_Addr		=	NULL,
	  .mmu_Addr		=	NULL,
	  .ion_handle	=	NULL,
	  .fd			=	-1,
	  .length		=	0	}
};
#endif

#endif

