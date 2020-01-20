
#ifndef _ISE_LIB_BI_H_
#define _ISE_LIB_BI_H_

#include "ISE_common.h"

/*****************************************************************************
* 基本参数配置参数
* in_h                  原图分辨率h，4的倍数
* in_w                  原图分辨率w，4的倍数
* pano_h            	全景图分辨率h，4的倍数（最小320，最大4032）
* pano_w           	    全景图分辨率w，8的倍数（最小320，最大8064）
* in_luma_pitch         原图亮度pitch
* in_chroma_pitch       原图色度pitch
* pano_luma_pitch       全景图亮度pitch
* pano_chroma_pitch     全景图色度pitch
* reserved          	保留字段，字节对齐
*******************************************************************************/
typedef struct _ISE_CFG_PARA_BI_{
	int                      in_h;
	int                      in_w;
	int						 in_luma_pitch;
	int						 in_chroma_pitch;
	int                      in_yuv_type;
	int						 out_en[1+MAX_SCALAR_CHNL];
	int                      out_h[1+MAX_SCALAR_CHNL];
	int                      out_w[1+MAX_SCALAR_CHNL];
	int						 out_flip[1+MAX_SCALAR_CHNL];
	int					     out_mirror[1+MAX_SCALAR_CHNL];
	int						 out_luma_pitch[1+MAX_SCALAR_CHNL];
	int						 out_chroma_pitch[1+MAX_SCALAR_CHNL];
	int                      out_yuv_type;
	float					 p0;
	int					     cx0;
	int					     cy0;
  	float                    p1;
	int						 cx1;
	int						 cy1;
	double					 calib_matr[3][3];
	double					 calib_matr_cv[3][3];
	double					 distort[8];
	char                     reserved[32];
}ISE_CFG_PARA_BI;

/******************************************************************************
* 主处理输入参数
* in_luma                     原始图像指针数组，亮度分量，nv12或者nv16格式
* in_chroma                   原始图像指针数组，色度分量，nv12或者nv16格式
*******************************************************************************/
typedef struct _ISE_PROCIN_PARA_BI_{
	myAddr			 *in_luma[2];	   // PIM_PROC_CALC_PANO参数
	myAddr			 *in_chroma[2];    // PIM_PROC_CALC_PANO参数
	char			 reserved[32];
}ISE_PROCIN_PARA_BI;

/******************************************************************************
* 主处理输出参数
* pano_luma            全景图像，亮度分量，nv12或者nv16格式
* pano_chroma          全景图像，色度分量，nv12或者nv16格式
*******************************************************************************/
typedef struct _ISE_PROCOUT_PARA_BI_{
	myAddr			 *out_luma[1+MAX_SCALAR_CHNL];
	myAddr			 *out_chroma_u[1+MAX_SCALAR_CHNL];
	myAddr			 *out_chroma_v[1+MAX_SCALAR_CHNL];
	char			 reserved[32];
}ISE_PROCOUT_PARA_BI;


// 临时显示，完全封装后需隐藏
int myMalloc(int fion, myAddr *addr, unsigned long numOfBytes );
void myMemSet(int fion, myAddr *addr, unsigned char value);
void myGetData(const char *path, myAddr *addr);
int myFree(int fion, myAddr *addr);

// 接口函数
ISE_HANDLE_BI *ISE_Create_Bi(ISE_CFG_PARA_BI *ise_cfg);

int ISE_SetAttr_Bi(ISE_HANDLE_BI *handle);

int ISE_Proc_Bi(
	ISE_HANDLE_BI 		*handle,
	ISE_PROCIN_PARA_BI 	*ise_procin, 
	ISE_PROCOUT_PARA_BI *ise_procout);

int ISE_CheckResult_Bi(
	ISE_HANDLE_BI 		*handle, 
	ISE_PROCOUT_PARA_BI *ise_procout);

int ISE_Destroy_Bi(ISE_HANDLE_BI *handle);

#endif


