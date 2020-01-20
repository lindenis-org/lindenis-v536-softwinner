/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file storage_manager.cpp
 * @brief 存储管理类源文件
 * @author id:826
 * @version v0.3
 * @date 2015-11-02
 * @verbatim
    History:
    - 2016-06-07, change for v40 ipc demo
   @endverbatim
 */
 //#define NDEBUG
#include "device_model/dialog_status_manager.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "common/thread.h"
#include <sys/resource.h>
#include <sys/mount.h>
#include <string.h>
#include <dirent.h>
#include <sstream>


#undef LOG_TAG
#define LOG_TAG "DialogStatusManager"

using namespace EyeseeLinux;
using namespace std;

DialogStatusManager::DialogStatusManager()
	:m_dialog_event_finish(true)
{

}

DialogStatusManager::~DialogStatusManager()
{

}

