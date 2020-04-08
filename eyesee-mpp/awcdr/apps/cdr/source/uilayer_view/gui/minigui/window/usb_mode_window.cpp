/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         usb_mode_window.cpp
 Version:             1.0
 Author:              KPA362
 Created:            2017/4/10
 Description:       usb window show &  usb function select
 * *******************************************************************************/

#include "window/usb_mode_window.h"
#include "widgets/text_view.h"
#include "widgets/graphic_view.h"
#include "widgets/view_container.h"
#include "resource/resource_manager.h"
#include "window/window_manager.h"
#include "window/user_msg.h"
#include "window/dialog.h"
#include "debug/app_log.h"
#include "bll_presenter/audioCtrl.h"
#include "widgets/list_view.h"
#include "common/app_def.h"
#include "common/buttonPos.h"


buttonPos_ UsbListItemPos[2]=
{
{0,90,640,90+135},
{0,90+135,640,90+135+135},
};


#undef LOG_TAG
#define LOG_TAG "USBModeWindow"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(USBModeWindow)




void USBModeWindow::keyProc(int keyCode,int isLongPress)
{
    switch(keyCode)
    {
        case SDV_KEY_LEFT:
			
			DoUsbKeyHanderSelectionDialog();
            break;

        case SDV_KEY_OK:
			if(cur_usb_select == 0){
				ListItemClickProc(1);
			}else{
				ListItemClickProc(0);
			}
            break;

        case SDV_KEY_RIGHT:
			DoUsbKeyHanderSelectionDialog();
            break;

        case SDV_KEY_POWER:
            break;
        case SDV_KEY_MENU:
            break;
        default:
            break;
    }
}


void USBModeWindow::ListItemClickProc()
{
    LVITEM item;
    int ret = list_view_->GetSelectedItem(item);
    if(ret < 0){
        db_warn("GetSelectedItem failed, ret[%d]", ret);
        return;
    }
    switch(item.nItem)
    {
        case USB_CHARGE_MODE:
        {
            db_warn("habo---> USB_CHARGE_MODE");
            title_->Hide();
            this->Hide();
            GetControl("mass_mode_icon")->Hide();
            listener_->sendmsg(this, USB_CHARGING, 1);
            m_current_status = USB_NORMAL_STATUS;
            

        }break;
        case USB_MASS_STORAGE_MODE:
        {
            db_warn("habo---> USB_MASS_STORAGE_MODE");
            title_->Hide();
            GraphicView::LoadImage(GetControl("mass_mode_icon"), "mass_storage");
            GetControl("mass_mode_icon")->Show();
            listener_->sendmsg(this, USB_MASS_STORAGE, 1);
            m_current_status = USB_MASS_STORAGE_STATUS;
        }break;
        case USB_WEBCAM_MODE:
        {
            db_warn("habo---> USB_WEBCAM_MODE");
            title_->Hide();
            GraphicView::LoadImage(GetControl("mass_mode_icon"), "webcam");
            GetControl("mass_mode_icon")->Show();
            listener_->sendmsg(this, USB_UVC, 1);
            m_current_status = USB_UVC_STATUS;
        }break;
        default:
            break;

    }
}

void USBModeWindow::ListItemClickProc(int nItem)
{
    switch(nItem)
    {
        case USB_CHARGE_MODE:
        {
            db_warn("habo---> USB_CHARGE_MODE");
            Window *win = static_cast<Window *>(parent_);
            listener_ = WindowManager::GetInstance();
            listener_->sendmsg(win, USB_CHARGING, 1);
            m_current_status = USB_NORMAL_STATUS;
            this->DoHide();
        }break;
        case USB_MASS_STORAGE_MODE:
        {
            db_warn("habo---> USB_MASS_STORAGE_MODE");
            title_->Hide();
            list_view_->Hide();
            GraphicView::LoadImage(GetControl("mass_mode_icon"), "mass_storage");
            GetControl("mass_mode_icon")->Show();
            Window *win = static_cast<Window *>(parent_);
            listener_ = WindowManager::GetInstance();
            listener_->sendmsg(win, USB_MASS_STORAGE, 1);
            m_current_status = USB_MASS_STORAGE_STATUS;
            ignore_message_flag_ = true;
        }break;
        default:
            break;

    }
}

void USBModeWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

int USBModeWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
     switch (message) {
        case MSG_COMMAND:
            {
                int id = LOSWORD(wparam);
                int code = HISWORD(wparam);
                db_warn("habo---> code = %d",code);
                if (code == LVN_CLICKED) {
                  //  ListItemClickProc();
                }
            }
            break;
        case MSG_START_RESUME:
            {
                Hide();
            }
            break;
        case MSG_LBUTTONDOWN:
            db_warn("habo--> usb mode window MSG_LBUTTONDOWN  x = %d  y = %d ",LOSWORD(lparam),HISWORD(lparam));
            db_error("ignore_message_flag_ is %d",ignore_message_flag_);
            if(!ignore_message_flag_){
                ListItemClickProc(getTouchPosID(LOSWORD(lparam),HISWORD(lparam),UsbListItemPos,2));
            }else {
                db_error("current status %d is USB_MASS_STORAGE_STATUS,ignore message");
            }
			break;
        default:
            
            break;
    }
    
     return SystemWindow::HandleMessage( hwnd, message, wparam, lparam );
}




USBModeWindow::USBModeWindow(IComponent *parent)
    : SystemWindow(parent)
    ,m_current_status(USB_NORMAL_STATUS)
    ,title_(NULL)
    ,title_str("")
    ,charge_mode_str("")
    ,mass_storage_str("")
    ,webcam_mode_str("")
    ,list_view_(NULL)
    ,ignore_message_flag_(false)
    ,ItemHight(135)
    ,cur_usb_select(-1)
{
     wname = "USBModeWindow";
    Load();
    
    string bkgnd_bmp = R::get()->GetImagePath("black_bg");
    SetWindowBackImage(bkgnd_bmp.c_str());
    InitUSBWin();
}



USBModeWindow::~USBModeWindow()
{

}

string USBModeWindow::GetResourceName()
{
    return string(GetClassName());
}

void USBModeWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{

}

void USBModeWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == string("usb_connect_list_title"))
    {
        ctrl->SetCtrlTransparentStyle(false);
    }
}

void USBModeWindow::InitUSBWin()
{
    R *r = R::get();
    StringVector usb_datas;
    r->GetString("tl_usb_connect", title_str);
    r->GetString("tl_usb_charge", charge_mode_str);
    r->GetString("tl_usb_mass_storage", mass_storage_str);
    r->GetString("tl_usb_webcam", webcam_mode_str);
    title_ = reinterpret_cast<TextView*>(GetControl("usb_connect_list_title"));
    title_->SetTextStyle(DT_VCENTER | DT_CENTER | DT_SINGLELINE);
    title_->SetCaptionEx(title_str.c_str());
    title_->SetCaptionColor(0xFFFFFFFF);
    
    /*set the list table view*/
    list_view_ = reinterpret_cast<ListView *> (GetControl("usb_connect_list"));
    list_view_->SetBackColor(0xFF36EFED);
    list_view_->RemoveAllItems();

    std::vector<LVCOLUMN> columns;
    LVCOLUMN col;
    col.pfnCompare  = NULL;
	col.width = DIALOG_WIDTH;
    col.pszHeadText = (char *)"mode";
    col.colFlags    = LVCF_CENTERALIGN;
    columns.push_back(col);
    list_view_->SetColumns(columns, false);
    usb_datas.push_back(charge_mode_str);
    usb_datas.push_back(mass_storage_str);
    //usb_datas.push_back(webcam_mode_str);

    LVSUBITEM subdata;
    LVITEM item;
    item.dwFlags  = 0;
    item.dwFlags &= ~LVIF_FOLD;
    item.nItemHeight = ItemHight;
    for (int i = 0; i < usb_datas.size(); i++) 
    {
        item.nItem = i;
        list_view_->AddItem(item);

        subdata.nTextColor = PIXEL_black;
        subdata.pszText = const_cast<char*>((usb_datas[i]).c_str());
        subdata.flags   = 0;
        subdata.image   = 0;
        subdata.nItem = i;
        subdata.subItem=0;
        list_view_->FillSubItem(subdata);
    }
    int bg_color = GetWindowElementAttr (list_view_->GetHandle(), WE_BGC_WINDOW);
    SetWindowElementAttr(list_view_->GetHandle(), WE_BGC_HIGHLIGHT_ITEM, bg_color);
	cur_usb_select = 0;
    list_view_->SelectItem(0);
    list_view_->Show();
}
void USBModeWindow::OnLanguageChanged()
{
    this->InitUSBWin();
}

void USBModeWindow::DoUsbKeyHanderSelectionDialog()

{
    db_warn("DoKeyHanderSelectionDialog");
    if(cur_usb_select != USB_CHARGE_MODE){
		cur_usb_select = USB_CHARGE_MODE;
	}else{
		cur_usb_select = USB_MASS_STORAGE_MODE;
	}
	list_view_->SelectItem(cur_usb_select);
	list_view_->Refresh();
}

void USBModeWindow::DoShow()
{
	title_->Hide();
	list_view_->Show();
	R *r = R::get();
    r->GetString("tl_usb_connect", title_str);
    title_ = reinterpret_cast<TextView*>(GetControl("usb_connect_list_title"));
    title_->SetCaptionEx(title_str.c_str());
    title_->Show();
    GetControl("mass_mode_icon")->Hide();
    db_error("USBModeWindow DoShow");
    Window::DoShow();
}

void USBModeWindow::DoHide()
{
    list_view_->Hide();
    title_->Hide();
    GetControl("mass_mode_icon")->Hide();
    SetVisible(false);
}


