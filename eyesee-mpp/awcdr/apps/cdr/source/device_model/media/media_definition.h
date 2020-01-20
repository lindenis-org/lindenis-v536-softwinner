/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file media_definition.h
 * @brief 定义应用层多媒体相关的宏定义，枚举，数据结构
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#ifndef  MEDIA_DEFINITION_H_
#define  MEDIA_DEFINITION_H_

namespace EyeseeLinux {

//the picture's compression ratio
#define NET_JPEG_QUALITY 20
#define NORMAL_JPEG_QUALITY 95

#define DEFAULT_CAPTURE_BUF_NUM 9
#define DEFAULT_CAPTURE_WIDTH   1280
#define DEFAULT_CAPTURE_HEIGHT  720
#define DEFAULT_1080P_BITRATE   (10<<20)

//#define ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))
//#define ALIGN32M(num) (ALIGN(num, (1<<25)))

#define MINUTE(ms) (ms/1000)
//#define VIDEO_PREALLOC_SIZE(ms) (ALIGN32M((MINUTE(ms)*DEFAULT_1080P_BITRATE/8)))

// //csi camer default init value
// #define CSI_DEFAULT_PREVIEW_WIDTH   960
// #define CSI_DEFAULT_PREVIEW_HEIGHT  540
// #define CSI_DEFAULT_VIDEO_SIZE_WIDTH    1920
// #define CSI_DEFAULT_VIDEO_SIZE_HEIGHT   1080
// #define CSI_CAM_FRAMERATE   30
// #define CAM_CSI_BITRATE (14<<20)

// //uvc camer default init value
// #define UVC_DEFAULT_PREVIEW_WIDTH   320
// #define UVC_DEFAULT_PREVIEW_HEIGHT  192
// #define UVC_DEFAULT_VIDEO_SIZE_WIDTH    1280
// #define UVC_DEFAULT_VIDEO_SIZE_HEIGHT   720
// #define UVC_CAM_FRAMERATE   25
// #define CAM_UVC_BITRATE (8<<20)

// //csi net and uvc net bitrate and preview size
// #define CAM_CSI_NET_BITRATE (2*1024*1024)
// #define CAM_CSI_NET_PREVIEW_WIDTH 1280
// #define CAM_CSI_NET_PREVIEW_HEIGHT 720

// #define CAM_UVC_NET_BITRATE (2*1024*1024)
// #define CAM_UVC_NET_PREVIEW_WIDTH 1280
// #define CAM_UVC_NET_PREVIEW_HEIGHT 720

// //small video wirte sd's bitrate and it's record video's size
// #define CAM_CSI_NET_SMALL_BITRATE   (1*1024*1024)
// #define CAM_CSI_NET_SMALL_VIDEO_WIDTH   640
// #define CAM_CSI_NET_SMALL_VIDEO_HEIGHT  480

/** Stream Sender Type */
typedef enum {
    STREAM_SENDER_RTSP = 1 << 0,
    STREAM_SENDER_TUTK = 1 << 1,
    STREAM_SENDER_UVC  = 1 << 2,
    STREAM_SENDER_NONE = 1 << 3,
} StreamSenderType;

typedef enum {
    PIC_RESOLUTION_8M= 0,
    PIC_RESOLUTION_13M,
    PIC_RESOLUTION_16M,
}PicResolution;
#if 0
 typedef enum {
     VIDEO_QUALITY_4K30FPS = 0,
     VIDEO_QUALITY_2_7K30FPS,
     VIDEO_QUALITY_1080P120FPS,
     VIDEO_QUALITY_1080P60FPS,
     VIDEO_QUALITY_1080P30FPS,
     VIDEO_QUALITY_720P240FPS,
     VIDEO_QUALITY_720P120FPS,
     VIDEO_QUALITY_720P60FPS,
     VIDEO_QUALITY_720P30FPS,
 }VideoQuality;
#else
typedef enum {
	VIDEO_QUALITY_4K30FPS = 0,
	VIDEO_QUALITY_2_7K30FPS,
	VIDEO_QUALITY_1080P30FPS,
	VIDEO_QUALITY_720P30FPS,
	VIDEO_QUALITY_720P60FPS,
	VIDEO_QUALITY_1080P120FPS,
	VIDEO_QUALITY_1080P60FPS,
	VIDEO_QUALITY_720P240FPS,
	VIDEO_QUALITY_720P120FPS,
}VideoQuality;

#endif
typedef enum{
    NORMAL_TAKE_PIC = 0,
    SMALL_TAKE_PIC,
    SCREENSHOT_TAKE_PIC,
}TakePicType;

// typedef enum {
    // VIDEO_120FPS = 0,
    // VIDEO_60FPS,
    // VIDEO_30FPS,
    // VIDEO_15FPS,
    // FPS_COUNT,
// }VideoFPS;

typedef enum {
    PIC_NORMAL= 0,
    PIC_FAST,
    PIC_CONTINUOUS,
    PIC_CONTINUOUS_FAST,
}PicMode;

typedef enum {
    NO_FLIP= 0,
    LEFT_RIGHT_FLIP,
    TOP_BOTTOM_FLIP,
}PreviewFlip;

typedef enum {
    AUTO= 0,
    DAYLIGHT,
    CLOUDY,
    INCANDESCENT,
    FLUORESCENT,
}WhiteBlance;

typedef struct Size {
    int width;
    int height;

    Size() {
        width = 0;
        height = 0;
    }

    Size(int w, int h) {
        width = w;
        height = h;
    }
} Size;

/**
 * 磁盘状态. 多媒体内部定义磁盘状态
 */
enum {
    STORAGE_OK = 0,
    STORAGE_NOT_OK,
    STORAGE_IS_FULL,
};

} /* EyeseeLinux */

#endif /* MEDIA_DEFINITION_H_ */
