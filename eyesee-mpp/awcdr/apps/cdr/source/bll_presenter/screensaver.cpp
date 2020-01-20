


#include "screensaver.h"

#include "common/app_log.h"
#include "device_model/system/power_manager.h"
#include "device_model/storage_manager.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG  "screensaver.cpp"


using namespace std;
using namespace EyeseeLinux;

static Mutex sc_mutex;
static Screensaver* gscr_saver_ = NULL;

Screensaver::Screensaver()
        : saver_stu_(false)
        , sec_counter_(60)
        , saver_switch_(false)
        , m_bPause(false)
        , m_bHdmiConnect(false)
        ,m_min(false)
{
    create_timer(this, &time_check_screensaver_status_timer_id_,CheckScreensaverStatusTimerProc);
    create_timer(this, &time_count_screensaver_timer_id_,CountScreensaverTimerTimerProc);
	stop_timer(time_count_screensaver_timer_id_);
}

Screensaver::~Screensaver()
{
    delete_timer(time_check_screensaver_status_timer_id_);
    delete_timer(time_count_screensaver_timer_id_);
}

Screensaver* Screensaver::GetInstance()
{
    Mutex::Autolock _l(sc_mutex);
    if(gscr_saver_ != NULL)
    {
        return gscr_saver_;
    }

    gscr_saver_ = new Screensaver();
    return gscr_saver_;
}

void Screensaver::SetDelayTime(int sec)
{
    sec_counter_ = sec;
}

void Screensaver::SetScreensaverEnable(bool onoff)
{
    saver_switch_ = onoff;
}


void Screensaver::Start()
{
    //db_msg("screensaver start");
    if(saver_switch_)
    {
        saver_stu_ = false;
        if (m_bPause)
            db_warn("screen saver has been pause");
        else
            set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
    }
}

void Screensaver::Stop()
{
    //db_msg("screensaver stop---saver_switch_ = %d",saver_switch_);
    saver_stu_ = false;
    if(saver_switch_){
        stop_timer(time_check_screensaver_status_timer_id_);
        
        set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
    }else{
    	//db_msg("debug_zhb---->stop screen saver timer ");
        stop_timer(time_check_screensaver_status_timer_id_);
    }
    if(!PowerManager::GetInstance()->IsScreenOn())
    {
        PowerManager::GetInstance()->ScreenOn();
    }
}

bool Screensaver::GetScreensaverStatus()
{
    return saver_stu_;
}
bool Screensaver::IsScreensaverEnable()
{
    return saver_switch_;
}
void Screensaver::CheckScreensaverStatusTimerProc(union sigval sigval)
{
    Screensaver *pp = reinterpret_cast<Screensaver *>(sigval.sival_ptr);
    if( pp->m_bHdmiConnect )
    {
        db_msg("hdmi mode cannot not support close screen!\n");
        return;
    }
    pp->ScreensaverStatusHandler();
    
}

void Screensaver::ScreensaverStatusHandler()
{
   //db_msg("debug_zhb---> screensaver::reStartCount  ready to close screen by timer out");
    //close screen
    if(PowerManager::GetInstance()->IsScreenOn()){
        PowerManager::GetInstance()->ScreenOff();
	 stop_timer(time_check_screensaver_status_timer_id_);
	 CountScreenSaverTime(true);
	}
    saver_stu_ = true;
}

int Screensaver::reStartCount()
{    
//db_msg("debug_zhb---> screensaver::reStartCount  saver_switch_ = %d",saver_switch_);
    if( saver_switch_ )
    {
       if(!PowerManager::GetInstance()->IsScreenOn() && !m_bHdmiConnect)
       {
       	     //db_msg("debug_zhb---> screensaver::reStartCount  ready to screen on  by key");
            PowerManager::GetInstance()->ScreenOn();
	    CountScreenSaverTime(false);
       }else if(PowerManager::GetInstance()->IsScreenOn()){
       		//db_msg("debug_zhb---> screensaver::reStartCount  ready to close screen  by key");
		PowerManager::GetInstance()->ScreenOff();
		stop_timer(time_check_screensaver_status_timer_id_);
		CountScreenSaverTime(true);
		return 1;
       	}

       if( m_bPause )
            return 0;
	//db_msg("debug_zhb---> screensaver::reStartCount  ready to  on timer");
       set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
    }

    return 0;
}
int Screensaver::ForceReTimerStatus()
{    
//db_msg("debug_zhb---> screensaver::ForceReTimerStatus  saver_switch_ = %d",saver_switch_);

       if(!PowerManager::GetInstance()->IsScreenOn())
       {
       	    //db_msg("debug_zhb---> screensaver::ForceReTimerStatus  ready to screen on  by MODE key");
            PowerManager::GetInstance()->ScreenOn();
	    	if(saver_switch_){
				//db_msg("debug_zhb---> screensaver::ForceReTimerStatus  ready to screen on  by MODE key and set timer becuase is no keep brightness");
				if(!m_bPause)
		     			set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
	    	}
	       	CountScreenSaverTime(false);
       }
	   else if(PowerManager::GetInstance()->IsScreenOn())
       {
       		//db_msg("debug_zhb---> screensaver::ForceReTimerStatus  ready to close screen  by key");
			PowerManager::GetInstance()->ScreenOff();
			stop_timer(time_check_screensaver_status_timer_id_);
			CountScreenSaverTime(true);
       	}    
    return 0;
}

int Screensaver::ForceScreenOnBySdcard()
{    
	db_msg("debug_zhb---> screensaver::ForceScreenOnBySdcard()  saver_switch_ = %d",saver_switch_);
    if(!PowerManager::GetInstance()->IsScreenOn())
    {
    	PowerManager::GetInstance()->ScreenOn();
	   	if(saver_switch_){
	    		set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
	    }
	    CountScreenSaverTime(false);
    }else{ 
		if(saver_switch_){
			set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
		}
    }
    return 0;
}

int Screensaver::pause(bool p_bPause)
{
    //db_msg("debug_zhb---> screensaver::pause = %d",p_bPause);
if(saver_switch_){
    m_bPause = p_bPause;
    if( p_bPause)
    {
        stop_timer(time_check_screensaver_status_timer_id_);
    }
    else
    {
        //reStartCount();
        stop_timer(time_check_screensaver_status_timer_id_);
        set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
    }
}
    return 0;
}
int Screensaver::reSetTimer()
{
    //db_msg("debug_zhb---> screensaver::reSetTimer saver_switch_ = %d m_bPause = %d",saver_switch_,m_bPause);
	if(saver_switch_){
	    if(!m_bPause)
	    {
	    	//db_msg("debug_zhb---> screensaver::reSetTimer ready to reset timer by other key");
	    	stop_timer(time_check_screensaver_status_timer_id_);
	       set_one_shot_timer(sec_counter_,0,time_check_screensaver_status_timer_id_);
	    }
	}
    return 0;
}

int Screensaver::stopTimer()
{
	//db_msg("debug_zhb---> stopTimer");
	if(saver_switch_){
		stop_timer(time_check_screensaver_status_timer_id_);
	}
	return 0;
}
int Screensaver::SetHdmiConnect(bool p_bConnect)
{
    m_bHdmiConnect = p_bConnect;

    return 0;
}

bool Screensaver::GetHdmiConnect()
{
    return m_bHdmiConnect;
}


int Screensaver::CountScreenSaverTime(bool start_)
{
	//db_msg("[debug_zhb]---> CountScreenSaverTime---start_ = %d",start_);
	if(start_){
		stop_timer(time_count_screensaver_timer_id_);
		set_one_shot_timer(60,0,time_count_screensaver_timer_id_);
	}else{
		   stop_timer(time_count_screensaver_timer_id_);
		}
	return 0;
}

void Screensaver::CountScreensaverTimerTimerProc(union sigval sigval)
{
    Screensaver *pp = reinterpret_cast<Screensaver *>(sigval.sival_ptr);
    pp->CountScreensaverTimeHandler();
    
}

void Screensaver::CountScreensaverTimeHandler()
{
    stop_timer(time_count_screensaver_timer_id_);
    m_min= true;
     //db_msg("debug_zhb--->ready to do change window ,clear user do");
	StorageManager *sm =   StorageManager::GetInstance();
	sm->Update(MSG_TO_PREVIEW_WINDOW);
}

