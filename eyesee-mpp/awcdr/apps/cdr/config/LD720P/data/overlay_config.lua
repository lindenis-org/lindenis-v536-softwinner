-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     overlay_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 
-- version  v0.3
-- date     2016-11-09

overlay = {
    -- 时间osd参数
    time_osd = {
        osd_enable = 1,
        time_format = 0,
        date_format = 0,
        left = 32,
        top  = 16,
    },
    -- 设备osd参数
    device_osd = {
        osd_enable = 1,
        left = 1920,
        top  = 960,
        device_name = "V40_IPC",
    },
	-- GPS osd参数
    GPS_osd = {
        osd_enable = 1,
        left = 1920,
        top  = 960,
    },
    camera = {
        -- cam0 array_id:1
        {
            -- 通道osd参数
            channel_osd = {
                osd_enable = 1,
                left = 500,
                top  = 7000,
                channel_name = "V40_chn0"
            },
            -- 遮盖块osd参数
            cover_osd = {
                -- array_id:1
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 666,
                },
                -- array_id:2
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 123,
                    top  = 567,
                    width = 897,
                    heigth  = 123,
                    color = 33523,
                },
                -- array_id:3
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 265,
                    top  = 678,
                    width = 782,
                    heigth  = 321,
                    color = 876,
                },
                -- array_id:4
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 88373,
                },
            },
        },
        -- cam1 array_id:2
        {
            -- 通道osd参数
            channel_osd = {
                osd_enable = 1,
                left = 500,
                top  = 7000,
                channel_name = "V40_chn0"
            },
            -- 遮盖块osd参数
            cover_osd = {
                -- array_id:1
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 500,
                },
                -- array_id:2
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 500,
                },
                -- array_id:3
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 500,
                },
                -- array_id:4
                {
                    osd_enable = 0,
                    cover_type = 1,
                    left = 500,
                    top  = 500,
                    width = 500,
                    heigth  = 500,
                    color = 88373,
                },
            },
        },
    },
}

