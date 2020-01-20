/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: listbox_impl.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef __LISTBOX_IMPL_H_
#define __LISTBOX_IMPL_H_

#include "widgets.h"
#ifdef  __cplusplus
extern  "C" {
#endif

/******* LBIF is used internal by the listbox *********/
#define LBIF_NORMAL         0x0000L
#define LBIF_SELECTED       0x0001L
#define LBIF_USEBITMAP      0x0002L

#define LBIF_DISABLE        0x0004L
#define LBIF_BOLDSTYLE      0x0008L

#define LBIF_BLANK          0x0000L
#define LBIF_CHECKED        0x0010L
#define LBIF_PARTCHECKED    0x0020L
#define LBIF_SIGNIFICANT    0x0040L
#define LBIF_CHECKMARKMASK  0x00F0L

/*********  *************/
#define LBIF_IMAGE          0x00010000      /*  if use item image*/
#define LBIF_VALUE_STRING   0x00020000      /*  if use value String */
#define LBIF_VALUE_IMAGE    0x00040000      /*  if use value Image */
#define LBIF_VALUE_STRING2  0x00200000      /*  if use value String2 */
#define LBIF_VALUE_IMAGE2   0x00400000      /*  if use value Image2 */
#define LBIF_WITHCHECKBOX   0x02000000

#define MAX_VALUE_COUNT 3

#define INSERTITEM 100
#define DELETEITEM 200

/*
 * first    item image (optional)
 * second   item string (required)
 * third    item value (picture or string)
 * */
typedef struct _LISTBOXITEM
{
    char*   key;                // item sort key

    DWORD   dwFlags;            // item flags

    DWORD   imageFlag;
    unsigned int valueCount;    /* the value's count */
    DWORD   valueFlag[MAX_VALUE_COUNT];

    char*   first_icon_image[2];            // item image show in the left
    DWORD   first_icon_value[0];            // item image show in the left
    DWORD   dwValueImage[MAX_VALUE_COUNT];  // value image show in the right
    char*   result_string;  // value string show in the right

    DWORD   dwAddData;          // item additional data
    struct  _LISTBOXITEM* next; // next item
} LISTBOXITEM;
typedef LISTBOXITEM* PLISTBOXITEM;



#ifndef _MGRM_THREADS
    #define DEF_LB_BUFFER_LEN       8
#else
    #define DEF_LB_BUFFER_LEN       16
#endif

#define LBF_FOCUS               0x0001

#ifdef  __cplusplus
}
#endif

#endif  // __LISTBOX_IMPL_H__

