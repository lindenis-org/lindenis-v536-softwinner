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

#ifndef __OTA_COMMON_H____
#define __OTA_COMMON_H____
enum OTA_RETURN {
    OTA_UPDATE_FAIL = 0,
    OTA_UPDATE_SUCCESS,
    OTA_MD5_CHECK_FAIL ,
    OTA_FOUND_IMG_FAIL,
};
#define MOUNT_PATH     "/mnt/extsd"
#define VERSION_4G_DIR_NET ".tmp/.net_4g_version"

#endif //__OTA_COMMON_H____
