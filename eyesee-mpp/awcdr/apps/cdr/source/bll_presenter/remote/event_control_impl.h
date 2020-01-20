/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file event_control_impl.h
 * @brief 事件控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-22
 */
#pragma once

#include "common/observer.h"
#include "common/message.h"

#include "interface/dev_ctrl_adapter.h"
#include "device_model/media/video_alarm_manager.h"
#include "lua/lua_config_parser.h"
#include "lua/lua.hpp"

#include <utils/Mutex.h>
#include <vector>


namespace EyeseeLinux
{


typedef struct tag_EventHandle {
    int   id;
    int   event_type;
    void *arg;
    EventHandleCallBack handle;
} EventHandle;


/**
 * @brief Record控制接口
 */
class EventControlImpl
    : public DeviceAdapter::EventControl
    , public IObserverWrap(EventControlImpl)
{
    public:
        EventControlImpl(IPresenter *presenter);

        ~EventControlImpl();

        int SetAlarmMdConfig(int vi_chn, const AWAlarmMd &alarm_md);

        int GetAlarmMdConfig(int vi_chn, AWAlarmMd &alarm_md);

        int SetAlarmCoverConfig(int vi_chn, const AWAlarmCover &alarm_cover);

        int GetAlarmCoverConfig(int vi_chn, AWAlarmCover &alarm_cover);

        int SetAlarmInputConfig(int alarm_chn);

        int GetAlarmInputConfig(int alarm_chn);

        int GetEventStatus(int chn, int event_type, int &status);

        int RegisterEventHandle(int event_type, EventHandleCallBack event_handle, void *arg);

        int UnregisterEventHandle(int handle_id);

        int SaveEventConfig(void);

        int DefaultEventConfig(void);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    private:
        int handle_id_;
        VideoAlarmManager *video_alarm_manager_;
        std::vector<EventHandle> handle_list_;
        Mutex event_lock_;
};

} // namespace EyeseeLinux
