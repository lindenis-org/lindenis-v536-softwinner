/* *******************************************************************************
 * Copyright (c), 2001-2016, Allwinner Tech. All rights reserved.
 * *******************************************************************************/
/**
 * @file    video_player.h
 * @brief   视频播放
 * @author  id:826
 * @version v0.3
 * @date    2016-09-20
 */

#pragma once

#include "common/subject.h"
#include "EyeseePlayer.h"

#include <string>
#include <list>
#include <mutex>

namespace EyeseeLinux {

enum PlayerState {
    PLAYER_ERROR = -1,
    PLAYER_IDLE = 0,
    PLAYER_PREPARING = 1,
    PLAYER_PREPARED = 2,
    PLAYER_STARTED = 3,
    PLAYER_PAUSED = 4,
    PLAYER_PLAY_COMPLETED = 5,
};

// class MediaPlayer {
// public:
// MediaPlayer();
// virtual ~MediaPlayer();
// virtual int PreparePlay(const String8& file_path) = 0;
// virtual void Start() = 0;
// virtual void Stop() = 0;
// virtual void Pause() = 0;
// virtual void Seek() = 0;
// virtual void Release() = 0;
// virtual void Reset() = 0;
// virtual void SetVolume(int val) = 0;
// }; // class Player

class Layer;

class VideoPlayer
        : public EyeseePlayer::OnPreparedListener
          , public EyeseePlayer::OnCompletionListener
          , public EyeseePlayer::OnErrorListener
          , public EyeseePlayer::OnVideoSizeChangedListener
          , public EyeseePlayer::OnInfoListener
          , public EyeseePlayer::OnSeekCompleteListener
          , public ISubjectWrap(VideoPlayer)
          , public IObserverWrap(VideoPlayer) {
    public:
        VideoPlayer();

        ~VideoPlayer();

        void ResetLayer();

        int PreparePlay(std::string file_path);

        int Start();

        int Pause();

        int Seek(float percent);

        int Seek(int msec);

        int Stop();

        int Reset();

        void SetVolume(int percent);

        void SetLoopingMode(bool loop);

        bool GetLoopingMode();

        void Release();

        int GetCurrentFileDuration(int &msec);

        PlayerState GetStatus();

        int SwitchAOCard(PCM_CARD_TYPE_E cardId);

        void onPrepared(EyeseePlayer *pMp);

        void onCompletion(EyeseePlayer *pMp);

        bool onError(EyeseePlayer *pMp, int what, int extra);

        void onVideoSizeChanged(EyeseePlayer *pMp, int width, int height);

        bool onInfo(EyeseePlayer *pMp, int what, int extra);

        void onSeekComplete(EyeseePlayer *pMp);

        void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

		int getCurrentPosition();

		int getDuration();

    private:
        EyeseePlayer *media_player_;
        Layer *layer_;
        PlayerState current_status_;
        std::string last_file_;
        bool seek_done_;
        std::mutex mutex_;
}; // class VideoPlayer

class AudioPlayer {
    public:
        AudioPlayer();

        virtual ~AudioPlayer();

        int PreparePlay(const std::string &file_path);

        void Start();

        void Stop();

        void SetVolume(int val);

}; // class AudioPlayer

} // namespace EyeseeLinux
