/*
*版权声明:暂无
*文件名称:newPreview.h
*创建者:陈振华
*创建日期:2018-5-12
*文件描述:本文件主要负责创建camera,recorder以及跟预览和录像相关的控制逻辑
*历史记录:无
*/


#ifndef __NEW_PREVIEW_H__
#define __NEW_PREVIEW_H__

#include "bll_presenter/gui_presenter_base.h"
#include "common_type.h"
#include "main.h"
#include <thread>

class PreviewWindow;
class DeviceAdapter;


namespace EyeseeLinux
{
    
    class NetManager;
	typedef enum
	{
		FRONT_BACK = 0,
		BACK_FRONT,
		ONLY_FRONT,
		ONLY_BACK,
	}CameraPreviewType;

	typedef enum {
    	CSI_ONLY = 0,
    	UVC_ONLY,
    	UVC_ON_CSI,
    	CSI_ON_UVC,
    	NO_PREVIEW
	}pipMode_t;
	class NewPreview
    : public GUIPresenterBase
    , public IPresenter
    , public ISubjectWrap(NewPreview)
    , public IObserverWrap(NewPreview)
	{
		public:
			NewPreview(MainModule *mm);
			~NewPreview();

		public:
			/*
			*名称: Camera* CreateCamera(int p_CamId)
			*功能: 跟据camId创建对应的camera对象
			*参数: 
			*	p_CamId: cameraID号 输入
			*返回值:
			*   非NULL:camera对象指针
			*   NULL:失败
			*修改: 创建2018/5/11
			*/
			Camera* CreateCamera(int p_CamId);

			/*
			*名称: int DestoryCamera(int p_CamId)
			*功能: 跟据camId销毁对应的camera对象
			*参数:
			*	p_CamId: cameraID号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int DestoryCamera(int p_CamId);

			/*
			*名称: Recorder* CreateRecorder(Camera *p_Cam)
			*功能: 创建cam所对应的recorder
			*参数:
			*	 p_CamId:  cameraID号 输入
			*    p_record_id: record id
			*返回值:
			*   非NULL:camera对应的recorder对象指针
			*   NULL:失败
			*修改: 创建2018/5/11
			*/
			Recorder* CreateRecorder(int p_CamId, int p_record_id);

			/*
			*名称: int DestoryRecorder(Camera *p_Cam)
			*功能: 销毁cam所对应的recorder
			*参数:
			*	 p_CamId:  cameraID号 输入
			*    p_record_id: record id
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int DestoryRecorder(int p_CamId, int p_record_id);

			/*
			*名称: int StartRecord(int p_CamId)
			*功能: 启动指定camera录像
			*参数: 
			*	p_CamId: 需要启动录像的camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int StartRecord(int p_CamId, int p_record_id);

			/*
			*名称: int StopRecord(int p_CamId)
			*功能: 停止指定camera录像
			*参数: 
			*	p_CamId: 需要停止录像的camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int StopRecord(int p_CamId, int p_record_id);

			/*
			*名称: int TakePicture(int p_CamId)
			*功能: 指定camera拍照
			*参数: 
			*	p_CamId:camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int TakePicture(int p_CamId, int flag);

			/*
			*名称: int SwitchDisplay()
			*功能: 双路摄像头预览画面切换
			*参数: 无
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int SwitchDisplay();
			int TransDisplay(pipMode_t mPIPMode);

						/*
			*名称: int RestoreDisplay()
			*功能: 恢复camera的默认显示(即只有前置camera，没有后置camera的情况)
			*参数: 无
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int RestoreDisplay();

			/*
			*名称: int SetCamPreviewRect(int p_CamId, int p_x, int p_y, int p_width, int p_height)
			*功能: 设置camera的显示位置
			*参数: 
			*	p_CamId: camera id号 输入
			*   p_x: X值 输入
			*   p_y: Y值  输入 
			*   p_width: 宽度  输入 
			*   p_height: 调试 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int SetCamPreviewRect(int p_CamId, int p_x, int p_y, int p_width, int p_height);

			/*
			*名称: int StartPreview(int p_CamId)
			*功能: 指定camera开始预览
			*参数: 
			*	p_CamId:camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int StartPreview(int p_CamId);

			/*
			*名称: int StopPreview(int p_CamId)
			*功能: 指定camera停止预览
			*参数:
			*	p_CamId:camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/

			int StopPreview(int p_CamId);

			/*
			*名称: int HandleGUIMessage(int p_msg, int p_val)
			*功能: 处理界面发送过来的指令
			*参数: 
			*	p_msg: 消息id号 输入
			*   p_val: 控制行为值 输入
			*   p_CamId:  一般不做用，可用来区分camId
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			
			int HandleGUIMessage(int p_msg, int p_val, int p_CamId);

			/*
			*名称: int Update(MSG_TYPE p_msg, int p_CamID=0)
			*功能: 处理model层上报上来的消息
			*参数:
			*	p_msg: 消息id号 输入
			*   p_CamID: camera id号
			*返回值: 无
			*修改: 创建2018/5/11
			*/
			void Update(MSG_TYPE p_msg, int p_CamID=0, int p_recordId = 0);

			/*
			*名称: int SetRecorderDataCallBack(int p_CamId)
			*功能: 设置指定camera数据回调
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/11
			*/
			int SetRecorderDataCallBack(int p_CamId, int p_record_id);

	        void OnWindowLoaded();

	        void OnWindowDetached();

	        void OnUILoaded();	

			void BindGUIWindow(::Window *win);
            
            int StopCamera(int camId);
			int StopCamera(int camId, int closeflag);
			int StartCamera(int camId);
			int ReStartCamera(int camId);
            void InitWindow();
			void DoChangewindowStatus(int newstatus, bool force=false);
			pipMode_t mPIPMode;
			bool playback_mode;
            bool playbackflag;
			bool isshutdownhappen_flag;

            int RemoteSwitchRecord(int value);
            int RemoteTakePhoto();
            int GetCurretRecordTime();
		private:
			/*
			*名称: bool CheckCamIdIsValid(int p_CamId)
			*功能: 判断指定camId是否合法
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   true:合法
			*   false:不合法
			*修改: 创建2018/5/11
			*/
			bool CheckCamIdIsValid(int p_CamId);

			/*
			*名称: bool CheckCameraExist(int p_CamId)
			*功能: 判断指定camera是否已经创建
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   true:成功
			*   false:失败
			*修改: 创建2018/5/11
			*/
			bool CheckCameraExist(int p_CamId);

			/*
			*名称: bool CheckRecorderExist(int p_Camid)
			*功能: 判断指定cam的recorder是否已经创建
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   true:成功
			*   false:失败
			*修改: 创建2018/5/11
			*/
			bool CheckRecorderExist(int p_CamId, int p_record_id);

			/*
			*名称: Camera *GetCamera(int p_CamId)
			*功能: 获取p_CamId对应的camera对象
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   true:成功
			*   false:失败
			*修改: 创建2018/5/11
			*/
			Camera *GetCamera(int p_CamId);

			/*
			*名称: Recorder *GetRecorder(int p_CamId)
			*功能: 获取p_CamId对应的recorder对象
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   true:成功
			*   false:失败
			*修改: 创建2018/5/11
			*/
			Recorder *GetRecorder(int p_CamId, int p_record_id);

			/*
			*名称:int TakePicforVideothumb(int p_CamId)
			*功能: 给camera拍照一张缩略图
			*参数: 
			*	p_CamId: camera id号 输入
			*返回值:
			*   0:成功
			*   -1:失败
			*修改: 创建2018/5/14
			*/
			int TakePicforVideothumb(int p_CamId, int p_record_id);
			static void LowPowerShutdownTimerHandler(union sigval sigval);
			static void MotionDetectOnOffHandle(union sigval sigval);
			void MotionDetectOnOff();
			void DoSystemShutdown();
			int WifiSOftApDisable();
			int WifiSOftApEnable();
		//	#ifdef SHOW_DEBUG_INFO
        	    static void DebugInfoThread(NewPreview *self);
		//	#endif
            int HandleSosRecord(int val);
		    bool m_sosRecordisStart;
            bool m_Impact_happen_flag;
            int m_workMode;

            #ifdef ENABLE_RTSP
            int  CreateRtspServer();
            static void *RtspThreadLoop(void *context);
            void RtspServerStart();
            void DestroyRtspServer();
                    
            void CreateRtspStreamSender();
            static void EncodeDataCallback(EncodeDataCallbackParam *param);
                    
            void SendRtspData(const VEncBuffer *frame, Recorder *rec, NewPreview *self);
            httpServer *m_httpServer_;
            DeviceAdapter *dev_adapter_; 
            NetManager *m_NetManger;            
            RtspServer *m_RtspServer;
            StreamSenderMap m_stream_sender_map;
            #endif

            

		private:
			int MediaInit();
			int MediaUnInit();
			void MediaDeInit();
		private:
             enum WorkingMode {
                    USB_MODE_CHARGE = 0,
                    USB_MODE_MASS_STORAGE, 
                };
			CameraMap m_cameraMap;
			CamRecMap m_CamRecMap;
			PreviewWindow *preview_win_;
			bool m_bOsdEnable;
			MainModule *m_MainModule;
			std::thread debug_info_thread_;
			timer_t lowpower_shutdown_timer_id_;
        	bool lowpower_shutdown_processing_;
			bool m_backCameraIsRecording;
			int m_camBShowFlag;
			//PreviewWindow *preview_win_;
			//std::thread debug_info_thread_;
			bool isRecordStart; 
            bool m_usb_attach_status;
            WorkingMode mode_;
            WindowManager *win_mg_;
			timer_t motiontimer_id_;
			bool motion_enable;
            pthread_mutex_t ahd_lock_;
            int m_camBInsertFlag;
	};
}

#endif
