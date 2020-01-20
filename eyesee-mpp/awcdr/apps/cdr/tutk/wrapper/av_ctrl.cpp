/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file av_ctrl.cpp
 * @brief tutk iot/av ctrl cmd handler
 * @author id:826
 * @version v0.3
 * @date 2016-08-26
 */

#include "av_ctrl.h"
#include "av_server.h"

#include <tutk/IOTCAPIs.h>
#include <tutk/AVAPIs.h>

#include "interface/dev_ctrl_adapter.h"
#include "interface/remote_connector.h"
#include "device_model/system/led.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

using namespace tutk;
using namespace EyeseeLinux;

AVCtrl::AVCtrl(DeviceAdapter *adapter)
    : adapter_(adapter)
{
}

AVCtrl::~AVCtrl()
{
}

void AVCtrl::SetAdapter(DeviceAdapter *adapter)
{
    adapter_ = adapter;
}

int AVCtrl::RegisterEventMsg(int sid, int av_index)
{
    EventHandleInfo info;
    info.av_index = av_index;
    info.sid = sid;
    info.handle_id = adapter_->event_ctrl_->RegisterEventHandle(EVENT_TYPE_MD|EVENT_TYPE_COVER,
                                                                AVCtrl::EventHandleCallBack, this);
    if (info.handle_id < 0) {
        printf("[FUN]:%s [LINE]:%d  Do RegisterEventMsg  sid:%d av_index:%d  error:0x%x!\n",
                                            __func__, __LINE__, sid, av_index, info.handle_id);
        return -1;
    }

    handle_info_list_.push_back(info);
    return 0;
}


int AVCtrl::UnRegisterEventMsg(int sid, int av_index)
{
    int ret = 0, handle_id = -1;
    std::list<EventHandleInfo>::iterator iter;
    for (iter = handle_info_list_.begin(); iter != handle_info_list_.end(); iter++) {
        if (iter->sid == sid && iter->sid == av_index) {
            handle_id = iter->handle_id;
            break;
        }
    }

    ret = adapter_->event_ctrl_->UnregisterEventHandle(handle_id);
    if (ret) {
        printf("[FUN]:%s [LINE]:%d  Do UnregisterEventHandle sid:%d  av_index:%d  handle_id:%d!\n",
                                            __func__, __LINE__, sid, av_index, handle_id);
        return -1;
    }

    handle_info_list_.erase(iter);

    return 0;
}

void AVCtrl::EventHandleCallBack(int chn, int event_type, void * arg)
{
    int ret = 0, av_index = 0;
    SMsgAVIoctrlAwSetChannelRESP resp;
    std::list<EventHandleInfo>::iterator iter;
    AVCtrl *av_ctrl = reinterpret_cast<AVCtrl*>(arg);

    resp.channel = chn;
    resp.value   = event_type;
    for (iter = av_ctrl->handle_info_list_.begin(); iter != av_ctrl->handle_info_list_.end(); iter++) {

        av_index = iter->av_index;
        if (av_index < 0 || av_index > 1000) {
            continue;
        }

        if (0 != (EVENT_TYPE_MD & event_type)) {
            ret = avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_TRIGGER_MD_ALARM, (char *) &resp,
                         sizeof(SMsgAVIoctrlAwSetChannelRESP));
            if (ret) {
                printf("[FUN]:%s [LINE]:%d  Do avSendIOCtrl av_index:%d  error:0x%x!\n",
                                                __func__, __LINE__, iter->av_index, ret);
                continue;
            }
        }

        if (0 != (EVENT_TYPE_COVER & event_type)) {
            ret = avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_TRIGGER_COVER_ALARM, (char *) &resp,
                         sizeof(SMsgAVIoctrlAwSetChannelRESP));
            if (ret) {
                printf("[FUN]:%s [LINE]:%d  Do avSendIOCtrl av_index:%d  error:0x%x!\n",
                                                __func__, __LINE__, iter->av_index, ret);
                continue;
            }
        }
    }
}


int AVCtrl::HandleIOTCtrlCmd(int sid, int av_index, char *buf, int type)
{
    printf("[FUN]:%s  [LINE]:%d  ===>>  sid:%d  av_index:%d  type:0x%x \n",__func__,__LINE__, sid, av_index, type);
    switch (type) {
        case IOTYPE_USER_IPCAM_GET_DEVICE_CONFIG_REQ: {
            SMsgAVIoctrDevConfigResp resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = p->channel;

            // TODO: get config from dev adapter api.
            /* channel info */
            resp.channel = 0;
            resp.channel_num = AW_MAX_CHN_NUM;

            /* media configs */
            AWEnCodeConfig media_cfg;
            ret = adapter_->media_ctrl_->GetMediaConfig(media_cfg);
            if (ret) {
                printf("Do GetMediaConfig fail! error:%d\n", ret);
            }
            resp.decodetype     = media_cfg.encode_format;
            resp.streamsuptype  = media_cfg.encode_type;
            resp.streamquality  = media_cfg.quality;
            resp.framerate      = media_cfg.fps[0] - 1;
            resp.Iframeinterval = media_cfg.gop[0] - 1;
            switch (media_cfg.encode_size[0].height) {
                case 1080:
                default:
                    resp.streamtype = 0;
                    break;
                case 720:
                    resp.streamtype = 1;
                    break;
                case 576:
                    resp.streamtype = 2;
                    break;
                case 288:
                    resp.streamtype = 3;
                    break;
            }
            if (media_cfg.bps[0] > 0 && media_cfg.bps[0] <= 1024*512) {
                resp.streamrate = 0;
            } else if (media_cfg.bps[0] > 1024*512 && media_cfg.bps[0] <= 1024*1024) {
                resp.streamrate = 1;
            } else if (media_cfg.bps[0] > 1024*1024 && media_cfg.bps[0] <= 1024*1024*2) {
                resp.streamrate = 2;
            } else if (media_cfg.bps[0] > 1024*1024*2 && media_cfg.bps[0] <= 1024*1024*5) {
                resp.streamrate = 3;
            } else if (media_cfg.bps[0] > 1024*1024*5) {
                resp.streamrate = 4;
            }

            /* overlay configs */
            AWTimeOsd time_osd;
            ret = adapter_->overlay_ctrl_->GetTimeOsd(0, time_osd);
            if (ret) {
                printf("Do GetTimeOsd fail! error:%d\n", ret);
            }
            resp.time_osd_enable = time_osd.osd_enable;
            resp.time_format = time_osd.time_format;
            resp.date_format = time_osd.date_format;

            resp.video_cover = 0;
            AWCoverOsd cover_osd;
            for (int cnt = 0; cnt < 4; cnt++) {
                adapter_->overlay_ctrl_->GetCoverOsd(p->channel, cnt, cover_osd);
                if (cover_osd.osd_enable) {
                    resp.video_cover |= (0x1<<cnt);
                }
            }

            AWChannelOsd chn_osd;
            ret = adapter_->overlay_ctrl_->GetChannelOsd(p->channel, 0, chn_osd);
            if (ret) {
                printf("Do GetTimeOsd fail! error:%d\n", ret);
            }
            resp.show_chn_enable = chn_osd.osd_enable;
            strncpy(resp.chn_name, chn_osd.channel_name, sizeof(resp.chn_name) -1);

            AWDeviceOsd device_osd;
            ret = adapter_->overlay_ctrl_->GetDeviceOsd(0, device_osd);
            if (ret) {
                printf("Do GetTimeOsd fail! error:%d\n", ret);
            }
            resp.show_dev_enable = device_osd.osd_enable;
            strncpy(resp.dev_name, device_osd.device_name, sizeof(resp.dev_name) -1);

            /* Image configs */
            AWImageColour image_colour;
            ret = adapter_->image_ctrl_->GetImageColour(p->channel, image_colour);
            if (ret) {
                printf("Do GetImageColour fail! error:%d\n", ret);
            }
            resp.brightness = image_colour.brightness;
            resp.contrast = image_colour.contrast;
            resp.saturation = image_colour.saturation;
            resp.sharpness = image_colour.sharpness;
            resp.hue = image_colour.hue;

            int flip = 0, mirror = 0;
            ret = adapter_->image_ctrl_->GetImageFlip(p->channel, flip);
            if (ret) {
                printf("Do GetImageFlip fail! error:%d\n", ret);
            }
            ret = adapter_->image_ctrl_->GetImageMirror(p->channel, mirror);
            if (ret) {
                printf("Do GetImageMirror fail! error:%d\n", ret);
            }
            resp.flip = flip + mirror*2;

            ret = adapter_->image_ctrl_->GetImageWhiteBalance(p->channel, resp.white_balance);
            if (ret) {
                printf("Do GetImageWhiteBalance fail! error:%d\n", ret);
            }

            ret = adapter_->image_ctrl_->GetImageExposure(p->channel, resp.exposure_type, resp.exposure_time);
            if (ret) {
                printf("Do GetImageExposure fail! error:%d\n", ret);
            }

            ret = adapter_->image_ctrl_->GetImageWideDynamic(p->channel, resp.wide_dynamic);
            if (ret) {
                printf("Do GetImageWideDynamic fail! error:%d\n", ret);
            }

            ret = adapter_->image_ctrl_->GetImageFlickerFreq(p->channel, resp.flicker_freq);
            if (ret) {
                printf("Do GetImageFlickerFreq fail! error:%d\n", ret);
            }

            ret = adapter_->image_ctrl_->GetImageDenoise(p->channel, resp.denoise);
            if (ret) {
                printf("Do GetImageDenoise fail! error:%d\n", ret);
            }

            AWDayNightMode mode;
            ret = adapter_->image_ctrl_->GetImageDayNightMode(p->channel, mode);
            if (ret) {
                printf("Do GetImageDayNightMode fail! error:%d\n", ret);
            }
            resp.day_night_mode = mode.day_night_mode;

            /* system config */
            adapter_->dev_ctrl_->GetDeviceId((int *)&resp.device_id);
            strncpy(resp.software_version, "V0.0.3", sizeof(resp.software_version) -1);
            strncpy(resp.hardware_version, "V40_DVB_DDR3", sizeof(resp.hardware_version) -1);

            /* storage */
            AWDiskInfo disk_info;
            ret = adapter_->dev_ctrl_->GetDiskInfo(disk_info);
            if (ret) {
                printf("Do GetDiskInfo fail! error:%d\n", ret);
            }
            resp.capacity    = disk_info.disk_status[0].capacity;
            resp.free_space  = disk_info.disk_status[0].free_space;
            resp.disk_status = disk_info.disk_status[0].disk_status;

            /* alarm config */
            AWAlarmCover alarm_cover;
            AWAlarmMd    alarm_md;
            ret = adapter_->event_ctrl_->GetAlarmMdConfig(p->channel, alarm_md);
            if (ret) {
                printf("Do chn:%d GetAlarmMdConfig fail! error:%d\n", p->channel, ret);
            }
            ret = adapter_->event_ctrl_->GetAlarmCoverConfig(p->channel, alarm_cover);
            if (ret) {
                printf("Do chn:%d GetAlarmCoverConfig fail! error:%d\n", p->channel, ret);
            }
            resp.alarm_md_enable       = alarm_md.md_enable;
            resp.alarm_md_sensitive    = alarm_md.sensitive;
            resp.alarm_cover_enable    = alarm_cover.cover_enable;
            resp.alarm_cover_sensitive = alarm_cover.sensitive;

            /* network config */
            AWNetAttr net_attr;
            adapter_->sys_ctrl_->GetNetworkAttr(net_attr);
            resp.net_type = 0;                 // val: 0-use ethernet, 1-use wifi station, 2-use wifi ap
            resp.dhcp = net_attr.dhcp_enable;  // val: 0-disable DHCP, 1-enable DHCP

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_GET_DEVICE_CONFIG_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrDevConfigResp)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SETSTREAMCTRL_REQ: {
            SMsgAVIoctrlSetStreamCtrlReq *p = (SMsgAVIoctrlSetStreamCtrlReq *) buf;
            printf("IOTYPE_USER_IPCAM_SETSTREAMCTRL_REQ, ch: %d, avIndex: %d, quality: %d\n",
                   p->channel, av_index, p->quality);

            unsigned int w, h, f, b;
            switch (p->quality) {
                case AVIOCTRL_QUALITY_MAX: // 1080P
                    w = 1920; h = 1080; f = 15; b = (4 << 20); break;
                case AVIOCTRL_QUALITY_HIGH: // 720P
                    w = 1080; h = 720; f = 25; b = (2 << 20); break;
                case AVIOCTRL_QUALITY_MIDDLE: // D1
                    w = 720; h = 576; f = 25; b = (2 << 20); break;
                case AVIOCTRL_QUALITY_LOW: // VGA
                    w = 640; h = 480; f = 30; b = (1 << 19); break;
                case AVIOCTRL_QUALITY_MIN:
                    w = 480; h = 360; f = 30; b = (1 << 19); break;
                case AVIOCTRL_QUALITY_UNKNOWN:
                default: break;
            }

            int ret = adapter_->media_ctrl_->SetVideoQuality(CTRL_TYPE_TUTK, 0, w, h, b, f);

            SMsgAVIoctrlSetStreamCtrlResp rp;
            rp.result = ret;
            avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SETSTREAMCTRL_RESP, (char *)&rp, sizeof(SMsgAVIoctrlPlayRecordResp));
        }
        break;
        case IOTYPE_USER_IPCAM_SPEAKERSTART: {
            SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *) buf;
        }
        break;
        case IOTYPE_USER_IPCAM_SPEAKERSTOP: {
            printf("IOTYPE_USER_IPCAM_SPEAKERSTOP\n\n");
        }
        break;
        case IOTYPE_USER_IPCAM_LISTEVENT_REQ: {
            printf("IOTYPE_USER_IPCAM_LISTEVENT_REQ\n\n");
            /*
                SMsgAVIoctrlListEventReq *p = (SMsgAVIoctrlListEventReq *)buf;
                if(p->event == AVIOCTRL_EVENT_MOTIONDECT)
                Handle search event(motion) list
                from p->stStartTime to p->stEndTime
                and get list to respond to App
            */
#if 0
            //SendPushMessage(AVIOCTRL_EVENT_MOTIONDECT);

            if (gbSearchEvent == 0) //sample code just do search event list once, actually must renew when got this cmd
            {
                gEventList = (SMsgAVIoctrlListEventResp *) malloc(
                        sizeof(SMsgAVIoctrlListEventResp) + sizeof(SAvEvent) * 3);
                memset(gEventList, 0, sizeof(SMsgAVIoctrlListEventResp));
                gEventList->total = 1;
                gEventList->index = 0;
                gEventList->endflag = 1;
                gEventList->count = 3;
                int i;
                for (i = 0; i < gEventList->count; i++) {
                    gEventList->stEvent[i].stTime.year = 2012;
                    gEventList->stEvent[i].stTime.month = 6;
                    gEventList->stEvent[i].stTime.day = 20;
                    gEventList->stEvent[i].stTime.wday = 5;
                    gEventList->stEvent[i].stTime.hour = 11;
                    gEventList->stEvent[i].stTime.minute = i;
                    gEventList->stEvent[i].stTime.second = 0;
                    gEventList->stEvent[i].event = AVIOCTRL_EVENT_MOTIONDECT;
                    gEventList->stEvent[i].status = 0;
                }
                gbSearchEvent = 1;
            }
            avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_LISTEVENT_RESP, (char *) gEventList,
                         sizeof(SMsgAVIoctrlListEventResp) + sizeof(SAvEvent) * gEventList->count);
#endif
        }
            break;
        case IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL: {
            SMsgAVIoctrlPlayRecord *p = (SMsgAVIoctrlPlayRecord *) buf;
            SMsgAVIoctrlPlayRecordResp res;
            printf("IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL cmd[%d]\n\n", p->command);
            if (p->command == AVIOCTRL_RECORD_PLAY_START) {
                res.command = AVIOCTRL_RECORD_PLAY_START;
                res.result = IOTC_Session_Get_Free_Channel(sid);
                printf("Sending res [%d]\n", res.result);
                if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL_RESP, (char *) &res,
                                 sizeof(SMsgAVIoctrlPlayRecordResp)) < 0)
                    break;
            } else if (p->command == AVIOCTRL_RECORD_PLAY_PAUSE) {
                res.command = AVIOCTRL_RECORD_PLAY_PAUSE;
                res.result = 0;
                if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL_RESP, (char *) &res,
                                 sizeof(SMsgAVIoctrlPlayRecordResp)) < 0)
                    break;
            } else if (p->command == AVIOCTRL_RECORD_PLAY_STOP) {
                break;
            }
        }
            break;

        /* Media config */
        /* setting encode reslution: 0-1080P, 1-720P */
        case IOTYPE_USER_IPCAM_SET_DECODETYPE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s [LINE]:%d  =====> type:0x%X  val:%d\n", __func__, __LINE__, type, p->value);

            int width = 0, height = 0;
            switch (p->value) {
                case 0:
                default:
                    width  = 1920;
                    height = 1080;
                    break;
                case 1:
                    width  = 1280;
                    height = 720;
                    break;
            }
            resp.value = adapter_->media_ctrl_->SetVencSize(CTRL_TYPE_TUTK, 0, width, height);
            if (0 == resp.value) {
                adapter_->media_ctrl_->SaveMediaConfig();
            }

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DECODETYPE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting stream type: 0-CVR, 1-VBR */
        case IOTYPE_USER_IPCAM_SET_STREAMTYPE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            resp.channel = p->channel;
            resp.value = adapter_->media_ctrl_->SetVencType(CTRL_TYPE_TUTK, 0, p->value);
            adapter_->media_ctrl_->SaveMediaConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_STREAMTYPE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting frame rate for encode. */
        case IOTYPE_USER_IPCAM_SET_FRAMERATE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            resp.channel = p->channel;
            resp.value = adapter_->media_ctrl_->SetVencFrameRate(CTRL_TYPE_TUTK, 0, p->value+1);
            adapter_->media_ctrl_->SaveMediaConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_FRAMERATE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting stream bit rate for encode. */
        case IOTYPE_USER_IPCAM_SET_STREAMRATE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;

            int bitrate = 0;
            resp.channel = p->channel;
            switch (p->value) {
                case 0:
                    bitrate = 1024*512;
                    break;
                case 1:
                    bitrate = 1024*1024;
                    break;
                case 2:
                    bitrate = 1024*1024*2;
                    break;
                case 3:
                default:
                    bitrate = 1024*1024*5;
                    break;
                case 4:
                    bitrate = 1024*1024*8;
                    break;
            }
            resp.value = adapter_->media_ctrl_->SetVencBitRate(CTRL_TYPE_TUTK, 0, bitrate);
            adapter_->media_ctrl_->SaveMediaConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_STREAMRATE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting encode format: 0-H264, 1-H265 */
        case IOTYPE_USER_IPCAM_SET_STREAMSUPTYPE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            resp.channel = p->channel;
            resp.value = adapter_->media_ctrl_->SetVencFormat(CTRL_TYPE_TUTK, 0, p->value);
            adapter_->media_ctrl_->SaveMediaConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_STREAMSUPTYPE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting encode quality: 0~2, 0:best, 1~better, 2~good */
        case IOTYPE_USER_IPCAM_SET_STREAMQUALITY_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s [LINE]:%d  =====> type:0x%X  val:%d\n", __func__, __LINE__, type, p->value);
            
            resp.channel = p->channel;
            resp.value = adapter_->media_ctrl_->SetVencQuality(CTRL_TYPE_TUTK, 0, p->value);
            adapter_->media_ctrl_->SaveMediaConfig();
            
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_STREAMQUALITY_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* setting encode gop.*/
        case IOTYPE_USER_IPCAM_SET_IFRAMEINTERVAL_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            resp.channel = p->channel;
            resp.value = adapter_->media_ctrl_->SetVencGop(CTRL_TYPE_TUTK, 0, p->value+1);
            adapter_->media_ctrl_->SaveMediaConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_IFRAMEINTERVAL_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* Network config */
        case IOTYPE_USER_IPCAM_SET_DHCP_REQ: {
            int ret = 0;
            AWNetAttr net_attr;
            memset(&net_attr, 0, sizeof(net_attr));
            strcpy(net_attr.interface, "eth0");
            net_attr.dhcp_enable = 1;
            ret = adapter_->sys_ctrl_->SetNetworkAttr(net_attr);
            if (ret) {
                printf("[FUN]:%s [LINE]:%d  Do SetNetworkAttr fail!\n", __func__, __LINE__);
            }
        }
            break;
        case IOTYPE_USER_IPCAM_SET_IPADDR_REQ: {
            SMsgAVIoctrlSetIPAddrReq *p = (SMsgAVIoctrlSetIPAddrReq *)buf;

            AWNetAttr net_attr, last_net_attr;
            memset(&net_attr, 0, sizeof(net_attr));
            memset(&last_net_attr, 0, sizeof(last_net_attr));

            strcpy(net_attr.interface, "eth0");
            strncpy(net_attr.mac,     p->macaddr,    sizeof(p->macaddr) - 1);
            strncpy(net_attr.ip,      p->ipaddr,     sizeof(p->ipaddr) - 1);
            strncpy(net_attr.gateway, p->netaddr,    sizeof(p->netaddr) - 1);
            strncpy(net_attr.mask,    p->subnetmark, sizeof(p->subnetmark) - 1);
            strncpy(net_attr.dns1,    p->firstdns,   sizeof(p->firstdns) - 1);
            strncpy(net_attr.dns2,    p->backupdns,  sizeof(p->backupdns) - 1);
            net_attr.dhcp_enable = 0;

            strcpy(last_net_attr.interface, net_attr.interface);
            int result = adapter_->sys_ctrl_->GetNetworkAttr(last_net_attr);
            if (result < 0) {
            	printf("get network attribute failed! interface parameter: %s\n", last_net_attr.interface);
            	last_net_attr.dhcp_enable = 1;
            }

            result = adapter_->sys_ctrl_->SetNetworkAttr(net_attr);

            printf(" p.macaddr:%s \n", p->macaddr);
            printf(" p.ipaddr:%s \n", p->ipaddr);
            printf(" p.netaddr:%s \n", p->netaddr);
            printf(" p.subnetmark:%s \n", p->subnetmark);
            printf(" p.firstdns:%s \n", p->firstdns);
            printf(" p.backupdns:%s \n", p->backupdns);

            if (result < 0) {
            	sleep(5);
            	printf("new ip address modified failed! try to resume the network attribute, interface parameter: %s\n", net_attr.interface);
            	result = adapter_->sys_ctrl_->SetNetworkAttr(last_net_attr);
            	if (result < 0) {
            		printf("resume the network attribute failed! interface parameter: %s\n", last_net_attr.interface);

            	}
            }
                break;
        }
            break;
        case IOTYPE_USER_IPCAM_GET_IPADDR_REQ: {
            SMsgAVIoctrlGetIPAddrReq *p = (SMsgAVIoctrlGetIPAddrReq *)buf;
            SMsgAVIoctrlGetAPAddrResp res;

            AWNetAttr net_attr;
            memset(&net_attr, 0, sizeof(net_attr));
            adapter_->sys_ctrl_->GetNetworkAttr(net_attr);

            strncpy(res.macaddr,    net_attr.mac,     sizeof(res.macaddr) - 1);
            strncpy(res.ipaddr,     net_attr.ip,      sizeof(res.ipaddr) - 1);
            strncpy(res.netaddr,    net_attr.gateway, sizeof(res.netaddr) - 1);
            strncpy(res.subnetmark, net_attr.mask,    sizeof(res.subnetmark) - 1);
            strncpy(res.firstdns,   net_attr.dns1,    sizeof(res.firstdns) - 1);
            strncpy(res.backupdns,  net_attr.dns2,    sizeof(res.backupdns) - 1);

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_GET_IPADDR_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlGetAPAddrResp)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_GETWIFI_REQ: {
            SMsgAVIoctrlGetWifiReq *p = (SMsgAVIoctrlGetWifiReq *)buf;
            SMsgAVIoctrlGetWifiResp res;

            adapter_->sys_ctrl_->GetWifi((char*)res.ssid, (char*)res.password, (char*)&res.mode, (char*)&res.enctype);

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_GETWIFI_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlGetWifiResp)) < 0)
                break;
        }
            break;
        case IOTYPE_USER_IPCAM_SETWIFI_REQ: {
            SMsgAVIoctrlSetWifiReq *p = (SMsgAVIoctrlSetWifiReq *) buf;
            SMsgAVIoctrlSetWifiResp res;
            /*judge the password whether is null or not .(if pwd = '0',meat :null)*/
            if('0' == p->password[0] && '\0' == p->password[1]){
                memset(p->password,0,32);
            }
            res.result = adapter_->sys_ctrl_->SetWifi((char *) p->ssid, (char *) p->password);

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SETWIFI_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlSetWifiResp)) < 0)
                break;
        }
            break;
        case IOTYPE_USER_SDV_WIFI_SETTING_REQ:
           {
                AVIOString3 *strInfo= (AVIOString3 *)buf;
                /*string1[ ssid]  string2[ old passwd]   string3[new passwd]*/
                adapter_->sys_ctrl_->SetWifi(strInfo->string1, strInfo->string3);
           }
           break;
        case IOTYPE_USER_IPCAM_GETSOFTAP_REQ: {
            SMsgAVIoctrlGetSoftApReq *p = (SMsgAVIoctrlGetSoftApReq *)buf;
            SMsgAVIoctrlGetSoftApResp res;

            adapter_->sys_ctrl_->GetSoftAp((char *) res.ssid, (char *) res.password, (int*)&res.mode, (int*)&res.enctype, (int*)&res.frequency);


            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_GETSOFTAP_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlGetSoftApResp)) < 0)
                break;
        }
            break;
        case IOTYPE_USER_IPCAM_SETSOFTAP_REQ: {
            SMsgAVIoctrlSetSoftApReq *p = (SMsgAVIoctrlSetSoftApReq *) buf;
            SMsgAVIoctrlSetSoftApResp res;

            /*Here need to switch data*/

            /*from Client data:
              0:NONE
              1:WPA_WPA2_PSK
            */

            /*IPC App data:
              0:NONE
              1:WEP
              2:WAP_WPA2_EAP
              3:WPA_WPA2_PSK
              ...
            */
            switch(p->enctype){
                case 0:
                    p->enctype = 0;
                    break;
                case 1:
                default:
                    p->enctype = 3;
                    break;
            }

            res.result = adapter_->sys_ctrl_->SetSoftAp((char *) p->ssid, (char *) p->password, (int)p->mode, (int)p->enctype, (int)p->frequency);
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SETSOFTAP_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlSetSoftApResp)) < 0)
                break;
        }
            break;
        case IOTYPE_USER_IPCAM_LISTWIFIAP_REQ: {
            SMsgAVIoctrlListWifiApReq *p = (SMsgAVIoctrlListWifiApReq *)buf;
            SMsgAVIoctrlListWifiApResp res;
            res.number = 20;

            AWWifiAp ap_list[res.number];
            adapter_->sys_ctrl_->GetApList((AWWifiAp**)&ap_list, res.number);

            memcpy(res.stWifiAp, ap_list, res.number);

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_LISTWIFIAP_RESP, (char *) &res,
                             sizeof(SMsgAVIoctrlListWifiApResp)) < 0)
                break;
        }
            break;

        /* OSD Setting or Getting! */
        case IOTYPE_USER_IPCAM_SET_SHOW_DATE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;
            int channel = 0, osd_enable = 0;
            AWTimeOsd time_osd;

            channel    = p->channel;
            osd_enable = p->value;

            ret = adapter_->overlay_ctrl_->GetTimeOsd(channel, time_osd);
            if (0 == ret) {
                time_osd.osd_enable = osd_enable;
                ret = adapter_->overlay_ctrl_->SetTimeOsd(channel, time_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }
            resp.channel = channel;
            resp.value   = ret;
            if ((ret = avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_SHOW_DATE_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0))
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_TIME_FORMAT_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;
            int channel = 0, time_format = 0;
            AWTimeOsd time_osd;

            channel     = p->channel;
            time_format = p->value;

            ret = adapter_->overlay_ctrl_->GetTimeOsd(channel, time_osd);
            if (0 == ret) {
                time_osd.time_format = time_format;
                ret = adapter_->overlay_ctrl_->SetTimeOsd(channel, time_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }
            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_TIME_FORMAT_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_DATE_FORMAT_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;
            int channel = 0, date_format = 0;
            AWTimeOsd time_osd;

            channel     = p->channel;
            date_format = p->value;

            ret = adapter_->overlay_ctrl_->GetTimeOsd(channel, time_osd);
            if (0 == ret) {
                time_osd.date_format = date_format;
                ret = adapter_->overlay_ctrl_->SetTimeOsd(channel, time_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }
            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DATE_FORMAT_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_SHOW_CHN_NAME_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;
            int channel = 0, osd_enable = 0;
            AWChannelOsd chn_osd;

            channel    = p->channel;
            osd_enable = p->value;

            ret = adapter_->overlay_ctrl_->GetChannelOsd(channel, 0, chn_osd);
            if (0 == ret) {
                chn_osd.osd_enable = osd_enable;
                ret = adapter_->overlay_ctrl_->SetChannelOsd(channel, 0, chn_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }
            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_SHOW_CHN_NAME_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_CHANNEL_NAME_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetCharReq *p = (SMsgAVIoctrlAwSetCharReq *)buf;
            int ret = 0;
            int channel = 0;
            AWChannelOsd chn_osd;

            channel = p->channel;

            ret = adapter_->overlay_ctrl_->GetChannelOsd(channel, 0, chn_osd);
            if (0 == ret) {
                strncpy(chn_osd.channel_name, p->valuechar, sizeof(chn_osd.channel_name) - 1);
                ret = adapter_->overlay_ctrl_->SetChannelOsd(channel, 0, chn_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }

            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_CHANNEL_NAME_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_SHOW_DEV_NAME_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;
            int channel = 0, osd_enable = 0;
            AWDeviceOsd dev_osd;

            channel    = p->channel;
            osd_enable = p->value;

            ret = adapter_->overlay_ctrl_->GetDeviceOsd(channel, dev_osd);
            if (0 == ret) {
                dev_osd.osd_enable = osd_enable;
                ret = adapter_->overlay_ctrl_->SetDeviceOsd(channel, dev_osd);
                adapter_->overlay_ctrl_->SaveOverlayConfig();
            }
            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_SHOW_DEV_NAME_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_VIDEO_COVER_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0, i = 0;
            int channel = 0, cover_bitmap = 0, cover_enable = 0;
            AWCoverOsd cover_osd;

            channel      = p->channel;
            cover_bitmap = p->value;

            for (i = 0; i < OSD_MAX_COVER_NUM; i++) {
                if ((cover_bitmap >> i) & 0x01) {
                    cover_enable = 1;
                } else {
                    cover_enable = 0;
                }

                ret = adapter_->overlay_ctrl_->GetCoverOsd(channel, i, cover_osd);
                if (0 == ret) {
                    cover_osd.osd_enable = cover_enable;
                    ret = adapter_->overlay_ctrl_->SetCoverOsd(channel, i, cover_osd);
                    adapter_->overlay_ctrl_->SaveOverlayConfig();
                }
            }

            resp.channel = channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_VIDEO_COVER_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* Image Setting or Getting! */
        case IOTYPE_USER_IPCAM_SET_BRIGHTNESS_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, brightness = 0;
            AWImageColour image_color;

            channel    = p->channel;
            brightness = p->value;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  brightness:%d \n",__func__,__LINE__, channel, brightness);

            ret = adapter_->image_ctrl_->GetImageColour(channel, image_color);
            if (0 == ret) {
                image_color.brightness = brightness + 1;
                ret = adapter_->image_ctrl_->SetImageColour(channel, image_color);
            }
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_BRIGHTNESS_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_CONTRAST_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, contrast = 0;
            AWImageColour image_color;

            channel  = p->channel;
            contrast = p->value;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  contrast:%d \n",__func__,__LINE__, channel, contrast);

            ret = adapter_->image_ctrl_->GetImageColour(channel, image_color);
            if (0 == ret) {
                image_color.contrast = contrast;
                ret = adapter_->image_ctrl_->SetImageColour(channel, image_color);
            }
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_CONTRAST_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_SATRUCTION_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, saturation = 0;
            AWImageColour image_color;

            channel    = p->channel;
            saturation = p->value;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  saturation:%d \n",__func__,__LINE__, channel, saturation);

            ret = adapter_->image_ctrl_->GetImageColour(channel, image_color);
            if (0 == ret) {
                image_color.saturation = saturation;
                ret = adapter_->image_ctrl_->SetImageColour(channel, image_color);
            }
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_SATRUCTION_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_SHARPNESS_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, sharpness = 0;
            AWImageColour image_color;

            channel    = p->channel;
            sharpness  = p->value;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sharpness:%d \n",__func__,__LINE__, channel, sharpness);

            ret = adapter_->image_ctrl_->GetImageColour(channel, image_color);
            if (0 == ret) {
                image_color.sharpness = sharpness;
                ret = adapter_->image_ctrl_->SetImageColour(channel, image_color);
            }
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_SHARPNESS_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_HUE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, hue = 0;
            AWImageColour image_color;

            channel = p->channel;
            hue     = p->value;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  hue:%d \n",__func__,__LINE__, channel, hue);

            ret = adapter_->image_ctrl_->GetImageColour(channel, image_color);
            if (0 == ret) {
                image_color.hue = hue;
                ret = adapter_->image_ctrl_->SetImageColour(channel, image_color);
            }
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_HUE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_WIDE_DYNAMIC_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0;
            channel = p->channel;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  wd:%d \n",__func__,__LINE__, channel, p->value);

            ret = adapter_->image_ctrl_->SetImageWideDynamic(channel, p->value);
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_WIDE_DYNAMIC_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_WHITE_BALANCE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0, wb = 0;
            channel = p->channel;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  wb:%d \n",__func__,__LINE__, channel, p->value);

            ret = adapter_->image_ctrl_->SetImageWhiteBalance(channel, p->value);
            resp.channel = channel;
            resp.value = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_WHITE_BALANCE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_FLICKER_FREQ_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0;
            channel = p->channel;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  freq:%d \n",__func__,__LINE__, channel, p->value);

            ret = adapter_->image_ctrl_->SetImageFlickerFreq(channel, p->value);
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_FLICKER_FREQ_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_DENOISE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0;
            channel = p->channel;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  denoice:%d \n",__func__,__LINE__, channel, p->value);

            ret = adapter_->image_ctrl_->SetImageDenoise(channel, p->value);
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DENOISE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_DAY_NIGHT_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0;
            AWDayNightMode day_night;
            channel = p->channel;

            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  day_night_mode:%d \n",__func__,__LINE__, channel, p->value);

            ret = adapter_->image_ctrl_->GetImageDayNightMode(channel, day_night);
            if (0 == ret) {
                day_night.day_night_mode = p->value;
                ret  = adapter_->image_ctrl_->SetImageDayNightMode(channel, day_night);
            }
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DAY_NIGHT_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_EXPORSE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlSetExpoTimeReq *p = (SMsgAVIoctrlSetExpoTimeReq *) buf;
            int ret = 0;
            int channel = 0;
            channel = p->channel;
            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  exp_type:%d  exp_time:%d \n",__func__,__LINE__, channel, p->isauto, p->time);

            ret = adapter_->image_ctrl_->SetImageExposure(channel, p->isauto, p->time);
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index,    IOTYPE_USER_IPCAM_SET_EXPORSE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_FLIP_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            int channel = 0;
            channel = p->channel;
            printf ("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  Flip:%d \n",__func__,__LINE__, channel, p->value);

            switch (p->value) {
                case 0:
                    ret = adapter_->image_ctrl_->SetImageFlip(channel, 0);
                    ret = adapter_->image_ctrl_->SetImageMirror(channel, 0);
                    break;
                case 1:
                    ret = adapter_->image_ctrl_->SetImageFlip(channel, 1);
                    ret = adapter_->image_ctrl_->SetImageMirror(channel, 0);
                    break;
                case 2:
                    ret = adapter_->image_ctrl_->SetImageFlip(channel, 0);
                    ret = adapter_->image_ctrl_->SetImageMirror(channel, 1);
                    break;
                case 3:
                    ret = adapter_->image_ctrl_->SetImageFlip(channel, 1);
                    ret = adapter_->image_ctrl_->SetImageMirror(channel, 1);
                    break;

                default:
                    ret = -1;
                    break;
            }
            resp.channel = channel;
            resp.value   = ret;
            adapter_->image_ctrl_->SaveImageConfig();

            if (avSendIOCtrl(av_index,    IOTYPE_USER_IPCAM_SET_FLIP_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        /* system Setting or Getting! */
        case IOTYPE_USER_IPCAM_SET_DEV_NAME_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetCharReq *p = (SMsgAVIoctrlAwSetCharReq *)buf;
            int ret = 0;
            int channel = 0;
            AWDeviceInfo dev_info;

            channel = p->channel;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->valuechar:%s \n", __func__, __LINE__, channel, p->valuechar);

            ret = adapter_->dev_ctrl_->SetDeviceName(p->valuechar);
            resp.channel = channel;
            resp.value   = adapter_->dev_ctrl_->SaveDeviceConfig();

            AWDeviceOsd device_osd;
            ret = adapter_->overlay_ctrl_->GetDeviceOsd(0, device_osd);
            strncpy(device_osd.device_name, p->valuechar, sizeof(device_osd.device_name) -1);
            ret = adapter_->overlay_ctrl_->SetDeviceOsd(0, device_osd);
            ret = adapter_->overlay_ctrl_->SaveOverlayConfig();

            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DEV_NAME_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_DEV_ID_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *)buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->dev_ctrl_->SetDeviceId(p->value);
            resp.channel = p->channel;
            resp.value   = adapter_->dev_ctrl_->SaveDeviceConfig();
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DEV_ID_RESP, (char *) &resp,
                             sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_DATE_TIME_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlSetTimeReq *p = (SMsgAVIoctrlSetTimeReq *) buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  year:%d  month:%d day:%d  hour:%d  minute:%d  second:%d \n",
                                    __func__, __LINE__,
                                    p->year, p->month, p->day,
                                    p->hour, p->minute, p->second);

            struct tm tm;
            memset(&tm, 0, sizeof(tm));

            tm.tm_year = p->year - 1900;
            tm.tm_mon = p->month - 1; // app has +1, so...
            tm.tm_mday = p->day;
            tm.tm_hour = p->hour;
            tm.tm_min = p->minute;
            tm.tm_sec = p->second;

            time_t time = mktime(&tm);

            struct timeval tv;
            tv.tv_sec = time;
            tv.tv_usec = 0;

            settimeofday(&tv, NULL);
            system("hwclock --systohc");

            resp.value = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_DATE_TIME_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_RESTORE_DEV_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->sys_ctrl_->DefaultSystemConfig();
            resp.channel = p->channel;
            resp.value = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_RESTORE_DEV_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_RESTART_DEV_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->sys_ctrl_->RebootSystem(10);
            resp.channel = p->channel;
            resp.value = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_RESTART_DEV_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_FORMAT_DISK_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->dev_ctrl_->FormatDisk(0);
            resp.channel = p->channel;
            resp.value = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_FORMAT_DISK_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_GET_DISK_STATUS_REQ: {
            SMsgAVIoctrlGetDiskInfoResp resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            AWDiskInfo disk_info;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->dev_ctrl_->GetDiskInfo(disk_info);
            resp.capacity    = disk_info.disk_status[0].capacity;
            resp.free_space  = disk_info.disk_status[0].free_space;
            resp.disk_status = disk_info.disk_status[0].disk_status;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_GET_DISK_STATUS_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlGetDiskInfoResp)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_UNMOUNT_DISK_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  p->value:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->dev_ctrl_->UmountDisk(0);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_UNMOUNT_DISK_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_ALARM_MD_ENABLE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            AWAlarmMd alarm_md;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  md_enable:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->event_ctrl_->GetAlarmMdConfig(p->channel, alarm_md);
            if (0 == ret) {
                alarm_md.md_enable = p->value;
                ret = adapter_->event_ctrl_->SetAlarmMdConfig(p->channel, alarm_md);
                ret = adapter_->event_ctrl_->SaveEventConfig();
            }
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_ALARM_MD_ENABLE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_ALARM_MD_SENSIT_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            AWAlarmMd alarm_md;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->event_ctrl_->GetAlarmMdConfig(p->channel, alarm_md);
            if (0 == ret) {
                alarm_md.sensitive = p->value;
                ret = adapter_->event_ctrl_->SetAlarmMdConfig(p->channel, alarm_md);
                ret = adapter_->event_ctrl_->SaveEventConfig();
            }
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_ALARM_MD_SENSIT_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_ALARM_COVER_ENABLE_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            AWAlarmCover alarm_cover;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  cover_enable:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->event_ctrl_->GetAlarmCoverConfig(p->channel, alarm_cover);
            if (0 == ret) {
                alarm_cover.cover_enable = p->value;
                ret = adapter_->event_ctrl_->SetAlarmCoverConfig(p->channel, alarm_cover);
                ret = adapter_->event_ctrl_->SaveEventConfig();
            }
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_ALARM_COVER_ENABLE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;

        case IOTYPE_USER_IPCAM_SET_ALARM_COVER_SENSIT_REQ: {
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            AWAlarmCover alarm_cover;

            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);

            ret = adapter_->event_ctrl_->GetAlarmCoverConfig(p->channel, alarm_cover);
            if (0 == ret) {
                alarm_cover.sensitive = p->value;
                ret = adapter_->event_ctrl_->SetAlarmCoverConfig(p->channel, alarm_cover);
                ret = adapter_->event_ctrl_->SaveEventConfig();
            }
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_SET_ALARM_COVER_SENSIT_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                break;
        }
            break;
        case IOTYPE_USER_IPCAM_SNAP_SHOT_REQ:
            SnapShotHandler(sid, av_index, buf, type);
            break;
        case IOTYPE_USER_SET_IPCAM_RECORD_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            
            ret = adapter_->record_ctrl_->RemoteSwitchRecord();
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SET_IPCAM_RECORD_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
            break;   
        
        case IOTYPE_USER_IPCAM_PHOTO_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            
            ret = adapter_->record_ctrl_->RemoteTakePhoto();
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_IPCAM_PHOTO_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
            break;
        case IOTYPE_USER_SDV_SET_RECORD_RESOLUTION_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetVideoResoulation(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_RESOLUTION_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
            break;
        case IOTYPE_USER_SDV_SET_RECORD_LOOP_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetRecordTime(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_LOOP_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
            break;
        case IOTYPE_USER_SDV_SET_RECORD_TIMELAPSE_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetRecordDelayTime(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_TIMELAPSE_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
            break;
        case IOTYPE_USER_SDV_SET_RECORD_VOLUME_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetRecordAudioOnOff(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_VOLUME_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_RECORD_EIS_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetRecordEisOnOff(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_EIS_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_RECORD_SLOWMOTION_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetSlowRecord(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_RECORD_SLOWMOTION_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_PHOTO_RESOLUTION_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetPicResolution(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_PHOTO_RESOLUTION_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_PHOTO_TIMED_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetTimeTakePic(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_PHOTO_TIMED_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_PHOTO_AUTO_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetAutoTimeTakePic(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_PHOTO_AUTO_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_PHOTO_DRAMASHOT_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetContinuousPictureMode(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_PHOTO_DRAMASHOT_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_CAMERA_EXPOSURE_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetExposureValue(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_EXPOSURE_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_CAMERA_WHITEBALANCE_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetWhiteBalance(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_WHITEBALANCE_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_CAMERA_LIGHTSOURCEFREQUENCY_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetLightFreq(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_LIGHTSOURCEFREQUENCY_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_CAMERA_IMAGEROTATION_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetPreviewFlip(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_IMAGEROTATION_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_TIME_WATERMARK_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetTimeWaterMark(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_TIME_WATERMARK_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_DEVICE_FORMAT_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->FormatStorage(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_DEVICE_FORMAT_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
       case IOTYPE_USER_SDV_SET_CAMERA_AUTOSCREENSAVER_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetCamerAutoScreenSaver(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_AUTOSCREENSAVER_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_CAMERA_TIMEDSHUTDOWN_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetCameraTimeShutDown(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_TIMEDSHUTDOWN_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_DEVICE_LANGUAGE_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetDeviceLanguage(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_DEVICE_LANGUAGE_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_DEVICE_DATETIME_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetDeviceDateTime(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_DEVICE_DATETIME_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_DEVICE_RESETFACTORY_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetDeviceReset(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_DEVICE_RESETFACTORY_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_CAMERA_DISTORTIONCALIBRATION_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetDeviceDistortioncalibrationSwitch(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_DISTORTIONCALIBRATION_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SET_CAMERA_LEDINDICATOR_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->SetDeviceLedSwitch(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SET_CAMERA_LEDINDICATOR_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_GET_DEVICE_CONFIG_REQ:{
            SMsgAVIoctrSDVDevConfigResp resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->GetDeviceConfig(CTRL_TYPE_TUTK,p->channel,p->value,resp);
            dumpMenuConfig(resp);
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_GET_DEVICE_CONFIG_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrSDVDevConfigResp)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_REMOTE_PRVIEW_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->RemotePreviewChange(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_REMOTE_PRVIEW_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelREQ)) < 0)
                        break;
            }
                break;
        case IOTYPE_USER_SDV_SAVE_ALL_CONFIG_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            int ret = 0;
            ret = adapter_->media_ctrl_->RemoteSaveMenuconfig(CTRL_TYPE_TUTK,p->channel,p->value);
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_SAVE_ALL_CONFIG_RESP, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                        break;
            }
                break;

        case IOTYPE_USER_SDV_FILE_DELETE_REQ:
        {
            SMsgAVIoctrlAwSetChannelRESP resp;
            resp.value = 0;

            char *fileName = ((aw_sdv_handle_file *)buf)->filename;
            printf("[FUN]:%s [LINE]: %d  rm %s\n",__FUNCTION__,__LINE__,fileName);
            int ret = adapter_->dev_ctrl_->RemoveFile(fileName);
            if( ret < 0 )
            {
                printf("[FUN]:%s [LINE]: %d  rm %s failed\n",__FUNCTION__,__LINE__,fileName);
                resp.value = 1;
            }

            if( avSendIOCtrl(av_index,IOTYPE_USER_SDV_FILE_DELETE_RESP,(char *)&resp,sizeof(SMsgAVIoctrlAwSetChannelRESP))< 0 )
            {
                printf("send IOTYPE_USER_SDV_DELETE_FILE_RESP failed\n");
            }
        }
        break;
        case IOTYPE_USER_SDV_REMOTE_SLOW_RECORD_REQ:{
            SMsgAVIoctrlAwSetChannelRESP resp;
            SMsgAVIoctrlAwSetChannelREQ *p = (SMsgAVIoctrlAwSetChannelREQ *) buf;
            int ret = 0;
            printf("[FUN]:%s  [LINE]:%d  ===>>  channel:%d  sensitive:%d \n", __func__, __LINE__, p->channel, p->value);
            
            ret = adapter_->record_ctrl_->RemoteSwitchSlowRecord();
            resp.channel = p->channel;
            resp.value   = ret;
            if (avSendIOCtrl(av_index, IOTYPE_USER_SDV_REMOTE_SLOW_RECORD_RESP, (char *) &resp,
                    sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0)
                    break;
            }
        break; 

        default:
            printf("avIndex %d: non-handle type[%X]\n", av_index, type);
            break;
    }

    return 0;
}

void AVCtrl::dumpMenuConfig(SMsgAVIoctrSDVDevConfigResp &resp)
{
    /**********dumpMenuConfig begin***********/
	printf("[fangjj]:menu.switch.record_eis:[%d]\n",                             resp.switch_record_eis);
	printf("[fangjj]:menu.switch.record_awmd:[%d]\n",                         resp.switch_record_awmd);
	printf("[fangjj]:menu.switch.record_drivingmode:[%d]\n",                resp.switch_record_drivingmode);
	printf("[fangjj]:menu.switch.camera_imagerotation:[%d]\n",             resp.switch_camera_imagerotation);
	printf("[fangjj]:menu.switch.camera_ledindicator:[%d]\n",                resp.switch_camera_ledindicator);    
	printf("[fangjj]:menu.switch.camera_timewatermark:[%d]\n",            resp.switch_camera_timewatermark);
	printf("[fangjj]:menu.switch.camera_distortioncalibration:[%d]\n",     resp.switch_camera_distortioncalibration);
	printf("[fangjj]:menu.switch.camera_keytone:[%d]\n",                      resp.switch_camera_keytone);

	printf("[fangjj]:menu.record.resolution.current:[%d]\n",    resp.record_resolution.current);
	printf("[fangjj]:menu.record.record_sound.current:[%d]\n",    resp.record_sound.current);
	printf("[fangjj]:menu.record.rear_resolution.current:[%d]\n",    resp.record_rear_resolution.current);
	printf("[fangjj]:menu.record.screenbrightness.current:[%d]\n",    resp.record_screen_brightness.current);
	printf("[fangjj]:menu.record.disturbmode.current:[%d]\n",    resp.record_screen_disturb_mode.current);
	printf("[zhb]:menu.record.voicephoto.current:[%d]\n",    resp.record_voice_take_photo.current);
	printf("[zhb]:menu.record.network4g.current:[%d]\n",    resp.record_4g_network_switch.current);
	printf("[zhb]:menu.record.record_switch_wifi.current:[%d]\n", resp.record_switch_wifi.current);
	printf("[zhb]:menu.record.volumeselect.current:[%d]\n",    resp.record_volume_selection.current);
	printf("[zhb]:menu.record.poweronvoice.current:[%d]\n",    resp.record_power_on_sound.current);
	printf("[zhb]:menu.record.keyvoice.current:[%d]\n",    resp.record_key_sound.current);
	printf("[zhb]:menu.record.drivereport.current:[%d]\n",    resp.record_drivering_report.current);
	printf("[zhb]:menu.record.adas.current:[%d]\n",    resp.record_adas.current);
	printf("[zhb]:menu.record.standbyclock.current:[%d]\n",    resp.record_standby_clock.current);
	printf("[zhb]:menu.record.adasfcw.current:[%d]\n",    resp.record_adas_forward_collision_waring.current);
	printf("[zhb]:menu.record.adaslsr.current:[%d]\n",    resp.record_adas_lane_shift_reminding.current);
	printf("[zhb]:menu.record.watchdog.current:[%d]\n",    resp.record_watchdog.current);
	printf("[zhb]:menu.record.probeprompt.current:[%d]\n",    resp.record_probeprompt.current);
	printf("[zhb]:menu.record.speedprompt.current:[%d]\n",    resp.record_speedprompt.current);
	printf("[zhb]:menu.record.timewatermark.current:[%d]\n",    resp.record_timewatermark.current);
	printf("[zhb]:menu.record.emerrecord.current:[%d]\n",    resp.record_emerrecord.current);
	printf("[zhb]:menu.record.emerrecordsen.current:[%d]\n",    resp.record_emerrecordsen.current);
	printf("[zhb]:menu.record.parkingwarnlamp.current:[%d]\n",    resp.record_parkingwarnlamp_switch.current);
    printf("[zhb]:menu.record.parkingmonitor.current:[%d]\n",    resp.record_parkingmonitor_switch.current);
	printf("[zhb]:menu.record.parkingabnormalmonitor.current:[%d]\n",    resp.record_parkingabnormalmonitory_switch.current);
	printf("[zhb]:menu.record.parkingabnormalnotice.current:[%d]\n",    resp.record_parkingloopabnormalnotice_switch.current);
	printf("[zhb]:menu.record.parkingloopsw.current:[%d]\n",    resp.record_parkingloop_switch.current);
	printf("[zhb]:menu.record.parkingloopresolution.current:[%d]\n",    resp.record_parkingloop_resolution.current);
	printf("[fangjj]:menu.record.loop.current:[%d]]\n",            resp.record_loop.current);
	printf("[fangjj]:menu.record.timelapse.current:[%d]\n",     resp.record_timelapse.current);
	printf("[fangjj]:menu.record.slowmotion.current:[%d]\n",  resp.record_slowmotion.current);

	printf("[fangjj]:menu.photo.resolution.current:[%d]\n",     resp.photo_resolution.current);
	printf("[fangjj]:menu.photo.timed.current:[%d]\n",           resp.photo_timed.current);
	printf("[fangjj]:menu.photo.auto.current:[%d]\n",             resp.photo_auto.current);
	printf("[fangjj]:menu.photo.dramashot.current:[%d]\n",     resp.photo_dramashot.current);

	printf("[fangjj]:menu.camera.exposure.current:[%d]\n",               resp.camera_exposure.current);
	printf("[fangjj]:menu.camera.whitebalance.current:[%d]\n",          resp.camera_whitebalance.current);
	printf("[fangjj]:menu.camera.lightfreq.current:[%d]\n",                resp.camera_lightfreq.current);  
	printf("[fangjj]:menu.camera.autoscreensaver.current:[%d]\n",      resp.camera_autoscreensaver.current);
	printf("[fangjj]:menu.camera.timedshutdown.current:[%d]\n",       resp.camera_timedshutdown.current);      
	printf("[fangjj]:menu.camera.wifiinfo.ssid:%s\n",                              resp.camera_wifiinfo.string1);
	printf("[fangjj]:menu.camera.wifiinfo.password:%s\n",                           resp.camera_wifiinfo.string2);

	printf("[fangjj]:menu.device.sysversion.version:%s\n",                              resp.device_sysversion.string1);

	printf("[fangjj]:menu.device.language.current:[%d]\n",         resp.device_language.current);
	printf("[zhb]:menu.device.updatetime.current:[%d]\n",         resp.device_datatime.current);
	printf("[fangjj]:menu.device.voicestatus.current:[%d]\n",      resp.device_voicestatus.current);
    /**********dumpMenuConfig end***********/
}


int AVCtrl::SendCmdData(int type,int chn ,int value, void *data)
{
    printf("[SendCmdData]:Send CMD is type = %d ; chn = %d ; value = %d",type,chn,value);
    int ret = 0;
    SMsgAVIoctrlAwSetChannelRESP resp;
    resp.channel = chn;
    resp.value = value;
    if (avSendIOCtrl(0, type, (char *) &resp,
                        sizeof(SMsgAVIoctrlAwSetChannelRESP)) < 0){
         printf("[error]:Send CMD to apk filed");
         return -1;
    }
    return ret;
}


int AVCtrl::SnapShotHandler(int sid, int av_index, char *buf, int type)
{
    return adapter_->media_ctrl_->SnapShot(CTRL_TYPE_TUTK, 0);
}

int AVCtrl::SetClientConnectStatus(bool p_Connect)
{
    if( p_Connect )
    {
    	//by hero ****** wifi led long light
        //LedControl::get()->EnableLed(LedControl::WIFI_LED, true, LedControl::LONG_LIGHT);
    }
    else
    {
    	//by hero ****** wifi led shining light
        //LedControl::get()->EnableLed(LedControl::WIFI_LED, true, LedControl::SHINING_LIGHT);
        adapter_->media_ctrl_->RemoteClientDisconnect();
    }

    return 0;
}
