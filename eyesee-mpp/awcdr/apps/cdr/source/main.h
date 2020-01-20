/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file main.h
 * @brief 该文件为应用程序入口，
 *
 *  定义程序入口及完成UI初始化及业务逻辑层各控制器的创建
 *
 * @author id:520
 * @date 2015-11-24
 *
 * @verbatim
    History:
    - 2016-06-01, id:826
      单独完成UI和业务逻辑层的初始化, 单独在一个线程中完成控制器的创建
      和初始化
   @endverbatim
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include "bll_presenter/presenter.h"
#include "common/observer.h"
#include "bll_presenter/common_type.h"

#ifdef GUI_SUPPORT
#include <minigui/common.h>
#include <minigui/minigui.h>
#endif


#include <list>
#include <pthread.h>
#include <map>

class WindowManager;
class IPresenter;

int CameraPreInit();

/**
 * @brief 应用程序入口
 *
 *  实际的入口为MiniGUIAppMain，这里定义宏是为了对使用者隐藏GUI的存在
 *
 * @param args 程序入口参数个数
 * @param argv 程序入口参数列表
 * @return 成功返回0, 出错返回-1
 */

#ifdef GUI_SUPPORT
#define Main \
MiniGUIAppMain (int args, const char* argv[]); \
int main_entry (int args, const char* argv[]) \
{ \
    int ret = 0; \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    fprintf(stderr, "%s time: %lds.%ldms\n", "start minigui init", tv.tv_sec, tv.tv_usec); \
    setenv("FB_SYNC", "1", 1); \
    setenv("SCREEN_INFO", SCREEN_INFO, 1); \
    gettimeofday(&tv, NULL); \
    fprintf(stderr, "%s time: %lds.%ldms\n", "end minigui init", tv.tv_sec, tv.tv_usec); \
    ret = MiniGUIAppMain (args, argv); \
    TerminateGUI (ret); \
    return ret; \
} \
int MiniGUIAppMain
#else
#define Main main
#endif


namespace EyeseeLinux {
/**
 * @brief 主模块类
 *
 *  用于完成UI及业务逻辑层的初始化,
 *  其中UI线程定义为主线程，业务逻辑初始化放在一个工作线程，
 *  以免影响UI的加载
 */
class SampleIPCPresenter;

class MainModule : public AsyncObserverWrap(MainModule)
{
    public:
        MainModule();
        ~MainModule();

        /**
         * @brief 创建各Presenter
         */
        void CreatePresenter();

#ifdef GUI_SUPPORT
        /**
         * @brief UI初始化
         */
        void UIInit();
#endif

        /**
         * @brief device model初始化
         */
        void DeviceModelInit();

        /**
         * @brief device model去初始化
         */
        void DeviceModelDeInit();

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);
        int LoadDriver();
        /**
         * @brief 业务逻辑层初始化工作线程
         * @param context 用于传递该类this指针
         * @return NULL
         */
        static void *DeviceInitThread(void *context);

        static void *MsgHandler(void *context);

#ifndef GUI_SUPPORT
        // for test
        void StartSampleIPC();
        void ExitSampleIPC();
#endif
        void ShowPromptInfo();
        static void *thread_update(void* context);
        void setCamerMap(const CameraMap &p_CamMap);
        void setRecoderMap(const CamRecMap &p_CamRecMap);

    const CamRecMap getRecoderMap();
    const CameraMap getCamerMap();
    private:
        pthread_t init_thread_id_;      /**< 线程id */
        //pthread_t thread_id_update;

#ifdef GUI_SUPPORT
        WindowManager *window_manager_; /**< 窗口管理器对象指针 */
#endif
        std::list<IPresenter*> presenter_list_;
        SampleIPCPresenter *sample_ipc_;
		CamRecMap m_CamRecMap;
		CameraMap m_CameraMap;
};
}
#endif //_MAIN_H_
