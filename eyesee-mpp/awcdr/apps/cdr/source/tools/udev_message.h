/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file udev_message.h
 * @brief 定义udev事件设备状态消息类型
 * @author id:826
 * @version v0.3
 * @date 2016-06-03
 */

#ifndef UDEV_MESSAGE_H_
#define UDEV_MESSAGE_H_

/**
 * @brief 存储设备类型
 */
enum StorageType {
    SDCARD = 0x10,
    UDISK  = 0x20,
    HDMI   = 0x30,
    CVBS   = 0x40,
    USB2HOST = 0x50,
    TP9950 = 0x60,

    UKNOWN_TYPE = 0xF0,
};

/**
 * @brief 存储设备状态类型
 */
enum StorageStatus {
    UDEV_INSERT   = 0x01,
    UDEV_MOUNTED  = 0x02,
    UDEV_REMOVE   = 0x03,
    UDEV_UMOUNT   = 0x04,
    UDEV_CHANGE   = 0x05,

    // Error Status
    UDEV_FS_ERROR = 0x08, /**< file system error */
    UKNOWN_ERROR = 0xF0,
};


#define SD_INSERT       (SDCARD|UDEV_INSERT)
#define SD_MOUNTED      (SDCARD|UDEV_MOUNTED)
#define SD_REMOVE       (SDCARD|UDEV_REMOVE)
#define SD_UMOUNT       (SDCARD|UDEV_UMOUNT)
#define SD_FS_ERROR     (SDCARD|UDEV_FS_ERROR)

#define UDISK_INSERT    (UDISK|UDEV_INSERT)
#define UDISK_MOUNTED   (UDISK|UDEV_MOUNTED)
#define UDISK_REMOVE    (UDISK|UDEV_REMOVE)
#define UDISK_UMOUNT    (UDISK|UDEV_UMOUNT)
#define UDISK_FS_ERROR  (UDISK|UDEV_FS_ERROR)

#define HDMI_INSERT     (HDMI|UDEV_INSERT)
#define HDMI_REMOVE     (HDMI|UDEV_REMOVE)

#define TVOUT_INSERT    (CVBS|UDEV_INSERT)
#define TVOUT_REMOVE    (CVBS|UDEV_REMOVE)

#define USB_HOST_CONNECTED (USB2HOST|UDEV_INSERT)
#define USB_HOST_DETACHED  (USB2HOST|UDEV_REMOVE)

#define AHD_CONNECTED (TP9950|UDEV_INSERT)
#define AHD_DETACHED  (TP9950|UDEV_REMOVE)


#endif /* udev_message.h */
