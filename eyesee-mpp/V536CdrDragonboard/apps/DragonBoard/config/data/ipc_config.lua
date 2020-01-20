-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     ipc_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 826
-- version  v0.3
-- date     2016-10-11

system = {
    screen = {
        width = 1280,
        height = 800,
    },
	device = {
		-- 设备名
		device_name = "V40-IPC",
		-- 设备ID
		device_id = 8,
	},
    ntp = {
        -- NTP校时使能标志
        ntp_enable = 0,
        -- NTP端口
        ntp_port = 0,
        -- NTP服务器地址
        ntp_server = "cn.ntp.org.cn",
        -- NTP校时操作时间间隔 单位小时
        interval = 6,
        -- 时区选择
        time_zone = 13,
    },
    uuid = "A5H9X6ZEXT86GJKL111A",
}
