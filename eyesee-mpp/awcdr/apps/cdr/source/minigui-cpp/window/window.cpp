												/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: window.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:
    two mode to show the window.
    normal mode: message loop runs in the Application
    modal mode : window loops processing the message itself in Window::run()
 History:
*****************************************************************************/
//#define NDEBUG

#include "window/window.h"
#include "window/user_msg.h"
#include "window/shutdown_window.h"
#include "application.h"
#include "debug/app_log.h"
#include "common/app_def.h"
#include "device_model/system/power_manager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"
#include "device_model/system/event_manager.h"
#include "parser/main_parser.h"
#include "resource/resource_manager.h"
#include "widgets/text_view.h"
#include "window/window_manager.h"
#include "device_model/dialog_status_manager.h"
#include "bll_presenter/AdapterLayer.h"
#include "dd_serv/common_define.h"
#include "common/buttonPos.h"
#include "window/status_bar_bottom_window.h"
#include "window/playback_window.h"
#include "window/preview_window.h"
#include "device_model/storage_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "dd_serv/dd_AGPS.h"
//#include "uberflow.h"
#include "bll_presenter/statusbarsaver.h"
#include "device_model/httpServer/httpServer.h"


#undef LCD_SHOW_DEBUG

#ifdef LCD_SHOW_DEBUG
int is_change_time = 1;
char timebuf_changetime[512]= {0};
char reboot_msg[512]= {0};
extern int repower_4g_count;
extern int reinit_4g_count;
extern char ip_4g[32];
extern int is_4g_need_reboot;
extern int is_mqtt_connect;
int show_times = 30;
std::string decode_username;
int is_got_username = 0;
int show_interval = 15;
#endif

#define ID_NOTICE    180
#define ID_TIMER  110
#define ID_MOUSELONGPRESS_TIMER  111
static HWND  Hwnd_notice = HWND_INVALID;


using namespace std;
using namespace EyeseeLinux;

IMPLEMENT_DYNCRT_CLASS(Window)

//@variable
#define QRC_PATH "/usr/share/minigui/res/layout/"

std::mutex Window::key_proc_mutex_;
std::mutex Window::GlobalKeyBlocker::global_key_mutex_;
bool Window::GlobalKeyBlocker::global_key_blocked_ = false;

Window::Window(IComponent *parent)
    : ContainerWidget(NULL)
    , parent_(parent)
    , listener_(NULL)
    , isKeyUp(true)
    , mode_(WINDOW_NORMAL)
{
    is_visible_ = false;
}

Window::~Window()
{
}

/*****************************************************************************
 Function: Window::CreateWidget
 Description: create main window
 Parameter: -
 Return: -
*****************************************************************************/
void Window::CreateWidget()
{
    MAINWINCREATE CreateInfo;
    HWND hOwner;
    CommonCreateParams params;
    memset( &params, 0, sizeof(params) );
    GetCreateParams( params );
    db_msg("create window: %s", params.alias);

    Application *app = Application::GetApp();
    if (parent_) {
        hOwner = ( dynamic_cast<IComponent*>( parent_) )->GetHandle();
    } else {
        hOwner = app->GetHandle();
    }
    CreateInfo.dwStyle = params.style;
    CreateInfo.dwStyle &= ~WS_VISIBLE;
    CreateInfo.dwExStyle = params.exstyle;
    CreateInfo.spCaption = params.class_name;
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = WindowProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = 0;
    CreateInfo.by = 0;
    CreateInfo.iBkColor = GetWindowElementColor(WE_BGC_WINDOW) ;
    CreateInfo.dwAddData = (DWORD)this;
    CreateInfo.hHosting = hOwner;
    handle_ = ::CreateMainWindow( &CreateInfo );
    assert(handle_ != HWND_INVALID);
    SetBackColor(0x00ffffff);
    SetWindowAdditionalData2(handle_,(DWORD)this );
    GetWindowRect( handle_, &bound_rect_);
    if ( OnCreate )
        OnCreate( this );
}

static int handelTouchMsg(int x, int y, int current_win_id, Window *form_)
{
    int ret = 0;
    if(form_ == NULL)
        return -1;
    //preview windown button
    if(current_win_id == WINDOWID_PREVIEW && (long long )WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM) == (long long)form_)
    {
        //if current window is preview buf touch range in the statusbarbottom ,should send key msg to preview window
         Window *form_pre = NULL;
         StatusBarBottomWindow *form_sbw = NULL;
         buttonPos_ PBPos[PrviewButtonPosLen];
         form_pre =  WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW);
         form_sbw = (StatusBarBottomWindow *)WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
         form_sbw->GetPreviewButtonPos(PBPos, PrviewButtonPosLen);
         int id = getTouchPosID(x,y,PBPos,PrviewButtonPosLen);
         //db_warn("[habo]---> id = %d ",id);
         #ifdef PHOTO_MODE
		 PreviewWindow* pvw = static_cast<PreviewWindow*>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
		 int status = pvw->GetWindowStatus();
		 #endif
         switch(id)
         {
            case PREVIEW_BUTTON_POS_RECORD_ID:
            {
				#ifdef PHOTO_MODE
				if (status == STATU_PREVIEW) 
				#endif
				{
					form_pre->keyProc(SDV_KEY_MENU,SHORT_PRESS);
					if (!pvw->GetisRecordStart())
                		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
				}
            }
            break;
            case PREVIEW_BUTTON_POS_PHOTO_ID:
            {
                form_pre->keyProc(SDV_KEY_OK,SHORT_PRESS);
//                AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
            }
            break;
            case PREVIEW_BUTTON_POS_LOCKED_ID:
            {
				#ifdef PHOTO_MODE
				if (status == STATU_PREVIEW) 
				#endif
				{
					form_pre->keyProc(SDV_KEY_MODE,SHORT_PRESS);
					if (!pvw->GetisRecordStart())
                		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
				}
            }
            break;
            case PREIVEW_BUTTON_POS_PLAYBACK_ID:
            {
				#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
				EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
				#endif
                form_pre->keyProc(SDV_KEY_RIGHT,SHORT_PRESS);//playback
                if (!pvw->GetisRecordStart())
                	AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
            }
            break;
            case PREVIEW_BUTTON_POS_SETTING_ID:
            {
				#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
				EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
				#endif
                form_pre->keyProc(SDV_KEY_LEFT,SHORT_PRESS);//settting 
                if (!pvw->GetisRecordStart())
                	AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
            }
            break;
			case PREVIEW_BUTTON_POS_VOICE_ID:
			{
				#ifdef PHOTO_MODE
				if (status == STATU_PREVIEW) 
				#endif
				{
					form_pre->keyProc(SDV_KEY_RETURN,SHORT_PRESS);
					if (!pvw->GetisRecordStart())
                		AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
				}
            }
            break;
            default :
            {
                db_warn("[Habo] handeltouchmsg error !!!");
                ret = -1;
            }
            break;
         }
    }
    else if(current_win_id == WINDOWID_PLAYBACK && (long long )WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM) == (long long)form_)
    {
         PlaybackWindow *form_pback = NULL;
         StatusBarBottomWindow *form_sbw = NULL;
         form_pback =(PlaybackWindow *)WindowManager::GetInstance()->GetWindow(WINDOWID_PLAYBACK);
         form_sbw = (StatusBarBottomWindow *)WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
         if(form_pback->IsPlayingWindow() && !form_pback->IsDeleteDialogWindow())
         {
            buttonPos_ PBPos[PlayingButtonPosLen];
            form_sbw->GetPreviewButtonPos(PBPos, PlayingButtonPosLen);
             int id = getTouchPosID(x,y,PBPos,PlayingButtonPosLen);
             db_warn("[habo]--->playing window  id = %d ",id);
             switch(id)
             {
                case PLAYING_BUTTON_POS_RETURN_ID:
                {
                    form_pback->keyProc(SDV_KEY_MENU,SHORT_PRESS);
                    //AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYING_BUTTON_POS_PREVIOUS_ID:
                {
                    form_pback->keyProc(SDV_KEY_OK,SHORT_PRESS);
                   // AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYING_BUTTON_POS_NEXT_ID:
                {
                    form_pback->keyProc(SDV_KEY_MODE,SHORT_PRESS);
                    //AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYING_BUTTON_POS_DELETE_ID:
                {
                    form_pback->keyProc(SDV_KEY_RIGHT,SHORT_PRESS);//playback
                    //AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYING_BUTTON_POS_START_PAUSE_ID:
                {
                    form_pback->keyProc(SDV_KEY_LEFT,SHORT_PRESS);//settting 
                    //AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                default :
                {
                    db_warn("[Habo] handeltouchmsg error !!!");
                    ret = -1;
                }
                break;
             }
         }
         else if(!form_pback->IsPlayingWindow() && (long long )WindowManager::GetInstance()->GetWindow(WINDOWID_STATUSBAR_BOTTOM) == (long long)form_)
         {
             buttonPos_ PBPos[PlayBackButtonPosLen];
             form_sbw->GetPlayBackButtonPos(PBPos, PlayBackButtonPosLen);
             int id = getTouchPosID(x,y,PBPos,PlayBackButtonPosLen);
             switch(id)
             {
                case PLAYBACK_BUTTON_POS_PREVIOUS_ID:
                {
                    form_pback->keyProc(SDV_KEY_OK,SHORT_PRESS);//previous 
                    AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYBACK_BUTTON_POS_LOCKED_ID:
                {
                    form_pback->keyProc(SDV_KEY_MODE,SHORT_PRESS);//lock
                    AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                case PLAYBACK_BUTTON_POS_NEXT_ID:
                {
                    form_pback->keyProc(SDV_KEY_RIGHT,SHORT_PRESS);//next
                    AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
                }
                break;
                default :
                {
                    db_warn("[Habo] handeltouchmsg error !!!");
                    ret = -1;
                }
                break;
             }
         }
    }

    return ret;
}
/*****************************************************************************
 Function: Window::WindowProc
 Description: the default window process function.
    if there is no special messages to process, it will handle to HandleMessage
 Parameter:
 Return:
*****************************************************************************/
long int Window::WindowProc( HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam )
{
    Window *form = NULL;
    int ret = DO_IT_MYSELF;
    static bool ignore = false;
    static bool long_press = false;
	static int saveX = -1;
	static int saveY = -1;
	static int moveflag = 0;
    static bool isShutdown = false;
    int window_id = 0;
    if (msg == MSG_NCCREATE) {
        form = reinterpret_cast<Window*>((( PMAINWINCREATE)lparam )->dwAddData);
        ::SetWindowAdditionalData2( hwnd, (DWORD)form );
    } else {
        form = reinterpret_cast<Window*>(::GetWindowAdditionalData2(hwnd));
    }
//    form = reinterpret_cast<Window*>(::GetWindowAdditionalData2(hwnd));
#ifdef LCD_SHOW_DEBUG
    if(show_interval-- <=0) {
        show_interval= 15;
        db_debug("agps_readcallback_data_size:%d,", agps_readback_data_size);
    }
#endif
    if (form) {
        switch (msg) {
#ifdef LCD_SHOW_DEBUG
			case MSG_CREATE:
				
				//if(Hwnd_notice == HWND_INVALID)
				{
					Hwnd_notice =  CreateWindow(CTRL_STATIC, NULL,
					WS_CHILD | WS_VISIBLE | SS_LEFT|WS_EX_TRANSPARENT, ID_NOTICE,
					0, 55, 600, 440,
					hwnd, 0);
					SetWindowBkColor(Hwnd_notice, COLOR_transparent);
					ShowWindow(Hwnd_notice, SW_SHOW);
					SetWindowBkColor(Hwnd_notice, 0x00000000);
					SetTimer(Hwnd_notice, ID_TIMER, 100);  
				}
			//	ret = DO_IT_MYSELF;
				break;
			case MSG_TIMER:
                {
					EventManager *m_EventManager = EventManager::GetInstance();
					LocationInfo_t locationinfo;
                    std::string username;
                    std::string secret;
					char strcnt[1024]={0};
                    if(is_got_username == 0) {
                        AdapterLayer::GetInstance()->getUserInfo(username, secret);
                        //db_error("username:%s", username.c_str());
                        if(strcmp(username.c_str(), "") != 0)
                        {
                            decode_username.clear();
                            decode_username = AdapterLayer::GetInstance()->base64_decode(username);
                            //db_error("decode_username:%s", decode_username.c_str());

                            is_got_username = 1;
                        }
                    }

                    time_t timer;//time_t就是long int 类型
                    struct tm *t;
                    timer = time(NULL);
                    t = localtime(&timer);
                    if(t->tm_mon + 1 != 7 && is_change_time) {
                        is_change_time = 0;
                        snprintf(timebuf_changetime, sizeof(timebuf_changetime),"%4d年%02d月%02d日 %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
                    }
                    if(is_4g_need_reboot) {
                        strncpy(reboot_msg, "net is ok,auto reboot to retest...", sizeof(reboot_msg));
                        if(show_times-- <= 0) {
                            system("reboot -f");
                        }
                    }
                    
					m_EventManager->getLocationInfo(locationinfo);
					//snprintf(strcnt,sizeof(strcnt)," singnalcnt:%d  agps_data_size:%d  usrname:%s\n %s\n 4G repower:%d reinit:%d ip:%s\n is_mqtt_connect:%d\n %s", locationinfo.GpsSignal, agps_readback_data_size, decode_username.c_str(), timebuf_changetime, repower_4g_count, reinit_4g_count, ip_4g, is_mqtt_connect, reboot_msg);
					//SampleYiAdasContext::GetInstance()->YiAdasGetCurrentDataStr(strcnt);
                                    //snprintf(strcnt,sizeof(strcnt),"alertCount: %d\n speed: %f \n objectNum: %d \n severity_status: %d\n distance: %d \n lane_type: %d \n left_warning: %d \n right_warning: %d \n",adasData.alertCount,adasData.speed,adasData.objectNum,adasData.severity_status,adasData.distance,adasData.lane_type,adasData.left_warning,adasData.right_warning);
					R* r = R::get();
					SetWindowFont(Hwnd_notice, r->GetFont());
					ShowWindow(Hwnd_notice, SW_SHOW);
					SetDlgItemText(hwnd, ID_NOTICE, strcnt);
				}
				//ret = DO_IT_MYSELF;
				break;
#endif	
			case MSG_TIMER:
			 	{
		 	      if(wparam == ID_MOUSELONGPRESS_TIMER) {
      				  	KillTimer(hwnd, ID_MOUSELONGPRESS_TIMER);
						if (!moveflag) {
				  			::SendMessage(hwnd, MSG_MOUSE_LONGPRESS, 0, 0 ); // 在minigui中发出的MSG_MOUSE_LONGPRESS消息无法收到,改为此方式实现
						}
         		   }
			 	}
			      break;
			case MSG_LBUTTONDOWN :
			    {
					saveX = LOWORD(lparam);
					saveY = HIWORD(lparam);
					moveflag = 0;
					if(!IsTimerInstalled(hwnd, ID_MOUSELONGPRESS_TIMER)){
						SetTimer(hwnd, ID_MOUSELONGPRESS_TIMER, 200);
					}
    				if(PowerManager::GetInstance()->IsScreenOn())
    					EyeseeLinux::Screensaver::GetInstance()->reSetTimer();
    				else
    					EyeseeLinux::Screensaver::GetInstance()->ForceReTimerStatus();
					#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                    EyeseeLinux::StatusBarSaver::GetInstance()->StatusBarSaverCtl();
					#endif
					db_error("MSG_LBUTTONDOWN");
				}break;
			case MSG_MOUSEMOVE :
				if (!moveflag) {
					int xx = LOWORD(lparam);
					int yy = HIWORD(lparam);
					if (abs(xx - saveX)>20 || abs(yy - saveY)>20) {
						moveflag = 1;
					}
				}
				break;
			case MSG_LBUTTONUP :
				if(IsTimerInstalled(hwnd, ID_MOUSELONGPRESS_TIMER)){
						KillTimer(hwnd, ID_MOUSELONGPRESS_TIMER);
						moveflag = 0;
				}
				db_error("MSG_LBUTTONUP");
                handelTouchMsg(LOWORD(lparam),HIWORD(lparam),WindowManager::GetInstance()->GetCurrentWinID(),(Window *)form);
				break;
            case MSG_INITDIALOG :
                    ret = HELP_ME_OUT;
                break;
            case MSG_CLOSE :
                    ret = DO_IT_MYSELF;
                break;
            case MSG_DESTROY:
                    // if( form->OnDestroy )
                        // form->OnDestroy( form );
                    ret = HELP_ME_OUT;
                break;
            case MSG_KEYUP:
                {
                      if(httpServer::GetInstance()->GetAppConnectedFlag())
                    {
                        db_warn("the app is connected don't respone anything\n");
                        return DO_IT_MYSELF;
                    }
					if(EventManager::GetInstance()->sdcardFlag)
					{
						if(!MediaFileManager::GetInstance()->DataBaseIsAlready()
							&& StorageManager::GetInstance()->GetStorageStatus() == MOUNTED) {
							db_warn("sd card is not ready don't resopone");
							key_proc_mutex_.unlock();
							return DO_IT_MYSELF;
						}
					}
                    db_warn("MSG_KEYUP wparam %d!!",wparam);
                    if( PowerManager::GetInstance()->getStandbyFlag() )
                    {
                        db_warn("system is standby mode, ignore this message");
                        key_proc_mutex_.unlock();
#if 1
                        if(EventManager::GetInstance()->mWakeup_event_flag == false) {
                          //  db_error("==============go to CloseStandbyDialog==============");
							if(wparam != SDV_KEY_POWER){
							    PowerManager::GetInstance()->CloseStandbyDialog();
							}
                        }
#endif
                        return DO_IT_MYSELF;
                    }
                    if(DialogStatusManager::GetInstance()->getMDialogEventFinish() == false)
                    {
                        db_warn("dialog event no finish, ignore this message");
                        key_proc_mutex_.unlock();
                        return DO_IT_MYSELF;
                    }
                    window_id = WindowManager::GetInstance()->GetCurrentWinID();
                    form = WindowManager::GetInstance()->GetWindow(window_id);
//					db_warn("window_id %d, form %p,isKeyUp %d",window_id,form,form->isKeyUp);
					form->isKeyUp = true;
					
					if(PowerManager::GetInstance()->IsScreenOn()) {
						if (!ignore) {
							//db_warn("call form keyProc");
							form->keyProc(wparam, SHORT_PRESS);
						}
					}
                    
                    if (!form->GetVisible()) {
//                        db_warn("window '%p' is hiden, ignore key event", form);
                        window_id = WindowManager::GetInstance()->GetCurrentWinID();
                        Window *win = WindowManager::GetInstance()->GetWindow(window_id);
                        db_debug("current win id: %d, address: %p, handle: %p, current active win: %p", window_id, win, win->GetHandle(), ::GetActiveWindow());
                        long_press = false;
						key_proc_mutex_.unlock();
                        break;
                    }
					if (long_press) {
						long_press = false;
						key_proc_mutex_.unlock();
                        break;
					}
					
                    if (wparam == SDV_KEY_POWER && !isShutdown) //进入关机流程后不响应锁屏操作
                    {
                        //db_error("[debug_zhb]----> long key to swicth the screen status ");
                        if(PowerManager::GetInstance()->getACconnectStatus()){
							// 开关
                            EyeseeLinux::Screensaver::GetInstance()->ForceReTimerStatus();
                        }
                    }
					
                }
                break;
            case MSG_KEYDOWN:
			{
                 if(httpServer::GetInstance()->GetAppConnectedFlag())
                {
                        db_warn("the app is connected don't respone anything\n");
                        return DO_IT_MYSELF;
                }
				if(EventManager::GetInstance()->sdcardFlag)
				{
					db_warn("sd card is not ready don't resopone");     
                    key_proc_mutex_.unlock();
					return DO_IT_MYSELF;
				}
				if( PowerManager::GetInstance()->getStandbyFlag())
				{
					db_warn("system is standby mode, ignore this message");
					return DO_IT_MYSELF;
				}
				if(DialogStatusManager::GetInstance()->getMDialogEventFinish() == false)
				{
					db_warn("dialog event no finish, ignore this message");
					return DO_IT_MYSELF;
				}
				if(PowerManager::GetInstance()->IsScreenOn())
    				EyeseeLinux::Screensaver::GetInstance()->reSetTimer();
    			
				ignore = false;
				window_id = WindowManager::GetInstance()->GetCurrentWinID();
				form = WindowManager::GetInstance()->GetWindow(window_id);
//				db_warn("form %p,isKeyUp %d",form,form->isKeyUp);
                if (form->isKeyUp) {
                    // send to CommonKeyProc to play keytone
                    if (form->listener_ != NULL) {
                    	 db_debug("MSG_KEYDOWN");
                        if (key_proc_mutex_.try_lock() == false) {
                            db_warn("get key proc mutex failed");
							return DO_IT_MYSELF;
                        } else {
                            ignore = form->listener_->sendmsg(form, msg, wparam);
                            key_proc_mutex_.unlock();
                        }
                    } else {
                        db_warn("listener_ is NULL, can not dispatch msg: %d, from win: %p", msg, form);
                    }
                }
                form->isKeyUp = false;
                break;
            }
            case MSG_KEYLONGPRESS:
			{
				db_debug("MSG_KEYLONGPRESS");
				if( PowerManager::GetInstance()->getStandbyFlag() )
				{
					db_warn("system is standby mode, ignore this message");
					return DO_IT_MYSELF;
				}

                long_press = true;
                if (wparam == SDV_KEY_POWER && !isShutdown)
				{
                    isShutdown = true;
                    db_error("power key long press!!!");
                    form = WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW);
                    form->keyProc(wparam, LONG_PRESS);
                }
				//else 
                //	form->keyProc(wparam, LONG_PRESS);
                form->isKeyUp = false; // by hero fix longpress Progress_bar
                break;
        	}
            case MSG_ISDIALOG :
                ret = HELP_ME_OUT;
                break;
            default:
                break;

        }
    }
    if( form )
    {
        ret = form->HandleMessage( hwnd, msg, wparam, lparam );
    }
    else
    {
        db_warn("form is null, msg is 0x%x",msg);
    }

    if (ret == HELP_ME_OUT) {
        return DefaultMainWinProc( hwnd, msg, wparam, lparam );
    } else {
        return DO_IT_MYSELF;
    }
}

void Window::keyProc(int keyCode, int isLongPress)
{
    db_debug("keyCode: %d, isLongPress: %d", keyCode, isLongPress);
}

/*****************************************************************************
 Function: Window::DoShow
 Description: show window in normal mode
 Parameter:
 Return:
*****************************************************************************/
void Window::DoShow()
{
    mode_ = WINDOW_NORMAL;
    SetVisible( true );
}

/*****************************************************************************
 Function: Window::DoShowModal
 Description: show window in modal mode.
 Parameter:
 Return:
*****************************************************************************/
int Window::DoShowModal()
{
    HWND hOwner;
    MSG Msg;
    HWND hFocus;
    mode_ = WINDOW_MODAL;
    hOwner = HWND_DESKTOP;
    SetVisible( true );
    run();
    MainWindowCleanup( GetHandle() );
    return 0;
}

void Window::SetVisible(bool new_value)
{
    ContainerWidget::SetVisible( new_value );
    if ( new_value ) {
        ::SetActiveWindow(GetHandle());
    }
}

/*****************************************************************************
 Function: Window::run
 Description: will be called in modal mode
 Parameter:
 Return:
*****************************************************************************/
void Window::run()
{
    MSG msg;
    while ( ::GetMessage( &msg, GetHandle() ) )
    {
        ::TranslateMessage( &msg );
        ::DispatchMessage( &msg );
    }
}

CtrlMap& Window::GetCtrlMap()
{
    return ctrl_map_;
}

StringMap& Window::GetTextMap()
{
    return text_map_;
}

void Window::LoadResComplete()
{
}


/*****************************************************************************
 Function: Window::Load
 Description: loading .qrc resource ,setting all textviews' caption and
    setting the window's font
 Parameter:
 Return:
*****************************************************************************/
void Window::Load()
{
    ParserBase* m_ResParse = NULL;

    m_ResParse = GetResParse();
    if (!m_ResParse)
        return;

    LoadRes(m_ResParse);
    LoadResComplete();

    delete m_ResParse;
    m_ResParse = NULL;
    
    R* r = R::get();
    string images_file(QRC_PATH);
    images_file += GetResourceName();
    images_file += ".qrc";
    db_msg("images_file :%s", images_file.c_str());
    r->LoadImages(images_file);
    r->LoadStrings(text_map_);
    FillTextView();
    ::SetWindowFont(GetHandle(), r->GetFont());
}


/*****************************************************************************
 Function: Window::FillTextView
 Description: set textviews' caption
 Parameter:
 Return:
*****************************************************************************/
void Window::FillTextView()
{
    TextView* view;
    string ctrl_name;
    for(StringMap::iterator iter = text_map_.begin(); iter != text_map_.end(); iter++) {
        ctrl_name = iter->first;
        view = reinterpret_cast<TextView *>
            (GetControl((char*)ctrl_name.c_str()));
        if (view) {
            view->SetCaption(text_map_[ctrl_name].c_str());
        }
    }
}

ParserBase* Window::GetResParse()
{
    return new MainParser();
}

/*****************************************************************************
 Function: Window::GetResourceName
 Description:
    @descendant
 Parameter:
 Return:
*****************************************************************************/
string Window::GetResourceName()
{
    return string(GetClassName());
}

/*****************************************************************************
 Function: Window::GetControl
 Description: find out the View pointer by the widget name
 Parameter:
 Return:
    the View's pointer
*****************************************************************************/
View* Window::GetControl(const char *widget_name)
{
    CtrlMap::iterator iter = ctrl_map_.find(string(widget_name));

    if (iter != ctrl_map_.end()) {
        return iter->second;
    } else
        db_error("can not find widget by name '%s'", widget_name);

    return NULL;
}

Window* Window::GetParentWindow()
{
    return static_cast<Window*>(parent_);
}

void Window::Update(MSG_TYPE msg,  int p_CamId, int p_recordId)
{
    //@todo
}

void Window::OnLanguageChanged()
{
}


/*****************************************************************************
 Function: Window::LoadRes
 Description: call the parser to parse resources
 Parameter:
 Return:
*****************************************************************************/
void Window::LoadRes(ParserBase* resParse)
{
    if (resParse) {
        db_msg("start parse ui resources");
        resParse->SetupUi(this);
    }
}


IMPLEMENT_DYNCRT_CLASS(SystemWindow);
SystemWindow::SystemWindow(IComponent *parent)
    : Window(parent)
    , bkimg_used_(false)
    , win_bkgnd_bmp_(NULL)
{
    pthread_mutex_init(&bmp_lock_, NULL);
}


SystemWindow::~SystemWindow()
{
    if (win_bkgnd_bmp_) {
        pthread_mutex_lock(&bmp_lock_);
        UnloadBitmap(win_bkgnd_bmp_);
        pthread_mutex_unlock(&bmp_lock_);
    }

    pthread_mutex_destroy(&bmp_lock_);
}

void SystemWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE;
    params.class_name = " ";
    params.alias      = strdup(GetResourceName().c_str());
}


/*****************************************************************************
 Function: SetWindowBackImage
 Description: set window background image
 Parameter: bmp image file path
 Return:
*****************************************************************************/
void SystemWindow::SetWindowBackImage(const char *bmp)
{
    if (bmp == NULL) {	// clear current BackImage
        if (win_bkgnd_bmp_) {
            db_info("unload window background image");

            pthread_mutex_lock(&bmp_lock_);
            UnloadBitmap(win_bkgnd_bmp_);
			free(win_bkgnd_bmp_);
			win_bkgnd_bmp_ = NULL;
            pthread_mutex_unlock(&bmp_lock_);
            db_msg("------ready to refresh----------");
            Refresh();
        } else
            db_error("failed, bmp = null");
        return;
    }

    if (win_bkgnd_bmp_ == NULL) {
        win_bkgnd_bmp_ = (BITMAP*)malloc(sizeof(BITMAP));
    } else {
        pthread_mutex_lock(&bmp_lock_);
        UnloadBitmap(win_bkgnd_bmp_);
        pthread_mutex_unlock(&bmp_lock_);
    }

    pthread_mutex_lock(&bmp_lock_);
    LoadBitmap(HDC_SCREEN, win_bkgnd_bmp_, bmp);
    pthread_mutex_unlock(&bmp_lock_);

    bkimg_used_ = true;
    db_msg("------ready to refresh");
    Refresh();
}

int SystemWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam,
                                        LPARAM lparam)
{
    switch ( message ) {
        case MSG_ERASEBKGND: {
                // 用于填充窗口背景图片
                HDC hdc = (HDC)wparam;
                const RECT* clip = (const RECT*) lparam;
                BOOL fGetDC = FALSE;
                RECT rcTemp;

                if (hdc == 0) {
                    hdc = GetClientDC (hwnd);
                    fGetDC = TRUE;
                }

                if (clip) {
                    rcTemp = *clip;
                    ScreenToClient (hwnd, &rcTemp.left, &rcTemp.top);
                    ScreenToClient (hwnd, &rcTemp.right, &rcTemp.bottom);
                    IncludeClipRect (hdc, &rcTemp);
                }
                else
                    GetClientRect (hwnd, &rcTemp);

                if (win_bkgnd_bmp_ != NULL) {
                    pthread_mutex_lock(&bmp_lock_);
                    FillBoxWithBitmap (hdc, 0, 0,
                            RECTW(rcTemp), RECTH(rcTemp), win_bkgnd_bmp_);
                    pthread_mutex_unlock(&bmp_lock_);
                } else {
                    // 清除无效区域, 如果没有为窗口设置背景图片，则以透明背景填充
                    SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0xFF, 0xFF, 0x00));
                    FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));
                }

                if (fGetDC)
                    ReleaseDC (hdc);
                return 0;
            }
      default:
          return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
   }
}


