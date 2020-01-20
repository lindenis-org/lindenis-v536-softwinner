#include "imgdecode.h"
#include "sunxi_mbr.h"
#include "ota_private.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "mtd_opration.h"
//#include "app_log.h"

#define db_msg printf

#define  IMAGE_BUFFER_LENGTH_NOR     (32 * 1024 * 1024)

#define  BOOT_LENGTH                 (704 * 1024)
#define  BOOT0_OFFSET                 0
#define  UBOOT_OFFSET                (64 * 1024)
#define  MBR_OFFSET                  ((1024 - 16) * 1024)
#define  BLKBURNBOOT0                _IO('v', 127)
#define  BLKBURNBOOT1                _IO('v', 128)


static void *imghd = NULL;
static void *imgitemhd = NULL;
static int  flash_handle = 0;

typedef enum {
    BOOT_NOR,
    ROOTFS_NOR,
    BOOTLOGO_NOR,
    ENV_NOR,
    OVERLAY_NOR,
    CUSTOM_NOR,
}flash_partition_nor_enum;

int ota_open_firmware(char *name)
{
    printf("firmware name %s\n", name);
    imghd = Img_Open(name);
    if(!imghd)
    {
        return -1;
    }
    return 0;
}

int ota_get_partition_info(sunxi_download_info  *dl_map)
{
    imgitemhd = Img_OpenItem(imghd, "12345678", "1234567890DLINFO");
    if(!imgitemhd)
    {
        return -1;
    }
    printf("try to read item dl map\n");
    if(!Img_ReadItem(imghd, imgitemhd, (void *)dl_map, sizeof(sunxi_download_info)))
    {
        printf("error : read dl map failed\n");
        return -1;
    }
    Img_CloseItem(imghd, imgitemhd);
    imgitemhd = NULL;
    return 0;
}

int ota_read_mbr(void  *img_mbr)
{
    imgitemhd = Img_OpenItem(imghd, "12345678", "1234567890___MBR");
    if(!imgitemhd)
    {
        return -1;
    }
    printf("try to read item dl map\n");
    if(!Img_ReadItem(imghd, imgitemhd, img_mbr, sizeof(sunxi_mbr_t)))
    {
        printf("error : read mbr failed\n");
        return -1;
    }
    Img_CloseItem(imghd, imgitemhd);
    imgitemhd = NULL;
    return 0;
}

static int cmdline_get_spinor_mbr_offset(unsigned long *len)
{
	char cmd[512];
        char *endp = NULL;
	int fd = -1;
	int ret;

	fd = open("/proc/cmdline", O_RDONLY);
	if(fd < 0)
	{
	        printf("open cmdline file fail\n");
		return -1;
	}

	ret = read(fd, cmd, sizeof(cmd));
	if(ret < 0)
	{
	        printf("read cmdline file fail\n");
		goto error;
	}

        char *p = strstr(cmd, "mbr=");
        if (!p)
		goto error;
        *len = strtol(p + 4, &endp, 16);
	if(fd >= 0)
		close(fd);
        return 0;

error:
	if(fd >= 0)
		close(fd);
	return -1;
}

int ota_write_mbr(void  *img_mbr, unsigned int mbr_size)
{
    int ret_len = 0;
    unsigned long mbr_offset;

    flash_handle = open("/dev/mtdblock0",O_RDWR);
    if(flash_handle < 0)
    {
        printf("open flash_handle fail\n");
        return -1;
    }

    if(cmdline_get_spinor_mbr_offset(&mbr_offset))
	    mbr_offset = MBR_OFFSET;

    printf("the mbr size:%d\n", mbr_offset);

    lseek(flash_handle, mbr_offset, SEEK_SET);
    ret_len = write(flash_handle,img_mbr,mbr_size);
    fdatasync(flash_handle);
    close(flash_handle);

    if(ret_len != mbr_size)
    {
        printf("error! ota write mbr to flash failed! line=%d,ret_len=%d \n",__LINE__,ret_len);
        return -1;
    }

    return 0;
}

static int __download_normal_part(dl_one_part_info *part_info,  char *buffer, flash_partition_nor_enum part_num)
{
    if((buffer == NULL)||(part_info == NULL))
    {
        printf("error, NULL pointer...\n");
        return -1;
    }

    int ret_len = 0;
    /* if you want to erase all partition ,you should set erase_flag and
     * need to select char node. like /dev/mtd/name/char_overlay.
     */
    int erase_flag = 0;
    signed long long  partdata_by_byte;
    int  ret = -1;

    /*open item by filename*/
    imgitemhd = Img_OpenItem(imghd, "RFSFAT16", (char *)part_info->dl_filename);
    if(!imgitemhd)
    {
        printf("error: ota open image item failed, file name: %s \n", part_info->dl_filename);
        return -1;
    }

    /*get file size*/
    partdata_by_byte = Img_GetItemSize(imghd, imgitemhd);
    if (partdata_by_byte <= 0)
    {
        printf("error: ota get item partition size failed, file name: %s \n", part_info->dl_filename);
        goto __download_normal_part_err1;
    }
    printf("partdata hi 0x%x\n", (uint)(partdata_by_byte>>32));
    printf("partdata lo 0x%x\n", (uint)partdata_by_byte);

    /*get file data to buffer*/
    Img_ReadItem(imghd, imgitemhd, buffer,IMAGE_BUFFER_LENGTH_NOR);

    /*app write file data to flash*/
    switch(part_num)
    {
        case BOOT_NOR:
            {
                flash_handle = open("/dev/mtd_name/boot",O_SYNC | O_RDWR);
                if(flash_handle < 0)
                {
                    printf("open boot partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
            }
            break;

        case ROOTFS_NOR:
            {
                flash_handle = open("/dev/mtd_name/rootfs",O_SYNC | O_RDWR);
                if(flash_handle < 0)
                {
                    printf("open rootfs partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
            }
            break;

        case BOOTLOGO_NOR:
            {
                flash_handle = open("/dev/mtd_name/bootlogo",O_SYNC | O_RDWR);
                if(flash_handle < 0)
                {
                    printf("open bootlogo partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
            }
            break;

        case ENV_NOR:
            {
                flash_handle = open("/dev/mtd_name/env",O_SYNC | O_RDWR);
                if(flash_handle < 0) {
                    printf("open env partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
            }
            break;

        case OVERLAY_NOR:
            {
                flash_handle = open("/dev/mtd_name/char_overlay",O_SYNC | O_RDWR);
                if(flash_handle < 0) {
                    printf("open overlay partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
		erase_flag = 1;
            }
            break;
        case CUSTOM_NOR:
            {
                flash_handle = open("/dev/mtd_name/char_custom",O_SYNC | O_RDWR);
                if(flash_handle < 0) {
                    printf("open custon partition fail\n");
                    ret = -1;
                    goto __download_normal_part_err1;
                }
		erase_flag = 1;
            }
            break;

        default:
            printf("error, open flash partition number invalid\n");
            break;
    }

    printf("write partition to flash,waiting......\n");
    printf("ota write normal partition to flash, line=%d, partdata_by_byte=%lld \n",__LINE__,partdata_by_byte);
    /*for erase partition, need to use mtd interface, so use /dev/mtd* file */
    if(erase_flag)
    {
	 printf("start erase this pratition\n");
	 erase_partiton(flash_handle);
	 printf("erase end\n");
	 ret_len = buffer_to_flash(flash_handle, 0, partdata_by_byte, buffer);
    }else {
	ret_len = write(flash_handle,buffer,partdata_by_byte);
	printf("ota write normal partition to flash, line=%d,ret_len=%d,;partdata_by_byte=%lld \n",__LINE__,ret_len,partdata_by_byte);
    }
    fdatasync(flash_handle);
    close(flash_handle);
    if(ret_len != partdata_by_byte){
        printf("error! ota write flash failed. line=%d,ret_len=%d \n",__LINE__,ret_len);
        ret = -1;
        goto __download_normal_part_err1;
    }
    printf("successed in writting a file to flash partition, file name is: %s\n", part_info->name);
    ret = 0;

__download_normal_part_err1:
    if(imgitemhd)
    {
        Img_CloseItem(imghd, imgitemhd);
        imgitemhd = NULL;
    }
    erase_flag = 0;
    return ret;
}


int ota_update_normal_part(sunxi_download_info *dl_map, update_part_flag ota_partition_flag)
{
    if(!dl_map->download_count)
    {
        printf("ota update normal partition error: no part need to write\n");
        return 0;
    }

    char *buffer = (char *)malloc(IMAGE_BUFFER_LENGTH_NOR);
    if(buffer == NULL)
    {
        printf("error, malloc buffer failed...\n");
        return -1;
    }

    dl_one_part_info    *part_info;
    int ret = -1;
    int i = 0;
    memset(buffer, 0x00, IMAGE_BUFFER_LENGTH_NOR);

    if(ota_partition_flag.update_kernel_flag == 1)
    {	
    	printf("update_kernel_flag\n");
        for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("boot", (char *)part_info->name))
            {
                ret = __download_normal_part(part_info, buffer, BOOT_NOR);
                if(ret != 0)
                {
                    printf("error: ota_update_normal_part, download %s part failed\n",part_info->name);
                }
            }
        }
    }

    if(ota_partition_flag.update_rootfs_flag == 1)
    {
		printf("update_rootfs_flag\n");
		for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("rootfs", (char *)part_info->name))
            {
                ret = __download_normal_part(part_info, buffer, ROOTFS_NOR);
                if(ret != 0)
                {
                    printf("err: ota_update_normal_part, download part %s failed\n",part_info->name);
                }
            }
        }
    }

    if(ota_partition_flag.update_boot_logo_flag == 1)
    {
		printf("update_boot_logo_flag\n");
		for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("bootlogo", (char *)part_info->name)){
                ret = __download_normal_part(part_info, buffer, BOOTLOGO_NOR);
                if(ret != 0)
                {
                    printf("err: ota_update_normal_part, download part %s failed\n",part_info->name);
                }
            }
        }
    }

    if(ota_partition_flag.update_env_flag == 1)
    {
    	printf("update_env_flag\n");
        for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("env", (char *)part_info->name)){
                ret = __download_normal_part(part_info, buffer, ENV_NOR);
                if(ret != 0)
                {
                    printf("err: ota_update_normal_part, download part %s failed\n",part_info->name);
                }
            }
        }
    }

    if(ota_partition_flag.update_overlay_flag == 1)
    {
		printf("update_overlay_flag\n");
		for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("overlay", (char *)part_info->name))
            {
                ret = __download_normal_part(part_info, buffer, OVERLAY_NOR);
                if(ret != 0)
                {
                    printf("err: ota_update_normal_part, download part %s failed\n",part_info->name);
                }
            }
        }
    }

    if(ota_partition_flag.update_custom_flag == 1)
    {
		printf("update_custom_flag\n");
		for(part_info = dl_map->one_part_info, i = 0; i < dl_map->download_count; i++, part_info++)
        {
            if (!strcmp("custom", (char *)part_info->name))
            {
                ret = __download_normal_part(part_info, buffer, CUSTOM_NOR);
                if(ret != 0)
                {
                    printf("err: ota_update_normal_part, download part %s failed\n",part_info->name);
                }
            }
        }
    }

    if(buffer)
    {
        free(buffer);
    }

    return 0;
}

int ota_update_uboot()
{
    int ret = 0;
    int ret_len = 0;
    char buffer[4 * 1024 * 1024] = {0};
    uint item_original_size;

    /*{filename = "boot_package_nor.fex", maintype = "12345678",    subtype = "BOOTPKG-NOR00000",},*/
    imgitemhd = Img_OpenItem(imghd, "12345678", "BOOTPKG-NOR00000");

    if(!imgitemhd)
    {
        printf("sprite update error: fail to open uboot item\n");
        return -1;
    }
    /*get uboot size*/
    item_original_size = Img_GetItemSize(imghd, imgitemhd);
    printf("ota get nor uboot item_original_size: %ud \n", item_original_size);
    if(!item_original_size)
    {
        printf("sprite update error: fail to get uboot item size\n");
        ret = -1;
        goto __uboot_err1;
    }

    /*get uboot data*/

    if(!Img_ReadItem(imghd, imgitemhd, (void *)buffer, item_original_size))
    {
        printf("update error: fail to read data from for uboot\n");
        ret = -1;
        goto __uboot_err1;
    }

    Img_CloseItem(imghd, imgitemhd);
    imgitemhd = NULL;

    flash_handle = open("/dev/mtdblock0",O_RDWR);
    if(flash_handle < 0)
    {
        printf("open flash_handle fail\n");
        return -1;
    }

    lseek(flash_handle, UBOOT_OFFSET, SEEK_SET);
    ret_len = write(flash_handle,buffer,item_original_size);

    printf("line=%d,ret_len=%d,;item_original_size=%ud",__LINE__,ret_len,item_original_size);
    fdatasync(flash_handle);
    close(flash_handle);
    if(ret_len != item_original_size)
    {
        printf("error! ota write uboot to flash failed! line=%d,ret_len=%d \n",__LINE__,ret_len);
        return -1;
    }

    printf("ota_update_uboot ok\n");
    return 0;

__uboot_err1:
    if(imgitemhd)
    {
        Img_CloseItem(imghd, imgitemhd);
        imgitemhd = NULL;
    }
    return ret;
}

int ota_update_boot0()
{
    int ret_len = 0;
    int ret = 0;
    char buffer[3*1024*1024] = {0};
    uint item_original_size;

    imgitemhd = Img_OpenItem(imghd, "12345678", "1234567890BNOR_0");
    if(!imgitemhd)
    {
        printf("sprite update error: fail to open boot0 item\n");
        return -1;
    }

    item_original_size = Img_GetItemSize(imghd, imgitemhd);
    printf("ota get nor boot0 item_original_size: %ud \n", item_original_size);
    if(!item_original_size)
    {
        printf("sprite update error: fail to get boot0 item size\n");
        ret = -1;
        goto __boot0_err1;
    }

    if(!Img_ReadItem(imghd, imgitemhd, (void *)buffer, item_original_size))
    {
        printf("update error: fail to read data from for boot0\n");
        ret = -1;
        goto __boot0_err1;
    }

    Img_CloseItem(imghd, imgitemhd);
    imgitemhd = NULL;

    flash_handle = open("/dev/mtdblock0",O_RDWR);
    if(flash_handle < 0)
    {
        printf("open flash_handle fail\n");
        return -1;
    }

    ret_len = write(flash_handle,buffer,item_original_size);
    fdatasync(flash_handle);
    close(flash_handle);
    if(ret_len != item_original_size)
    {
        printf("error! ota write boot0 to flash failed! line=%d,ret_len=%d \n",__LINE__,ret_len);
        return -1;
    }

    printf("ota_update_boot0 ok\n");
    return 0;
__boot0_err1:
    if(imgitemhd)
    {
        Img_CloseItem(imghd, imgitemhd);
        imgitemhd = NULL;
    }
    return ret;
}
