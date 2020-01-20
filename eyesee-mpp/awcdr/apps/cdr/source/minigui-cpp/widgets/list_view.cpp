/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file list_viwe.cpp
 * @brief listview控件
 * @author id:826
 * @version v0.3
 * @date 2016-11-17
 */
#include "widgets/list_view.h"

#undef LOG_TAG
#define LOG_TAG "ListView"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(ListView)

ListView::ListView(View *parent)
    :SystemWidget(parent)   
    #ifdef SETBG
    , bg_image_(NULL)
    #endif
{
    isKeyUp = true;
    downKey = 0;
    itemCont = 0;
    hi_idx_ = -1;
    db_msg(" ");
}

ListView::~ListView()
{
#ifdef SETBG
    if (bg_image_) {
        UnloadBitmap(bg_image_);
    }
#endif
}

void ListView::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_LISTVIEW;
    params.alias      = GetClassName();
    //params.style      = WS_VISIBLE | WS_CHILD  | WS_VSCROLL;
    params.style      = WS_VISIBLE | WS_CHILD | LVS_NOTIFY;
    params.exstyle    = WS_EX_USEPARENTFONT;//WS_EX_TRANSPARENT;//WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}


void ListView::keyProc(int keyCode, int isLongPress)
{
    switch(keyCode){
        case SDV_KEY_LEFT:
        {
            int ret = 0;
			db_warn("[debug_jaosn]:SDV_KEY_LEFT GetItemCont() = %d ,itemCont = %d",GetItemCont(),itemCont);
            if(itemCont >= (GetItemCont()-1)){
                itemCont = 0;
            }else{
				//  db_warn("[debug_jaosn]:SDV_KEY_LEFT itemCont = %d ",itemCont);
				itemCont = itemCont+1 ;
			}
			db_warn("[debug_jaosn]:SDV_KEY_LEFT itemCont = %d ",itemCont);
            ret = SendMessage(handle_,LVM_CHOOSEITEM,itemCont,0);
            if(ret < 0)
            {
                db_error("[debug_jaosn]:Chose the listview item");
            }
           // itemCont++;
            break;
        }
        case SDV_KEY_MODE:
            // this->DoHide();
            // static_cast<Window *>(parent_)->DoShow();
            break;
        case SDV_KEY_OK:
            break;
        case SDV_KEY_RIGHT:
        {
            int ret = 0;
            if(itemCont == 0){
                itemCont = GetItemCont() - 1;
            }else{
                //db_error("[debug_jaosn]:@@@@@SDV_KEY_RIGHT itemCont = %d",itemCont);
                itemCont = itemCont - 1;
                //db_error("[debug_jaosn]:@@@@@SDV_KEY_RIGHT itemCont = %d",itemCont);
            }
            //db_error("[debug_jaosn]:SDV_KEY_RIGHT itemCont = %d ",itemCont);
            ret = SendMessage(handle_,LVM_CHOOSEITEM,itemCont,0);
            break;
        }
        default:
            db_msg("[debug_joson]:invild keycode");
            break;
    }
}

int ListView::GetCurItem()
{
	return itemCont;
}

int ListView::SetCurItem(int curitem)
{
	itemCont = curitem;
}

int ListView::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
#ifdef SETBG
        case MSG_ERASEBKGND: {
            HDC hdc = (HDC)wparam;
            const RECT* clip = (const RECT*) lparam;
            BOOL fGetDC = FALSE;
            RECT rcTemp;

            if (hdc == 0) {
                hdc = GetClientDC (hwnd);
                fGetDC = TRUE;
            }

            if (clip) {
                rcTemp = *clip;
                ScreenToClient (hwnd, &rcTemp.left, &rcTemp.top);
                ScreenToClient (hwnd, &rcTemp.right, &rcTemp.bottom);
                IncludeClipRect (hdc, &rcTemp);
            }
            else
                GetClientRect (hwnd, &rcTemp);

            if (bg_image_ != NULL) {
                db_msg("zhb---------retemp.left = %d  retemp.right = %d   RECTW(rcTemp) = %d    RECTH(rcTemp) = %d",rcTemp.left,rcTemp.right,RECTW(rcTemp), RECTH(rcTemp));
                FillBoxWithBitmap (hdc, 0, 0,
                                   RECTW(rcTemp), RECTH(rcTemp), bg_image_);
            }else {                    // <C7><E5><B3><FD><CE><DE>Ч<C7><F8><D3><F2>, <C8><E7><B9><FB>û<D3><D0>Ϊ<B4><B0><BF><DA><C9><E8><D6>ñ<B3><BE><B0>ͼƬ<A3><AC><D4><F2><D2><D4><U+0378><C3><U+1F1CFE><B0><CC><EE><B3><E4>
                       db_msg("zhb-------------111--------MSG_ERASEBKGND");
                                       SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x0d, 0x02, 0x46));
                       FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
                       }

            if (fGetDC)
                ReleaseDC (hdc);
            return 0;
        }
#endif
        case MSG_KEYUP:
            {
                db_msg("[debug_jaosn]:short MSG_KEYDOWN");
                keyProc(wparam, SHORT_PRESS);
            }
            break;
#if 0
		case MSG_KEYDOWN:
            {
                db_msg("[debug_jaosn]:short MSG_KEYDOWN");
                keyProc(wparam, SHORT_PRESS);
            }
            break;

        case MSG_KEYUP:
            {
                db_msg("[debug_jaosn]:short MSG_KEYDOWN");
                if(isKeyUp == true) {
                    downKey = wparam;
                    SetTimer(hwnd, ID_LISTVIEW_TIMER_KEY, LONG_PRESS_TIME);
                    isKeyUp = false;
                }
                break;
            }
        case MSG_KEYLONGPRESS:
            {
                db_msg("[debug_jaosn]:long press\n");
                downKey = -1;
                keyProc(wparam, LONG_PRESS);
                break;
            }
        case MSG_TIMER:
            {
                if(wparam == ID_LISTVIEW_TIMER_KEY) {
                    db_msg("[debug_jaosn]:short MSG_TIMER");
                    isKeyUp = true;
                    SendMessage(hwnd, MSG_KEYLONGPRESS, downKey, 0);
                    KillTimer(hwnd, ID_LISTVIEW_TIMER_KEY);
                }
                break;
            }
#endif
        case MSG_LBUTTONDOWN:
			break;
        case MSG_LBUTTONUP:
            break;
        case MSG_MOUSEMOVE:
            break;
        default:
            return SystemWidget::HandleMessage( hwnd, message, wparam, lparam );
            break;

    }
    return HELP_ME_OUT;
}

int ListView::OnMouseUp(unsigned int button_status, int x, int y)
{
    #if 0
    db_msg("button status: %d, x: %d, y: %d", button_status, x, y);

    if(OnItemClick)
        OnItemClick(this);

     return HELP_ME_OUT;
    #endif
 }

int ListView::SetColumns(std::vector<LVCOLUMN> columns, bool auto_width)
{
    int ret = 0;
    vector<LVCOLUMN>::iterator it;

    // set column/head height
    ret = SendMessage (handle_, LVM_SETHEADHEIGHT, 0, 0);
    if (ret < 0) {
        db_error("set column head height failed");
    }

    // set column data
    for (it = columns.begin(); it != columns.end(); ++it) {
        if (auto_width || it->width <= 0) {
            RECT rect;
            this->GetRect(&rect);
            it->width = RECTW(rect) / columns.size();
            db_msg("column width: %d", it->width);
        }

        it->nCols = it - columns.begin();

        ret = SendMessage (handle_, LVM_ADDCOLUMN, 0, (LPARAM) & (*it));
        if (ret < 0) {
            db_error("add column failed");
            break;
        }
    }

    return ret;
}

int ListView::SetColHead(std::vector<LVCOLUMN> columns)
{
	//LVM_MODIFYHEAD
	 int ret = 0;
        vector<LVCOLUMN>::iterator it;	
	 for (it = columns.begin(); it != columns.end(); ++it) {
        it->nCols = it - columns.begin();
        ret = SendMessage (handle_, LVM_MODIFYHEAD, 0, (LPARAM) & (*it));
        if (ret < 0) {
            db_error("SetColHead  failed");
            break;
        }
    }
    return ret;
}


int ListView::AddItems(std::vector<LVITEM> items)
{
    int ret = 0;

    vector<LVITEM>::iterator it;
    for ( it = items.begin(); it != items.end(); ++it) {

        it->nItem = it - items.begin();
        ret = SendMessage (handle_, LVM_ADDITEM, 0, (LPARAM) & (*it));

        if (ret == 0) {
            db_error("add item failed, ret: %d", ret);
        }
    }

    return ret;
}

int ListView::SetItemText(std::string text, int row, int col)
{
    int ret = 0;

    LVSUBITEM data;
    data.nItem = row;
    data.subItem = col;
    data.pszText = const_cast<char*>(text.c_str());

    ret = SendMessage (handle_, LVM_SETSUBITEMTEXT, 0, (LPARAM) & (data));
    if (ret < 0) {
        db_error("set item text failed, ret: %d", ret);
    }

    return ret;
}

int ListView::AddItem(LVITEM &item)
{
   int ret = 0;
    ret = SendMessage (handle_, LVM_ADDITEM, 0, (LPARAM) & item);

    if (ret == 0) {
        db_error("add item failed, ret: %d", ret);
        return ret;
    }
    
    return ret;
}


int ListView::FillSubItem(LVSUBITEM &data)
{
    int ret = 0;
    ret = SendMessage (handle_, LVM_FILLSUBITEM, 0, (LPARAM) &data);

    if (ret < 0) {
        db_error("LVM_FILLSUBITEM failed, ret: %d", ret);
        return ret;
    }
    
    return ret;
}

int ListView::UpdateItemData(LVSUBITEM data, int row, int col)
{
    int ret = 0;

    data.nItem = row;
    data.subItem = col;

    ret = SendMessage (handle_, LVM_SETSUBITEM, 0, (LPARAM) & (data));
    if (ret < 0) {
        db_error("update item data failed, ret: %d", ret);
    }

    return ret;
}

int ListView::InsertItemDatas(std::vector<LVSUBITEM> datas, int row)
{
    int ret = 0;

    vector<LVSUBITEM>::iterator it;
    for ( it = datas.begin(); it != datas.end(); ++it) {
        it->nItem = row;
        it->subItem = it - datas.begin();
        ret = SendMessage (handle_, LVM_SETSUBITEM, 0, (LPARAM) & (*it));
	
        if (ret < 0) {
            db_error("insert item datas failed, ret: %d", ret);
        }
    }

    return ret;
}

int ListView::AddItemWithDatas(LVITEM &item, std::vector<LVSUBITEM> datas)
{
    int ret = 0;

    ret = SendMessage (handle_, LVM_ADDITEM, 0, (LPARAM) & item);

    if (ret == 0) {
        db_error("add item failed, ret: %d", ret);
        return ret;
    }

    ret = InsertItemDatas(datas, item.nItem);

    return ret;
}

int ListView::RemoveItem(int row)
{
    int ret = 0;

    ret = SendMessage(handle_, LVM_DELITEM, (WPARAM)row, 0);

    if (ret < 0) {
        db_error("remove item [%d] failed, ret: %d", row, ret);
    }

    return ret;
}

int ListView::RemoveItem(HLVITEM item)
{
    int ret = 0;

    ret = SendMessage(handle_, LVM_DELITEM, 0, (LPARAM)item);

    if (ret < 0) {
        db_error("remove item [%d] failed, ret: %d", item, ret);
    }

    return ret;
}

int ListView::RemoveAllItems()
{
    int ret = 0;

    ret = SendMessage(handle_, LVM_DELALLITEM, 0, 0);

    if (ret < 0) {
        db_error("remove all item failed, ret: %d", ret);
    }

    return ret;
}


int ListView::GetItemCont()
{
    int ret = 0;
    ret = SendMessage(handle_,LVM_GETITEMCOUNT,0,0);
    db_msg("[debug_joasn]: item cont is %d",ret);
    if(ret <= 0)
    {
        db_error("[debug_jaosn]: get item is filed");
        return -1;
    }

    return ret;
}

void ListView::SetWindowBackImage(const char *bmp)
{
#ifdef SETBG

    if (bmp == NULL) {
        db_error("failed, bmp = null");
        return;
    }

    if (bg_image_ == NULL) {
        bg_image_ = (BITMAP*)malloc(sizeof(BITMAP));
    }

    LoadBitmap(HDC_SCREEN, bg_image_, bmp);
#endif
}


int ListView::CancleAllHilight()
{
    return 0;
}
int ListView::SelectItem(int row)
{
    int ret = 0;

    ret = SendMessage(handle_, LVM_CHOOSEITEM, row, 0);
    if (ret < 0) {
        db_error("set select item failed");
    } else {
        hi_idx_ = row;
    }

    return ret;
}
int ListView::SetSelectedItemKeyDown()
{
    SendMessage(handle_, MSG_KEYDOWN, 0, 0);
    return 0;
}

int ListView::GetSelectedItem(LVITEM &lvitem)
{
    HLVITEM item_handle;

    return GetSelectedItem(item_handle, lvitem);
}

int ListView::GetSelectedItem(HLVITEM &item_handle, LVITEM &lvitem)
{
    item_handle = (HLVITEM)SendMessage(handle_, LVM_GETSELECTEDITEM, 0, (LPARAM) &lvitem);

    if (item_handle == 0) {
        db_error("get selected item failed, item handle == 0");
        return -1;
    }

    return 0;
}

string ListView::GetItemText(HLVITEM item, int col)
{
    int ret = 0;
    LVSUBITEM item_data;

    item_data.subItem = col;

    int len = SendMessage(handle_, LVM_GETSUBITEMLEN, (WPARAM)item, (LPARAM) &item_data);

    if (len <= 0) {
        db_warn("why subitem len <= 0 ?");
        return "";
    }

    item_data.pszText = new char[len + 1];
    if (item_data.pszText == nullptr) {
        db_warn("alloc failed");
        return "";
    }
    ret = SendMessage(handle_, LVM_GETSUBITEMTEXT, (WPARAM)item, (LPARAM) &item_data);

    if (ret < 0) {
        db_error("get item sub: %d item text failed, ret: %d", col, ret);
        return string();
    }

    string text_str = item_data.pszText;
    delete []item_data.pszText;

    return text_str;
}

string ListView::GetItemText(int row, int col)
{
    int ret = 0;
    LVSUBITEM item_data;

    item_data.nItem = row;
    item_data.subItem = col;

    int len = SendMessage(handle_, LVM_GETSUBITEMLEN, 0, (LPARAM) &item_data);

    if (len <= 0) {
        db_warn("why subitem len <= 0 ?");
        return "";
    }

    item_data.pszText = new char[len + 1];
    if (item_data.pszText == nullptr) {
        db_warn("alloc failed");
        return "";
    }
    ret = SendMessage(handle_, LVM_GETSUBITEMTEXT, 0, (LPARAM) &item_data);

    if (ret < 0) {
        db_error("get row: %d, sub: %d item text failed, ret: %d", row, col, ret);
    }

    string text_str = item_data.pszText;
    delete []item_data.pszText;

    return text_str;
}
int ListView::GetHilight()
{
    return hi_idx_;
}

int ListView::SetHilight(int row)
{
    int ret = 0;
   
    ret = SendMessage(handle_, LVM_SELECTITEM, row, 0);
    if (ret < 0) {
        db_error("set select item failed");
    } else {
        ret = SendMessage(handle_, LVM_SHOWITEM, row, 0);
        if (ret < 0) {
            db_error("show item failed");
        } else {
            hi_idx_ = row;
        }
    }

    return ret;
}

