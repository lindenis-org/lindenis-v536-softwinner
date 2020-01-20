/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file usb_gadget.h
 * @brief 用于配置usb gadget相关功能
 *
 * 目前支持配置usb gadget为大容量存储(mass storage), 及webcam/uvc
 *
 * @author id:826
 * @date 2017-03-01
 *
 * @verbatim
    History:
   @endverbatim
 */

#pragma once

#include "common/singleton.h"
#include "common/subject.h"
#include "common/observer.h"
#include "video.h"
#include "uvc.h"

#include <linux/usb/ch9.h>
#include <linux/videodev2.h>

#include <thread>
#include <string>
#include <mutex>

#include <stdint.h>

struct uvc_device
{
    int fd;

    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;

    int control;

    unsigned int fcc;
    unsigned int width;
    unsigned int height;

    void **mem;
    unsigned int nbufs;
    unsigned int bufsize;

    unsigned int bulk;
    uint8_t color;
    unsigned int imgsize;
    void *imgdata;

    /* update for Linux-3.10 */
    int is_streaming;
};

namespace EyeseeLinux {

class USBGadget
    : public Singleton<USBGadget>
    , public IObserverWrap(USUSBGadget)
    , public ISubjectWrap(USUSBGadget)
{
        friend class Singleton<USBGadget>;

    public:
        int8_t InitUSBGadget();

        int8_t DeInitUSBGadget();

        std::string CheckGadgetFunc();

        void ActiveMassStorage(const std::string &dev);

        void DeactiveMassStorage();

        int8_t ActiveUVC();

        int8_t DeactiveUVC();

        void PutUVCVideoData(uint8_t *data, uint32_t size);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

    private:
        static void* UVCEventsProcessThread(void *arg);

        USBGadget();

        ~USBGadget();

        USBGadget &operator=(const USBGadget &o);

        struct uvc_device *UVCOpen();

        void UVCClose(struct uvc_device *dev);

        int UVCVideoInit(struct uvc_device *dev __attribute__((__unused__)));

        void UVCVideoFillBuffer(struct uvc_device *dev, struct v4l2_buffer *buf);

        int UVCVideoProcess(struct uvc_device *dev);

        int UVCVideoStream(struct uvc_device *dev, int enable);

        int UVCVideoSetFormat(struct uvc_device *dev);

        int UVCVideoReqbufs(struct uvc_device *dev, int nbufs);

        void UVCEventsInit(struct uvc_device *dev);

        void UVCEventsProcess(struct uvc_device *dev);

        void UVCEventsProcessStandard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp);

        void UVCEventsProcessControl(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp);

        void UVCEventsProcessStreaming(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp);

        void UVCEventsProcessClass(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp);

        void UVCEventsProcessSetup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp);

        void UVCEventsProcessData(struct uvc_device *dev, struct uvc_request_data *data);

        void UVCFillStreamingControl(struct uvc_device *dev, struct uvc_streaming_control *ctrl, int iframe, int iformat);

        struct uvc_device *dev_;

        std::mutex uvc_mutex_;
        pthread_t thread_id;
        bool m_thread_exit;
        bool m_buf_init;
        bool m_bIsActive;
};

}
