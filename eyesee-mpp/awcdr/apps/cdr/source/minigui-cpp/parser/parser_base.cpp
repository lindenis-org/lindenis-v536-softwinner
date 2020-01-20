/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: parser_base.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "parser/parser_base.h"
#include "window/window.h"
#include "debug/app_log.h"


#undef LOG_TAG
#define LOG_TAG "ParserBase"

ParserBase::ParserBase()
{
    owner_ = NULL;
}

ParserBase::~ParserBase()
{

}

void ParserBase::SetupUi(Window* owner)
{
    db_msg("owner %p", owner);
    owner_ = owner;
    start();
}

