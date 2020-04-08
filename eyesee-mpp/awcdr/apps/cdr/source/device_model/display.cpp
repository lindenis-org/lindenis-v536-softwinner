#include "display.h"
#include "common/app_def.h"
#include "common/app_log.h"

#include <string.h>

#include <mpi_vo.h>

#undef LOG_TAG
#define LOG_TAG "Display"

using namespace EyeseeLinux;
using namespace std;

Layer::Layer()
{
	pthread_mutex_init(&m_lock, NULL);
    AW_MPI_VO_Enable(0);
    memset(layers_, -1, MAX_LAYER_NUM);
    memset(status_, 0, MAX_LAYER_NUM);
    layers_[LAYER_UI] = HLAY(2, 0);
    AW_MPI_VO_AddOutsideVideoLayer(layers_[LAYER_UI]);
    status_[LAYER_UI] = OPENED;
}

Layer::~Layer()
{
    for (int i = 0; i < MAX_LAYER_NUM; ++i)
	{
        if (status_[i] == OPENED && i != LAYER_UI)
		{
            CloseLayer(i);
            ReleaseLayer(i);
        }
    }

    if(layers_[LAYER_UI] >= 0)
    {
        AW_MPI_VO_RemoveOutsideVideoLayer(layers_[LAYER_UI]);
        layers_[LAYER_UI] = -1;
    }
    AW_MPI_VO_Disable(0);
	pthread_mutex_destroy(&m_lock);
}

//in V3, V40, just chan0(layer0 -layer3) can be used for video
int Layer::RequestLayer(LAYER_ID index, unsigned char layer_id, ViewInfo *sur)
{
	pthread_mutex_lock(&m_lock);
	if( layers_[index] < 0 )
	{
	    VO_LAYER hlay = 0;
	    while(hlay < VO_MAX_LAYER_NUM)
	    {
	        if(SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay))
	        {
	            break;
	        }
	        hlay++;
	    }
	    if(hlay >= VO_MAX_LAYER_NUM)
	    {
	        db_error("fatal error! enable video layer fail!");
	        hlay = -1;
	    }
	    layers_[index] = hlay;
	    db_msg("request index: %d layer: %d, hlay: %d", index, layer_id, layers_[index]);
	    if (layers_[index] < 0) {
	        db_error("request layer failed");
			pthread_mutex_unlock(&m_lock);
	        return -1;
	    }
	}

    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    AW_MPI_VO_GetVideoLayerAttr(layers_[index], &stLayerAttr);
    stLayerAttr.stDispRect.X = sur->x;
    stLayerAttr.stDispRect.Y = sur->y;
    stLayerAttr.stDispRect.Width = sur->w;
    stLayerAttr.stDispRect.Height = sur->h;
    AW_MPI_VO_SetVideoLayerAttr(layers_[index], &stLayerAttr);
	pthread_mutex_unlock(&m_lock);
    this->CleanLayer(layer_id);
    status_[index] = OPENED;

    return layers_[index];
}

// NOTE: only used for player
int Layer::RequestLayer(LAYER_ID index, unsigned char channel_id, unsigned char layer_id, unsigned char zorder, ViewInfo *sur)
{	
	pthread_mutex_lock(&m_lock);
	if(layers_[index] < 0)
	{
	    if(SUCCESS == AW_MPI_VO_EnableVideoLayer(HLAY(channel_id, layer_id)))
	    {
	        layers_[index] = HLAY(channel_id, layer_id);
	        AW_MPI_VO_SetVideoLayerPriority(layers_[index], (unsigned int)zorder);
	    }
	    else
	    {
	        db_error("enable video layer[%d] failed! channel[%d]", layer_id, channel_id);
	        layers_[index] = -1;
	    }

	    db_msg("request layer: %d, hlay: %d", layer_id, layers_[index]);
	    if (layers_[index] < 0) {
	        db_error("request layer failed");
			pthread_mutex_unlock(&m_lock);
	        return -1;
	    }

	    //hwd_layer_set_rect(layers_[LAYER_PLAYER], sur);
	    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	    AW_MPI_VO_GetVideoLayerAttr(layers_[index], &stLayerAttr);
	    stLayerAttr.stDispRect.X = sur->x;
	    stLayerAttr.stDispRect.Y = sur->y;
	    stLayerAttr.stDispRect.Width = sur->w;
	    stLayerAttr.stDispRect.Height = sur->h;
	    AW_MPI_VO_SetVideoLayerAttr(layers_[index], &stLayerAttr);
	}
	else
	{
		db_msg("layer has been request! index %d channel %d layerId %d\n",index, channel_id, layer_id);
	}
    pthread_mutex_unlock(&m_lock);
    this->CleanLayer(index);
    status_[index] = OPENED;


    return layers_[index];
}

int Layer::SetLayerZorder(LAYER_ID p_index, unsigned int p_zorder)
{
	pthread_mutex_lock(&m_lock);
	if(layers_[p_index] < 0)
	{
		pthread_mutex_unlock(&m_lock);
		return -1;
	}
	AW_MPI_VO_SetVideoLayerPriority(layers_[p_index], p_zorder);

	pthread_mutex_unlock(&m_lock);
	return 0;
}

void Layer::OpenLayer(int layer_id)
{
	pthread_mutex_lock(&m_lock);
	if(layers_[layer_id] < 0 )
	{
		db_error("layer %d not opend yet!\n",layer_id);
		pthread_mutex_unlock(&m_lock);
		return ;
	}

    if (status_[layer_id] == CLOSED)
	{
        AW_MPI_VO_OpenVideoLayer(layers_[layer_id]);
        status_[layer_id] = OPENED;
        db_msg("open layer: %d, hlay: %d", layer_id, layers_[layer_id]);
    }
	pthread_mutex_unlock(&m_lock);
}

void Layer::CloseLayer(int layer_id)
{
	pthread_mutex_lock(&m_lock);
    if(layers_[layer_id] < 0)
	{
        db_error("layer:%d is not exits,can't close layer", layer_id);
		pthread_mutex_unlock(&m_lock);
        return;
    }

    if (status_[layer_id] == OPENED)
	{
        AW_MPI_VO_CloseVideoLayer(layers_[layer_id]);
        db_msg("close layer: %d, hlay: %d", layer_id, layers_[layer_id]);
        status_[layer_id] = CLOSED;
    }
	pthread_mutex_unlock(&m_lock);
}

ViewInfo Layer::SetLayerRectByDispType(int layer_id, int rect_id)
{
    pthread_mutex_lock(&m_lock);
    int ret;
    //int disp_type, tv_mode;
    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    GetDisplayDeviceType(&disp_type, &tv_mode);

    ViewInfo rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    db_debug("disp_type: 0x%x, tv_mode: 0x%x", disp_type, tv_mode);

    switch (disp_type) {
        case VO_INTF_LCD/*DISP_OUTPUT_TYPE_LCD*/: {
                if (rect_id == 0) { /* dualcam, CAM_A */
                    rect.x = 666;
                    rect.y = 180;
                    rect.w = 443;
                    rect.h = 400;
                } else if (rect_id == 1) { /* dualcam, CAM_B */
                    rect.x = 158;
                    rect.y = 180;
                    rect.w = 443;
                    rect.h = 400;
                } else if (rect_id == -1) { /* onecam, default */
                    if (layer_id == LAYER_UI) {
                        rect.x = 0;
                        rect.y = 0;
                        rect.w = SCREEN_WIDTH;
                        rect.h = SCREEN_HEIGHT;
                    } else {
                        rect.x = 0;
                        rect.y = 0;
                        rect.w = SCREEN_WIDTH;
                        rect.h = SCREEN_HEIGHT;
                    }
                }
            }
            break;
        default:
            db_error("unknown disp type[0x%x]", disp_type);
            break;
    }

    pthread_mutex_unlock(&m_lock);
    if ( (ret = SetLayerRect(layer_id, &rect)) < 0 ) {
        db_error("set layer rect failed, ret: %d", ret);
    }

    return rect;
}

int Layer::SetLayerRect(int layer_id, ViewInfo *sur)
{
	pthread_mutex_lock(&m_lock);
    if(layers_[layer_id] < 0) {
        db_error("layer:%d is not exits,can't set layer", layer_id);
		pthread_mutex_unlock(&m_lock);
        return -1;
    }

    //return hwd_layer_set_rect(layers_[layer_id], sur);
    int result = 0;
    ERRORTYPE ret;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    AW_MPI_VO_GetVideoLayerAttr(layers_[layer_id], &stLayerAttr);
    stLayerAttr.stDispRect.X = sur->x;
    stLayerAttr.stDispRect.Y = sur->y;
    stLayerAttr.stDispRect.Width = sur->w;
    stLayerAttr.stDispRect.Height = sur->h;
    db_debug("set disp layer rect, layer: %d, rect:[%d, %d, %d, %d]", layer_id,
            stLayerAttr.stDispRect.X, stLayerAttr.stDispRect.X,
            stLayerAttr.stDispRect.Width, stLayerAttr.stDispRect.Height);
    ret = AW_MPI_VO_SetVideoLayerAttr(layers_[layer_id], &stLayerAttr);
    if(ret != SUCCESS)
    {
        result = -1;
    }
	pthread_mutex_unlock(&m_lock);
    return result;
}

void Layer::ReleaseLayer(int layer_id)
{
	pthread_mutex_lock(&m_lock);
    if(layers_[layer_id] < 0) {
        db_error("layer:%d is not exits, can't release layer", layer_id);
		pthread_mutex_unlock(&m_lock);
        return;
    }

    db_info("release layer: %d, hlay: %d", layer_id, layers_[layer_id]);
    AW_MPI_VO_DisableVideoLayer(layers_[layer_id]);
    layers_[layer_id] = -1;
	pthread_mutex_unlock(&m_lock);
}

void Layer::CleanLayer(int layer_id)
{
	pthread_mutex_lock(&m_lock);
    if(layers_[layer_id] > 0) {
        //hwd_layer_clear(layers_[layer_id]);
        AW_MPI_VO_CloseVideoLayer(layers_[layer_id]);
    }
	pthread_mutex_unlock(&m_lock);
}

void Layer::SwitchLayer(int layer_id, int on)
{
    pthread_mutex_lock(&m_lock);
    if(1 == on) {
        OpenLayer(layer_id);
    } else if(0 == on) {
        CloseLayer(layer_id);
    }
    pthread_mutex_unlock(&m_lock);
}

void Layer::SetLayerTop(int layer_id)
{
	pthread_mutex_lock(&m_lock);
    if (layers_[layer_id] > 0) {
        //hwd_layer_top(layers_[layer_id]);
        AW_MPI_VO_SetVideoLayerPriority(layers_[layer_id], ZORDER_MAX);
    }
	pthread_mutex_unlock(&m_lock);
}

void Layer::SetLayerAlpha(int layer_id, unsigned char alpha)
{
    pthread_mutex_lock(&m_lock);
    VO_VIDEO_LAYER_ALPHA_S stAlpha;
    stAlpha.mAlphaMode = 0;
    stAlpha.mAlphaValue = alpha;
    int ret = AW_MPI_VO_SetVideoLayerAlpha(layers_[layer_id], &stAlpha);
    if (ret != RET_OK) {
        db_error("set failed, layer id: %d, hlay: %d, alpha: %d", layer_id, layers_[layer_id], alpha);
    }

    db_debug("layer id: %d, hlay: %d, alpha: %d", layer_id, layers_[layer_id], alpha);
   pthread_mutex_unlock(&m_lock);
}

int Layer::GetDisplayDeviceType(VO_INTF_TYPE_E *disp_type, VO_INTF_SYNC_E *tv_mode)
{
    VO_PUB_ATTR_S stPubAttr;
    ERRORTYPE ret = AW_MPI_VO_GetPubAttr(0, &stPubAttr);
    if(ret == SUCCESS)
    {
        *disp_type = stPubAttr.enIntfType;
        *tv_mode = stPubAttr.enIntfSync;
        fprintf(stderr, "get disp_type[%d]\n", *disp_type);
        return 0;
    }
    else
    {
        db_warn("VO DEV is not enable!");
        return -1;
    }
}

int Layer::SwitchDisplayDevice(DISPLAY_DEVICE display_dev)
{
    int ret = 0;
    VO_INTF_TYPE_E  disp_type = VO_INTF_LCD;
    VO_INTF_SYNC_E  tv_mode = VO_OUTPUT_NTSC;

    switch (display_dev) {
        case DISPLAY_DEV_LCD:
            disp_type = VO_INTF_LCD;    //DISP_OUTPUT_TYPE_LCD;
            tv_mode   = VO_OUTPUT_NTSC;   //DISP_TV_MOD_480I;
            break;
        default:
            break;
    }
    VO_PUB_ATTR_S stPubAttr;
    AW_MPI_VO_GetPubAttr(0, &stPubAttr);
    stPubAttr.enIntfType = disp_type;
    stPubAttr.enIntfSync = tv_mode;
    AW_MPI_VO_SetPubAttr(0, &stPubAttr);
    return ret;
}
void Layer::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{

}
#if 0
int Layer::SetPreviewType(CameraPreviewType p_previewType)
{
	m_PreviewStatus = p_previewType;
	return 0;
}

int Layer::GetPreviewType(CameraPreviewType *p_previewType)
{
	*p_previewType = m_PreviewStatus;

	return 0;
}
#endif
