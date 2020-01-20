#include "listbox_view.h"
#define NDEBUG

IMPLEMENT_DYNCRT_CLASS(ListBoxView)
    
ListBoxView::ListBoxView(View *parent)
    :CustomWidget(parent)
    ,list(NULL)
{
    list = (ListboxView*) malloc(sizeof(ListboxView));
	memset((void *) list, 0, sizeof(ListboxView));
    list->itDt = (ItemDataBox*) malloc(sizeof(ItemDataBox)*list->itSz);
	memset((void *) list->itDt, 0, sizeof(ItemDataBox)*list->itSz);

}

ListBoxView::~ListBoxView()
{
    if(list->winBg != NULL)
    {
        FreeListboxViewImage(list->winBg);
        list->winBg = NULL;
    }
    if(list->ltbBg != NULL)
    {
        FreeListboxViewImage(list->ltbBg);
        list->ltbBg = NULL;
    }
    for(int i=1; i<list->itSz; i++)
    {
        
        if(list->itDt[i].lB != NULL)
        {
           FreeListboxViewImage(list->itDt[i].lB);
           list->itDt[i].lB= NULL;
        }
        if(list->itDt[i].rB != NULL)
        {
           FreeListboxViewImage(list->itDt[i].rB);
           list->itDt[i].rB= NULL;
        }

       if(list->itDt[i].btn->onBmp != NULL)
        {
           FreeListboxViewImage(list->itDt[i].btn->onBmp);
           list->itDt[i].btn->onBmp= NULL;
        } 
       if(list->itDt[i].btn->offBmp != NULL)
        {
           FreeListboxViewImage(list->itDt[i].btn->offBmp);
           list->itDt[i].btn->offBmp= NULL;
        } 
        
    }
}
void ListBoxView::GetCreateParams(CommonCreateParams &params)
{
     db_warn("[habo]---> ListBoxView  GetCreateParams \n");
    params.class_name = CTRL_LISTBOX_VIEW;
    params.alias      = GetClassName();
    params.style      = WS_NONE;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = 320;//DEFAULT_CTRL_WIDTH;
    params.h          = 240;//DEFAULT_CTRL_HEIGHT;
}


int ListBoxView::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
	int i;
	int newX = 0;
	int newY = 0;
	int newItem;
	int selectIndex = -1;	// -1为无效值
	
	//list = (ListboxView*)GetWindowAdditionalData(hwnd);
    if(list == NULL)
    {
        db_warn("list is null ");
        return 0;
    }
	switch (message) 
	{
	case MSG_CREATE:
	{
		return 0;
	}
	case MSG_PAINT: 
	{
        //db_msg("[habo]--->ListboxViewProc MSG_PAINT \n");
		int res;
		int a, b;
		SIZE size;
		HDC hdc = BeginPaint(hwnd);

		// 根据gap计算index总数
		a = list->moveGap / (list->iy+ list->vgap);
		b = list->moveGap % (list->iy+ list->vgap);
		list->moveGap = b;
		list->Index = list->Index - a;
		if(list->Index <= 0)
		{
			list->Index = 0;
			if(list->moveGap>=0)
			{
				list->moveGap = 0;
			}
		}
		else if(list->Index >= list->itSz-list->showSz)
		{
			list->Index = list->itSz-list->showSz;
			if(list->moveGap<0)
			{
				list->moveGap = 0;
			}
		}
		//printf("oldGap=%d, Index=%d, showSz=%d\n", list->moveGap, list->Index, list->showSz);
		SetBkMode(hdc, BM_TRANSPARENT);
		// 如果index大于0 要多画一个item
		if(list->Index > 0)
		{
			newItem=-1;
		}
		else
		{
			newItem=0;
		}
		// 根据gap绘制item里面的内容
		for(; newItem<=list->showSz; newItem++)
		{
			// 不能越界
			if((newItem + list->Index) >= list->itSz)
			{
				break;
			}
			
			SetTextColor(hdc, list->lTextColor);
			//SelectFont(hdc, list->lTextFt);
			
			// item 风格
			if((list->itType & ITEM_BMP) && (list->itType & ITEM_LINE))
			{
				FillBoxWithBitmap(hdc, list->hfp, (list->iy+ list->vgap) * newItem + list->moveGap, list->ix, list->iy, list->ltbBg);

				SetPenColor(hdc, list->lineColor);
				MoveTo(hdc, list->hfp, (list->iy + list->vgap) * newItem - (list->vgap/2) + list->moveGap);
				LineTo(hdc, list->hfp + list->ix, (list->iy + list->vgap) * newItem - (list->vgap/2) + list->moveGap);
            }
			else if(list->itType & ITEM_BMP)
			{
				FillBoxWithBitmap(hdc, list->hfp, (list->iy+ list->vgap) * newItem + list->moveGap, list->ix, list->iy, list->ltbBg);
			}
			else if(list->itType & ITEM_LINE)
			{
				SetPenColor(hdc, list->lineColor);
				MoveTo(hdc, list->hfp, (list->iy + list->vgap) * newItem - (list->vgap/2) + list->moveGap);
				LineTo(hdc, list->hfp + list->ix, (list->iy + list->vgap) * newItem - (list->vgap/2) + list->moveGap);
			}
			
			// item左边内容
			 //db_msg("[habo]--->ListboxViewProc MSG_PAINT 333333333  list->itDt[%d].lT = %s\n",list->Index+newItem,list->itDt[list->Index+newItem].lT.c_str());
			GetTextExtent(hdc, list->itDt[list->Index+newItem].lT.c_str(), -1, &size);
			TextOut(hdc, list->hfp + list->lthGap, 
				(list->iy-size.cy) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, list->itDt[list->Index+newItem].lT.c_str());

			// item右边内容
			if(list->itDt[list->Index+newItem].rType & RITEM_TEXT)
			{
          //db_msg("[habo]--->ListboxViewProc MSG_PAINT 333333333  list->itDt[%d].rT = %s\n",list->Index+newItem,list->itDt[list->Index+newItem].rT.c_str());
				GetTextExtent(hdc, list->itDt[list->Index+newItem].rT.c_str(), -1, &size);
				TextOut(hdc, list->hfp + list->ix - list->lthGap - size.cx, 
					(list->iy-size.cy) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, list->itDt[list->Index+newItem].rT.c_str());

            }
			else if(list->itDt[list->Index+newItem].rType & RITEM_BMP)
			{
				FillBoxWithBitmap(hdc, list->hfp + list->ix - list->lthGap - list->itDt[list->Index+newItem].rB->bmWidth, 
					(list->iy-list->itDt[list->Index+newItem].rB->bmHeight) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, 
					list->itDt[list->Index+newItem].rB->bmWidth, 
					list->itDt[list->Index+newItem].rB->bmHeight, list->itDt[list->Index+newItem].rB);
			}
			else if(list->itDt[list->Index+newItem].rType & RITEM_BUTTON)
			{
				if(list->itDt[list->Index+newItem].btn->onStus)
				{
					FillBoxWithBitmap(hdc, list->hfp + list->ix - list->lthGap - list->itDt[list->Index+newItem].btn->onBmp->bmWidth, 
					(list->iy-list->itDt[list->Index+newItem].btn->onBmp->bmHeight) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, 
					list->itDt[list->Index+newItem].btn->onBmp->bmWidth, 
					list->itDt[list->Index+newItem].btn->onBmp->bmHeight, list->itDt[list->Index+newItem].btn->onBmp);
				}
				else
				{
					FillBoxWithBitmap(hdc, list->hfp + list->ix - list->lthGap - list->itDt[list->Index+newItem].btn->offBmp->bmWidth, 
					(list->iy-list->itDt[list->Index+newItem].btn->offBmp->bmHeight) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, 
					list->itDt[list->Index+newItem].btn->offBmp->bmWidth, 
					list->itDt[list->Index+newItem].btn->offBmp->bmHeight, list->itDt[list->Index+newItem].btn->offBmp);
				}

				if(list->itDt[list->Index+newItem].btn->sltItem >= 0)
				{
					//SelectFont(hdc, list->rTextFt);
					//SetTextColor(hdc, PIXEL_lightgray);
					SetTextColor(hdc, list->rTextColor);
					GetTextExtent(hdc, list->itDt[list->Index+newItem].btn->text.c_str(), -1, &size);
					//size.cy = 24;
					TextOut(hdc, list->hfp + list->ix - list->lthGap - size.cx - list->itDt[list->Index+newItem].btn->onBmp->bmWidth, 
						(list->iy-size.cy) / 2 + (list->iy+ list->vgap) * newItem + list->moveGap, 
						list->itDt[list->Index+newItem].btn->text.c_str());
				}
			}
		}
		EndPaint(hwnd, hdc);

		return 0;
	}

	case MSG_LBUTTONDOWN: 
	{
		newX = LOWORD(lparam);
		newY = HIWORD(lparam);
		//printf("newX=%d, newY=%d\n", newX, newY);
		list->isSlidText = TRUE;
		list->oldY = newY;
		list->moveNum = 0;

		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}

	case MSG_LBUTTONUP:
	{
        //printf("[habo]--->MSG_LBUTTONUP \n");
		newX = LOWORD(lparam);
		newY = HIWORD(lparam);
		//printf("newX=%d, newY=%d\n", newX, newY);
		list->isSlidText = FALSE;

		// 边界处理
		if(list->Index <= 0)
		{
			list->moveGap = 0;
		}
		if(list->Index >= list->itSz-list->showSz)
		{
			list->moveGap = 0;
		}

		// 计算点击item
		if(list->Index > 0)
		{
			newItem=-1;
		}
		else
		{
			newItem=0;
		}
		for(; newItem<=list->showSz; newItem++)
		{
			if(newY > ((list->iy+ list->vgap) * newItem + list->moveGap) 
				&& newY < ((list->iy+ list->vgap) * newItem + list->moveGap + list->iy))
			{
				selectIndex = list->Index + newItem;
			}
		}

		// 如果窗口移动过，不产生按钮事件
		if(list->moveFlag == 1)
		{
			list->moveFlag = 0;
			selectIndex = -1;
		}
		//printf("index=%d, selectIndex=%d\n", list->Index, selectIndex);

		if(selectIndex >= 0)
		{
			if(list->itDt[selectIndex].rType & RITEM_BUTTON)
			{
				if(list->itDt[selectIndex].btn->onStus)
				{
					list->itDt[selectIndex].btn->onStus = 0;
				}
				else
				{
					list->itDt[selectIndex].btn->onStus = 1;
				}
				list->itDt[selectIndex].btn->fun(selectIndex, (DWORD)list); 

				selectIndex = -1;
			}
		}

		InvalidateRect(hwnd, NULL, TRUE);
	
		//SetTimer(hwnd, LISTBOX_TIMER_ID, 1);
		break;
	}

	case MSG_MOUSEMOVE: 
	{
		newX = LOWORD(lparam);
		newY = HIWORD(lparam);
		//db_msg("[habo]--->MSG_MOUSEMOVE \n");
		// 计算滑动gap
		if (list->isSlidText) {
			list->moveNum++;
			if (list->moveNum >= 2) {
				list->moveNum = 0;
				//printf("newX=%d, newY=%d\n", newX, newY);
				int gap = newY - list->oldY;
				int newGap = list->moveGap + gap;
				list->moveFlag = 1;
				
				if ((list->Index <= 0 && gap > 0) || (list->Index >= list->itSz-list->showSz && gap < 0))
				{
					break;
				}

				list->oldY = newY;
				list->moveGap = newGap;
				InvalidateRect(hwnd, NULL, TRUE);
				//printf("moveinfo: (%d, %d, %d, %d, %d, %d)\n", newX, newY, list->oldY, gap, list->moveGap, newGap);
			}
		}
		break;
	}
    case LB_LISTBOXVIEW_ADD_DATA:
    {
        db_msg("----------------start------------------> LB_LISTBOXVIEW_ADD_DATA");
        ListboxView *listboxview_data = (ListboxView*)(lparam);
        list = (ListboxView*) malloc(sizeof(ListboxView));
        if(list == NULL)
        {
            db_error("[habo]---> list is null ");
            return 0;
        }
	    memset((void *) list, 0, sizeof(ListboxView));
        list->lineColor = listboxview_data->lineColor;
        list->itType = listboxview_data->itType;

        list->lTextColor = listboxview_data->lTextColor;
        list->rTextColor = listboxview_data->rTextColor;
        list->ht = listboxview_data->ht;
        list->ix = listboxview_data->ix;
        list->hfp = listboxview_data->hfp; 
        list->hbp = listboxview_data->hbp;  
        list->vt = listboxview_data->vt;          
        list->iy = listboxview_data->iy;           
        list->vfp = listboxview_data->vfp;
        list->vbp = listboxview_data->vbp;
        list->vgap = listboxview_data->vgap;
        list->itSz = listboxview_data->itSz;
        list->Index = listboxview_data->Index;
        list->showSz = listboxview_data->showSz;
        list->lthGap = listboxview_data->lthGap;
        list->ltvGap = listboxview_data->ltvGap;
        list->moveGap = listboxview_data->moveGap;
        if(!(listboxview_data->winBg_path[0].empty()))
        {
            list->winBg = AllocListboxViewImage((char*)(listboxview_data->winBg_path[0].c_str()));
            list->winBg_path[0] = listboxview_data->winBg_path[0];
        }
        if(!(listboxview_data->ltbBg_path[0].empty()))
        {
            list->ltbBg = AllocListboxViewImage((char*)(listboxview_data->ltbBg_path[0].c_str()));
            list->ltbBg_path[0] = listboxview_data->ltbBg_path[0];
        }


        list->itDt = (ItemDataBox*) malloc(sizeof(ItemDataBox)*list->itSz);
        if(list->itDt == NULL)
        {
            db_error("[habo]---> list->itDt is null ");
            //this need to free the img
            return 0;
        }
	    memset((void *) list->itDt, 0, sizeof(ItemDataBox)*list->itSz);
        
        for(int i=0; i<listboxview_data->itSz; i++)
        {
            list->itDt[i].rType = listboxview_data->itDt[i].rType;
            list->itDt[i].lT = listboxview_data->itDt[i].lT;
            list->itDt[i].rT = listboxview_data->itDt[i].rT;
            
            if(!(listboxview_data->itDt[i].lrB_path[0].empty()))
            {
                list->itDt[i].lB = AllocListboxViewImage((char*)(listboxview_data->itDt[i].lrB_path[0].c_str()));
                list->itDt[i].lrB_path[0] = listboxview_data->itDt[i].lrB_path[0];
            }
            if(!(listboxview_data->itDt[i].lrB_path[1].empty()))
            {
                list->itDt[i].rB = AllocListboxViewImage((char*)(listboxview_data->itDt[i].lrB_path[1].c_str()));
                list->itDt[i].lrB_path[1] = listboxview_data->itDt[i].lrB_path[1];
            }
            
            if(listboxview_data->itDt[i].btn != NULL)
            {
                if(!(listboxview_data->itDt[i].btn->onOffB_path[0].empty()))
                {
                    list->itDt[i].btn->onBmp = AllocListboxViewImage((char*)(listboxview_data->itDt[i].btn->onOffB_path[0].c_str()));
                    list->itDt[i].btn->onOffB_path[0] = listboxview_data->itDt[i].btn->onOffB_path[0];
                }
                if(!(listboxview_data->itDt[i].btn->onOffB_path[1].empty()))
                {
                    list->itDt[i].btn->offBmp = AllocListboxViewImage((char*)(listboxview_data->itDt[i].btn->onOffB_path[1].c_str()));
                    list->itDt[i].btn->onOffB_path[1] = listboxview_data->itDt[i].btn->onOffB_path[1];
                }
                list->itDt[i].btn->onStus = listboxview_data->itDt[i].btn->onStus ;
                list->itDt[i].btn->sltItem = listboxview_data->itDt[i].btn->sltItem ;
                list->itDt[i].btn->text = listboxview_data->itDt[i].btn->text ;
            }
            
        }
         db_msg("-----------------------end-----------> LB_LISTBOXVIEW_ADD_DATA");
    }
    break;
	default:
		break;
	}

	return DefaultControlProc(hwnd, message, wparam, lparam);
}

void ListBoxView::add(ListboxView &data)
{
    db_msg("[habo]----> test add data ");

    SendMessage(GetHandle(), LB_LISTBOXVIEW_ADD_DATA, 1, (LPARAM)&data);
}


BITMAP* ListBoxView::AllocListboxViewImage(const char *image_path)
{
    int ret = 0;

    if (image_path == NULL)
		return NULL;

	if( !strncmp(image_path, "", 1) )
		return NULL;

    BITMAP *data = (BITMAP*)malloc(sizeof(BITMAP));
    db_warn("zhb---------------image_path=%s",image_path);
    ret = LoadBitmapFromFile(HDC_SCREEN, data, image_path);
    if (ret != 0)
	{
        free(data);
		data = NULL;
    }

    return data;
}

void ListBoxView::FreeListboxViewImage(BITMAP *data)
{
    UnloadBitmap(data);
}


#if 0
BOOL RegisterListboxView(void) 
{
	WNDCLASS MyClass;
	MyClass.spClassName = LISTBOX_VIEW;
	MyClass.dwStyle = WS_NONE;
	MyClass.dwExStyle = WS_EX_NONE;
	MyClass.hCursor = 0;
	MyClass.iBkColor = PIXEL_lightgray;
	MyClass.WinProc = ListboxViewProc;

	return RegisterWindowClass(&MyClass);
}

void UnregisterListboxView(void)
{
	UnregisterWindowClass(LISTBOX_VIEW);
}
#endif

