/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: images_parser.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _IMAGE_PARSER_H_
#define _IMAGE_PARSER_H_

#include "xml_parser.h"
#include "type/types.h"

class ImageParser : public XmlParser
{
public:
    ImageParser();
    virtual ~ImageParser();
    std::string GetResourceFile();
    virtual std::string GetImagesFile();
protected:
    virtual bool analysis();
    virtual void generate();
    StringMap file_map_;
private:
    std::string prefix_;
};

#endif //_IMAGE_PARSER_H_
