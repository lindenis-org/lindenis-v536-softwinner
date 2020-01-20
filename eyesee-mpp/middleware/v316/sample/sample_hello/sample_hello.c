/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_face_detect.c
  Version       : V6.0
  Author        : Allwinner BU3-XIAN Team
  Created       :
  Last Modified : 2017/11/30
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>

//#include <string.h>
//#include <pthread.h>
//#include <assert.h>
//#include <time.h>

#include "sample_hello.h"

int main(int argc, char *argv[])
{
    printf("hello, world! [%s][%s][%s][%d][%s].\n", __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__);
    int ret = 0;

    wordexp_t p;
    char **w;
    int i;

    wordexp("ls -al *.c", &p, 0);
    w = p.we_wordv;
    for (i = 0; i < p.we_wordc; i++)
        printf("%s\n", w[i]);
    wordfree(&p);

    return ret;
}

