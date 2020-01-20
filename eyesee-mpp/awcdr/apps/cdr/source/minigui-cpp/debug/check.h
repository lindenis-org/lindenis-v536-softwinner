/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: check.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _CHECK_H_
#define _CHECK_H_

#define NULL_BREAK(pointer) \
    if (NULL == (pointer)) { \
        break; \
    }

#define NULL_RETURN(pointer, value) \
    if (NULL == (pointer)) { \
        return value; \
    }

#endif //_CHECK_H_
