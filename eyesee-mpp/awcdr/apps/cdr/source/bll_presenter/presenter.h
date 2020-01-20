/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file presenter.h
 * @brief presenter接口类
 * @author id:826
 * @version v0.3
 * @date 2016-06-07
 */
#pragma once

namespace EyeseeLinux {

typedef enum {
    MODEL_UNINIT = 0,
    // CAMREA_INITED,
    // RECORDER_INITED,
    MODEL_INITED,
} ModelInitStatus;

class IPresenter {
    public:
        /** 初始化与该presenter相关的device model */
        virtual int DeviceModelInit(){ return 0;};

        /** 反初始化与该presenter相关的device model */
        virtual int DeviceModelDeInit(){ return 0;};

        /** 安全退出 */
        virtual void PrepareExit(){ return ;};

        virtual int RemoteSwitchRecord(){ return 0;};
        
        virtual int RemoteSwitchRecord(int value){ return 0;};

        virtual int RemoteTakePhoto(){ return 0;};
        virtual int GetRemotePhotoInitState(bool &init_state){ return 0;};
        virtual int RemoteSwitchSlowRecord(){ return 0;};

        virtual ~IPresenter() {};
};
} /* EyeseeLinux */

