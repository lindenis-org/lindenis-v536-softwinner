#ifndef LISTBOX_VIEW_H_
#define LISTBOX_VIEW_H_

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include "widgets.h"
#include "type/types.h"
#include <vector>

#include <string.h>
#include <unistd.h>

#define LIST_MAX_SIZE 15
#define NAME_MAX_SIZE 64

// item 右边属性
#define RITEM_NONE 		0x0
#define RITEM_TEXT 		0x01
#define RITEM_BMP 		0x02
#define RITEM_BUTTON 	0x04
#define RITEM_TEXT_BMP  0x05
#define RITEM_TEXT_BUTTON  0x06


// item 的属性
#define ITEM_NONE 		0x0
#define ITEM_LINE 		0x01
#define ITEM_BMP 		0x02
#define LB_LISTBOXVIEW_ADD_DATA         0xF1F0

typedef void (*pButtonFun)(int index, DWORD param);

typedef struct {
	BITMAP *onBmp;
	BITMAP *offBmp;
    std::string onOffB_path[2];//add by habo 0-->onBmp   1--> offBmp
	int onStus;
	int sltItem;
	std::string text;
	pButtonFun fun;
}ButtonData;

typedef struct {
	int rType;
	std::string lT;
	std::string rT;
	BITMAP *rB;
    BITMAP *lB;//add by habo
    std::string lrB_path[2];//add by habo  0-->lB   1--> rB
	ButtonData *btn;
	int l_icon_text_gap;
    int r_icon_text_gap;
}ItemDataBox;

typedef struct {
	HWND sfHwnd;
	HWND parentHwnd;
	PLOGFONT lTextFt;
	gal_pixel lTextColor;
	PLOGFONT rTextFt;
	gal_pixel rTextColor;
	BITMAP *winBg;
    std::string winBg_path[1];//add by habo

	int ht;
	int hfp;
	int ix;
	int hbp;
		
	int vt;
	int iy;
	int vfp;
	int vbp;
	int vgap;

	int itType;
	gal_pixel lineColor;
	BITMAP *ltbBg;
    std::string ltbBg_path[1];//add by habo
	ItemDataBox *itDt;
	int itSz;
	int Index;
	int showSz;
	int lthGap;
	int ltvGap;

	// 辅佐成员，可以不用复制
	int moveGap;
	BOOL isSlidText;
	int oldY; 
	int moveNum;
	int moveFlag;
}ListboxView;


class ListBoxView : public CustomWidget
{
    DECLARE_DYNCRT_CLASS(ListBoxView, Runtime)
public:
	ListBoxView(View *parent);
	virtual ~ListBoxView();
	virtual void GetCreateParams(CommonCreateParams &params);
	virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);
    NotifyEvent OnListBoxViewClick;
    void add(ListboxView &data);
    BITMAP* AllocListboxViewImage(const char *image_path);
    void FreeListboxViewImage(BITMAP *data);
private:
       ListboxView *list;
};

#endif

