/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: card_view.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "widgets/card_view.h"
#include "debug/app_log.h"
#include "debug/check.h"
#include "widgets/ctrlclass.h"
#include "memory/x_memory.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "CardView"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(CardView)

CardView::CardView(View *parent)
    : CustomWidget(parent)
{
    start_id_ = 0;//PLAYBACKPREVIEW_LABEL_CARD0;
    card_count_ = 0;
    current_card_index_ = 0;
    current_object_index_ = 0;
    object_x_count_ = 0;
    object_y_count_ = 0;
    last_card_index_ = -1;
    need_update_ = 0;
    newdata_flag_ = 0;
}

CardView::~CardView()
{

}

void CardView::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_CARD_IMAGEVIEW;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE | WS_CHILD  | WS_VSCROLL;
    params.exstyle    = WS_EX_USEPARENTFONT;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}

/*****************************************************************************
 Function: CardView::AllocImage
 Description: load the image from the image_path.
    xmemory records the memory information.
 Parameter:
 Return:
*****************************************************************************/
BITMAP* CardView::AllocImage(char *image_path)
{
    int ret = 0;
    BITMAP *data = (BITMAP*)xmalloc(sizeof(BITMAP));

    ret = LoadBitmapFromFile(HDC_SCREEN, data, image_path);
    if (ret != 0) {
        xfree(data);
    }
    return data;
}

void CardView::FreeImage(BITMAP *data)
{
    UnloadBitmap(data);
    xfree(data);
}

/*****************************************************************************
 Function: CardView::CreateCardView
 Description: cardview is composed of card1 and objects.
    following chart may help to illustrate.

  |---------------------------------------|
  | card1   |          |          |       |
  |_________|   obj1   |   obj2   |   ... |
  |         |          |          |       |
  | card2   |------------------------------
  |_________|   obj3   |   objn   |   ... |
  |         |          |          |       |
  | cardn   |          |          |       |
  |---------------------------------------|

 Parameter: -
 Return: -
*****************************************************************************/
void CardView::CreateCardView()
{
    db_msg(" ");
    HWND hwnd = GetHandle();
    const WINDOWINFO* info = GetWindowInfo(hwnd);
    int window_w = info->right  - info->left;
    int window_h = info->bottom - info->top;

    int card_w = window_w / 6;
    int card_h = window_h / card_count_;
    int x = 0;
    int y = 0;
    int object_w = (window_w - card_w) / object_x_count_;
    int object_h = window_h / object_y_count_;
    char debug_name[32];
    int id_index = start_id_;


    /* create [card_count_] cards */
    for(int i = 0; i<card_count_; i++) {
        snprintf(debug_name, sizeof(debug_name),"%s %d", items_[i]->item_string.c_str(), i);
        CreateWindowEx(CTRL_STATIC, debug_name,
        WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_BITMAP | SS_CENTERIMAGE
        | SS_CENTER | SS_VCENTER | SS_ICON,
        WS_EX_USEPARENTFONT,
        id_index++,
        x, y, card_w, card_h,
        hwnd, (DWORD)(items_[i]->icon[UNFOCUS_ICON]));
        y += card_h;
        SetWindowBkColor(hwnd, RGBA2Pixel (HDC_SCREEN, 0xff,0x0,0x0,0xff));
    }
    y = 0;
    x = card_w;

    /* create [object_x_count_ X object_y_count_] objects */
    for (int i = 0; i<object_x_count_; i++) {
        for (int j=0; j<object_y_count_; j++) {
            snprintf(debug_name, sizeof(debug_name),"%s %d %d", "obj", i, j);
            HWND hwnd_ex = CreateWindowEx(CTRL_STATIC, debug_name,
            WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_BITMAP | SS_CENTERIMAGE
            | SS_CENTER | SS_VCENTER | SS_ICON,
            WS_EX_USEPARENTFONT,
            id_index++,
            x, y, object_w, object_h,
            hwnd, (DWORD)0);
            SetWindowBkColor(hwnd_ex, RGBA2Pixel(HDC_SCREEN,
                0x10*i+j+0x50, 0x20*j+i+0x50, 0x14*i+j*i, 0xff));
            y += object_h;
        }
        x += object_w;
        y = 0;
    }
}


/*****************************************************************************
 Function: CardView::DisplayAllImage
 Description: display images from object0 to object n.
    (n = object_x_count_ X object_y_count_ - 1)
 Parameter: -
 Return: -
*****************************************************************************/
void CardView::DisplayAllImage()
{
    HWND hwnd = GetHandle();
    int unit = object_x_count_ * object_y_count_;
    int predict_index = current_object_index_ + unit;
    int object_start_id = start_id_ + card_count_;
    int ret;
    db_msg(" ");
    int j = 0;
    vector<ImageInfo *> image_vector= items_[current_card_index_]->image_vector;
    ImageInfo *data;
    for (unsigned int i=0; i<image_vector.size(); i++) {
        data = image_vector[i];
        NULL_BREAK(data);
        if (!data->bmp_data) {
            data->bmp_data = AllocImage((char*)data->path.c_str());
        }
        SendMessage(GetDlgItem(hwnd, object_start_id + i), STM_SETIMAGE,
            (WPARAM)data->bmp_data, 0);
    }
}

extern PCONTROL gui_Control (HWND hwnd);
int CardView::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam)
{
    PCONTROL pCtrl = gui_Control(hwnd);
    switch(message) {
        case MSG_COMMAND: {
            int id   = LOWORD(wparam);  //use the id the identify static
            int code = HIWORD(wparam);  //code means pressed down or up
            if (id >= start_id_ + card_count_) {
                break;
            }
            db_msg(" ");
            if (STN_CLICKED == code) {  //keydown
//              ChangeClickedIcon();
                SetWindowBkColor(GetDlgItem(hwnd,
                    start_id_ + current_card_index_), 0xffffffff);
                newdata_flag_ = 1;
                last_card_index_ = current_card_index_;
                current_card_index_ = id - start_id_;
                current_object_index_ = 0;
                db_msg("current_card_index_ %d last_index %d",
                        current_card_index_, last_card_index_);
                db_msg(" ");
                SendMessage(GetDlgItem(hwnd, start_id_ + current_card_index_),
                    STM_SETIMAGE,
                    (WPARAM)items_[current_card_index_]->icon[FOCUS_ICON], 0 );
                if (last_card_index_ != current_card_index_) {
                    SendMessage(GetDlgItem(hwnd, start_id_ + last_card_index_),
                    STM_SETIMAGE,
                    (WPARAM)items_[last_card_index_]->icon[UNFOCUS_ICON], 0 );
                }
                if (onChanged) {
                    onChanged(this);
                }
                //InvalidateRect (hwnd, NULL, 0);
            // FIXME: need minigui lib to support this event report
            // } else if (STN_HILITE == code){ //keyup
                // db_msg(" ");
            }
            db_msg("command id %d code %d", id, code);
        }
        break;
//      case MSG_VSCROLL: {
//          db_msg("get msg_vscroll");
//
//          switch(wparam)
//          {
//              case SB_LINEUP: {
//                  db_msg(" ");
//                  int unit_count = object_x_count_ * object_y_count_;
//                  db_msg(" %d %d %d", unit_count, current_object_index_ ,
//                      data_->object_data_count);
//                  if (current_object_index_ - unit_count < 0) {
//                      break;
//                  }
//                  current_object_index_ -= unit_count;
//                  need_update_ = 1;
//                  InvalidateRect (hwnd, NULL, TRUE);
//              }
//              break;
//              case SB_LINEDOWN: {
//
//                  int unit_count = object_x_count_ * object_y_count_;
//                  db_msg(" %d %d %d", unit_count, current_object_index_ ,
//                      data_->object_data_count);
//                  if (current_object_index_ + unit_count >= data_->object_data_count){
//                      break;
//                  }
//                  current_object_index_ += unit_count;
//                  need_update_ = 1;
//                  InvalidateRect (hwnd, NULL, TRUE);
//              }
//              break;
//          }
//      }
//      break;
        default:
            return CustomWidget::HandleMessage(hwnd, message, wparam, lparam);
        break;
    }
    return HELP_ME_OUT;
}

void CardView::init(vector<CardViewItem> &item_vector)
{
    card_count_ = item_vector.size();
    object_x_count_ = 2;
    object_y_count_ = 2;
    CardViewItem *item;
    for(unsigned int i = 0; i<item_vector.size(); i++) {
        item = new CardViewItem(&item_vector[i]);
        db_msg(" ");
        if (!item->icon_path[UNFOCUS_ICON].empty()) {
            item->icon[UNFOCUS_ICON] =
                AllocImage((char*)item->icon_path[UNFOCUS_ICON].c_str());
        }
        db_msg(" ");
        if (!item->icon_path[FOCUS_ICON].empty()) {
            item->icon[FOCUS_ICON] =
                AllocImage((char*)item->icon_path[FOCUS_ICON].c_str());
        }
        db_msg(" ");
        items_.push_back(item);
    }
    CreateCardView();
}

int CardView::GetCurrentCardIndex()
{
    return current_card_index_;
}

int CardView::GetCurrentObjIndex()
{
    return current_object_index_;
}

void CardView::ReleaseImages(vector<ImageInfo *> &image_vector)
{
    for (unsigned int i=0; i<image_vector.size(); i++) {
        if (image_vector[i]->bmp_data) {
            FreeImage(image_vector[i]->bmp_data);
        }
        delete image_vector[i];
    }
}

void CardView::SetData(int card_index, vector<ImageInfo> &image_vector)
{
    vector<ImageInfo *> last_image_vector = items_[card_index]->image_vector;
    items_[card_index]->image_vector.clear();

    ImageInfo *info;
    for (unsigned int i=0; i<image_vector.size(); i++) {
        info = new ImageInfo(image_vector[i].path, image_vector[i].text);
        items_[card_index]->image_vector.push_back(info);
    }
    DisplayAllImage();
    xdump();
    ReleaseImages(last_image_vector);
    xdump();
}

CardViewItem::CardViewItem()
{
    icon_path[UNFOCUS_ICON] = "";
    icon_path[FOCUS_ICON]   = "";
    icon[UNFOCUS_ICON]      = NULL;
    icon[FOCUS_ICON]        = NULL;
    image_vector.clear();
}

CardViewItem::CardViewItem(const CardViewItem *data)
{
    icon_path[UNFOCUS_ICON] = data->icon_path[UNFOCUS_ICON];
    icon_path[FOCUS_ICON]   = data->icon_path[FOCUS_ICON];
    icon[UNFOCUS_ICON]      = data->icon[UNFOCUS_ICON];
    icon[FOCUS_ICON]        = data->icon[FOCUS_ICON];
    item_string             = data->item_string;
    ImageInfo *info = NULL;
    for(unsigned int i = 0; i<data->image_vector.size(); i++) {
        info = new ImageInfo;
        memcpy(info, image_vector[i], sizeof(ImageInfo));
        image_vector.push_back(info);
    }
}


CardViewItem::~CardViewItem()
{
    for (int i = UNFOCUS_ICON; i <= FOCUS_ICON; i++) {
        if (icon[i]) {
            UnloadBitmap(icon[i]);
            icon[i] = NULL;
        }
    }
    for(unsigned int i = 0; i<image_vector.size(); i++) {
        if (image_vector[i]) {
            delete image_vector[i];
        }
    }
    image_vector.clear();
}



