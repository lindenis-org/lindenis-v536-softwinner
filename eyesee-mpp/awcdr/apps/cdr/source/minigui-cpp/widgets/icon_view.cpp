/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file icon_viwe.cpp
 * @brief iconview控件
 * @author sh
 * @version v2.0
 * @date 2018-01-09
 */
 #include "widgets/icon_view.h"

 #undef LOG_TAG
 #define LOG_TAG "IconView"

 using namespace std;

 IMPLEMENT_DYNCRT_CLASS(IconView)


 IconView::IconView(View *parent)
    :SystemWidget(parent)
 {
    hilighted_idx_ = -1;
    model_id      = 0;
    db_msg(" ");
 }

 IconView::~IconView()
 {

 }

 void IconView::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_ICONVIEW;
    params.alias      = GetClassName();
    //params.style      = WS_VISIBLE | WS_CHILD  | WS_VSCROLL;
    //params.style      = WS_VISIBLE | WS_CHILD;
	params.style      = WS_VISIBLE | WS_CHILD | IVS_UPNOTIFY;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

int IconView::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                               LPARAM lparam)
{
	   switch (message) {

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
   			   #if 0
			   if (bg_image_ != NULL) {
				   db_msg("zhb---------retemp.left = %d  retemp.right = %d	 RECTW(rcTemp) = %d    RECTH(rcTemp) = %d",rcTemp.left,rcTemp.right,RECTW(rcTemp), RECTH(rcTemp));
				   FillBoxWithBitmap (hdc, 0, 0,
									  RECTW(rcTemp), RECTH(rcTemp), bg_image_);
			   }else {					  // <C7><E5><B3><FD><CE><DE>效<C7><F8><D3><F2>, <C8><E7><B9><FB>没<D3><D0>为<B4><B0><BF><DA><C9><E8><D6>帽<B3><BE><B0>图片<A3><AC><D4><F2><D2><D4><U+0378><C3><U+1F1CFE><B0><CC><EE><B3><E4>
						  db_msg("zhb-------------111--------MSG_ERASEBKGND");
										  SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x0d, 0x02, 0x46));
						  FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
						  }
   			#else
			SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x00, 0x00, 0x00));
			FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
			#endif
			   if (fGetDC)
				   ReleaseDC (hdc);
			   return 0;
		   }
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
		   		db_error("iconView::MSG_LBUTTONDOWN\n");
			   break;
		   case MSG_LBUTTONUP:
		   		db_error("iconView::MSG_LBUTTONDOWN\n");
			   break;
		   case MSG_MOUSEMOVE:
			   //db_warn("[habo]--->MSG_LBUTTONUP \n");
			   break;
		   default:
			   return SystemWidget::HandleMessage( hwnd, message, wparam, lparam );
			   break;
   
	   }
	   return HELP_ME_OUT;
}

int IconView::SetIconSize(const int &iconwidth, const int &iconheight)
{
    int ret        = 0;

    ret = SendMessage(handle_, IVM_SETITEMSIZE, iconwidth, iconheight);
    if (ret < 0) {
        db_error("set the icon size failed");
    }
    return ret;
}


 int IconView::AddIconViewItems(const std::vector<IVITEMINFO> &iconitems)
 {
    int ret = 0;

    for (auto iter : iconitems) {
        ret = SendMessage(handle_, IVM_ADDITEM, 0, (LPARAM)&iter);
        if (ret < 0) {
            db_error("add the icon item failed, ret: %d", ret);
            break;
        }
    }

    return ret;
 }


int IconView::AddIconViewItem(const IVITEMINFO &iconitem)
{
    int ret = 0;
    ret = SendMessage(handle_, IVM_ADDITEM, 0, (LPARAM) &(iconitem));

    if (ret < 0) {
        db_error("add the icon item failed, ret: %d", ret);
    }
    return ret;
}


int IconView::RemoveAllIconItem()
{
    int ret = 0;

    ret = SendMessage(handle_, IVM_RESETCONTENT, 0, 0);

    if (ret < 0) {
        db_error("remove all icon item failed, ret: %d", ret);
    }

    return ret;
}

int IconView::RemoveIconItem(int icon_index)
{
    int ret = 0;

    ret = SendMessage(handle_, IVM_DELITEM, icon_index, 0);

    if (ret < 0) {
        db_error("remove item [%d] failed, ret: %d", icon_index, ret);
    }

    return ret;
}

int IconView::SetScrollWidth()
{
    int ret = 0;
    int scroll_width = 0;

    ret = SendMessage(handle_, IVM_SETCONTWIDTH, scroll_width, 0);

    if (ret < 0) {
        db_error("set the scollor width failed, ret: %d", ret);
    }

    return ret;
}

int IconView::SetScrollHeight()
{
    int ret = 0;
    int scroll_height = 180;

    ret = SendMessage(handle_, IVM_SETCONTWIDTH, scroll_height, 0);

    if (ret < 0) {
        db_error("set the scollor height failed, ret: %d", ret);
    }

    return ret;
}

int IconView::SetIconHighlight(int icon_index, BOOL bvisible)
{
    int ret = 0;

    ret = SendMessage(handle_, IVM_SETCURSEL, icon_index, bvisible);

    if (ret < 0) {
        db_error("set the highlight icon failed, ret: %d", ret);
    }

    return ret;
}

int IconView::GetIconHighlight()
{
    hilighted_idx_ = SendMessage(handle_, IVM_GETCURSEL, 0, 0);
    if (hilighted_idx_ < 0) {
        db_error("get the highlight icon failed, hilighted_idx_: %d", hilighted_idx_);
    }
    return hilighted_idx_;
}

int IconView::SelectIconItem(int icon_index, BOOL bSel)
{
    int ret = 0;

    ret = SendMessage(handle_, IVM_SELECTITEM, icon_index, bSel);

    if (ret < 0) {
        db_error("select icon item failed, ret: %d", ret);
    }

    return ret;
}

int IconView::SetIconMargins()
{
    int ret = 0;
    RECT rcMargin;
    rcMargin.bottom = 0;
    rcMargin.top    = 0;
    rcMargin.left   = 0;
    rcMargin.right  = 0;
    ret = SendMessage(handle_, SVM_SETMARGINS, 0, (LPARAM)&rcMargin);

    if (ret < 0) {
        db_error("set icon margins failed, ret: %d", ret);
    }
    return ret;
}


int IconView::ChooseIconItem(int icon_index)
{
    int ret = 0;

    ret = SendMessage(handle_, IVM_CHOOSEITEM, icon_index, 0);

    if (ret < 0) {
        db_error("choose icon item failed, ret: %d", ret);
    }

    return ret;
}

int IconView::GetIconItemCount()
{
    int count = 0;
    count = SendMessage(handle_, IVM_GETITEMCOUNT, 0, 0);
    return count;
}

void IconView::SetBackColor(DWORD new_color)
{
    ::SetWindowElementAttr(handle_, WE_BGC_HIGHLIGHT_ITEM, 0xFFD1CD00);
    Widget::SetBackColor(new_color);
}


void IconView::SetCaptionColor(DWORD new_color)
{
    ::SetWindowElementAttr(handle_, WE_FGC_WINDOW, new_color);
    ::SetWindowElementAttr(handle_, WE_FGC_HIGHLIGHT_ITEM, new_color);
}

int IconView::OnMouseUp(unsigned int button_status, int x, int y)
{
	db_error("IconView::OnMouseUp\n");
    //if (onIconClickEvent) {
    //    onIconClickEvent(this, GetIconHighlight());		// PlaybackWindow::thumbIconItemClickProc
    //}

    //return View::OnMouseUp(button_status, x, y);
}

int IconView::SetIconItem_Bmp(PBITMAP pbitbmp, int icon_index)
{
    int ret = 0;
    ret = SendMessage(handle_, IVM_SETITEMBMP, icon_index, (LPARAM)pbitbmp);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }
    return ret;
}

int IconView::SetIconItem_Icon(PBITMAP pbitbmp, int icon_index, int id)
{
    int ret = 0;

	if (id < 0 || id > 2) return -1;
	DWORD index = ((DWORD)id << 24) + (DWORD)icon_index;
	//db_error("IVM_SETITEMICON %08x",index );
    ret = SendMessage(handle_, IVM_SETITEMICON, index, (LPARAM)pbitbmp);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;
}
int IconView::SetIconItem_select(int icon_index, int id)
{
	int ret = 0;

    ret = SendMessage(handle_, IVM_SETITEM_SELECT, icon_index, (LPARAM)id);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;
}
int IconView::GetIconItem_select(int icon_index)
{
	int ret = 0;

    ret = SendMessage(handle_, IVM_GETITEM_SELECT, icon_index, 0);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;
}

int IconView::GetIconItem_FirstVisable()
{
	int ret = 0;

    ret = SendMessage(handle_, IVM_GETFIRSTVISIBLEITEM, 0, 0);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;
}

int IconView::SetIconItem_selectall(int id)
{
	int ret = 0;

    ret = SendMessage(handle_, IVM_SETITEM_SELECTALL, 0, (LPARAM)id);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;
}

int IconView::SetIconItem_lock(int icon_index, int id)
{
	int ret = 0;

    ret = SendMessage(handle_, IVM_SETITEM_LOCK, icon_index, (LPARAM)id);

    if (ret < 0) {
        db_error("SetIconItem_bmp failed, ret: %d", ret);
    }

    return ret;	
}

