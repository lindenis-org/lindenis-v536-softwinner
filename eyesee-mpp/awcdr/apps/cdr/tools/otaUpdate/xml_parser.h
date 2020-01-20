#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_
#include "types.h"

class XmlParser
{
public:
    XmlParser();
    virtual ~XmlParser();
    void start();
	std::string GetResourceFile();
	bool analysis();
	void GetStringArray(std::string array_name, StringVector &result);
	void GetString(std::string item_name, std::string &result);
    StringVector lines_;
    StringMap string_map_;
    StringVectorMap string_vector_map_;
};

#endif //_XML_PARSER_H_

