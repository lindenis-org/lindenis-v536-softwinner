

#pragma once

#include "common/subject.h"
#include <utils/Mutex.h>
#include "common/posix_timer.h"
#include <time.h>
#include <signal.h>


namespace EyeseeLinux{


class Screensaver
	: public ISubjectWrap(Screensaver)
{
public:
	Screensaver();
	~Screensaver();
	static Screensaver* GetInstance();
	void SetDelayTime(int sec);
	void Start();
	void Stop();
	bool GetScreensaverStatus();
	bool IsScreensaverEnable();
	void SetScreensaverEnable(bool onoff);
	static void CheckScreensaverStatusTimerProc(union sigval sigval);
	int reStartCount();
	int pause(bool p_bPause);
	int SetHdmiConnect(bool p_bConnect);
	bool GetHdmiConnect();
	int stopTimer();
	int reSetTimer();
	static void CountScreensaverTimerTimerProc(union sigval sigval);
	int CountScreenSaverTime(bool start_);
	int ForceReTimerStatus();
	int ForceScreenOnBySdcard();
private:
	bool saver_stu_;
	int  sec_counter_;
	bool saver_switch_;
	timer_t time_check_screensaver_status_timer_id_;
	bool m_bPause;
	bool m_bHdmiConnect;
	void ScreensaverStatusHandler();
	timer_t time_count_screensaver_timer_id_;
	bool m_min;//screen saver had been 1 minue 
	void CountScreensaverTimeHandler();

};

}

