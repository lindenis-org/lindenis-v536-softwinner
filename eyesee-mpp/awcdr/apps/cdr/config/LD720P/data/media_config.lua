-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     ipc_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 826
-- version  v0.3
-- date     2016-10-11

media = {
    camera = {
        -- cam0
        {
            width = 1920,
            height = 1080,
            -- video encoder config
            venc = {
                -- encode format, 0-h264, 1-h265
                format = 0,
                -- encode type, 0-cbr, 1-vbr
                type = 0,
                -- encode quality leve,  0~2
                quality = 0,
                -- encode bps for 4 chn, size for kbps
                bps = {
                    4194304,
                    2097152,
                    1048576,
                    524288,
                },
                -- encode fps for 4 chn
                fps = {
                    25,
                    25,
                    25,
                    25,
                },
                -- encode gop for 4 chn
                gop = {
                    25,
                    50,
                    50,
                    25,
                },
                -- encode size
                size = {
                    -- 1080P
                    {
                        width = 1920,
                        height = 1080,
                    },
                    -- 720P
                    {
                        width = 1280,
                        height = 720,
                    },
                    -- VGA
                    {
                        width = 640,
                        height = 360,
                    },
                    -- CIF
                    {
                        width = 360,
                        height = 288,
                    },
                },
            },
        },
        -- cam1
        {
            width = 1920,
            height = 1080,
            -- video encoder config
            venc = {
                -- encode format, 0-h264, 1-h265
                format = 0,
                -- encode type, 0-cbr, 1-vbr
                type = 0,
                -- encode quality leve,  0~2
                quality = 0,
                -- encode bps for 4 chn, size for kbps
                bps = {
                    4194304,
                    2097152,
                    1048576,
                    524288,
                },
                -- encode fps for 4 chn
                fps = {
                    25,
                    25,
                    25,
                    25,
                },
                -- encode gop for 4 chn
                gop = {
                    25,
                    50,
                    50,
                    25,
                },
                -- encode size
                size = {
                    -- 1080P
                    {
                        width = 1920,
                        height = 1080,
                    },
                    -- 720P
                    {
                        width = 1280,
                        height = 720,
                    },
                    -- VGA
                    {
                        width = 640,
                        height = 360,
                    },
                    -- CIF
                    {
                        width = 360,
                        height = 288,
                    },
                },
            },
        },
    },
}
