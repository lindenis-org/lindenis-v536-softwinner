


#include "closescreen.h"
#include <utils/Mutex.h>
#include "common/posix_timer.h"

#include "common/app_log.h"
#include "device_model/system/power_manager.h"



#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG  "Closescreen.cpp"


using namespace std;
using namespace EyeseeLinux;

static Mutex sc_mutex;
static Closescreen* gcs_cs_ = NULL;

Closescreen::Closescreen()
        : close_statu_(false)
        , sec_counter_(10)
        , switch_(false)
        , m_bHdmiConnect(false)
{
  create_timer(this, &time_wifi_open_closescreen_timer_id_,ClosescreenTimerProc);

}

Closescreen::~Closescreen()
{
   delete_timer(time_wifi_open_closescreen_timer_id_);

}

Closescreen* Closescreen::GetInstance()
{
    Mutex::Autolock _l(sc_mutex);
    if(gcs_cs_ != NULL)
    {
        return gcs_cs_;
    }

    gcs_cs_ = new Closescreen();
    return gcs_cs_;
}

void Closescreen::SetDelayTime(int sec)
{
    sec_counter_ = sec;
}

void Closescreen::SetClosescreenEnable(bool onoff)
{
    switch_ = onoff;
}

void Closescreen::Start()
{
    db_msg("Closescreen start");
    if(switch_)
    {
        close_statu_ = false;
        set_one_shot_timer(sec_counter_,0,time_wifi_open_closescreen_timer_id_);
    }
}

void Closescreen::Stop()
{
    db_msg("Closescreen stop----switch_ = %d",switch_);
	Mutex::Autolock _l(sc_mutex);
    close_statu_ = false;
    if(switch_){
		db_msg("Closescreen stop---set_one_shot_timer-");
        set_one_shot_timer(sec_counter_,0,time_wifi_open_closescreen_timer_id_);
    }else{
        db_msg("Closescreen stop---stop_timer-");
        stop_timer(time_wifi_open_closescreen_timer_id_);
    }
    if(!PowerManager::GetInstance()->IsScreenOn() && !m_bHdmiConnect)
    {
        PowerManager::GetInstance()->ScreenOn();
    }
}

bool Closescreen::IsScreenClosed()
{
	Mutex::Autolock _l(sc_mutex);
    return close_statu_;
}

bool Closescreen::IsClosescreenEnable()

{
    return switch_;
}

void Closescreen::ClosescreenTimerProc(union sigval sigval)
{
    db_msg("check Closescreen status timer");
    Closescreen *pp = reinterpret_cast<Closescreen *>(sigval.sival_ptr);
    pp->ClosescreenStatusHandler();

}

void Closescreen::ClosescreenStatusHandler()
{
    //close screen
    db_msg("ClosescreenStatusHandler close screen");
	if(PowerManager::GetInstance()->IsScreenOn())
        PowerManager::GetInstance()->ScreenOff();
	Mutex::Autolock _l(sc_mutex);
    close_statu_ = true;
}

int Closescreen::SetHdmiConnect(bool p_bConnect)
{
    m_bHdmiConnect = p_bConnect;

    return 0;
}
