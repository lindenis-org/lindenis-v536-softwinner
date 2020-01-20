#include <stdio.h>
#include "buttonPos.h"
int getTouchPosID(unsigned int x,unsigned y,struct buttonPos bpos[],int len)
{
    int i = 0;
    for(i = 0 ; i < len ; i++)
    {
        if((bpos[i].x1 < x && x< bpos[i].x2) && (bpos[i].y1 <= y && y < bpos[i].y2))
            return i;
    }
    printf("[habo]---> getTouchPosId fail \n");
    return -1;
}

