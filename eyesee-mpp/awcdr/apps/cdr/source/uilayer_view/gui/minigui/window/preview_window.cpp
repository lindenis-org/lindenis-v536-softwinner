/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file preview_window.cpp
 * @brief 鍗曡矾棰勮褰曞儚绐楀彛
 * @author id:826
 * @version v0.3
 * @date 2016-11-03
 */

//#define NDEBUG
#include "window/preview_window.h"
#include "window/usb_mode_window.h"
#include "window/playback_window.h"
#include "window/status_bar_bottom_window.h"
#include "window/user_msg.h"
#include "window/dialog.h"
#include "debug/app_log.h"
#include "widgets/graphic_view.h"
#include "resource/resource_manager.h"
//#include "widgets/button.h"
#include "widgets/text_view.h"
#include "widgets/switch_button.h"
//#include "widgets/progress_bar.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "application.h"
#include "window/shutdown_window.h"
#include "bll_presenter/closescreen.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/camRecCtrl.h"
#include "bll_presenter/newPreview.h"

#include "window/prompt.h"
#include "device_model/storage_manager.h"
#include "device_model/system/power_manager.h"
#include "device_model/menu_config_lua.h"
#include "common/setting_menu_id.h"
#include <sstream>
#include <thread>
#include "window/setting_window.h"
#include "window/promptBox.h"
#include "window/bulletCollection.h"
#include "device_model/version_update_manager.h"
#include "device_model/system/event_manager.h"
//#include "bll_presenter/AdapterLayer.h"
#include <signal.h>
#include <pthread.h>
#include "device_model/system/watchdog.h"
#include "device_model/download_4g_manager.h"
#include "window/newSettingWindow.h"
#include "bll_presenter/audioCtrl.h"
#include "bll_presenter/statusbarsaver.h"

#undef LOG_TAG
#define LOG_TAG "PreviewWindow"
//#define S_I_O_RECORD_STRING //whether to display the recording string inside or outside the car
using namespace std;
using namespace EyeseeLinux;

#define KEEPALIVE_TIMER_ID 1000
#define CHECKACC_TIMER_ID 1100

static bool hide_flag = false;

IMPLEMENT_DYNCRT_CLASS(PreviewWindow)

int debug_get_info()
{
       char line[128];
       FILE *fp = fopen("/proc/meminfo", "r");
       char num[32]="0";
       if(fp==NULL){
        //printf("open file failed.");
        return 0;
       }
       while(fgets(line, sizeof(line), fp)) {	// read one line
               if (strncmp(line, "MemFree", strlen("MemFree")) == 0) {
                        //puts(line);
                        //char *str = strstr(line, ":");
                        //str += 1;
                        //sscanf(str, "%[^a-z]", num);
            			db_error("%s",line);            
                        break;
               }
       }
       fclose(fp);
		return 0;
}

void PreviewWindow::keyProc(int keyCode, int isLongPress)
{
    db_warn("[debug_joson]: keyCode = %d , inLongPress = %d",keyCode,isLongPress);
    static bool isShutdown = false;
	char buf[32];
	#if 0
	int *ptest = NULL;
	*ptest = 1234;
	#endif
	//debug_get_info();
	if(isMotionDetectHappen)
    {
        db_msg("BeCareful Motion is happen ingore the key event\n");
        return;
    }
    int ret = pthread_mutex_trylock(&proc_lock_);
    if (ret != 0) {
        db_info("key event is handling, ignore this one, keyCode: %d, isLongPress: %d", keyCode, isLongPress);
        return;
    }
    int m_recodTime = 0;
	if(m_BulletCollection_->m_button_dialog_flag_){
		//db_warn("[debug_zhb]--->play_BulletCollection_->m_button_dialog_flag_");
		m_BulletCollection_->keyProc(keyCode, isLongPress);
		goto out;
	}
	if(usb_win_->GetVisible()){
		//db_warn("1111111111111111111111111");
		usb_win_->keyProc(keyCode, isLongPress);
		goto out;
	}
    switch(keyCode)
    {
        case SDV_KEY_MENU:// button5 setting
            db_warn("[debug_zhb]:this is CDR_KEY_LEFT key code");
			
            if(/*(RecordState !=0) ||*/ isRecordStart)
            {
                ShowPromptBox(PROMPT_BOX_DEVICE_RECORDING,2);
                break;
            }
			if(photoing_flag)
            {
                ShowPromptBox(PROMPT_BOX_DEVICE_PHOTOING,2);
                break;
            }
            if(!isTakepicFinish){
                db_error("take pic not finish");
                break;
            }
             if((win_statu == STATU_PREVIEW) || (win_statu == STATU_PHOTO))
            {
				win_statu_save = win_statu;
				win_statu = STATU_PLAYBACK;
				#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
				#endif
				
                listener_->sendmsg(this, PREVIEW_TO_SETTING_BUTTON, 0);
                listener_->sendmsg(this,PREVIEW_TO_SETTING_NEW_WINDOW,0);
                listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_SETTING_NEW);
                win_statu = STATU_PREVIEW;

            }

            break;
        case SDV_KEY_RETURN:// button3 file locked
            {
                if(isRecordStart){
                    m_recodTime = this->GetCurrentRecordTime();
                    db_warn("Current Record Time is %d",m_recodTime);
                    listener_->sendmsg(this,PREVIEW_EMAGRE_RECORD_CONTROL, m_recodTime);
                }else{
                    //should show please start record first
                     ShowPromptBox(PROMPT_BOX_LOCK_RECORD_TIP_FILE,2);
                }
                if(!isTakepicFinish){
                    db_error("take pic not finish");
                    break;
                }
                db_msg("[habo]:this is SDV_KEY_MODE key code win_statu %d",win_statu);
                break;
            }
        case SDV_KEY_LEFT://button2  pohote
            { 
               db_msg("[debug_zhb]:this is cdr_key_right key code");
				#ifdef CAMB_PREVIEW
    			if(win_statu == STATU_PREVIEW){
             		HideCamBRecordIcon();
             		//listener_->sendmsg(this, PREVIEW_CAMB_PREVIEW_CONTROL , 0);
    				listener_->sendmsg(this,PREVIEW_SWITCH_LAYER, 0);
             		ShowCamBRecordIcon();
    			}
             	#endif
            }
            break;
        case SDV_KEY_OK: //button1 recording
            {
                if(((win_statu == STATU_PREVIEW ) && !isRecordStart))
                {
                    if(HandlerPromptInfo() == -1)
                    {
                        db_msg("[fangjj]: TF ERROR: no tf or tf full \n");
                        break;
                    }
                }

			if(win_statu == STATU_PREVIEW){
				if(photoing_flag)
				{
					ShowPromptBox(PROMPT_BOX_DEVICE_PHOTOING,2);
					break;
				}
			   if(isRecordStart == false){
				//if (this->GetRecordState() == 0) {
				  db_msg("debug_zhb-----------------ready to recording ");
				  listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 1);	// -> newPreview.cpp 
				  ShowPromptBox(PROMPT_BOX_RECORDING_START,2);
					usleep(500*1000);
				//}
			  }else{
				// if (this->GetRecordState() >= 2)  {
				  listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);
				  ShowPromptBox(PROMPT_BOX_RECORDING_STOP,2);
				  usleep(500*1000);
				//  }
			  }
			}else if (win_statu == STATU_PHOTO)
    		   {
    #ifdef PHOTO_MODE
    	#if 0
    				if (win_statu == STATU_PREVIEW) {
    					Camera* cam = NewPreview::GetCamera(CAM_A);
    					cam->ReSetVideoResolutionForNormalphoto(MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420);
    					win_statu = STATU_PHOTO;
    				}
    	#endif
    				db_warn("photoing_flag = %d ",photoing_flag);
    				if(photoing_flag)
    				{
    					current_count = photo_time ;
    					photoing_flag = false;
    						StatusBarMiddleWindow *sbw = reinterpret_cast<StatusBarMiddleWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
    					sbw->HidePhotoStatusTimeUi();	
    					pthread_mutex_unlock(&proc_lock_);
    					return ;
    				}
    			   isTakepicFinish = true;
    			   if(photo_mode == MODE_PIC_NORMAL)
    			   {
    					TakePicControl(TAKE_PHOTO_NORMAL);	 // normal photo
    			   }
    			   else if(photo_mode == MODE_PIC_TIME)
    			   {
    					photoing_flag = true;
    					current_count = photo_time ;
    						StatusBarMiddleWindow *sbw = reinterpret_cast<StatusBarMiddleWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
    					sbw->PhotoStatusTimeUi(photo_time,photo_mode);	// 更新status_bar_top 倒计时显示 		
    					
    			   }else if(photo_mode == MODE_PIC_AUTO){
    					photoing_flag = true;
    						StatusBarMiddleWindow *sbw = reinterpret_cast<StatusBarMiddleWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
    					sbw->PhotoStatusTimeUi(photo_time,photo_mode);	
    			   }else if(photo_mode == MODE_PIC_CONTINUE){
    				//	TakePicControl();				
    			   }					   
     #endif
    		  }

            }
            break;
        case SDV_KEY_MODE://button4 to playback
            {
                db_msg("[debug_zhb]:this is SDV_KEY_RIGHT key code");
                if(isRecordStart)
                {
                    ShowPromptBox(PROMPT_BOX_DEVICE_RECORDING,2);
                    break;
                }
                if(!isTakepicFinish || photoing_flag){
                    db_error("take pic not finish");
					ShowPromptBox(PROMPT_BOX_DEVICE_PHOTOING,2);
                    break;
                }
                if(win_statu == STATU_PREVIEW) 
				{
	#if 1
					//db_warn("00000[debug_zhb]:this is button2  pohote key code");
					WindowManager *win_mg_ = WindowManager::GetInstance();
					StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
					sbw->StatusBarWindowProc(NULL);
					usleep(500*1000);
					db_warn("[debug_zhb]:this is SDV_KEY_LEFT222 key win_statu = %d ",win_statu);
				}else if(win_statu == STATU_PHOTO)
                {
	#endif
					win_statu_save = STATU_PREVIEW;
                    win_statu = STATU_PLAYBACK;
		#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                    EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
		#endif
                    listener_->sendmsg(this, PREVIEW_GO_PLAYBACK_BUTTON, win_statu);
					usleep(500*1000);
                    listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PLAYBACK);
                    win_statu = STATU_PREVIEW;

                }
            }

            break;
	case SDV_KEY_RIGHT:	// voice
	{
		db_warn("==========SDV_KEY_RETURN==========");
	//#ifdef CAMB_PREVIEW
	//	HideCamBRecordIcon();
	//	listener_->sendmsg(this, PREVIEW_CAMB_PREVIEW_CONTROL , 0);
	//	ShowCamBRecordIcon();
	//#endif
	if(win_statu == STATU_PREVIEW){
		bool audio_record;
		WindowManager *win_mg_ = WindowManager::GetInstance();
	    MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
		StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
		
		//db_warn("[PreviewWindow]---> audio_record = %d ,",audio_record);
		//menuconfiglua->SetMenuIndexConfig(MSG_SET_RECORD_VOLUME,!audio_record);
		NewSettingWindow *nsw  = static_cast<NewSettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING_NEW));
		int idx = nsw->GetPosInlistview_item_ids(SETTING_RECORD_VOLUME);
		if (idx < 0) {
			db_error("idx(%d) error!!!!!",idx);
			break;
		}
		nsw->PingpangListViewItem(idx, false);	// SETTING_RECORD_VOLUME对应的菜单序号
		sbw->VoiceIconHandler(true);
		audio_record = (bool)menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_VOLUME);
		this->SetRecordMute(audio_record);
		}	
	    break;
	}
	case SDV_KEY_POWER://button7 to KEY_POWER
     {
		if (isLongPress) {
			db_warn("==========SDV_KEY_POWER==========");
			ShutDown();
		}
	    break;
	}
        default:
            db_msg("[debug_jaosn]:this is invild key code");
            break;
    }
out:
    pthread_mutex_unlock(&proc_lock_);

}

void PreviewWindow::TakePicControl(int flag)
{
	db_warn("TakePicControl");
	if(isTakepicFinish){			
	    isTakepicFinish = false;
	    
	    listener_->sendmsg(this,PREVIEW_TAKE_PIC_CONTROL, flag);
	}else{
	  	db_warn("[debug_zhb]:take pic is not done\n");   
	}
}

int PreviewWindow::ShowCamBRecordIcon()
{
    StorageManager *sm = StorageManager::GetInstance();
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))
	{
		HideCamBRecordIcon();
		return -1;
    }

	int IsShowing = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_CAMB_PREVIEWING);
	int IsRecording = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_REAR_RECORD_RESOLUTION);
	if( IsRecording && isRecordStart)
	{
		if( IsShowing )
		{
			if(GetControl("record_icon1") != NULL)
				GetControl("record_icon1")->Hide();

			if( m_record_info1 != NULL)
				m_record_info1->Hide();

			GraphicView::LoadImage(GetControl("record_icon"), "rec_hint");
			GetControl("record_icon")->Show();
			if( m_record_info == NULL)
			{
				m_record_info = reinterpret_cast<TextView *>(GetControl("record_info"));
				m_record_info->SetCaptionColor(0xFFFFFFFF);//0xFF5f6174
				m_record_info->SetBackColor(0xff000000);
			}
			m_record_info->SetCaption("REC");
			m_record_info->Show();
		}
		else
		{
			if(GetControl("record_icon") != NULL)
				GetControl("record_icon")->Hide();

			if( m_record_info != NULL)
				m_record_info->Hide();

			GraphicView::LoadImage(GetControl("record_icon1"), "rec_hint");
			GetControl("record_icon1")->Show();

			if( m_record_info1 == NULL)
			{
				m_record_info1 = reinterpret_cast<TextView *>(GetControl("record_info1"));
				m_record_info1->SetCaptionColor(0xFFFFFFFF);//0xFF5f6174
				m_record_info1->SetBackColor(0xff000000);
			}
			m_record_info1->SetCaption("REC");
			m_record_info1->Show();
		}
	}
	else
		HideCamBRecordIcon();

	return 0;
}

void PreviewWindow::SetPhotoTimeShot()
{
	WindowManager *win_mg = ::WindowManager::GetInstance();
	PreviewWindow *pw = static_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
	pw->listener_->sendmsg(pw,PREVIEW_TAKE_PIC_CONTROL, TAKE_PHOTO_NORMAL);		// normal
}

static int GetTimeCountDown(int idx)
{
    int map[] {0, 3, 5, 10, 20};
    return map[idx];
}

static int GetAutoTimeCountDown(int idx)
{
    int map[] {0, 3, 10, 15, 20, 30};
    return map[idx];
}

static int GetCountinuCountDown(int idx)
{
    int map[] {0, 3, 5, 10};
    return map[idx];
}

void PreviewWindow::UpdatePhotoMode(int mode,int index)
{
	db_warn("update photo mode mode =%d   index=%d",mode,index);

	photo_mode = mode;

	if(photo_mode == MODE_PIC_TIME)
	{
		photo_time = GetTimeCountDown(index);
	}
	else if(photo_mode == MODE_PIC_AUTO)
	{
		photo_time = GetAutoTimeCountDown(index);
	}
	else if(photo_mode == MODE_PIC_CONTINUE)
	{
		photo_time = GetCountinuCountDown(index);
	}else{
		photo_time = 0 ;
	}
	current_count = photo_time;
	db_warn("update photo mode photo_mode =%d   photo_time=%d",photo_mode,photo_time);
}
int PreviewWindow::HideCamBRecordIcon()
{
	if(GetControl("record_icon") != NULL)
		GetControl("record_icon")->Hide();

	if(GetControl("record_icon1") != NULL)
		GetControl("record_icon1")->Hide();

	if( m_record_info != NULL)
		m_record_info->Hide();

	if( m_record_info1 != NULL)
		m_record_info1->Hide();

	return 0;
}

int PreviewWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    //  db_error("[debug_jaosn]:the message=%d ; wparam=%u ; lparam = %lu",message,wparam,lparam);
    WindowManager *win_mg_ = WindowManager::GetInstance();
    static int x=0,y=0;
    switch (message) {
		case MSG_CREATE:
			SetTimer(hwnd, KEEPALIVE_TIMER_ID, 100);
			break;
        case MSG_PAINT: {
#if 0
            HDC hdc = ::BeginPaint(hwnd);
            HDC mem_dc = CreateMemDC (1280, 200, 16, MEMDC_FLAG_HWSURFACE | MEMDC_FLAG_SRCALPHA,
                                  0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F);

            /* 璁剧疆涓�涓崐閫忔槑鐨勫埛瀛愬苟濉厖鐭╁舰 */
            SetBrushColor (mem_dc, RGBA2Pixel (mem_dc, 0x00, 0x00, 0x00, 0xA0));
            FillBox (mem_dc, 0, 0, 1280, 200);

            SetBkMode (mem_dc, BM_TRANSPARENT);
            BitBlt (mem_dc, 0, 0, 1280, 200, hdc, 0, 600, 0);

            ::EndPaint(hwnd, hdc);
#endif
        }
            return HELP_ME_OUT;
		case MSG_TIMER:
			if(wparam == KEEPALIVE_TIMER_ID)
			{
				WatchDog::GetInstance()->SetFeedTime();
				return HELP_ME_OUT;
			} 
			#if 0
			else if (wparam == CHECKACC_TIMER_ID) 
			{
				PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				KillTimer(hwnd, CHECKACC_TIMER_ID);
				db_error("CHECKACC_TIMER_ID time out");
				return HELP_ME_OUT;	
			}
			#endif
            break;
		case MSG_LBUTTONDOWN :
		    {
		    	x = LOWORD(lparam);
		    	y = HIWORD(lparam);
				
				Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
				StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
				StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
				if((y > 60) && (y < 260)){
					if(hide_flag == false){
						stb->hideStatusBarBottomWindow();
						sbw->hideStatusBarWindow();
						hide_flag = true;
					}else{
						stb->showStatusBarBottomWindow();
						sbw->showStatusBarWindow();
						hide_flag = false;
					}
				}
				//PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				//SetFocus(pw->GetHandle());
				//db_warn("[PreviewWindow]---> x = %d ,y = %d",x,y);   
				//db_error("StatusBarWindow Visible: %d",sbw->GetVisible() );
				//SetActiveWindow(pw->GetHandle());
			}
			break;
        case MSG_MOUSE_FLING:
        {  
            int direction = LOSWORD (wparam);
			Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
			StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
			db_error("StatusBarWindow Visible: %d",sbw->GetVisible() );
			PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			#if 0
			Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
			StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
			StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
			if(hide_flag == false){
				stb->hideStatusBarBottomWindow();
				sbw->hideStatusBarWindow();
				hide_flag = true;
			}else{
				stb->showStatusBarBottomWindow();
				sbw->showStatusBarWindow();
				hide_flag = false;
			}
			#endif
			pw->OnOffStatusBar();
            if (direction == MOUSE_LEFT)
            {
//                listener_->sendmsg(this,PREVIEW_SWITCH_LAYER, 0);
				db_warn("======MOUSE_LEFT==========");
			

            }
            else if(direction == MOUSE_RIGHT)
            {
				db_warn("===========MOUSE_RIGHT==========");
			}
			else if(direction == MOUSE_UP)
            {
				db_warn("===========MOUSE_UP==========");
			}
			else if(direction == MOUSE_DOWN)
            {
				db_warn("===========MOUSE_DOWN==========");
			}else
                db_warn("please right or left flig  to update cam_A and Cam_B layer");

            
        }break;
		case MSG_CHANGEWINDOWMODE:
		{
			//Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
			PreviewWindow *pvw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
			int value0 = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_MOTION_DETECT);
			if (!value0) {	// 移动侦测打开不允许切换到拍照模式
				pvw->ChangeWindowStatus((int)wparam);
				usleep(500*1000);
			} else {
				ShowPromptBox(PROMPT_BOX_MOTIONISON,2);
			}
		}break;
        default:
          return ContainerWidget::HandleMessage( hwnd, message, wparam, lparam );
    }
	return HELP_ME_OUT;
}

void PreviewWindow::OnOffStatusBar()
{
	WindowManager *win_mg_ = WindowManager::GetInstance();
	Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
	StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
	if(hide_flag == false){
		stb->hideStatusBarBottomWindow();
		sbw->hideStatusBarWindow();
		hide_flag = true;
	}else{
		stb->showStatusBarBottomWindow();
		sbw->showStatusBarWindow();
		hide_flag = false;
	}
	PreviewWindow *pw  = static_cast<PreviewWindow*>(win_mg_->GetWindow(WINDOWID_PREVIEW));
	SetFocus(pw->GetHandle());
	SetActiveWindow(pw->GetHandle());
}

void PreviewWindow::showShutDownLogo()
{
	m_shutwindowObj->Show();
}

void PreviewWindow::USBModeWindowLangChange()
{
	this->usb_win_->OnLanguageChanged();
}

void PreviewWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE;
    params.class_name = " ";
    params.alias      = GetClassName();
}

PreviewWindow::PreviewWindow(IComponent *parent)
        : SystemWindow(parent)
        , win_statu(STATU_PREVIEW)
        , zoom_val_(0)
        , isRecordStart(false)
        , m_isRecord(false)
        , isTakepicFinish(true)
        , isWifiStarting(false)
        , m_bAutoTakePic(false)
        , m_bTimeTakePic(false)
        , m_nCurrentWin(WINDOWID_PREVIEW)
        , m_bUsbDialogShow(false)
        , m_bHdmiConnect(false)
        , m_standby_flag(false)
        , m_preview_button_dialog_index(PREVIEW_BUTTON_DIALOG_FORMAT_SDCARD)
        ,m_stop2down(false)
        , version_dl_thread_id(0)
        ,m_dl_finish(false)
        , is_start_download(false)
        , m_download_status(STATUS_DOWNLOAD_INIT)
        ,win_mg(::WindowManager::GetInstance())
		,version_4g_dl_thread_id(0)
		,m_record_info(NULL)
		,m_record_info1(NULL)
		,usb_win_(NULL)
		,photo_mode(MODE_PIC_NORMAL)
		,photoing_flag(false)
		,isRecordStartFake(false)
		,RecordState(0)
		,isMotionDetectHappen(false)
{
    db_msg(" ");
    
    Load();

    //SetWindowBackImage("/usr/share/minigui/res/images/bg.png");
    SetBackColor(0x00000000);

    // alpha is 0, that means set background transparent
    DWORD ctrl_bg_color = 0x00000000;

    m_shutwindowObj = new ShutDownWindow(this);
    m_shutwindowObj->Hide();
    PromptBox_ = new PromptBox(this);
    prompt_ = new Prompt(this);

#ifdef SHOW_DEBUG_INFO
#if 0
    TextView *debug_info_label = static_cast<TextView*>(GetControl("debug_info"));
    debug_info_label->SetCaptionColor(PIXEL_red);
    debug_info_label->SetTextStyle(DT_LEFT| DT_WORDBREAK | DT_EDITCONTROL);
#endif
#endif

  //add by zhb init record_time
    //initRecordTimeUi();
    //PauseRecordCtrl(true);
    initTopBottomBg(true,true);

	int value0 = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_PHOTO_TIMED);
	int value1 = MenuConfigLua::GetInstance()->GetMenuIndexConfig(SETTING_PHOTO_AUTO);
	if(value0 != 0)
	{
		photo_mode = MODE_PIC_TIME;
		photo_time = GetTimeCountDown(value0);
	}else if(value1 != 0)
	{
		photo_mode = MODE_PIC_AUTO;
		photo_time = GetAutoTimeCountDown(value1);
	}else
	{
		photo_mode = MODE_PIC_NORMAL;
		photo_time = 0;
	}
	current_count = photo_time ;
	db_warn("photo mode =%d      time =%d\n",photo_mode,photo_time);
    m_BulletCollection_ = new BulletCollection();
    m_BulletCollection_->initButtonDialog(this);
    create_timer(this, &dl_timer_id, DLTimerProc);
    stop_timer(dl_timer_id);
    pthread_mutex_init(&proc_lock_, NULL);
#ifdef USB_MODE_WINDOW
    usb_win_ = new USBModeWindow(this);
#endif
	win_statu_save = win_statu;
}

PreviewWindow::~PreviewWindow()
{
    db_msg("destruct");
#ifdef USB_MODE_WINDOW
   if(usb_win_){
	    delete usb_win_;
	    usb_win_ = NULL;
   	}
#endif
#if 1

   if(m_shutwindowObj){
	    delete m_shutwindowObj;
	    m_shutwindowObj = NULL;
   	}
 #endif
    if(m_BulletCollection_){
		delete m_BulletCollection_;
		m_BulletCollection_ = NULL;
    	}
    pthread_mutex_destroy(&proc_lock_);
	if( version_dl_thread_id > 0 )
		pthread_cancel(version_dl_thread_id);

	if( version_4g_dl_thread_id > 0 )
		pthread_cancel(version_4g_dl_thread_id);


	delete_timer(dl_timer_id);
	

}

void PreviewWindow::initTopBottomBg(bool mdown,bool msbottom)
{
#if 0
	if(mdown){
		GraphicView::LoadImage(GetControl("top_bg_icon"), "top_bg");
		GraphicView::LoadImage(GetControl("bottom_bg_icon"), "bottom_bg");
	}
	if(msbottom)
		GetControl("bottom_bg_icon")->Show();
	else
		GetControl("bottom_bg_icon")->Hide();
	GetControl("top_bg_icon")->Show();
#endif
}

int  getSdcardVersion(std::vector<std::string> &p_FileNameList,std::string path_str = "/mnt/extsd/version/",std::string filter_str = ".img")
{
	StorageManager *sm = StorageManager::GetInstance();
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))
		return -1;

	char *filepath =(char *)path_str.c_str();
	DIR *sdcard_dir = NULL;
	struct dirent *dirp;
	  int ret = -1;
	  db_msg("debug_zhb---> scan sdcard path : %s",filepath);
	if((sdcard_dir = opendir(filepath)) == NULL)
	{
	   db_error("opendir fail");
	   return -1;
	}
	while((dirp = readdir(sdcard_dir)) != NULL)
	{
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;
		int size = strlen(dirp->d_name);
		//if(strcmp((dirp->d_name+( size - 4)),".img")!=0)
		if(strcmp((dirp->d_name+( size - strlen(filter_str.c_str()))),filter_str.c_str())!=0)
			continue;
		ret = 0;
	   p_FileNameList.push_back(dirp->d_name);
	}
	    closedir(sdcard_dir);
	return ret;
}
int  getSdcardBin(std::vector<std::string> &p_FileNameList,std::string path_str,std::string filter_str)
{
	StorageManager *sm = StorageManager::GetInstance();
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))
		return -1;

    char *filepath =(char *)path_str.c_str();
    DIR *sdcard_dir = NULL;
    struct dirent *dirp;
    int ret = -1;
    db_msg("debug_zhb---> scan sdcard path : %s",filepath);
	if((sdcard_dir = opendir(filepath)) == NULL)
	{
	   db_error("opendir fail");
	   return -1;
	}
	while((dirp = readdir(sdcard_dir)) != NULL)
	{
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;
		int size = strlen(dirp->d_name);
		//if(strcmp((dirp->d_name+( size - 4)),".img")!=0)
		if(strcmp((dirp->d_name+( size - strlen(filter_str.c_str()))),filter_str.c_str())!=0)
			continue;
		ret = 0;
	   p_FileNameList.push_back(dirp->d_name);
	}
	    closedir(sdcard_dir);
	return ret;
}
bool PreviewWindow::Md5CheckVersionPacket(string p_path,string md5Code)
{
    FILE *ptr = NULL;
    char buf_ps[128]={0};
    char md5str[128]= {0};
    char temp[128]={0};;
    snprintf(temp,sizeof(temp),"md5sum /mnt/extsd/version/%s",p_path.c_str());
    db_warn("md5check command : %s",temp);
    if((ptr = popen(temp,"r"))!= NULL)
    {
        while(fgets(buf_ps, 128, ptr) !=NULL)
        {
            if(strstr(buf_ps,p_path.c_str())== NULL)
                continue;
            char *saveptr = strstr(buf_ps," ");
            memset(md5str, 0, sizeof(md5str));
            strncpy(md5str, buf_ps, saveptr-buf_ps);
            db_warn("debug_zhb--->md5str = %s ",md5str);
            break;
        }
        pclose(ptr);
    }
    db_warn("debug_zhb--->md5Code = %s ",md5Code.c_str());
    if(strncmp(md5Code.c_str(),md5str,strlen(md5Code.c_str())) == 0)
        return true;

    return false;
}
bool PreviewWindow::IsNewVersion(std::string external_version,std::string local_version)
{
#if 0
		//V-1.00.15d26_CN_debug
		//V-1.00.15d26_CN
	if(external_version.empty() || local_version.empty())
	{
		db_error("%s  external_version or local_version is empty",__func__);
		return false;
	}
	db_warn("IsNewVersion    external_version : %s ",external_version.c_str());
    db_warn("IsNewVersion     local_version : %s",local_version.c_str());
	//pars external_version
	string::size_type e_rc_start = external_version.rfind("V536-");
	if( e_rc_start == string::npos)
	{
		db_warn("invalid fileName:%s",external_version.c_str());
		return false;
	}
	string::size_type e_rc_end = external_version.rfind("d26");
	if( e_rc_end == string::npos)
	{
		db_warn("invalid fileName:%s",external_version.c_str());
		return false;
	}
	string e_str = external_version.substr(e_rc_start+2,e_rc_end-(e_rc_start+2) );

	db_warn("pars external_version : %s",e_str.c_str());
	int e_num_first = 0, e_num_second = 0, e_num_third = 0;
	sscanf(e_str.c_str(),"%i.%i.%i",&e_num_first,&e_num_second,&e_num_third);
	db_warn("pars external version value : %d  %2d  %2d",e_num_first,e_num_second,e_num_third);

	//pars local version
	string::size_type local_rc_start = local_version.rfind("V536-");
	if( local_rc_start == string::npos)
	{
		db_warn("invalid fileName:%s",local_version.c_str());
		return false;
	}

	string::size_type local_rc_end = local_version.rfind("d26");
	if( local_rc_end == string::npos)
	{
		db_warn("invalid fileName:%s",local_version.c_str());
		return false;
	}
	string local_str = local_version.substr(local_rc_start+2,local_rc_end-(local_rc_start+2) );

	db_warn("pars local_version : %s",local_str.c_str());
	int l_num_first = 0, l_num_second = 0, l_num_third = 0;
	sscanf(local_str.c_str(),"%i.%i.%i",&l_num_first,&l_num_second,&l_num_third);
	db_warn("pars local version value : %d  %2d  %2d",l_num_first,l_num_second,l_num_third);

	//cmp the version
	if(e_num_first > l_num_first)
	{
		return true;
	}
	else if(e_num_first < l_num_first)
	{
		return false;
	}
	else//e_num_first == l_num_first
	{
		if(e_num_second > l_num_second)
		{
			return true;
		}
		else if(e_num_second < l_num_second)
		{
			return false;
		}
		else//e_num_second == l_num_second
		{
			if(e_num_third > l_num_third)
			{
				return true;
			}
			else if(e_num_third < l_num_third)
			{
				return false;
			}
			else//e_num_third == l_num_third
			{
				return false;
			}
		}
	}

	return true;
#endif

	//升级固件路径为/mnt/extsd/version,固件名称为V536-CDR-32位md5值.img
	//eg:V536-CDR-dc1afd85b49f47377c7e6b62ad0045c3.img
	if(external_version.empty() || local_version.empty())
    {
        db_error("%s  external_version or local_version is empty",__func__);
        return false;
    }
    db_warn("IsNewVersion    external_version : %s ",external_version.c_str());
    db_warn("IsNewVersion     local_version : %s",local_version.c_str());
    //pars external_version
    string::size_type e_rc_start = external_version.rfind(MODEL_PREFIX);
    if( e_rc_start == string::npos)
    {
        db_warn("invalid fileName:%s",external_version.c_str());
        return false;
    }

    string::size_type local_rc_start = local_version.rfind(MODEL_PREFIX);
    if( local_rc_start == string::npos)
    {
        db_warn("invalid fileName:%s",local_version.c_str());
        return false;
    }

    return true;
}
int  PreviewWindow::DetectSdcardNewVersion()
{
    int ret = -1;
    db_warn("debug_zhb-------ready------detecte the new version");
	HideIspSaveFile();
    std::vector<std::string> p_FileNameList;
    p_FileNameList.clear();
    if(getSdcardVersion(p_FileNameList) < 0){
        db_warn("get sdcard version info fail");
        return -1;
    }
    if(p_FileNameList.size() > 1)
    {
		db_warn("detect more than 1 img in the /mnt/extsd/version/");
		m_stop2down = true;
		m_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_MORE_IMG);
		m_BulletCollection_->ShowButtonDialog();
		return 0;
	}
    #ifdef SETTING_WIN_USE
	SettingWindow *s_win = reinterpret_cast<SettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING));
    #else
    NewSettingWindow *s_win = reinterpret_cast<NewSettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING_NEW));
    #endif
	string vstr = s_win->getVersionStr();

	if(IsNewVersion(p_FileNameList[0],vstr) == true)
	{
#if 0
		string::size_type rc_cn_ = p_FileNameList[0].rfind("CN_");
		if( rc_cn_ == string::npos)
		{
            rc_cn_ = p_FileNameList[0].rfind("EN_");//
            if( rc_cn_ == string::npos)
            {
			    db_warn("invalid fileName:%s",p_FileNameList[0].c_str());
			    return -1;
            }
		}
        //check name if _CN_debug 
        string md5_str;
        string::size_type rc_cn_debug = p_FileNameList[0].rfind("debug_");
        if(rc_cn_debug == string::npos)
        {
            //no debug str
           md5_str = p_FileNameList[0].substr(rc_cn_+strlen("CN_"));
        }
        else
        {
            //_debug 
            md5_str = p_FileNameList[0].substr(rc_cn_+strlen("CN_debug_"));
        }
#endif
        string md5_str;
		string prefixstr = string(MODEL_PREFIX)+"-";
        string::size_type rc_cn_debug = p_FileNameList[0].rfind(prefixstr);
        md5_str = p_FileNameList[0].substr(rc_cn_debug+strlen(prefixstr.c_str()));
        if(md5_str.empty()){
            db_error("md5 str is empty!!");
            return -1;
        }

        char temp[128]={0};
        if(strncpy(temp,md5_str.c_str(),32) == NULL)
        {
            db_warn("strncpy md5 str fail");
            return -1;
        }
        db_warn("get the md5 str form version name : %s",temp);
        md5_str.clear();
        md5_str = temp;

		//check md5
		if(Md5CheckVersionPacket(p_FileNameList[0],md5_str) == false)
		{
			db_warn("check the version  md5  fail");
			m_stop2down = true;
			m_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_CHECK_MD5_FAILE);
			m_BulletCollection_->ShowButtonDialog();
			return 0;
		}
		ret =0;
		db_msg("debug_zhb-------------detecte the new version");
		m_stop2down = true;
		m_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION);
		m_BulletCollection_->ShowButtonDialog();
	}

	return ret;
}
bool PreviewWindow::IsHighCalssCard()
{
	StorageManager *sm = StorageManager::GetInstance();
	if(sm->IsHighClassCard() == LOW_SPEED)
	{
		db_warn("is low speed card set m_stop2down true");
		m_stop2down = true;
		ShowPromptInfo(PROMPT_TF_LOW_SPEED, 0);
		return false;
	}
	return true;
}

bool PreviewWindow::IsFileSystemError()
{
    StorageManager *sm = StorageManager::GetInstance();
    if (sm->GetStorageStatus() == MOUNTED)
    {
        db_msg("debug_zhb----> filesystem check");
        if(sm->CheckFileSystemError()!= true)
        {
            db_error("checkFileSystemError---");
            return true;
        }
    }
    db_msg("debug_zhb----> filesystem is ok");
    return false;
}
#define s_w 640
#define s_h 360


int PreviewWindow::GetCurrentRecordTime()
{
    WindowManager *win_mg_ = WindowManager::GetInstance();
    StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    return sbw->GetCurrentRecordTime();
}

void PreviewWindow::RecordStatusTimeUi(bool mstart)
{
	db_warn("---RecordStatusTimeUi mstart = %d ",mstart);
    if(mstart){
        isRecordStart = true;
    }else{
        isRecordStart = false;
    }

    WindowManager *win_mg_ = WindowManager::GetInstance();
    StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    sbw->RecordStatusTimeUi(mstart);
    sbw->SetRecordStatusFlag(mstart);
}
void PreviewWindow::ResetRecordTime()
{
    WindowManager *win_mg_ = WindowManager::GetInstance();
    StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    sbw->ResetRecordTime();
	sbw->LockIconHander(false);
}

void PreviewWindow::SwitchButtonProc(View *control)
{
    SwitchButton *sw_button = reinterpret_cast<SwitchButton *>(control);

    if (sw_button == audio_switch_ && rec_switch_->GetSwitchStatus()) {
        db_info("record already started, can not enable/disable audio record");

        // restore to last state
        audio_switch_->SetSwitchState(audio_switch_->GetSwitchStatus()?SWITCH_OFF:SWITCH_ON);
        return;
    }

    pthread_mutex_lock(&proc_lock_);
    listener_->sendmsg(this, sw_button->GetTag(), sw_button->GetSwitchStatus());
    pthread_mutex_unlock(&proc_lock_);
}

void PreviewWindow::GraphicViewButtonProc(View *control)
{
    int tag = control->GetTag();

    switch (tag) {
        case PREVIEW_GO_PLAYBACK_BUTTON:
            {
                db_warn("habo --->GraphicViewButtonProc PREVIEW_GO_PLAYBACK_BUTTON ");
                // NOTE: do not change the order
                //listener_->notify(this, tag, 1);
                //listener_->notify(this, WM_WINDOW_CHANGE, WINDOWID_PLAYBACK);
                string title_str;
                if(win_statu == STATU_PREVIEW){
                    db_msg("zhb----@@@@@@@@@@");
                    win_statu = STATU_PHOTO;
                    GetControl("gv_shotcut")->Show();
                }else if(win_statu == STATU_PHOTO){
                    win_statu = STATU_SLOWRECOD;
                    GetControl("gv_shotcut")->Hide();
                }else if(win_statu == STATU_SLOWRECOD){
                    win_statu = STATU_PREVIEW;
                    GetControl("gv_shotcut")->Hide();
                    //listener_->notify(WINDOWID_STATUSBAR_BOTTOM, WM_WINDOW_CHANGE, WINDOWID_PLAYBACK);
                    listener_->notify(this, WM_WINDOW_CHANGE, WINDOWID_PLAYBACK);
                    //listener_->notify(this, WM_WINDOW_CHANGE, WINDOWID_PLAYBACK);
                    // string title_str;
                }
                listener_->notify(this, tag, win_statu);
            }
            break;
        case PREVIEW_SHOTCUT_BUTTON:
            listener_->notify(this, tag, 1);
            break;
        case PREVIEW_SHUTDOWN_BUTTON:
            listener_->notify(this, tag, 1);
            break;
		case PREVIEW_GO_PHOTO_BUTTON:
		{			
			db_warn("^^^^^^^^^^^^^^^^^^^^^^^^^^^PREVIEW_GO_PHOTO_BUTTON ");
         //   listener_->notify(this, tag, 1);
            break;	
        }
        default:
            break;
    }
}

string PreviewWindow::GetResourceName()
{
    return string(GetClassName());
}

void PreviewWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
	PreviewWindow* pv_win = reinterpret_cast<PreviewWindow *>(win_mg->GetWindow(WINDOWID_PREVIEW));
    db_warn("handle msg:%d", msg);
    switch ((int)msg) {
        case MSG_CAMERA_START_PREVIEW:
            break;
        case MSG_CAMERA_STOP_PREVIEW:
            break;
        case MSG_CLOSE_STANDBY_DIALOG:
            {
                db_warn("by hero *** ------PreviewWindow------------MSG_CLOSE_STANDBY_DIALOG ---");
                EyeseeLinux::Screensaver::GetInstance()->ForceScreenOnBySdcard();
                db_warn("==========PromptShowFlag %d,getPromptId %d,m_prompt_standby_flag_ %d==========",
                        prompt_->GetPromptShowFlag(),prompt_->getPromptId(),prompt_->GetStandbyFlagStatus());
                if(prompt_->GetPromptShowFlag() && prompt_->getPromptId() == PROMPT_STANDBY_MODE
                        && prompt_->GetStandbyFlagStatus() == true)
                {
                    prompt_->HidePromptInfo();
					if(EyeseeLinux::EventManager::GetInstance()->mStandbybreak_flag == false)
					{
						db_warn("========set mStandbybreak_flag true=========");
						EyeseeLinux::EventManager::GetInstance()->mStandbybreak_flag = true;
						EyeseeLinux::EventManager::GetInstance()->standby_try_count = 0;
					}
					if(EyeseeLinux::EventManager::GetInstance()->mNotify_acc_off_flag) {
						db_warn("========reset mNotify_acc_off_flag=========");
						EyeseeLinux::EventManager::GetInstance()->mNotify_acc_off_flag = false;
					}
					if(prompt_->GetStandbyFlagStatus() == true) {
						db_warn("prompt stanby flag is true,reset prompt stanby flag");
						prompt_->SetStandbyFlagStatus(false);
					}

				}
				m_stop2down = false;
			}
			break;
		#if 0
		case MSG_ACCON_STARTCHECK:
			int window_id = WindowManager::GetInstance()->GetCurrentWinID();
            if(WINDOWID_PREVIEW == window_id){
                if(1){
					db_error("start reocrd by MSG_ACCON_HAPPEN check........");
					SetTimer(pv_win->GetHandle(), CHECKACC_TIMER_ID, 25);	// 开始检测插进来的是
                }
            }
			break;
		#endif
		case MSG_ACCON_HAPPEN:
			{
				db_warn("by hero *** ------PreviewWindow------------MSG_ACCON_HAPPEN ---");
				EyeseeLinux::Screensaver::GetInstance()->ForceScreenOnBySdcard();
				if(m_standby_flag){
					m_standby_flag = false;
					if(prompt_->GetPromptShowFlag() && prompt_->getPromptId() == PROMPT_STANDBY_MODE)
					{
						prompt_->HidePromptInfo();
						if(EyeseeLinux::EventManager::GetInstance()->mNotify_acc_off_flag) {
							db_warn("========reset mNotify_acc_off_flag=========");
							EyeseeLinux::EventManager::GetInstance()->mNotify_acc_off_flag = false;
						}
						if(prompt_->GetStandbyFlagStatus() == true) {
							db_warn("prompt stanby flag is true,reset prompt stanby flag");
							prompt_->SetStandbyFlagStatus(false);
						}

					}
					m_stop2down = false;
				}

				if(win_mg ->GetCurrentWinID() == WINDOWID_BINDING){
					db_msg("current is bindwindow ,stop to going down");
					 break;
				 }
				int window_id = WindowManager::GetInstance()->GetCurrentWinID();
                if(WINDOWID_PREVIEW == window_id){
					if (GetWindowStatus() == STATU_PREVIEW) {
	                    if(!GetRecordStatus() && (HandlerPromptInfo() != -1)){
							
							if(!PowerManager::GetInstance()->getUsbconnectStatus()) {
								// no usb connect
								db_error("start reocrd by MSG_ACCON_HAPPEN........");
	                        	listener_->sendmsg(this, PREVIEW_RECORD_BUTTON, 1);
							}
	                        set_one_shot_timer(1,0,dl_timer_id);
							
							//SetTimer(pv_win->GetHandle(), CHECKACC_TIMER_ID,25);
	                    }
					}
                }else{
                    db_error("acc on happen,but current window %d is not preview,do not recorder",window_id);
                }
			}
			break;
		case MSG_ACCOFF_HAPPEN:
			{
				db_warn("by hero *** ------PreviewWindow------------MSG_ACCOFF_HAPPEN");
				if(!m_standby_flag)
					EyeseeLinux::Screensaver::GetInstance()->ForceScreenOnBySdcard();
				m_standby_flag = true;
				//should colse all style dialog window and the show the  PROMPT_STANDBY_MODE
				PlaybackWindow* playback_win_ = reinterpret_cast<PlaybackWindow *>(win_mg->GetWindow(WINDOWID_PLAYBACK));
				if(playback_win_->GetPlayBCHandle()->getButtonDialogShowFlag())//close the playback window button dialog
				{
					playback_win_->GetPlayBCHandle()->BCDoHide();
					playback_win_->ShowVideoPlayImg();
				}
				if(m_BulletCollection_->getButtonDialogShowFlag())//close the preview window button dialog
				{
			        	m_BulletCollection_->BCDoHide();
			        	StorageManager::GetInstance()->setMReadOnlyDiskFormatFinish(true);//reset the ready only thread flag
		        	}

				if(PromptBox_->GetPromptBoxShowFlag())//close the all window promptbox
					PromptBox_->HidePromptBox();
                #ifdef SETTING_WIN_USE
                SettingWindow*s_win = reinterpret_cast<SettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING));
                #else
                 NewSettingWindow*s_win = reinterpret_cast<NewSettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING_NEW));
                #endif
				s_win->ForceCloseSettingWindowAllDialog();
				m_stop2down = FALSE;
				if(EyeseeLinux::EventManager::GetInstance()->mNotify_acc_on_flag) {
					db_warn("========reset mNotify_acc_on_flag=========");
					EyeseeLinux::EventManager::GetInstance()->mNotify_acc_on_flag = false;
				}
				ShowPromptInfo(PROMPT_STANDBY_MODE, PROMPT_POWEROFF_TIME, true);//force close the all window prompt
			}
			break;
		case MSG_GPSON_HAPPEN:
		{
			db_error("set gps icon on");
			StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
			sbw->UpdateGpsStatus(true, 0);	// show
			break;
		}
		case MSG_GPSOFF_HAPPEN:
		{
			db_error("set gps icon off");
			StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
			sbw->UpdateGpsStatus(false, 0);	// hide
			break;	
		}
        case MSG_PREPARE_TO_SUSPEND:
            //fix 4G icon error
            EventManager::GetInstance()->SetNetStatus(0);
            break;
        case MSG_RECORD_START:		// 20
            {
                db_warn("[habo]--->start to record !!!!");		// 真正开始录像
                RecordState = 2;
                initTopBottomBg(false,false);
                RecordStatusTimeUi(true);
				#ifndef SUPPORT_MODEBUTTON_TOP
				StatusBarBottomWindow *sbbw  = static_cast<StatusBarBottomWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
				sbbw->PreviewWindownModeButtonStatus(1);
				#endif
				StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
				sbw->SetStatusRecordStatus(2);
            }break;
        case MSG_RECORD_STOP:		// 21
            {
				db_warn("[habo]--->record stop!!!!");
				if(isRecordStart)
                {
                    ShowPromptBox(PROMPT_BOX_RECORDING_STOP,2);
                }
                isMotionDetectHappen = false;
                initTopBottomBg(false, true);
				RecordState = 0;
                RecordStatusTimeUi(false);
				#ifndef SUPPORT_MODEBUTTON_TOP
				StatusBarBottomWindow *sbbw  = static_cast<StatusBarBottomWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
				sbbw->PreviewWindownModeButtonStatus(0);
				#endif
				
            }break;
        case MSG_RECORD_FILE_DONE:	// 22
        	{   
			db_warn("[habo]--->record file done!!!!");
            isMotionDetectHappen = false;
			RecordState = 3;
			StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
			if (this->RecordMode != 0) {
				sbw->SetStatusRecordStatus(0);
			}
			ResetRecordTime();
        	}
            break;
        case MSG_CAMERA_TAKEPICTURE_ERROR:
            isTakepicFinish = true;
            //m_bTimeTakePic = false;
            break;
        case MSG_CAMERA_TAKEPICTURE_FINISHED:
            {
            	WindowManager *win_mg_ = WindowManager::GetInstance();
            	Window *win = win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM);
				StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win);
                db_warn("[habo]:MSG_CAMERA_TAKEPICTURE_FINISHED \n");
                stb->updatePhotoIcon(true);
                #if 0
                m_bTimeTakePic = false;
                MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
                int val = menuconfiglua->GetMenuIndexConfig(SETTING_PHOTO_AUTO);
                if( val == 0 )
                {
                    printf("PreviewWindow::Update MSG_CAMERA_TAKEPICTURE_FINISHED continue recount\n");
                }
                #endif
                if(!isTakepicFinish)
                {
                    isTakepicFinish = true;
//                    AudioCtrl::GetInstance()->PlaySound(AudioCtrl::AUTOPHOTO_SOUND);
					
					
                }
				if(photo_mode == MODE_PIC_TIME) {
					photoing_flag = false;
				}
                break;
            }
        case MSG_STORAGE_UMOUNT:
            {

				HideCamBRecordIcon();
                m_stop2down = false;
                if(!m_standby_flag)
                    EyeseeLinux::Screensaver::GetInstance()->ForceScreenOnBySdcard();
                db_msg("zhb----------preview_window--------MSG_STORAGE_UMOUNT  ");
				if (photoing_flag) {		
					current_count = photo_time ;
				   	photoing_flag = false;
 					StatusBarMiddleWindow *sbw = reinterpret_cast<StatusBarMiddleWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR_MIDDLE));
					sbw->HidePhotoStatusTimeUi();	
				    isTakepicFinish = true;
				}
                if( win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW && (m_BulletCollection_->getButtonDialogShowFlag())){
                    db_msg("debug_zhb--->button dialog show ready to hide");
                    m_BulletCollection_->BCDoHide();
                    break;
                }

                if(prompt_->GetPromptShowFlag() && (prompt_->getPromptId() ==PROMPT_TF_LOW_SPEED))
                {
                    prompt_->HidePromptInfo();
                }

                if(StorageManager::GetInstance()->getFormatFlag() == false && win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW)
                {
                    listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);
                    //sleep(2);
                    ShowPromptBox(PROMPT_BOX_TF_OUT,2);
                }
#ifdef USB_MODE_WINDOW
                if(usb_win_->GetVisible()){
                    db_error("Hide usb window...");
                    usb_win_->DoHide();
                }
#endif
            }
            break;
		case MSG_STORAGE_MOUNTED:
			{
				if(!m_standby_flag)
					EyeseeLinux::Screensaver::GetInstance()->ForceScreenOnBySdcard();
				//download finished and is formated need to reset the setting ui flag
				#if 0
                if((m_4g_dl_finish || m_dl_finish) && StorageManager::GetInstance()->getFormatFlag()){
                    db_msg(" update the settingwindow version update flag");
                    if(m_dl_finish){
                        VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
                        vum->ClearPackData();
                    }
                    if(m_4g_dl_finish){
                        DownLoad4GManager *dl4m = DownLoad4GManager::GetInstance();
                        dl4m->ClearPackData();
                    }
                    s_win->setNewVersionUpdateFlag(false);
                    s_win->SetUpdateVersionFlag(0);
                    m_dl_finish = false;
                }
                #endif

				if(StorageManager::GetInstance()->getFormatFlag() == false && win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW)
				{
                    ShowPromptBox(PROMPT_BOX_TF_INSERT,2);
					sleep(2);
				}

                if(win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW && StorageManager::GetInstance()->getFormatFlag() == false)//only insert sdcard by manual need to detect the sdcard if has the new version.
                {
					//
					if(DetectSdcardNewVersion() >= 0)
                        break;
                }

				int status = StorageManager::GetInstance()->GetStorageStatus();
				if(!isRecordStart && ( (status != UMOUNT) && (status != STORAGE_FS_ERROR) ))
				{
                    if(HandlerPromptInfo() != -1)
					{
                        
						if( win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW)
						{	
							// ShowPromptInfo
							if(pv_win->prompt_->GetPromptShowFlag() && pv_win->prompt_->getPromptId() == PROMPT_STANDBY_MODE){
								db_warn(" standby mode prompt has been show ,no to do anything");
								break;
							}
							
							#ifdef SUPPORT_TFCARDIN_RECORD
                            listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 1);	// 插卡自动录像
							ShowPromptBox(PROMPT_BOX_RECORDING_START,2);
							#endif
						}
					}
				}

				ShowCamBRecordIcon();
                StorageManager::GetInstance()->setFormatFlag(false);//reset the flag
            }
            break;
        case MSG_STORAGE_FS_ERROR:
            {
                if( win_mg ->GetCurrentWinID() != WINDOWID_PREVIEW)
                    break;

                if(GetRecordStatus())
                {
                    listener_->sendmsg(this, PREVIEW_RECORD_BUTTON, 0);
                }


                if(PromptBox_->GetPromptBoxShowFlag())//close the all window promptbox
                    PromptBox_->HidePromptBox();
                sleep(3);
                m_stop2down = true;
                m_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DD_TF_FS_ERROR);
                m_BulletCollection_->ShowButtonDialog();

                break;
            }
        case MSG_RECORD_AUDIO_OFF:
            db_msg("[dbeug_jaosn]:you should change the icon");
            break;
#ifdef USB_MODE_WINDOW
        case MSG_USB_HOST_CONNECTED:
            {
                db_warn("habo---> MSG_USB_HOST_CONNECTED");
				if(isRecordStart)
	            {
	                ShowPromptBox(PROMPT_BOX_DEVICE_RECORDING,2);
	                break;
	            }
				if(photoing_flag)
	            {
	                ShowPromptBox(PROMPT_BOX_DEVICE_PHOTOING,2);
	                break;
	            }
				#if 0
				if (::IsTimerInstalled(pv_win->GetHandle(),CHECKACC_TIMER_ID)) {
					KillTimer(pv_win->GetHandle(), CHECKACC_TIMER_ID);
					db_warn("habo---> MSG_USB_HOST_CONNECTED check!");
				}
				#endif
				PlaybackWindow *pw_win = reinterpret_cast<PlaybackWindow*>(win_mg->GetWindow(WINDOWID_PLAYBACK));
				
                m_bUsbDialogShow = true;
				int curwin = win_mg->GetCurrentWinID();
                if( WINDOWID_PLAYBACK == curwin)
                {
                    //stop playing 
                    PlaybackWindow *pb_win = reinterpret_cast<PlaybackWindow*>(win_mg->GetWindow(WINDOWID_PLAYBACK));
					#if 0
                    pb_win->keyPlayStopButtonProc();
                    db_msg("set playback ignore message flag true");
                    pb_win->SetPlaybackWinMessageReceiveFlag(true);
					//#else
					//pb_win->ChangePlayStatusPlaylist();
					pb_win->keyProc(SDV_KEY_MODE,0);
					#endif
					if(pb_win->IsPlayingWindow()){
						//pb_win->keyProc(SDV_KEY_MODE,0);
						pb_win->ChangePlayStatusPlaylist();
						usleep(50*1000);
						pb_win->keyProc(SDV_KEY_MODE,0);
					}else{
						pb_win->keyProc(SDV_KEY_MODE,0);
					}
                }
				// 切换回preview窗口模式
				if( WINDOWID_PREVIEW != curwin) {
					//win_mg->ChangeWindow(curwin,WINDOWID_PREVIEW,0);
				}
                win_mg->GetWindow(win_mg->GetCurrentWinID())->Hide();
				// 关闭镜头
				listener_->sendmsg(this, PREVIEW_ONCAMERA_USBMODE , 0);
                usb_win_->DoShow();
                break;
            }
        case MSG_USB_HOST_DETACHED:
            {
                db_warn("habo---> MSG_USB_HOST_DETACHED");
                #if 1
                ShutDown();
                break;
                #else
                if( !m_bUsbDialogShow)
                    break;

                m_bUsbDialogShow = false;
                usb_win_->Hide();
				// 重新打开镜头
				listener_->sendmsg(this, PREVIEW_ONCAMERA_USBMODE , 1);
				usleep(1000*1000);
                usb_win_->SetUSBWinMessageReceiveFlag(false);
                if( WINDOWID_PLAYBACK == win_mg->GetCurrentWinID())
                {
                    db_error("set playback ignore message flag false");
                    PlaybackWindow *pb_win = reinterpret_cast<PlaybackWindow*>(win_mg->GetWindow(WINDOWID_PLAYBACK));
                    pb_win->SetPlaybackWinMessageReceiveFlag(false);
					// 因为连了电脑,需要重新加载图片
					pb_win->HideDeleteDialog();
                }
				#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                EyeseeLinux::StatusBarSaver::GetInstance()->pause(false);
                if( WINDOWID_SETTING_NEW != win_mg->GetCurrentWinID()){
                    StatusBarBottomWindow * sbb = reinterpret_cast<StatusBarBottomWindow *>(win_mg->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
                    sbb->Show();
                    db_msg("status bar bottom window show");
                }
				#endif
				int curwin = ::WindowManager::GetInstance()->GetCurrentWinID();
                ResumeWindow(curwin,WIN_USBMode);

				if (curwin != WINDOWID_PREVIEW) {
					//listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
					db_error("curwin: %d",curwin);
				}
				//this->sendmsg((MSG_TYPE)MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM);
				//StatusBarWindow *sb_win = reinterpret_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
				//sb_win->Update((MSG_TYPE)MSG_SETTING_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM,0,0);
                break;
                #endif
            }
       case MSG_USB_CHARGING:
//       case MSG_USB_MASS_STORAGE_SD_REMOVE:
            {
                db_warn("habo---> MSG_USB_CHARGING");
                ResumeWindow(::WindowManager::GetInstance()->GetCurrentWinID(),WIN_USBMode);
                m_bUsbDialogShow = false;
                usb_win_->SetUSBWinMessageReceiveFlag(false);
                if( WINDOWID_PLAYBACK == win_mg->GetCurrentWinID())
                {
                   usleep(500*1000);
                   db_msg("set playback ignore message flag false");
                   PlaybackWindow *pb_win = reinterpret_cast<PlaybackWindow*>(win_mg->GetWindow(WINDOWID_PLAYBACK));
                   pb_win->SetPlaybackWinMessageReceiveFlag(false);
                }
				#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                StatusBarBottomWindow * sbb = reinterpret_cast<StatusBarBottomWindow *>(win_mg->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
                if( WINDOWID_SETTING_NEW == win_mg->GetCurrentWinID()){
                    EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
                    sbb->DoHide();
                    db_msg("status bar bottom window hide");
                }
				#endif
            }
            break;
       case MSG_USB_MASS_STORAGE:
           {
               db_warn("habo---> MSG_USB_MASS_STORAGE");
               WindowManager *win_mg_ = ::WindowManager::GetInstance();
			   #ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
               StatusBarBottomWindow * sbb = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
               EyeseeLinux::StatusBarSaver::GetInstance()->pause(true);
               sbb->DoHide();
			   #endif
               db_msg("status bar bottom window hide");
               break;
           }
#endif
        case MSG_SOFTAP_DISABLED:
            {
                //db_warn("[debug_jaosn]: MSG_SOFTAP_DISABLED \n");
                isWifiStarting = false;
            }
            break;
        case MSG_SYSTEM_POWEROFF:
            m_shutwindowObj->Show();
            break;
        case MSG_SHOW_HDMI_MASK:
            {
                WindowManager* win_m = WindowManager::GetInstance();
                if (win_m->GetCurrentWinID() == WINDOWID_PREVIEW) {
                    win_m->GetWindow(WINDOWID_STATUSBAR)->Hide();
                    win_m->GetWindow(WINDOWID_STATUSBAR_BOTTOM)->Hide();
                    #ifdef SHOW_DEBUG_INFO
                     //   ShowDebugInfo(false);
                    #endif
                }
            }
            break;
        case MSG_HIDE_HDMI_MASK:
            {
                WindowManager* win_m = WindowManager::GetInstance();
                if (win_m->GetCurrentWinID() == WINDOWID_PREVIEW) {
                    win_m->GetWindow(WINDOWID_STATUSBAR)->Show();
                    win_m->GetWindow(WINDOWID_STATUSBAR_BOTTOM)->Show();
                    #ifdef SHOW_DEBUG_INFO
                      //  ShowDebugInfo(true);
                    #endif
                    this->DoShow();
                }
            }
            break;
        case MSG_BATTERY_FULL:
            ShowPromptInfo(PROMPT_BAT_FULL, 2);
            break;
        case MSG_BATTERY_LOW:
            {
                ShowPromptInfo(PROMPT_BAT_LOW, 2);
                db_msg("isRecordStart %d, isTakepicFinish %d\n",isRecordStart,isTakepicFinish);
                if(isRecordStart)
                {
                    db_msg("stop record\n");
                    listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);//stop record
                }
                /*
                   if(isTakepicFinish)
                   {
                   db_msg("stop take picture\n");
                   listener_->sendmsg(this, PREVIEW_SHOTCUT_BUTTON , 1);//stop take picture
                   }
                   */
                db_warn("lowpower! ready to shutdown system");
                listener_->notify(this, PREVIEW_LOWPOWER_SHUTDOWN , 0); //close system
            }
            break;
        case MSG_CAMERA_ON_ERROR:{
            //ShowPromptInfo(PROMPT_CAMEAR_ERROR, 2);
#if 0
            int flag = 0;
            if(isRecordStart)
            {
               db_error("stop record MSG_CAMERA_ON_ERROR\n");
               listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);//stop record first
               flag = 1;
            }
            // reset camera
            listener_->sendmsg(this, PREVIEW_RESET_CAMERA_ON_ERROR,0);
         
            if (flag) {       
                listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 1);//restart record 
                flag = 0;
            }
#endif
            listener_->sendmsg(this, PREVIEW_RESET_CAMERA_ON_ERROR,0);
        }
            break;
        case MSG_CAMERA_REINIT_RECORD:
       {
           //ShowPromptInfo(PROMPT_CAMEAR_ERROR, 2);
           listener_->sendmsg(this, PREVIEW_RECORD_BUTTON,1);
        }
            break;
        case MSG_WIFI_CLOSE:
            {
                //ResumeWindow(m_nCurrentWin, WIN_WIFI);
                if( !m_bHdmiConnect )
                {
                    Closescreen *cs = Closescreen::GetInstance();
                    cs->SetClosescreenEnable(false);
                    cs->Stop();
                }
                db_msg("m_nCurrentWin[%d]\n",m_nCurrentWin);
            }
            break;
        case MSG_HDMI_PLUGIN:
            m_bHdmiConnect = true;
            break;
        case MSG_HDMI_PLUGOUT:
            m_bHdmiConnect = false;
            break;
        case MSG_SET_WIFI_ON:
            {
                db_warn("[debug_jason]: isWifiStarting = %d",isWifiStarting);
                if(isWifiStarting)
                    break;
                listener_->sendmsg(this, PREVIEW_WIFI_SWITCH_BUTTON , 1);
                isWifiStarting = true;
                break;
            }
        case MSG_SET_WIFI_OFF:
            db_warn("[debug_jason]: isWifiStarting = %d",isWifiStarting);
            if(!isWifiStarting)
                break;
            listener_->sendmsg(this, PREVIEW_WIFI_SWITCH_BUTTON , 0);
            break;
        case MSG_WIFI_DISABLED:
            db_warn("[debug_jason]:  11 isWifiStarting = %d",isWifiStarting);
            isWifiStarting = false;
            break;
        case MSG_DATABASE_IS_FULL:
            ShowPromptInfo(PROMPT_DATABASE_FULL, 2);
            break;
	   case MSG_STORAGE_CAP_NO_SUPPORT:
		ShowPromptInfo(PROMPT_TF_CAP_NO_SUPPORT, 3);
		break;
		case MSG_UNBIND_SUCCESS:
			db_warn("[debug_jaosn]:MSG_UNBIND_SUCCESS 111\n");
			CamRecCtrl::GetInstance()->StopAllRecord();
  		    db_error("=========StopAllRecord=========");
			listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_BINDING);
			break;
		case MSG_ADAS_EVENT:
		{
			if(prompt_->GetPromptShowFlag() && (prompt_->getPromptId() !=PROMPT_STANDBY_MODE))
			{
				prompt_->HidePromptInfo();
			}

                if(win_mg ->GetCurrentWinID() == WINDOWID_PREVIEW)
                {
                    sleep(1);
                    ShowPromptInfo(p_CamID, 10, 1);
                }
                break;
            }
        case MSG_CAMERA_MOTION_HAPPEN:
            isMotionDetectHappen = true;
            db_error("isMotionDetectHappen %d",isMotionDetectHappen);
			if (win_mg ->GetisWindowChanging()) {	// 正在切换窗口
				db_error("is WindowChanging now!");
                isMotionDetectHappen = false;
				break;
			}
			if(win_mg ->GetCurrentWinID() != WINDOWID_PREVIEW) {
                isMotionDetectHappen = false;
				break;
			}
            if(isRecordStart){
                isMotionDetectHappen = false;
                db_error("[debug_690]:the record is start do nothing\n");
            }else{
                 if(HandlerPromptInfo() == -1)
                 {
                     db_msg("TF ERROR: no tf or tf full \n");
                     isMotionDetectHappen = false;
                     break;
                 }
				 if (/*PowerManager::GetInstance()->getPowenOnType() == 2*/ 1) {		// 
					 if (PowerManager::GetInstance()->getACconnectStatus()) {
					 	db_error("do MOTION_HAPPEN record");
	                 	listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 1);
					 }
				 }
            }
            break;
        default:
            break;
    }
}

void PreviewWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    ctrl->SetCtrlTransparentStyle(true);

#ifdef SHOW_DEBUG_INFO
    #if 0
    if (ctrl_name == string("debug_info")) {
        ::ExcludeWindowStyle(ctrl->GetHandle(), SS_CENTER);
        ::IncludeWindowStyle(ctrl->GetHandle(), SS_LEFT);
    }
    #endif
#endif

   if (ctrl_name == "time_label") {// ctrl_name == "rec_time_label"
        ctrl->SetCtrlTransparentStyle(false);
        TextView* time_label = reinterpret_cast<TextView *>(ctrl);
        time_label->SetTextStyle(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

}

void PreviewWindow::showLockFileUiInfo(int value)
{
    db_warn("showLockFileUiInfo value is %d ,isRecordStart = %d",value,isRecordStart);
	WindowManager *win_mg_ = WindowManager::GetInstance();
	StatusBarWindow *sbw  = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	StatusBarBottomWindow *stb = static_cast<StatusBarBottomWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    switch(value)
    {
        case 0:
            ShowPromptBox(PROMPT_BOX_UNLOCK_FILE,2);
			sbw->LockIconHander(false);
			stb->updateLockIcon(true);
            break;
        case 1:
            ShowPromptBox(PROMPT_BOX_LOCK_FILE,2);    
			sbw->LockIconHander(true);
			stb->updateLockIcon(false);
            break;
        case 2:
            ShowPromptBox(PROMPT_BOX_LOCK_RECORD_TIP_FILE,2);
            break;
        case 3:
            ShowPromptBox(PROMPT_BOX_FILE_LOCKED,2);
			sbw->LockIconHander(true);
			stb->updateLockIcon(false);
            break;
    }
}

void PreviewWindow::HandleButtonDialogMsg(int val)
{
	switch(m_BulletCollection_->getButtonDialogCurrentId()){
		case BC_BUTTON_DIALOG_DD_NOTICE:
			db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DD_NOTICE");
			break;
		case BC_BUTTON_DIALOG_DD_NOTICE_FULL:
			db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DD_NOTICE_FULL");
			break;
		case BC_BUTTON_DIALOG_DD_TF_FS_ERROR:
			db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DD_TF_FS_ERROR");
			if(val == 1){
				usleep(300*1000);
				ShowPromptInfo(PROMPT_TF_FORMATTING,0);
			 	if(StorageManager::GetInstance()->Format() < 0)
					ShowPromptInfo(PROMPT_TF_FORMAT_FAILED,2,true);
				else
					ShowPromptInfo(PROMPT_TF_FORMAT_FINISH,2,true);
				}
			m_stop2down = false;
			StorageManager::GetInstance()->setMReadOnlyDiskFormatFinish(true);
			break;
		case BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION:
			db_msg("[debug_zhb]------BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION");
			if(val == 1){
				m_stop2down = false;
				listener_->sendmsg(this, PREVIEW_TO_SETTINGWINDOW_UPDATE_VERSION, 0);
				}else{
						m_stop2down = false;
						if(!GetRecordStatus() && (HandlerPromptInfo() != -1))
							listener_->sendmsg(this, PREVIEW_RECORD_BUTTON, 1);
					}

			break;
		case BC_BUTTON_DIALOG_MORE_IMG:
		case BC_BUTTON_DIALOG_CHECK_MD5_FAILE:
			m_stop2down = false;
			if(!GetRecordStatus() && (HandlerPromptInfo() != -1))
				listener_->sendmsg(this, PREVIEW_RECORD_BUTTON, 1);
			break;
		default:
			if(val == 1){
				db_msg("[debug_zhb]--->default p_camid = 1");
				}else{
					db_msg("[debug_zhb]--->default p_camid = 0");

					}
			break;
		}
	m_BulletCollection_->setButtonDialogShowFlag(false);
}
void PreviewWindow::VideoRecordDetect(bool start_)
{
#ifdef SETTING_WIN_USE
	db_msg("[debug_zhb]-----PreviewWindow---VideoRecordDetect  ---start_-= %d",start_);
	if(start_){
		if(isRecordStart == false && m_isRecord){
		    if(HandlerPromptInfo())
                    {
                          db_msg("[debug_zhb]: TF ERROR: no tf or tf full \n");
                          return ;
                    }
		   db_msg("[debug_zhb]---> now is stop record,ready to start  recording ");
		   m_isRecord = false;
                 listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 1);//start
		}
	}else{
		  if(isRecordStart){
		  	db_msg("[debug_zhb]---> now is recording,ready to stop recording ");
		  	m_isRecord = true;
		  	listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);//stop
		  	}else
				m_isRecord = false;
	}
#else
	db_msg("[debug_zhb]-----PreviewWindow---VideoRecordDetect  ---start_-= %d",start_);
	listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , start_);//start
#endif

}

void PreviewWindow::VideoStopRecordCtl()
{
    listener_->sendmsg(this, PREVIEW_RECORD_BUTTON , 0);    

}
void PreviewWindow::ChangeZoomTimes(int flag)
{
    switch(flag)
    {
        case 0:
        {
            if(zoom_val_==10)
                break;

            zoom_val_+=1;
            break;
        }

        case 1:
        {
            if(zoom_val_==0)
                break;

            zoom_val_-=1;
            break;
        }

        default:
            break;
    }
}


void PreviewWindow::ShowPromptInfo(unsigned int prompt_id,unsigned int showtimes,bool m_force)
{
	 db_warn("[debug_zhb]-------PreviewWindow-----ShowPromptInfo  prompt_id:%d",prompt_id);
	 if(prompt_->GetStandbyFlagStatus()) {
		 db_warn("now is the standy mode no finsh,not show prompt");
		 return;
	 }
     if (prompt_->GetPromptShowFlag()){
         if(prompt_->getPromptId() == PROMPT_STANDBY_MODE){
             db_warn("now is the standy mode show ,shuold not to do anything");
             return ;
         }
         if(!m_force){
             db_warn("if has been show ,should close and then show other");
             return ;
         }else{
             db_warn("force kill the promptinfo dialog and show other dialog");
             prompt_->HidePromptInfo();
         }
     }

     string bkgnd_bmp ;
     if(prompt_id >= PROMPT_FULL_SDCARD_FORMAT && prompt_id <=PROMPT_FULL_WIFI_CONNET){
         bkgnd_bmp = R::get()->GetImagePath("promtp_full_bg");
         prompt_->SetPosition(0, 120, GUI_SCN_WIDTH, GUI_SCN_HEIGHT-60*2);
     }else{
         bkgnd_bmp = R::get()->GetImagePath("bg_transparent");
         prompt_->SetPosition((GUI_SCN_WIDTH - 460) / 2, (GUI_SCN_HEIGHT - 232) / 2, 460, 232);
     }
     prompt_->SetWindowBackImage(bkgnd_bmp.c_str());
     prompt_->DoShow();
     prompt_->ShowPromptInfo(prompt_id,showtimes);
}

int  PreviewWindow::HandlerPromptInfo(void)
{
    if(IsHighCalssCard() == false)
    {
        db_warn("Detected is low speed card , stop to recording");
        return -1;
    }

    if(m_stop2down){
        db_warn("when power on detect the tf is low speed / fs error/ new version to update stop to going down");
        return -1;
    }
    int ret = 0;
    int status = 0;
    StorageManager *sm = StorageManager::GetInstance();
    // update storage status
    status = sm->GetStorageStatus();
    if (status == MOUNTED) {
        ret = 0;
    } else if (status == STORAGE_DISK_FULL || status == STORAGE_LOOP_COVERAGE) {
        db_warn("[debug_jaosn]:PROMPT_TF_FULL 00");
        ShowPromptInfo(PROMPT_TF_FULL,2);
        ret = -1;
    }else if(status == STORAGE_FS_ERROR){
    	db_warn("[debug_jaosn]:STORAGE_FS_ERROR 11");
        ret = -1;
    }else {
        db_warn("[debug_jaosn]:PROMPT_TF_NULL 22");
        ShowPromptInfo(PROMPT_TF_NULL,3);
        ret = -1;
    }
    return ret;
}
void PreviewWindow::ShowPromptBox(unsigned int promptbox_id,unsigned int showtimes)
{

     db_msg("[debug_zhb]-------PreviewWindow-----ShowPromptBox  promptbox_id:%d",promptbox_id);
	if(prompt_->GetPromptShowFlag() && prompt_->getPromptId() == PROMPT_STANDBY_MODE){
			db_warn(" standby mode prompt has been show ,no to do anything");
			return ;
		}

	 if(m_BulletCollection_->getButtonDialogShowFlag() && (m_BulletCollection_->getDialogCurrentId() == BC_BUTTON_DIALOG_FORMAT_SDCARD ||
	 	m_BulletCollection_->getDialogCurrentId() == BC_BUTTON_DIALOG_DD_TF_FS_ERROR||
	 	m_BulletCollection_->getDialogCurrentId() == BC_BUTTON_DIALOG_SDCARD_UPDATE_VERSION ||
	 	m_BulletCollection_->getDialogCurrentId() == BC_BUTTON_DIALOG_MORE_IMG ||
	 	m_BulletCollection_->getDialogCurrentId() == BC_BUTTON_DIALOG_CHECK_MD5_FAILE)){
			printf(" fs/version update/format mode prompt has been show ,no to do anything");
			return ;
		}
     if(promptbox_id >= PROMPT_BOX_RECORD_SOUND_OPEN && promptbox_id <= PROMPT_BOX_TF_OUT){
        #ifdef SETTING_WIN_USE
	      if( win_mg ->GetCurrentWinID() == WINDOWID_SETTING || win_mg ->GetCurrentWinID() == WINDOWID_PLAYBACK)
        #else
          if( win_mg ->GetCurrentWinID() == WINDOWID_SETTING_NEW || win_mg ->GetCurrentWinID() == WINDOWID_PLAYBACK)
        #endif
            {
		  	    db_warn("not need to show the promptbox when window id is not the previewwindow");
		  	    return;
	      	}
     	}
     if(PromptBox_->GetPromptBoxShowFlag()){
		db_warn("promptbox has been show ,no to do anything");
		return;
     	}
     int p_len = 0;
     PromptBox_->ShowPromptBox(promptbox_id,showtimes);
     p_len = PromptBox_->getPromptBoxLen();
     PromptBox_->SetPosition((GUI_SCN_WIDTH -p_len) / 2 , (GUI_SCN_HEIGHT - 48) / 2, p_len,48);
     PromptBox_->DoShow();

}

int PreviewWindow::GetRecordStatus()
{
    return isRecordStart;
}

void PreviewWindow::SetRecordMute(bool value)
{
    listener_->sendmsg(this, PREVIEW_SET_RECORD_MUTE, value);
}

int PreviewWindow::GetWindowStatus()
{
    return win_statu;
}
void PreviewWindow::SetWindowStatus(int status)
{
	win_statu = status;
}

int PreviewWindow::ResumeWindow(int p_nWinId, CurWinStatus p_Status)
{
    switch( p_nWinId )
    {
        #ifdef SETTING_WIN_USE
        case WINDOWID_SETTING:
        {
            if(WIN_WIFI == p_Status)
            {
                SettingWindow *sett_win = reinterpret_cast<SettingWindow *>(win_mg->GetWindow(p_nWinId));
                sett_win->Update();
                win_mg->GetWindow( p_nWinId)->DoShow();
                return 0;
            }
            else if( WIN_USBMode == p_Status)
            {
                SettingWindow *sett_win = reinterpret_cast<SettingWindow *>(win_mg->GetWindow(p_nWinId));
                sett_win->HideDialog();
            }
        }
        break;
        #endif
        case WINDOWID_PREVIEW:
            break;
    }


    win_mg->GetWindow( p_nWinId)->Show();

    return 0;
}

bool PreviewWindow::IsUsbAttach()
{
    return m_bUsbDialogShow;
}

#ifdef SHOW_DEBUG_INFO
void PreviewWindow::ShowDebugInfo(bool value)
{
    if (value) {
        GetControl("debug_info")->Show();
    } else {
        GetControl("debug_info")->Hide();
    }
}

void PreviewWindow::ClearDebugInfo()
{
    debug_info_.clear();
}

void PreviewWindow::InsertDebugInfo(const string &key, const string &value)
{
    debug_info_.emplace(key, value);

    UpdateDebugInfo();
}

void PreviewWindow::RemoveDebugInfo(const string &key)
{
    auto it = debug_info_.find(key);
    if (it != debug_info_.end()) {
        debug_info_.erase(key);
    }

    UpdateDebugInfo();
}

void PreviewWindow::UpdateDebugInfo()
{
    stringstream ss;

    for (auto str : debug_info_) {
        ss << str.second << "\n";
    }

    UpdateDebugInfo(ss.str());
}

void PreviewWindow::UpdateDebugInfo(const std::string &info)
{
    TextView *debug_info_label = static_cast<TextView*>(GetControl("debug_info"));
    debug_info_label->SetText(info);
}
#endif

void PreviewWindow::OnLanguageChanged()
{
#if 0
	int string_x = 0, icon_x = 0 , str_len = 0;
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
    	int val = menuconfiglua->GetMenuIndexConfig(SETTING_DEVICE_LANGUAGE);
	string str;
	if(isRecordStart){
		 R::get()->GetString("ml_preview_recording", str);
		 str_len = strlen(str.c_str());
		if(val == 1)
			str_len = str_len*11;
		else
			str_len = str_len*8;
		 string_x = s_w-12-str_len;
		 GetControl("rec_video_recording_label")->SetPosition(string_x,400,str_len,33);
		 GetControl("rec_video_recording_label")->SetCaption(str.c_str());
		 GetControl("rec_hint_icon")->SetPosition(string_x-22,409,16,16);
		}else{
			R::get()->GetString("ml_preview_record_pause", str);
			 str_len = strlen(str.c_str());
			if(val == 1)
				str_len = str_len*11;
			else
				str_len = str_len*8;
			string_x = s_w-12-str_len;
		 	GetControl("rec_pause_label")->SetPosition(string_x,400,str_len,33);
		 	GetControl("rec_pause_label")->SetCaption(str.c_str());
			GetControl("rec_hint_w_icon")->SetPosition(string_x-22,409,16,16);
		}
#endif
}


void PreviewWindow::HidePromptInfo()
{
    prompt_->HidePromptInfo();
}



//handel system version download
void PreviewWindow::DLTimerProc(union sigval sigval)
{
    PreviewWindow *pw = reinterpret_cast<PreviewWindow *>(sigval.sival_ptr);
	pw->DoCreateDLThread();
}

void PreviewWindow::DoCreateDLThread()
{
	if(version_dl_thread_id == 0){
		ThreadCreate(&version_dl_thread_id,NULL,PreviewWindow::DownLoadVersionFromNetThread,this);
		}
}

void * PreviewWindow::DownLoadVersionFromNetThread(void *arg)
{
	prctl(PR_SET_NAME, "DownLoadVersionFromNetThread", 0, 0, 0);
	PreviewWindow*pw = reinterpret_cast<PreviewWindow*>(arg);
    int running = 1;
    int ret = -1;
    int ischeck = 10;

    while(ischeck-- > 0)
    {
	    ret = pw->CheckVersionFromNet();
        if(ret != 0)
        {
            db_error("CheckVersionFromNet failed!");
            sleep(5);
            continue;
        }
        else
        {
            ischeck = 0;
            break;
        }
    }
    while(running) {
        if(pw->is_start_download == true) {
            pw->is_start_download = false;
	        int ret = pw->DownLoadVersionFromNet();
            if(ret == 0) {
                pw->is_start_download = false;
                break;
            }
            //else {
            //    pw->is_start_download = true;
            //}
	        //pthread_join(pw->version_dl_thread_id,NULL);
        }
        sleep(3);
    }
	pw->version_dl_thread_id = 0;
	//start 4G version download
	//pw->DoCreate4GDLThread();

	return NULL;
}

void PreviewWindow::SetDownloadStatus(int status)
{
    if(m_download_status != status)
    {
        m_download_status = status;
    }
}

bool PreviewWindow::IsDownloadStatusReady(void)
{
    int status = GetDownloadStatus();
    if(status != STATUS_DOWNLOAD_NETFAIL &&
            status != STATUS_DOWNLOAD_FSUNMOUNT &&
            status != STATUS_DOWNLOAD_FSFAIL)
    {
        return true;
    }
    return false;
}

int PreviewWindow::GetDownloadStatus(void)
{
	EventManager *ev = EventManager::GetInstance();
    StorageManager *sm = StorageManager::GetInstance();
	if(ev->GetNetStatus() == 0)
    {
        SetDownloadStatus(STATUS_DOWNLOAD_NETFAIL);
        db_error("STATUS_DOWNLOAD_NETFAIL!");
        return STATUS_DOWNLOAD_NETFAIL;
    }
    if(sm->GetStorageStatus() == UMOUNT)
    {
        SetDownloadStatus(STATUS_DOWNLOAD_FSUNMOUNT);
        db_error("STATUS_DOWNLOAD_FSUNMOUNT!");
        return STATUS_DOWNLOAD_FSUNMOUNT;
    }
    if(sm->GetStorageStatus() == STORAGE_FS_ERROR)
	{
        SetDownloadStatus(STATUS_DOWNLOAD_FSFAIL);
        db_error("STATUS_DOWNLOAD_FSFAIL!");
        return STATUS_DOWNLOAD_FSFAIL;
	}
    return m_download_status;
}

void PreviewWindow::SetStartDownload(bool flag)
{
    if(is_start_download != flag)
    {
        is_start_download = flag;
    }
}

int PreviewWindow::CheckVersionFromNet()
{
	m_dl_finish = false;
	char temp_path[128]={0};
	string temp_str;
	VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
	EventManager *ev = EventManager::GetInstance();
	StorageManager *sm = StorageManager::GetInstance();
    #ifdef SETTING_WIN_USE
	 SettingWindow*s_win = reinterpret_cast<SettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING));
    #endif
	while((ev->GetNetStatus() == 0) || (sm->GetStorageStatus() == UMOUNT) || (sm->GetStorageStatus() == STORAGE_FS_ERROR))
	{
//		db_warn("debug_zhb--->wait the 4G ok and sdcard ok");
		sleep(2);
	}

	//clear the pack_data
	vum->ClearPackData();
	//check the update info
	string request_str;
	request_str.clear();
	string str_qurl ;
	str_qurl.clear();
	vum->getRequestUrl(str_qurl);
	const char * qurl = str_qurl.c_str();
	if(vum->getVersionUpdateInfo(qurl,request_str) < 0)//get the version info from the url
	{
		db_error("get version update information form net failed");
		return -1;
	}


	//parspacket
	if(vum->ParsPackets(request_str) < 0)
	{
		db_error("ParsPackets failed");
		return -1;
	}
      //check if has the new version need to downlocal
	if(vum->getUpdateStatus() == 0){
		 db_error("no need to downlocal the version");
		 vum->ClearPackData();
		 return -1;
		}

	//check if sdcard net_version if has the new version
	std::vector<std::string> tmp_FileNameList;
	tmp_FileNameList.clear();
	snprintf(temp_path,sizeof(temp_path),"%s/%s/",MOUNT_PATH,VERSION_DIR_NET);
	temp_str = temp_path;
	if(getSdcardVersion(tmp_FileNameList,temp_str) >= 0){
		db_warn("scan %s  has version exist ",temp_str.c_str());
		//detect has the new version int the sdcard version need to check md5sum
		//checket sdcard the md5code
		if(vum->Md5CheckVersionPacket(true) == true)
		{
			db_error("Md5CheckVersionPacket ok no need to download again");
			if(!m_4g_dl_finish){//
				db_warn("if 4g module version has been download ok,no need to do again");
                #ifdef SETTING_WIN_USE
				s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
				// update the settingwindow version update flag
				s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
                #endif
			}

			// set new version packet len
			//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
			m_dl_finish = true;
			//return 1;
		}else{
			db_error("Md5CheckVersionPacket failed");
			memset(temp_path,0,sizeof(temp_path));
			snprintf(temp_path,sizeof(temp_path),"rm -rf %s/%s/*.img",MOUNT_PATH,VERSION_DIR_NET);
			system(temp_path);
            return -1;
		}
	}
    printf("New download update version available ...\n");
    #ifdef SETTING_WIN_USE
    s_win->setNewVersionFlag(true);
    #endif

#if 0
	//create the temp dir (/tmp/net_version/)
	vum->CreateTempDir();

	//check tmp/net_version if has the new ,do not need to update again
	std::vector<std::string> p_FileNameList;
	p_FileNameList.clear();
	if(getSdcardVersion(p_FileNameList,"/tmp/net_version/") < 0){
		db_warn("scan /tmp/net_version/ not version exist , ready to downlocal");
		vum->setLocalFileLen(0.0);//if file not exist ,shuold set the downlocal file pos from 0.0
	}else{
		db_warn("scan tmp net_version has version exist");
		for (unsigned int i=0; i<p_FileNameList.size(); i++)
		{
			//db_msg("debug_zhb--------p_FileNameList[%d] = %s",i,p_FileNameList[i].c_str());
			string::size_type rc = p_FileNameList[i].rfind('V');
			if( rc == string::npos)
			{
				db_warn("invalid fileName:%s",p_FileNameList[i].c_str());
				continue;
			}
			string filename = p_FileNameList[i].substr(rc);
			string filename_net = vum->getVersionFileName();
			string::size_type rc1 = filename_net.rfind('V');
			if( rc1 == string::npos)
			{
				db_warn("invalid filename_net:%s",filename_net.c_str());
				continue;
			}
			string filename_ = filename_net.substr(rc1);
			//db_msg("debug_zhb --- filename = %s  filename_ = %s",filename.c_str(),filename_.c_str());
			if(strncmp(filename.c_str(),filename_.c_str(),strlen(filename_.c_str())) ==  0){//判断存在的固件文件名和网络上的一样
				//需要判断已经存在的文件大小
				long long m_local_file_len = 0;
				char local_file_path[128]={0};
				snprintf(local_file_path,sizeof(local_file_path),"/tmp/net_version/%s",filename_net.c_str());
				vum->getLocalFileAllLen(local_file_path,&m_local_file_len);
				db_warn("debug_zhb--->m_local_file_len = %lld      vum->getVersionPacketLen() = %lld",m_local_file_len,(long long)vum->getVersionPacketLen());
				//判断本地文件和要下载的文件的大小对比
				if(m_local_file_len == (long long)vum->getVersionPacketLen()){
					db_warn("need to downlocal file and local file len is the same ,do not need to downlocal again");
					if(vum->MvVersion2Sdcard() < 0){//copy the version to sdcard
						db_error("mv version to sdcard failed");
						vum->ClearPackData();
						return -1;
					       	}
					vum->RemoveTempDir();//delete the tmp/net_version dir and files
					if(!m_4g_dl_finish){//
						db_warn("if 4g module version has been download ok,no need to do again");
						s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
						// update the settingwindow version update flag
						s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
					}

					// set new version packet len
					//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
					m_dl_finish = true;
					return 0;
				}else{
						vum->setLocalFileLen(m_local_file_len);
					}
			}else{
					//如果固件版本名称不一样，就删除它
					db_warn("ready to delete the other version packet");
					system("rm /tmp/net_version/* -rf");
					vum->setLocalFileLen(0.0);
				}
		}
	}

	//download new version
	string path_str,path_url;
	vum->getLoadFileOutPath(path_str,false);
	vum->getDownLoadFileUrl(path_url);
	if(vum->downLoadFile(path_url.c_str(),path_str)< 0)
	{
		db_error("downLoadFile failed");
		return -1;//下次进来接着下载
	}

	//mv img to sdcard
       if(vum->MvVersion2Sdcard() < 0){//copy the version to sdcard
		db_error("mv version to sdcard failed");
		//vum->RemoveTempDir();
		vum->ClearPackData();
		return -1;
       	}

	  //checket the md5code
	if(vum->Md5CheckVersionPacket(true) == false)
	{
		db_error("Md5CheckVersionPacket failed");
		 vum->RemoveTempDir();//delete the tmp/net_version dir and files
		vum->ClearPackData();
		return -1;
	}

	 vum->RemoveTempDir();//delete the tmp/net_version dir and files

	//set new version packet update flag
	if(!m_4g_dl_finish){
		db_warn("if 4g module version has been download ok,no need to do again");
		s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
		// update the settingwindow version update flag
		s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
	}

	// set new version packet len
	//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
	m_dl_finish = true;

    //set force update flag
	bool forceupdate = vum->getForceUpdate();
	if(vum->setForceUpdateFlag(forceupdate) != 0) {
	    db_error("setForceUpdateFlag failed!");
    }
	db_warn("downLoadFile end");
#endif
	return 0;
}

int PreviewWindow::DownLoadVersionFromNet()
{
	m_dl_finish = false;
	char temp_path[128]={0};
	string temp_str;
	VersionUpdateManager *vum = VersionUpdateManager::GetInstance();
	EventManager *ev = EventManager::GetInstance();
	StorageManager *sm = StorageManager::GetInstance();
    #ifdef SETTING_WIN_USE
	 SettingWindow*s_win = reinterpret_cast<SettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING));
    #endif
	if(ev->GetNetStatus() == 0)
    {
        SetDownloadStatus(STATUS_DOWNLOAD_NETFAIL);
        db_error("STATUS_DOWNLOAD_NETFAIL!");
        return -1;
    }
    if(sm->GetStorageStatus() == UMOUNT)
    {
        SetDownloadStatus(STATUS_DOWNLOAD_FSUNMOUNT);
        db_error("STATUS_DOWNLOAD_FSUNMOUNT!");
        return -1;
    }
    if(sm->GetStorageStatus() == STORAGE_FS_ERROR)
	{
        SetDownloadStatus(STATUS_DOWNLOAD_FSFAIL);
        db_error("STATUS_DOWNLOAD_FSFAIL!");
        return -1;
	}

#if 0
	//clear the pack_data
	vum->ClearPackData();
	//check the update info
	string request_str;
	request_str.clear();
	string str_qurl ;
	str_qurl.clear();
	vum->getRequestUrl(str_qurl);
	const char * qurl = str_qurl.c_str();
	if(vum->getVersionUpdateInfo(qurl,request_str) < 0)//get the version info from the url
	{
		db_error("get version update information form net failed");
		return -1;
	}


	//parspacket
	if(vum->ParsPackets(request_str) < 0)
	{
		db_error("ParsPackets failed");
		return -1;
	}
      //check if has the new version need to downlocal
	if(vum->getUpdateStatus() == 0){
		 db_error("no need to downlocal the version");
		 vum->ClearPackData();
		 return -1;
		}

	//check if sdcard net_version if has the new version
	std::vector<std::string> tmp_FileNameList;
	tmp_FileNameList.clear();
	snprintf(temp_path,sizeof(temp_path),"%s/%s/",MOUNT_PATH,VERSION_DIR_NET);
	temp_str = temp_path;
	if(getSdcardVersion(tmp_FileNameList,temp_str) >= 0){
		db_warn("scan %s  has version exist ",temp_str.c_str());
		//detect has the new version int the sdcard version need to check md5sum
		//checket sdcard the md5code
		if(vum->Md5CheckVersionPacket(true) == true)
		{
			db_error("Md5CheckVersionPacket ok no need to download again");
			if(!m_4g_dl_finish){//
				db_warn("if 4g module version has been download ok,no need to do again");
				s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
				// update the settingwindow version update flag
				s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
			}

			// set new version packet len
			//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
			m_dl_finish = true;
			return 1;
		}else{
			db_error("Md5CheckVersionPacket failed");
			memset(temp_path,0,sizeof(temp_path));
			snprintf(temp_path,sizeof(temp_path),"rm -rf %s/%s/*.img",MOUNT_PATH,VERSION_DIR_NET);
			system(temp_path);
			//system("rm -rf /mnt/extsd/net_version/*.img");//
			}
	}
#endif

	//create the temp dir (/tmp/net_version/)
	vum->CreateTempDir();

	//check tmp/net_version if has the new ,do not need to update again
	std::vector<std::string> p_FileNameList;
	p_FileNameList.clear();
	if(getSdcardVersion(p_FileNameList,"/tmp/net_version/") < 0){
		db_warn("scan /tmp/net_version/ not version exist , ready to downlocal");
		vum->setLocalFileLen(0.0);//if file not exist ,shuold set the downlocal file pos from 0.0
	}else{
		db_warn("scan tmp net_version has version exist");
		for (unsigned int i=0; i<p_FileNameList.size(); i++)
		{
			//db_msg("debug_zhb--------p_FileNameList[%d] = %s",i,p_FileNameList[i].c_str());
			string::size_type rc = p_FileNameList[i].rfind('V');
			if( rc == string::npos)
			{
				db_warn("invalid fileName:%s",p_FileNameList[i].c_str());
				continue;
			}
			string filename = p_FileNameList[i].substr(rc);
			string filename_net = vum->getVersionFileName();
			string::size_type rc1 = filename_net.rfind('V');
			if( rc1 == string::npos)
			{
				db_warn("invalid filename_net:%s",filename_net.c_str());
				continue;
			}
			string filename_ = filename_net.substr(rc1);
			//db_msg("debug_zhb --- filename = %s  filename_ = %s",filename.c_str(),filename_.c_str());
			if(strncmp(filename.c_str(),filename_.c_str(),strlen(filename_.c_str())) ==  0){//判断存在的固件文件名和网络上的一样
				//需要判断已经存在的文件大小
				long long m_local_file_len = 0;
				char local_file_path[128]={0};
				snprintf(local_file_path,sizeof(local_file_path),"/tmp/net_version/%s",filename_net.c_str());
				vum->getLocalFileAllLen(local_file_path,&m_local_file_len);
				db_warn("debug_zhb--->m_local_file_len = %lld      vum->getVersionPacketLen() = %lld",m_local_file_len,(long long)vum->getVersionPacketLen());
				//判断本地文件和要下载的文件的大小对比
				if(m_local_file_len == (long long)vum->getVersionPacketLen()){
					db_warn("need to downlocal file and local file len is the same ,do not need to downlocal again");
					if(vum->MvVersion2Sdcard() < 0){//copy the version to sdcard
						db_error("mv version to sdcard failed");
						vum->ClearPackData();
                        SetDownloadStatus(STATUS_DOWNLOAD_FAIL);
						return -1;
					       	}
					vum->RemoveTempDir();//delete the tmp/net_version dir and files
					if(!m_4g_dl_finish){//
						db_warn("if 4g module version has been download ok,no need to do again");
                        #ifdef SETTING_WIN_USE
						s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
						// update the settingwindow version update flag
						s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
                        #endif
					}

					// set new version packet len
					//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
					m_dl_finish = true;
                    SetDownloadStatus(STATUS_DOWNLOAD_SUCC);
					return 0;
				}else{
						vum->setLocalFileLen(m_local_file_len);
					}
			}else{
					//如果固件版本名称不一样，就删除它
					db_warn("ready to delete the other version packet");
					system("rm /tmp/net_version/* -rf");
					vum->setLocalFileLen(0.0);
				}
		}
	}

	//download new version
	string path_str,path_url;
	vum->getLoadFileOutPath(path_str,false);
	vum->getDownLoadFileUrl(path_url);
	if(vum->downLoadFile(path_url.c_str(),path_str)< 0)
	{
		db_error("downLoadFile failed");
        SetDownloadStatus(STATUS_DOWNLOAD_FAIL);
		vum->setLocalFileLen(0.0);
		vum->clearProgressPercent();
		return -1;//下次进来接着下载
	}

	//mv img to sdcard
       if(vum->MvVersion2Sdcard() < 0){//copy the version to sdcard
		db_error("mv version to sdcard failed");
		//vum->RemoveTempDir();
		vum->ClearPackData();
        SetDownloadStatus(STATUS_DOWNLOAD_FAIL);
		return -1;
       	}

	  //checket the md5code
	if(vum->Md5CheckVersionPacket(true) == false)
	{
		db_error("Md5CheckVersionPacket failed");
		 vum->RemoveTempDir();//delete the tmp/net_version dir and files
		vum->ClearPackData();
        SetDownloadStatus(STATUS_DOWNLOAD_FAIL);
		return -1;
	}

	 vum->RemoveTempDir();//delete the tmp/net_version dir and files

	//set new version packet update flag
	if(!m_4g_dl_finish){
		db_warn("if 4g module version has been download ok,no need to do again");
        #ifdef SETTING_WIN_USE
		s_win->setNewVersionUpdateFlag(vum->getUpdateStatus());
		// update the settingwindow version update flag
		s_win->SetUpdateVersionFlag(vum->getUpdateStatus());
        #endif
	}

	// set new version packet len
	//s_win->setVersionPacketLen(vum->getVersionPacketLen()/1024/1024);
	m_dl_finish = true;

    //set force update flag
	bool forceupdate = vum->getForceUpdate();
	if(vum->setForceUpdateFlag(forceupdate) != 0) {
	    db_error("setForceUpdateFlag failed!");
    }
	db_warn("downLoadFile end");
    SetDownloadStatus(STATUS_DOWNLOAD_SUCC);
	return 0;
}


//do download 4G version
void PreviewWindow::DoCreate4GDLThread()
{
	if(version_4g_dl_thread_id == 0){
		ThreadCreate(&version_4g_dl_thread_id,NULL,PreviewWindow::DownLoad4GVersionFromNetThread,this);
		}
}

void PreviewWindow::ShutDown()
{
	listener_->sendmsg(this, PREVIEW_TO_SHUTDOWN , 0);
}
void * PreviewWindow::DownLoad4GVersionFromNetThread(void *arg)
{
	prctl(PR_SET_NAME, "DownLoad4GVersionFromNetThread", 0, 0, 0);
	PreviewWindow*pw = reinterpret_cast<PreviewWindow*>(arg);
	pw->DownLoad4GVersionFromNet();
	pthread_join(pw->version_4g_dl_thread_id,NULL);
	pw->version_4g_dl_thread_id = 0;
	return NULL;
}

int PreviewWindow::DownLoad4GVersionFromNet()
{
	m_4g_dl_finish = false;
	char temp_path[128]={0};
	string temp_str;
	DownLoad4GManager *dl4m = DownLoad4GManager::GetInstance();
	EventManager *ev = EventManager::GetInstance();
	StorageManager *sm = StorageManager::GetInstance();
    #ifdef SETTING_WIN_USE
	 SettingWindow*s_win = reinterpret_cast<SettingWindow*>(win_mg->GetWindow(WINDOWID_SETTING));
    #endif
	while((ev->GetNetStatus() == 0) || (sm->GetStorageStatus() == UMOUNT) || (sm->GetStorageStatus() == STORAGE_FS_ERROR))
	{
		//db_warn("debug_zhb--->wait the 4G ok and sdcard ok");
		sleep(2);
	}

	//clear the pack_data
	dl4m->ClearPackData();
	//check the update info
	string request_str;
	request_str.clear();
	string str_qurl ;
	str_qurl.clear();
	dl4m->getRequestUrl(str_qurl);
	const char * qurl = str_qurl.c_str();
	if(dl4m->getVersionUpdateInfo(qurl,request_str) < 0)//get the version info from the url
	{
		db_error("get version update information form net failed");
		return -1;
	}


	//parspacket
	if(dl4m->ParsPackets(request_str) < 0)
	{
		db_error("ParsPackets failed");
		return -1;
	}
      //check if has the new version need to downlocal
	if(dl4m->getUpdateStatus() == 0){
		 db_error("no need to downlocal the version");
		 dl4m->ClearPackData();
		 return -1;
		}

	//check if sdcard net_version if has the new version
	std::vector<std::string> tmp_FileNameList;
	tmp_FileNameList.clear();
	snprintf(temp_path,sizeof(temp_path),"%s/%s/",MOUNT_PATH,VERSION_4G_DIR_NET);
	temp_str = temp_path;
	if(getSdcardBin(tmp_FileNameList,temp_str,".bin") >= 0){
		db_warn("scan %s*.bin  has version exist ",temp_str.c_str());
		//detect has the new version int the sdcard version need to check md5sum
		//checket sdcard the md5code
		if(dl4m->Md5CheckVersionPacket(true) == true)
		{
			db_error("Md5CheckVersionPacket ok no need to download again");
			if(!m_dl_finish){//
				db_warn("if system version has been download ok,no need to do again");
                #ifdef SETTING_WIN_USE
				s_win->setNewVersionUpdateFlag(dl4m->getUpdateStatus());
				// update the settingwindow version update flag
				s_win->SetUpdateVersionFlag(dl4m->getUpdateStatus());
                #endif
			}

			// set new version packet len
			//s_win->setVersionPacketLen(dl4m->getVersionPacketLen()/1024/1024);

			m_4g_dl_finish = true;
			return 1;
		}else{
			db_error("Md5CheckVersionPacket failed");
			//system("rm -rf /mnt/extsd/net_4g_version/*.bin");//
			memset(temp_path,0,sizeof(temp_path));
			snprintf(temp_path,sizeof(temp_path),"rm -rf %s/%s/*.bin",MOUNT_PATH,VERSION_4G_DIR_NET);
			system(temp_path);
			}
	}

	//create the temp dir (/tmp/net_4g_version/)
	dl4m->CreateTempDir();

	//check tmp/net_4g_version if has the new ,do not need to update again
	std::vector<std::string> p_FileNameList;
	p_FileNameList.clear();
	if(getSdcardBin(p_FileNameList,"/tmp/net_4g_version/",".bin") < 0){
		db_warn("scan /tmp/net_4g_version/ not version exist , ready to downlocal");
		dl4m->setLocalFileLen(0.0);//if file not exist ,shuold set the downlocal file pos from 0.0
	}else{
		db_warn("scan tmp net_4g_version has version exist");
		for (unsigned int i=0; i<p_FileNameList.size(); i++)
		{
			//db_msg("debug_zhb--------p_FileNameList[%d] = %s",i,p_FileNameList[i].c_str());
			string::size_type rc = p_FileNameList[i].rfind('V');
			if( rc == string::npos)
			{
				db_warn("invalid fileName:%s",p_FileNameList[i].c_str());
				continue;
			}
			string filename = p_FileNameList[i].substr(rc);
			string filename_net = dl4m->getVersionFileName();
			string::size_type rc1 = filename_net.rfind('V');
			if( rc1 == string::npos)
			{
				db_warn("invalid filename_net:%s",filename_net.c_str());
				continue;
			}
			string filename_ = filename_net.substr(rc1);
			db_msg("debug_zhb --- filename = %s  filename_ = %s",filename.c_str(),filename_.c_str());
			if(strncmp(filename.c_str(),filename_.c_str(),strlen(filename_.c_str())) ==  0){//ÅÐ¶Ï´æÔÚµÄ¹Ì¼þÎÄ¼þÃûºÍÍøÂçÉÏµÄÒ»Ñù
				//ÐèÒªÅÐ¶ÏÒÑ¾­´æÔÚµÄÎÄ¼þ´óÐ¡
				long long m_local_file_len = 0;
				char local_file_path[128]={0};
				snprintf(local_file_path,sizeof(local_file_path),"/tmp/net_4g_version/%s",filename_net.c_str());
				dl4m->getLocalFileAllLen(local_file_path,&m_local_file_len);
				//db_warn("m_local_file_len = %lld      vum->getVersionPacketLen() = %lld",m_local_file_len,(long long)dl4m->getVersionPacketLen());
				//ÅÐ¶Ï±¾µØÎÄ¼þºÍÒªÏÂÔØµÄÎÄ¼þµÄ´óÐ¡¶Ô±È
				if(m_local_file_len == (long long)dl4m->getVersionPacketLen()){
					db_warn("need to downlocal file and local file len is the same ,do not need to downlocal 4g version again");
					if(dl4m->MvVersion2Sdcard() < 0){//copy the version to sdcard
						db_error("mv 4g version to sdcard failed");
						dl4m->ClearPackData();
						return -1;
					       	}
					dl4m->RemoveTempDir();//delete the tmp/net_version dir and files

					if(!m_dl_finish){//
						db_warn("if system version has been download ok,no need to do again");
                        #ifdef SETTING_WIN_USE
						s_win->setNewVersionUpdateFlag(dl4m->getUpdateStatus());
						// update the settingwindow version update flag
						s_win->SetUpdateVersionFlag(dl4m->getUpdateStatus());
                        #endif
					}

					// set new version packet len
					//s_win->setVersionPacketLen(dl4m->getVersionPacketLen()/1024/1024);

					m_4g_dl_finish = true;
					return 0;
				}else{
						dl4m->setLocalFileLen(m_local_file_len);
					}
			}else{
					//Èç¹û¹Ì¼þ°æ±¾Ãû³Æ²»Ò»Ñù£¬¾ÍÉ¾³ýËü
					db_warn("ready to delete the other version packet");
					system("rm /tmp/net_4g_version/* -rf");
					dl4m->setLocalFileLen(0.0);
				}
		}
	}

	//download new version
	string path_str,path_url;
	dl4m->getLoadFileOutPath(path_str,false);
	dl4m->getDownLoadFileUrl(path_url);
	if(dl4m->downLoadFile(path_url.c_str(),path_str)< 0)
	{
		db_error("downLoadFile failed");
		return -1;//ÏÂ´Î½øÀ´½Ó×ÅÏÂÔØ
	}


       if(dl4m->MvVersion2Sdcard() < 0){//copy the version to sdcard
		db_error("mv version to sdcard failed");
		dl4m->ClearPackData();
		//dl4m->RemoveTempDir();//delete the tmp/net_version dir and files
		return -1;
       	}

	 //checket the md5code
	if(dl4m->Md5CheckVersionPacket(true) == false)
	{
		db_error("Md5CheckVersionPacket failed");
		 dl4m->RemoveTempDir();//delete the tmp/net_version dir and files
		dl4m->ClearPackData();
		return -1;
	}

	 dl4m->RemoveTempDir();//delete the tmp/net_version dir and files

	//set new version packet update flag
	if(!m_dl_finish){//
		db_warn("if system version has been download ok,no need to do again");
        #ifdef SETTING_WIN_USE
		s_win->setNewVersionUpdateFlag(dl4m->getUpdateStatus());
		// update the settingwindow version update flag
		s_win->SetUpdateVersionFlag(dl4m->getUpdateStatus());
        #endif
	}
	// set new version packet len
	//s_win->setVersionPacketLen(dl4m->getVersionPacketLen()/1024/1024);

	m_4g_dl_finish = true;
	db_warn("downLoadFile 4g version end");
	return 0;
}


bool PreviewWindow::getIsRecordStartFlag()
{
    return isRecordStart;
}

void PreviewWindow::ChangeWindowStatus(int newstatus)
{
	if (isRecordStart || photoing_flag) {
		db_error("isRecordStart or Takepic not Finish");
		return;
	}
	//AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
	listener_->sendmsg(this,PREVIEW_CHANGEWINDOWSTATUS,newstatus);
}

void PreviewWindow::mySetRecordval()
{
	WindowManager *win_mg_ = WindowManager::GetInstance();
	NewSettingWindow *nw  = static_cast<NewSettingWindow*>(win_mg_->GetWindow(WINDOWID_SETTING_NEW));
	MenuConfigLua *menuconfiglua=MenuConfigLua::GetInstance();
 	int val = menuconfiglua->GetMenuIndexConfig(SETTING_RECORD_RESOLUTION);
			
	nw->SetListViewItemEx(0,val,0);	// 0= SETTING_RECORD_RESOLUTION    val=id 0=for video
}
void PreviewWindow::ResetPhotoMode()
{
	photo_mode = MODE_PIC_NORMAL;
	photo_time = 0;
	photoing_flag = 0;
}

void PreviewWindow::OnOffCamera(int camid, int status)
{
	listener_->sendmsg(this, PREVIEW_ONCAMERA_FROM_PLAYBACK, status);
}



