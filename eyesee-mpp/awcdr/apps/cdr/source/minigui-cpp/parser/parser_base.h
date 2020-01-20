/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: parser_base.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#ifndef _PARSER_BASE_H_
#define _PARSER_BASE_H_

class Window;

class ParserBase
{
public:
    ParserBase();
    virtual ~ParserBase();
    void SetupUi(Window* owner);
protected:
    virtual void start()=0;
    Window* owner_;
};

#endif //_PARSER_BASE_H_

