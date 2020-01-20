/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: utils.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-18
 Description:
    common api for all modules
 History:
*****************************************************************************/

#ifndef _API_H_
#define _API_H_
#include "type/types.h"

#include <stdio.h>

/* return the result whether the file is existed */
extern bool FileExists( const char* fileName);

/* get one line string from FILE stream */
extern int  __getline(char **lineptr, int *n, FILE *stream);

/* get the string
 *  from 'search_string_start' to 'search_string_end' in the string 'line'
 *
 */
const char* TrimString2(const char* line, const char* search_string_start,
                const char* search_string_end, std::string &result);

/* get the string from the first 'search_string'
 * to the second 'serach_string' in string 'line'
 */
const char* TrimString(const char* line, const char* search_string, std::string &result);

#endif //_API_H_
