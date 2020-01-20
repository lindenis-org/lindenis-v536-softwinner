#ifndef DATATYPES_H
#define DATATYPES_H

#include "stdint.h"
#include "malloc.h"
#include "stdio.h"
#include "stdlib.h"


#define MAX_MEMORY_SIZE    (1u<<27)
#define MAX_SRC_CACHE_SIZE (6144)  //(1u<<13)    // rows * cols
#define MAX_LUT_CACHE_SIZE (1u<<16)     // rows * cols

#define MAX_ROI_WIDTH      96
#define MAX_ROI_HEIGHT     64

#define Y_SHIFT_BITS       16
#define X_MASK             ((1<<16) - 1)

#define LOWBIT_SHIFTS      4
#define LOWBIT_MASK        ((1 << LOWBIT_SHIFTS) - 1)

#define EDGE_PIXES         1

#define COMP_PIXES         8
#define COMP_PIXES_16      16
#define BLOCK_SIZE         32

#define UNCOMP_PRECISE     16
#define UNCOMP_LOW_MASK    ((1 << UNCOMP_PRECISE) - 1)
#define UNCOMP_HIGH_SHIFT  UNCOMP_PRECISE

const int static SCALAR_PRECISE          = 15;
const int static INV_SCALAR_PRECISE      = 8;
const int static INV_SCALAR_PRECISE_0[2] = {8, 9};

const static uint32_t uncomp_multi  = (1.0 * (COMP_PIXES - 1) * (1 << 16) / (BLOCK_SIZE - 1));  //

const static uint32_t uncomp_multi_16x16  = (1.0 * (COMP_PIXES_16 - 1) * (1 << 16) / (BLOCK_SIZE - 1));  //

const static double   comp_rate     =  1.0 * (BLOCK_SIZE - 1) / (COMP_PIXES - 1);

#ifndef __cplusplus
typedef enum {false, true} bool;
#endif

typedef enum IMAGE_FORMAT
{
	PLANE_YUV420  = 0x0,
	PLANE_YVU420  = 0x01,
	PLANE_YUV420p = 0x02,
	PLANE_YUV422  = 0x04,
	PLANE_YVU422  = 0x05,
	PLANE_YUV422p = 0x06,
	PLANE_RGB     = 0x07,
	PLANE_RGBp    = 0x08
}IMAGE_FORMAT;

typedef enum PLANE_FORMAT
{
   C_PLANE,
   Y_PLANE,
   U_PLANE,
   V_PLANE
}PLANE_FORMAT;

typedef struct tagROI
{
	int32_t offx;
	int32_t offy;
	int32_t cWidth;
	int32_t cHeight;
}ROI;

enum DUMPOUT_INDEX{
	OUT_Y_DATA_FH,
	OUT_U_DATA_FH,
	OUT_V_DATA_FH
};

typedef struct  tagRCTGL
{
	uint16_t left;
	uint16_t right;
	uint16_t bottom;
	uint16_t top;
}RCTGL, Boundry;


typedef struct cacheLut
{
	uint32_t cdata[MAX_LUT_CACHE_SIZE];

	int32_t cWidth;
	int32_t cHeight;

	int32_t offx; 	//
	int32_t offy;

}cacheLut;

typedef struct lutCompBlock
{
	uint32_t lut_data[COMP_PIXES][COMP_PIXES];
	
	uint32_t block_width;
	uint32_t block_height;

	uint32_t comp_cols_pixes;
	uint32_t comp_rows_pixes;

	uint32_t x_off;
	uint32_t y_off;

	uint32_t block_index;    //第i块
}lutCompBlock;

typedef struct 
{
	struct cacheLut *plut;

	int32_t cWidth;
	int32_t cHeight;

	int32_t offx; 	//
	int32_t offy;
}subCacheLut;

typedef struct cacheSrc
{
	int8_t cache0[MAX_SRC_CACHE_SIZE];
	int8_t cache1[MAX_SRC_CACHE_SIZE / 2];

	int32_t cWidth;
	int32_t cHeight;
	int32_t line_size;

	int32_t offx; 	//
	int32_t offy;
}cacheSrc;

typedef struct srcBuffers
{
	int8_t src_buffers[MAX_SRC_CACHE_SIZE];
	int32_t cWidth;
	int32_t cHeight;

	int32_t offx;
	int32_t offy;
}srcBuffers;

typedef struct cacheDst
{
	int8_t cache0[MAX_LUT_CACHE_SIZE];
	int8_t cache1[MAX_LUT_CACHE_SIZE];

	int32_t cWidth;
	int32_t cHeight;

	int32_t offx; 	//
	int32_t offy;
}cacheDst;

typedef struct yuvImage
{
	unsigned char *yuv_data[2];

	int32_t width;
	int32_t height;
	int32_t pitch_size[2];
}yuvImage;


typedef struct rgbImage
{
	unsigned char *rgbData;

	int32_t width;
	int32_t height;
	int32_t widthStep;
}rgbImage; 

typedef struct LUTHeader
{
	uint32_t *lut_data;
	int32_t width;
	int32_t height;
}LUTHeader;

//old def
typedef struct LUTCompHeader
{
	uint32_t *lut_comp_data;

	int32_t width;     //LUT 宽度
	int32_t height;    //LUT 高度

	int32_t block_width;  // LUT block width
	int32_t block_height; // LUT block height

	int32_t comp_cols_pixes;  // Compressed block width
	int32_t comp_rows_pixes;

	int32_t block_step;

	Boundry *src_roi_info;
	char    *split_info;
	bool     b_has_src_info;  //包含src ROI信息

}LUTCompHeader;

typedef struct  LutPixelData
{
	float y;
	float x;
}LutPixelData;

typedef struct LutStreamData
{
	LutPixelData *lut_data;

	int32_t width;
	int32_t height;
	int32_t pitch_size;
}LutStreamData;


#endif
