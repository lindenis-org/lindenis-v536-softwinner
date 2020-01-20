/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file event_control_impl.cpp
 * @brief 事件控制接口
 * @author id:
 * @version v0.3
 * @date 2016-11-22
 */

#include "interface/dev_ctrl_adapter.h"
#include "event_control_impl.h"
#include "common/app_log.h"

#include <string.h>
#include <string>
#include <iostream>

#undef LOG_TAG
#define LOG_TAG "EventControlImpl"

#define EVENT_CONFIG_FILE "/tmp/data/event_config.lua"

using namespace EyeseeLinux;
using namespace std;

EventControlImpl::EventControlImpl(IPresenter *presenter)
{
    handle_id_ = 0;
    video_alarm_manager_ = VideoAlarmManager::GetInstance();
    video_alarm_manager_->Attach(this);
}

EventControlImpl::~EventControlImpl()
{
    video_alarm_manager_->Detach(this);
}

int EventControlImpl::DefaultEventConfig()
{
    int ret = 0;

    ret = video_alarm_manager_->DefaultVideoAlarmConfig();
    if (ret) {
        db_error("Do DefaultVideoAlarmConfig fail! ret:%d", ret);
        return -1;
    }

    return 0;
}

int EventControlImpl::SaveEventConfig()
{
    int ret = 0;

    ret = video_alarm_manager_->SaveVideoAlarmConfig();
    if (ret) {
        db_error("Do SaveVideoAlarmConfig fail! ret:%d", ret);
        return -1;
    }

    return 0;
}

int EventControlImpl::SetAlarmMdConfig(int vi_chn, const AWAlarmMd &alarm_md)
{
    int ret = 0;

    if (vi_chn < 0 || vi_chn > AW_MAX_CHN_NUM) {
        db_error("Input vi_chn:%d error!", vi_chn);
        return -1;
    }

    if (0 == alarm_md.md_enable) {
        ret = video_alarm_manager_->DisableMdAlarm(vi_chn);
        if (ret) {
            db_error("Do vi_chn:%d DisableMdAlarm faile! ret:%d", vi_chn, ret);
            return -1;
        }
        //return 0;
    } else {
        ret = video_alarm_manager_->EnableMdAlarm(vi_chn);
        if (ret) {
            db_error("Do vi_chn:%d EnableMdAlarm faile! ret:%d", vi_chn, ret);
            return -1;
        }
    }

    ret = video_alarm_manager_->SetMdAlarmSensitive(vi_chn, alarm_md.sensitive);
    if (ret) {
        db_error("Do vi_chn:%d SetMdAlarmSensitive %d faile! ret:%d", vi_chn, alarm_md.sensitive, ret);
        return -1;
    }

    ret = video_alarm_manager_->SetMdAlarmDelayTime(vi_chn, alarm_md.delay_time);
    if (ret) {
        db_error("Do vi_chn:%d SetMdAlarmDelayTime %d faile! ret:%d", vi_chn, alarm_md.delay_time, ret);
        return -1;
    }

    VideoDetectArea detect_area;
    memcpy(detect_area.detect_area, alarm_md.detec_area, sizeof(detect_area.detect_area));
    ret = video_alarm_manager_->SetMdAlarmArea(vi_chn, detect_area);
    if (ret) {
        db_error("Do vi_chn:%d SetMdAlarmArea faile! ret:%d", vi_chn, ret);
        return -1;
    }

    return 0;
}

int EventControlImpl::GetAlarmMdConfig(int vi_chn, AWAlarmMd &alarm_md)
{
    int ret = 0;

    if (vi_chn < 0 || vi_chn > AW_MAX_CHN_NUM) {
        db_error("Input vi_chn:%d error!", vi_chn);
        return -1;
    }

    ret = video_alarm_manager_->GetMdAlarmStatus(vi_chn, alarm_md.md_enable);
    if (ret) {
        db_error("Do vi_chn:%d GetMdAlarmStatus faile! ret:%d", vi_chn, ret);
        return -1;
    }

    ret = video_alarm_manager_->GetMdAlarmSensitive(vi_chn, alarm_md.sensitive);
    if (ret) {
        db_error("Do vi_chn:%d GetMdAlarmSensitive %d faile! ret:%d", vi_chn, alarm_md.sensitive, ret);
        return -1;
    }

    ret = video_alarm_manager_->GetMdAlarmDelayTime(vi_chn, alarm_md.delay_time);
    if (ret) {
        db_error("Do vi_chn:%d GetMdAlarmDelayTime %d faile! ret:%d", vi_chn, alarm_md.delay_time, ret);
        return -1;
    }

    VideoDetectArea detect_area;
    ret = video_alarm_manager_->GetMdAlarmArea(vi_chn, detect_area);
    if (ret) {
        db_error("Do vi_chn:%d GetMdAlarmArea faile! ret:%d", vi_chn, ret);
        return -1;
    }
    memcpy(alarm_md.detec_area, detect_area.detect_area, sizeof(detect_area.detect_area));

    return 0;
}


int EventControlImpl::SetAlarmCoverConfig(int vi_chn, const AWAlarmCover &alarm_cover)
{
    int ret = 0;

    if (vi_chn < 0 || vi_chn > AW_MAX_CHN_NUM) {
        db_error("Input vi_chn:%d error!", vi_chn);
        return -1;
    }

    if (0 == alarm_cover.cover_enable) {
        ret = video_alarm_manager_->DisableCoverAlarm(vi_chn);
        if (ret) {
            db_error("Do vi_chn:%d DisableCoverAlarm faile! ret:%d", vi_chn, ret);
            return -1;
        }
        //return 0;
    } else {
        ret = video_alarm_manager_->EnableCoverAlarm(vi_chn);
        if (ret) {
            db_error("Do vi_chn:%d EnableCoverAlarm faile! ret:%d", vi_chn, ret);
            return -1;
        }
    }

    ret = video_alarm_manager_->SetCoverAlarmSensitive(vi_chn, alarm_cover.sensitive);
    if (ret) {
        db_error("Do vi_chn:%d SetCoverAlarmSensitive %d faile! ret:%d", vi_chn, alarm_cover.sensitive, ret);
        return -1;
    }

    ret = video_alarm_manager_->SetCoverAlarmDelayTime(vi_chn, alarm_cover.delay_time);
    if (ret) {
        db_error("Do vi_chn:%d SetCoverAlarmDelayTime %d faile! ret:%d", vi_chn, alarm_cover.delay_time, ret);
        return -1;
    }

    return 0;
}


int EventControlImpl::GetAlarmCoverConfig(int vi_chn, AWAlarmCover &alarm_cover)
{
    int ret = 0;

    if (vi_chn < 0 || vi_chn > AW_MAX_CHN_NUM) {
        db_error("Input vi_chn:%d error!", vi_chn);
        return -1;
    }

    ret = video_alarm_manager_->GetCoverAlarmStatus(vi_chn, alarm_cover.cover_enable);
    if (ret) {
        db_error("Do vi_chn:%d GetCoverAlarmStatus faile! ret:%d", vi_chn, ret);
        return -1;
    }

    ret = video_alarm_manager_->GetCoverAlarmSensitive(vi_chn, alarm_cover.sensitive);
    if (ret) {
        db_error("Do vi_chn:%d GetCoverAlarmSensitive %d faile! ret:%d", vi_chn, alarm_cover.sensitive, ret);
        return -1;
    }

    ret = video_alarm_manager_->GetCoverAlarmDelayTime(vi_chn, alarm_cover.delay_time);
    if (ret) {
        db_error("Do vi_chn:%d GetCoverAlarmDelayTime %d faile! ret:%d", vi_chn, alarm_cover.delay_time, ret);
        return -1;
    }

    return 0;
}


int EventControlImpl::RegisterEventHandle(int event_type, EventHandleCallBack event_handle, void *arg)
{
    if (NULL == event_handle) {
        db_error("Input event_handle is NULL!\n");
        return -1;
    }

    if (0 == event_type) {
        db_error("Input event_type:%d is error!\n", event_type);
        return -1;
    }

    AutoMutex lock(event_lock_);

    EventHandle st_event_handle;
    ++handle_id_;
    st_event_handle.id = handle_id_;
    st_event_handle.event_type = event_type;
    st_event_handle.handle = event_handle;
    st_event_handle.arg = arg;
    handle_list_.push_back(st_event_handle);

    return st_event_handle.id;
}

int EventControlImpl::UnregisterEventHandle(int handle_id)
{
    AutoMutex lock(event_lock_);

    int idx  = 0;
    std::vector<EventHandle>::iterator it;
    for (it = handle_list_.begin(), idx = 0; it != handle_list_.end(); ++it, idx++) {
        if (handle_id == it->id) {
            break;
        }
    }

    if (it == handle_list_.end()) {
        db_error("Can't find this handle_id:%d!\n", handle_id);
        return -1;
    }

    handle_list_.erase(handle_list_.begin() + idx);

    return 0;
}


void EventControlImpl::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    AutoMutex lock(event_lock_);

    switch (msg) {
        case MSG_VIDEO_MD_ALARM:
        case MSG_VIDEO_COVER_ALARM:
            break;
        default:
            db_msg("This msg:%d don't sup\n", msg);
            return;
    }

    std::vector<EventHandle>::iterator it;
    for (it = handle_list_.begin(); it != handle_list_.end(); ++it) {
        if (MSG_VIDEO_MD_ALARM == msg &&
            0 != (it->event_type & EVENT_TYPE_MD)) {
            it->handle(0, EVENT_TYPE_MD, it->arg);
        }
        else if (MSG_VIDEO_COVER_ALARM == msg &&
            0 != (it->event_type & EVENT_TYPE_COVER)) {
            it->handle(0, EVENT_TYPE_COVER, it->arg);
        }
    }
}


int EventControlImpl::SetAlarmInputConfig(int alarm_chn)
{
    return 0;
}

int EventControlImpl::GetAlarmInputConfig(int alarm_chn)
{
    return 0;
}

int EventControlImpl::GetEventStatus(int chn, int event_type, int &status)
{
    return 0;
}
