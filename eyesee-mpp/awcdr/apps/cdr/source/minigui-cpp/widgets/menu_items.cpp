/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: menu_items.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/
//#define NDEBUG 

#include "widgets/menu_items.h"
#include "widgets/ctrlclass.h"
#include "widgets/listbox_impl.h"
#include "data/gui.h"
#include "window/user_msg.h"
#include "memory/x_memory.h"
#include "utils/utils.h"
#include <sstream>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define ITEM_VISIBLE 6
#define ITEM_HIGHT 59
#define ITEM_UPPER_CLEARANCE_H 11//tiem offset the top hight
#define ITEM_UL_CLEANRANCE_H 0 //item upper and lower clearance height
#define F_ITEM_OFFSET_LEFT_W 20 //the first item offset the left width
#define F_ITEM_UNHILIGHT_W 290+36
#define F_ITEM_HILIGHT_W_OFFSET 36-36 //the hilight laster than unhilight item icon 30

#define F_F_ICON_UNHIGHT_X 35 //first level first unhight icon x position 
#define F_F_ICON_HIGHT_X 35 //first level first unhight icon x position 
#define F_F_ICON_STRING_W 20 //first level first icon and string clearance
#define F_S_STRING_X 378 //first level second string x position
#define F_S_STRING_ITEM_Y 121 //first level second string y position
#define F_S_STRING_ITEM_H 66 //first level second item string height
#define F_S_STRING_ITEM_UL_CLEANRANCE_H 12 //first level second item string upper and lower clearance height
#define F_BUTTON_X 467 //first level button x position
#define F_BUTTONG_STRING_OFFSET 32 //first level button and string offset height
#define F_LINE_X 380 //first level line x position
#define F_LINE_W 442 //first level line w
#define F_LASTER_ICON_OFFSET 34 //the first level laster icon offset the right


#define S_LINE_X 156 //second level line x position
#define S_LINE_W 666 //second level line w
#define S_F_ICON_BG_W 106 //second level first icon bg width
#define S_F_ICON_BG_H 72 //second level first icon bg height
#define S_F_ICON_P 32 //the second level first icon x point
#define S_HEAD_STRING_X 154 //the second level head string x position/subitem string bg x position/tips string x position
#define S_ITEM_STRING_BG_W 668 //the second level item string bg width
#define S_ITEM_STRING_BG_H 66 //the second level item string bg height
#define S_ITEM_STRING_BG_OFFSET 32 //the second level item string bg offset the right
#define S_ITEM_STRING_BG_OFFSET_TOP 109 //the second level item string bg offset the top
#define S_STRING_P 202 //the second level string x point
#define S_LASTER_ICON_OFFSET 80 //the second level laster icon offset the right
#define S_BUTTON_X 355 //first level button x position
#define S_BUTTONG_STRING_OFFSET 32 //first level button and string offset height


#define LOG_TAG "menu_items"
#define DASH_CAMERA_SETTING "Dash Camera setting"
//#define USESCROLLBAR
//#define USELINE
#define OFFSETMAX 120
IMPLEMENT_DYNCRT_CLASS(MenuItems)

inline void Line2 (void* hdc, int x1, int y1, int x2, int y2)
{
    MoveTo(hdc, x1, y1);
    LineTo(hdc, x2, y2);
}

/*
 *-----------------------------------------------------------------|           |
 *[#icon] [#item_string] [#result_string/result_image]             |#scroll_bar|
 *-----------------------------------------------------------------|           |
 *                                                    |<-#end_gap->|           |
 */
MenuItems::MenuItems(View *parent)
    :CustomWidget(parent)
    , bg_image_(NULL)
{
	main_item = 0;
    submenu_status = false;
    for(int i = 0; i < HILIGHT_IMAGE_NUM; i++){
        hilight_image_[i]= NULL;
        subhilight_image_[i] = NULL;
        button_image_[i] = NULL;
    }
	frist_bg_image_  = NULL;
    left_ = 0;
    width_ = 0;
    max_width_ = 0;
    offset_submenu= 0;
    offset_menu= 0;
    count_ = 0;
    top_ = 0;
    hilighted_idx_ = -1;
    last_selected_idx_ = -1;
    icon_restore_idx_ = -1;
    fling_flag_ = FALSE;
    index_flag = 0;
    exit_flag = false;
    subSwitch_value = 0;
    for(int i = 0; i < ITEMNUMBER ; i++){
        submenu_first_image_[i]= NULL;
        submenu_first_image_w[i]= NULL;
    }
    s_width=-1;
    s_height = -1;
    m_add_data_finish = false;
    m_paint_second_ = false;
    first_ts_current_id = 0;
}

MenuItems::~MenuItems()
{
    if (bg_image_) {
        UnloadBitmap(bg_image_);
    }
    FreedBeforeRemoveAll();
    for(unsigned int i=0; i<data_.size(); i++) {
        delete data_[i];
    }
    for(int i = 0 ; i < HILIGHT_IMAGE_NUM ; i++){
        if(hilight_image_[i] != NULL){
            FreeImage(hilight_image_[i]);
            hilight_image_[i] = NULL;
        }
        if(subhilight_image_[i] != NULL){
            FreeImage(subhilight_image_[i]);
            subhilight_image_[i] = NULL;
        }
        if(button_image_[i] != NULL){
            FreeImage(button_image_[i]);
            button_image_[i] = 	NULL;
        }
    }
    for(int i = 0; i < ITEMNUMBER ; i++){
        if(submenu_first_image_w[i] != NULL){
            FreeImage(submenu_first_image_w[i]);
            submenu_first_image_w[i] = 	NULL;
        }
        if(submenu_first_image_[i] != NULL){
            FreeImage(submenu_first_image_[i]);
            submenu_first_image_[i] = 	NULL;
        }
    }
	 if(frist_bg_image_ != NULL){
		 	FreeImage(frist_bg_image_);
	 		frist_bg_image_ = NULL;
	 }
}
bool MenuItems::getSubmenuStatusFlag()
{
	return submenu_status;
}
BITMAP* MenuItems::AllocImage(const char *image_path)
{
    int ret = 0;

    if (image_path == NULL)
		return NULL;

	if( !strncmp(image_path, "", 1) )
		return NULL;

    BITMAP *data = (BITMAP*)malloc(sizeof(BITMAP));
    //db_warn("zhb---------------image_path=%s",image_path);
    ret = LoadBitmapFromFile(HDC_SCREEN, data, image_path);
    if (ret != 0)
	{
        free(data);
		data = NULL;
    }

    return data;
}

void MenuItems::FreeImage(BITMAP *data)
{
    UnloadBitmap(data);
}

void MenuItems::setSwitchValue(int value)
{
	subSwitch_value = value;
}

int MenuItems::getSwitchValue()
{
	return subSwitch_value;
}

void MenuItems::SetWindowBackImage(const char *bmp)
{
    if (bmp == NULL) {
        db_error("failed, bmp = null");
        return;
    }

    if (bg_image_ == NULL) {
        bg_image_ = (BITMAP*)malloc(sizeof(BITMAP));
    }

    LoadBitmap(HDC_SCREEN, bg_image_, bmp);
}

void MenuItems::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_MENULIST;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | WS_CHILD  | WS_VSCROLL;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

#define _USE_FIXSTR      1

#define ITEM_BOTTOM(x)  (x->top_ + x->visible_cnt_ - 1)

#define LST_INTER_BMPTEXT       2
/** minimum height of checkmark */
#define LFRDR_LB_CHECKBMP_MIN   6

/* pixels to the left of the selected hilighted item */
static unsigned char HILIGHT_LEFT = 0;
static unsigned char HILIGHT_RIGHT = 0;

static gal_pixel gp_hilite_bgc;
static gal_pixel gp_hilite_fgc;
static gal_pixel gp_sign_bgc;
static gal_pixel gp_sign_fgc;
static gal_pixel gp_normal_bgc;
static gal_pixel gp_normal_fgc;
static gal_pixel gp_linec;
static gal_pixel gp_normal_stringc;
static gal_pixel gp_selected_stringc;
static gal_pixel gp_normal_valuec;
static gal_pixel gp_selected_valuec;
static gal_pixel gp_menu_stringc_color;
static gal_pixel gp_submenu_head_color;


// static PCONTROL gui_Control (HWND hwnd)
// {
    // PCONTROL pCtrl;

    // pCtrl = (PCONTROL) hwnd;

    // if (pCtrl && pCtrl->WinType == TYPE_CONTROL)
        // return pCtrl;

    // return NULL;
// }

static int CheckMarkHeight (int item_height)
{
    db_msg(" ");

    int checkmark_height =
        item_height - LFRDR_LB_CHECKBMP_MIN;

    if (0 < checkmark_height)
        checkmark_height += checkmark_height >> 2;

    return checkmark_height;
}

static int lstGetItemWidth (HWND hwnd, ItemData *data, int item_height)
{
    PCONTROL    pCtrl;
    DWORD       dwStyle;

    int         x = 0;
    pCtrl = gui_Control (hwnd);
    dwStyle = pCtrl->dwStyle;

    x = LST_INTER_BMPTEXT;

    if (dwStyle & LBS_CHECKBOX){
        x += CheckMarkHeight (item_height) + LST_INTER_BMPTEXT;
    }
    if(data->first_icon[UNHILIGHTED_ICON] != NULL)
        x += (data->first_icon[UNHILIGHTED_ICON])->bmWidth;
    x += LST_INTER_BMPTEXT;
    if (data->item_string.c_str() && data->item_string.c_str()[0] != '\0') {
        SIZE size;
        HDC hdc;
        hdc = GetClientDC (hwnd);
        GetTabbedTextExtent (hdc, data->item_string.c_str(), -1, &size);
        ReleaseDC (hdc);
        x += size.cx + LST_INTER_BMPTEXT;
    }

    return x;
}

void MenuItems::GetHilightItemRect(RECT * rect)
{
	rect->top = rect_.top;
	rect->bottom = rect_.bottom;
	rect->left = 0;
	rect->right = s_width-S_HEAD_STRING_X-S_BUTTONG_STRING_OFFSET;
}
void MenuItems::SaveHilightItemRect(RECT & rect)
{
	rect_.left = rect.left;
	rect_.right = rect.right;
	rect_.top = rect.top;
	rect_.bottom = rect.bottom;
}

void MenuItems::GetItemsRect (int start, int end, RECT* prc)
{
    if (start < 0)
        start = 0;

    prc->top = (start - top_)*height_+ITEM_UPPER_CLEARANCE_H;

    if (end >= 0)
        prc->bottom = (end - top_ + 1)*height_+ITEM_UPPER_CLEARANCE_H;
}
void MenuItems::GetSubmenuRect (RECT* prc,bool submenuflag)
{
    if(submenuflag)
        prc->left= S_HEAD_STRING_X;
    else
        prc->left= F_S_STRING_X;
	
	prc->top = 0;
	prc->right= s_width;
	prc->bottom= s_height;
}

void MenuItems::InvalidateItem (HWND hwnd, int pos)
{
    db_msg(" ");

    RECT rcInv;

    if (pos < top_ || pos > (top_ + visible_cnt_))
        return;

    GetClientRect (hwnd, &rcInv);
    rcInv.top = (pos - top_)*height_;
    rcInv.bottom = rcInv.top + height_;

    InvalidateRect (hwnd, &rcInv, TRUE);
}

bool MenuItems::InvalidateUnderMultiItem (HWND hwnd, int start, int end)
{
    RECT rcInv;
    int pos;
    int max;

    if (start < 0) start = 0;
    if (end < 0) end = 0;

    pos = start > end ? end : start;
    max = start > end ? start : end;
    db_msg("top %d visible_cnt_ %d", top_, visible_cnt_);

    if (pos > (top_ + visible_cnt_))
        return false;
    db_msg("max %d top_ %d", max, top_);
    if (max < top_)
        return false;

    GetClientRect (hwnd, &rcInv);
    GetItemsRect ( pos, max, &rcInv);
    db_msg(" %d %d", rcInv.top, rcInv.bottom);
    if (rcInv.top < rcInv.bottom) {
        db_msg(" ");
        //InvalidateRect (hwnd, &rcInv, TRUE);
        InvalidateRect (hwnd, NULL, TRUE);
        db_msg(" ");
    }

    return true;
}

bool MenuItems::InvalidateUnderItem (HWND hwnd, int pos)
{
    db_msg(" ");

    RECT rcInv;

    if (pos > (top_ + visible_cnt_))
        return false;

    if (pos < top_)
        return false;

    GetClientRect (hwnd, &rcInv);

    GetItemsRect ( pos, -1, &rcInv);

    if (rcInv.top < rcInv.bottom)
        InvalidateRect (hwnd, &rcInv, TRUE);

    return true;
}

void MenuItems::CalcParams (const RECT* rcClient)
{
    visible_cnt_ = ((RECTHP (rcClient)) + height_ / 5) / height_;
    width_ = RECTWP(rcClient);
    db_msg("itemVisibles is %d", visible_cnt_);
}

void MenuItems::DrawItems (HWND hwnd, HDC hdc, int width)
{
    PCONTROL pWin;
    pWin = (PCONTROL) hwnd;
    if (NULL == pWin) return;

    if (!pWin->we_rdr)
    {
        db_warn("CONTROL>ListBox: NULL LFRDR.");
        return;
    }
    if (data_.size() <= 0) return;

    ItemData *data = data_[top_];
    db_debug("[fangjj]:top_:[%d]  visible_cnt_:[%d]  hilighted_idx_:[%d] count_:[%d]  \n", top_, visible_cnt_, hilighted_idx_, count_);
    SetBkMode(hdc, BM_TRANSPARENT);
    if(data->submenuflag_second == 1){
        //db_msg("[DEBUG_ZHB]=======================draw second level menu");
        DrawSecondLevelMenu(data, hdc, width, -offset_submenu);
        offset_menu = 0;
	}else{
		//db_msg("[DEBUG_ZHB]=======================draw first level menu");
		DrawFirstLevelMenu(data, hdc, width, offset_menu);
		offset_submenu = 0;
	}
}
#if 0
void MenuItems::DrawFirstLevelMenu(ItemData *data,HDC hdc, int width,int offset)
{
	 PCONTROL pWin;
    	 RECT rect, tempRect;
        int i,start_val = -1,end_val = -1;
        int x = 0, y = 0;
        int icon_width, icon_height, medial_virtical_pos;
	submenu_status = false;
	SetPenColor(hdc, gp_linec); /* set the line color */
	Line2(hdc, 160-OFFSETMAX+offset, height_, 320, height_);
	for (i = top_; data && i < (visible_cnt_ + 1+top_); i++)
	{
		rect.left  = 0;
		rect.top   = y;
		rect.right = width-OFFSETMAX+offset;
		rect.bottom = rect.top + height_;

		/* draw the hilight and unhilight item backgroud */
		if (data->dwFlags & LBIF_SELECTED) {
			db_msg(" DrawItems LBIF_SELECTED");
			SetTextColor (hdc, gp_hilite_fgc);
			/** render hilited item */
			tempRect.left	= rect.left + HILIGHT_LEFT;
			tempRect.top	= rect.top+2 ;
			tempRect.right	= rect.right - HILIGHT_RIGHT;
			tempRect.bottom = rect.bottom -1;
		 //db_msg("zhb--00-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
		 //db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
			FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
			tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, hilight_image_[0]);
		} else {
			db_msg(" DrawItems UN SELECTED");
		 	tempRect.left	 = rect.left + HILIGHT_LEFT;
			tempRect.top	= rect.top +2;
			tempRect.right	= rect.right - HILIGHT_RIGHT;
			tempRect.bottom = rect.bottom -1;
			//db_msg("zhb--11-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
		 //db_msg("zhb--unhilight-----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
			FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
			tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, hilight_image_[1]);
		}
		
	
		/*----------------------------------------------------------------*/
		if (data->dwFlags & LBIF_SELECTED)
			x= 10; //set hilight status x pos
		else
			x= 6;//unhilight status x pos
		/*----------------------------------------------------------------*/
		/* 1. display the #first_icon */
		int first_icon_index = UNHILIGHTED_ICON;
		if (hilighted_idx_ == i) {	//update hilighted image
			first_icon_index = HILIGHTED_ICON;//alloc the hilighted image
		}
		if (icon_restore_idx_ == i && (icon_restore_idx_ != hilighted_idx_)) {
			first_icon_index = UNHILIGHTED_ICON;
			icon_restore_idx_ = -1;
		}
		db_msg("first index %d, hilighted_idx_: %d, i: %d", first_icon_index, hilighted_idx_, i);
		icon_width = data->first_icon[first_icon_index]->bmWidth;  //image width
		icon_height = data->first_icon[first_icon_index]->bmHeight;//image height
		if(icon_height < height_) {
			medial_virtical_pos = (height_ - icon_height) / 2 + y;
		} else {
			medial_virtical_pos = y;
		}
		if(data->type == TYPE_IMAGE_STRING){//draw the first icon //add by zhb
		//db_msg("[debug_zhb]-----TYPE_IMAGE_STRING");
			FillBoxWithBitmap (hdc, x, medial_virtical_pos,
				icon_width, icon_height, data->first_icon[first_icon_index]);
			x += data->first_icon[first_icon_index]->bmWidth-OFFSETMAX+offset;//change tht x pos
		}
	
	    /*----------------------------------------------------------------*/
		/* 2. display #item_string */
		if (data->dwFlags & LBIF_SELECTED) {
		 SetTextColor(hdc, gp_menu_stringc_color);
			db_msg("LBIF_SELECTED, gp_selected_stringc ...... ");
		} else {
		 SetTextColor(hdc, gp_menu_stringc_color);
			db_msg("LBIF_NORMAL, gp_normal_stringc ...... ");
		}
		TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
	        db_msg("[debug_zhb]-----string = %s ",data->item_string.c_str());
		   
		if ((unsigned int)(i+1) >= data_.size()) {
			break;
		}
		data = data_[i+1];//数据指针增加
		y += height_;
	    }
	
		/*----------------------------------------------------------------*/
		//draw the restult_string item
		ItemData *subdata = data_[hilighted_idx_];
		vector<string>::const_iterator it;
		y = 0;//reset the y point
	
		//draw submenu head
		 rect.left	= width;//160
		 rect.top   = y;
		 rect.right = width;//160
		 rect.bottom = rect.top + height_;
		 x = 160-OFFSETMAX+offset; //set submenu string x
		 SetTextColor(hdc, gp_submenu_head_color);
		 TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->item_string.c_str());
		  db_msg("[debug_zhb]--submenu-head-x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->item_string.c_str());
		y += height_;
		
		//if(subdata->result_cnt <= 4)
		//	start_val = 0;
		//else
		//	start_val = subdata->sub_hilight;
		for(i = 0,it = subdata->result_string.begin(); it !=  subdata->result_string.end(); it++,i++)
		{
			if(subdata->result_cnt <= 4){
			rect.left  = width;//160
			rect.top   = y;
			rect.right = width;//160
			rect.bottom = rect.top + height_;
			//draw the restult_string item
			x = 160-OFFSETMAX+offset; //set submenu string x
			SetTextColor(hdc, gp_normal_stringc);
			TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
			db_msg("[debug_zhb]--submenu--x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
	
			//draw the restult_string image
			int second_icon_index;
			if(i == subdata->sub_hilight)//draw submenu
				 second_icon_index=1; //select
			else
				second_icon_index=0;//unselect
			 
		      icon_width = subdata->second_icon[second_icon_index]->bmWidth;  //image width
		      icon_height = subdata->second_icon[second_icon_index]->bmHeight;//image height
		      if(icon_height < height_) {
			 	medial_virtical_pos = (height_ - icon_height) / 2 + y;
		      } else {
				medial_virtical_pos = y;
		      }
		      x = width*2-icon_width-OFFSETMAX+96+offset/5;
		      db_msg("debug_zhb------x = %d  y = %d	w = %d	 h = %d ", x, medial_virtical_pos,icon_width, icon_height);
		      FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, data->second_icon[second_icon_index]);
		   
		      y += height_;
		#if 1
			}else if(subdata->result_cnt>4 && i== subdata->sub_hilight){
				rect.left  = width;//160
				rect.top   = y;
				rect.right = width;//160
				rect.bottom = rect.top + height_;
				//draw the restult_string item
				x = 160-OFFSETMAX+offset; //set submenu string x
				SetTextColor(hdc, gp_normal_stringc);
				TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
				db_msg("[debug_zhb]--submenu--x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
		
				//draw the restult_string image
				int second_icon_index;
				if(i == subdata->sub_hilight)//draw submenu
					 second_icon_index=1; //select
				else
					second_icon_index=0;//unselect
				 
			      icon_width = subdata->second_icon[second_icon_index]->bmWidth;  //image width
			      icon_height = subdata->second_icon[second_icon_index]->bmHeight;//image height
			      if(icon_height < height_) {
				 	medial_virtical_pos = (height_ - icon_height) / 2 + y;
			      } else {
					medial_virtical_pos = y;
			      }
			      x = width*2-icon_width-OFFSETMAX+96+offset/5;
			      db_msg("debug_zhb------x = %d  y = %d	w = %d	 h = %d ", x, medial_virtical_pos,icon_width, icon_height);
			      FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, data->second_icon[second_icon_index]);
			   
			      y += height_;
			}
	#endif
		}
		

}

void MenuItems::DrawSecondLevelMenu(ItemData *data, HDC hdc, int width, int offset)
{
    PCONTROL pWin;
    RECT rect, tempRect;
    int i, start_val = -1, end_val = -1;
    int x = 0, y = 0;
    int icon_width, icon_height, medial_virtical_pos;
    submenu_status = true;
    SetPenColor(hdc, gp_linec); /* set the line color */
    Line2(hdc, 30 + OFFSETMAX + offset, height_, 320, height_);
	
	if(data->value < visible_cnt_){
		start_val = 0;
		end_val = visible_cnt_;//5
	}else{
		start_val = data->value+1-visible_cnt_;
		end_val = data->value+1;
	}
      db_msg("debug_zhb------------visible_cnt_  = %d  data->value = %d start_val = %d   end_val = %d ",visible_cnt_,data->value,start_val,end_val);
    for (i = start_val;  i < end_val; i++)//draw the first icon
	{
		rect.left  = 0;
    		rect.top   = y;
    		rect.right = width-130;//30
    		rect.bottom = rect.top + height_;
		/* draw the menu hilight and unhilight item backgroud */
       	     if ( i!= data->value) {
	            db_msg(" DrawItems unhilight image----------data->value = %d",data->value);
	            /** render hilited item */
	            tempRect.left   = rect.left + HILIGHT_LEFT;
	            tempRect.top    = rect.top+2 ;
	            tempRect.right  = rect.right - HILIGHT_RIGHT;
	            tempRect.bottom = rect.bottom -1;
		     //db_msg("zhb--00-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
		     db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
	            FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
	            tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, subhilight_image_[1]);
            	   }

		x = 0;
	        icon_width = submenu_first_image_[i]->bmWidth;  //image width
	        icon_height = submenu_first_image_[i]->bmHeight;//image height
	        if(icon_height < height_) {
	            medial_virtical_pos = (height_ - icon_height) / 2 + y;
	        } else {
	            medial_virtical_pos = y;
	        }
		db_msg("debug_zhb------------x = %d   y = %d  i = %d  ",x,medial_virtical_pos,i);
		 FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, submenu_first_image_[i]);
   			 y += height_;
         }
	if(offset != -OFFSETMAX){
		  vector<string>::const_iterator it;
		  y = 0;
		  SetTextColor(hdc, gp_menu_stringc_color);
	         for(i = start_val,it = data->menuitem_string.begin(); it !=  data->menuitem_string.end(),i<end_val; it++,i++)
			{
				//draw the restult_string item
				x = 32+offset; //set submenu string x
				TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->menuitem_string[i].c_str());
		        	db_msg("[debug_zhb]--x = %d  y = %d  ---menuitem_string = %s ",x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->menuitem_string[i].c_str());
			        y += height_;
			}
		}
	
	       y = 0;		
	       //draw submenu head
		rect.left  = width-130;
	        rect.top   = y;
	        rect.right = width+130;
	        rect.bottom = rect.top + height_+30;
		 x = 40+OFFSETMAX+offset; //set submenu string x 40 offset = -120 stop
		 SetTextColor(hdc, gp_submenu_head_color);
		 TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->subitem_string.c_str());
		  db_msg("[debug_zhb]--submenu-head-x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),data->subitem_string.c_str());
		y += height_;
	for (i = top_; data && i < (visible_cnt_ + 1+top_); i++)
	{
	        rect.left  = width-130+OFFSETMAX+offset;
	        rect.top   = y;
	        rect.right = width*2;
	        rect.bottom = rect.top + height_;

        	/* draw the hilight and unhilight item backgroud */
	        if (data->dwFlags & LBIF_SELECTED) {
	            db_msg(" DrawItems LBIF_SELECTED");
	            SetTextColor (hdc, gp_hilite_fgc);
	            /** render hilited item */
	            tempRect.left   = rect.left + HILIGHT_LEFT;
	            tempRect.top    = rect.top+2 ;
	            tempRect.right  = rect.right - HILIGHT_RIGHT;
	            tempRect.bottom = rect.bottom -1;
		     //db_msg("zhb--00-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
		     db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
	            FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
	            tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, subhilight_image_[0]);
	        }
	

		x= 40+OFFSETMAX+offset;//set submenu string x 40 offset = -120 stop
		/*----------------------------------------------------------------*/
	        /*draw submenu item_string */
		  SetTextColor(hdc, gp_menu_stringc_color);
	        TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
		  db_msg("[debug_zhb]---x  = %d   y  = %d  --string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
		  
	 /*----------------------------------------------------------------*/
		//draw the restult_string image
		int third_icon_index;
		if(i == data->sub_hilight)//draw submenu
			 third_icon_index=1; //select
		else
			third_icon_index=0;//unselect
		 
               icon_width = data->third_icon[third_icon_index]->bmWidth;  //image width
               icon_height = data->third_icon[third_icon_index]->bmHeight;//image height
               if(icon_height < height_) {
               	medial_virtical_pos = (height_ - icon_height) / 2 + y;
               } else {
               	medial_virtical_pos = y;
               }
               x = width*2-icon_width+offset/5;
		db_msg("debug_zhb------x = %d  y = %d   w = %d   h = %d ", x, medial_virtical_pos,icon_width, icon_height);
               FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, data->third_icon[third_icon_index]);
		/*----------------------------------------------------------------*/
		
	        if ((unsigned int)(i+1) >= data_.size()) {
	            break;
	        }
	        data = data_[i+1];//数据指针增加
	        y += height_;
    }
}
#else
void MenuItems::DrawFirstLevelMenu(ItemData *data,HDC hdc, int width,int offset)
{
    PCONTROL pWin;
    RECT rect, tempRect;
    int i,hilight_image_index_ = -1,button_image_index_ = -1;
    uint32_t style;
    int x = 0, y = ITEM_UPPER_CLEARANCE_H;
    int icon_width, icon_height, medial_virtical_pos;
	submenu_status = false;
	SetPenColor(hdc, gp_linec); /* set the line color */
	//Line2(hdc, F_LINE_X, height_+ITEM_UPPER_CLEARANCE_H, F_LINE_W+F_LINE_X, height_+ITEM_UPPER_CLEARANCE_H);
	for (i = top_; data && i < (visible_cnt_ + 1+top_); i++)
	{
		rect.left  = F_ITEM_OFFSET_LEFT_W;
		rect.top   = y;
		rect.right = width+F_ITEM_OFFSET_LEFT_W;
		rect.bottom = rect.top + height_;

		/* draw the hilight and unhilight item backgroud */
		if ((data->dwFlags & LBIF_SELECTED) || (hilighted_idx_ == i)) {
			//db_msg(" DrawItems LBIF_SELECTED---data->fupdate = %d",data->fupdate);
			SetTextColor (hdc, gp_hilite_fgc);
			/** render hilited item */
			tempRect.left	= rect.left;
			tempRect.top	= rect.top+ITEM_UL_CLEANRANCE_H ;
			tempRect.right	= rect.right+F_ITEM_HILIGHT_W_OFFSET;
			tempRect.bottom = rect.bottom -ITEM_UL_CLEANRANCE_H;
			if((data->type == TYPE_IMAGE_BUTTON && data->fupdate) || 
                    (data->type == TYPE_IMAGE_NEED_BIND_BUTTON && data->devIsNeedBind == false))
				hilight_image_index_ = 2;
			else
				hilight_image_index_ = 0;
        } else {
           // db_error(" DrawItems UN SELECTED");
            tempRect.left	 = rect.left;
            tempRect.top	= rect.top + ITEM_UL_CLEANRANCE_H;
            tempRect.right	= rect.right;
            tempRect.bottom = rect.bottom - ITEM_UL_CLEANRANCE_H;
            if((data->type == TYPE_IMAGE_BUTTON &&data->fupdate) || 
                    (data->type == TYPE_IMAGE_NEED_BIND_BUTTON && data->devIsNeedBind == false))
                hilight_image_index_ = 3;
            else
                hilight_image_index_ = 1;
        }
		FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, hilight_image_[hilight_image_index_]);
	
		/*----------------------------------------------------------------*/
        if ((data->dwFlags & LBIF_SELECTED) || (hilighted_idx_ == i))
            x= F_F_ICON_HIGHT_X; //set hilight status x pos
        else
            x= F_F_ICON_UNHIGHT_X;//unhilight status x pos
		/*----------------------------------------------------------------*/
		/* 1. display the #first_icon */
		int first_icon_index = UNHILIGHTED_ICON;
		if (hilighted_idx_ == i) {	//update hilighted image
			first_icon_index = HILIGHTED_ICON;//alloc the hilighted image
		}

		//db_msg("first index %d, hilighted_idx_: %d, i: %d", first_icon_index, hilighted_idx_, i);
		//db_msg("debug_zhb---------------first icon path = %s",data->first_icon_path[first_icon_index].c_str());
		if(data->first_icon[first_icon_index] != NULL)
		{
			icon_width = data->first_icon[first_icon_index]->bmWidth;  //image width
			icon_height = data->first_icon[first_icon_index]->bmHeight;//image height
		}
		else
		{
			icon_width = 0;
			icon_height = 0;
		}
		if(icon_height < height_) {
			medial_virtical_pos = (height_ - icon_height) / 2 + y;
		} else {
			medial_virtical_pos = y;
		}
        if(data->first_icon[first_icon_index] != NULL)
        {
            FillBoxWithBitmap (hdc, x, medial_virtical_pos,
                    icon_width, icon_height, data->first_icon[first_icon_index]);
            x += data->first_icon[first_icon_index]->bmWidth+F_F_ICON_STRING_W;//change tht x pos
        }		
	    /*----------------------------------------------------------------*/
		/* 2. display #item_string */
        if ((data->dwFlags & LBIF_SELECTED) || (hilighted_idx_ == i)) {
            SetTextColor(hdc, gp_selected_stringc);
            //db_msg("LBIF_SELECTED, gp_selected_stringc ...... ");
        } else {
            SetTextColor(hdc, gp_normal_stringc);
            //db_msg("LBIF_NORMAL, gp_normal_stringc ...... ");
        }
        TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
        //db_msg("[debug_zhb]-----string = %s ",data->item_string.c_str());

        if ((unsigned int)(i+1) >= data_.size()) {
            break;
        }
        data = data_[i+1];//数据指针增加
        y += height_;
    }

    /*----------------------------------------------------------------*/
    //draw the restult_string item
    ItemData *subdata = data_[hilighted_idx_];
    y = ITEM_UPPER_CLEARANCE_H+20;//reset the y point

    //draw submenu head
    rect.left	= F_S_STRING_X;//
    rect.top   = y;
    rect.right = s_width;//
    rect.bottom = rect.top + height_;
    x = F_S_STRING_X; //set submenu string x
    SetTextColor(hdc, gp_submenu_head_color);
    //TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->item_string.c_str());
    TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->subitem_string.c_str());
    //db_msg("[debug_zhb]--submenu-head-x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->subitem_string.c_str());
    y += height_;
    //db_msg("zhb_debug----------test y = %d",y);
    if(subdata->type == TYPE_IMAGE_BUTTON || subdata->type == TYPE_IMAGE_NEED_BIND_BUTTON){
        //db_msg("[debug_zhb]---TYPE_IMAGE_BUTTON----button_string1 = %s  string2 = %s",subdata->button_string1.c_str(),subdata->button_string2.c_str());
        SetTextColor(hdc, gp_normal_stringc);
        x = F_S_STRING_X;
        y = F_S_STRING_ITEM_Y;
        RECT rect_;
        rect_.left = x;
        rect_.top = y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1);
        rect_.right= s_width;
        rect_.bottom= rect_.top + F_S_STRING_ITEM_H*2;
        //db_msg("zhb----x = %d , y = %d  w = %d  h = %d",rect_.left,rect_.top,rect_.right-rect_.left,rect_.bottom-rect_.top);
        DrawText( hdc, subdata->button_string1.c_str(), -1, &rect_, DT_LEFT|DT_WORDBREAK);
        if(subdata->type == TYPE_IMAGE_BUTTON && subdata->fupdate){
            button_image_index_ = 2;
            style = DT_CENTER;//DT_LEFT;
        }else if(subdata->type == TYPE_IMAGE_NEED_BIND_BUTTON && subdata->devIsNeedBind == false){
            button_image_index_ = 3;
            style = DT_CENTER;
        }else
        {
            button_image_index_ = 0;
            style = DT_CENTER;
        }

        icon_width = button_image_[button_image_index_]->bmWidth;  //image width
        icon_height = button_image_[button_image_index_]->bmHeight;//image height
        x = F_BUTTON_X;
        y = rect_.bottom+F_BUTTONG_STRING_OFFSET;
		//db_error("[debug_jason] : xxxxxx x= %d ; y = %d ; icon_width = %d ;icon_height = %d",x,y,icon_width,icon_height);
        FillBoxWithBitmap (hdc, x, y,icon_width,icon_height, button_image_[button_image_index_]);
        rect_.left = x;
        rect_.top = y + ((height_ - GetCurFont(hdc)->size) >> 1);
        rect_.right= x+icon_width;
        rect_.bottom= y+icon_height;
        DrawText( hdc, subdata->button_string2.c_str(), -1, &rect_, style);
    }else if(subdata->type == TYPE_IMAGE_DATATIME ){ 
        //db_msg("[debug_zhb]---TYPE_IMAGE_DATATIME----  time_string = %s",subdata->time_string.c_str());
        SetTextColor(hdc, gp_normal_stringc);
        subdata->time_string = getCurrentTimeStr();
        TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->time_string.c_str());

    }else if(subdata->type ==TYPE_IMAGE_STRING_ONLY){
        //db_msg("[debug_zhb]---TYPE_IMAGE_STRING_ONLY----  button_string1 = %s",subdata->button_string1.c_str());
        SetTextColor(hdc, gp_normal_stringc);
        //TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),subdata->button_string1.c_str());
        x = F_S_STRING_X;
        y = F_S_STRING_ITEM_Y;
        RECT rect_;
        rect_.left = x;
        rect_.top = y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1);
        rect_.right= s_width;
        rect_.bottom= rect_.top + F_S_STRING_ITEM_H*2;
        DrawText(hdc,subdata->button_string1.c_str(),-1, &rect_,DT_LEFT|DT_WORDBREAK);
    }else{
        //db_msg("[debug_zhb]-------subdata->subitemcnt  = %d",subdata->subitemcnt);
        std::vector<std::string>::const_iterator it;
        for(i = 0 ,it = subdata->result_string.begin(); it !=  subdata->result_string.end() && i < subdata->subitemcnt; it++,i++)
        {

            rect.left  = F_S_STRING_X;
            rect.top   = y;
            rect.right = s_width-F_S_STRING_X;//160
            rect.bottom = rect.top + F_S_STRING_ITEM_H;
            //draw the restult_string item
            x = F_S_STRING_X; //set submenu string x
            SetTextColor(hdc, gp_selected_stringc);
            int retType = -1;
            retType = getSubMenuStringType(subdata,subdata->result_string[i]);
            if(retType == TYPE_SUBITEM_SET){
                int retvalue = -1;
                std::stringstream ss;
                std::string str;
                retvalue = getSubMenuStringValue(subdata,subdata->result_string[i]);
                ss << (retvalue+1)<<getLevelStr();
                str = subdata->result_string[i];
                str +=ss.str();
                TextOut (hdc, x, y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1),str.c_str());
                //db_msg("[debug_zhb]--submenu-string = %s ",str.c_str());
            }else{
                TextOut (hdc, x, y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
                //db_msg("[debug_zhb]--submenu--x = %d	y = %d -string = %s ",x,y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1),subdata->result_string[i].c_str());
            }
			int third_icon_index;
			int index;
            if(retType == TYPE_SUBITEM_SWITCH){
              //  x = s_width- F_LASTER_ICON_OFFSET-25;
              //  string str;
                int retvalue = -1;
               // retvalue = getSwitchValue();
				retvalue = getSubMenuStringValue(subdata,subdata->result_string[i]);
			//	db_error("[debug_zhb]-------------getSubMenuStringValue(data) = %d",retvalue);
				if(retvalue == 0 && (!(subdata->dwFlags & LBIF_SELECTED)))
					third_icon_index = SUBSWITCH_OFF;//6;//subswitch_off
				else if(retvalue == 0 && (subdata->dwFlags & LBIF_SELECTED))
					third_icon_index = SUBSWITCH_OFF_H;//7;//subswitch_off_h
				else if(retvalue == 1 &&  (!(subdata->dwFlags & LBIF_SELECTED)))
					third_icon_index = SUBSWITCH_ON;//4;//subswitch_on
				else if(retvalue == 1 &&  (subdata->dwFlags & LBIF_SELECTED))
					third_icon_index = SUBSWITCH_ON_H;//5;//subswitch_on_h

				if(subdata->third_icon[third_icon_index] != NULL)
				{
		           icon_width = subdata->third_icon[third_icon_index]->bmWidth;  //image width
		           icon_height = subdata->third_icon[third_icon_index]->bmHeight;//image height
				}else{
				   icon_width = 0;
				   icon_height = 0;
				}
		        if(icon_height < S_ITEM_STRING_BG_H) {
		           medial_virtical_pos = (S_ITEM_STRING_BG_H - icon_height) / 2 + y;
		        } else {
		           medial_virtical_pos = y;
		        }
		        x = s_width-icon_width-F_LASTER_ICON_OFFSET;
				  if(i == first_ts_current_id)//draw submenu
                      index=1; //select
                  else
                      index=0;//unselect

				FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, subdata->third_icon[third_icon_index]);
				if(strcmp(subdata->subitem_string.c_str(),DASH_CAMERA_SETTING) == 0){
				SetTextColor(hdc, gp_normal_stringc);
        		RECT rect_;
        		rect_.left = F_S_STRING_X+10;
       			rect_.top = 310;
        		rect_.right= s_width;
        		rect_.bottom= rect_.top + F_S_STRING_ITEM_H*2;
        		DrawText(hdc,subdata->item_tips.c_str(),-1, &rect_,DT_LEFT|DT_WORDBREAK);
				}
 //               if(retvalue)
 //                 str = subdata->on_string;
 //               else
 //                   str = subdata->off_string;
 //               TextOut (hdc, x, y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1),str.c_str());
                //db_msg("[debug_zhb]--submenu--x = %d	y = %d -string = %s ",x,y + ((F_S_STRING_ITEM_H - GetCurFont(hdc)->size) >> 1),str.c_str());
            }else{
                int second_icon_index;
                switch(retType){
                    case TYPE_SUBITEM_CHOICE:
                        {
                            if(i == first_ts_current_id)//draw submenu
                                second_icon_index=1; //select
                            else
                                second_icon_index=0;//unselect
                            if(  subdata->second_icon[second_icon_index] != NULL)
                            {
                                icon_width = subdata->second_icon[second_icon_index]->bmWidth;  //image width
                                icon_height = subdata->second_icon[second_icon_index]->bmHeight;//image height
                            }
                            else
                            {
                                icon_width = 0;
                                icon_height = 0;
                            }
                        }break;
                    case TYPE_SUBITEM_SET:
                        {
                            second_icon_index=SUBSETTING;//10; //subsetting
                            if( subdata->third_icon[second_icon_index] )
                            {
                                icon_width = subdata->third_icon[second_icon_index]->bmWidth;  //image width
                                icon_height = subdata->third_icon[second_icon_index]->bmHeight;//image height
                            }
                            else
                            {
                                icon_width = 0;
                                icon_height = 0;
                            }
                        }break;
                    default:
                        break;
                }

                //draw the restult_string image

                if(icon_height < F_S_STRING_ITEM_H) {
                    medial_virtical_pos = (F_S_STRING_ITEM_H - icon_height) / 2 + y;
                } else {
                    medial_virtical_pos = y;
                }
                x = s_width-icon_width - F_LASTER_ICON_OFFSET;
     //           db_error("debug_zhb------x = %d  y = %d	w = %d	 h = %d ", x, medial_virtical_pos,icon_width, icon_height);
                if(second_icon_index ==1)//
                {
                    FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height,data->third_icon[SELECT_ON_H]);
					//FillBoxWithBitmap (hdc, F_S_STRING_X, medial_virtical_pos-10,442, 66,frist_bg_image_);
                }else{
                    FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height,data->third_icon[SELECT_OFF]);
                }				
            }
            y += F_S_STRING_ITEM_H;
        }
    }
}

void MenuItems::DrawSecondLevelMenu(ItemData *data,HDC hdc, int width,int offset)
{
    PCONTROL pWin;
    RECT rect, tempRect;
    int i,start_val = -1,end_val = -1,subhilight_image_index_ = -1;
    int x = 0, y = ITEM_UPPER_CLEARANCE_H;
    int icon_width, icon_height, medial_virtical_pos;
    submenu_status = true;
    SetPenColor(hdc, gp_linec); /* set the line color */
    Line2(hdc, S_LINE_X, height_+ITEM_UPPER_CLEARANCE_H, S_LINE_W+S_LINE_X, height_+ITEM_UPPER_CLEARANCE_H);
	
	if(data->value < visible_cnt_){
		start_val = 0;
		end_val = ITEM_VISIBLE;
	}else{
		start_val = data->value-ITEM_VISIBLE+1;
		end_val = data->value+1;
	}
     // db_msg("debug_zhb------------data->value = %d start_val = %d   end_val = %d ",data->value,start_val,end_val);
    for (i = start_val;  i <end_val; i++)//draw the first icon
	{
		rect.left  = 0;
    		rect.top   = y;
    		rect.right = S_F_ICON_BG_W;//30
    		rect.bottom = rect.top + S_F_ICON_BG_H;
		/* draw the menu hilight and unhilight item backgroud */
       	     if ( i!= data->value) {
	            //db_msg(" DrawItems unhilight image----------data->value = %d",data->value);
	            /** render hilited item */
	            tempRect.left   = rect.left;
	            tempRect.top    = rect.top ;
	            tempRect.right  = rect.right;
	            tempRect.bottom = rect.bottom;
		     //db_msg("zhb--00-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
		    // db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
		    if(data->value == start_val && i == start_val+1){

				subhilight_image_index_ = 1;
		     	}else if(data->value == end_val -1&& i == end_val-2 ){
				subhilight_image_index_ = 3;
		     		}else if(data->value > start_val && data->value < end_val -1){
						if(i == data->value-1)
							subhilight_image_index_ = 3;
						else if(i == data->value+1)
							subhilight_image_index_ = 1;
						else 
							subhilight_image_index_ = 2;
		     			}else{
							subhilight_image_index_ = 2;
		     				}
		    FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
	            tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, subhilight_image_[subhilight_image_index_]);
            	   }

		x = S_F_ICON_P;
	        icon_width = submenu_first_image_[i]->bmWidth;  //image width
	        icon_height = submenu_first_image_[i]->bmHeight;//image height
	        if(icon_height < S_F_ICON_BG_H) {
	            medial_virtical_pos = (S_F_ICON_BG_H - icon_height) / 2 + y;
	        } else {
	            medial_virtical_pos = y;
	        }
		//db_msg("debug_zhb------------x = %d   y = %d  i = %d  ",x,medial_virtical_pos,i);
		if(i!= data->value)
		 	FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, submenu_first_image_w[i]);
		else
			FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, submenu_first_image_[i]);
   			 y += S_F_ICON_BG_H;

         }
	  
	
		//draw submenu head
		y = ITEM_UPPER_CLEARANCE_H+ITEM_UL_CLEANRANCE_H;//reset the y point
		rect.left  = S_HEAD_STRING_X;
	        rect.top   = y;
	        rect.right = s_width;//width+130;
	        rect.bottom = rect.top + height_;
		 x = S_HEAD_STRING_X; //set submenu string x
		 SetTextColor(hdc, gp_submenu_head_color);
		 TextOut (hdc, x, y + ((height_ - GetCurFont(hdc)->size) >> 1),data->subitem_string.c_str());
		 // db_msg("[debug_zhb]--submenu-head-x = %d	y = %d -string = %s ",x,y + ((height_ - GetCurFont(hdc)->size) >> 1),data->subitem_string.c_str());
		//y += height_;
		y = S_ITEM_STRING_BG_OFFSET_TOP;
	if(data->type == TYPE_IMAGE_SELECT){
		for (i = top_; data && i < (visible_cnt_+1+top_)&& i < count_ ; i++)//
			{
				//db_msg("[debug_zhb]-----------data_size() = %d  top_ =%d",data_.size(),top_);
			        rect.left  = S_HEAD_STRING_X;
			        rect.top   = y;
			        rect.right = S_ITEM_STRING_BG_W+S_HEAD_STRING_X;
			        rect.bottom = rect.top + S_ITEM_STRING_BG_H;

		        	/* draw the hilight and unhilight item backgroud */
			        if (data->dwFlags & LBIF_SELECTED) {
			            db_msg(" DrawItems LBIF_SELECTED");
			            SetTextColor (hdc, gp_hilite_fgc);
			            /** render hilited item */
			            tempRect.left   = rect.left;
			            tempRect.top    = rect.top;//+2 ;
			            tempRect.right  = rect.right;
			            tempRect.bottom = rect.bottom;// -1;
				     //db_msg("zhb--00-----tempRect.left = %d tempRect.top = %d tempRect.right = %d tempRect.bottom = %d",tempRect.left,tempRect.top,tempRect.right,tempRect.bottom);
				    // db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",tempRect.left,tempRect.top,tempRect.right-tempRect.left,tempRect.bottom-tempRect.top);  
			            FillBoxWithBitmap (hdc, tempRect.left, tempRect.top,
			            tempRect.right-tempRect.left,tempRect.bottom-tempRect.top, subhilight_image_[0]);
				     SaveHilightItemRect(tempRect);
			        }
				x= S_STRING_P;
				/*----------------------------------------------------------------*/
				//draw the submenu item line ????????????????????
				int retType = -1;
			 	retType = getSubMenuStringType(data,data->item_string);
				if(retType == TYPE_SUBMENU_SUBITEM_SWITCH || retType == TYPE_SUBMENU_SUBITEM_ENLARGE || retType == TYPE_SUBMENU_SUBITEM_CHOICE){
					SetPenColor(hdc, 0xFF3C3F55);
					Line2(hdc, x, y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1)+8, x+15, y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1)+8);
					x+=15;
					}
			        /*draw submenu item_string */
				  SetTextColor(hdc, gp_selected_stringc);
				if(retType == TYPE_SUBITEM_SET){
					int retvalue = -1;
					std::stringstream ss;
					std::string str;
					retvalue = getSubMenuStringValue(data,data->item_string);
					ss << (retvalue+1)<<getLevelStr();
					str = data->item_string;
					str +=ss.str();
					TextOut (hdc, x, y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1),str.c_str());
				}else{
			        	TextOut (hdc, x, y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
					}
				  //db_msg("[debug_zhb]---x  = %d   y  = %d  --string = %s ",x,y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1),data->item_string.c_str());
			
			 /*----------------------------------------------------------------*/
				//draw the restult_string image
				//db_msg("[debug_zhb]------------i = %d -retType = %d  data->sub_hilight = %d",i,retType,data->sub_hilight);
				int third_icon_index;
				switch(retType){
					case TYPE_SUBITEM_CHOICE:
						{
							if(i == data->sub_hilight && (data->dwFlags & LBIF_SELECTED))//draw submenu
								 third_icon_index=SELECT_ON_H;//1; //select_on_h
							else if(i == data->sub_hilight && (!(data->dwFlags & LBIF_SELECTED)))
								third_icon_index=SELECT_ON;//0;//select_on
							else
								third_icon_index=SELECT_OFF;//2;//select_off
						}
						break;
					case TYPE_SUBMENU_SUBITEM_CHOICE:
						{
							int ret = -1;
							ret = getSubMenuStringValue(data,data->item_string);
							//db_msg("[debug_zhb]-------------data->item_string = %s   getSubMenuStringValue(data) = %d",data->item_string.c_str(),ret);
							if(ret == 0)
								third_icon_index = SELECT_OFF;//2;//select_off
							else if(ret == 1 &&  (!(data->dwFlags & LBIF_SELECTED)))
								third_icon_index = SELECT_ON;//0;//select_on
							else if(ret == 1 &&  (data->dwFlags & LBIF_SELECTED))
								third_icon_index = SELECT_ON_H;//1;//select_on_h
						}break;
					case TYPE_SUBITEM_SWITCH:
					case TYPE_SUBMENU_SUBITEM_SWITCH:
						{
							int ret = -1;
							ret = getSubMenuStringValue(data,data->item_string);
							//db_msg("[debug_zhb]-------------getSubMenuStringValue(data) = %d",ret);
							if(ret == 0 && (!(data->dwFlags & LBIF_SELECTED)))
								third_icon_index = SUBSWITCH_OFF;//6;//subswitch_off
							else if(ret == 0 && (data->dwFlags & LBIF_SELECTED))
								third_icon_index = SUBSWITCH_OFF_H;//7;//subswitch_off_h
							else if(ret == 1 &&  (!(data->dwFlags & LBIF_SELECTED)))
								third_icon_index = SUBSWITCH_ON;//4;//subswitch_on
							else if(ret == 1 &&  (data->dwFlags & LBIF_SELECTED))
								third_icon_index = SUBSWITCH_ON_H;//5;//subswitch_on_h
						}
						break;
					case TYPE_SUBITEM_ENLARGE:
					case TYPE_SUBMENU_SUBITEM_ENLARGE:
							if(data->dwFlags & LBIF_SELECTED)
								third_icon_index = ENLARGE_H;//9;//enlarge_h
							else
								third_icon_index = ENLARGE;//8;//enlarge
						break;
					case TYPE_SUBITEM_SET:
						if(data->dwFlags & LBIF_SELECTED)
							third_icon_index = SUBSETTING_H;//11;//subsetting_h
						else
							third_icon_index = SUBSETTING;//10;//subsetting
						break;
					case TYPE_SUBITEM_UPDATE:
						if(data->dwFlags & LBIF_SELECTED)
							third_icon_index = UPLOAD_H;//13;//update_h
						else
							third_icon_index =UPLOAD;// 12;//update
						break;
					default:

						break;
				
					}
					if( data->third_icon[third_icon_index] != NULL)
					{
		               icon_width = data->third_icon[third_icon_index]->bmWidth;  //image width
		               icon_height = data->third_icon[third_icon_index]->bmHeight;//image height
					}
					else
					{
						icon_width = 0;
						icon_height = 0;
					}
		               if(icon_height < S_ITEM_STRING_BG_H) {
		               	medial_virtical_pos = (S_ITEM_STRING_BG_H - icon_height) / 2 + y;
		               } else {
		               	medial_virtical_pos = y;
		               }
		               x = s_width-icon_width-S_LASTER_ICON_OFFSET;
				//db_msg("debug_zhb------third_icon_index = %d x = %d  y = %d   w = %d   h = %d ",third_icon_index, x, medial_virtical_pos,icon_width, icon_height);
		               FillBoxWithBitmap (hdc, x, medial_virtical_pos,icon_width, icon_height, data->third_icon[third_icon_index]);
				/*----------------------------------------------------------------*/
				
			        if ((unsigned int)(i+1) >= data_.size()) {
			            break;
			        }
			        data = data_[i+1];//数据指针增加
			        y += S_ITEM_STRING_BG_H;
		    }
			//draw the tips:
			x = S_HEAD_STRING_X;
			//y += S_ITEM_STRING_BG_H;
			y = s_height - 75;
			RECT _rect_;
			_rect_.left = x;
			_rect_.top = y;
			_rect_.right = s_width-S_ITEM_STRING_BG_OFFSET;//offset 60
			_rect_.bottom = s_height;//
			SetTextColor(hdc, gp_normal_stringc);
			DrawText( hdc, data->item_tips.c_str(), -1, &_rect_, DT_LEFT|DT_WORDBREAK);
			//db_msg("[debug_zhb]-----TYPE_IMAGE_SELECT------item_tips = %s",data->item_tips.c_str());
		}else if(data->type == TYPE_IMAGE_BUTTON || data->type == TYPE_IMAGE_STRING_ONLY){
			//db_msg("[debug_zhb]---TYPE_IMAGE_BUTTON----button_string1 = %s ",data->button_string1.c_str());
			SetTextColor(hdc, gp_selected_stringc);
			x = S_STRING_P;
			RECT rect_;
			rect_.left = x;
			rect_.top = y + ((height_ - GetCurFont(hdc)->size) >> 1);
			rect_.right= s_width-S_LASTER_ICON_OFFSET;
			rect_.bottom= S_ITEM_STRING_BG_H*3+rect_.top;
			DrawText( hdc, data->button_string1.c_str(), -1, &rect_, DT_LEFT|DT_WORDBREAK);
			if(data->type != TYPE_IMAGE_STRING_ONLY){
				icon_width = button_image_[1]->bmWidth;  //image width
				icon_height = button_image_[1]->bmHeight;//image height
				x = S_BUTTON_X;
				y = S_BUTTONG_STRING_OFFSET+rect_.bottom-70;
				FillBoxWithBitmap (hdc, x, y,icon_width,icon_height, button_image_[1]);
				rect_.left = x;
				rect_.top = y + ((height_ - GetCurFont(hdc)->size) >> 1);
				rect_.right= x+icon_width;
				rect_.bottom= y+icon_height;
				//db_msg("[debug_zhb]--- string2 = %s",data->button_string2.c_str());
				DrawText( hdc, data->button_string2.c_str(), -1, &rect_, DT_CENTER|DT_WORDBREAK);
			}
		}else if(data->type == TYPE_IMAGE_DATATIME){
				//db_msg("[debug_zhb]---TYPE_IMAGE_DATATIME----");
					x = S_STRING_P;
					y += 20;
					SetTextColor (hdc, gp_hilite_fgc);
					//db_msg("[debug_zhb]---x  = %d   y  = %d  --string = %s ",x,y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1),data->time_string.c_str());
					TextOut (hdc, x, y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1),data->time_string.c_str());
					//draw button icon
					icon_width = button_image_[1]->bmWidth;  //image width
					icon_height = button_image_[1]->bmHeight;//image height
					x = S_BUTTON_X;
					y += S_BUTTONG_STRING_OFFSET+100;
					RECT _rect_;
					for (i = top_; data && i < (visible_cnt_+1+top_)&& i < count_ ; i++)//
					{
						_rect_.left = x;
						_rect_.top = y;
						_rect_.right = x+icon_width;
						_rect_.bottom = y+icon_height;
						if (data->dwFlags & LBIF_SELECTED) {
					            //db_msg(" DrawItems DATATIME BUTTON LBIF_SELECTED");
					           
						    // db_msg("zhb---hilight----x = %d y = %d w = %d h = %d",_rect_.left,_rect_.top,_rect_.right-_rect_.left,_rect_.bottom-_rect_.top);  
					            FillBoxWithBitmap (hdc, _rect_.left, _rect_.top,_rect_.right-_rect_.left,_rect_.bottom-_rect_.top, button_image_[1]);
					        }else{
							//db_msg(" DrawItems DATATIME BUTTON NOT LBIF_SELECTED");
							//db_msg("zhb---unhilight----x = %d y = %d w = %d h = %d",_rect_.left,_rect_.top,_rect_.right-_rect_.left,_rect_.bottom-_rect_.top);  
					               FillBoxWithBitmap (hdc, _rect_.left, _rect_.top,_rect_.right-_rect_.left,_rect_.bottom-_rect_.top, button_image_[0]);
					        	}
						_rect_.top = y + ((S_ITEM_STRING_BG_H - GetCurFont(hdc)->size) >> 1);
						DrawText( hdc, data->item_string.c_str(), -1, &_rect_, DT_CENTER|DT_WORDBREAK);
						goto draw_tip;
						data = data_[i+1];//数据指针增加
			       	 		x += icon_width+5;
					}
					draw_tip:
					//draw the tips
					x = S_HEAD_STRING_X;
					//y =S_ITEM_STRING_BG_H+_rect_.bottom;
					y = s_height - 90;
	 				_rect_.left = x;
	 				_rect_.top = y;
	 				_rect_.right = s_width-S_LASTER_ICON_OFFSET;
	 				_rect_.bottom = s_height;
					SetTextColor(hdc, gp_normal_stringc);
	 				DrawText( hdc, data->item_tips.c_str(), -1, &_rect_, DT_LEFT|DT_WORDBREAK);
					//db_msg("[debug_zhb]-----data time------item_tips = %s",data->item_tips.c_str());
				}
	m_add_data_finish = false;
	setPaintSecondLevelFlag(false);
}
#endif

int MenuItems::getSubMenuStringType(ItemData *data,std::string &str)
{
	int ret = -1;
	StringIntMap::iterator it;
	it = data->subMenuStringType.find(str.c_str());//找到当前item的value
		if(it!=data->subMenuStringType.end()){
			ret = it->second;
			//db_msg("debug_zhb----------menu_item---getSubMenuStringType----it->c_str = %s- it->second = %d",str.c_str(),ret);
		}
	return ret;
}

int MenuItems::getSubMenuStringValue(ItemData *data,std::string &str)
{
	int ret = -1;
	StringIntMap::iterator it;
	//db_error("debug_zhb----------menu_item---getSubMenuStringValue----- it->c_str = %s",str.c_str());
	it = data->subMenuStringvalue.find(str.c_str());//找到当前item的value
		if(it!=data->subMenuStringvalue.end()){
			ret = it->second;
			//db_error("debug_zhb----------menu_item---getSubMenuStringValue----- it->second = %d",ret);
		}
	return ret;
}

void MenuItems::changeItemData(ItemData *itemdatadest,ItemData *itemdatasrc)
{
	itemdatadest->first_icon[UNHILIGHTED_ICON] = AllocImage((char*)
	                    (itemdatasrc->first_icon_path[UNHILIGHTED_ICON].c_str()));
        itemdatadest->first_icon[HILIGHTED_ICON]   = AllocImage((char*)
                (itemdatasrc->first_icon_path[HILIGHTED_ICON].c_str()));
	 itemdatadest->second_icon[UNHILIGHTED_ICON] = AllocImage((char*)
                (itemdatasrc->second_icon_path[UNHILIGHTED_ICON].c_str()));
        itemdatadest->second_icon[HILIGHTED_ICON]   = AllocImage((char*)
                (itemdatasrc->second_icon_path[HILIGHTED_ICON].c_str()));
	for(int k = 0; k < THIRD_ICON_NUM; k++)
		itemdatadest->third_icon[k]   = AllocImage((char*)
                (itemdatasrc->third_icon_path[k].c_str()));
	itemdatadest->item_string        = itemdatasrc->item_string;
	//db_msg("--11----  itemdatades->item_string = %s  .item_string,data_[index-1]->item_string.c_str() = %s\n",itemdatadest->item_string.c_str(),itemdatasrc->item_string.c_str());
	itemdatadest->subitem_string        = itemdatasrc->subitem_string;
	itemdatadest->result_string      = itemdatasrc->result_string;
	itemdatadest->menuitem_string= itemdatasrc->menuitem_string;
	itemdatadest->type               = itemdatasrc->type;
	itemdatadest->result_image       = itemdatasrc->result_image;
	itemdatadest->value              = itemdatasrc->value;//menu hilight
	itemdatadest->sub_hilight = itemdatasrc->sub_hilight;//submenu hilight
	itemdatadest->result_cnt         = itemdatasrc->result_cnt;
	itemdatadest->subMenuStringType = itemdatasrc->subMenuStringType;
	itemdatadest->subMenuStringvalue = itemdatasrc->subMenuStringvalue;
	itemdatadest->subitemcnt = itemdatasrc->subitemcnt;
	itemdatadest->submenuflag_second = itemdatasrc->submenuflag_second;
	itemdatadest->dwFlags=itemdatasrc->dwFlags;
	itemdatadest->off_string = itemdatasrc->off_string;
	itemdatadest->on_string = itemdatasrc->on_string;
	itemdatadest->fupdate= itemdatasrc->fupdate;
}

int MenuItems::SelectItem (HWND hwnd, unsigned int newSel)
{
    db_msg("zhb-----------SeletItem newSel = %d",newSel);
     RECT rcInv;
    ItemData *plbi, *newItem;
    unsigned int index;

    newItem = data_[newSel];

    if (newItem->dwFlags & LBIF_DISABLE)
        return newSel;

#ifdef _DEBUG
    if (!newItem)
        _MG_PRINTF ("CONTROL>ListBox: return value of lstGetItem is NULL. ");
#endif

    if (GetWindowStyle (hwnd) & LBS_MULTIPLESEL) {
        newItem->dwFlags ^= LBIF_SELECTED;
        return newSel;
    }

    for(index=0; index<data_.size(); index++) {
        plbi = data_[index];
        if (plbi->dwFlags & LBIF_SELECTED) {
            if (index != newSel) {
                plbi->dwFlags &= ~LBIF_SELECTED;
                newItem->dwFlags |= LBIF_SELECTED;
                return index;
            }
        }
    }
    newItem->dwFlags |= LBIF_SELECTED;
    return hilighted_idx_;
}

int MenuItems::CancleSelected (HWND hwnd)
{
    db_msg(" ");

    ItemData *plbi;
    unsigned int index;

    for(index=0; index<data_.size(); index++) {
        plbi = data_[index];
        if (plbi->dwFlags & LBIF_SELECTED) {
            RECT rc;
            icon_restore_idx_ = index;
            plbi->dwFlags &= ~LBIF_SELECTED;
            GetItemsRect ( index, index, &rc);
            InvalidateRect (hwnd, &rc, TRUE);
            return index;
        }
    }

    return -1;
}

int MenuItems::CancleAllHilight()
{
    return this->CancleSelected(handle_);
}

void MenuItems::DrawFocusRect (HWND hwnd, HDC hdc)
{
    db_msg(" ");

    RECT rc;
    PCONTROL pWin;
    DWORD light_dword;
    pWin = (PCONTROL) hwnd;
    if (!pWin || !pWin->we_rdr) return;
    if (hilighted_idx_ < top_
            || hilighted_idx_ > (top_ + visible_cnt_))
        return;

    if (dwFlags_ & LBF_FOCUS) {
        GetClientRect (hwnd, &rc);
        GetItemsRect (hilighted_idx_, hilighted_idx_, &rc);
        InflateRect (&rc, -1, -1);
        light_dword = pWin->we_rdr->calc_3dbox_color
            (GetWindowElementAttr (hwnd, WE_BGC_WINDOW),
             LFRDR_3DBOX_COLOR_LIGHTEST);
        pWin->we_rdr->draw_focus_frame (hdc, &rc, light_dword);
    }
}


void MenuItems::ChangeScrollbar (HWND hwnd, BOOL bShow, int iSBar)
{
    db_msg(" ");

    if (scrollbar_policy_ == SB_POLICY_ALWAYS) {
        if (iSBar != SB_VERT)
            EnableScrollBar (hwnd, iSBar, bShow);
    }
    else if (scrollbar_policy_ == SB_POLICY_AUTOMATIC) {
        ShowScrollBar (hwnd, iSBar, bShow);
    }
}

void MenuItems::SetVScrollbar (HWND hwnd, BOOL fRedraw)
{
    db_msg(" ");
    SCROLLINFO si;
    memset(&si, 0, sizeof(si));

    if (scrollbar_policy_ == SB_POLICY_NEVER) {
        ShowScrollBar (hwnd, SB_VERT, 0);
        return;
    }
    if (visible_cnt_ >= count_) {   //no need to show scrollbar
        SetScrollPos (hwnd, SB_VERT, 0);
        if (scrollbar_policy_ == SB_POLICY_ALWAYS) {
            si.fMask = SIF_PAGE;
            si.nMax  = 1;
            si.nMin  = 0;
            si.nPage = 0;
            SetScrollInfo (hwnd, SB_VERT, &si, fRedraw);
        }
        ChangeScrollbar (hwnd, 0, SB_VERT);
        return;
    }

    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMax = count_*height_ - 1;
    if (si.nMax < 0) si.nMax = 0; /*when Init. itemcount is 0*/
    si.nMin = 0;
    si.nPage = MIN (visible_cnt_, count_)*height_;
    si.nPos = top_ * height_;
    SetScrollInfo (hwnd, SB_VERT, &si, fRedraw);
    EnableScrollBar (hwnd, SB_VERT, TRUE);
    ChangeScrollbar (hwnd, TRUE, SB_VERT);
}

void MenuItems::SetHScrollbar (HWND hwnd, BOOL fRedraw)
{
    db_msg(" ");
    SCROLLINFO si;
    memset(&si, 0, sizeof(si));

    if (scrollbar_policy_ == SB_POLICY_NEVER) {
        ShowScrollBar (hwnd, SB_HORZ, 0);
        return;
    }

    if (width_ >= max_width_) {
        SetScrollPos (hwnd, SB_HORZ, 0);
        ChangeScrollbar (hwnd,  0, SB_HORZ);
        return;
    }

    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMax = max_width_;
    si.nMin = 0;
    si.nPage = width_;
    si.nPos = left_;
    SetScrollInfo (hwnd, SB_HORZ, &si, fRedraw);
    EnableScrollBar (hwnd, SB_HORZ, TRUE);
    ChangeScrollbar (hwnd,  TRUE, SB_HORZ);
}

void MenuItems::SetScrollbar(HWND hwnd, BOOL fRedraw)
{
    SetVScrollbar (hwnd, fRedraw);
    SetHScrollbar (hwnd, fRedraw);
}

static int ClickItem(HWND hwnd, int menu_index)
{
    db_msg(" ");
//  PMENULISTITEMINFO data = listbox_data_->menu_item_info;
//
//  if (data[menu_index].flagsEX.valueFlag[0] & VMFLAG_IMAGE) {
//      ChangeSwitchIcon(hwnd, menu_index);
//  } else if (data[menu_index].flagsEX.valueFlag[1] & VMFLAG_STRING) {
//      listbox_data_->add_data->callback->callback(MSG_MENULIST_SET_CURRENT_VALUE,
//listbox_data_->add_data->is_submenu, (unsigned int*)menu_index);
//      listbox_data_->add_data->is_submenu = 1;
//      db_msg("ppp %p", listbox_data_->add_data->callback);
//      int select_submenu_index = ShowSubMenu(hwnd, listbox_data_->add_data);
//      char str[23];
//      sprintf(str, "clicked %d", select_submenu_index);
        //MessageBox(hwnd, str, "haha", 0);
//      if (select_submenu_index >= 0) {//has modified
//          listbox_data_->add_data->callback->callback(MSG_MENULIST_SET_CURRENT_VALUE,
//listbox_data_->add_data->is_submenu, (unsigned int*)select_submenu_index);
//          ChangeItemLabel(hwnd, menu_index, select_submenu_index);
//      }
//      listbox_data_->add_data->is_submenu = 0;
//  }
    return 0;
}

//@todo
typedef struct MenulistAttr{
    unsigned int normalBgc;
    unsigned int normalFgc;
    unsigned int linec;
    unsigned int normalStringc;
    unsigned int normalValuec;
    unsigned int selectedStringc;
    unsigned int selectedValuec;
    unsigned int scrollbarc;
    int itemHeight;
    void* context;
    int sub_menu_flag;
}MenulistAttr;


MenulistAttr trial_data;

extern const char*  Message2Str (int message);

int MenuItems::setSubMenuItemPos(int value)
{
	first_ts_current_id = value;
	return 0;
}

int MenuItems::setNeedPrint(bool Print_flag)
{
	need_print_flag = Print_flag;
	return 0;
}


int MenuItems::first_MenuItem_Message(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    int oldSel, mouseX, mouseY, hit;
    RECT rcInv, rcInvSubmenu;
    BOOL click_mark = 0;
    ItemData *hitItem;

    PCONTROL    pCtrl = gui_Control(hwnd);
    DWORD       dwStyle = pCtrl->dwStyle;
    
    if (count_ == 0)
        return -1;

    mouseX = LOSWORD (lparam);
    mouseY = HISWORD (lparam);
   
    if((mouseX > F_LINE_X) && (mouseY > height_+ITEM_UPPER_CLEARANCE_H)){
		if(need_print_flag){
			GetClientRect (hwnd, &rcInv);
//			db_error("[debug_jaosn]:rcInv.left = %d，rcInv.right = %d, rcInv.top = %d,rcInv.bottom = %d",rcInv.left,rcInv.right,rcInv.top,rcInv.bottom);
    		InvalidateRect (hwnd, &rcInv, TRUE);
		}else{
			//db_error("[debug_jaosn]: no need print subMenuitem");
		}
		return -1;
	}else if((mouseX > F_LINE_X) && (mouseY < 97))
	{
		//db_error("[debug_jason]: no need do anything");
		return -1;
	}
    hit = mouseY / height_;
    hit += top_;
    if (hit >= count_)
        return -1;
    db_msg("hit %d", hit);
    GetClientRect (hwnd, &rcInv);

    hitItem = data_[hit];
    if (hitItem->dwFlags & LBIF_DISABLE) {
        return -1;
    }
    oldSel = SelectItem (hwnd, hit);
    if (dwStyle & LBS_CHECKBOX) 
    {
        if (mouseX > 0 && mouseX < CheckMarkHeight (height_)) 
        {
            click_mark = TRUE;
            if (message == MSG_LBUTTONUP && dwStyle & LBS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, LBN_CLICKCHECKMARK);

            if (dwStyle & LBS_AUTOCHECK) 
            {
                if (message == MSG_LBUTTONUP) 
                {
                    ItemData *plbi;
                    plbi = data_[hit];

                    switch (plbi->dwFlags & LBIF_CHECKMARKMASK) 
                    {
                        case LBIF_CHECKED:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            if (message == MSG_LBUTTONUP && dwStyle & LBS_NOTIFY)
                                NotifyParent(hwnd, pCtrl->id,LBN_SELCANCEL);
                            break;
                        default:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            plbi->dwFlags |= LBIF_CHECKED;
                            break;
                    }
                }
                InvalidateItem (hwnd,  hit);
            }
        }
    }

    if (oldSel >= 0) 
    {
        if (oldSel >= top_ && (oldSel <= top_ + visible_cnt_)) 
        {
            db_msg("oldSel======%d, top_==%d", oldSel, top_);
            GetSubmenuRect(&rcInvSubmenu, submenu_status);
            InvalidateRect (hwnd, &rcInvSubmenu, TRUE);
            GetItemsRect ( oldSel, oldSel, &rcInv);
            InvalidateRect (hwnd, &rcInv, TRUE);
        }
    }
    db_msg(" ");
    hilighted_idx_ = hit;
    last_selected_idx_ = hilighted_idx_;
    GetSubmenuRect(&rcInvSubmenu, submenu_status);
    InvalidateRect (hwnd, &rcInvSubmenu, TRUE);
    //GetItemsRect ( hilighted_idx_, hilighted_idx_, &rcInv);
    //InvalidateRect (hwnd, &rcInv, TRUE);
    GetItemsRect ( hit, hit, &rcInv);
    InvalidateRect (hwnd, &rcInv, TRUE);

    SetScrollbar (hwnd,  TRUE);

    if (!fling_flag_) {
        if (OnItemClick) 
        {
            OnItemClick(this);
        }
    }
    fling_flag_ = 0;

    if ( message == MSG_LBUTTONUP && (dwStyle & LBS_NOTIFY) ) 
    {
        db_msg("message:%d, dwStyle:%x", message, dwStyle);
        if (dwStyle & LBS_MOUSEFOLLOW) 
        {
            db_msg("dwStyle:...............");
            oldSel = last_selected_idx_;
            last_selected_idx_ = hit;
        }
        if (oldSel != hit)
            NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
        if (!click_mark)
            NotifyParent (hwnd, pCtrl->id, LBN_CLICKED);
    }
   
    SendMessage(GetParent(hwnd), MLM_NEW_SELECTED, oldSel, hit);
    return 0;
}

int MenuItems::second_MenuItem_Message(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    int oldSel, mouseX, mouseY, hit;
    RECT rcInv, rcInvSubmenu;
    BOOL click_mark = 0;
    ItemData *hitItem;

    PCONTROL    pCtrl = gui_Control(hwnd);
    DWORD       dwStyle = pCtrl->dwStyle;
    
    if (count_ == 0)
        return -1;

    mouseX = LOSWORD (lparam);
    mouseY = HISWORD (lparam);
    //db_msg("[ghy], height=%d, count_=%d, mouseY=%d, mouseX=%d, main_item=%d", height_, count_, mouseY, mouseX, this->main_item);
  
    //if(this->main_item == 4 || this->main_item == 5 || this->main_item == 13)
    if(this->main_item == 4)
    {
        if(mouseX < 320 || mouseX > 600 || mouseY < 250 || mouseY > 360)
            return -1;
        else
        {
            hit = 0;
        }
    }else
    {
        if(mouseX < S_F_ICON_BG_W || mouseY < S_ITEM_STRING_BG_OFFSET_TOP || 
                mouseY > (S_ITEM_STRING_BG_OFFSET_TOP + count_ * S_ITEM_STRING_BG_H))
            return -1;

        hit = (mouseY - S_ITEM_STRING_BG_OFFSET_TOP) / S_ITEM_STRING_BG_H;
    }
    if (hit >= count_)
        return -1;
    db_msg("hit %d", hit);
    GetClientRect (hwnd, &rcInv);

    hitItem = data_[hit];
    if (hitItem->dwFlags & LBIF_DISABLE) {
        return -1;
    }
    oldSel = SelectItem (hwnd, hit);
    if (dwStyle & LBS_CHECKBOX) 
    {
        if (mouseX > 0 && mouseX < CheckMarkHeight (height_)) 
        {
            click_mark = TRUE;
            if (message == MSG_LBUTTONUP && dwStyle & LBS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, LBN_CLICKCHECKMARK);

            if (dwStyle & LBS_AUTOCHECK) 
            {
                if (message == MSG_LBUTTONUP) 
                {
                    ItemData *plbi;
                    plbi = data_[hit];

                    switch (plbi->dwFlags & LBIF_CHECKMARKMASK) 
                    {
                        case LBIF_CHECKED:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            if (message == MSG_LBUTTONUP && dwStyle & LBS_NOTIFY)
                                NotifyParent(hwnd, pCtrl->id,LBN_SELCANCEL);
                            break;
                        default:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            plbi->dwFlags |= LBIF_CHECKED;
                            break;
                    }
                }
                InvalidateItem (hwnd,  hit);
            }
        }
    }

    if (oldSel >= 0) 
    {
        if (oldSel >= top_ && (oldSel <= top_ + visible_cnt_)) 
        {
            db_msg("oldSel======%d, top_==%d", oldSel, top_);
            GetSubmenuRect(&rcInvSubmenu, submenu_status);
            InvalidateRect (hwnd, &rcInvSubmenu, TRUE);
            GetItemsRect ( oldSel, oldSel, &rcInv);
            InvalidateRect (hwnd, &rcInv, TRUE);
        }
    }
    db_msg(" ");
    hilighted_idx_ = hit;
    last_selected_idx_ = hilighted_idx_;
    GetSubmenuRect(&rcInvSubmenu, submenu_status);
    InvalidateRect (hwnd, &rcInvSubmenu, TRUE);
    //GetItemsRect ( hilighted_idx_, hilighted_idx_, &rcInv);
    //InvalidateRect (hwnd, &rcInv, TRUE);
    GetItemsRect ( hit, hit, &rcInv);
    InvalidateRect (hwnd, &rcInv, TRUE);

    SetScrollbar (hwnd,  TRUE);

    if (!fling_flag_) {
        if (OnItemClick) 
        {
            OnItemClick(this);
        }
    }
    fling_flag_ = 0;

    if ( message == MSG_LBUTTONUP && (dwStyle & LBS_NOTIFY)) 
    {
        db_msg("message:%d, dwStyle:%x", message, dwStyle);
        if (dwStyle & LBS_MOUSEFOLLOW) 
        {
            oldSel = last_selected_idx_;
            last_selected_idx_ = hit;
        }
        if (oldSel != hit)
            NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
        if (!click_mark)
            NotifyParent (hwnd, pCtrl->id, LBN_CLICKED);
    }
   
    SendMessage(GetParent(hwnd), MLM_NEW_SELECTED, oldSel, hit);
    return 0;
}


int MenuItems::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    PCONTROL    pCtrl;
    DWORD           dwStyle;
    int count;
#if _USE_FIXSTR
    int             len;
#endif
    pCtrl   = gui_Control(hwnd);
    dwStyle = pCtrl->dwStyle;

    switch(message) {
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
                    FillBoxWithBitmap (hdc, 0, 0,
                            RECTW(rcTemp), RECTH(rcTemp), bg_image_);
                }

                if (fGetDC)
                    ReleaseDC (hdc);
                return 0;
            }
    case MSG_CREATE:
        {
            db_msg(" MSG_CREATE");
            //the color is formated argb
            // trial_data.normalBgc = 0xFF919191,  //the main background color
            //trial_data.normalFgc = 0xFFDCDCDC,
            trial_data.normalBgc = 0xFF070129,  //the main background color    no choose item background 
                trial_data.normalFgc = 0xFF000000,
                trial_data.linec = 0xFF290DF4;//0xFF363636,  //the line color
            trial_data.normalStringc = 0xFF5E6073,  //the item name   submenu no choose font color
                trial_data.normalValuec = 0xFFF5F5F5,   //current value
                //trial_data.selectedStringc = 0xFF060606,    //abgr  choose font color
                trial_data.selectedStringc = 0xFFE4EDFA,    //abgr  menu choose font color
                trial_data.selectedValuec = 0xFF060606,
                trial_data.scrollbarc = 0xFF59595A,
                trial_data.itemHeight = 26,
                trial_data.context = 0;
            trial_data.sub_menu_flag = -1;
            MenulistAttr* attr = &trial_data;

            //listbox_data_ = xmalloc(sizeof(LISTBOXDATA));

            /**** global attribute from MiniGUI.cfg *****/
            //gp_hilite_bgc   = GetWindowElementPixelEx(hwnd, HDC_SCREEN,
            // WE_BGC_HIGHLIGHT_ITEM);
            // gp_hilite_fgc   = GetWindowElementPixelEx(hwnd, HDC_SCREEN,
            //WE_FGC_HIGHLIGHT_ITEM);
            gp_hilite_fgc=0xFFFFFFFF;
            gp_hilite_bgc=0xFF290DF4;
            gp_sign_bgc     = GetWindowElementPixelEx(hwnd, HDC_SCREEN,
                    WE_BGC_SIGNIFICANT_ITEM);
            gp_sign_fgc     = GetWindowElementPixelEx(hwnd, HDC_SCREEN,
                    WE_FGC_SIGNIFICANT_ITEM);
            gp_normal_bgc = attr->normalBgc;
            gp_normal_fgc = attr->normalFgc;
            SetWindowElementAttr(hwnd, WE_BGC_WINDOW,
                    Pixel2DWORD(HDC_SCREEN, gp_normal_bgc));
            SetWindowElementAttr(hwnd, WE_FGC_WINDOW,
                    Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
            SetWindowBkColor(hwnd, gp_normal_bgc);
            gp_linec = attr->linec;
            gp_normal_stringc = attr->normalStringc;
            gp_normal_valuec = attr->normalValuec;
            gp_selected_stringc = attr->selectedStringc;
            gp_selected_valuec = attr->selectedValuec;
            gp_menu_stringc_color = 0xFFFFFFFF;//for first menu string color no change
            gp_submenu_head_color = 0xFF2772DB;
            /****** scoll bar use the mainc_3dbox element color,
              the element is RGBA format **********/
            SetWindowElementAttr(hwnd, WE_MAINC_THREED_BODY,
                    Pixel2DWORD(HDC_SCREEN, attr->scrollbarc) );

            /*********  set the scroll bar width ************/
            SetWindowElementAttr(hwnd, WE_METRICS_SCROLLBAR, 8);

            last_selected_idx_ = -1;
            hilighted_idx_= -1;
            count_ = 0;
#ifdef USESCROLLBAR //add by zhb hide the scrollbar
            if (GetWindowStyle (hwnd) & LBS_SBALWAYS)
                scrollbar_policy_ = SB_POLICY_ALWAYS;
            else
                scrollbar_policy_ = SB_POLICY_AUTOMATIC;
#else
            scrollbar_policy_ = SB_POLICY_NEVER;
#endif
            height_ = attr->itemHeight;

            SetScrollbar (hwnd, 0);
        //  (*GetWindowCallbackProc(hwnd))(hwnd, message, wparam, lparam);
//          SendMessage(hwnd, LB_MULTIADDITEM, 10, (LPARAM)menuListII);

            SendMessage(hwnd, LB_SETITEMHEIGHT, 0, ITEM_HIGHT);
            if(frist_bg_image_ == NULL)
                frist_bg_image_ = AllocImage("/usr/share/minigui/res/images/frist_sub_item_height.png");
            if(hilight_image_[0] == NULL)
                hilight_image_[0]= AllocImage("/usr/share/minigui/res/images/hilight.png");
            if(hilight_image_[1] == NULL)
                hilight_image_[1]= AllocImage("/usr/share/minigui/res/images/unhilight.png");
            if(hilight_image_[2] == NULL)
                hilight_image_[2]= AllocImage("/usr/share/minigui/res/images/hilight_update.png");
            if(hilight_image_[3] == NULL)
                hilight_image_[3]= AllocImage("/usr/share/minigui/res/images/unhilight_update.png");
            if(subhilight_image_[0] == NULL)
                subhilight_image_[0] = AllocImage("/usr/share/minigui/res/images/sub_hilight.png");
            if(subhilight_image_[1] == NULL)
                subhilight_image_[1] = AllocImage("/usr/share/minigui/res/images/set_subhilight1.png");
            if(subhilight_image_[2] == NULL)
                subhilight_image_[2] = AllocImage("/usr/share/minigui/res/images/set_subhilight2.png");
            if(subhilight_image_[3] == NULL)
                subhilight_image_[3] = AllocImage("/usr/share/minigui/res/images/set_subhilight3.png");
            for(int i = 0; i < ITEMNUMBER ; i++){
                if(submenu_first_image_w[i] == NULL)
                    submenu_first_image_w[i]= AllocImage(sub_first_icon_patch_[i].sub_first_icon_w);
                if(submenu_first_image_[i] == NULL)
                    submenu_first_image_[i]= AllocImage(sub_first_icon_patch_[i].sub_first_icon);
            }
            if(button_image_[0] == NULL)
                button_image_[0]=AllocImage("/usr/share/minigui/res/images/firstlevel_second.png");
            if(button_image_[1] == NULL)
                button_image_[1]=AllocImage("/usr/share/minigui/res/images/secondlevel_bg.png");
            if(button_image_[2] == NULL)
                button_image_[2]=AllocImage("/usr/share/minigui/res/images/firstlevel_update.png");
            if(button_image_[3] == NULL)
                button_image_[3]=AllocImage("/usr/share/minigui/res/images/firstlevel_binding.png");
        }
        break;
    case MSG_DESTROY:
        {
            db_msg(" MSG_DESTROY");
            FreedBeforeRemoveAll();
            for(unsigned int i=0; i<data_.size(); i++) {
                delete data_[i];
            }
        }
    break;
    case MSG_PAINT:
    {
        db_msg(" MSG_PAINT");
        HDC     hdc;
        RECT    rc;

        hdc = BeginPaint (hwnd);
        GetClientRect (hwnd, &rc);
        SelectFont(hdc, GetWindowFont(hwnd));
        s_width = RECTW (rc);
        s_height = RECTH (rc)-36;
        DrawItems(hwnd, hdc, F_ITEM_UNHILIGHT_W);
        // DrawFocusRect (hwnd, hdc);
#ifdef USELINE
        SetPenColor(hdc, gp_linec); /* set the line color */
        db_msg("count_:%d", count_);
        for( count = 1; count <= count_; count++ ) {
            Line2(hdc, 0, height_ * count, RECTW(rc), height_ * count);
        }
#endif
        xdump();
        EndPaint (hwnd, hdc);
        return 0;
    }
    case MSG_SIZECHANGED:
    {
        db_msg(" MSG_SIZECHANGED");
        int oldvisible, oldwidth;
        const RECT *rc = (const RECT*)lparam;

        oldvisible = visible_cnt_;
        oldwidth = width_;

        /*from mpeer data is abnormal*/
        if ( (rc->left >= rc->right) || (rc->top >= rc->bottom))
            break;

        CalcParams (rc);

        if (oldvisible !=  visible_cnt_ || oldwidth != width_ )
            SetScrollbar (hwnd,  0);
    }
    break;
    case LB_SETCURSEL:
    case LB_SETCARETINDEX:
        {
            db_msg(" LB_SETCARETINDEX");
            int new_index = (int)wparam;
            int old, newTop;
            if (new_index < 0 || new_index > count_ - 1) {
                if (dwStyle & LBS_MULTIPLESEL)
                    return LB_ERR;
                else
                    return CancleSelected (hwnd);
            }

            old = hilighted_idx_;
            if (new_index >= 0 && new_index != old) {
                if (count_ - new_index >= visible_cnt_)
                    newTop = new_index;
                else
                    newTop = MAX (count_ - visible_cnt_, 0);

                top_ = newTop;
                hilighted_idx_ = new_index;
                last_selected_idx_ = hilighted_idx_;
                SetScrollbar (hwnd,  TRUE);
            }
            if (!(dwStyle & LBS_MULTIPLESEL))
            {
                ItemData *newItem = data_[new_index];
                if (newItem->dwFlags & LBIF_DISABLE) {
                    CancleSelected(hwnd);
                    return old;
                }
                else
                    SelectItem (hwnd,  new_index);
            }
            InvalidateRect (hwnd, NULL, TRUE);
            db_msg(" LB_SETCARETINDEX  %d  %d",old,new_index);
            SendMessage(GetParent(hwnd), MLM_NEW_SELECTED, old, new_index);
            return old;
        }

    case LB_GETCOUNT:
        return count_;
    case LB_GETCURSEL:
        {
            db_msg(" LB_GETCURSEL");
            ItemData *plbi;
            unsigned int index = 0;

            if (dwStyle & LBS_MULTIPLESEL)
                return hilighted_idx_;
            for(index=0; index<data_.size(); index++) {
                plbi = data_[index];
                if (plbi->dwFlags & LBIF_SELECTED)
                    return index;
            }
            return LB_ERR;
        }
    case LB_GETITEMDATA:
        {
            db_msg(" LB_GETITEMDATA");
            ItemData *plbi;
            ItemData *item_data;
            unsigned int count;

            if (!(dwStyle & LBS_CHECKBOX || dwStyle & LBS_USEICON)) {
                return LB_ERR;
            }

            if (!(plbi = data_[(int)wparam]))
                return LB_ERR;

            item_data = (ItemData*)lparam;
            if (!item_data)
                return LB_ERR;
            //todo

            return LB_OKAY;
        }

    case LB_SETITEMDATA:
        {
            db_msg(" LB_SETITEMDATA");
            ItemData *plbi;
            ItemData *item_data;
            unsigned int count = 0;

            if (!(dwStyle & LBS_CHECKBOX || dwStyle & LBS_USEICON)) {
                return LB_ERR;
            }

            if (!(plbi = data_[(int)wparam]))
                return LB_ERR;

            item_data = (ItemData*)lparam;
            if (!item_data)
                return LB_ERR;

            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;

            plbi->item_string = item_data->item_string;

            if (item_data->type == TYPE_IMAGE) {
                plbi->result_image = item_data->result_image;
            } else if (item_data->type == TYPE_STRING) {
                plbi->result_string = item_data->result_string;
            }

            InvalidateItem (hwnd, (int)wparam);
            return LB_OKAY;
        }
    case LB_SETITEMHEIGHT:
        db_msg(" LB_SETITEMHEIGHT");
        if (height_ != LOWORD (lparam)) {
            RECT rcClient;

            height_ = LOWORD (lparam);
            if (height_ < pCtrl->pLogFont->size)
                height_ = pCtrl->pLogFont->size;

            GetClientRect (hwnd, &rcClient);
            CalcParams (&rcClient);

            SetScrollbar (hwnd,  TRUE);
            InvalidateRect (hwnd, NULL, TRUE);
        }
        return  height_;

    case MSG_SETFOCUS:
        {
            db_msg(" MSG_SETFOCUS");
            RECT rc;


            if (dwFlags_ & LBF_FOCUS)
                break;

            dwFlags_ |= LBF_FOCUS;

            if (hilighted_idx_ >= 0) {
                GetClientRect (hwnd, &rc);
                GetItemsRect ( hilighted_idx_, hilighted_idx_, &rc);
                InvalidateRect (hwnd, &rc, TRUE);
            }

            if (dwStyle & LBS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, LBN_SETFOCUS);
            break;
        }

    case MSG_KILLFOCUS:
        {
            db_msg(" MSG_KILLFOCUS");
            RECT rc;
            if (hilighted_idx_ >= 0) {
                GetClientRect (hwnd, &rc);
                GetItemsRect ( hilighted_idx_, hilighted_idx_, &rc);
                InvalidateRect (hwnd, &rc, TRUE);
            }

            dwFlags_ &= ~LBF_FOCUS;

            if (dwStyle & LBS_NOTIFY)
            {
                NotifyParent (hwnd, pCtrl->id, LBN_SELCANCEL);
                NotifyParent (hwnd, pCtrl->id, LBN_KILLFOCUS);
            }
            break;
        }
    case MSG_FONTCHANGED:
        {
            db_msg(" MSG_FONTCHANGED");
            RECT rcClient;

            if (height_ < pCtrl->pLogFont->size)
                height_ = pCtrl->pLogFont->size;

#if 0
            if (listbox_data_->lst_font != INV_LOGFONT) {
                DestroyLogFont(listbox_data_->lst_font);
                listbox_data_->lst_font = INV_LOGFONT;
            }
            listbox_data_->lst_font = CreateLogFontIndirect(GetWindowFont(hwnd));
#endif
//          updateWindowFont(hwnd);
            GetClientRect (hwnd, &rcClient);
            CalcParams (&rcClient);

            SetScrollbar (hwnd,  TRUE);
            InvalidateRect (hwnd, NULL, TRUE);
            return 0;
        }
        break;
    case MSG_LBUTTONDBLCLK:
        db_msg(" MSG_LBUTTONDBLCLK");
        if (dwStyle & LBS_NOTIFY)
            NotifyParent (hwnd, pCtrl->id, LBN_DBLCLK);
        break;

    case MSG_MOUSEMOVE:
        break;
   // case MSG_LBUTTONUP: {
   //     db_msg(" MSG_LBUTTONUP");
   //     //CancleSelected (hwnd);
   //     if ((dwStyle & LBS_MULTIPLESEL)
   //             || !(dwStyle & LBS_MOUSEFOLLOW))
   //         break;
   //     }
    case MSG_LBUTTONDOWN:
        {
            int ret = -1;
            db_msg(" MSG_LBUTTONDOWN");
            ItemData *c_item = data_[top_];
            db_msg("submenuflag_second=%d", c_item->submenuflag_second);
            if(c_item->submenuflag_second == 0)
            {
                ret = first_MenuItem_Message(hwnd, message, wparam, lparam);
                if(ret < 0)
                    return 0;
            }else
            {
               // ret = second_MenuItem_Message(hwnd, message, wparam, lparam);
              //  if(ret < 0)
                //    return 0;
            }
            
            return 0;
        }
        case MSG_KEYDOWN:
            db_msg("[ghy],MSG_KEYDOWN");
            return 0;
        case MSG_KEYUP:
        {
            db_msg(" MSG_KEYUP");
            /**** set the new select item, and which is the new top item *****/
            int oldSel = -1, newSel, newTop;
            RECT rcInv,rcInvSubmenu;
            ItemData *pnewItem;

            if (count_ == 0)
                break;

            /* newTop and newSel equal the current itemTop and itemHilightted */
            newTop = top_;
            newSel = hilighted_idx_;
            switch (LOWORD (wparam)) {
                case SDV_KEY_OK:
			InvalidateRect (hwnd, NULL, TRUE);//refresh all rect
			break;
#if 0
		case SDV_KEY_MODE:
			db_msg("[debug_zhb]---menu_items----SDV_KEY_MODE---offset_submenu = %d    offset_menu = %d",offset_submenu,offset_menu);
			if(!getSubmenuStatusFlag()){
				if(offset_menu!= OFFSETMAX ){
					offset_menu += OFFSETMAX/6;
					db_msg("[debug_zhb]---menu_items--111--SDV_KEY_MODE");
					InvalidateRect (hwnd, NULL, TRUE);//refresh all rect
				 }
				}else{
					if(offset_submenu!= OFFSETMAX ){
						offset_submenu += OFFSETMAX/6;
						db_msg("[debug_zhb]---menu_items--111--SDV_KEY_MODE");
						InvalidateRect (hwnd, NULL, TRUE);//refresh all rect
					 }
				}
			break;
#endif
		 case SDV_KEY_RIGHT:
		 {
			#if 0
			if(exit_flag == true)
			{
				exit_flag = false;
				db_warn("setting window is switch to the exit no need newSel++");
				break;
		    }
			newSel ++;
            if (newSel >= count_)
			{
                newSel = 0;
                newTop = 0; /* back to the first one */
                break;
            }
            if (newSel > ITEM_BOTTOM (this))
                newTop ++;
			#endif
                    break;
		 }
         case SDV_KEY_LEFT:
		 {
			#if 0
			newSel --;
            if (newSel < 0) {
                newSel = count_ - 1;
                if(count_ > visible_cnt_) {
                    newTop = count_ - visible_cnt_;
                } else {
                    newTop = 0;
                }
                break;
            }
            if (newSel < top_)
                newTop --;
			#endif
            break;
         }
         default:
             return 0;
            }
            GetClientRect (hwnd, &rcInv);
	     //db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInv.left,rcInv.top,rcInv.right-rcInv.left,rcInv.bottom-rcInv.top);
            if (hilighted_idx_ != newSel) {
                if (top_ != newTop) {
                    /***** item top changed ****/
			//db_msg("zhb--------------***** item top changed ****");
                    top_ = newTop;
                    hilighted_idx_ = newSel;
                    last_selected_idx_ = hilighted_idx_;
                    /* if not use the MULTIPLESEL */
                    if (!(dwStyle & LBS_MULTIPLESEL)) {
                        pnewItem = data_[newSel];
                        if(pnewItem->dwFlags & LBIF_DISABLE)
                            CancleSelected(hwnd);
                        else {
                            oldSel = SelectItem (hwnd,  newSel);
                            if ((dwStyle & LBS_NOTIFY) && (oldSel != newSel))
                                NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
                        }
                    }
			//db_msg("zhb--------111------***** item top changed ****");
                    InvalidateRect (hwnd, NULL, TRUE);
                }
                else {
                    /******* item top not changed ********/
			//db_msg("zhb--------------***** item top not changed ****");
                    if (!(dwStyle & LBS_MULTIPLESEL)) {
                        pnewItem = data_[newSel];
                        if(pnewItem->dwFlags&LBIF_DISABLE) {
                            CancleSelected(hwnd);
				//db_msg("zhb------1111--------***** item top not changed ****");
                            InvalidateRect(hwnd, NULL, TRUE);
                        }
                        else
                        {
                            oldSel = SelectItem (hwnd,  newSel);
                            if ((dwStyle & LBS_NOTIFY) && (oldSel != newSel))
                                NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
                            if (oldSel >= 0) {
                                if (oldSel >= top_ && oldSel <=
                                    (ITEM_BOTTOM (this) + 1)) {
                                    GetSubmenuRect(&rcInvSubmenu,submenu_status);
					//db_msg("zhb-----2222222222222222---------***** item top not changed ****");
					//db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInvSubmenu.left,rcInvSubmenu.top,rcInvSubmenu.right-rcInvSubmenu.left,rcInvSubmenu.bottom-rcInvSubmenu.top);
					InvalidateRect (hwnd, &rcInvSubmenu, TRUE);//刷新右边的submenu rect
                                    GetItemsRect ( oldSel, oldSel, &rcInv);
					//db_msg("zhb-----2222---------***** item top not changed ****");
					//db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInv.left,rcInv.top,rcInv.right-rcInv.left,rcInv.bottom-rcInv.top);
                                    InvalidateRect (hwnd, &rcInv, TRUE);
					
                                }
                            }
                        }
                        if (newSel < newTop) {
                            hilighted_idx_ = newSel;
                            last_selected_idx_ = hilighted_idx_;
                            break;
                        }

                        hilighted_idx_ = newSel;
                        last_selected_idx_ = hilighted_idx_;
			   GetSubmenuRect(&rcInvSubmenu,submenu_status);
			//db_msg("zhb-----333333333333---------***** item top not changed ****");
			//db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInvSubmenu.left,rcInvSubmenu.top,rcInvSubmenu.right-rcInvSubmenu.left,rcInvSubmenu.bottom-rcInvSubmenu.top);
			InvalidateRect (hwnd, &rcInvSubmenu, TRUE);//刷新右边的submenu rect
                        GetItemsRect ( newSel, newSel, &rcInv);
			//db_msg("zhb-----3333---------***** item top not changed ****");
			//db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInv.left,rcInv.top,rcInv.right-rcInv.left,rcInv.bottom-rcInv.top);
                        InvalidateRect (hwnd, &rcInv, TRUE);
			  
                    }
                    else {
                        pnewItem = data_[newSel];
                        if(pnewItem->dwFlags&LBIF_DISABLE)
                            break;
                        GetItemsRect ( hilighted_idx_, hilighted_idx_, &rcInv);
			   //db_msg("zhb-----4444---------***** item top not changed ****");
			   //db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInv.left,rcInv.top,rcInv.right-rcInv.left,rcInv.bottom-rcInv.top);
                        InvalidateRect (hwnd, &rcInv, TRUE);
                        hilighted_idx_ = newSel;
                        GetItemsRect ( newSel, newSel, &rcInv);
			   //db_msg("zhb------5555--------***** item top not changed ****");
			   //db_msg("zhb---------- x = %d  y = %d  w = %d  h= %d",rcInv.left,rcInv.top,rcInv.right-rcInv.left,rcInv.bottom-rcInv.top);
                        InvalidateRect (hwnd, &rcInv, TRUE);
                    }
                }
                SetScrollbar (hwnd,  TRUE);
            }
            SendMessage(GetParent(hwnd), MLM_NEW_SELECTED, oldSel, newSel);
        }
        break;
    case MSG_VSCROLL:
        {
            db_msg(" MSG_VSCROLL");
            int newTop;
            int scrollHeight = 0;

            newTop = top_;      /* get the top item's index */
            db_msg("[ghy], MSG_VSCROLL wparam:%d", wparam);
            switch(wparam)
            {
            case SB_LINEDOWN:
                if (ITEM_BOTTOM (this) < (count_ - 1)) {
                    newTop ++;
                    scrollHeight = -height_;
                }
                break;

            case SB_LINEUP:
                if (top_ > 0) {
                    newTop --;
                    scrollHeight = height_;
                }
                break;

            case SB_PAGEDOWN:
                if ((top_ + (visible_cnt_ << 1)) <=
                        count_)
                    newTop += visible_cnt_;
                else
                    newTop = count_ - visible_cnt_;

                if (newTop < 0)
                    return 0;

                scrollHeight = -(newTop - top_)
                    *height_;
                break;

            case SB_PAGEUP:
                if (top_ >= visible_cnt_)
                    newTop -= visible_cnt_;
                else
                    newTop = 0;

                scrollHeight = (top_ - newTop)*height_;
                break;

            case SB_THUMBTRACK:
                newTop = MIN ((int)lparam/height_,
                        (count_ - visible_cnt_));
                scrollHeight = (top_ - newTop)*height_;
                break;

            default:
                break;
            }
            fling_flag_ = true;
            if (scrollHeight) {
                top_ = newTop;
                SetScrollbar (hwnd,  TRUE);
                InvalidateRect (hwnd, NULL, TRUE);
                return 0;
            }
        }
        break;
    case LB_SETITEMDISABLE:
        {
            db_msg(" LB_SETITEMDISABLE");
            ItemData *item;
            int index = (int)wparam;

            if (count_ <= 0 || index < 0 || index > count_)
                return LB_ERR;

            item = data_[index];
            if (lparam)
                item->dwFlags |= LBIF_DISABLE;
            else
                item->dwFlags &=~LBIF_DISABLE;

            return InvalidateUnderItem (hwnd,  index);
        }

    case LB_GETITEMDISABLE:
        {
            db_msg(" LB_GETITEMDISABLE");
            ItemData *item;
            int index = (int)wparam;

            if (count_ <= 0 || index < 0 || index > count_)
                return LB_ERR;
            item = data_[index];
            return item->dwFlags&LBIF_DISABLE;
        }
        break;
    case LB_MULTIADDITEM:
        {
            db_msg(" LB_MULTIADDITEM");
            ItemData *item_data = (ItemData*)lparam; /* menu list item info */
            ItemData *data;
            int item_width;
            int start, pos = -1;
            int added_count = (int)wparam;
	    //db_msg("[debug_zhb]----------index_flag = %d ,  item_data->submenuflag = %d",index_flag, item_data->submenuflag);
            if(index_flag != item_data->submenuflag)
            {
                index_flag = item_data->submenuflag;
		  //FreedBeforeRemoveAll();
                if (data_.size() > 0) {
                    count_ = 0;
                    db_warn("[debug_zhb]: data_.size:%d  capacity:%d",data_.size(),data_.capacity());
                    data_.clear();
		     
                }
            }
	   // db_msg("[debug_zhb]----------count_ = %d ",count_);
            start = count_;
	if(added_count < INSERTITEM){
	            for (int i = 0; i < added_count; i++)
	            {
	                data = new ItemData;
	                data->first_icon[UNHILIGHTED_ICON] = AllocImage((char*)
	                    (item_data->first_icon_path[UNHILIGHTED_ICON].c_str()));
	                data->first_icon[HILIGHTED_ICON]   = AllocImage((char*)
	                    (item_data->first_icon_path[HILIGHTED_ICON].c_str()));
			  data->second_icon[UNHILIGHTED_ICON] = AllocImage((char*)
	                    (item_data->second_icon_path[UNHILIGHTED_ICON].c_str()));
	                data->second_icon[HILIGHTED_ICON]   = AllocImage((char*)
	                    (item_data->second_icon_path[HILIGHTED_ICON].c_str()));
			for(int k = 0; k < THIRD_ICON_NUM; k++){
				data->third_icon[k]   = AllocImage((char*)
	                    (item_data->third_icon_path[k].c_str()));
				}
	                data->item_string        = item_data->item_string;
			  data->subitem_string        = item_data->subitem_string;
	                data->result_string      = item_data->result_string;
			  data->menuitem_string= item_data->menuitem_string;
	                data->type               = item_data->type;
	                data->value              = item_data->value;//menu hilight
	                //db_msg("zhb-------------data->value= %d",data->value);
	                data->sub_hilight = item_data->sub_hilight;//submenu hilight
	                data->result_cnt         = item_data->result_cnt;
			 data->subMenuStringType = item_data->subMenuStringType;
			 data->subMenuStringvalue = item_data->subMenuStringvalue;
			 data->off_string = item_data->off_string;
			 data->on_string = item_data->on_string;
			 data->button_string1 = item_data->button_string1;
			 data->button_string2 = item_data->button_string2;
			 data->time_string = item_data->time_string;
			 data->item_tips= item_data->item_tips;
			 data->fupdate= item_data->fupdate;
	#if 0
			 for(StringIntMap::iterator it = data->subMenuStringType.begin();it!=  data->subMenuStringType.end();it++){
				if(it!=data->subMenuStringType.end()){
					db_msg("debug_zhb----------menu_item---subMenuStringType----- it->c_str = %s  it->second = %d",it->first.c_str(),it->second);
				}
			}
			for(StringIntMap::iterator it = data->subMenuStringvalue.begin();it!=  data->subMenuStringvalue.end();it++){
				if(it!=data->subMenuStringvalue.end()){
					db_msg("debug_zhb----------menu_item--subMenuStringvalue------ it->c_str = %s  it->second = %d",it->first.c_str(),it->second);
				}
			}
	#endif
			 data->subitemcnt = item_data->subitemcnt;
			 data->submenuflag = item_data->submenuflag;
			 //db_msg("zhb-----count_ = %d--------data->subitemcnt= %d  data->submenuflag = %d",count_,data->subitemcnt,data->submenuflag);
			if(item_data->submenuflag_second){
					if( count_== item_data->sub_hilight){
					data->dwFlags			=  LBIF_SELECTED;
					 hilighted_idx_ = item_data->sub_hilight;////////////////////////////////////////////***************
					}else {
					data->dwFlags			 = LBIF_NORMAL;
					}
				}else{
					   if( count_== item_data->value ){
			                      data->dwFlags            =  LBIF_SELECTED;
						hilighted_idx_ = item_data->value;////////////////////////////////////////////***************
					   }else {
					      data->dwFlags            = LBIF_NORMAL;
					   }
				}
			  data->submenuflag_second = item_data->submenuflag_second;
	                data_.push_back(data);
	                count_++;
	                item_width = lstGetItemWidth (hwnd, data, height_);
	                //db_msg("menuitem height:%d", height_);
			 //db_msg("[debug_zhb]menuitem item_width:%d   max_width_ = %d ", item_width,max_width_);
	                if (max_width_ < item_width)
	                    max_width_ = item_width;
	            }
		}else if(added_count > INSERTITEM  && added_count < INSERTITEM+100){ 
				//db_msg("[debug_zhb]-------------insert item data");
				data = new ItemData;
				changeItemData(data, item_data);
				data_.insert(data_.begin()+added_count-INSERTITEM,data);
				count_++;
			}else if(added_count > DELETEITEM && added_count < DELETEITEM+100){
					//db_msg("[debug_zhb]-------------delete item data");
					data_.erase(data_.begin()+added_count-DELETEITEM);
					count_--;
				}
	     SetTop();
            InvalidateUnderMultiItem (hwnd, start, pos);
            SetScrollbar (hwnd,  TRUE);
		//db_msg("debug_zhb---> data->subitemcnt = %d   (int)data_.size() = %d",data->subitemcnt,(int)data_.size());
	    if(data->value >= 0 && data->value<=6){
	    	 if((data->subitemcnt== (int)data_.size()) && (data->submenuflag_second == 1))//为了解决在一级菜单界面下快速按ok 按键，导致多次加载子菜单数据
		 		m_add_data_finish = true;
	    }else{
		   if((data->subitemcnt +1 == (int)data_.size()) && (data->submenuflag_second == 1))
		   		m_add_data_finish = true;
	    	}
            return pos;
        }
        break;
    default:
        return CustomWidget::HandleMessage( hwnd, message, wparam, lparam );
        break;
    }
    return HELP_ME_OUT;
}
void MenuItems::SetQuitFlag(bool flag)
{
	//db_msg("zhb------SetQuitFlag----flag = %d",flag);
	exit_flag = flag;
}
void MenuItems::add(ItemData &data)
{
    db_msg(" ");

    SendMessage(GetHandle(), LB_MULTIADDITEM, 1, (LPARAM)&data);
}

void MenuItems::addOrDelete(ItemData &data,int dex)
{
    db_msg(" ");

    SendMessage(GetHandle(), LB_MULTIADDITEM, dex, (LPARAM)&data);
}

void MenuItems::remove(unsigned int pos)
{
    int start, end = 0;
    start = count_;

    if (pos > data_.size()) {
        db_error("pos: %d no item found", pos);
        return;
    }
    data_.erase(data_.begin() + pos); count_--;
    InvalidateUnderMultiItem (handle_, start, end);
    SetScrollbar (handle_,  TRUE);
}
void MenuItems::FreedBeforeRemoveAll()
{
	for(unsigned int k = 0 ; k < data_.size() ; k++)
	{
		delete(data_[k]);
		data_[k] = NULL;	
	}
	db_warn("debug_zhb--->FreedBeforeRemoveAll");
	count_ = 0;
	data_.clear();
}

void MenuItems::removeAll()
{
	db_warn("debug_zhb--->removeAll");
    count_ = 0;
    if( data_.size())
        data_.clear();
}
int MenuItems::GetHilight()
{
    db_msg(" ");

    return hilighted_idx_;
}


void MenuItems::SetHilight(int value)
{
    //db_msg("[debug_zhb]:-----SetHilight---------hilighted_idx_ =: %d \n",value);
     hilighted_idx_=value;
     SendMessage(GetHandle(), LB_SETCURSEL, value, 0);
}
int MenuItems::GetCount()
{
    //db_debug("[fangjj]:-----GetCount---------count_ =: %d \n",count_);
    return count_;
}

int MenuItems::SetTop()
{
	//db_msg("debug_zhb----SetTop---visible_cnt_ = %d  hilighted_idx_ = %d   count_ = %d ",visible_cnt_,hilighted_idx_,count_);

      if( (visible_cnt_<= hilighted_idx_) && (visible_cnt_ <= count_))
  	{
	   top_ = hilighted_idx_ - visible_cnt_ +1;
	  // db_msg("debug_zhb----SetTop---top_ = %d",top_);
  	}
	  else
  	{
  	  top_ = 0;
  	}
  //  db_debug("[fangjj]:-----SetTop---------top_ =: %d \n",top_);
    return 0;
}


ItemData *MenuItems::GetItemData(int item_index)
{
    //normally, item_index is same with hilighted_idx_.
    db_msg("menu item data_ maybe changed by other caller!!!");
    ItemData *result = NULL;
    if ((unsigned int)item_index < data_.size()) {
        result = data_[item_index];
    } else {
        db_warn("current item index[%d] invalid! maybe tutk setting config?   data_.size() = %d", item_index,data_.size());
    }

    return result;
}
void MenuItems::SetSubHilightValue(int value)
{
    //db_msg("[debug_zhb]----SetSubHilightValue----value = %d",value);
    for (unsigned int k = 0 ;k < data_.size() ; k++) {
       data_[k]->sub_hilight = value;
    }
}
void MenuItems::SetLanuageStr(const std::string & str)
{
     for (unsigned int k = 0 ;k < data_.size() ; k++) {
       data_[k]->subitem_string= str;
    }
}

void MenuItems::SetTimeString(const std::string& time_string)
{
    for (unsigned int k = 0 ;k < data_.size() ; k++) {
       data_[k]->time_string= time_string;
    }
}

void MenuItems::SetUpdateVersionFlag(int val,const std::string str,const std::string str2,int type)
{
   if(!getSubmenuStatusFlag()){
	    for (unsigned int k = 0 ;k < data_.size() ; k++) {
		if(k == data_.size()-1){
	       		data_[k]->fupdate= val;
			data_[k]->button_string2 = str.c_str();
			data_[k]->button_string1 = str2.c_str();
			data_[k]->type = type;
			}
	    }
   }
}

ItemData::ItemData()
    : result_image(NULL)
    , type(TYPE_STRING)
    , value(0)
    , result_cnt(0)
    , submenuflag(0)
    , submenuflag_second(0)

{
    first_icon_path[UNHILIGHTED_ICON] = "";
    first_icon_path[HILIGHTED_ICON] = "";
    first_icon[UNHILIGHTED_ICON] = NULL;
    first_icon[HILIGHTED_ICON] = NULL;
    second_icon_path[UNHILIGHTED_ICON] = "";
    second_icon_path[HILIGHTED_ICON] = "";
    second_icon[UNHILIGHTED_ICON] = NULL;
    second_icon[HILIGHTED_ICON] = NULL;
    for(int i = 0 ; i < 14 ; i++){
		third_icon_path[i] = "";
   		third_icon[i] = NULL;
    	}
	item_string.clear();
	subitem_string.clear();
	result_string.clear();
	menuitem_string.clear();
	result_image = NULL;
	subMenuStringType.clear();
	subMenuStringvalue.clear();
	on_string.clear();
	off_string.clear();
	button_string1.clear();
	button_string2.clear();
	time_string.clear();
	item_tips.clear();
}


ItemData::~ItemData()
{	
    for (int i = UNHILIGHTED_ICON; i <= HILIGHTED_ICON; i++) {
        if (first_icon[i] != NULL) {
            UnloadBitmap(first_icon[i]);
	    	first_icon[i] = NULL;
        }
	 if (second_icon[i] != NULL) {
            UnloadBitmap(second_icon[i]);
            second_icon[i] = NULL;
        }
    }

    for(int j = 0 ; j < 14 ; j++){
		if (third_icon[j] != NULL) {
	            UnloadBitmap(third_icon[j]);
	            third_icon[j] = NULL;
	        }
    	}
	if(result_image != NULL)
	{
		UnloadBitmap(result_image);
		result_image = NULL;
	}
}
