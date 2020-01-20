/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file list_viwe.h
 * @brief listview控件
 * @author id:826
 * @version v0.3
 * @date 2016-11-17
 */
#pragma once

#include "system_widget.h"
#include "type/types.h"
//#define SETBG
class ListView
        : public SystemWidget {
    DECLARE_DYNCRT_CLASS(ListView, Runtime)

    public:
        NotifyEvent OnItemClick;

        ListView(View *parent);

        virtual ~ListView();

        virtual void GetCreateParams(CommonCreateParams &params);

        virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        virtual int SetColumns(std::vector<LVCOLUMN> columns, bool auto_width);

        virtual int AddItems(std::vector<LVITEM> items);

        virtual int InsertItemDatas(std::vector<LVSUBITEM> datas, int row);

        virtual int AddItemWithDatas(LVITEM &item, std::vector<LVSUBITEM> datas);

        virtual int RemoveItem(int row);

        virtual int RemoveItem(HLVITEM item);

        virtual int RemoveAllItems();

        virtual int GetSelectedItem(LVITEM &lvitem);

        virtual int GetSelectedItem(HLVITEM &item_handle, LVITEM &lvitem);

        virtual std::string GetItemText(HLVITEM item, int col);

        virtual std::string GetItemText(int row, int col);

        virtual int CancleAllHilight();

        virtual void SetWindowBackImage(const char *bmp);

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        void keyProc(int keyCode, int isLongPress);
        int GetItemCont();
		int GetCurItem();
		int SetCurItem(int curitem);	
	    virtual int SetColHead(std::vector<LVCOLUMN> columns);
        virtual int FillSubItem(LVSUBITEM &data);
        virtual int AddItem(LVITEM &item);

        virtual int UpdateItemData(LVSUBITEM data, int row, int col);
        virtual int SelectItem(int row);
        virtual int SetItemText(std::string text, int row, int col);
        virtual int GetHilight();
        virtual int SetHilight(int row);
        virtual int SetSelectedItemKeyDown();
    private:
        bool isKeyUp;
        WPARAM downKey;
        bool isLongPress;
        int itemCont;
        int hi_idx_;
#ifdef SETBG
	 BITMAP * bg_image_;
#endif
};
