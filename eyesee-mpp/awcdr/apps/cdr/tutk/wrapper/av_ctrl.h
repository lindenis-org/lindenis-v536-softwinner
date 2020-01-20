/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file av_ctrl.h
 * @brief tutk iot/av ctrl cmd handler
 * @author id:826
 * @version v0.3
 * @date 2016-08-26
 */

#pragma once

#include <list>
#include "AVIOCTRLDEFs.h"


class DeviceAdapter;

namespace tutk {

typedef struct tag_EventHandleInfo {
    int av_index;
    int sid;
    int handle_id;
}EventHandleInfo;

    class AVCtrl {
        public:
            AVCtrl(DeviceAdapter *adapter);
            ~AVCtrl();

            void SetAdapter(DeviceAdapter *adapter);

            int HandleIOTCtrlCmd(int sid, int av_index, char *buf, int type);

            int SnapShotHandler(int sid, int av_index, char *buf, int type);

            int RegisterEventMsg(int sid, int av_index);

            int UnRegisterEventMsg(int sid, int av_index);

            static void EventHandleCallBack(int chn, int event_type, void *arg);

            void dumpMenuConfig(SMsgAVIoctrSDVDevConfigResp &resp);
            int SendCmdData(int type,int chn,int value , void *data);
            int SetClientConnectStatus(bool p_Connect);
        private:
            DeviceAdapter *adapter_;
            std::list<EventHandleInfo> handle_info_list_;
    };

}
