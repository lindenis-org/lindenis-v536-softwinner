/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: progress_bar.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#include "widgets/progress_bar.h"
#include "widgets/ctrlclass.h"
#include "window/user_msg.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ProgressBar"
#include "debug/app_log.h"
#include "progress_bar.h"
//#define USE_LEFT_RIGHT_LABEL
#define IDC_LABEL_LEFT  601
#define IDC_LABEL_RIGHT 602
#define IDC_PROGRESSBAR 603


static WNDPROC oldProc;

IMPLEMENT_DYNCRT_CLASS(ProgressBar)

ProgressBar::ProgressBar(View *parent)
    : CustomWidget(parent)
{
    progress_data_.nMin = 0;
    progress_data_.nMax = 100;
    progress_data_.nPos = 0;
    progress_data_.nStepInc = 1;
}

ProgressBar::~ProgressBar()
{

}

void ProgressBar::GetCreateParams(CommonCreateParams &params)
{
    params.class_name = CTRL_SEEKBAR;
    params.alias      = GetClassName();
    params.style      = WS_VISIBLE;
    params.exstyle    = WS_EX_USEPARENTFONT | transparent_style_;
    params.x          = 0;
    params.y          = 0;
    params.w          = DEFAULT_CTRL_WIDTH;
    params.h          = DEFAULT_CTRL_HEIGHT;
}


/************* not support vertical ******************/
static void my_draw_progress (HWND hwnd, HDC hdc, int nMax, int nMin,
                                        int nPos)
{
    RECT    rcClient;
    int     x, y, w, h;
    ldiv_t   ndiv_progress;
    unsigned int     nAllPart;
    unsigned int     nNowPart;
    int     whOne, nRem;
    int     ix;
    unsigned int     i;
    int     step;
    int pbar_border = 1;
    gal_pixel old_color;

    if (nMax == nMin)
        return;

    if ((nMax - nMin) > 5)
        step = 1;
    else
        step = 1;

    GetClientRect (hwnd, &rcClient);

    x = rcClient.left + pbar_border;
    y = rcClient.top + pbar_border;
    w = RECTW (rcClient) - (pbar_border << 1);
    h = RECTH (rcClient) - (pbar_border << 1);

    //SetWindowBkColor(hwnd, RGBA2Pixel(hdc, 0xff, 0xff, 0xff, 0x4c));
    SetWindowBkColor(hwnd, 0x4CFFFFFF);
    if (hwnd != HWND_NULL)
        old_color = SetBrushColor (hdc, GetWindowBkColor (hwnd));
    else
        old_color = SetBrushColor (hdc,
                    GetWindowElementPixel(HWND_DESKTOP, WE_BGC_DESKTOP));

    //draw the erase background
    FillBox (hdc, rcClient.left, rcClient.top,
            RECTW (rcClient), RECTH (rcClient));

    SetPenColor(hdc, old_color);

    ndiv_progress = ldiv (nMax - nMin, step);
    nAllPart = ndiv_progress.quot;

    ndiv_progress = ldiv (nPos - nMin, step);
    nNowPart = ndiv_progress.quot;

    ndiv_progress = ldiv (w, nAllPart); /* calculate the with for each step*/

    /* set the fill color */
    //SetBrushColor(hdc, RGB2Pixel(hdc, 0xF7, 0x95, 0x00));
	SetBrushColor(hdc, 0xFF2772DB);

#if 0
    whOne = ndiv_progress.quot;
    nRem = ndiv_progress.rem;

    /* dislay the % */
    if (whOne >= 4) {
        for (i = 0, ix = x + 1; i < nNowPart; ++i) {
            if ((ix + whOne) > (x + w))
                whOne = x + w - ix;

            FillBox (hdc, ix, y + 1, whOne, h - 2);
            ix += whOne;
/*
            if(nRem > 0) {
                ix ++;
                nRem --;
            }
*/
        }
    } else
#endif
    {
        int prog = w * nNowPart/nAllPart;

        FillBox (hdc, x, y, prog, h);
    }
}

static long int NewProgressBarProc (HWND hwnd, unsigned int message, WPARAM wparam,
                                        LPARAM lparam)
{
    HDC           hdc;
    PCONTROL      pCtrl;

    pCtrl = gui_Control (hwnd);

    switch(message) {
    case MSG_CREATE:
        {
        }
        break;
    case MSG_PAINT:
        {
            PROGRESSDATA *data = (PROGRESSDATA *)pCtrl->dwAddData2;
            hdc = BeginPaint (hwnd);

            my_draw_progress (hwnd, hdc, data->nMax, data->nMin, data->nPos);

            EndPaint (hwnd, hdc);
            return 0;
        }

    default:
        break;
    }

    return (*oldProc) (hwnd, message, wparam, lparam);
}

int ProgressBar::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch(message) {
    case MSG_CREATE:
        {
            HWND retWnd;
            RECT rect;
            int x, y, w, h;
            db_msg(" ");
            ProgressBarData_t PGBData_;
            PGBData_.bgcWidget = PIXEL_red;
            PGBData_.fgcWidget = PIXEL_green;

            ProgressBarData_t* PGBData;
            PGBData = &PGBData_;//(ProgressBarData_t*)lparam;

/************************************************************
    |  _________     _____________________     _________  |
    |1| label_w | 5 |    progressbar_w    | 5 | label_w |1|
    |  ---------     ---------------------     ---------  |
*************************************************************/
#ifdef USE_LEFT_RIGHT_LABEL

            /********************** create the left label *******************/
            retWnd = CreateWindowEx(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE | SS_SIMPLE,
                    WS_EX_NONE | SS_RIGHT,
                    IDC_LABEL_LEFT,
                    0, 0, 0, 0,
                    hwnd, 0);
            if(retWnd == HWND_INVALID) {
                db_error("create playback progress bar label left failed");
                break;
            }
            SetWindowBkColor(retWnd, PIXEL_black);
            /**************** create the right label ******************/
            retWnd = CreateWindowEx(CTRL_STATIC, NULL,
                    WS_CHILD | WS_VISIBLE | SS_SIMPLE,
                    WS_EX_NONE | SS_LEFT,
                    IDC_LABEL_RIGHT,
                    0, 0, 0, 0,
                    hwnd, 0);
            if(retWnd == HWND_INVALID) {
                db_error("create playback progress bar label right failed");
                break;
            }
            SetWindowBkColor(retWnd, PIXEL_black);
#endif
            /***************  create the progress bar*****************/
            retWnd = CreateWindowEx(CTRL_PROGRESSBAR, NULL,
                    WS_CHILD | WS_VISIBLE,
                    WS_EX_NONE,
                    IDC_PROGRESSBAR,
                    0, 0, 0, 0,
                    hwnd, 0);
            if(retWnd == HWND_INVALID) {
                db_error("create playback progress bar label right failed");
                break;
            }
            oldProc = SetWindowCallbackProc(retWnd, NewProgressBarProc);

            // SetWindowBkColor(hwnd, PGBData->bgcWidget);
            // SetWindowElementAttr(GetDlgItem(hwnd, IDC_LABEL_LEFT),
                // WE_FGC_WINDOW, 0xFFF79500);
            // SetWindowElementAttr(GetDlgItem(hwnd, IDC_LABEL_RIGHT),
                // WE_FGC_WINDOW, 0xFFF79500);

            PROGRESSDATA prg_data;
            prg_data.nMin = 0;
            prg_data.nMax = 100;
            prg_data.nPos = 0;
            prg_data.nStepInc = 1;

            SendMessage(hwnd, PGBM_SETTIME_RANGE, (WPARAM)&prg_data, 0);
            SendMessage(hwnd, PGBM_SETCURTIME, (WPARAM)&prg_data, 0);
            SendMessage(hwnd, PGBM_SETSTEP, (WPARAM)&prg_data, 0);
        }
        break;
    case MSG_SIZECHANGED: {
            RECT rect;
            GetClientRect(hwnd, &rect);
            int c_x, c_y, c_w, c_h; // control's x, y, w, h
            c_x = rect.left;
            c_y = rect.top;
            c_w = rect.right;
            c_h = rect.bottom;
            // label char length
            int char_len = strlen("00:00");

            // label width
            int label_w = 8*char_len;

            // progress bar width
            int pgbar_w =c_w;// c_w-(2*label_w+12);

            // visible part height
            int v_h = c_h;

            // visible part y
            int v_y = c_y;//(c_h - v_h) / 2;

            // left label x
            int ll_x =c_x + 1;

            // progress bar x
            int pgbar_x = c_x;// ll_x + label_w + 5;

            // right label x
            int rl_x = ll_x+label_w+2;//pgbar_x + pgbar_w + 5;
	#ifdef USE_LEFT_RIGHT_LABEL
	     MoveWindow(GetDlgItem(hwnd, IDC_LABEL_LEFT), ll_x, v_y , label_w, v_h, false);
            MoveWindow(GetDlgItem(hwnd, IDC_LABEL_RIGHT), rl_x, v_y, label_w, v_h, false);
	#endif
            MoveWindow(GetDlgItem(hwnd, IDC_PROGRESSBAR), pgbar_x, v_y , pgbar_w, v_h, false);
        }
        break;
    case PGBM_SETTIME_RANGE:
        {
            PGBTime_t start_time, end_time;
            char buf_left[20] = {0}, buf_right[20] = {0};

            /** wparam is left label time, lparam is right label time *****/
            PROGRESSDATA *prg_data = (PROGRESSDATA *)wparam;

            start_time.min = prg_data->nMin / 60;
            start_time.sec = prg_data->nMin % 60;
            end_time.min = prg_data->nMax / 60;
            end_time.sec = prg_data->nMax % 60;
#ifdef USE_LEFT_RIGHT_LABEL
            snprintf(buf_left, sizeof(buf_left),"%02d:%02d/", start_time.min, start_time.sec);
            snprintf(buf_right, sizeof(buf_right),"%02d:%02d", end_time.min, end_time.sec);
            SetWindowText(GetDlgItem(hwnd, IDC_LABEL_LEFT), buf_left);
            SetWindowText(GetDlgItem(hwnd, IDC_LABEL_RIGHT), buf_right);
#endif
            SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_SETRANGE, prg_data->nMin, prg_data->nMax);
            SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_SETPOS, prg_data->nPos, 0);
        }
        break;
    case PGBM_SETCURTIME:
        {
            char buf_cur[20] = {0};
            PGBTime_t cur_time;

            PROGRESSDATA *prg_data = (PROGRESSDATA *)wparam;

            cur_time.min = prg_data->nPos / 60;
            cur_time.sec = prg_data->nPos % 60;
#ifdef USE_LEFT_RIGHT_LABEL
            snprintf(buf_cur, sizeof(buf_cur),"%02d:%02d/", cur_time.min, cur_time.sec);
            SetWindowText(GetDlgItem(hwnd, IDC_LABEL_LEFT), buf_cur);
#endif
            SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_SETPOS, prg_data->nPos, 0);
        }
        break;
    case PGBM_SETSTEP: {
        PROGRESSDATA *prg_data = (PROGRESSDATA *)wparam;
        return SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_SETSTEP, prg_data->nStepInc, 0);
    }
    case PGBM_DELTAPOS:
        return SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_DELTAPOS, wparam, 0);
    case PGBM_STEPIT: {
        char buf_cur[20] = {0};
        PGBTime_t cur_time;

        PROGRESSDATA *prg_data = (PROGRESSDATA *)wparam;
        prg_data->nPos += prg_data->nStepInc;

        if (prg_data->nPos > prg_data->nMax) break;

        cur_time.min = prg_data->nPos / 60;
        cur_time.sec = prg_data->nPos % 60;
#ifdef USE_LEFT_RIGHT_LABEL
        snprintf(buf_cur, sizeof(buf_cur),"%02d:%02d/", cur_time.min, cur_time.sec);
        SetWindowText(GetDlgItem(hwnd, IDC_LABEL_LEFT), buf_cur);
#endif
        return SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_STEPIT, 0, 0);
    }
    default:
        return CustomWidget::HandleMessage( hwnd, message, wparam, lparam );
    }
    return HELP_ME_OUT;
}

int ProgressBar::OnMouseUp(unsigned int button_status, int x, int y)
{
    RECT rect;
    ::GetClientRect(GetDlgItem(handle_, IDC_PROGRESSBAR), &rect);

    int s_x = 0, s_y = 0;
    ::ClientToScreen(GetDlgItem(handle_, IDC_PROGRESSBAR), &s_x, &s_y);

    float s_w = RECTW(rect);
    double seek = (x- s_x) / s_w * 100;

    int sec = seek / 100 * progress_data_.nMax;

#if 1
    db_debug("seek to: %f%%, sec: %d", seek, sec);
#endif
    this->SetProgressSeekValue(sec);

    if (OnProgressSeek)
        OnProgressSeek(this, sec);
    return 0;
}

void ProgressBar::SetProgressRange(int min, int max)
{
    progress_data_.nMin = min;
    progress_data_.nMax = max;

    ::SendMessage(handle_, PGBM_SETTIME_RANGE, (WPARAM)&progress_data_, 0);
}

void ProgressBar::SetProgressSeekValue(int pos)
{
    progress_data_.nPos = pos;
    ::SendMessage(handle_, PGBM_SETCURTIME, (WPARAM)&progress_data_, 0);
}

void ProgressBar::SetProgressStep(int step)
{
    progress_data_.nStepInc = step;
    SendMessage(handle_, PGBM_SETSTEP, (WPARAM)&progress_data_, 0);
}

void ProgressBar::UpdateProgressByStep()
{
    ::SendMessage(handle_, PGBM_STEPIT, (WPARAM)&progress_data_, 0);
}
