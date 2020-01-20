

#pragma once

#include "common/subject.h"
#include <utils/Mutex.h>
#include "common/posix_timer.h"
#include <time.h>
#include <signal.h>


namespace EyeseeLinux{


	class Autoshutdown
		: public ISubjectWrap(Autoshutdown)

	{
		public:
			Autoshutdown();
			~Autoshutdown();
			static Autoshutdown* GetInstance();
			void SetDelayTime(int sec);
			void Start();
			void Stop();
			bool GetAutoshutdownStatus();
			bool IsAutoshutdownEnable();
			void SetAutoshutdownEnable(bool onoff);
			static void CheckAutoshutdownStatusTimerProc(union sigval sigval);
                      int reStartCount();
		       int pause(bool p_bPause);
		private:
			bool saver_stu_;
			int  sec_counter_;
			bool autoshutdown_switch_;
			timer_t time_check_autoshutdown_status_timer_id_;
                      bool m_bPause;
			void AutoshutdownStatusHandler();

	};

}

