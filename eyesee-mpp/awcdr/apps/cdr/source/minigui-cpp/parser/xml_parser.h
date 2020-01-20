/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: xml_parser.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_
#include "parser_base.h"
#include "type/types.h"
#include "window/window.h"

typedef struct {
    std::string class_name;
    std::string ctrl_name;
    int x;
    int y;
    int w;
    int h;
    int level;          //view's level
    View *obj;          //class pointer
    std::string text;
    bool end;           //end flag
    int parent_idx;
}ObjectInfo;

typedef std::vector<ObjectInfo> ObjectVector;

class XmlParser : public ParserBase
{
public:
    ObjectVector objects_;
    XmlParser();
    virtual ~XmlParser();
protected:
    virtual void start();
    virtual bool analysis();
    virtual void generate();
    virtual std::string GetResourceFile() = 0;
    StringVector lines_;
};

#endif //_XML_PARSER_H_
