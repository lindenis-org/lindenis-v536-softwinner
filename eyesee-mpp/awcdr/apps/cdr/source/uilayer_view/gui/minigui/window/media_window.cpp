/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: media_window.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "window/media_window.h"
#include "debug/app_log.h"
#include "widgets/text_view.h"
#include "widgets/card_view.h"
#include "widgets/graphic_view.h"
#include "window/user_msg.h"
#include "window/window_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "MediaWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(MediaWindow)

MediaWindow::MediaWindow(IComponent *parent)
    : SystemWindow(parent)
{
    db_msg(" ");
    Load();
    //SetPosition( 100, 0, 300, 200 );
    db_msg(" ");
    GraphicView* view = reinterpret_cast<GraphicView *>
        (GetControl("gv_return"));
    view->SetTag(WINDOWID_LAUNCHER);
    view->OnClick.bind(this, &MediaWindow::ViewClickProc);

    vector<CardViewItem> item_vector;
    CardViewItem item;
    item.icon_path[0] = "/usr/share/minigui/res/images/null.png";
    item.icon_path[1] = "/usr/share/minigui/res/images/null.png";
    item.item_string  = "item1";
    item_vector.push_back(item);
    item.icon_path[0] = "/usr/share/minigui/res/images/null.png";
    item.icon_path[1] = "/usr/share/minigui/res/images/null.png";
    item.item_string  = "item2";
    item_vector.push_back(item);
    item.icon_path[0] = "/usr/share/minigui/res/images/null.png";
    item.icon_path[1] = "/usr/share/minigui/res/images/null.png";
    item.item_string  = "item3";
    item_vector.push_back(item);
    item.icon_path[0] = "/usr/share/minigui/res/images/null.png";
    item.icon_path[1] = "/usr/share/minigui/res/images/null.png";
    item.item_string  = "item4";
    item_vector.push_back(item);

    CardView* card_view = reinterpret_cast<CardView *>
        (GetControl("tv_media"));
    card_view->init(item_vector);
    card_view->onChanged.bind(this, &MediaWindow::onChange);
}


void MediaWindow::onChange(View *control)
{
    CardView *card_view = (CardView*)control;
    db_msg("current_card %d, current index %d",card_view->GetCurrentCardIndex(),
        card_view->GetCurrentObjIndex());
    int current_index = card_view->GetCurrentCardIndex();
    vector<ImageInfo> image_vector;
    ImageInfo image("");
    switch(current_index) {
        case 0:
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            break;
        case 1:
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            break;
        case 2:
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            break;
        case 3:
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            image.path = "/usr/share/minigui/res/images/null.png";
            image_vector.push_back(image);
            break;
        break;
        default:
            break;
    }
    card_view->SetData(card_view->GetCurrentCardIndex(), image_vector);
}

MediaWindow::~MediaWindow()
{

}

void MediaWindow::ViewClickProc(View *control)
{
    db_msg("%d clicked!", control->GetTag());
    listener_->notify(this, WM_WINDOW_CHANGE, control->GetTag());
}


string MediaWindow::GetResourceName()
{
    return string(GetClassName());
}


