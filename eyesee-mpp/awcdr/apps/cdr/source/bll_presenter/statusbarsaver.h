

#pragma once

#include "common/subject.h"
#include <utils/Mutex.h>
#include "common/posix_timer.h"
#include <time.h>
#include <signal.h>



namespace EyeseeLinux{


class StatusBarSaver
	: public ISubjectWrap(StatusBarSaver)
{
public:
	StatusBarSaver();
	~StatusBarSaver();
	static StatusBarSaver* GetInstance();
	void SetDelayTime(int sec);
	void Start();
	void Stop();
	void SetStatusBarSaverEnable(bool onoff);
	static void StatusBarSaverTimerProc(union sigval sigval);
	int pause(bool p_bPause);
	int StatusBarSaverCtl();
	int reSetTimer();
    void StatusBarSaverTimerHandler();
private:
	bool saver_stu_;
	int  sec_counter_;
	bool saver_enable;
	timer_t statusbarsaver_timer_id;
	bool m_bPause;
};

}

