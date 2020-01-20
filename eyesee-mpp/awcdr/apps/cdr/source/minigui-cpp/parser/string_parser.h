/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: string_parser.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _STRING_PARSER_H_
#define _STRING_PARSER_H_

#include "xml_parser.h"
#include "type/types.h"

class StringParser : public XmlParser
{
public:
    StringParser();
    virtual ~StringParser();
    std::string GetResourceFile();
protected:
    virtual std::string GetStringsFile();
    virtual bool analysis();
    virtual void generate();

    /* common single string */
    StringMap string_map_;

    /* string-array */
    StringVectorMap string_vector_map_;
private:
};

#endif //_STRING_PARSER_H_
