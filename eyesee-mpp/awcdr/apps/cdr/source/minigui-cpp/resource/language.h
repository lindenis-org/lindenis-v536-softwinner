/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: language.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

#include "type/types.h"

typedef struct LanguageInfo_
{
    int id;
    std::string path;
}LanguageInfo;

class Language
{
public:
    Language();
    ~Language();
    static Language* get();
    LOGFONT *GetFont();
    LOGFONT *GetFontBySize(int sz);
    std::string getLanguageFile();
    int setLangID(int pLangID);
    int initLanguage(StringIntMap pLangInfo);

private:

    std::map<int, std::string> mlanguage_conf;

    LOGFONT *mfont,*mfont_28,*mfont_30,*mfont_32,*mfont_34;

    int mCurrentLangID;

    static pthread_mutex_t mutex_;
    static Language* instance_;
    int mSupportLangNum;
};
#endif //_LANGUAGE_H_
