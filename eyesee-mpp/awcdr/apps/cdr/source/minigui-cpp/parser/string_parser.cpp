/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: string_parser.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "parser/string_parser.h"

#include "application.h"
#include "utils.h"

#include "debug/app_log.h"
#include "resource/language.h"

using namespace std;

#undef LOG_TAG
#define LOG_TAG "StringParser"

#define LANG_PATH "/usr/share/minigui/res/lang/"

#define RESOURCE_BEGIN_FLAG ("<resources>")
#define STRING_BEGIN_FLG ("<string name=")
#define STRING_ARRAY_BEGIN_FLG ("<string-array")
#define STRING_ARRAY_END_FLG ("</string-array")
#define ARRAY_ITEM_BEGIN_FLG ("<item>")
#define RESOURCE_END_FLAG ("</resources>")

enum {
    TYPE_NULL = 0,
    TYPE_START,
    TYPE_END,
    TYPE_STRING,
    TYPE_STRING_ARRAY,
    TYPE_STRING_ARRAY_END,
    TYPE_ARRAY_ITEM,
};

StringParser::StringParser()
{
    db_msg(" ");
}

StringParser::~StringParser()
{

}

string StringParser::GetResourceFile()
{
    return GetStringsFile();

}

string StringParser::GetStringsFile()
{
	return Language::get()->getLanguageFile();
}


static int ParseType(const string& line)
{
    int pos;
    pos = line.find(STRING_BEGIN_FLG);
    if (pos != -1) {
        return TYPE_STRING;
    }
    pos = line.find(STRING_ARRAY_END_FLG);  //necessary to check end flag first
    if (pos != -1) {
        return TYPE_STRING_ARRAY_END;
    }
    pos = line.find(STRING_ARRAY_BEGIN_FLG);
    if (pos != -1) {
        return TYPE_STRING_ARRAY;
    }
    pos = line.find(ARRAY_ITEM_BEGIN_FLG);
    if (pos != -1) {
        return TYPE_ARRAY_ITEM;
    }
    pos = line.find(RESOURCE_END_FLAG);
    if (pos != -1) {
        return TYPE_END;
    }
    return TYPE_NULL;
}

static void dump(StringMap &string_map, StringVectorMap &string_vector_map)
{
    unsigned int i = 0, j = 0;
    for(StringMap::iterator iter = string_map.begin(); iter != string_map.end(); iter++) {
        string key = iter->first;
        string value = iter->second;
        db_msg("key %s , value %s", key.c_str(), value.c_str());
    }
    for(StringVectorMap::iterator s_iter = string_vector_map.begin(); s_iter != string_vector_map.end(); s_iter++) {
        string key = s_iter->first;
        StringVector keyvector = s_iter->second;
        for(StringVector::iterator k_iter = keyvector.begin(); k_iter != keyvector.end(); k_iter++) {
            string value = *k_iter;
            db_msg("key %s , value%d %s", key.c_str(), j, value.c_str());
        }
    }
}

bool StringParser::analysis()
{
    StringVector::iterator it = lines_.begin();
    db_msg("%s ", it->c_str());
    it++;
    db_msg("%s ", it->c_str());
    int pos = it->find(RESOURCE_BEGIN_FLAG);
    if (pos == -1) {
        return false;
    }
    db_msg("%s ", it->c_str());
    it += 2;
    string result_id;
    string result_text;
    StringVector string_vector;
	if( string_map_.size() != 0 )		
		string_map_.clear();

	if( string_vector_map_.size() != 0)
		string_vector_map_.clear();
	
    for (it = lines_.begin(); it != lines_.end(); it++) {
        db_msg("l %s ", it->c_str());
        switch(ParseType(*it)) {
            case TYPE_END:
                break;
            case TYPE_STRING:
                TrimString(it->c_str(), "\"", result_id);
                TrimString2(it->c_str(), ">", "<", result_text);
                string_map_.insert(make_pair(result_id, result_text));
                break;
            case TYPE_STRING_ARRAY:
                TrimString(it->c_str(), "\"", result_id);
                break;
            case TYPE_STRING_ARRAY_END:
                string_vector_map_.insert(make_pair(result_id, string_vector));
                string_vector.clear();
                break;
            case TYPE_ARRAY_ITEM:
                TrimString2(it->c_str(), ">", "<", result_text);
                string_vector.push_back(result_text);
                break;
        }
    }
    //dump(string_map_, string_vector_map_);
    return true;
}

void StringParser::generate()
{

}


