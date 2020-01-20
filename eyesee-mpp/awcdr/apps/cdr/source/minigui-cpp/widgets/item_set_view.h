#ifndef _ITEM_SET_VIEW_H_
#define _ITEM_SET_VIEW_H_

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#define MAX_ITEM_SIZE 10
#define ITEM_MAX_NAME 64

typedef struct
{
	int x;
	int y;
	int w;
	int h;

	char hdText[ITEM_MAX_NAME];
	int hth;
	PLOGFONT hdFont;
	gal_pixel hTextColor;
	
	PLOGFONT itemFont;
	gal_pixel iTextColor;
	gal_pixel lineColor;
	BITMAP itemBmp;
	char item[MAX_ITEM_SIZE][ITEM_MAX_NAME];
	int iy;
	int iSz;
	int iIndex;
	int iGap;

	BOOL isSlidText;
	int moveNum;
}ItemSetInfo;

int ShowItemSetDialog(HWND hParent, ItemSetInfo *info);

#endif

