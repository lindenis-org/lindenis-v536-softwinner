


#include "statusbarsaver.h"
#include "common/app_log.h"
#include "window/status_bar_bottom_window.h"
#include "window/window_manager.h"
#include "device_model/display.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG  "statusbarsaver.cpp"


using namespace std;
using namespace EyeseeLinux;

static Mutex sc_mutex;
static StatusBarSaver* gscr_saver_ = NULL;

StatusBarSaver::StatusBarSaver()
        : saver_stu_(false)
        ,sec_counter_(5)
        , saver_enable(false)
        , m_bPause(false)
{

    create_timer(this, &statusbarsaver_timer_id,StatusBarSaverTimerProc);
}

StatusBarSaver::~StatusBarSaver()
{
    delete_timer(statusbarsaver_timer_id);
}

StatusBarSaver* StatusBarSaver::GetInstance()
{
    Mutex::Autolock _l(sc_mutex);
    if(gscr_saver_ != NULL)
    {
        return gscr_saver_;
    }

    gscr_saver_ = new StatusBarSaver();
    return gscr_saver_;
}

void StatusBarSaver::SetDelayTime(int sec)
{
    sec_counter_ = sec;
}

void StatusBarSaver::SetStatusBarSaverEnable(bool onoff)
{
    saver_enable = onoff;
}


void StatusBarSaver::Start()
{
    if(saver_enable)
    {
        saver_stu_ = false;
        if (m_bPause)
            db_warn("statusbar saver has been pause");
        else
            set_one_shot_timer(sec_counter_,0,statusbarsaver_timer_id);
    }
}

void StatusBarSaver::Stop()
{
    saver_stu_ = false;
    if(saver_enable){
        stop_timer(statusbarsaver_timer_id);
    }
}

void StatusBarSaver::StatusBarSaverTimerProc(union sigval sigval)
{
    StatusBarSaver *sbs = reinterpret_cast<StatusBarSaver *>(sigval.sival_ptr);
    sbs->StatusBarSaverTimerHandler();
    
}

void StatusBarSaver::StatusBarSaverTimerHandler()
{
//    db_warn("now is the time to do hide StatusBarBottomWindow !!!");
    //sendmsg to stabarbottomwindow to do hide 
    WindowManager *win_mg_ = ::WindowManager::GetInstance();
    StatusBarBottomWindow * sbb = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    sbb->DoHide();
	stop_timer(statusbarsaver_timer_id);
    saver_stu_ = true;
}


int StatusBarSaver::pause(bool p_bPause)
{
    db_msg("debug_zhb---> screensaver::pause = %d",p_bPause);
    if(saver_enable){
        m_bPause = p_bPause;
        if( p_bPause)
        {
            stop_timer(statusbarsaver_timer_id);
        }
        else
        {
            stop_timer(statusbarsaver_timer_id);
            set_one_shot_timer(sec_counter_,0,statusbarsaver_timer_id);
        }
    }
    return 0;
}
int StatusBarSaver::reSetTimer()
{
	if(saver_enable){
	    if(!m_bPause)
	    {
	       stop_timer(statusbarsaver_timer_id);
	       set_one_shot_timer(sec_counter_,0,statusbarsaver_timer_id);
	    }
	}
    return 0;
}

int StatusBarSaver::StatusBarSaverCtl()
{
    WindowManager *win_mg_ = ::WindowManager::GetInstance();
    StatusBarBottomWindow * bottom_bar_ = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    if(saver_enable){
        if(!m_bPause)//preview window
        {
            db_msg("StatusBarSaver: ready to do show statusbarbottom !!!");
            if(!bottom_bar_->GetVisible())
            {
                bottom_bar_->Show();
            }
            reSetTimer();//reset the timer
        }
    }
    return 0;
}
