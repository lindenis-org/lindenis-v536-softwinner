/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file magic_block.cpp
 * @brief 用于创建动态区块的控件
 *
 *  区块可以在限定区域内任意移动和缩放, 区块的形状可以是矩形框、 矩形块
 *  或其它形状
 *
 * @author id:826
 * @version v0.3
 * @date 2017-02-10
 */

#include "widgets/magic_block.h"
#include "debug/app_log.h"
#include "memory/x_memory.h"

#undef LOG_TAG
#define LOG_TAG "MagicBlock"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(MagicBlock)

MagicBlock::MagicBlock(View *parent)
    : CustomWidget(parent)
    , pen_color_(PIXEL_red)
    , brush_color_(PIXEL_red)
    , block_type_(RECTANGLE)
{
}

MagicBlock::~MagicBlock()
{

}

void MagicBlock::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_MAGIC_BLOCK;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | WS_CHILD;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int MagicBlock::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    switch(message) {
        case MSG_PAINT: {
              HDC hdc = ::BeginPaint(hwnd);
              RECT rect;
              ::GetWindowRect(hwnd, &rect);

              switch (block_type_) {
                  case RECTANGLE:
                      ::SetPenColor(hdc, pen_color_);
                      ::Rectangle(hdc, 0, 0, RECTW(rect) - 1, RECTH(rect) - 1);
                      break;
                  case BOX:
                      ::SetBrushColor(hdc, brush_color_);
                      ::FillBox(hdc, 0, 0, RECTW(rect), RECTH(rect));
                      break;
                  default:
                      break;
              }

              ::EndPaint(hwnd, hdc);
          }
          break;
        default:
            return CustomWidget::HandleMessage(hwnd, message, wparam, lparam);
            break;
    }
    return HELP_ME_OUT;
}

void MagicBlock::MoveAndResize(RECT &rect)
{
    View::Resize(&rect);
}

void MagicBlock::SetBlockPenColor(DWORD color)
{
    pen_color_ = color;
}

void MagicBlock::SetBlockBrushColor(DWORD color)
{
    brush_color_ = color;
}

void MagicBlock::SetBlockType(BlockType type)
{
    block_type_ = type;
}
