/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file dialog.h
 * @brief 对话框窗口
 * @author id:826
 * @version v0.3
 * @date 2016-07-07
 */

#pragma once


#include "window/window.h"
#include "window/user_msg.h"
#include "widgets/graphic_view.h"

#define CONFIRM_BUTTON  (USER_MSG_BASE+1)
#define CANCEL_BUTTON   (USER_MSG_BASE+2)

enum DIALOG_TYPE {
    SELECTION_DIALOG = 0,
    INFO_DIALOG,
    SET_FIRMWARE_DOWNLOAD,
};

enum SELECT_TYPE {
    BUTTON_OK = 0,
    BUTTON_CANCEL,
};
class Dialog
    : public SystemWindow
{
    DECLARE_DYNCRT_CLASS(Dialog, Runtime)

    public:
        NotifyEvent OnClick;

        Dialog(IComponent *parent);

        Dialog(DIALOG_TYPE type, IComponent *parent);

        virtual ~Dialog();

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        std::string GetResourceName();

        void GetCreateParams(CommonCreateParams &params);

        int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        void PreInitCtrl(View *ctrl, std::string &ctrl_name);

        void DoShow();

        void DoHide();
        void ShowParent();
        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        void keyProc(int keyCode,int isLongPress);
        void DoKeyLeftHanderInfoDialog();
        void DoKeyRightHanderInfoDialog();
        void DoKeyLeftHanderSelectionDialog();
        void DoKeyRightHanderSelectionDialog();
        void DoKeyOkHanderInfoDialog();
        void DoKeyOkHanderSelectionDialog();
		void DoKeyHanderSelectionDialog();  //sun
        void FocusOkButton();
        void FocusCancelButton();
        int  SetKeyNewHandleFlag(bool p_bNewType);
        void DoKeyHanderInfoDialog(int type);
        void CancelDialog();
        void DoHideDialog();
        void SetDialogMsgType(DIALOG_TYPE type); 
    private:
        DIALOG_TYPE type_;
		int cur_select;
		DWORD default_color;
		DWORD focus_color;
        bool m_nNewHandletype;
        bool m_initFocusFlag;
		 pthread_mutex_t d_lock_;
};
