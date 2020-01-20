/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: x_memory.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _X_MEMORY_H_
#define _X_MEMORY_H_
#include "type/types.h"

class XMemory
{
public:
    XMemory();
    ~XMemory();
    static XMemory* get();
    void *alloc(char *class_name, int size);
    void free(char *class_name, void *p);
    void dump(char *class_name);
private:
    static pthread_mutex_t mutex_;
    static XMemory* instance_;
    StringIntMap class_size_map_;
};


#define  xmalloc(size) \
    (XMemory::get()->alloc(this->GetClassName(), (size)))

#define xfree(p) \
    (XMemory::get()->free(this->GetClassName(), (p)))

#define xdump() \
    XMemory::get()->dump(this->GetClassName())
#endif //_X_MEMORY_H_
