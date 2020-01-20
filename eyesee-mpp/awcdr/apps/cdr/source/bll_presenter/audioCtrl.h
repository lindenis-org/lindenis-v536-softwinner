#pragma once
#include "common/singleton.h"
#include "device_model/media/player/wav_player.h"

#include <map>
#include <string>

namespace EyeseeLinux {

class WavParser;
class WavPlayer;

class AudioCtrl
    :public Singleton<AudioCtrl>
{
    friend class Singleton<AudioCtrl>;

	public:
		enum sounds_type
		{
			KEY1_SOUND = 1,
			AUTOPHOTO_SOUND,
			SHUTDOWN_SOUND,
			TIMECOUNT_SOUND,
			STARTUP_SOUND,
			ADAS_FCW_WARNING_SOUND,
			ADAS_FCW_DANGER_SOUND,
			ADAS_LDW_SOUND,
			ADAS_ATTENTION_SOUND,
		};

	public:
	int PlaySound(sounds_type type);
        int InitPlayer(int ao_card);
        int DeInitPlayer();
        int SwitchAOCard(int ao_card);
        void SetBeepToneVolume(int val);

	private:
        std::map<int, PcmWaveData> pcm_data_map_;
        std::map<int, std::string> wav_file_list_;
        WavParser *wav_parser_;
        WavPlayer *wav_player_;
        std::mutex init_mutex_;
        bool inited_;
        int volume_;

		AudioCtrl();
		~AudioCtrl();
        AudioCtrl(const AudioCtrl &o);
        AudioCtrl &operator=(const AudioCtrl &o);
};

}
