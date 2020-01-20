/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: cliprect.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef GUI_CLIPRECT_H
    #define GUI_CLIPRECT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct tagGCRINFO
{
#ifdef _MGRM_THREADS
    pthread_mutex_t lock;
#endif
    CLIPRGN         crgn;
    unsigned int    age;
    unsigned int    old_zi_age;
} GCRINFO;
typedef GCRINFO* PGCRINFO;

typedef struct tagINVRGN
{
#ifdef _MGRM_THREADS
    pthread_mutex_t lock;
#endif
    CLIPRGN         rgn;
    int             frozen;
} INVRGN;
typedef INVRGN* PINVRGN;

/* Function definitions */

#define MAKE_REGION_INFINITE(rgn) do { \
        RECT rc = {INT_MIN/2,INT_MIN/2,INT_MAX/2,INT_MAX/2}; \
        SetClipRgn(rgn, &rc); \
    } while (0)

#undef _REGION_DEBUG

#ifdef _REGION_DEBUG
void dbg_dumpRegion (CLIPRGN* region);
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GUI_CLIPRECT_H */
