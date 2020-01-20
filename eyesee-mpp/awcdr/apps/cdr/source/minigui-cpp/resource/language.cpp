/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: language.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "debug/app_log.h"
#include "data/gui.h"
#include "utils/utils.h"
#include "resource/language.h"

#include <errno.h>

#undef LOG_TAG
#define LOG_TAG "Language"

using namespace std;

Language* Language::instance_ = NULL;
pthread_mutex_t Language::mutex_ = PTHREAD_MUTEX_INITIALIZER;

Language* Language::get()
{
    if ( Language::instance_ == NULL )
    {
        pthread_mutex_lock( &Language::mutex_ );
        if (Language::instance_ == NULL )
        {
            Language::instance_ = new Language();
        }
        pthread_mutex_unlock( &Language::mutex_ );
        }
	return Language::instance_;
}


Language::Language()
{
  mlanguage_conf.clear();

  mCurrentLangID = -1;
  mSupportLangNum = 0;

  mfont = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 26, 0);
  mfont_28 = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 28, 0);
  mfont_30 = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 30, 0);
  mfont_32 = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 32, 0);
  mfont_34 = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 34, 0);
}

Language::~Language()
{

}


string Language::getLanguageFile()
{
    map<int, std::string>::iterator it = mlanguage_conf.find(mCurrentLangID);
    if( it == mlanguage_conf.end())
    {
        db_msg("not found plangID %d\n",mCurrentLangID);
        return NULL;
    }


    return it->second;
}

int Language::setLangID(int pLangID)
{
	if( pLangID < 0 || pLangID >= mSupportLangNum )
	{
		db_error("this is a  unsupported language, setlangID failed");
		return -1;
	}

	mCurrentLangID = pLangID;
	return 0;
}

int Language::initLanguage(StringIntMap pLangInfo)
{

    for(StringIntMap::iterator iter = pLangInfo.begin();iter != pLangInfo.end();iter++)
    {
        mSupportLangNum++;
        mlanguage_conf.insert(make_pair(iter->second,iter->first));
    }

    if( mSupportLangNum == 0 )
    {
        db_msg("initLanguage failed\n");
        return -1;
    }

    return 0;
}
LOGFONT *Language::GetFontBySize(int sz)
{
	db_msg("zhb----GetFontBySize--mfont = %p  mfont_28 = %p mfont_30= %p  mfont_32 = %p  mfont_34= %p",mfont,mfont_28,mfont_30,mfont_32,mfont_34);
	if(sz == 24){
		db_msg("zhb----GetFontBySize--24");
		return mfont;
	}else if(sz == 28){
		db_msg("zhb----GetFontBySize--28");
		return mfont_28;
	}else if(sz == 30){
		db_msg("zhb----GetFontBySize--30");
		return mfont_30;
	}else if(sz == 32){
		db_msg("zhb----GetFontBySize--32 -");
		return mfont_32;
	}else if(sz == 34){
		db_msg("zhb----GetFontBySize--34");
		return mfont_34;
		}
	
	return mfont;
}

LOGFONT *Language::GetFont()
{
    db_msg(" ");
    return mfont;
}
