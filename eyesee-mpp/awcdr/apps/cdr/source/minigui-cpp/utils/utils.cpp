/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: api.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-18
 Description:
    common API for all modules
 History:
*****************************************************************************/

#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

bool FileExists( const char* fileName )
{
    if (0 == access( fileName, 0 ))
        return true;
    else
        return false;
}


#define maxsize 35
/*****************************************************************************
 Function: __getline
 Description:
    the first parameter memory *lineptr need to be release outside
 Parameter:
 Return:
*****************************************************************************/
int  __getline(char **lineptr, int *n, FILE *stream)
{
    int count = 0;
    int buf;

    if(*lineptr == NULL) {
        *n = maxsize;
        *lineptr = (char*)malloc(*n);
    }

    if(( buf = fgetc(stream) ) == EOF ) {
        return -1;
    }

    do
    {
        if(buf=='\n') {
            count += 1;
            break;
        }

        count++;

        *(*lineptr+count-1) = buf;
        *(*lineptr+count) = '\0';

        if(*n <= count)
            *lineptr = (char*)realloc(*lineptr,count*2);
        buf = fgetc(stream);
    } while( buf != EOF);

    return count;
}

/*****************************************************************************
 Function: TrimString2
 Description:get the truncation from the first location of 'search_string_start'
    to the first loacation of 'search_string_end' in the string 'line'
 Parameter:
 Return:
*****************************************************************************/
const char* TrimString2(const char* line,
const char* search_string_start, const char* search_string_end, string &result)
{
    char buf[256]={0};//@risk
    const char *str_start = NULL;
    const char *str_end = NULL;

    str_start= strstr(line, search_string_start);
    if (!str_start) {
        return NULL;
    }
    str_end  = strstr(str_start+1, search_string_end);
    if (!str_end) {
        return NULL;
    }

    strncpy(buf, str_start+1, str_end - str_start - 1);
    result = buf;

    return str_start + 1;
}


const char* TrimString(const char* line,
    const char* search_string, string &result)
{
    return TrimString2(line, search_string, search_string, result);
}

