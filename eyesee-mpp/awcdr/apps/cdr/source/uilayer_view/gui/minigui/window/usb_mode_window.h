/* *******************************************************************************
 Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 *********************************************************************************
 File Name:         usb_mode_window.h
 Version:             1.0
 Author:              KPA362
 Created:            2017/4/10
 Description:       usb window show &  usb function select
 * *******************************************************************************/

#pragma once

#include "window/window.h"
#include "window/user_msg.h"

#define USB_CHARGING      (USER_MSG_BASE+1000)
#define USB_MASS_STORAGE    (USER_MSG_BASE+1001)
#define USB_UVC             (USER_MSG_BASE+1002)
class ListView;
class TextView;
class USBModeWindow
        : public SystemWindow 
{
    DECLARE_DYNCRT_CLASS(USBModeWindow, Runtime)

    public:
        USBModeWindow(IComponent *parent);
        
        virtual ~USBModeWindow();

        std::string GetResourceName();


        /****************************************************
                Name:    HandleMessage()
                Description:
                    handle window msg 
               *****************************************************/
        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        /*********************************************************
                Name:    PreInitCtrl()
                Description:
                    if use control's backgroundclor,we should imcomplete  this function 
               **********************************************************/
        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        /****************************************************
                Name:    keyProc()
                Description:
                    handle window key
               *****************************************************/
        void keyProc(int keyCode, int isLongPress);

        /****************************************************
                Name:    GetCreateParams()
                Description:
                    get window create params
               *****************************************************/
        void GetCreateParams(CommonCreateParams& params);

        void InitUSBWin();
        void OnLanguageChanged();
        void ListItemClickProc();
        void ListItemClickProc(int nItem);
        void DoShow();
        void DoHide();
		int cur_usb_select;
		void DoUsbKeyHanderSelectionDialog();
        inline void SetUSBWinMessageReceiveFlag(bool flag){ignore_message_flag_ = flag;}
    protected:
        virtual void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    private:
        enum UsbWorkingStatus {
            USB_NORMAL_STATUS=0,
            USB_MASS_STORAGE_STATUS,
            USB_UVC_STATUS,
        };
        enum UsbModeItem {
            USB_CHARGE_MODE=0,
            USB_MASS_STORAGE_MODE,
            USB_WEBCAM_MODE,
        };
    UsbWorkingStatus m_current_status;
    TextView * title_;
    std::string title_str, charge_mode_str, mass_storage_str,webcam_mode_str;
    ListView *list_view_;
    bool ignore_message_flag_;
    int ItemHight;
};
