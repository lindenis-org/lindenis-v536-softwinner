#include "autoshutdown.h"

#include "common/app_log.h"
#include "device_model/system/power_manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG  "autoshutdown.cpp"


using namespace std;
using namespace EyeseeLinux;

static Mutex as_mutex;
static Autoshutdown* gauto_shutdown_ = NULL;

Autoshutdown::Autoshutdown()
        : saver_stu_(false)
        , sec_counter_(60)
        , autoshutdown_switch_(false)
{
    create_timer(this, &time_check_autoshutdown_status_timer_id_,CheckAutoshutdownStatusTimerProc);   
}

Autoshutdown::~Autoshutdown()
{
    delete_timer(time_check_autoshutdown_status_timer_id_);
}

Autoshutdown* Autoshutdown::GetInstance()
{
    Mutex::Autolock _l(as_mutex);
    if(gauto_shutdown_ != NULL)
    {
        return gauto_shutdown_;
    }

    gauto_shutdown_ = new Autoshutdown();
    return gauto_shutdown_;
}

void Autoshutdown::SetDelayTime(int sec)
{
    sec_counter_ = sec;
}

void Autoshutdown::SetAutoshutdownEnable(bool onoff)
{
    autoshutdown_switch_ = onoff;
}


void Autoshutdown::Start()
{
    db_msg("Autoshutdown start");
    if(autoshutdown_switch_)
    {
        saver_stu_ = false;
        set_one_shot_timer(sec_counter_,0,time_check_autoshutdown_status_timer_id_);
    }
}

void Autoshutdown::Stop()
{
    db_msg("Autoshutdown stop");
    
    if(autoshutdown_switch_){
        saver_stu_ = false;
        stop_timer(time_check_autoshutdown_status_timer_id_);
    }
    
}

bool Autoshutdown::GetAutoshutdownStatus()
{
    return saver_stu_;
}
bool Autoshutdown::IsAutoshutdownEnable()
{
    return autoshutdown_switch_;
}
void Autoshutdown::CheckAutoshutdownStatusTimerProc(union sigval sigval)
{
    db_msg("check Autoshutdown status timer");
    Autoshutdown *pp = reinterpret_cast<Autoshutdown *>(sigval.sival_ptr);
    pp->AutoshutdownStatusHandler();
    
}

void Autoshutdown::AutoshutdownStatusHandler()
{
    //close screen
    db_msg("AutoshutdownStatusHandler close screen");
    if(PowerManager::GetInstance()->getACconnectStatus())
    {
        db_warn("ac is connect, should not shutdown system, restart count shutdown system time");
        stop_timer(time_check_autoshutdown_status_timer_id_);
        set_one_shot_timer(sec_counter_,0,time_check_autoshutdown_status_timer_id_);
        return ;
    }
    this->Notify(MSG_SYSTEM_POWEROFF);
    saver_stu_ = true;
}
