/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: xml_parser.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "parser/xml_parser.h"
#include "utils.h"

#include "debug/app_log.h"

#undef LOG_TAG
#define LOG_TAG "XmlParser"

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
        generate();
    }
}

bool XmlParser::analysis()
{
    return true;
}

void XmlParser::generate()
{
    return;
}

