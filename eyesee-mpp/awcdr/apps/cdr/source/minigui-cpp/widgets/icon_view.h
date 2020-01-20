/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file icon_viwe.h
 * @brief iconview控件
 * @author sh
 * @version v2.0
 * @date 2018-01-09
 */
#pragma once

#include "system_widget.h"
#include "type/types.h"

typedef fastdelegate::FastDelegate2<View*, int> clickEvent;

class IconView
        : public SystemWidget {
    DECLARE_DYNCRT_CLASS(IconView, Runtime)

    public:
        clickEvent onIconClickEvent;
        IconView(View *parent);
        virtual ~IconView();
        virtual void GetCreateParams(CommonCreateParams &params);
        
        virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                   LPARAM lparam);
        virtual int AddIconViewItems(const std::vector<IVITEMINFO> &iconitems);
        virtual int AddIconViewItem(const IVITEMINFO &iconitem);
        virtual int SetIconSize(const int &iconwidth, const int &iconheight);
        virtual int RemoveAllIconItem();
        virtual int RemoveIconItem(int icon_index);
        virtual int SetScrollWidth();//
        virtual int SetScrollHeight();
        virtual int SetIconHighlight(int icon_index, BOOL bvisible);
        virtual int GetIconHighlight();
        virtual int SelectIconItem(int icon_index, BOOL bSel);
        virtual int ChooseIconItem(int icon_index);
        virtual int GetIconItemCount();
        virtual int SetIconMargins();
        virtual void SetBackColor(DWORD new_color);
        virtual void SetCaptionColor(DWORD new_color);
		virtual int SetIconItem_Icon(PBITMAP pbitbmp, int icon_index_ex, int id);
		virtual int SetIconItem_Bmp(PBITMAP pbitbmp, int icon_index);
		virtual int SetIconItem_select(int icon_index, int id);
		virtual int SetIconItem_selectall(int id);
		virtual int SetIconItem_lock(int icon_index, int id);

		virtual int GetIconItem_select(int icon_index);
		virtual int GetIconItem_FirstVisable();
        virtual int OnMouseUp(unsigned int button_status, int x, int y);
    private:
        int hilighted_idx_;
        int model_id;
};
