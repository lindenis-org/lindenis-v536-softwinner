/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: types.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _TYPES_H_
#define _TYPES_H_

#include <map>
#include <string>
#include <vector>
#include <string.h>

typedef void* HWND_EX;
class View;
class Window;
class Runtime;
class ControllerBase;

typedef std::vector<std::string> StringVector;
typedef std::map<HWND_EX, View*> KeyMap;
typedef std::map<int, Window*> WindowMap;
typedef std::map<int, ControllerBase*> ControllerMap;
typedef std::map<std::string, View*> CtrlMap;
typedef std::map<std::string, int> StringIntMap;
typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, StringVector> StringVectorMap;

#define SDV_KEY_LEFT    SCANCODE_SUNXILEFT
#define SDV_KEY_RIGHT   SCANCODE_SUNXIRIGHT
#define SDV_KEY_MODE    SCANCODE_SUNXIMODE
#define SDV_KEY_OK      SCANCODE_SUNXIOK
#define SDV_KEY_POWER   SCANCODE_SUNXIPOWER
#define SDV_KEY_MENU    SCANCODE_SUNXIMENU
#define SDV_KEY_RETURN  SCANCODE_SUNXIRETURN
#define SDV_KEY_VIRTUAL (SDV_KEY_POWER + 10)

#define LONG_PRESS_TIME 200 /* unit:10ms */
#define ID_PREVIEW_TIMER_KEY 100
#define ID_PLYLIST_TIMER_KEY 101
#define ID_PLAYBACK_TIMER_KEY 102
#define ID_MENU_TIMER_KEY 103
#define ID_LISTVIEW_TIMER_KEY 104
#define ID_USB_MODE_TIMER_KEY 105
#define ID_SETTING_HANDLER_WINDOW_TIMER_KEY 106
#define ID_PROMPT_TIMER_KEY 107
#define ID_DIALOG_TIMER_KEY 108
#define ID_SUBMENU_TIMER_KEY 109
#define ID_TIMESETTING_TIMER_KEY 110
#define ID_DIGHT_ZOOM_KEY 111
#define ID_PLAYBACK_SEEK_KEY 112

enum {
    SHORT_PRESS = 0,
    LONG_PRESS
};

inline View* KeyMapSearch(KeyMap view_map, HWND_EX handle)
{
    KeyMap::iterator iter = view_map.find(handle);
    if ( iter != view_map.end() ) {
        return iter->second;
    }
    return NULL;
}

#endif //_TYPES_H_
