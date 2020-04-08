-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     alarm_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 
-- version  v0.3
-- date     2016-11-09

event = {

alarm_md = {
    camera = {
        -- cam0 array_id:1
        {
            -- 移动侦测使能
            md_enable = 0,
            -- VI通道号
            channel = 0,
            -- 灵敏度
            sensitive = 0,
            -- 报警延时
            delay_time = 20,
            -- 侦测区域
            detect_area = {0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, },
            -- 布防计划使能
            schedule_enable = 0,
            -- 报警联动处理列表
            alarm_handle = {
                snap_enable = 0,
                snap_num = 4,
                snap_chn = 0,

                record_enable = 0,
                record_time = 60,
                record_chn = 0,

                alarm_out_enable = 0,
                alarm_out_chn = 0,
                alarm_time = 30,
                
                ptz_enable = 0,
                ptz_point = 0,
                ptz_delay = 10,
                
                email_enable = 0,
            },
        },
        -- cam1 array_id:2
        {
            -- 移动侦测使能
            md_enable = 0,
            -- VI通道号
            channel = 1,
            -- 灵敏度
            sensitive = 0,
            -- 报警延时
            delay_time = 20,
            -- 侦测区域
            detect_area = {0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 
                          0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff, },
            -- 布防计划使能
            schedule_enable = 0,
            -- 报警联动处理列表
            alarm_handle = {
                snap_enable = 0,
                snap_num = 4,
                snap_chn = 0,

                record_enable = 0,
                record_time = 60,
                record_chn = 0,

                alarm_out_enable = 0,
                alarm_out_chn = 0,
                alarm_time = 30,
                
                ptz_enable = 0,
                ptz_point = 0,
                ptz_delay = 10,
                
                email_enable = 0,
            },
        },
    },
},

alarm_cover = {
    camera = {
        -- cam0 array_id:1
        {
            -- 移动侦测使能
            cover_enable = 0,
            -- VI通道号
            channel = 0,
            -- 灵敏度
            sensitive = 0,
            -- 报警延时
            delay_time = 10,
            -- 布防计划使能
            schedule_enable = 0,
            -- 报警联动处理列表
            alarm_handle = {
                snap_enable = 0,
                snap_num = 4,
                snap_chn = 0,

                record_enable = 0,
                record_time = 60,
                record_chn = 0,

                alarm_out_enable = 0,
                alarm_out_chn = 0,
                alarm_time = 30,
                
                ptz_enable = 0,
                ptz_point = 0,
                ptz_delay = 10,
                
                email_enable = 0,
            },
        },
        -- cam1 array_id:2
        {
            -- 移动侦测使能
            cover_enable = 0,
            -- VI通道号
            channel = 1,
            -- 灵敏度
            sensitive = 0,
            -- 报警延时
            delay_time = 10,
            -- 布防计划使能
            schedule_enable = 0,
            -- 报警联动处理列表
            alarm_handle = {
                snap_enable = 0,
                snap_num = 4,
                snap_chn = 0,

                record_enable = 0,
                record_time = 60,
                record_chn = 0,

                alarm_out_enable = 0,
                alarm_out_chn = 0,
                alarm_time = 30,
                
                ptz_enable = 0,
                ptz_point = 0,
                ptz_delay = 10,
                
                email_enable = 0,
            },
        },
    },
}
}
