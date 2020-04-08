-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     record_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 826
-- version  v0.3
-- date     2016-11-09

record = {
    camera = {
        -- cam0 array_id:1
        {
            -- 录像的VI通道
            channel = 0,
            -- 录像类型
            record_type = 0,
            -- 所录像的码流类型
            stream_type = 0,
            -- 音频录像使能
            audio_enable = 0,
            -- 循环覆盖使能
            cover_enable = 1,
            -- 录像文件分包时间
            pack_time = 0,
            -- 预录时间
            perrecord_time = 0,
        },
        -- cam1 array_id:2
        {
            -- 录像的VI通道
            channel = 1,
            -- 录像类型
            record_type = 0,
            -- 所录像的码流类型
            stream_type = 0,
            -- 音频录像使能
            audio_enable = 0,
            -- 循环覆盖使能
            cover_enable = 1,
            -- 录像文件分包时间
            pack_time = 0,
            -- 预录时间
            perrecord_time = 0,
        },
    },
}

snap = {
    camera = {
        -- cam0 array_id:1
        {
            -- 抓拍的VI通道
            channel = 0,
            -- 抓拍图像质量
            quality = 0,
            -- 抓拍图像的宽度
            pic_wide = 0,
            -- 抓拍图像的高度
            pic_high = 0,
            -- 抓拍延迟ms
            snap_delay = 1,
            -- 抓拍图片数量
            snap_num = 0,
            -- 连续抓拍间隔
            interval = 0,
        },
        -- cam1 array_id:2
        {
            -- 抓拍的VI通道
            channel = 1,
            -- 抓拍图像质量
            quality = 0,
            -- 抓拍图像的宽度
            pic_wide = 0,
            -- 抓拍图像的高度
            pic_high = 0,
            -- 抓拍延迟ms
            snap_delay = 1,
            -- 抓拍图片数量
            snap_num = 0,
            -- 连续抓拍间隔
            interval = 0,
        },
    },
}
