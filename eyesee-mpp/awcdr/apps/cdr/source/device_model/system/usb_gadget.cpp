/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file usb_gadget.cpp
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

#include "usb_gadget.h"
#include "device_model/system/event_manager.h"
#include "common/app_log.h"

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <pthread.h>

#include <stdlib.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

using namespace EyeseeLinux;
using namespace std;

#undef LOG_TAG
#define LOG_TAG "USBGadget"

#define clamp(val, min, max) ({                 \
        __typeof__(val) __val = (val);              \
        __typeof__(min) __min = (min);              \
        __typeof__(max) __max = (max);              \
        (void) (&__val == &__min);              \
        (void) (&__val == &__max);              \
        __val = __val < __min ? __min: __val;   \
        __val > __max ? __max: __val; })

#define ARRAY_SIZE(a)	((sizeof(a) / sizeof(a[0])))
#define MAX_IMGSIZE     (512*1000)

struct uvc_frame_info
{
    unsigned int width;
    unsigned int height;
    unsigned int intervals[8];
};

struct uvc_format_info
{
    unsigned int fcc;
    const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
    //	{ 640, 480, { 666666, 10000000, 50000000, 0 }, },
    { 1280, 720, { 333333, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
};


USBGadget::USBGadget()
    : dev_(NULL)
    ,m_thread_exit(true)
    ,m_buf_init(true)
    ,m_bIsActive(false)
{
    EventManager::GetInstance()->Attach(this);
    InitUSBGadget();
}

USBGadget::~USBGadget()
{
    EventManager::GetInstance()->Detach(this);
}

string USBGadget::CheckGadgetFunc()
{
    return "";
}

int8_t USBGadget::InitUSBGadget()
{
    return 0;
}

int8_t USBGadget::DeInitUSBGadget()
{
    return 0;
}

void USBGadget::ActiveMassStorage(const string &dev)
{
    system("echo > /sys/kernel/config/usb_gadget/g1/UDC");
    system("echo 0x100f > /sys/kernel/config/usb_gadget/g1/idProduct");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.usb0");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
    stringstream ss;
    ss << "echo " << dev << " > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/lun.0/file";
    system(ss.str().c_str());
    system("ln -s /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/ /sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.usb0");
    system("ls /sys/class/udc | xargs echo > /sys/kernel/config/usb_gadget/g1/UDC");
}

void USBGadget::DeactiveMassStorage()
{
    system("echo  > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/lun.0/file");
    system("echo 0x1002 > /sys/kernel/config/usb_gadget/g1/idProduct");
    system("ln -s /sys/kernel/config/usb_gadget/g1/functions/ffs.adb/ /sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
}

int8_t USBGadget::ActiveUVC()
{
    if( m_bIsActive )
        return 0;

    system("echo > /sys/kernel/config/usb_gadget/g1/UDC");
    system("echo 0x000e > /sys/kernel/config/usb_gadget/g1/idProduct");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.usb0");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/uvc.usb0");
    system("ln -s /sys/kernel/config/usb_gadget/g1/functions/uvc.usb0/ /sys/kernel/config/usb_gadget/g1/configs/c.1/uvc.usb0");
    system("ls /sys/class/udc | xargs echo > /sys/kernel/config/usb_gadget/g1/UDC");

    dev_ = UVCOpen();
    if (dev_ == NULL) {
        db_error("uvc open /dev/video4 failed errno %s",strerror(errno));
        return -1;
    }

    dev_->width = 1280;
    dev_->height = 720;
    dev_->bulk = 0;
    dev_->is_streaming = 0;
    dev_->imgsize = MAX_IMGSIZE;
    UVCEventsInit(dev_);

    UVCVideoInit(dev_);
    m_bIsActive = true;
    m_thread_exit = false;
    pthread_create(&thread_id,NULL,USBGadget::UVCEventsProcessThread,this);

    return 0;
}

int8_t USBGadget::DeactiveUVC()
{
    lock_guard<mutex> lock(uvc_mutex_);

    if( !m_bIsActive )
        return 0;

    if (dev_ == NULL) return 0;

    m_thread_exit = true;
    pthread_join(thread_id,NULL);
    if (dev_->is_streaming) {
        UVCVideoStream(dev_, 0);
        UVCVideoReqbufs(dev_, 0);
    }

    UVCClose(dev_);
    system("echo > /sys/kernel/config/usb_gadget/g1/UDC");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/uvc.usb0");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.usb0");
    system("ln -s /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/ /sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.usb0");
    system("rm /sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
    system("ln -s /sys/kernel/config/usb_gadget/g1/functions/ffs.adb/ /sys/kernel/config/usb_gadget/g1/configs/c.1/ffs.adb");
    system("echo 0x1002 > /sys/kernel/config/usb_gadget/g1/idProduct");
    system("ls /sys/class/udc | xargs echo > /sys/kernel/config/usb_gadget/g1/UDC");
    m_bIsActive = false;

    return 0;
}

struct uvc_device *USBGadget::UVCOpen()
{
    char dev_name[64];
    bzero(dev_name,sizeof(dev_name));
    int ret = -1;
    int fd = - 1;

    for(int i =0; i<254;i++)
    {
        snprintf(dev_name,64,"/dev/video%d",i);
        ret = access(dev_name, F_OK);
        if( ret != 0 )
        {
            continue;
        }

        fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if (fd < 0)
        {
            continue;
        }

        struct v4l2_capability cap;
        ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
        if (ret < 0)
        {
            close(fd);
            continue;
        }

        if (strcmp((char*)cap.driver, "g_uvc") == 0)
        {
            db_info("uvcvideo device node is %s\n", dev_name);
            break;
        }

        close(fd);
        fd = -1;
    }

    if( fd < 0 )
    {
        db_info("could not found uvc device\n");
        return NULL;
    }

    db_info("open succeeded, file descriptor = %d\n", fd);

    struct uvc_device *dev;
    dev = (struct uvc_device*)malloc(sizeof *dev);
    if (dev == NULL) {
        close(fd);
        return NULL;
    }
    memset(dev, 0, sizeof *dev);
    dev->fd = fd;

    return dev;
}

void USBGadget::UVCClose(struct uvc_device *dev)
{
    if(dev->fd != -1)
    {
        close(dev->fd);
        dev->fd = -1;
    }

    if( dev->mem != NULL)
    {
        free(dev->mem);
        dev->mem = NULL;
    }
    if( dev != NULL)
    {
        free(dev);
        dev = NULL;
    }
}

int USBGadget::UVCVideoInit(struct uvc_device *dev __attribute__((__unused__)))
{
    return 0;
}

void USBGadget::UVCVideoFillBuffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
    unsigned int bpl;
    unsigned int i;

    switch (dev->fcc) {
        case V4L2_PIX_FMT_YUYV:
            /* Fill the buffer with video data. */
            bpl = dev->width * 2;
            for (i = 0; i < dev->height; ++i)
                memset((uint8_t*)dev->mem[buf->index] + i*bpl, dev->color++, bpl);

            buf->bytesused = bpl * dev->height;
            break;

        case V4L2_PIX_FMT_MJPEG:
            memcpy(dev->mem[buf->index], dev->imgdata, dev->imgsize);
            buf->bytesused = dev->imgsize;
            break;
    }
}

int USBGadget::UVCVideoProcess(struct uvc_device *dev)
{
    int ret;
    struct v4l2_buffer buf;
    if( m_buf_init == false )
    {
        unsigned int i;
        for ( i = 0; i < dev->nbufs; ++i)
        {
            memset(&buf, 0, sizeof buf);
            buf.index = i;
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            buf.memory = V4L2_MEMORY_MMAP;
            UVCVideoFillBuffer(dev, &buf);
        }

        if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
            db_info("Unable to queue buffer: %s (%d).\n",
                    strerror(errno), errno);
            return ret;
        }

        m_buf_init = true;
    }

    memset(&buf, 0, sizeof buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_MMAP;

    if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0) {
        return ret;
    }

    UVCVideoFillBuffer(dev, &buf);

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
        db_info("Unable to requeue buffer: %s (%d).\n", strerror(errno),
                errno);
        db_info("dev->fd: %d\n", dev->fd);
        return ret;
    }
    return 0;
}

int USBGadget::UVCVideoStream(struct uvc_device *dev, int enable)
{
    struct v4l2_buffer buf;
    unsigned int i;
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    int ret = 0;

    if (!enable) {
        db_info("Stopping video stream.\n");
        ioctl(dev->fd, VIDIOC_STREAMOFF, &type);
        dev->is_streaming = 0;
        return 0;
    }

    db_info("Starting video stream.\n");

    ioctl(dev->fd, VIDIOC_STREAMON, &type);
    dev->is_streaming = 1;
    return ret;
}

int USBGadget::UVCVideoSetFormat(struct uvc_device *dev)
{
    struct v4l2_format fmt;
    int ret;

    db_info("Setting format to 0x%08x %ux%u\n",
            dev->fcc, dev->width, dev->height);

    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = dev->width;
    fmt.fmt.pix.height = dev->height;
    fmt.fmt.pix.pixelformat = dev->fcc;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (dev->fcc == V4L2_PIX_FMT_MJPEG)
        fmt.fmt.pix.sizeimage = dev->imgsize * 1.5;

    if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
        db_info("Unable to set format: %s (%d).\n",
                strerror(errno), errno);

    return ret;
}

int USBGadget::UVCVideoReqbufs(struct uvc_device *dev, int nbufs)
{
    struct v4l2_requestbuffers rb;
    struct v4l2_buffer buf;
    unsigned int i;
    int ret;

    for (i = 0; i < dev->nbufs; ++i) {
        if (dev->mem[i] != MAP_FAILED)
            munmap(dev->mem[i], dev->bufsize);
    }

    if (dev->mem != NULL) {
        free(dev->mem);
        dev->mem = NULL;
    }

    dev->nbufs = 0;

    memset(&rb, 0, sizeof rb);
    rb.count = nbufs;
    rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    rb.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        db_info("Unable to allocate buffers: %s (%d).\n",
                strerror(errno), errno);
        return ret;
    }

    db_info("%u buffers allocated.\n", rb.count);

    /* Map the buffers. */
    dev->mem = (void**)malloc(rb.count * sizeof dev->mem[0]);

    for (i = 0; i < rb.count; ++i) {
        memset(&buf, 0, sizeof buf);
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            db_info("Unable to query buffer %u: %s (%d).\n", i,
                    strerror(errno), errno);
            return -1;
        }
        db_info("length: %u offset: %u\n", buf.length, buf.m.offset);

        dev->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);
        if (dev->mem[i] == MAP_FAILED) {
            db_info("Unable to map buffer %u: %s (%d)\n", i,
                    strerror(errno), errno);
            return -1;
        }
        db_info("Buffer %u mapped at address %p.\n", i, dev->mem[i]);
    }

    dev->bufsize = buf.length;
    dev->nbufs = rb.count;

    m_buf_init = true;
    if( nbufs > 0)
        m_buf_init = false;

    return 0;
}

void USBGadget::UVCEventsInit(struct uvc_device *dev)
{
    struct v4l2_event_subscription sub;

    UVCFillStreamingControl(dev, &dev->probe, 0, 0);
    UVCFillStreamingControl(dev, &dev->commit, 0, 0);

    if (dev->bulk) {
        /* FIXME Crude hack, must be negotiated with the driver. */
        dev->probe.dwMaxPayloadTransferSize = 16 * 1024;
        dev->commit.dwMaxPayloadTransferSize = 16 * 1024;
    }


    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

void USBGadget::UVCEventsProcess(struct uvc_device *dev)
{
    struct v4l2_event v4l2_event;
    struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
    struct uvc_request_data resp;
    int ret;

    ret = ioctl(dev->fd, VIDIOC_DQEVENT, &v4l2_event);
    if (ret < 0) {
        db_info("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
                errno);
        return;
    }

    memset(&resp, 0, sizeof resp);
    resp.length = -EL2HLT;

    switch (v4l2_event.type) {
        case UVC_EVENT_CONNECT:
            db_info("UVC_EVENT_CONNECT\n");
            return;
        case UVC_EVENT_DISCONNECT:
            db_info("UVC_EVENT_DISCONNECT\n");
            return;

        case UVC_EVENT_SETUP:
            db_info("UVC_EVENT_SETUP\n");
            UVCEventsProcessSetup(dev, &uvc_event->req, &resp);
            break;

        case UVC_EVENT_DATA:
            db_info("UVC_EVENT_DATA\n");
            UVCEventsProcessData(dev, &uvc_event->data);
            return;

        case UVC_EVENT_STREAMON:
            db_info("UVC_EVENT_STREAMON\n");
            {
                lock_guard<mutex> lock(uvc_mutex_);
                UVCVideoReqbufs(dev, 4);
                UVCVideoStream(dev, 1);
                break;
            }
        case UVC_EVENT_STREAMOFF:
            db_info("UVC_EVENT_STREAMOFF\n");
            {
                lock_guard<mutex> lock(uvc_mutex_);
                UVCVideoStream(dev, 0);
                UVCVideoReqbufs(dev, 0);
                break;
            }
        default:
            break;
    }

    ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0) {
        db_info("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno),
                errno);
        return;
    }
}

void USBGadget::UVCEventsProcessStandard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    db_info("standard request\n");
    (void)dev;
    (void)ctrl;
    (void)resp;
}

void USBGadget::UVCEventsProcessControl(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
    db_info("control request (req %02x cs %02x)\n", req, cs);
    (void)dev;
    (void)resp;
}

void USBGadget::UVCEventsProcessStreaming(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
    struct uvc_streaming_control *ctrl;

    db_info("streaming request (req %02x cs %02x)\n", req, cs);

    if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
        return;

    ctrl = (struct uvc_streaming_control *)&resp->data;
    resp->length = sizeof *ctrl;

    switch (req) {
        case UVC_SET_CUR:
            dev->control = cs;
            resp->length = 34;
            break;

        case UVC_GET_CUR:
            if (cs == UVC_VS_PROBE_CONTROL)
                memcpy(ctrl, &dev->probe, sizeof *ctrl);
            else
                memcpy(ctrl, &dev->commit, sizeof *ctrl);
            break;

        case UVC_GET_MIN:
        case UVC_GET_MAX:
        case UVC_GET_DEF:
            UVCFillStreamingControl(dev, ctrl, req == UVC_GET_MAX ? -1 : 0,
                    req == UVC_GET_MAX ? -1 : 0);
            break;

        case UVC_GET_RES:
            memset(ctrl, 0, sizeof *ctrl);
            break;

        case UVC_GET_LEN:
            resp->data[0] = 0x00;
            resp->data[1] = 0x22;
            resp->length = 2;
            break;

        case UVC_GET_INFO:
            resp->data[0] = 0x03;
            resp->length = 1;
            break;
    }
}

void USBGadget::UVCEventsProcessClass(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
        return;

    switch (ctrl->wIndex & 0xff) {
        case UVC_INTF_CONTROL:
            UVCEventsProcessControl(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
            break;

        case UVC_INTF_STREAMING:
            UVCEventsProcessStreaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
            break;

        default:
            break;
    }
}

void USBGadget::UVCEventsProcessSetup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    dev->control = 0;

    db_info("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
            "wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
            ctrl->wValue, ctrl->wIndex, ctrl->wLength);

    switch (ctrl->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD:
            UVCEventsProcessStandard(dev, ctrl, resp);
            break;

        case USB_TYPE_CLASS:
            UVCEventsProcessClass(dev, ctrl, resp);
            break;

        default:
            break;
    }
}

void USBGadget::UVCEventsProcessData(struct uvc_device *dev, struct uvc_request_data *data)
{
    struct uvc_streaming_control *target;
    struct uvc_streaming_control *ctrl;
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    const unsigned int *interval;
    unsigned int iformat, iframe;
    unsigned int nframes;

    switch (dev->control) {
        case UVC_VS_PROBE_CONTROL:
            db_info("setting probe control, length = %d\n", data->length);
            target = &dev->probe;
            break;

        case UVC_VS_COMMIT_CONTROL:
            db_info("setting commit control, length = %d\n", data->length);
            target = &dev->commit;
            break;

        default:
            db_info("setting unknown control, length = %d\n", data->length);
            return;
    }

    ctrl = (struct uvc_streaming_control *)&data->data;
    iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U,
            (unsigned int)ARRAY_SIZE(uvc_formats));
    format = &uvc_formats[iformat-1];

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
    frame = &format->frames[iframe-1];
    interval = frame->intervals;

    while (interval[0] < ctrl->dwFrameInterval && interval[1])
        ++interval;

    target->bFormatIndex = iformat;
    target->bFrameIndex = iframe;
    switch (format->fcc) {
        case V4L2_PIX_FMT_YUYV:
            target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
            break;
        case V4L2_PIX_FMT_MJPEG:
            if (dev->imgsize == 0)
                db_info("WARNING: MJPEG requested and no image loaded.\n");
            target->dwMaxVideoFrameSize = frame->width * frame->height * 2;//dev->imgsize;
            break;
    }
    target->dwFrameInterval = *interval;

    if (dev->control == UVC_VS_COMMIT_CONTROL) {
        dev->fcc = format->fcc;
        dev->width = frame->width;
        dev->height = frame->height;

        UVCVideoSetFormat(dev);
        if (dev->bulk)
            UVCVideoStream(dev, 1);
    }
}

void USBGadget::UVCFillStreamingControl(struct uvc_device *dev, struct uvc_streaming_control *ctrl, int iframe, int iformat)
{
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    unsigned int nframes;

    if (iformat < 0)
        iformat = ARRAY_SIZE(uvc_formats) + iformat;
    if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
        return;
    format = &uvc_formats[iformat];

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    if (iframe < 0)
        iframe = nframes + iframe;
    if (iframe < 0 || iframe >= (int)nframes)
        return;
    frame = &format->frames[iframe];

    memset(ctrl, 0, sizeof *ctrl);

    ctrl->bmHint = 1;
    ctrl->bFormatIndex = iformat + 1;
    ctrl->bFrameIndex = iframe + 1;
    ctrl->dwFrameInterval = frame->intervals[0];
    switch (format->fcc) {
        case V4L2_PIX_FMT_YUYV:
            ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
            break;
        case V4L2_PIX_FMT_MJPEG:
            ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;//dev->imgsize;
            break;
    }
    ctrl->dwMaxPayloadTransferSize = 512;	/* TODO this should be filled by the driver. */
    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;
}

void* USBGadget::UVCEventsProcessThread(void *arg)
{
    USBGadget *self = (USBGadget *)arg;
    fd_set fds;

    prctl(PR_SET_NAME, "UVCEventsProcessThread", 0, 0, 0);

    FD_ZERO(&fds);
    FD_SET(self->dev_->fd, &fds);

    while (!self->m_thread_exit)
    {
        struct timeval tv;
        fd_set efds = fds;
        tv.tv_sec  = 0;
        tv.tv_usec = 300 * 1000;

        int ret = select(self->dev_->fd + 1, NULL, NULL,  &efds, &tv);
        switch(ret)
        {
            case 0:
                break;
            case -1:
                perror("select");
                break;
            default:
                if (FD_ISSET(self->dev_->fd, &efds))
                {
                    self->UVCEventsProcess(self->dev_);
                }
                break;
        }
    }
    return NULL;
}

void USBGadget::PutUVCVideoData(uint8_t *data, uint32_t size)
{
    lock_guard<mutex> lock(uvc_mutex_);
    if( size == 0 )
    {
        db_info("warining date size is 0\n");
        return ;
    }

    if (size > MAX_IMGSIZE) {
        db_info("data size[%d] is larger than max imgdata size[%d]\n", size, MAX_IMGSIZE);
        return;
    }

    if (dev_ == NULL || data == NULL)
    {
        db_info("dev[%p] data[%p]\n",dev_,data);
        return;
    }
    if (dev_->is_streaming == 1) {
        dev_->imgdata = data;
        dev_->imgsize = size;
        UVCVideoProcess(dev_);
    }
}

void USBGadget::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    switch (msg) {
        case MSG_USB_HOST_CONNECTED:
            db_msg("MSG_USB_HOST_CONNECTED");
            Notify(msg);
            break;
        case MSG_USB_HOST_DETACHED:
            db_msg("MSG_USB_HOST_DETACHED");
            Notify(msg);
            break;
        default:
            break;
    }
}
