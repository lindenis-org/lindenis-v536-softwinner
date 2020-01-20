/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file magic_block.h
 * @brief 用于创建动态区块的控件
 *
 *  区块可以在限定区域内任意移动和缩放, 区块的形状可以是矩形框、 矩形块
 *  或其它形状
 *
 * @author id:826
 * @version v0.3
 * @date 2017-02-10
 */
#pragma once

#include "widgets/widgets.h"
#include "type/types.h"

class MagicBlock
    : public CustomWidget
{
        DECLARE_DYNCRT_CLASS(MagicBlock, Runtime)
    public:
        enum BlockType {
            RECTANGLE = 0,
            BOX,
        };

        MagicBlock(View *parent);

        virtual ~MagicBlock();

        virtual void GetCreateParams(CommonCreateParams &params);

        virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam);
        void MoveAndResize(RECT &rect);

        void SetBlockPenColor(DWORD color);

        void SetBlockBrushColor(DWORD color);

        void SetBlockType(BlockType type);

    private:
        DWORD pen_color_;
        DWORD brush_color_;
        BlockType block_type_;
};
