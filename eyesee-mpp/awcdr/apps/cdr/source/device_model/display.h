#ifndef _LAYER_H_
#define _LAYER_H_

#include "vo/hwdisplay.h"
#include "common/singleton.h"
#include "common/subject.h"
#include "common/observer.h"
#include <mm_comm_vo.h>

#include <mutex>

typedef struct view_info ViewInfo;

namespace EyeseeLinux {

/** 视频通道视频显示缩放比例 */
#define VIDEO_DISPLAY_SCALING_VALUE     2

/*
 *layer id, used by app.
 */
enum LAYER_ID {
    LAYER_CAM0 = 0,
    LAYER_CAM1,
    LAYER_PLAYER,
    LAYER_UI,
    LAYER_NUM,
};
typedef enum tag_DISPLAY_DEVICE {
    DISPLAY_DEV_LCD = 0,
    DISPLAY_DEV_HDMI,
    DISPLAY_DEV_VGA,
    DISPLAY_DEV_TV,
    DISPLAY_DEV_BOTTOM,
} DISPLAY_DEVICE;


#define CLOSED 0
#define OPENED 1

#define MAX_LAYER_NUM 16

class Layer
        : public Singleton<Layer>
        , public IObserverWrap(LayerManager)
        , public ISubjectWrap(LayerManager)

{
        friend class Singleton<Layer>;

    public:
        /** create and return hlay */
        int RequestLayer(LAYER_ID index, unsigned char layer_id, ViewInfo *sur);

        /**
         * @brief 请求指定图层
         * @param channel_id 通道id
         * @param layer_id   图层id
         * @param zorder     图层Z序
         * @param sur        surface信息
         * @return 返回图层句柄， 由HLAY（channel_id, layer_id）计算得出
         *  返回-1则请求失败
         */
        int RequestLayer(LAYER_ID index, unsigned char channel_id, unsigned char layer_id, unsigned char zorder, ViewInfo *sur);

        void OpenLayer(int layer_id);

        void CloseLayer(int layer_id);

        int SetLayerRect(int layer_id, ViewInfo *sur);

        ViewInfo SetLayerRectByDispType(int layer_id, int rect_id);

        void ReleaseLayer(int layer_id);

        void CleanLayer(int layer_id);

        void SetLayerTop(int layer_id);

        void SetLayerAlpha(int layer_id, unsigned char alpha);

        int GetDisplayDeviceType(VO_INTF_TYPE_E *disp_type, VO_INTF_SYNC_E *tv_mode);

        int SwitchDisplayDevice(DISPLAY_DEVICE display_dev);

//		int SetPreviewType(CameraPreviewType p_previewType);

//		int GetPreviewType(CameraPreviewType *p_previewType);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

		int SetLayerZorder(LAYER_ID p_index, unsigned int p_zorder);

    private:
        Layer();

        ~Layer();

        Layer &operator=(const Layer &o);

        // TODO:define a Garbo class to distruct it
        // open and close layer
        void SwitchLayer(int layer_id, int on);

        int layers_[MAX_LAYER_NUM];
        int status_[MAX_LAYER_NUM];
		pthread_mutex_t m_lock;
};

} //namespace EyeseeLinux;
#endif //_LAYER_H_
