#include "xml_parser.h"
#include "utils.h"

#include "app_log.h"

#undef LOG_TAG
#define LOG_TAG "XmlParser"
#define LANG_PATH "/usr/share/minigui/res/lang/zh-EN.xml"

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
using namespace std;

XmlParser::XmlParser()
{

}

XmlParser::~XmlParser()
{

}

void XmlParser::start()
{
    db_msg("XmlParser @@@@@");
    FILE *fp;
    int len = 256;
    char line[256] = {0};

    string file = GetResourceFile();
    db_msg("file %s", file.c_str());
    if (!FileExists(file.c_str())) {
        db_error("could not find file %s", file.c_str());
        return;
    }

    fp = fopen(file.c_str(), "r");

	if(lines_.size() != 0) {
			lines_.clear();
		}

    while (1) {
        fgets(line, len, fp);
        if ( (feof(fp) != 0) || (ferror(fp) != 0) ) break;
        lines_.push_back(string(line));
    }
    fclose(fp);
    db_msg(" ");
    if (analysis()) {
        db_msg(" ");
    }
}
string XmlParser::GetResourceFile()
{
    return LANG_PATH;

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

bool XmlParser::analysis()
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
       // db_msg("l %s ", it->c_str());
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
    return true;
}
void XmlParser::GetStringArray(string array_name, StringVector &result)
{
    StringVectorMap::iterator iter = string_vector_map_.find(array_name);
    if (iter != string_vector_map_.end()) {
        result = iter->second;
    }
}

void XmlParser::GetString(string item_name, string &result)
{
    StringMap::iterator iter = string_map_.find(item_name);
    if (iter != string_map_.end()) {
        result = iter->second;
    }
}
