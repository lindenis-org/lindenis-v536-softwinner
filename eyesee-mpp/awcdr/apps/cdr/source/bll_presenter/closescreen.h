

#pragma once

#include <time.h>
#include <signal.h>

class Closescreen

	{
		public:
			Closescreen();
			~Closescreen();
			static Closescreen* GetInstance();
			void SetDelayTime(int sec);
			void Start();
			void Stop();
			bool IsScreenClosed();
			bool IsClosescreenEnable();
			void SetClosescreenEnable(bool onoff);
			static void ClosescreenTimerProc(union sigval sigval);            
            int SetHdmiConnect(bool p_bConnect);
		private:
			bool close_statu_;
			int  sec_counter_;
			bool switch_;
            timer_t time_wifi_open_closescreen_timer_id_;
			bool m_bHdmiConnect;
			void ClosescreenStatusHandler();
			
	};


