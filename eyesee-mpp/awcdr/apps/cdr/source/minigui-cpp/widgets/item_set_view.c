#include "item_set_view.h"
//#include "resource.h"
//#include "common.h"

static int ItemSetDialogProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	int i;
	int newX = 0;
	int newY = 0;
	SIZE size;
	int selectIndex = -1;	// -1为无效值
	ItemSetInfo *info = NULL;

	switch(message)
	{
	case MSG_INITDIALOG:
	{
		ItemSetInfo *itemInfo;
		itemInfo = (ItemSetInfo*)lParam;
		if(!itemInfo) 
		{
			printf("invalid tipLabelData\n");
			return -1;
		}
		SetWindowAdditionalData(hDlg, (DWORD)itemInfo);
		//SetWindowBkColor(hDlg, PIXEL_lightwhite);
		
		break;
	}

	case MSG_PAINT:
	{
		HDC hdc;

		hdc = BeginPaint(hDlg);
		info = (ItemSetInfo*)GetWindowAdditionalData(hDlg);
		SetBkMode(hdc, BM_TRANSPARENT);
		//SetWindowBkColor(hDlg, PIXEL_lightwhite);

		// 绘制head
		SelectFont(hdc, info->hdFont);
		SetTextColor(hdc, info->hTextColor);
		GetTextExtent(hdc, info->hdText, -1, &size);
		TextOut(hdc, info->iGap, (info->hth-size.cy) / 2, info->hdText);

		for(i=0; i<info->iSz; i++)
		{
			// 绘制线条
			//SetPenColor(hdc, PIXEL_lightgray);
			SetPenColor(hdc, info->lineColor);
			MoveTo(hdc, info->iGap, info->hth + info->iy * i);
			LineTo(hdc, info->w - info->iGap, info->hth + info->iy*i); 

			// 绘制item
			SelectFont(hdc, info->itemFont);
			SetTextColor(hdc, info->iTextColor);
			GetTextExtent(hdc, info->item[i], -1, &size);
			TextOut(hdc, info->iGap, info->hth + info->iy * i + (info->iy-size.cy) / 2, info->item[i]);
		}

		// 绘制bmp
		if(info->itemBmp.bmBits){
			FillBoxWithBitmap(hdc, info->w-info->iGap-info->itemBmp.bmWidth, 
				info->hth + info->iy * info->iIndex + (info->iy-info->itemBmp.bmHeight) / 2, 
				info->itemBmp.bmWidth, info->itemBmp.bmHeight, &info->itemBmp);
		}
		EndPaint(hDlg, hdc);
		break;
	}

	case MSG_LBUTTONDOWN: 
	{
		newX = LOWORD(lParam);
		newY = HIWORD(lParam);
		info = (ItemSetInfo*)GetWindowAdditionalData(hDlg);
		
		info->isSlidText = TRUE;
		info->moveNum = 0;
		break;
	}

	case MSG_MOUSEMOVE: 
	{
		newX = LOWORD(lParam);
		newY = HIWORD(lParam);
		info = (ItemSetInfo*)GetWindowAdditionalData(hDlg);

		if (info->isSlidText) {
			info->moveNum++;
			if (info->moveNum >= 2) {
				info->moveNum = 0;
			}
		}
		break;
	}

	case MSG_LBUTTONUP:
	{
		newX = LOWORD(lParam);
		newY = HIWORD(lParam);
		info = (ItemSetInfo*)GetWindowAdditionalData(hDlg);
		info->isSlidText = FALSE;

		// 计算点击item
		
		for(i=0; i<info->iSz; i++)
		{
			if((newY > info->hth + info->iy * i) && (newY < (info->hth + info->iy * (i+1))))
			{
				selectIndex = i;
			}
			
		}
		printf("selectIndex=%d\n", selectIndex);

		if(selectIndex>=0)
		{
			InvalidateRect(hDlg, NULL, TRUE);
			info->iIndex = selectIndex;
			selectIndex = -1;

			
			EndDialog(hDlg, info->iIndex);
		}
		break;
	}

	default:
	{
		break;
	}
	
	}

	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int ShowItemSetDialog(HWND hParent, ItemSetInfo *info)
{
	DLGTEMPLATE dlg;
	dlg.dwStyle = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
	dlg.dwExStyle = WS_EX_TRANSPARENT;
	dlg.x = info->x;
	dlg.y = info->y;
	dlg.w = info->w;
	dlg.h =	info->h;
	dlg.caption = "";
	dlg.hIcon = 0;
	dlg.hMenu = 0;
	dlg.controlnr = 0;
	dlg.controls = NULL;
	dlg.dwAddData = 0;
	
	return DialogBoxIndirectParam(&dlg, hParent, ItemSetDialogProc, (LPARAM)info);
}


