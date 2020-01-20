/*
 *   This file is part of MiniGUI, a mature cross-platform windowing 
 *   and Graphics User Interface (GUI) support system for embedded systems
 *   and smart IoT devices.
 * 
 *   Copyright (C) 2002~2018, Beijing FMSoft Technologies Co., Ltd.
 *   Copyright (C) 1998~2002, WEI Yongming
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   Or,
 * 
 *   As this program is a library, any link to this program must follow
 *   GNU General Public License version 3 (GPLv3). If you cannot accept
 *   GPLv3, you need to be licensed from FMSoft.
 * 
 *   If you have got a commercial license of this program, please use it
 *   under the terms and conditions of the commercial license.
 * 
 *   For more information about the commercial license, please refer to
 *   <http://www.minigui.com/en/about/licensing-policy/>.
 */
/*
** iconview.c: an iconview control
**
** Craeted By Zhong Shuyi at 2004/03/01.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include <sys/time.h>

#ifdef _MGCTRL_ICONVIEW
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"
#include "ctrl/iconview.h"
#include "scrolled.h"
#include "listmodel.h"
#include "iconview_impl.h"


#define USE_AVERAGE 0
#if 1
#define ID_INERTIAL_FLING_TIMER2 2019

static struct timeval ivlast_move_time;
static int ivlast_move_start = 0;
static BOOL ivstop_inertia_move = FALSE;
static float ivcur_acceleration = 0.0f;
static float ivduration = 0;
static int ivdirection = 0;
BOOL ivmove_flag = FALSE;
int ivmouse_updown_flag = 0;
#endif

/* ------------------------------------ ivlist --------------------------- */

static void freeIvItem (IconviewItem *pivi)
{
    if (pivi->label)
        free (pivi->label);
	#if 0
	if (pivi->bmppath)
        free (pivi->bmppath);
	#endif

	
    free (pivi);
}

/*
 * ivlistItemAdd : add an new icon item to the icon list
 */
static IconviewItem* 
ivlistItemAdd (IconviewList* pivlist, IconviewItem* preitem, 
            IconviewItem* nextitem, PIVITEMINFO pii, int *idx)
{
    IconviewItem* ci;
    int ret;

    ci = (IconviewItem*) calloc (1, sizeof(IconviewItem));
    if (!ci)
        return NULL;

    if (pii) {
        ((MgItem *)ci)->addData = pii->addData;
        ci->bmp = pii->bmp;
		ci->icon_filetype = pii->icon_filetype;
		ci->icon_filesel[0] = pii->icon_filesel[0];
		ci->icon_filesel[1] = pii->icon_filesel[1];
		ci->icon_filelock[0] = pii->icon_filelock[0];
		ci->icon_filelock[1] = pii->icon_filelock[1];
		ci->selected = pii->selected;
		ci->filelock = pii->filelock;
        if (pii->label)
            ci->label = strdup (pii->label);	// 分配label空间,然后复制pii->label进去,之后 ci->label 指向新的label空间
    }

    ret = mglist_add_item ( (MgList *)pivlist, (MgItem *)ci, 
            (MgItem *)preitem, (MgItem *)nextitem, pii->nItem, idx);

    if (ret < 0) {
        freeIvItem (ci);
        return NULL;
    }

    return ci;
}

/*
 * ivlistItemDel : Deletes an icon item from the icon list
 */
static int ivlistItemDel (IconviewList* pivlist, IconviewItem* pci)
{
    mglist_del_item ((MgList *)pivlist, (MgItem *)pci);
    freeIvItem (pci);
    return 0;
}

/*
 * ivlist_reset_content : Clear icon list
 */
static void ivlist_reset_content (HWND hWnd, IconviewList* pivlist)
{
    IconviewItem* pci;

    while (!mglist_empty((MgList *)pivlist)) {
        pci = (IconviewItem*)mglist_first_item ((MgList *)pivlist);
        ivlistItemDel (pivlist, pci);
    }
    ((MgList *)pivlist)->nTotalItemH = 0;
}

/*
 * isInItem : Decides wheter an item is being clicked
 */
static int isInItem (MgList *mglst, int mouseX, int mouseY, 
                     MgItem** pRet, int *itemy)
{
    IconviewList *pivlist = (IconviewList *)mglst;
    IconviewItem* pci = NULL;
    int row, col, index;

    if (mouseY < 0 || mouseY >= mglst->nTotalItemH)
        return -1;

    if (mouseX < 0 || mouseX >= pivlist->nItemWidth * pivlist->nCol)
        return -1;

    row = mouseY / pivlist->nItemHeight;
    col = mouseX / pivlist->nItemWidth;

    index = row * pivlist->nCol + col;
    pci = (IconviewItem *)mglist_getitem_byindex (mglst, index);

    if (pRet)
        *pRet = (MgItem*)pci;

    return index;
}

int 
iconview_is_in_item (IconviewData* pivdata, 
        int mousex, int mousey, HITEM *phivi)
{
    return isInItem ((MgList *)&pivdata->ivlist, 
            mousex, mousey, (MgItem**)phivi, NULL);
}

/* --------------------------------------------------------------------------------- */

HITEM iconview_add_item (HWND hWnd, IconviewData* pivdata, HITEM prehivi,
                         PIVITEMINFO pii, int *idx)
{
    IconviewList* pivlist = &pivdata->ivlist;
    IconviewItem* pci;
    int index;

    if ((pci = 
        ivlistItemAdd (pivlist, (IconviewItem*)prehivi, NULL, pii, &index))) {
        int newh = 0;
        int count = mglist_get_item_count((MgList *)pivlist) ;

        if (pivlist->nCol == 1 
            || count % pivlist->nCol == 1)
            newh = pivlist->nItemHeight;

        if(count <=  pivlist->nCol) {
			// 
            scrolled_set_cont_width (hWnd, pivscr, 
                    count * pivdata->ivlist.nItemWidth);
        }
        mglist_adjust_items_height (hWnd, (MgList *)pivlist, pivscr, newh);
    }

    if (idx)
        *idx = index;

    return (HITEM)pci;
}

//FIXME
HITEM iconview_add_item_ex (HWND hWnd, IconviewData* pivdata, HITEM prehivi, 
                HITEM nexthivi, PIVITEMINFO pii, int *idx)
{
    IconviewList* pivlist = &pivdata->ivlist;
    IconviewItem* pci;
    int index;

    if ( (pci = ivlistItemAdd (pivlist, 
            (IconviewItem*)prehivi, (IconviewItem*)nexthivi, pii, &index))) {
        int count = mglist_get_item_count((MgList *)pivlist) ;

        if (count % pivlist->nCol == 1)
            mglist_adjust_items_height (hWnd, 
                (MgList *)pivlist, pivscr, pivlist->nItemHeight);
        if(count <=  pivlist->nCol) {
            scrolled_set_cont_width (hWnd, pivscr, 
                    count * pivdata->ivlist.nItemWidth);
        }
    }

    if (idx)
        *idx = index;

    return (HITEM)pci;
}

int iconview_move_item (IconviewData* pivdata, HITEM hivi, HITEM prehivi)
{
    if (!hivi)
        return -1;

    mglist_move_item ((MgList *)&pivdata->ivlist, 
            (MgItem *)hivi, (MgItem *)prehivi);
    scrolled_refresh_content (pivscr);
    return 0;
}

int 
iconview_del_item (HWND hWnd, IconviewData* pivdata, int nItem, HITEM hivi)
{
    IconviewList* pivlist = &pivdata->ivlist;
    IconviewItem* pci;

    if (hivi)
        pci = (IconviewItem*)hivi;
    else {
        if (!(pci =
            (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem)))
            return -1;
    }

    if (ivlistItemDel (pivlist, pci) >= 0) {
        int delh = 0;
        int count = mglist_get_item_count((MgList *)pivlist) ;
        if (count % pivlist->nCol == 0)
            delh = pivlist->nItemHeight;

        if(count <=  pivlist->nCol) {
            scrolled_set_cont_width (hWnd, pivscr, 
                    count * pivdata->ivlist.nItemWidth);
        }
        mglist_adjust_items_height (hWnd, (MgList *)pivlist, pivscr, -delh);
    }
    return 0;
}

PBITMAP iconview_get_item_bitmap (HITEM hitem)
{
    return ((IconviewItem *)hitem)->bmp;
}

const char* iconview_get_item_label (HITEM hitem)
{
    return ((IconviewItem *)hitem)->label;
}

static void recalc_total_h (HWND hWnd, IconviewData *pivdata)
{
    int newh;
    int content;
    IconviewList* pivlist = &pivdata->ivlist;
    int oldh = ((MgList *)pivlist)->nTotalItemH;

    //pivdata->ivlist.nCol = pivscr->nContWidth / pivdata->ivlist.nItemWidth;
    pivdata->ivlist.nCol = MAX(pivscr->nContWidth, pivscr->visibleWidth) / pivdata->ivlist.nItemWidth;
    content = pivdata->ivlist.nCol * pivdata->ivlist.nItemWidth;
    scrolled_set_cont_width (hWnd, pivscr, content);

	//FIXED BY Dongjunjie
	if(pivdata->ivlist.nCol <= 0)
		pivdata->ivlist.nCol = 1;
    newh = (mglist_get_item_count ((MgList *)pivlist) + pivlist->nCol - 1)
           / pivlist->nCol * pivlist->nItemHeight;
    if (newh != oldh) {
        mglist_adjust_items_height (hWnd, 
            (MgList *)pivlist, pivscr, newh - oldh);
    }
}

static int 
iconview_get_item_rect (HWND hWnd, HITEM hivi, RECT *rcItem, BOOL bConv)
{
    IconviewData *pivdata = (IconviewData *)GetWindowAdditionalData2(hWnd);

    iconview_get_item_pos (pivdata, hivi, &rcItem->left, &rcItem->top);
    if (bConv)
        scrolled_content_to_window (pivscr, &rcItem->left, &rcItem->top);
    rcItem->bottom = rcItem->top + pivdata->ivlist.nItemHeight;
    rcItem->right = rcItem->left + pivdata->ivlist.nItemWidth;

    return 0;
}

int iconview_get_item_pos (IconviewData* pivdata, HITEM hivi, int *x, int *y)
{
    int index;
    IconviewList *pivlist = &pivdata->ivlist;

    index = mglist_get_item_index ((MgList *)pivlist, (MgItem *)hivi);
    if (index < 0)
        return -1;

    *y = (index / pivlist->nCol ) * pivlist->nItemHeight;
    *x = (index % pivlist->nCol ) * pivlist->nItemWidth;

    return 0;
}

int iconview_is_item_hilight (HWND hWnd, HITEM hivi)
{
    IconviewData* pivdata = (IconviewData*)GetWindowAdditionalData2(hWnd);
    return (int)mglist_is_item_hilight (
            (MgList *)&pivdata->ivlist, (MgItem *)hivi);
}

DWORD iconview_get_item_adddata (HITEM hivi)
{
    return mglist_get_item_adddata (hivi);
}

void iconview_reset_content (HWND hWnd, IconviewData* pivdata)
{
    /* delete all ivlist content */
    ivlist_reset_content (hivwnd, &pivdata->ivlist);

    if (pivdata->scrdata.sbPolicy != SB_POLICY_ALWAYS) {
        ShowScrollBar (hWnd, SB_HORZ, FALSE);
        ShowScrollBar (hWnd, SB_VERT, FALSE);
    }

    /* reset content and viewport size */
    scrolled_init_contsize (hWnd, &pivdata->scrdata);
    /* reset ivlist window */
    //iconview_set_ivlist (hWnd, &pivdata->scrdata);
    /* reset viewport window */
    scrolled_set_visible (hWnd, &pivdata->scrdata);

    scrolled_set_hscrollinfo (hWnd, &pivdata->scrdata);
    scrolled_set_vscrollinfo (hWnd, &pivdata->scrdata);

    //FIXME
    //scrolled_refresh_content (pivscr);
    InvalidateRect (hWnd, NULL, TRUE);
}

static void ivDrawItem (HWND hWnd, GHANDLE hivi, HDC hdc, RECT *rcDraw)
{
    IconviewItem *ivitem = (IconviewItem *)hivi;
    RECT rcTxt;
    int x, y;
    const WINDOWINFO* win_info = GetWindowInfo (hWnd);

    memset (&rcTxt, 0, sizeof(rcTxt));
    SetBkMode (hdc, BM_TRANSPARENT);

    if (iconview_is_item_hilight(hWnd, hivi)) {
        SetTextColor (hdc, GetWindowElementPixel (hWnd, WE_FGC_HIGHLIGHT_ITEM));
		#if 0
        win_info->we_rdr->draw_hilite_item (hWnd, hdc, rcDraw, 
                GetWindowElementAttr (hWnd, WE_BGC_HIGHLIGHT_ITEM));
		#else
		
		SetPenColor(hdc, 0xffff3030);
		#endif
    }
    else {
        SetTextColor (hdc, GetWindowElementPixel (hWnd, WE_FGC_WINDOW));
        // win_info->we_rdr->draw_normal_item (hWnd, hdc, rcDraw, 
        //         GetWindowElementAttr (hWnd, WE_BGC_WINDOW));
        SetPenColor(hdc, 0x00000000);
    }
	#if 1		// draw highlight rect
	MoveTo(hdc, rcDraw->left+2, rcDraw->top+2);
	LineTo(hdc, rcDraw->right-2, rcDraw->top+2);
	LineTo(hdc, rcDraw->right-2, rcDraw->bottom-2);
    LineTo(hdc, rcDraw->left+2, rcDraw->bottom-2);
	LineTo(hdc, rcDraw->left+2, rcDraw->top+2);
	#endif
	int bmpw = 0, bmph = 0;
	int x0 = 0,y0 = 0;
	int drawflag = 0;
	PBITMAP tmpbmp = ivitem->bmp;
	if (tmpbmp) {
		//fprintf(stderr,"drawbmp\n");
		#if 0
        x = rcDraw->left + (RECTWP(rcDraw) - (int)ivitem->bmp->bmWidth ) / 2; 
        if (x < rcDraw->left) {
            x = rcDraw->left;
            bmpw = RECTWP(rcDraw);
        }
        y = rcDraw->top 
            + ( RECTHP(rcDraw) - RECTH(rcTxt) - (int)ivitem->bmp->bmHeight) / 2;
        if (y < rcDraw->top) {
            y = rcDraw->top;
            bmph = RECTHP(rcDraw) - RECTH(rcTxt);
        }
		#else
		bmpw = 178;	// for test 
		bmph = 100;
		x = rcDraw->left + (RECTWP(rcDraw) - bmpw ) / 2; 	
        if (x < rcDraw->left) {
            x = rcDraw->left;  
        }
        y = rcDraw->top  + (RECTHP(rcDraw) - bmph ) / 2;
        if (y < rcDraw->top) {
            y = rcDraw->top;     
        }
		#endif
		x0 = x;
		y0 = y;
        FillBoxWithBitmap (hdc, x, y, bmpw, bmph, tmpbmp);
		drawflag = 1;
		//fprintf(stderr,"draw bmp\n");
    }
	tmpbmp = ivitem->icon_filetype;
	if (tmpbmp && drawflag) {		// left-top
		//x = rcDraw->left;
		//y = rcDraw->top;
		x = x0;
		y = y0;
		bmpw = (int)tmpbmp->bmWidth;
		bmph = (int)tmpbmp->bmHeight;
		
		FillBoxWithBitmap (hdc, x, y, bmpw, bmph, tmpbmp);
		
	}
	if (ivitem->selected == 1) {
		tmpbmp = ivitem->icon_filesel[0];	
	} else if (ivitem->selected == 2) {
		tmpbmp = ivitem->icon_filesel[1];	
	} else 
		tmpbmp = NULL;
	
	if (tmpbmp && drawflag) {		// right-top
		//x = rcDraw->right - (int)tmpbmp->bmWidth ;
		//if (x < rcDraw->left ) x = rcDraw->left;
		//y = rcDraw->top;
		x = x0 + 178 - (int)tmpbmp->bmWidth;
		y = y0;
		bmpw = (int)tmpbmp->bmWidth;
		bmph = (int)tmpbmp->bmHeight;
		FillBoxWithBitmap (hdc, x, y, bmpw, bmph, tmpbmp);
		//fprintf(stderr,"draw icon 1\n");
	}
	if (ivitem->filelock == 1) {
		tmpbmp = ivitem->icon_filelock[0];	
	} else if (ivitem->filelock == 2) {
		tmpbmp = ivitem->icon_filelock[1];	
	} else 
		tmpbmp = NULL;
	
	if (tmpbmp && drawflag) {		// left-bottom
		//x = rcDraw->left;
		//y = rcDraw->bottom - (int)tmpbmp->bmHeight ;
		x = x0;
		y = y0 + 100 - (int)tmpbmp->bmHeight;
		if (y < rcDraw->top ) y = rcDraw->top;
		bmpw = (int)tmpbmp->bmWidth;
		bmph = (int)tmpbmp->bmHeight;
		FillBoxWithBitmap (hdc, x, y, bmpw, bmph, tmpbmp);
		//fprintf(stderr,"draw icon 2\n");
	}
	if (ivitem->label && drawflag) {
        rcTxt = *rcDraw;
        rcTxt.top   = rcTxt.bottom - GetWindowFont(hWnd)->size - 12;
		rcTxt.right = rcTxt.right - 20;
        DrawText (hdc, ivitem->label, -1, 
                &rcTxt, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
		//fprintf(stderr,"draw label\n");
    }
}

/* adjust the position and size of the ivlist window */
void iconview_set_ivlist (HWND hWnd, PSCRDATA pscrdata, BOOL visChanged)
{
    IconviewData* pivdata = (IconviewData*) GetWindowAdditionalData2 (hWnd);

    if (visChanged)
        InvalidateRect (hWnd, NULL, TRUE);
    else {
        //scrolled_refresh_content (pivscr);
        scrolled_refresh_view (pivscr);
    }
}

static int ivlist_init (HWND hWnd, IconviewList* pivlist)
{
    mglist_init((MgList *)pivlist, hWnd);

    pivlist->nItemWidth = IV_DEF_ITEMWIDTH;
    pivlist->nItemHeight = IV_DEF_ITEMHEIGHT;
    pivlist->nCol = 0;

    ((MgList *)pivlist)->iop.drawItem = ivDrawItem;
    ((MgList *)pivlist)->iop.getRect = iconview_get_item_rect;
    ((MgList *)pivlist)->iop.isInItem = isInItem;

    return 0;
}

/*
 * initialize iconview internal structure
 */
static int ivInitData (HWND hWnd, IconviewData* pivdata)
{
    RECT rcWnd;

    GetClientRect (hWnd, &rcWnd);
    scrolled_init (hWnd, &pivdata->scrdata, 
                    RECTW(rcWnd) - IV_LEFTMARGIN - IV_RIGHTMARGIN,
                    RECTH(rcWnd) - IV_TOPMARGIN - IV_BOTTOMMARGIN);

#ifdef __TARGET_MSTUDIO__
    pivdata->scrdata.sbPolicy = SB_POLICY_ALWAYS;
#endif

    scrolled_init_margins (pivscr, IV_LEFTMARGIN, IV_TOPMARGIN,
                           IV_RIGHTMARGIN, IV_BOTTOMMARGIN);

    ivlist_init (hWnd, &pivdata->ivlist);
    pivdata->ivlist.nCol = pivscr->nContWidth / pivdata->ivlist.nItemWidth;
	if(pivdata->ivlist.nCol <= 0)
		pivdata->ivlist.nCol = 1;

    pivdata->scrdata.move_content = iconview_set_ivlist; 
    pivdata->flags = 0;

    return 0;
}

/* 
 * shoulded be called before iconview is used
 * hWnd: the scrolled window
 */
//FIXME, to delete
int iconview_init (HWND hWnd, IconviewData* pivdata)
{
    if (!pivdata)
        return -1;

    SetWindowAdditionalData2 (hWnd, 0);
    ShowScrollBar (hWnd, SB_HORZ, FALSE);
    ShowScrollBar (hWnd, SB_VERT, FALSE);

    ivInitData (hWnd, pivdata);
    SetWindowAdditionalData2 (hWnd, (DWORD) pivdata);

    /* set scrollbar status */
    scrolled_set_hscrollinfo (hWnd, pivscr);
    scrolled_set_vscrollinfo (hWnd, pivscr);

    return 0;
}

/*
 * destroy a iconview
 */
void iconview_destroy (IconviewData* pivdata)
{
    ivlist_reset_content (hivwnd, &pivdata->ivlist);
}

static LRESULT IconViewCtrlProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    IconviewData* pivdata = NULL;
    IconviewList* pivlist = NULL;
	static int ivmove_distance = 0;
    if (message != MSG_CREATE) {
        pivdata = (IconviewData*) GetWindowAdditionalData2 (hWnd);
        pivlist = &pivdata->ivlist;
    }

    switch (message) {

    case MSG_CREATE:
    {
        pivdata = (IconviewData*) malloc(sizeof (IconviewData));
        if (!pivdata)
            return -1;
        iconview_init (hWnd, pivdata);
        if (GetWindowStyle(hWnd) & IVS_AUTOSORT) {
            iconview_set_autosort (pivdata);
        }
        break;
    }

    case MSG_DESTROY:
        iconview_destroy (pivdata);
        free (pivdata);
        break;
    
    case MSG_GETDLGCODE:
        return DLGC_WANTARROWS;

    case MSG_KEYDOWN:
    {
        HITEM hivi = 0, curHilighted;
        int cursel;
        int count = mglist_get_item_count ((MgList *)&pivdata->ivlist);

        curHilighted = 
            (HITEM) mglist_get_hilighted_item((MgList *)&pivdata->ivlist);
        cursel = mglist_get_item_index (
                (MgList *)&pivdata->ivlist, (MgItem *)curHilighted);

        if (wParam == SCANCODE_CURSORBLOCKDOWN) {
            int next = cursel + pivdata->ivlist.nCol;
            if (GetWindowStyle(hWnd) & IVS_LOOP && next >= count) {
                next -= (count + pivdata->ivlist.nCol 
                        - count % pivdata->ivlist.nCol);
                if (next < 0) next += pivdata->ivlist.nCol;
            }
            hivi = (HITEM)mglist_getitem_byindex (
                    (MgList *)&pivdata->ivlist, next);
        }
        else if (wParam == SCANCODE_CURSORBLOCKUP) {
            int next = cursel - pivdata->ivlist.nCol;
            if (GetWindowStyle(hWnd) & IVS_LOOP && next < 0) {
                next += (count + pivdata->ivlist.nCol 
                        - count % pivdata->ivlist.nCol);
                if (next >= count) next -= pivdata->ivlist.nCol;
            }
            hivi = (HITEM)mglist_getitem_byindex (
                    (MgList *)&pivdata->ivlist, next);
        }
        else if (wParam == SCANCODE_CURSORBLOCKLEFT) {
            hivi = iconview_get_prev_item(pivdata, curHilighted);
            if (GetWindowStyle(hWnd) & IVS_LOOP && !hivi) {
                hivi = iconview_get_prev_item(pivdata, 0);
            }
        }
        else if (wParam == SCANCODE_CURSORBLOCKRIGHT) {
            hivi = iconview_get_next_item(pivdata, curHilighted);
            if (GetWindowStyle(hWnd) & IVS_LOOP && !hivi) {
                hivi = iconview_get_next_item(pivdata, 0);
            }
        }
        else if (wParam == SCANCODE_HOME) {
            hivi = iconview_get_next_item(pivdata, 0);
        }
        else if (wParam == SCANCODE_END) {
            hivi = iconview_get_prev_item(pivdata, 0);
        }

        if (hivi) {
            if (hivi != curHilighted) {
                mglist_hilight_item (
                    (MgList *)&pivdata->ivlist, (MgItem *)hivi);
                NotifyParentEx (hWnd, 
                    GetDlgCtrlID(hWnd), IVN_SELCHANGED, (DWORD)hivi);
            }
            mglist_make_item_visible ((MgList *)&pivdata->ivlist, pivscr, hivi);
        }
        break;
    }
	case MSG_MOUSEMOVE:
    {
			int mouseX = LOSWORD (lParam);
			int mouseY = HISWORD (lParam);
			int cur_pos = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);	// 当前Y方向的offset
			//fprintf(stderr, " cur_pos: %d\n",cur_pos);
			//fprintf(stderr,"habo---> iconview MSG_MOUSEMOVE  x= %d  y= %d \n",mouseX,mouseY);
					
			ivmove_flag = TRUE;
			ScreenToClient (hWnd, &mouseX, &mouseY);
			/* NOTE: wParam = MAKELONG (direction, velocity); */
						
			int velocity = HISWORD(wParam);
#define WPARAM_MASK 0x400 //to get the direction of mouse move
			ivdirection = LOSWORD(wParam) & ~(WPARAM_MASK);
			//fprintf(stderr, "velocity: %d ivdirection: %d \n",velocity,ivdirection);
			if (ivdirection == MOUSE_UP) {	// 本次是向上
				if(ivmouse_updown_flag == MOUSE_DOWN){	// 上次是向下
					cur_pos = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);
					//fprintf(stderr, "1 cur_pos: %d\n",cur_pos);
				}
				cur_pos += velocity;
				ivmouse_updown_flag = MOUSE_UP;
				//ScrollViewCtrlProc (hWnd, MSG_VSCROLL, SB_LINEDOWN, 0);
			} else if (ivdirection == MOUSE_DOWN) {	// 本次是向下
				if(ivmouse_updown_flag == MOUSE_UP){	// 上次是向上
					cur_pos = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);
					//fprintf(stderr, "2 cur_pos: %d\n",cur_pos);
				}
				cur_pos -= velocity;
				ivmouse_updown_flag = MOUSE_DOWN;
				//ScrollViewCtrlProc (hWnd, MSG_VSCROLL, SB_LINEUP, 0);
			}
			ivstop_inertia_move = TRUE;

			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t d_usec = (tv.tv_sec - ivlast_move_time.tv_sec) * 1000000
					+ (tv.tv_usec - ivlast_move_time.tv_usec);
			float speed = (mouseY - ivlast_move_start) * 1000.0f / d_usec; // pixel/msec
			int dir = speed > 0 ? 1 : -1;
			ivcur_acceleration = - (dir * speed) * 1000.0f / d_usec;
#if USE_AVERAGE // 
			accel_data[accel_data_count++ % MAX_ACCEL_DATA_COUNT] = cur_acceleration;
			/* fprintf(stderr, "count: %d, speed: %f, acceleration: %f, cur_pos: %d\n", accel_data_count, speed, cur_acceleration, plvdata->cur_pos); */
#endif
		
			/* fprintf(stderr, "MSG_MOUSE, mouseY: %d, last_move_start: %d\n", mouseY, last_move_start); */
			ivlast_move_start = mouseY;
			//ivlast_move_time.tv_sec = tv.tv_sec;
			//ivlast_move_time.tv_usec = tv.tv_usec;
			//fprintf(stderr, "new pos: %d\n",cur_pos);
	
			if (cur_pos < 0) cur_pos = 0;
				return ScrollViewCtrlProc (hWnd, MSG_VSCROLL, SB_THUMBTRACK, cur_pos);
						 
			 break;
		
	}
	case MSG_LBUTTONDOWN:
		{
                int mouseX = LOSWORD (lParam);
                int mouseY = HISWORD (lParam);
                int nCols = -1, nRows;
                POINT pt;
                #ifdef PRODUCT_V316_CDR
                ivmove_distance = mouseY+120;//
                #else
                ivmove_distance = mouseY;
                #endif
                //fprintf(stderr,"habo---> iconview MSG_LBUTTONDOWN  x= %d  y= %d ivmove_distance= %d\n",mouseX,mouseY,ivmove_distance);
                //RECT rect, rcClient;
                //PLSTHDR p1;
                //PITEMDATA pi;

                //GetClientRect (hWnd, &rcClient);

                ivlast_move_start = mouseY;
#if USE_AVERAGE
                memset(accel_data, 0, MAX_ACCEL_DATA_COUNT);
#endif

                //if (GetCapture() == hWnd)
                //    break;

                gettimeofday(&ivlast_move_time, NULL);
                ivstop_inertia_move = TRUE;
				HITEM hivi;
				if (iconview_is_in_item(pivdata,mouseX,mouseY,&hivi)>=0) {
					IVSTATUS(hWnd) |= 0x0008;
					//fprintf(stderr,"hivi is %d\n",hivi);
				}
                
				#if 0
                if (nCols >= 0) {  
                    /* clicks on the header*/
                    LVSTATUS(hWnd) |= LVST_HEADCLICK;
                    LVSTATUS(hWnd) |= LVST_INHEAD;
                    plvdata->pHdrClicked = p1;

                    SetRect (&rect, p1->x - plvscr->nContX, LV_HDR_TOP, 
                            p1->x - plvscr->nContX+ p1->width, 
                            LV_HDR_TOP + LV_HDR_HEIGHT);
                    InvalidateRect (hWnd, &rect, TRUE);
                }
                else {
                    nCols = lvInWhichHeadBorder (mouseX, mouseY, &p1, plvdata);
                    if (plvdata->nHeadHeight > 0 && nCols >= 0) {
                        LVSTATUS(hWnd) |= LVST_BDDRAG;
                        plvdata->nItemDraged = nCols;
                    }
                    else if ((nRows = isInLVItem (mouseX, mouseY, &pi, plvdata, 
                                            &pt)) >= 0)
                    {
                        int indent;
                        //FIXME
                        if ( !(GetWindowStyle(hWnd) & LVS_UPNOTIFY) ) {
                            /*NotifyParent (hWnd, GetDlgCtrlID(hWnd), LVN_SELCHANGE);*/
                        }
                        else {
                            LVSTATUS(hWnd) |= LVST_ITEMDRAG;
                        }
                        indent = lvGetItemIndent (plvdata, pi);
                        if (pi->child && pt.x > indent && pt.x < indent + 9
                                /*bmp_fold->bmWidth*/ + 4) 
                        {
                            BOOL bfold;
                            bfold = (ISFOLD(pi)) ? FALSE : TRUE;
                            /* select highlight item first.*/
                            ScrollViewCtrlProc (hWnd, message, wParam, lParam);
                            lvFoldItem (hWnd, plvdata, pi, bfold);
                            NotifyParentEx (hWnd, GetDlgCtrlID(hWnd), 
                                    bfold ? LVN_FOLDED : LVN_UNFOLDED, (DWORD)pi);
                            return 0;
                        }
                    }
                }
				
				#endif
                break;
            }
	case MSG_LBUTTONUP:
		{
                //PLSTHDR p1;
                //int nCols;
                int mouseX = LOSWORD (lParam);
                int mouseY = HISWORD (lParam);
                ivmove_distance -= mouseY;
                //fprintf(stderr,"habo---> iconview MSG_LBUTTONUP  x= %d  y= %d ivmove_distance= %d\n",mouseX,mouseY,ivmove_distance);
                //LVNM_NORMAL lvnm;

                //lvnm.wParam = wParam;
                //lvnm.lParam = lParam;

                //plvdata->cur_pos = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);
                UpdateWindow(hWnd,TRUE);
                //if (GetCapture() != hWnd)
                //    break;

				
				#if 0
                if (LVSTATUS(hWnd) & LVST_CAPTURED) {
                    LVSTATUS(hWnd) &= ~LVST_CAPTURED;
                    ReleaseCapture ();
                    ScreenToClient (hWnd, &mouseX, &mouseY);
                }
                else
                    break;

                if (LVSTATUS(hWnd) & LVST_HEADCLICK)
                {
                    LVSTATUS(hWnd) &= ~LVST_HEADCLICK;
                    LVSTATUS(hWnd) &= ~LVST_INHEAD;

                    nCols = isInListViewHead (mouseX, mouseY, NULL, plvdata);
                    if (nCols < 0)
                        break;
                    p1 = lvGetHdrByCol (&plvdata->hdrqueue, nCols);
                    if (!p1 || p1 != plvdata->pHdrClicked)
                        break;

                    lvToggleSortStatus(p1);
                    if ( !(GetWindowStyle(hWnd)&(LVS_TREEVIEW)) && GetWindowStyle(hWnd)&(LVS_SORT) )
                        lvSortItem (NULL, nCols, p1->sort, plvdata);

                    //FIXME
                    InvalidateRect (hWnd, NULL, TRUE);
                }
                else if (LVSTATUS(hWnd) & LVST_BDDRAG)
                {
                    LVSTATUS(hWnd) &= ~LVST_BDDRAG;
                    lvSetContWidth (hWnd, plvdata, plvdata->nHeadWidth);
                }
                else if (LVSTATUS(hWnd) & LVST_ITEMDRAG && GetWindowStyle(hWnd) 
                        & LVS_UPNOTIFY) {
                    HSVITEM hsvi;
                    int idx;

                    if (ivmove_flag == TRUE) {
                        ivmove_flag = FALSE;
                        if (abs(ivmove_distance) > 15) {
                            ivstop_inertia_move = FALSE;
#if USE_AVERAGE // 使用
							float sum = 0.0f;
                            if (accel_data_count >= MAX_ACCEL_DATA_COUNT) {
                                for (int i = 0; i < MAX_ACCEL_DATA_COUNT; i++) {
                                    sum += accel_data[i];
                                }
                                cur_acceleration = sum / MAX_ACCEL_DATA_COUNT;
                            } else {
                                for (int i = 0; i < accel_data_count; i++) {
                                    sum += accel_data[i];
                                }
                                cur_acceleration = sum / accel_data_count;
                            }
                            accel_data_count = 0;
#endif

                            /* if (cur_acceleration < -0.0150f) cur_acceleration = -0.0150f; */
                            ivduration = ivcur_acceleration * pow(FLING_TIME(FLING_PHY_TIME), 2) / 2.0f;
                            /* fprintf(stderr, "acceleration: %f, duration: %f\n", cur_acceleration, duration); */
                            if (IsTimerInstalled(hWnd, ID_INERTIAL_FLING_TIMER2)) {
                                ResetTimer(hWnd, ID_INERTIAL_FLING_TIMER2, 1);
                            } else {
                                SetTimer(hWnd, ID_INERTIAL_FLING_TIMER2, 1);
                            }
                            break;
                        }
                    }
					
                    ivstop_inertia_move = TRUE;
			// todo
                    //idx = isInItem (mouseX, mouseY, NULL, plvdata, NULL);
                    //hsvi = scrollview_get_item_by_index (&plvdata->svdata, idx);
                    //NotifyParent (hWnd, GetDlgCtrlID(hWnd), LVN_CLICKED);
                    /* SendMessage(GetParent(hWnd), (MSG_USER+200), idx, lParam); */
                    /* LVS_UPNOTIFY only affects LVN_CLICKED 

                       if (hsvi && hsvi != scrollview_get_hilighted_item 
                                            (&plvdata->svdata)) {
                       if ( GetWindowStyle(hWnd) & LVS_UPNOTIFY ) {
                       NotifyParent (hWnd, GetDlgCtrlID(hWnd), LVN_SELCHANGE);
                       }
                       }
                       */
                }
				#endif
				if (IVSTATUS(hWnd) & 0x0008 )
				{
                	IVSTATUS(hWnd) &= ~0x0008;
					if (ivmove_flag == TRUE) {
                        ivmove_flag = FALSE;
                        if (abs(ivmove_distance) > 15) {
                            ivstop_inertia_move = FALSE;
#if USE_AVERAGE // 使用
							float sum = 0.0f;
                            if (accel_data_count >= MAX_ACCEL_DATA_COUNT) {
                                for (int i = 0; i < MAX_ACCEL_DATA_COUNT; i++) {
                                    sum += accel_data[i];
                                }
                                cur_acceleration = sum / MAX_ACCEL_DATA_COUNT;
                            } else {
                                for (int i = 0; i < accel_data_count; i++) {
                                    sum += accel_data[i];
                                }
                                cur_acceleration = sum / accel_data_count;
                            }
                            accel_data_count = 0;
#endif

                            /* if (cur_acceleration < -0.0150f) cur_acceleration = -0.0150f; */
                            ivduration = ivcur_acceleration * pow(100, 2) / 2.0f;
                            /* fprintf(stderr, "acceleration: %f, duration: %f\n", cur_acceleration, duration); */
                            if (IsTimerInstalled(hWnd, ID_INERTIAL_FLING_TIMER2)) {
                                ResetTimer(hWnd, ID_INERTIAL_FLING_TIMER2, 1);
                            } else {
                                SetTimer(hWnd, ID_INERTIAL_FLING_TIMER2, 1);
                            }
							//fprintf(stderr, "do flip\n"); 
                            break;
                        }
                    }
					ivstop_inertia_move = TRUE;
					if (ivmove_flag == FALSE) { 
	                    
						HITEM hivi;
	                    //iconview_is_item_hilight(pivdata,&hivi);
	                    int idx = iconview_is_in_item(pivdata,mouseX,mouseY,&hivi);	// 当前窗口显示的序号 0~5
	                    NotifyParent (hWnd, idx, 0x0001);
	                    //fprintf(stderr, "iconview LVN_CLICKED %d\n", idx);
					}
               }        
		}
		
        break;
		
	case MSG_TIMER:
            {
                int timer_id = (int)wParam;
                if (timer_id == ID_INERTIAL_FLING_TIMER2) {
                    static int t = 0;
                    t += 10;

                    float pos = ivduration - ivcur_acceleration * powf(t, 2.0f) / 2;
					#if 1
					int cur_posx = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);
                    if (ivdirection == MOUSE_UP) {
                        cur_posx += abs(pos );
                    } else if (ivdirection == MOUSE_DOWN) {
                        cur_posx -= abs(pos );
                    }
                    /* fprintf(stderr, "t: %d, pos: %f, cur_pos: %d\n", t, pos, plvdata->cur_pos); */
                    if (cur_posx < 0) cur_posx = 0;
                    	//ScrollViewCtrlProc (hWnd, MSG_VSCROLL, SB_THUMBTRACK, plvdata->cur_pos);
                    	ScrollViewCtrlProc (hWnd, MSG_VSCROLL, SB_THUMBTRACK, cur_posx);
						//fprintf(stderr, "cur_posx: %d\n",cur_posx);
                    if (t == (100) || ivstop_inertia_move) {
                        KillTimer(hWnd, ID_INERTIAL_FLING_TIMER2);
                        //cur_posx = ScrollViewCtrlProc(hWnd, SVM_GETCONTENTY, 0, 0);
                        /* fprintf(stderr, "MSG_TIMER stop, cur_pos: %d\n", plvdata->cur_pos); */
                        t = 0;
						NotifyParent (hWnd, 0x0, 0x0801);
                        break;
                    }
					#endif
                }
            }
            break;
    case MSG_PAINT:
    {
        HDC hdc = BeginPaint (hWnd);
        list_t *me;
        IconviewItem* pci;
        RECT rcAll, rcDraw;
        int i = 0;
        RECT rcVis;

        rcAll.left = 0;
        rcAll.top = 0;
        rcAll.right = pivscr->nContWidth;
        rcAll.bottom = pivscr->nContHeight;

        scrolled_content_to_window (pivscr, &rcAll.left, &rcAll.top);
        scrolled_content_to_window (pivscr, &rcAll.right, &rcAll.bottom);

        scrolled_get_visible_rect (pivscr, &rcVis);
        ClipRectIntersect (hdc, &rcVis);

        mglist_for_each (me, pivlist) {
            pci = (IconviewItem*)mglist_entry (me);

            rcDraw.left = rcAll.left + (i % pivlist->nCol) * pivlist->nItemWidth;
            rcDraw.right = rcDraw.left + pivlist->nItemWidth;
            rcDraw.top = rcAll.top + (i / pivlist->nCol) * pivlist->nItemHeight;
            rcDraw.bottom = rcDraw.top + pivlist->nItemHeight;

            if (rcDraw.bottom < rcVis.top) {
                i++;
                continue;
            }
            if (rcDraw.top > rcVis.bottom)
                break;

            if ( ((MgList *)pivlist)->iop.drawItem ) {
                ((MgList *)pivlist)->iop.drawItem (hWnd, (HITEM)pci, hdc, &rcDraw);
            }
            i++;
        }

        EndPaint (hWnd, hdc);
        return 0;
    }

    case MSG_SIZECHANGED:
        if (pivdata) {
            DefaultItemViewProc (hWnd, message, wParam, lParam, 
                                 pivscr, (MgList *)&pivdata->ivlist);
            recalc_total_h (hWnd, pivdata);
        }
        return 0;

    case IVM_ADDITEM:
    {
        int idx;
        HITEM hivi;

        hivi = iconview_add_item (hWnd, pivdata, 0, (PIVITEMINFO)lParam, &idx);		// lParam: iconitems
        if (wParam)
            *(HITEM *)wParam = hivi;
        return hivi?idx:-1;
    }

    case IVM_DELITEM:
        iconview_del_item (hWnd, pivdata, wParam, (HITEM)lParam);	// wParam: icon_index, lParam: 0
        return 0;
	//case IVM_GETITEM:
	//	//iconview_get_item(hWnd,)
	//	return 0;
    case IVM_RESETCONTENT:
        iconview_reset_content (hWnd, pivdata);
        return 0;

    case IVM_SETITEMSIZE:
    {
        
        if (wParam == 0) 
            pivdata->ivlist.nItemWidth = IV_DEF_ITEMWIDTH;
        else
            pivdata->ivlist.nItemWidth = wParam;
        
        if (lParam == 0)
            pivdata->ivlist.nItemHeight = IV_DEF_ITEMHEIGHT;
        else
            pivdata->ivlist.nItemHeight = lParam;
        
        pivdata->ivlist.nCol = pivscr->nContWidth / pivdata->ivlist.nItemWidth;
		if(pivdata->ivlist.nCol <= 0)
			pivdata->ivlist.nCol = 1;
        return 0;
    }
    case IVM_GETFIRSTVISIBLEITEM:
    {
        int i = 0;
        int top = 0, bottom = 0; 
        list_t *me;

        mglist_for_each (me, pivlist) {
            top = (i / pivlist->nCol) * pivlist->nItemHeight;
            bottom = top + pivlist->nItemHeight;
            scrolled_content_to_visible (pivscr, NULL, &bottom);

            if (bottom < 0)
                i++;
            else
                break;
        }

        return i;
    }
	case IVM_SETITEMBMP:
        return iconview_set_item_bitmap (hWnd, (IconviewData*)pivdata, wParam, lParam);
        
	
	case IVM_SETITEMICON:
        return iconview_set_item_icon (hWnd, (IconviewData*)pivdata, wParam, lParam);
        
	case IVM_SETITEM_LOCK:
        return iconview_set_item_lock (hWnd, (IconviewData*)pivdata, wParam, lParam);

	case IVM_SETITEM_SELECT:
        return iconview_set_item_select (hWnd, (IconviewData*)pivdata, wParam, lParam);
		
	case IVM_SETITEM_SELECTALL:
        return iconview_set_item_selectall (hWnd, (IconviewData*)pivdata, wParam, lParam);

	case IVM_GETITEM_SELECT:
        return iconview_get_item_select (hWnd, (IconviewData*)pivdata, wParam, lParam);
	
	
    }/* end switch */

    return DefaultItemViewProc (hWnd, message, wParam, lParam, 
                                pivscr, (MgList *)&pivdata->ivlist);
}

BOOL RegisterIconViewControl (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_ICONVIEW;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementPixel (HWND_NULL, WE_BGC_WINDOW);
    WndClass.WinProc     = IconViewCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

///////////////////////////
// Add by dongjunjie
GHANDLE iconview_get_item(HWND hwnd, int index)
{
	IconviewData* pivdata = (IconviewData*)GetWindowAdditionalData2(hwnd);
	return (GHANDLE)mglist_getitem_byindex((MgList*)&pivdata->ivlist, index);
}

BOOL iconview_set_item_lable(GHANDLE hivi, const char* strLabel)
{
	IconviewItem * iitem;

	if (hivi == 0 || strLabel == NULL) {
		return FALSE;
	}

	iitem = (IconviewItem*)hivi;
	if(iitem->label)
		free(iitem->label);
	iitem->label = strdup(strLabel);
	return TRUE;
}

int iconview_set_item_bitmap(HWND hWnd, IconviewData* pivdata, DWORD nItem, GHANDLE bmp)
{	
	//IconviewData* tmppivdata = (IconviewData*) pivdata;
	IconviewList* pivlist = &pivdata->ivlist;
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem);
	if (!pci) 
		return -1;
	pci->bmp = (PBITMAP)bmp;
    return 0;

}
// HWND hWnd, IconviewData* pivdata, int nItem, HITEM hivi
int iconview_set_item_icon(HWND hWnd, IconviewData* pivdata, DWORD nItem_ex, GHANDLE bmp)
{
#if 0
	//IconviewData* tmppivdata = (IconviewData*) pivdata;
	IconviewList* pivlist = &pivdata->ivlist;
	int nId = nItem_ex >> 24;		// 0xff000000
	int nItem = nItem_ex & 0x00ffffff;
	//fprintf(stderr, "nItem_ex: %08x %p\n",nItem_ex,bmp);
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem);
	if (!pci) 
		return -1;
	if (nId < 0 || nId>2 ) return -1;
	pci->icon[nId] = (PBITMAP)bmp;
#endif
    return 0;
}
int iconview_set_item_select(HWND hWnd, IconviewData* pivdata, DWORD nItem, DWORD id)
{
	IconviewList* pivlist = &pivdata->ivlist;
	
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem);
	if (!pci) 
		return -1;
	pci->selected = id;

    return 0;
}

int iconview_set_item_selectall(HWND hWnd, IconviewData* pivdata, DWORD nItem, DWORD id)
{
	IconviewList* pivlist = &pivdata->ivlist;

	int count = mglist_get_item_count((MgList *)pivlist);
	for (int i=0; i<count; i++) {
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, i);
		if (!pci) 
			continue;
		pci->selected = id;
	}

    return 0;
}

int iconview_set_item_lock(HWND hWnd, IconviewData* pivdata, DWORD nItem, DWORD id)
{
	IconviewList* pivlist = &pivdata->ivlist;
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem);
	if (!pci) 
		return -1;
	pci->filelock = id;

    return 0;
}

int iconview_get_item_select(HWND hWnd, IconviewData* pivdata, DWORD nItem, DWORD id)
{
	IconviewList* pivlist = &pivdata->ivlist;
	
    IconviewItem* pci = (IconviewItem*)mglist_getitem_byindex((MgList *)pivlist, nItem);
	if (!pci) 
		return -1;
	return pci->selected;
}


#endif /* _MGCTRL_ICONVIEW */

