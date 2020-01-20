/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: resource_manager.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "type/types.h"
#include "parser/images_parser.h"
#include "parser/string_parser.h"
#include "resource/language.h"
//#include "device_model/menu_config_lua.h"


class Language;

class R : public ImageParser, StringParser
{
public:
    R();
    ~R();
    static R* get();
    std::string GetImagePath(const char* alias);
    void LoadImagesPath();
    void LoadImages(std::string path);
    std::string GetImagesFile();

    /* load the strings what the TextView displays */
    void LoadStrings(StringMap &text_map);
    LOGFONT* GetFont();
    LOGFONT*GetFontBySize(int sz);
    std::string GetLangFile();
    int SetLangID(int pLangID);
    int InitLanguage(StringIntMap pLangInfo);
    void GetString(std::string item_name, std::string &result);
    void GetStringArray(std::string array_name, StringVector &result);
   // int GetCurrentLanglD();
private:
    static pthread_mutex_t mutex_;
    static R* instance_;
    std::string strings_file_;
    std::string images_file_;
    Language *language_;
};

#endif //_RESOURCE_MANAGER_H_
