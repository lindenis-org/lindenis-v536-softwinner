/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: card_view.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _CARD_VIEW_H_
#define _CARD_VIEW_H_

#include "widgets/widgets.h"
#include "type/types.h"

struct ImageInfo;

typedef struct ImageInfo{
    ImageInfo(){bmp_data=0;}
    ImageInfo(const char* file):path(file){bmp_data=0;}
    ImageInfo(std::string path_data, std::string text_data){
        path = path_data;
        text = text_data;
        bmp_data=0;
    }
    std::string path;    //the image path
    std::string text;    //the text for display
    BITMAP* bmp_data;
}ImageInfo;

enum {
    UNFOCUS_ICON = 0,
    FOCUS_ICON,
};

class CardViewItem
{
public:
    CardViewItem();
    ~CardViewItem();
    CardViewItem(const CardViewItem *data);
    std::string icon_path[2];
    BITMAP *icon[2];
    std::string item_string;
    /* the storage of all the images which need to be shown */
    std::vector<ImageInfo *> image_vector;
};


class CardView : public CustomWidget
{
    DECLARE_DYNCRT_CLASS(CardView, Runtime)
public:
    CardView(View *parent);
    virtual ~CardView();
    virtual void GetCreateParams(CommonCreateParams &params);
    virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                    LPARAM lparam);
    void CreateCardView();
    BITMAP* AllocImage(char *image_path);
    void FreeImage(BITMAP *data);

    /* initialize the cardview */
    void init(std::vector<CardViewItem> &item_vector);

    /* notify the content holder that images should be replaced */
    NotifyEvent onChanged;

    int GetCurrentCardIndex();
    int GetCurrentObjIndex();

    /* update the objects' content */
    void SetData(int card_index, std::vector<ImageInfo> &image_vector);

    /* update all the images */
    void DisplayAllImage();

    /* release the image resources */
    void ReleaseImages(std::vector<ImageInfo *> &image_vector);
protected:
    std::vector<CardViewItem*> items_;   //record the cardview's all content
private:
    int current_object_index_;
    int start_id_;  //start_id + card_count = object_id
    int card_count_;
    int object_x_count_;
    int object_y_count_;
    unsigned char current_card_index_;
    unsigned char last_card_index_;
    bool need_update_;
    int newdata_flag_;
};

#endif //_CARD_VIEW_H_
