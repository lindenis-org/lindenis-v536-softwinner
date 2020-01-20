/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: x_memory.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "memory/x_memory.h"
#include "widgets/view.h"

#include <malloc.h>

using namespace std;

XMemory* XMemory::instance_ = NULL;
pthread_mutex_t XMemory::mutex_ = PTHREAD_MUTEX_INITIALIZER;


XMemory::XMemory()
{

}

XMemory::~XMemory()
{

}

XMemory* XMemory::get()
{
    if ( XMemory::instance_ == NULL )
    {
        pthread_mutex_lock( &XMemory::mutex_ );
        if (XMemory::instance_ == NULL )
        {
            XMemory::instance_ = new XMemory();
        }
        pthread_mutex_unlock( &XMemory::mutex_ );
    }
    return XMemory::instance_;
}

void *XMemory::alloc(char *class_name, int size)
{
    void *p = calloc(1, size);
    size = malloc_usable_size(p);
    string key(class_name);
    if (p) {
        StringIntMap::iterator iter = class_size_map_.find(key);
        int alloced_size = 0;
        if (iter != class_size_map_.end()) {
            alloced_size = iter->second;
            alloced_size += size;
            iter->second = alloced_size;
        } else {
            class_size_map_.insert(make_pair(key, size));
        }
    }
    return p;
}

void XMemory::free(char *class_name, void *p)
{
    int size = malloc_usable_size(p);
    ::free(p);
    string key(class_name);
    StringIntMap::iterator iter = class_size_map_.find(key);
    int alloced_size = 0;
    if (iter != class_size_map_.end()) {
        alloced_size = iter->second;
        alloced_size -= size;
        iter->second = alloced_size;
    }
}

void XMemory::dump(char *class_name)
{
    string key(class_name);
    StringIntMap::iterator iter = class_size_map_.find(key);
    if (iter != class_size_map_.end()) {
        db_msg("class %s used memory %d bytes", key.c_str(),
            iter->second);
    }
}
