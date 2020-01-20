-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     net_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 
-- version  v0.3
-- date     2016-10-11

network = {
    net_attr = {
        {
            interface = "eth0",
            mac = "78-D0-2B-33-D3-C5",
            ip = "192.168.1.8",
            mask = "255.255.255.255",
            gateway = "192.168.1.1",
            dns1 = "192.168.1.1",
            dns2 = "192.168.1.1",
            dhcp_enable = 1,
        },
        {
            interface = "wlan0",
            mac = "78-D8-2B-66-D8-C8",
            ip = "192.168.1.8",
            mask = "255.255.255.255",
            gateway = "192.168.1.1",
            dns1 = "192.168.1.1",
            dns2 = "192.168.1.1",
            dhcp_enable = 1,
        },
    },
    wifi = {
        ssid = "PD2-IPC-test",
        pwd = "12345678",
        alg_type = 0,
        security = 3,
    },
    softap = {
        ssid_prefix = "V40_IPC_AP",
        pwd = "12345678",
        channel = 8,
        hidden_ssid = 0,
        frequency = 0,
        security = 3,
	},
}
