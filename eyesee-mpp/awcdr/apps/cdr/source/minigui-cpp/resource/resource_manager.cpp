/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: resource_manager.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/
#define NDEBUG

#include "resource/resource_manager.h"
#include "parser/images_parser.h"



#include "debug/app_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "R"

using namespace std;

R* R::instance_ = NULL;
pthread_mutex_t R::mutex_ = PTHREAD_MUTEX_INITIALIZER;

R* R::get()
{
    if ( R::instance_ == NULL )
    {
        pthread_mutex_lock( &R::mutex_ );
        if (R::instance_ == NULL )
        {
            R::instance_ = new R();
        }
        pthread_mutex_unlock( &R::mutex_ );
    }
    return R::instance_;
}


R::R()
{
    language_ = Language::get();
    StringIntMap pLangInfo;
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-CN.xml"), 0));	// 简体中文
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-EN.xml"), 1));	// 英语
#if 1 //add by zhb
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-TW.xml"), 2));	// 繁体中文
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-JA.xml"), 3));	// 日语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-KO.xml"), 4));	// 韩语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-RU.xml"), 5));	// 俄语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-DE.xml"), 6));	// 德语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-FR.xml"), 7));	// 法语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-IT.xml"), 8));	// 意大利语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-ES.xml"), 9));	// 西班牙语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-PT.xml"), 10));	// 葡萄牙语
    pLangInfo.insert(make_pair(string("/usr/share/minigui/res/lang/zh-TI.xml"), 11));	// 泰语
#endif
    InitLanguage(pLangInfo);
    SetLangID(0);
}

R::~R()
{


}
LOGFONT* R::GetFontBySize(int sz)
{
    return language_->GetFontBySize(sz);
}

LOGFONT* R::GetFont()
{
    return language_->GetFont();
}

string R::GetLangFile()
{
    return language_->getLanguageFile();
}
/*
int R::GetCurrentLanglD()
{   
    int index = 0;
    MenuConfigLua *mfl=MenuConfigLua::GetInstance();
    index = mfl->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);
    return index;
}
*/

int R::SetLangID(int pLangID)
{
    language_->setLangID(pLangID);
    StringParser::start();
    return 0;
}

int R::InitLanguage(StringIntMap pLangInfo)
{
	return  language_->initLanguage(pLangInfo);
}

string R::GetImagePath(const char *alias)
{
    if (alias == NULL || strlen(alias) == 0) {
        return "";
    }

    StringMap::iterator iter = file_map_.find(string(alias));
    if (iter != file_map_.end()) {
        if (access(iter->second.c_str(), F_OK) == 0)
            return iter->second;
    }

    db_warn("can not found resource: %s", alias);
    return string("/usr/share/minigui/res/images/null.png");
}

void R::LoadImagesPath()
{
    //string path="/usr/share/minigui/res/window/preview.qrc";
}


void R::LoadImages(string images_file)
{
    images_file_ = images_file;
    ImageParser::start();
}

void R::LoadStrings(StringMap &text_map)
{
    db_msg(" ");
    int text_map_size = text_map.size();
    int src_string_map_size = string_map_.size();
    string ctrl_name;
    int index;
    for (StringMap::iterator iter = text_map.begin(); iter != text_map.end(); iter++) {
        ctrl_name = iter->first;
        db_msg(" ctrl_name %s", ctrl_name.c_str());
        StringMap::iterator s_iter = string_map_.find(ctrl_name);
        if (s_iter != string_map_.end()) {
            db_msg(" string_map_[index] %s", s_iter->second.c_str());
            // text_map.replaceValueAt(i, string_map_[index]);
            iter->second = s_iter->second;
        }
    }
}

string R::GetImagesFile()
{
    return images_file_;
}

void R::GetStringArray(string array_name, StringVector &result)
{
    StringVectorMap::iterator iter = string_vector_map_.find(array_name);
    if (iter != string_vector_map_.end()) {
        result = iter->second;
    }
}

void R::GetString(string item_name, string &result)
{
    StringMap::iterator iter = string_map_.find(item_name);
    if (iter != string_map_.end()) {
        result = iter->second;
    }
}
