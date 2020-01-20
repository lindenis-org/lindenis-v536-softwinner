/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __OTA_PRIVATE_H____
#define __OTA_PRIVATE_H____

#include "sunxi_mbr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int update_uboot_flag;
    int update_kernel_flag;
    int update_rootfs_flag;
    int update_boot_logo_flag;
    int update_env_flag;
    int update_overlay_flag;
    int update_custom_flag;
}update_part_flag;

int ota_open_firmware(char *name);
int ota_get_partition_info(sunxi_download_info  *dl_map);

int ota_read_mbr(void  *img_mbr);
int ota_write_mbr(void  *img_mbr, unsigned int mbr_size);

int ota_update_normal_part(sunxi_download_info *dl_map, update_part_flag ota_partition_flag);
int ota_update_uboot();
int ota_update_boot0();

int ota_main(char *image_path, update_part_flag ota_partition_flag);

#ifdef __cplusplus
}
#endif
#endif //__OTA_PRIVATE_H____
