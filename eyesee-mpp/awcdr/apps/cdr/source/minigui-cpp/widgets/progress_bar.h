/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: progress_bar.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _PROGRESS_BAR_H_
#define _PROGRESS_BAR_H_

#include "widgets.h"
#include "data/gui.h"

extern int ProgressbarCallback(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

typedef struct {
    gal_pixel bgcWidget;
    gal_pixel fgcWidget;
} ProgressBarData_t;

typedef struct {
    unsigned char sec;            /* Seconds. [0-60] (1 leap second) */
    unsigned char min;            /* Minutes. [0-59] */
} PGBTime_t;

typedef  struct tagPROGRESSDATA
{
    int nMin;
    int nMax;
    int nPos;
    int nStepInc;
}PROGRESSDATA;
typedef PROGRESSDATA* PPROGRESSDATA;

typedef fastdelegate::FastDelegate2<View*, int> SeekEvent;     //simple notify event

class ProgressBar
        : public CustomWidget {
    DECLARE_DYNCRT_CLASS(ProgressBar, Runtime)

    public:
        SeekEvent OnProgressSeek;

        ProgressBar(View *parent);

        ~ProgressBar();

        virtual void GetCreateParams(CommonCreateParams &params);

        virtual int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

        virtual int OnMouseUp(unsigned int button_status, int x, int y);

        void SetProgressRange(int min, int max);

        void SetProgressSeekValue(int pos);

        void SetProgressStep(int step);

        void UpdateProgressByStep();

    private:
        PROGRESSDATA progress_data_;
};


#endif //_PROGRESS_BAR_H_
