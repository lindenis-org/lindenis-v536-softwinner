/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    video_player.cpp
 * @brief   视频播放
 * @author  id:826
 * @version v0.3
 * @date    2016-09-20
 */

#include <mpi_vdec.h>
#include "device_model/media/player/video_player.h"
#include "device_model/display.h"
#include "common/app_log.h"
#include <mutex>
#include "common/app_def.h"

#undef LOG_TAG
#define LOG_TAG "VideoPlayer"

using namespace EyeseeLinux;
using namespace std;

VideoPlayer::VideoPlayer()
    : current_status_(PLAYER_IDLE)
    , seek_done_(true)
{
    int handler = -1;
    media_player_ = new EyeseePlayer();
    layer_ = Layer::GetInstance();

    // just default value
    ViewInfo sur;
    sur.x = 0;
    sur.y = 0;
    sur.w = SCREEN_WIDTH;
    sur.h = SCREEN_HEIGHT;
    handler = layer_->RequestLayer(LAYER_PLAYER, 0, 0, 0, &sur);
    if (handler >= 0) {
        media_player_->setDisplay(handler);
    }

    media_player_->setOnPreparedListener(this);
    media_player_->setOnVideoSizeChangedListener(this);
    media_player_->setOnCompletionListener(this);
    media_player_->setOnErrorListener(this);
    media_player_->setOnInfoListener(this);
    media_player_->setOnSeekCompleteListener(this);

}

VideoPlayer::~VideoPlayer()
{
    db_msg("destruct");
    layer_->CloseLayer(LAYER_PLAYER);
    layer_->ReleaseLayer(LAYER_PLAYER);
    Reset();
    delete media_player_;
}

void VideoPlayer::ResetLayer()
{
    db_msg("reset video layer...");
    layer_->CloseLayer(LAYER_PLAYER);
    layer_->ReleaseLayer(LAYER_PLAYER);
}

int VideoPlayer::PreparePlay(string file_path)
{
    int ret = -1;
    int wait_count = 0;

    lock_guard<mutex> lock(mutex_);

    last_file_ = file_path;
    ret = media_player_->setDataSource(file_path);
    if (ret != NO_ERROR) {
        db_error("set data source failed:%d", ret);
        current_status_ = PLAYER_IDLE;
        return -1;
    }
    media_player_->setOutputPixelFormat(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
#if 0
    ret = media_player_->setAudioStreamType(AUDIO_STREAM_MUSIC);    /*AudioManager.STREAM_MUSIC*/
    if (ret != NO_ERROR) {
        db_error("set audio stream type failed:%d", ret);
        return ret;
    }
    media_player_->setScreenOnWhilePlaying(true);

    //play video rotation 90 degree for IPS(vertical screen)
    media_player_->setRotation(1);

    /* Initialized */
    ret = media_player_->prepareAsync();
#endif
    VO_INTF_TYPE_E disp_type;
    VO_INTF_SYNC_E tv_mode;
    Layer::GetInstance()->GetDisplayDeviceType(&disp_type, &tv_mode);
    // 0-no rotation, 1-90degree, 2-180degree, 3-270degree
    if( disp_type == VO_INTF_HDMI )
    {
        media_player_->setAOCardType(PCM_CARD_TYPE_SNDHDMI);
        media_player_->setRotation(ROTATE_NONE);
    }
    else
    {
        media_player_->setAOCardType(PCM_CARD_TYPE_AUDIOCODEC);
        media_player_->setRotation(ROTATE_270);
        media_player_->enableScaleMode(true, 320, 240);
    }
    //AW_MPI_VDEC_SetVEFreq(MM_INVALID_CHN, 648);
    ret = media_player_->prepare();
    if (ret != NO_ERROR) {
        db_error("prepare async failed:%d", ret);
        return ret;
    }
    current_status_ = PLAYER_PREPARED;
    int video_width = 0, video_height = 0;
    media_player_->getVideoWidth(&video_width);
    media_player_->getVideoHeight(&video_height);
	SetVolume(100);

    Layer::GetInstance()->SetLayerRectByDispType(LAYER_PLAYER, -1);

#if 0
    while (1) {
        if (current_status_ == PLAYER_PREPARED) {
            db_msg("Player PREPARED");
            break;
        } else {
            db_msg("wait Player state change to PREPARED");
            usleep(60 * 1000);
        }
        if (++wait_count > 20) {
            db_msg("wait PREPARED too long, may be invalid video");
            this->Reset();
            return -1;
        }
    }
#endif
    return ret;
}

int VideoPlayer::Start()
{
    int ret = -1;

    lock_guard<mutex> lock(mutex_);

    if (current_status_ != PLAYER_PREPARED && current_status_ != PLAYER_PAUSED && current_status_ != PLAYER_PLAY_COMPLETED){
        mutex_.unlock();
        //db_msg("[debug_jason]:@@@@@@@@@@@@@@@@@@");
        ret = this->PreparePlay(last_file_);
        mutex_.lock();
    }

    if (current_status_ == PLAYER_PREPARED || current_status_ == PLAYER_PAUSED || current_status_ == PLAYER_PLAY_COMPLETED) {
        //db_msg("[debug_jason]:%%%%%%%%%%%%%%%%%%%%%%%%%%%");
        this->Notify(MSG_VIDEO_PLAY_START);
        ret = media_player_->start();
        current_status_ = PLAYER_STARTED;
    }

    return ret;
}

int VideoPlayer::Pause()
{
    int ret = -1;

    lock_guard<mutex> lock(mutex_);

    if (current_status_ == PLAYER_STARTED) {
        db_msg("[debug_jason]:%%%%%%%%%%%%%%%%%%%%%%%%%%%@@@@@@");
        ret = media_player_->pause();
        current_status_ = PLAYER_PAUSED;
    }
    this->Notify(MSG_VIDEO_PLAY_PAUSE);

    return ret;
}

int VideoPlayer::Seek(float percent)
{
    int ret = -1;
    int32_t total, seek_to;

    if (percent < 0) percent = 0;

    if (GetCurrentFileDuration(total) < 0)  return -1;

    seek_to = (int32_t)percent / 100 * total;

    ret = Seek(seek_to);

    return ret;
}

int VideoPlayer::Seek(int msec)
{
    int total = 0, ret = -1;

    lock_guard<mutex> lock(mutex_);

    total = media_player_->getDuration();
    if (total < 0) return -1;

    if (msec > total) msec = total;

    if (seek_done_) {
        seek_done_ = false;
        ret = media_player_->seekTo(msec);
        if (ret != NO_ERROR) {
            db_error("seek to %d failed: %d", msec, ret);
        }
    } else {
        db_warn("last seek is not complete");
    }

    return ret;
}

int VideoPlayer::Stop()
{
    int ret = -1;

    lock_guard<mutex> lock(mutex_);

    db_debug("Stop record...");
    db_debug("current status: %d", current_status_);
    if (current_status_ != PLAYER_IDLE) {
        ret = media_player_->stop();
        media_player_->reset();
        current_status_ = PLAYER_IDLE;
    }
    db_debug("Stop record done");

    //this->Release();
    this->Notify(MSG_VIDEO_PLAY_STOP);

    return ret;
}

int VideoPlayer::Reset()
{
    int ret = -1;

    lock_guard<mutex> lock(mutex_);

    db_debug("status: %d", current_status_);

    if (current_status_ != PLAYER_IDLE) {
        ret = media_player_->reset();
        current_status_ = PLAYER_IDLE;
    }

    return ret;
}

void VideoPlayer::SetVolume(int percent)
{
    float vol = percent / 100.0f;

   // lock_guard<mutex> lock(mutex_);

    media_player_->setVolume(vol, vol);
}

void VideoPlayer::SetLoopingMode(bool loop)
{
    lock_guard<mutex> lock(mutex_);

    media_player_->setLooping(loop);
}

bool VideoPlayer::GetLoopingMode()
{
    lock_guard<mutex> lock(mutex_);

    return media_player_->isLooping();
}

#if 0
void VideoPlayer::Release()
{
    if (current_status_ != PLAYER_IDLE) {
        this->Stop();
    }
    media_player_->release();
}
#endif

int VideoPlayer::GetCurrentFileDuration(int &msec)
{
    lock_guard<mutex> lock(mutex_);

    msec = media_player_->getDuration();
    db_msg("current file duration: %dmsec", msec);

    return msec;
}

PlayerState VideoPlayer::GetStatus()
{
    lock_guard<mutex> lock(mutex_);

    return current_status_;
}

void VideoPlayer::onPrepared(EyeseePlayer *pMp)
{
    db_msg("prepared");

    lock_guard<mutex> lock(mutex_);

    current_status_ = PLAYER_PREPARED;

    this->Notify(MSG_VIDEO_PLAY_PREPARED);
}

void VideoPlayer::onCompletion(EyeseePlayer *pMp)
{
    db_msg("completion");
    /*
    if (!GetLoopingMode()) {
        media_player_->reset();
        current_status_ = PLAYER_IDLE;
    }
*/
    //this->Release();

    // NOTE: 手动停止回放和自动播放完成同时发生时, 在这里调用reset可能发生死锁
    // media_player_->reset();
    current_status_ = PLAYER_PLAY_COMPLETED;

    this->Notify(MSG_VIDEO_PLAY_COMPLETION);
}

bool VideoPlayer::onError(EyeseePlayer *pMp, int what, int extra)
{
    db_error("receive onError message!");
    return false;
}

void VideoPlayer::onVideoSizeChanged(EyeseePlayer *pMp, int width, int height)
{
    db_msg("onVideoSizeChanged!");
}

bool VideoPlayer::onInfo(EyeseePlayer *pMp, int what, int extra)
{
    db_msg("receive onInfo message!");
    return false;
}

void VideoPlayer::onSeekComplete(EyeseePlayer *pMp)
{
    db_msg("receive onSeekComplete message!");
    seek_done_ = true;
}

void VideoPlayer::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        default:
            break;
    }
}

int VideoPlayer::getCurrentPosition()

{
    return media_player_->getCurrentPosition();
}

int VideoPlayer::getDuration()

{
    return media_player_->getDuration();
}

int VideoPlayer::SwitchAOCard(PCM_CARD_TYPE_E cardId)
{
    return media_player_->setAOCardType(cardId);
}
