#ifndef __BUTTON_POS_H__
#define __BUTTON_POS_H__

#include <stdio.h>
#include <stdlib.h>


typedef struct buttonPos
{
    unsigned int x1;
    unsigned int y1;
    unsigned int x2;
    unsigned int y2;
}buttonPos_;

#define PrviewButtonPosLen 6

typedef enum
{
    PREVIEW_BUTTON_POS_RECORD_ID = 0,
    PREVIEW_BUTTON_POS_PHOTO_ID,
    PREVIEW_BUTTON_POS_LOCKED_ID,
    PREIVEW_BUTTON_POS_PLAYBACK_ID,
    PREVIEW_BUTTON_POS_SETTING_ID,
    PREVIEW_BUTTON_POS_VOICE_ID,
};

#define PlayBackButtonPosLen 3

typedef enum
{
    PLAYBACK_BUTTON_POS_PREVIOUS_ID = 0,
    PLAYBACK_BUTTON_POS_LOCKED_ID,
    PLAYBACK_BUTTON_POS_NEXT_ID,
};

#define PlayingButtonPosLen 5

typedef enum
{
    PLAYING_BUTTON_POS_RETURN_ID = 0,
    PLAYING_BUTTON_POS_PREVIOUS_ID,
    PLAYING_BUTTON_POS_NEXT_ID,
    PLAYING_BUTTON_POS_DELETE_ID,
    PLAYING_BUTTON_POS_START_PAUSE_ID,
};

#ifdef __cplusplus
extern "C" {
#endif

int getTouchPosID(unsigned int x,unsigned y,struct buttonPos bpos[],int len);

#ifdef __cplusplus
}  /* end of extern "C" */
#endif


#endif

