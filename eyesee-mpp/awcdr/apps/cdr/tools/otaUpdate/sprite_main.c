#include "ota_private.h"
#include "sunxi_mbr.h"
#include "imgdecode.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "app_log.h"



#define db_msg printf
//#define OTA_UPDATE_BOOT

void __dump_dlmap(sunxi_download_info *dl_info)
{
    dl_one_part_info        *part_info;
    u32 i;
    char buffer[32];

    printf("*************DOWNLOAD MAP DUMP************\n");
    printf("total download part %ud\n", dl_info->download_count);
    printf("\n");
    for(part_info = dl_info->one_part_info, i=0;i<dl_info->download_count;i++, part_info++)
    {
        memset(buffer, 0, 32);
        memcpy(buffer, part_info->name, 16);
        printf("download part[%ud] name          :%s\n", i, buffer);
        //memset(buffer, 0, 32);
        memcpy(buffer, part_info->dl_filename, 16);
        printf("download part[%ud] download file :%s\n", i, buffer);
        //memset(buffer, 0, 32);
        memcpy(buffer, part_info->vf_filename, 16);
        printf("download part[%ud] verify file   :%s\n", i, buffer);
        printf("download part[%ud] lenlo         :0x%x\n", i, part_info->lenlo);
        printf("download part[%ud] addrlo        :0x%x\n", i, part_info->addrlo);
        printf("download part[%ud] encrypt       :0x%x\n", i, part_info->encrypt);
        printf("download part[%ud] verify        :0x%x\n", i, part_info->verify);
        printf("\n");
    }
}


void __dump_mbr(sunxi_mbr_t *mbr_info)
{
    sunxi_partition         *part_info;
    u32 i;
    char buffer[32] = {0};

    printf("*************MBR DUMP***************\n");
    printf("total mbr part %ud\n", mbr_info->PartCount);
    printf("\n");
    for(part_info = mbr_info->array, i=0;i<mbr_info->PartCount;i++, part_info++)
    {
        memset(buffer, 0, 32);
        memcpy(buffer, part_info->name, 16);
        printf("part[%ud] name      :%s\n", i, buffer);
        //memset(buffer, 0, 32);
        memcpy(buffer, part_info->classname, 16);
        printf("part[%ud] classname :%s\n", i, buffer);
        printf("part[%ud] addrlo    :0x%x\n", i, part_info->addrlo);
        printf("part[%ud] lenlo     :0x%x\n", i, part_info->lenlo);
        printf("part[%ud] user_type :0x%x\n", i, part_info->user_type);
        printf("part[%ud] keydata   :0x%x\n", i, part_info->keydata);
        printf("part[%ud] ro        :0x%x\n", i, part_info->ro);
        printf("\n");
    }
}

static int ota_update_boot()
{
    uchar  img_mbr[32 * 1024] = {0};

#ifdef OTA_UPDATE_BOOT
    if(ota_update_boot0())
    {
        printf("error : download boot0 error\n");
        return -1;
    }

    if(ota_update_uboot())
    {
        printf("error : download uboot error\n");
        return -1;
    }
#else
    printf("don't not update boot\n");
#endif
    if(ota_read_mbr(&img_mbr))
    {
        printf("error : fetch mbr error \n");
        return -1;
    }
    int mbr_num = 1;
    if(ota_write_mbr(&img_mbr, sizeof(sunxi_mbr_t) * mbr_num))
    {
        printf("error: download mbr err \n");
        return -1;
    }
    __dump_mbr((sunxi_mbr_t *)img_mbr);

    return 0;
}

int ota_main(char *image_path, update_part_flag ota_partition_flag)
{

    if(ota_open_firmware(image_path)< 0)
    {
        printf("error, get image file failed!\n");
        return -1;
    }

    sunxi_download_info  dl_map;
    memset(&dl_map, 0, sizeof(sunxi_download_info));
    if(ota_get_partition_info(&dl_map)<0)
    {
        printf("error : fetch download map error\n");
        return -1;
    }

    __dump_dlmap(&dl_map);
    if(ota_update_normal_part(&dl_map, ota_partition_flag))
    {
        printf("error : download part error\n");
        return -1;
    }
 
#ifdef OTA_UPDATE_BOOT
    if(ota_update_boot())
    {
        printf("error : download uboot error\n");
        return -1;
    }
#endif
    return 0;
}
