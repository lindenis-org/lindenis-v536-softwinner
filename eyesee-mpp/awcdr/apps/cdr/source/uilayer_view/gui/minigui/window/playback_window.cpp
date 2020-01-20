/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file playback_window.cpp
 * @brief 回放界面
 * @author id:826
 * @version v0.3
 * @date 2016-11-03
 */
#define NDEBUG 

#include "window/preview_window.h"
#include "window/playback_window.h"
#include "window/dialog.h"
#include "window/playlist_window.h"
#include "debug/app_log.h"
#include "widgets/graphic_view.h"
#include "resource/resource_manager.h"
//#include "widgets/button.h"
//#include "widgets/text_view.h"
#include "widgets/switch_button.h"
#include "widgets/progress_bar.h"
#include "window/window_manager.h"
#include "common/message.h"
#include "common/posix_timer.h"
#include "common/setting_menu_id.h"
#include "application.h"
#include "device_model/system/power_manager.h"
#include "bll_presenter/screensaver.h"
#include "bll_presenter/audioCtrl.h"
#include <thread>
#include "window/promptBox.h"
#include "window/prompt.h"
#include "device_model/storage_manager.h"
#include "window/bulletCollection.h"
#include "window/status_bar_bottom_window.h"
#include "common/utils/utils.h"
#include "bll_presenter/statusbarsaver.h"
#include "window/usb_mode_window.h"



#define FILE_CREATE_TIME 

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "PlaybackWindow"
using namespace std;
IMPLEMENT_DYNCRT_CLASS(PlaybackWindow)

static const char *voice_icons[5] = {"silent", "voice1", "voice2", "voice3", "voice4"};
#ifdef USEICONTHUMB
static int lockxx = 0;
static int lastupdate = -1;
#endif
#if 1
void PlaybackWindow::keyProc(int keyCode, int isLongPress)
{
    pthread_mutex_lock(&playback_proc_lock_);
	if(play_BulletCollection_->m_button_dialog_flag_){
	//	db_warn("[debug_zhb]--->play_BulletCollection_->m_button_dialog_flag_");
		select_status = 0;
		play_BulletCollection_->keyProc(keyCode, isLongPress);
    	if(keyCode == SDV_KEY_MENU || keyCode == SDV_KEY_MODE){
    	//	db_warn("[debug_zhb]--->222222222222222222222222");
			ivplayback_thumb_->SetIconItem_selectall(0);
			SetFileInfoSelect(-1,0);
    		SetPlaybackStatusIconInvalidFlag(false);
    	}
		goto out;
	}
	if(ignore_message_flag_){
        db_error("usb host connect,ignore message!!!");
        //return;
        goto out;
    }
    switch(keyCode){
        case SDV_KEY_MODE://return 
        {
            db_warn("[debug_zhb]--->playwindow SDV_KEY_MENU");
            if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }
            if(!IsPlayingWindow())//now is the playback windown
            {
                if(!IsDeleteDialogWindow())
                {
                    WindowManager *win_mg = ::WindowManager::GetInstance();
                    PreviewWindow* pw  = static_cast<PreviewWindow*>(win_mg->GetWindow(WINDOWID_PREVIEW));
					int win_statu_save = pw->Get_win_statu_save();
                    #ifdef STOPCAMERA_TO_PLAYBACK
					//pw->OnOffCamera(0,win_statu_save);
                    #endif
                    StatusBarWindow* sbw  = static_cast<StatusBarWindow*>(win_mg->GetWindow(WINDOWID_STATUSBAR));
					
                    
                    ivplayback_thumb_->Hide();
                    //HideVideoPlayImgAndIcon(true);
                    db_error("habo---> from the playback window to return preview windown !!!");
					//listener_->sendmsg(this, PLAYBACK_HIDETHUMB, 0);
                    #if 0
					// 停止iconview更新图片
					if (lockxx) {
						int count = 100;
						CancelUpdateListview = true;
						while (CancelUpdateListview) {
							usleep(50*1000);	
							count--;
							if (!count) break;
						}
					}
					#endif
					//if (lockxx) {
					//	db_warn("system busy,ignore this message");
					//	//break;
					//}
					listview_flag_= false;
					CancelUpdateListview = true;
					//db_warn("I wantto stop CancelUpdateListview");
					ivplayback_thumb_->Hide();
					//m_playback_thumb_bk->Show();
					//this->Refresh();
					//usleep(500*1000);
					//usleep(1000*1000);
                    //usleep(1000*1000);
                    //usleep(1000*1000);
					ivplayback_thumb_->RemoveAllIconItem();
					//ivplayback_thumb_->Refresh();
					//usleep(250*1000);
                    #ifdef S_PLAYB_STATUSBAR
                    playBackSatutarbar(false);
                    #endif
                    select_file_ = 0;
                    page_num_ = 1;
					#ifdef USEICONTHUMB
					select_status = 0;
					#endif
                    ResetFlagStatus();
					#ifdef SUPPORT_AUTOHIDE_STATUSBOTTOMBAR
                    EyeseeLinux::StatusBarSaver::GetInstance()->pause(false);
					#endif
					
					
					//pw->OnOffCamera(0,win_statu_save);
					//this->StopCamera(0);
                    listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW, 0);
                    usleep(500 * 1000);//等待playback执行完MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW消息

                    listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
                    
					CancelUpdateListview = false;
                }

            }
            else //now is the playing windown
            {
                db_msg("habo---> from playing window to return the playback windown !!!");
                HideStatusbar();
                Screensaver::GetInstance()->pause(false);//start the screensaver timer
                listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
                listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);
                PlayingToPlaybackUpdateShowLeftRightIcon();
                ResetFlagStatus();
                #ifdef S_PLAYB_STATUSBAR
                playBackSatutarbar(true);
                #endif
                break;

            }
        }break;

        case SDV_KEY_RIGHT://Album previous page ; fast rewind ;previous video 
        {
            db_warn("[debug_zhb]--->playwindow SDV_KEY_OK");
            if(m_nofile_nosdcard_)
            {
                db_warn("[debug_zhb]---> no file or no sdcard status");
                goto out;
            }

            if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }
            if(!IsPlayingWindow())
            {
                if(IsDeleteDialogWindow())
               {
                    db_warn("now is show the delete files dialog ,ignore this message");
                    break;
               }
			   #ifdef USEICONTHUMB
               //Album previous page
               db_msg("habo---> ready to do album previous page  !!!");
                db_warn("page_num_ %d  select_file_ %d page_sum_ %d",
                page_num_,select_file_,page_sum_);
                if(isLongPress == LONG_PRESS) //handle fling update page
                {
                    if(page_num_ == page_sum_) 
                    {
                        if(page_num_ == 1)
                        { //files only has one page 
                            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_HIDE);
                            break;
                        } 
                        else if(page_num_ != 1)
                        {
                            page_num_ --;
                            if(page_num_ == 1) 
                            {
                                listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,RIGHT_SHOW_ONLY);
                            } 
                            else 
                            { //到末页,往上翻
                                listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_SHOW);
                            }
                        }
                    }
                    else if(page_num_ < page_sum_) 
                    {
                       if(page_num_ == 1) 
                        { //文件大于一页且正处于首页
                            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,RIGHT_SHOW_ONLY);
                            break;
                        }
                        else
                            page_num_ --;
                        if(page_num_ == 1) 
                        { //已经翻到首页
                            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,RIGHT_SHOW_ONLY);
                        }
                        else 
                        { //δ����ҳ
                            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_SHOW);
                        }
                    }
                    db_warn("page_num_ %d  select_file_ %d page_sum_ %d",page_num_,select_file_,page_sum_);
                    if(select_file_ > 0)
                    {
                        select_file_-=6;
                        UpdateListView(true);
                    }
                }
                else //handle button update pre select
                { 
                    if(select_file_ > 0)
                     {
                        select_file_--;
                        if(select_file_%IMAGEVIEW_NUM == (IMAGEVIEW_NUM-1)){
                            UpdateListView(false);
                            if(page_num_ > 1)
                                page_num_ --;
                        }
                        else
                        	UpdateListView(false);
                    }else{
						db_warn("6666666666666666666");
						PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
						pre_win->ShowPromptBox(PROMPT_BOX_FIRST_VIDEO);
					}
					ivplayback_thumb_->Refresh();
                }
				#else
				// delete select 删除选择的文件
				if (GetFileInfoSelectCount()) {
					ShowDeleteDialog(2);
				}
				#endif
            }
            else
            {
               
               if (isLongPress == SHORT_PRESS) //handle switch previous video
              {
                db_warn("[debug_zhb]--->playwindow SDV_KEY_OK  play_flag_ = %d select_flag_ = %d",play_flag_,select_flag_);
    				if(IsPlayingWindow() && (!select_flag_))
                     {
                        db_warn("habo---> ready to do switch previous video  !!!");
    					PreVideoPlay();
						#ifndef USEICONTHUMB
    					db_warn("select file %d,page_num_ %d",select_file_,page_num_);
    					if(select_file_  < page_num_ * IMAGEVIEW_NUM && ((select_file_ + 1) == (IMAGEVIEW_NUM * (page_num_ - 1))))
                        {
    						page_num_--;
    						db_warn("page_num_ %d",page_num_);
    					}
						#endif
    				 }
			   }
               else //key long msg  is  do fast rewind
               {
                   if ((player_status_ == PAUSED || player_status_ == PLAYING))
                    {
                        db_msg("habo---> ready to do video playing fast rewind  !!!");
        				std::thread([=] 
                        {
        					if(PAUSED == player_status_)
                            {
        						if(select_mode_ == PLAY_DELETE)
                                {
        							listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
        							select_mode_ = PLAY_STOP;
//        							UpdateVideoPlayImg();
        						}
        						HideVideoPlayImg();
        						keyPlayPauseButtonProc();
        					}
        					while(!isKeyUp) 
                            {
        						usleep(100*1000);
        						listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 0);
        					}
        					stop_timer(play_timer_id_);
        					set_period_timer(1, 0, play_timer_id_);
        				}).detach();
				   }
                
               }
            }
            
        }break;

        case SDV_KEY_RETURN://unlock and locked ; Next video ;fast forward 
        {
            db_warn("[debug_zhb]--->playwindow SDV_KEY_MODE");
            
            if(m_nofile_nosdcard_)
            {
                db_warn("[debug_zhb]---> no file or no sdcard status");
                goto out;
            }

            if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }
            db_warn("[debug_zhb]--->playwindow SDV_KEY_MODE player_status_ = %d ",player_status_);
            if(IsPlayingWindow())
            {/*
                if(!IsDeleteDialogWindow())
                {
                    //handle next video
                    if (isLongPress == SHORT_PRESS) 
                    {
                        db_warn("[debug_zhb]--->playwindow SDV_KEY_MODE  1111111111");
                        if(invalid_flag_ == true) 
                        {
                            db_warn("show delete file dialog,ignore this message");
                            break;
                        }
                        db_warn("[debug_zhb]--->playwindow SDV_KEY_MODE  play_flag_ = %d select_flag_ = %d",play_flag_,select_flag_);
                        if(IsPlayingWindow() && (!select_flag_))
                        {
                            db_warn("habo---> ready to do next video  !!!");
                            NextVideoPlay();
							#ifndef USEICONTHUMB
                            db_warn("select file %d,page_num_ %d",select_file_,page_num_);
                            if(select_file_  >= page_num_ * IMAGEVIEW_NUM)
                            {
                                page_num_++;
                                db_warn("page_num_ %d",page_num_);
                            }
							#endif
                        }
                    }
                    else//long key msg do fast forward 
                    {
                        if ((player_status_ == PAUSED || player_status_ == PLAYING) && isLongPress == LONG_PRESS)
                        {
                            db_msg("habo---> ready to do  video playing fast forward  !!!");
                            std::thread([=] 
                            {
                                if(PAUSED == player_status_)
                                {
                                    if(select_mode_ == PLAY_DELETE)
                                    {
                                        listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
                                        select_mode_ = PLAY_STOP;
//                                        UpdateVideoPlayImg();
                                    }
                                    HideVideoPlayImg();
                                    keyPlayPauseButtonProc();
                                }									
                                while(!isKeyUp) 
                                {
                                    usleep(100*1000);
                                    listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 1);
                                }
                                stop_timer(play_timer_id_);
                                set_period_timer(1, 0, play_timer_id_);										
                            }).detach();
                        }

                    }
                }*/
            }
            else
            {
                //do files lock and unlock
                db_msg("To do files lock and unlock msg");
                FileLockCtl();
				#ifdef USEICONTHUMB
				UpdateListView(UPDATEVIEW_NOLOAD);
				#endif
            }

        }break;
		case SDV_KEY_MENU:
		{
			if(m_nofile_nosdcard_)
            {
                db_warn("[debug_zhb]---> no file or no sdcard status");
                goto out;
            }

           /* if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }*/
			if(!IsPlayingWindow()){
				db_warn("SDV_KEY_MENU select_status = %d",select_status);
                 if (select_status == 0) {
    				ivplayback_thumb_->SetIconItem_select(select_file_,2);	// 待选择状态
    				SetFileInfoSelect(select_file_,2);	
    				if (GetFileInfoSelectCount()) {
    					ShowDeleteDialog(1);
    				}
					select_status = 1;
				}
    			ivplayback_thumb_->Refresh();
			} else
            {	// 目前在播放界面
                db_msg("habo---> ready to do delete video !!!");
                //delete video 
                if ((player_status_ == PAUSED || player_status_ == PLAYING || player_status_ == COMPLETION)) 
                {
                    //check the files type 
                    int type_ = -1;
					#ifndef USEICONTHUMB
					type_ = mediaf_image_[select_file_%IMAGEVIEW_NUM].fileType;
					#else
					std::map<int, FileInfo>::iterator itr;
					itr = FileInfo_.find(select_file_);
					if (itr ==FileInfo_.end()) {
						// no found
						db_error("no key in FileInfo_");
						break;
					}
					
                    type_ = (itr->second).fileType;
					db_error("delete filetype: %d",type_);
					#endif
                    if(type_ == VIDEO_A_SOS || type_ == VIDEO_B_SOS || type_ == VIDEO_A_P || type_ == VIDEO_B_P)
                    {
                        //show dialog current file is sos type ,should not be deleted
                        PreviewWindow *p_win  = static_cast<PreviewWindow*>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
                        p_win->ShowPromptBox(PROMPT_BOX_FILE_LOCKED,2);
                        break;
                    }
					if (player_status_ == PLAYING)
						keyPlayPauseButtonProc();
					listener_->sendmsg(this, PLAYBACK_SET_SHOW_THS, select_file_);
					listener_->notify(this, MSG_PLAYBACK_SHOW_DELETE_DIALOG, getCurrentFileType());
					listener_->sendmsg(this, MSG_PLAYBACK_SHOW_GRAY_ICON,0);
					SetPlaybackStatusIconInvalidFlag(true);
					db_warn("show delete file dialog,set invalid flag true ");
				}
            }
		}
		break;
		case SDV_KEY_LEFT://Album next page ;delete video
		{
            db_warn("[debug_zhb]--->playwindow SDV_KEY_RIGHT");
            
            if(m_nofile_nosdcard_)
            {
                db_warn("[debug_zhb]---> no file or no sdcard status");
                goto out;
            }

            if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }
            if(!IsPlayingWindow())
            {	// 目前在缩略图界面
                //Album next page 
                if (player_status_ == STOPED) 
               {
					db_warn("select_file_%d file_sum_%d page_num_ %d page_sum_ %d",select_file_,file_sum_,page_num_,page_sum_);
                    if(isLongPress == LONG_PRESS) //handle fling update page
                    {
						#ifndef USEICONTHUMB
                        if(page_sum_  > page_num_)
                        {
                            db_msg("habo---> ready to do album next page !!!");
                            if(page_num_  +1 == page_sum_)
                            {
                                //handle the last page selecte num
                                select_file_ = file_sum_ - 1;
                            }
                            else
                            {
    						    select_file_ +=IMAGEVIEW_NUM;
                            }
    						UpdateListView(true);
    						page_num_ ++;
    						int tmp = (page_num_)*IMAGEVIEW_NUM;
    						if(tmp >= file_sum_)
                            {
    							listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_SHOW_ONLY);
    						}
                            else 
                            {
    							listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_SHOW);
    						}
    					} 
                        else 
                        {
    						listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_HIDE);//LEFT_SHOW_ONLY
    					}
						#endif
                    }
					
                    else //handle button update next select
                    {
#ifdef USEICONTHUMB
						if(select_file_ < file_sum_ - 1)
                        {
                            select_file_++;
                            if(select_file_%IMAGEVIEW_NUM)
                                UpdateListView(UPDATEVIEW_NOLOAD);
                            else{
                                UpdateListView(UPDATEVIEW_NOLOAD);
                                if(page_sum_  > page_num_)
                                    page_num_ ++;
                            }

                        }else{
							db_warn("2222222222222222222222");							
							PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
							pre_win->ShowPromptBox(PROMPT_BOX_LAST_VIDEO);
							
						}
						ivplayback_thumb_->Refresh();
                        
#else
						else


                        {
						
                        if (select_status == 0) {
							ivplayback_thumb_->SetIconItem_selectall(1);
							SetFileInfoSelect(-1,2);
							select_status = 1;
                        } else {
							ivplayback_thumb_->SetIconItem_selectall(0);
							SetFileInfoSelect(-1,0);
							select_status = 0;
                        }
						
						ivplayback_thumb_->Refresh();
						
                        }
#endif

                    }
					
			   }
            }
            else
            {	// 目前在播放界面
				db_warn("[debug_zhb]--->playwindow SDV_KEY_left 1111111111");
				if(invalid_flag_ == true) 
				{
					db_warn("show delete file dialog,ignore this message");
					break;
				}
				db_warn("[debug_zhb]--->playwindow SDV_KEY_MODE  play_flag_ = %d select_flag_ = %d",play_flag_,select_flag_);
				if(IsPlayingWindow() && (!select_flag_))
				{
					db_warn("habo---> ready to do next video  !!!");
					NextVideoPlay();
#ifdef USEICONTHUMB
				//	db_warn("select file %d,page_num_ %d",select_file_,page_num_);
					if(select_file_  >= page_num_ * IMAGEVIEW_NUM)
					{
						page_num_++;
						db_warn("page_num_ %d",page_num_);
					}
#endif
				}

            }

        }break;

        case SDV_KEY_OK://start and pause play 
        {
            db_warn("[debug_zhb]--->playwindow SDV_KEY_LEFT");
            
            if(m_nofile_nosdcard_)
            {
                db_warn("[debug_zhb]---> no file or no sdcard status");
                goto out;
            }

            if(invalid_flag_ == true) 
            {
                db_warn("show delete file dialog,ignore this message");
                goto out;
            }
            if(IsPlayingWindow())
            {
                db_warn("habo ---->  sdv key left  player_status_ = %d ",player_status_);
                 if(player_status_ == PLAYING || player_status_ == PAUSED )
                 {
					 if(m_nofile_nosdcard_ || m_delete_flag ||StorageManager::GetInstance()->getUmountFlag())
                    {
						db_warn(" no file or no sdcard status / Refreshing /sdcard umount,shuold stop to do anything");
						break;
					}
                    db_warn("habo ---->  sdv key left  select_mode_ = %d ",select_mode_);
					if(select_mode_ == PLAY_STOP)
                    {
                        db_msg("~~~~~~~~~~~~~ player_status_ is playing or paused ");
						keyPlayPauseButtonProc();
					}
				 } 
                 else if(player_status_ == COMPLETION)
                 {
                    db_msg("~~~~~~~~~~~~~ player_status_ is COMPLETION ");
					 keyPlayPauseButtonProc();
				 }
                
            }
            else
            {
                db_warn("habo---> ready to play the video");
                //if completion ,replay .if stoped ,start play
                if(player_status_ == STOPED)
                {
                    #ifdef S_PLAYB_STATUSBAR
                    playBackSatutarbar(false);
                    #endif
                    listview_flag_ = true;
                    keyPlayPauseButtonProc(); 
                    listener_->sendmsg(this, MSG_PLAYBACK_TO_PLAY_WINDOW, 0);
                }
            }
        }break;
		default:
			db_msg("[habo]:invild keycode");
			break;
	}
out:
	
    pthread_mutex_unlock(&playback_proc_lock_);
	
}
#else
#if 0
void PlaybackWindow::keyProc(int keyCode, int isLongPress)
{
    pthread_mutex_lock(&playback_proc_lock_);
    switch(keyCode){
		case SDV_KEY_LEFT: //Album up page ;  playback fast forward ; previous video
			db_warn("[debug_zhb]--->SDV_KEY_LEFT");
			{ 
			     if(m_nofile_nosdcard_){
			    	 db_warn("[debug_zhb]---> no file or no sdcard status");
				 	break;
			     	}
		            if (isLongPress == SHORT_PRESS) {
				if(invalid_flag_ == true) {
					db_warn("show delete file dialog,ignore this message");
					break;
				}
				 if(IsPlayingWindow() && (!select_flag_)){
						 NextVideoPlay();
						 db_warn("select file %d,page_num_ %d",select_file_,page_num_);
						 if(select_file_  >= page_num_ * 6){
							 page_num_++;
							 db_warn("page_num_ %d",page_num_);
						 }
				 }else{
					  if ( player_status_ == STOPED){
						  db_warn("page_num_ %d  select_file_ %d page_sum_ %d",
								  page_num_,select_file_,page_sum_);
							if(page_num_ == page_sum_) {
								if(page_num_ == 1){ //files only has one page 
									 listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,0);
									 listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,0);
									 break;
								} else if(page_num_ != 1){
								    page_num_ --;
								    if(page_num_ == 1) {
								    	listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,0);
										listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
//								    	break;
									} else { //到末页,往上翻
										#ifndef USEICONTHUMB
								    	listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,1);
										#endif
										listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
									}
								}
							}else if(page_num_ < page_sum_) {
								if(page_num_ == 1) { //文件大于一页且正处于首页
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,0);
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
									break;
								}
								else
									page_num_ --;
								if(page_num_ == 1) { //已经翻到首页
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,0);
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
								} else { //δ����ҳ
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,1);
									listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
								}
							}
							db_warn("page_num_ %d  select_file_ %d page_sum_ %d",
												  page_num_,select_file_,page_sum_);
							if(select_file_ > 0){
								select_file_-=6;
								UpdateListView(true);
							}
#if 0
							if(select_file_ > 0){
									select_file_--;
									if(select_file_%IMAGEVIEW_NUM == (IMAGEVIEW_NUM-1)){
										UpdateListView(true);
									}else
										UpdateListView(false);
								}
#endif
			                }else if(player_status_ == PAUSED){
								if(select_mode_ == PLAY_STOP)
									select_mode_ = PLAY_DELETE;
								else
									select_mode_ = PLAY_STOP;
								UpdateVideoPlayImg();
								listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, select_mode_ ==PLAY_STOP?1:0);
			                }
				 	}				
			            } else {
					   if ((player_status_ == PAUSED || player_status_ == PLAYING) && isLongPress == LONG_PRESS){
			                    std::thread([=] {
									if(PAUSED == player_status_){
										if(select_mode_ == PLAY_DELETE){
											listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
											select_mode_ = PLAY_STOP;
											UpdateVideoPlayImg();
										}
										HideVideoPlayImg();
										keyPlayPauseButtonProc();
									}									
			                        while(!isKeyUp) {
										usleep(100*1000);
									listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 1);
			                        }
									stop_timer(play_timer_id_);
									set_period_timer(1, 0, play_timer_id_);										
			                    }).detach();
			                }
			            }
			}break;
		case SDV_KEY_RIGHT: //播放界面删除键
			{
				db_warn("=========SDV_KEY_RIGHT=========");
			    if(m_nofile_nosdcard_){
					db_warn("[debug_zhb]---> no file or no sdcard status");
					break;
				}
			    if(invalid_flag_ == true) {
					db_warn("show delete file dialog,ignore this message");
					break;
				}
				db_msg("[debug_zhb]--->SDV_KEY_RIGHT");
				if ((player_status_ == PAUSED || player_status_ == PLAYING || player_status_ == COMPLETION)) {
					listener_->sendmsg(this, PLAYBACK_SET_SHOW_THS, select_file_);
					listener_->notify(this, MSG_PLAYBACK_SHOW_DELETE_DIALOG, getCurrentFileType());
					listener_->sendmsg(this, MSG_PLAYBACK_SHOW_GRAY_ICON,0);
					SetPlaybackStatusIconInvalidFlag(true);
					db_warn("show delete file dialog,set invalid flag true ");
				}
			}break;
		case SDV_KEY_POWER: //播放界面快退或上一个视频文件
			db_warn("=========SDV_KEY_POWER=========");
		     if(m_nofile_nosdcard_){
				db_warn("[debug_zhb]---> no file or no sdcard status");
				break;
			}
			if(invalid_flag_ == true) {
				db_warn("show delete file dialog,ignore this message");
				break;
			}
#if 1
			if (isLongPress == SHORT_PRESS) {
				if(IsPlayingWindow() && (!select_flag_)){
					PreVideoPlay();
					db_warn("select file %d,page_num_ %d",select_file_,page_num_);
					if(select_file_  < page_num_ * 6 && ((select_file_ + 1) == (6 * (page_num_ - 1)))){
						page_num_--;
						db_warn("page_num_ %d",page_num_);
					}
				}
			} else {
				if ((player_status_ == PAUSED || player_status_ == PLAYING)){
				std::thread([=] {
					if(PAUSED == player_status_){
						if(select_mode_ == PLAY_DELETE){
							listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
							select_mode_ = PLAY_STOP;
							UpdateVideoPlayImg();
						}
						HideVideoPlayImg();
						keyPlayPauseButtonProc();
					}
					while(!isKeyUp) {
						usleep(100*1000);
						listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 0);
					}
					stop_timer(play_timer_id_);
					set_period_timer(1, 0, play_timer_id_);
				}).detach();
				}
			}
#endif
			break;
		case SDV_KEY_OK: //相册界面翻页响应,向下翻页  播放界面播放键
		{
			db_warn("==========SDV_KEY_OK==========");
			if(m_nofile_nosdcard_){
				db_warn("[debug_zhb]---> no file or no sdcard status");
				break;
			}
			if(invalid_flag_ == true) {
				db_warn("show delete file dialog,ignore this message");
				break;
			}
				if (player_status_ == STOPED) {
					if(m_nofile_nosdcard_){
						db_warn("[debug_zhb]---> no file or no sdcard status");
						break;
					}
					db_warn("select_file_%d file_sum_%d page_num_ %d page_sum_ %d",
							select_file_,file_sum_,page_num_,page_sum_);
					if(page_sum_  > page_num_){
						select_file_ +=6;
						UpdateListView(true);
						page_num_ ++;
						int tmp = (page_num_)*6;
						if(tmp >= file_sum_){
							listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,1);
							listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,0);
						}else {
							listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_UP_ICON,1);
							listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,1);
						}
					} else {
						listener_->sendmsg(this,MSG_PLAYBACK_SHOW_PAGE_DOWN_ICON,0);
					}
				 } else if(player_status_ == PLAYING || player_status_ == PAUSED ){
					 if(m_nofile_nosdcard_ || m_delete_flag ||StorageManager::GetInstance()->getUmountFlag()){
						db_warn(" no file or no sdcard status / Refreshing /sdcard umount,shuold stop to do anything");
						break;
					}
					if(player_status_ == PLAYING)
					{
						gettimeofday(&tm2, NULL);
#if 0
						 if( (tm2.tv_sec - tm1.tv_sec)*1000000 + (tm2.tv_usec - tm1.tv_usec) < 1000*1000)
						 {
							break;
						 };
#endif
					}
					if(select_mode_ == PLAY_STOP){
						keyPlayPauseButtonProc();
					}
				 } else if(player_status_ == COMPLETION){
					 keyPlayPauseButtonProc();
				 }
		}
		break;
		case SDV_KEY_MODE://button1
		{
#if 0
			 if(PowerManager::GetInstance()->IsScreenOn())
				 if(player_status_ == PLAYING)
			 		keyPlayPauseButtonProc();
#endif
		}
		break;
		case SDV_KEY_MENU: //相册/播放界面返回键
			db_warn("SDV_KEY_RETURN");
			if(invalid_flag_ == true) {
				db_warn("show delete file dialog,ignore this message");
				break;
			}
			/* if(getRefreshFlag()){
				 	db_warn("wait last operate finish, ignore this message");
				 	break;
			     }	*/
			db_msg("[debug_zhb]--->SDV_KEY_POWER--select_flag_ = %d   player_status_ = %d",select_flag_,player_status_);
            if((player_status_ == PLAYING || player_status_ == PAUSED || player_status_ == COMPLETION) && select_flag_ == false){
				HideStatusbar();
				 Screensaver::GetInstance()->pause(false);//start the screensaver timer
				listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
				listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);
				ResetFlagStatus();
				#ifdef S_PLAYB_STATUSBAR
					playBackSatutarbar(true);
				#endif
                break;
            }else if((player_status_ == PLAYING || player_status_ == PAUSED || player_status_ == COMPLETION) && select_flag_ == true){
				if(select_mode_ == PLAY_DELETE){
					listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
					select_mode_ = PLAY_STOP;
					UpdateVideoPlayImg();
				}
				HideVideoPlayImg();
				break;
            }
			#ifdef S_PLAYB_STATUSBAR
				playBackSatutarbar(false);
			#endif
			select_file_ = 0;
				page_num_ = 1;
			ResetFlagStatus();
			listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW, 0);
			listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
						//ShowStatusbar();
		break;
		default:
			db_msg("[debug_joson]:invild keycode");
			break;
	}
out:
	
    pthread_mutex_unlock(&playback_proc_lock_);
	
}
#endif
#endif
void PlaybackWindow::SetPreviewButtonStatus()
{
	listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW, 0);
}
/*
enum PlayerStatus {
        STOPED = 0,		// 0
        PLAYING,		// 1
        PAUSED,			// 2
        COMPLETION,		// 3
};
*/
int PlaybackWindow::NextVideoPlay()
{
	int ret = 0;
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))	
		return -1;
	db_error("player_status_: %d",player_status_);
    if(player_status_ == PLAYING ||player_status_ == COMPLETION ||player_status_ == PAUSED)
	{
	  	if(player_status_ == PLAYING ||player_status_ == PAUSED){
			listener_->sendmsg(this, PLAYBACK_PLAYING_STOP, 0);
	  	}
		listener_->sendmsg(this, PLAYBACK_SHOW_NEXT_THS, 0);
	}
	if ((player_status_ == STOPED || player_status_ == COMPLETION ) && !last_first_video_flag_) {
	  	if(select_file_ < file_sum_-1)
			select_file_++;
		ret = listener_->sendmsg(this, PLAYBACK_PLAY_PAUSE, 1);
		if (ret != 0) {
			// TODO: info to user
			return -1;
		}
		
	}
	return 0;
}

int PlaybackWindow::PreVideoPlay()
{
	int ret = 0;
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR) || (status == FORMATTING))	
		return -1;
	db_error("player_status_: %d",player_status_);
	if(player_status_ == PLAYING ||player_status_ == COMPLETION ||player_status_ == PAUSED)
	{
		if(player_status_ == PLAYING ||player_status_ == PAUSED)
		{
			listener_->sendmsg(this, PLAYBACK_PLAYING_STOP, 1);
		}
	  	listener_->sendmsg(this, PLAYBACK_SHOW_PRVE_THS, 0);
	}
	if ((player_status_ == STOPED || player_status_ == COMPLETION ) && !last_first_video_flag_) 
	{
		if(select_file_ > 0)
			select_file_--;
		ret = listener_->sendmsg(this, PLAYBACK_PLAY_PAUSE, 1);
		if (ret != 0) {
			// TODO: info to user
			return -1;
		}
	}
	return 0;
}

void PlaybackWindow::DoClearOldOperation()
{
	set_one_shot_timer(1,0,p_clearOldOperation_timer_id);
}

void PlaybackWindow::ClearOldOperationTimerProc(union sigval sigval)
{
    PlaybackWindow *pw = reinterpret_cast<PlaybackWindow *>(sigval.sival_ptr);
    pw->ClearOldOperation();
}

void PlaybackWindow::ClearOldOperation()
{
	if(IsPlayingWindow()){
			if(select_flag_){
					if(play_BulletCollection_->getButtonDialogShowFlag()){
						play_BulletCollection_->BCDoHide();
						}
					if(select_mode_ == PLAY_DELETE){
						listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY, 1);
						select_mode_ = PLAY_STOP;
//						UpdateVideoPlayImg();
					}
					HideVideoPlayImg();
					usleep(200*1000);
				}
			//
			HideStatusbar();
			Screensaver::GetInstance()->pause(false);//start the screensaver timer
			listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
			listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);
			ResetFlagStatus();
			#ifdef S_PLAYB_STATUSBAR
			playBackSatutarbar(true);
			#endif
			usleep(200*1000);
		}
	//
	if(play_BulletCollection_->getButtonDialogShowFlag()){
		play_BulletCollection_->BCDoHide();
	}
#ifdef S_PLAYB_STATUSBAR
	playBackSatutarbar(false);
#endif
	select_file_ = 0;
	ResetFlagStatus();
	listener_->sendmsg(this, MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW, 0);
	listener_->sendmsg(this, WM_WINDOW_CHANGE, WINDOWID_PREVIEW);
	
}


int PlaybackWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch (message) 
    {
        case MSG_PAINT: {	
        	break;
        }
            return HELP_ME_OUT;
        case MSG_ERASEBKGND:
            {
#ifndef S_FAKE_BG
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
                SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0xFF, 0xFF, 0x00));
                FillBox (hdc, rcTemp.left, rcTemp.top, RECTW(rcTemp), RECTH(rcTemp));

                if (fGetDC)
                    ReleaseDC (hdc);
#endif
            }
            break;
            
         case MSG_MOUSE_FLING:
        { 
            if(!IsPlayingWindow())
            {
                int direction = LOSWORD (wparam);
                //todo Ablum up down page
                if (direction == MOUSE_LEFT || direction == MOUSE_RIGHT)
                {
                    db_warn("playback window right or left flig !!!");
                     m_flig_rl = true;
                     if(direction == MOUSE_LEFT)
                        this->keyProc(SDV_KEY_OK, LONG_PRESS);
                     else
                        this->keyProc(SDV_KEY_RIGHT, LONG_PRESS);
                }
                else {
                    //db_warn("please right or left flig  to update Ablum page !!!");
                }

            }
        }break;
        case MSG_TIMER:
            break;
        case MSG_MOUSEMOVE://habo
            {
                if(IsPlayingWindow())
                {
                    static int ignore = 0;
                    int direction = LOSWORD(wparam) & ~(0x400);
                    if (player_status_ != PLAYING) {
                        break;
                    }

                    // NOTE: msg are too frequent , flitering out some msg
                    if (!ignore--) {
                        ignore = 3;
                    } else {
                        break;
                    }

                    if (direction == MOUSE_LEFT) 
                    {
                        listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 0);
                    }
                    else if(direction == MOUSE_RIGHT)
                    {
                        listener_->sendmsg(this, PLAYBACK_PLAY_SEEK, 1);
                    }
                }
				#ifdef USEICONTHUMB
				#ifdef UPDATEIMGUSETHREAD
				UpdateIconviewImg_thread_flag = 1;
				#endif
				#endif
            }
            break;
		#ifdef USEICONTHUMB
		case MSG_LBUTTONUP:
			#ifdef UPDATEIMGUSETHREAD
			UpdateIconviewImg_thread_flag = 1;
			#endif
			break;
		case MSG_MOUSE_LONGPRESS:
			db_error("[Habo]--xx-> MSG_MOUSE_LONGPRESS !!! ");
			if (FileInfo_.size()) {
				if (listview_flag_) {
					if (!select_status) {
						db_warn("change icon view to multiselect status");	
						select_status = 1;
						ivplayback_thumb_->SetIconItem_selectall(1);
						SetFileInfoSelect(-1,1);
					} else {
						select_status = 0;
						ivplayback_thumb_->SetIconItem_selectall(0);
						SetFileInfoSelect(-1,0);
					}
				}
				ivplayback_thumb_->Refresh();
			}
			return 0;
		
		case MSG_COMMAND:
            {
                int id = LOSWORD(wparam);
                int code = HISWORD(wparam);
				//db_error("[Habo]--xx-> MSG_COMMAND !!! id:%d code:%d",id,code);
                if (code == IVN_CLICKED) 
				{
					int index = ivplayback_thumb_->GetIconHighlight();
					db_warn("icon view click %d",index);
                    thumbIconItemClickProc(ivplayback_thumb_,index);        
                } 
				else if (code == IVN_SELCHANGED) 
				{
					//UpdateIconviewImg_thread_flag = 1;
                }
				else if (code == 0x0801) //  mouse move and leftbutton up
				{
					UpdateIconviewImg_thread_flag = 1;
                }
				
            }
            break;
		#endif
        default:
            break;
    }
    return SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
}

int PlaybackWindow::GetPlaybackPageStatus()
{
	db_warn("page_sum_ %d,page_num_ %d",page_sum_,page_num_);
	if(page_sum_ == 1){
		return 0;
	}
	else if(page_sum_ > page_num_ && page_num_ == 1){
		return 1;
	}
	else if(page_sum_ > page_num_ && page_num_ != 1){
		return 2;
	}
	else if(page_sum_ == page_num_ && page_sum_ != 1){
		return 3;
	}
	else {
		return 0;
	}

}

PlaybackWindow::PlaybackWindow(IComponent *parent)
        : SystemWindow(parent)
        , player_status_(STOPED)
        , select_file_(0)
		, file_sum_(0)
		, page_num_(1)
	    , page_sum_(0)
        , file_index_(0)
		, invalid_flag_(false)
        , listview_flag_(true)
        ,select_flag_(false)
        ,play_flag_(false)
        ,last_first_video_flag_(false)
        ,m_nofile_nosdcard_(false)
        ,m_refresh(false)
        ,ignore_message_flag_(false)
{
    wname = "PlaybackWindow";
    db_error("PlaybackWindow create ");
    Load();
	m_delete_flag = false;
    m_flig_rl = false;
	#ifdef USEICONTHUMB
	select_status = 0;
	m_UpdateIconviewImg_thread_id = 0;
	CancelUpdateListview = false;
	#endif
#ifdef S_FAKE_BG
    SetBackColor(0x00ffffff);
#else
    SetWindowBackImage(R::get()->GetImagePath("playback_bg").c_str());
#endif
    OnClick.bind(this, &PlaybackWindow::PlaybackWindowProc);
    #ifdef S_PLAYB_STATUSBAR
	#ifdef USEICONTHUMB
	GraphicView::LoadImage(GetControl("p_return_icon"), "return_y", "return_y_h", GraphicView::NORMAL);
	#else
	GraphicView::LoadImage(GetControl("p_return_icon"), "return_y");
	#endif
    GraphicView *view;
    view = reinterpret_cast<GraphicView *>(GetControl("p_return_icon"));
    view->OnClick.bind(this, &PlaybackWindow::ButtonClickProc);
	GetControl("p_return_icon")->Hide();

	m_playback_thumb_bk = reinterpret_cast<GraphicView *>(GetControl("p_playback_thumb_bk"));
	GraphicView::LoadImage(m_playback_thumb_bk, "playback_bg");
	m_playback_thumb_bk->Hide();
	//
	string _str;
	R::get()->GetString("tl_playback", _str);
	TextView* playback_label = reinterpret_cast<TextView *>(GetControl("p_playback_label"));
	playback_label->SetCaption(_str.c_str());
	playback_label->SetCaptionColor(0xFFFFFFFF);//
	playback_label->SetTextStyle(DT_CENTER);
	//playback_label->SetBackColor(0x00000000);
	playback_label->Hide();
       //
	TextView* fileinfo_total = reinterpret_cast<TextView *>(GetControl("p_fileinfo_total"));
	fileinfo_total->SetCaptionColor(0xFFFFFFFF);//0x4cFFFFFF
	fileinfo_total->SetBackColor(0xff313747);
	fileinfo_total->Hide();
    #endif
	#ifdef S_FAKE_BG
	GraphicView::LoadImage(GetControl("show_fake_bg"), "playback_bg");
    	fake_bg_icon = reinterpret_cast<GraphicView *>(GetControl("show_fake_bg"));
	fake_bg_icon->Hide();
	#endif
    // alpha is 0, that means set background transparent
	DWORD ctrl_bg_color = 0x00000000;
//	GraphicView::LoadImage(GetControl("show_play_img"), "pb_playing_y");
//	GraphicView::LoadImage(GetControl("show_delete_img"), "pb_delete_n");
	GraphicView::LoadImage(GetControl("show_no_file_sdcard_icon"), "no_sdcard");
	show_thumb_jpg_ = reinterpret_cast<GraphicView *>(GetControl("show_thumb_jpg"));
	show_thumb_jpg_->Hide();
	play_back_icon_ = reinterpret_cast<GraphicView *>(GetControl("show_play_img"));
	play_back_icon_->Hide();
	delete_icon_ = reinterpret_cast<GraphicView *>(GetControl("show_delete_img"));
	delete_icon_->Hide();
    progress_bar_ = reinterpret_cast<ProgressBar *>(GetControl("progress_bar"));
    progress_bar_->SetTag(PLAYBACK_PLAY_SEEK);
    progress_bar_->OnProgressSeek.bind(this, &PlaybackWindow::PlayProgressSeekProc);
    progress_bar_->SetBackColor(0x30000000);
    progress_bar_->Hide();
	
    PGBTime_t pgb_time;
    pgb_time.min = 59;
    pgb_time.sec = 59;

    int min = 0;
    int max = pgb_time.sec * 60 + pgb_time.min;
    progress_bar_->SetProgressRange(min, max);
    progress_bar_->SetProgressStep(1);
    ::create_timer(this, &play_timer_id_, &PlaybackWindow::PlayProgressUpdate);
	
    play_BulletCollection_ = new BulletCollection();//
    play_BulletCollection_->initButtonDialog(this);

    ::create_timer(this, &ctrlbar_timer_id_, &PlaybackWindow::CtrlbarTimerProc);
    pthread_mutex_init(&playback_proc_lock_, NULL);
    pthread_mutex_init(&m_lock_file_ctl, NULL);
	#ifdef USEICONTHUMB
	#ifdef UPDATEIMGUSETHREAD
	pthread_mutex_init(&loadfile_ctl, NULL);
	#endif
	#endif
	//playback_preview
	char image_str[64] = {0};
	#ifndef USEICONTHUMB
	for(int i = 0; i < IMAGEVIEW_NUM; i++)
	{

		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "imageview_%d", i);
		show_thumb_view_[i] = reinterpret_cast<GraphicView *>(GetControl(image_str));
        GraphicView *view;
        view = reinterpret_cast<GraphicView *>(GetControl(image_str));
        view->SetTag(i);
        view->OnClick.bind(this, &PlaybackWindow::ImageViewClickProc);
		//show_thumb_view_[i]->SetPlayBackImage(R::get()->GetImagePath("no_video_image").c_str());
		show_thumb_view_[i]->Hide();
		
		CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);
		
		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "label_%d", i);
		show_thumb_time_[i] = static_cast<TextView*>(GetControl(image_str));
		show_thumb_time_[i]->SetTextStyle(DT_RIGHT|DT_VCENTER);
		show_thumb_time_[i] ->SetCaption(" ");
		show_thumb_time_[i] ->SetCaptionColor(0xFFFFFFFF);
		show_thumb_time_[i] ->Hide();

		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "imageview_h_top_%d", i);
		s_rect_[i].select_rect_icon_top= reinterpret_cast<GraphicView *>(GetControl(image_str));
		s_rect_[i].select_rect_icon_top->SetPlayBackImage(R::get()->GetImagePath("file_select_h").c_str());
		s_rect_[i].select_rect_icon_top->Hide();

		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "imageview_h_bottom_%d", i);
		s_rect_[i].select_rect_icon_bottom= reinterpret_cast<GraphicView *>(GetControl(image_str));
		s_rect_[i].select_rect_icon_bottom->SetPlayBackImage(R::get()->GetImagePath("file_select_h").c_str());
		s_rect_[i].select_rect_icon_bottom->Hide();

		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "imageview_v_left_%d", i);
		s_rect_[i].select_rect_icon_right= reinterpret_cast<GraphicView *>(GetControl(image_str));
		s_rect_[i].select_rect_icon_right->SetPlayBackImage(R::get()->GetImagePath("file_select_v").c_str());
		s_rect_[i].select_rect_icon_right->Hide();

		memset(image_str, 0, sizeof(image_str));
		snprintf(image_str, sizeof(image_str), "imageview_v_right_%d", i);
		s_rect_[i].select_rect_icon_left = reinterpret_cast<GraphicView *>(GetControl(image_str));
		s_rect_[i].select_rect_icon_left->SetPlayBackImage(R::get()->GetImagePath("file_select_v").c_str());
		s_rect_[i].select_rect_icon_left->Hide();
	}	
	#endif

    playback_time_label = reinterpret_cast<TextView *>(GetControl("playback_time_label"));
    //playback_time_label->SetTimeCaption("");
    playback_time_label->SetCaptionColor(0xFFFFFFFF);
    playback_time_label->SetBackColor(0x00FFFFFF);
    playback_time_label->Hide();
	#ifdef USEICONTHUMB
	ivplayback_thumb_ = reinterpret_cast<IconView *>(GetControl("p_playback_thumb"));
	
	//ivplayback_thumb_->onIconClickEvent.bind(this, &PlaybackWindow::thumbIconItemClickProc);
    ivplayback_thumb_->SetBackColor(0x00000000);
    ivplayback_thumb_->SetIconMargins();
    ivplayback_thumb_->SetCaptionColor(0xFFFFFFFF);

    ivplayback_thumb_->RemoveAllIconItem();
    int item_width = 640 / 3;	// 213
    int item_height =240 / 2;
    ivplayback_thumb_->SetIconSize(item_width, item_height);
	//ivplayback_thumb_->Hide();
	//iconlist_.clear();
	string icon_path;
	//BITMAP icon_filetype[8];
	icon_path = "/usr/share/minigui/res/images/file_photo.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[0], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_photo.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[1], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_video_A.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[2], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_video_B.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[3], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_video_SOS_A.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[4], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_video_SOS_B.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[5], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_urgent.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[6], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/file_packing.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[7], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/select_off.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[8], icon_path.c_str());
	icon_path = "/usr/share/minigui/res/images/select_on.png";
	::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype_[9], icon_path.c_str());
	#endif
    create_timer(this, &p_clearOldOperation_timer_id,ClearOldOperationTimerProc);
    stop_timer(p_clearOldOperation_timer_id);
}


PlaybackWindow::~PlaybackWindow()
{
    db_error("~PlaybackWindow destruct");
    // delete playlist_win_;
    // playlist_win_ = NULL;

    ::delete_timer(ctrlbar_timer_id_);
    ::delete_timer(play_timer_id_);
	delete_timer(p_clearOldOperation_timer_id);
    pthread_mutex_destroy(&playback_proc_lock_);
    pthread_mutex_destroy(&m_lock_file_ctl);
	#ifdef USEICONTHUMB
	#ifdef UPDATEIMGUSETHREAD
	pthread_mutex_destroy(&loadfile_ctl);
	if( m_UpdateIconviewImg_thread_id > 0 )
		pthread_cancel(m_UpdateIconviewImg_thread_id);
	pthread_mutex_destroy(&loadfile_ctl);
	#endif
	#endif
}
#ifdef S_FAKE_BG
void PlaybackWindow::fakeBg(bool m_show)
{
	if(m_show)
		fake_bg_icon->Show();
	else
		fake_bg_icon->Hide();
}
#endif
#ifdef S_PLAYB_STATUSBAR
void PlaybackWindow::playBackSatutarbar(bool mshow)
{
	db_msg("debug_zhb-------playBackSatutarbar----> mshow = %d",mshow);
	TextView* playback_label = reinterpret_cast<TextView *>(GetControl("p_playback_label"));
	TextView* fileinfo_total = reinterpret_cast<TextView *>(GetControl("p_fileinfo_total"));
	if(mshow){
		GetControl("p_return_icon")->Show();
		string _str;
		R::get()->GetString("tl_playback", _str);
		playback_label->SetCaption(_str.c_str());
		playback_label->Show();
		fileinfo_total->Show();
	}else{
		GetControl("p_return_icon")->Hide();
		playback_label->Hide();
		fileinfo_total->Hide();
		
	}
}
void PlaybackWindow::UpdatePlaybackFileInfo(int index,int count,bool flag)
{
    TextView* fileinfo_total = reinterpret_cast<TextView *>(GetControl("p_fileinfo_total"));
    int idx;
    if (count != 0) {
        idx =index + 1;
    } else {
        idx = 0;
    }
	char buf[32]={0};
  	snprintf(buf,sizeof(buf),"%d/%d",idx,count);
	 if(idx == 0 && count == 0)
       		fileinfo_total->SetCaption("");
	 else
	 	fileinfo_total->SetCaption(buf);
	if(flag == true)
	fileinfo_total->Show();
	else
		fileinfo_total->Hide();
}
#endif


void PlaybackWindow::CtrlNoFileOrSdcard(bool mshow, bool mfile)
{
	if(mshow){
		string str_;
		TextView* show_no_file_sdcard_label_ = reinterpret_cast<TextView *>(GetControl("show_no_file_sdcard_label"));
		show_no_file_sdcard_label_->SetCaptionColor(0xFFFFFFFF);
		if(mfile){
			R::get()->GetString("ml_playback_no_file", str_);
			GraphicView::LoadImage(GetControl("show_no_file_sdcard_icon"), "no_file");
			}else{
				R::get()->GetString("ml_playback_no_sdcard", str_);
				GraphicView::LoadImage(GetControl("show_no_file_sdcard_icon"), "no_sdcard");
				}
			show_no_file_sdcard_label_->SetCaption(str_.c_str());
			show_no_file_sdcard_label_->Show();
			GetControl("show_no_file_sdcard_icon")->Show();
			m_nofile_nosdcard_ = true;
	}else{
		GetControl("show_no_file_sdcard_icon")->Hide();
		TextView* show_no_file_sdcard_label_ = reinterpret_cast<TextView *>(GetControl("show_no_file_sdcard_label"));
		show_no_file_sdcard_label_->Hide();
		m_nofile_nosdcard_ = false;
	}
}

void PlaybackWindow::UpdatePlaybackTime(int timer_start, int timer_end)
{
	char buf_timer[20] = {0};
	snprintf(buf_timer, sizeof(buf_timer),"%02d:%02d/%02d:%02d", timer_start/60, timer_start%60, timer_end/60, timer_end%60);
	if(!playback_time_label->GetVisible()){
		db_msg("debug_zhb---> ready to show the playback time label");
		playback_time_label->Show();
		}
	playback_time_label->SetTimeCaption(buf_timer,0x00000000);
}
void PlaybackWindow::HidePlaybackTime(bool flag)
{
	if(flag == true)
		playback_time_label->Hide();
	/*else
		playback_time_label->Show();*/
}

void PlaybackWindow::videoTypeIcon(int videoType_id, int videoType,bool videoType_s)
{
	char image_str[64] = {0};
	memset(image_str, 0, sizeof(image_str));
	snprintf(image_str, sizeof(image_str),"videoType_%d", videoType_id);
	if(videoType_s){
		if(videoType)
			GraphicView::LoadImage(GetControl(image_str), "file_urgent");//file is urgent record video
		else {
			GraphicView::LoadImage(GetControl(image_str), "file_packing");//file is packing record video 
		}
		GetControl(image_str)->Show();
	}else{
		GetControl(image_str)->Hide();

	}
}
void PlaybackWindow::fileTypeIcon(int fileType_id, int fileType,bool fileType_s)
{
	char image_str[64] = {0};
	memset(image_str, 0, sizeof(image_str));
	snprintf(image_str, sizeof(image_str),"fileType_%d", fileType_id);
	if(fileType_s && (fileType == VIDEO_A || fileType == VIDEO_B || fileType == VIDEO_A_SOS || 
        fileType == VIDEO_B_SOS || fileType == VIDEO_A_P || fileType == VIDEO_B_P ||
        fileType == PHOTO_A || fileType == PHOTO_B)){
		if(fileType == VIDEO_A)
			GraphicView::LoadImage(GetControl(image_str), "playback_video_A");//file is video
		else if(fileType == VIDEO_B)
			GraphicView::LoadImage(GetControl(image_str), "playback_video_B");//file is video
		else if(fileType == VIDEO_A_SOS) {
			GraphicView::LoadImage(GetControl(image_str), "playback_video_locked_A");//file is video sos
		}else if(fileType == VIDEO_B_SOS) {
			GraphicView::LoadImage(GetControl(image_str), "playback_video_locked_B");//file is video sos
		}else if(fileType == VIDEO_A_P) {
			GraphicView::LoadImage(GetControl(image_str), "playback_video_locked_A");//file is video sos
		}else if(fileType == VIDEO_B_P) {
			GraphicView::LoadImage(GetControl(image_str), "playback_video_locked_B");//file is video sos
		}else if(fileType == PHOTO_A){
            GraphicView::LoadImage(GetControl(image_str), "file_photo");
		}else if(fileType == PHOTO_B){
            GraphicView::LoadImage(GetControl(image_str), "file_photo");
		}
		GetControl(image_str)->Show();
	}else{
		GetControl(image_str)->Hide();

	}
}

void PlaybackWindow::CtrlFileTypeIcon(int fileType,int id_)
{
#ifndef USEICONTHUMB
	switch(fileType){
		case PHOTO_A:
		case PHOTO_B:
			{
				fileTypeIcon(id_, 0,true);//show photo icon
				videoTypeIcon(id_, 0,false);//hide video type icon
			}break;
		case VIDEO_A:
		case VIDEO_B:
			{
				fileTypeIcon(id_, fileType,true);//show the video icon
				videoTypeIcon(id_, 0,false);//hide the video type icon because is normal video
			}break;
		case VIDEO_A_SOS:
		case VIDEO_B_SOS:
			{
				fileTypeIcon(id_, fileType,true);//show the video icon
				videoTypeIcon(id_, 1,true);//show the urgent video icon
				//videoTypeIcon(id_, 0,false);//hide the urgent video icon
			}break;
		case VIDEO_A_P:
		case VIDEO_B_P:
			{
				fileTypeIcon(id_, fileType,true);//show the video icon
				videoTypeIcon(id_, 0,true);//show the packing video icon
			}break;
		default:
			{
				fileTypeIcon(id_, 1,false);//hide the icon
				videoTypeIcon(id_, 0,false);//hide the icon
			}break;
		}
#endif
}

string PlaybackWindow::GetVideoCreatTime(const std::string &filename)
{
    time_t timep = MediaFileManager::GetInstance()->GetFileTimestampByName(filename);
    if (timep < 0) return "";

    struct tm *tm = localtime(&timep);

    char cDate[128];
    memset(cDate, 0, sizeof(cDate));
    //snprintf(cDate, sizeof(cDate)-1,"%04d.%02d.%02d",(1900 + tm->tm_year), (1 + tm->tm_mon), tm->tm_mday);
    snprintf(cDate, sizeof(cDate)-1,"%02d-%02d",(1 + tm->tm_mon),tm->tm_mday);
    //char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char cTime[128];
    memset(cTime, 0, sizeof(cTime));
   // snprintf(cTime, sizeof(cTime)-1, "%s %s %d:%02d:%02d", cDate, wday[tm->tm_wday], tm->tm_hour, tm->tm_min, tm->tm_sec);
    //snprintf(cTime, sizeof(cTime)-1, "%s     %02d:%02d", cDate, tm->tm_hour, tm->tm_min);
    snprintf(cTime, sizeof(cTime)-1, "%s  %02d:%02d", cDate, tm->tm_hour, tm->tm_min);
    db_msg("ShowVideoCreatTime : %s", cTime);
    return cTime;
}
int  PlaybackWindow::GetVideoCreatTime(const std::string &filename,std::string &fileTime)
{
    time_t timep = MediaFileManager::GetInstance()->GetFileTimestampByName(filename);
    if (timep < 0)
	return -1;
   struct tm *tm = localtime(&timep);
   char temp[128]={0};
   snprintf(temp, sizeof(temp)-1,"%04d - %02d - %02d  %02d:%02d:%02d",(1900 + tm->tm_year), (1 + tm->tm_mon), tm->tm_mday,tm->tm_hour, tm->tm_min,tm->tm_sec); 
   fileTime = temp;
    return 0;
}

#ifndef USEICONTHUMB
int PlaybackWindow::getFileInfo(bool flag)
#else
int PlaybackWindow::getFileInfo(int flag)
#endif
{
	int index;
	int i = 0;
	string file_bak;
	std::string type;
	vector<string>::const_iterator c_it;
	int count = 0;
	if(flag){
		#ifndef USEICONTHUMB
	    for(int i = 0; i < IMAGEVIEW_NUM; i++){
	        mediaf_image_[i].filename = "";
	        mediaf_image_[i].filepath = "";
		 mediaf_image_[i].fileCreatTime= "";
	        mediaf_image_[i].isHaveThumJPG = 0;
		 mediaf_image_[i].fileType = FileType_UNKNOWN_TYPE;
	    }		
		count = playlist_.size();
		if(playlist_.size() <= 0){
	        return -1;
	    }
		index = select_file_/IMAGEVIEW_NUM;

		for(i = 0; i < IMAGEVIEW_NUM; i++)
        {
		    c_it = playlist_.begin()+index*IMAGEVIEW_NUM+i;
			if(c_it == playlist_.end())
				return i-1;
		    file_bak = c_it->c_str();

			string::size_type rc = file_bak.rfind(".");
			if( rc == string::npos)
			{
				db_warn("invalid fileName:%s",file_bak.c_str());
				continue;
			}
            mediaf_image_[i].filename=c_it->c_str();
		    	

            #ifdef FILE_CREATE_TIME
			mediaf_image_[i].fileCreatTime= GetVideoCreatTime(c_it->c_str());
            #else
            mediaf_image_[i].fileCreatTime= "";
            #endif
			type = MediaFileManager::GetInstance()->GetMediaFileType(c_it->c_str());
        	if(type == "video_A"){
				mediaf_image_[i].fileType = VIDEO_A;
                file_bak = file_bak.substr(0,rc);
                file_bak += "_ths.jpg";
        	}else if(type == "video_B"){
				mediaf_image_[i].fileType = VIDEO_B;
                file_bak = file_bak.substr(0,rc);
                file_bak += "_ths.jpg";
        	}else if(type == "videoA_SOS"){
				mediaf_image_[i].fileType = VIDEO_A_SOS;
                file_bak = file_bak.substr(0,rc-strlen("_SOS"));
                file_bak += "_ths_SOS.jpg";
        	}else if(type == "videoB_SOS"){
				mediaf_image_[i].fileType = VIDEO_B_SOS;
                file_bak = file_bak.substr(0,rc-strlen("_SOS"));
                file_bak += "_ths_SOS.jpg";
        	}else if(type == "videoA_PARK"){
				mediaf_image_[i].fileType = VIDEO_A_P;
                file_bak = file_bak.substr(0,rc-strlen("_PARK"));
                file_bak += "_PARK_ths.jpg";
        	}else if(type == "videoB_PARK"){
				mediaf_image_[i].fileType = VIDEO_B_P;
                file_bak = file_bak.substr(0,rc-strlen("_PARK"));
                file_bak += "_PARK_ths.jpg";
        	}else if(type == "photo_A"){
				mediaf_image_[i].fileType = PHOTO_A;	
                file_bak = file_bak.substr(0,rc);
//                file_bak += ".jpg";
                file_bak += "_ths.jpg";
        	}else if(type == "photo_B"){
				mediaf_image_[i].fileType = PHOTO_B;
                file_bak = file_bak.substr(0,rc);
//                file_bak += ".jpg";
                file_bak += "_ths.jpg";
			}
            // db_warn("habo----->filename  = %s   ths = %s",mediaf_image_[i].filename.c_str(),file_bak.c_str());
            if(access(file_bak.c_str(), F_OK) == 0){
		    	mediaf_image_[i].filepath = file_bak.c_str();
				mediaf_image_[i].isHaveThumJPG = 1;
			}
			

		}
		#endif
	}
#ifdef USEICONTHUMB	
	else {
		// get all fileinfo    playlist_-> FileInfo_ 一一对应
		#if 0
		FileInfo_.clear();
		#else
		if (FileInfo_.size()) {
			std::map<int, FileInfo>::iterator iter;
			for (iter = FileInfo_.begin(); iter != FileInfo_.end(); ) 
			{
				if ((iter->second).thumbbmpload) {
					(iter->second).thumbbmpload = 0;
					if ((iter->second).thumbbmp.bmBits) {
						UnloadBitmap(&(iter->second).thumbbmp);	// 释放缩略图空间
						(iter->second).thumbbmp.bmBits = NULL;
					}
					
				} 
				FileInfo_.erase(iter++);		// 迭代器指向下一个元素
			}
		}
		FileInfo_.clear();
		#endif
		count = playlist_.size();
		if(playlist_.size() <= 0){
	        return 0;
	    }
		
		index = 0;
		db_warn("index :%d count: %d",index, count);
		//for(i = index; i < count; i++)
		FileInfo mediaf_image_tmp;
		for(i = 0; i < count; i++)
        {
			c_it = playlist_.begin() + i;
			if(c_it == playlist_.end())
				return i-1;
		    file_bak = c_it->c_str();
			//db_warn("file: %s", file_bak.c_str());
			string::size_type rc = file_bak.rfind(".");
			if( rc == string::npos)
			{
				db_warn("invalid fileName:%s",file_bak.c_str());
				continue;
			}
            mediaf_image_tmp.filename=c_it->c_str();
		    	

            #ifdef FILE_CREATE_TIME
			mediaf_image_tmp.fileCreatTime= GetVideoCreatTime(c_it->c_str());
            #else
            mediaf_image_tmp.fileCreatTime= "";
            #endif
			type = MediaFileManager::GetInstance()->GetMediaFileType(c_it->c_str());
        	if(type == "video_A"){
				mediaf_image_tmp.fileType = VIDEO_A;
                file_bak = file_bak.substr(0,rc);
                file_bak += "_ths.jpg";
        	}else if(type == "video_B"){
				mediaf_image_tmp.fileType = VIDEO_B;
                file_bak = file_bak.substr(0,rc);
                file_bak += "_ths.jpg";
        	}else if(type == "videoA_SOS"){
				mediaf_image_tmp.fileType = VIDEO_A_SOS;
                file_bak = file_bak.substr(0,rc-strlen("_SOS"));
                file_bak += "_SOS_ths.jpg";
        	}else if(type == "videoB_SOS"){
				mediaf_image_tmp.fileType = VIDEO_B_SOS;
                file_bak = file_bak.substr(0,rc-strlen("_SOS"));
                file_bak += "_SOS_ths.jpg";
        	}else if(type == "videoA_PARK"){
				mediaf_image_tmp.fileType = VIDEO_A_P;
                file_bak = file_bak.substr(0,rc-strlen("_PARK"));
                file_bak += "_PARK_ths.jpg";
        	}else if(type == "videoB_PARK"){
				mediaf_image_tmp.fileType = VIDEO_B_P;
                file_bak = file_bak.substr(0,rc-strlen("_PARK"));
                file_bak += "_PARK_ths.jpg";
        	}else if(type == "photo_A"){
				mediaf_image_tmp.fileType = PHOTO_A;	
                file_bak = file_bak.substr(0,rc);
//                file_bak += ".jpg";
                file_bak += "_ths.jpg";
        	}else if(type == "photo_B"){
				mediaf_image_tmp.fileType = PHOTO_B;
                file_bak = file_bak.substr(0,rc);
//                file_bak += ".jpg";
                file_bak += "_ths.jpg";
			}
            // db_warn("habo----->filename  = %s   ths = %s",mediaf_image_tmp.filename.c_str(),file_bak.c_str());
            if(access(file_bak.c_str(), F_OK) == 0){
		    	mediaf_image_tmp.filepath = file_bak.c_str();
				mediaf_image_tmp.isHaveThumJPG = 1;
			}
			mediaf_image_tmp.fileselect = 0;
			mediaf_image_tmp.thumbbmpload = 0;
			FileInfo_.emplace(i, mediaf_image_tmp);	// {key, value}

		}
	}
#endif
	return i;
}

int PlaybackWindow::getCurrentFileIndex()
{
	return select_file_;
}

// 传进来的是PlaybackPresenter的	playlist_
void PlaybackWindow::UpdateFileList(const vector<string> file_list, int sum,bool reset_flag)
{
	static bool update_flag = false;
    // remove last list
	int i = 0;
	playlist_.clear();

	playlist_ = file_list;	// 更新 PlaybackWindow 的 playlist_
	file_sum_ = sum;
	page_sum_ = file_sum_ / 6;

	if(file_sum_ % 6 != 0)
		page_sum_ ++;
	if(select_file_ >= file_sum_){
		if(select_file_ > 0){
			select_file_ = file_sum_;
			select_file_ --;
		}
	}

	if(page_num_ > page_sum_ && page_num_ != 1){
		page_num_ --;
		db_warn("update page num");
	}

	db_warn("file_sum_ %d page_sum_ %d page_num_ %d",
				file_sum_,page_sum_ ,page_num_);

	if(reset_flag)
		ResetFlagStatus();

#if 0
    vector<string>::const_iterator c_it;
    for ( c_it = playlist_.begin(); c_it != playlist_.end(); ++c_it) {
		db_msg("by hero *** PlaybackWindow playlist[%d] = %s\n",i++, c_it->c_str());
    }	
#endif
}
void PlaybackWindow::ResetPlayList()
{
#ifndef USEICONTHUMB
	for(int i = 0; i < IMAGEVIEW_NUM; i++)
	{
		show_thumb_view_[i]->Hide();
		CtrlSelectFileRect(i,false);
		show_thumb_time_[i] ->Hide();
		CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);
		
	}
#endif
}
#ifndef USEICONTHUMB
void PlaybackWindow::UpdateListView(bool flag)
#else
void PlaybackWindow::UpdateListView(int flag)
#endif
{
#ifndef USEICONTHUMB
	string file_bak;
	int view_num;
	vector<string>::const_iterator c_it;
	view_num = getFileInfo(flag); //update image view
	listview_flag_ = true;	
	for(int i = 0; i < IMAGEVIEW_NUM; i++){
		if(flag){
			if(mediaf_image_[i].isHaveThumJPG == 1){
				show_thumb_view_[i]->SetPlayBackImage(mediaf_image_[i].filepath.c_str());
				if(!show_thumb_view_[i]->GetVisible())
					show_thumb_view_[i]->Show();
				show_thumb_time_[i] ->SetCaption(mediaf_image_[i].fileCreatTime.c_str());
				if(!show_thumb_time_[i]->GetVisible())
					show_thumb_time_[i]->Show();
				CtrlFileTypeIcon(mediaf_image_[i].fileType,i);
			}else if(mediaf_image_[i].isHaveThumJPG == 0 && mediaf_image_[i].fileType != FileType_UNKNOWN_TYPE){
				show_thumb_view_[i]->SetPlayBackImage(R::get()->GetImagePath("no_video_image").c_str());
				if(!show_thumb_view_[i]->GetVisible())
					show_thumb_view_[i]->Show();
				//show_thumb_view_[i]->Hide();
				show_thumb_time_[i]->Hide();
				CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);//hide the icon
			}else if(mediaf_image_[i].isHaveThumJPG == 0 && mediaf_image_[i].fileType == FileType_UNKNOWN_TYPE){
                db_warn("habo---> filename = %s ",mediaf_image_[i].filename.c_str());
                db_warn("habo---> filepath = %s ",mediaf_image_[i].filepath.c_str());
				show_thumb_view_[i]->Hide();
				show_thumb_time_[i]->Hide();
				CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);//hide the icon
			}
			
			if(i > view_num){//handel the last page  hide the thumb/time_label/icon
				CtrlSelectFileRect(i,false);
				show_thumb_time_[i]->Hide();
				CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);
				continue;
			}
		}
		if(i == file_index_%IMAGEVIEW_NUM){
			CtrlSelectFileRect(i,false);
		}	
		if(i == select_file_%IMAGEVIEW_NUM){
			CtrlSelectFileRect(i,true);
		}
	}
	listener_->sendmsg(this, PLAYBACK_SHOW_FILEOSD, select_file_);
	file_index_ = select_file_;
#endif
#ifdef USEICONTHUMB
	//db_warn("UpdateListView flag: %d",flag);
	listview_flag_ = true;
	setenv("MOSE_MOVE_ADJUST", "1", 1);		// 让底层只处理Y方向的变化,防止有时候拖不动的现象
	if(flag == UPDATEVIEW_LOAD){
		lockxx = 1;
		UpdateIconviewImg_thread_flag = 0;
		pthread_mutex_lock(&loadfile_ctl);
		#if 1
		string file_bak;
		int view_num;
		//vector<string>::const_iterator c_it;
		ivplayback_thumb_->RemoveAllIconItem();
		view_num = getFileInfo(0); //update image view	这里面会FileInfo_.clear();
		//thumb_icon_views_.clear();
		if (view_num > 0) { 
		//BITMAP icon_filetype,icon_fileselect,icon_filelock;
		
		//string icon_path;
		//icon_path= "/usr/share/minigui/res/images/file_video_A.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());	
		//icon_path = "/usr/share/minigui/res/images/file_urgent.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
		//icon_path = "/usr/share/minigui/res/images/select_on_h.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_fileselect, icon_path.c_str());

		std::vector<IVITEMINFO> iconview;
		//db_error("FileInfo_.size: %d",FileInfo_.size());
	    for (auto iter : FileInfo_) {
	        IVITEMINFO items;
			//BITMAP tmpbmp;
			//memset(&tmpbmp, 0, sizeof(BITMAP));
			memset(&items, 0, sizeof(IVITEMINFO));
			items.nItem = iter.first;
			//db_error("iter.first: %d  file: %s",iter.first, FileInfo_[iter.first].filename);
			// 缩略图		
			memset(&FileInfo_[iter.first].thumbbmp, 0, sizeof(BITMAP));
			#ifndef UPDATEIMGUSETHREAD
			string bmp_path = FileInfo_[iter.first].filepath;
			::LoadBitmapFromFile(HDC_SCREEN, &FileInfo_[iter.first].thumbbmp, bmp_path.c_str());
			FileInfo_[iter.first].thumbbmpload = 1;
			items.bmp   = &FileInfo_[iter.first].thumbbmp;
			#else
			FileInfo_[iter.first].thumbbmpload = 0;
			items.bmp   = NULL;
			#endif
			
			items.label = FileInfo_[iter.first].fileCreatTime.c_str();	
			items.filelock = 0;
			items.icon_filetype = NULL;
			//db_error("FileInfo_[iter.first].fileType: %d",FileInfo_[iter.first].fileType);
			switch (FileInfo_[iter.first].fileType) {
				case PHOTO_A: 
					items.icon_filetype = &icon_filetype_[0];
					break;
				case PHOTO_B: 
					items.icon_filetype = &icon_filetype_[1];
					break;
				case VIDEO_A: 
					items.icon_filetype = &icon_filetype_[2];
					break;
				case VIDEO_B: 
					items.icon_filetype = &icon_filetype_[3];
					break;
				case VIDEO_A_SOS: 
					items.icon_filetype = &icon_filetype_[4];
					items.filelock = 1;
					break;
				case VIDEO_B_SOS: 
					items.icon_filetype = &icon_filetype_[5];
					items.filelock = 1;
					break;
				case VIDEO_A_P: 
					items.icon_filetype = &icon_filetype_[2];
					items.filelock = 2;
					break;
				case VIDEO_B_P: 
					//memcpy(&icon_filetype, &icon_filetype_[3],sizeof(BITMAP));
					//memcpy(&icon_filelock, &icon_filetype_[7],sizeof(BITMAP));
					items.icon_filetype = &icon_filetype_[3];
					items.filelock = 2;
					break;
				
			}
			items.selected = 0;
			
			items.icon_filesel[0] = &icon_filetype_[8];
			items.icon_filesel[1] = &icon_filetype_[9];		// select_on
			items.icon_filelock[0] = &icon_filetype_[6];
			items.icon_filelock[1] = &icon_filetype_[7];
			iconview.push_back(items);
	    }
	    ivplayback_thumb_->AddIconViewItems(iconview);
		}
		ivplayback_thumb_->Show();
		#ifdef UPDATEIMGUSETHREAD
		lastupdate = -1;
		lockxx = 0;
		#endif
		#else
		string file_bak;
		int view_num;
		vector<string>::const_iterator c_it;
		view_num = getFileInfo(0); //update image view
		ivplayback_thumb_->RemoveAllIconItem();
		thumb_icon_views_.clear();

		BITMAP icon_filetype,icon_fileselect,icon_filelock;
		
		string icon_path;
		//icon_path= "/usr/share/minigui/res/images/file_video_A.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());	
		//icon_path = "/usr/share/minigui/res/images/file_urgent.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
		//icon_path = "/usr/share/minigui/res/images/select_on_h.png";
		//::LoadBitmapFromFile(HDC_SCREEN, &icon_fileselect, icon_path.c_str());

		
	    for (auto iter : FileInfo_) {
	        IVITEMINFO items;
			BITMAP bmp;
			memset(&bmp, 0, sizeof(BITMAP));
			memset(&icon_filetype, 0, sizeof(BITMAP));
			memset(&icon_fileselect, 0, sizeof(BITMAP));
			memset(&icon_filelock, 0, sizeof(BITMAP));
			//string bmp_path = "/usr/share/minigui/res/images/mass_storage.png";
			
			string bmp_path = FileInfo_[iter.first].filepath;
			::LoadBitmapFromFile(HDC_SCREEN, &bmp, bmp_path.c_str());
			ThumbIconViewBmp icon_view_bmp;
			icon_view_bmp.name = FileInfo_[iter.first].fileCreatTime;
			memcpy(&icon_view_bmp.bmp, &bmp, sizeof(BITMAP));

			//std::map<int, BITMAP>::iterator itr;
			switch (FileInfo_[iter.first].fileType) {
				case PHOTO_A: 
					memcpy(&icon_filetype, &icon_filetype_[0],sizeof(BITMAP));
					break;
				case PHOTO_B: 
					//icon_path = "/usr/share/minigui/res/images/file_photo.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[1],sizeof(BITMAP));
					break;
				case VIDEO_A: 
					//icon_path = "/usr/share/minigui/res/images/file_video_A.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[2],sizeof(BITMAP));
					break;
				case VIDEO_B: 
					//icon_path = "/usr/share/minigui/res/images/file_video_B.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[3],sizeof(BITMAP));
					break;
				case VIDEO_A_SOS: 
					//icon_path = "/usr/share/minigui/res/images/file_video_SOS_A.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					//icon_path = "/usr/share/minigui/res/images/file_urgent.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[4],sizeof(BITMAP));
					memcpy(&icon_filelock, &icon_filetype_[6],sizeof(BITMAP));
					break;
				case VIDEO_B_SOS: 
					//icon_path = "/usr/share/minigui/res/images/file_video_SOS_A.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					//icon_path = "/usr/share/minigui/res/images/file_urgent.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[5],sizeof(BITMAP));
					memcpy(&icon_filelock, &icon_filetype_[6],sizeof(BITMAP));
					break;
				case VIDEO_A_P: 
					//icon_path = "/usr/share/minigui/res/images/file_video_A.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					//icon_path = "/usr/share/minigui/res/images/file_packing.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[2],sizeof(BITMAP));
					memcpy(&icon_filelock, &icon_filetype_[7],sizeof(BITMAP));
					break;
				case VIDEO_B_P: 
					//icon_path = "/usr/share/minigui/res/images/file_video_B.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filetype, icon_path.c_str());
					//icon_path = "/usr/share/minigui/res/images/file_packing.png";
					//::LoadBitmapFromFile(HDC_SCREEN, &icon_filelock, icon_path.c_str());
					memcpy(&icon_filetype, &icon_filetype_[3],sizeof(BITMAP));
					memcpy(&icon_filelock, &icon_filetype_[7],sizeof(BITMAP));
					break;
				
			}
			memcpy(&icon_view_bmp.icon_filetype, &icon_filetype, sizeof(BITMAP));
			memcpy(&icon_view_bmp.icon_fileselect, &icon_fileselect, sizeof(BITMAP));
			memcpy(&icon_view_bmp.icon_filelock, &icon_filelock, sizeof(BITMAP));
			
			thumb_icon_views_.emplace(iter.first, icon_view_bmp);
	    }
		std::vector<IVITEMINFO> iconview;
	    for (auto iter : thumb_icon_views_) {
			IVITEMINFO items;
	        memset(&items, 0, sizeof(IVITEMINFO));
	        
	        items.nItem = iter.first;
			items.bmp     = &thumb_icon_views_[iter.first].bmp;
			items.icon[0] = &thumb_icon_views_[iter.first].icon_filetype;
			items.icon[1] = &thumb_icon_views_[iter.first].icon_fileselect;
			items.icon[2] = &thumb_icon_views_[iter.first].icon_filelock;
	        items.label   = const_cast<char*>(thumb_icon_views_[iter.first].name.c_str());

	        //items.addData = (DWORD)iconlabels[j];
	        iconview.push_back(items);
	    }
	    ivplayback_thumb_->AddIconViewItems(iconview);
		ivplayback_thumb_->Show();
		#endif
		db_error("UpdateListView end ...");
		pthread_mutex_unlock(&loadfile_ctl);
		UpdateIconviewImg_thread_flag = 1; 
	} else if(flag == UPDATEVIEW_NOLOAD){
		ivplayback_thumb_->Show();
		#ifdef UPDATEIMGUSETHREAD
		UpdateIconviewImg_thread_flag = 1; 
		#endif
	}
	if (select_file_ < 0) select_file_ = 0;
	if (select_file_ >= file_sum_) select_file_ = file_sum_ - 1;
	ivplayback_thumb_->SetIconHighlight(select_file_,true);
	UpdateSelectIndexAndTotalFiles(true);
	#endif
}

void PlaybackWindow::CtrlSelectFileRect(int val,bool m_show)
{
#ifndef USEICONTHUMB
	if(m_show){
		if(!s_rect_[val].select_rect_icon_top->GetVisible())
			s_rect_[val].select_rect_icon_top->Show();
		if(!s_rect_[val].select_rect_icon_bottom->GetVisible())
			s_rect_[val].select_rect_icon_bottom->Show();
		if(!s_rect_[val].select_rect_icon_left->GetVisible())
			s_rect_[val].select_rect_icon_left->Show();
		if(!s_rect_[val].select_rect_icon_right->GetVisible())
			s_rect_[val].select_rect_icon_right->Show();
	}else{
		if(s_rect_[val].select_rect_icon_top->GetVisible())
			s_rect_[val].select_rect_icon_top->Hide();
		if(s_rect_[val].select_rect_icon_bottom->GetVisible())
			s_rect_[val].select_rect_icon_bottom->Hide();
		if(s_rect_[val].select_rect_icon_left->GetVisible())
			s_rect_[val].select_rect_icon_left->Hide();
		if(s_rect_[val].select_rect_icon_right->GetVisible())
			s_rect_[val].select_rect_icon_right->Hide();
	}
#endif
}

int PlaybackWindow::getCurrentFileType()
{
	//db_msg("[debug_zhb]--->getCurrentFileType  mediaf_image_[%d].fileType = %d",select_file_%IMAGEVIEW_NUM,mediaf_image_[select_file_%IMAGEVIEW_NUM].fileType);
	#ifndef USEICONTHUMB
	if(mediaf_image_[select_file_%IMAGEVIEW_NUM].fileType == PHOTO_A || mediaf_image_[select_file_%IMAGEVIEW_NUM].fileType == PHOTO_B)
	#else
	std::map<int, FileInfo>::iterator itr;
	itr = FileInfo_.find(select_file_);
	if (itr ==FileInfo_.end()) {
		// no found
		return -1;		// error
	}
	
	if((itr->second).fileType == PHOTO_A || (itr->second).fileType == PHOTO_B)
	#endif
		return 0;//current file is photo
	else 
		return 1;//current file is video
}

void PlaybackWindow::HideListView()
{
	#ifdef S_FAKE_BG
	db_msg("debug_zhb---> hide the fakebg");
	fakeBg(false);//hide the 
	#endif
	listview_flag_ = false;
	#ifndef USEICONTHUMB
	for(int i = 0; i < IMAGEVIEW_NUM; i++){
		show_thumb_view_[i]->Hide();
		CtrlSelectFileRect(i,false);
		show_thumb_time_[i] ->Hide();
		CtrlFileTypeIcon(FileType_UNKNOWN_TYPE,i);
	} 	
	#else
	ivplayback_thumb_->Hide();
	setenv("MOSE_MOVE_ADJUST", "0", 1);
	#endif
}

void PlaybackWindow::ShowListView()
{
	listview_flag_ = true;
	#ifndef USEICONTHUMB
	for(int i = 0; i < IMAGEVIEW_NUM; i++){
		show_thumb_view_[i]->Show();
		CtrlSelectFileRect(i,true);
	} 	
	#else
	setenv("MOSE_MOVE_ADJUST", "1", 1);
	ivplayback_thumb_->Show();
	#endif
}

void PlaybackWindow::UpdateVideoPlayImg()
{
	if(select_mode_ == PLAY_STOP){
		play_back_icon_->SetImage(R::get()->GetImagePath("pb_playing_y").c_str());
		play_back_icon_->Show();
		delete_icon_->SetImage(R::get()->GetImagePath("pb_delete_n").c_str());
		delete_icon_->Show();
	}else if(select_mode_ == PLAY_DELETE){
		play_back_icon_->SetImage(R::get()->GetImagePath("pb_playing_n").c_str());
		play_back_icon_->Show();
		delete_icon_->SetImage(R::get()->GetImagePath("pb_delete_y").c_str());
		delete_icon_->Show();
	}
}

void PlaybackWindow::ShowVideoPlayImg()
{
	db_msg("[debug_zhb]--------ready to show  the video play img");
	play_back_icon_->Show();
	delete_icon_->Show();
	select_flag_ = true;
}

void PlaybackWindow::HideVideoPlayImgAndIcon(bool mshow)
{
    db_warn("[debug_zhb]--->HideVideoPlayImgAndIcon");
#ifndef S_FAKE_BG
    if(mshow)
    	SetWindowBackImage(R::get()->GetImagePath("playback_bg").c_str());
#else
	db_msg("debug_zhb---> show the fakebg");
	fakeBg(true);//show the fakebg
#endif
    HideVideoPlayImg();
    show_thumb_jpg_->Hide();
    progress_bar_->Hide();
    
}

void PlaybackWindow::ShowVideoPlayImgAndIcon()
{
    db_msg("[debug_zhb]--------ShowVideoPlayImgAndIcon");
    play_back_icon_->Show();
    delete_icon_->Show();
    show_thumb_jpg_->Show(); 
}

void PlaybackWindow::HideVideoPlayImg()
{
	db_msg("[debug_zhb]--------ready to  hide the video play img");
       play_back_icon_->Hide();
	delete_icon_->Hide();
	select_flag_ = false;
}

void PlaybackWindow::HideWindowBackImg()
{
    #ifndef S_FAKE_BG
    SetWindowBackImage(NULL);
    #endif
}

void PlaybackWindow::ShowThumbImg()
{
    show_thumb_jpg_->Show();
}

void PlaybackWindow::SetThumbShowCtrl(const char * path)
{
    if (path != NULL && access(path, F_OK) == 0)
    {
    	 db_msg("zhb-----SetThumbShowCtrl-----");
	#ifndef S_FAKE_BG
        SetWindowBackImage(NULL);
	#endif
        show_thumb_jpg_->SetImage(path);
        show_thumb_jpg_->Show();
    }
    else
    {
        HideVideoPlayImgAndIcon();
    }
}

void PlaybackWindow::ShowPlaylistButtonProc(View *control)
{
    // let presenter to fill playlist before show playlist window
    listener_->notify(this, PLAYLIST_FILLLIST, 1);
    playlist_win_->DoShow();
}

void PlaybackWindow::SettingButtonProc(View *control)
{
    // TODO:

    stop_timer(ctrlbar_timer_id_);
    set_one_shot_timer(3, 0, ctrlbar_timer_id_);
}

void PlaybackWindow::PlayProgressSeekProc(View *control, int pos)
{
    listener_->notify(this, control->GetTag(), pos);
}


void PlaybackWindow::keyPlayPauseButtonProc()
{
	db_msg("debug_zhb-----keyPlayPauseButtonProc-----select_file_ = %d",select_file_);
	int ret = 0;
    int status = StorageManager::GetInstance()->GetStorageStatus();
    if( (status == UMOUNT) || (status == STORAGE_FS_ERROR)  || (status == FORMATTING) )	
		return ;

	
	if(listview_flag_){
		HideListView();
		//SetThumbShowCtrl(mediaf_image_[select_file_%IMAGEVIEW_NUM].filepath.c_str());
		//UpdateVideoPlayImg();
		player_status_ = STOPED;
		//return;
	}
	if (player_status_ == PAUSED || player_status_ == STOPED || player_status_ == COMPLETION) {
        db_msg("[debug_jason]:########player_status_ = PAUSED###########");
		listener_->sendmsg(this, PLAYBACK_SET_SHOW_THS, select_file_);
		ret = listener_->sendmsg(this, PLAYBACK_PLAY_PAUSE, 1);
		if (ret != 0) {
			// TODO: info to user
			return;
		}
	
	} else if (player_status_ == PLAYING) {
	   db_msg("[debug_jason]:########player_status_ = PLAYING###########");
	   listener_->sendmsg(this, PLAYBACK_SET_SHOW_THS, select_file_);
		ret = listener_->sendmsg(this, PLAYBACK_PLAY_PAUSE, 0);	
		if (ret != 0) {
			// TODO: info to user
			return;
		}
	
	}
}

void PlaybackWindow::ResetFlagStatus()
{
	 //db_msg("debug_zhb--->ResetFlagStatus");
	 player_status_ = STOPED;
	 select_mode_ = PLAY_STOP;
}

void PlaybackWindow::keyPlayStopButtonProc()
{
	int ret = 0;
	if(player_status_ == PLAYING || player_status_ == PAUSED)
    	{
	        ret = listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
	        if (ret != 0)
	        {
	            return;
	        }
	}
	return ;
}



void PlaybackWindow::PlayPauseButtonProc(View *control)
{
    int ret = 0;

    if (player_status_ == PAUSED || player_status_ == STOPED) {
        ret = listener_->sendmsg(this, control->GetTag(), 1);

        if (ret != 0) {
            // TODO: info to user
            return;
        }

    } else if (player_status_ == PLAYING) {
        ret = listener_->sendmsg(this, control->GetTag(), 0);

        if (ret != 0) {
            // TODO: info to user
            return;
        }

    }

    stop_timer(ctrlbar_timer_id_);
    set_one_shot_timer(3, 0, ctrlbar_timer_id_);
}

void PlaybackWindow::PlayStopButtonProc(View *control)
{
    int ret = 0;

    ret = listener_->sendmsg(this, control->GetTag(), 0);

    if (ret != 0) {
        // TODO: info to user
        return;
    }

    stop_timer(ctrlbar_timer_id_);
    set_one_shot_timer(3, 0, ctrlbar_timer_id_);
}

void PlaybackWindow::StartPlay()
{
    set_period_timer(1, 0, play_timer_id_);
	#ifndef S_FAKE_BG
    SetWindowBackImage(NULL);
	#endif
    db_msg("[debug_jaosn]:StartPlay");
    progress_bar_->Show();
    player_status_ = PLAYING;
}

void PlaybackWindow::PausePlay()
{
    stop_timer(play_timer_id_);

    db_msg("[debug_jaosn]:PausePlay");
   // ShowVideoPlayImg();
    player_status_ = PAUSED;
}

void PlaybackWindow::StopPlay()
{
    stop_timer(play_timer_id_);
    if( player_status_ == PLAYING || player_status_ == PAUSED)
    {
        progress_bar_->SetProgressSeekValue(100);
        progress_bar_->SetProgressSeekValue(0);
        progress_bar_->Hide();
    }
    db_msg("[debug_jaosn]:StopPlay");
    player_status_ = STOPED;
}

void PlaybackWindow::SetPlayerFinish()
{
    player_status_ = COMPLETION;
}

void PlaybackWindow::VoiceControlButtonProc(View *control)
{
    static int voice_level = -1;

    voice_level++;

    if (voice_level > 4) voice_level = 0;

    // TODO: read value from default setting

    listener_->notify(this, control->GetTag(), voice_level);

    GraphicView::LoadImage(GetControl("gv_voice"), voice_icons[voice_level]);

    stop_timer(ctrlbar_timer_id_);
    set_one_shot_timer(3, 0, ctrlbar_timer_id_);
}

string PlaybackWindow::GetResourceName()
{
    return string(GetClassName());
}

void PlaybackWindow::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    db_msg("handle msg:%d", msg);
    switch (msg) {
        case MSG_CAMERA_START_PREVIEW:
            break;
        case MSG_CAMERA_STOP_PREVIEW:
            break;
        case MSG_RECORD_START: {
            // tv_loading0_->Hide();
            // tv_loading1_->Hide();
        }
            break;
        case MSG_RECORD_STOP: {
        }
            break;
        case MSG_VIDEO_PLAY_START:
            this->StartPlay();
            Screensaver::GetInstance()->pause(true);//stop  the timer
            gettimeofday(&tm1, NULL);
            break;
        case MSG_VIDEO_PLAY_STOP:
            this->StopPlay();
            //Screensaver::GetInstance()->pause(false);//stop status �п����ǻ���Ƶ��ʱ��������Ϣ��Ҳ�п����ǲ��ŵĹ������л�����
            break;
        case MSG_VIDEO_PLAY_PAUSE:
            this->PausePlay();
            break;
        case MSG_VIDEO_PLAY_COMPLETION:
            {
		SendMsgProxy(this, PLAYBACK_COMPLETION_TIMER, playback_timer_);		
		progress_bar_->Show();
		player_status_ = COMPLETION;
		//Screensaver::GetInstance()->pause(false);
		stop_timer(play_timer_id_);		
            }
            break;
        case MSG_SHOW_HDMI_MASK:
            {
                WindowManager* win_m = WindowManager::GetInstance();
                if (win_m->GetCurrentWinID() == WINDOWID_PLAYBACK) {
                    win_m->GetWindow(WINDOWID_STATUSBAR)->Hide();
                    progress_bar_->Hide();
                }
            }
            break;
        case MSG_HIDE_HDMI_MASK:
            {
                WindowManager* win_m = WindowManager::GetInstance();
                if (win_m->GetCurrentWinID() == WINDOWID_PLAYBACK) {
                    win_m->GetWindow(WINDOWID_STATUSBAR)->Show();
                    win_m->GetWindow(WINDOWID_STATUSBAR_BOTTOM)->Hide();
                    this->DoShow();
                }
            }
            break;
        default:
            break;
    }
}

void PlaybackWindow::PreInitCtrl(View *ctrl, string &ctrl_name)
{
    if (ctrl_name == "progress_bar" ||
        ctrl_name == "show_thumb_jpg"||
        ctrl_name == "show_play_img" ||
        ctrl_name == "show_delete_img"){
        ctrl->SetCtrlTransparentStyle(false);
    }else{
        ctrl->SetCtrlTransparentStyle(true);
    	}
    if (ctrl_name == string("show_thumb_jpg") ||
	ctrl_name == string("imageview_0") ||
	ctrl_name == string("imageview_1") ||
	ctrl_name == string("imageview_2") ||
	ctrl_name == string("imageview_3") ||
	ctrl_name == string("imageview_4") ||
	ctrl_name == string("imageview_5")) {
        DWORD style;
        GraphicView *gv = reinterpret_cast<GraphicView *>(ctrl);
        gv->GetOptionStyle(style);
        style &= ~SS_REALSIZEIMAGE;
        gv->SetOptionStyle(style);
    }
	
}

void PlaybackWindow::PlayProgressUpdate(union sigval sigval)
{
    PlaybackWindow *self = reinterpret_cast<PlaybackWindow *>(sigval.sival_ptr);
    self->progress_bar_->UpdateProgressByStep();
	self->SendMsgProxy(self, PLAYBACK_UPDATE_TIMER, self->playback_timer_);
}

void PlaybackWindow::ResetPlayProgress()
{
    progress_bar_->SetProgressSeekValue(0);
}

void PlaybackWindow::SetPlayProgress(int msec)
{
    progress_bar_->SetProgressSeekValue(msec / 1000);
}

void PlaybackWindow::SetPlayDuration(int msec)
{
    progress_bar_->SetProgressRange(0, msec / 1000);
    playback_timer_ = msec / 1000;
}

PlaylistWindow *PlaybackWindow::GetPlaylistWindow()
{
    return playlist_win_;
}

void PlaybackWindow::HideControlBar()
{
    progress_bar_->Hide();
}

void PlaybackWindow::ShowControlBar()
{
    progress_bar_->Show();
}

void PlaybackWindow::CtrlbarTimerProc(union sigval sigval)
{
    PlaybackWindow *self = reinterpret_cast<PlaybackWindow*>(sigval.sival_ptr);
    self->ShowControlBar();
}

void PlaybackWindow::NotifyProxy(class Window *from, int msg, int val)
{
    listener_->notify(from, msg, val);
}

int PlaybackWindow::SendMsgProxy(class Window *from, int msg, int val)
{
    return listener_->sendmsg(from, msg, val);
}

int PlaybackWindow::OnMouseUp(unsigned int button_status, int x, int y)
{
	return 0;
    RECT top_rect, bottom_rect;

    GetControl("return_btn")->GetRect(&top_rect);
    GetControl("gv_voice")->GetRect(&bottom_rect);

    db_info("touch.y: %d, top.y: %d, bottom.y: %d", y, top_rect.top, bottom_rect.top);

    if (y > top_rect.top && y < bottom_rect.top) {
         if (OnClick) OnClick(this);
    }

    return View::OnMouseUp(button_status, x, y);
}

void PlaybackWindow::PlaybackWindowProc(View *control)
{
    ShowControlBar();

    stop_timer(ctrlbar_timer_id_);
    set_one_shot_timer(3, 0, ctrlbar_timer_id_);
}

void PlaybackWindow::ShowDeleteDialog(int val)
{
#ifndef USEICONTHUMB
    if(val)
    		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_PHOTO);
	else
		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_VIDEO);
#else
    if(!val)
    		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_PHOTO);
	else if (val == 1)
		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_VIDEO);
	else
		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_ALLSELECTED);
#endif
    play_BulletCollection_->ShowButtonDialog();
}
void PlaybackWindow::HideDeleteDialog()
{

	if(play_BulletCollection_->getButtonDialogShowFlag()){
		play_BulletCollection_->BCDoHide();
	}
}
void PlaybackWindow::ShowStatusbar()
{
	db_msg("debug_zhb-----ShowStatusbar----");
    WindowManager* win_m = WindowManager::GetInstance();
	win_m->GetWindow(WINDOWID_STATUSBAR)->Show();
	this->DoShow();
    play_flag_ = true;
}

void PlaybackWindow::HideStatusbar()
{
	db_msg("debug_zhb-----HideStatusbar----");
    WindowManager* win_m = WindowManager::GetInstance();
	win_m->GetWindow(WINDOWID_STATUSBAR)->Hide();
	play_flag_ = false;
}

int PlaybackWindow::getPlayerStatus()
{
    return player_status_;
}

void PlaybackWindow::AbnormalVideoPlayReSetStatus(bool flag)
{
	db_warn("AbnormalVideoPlayReSetStatus : %d",flag);
	ResetFlagStatus();	
	HideStatusbar();
	listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
	listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);	// 回到播放缩略图界面
	#ifdef S_PLAYB_STATUSBAR
	playBackSatutarbar(true);
	#endif
	select_flag_ = false;
	 Screensaver::GetInstance()->pause(false);//start screensaver timer
	if(flag){
		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_ERROR_VIDEO);	// 显示删除视频文件错误窗口
    	play_BulletCollection_->ShowButtonDialog();
    	SetPlaybackStatusIconInvalidFlag(true);
    	db_warn("show delete error file dialog,set invalid flag true ");
    	listener_->sendmsg(this,PLAYBACK_DELETEFILE_ICONGRAY,0);
	}
}

void PlaybackWindow::AbnormalPhotoPlayReSetStatus(bool flag)
{
	db_warn("AbnormalPhotoPlayReSetStatus : %d",flag);
	ResetFlagStatus();	
	HideStatusbar();
	listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);		// MSG_PIC_PLAY_START
	listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);	// 回到播放缩略图界面
	#ifdef S_PLAYB_STATUSBAR
	playBackSatutarbar(true);
	#endif
	select_flag_ = false;
	 Screensaver::GetInstance()->pause(false);//start screensaver timer
	if(flag){
		play_BulletCollection_->setButtonDialogCurrentId(BC_BUTTON_DIALOG_DELETE_ERROR_PHOTO);	// 显示删除视频文件错误窗口
    	play_BulletCollection_->ShowButtonDialog();
    	SetPlaybackStatusIconInvalidFlag(true);
    	db_warn("show delete error file dialog,set invalid flag true ");
    	listener_->sendmsg(this,PLAYBACK_DELETEFILE_ICONGRAY,0);
	}
}

void PlaybackWindow::OnLanguageChanged()
{

}


void PlaybackWindow::ButtonClickProc(View *control)
{
    db_warn("[habo]--->PlaybackWindow ButtonClickProc  ready to return the preview\n");
	//this->ivplayback_thumb_->Hide();
	//AudioCtrl::GetInstance()->PlaySound(AudioCtrl::KEY1_SOUND);
	//usleep(50*1000);
    this->keyProc(SDV_KEY_MENU, SHORT_PRESS);
    
}

bool PlaybackWindow::IsPlayingWindow()
{
    if(player_status_ == PLAYING || player_status_ == PAUSED || player_status_ == COMPLETION)
        return true;
    else
        return false;
}

bool PlaybackWindow::IsDeleteDialogWindow()
{
   return select_flag_;

}


void PlaybackWindow::ImageViewClickProc(View *control)
{
    if(ignore_message_flag_){
        db_error("usb host connect,ignore message!!!");
        return;
    }
#ifndef USEICONTHUMB    
    
    if(m_flig_rl)
    {
        db_warn("now is right or left flig !!!");
        m_flig_rl = false;
        return;
    }
    
     int tag = control->GetTag();
    db_warn("[habo]--->PlaybackWindow ImageViewClickProc tag = %d\n",tag);
    
    
    if (player_status_ == STOPED) 
   {
		if(page_num_ == 1)
    		select_file_ =  tag;
    	else
    		select_file_ = (page_num_ - 1) * IMAGEVIEW_NUM + tag;
    	db_warn("[ghy], select_file_choose=%d select_file_ %d,file_sum_ %d",tag,select_file_,file_sum_);
		if(select_file_ >= file_sum_)
        {
			 if (select_file_ == 0 && file_sum_ == 1) //相册中只有一个文件
				 goto Play;
			 else 
             {
				 db_warn("no video play");
				 return ;
			 }
		}
Play:
		if (player_status_ == STOPED || player_status_ == COMPLETION) 
       {//if completion ,replay .if stoped ,start play
			if(select_file_%IMAGEVIEW_NUM)
            {
				UpdateListView(false);
			} 
            else 
            {
				UpdateListView(true);
			}
		}
		#ifdef S_PLAYB_STATUSBAR
		playBackSatutarbar(false);
		#endif
		listview_flag_ = true;
		keyPlayPauseButtonProc(); 
		listener_->sendmsg(this, MSG_PLAYBACK_TO_PLAY_WINDOW, 0);
    }
    else
    {
    	db_error("video is play==============");
    }
#endif    
}


int PlaybackWindow::FileLockCtl()
{
#ifndef USEICONTHUMB
    pthread_mutex_lock(&m_lock_file_ctl);
    int ret = -1;
    int index = -1;

    index = select_file_%IMAGEVIEW_NUM;
    //get current select file name 
    if(mediaf_image_[index].fileType == VIDEO_A_SOS || mediaf_image_[index].fileType == VIDEO_B_SOS
       ||mediaf_image_[index].fileType == VIDEO_A || mediaf_image_[index].fileType == VIDEO_B )
    {
        //lock <---> unlock   VIDEO_A_SOS <--> VIDEO_A   VIDEO_B_SOS <---> VIDEO_B

        int lock_status = -1;
        std::string dts_path;
        std::string file_name,file_name_bak,file_ths_name,file_ths_name_bak,file_type;
        char buf[512]={0};
        vector<string>::const_iterator c_it;	
		if(playlist_.size() <= 0){
            ret = -1;
	        goto out;
	    }
		c_it = playlist_.begin()+select_file_;
		if(c_it == playlist_.end())
		{
            db_error("now is playlist.end ");
			ret = -1;
	        goto out;
		}
        file_name = mediaf_image_[index].filename;
        if(access(file_name.c_str(), F_OK) != 0)
        {
            db_error("filename is not exist !!!");
            ret = -1;
	        goto out;
        }
        string::size_type rc = file_name.rfind(".");
		if( rc == string::npos)
		{
			db_error("invalid fileName:%s",file_name.c_str());
			ret = -1;
	        goto out;
		}

        //change the mediaf_image fileType
        if(mediaf_image_[index].fileType == VIDEO_A)
        {
            mediaf_image_[index].fileType = VIDEO_A_SOS;
            file_type = "videoA_SOS";
            lock_status = 1;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("F/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
            file_name_bak = file_name.substr(rc1+2,rc-(rc1+2));
            dts_path = DIR_2CAT(MOUNT_PATH, EVENT_DIR_A);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
            file_name_bak+="_SOS.ts";
            if(mediaf_image_[index].isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_ths_SOS.jpg";
                file_ths_name = file_name.substr(0,rc);	
                file_ths_name += "_ths.jpg";	
            }
            else
                file_ths_name="";

            CtrlFileTypeIcon(VIDEO_A_SOS,index);
        }
        else if(mediaf_image_[index].fileType == VIDEO_B)
        {
            mediaf_image_[index].fileType = VIDEO_B_SOS;
            file_type = "videoB_SOS";
            lock_status = 1;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("R/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
            file_name_bak = file_name.substr(rc1+2,rc-(rc1+2));	
            dts_path = DIR_2CAT(MOUNT_PATH, EVENT_DIR_B);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
            file_name_bak+="_SOS.ts";
            if(mediaf_image_[index].isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_ths_SOS.jpg";
                file_ths_name = file_name.substr(0,rc);	
                file_ths_name += "_ths.jpg";
            }
            else
                file_ths_name="";

            CtrlFileTypeIcon(VIDEO_B_SOS,index);
        }
        else if(mediaf_image_[index].fileType == VIDEO_A_SOS)
        {
            mediaf_image_[index].fileType = VIDEO_A;
            file_type = "video_A";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("F/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
            file_name_bak+=".ts";
            if(mediaf_image_[index].isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_SOS"));	
                file_ths_name += "_ths_SOS.jpg";
            }
            else
                file_ths_name="";

            CtrlFileTypeIcon(VIDEO_A,index);
        }
        else if(mediaf_image_[index].fileType == VIDEO_B_SOS)
        {
            mediaf_image_[index].fileType = VIDEO_B;
            file_type = "video_B";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("R/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));	
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
            file_name_bak+=".ts";
            if(mediaf_image_[index].isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_SOS"));	
                file_ths_name += "_ths_SOS.jpg";
            }
            else
                file_ths_name="";

            CtrlFileTypeIcon(VIDEO_B,index);
        }
        
        //update the playlist and filepath info
        if(!file_ths_name_bak.empty())
            mediaf_image_[index].filepath = file_ths_name_bak.c_str();
        mediaf_image_[index].filename = file_name_bak.c_str();
        playlist_[select_file_] = file_name_bak.c_str();//update playlist file name
        db_debug("==========index %d playlist_ file name %s==========",
                index,playlist_[select_file_].c_str());
        m_update_file_name.clear();
        m_update_file_name = file_name_bak.c_str();
        //mv the ths.jpg and ts
        snprintf(buf,sizeof(buf)-1,"mv %s %s",file_name.c_str(),file_name_bak.c_str());
        system(buf);
        if(!file_ths_name_bak.empty())
        {
            snprintf(buf,sizeof(buf)-1,"mv %s %s",file_ths_name.c_str(),file_ths_name_bak.c_str());
            system(buf);
        }
        //change the sql 
        db_debug("file_name %s file_name_bak %s select_file_ %d",file_name.c_str(),file_name_bak.c_str(),select_file_);
        MediaFileManager::GetInstance()->SetFileInfoByName(file_name,file_name_bak,file_type,lock_status,0);
        listener_->sendmsg(this, PLAYBACK_CHANGE_FILE_NAME, select_file_);
    }
    else if(mediaf_image_[index].fileType == VIDEO_A_P || mediaf_image_[index].fileType == VIDEO_B_P)
    {
        PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
	    pre_win->ShowPromptBox(PROMPT_BOX_PARKING_VIDEO);
    }
    else
    {
        db_warn("current file index %d type %d is not the video type !!!",
                index, mediaf_image_[index].fileType);
        ret = -1;
    }
out:
    
    pthread_mutex_unlock(&m_lock_file_ctl);

    return ret;

#else
#if 0
	int count = FileInfo_.size();
	
	for (int i=0; i<count; i++)
	{
		if (ivplayback_thumb_->GetIconItem_select(i) == 2) {
			FileLockCtl(i);
		}
		
	}
	ivplayback_thumb_->Refresh();
#else
	FileLockCtl(select_file_);
#endif
#endif
}
#ifdef USEICONTHUMB
int PlaybackWindow::FileLockCtl(int index)
{
    pthread_mutex_lock(&m_lock_file_ctl);
    int ret = -1;

    //index = select_file_%IMAGEVIEW_NUM;
    //get current select file name 
    //index = select_file_;
    
	db_error("FileLockCtl: %d ",index);
	std::map<int, FileInfo>::iterator itr;
	itr = FileInfo_.find(index);
	if (itr ==FileInfo_.end()) {
		// no found
		db_error("no key[%] in FileInfo_",index);
		ret = -1;
	    goto out;
	}
	//db_error("%d %s",(itr->first), (itr->second).filename);
		
    if((itr->second).fileType == VIDEO_A_SOS || (itr->second).fileType == VIDEO_B_SOS
       ||(itr->second).fileType == VIDEO_A || (itr->second).fileType == VIDEO_B )
    {
        //lock <---> unlock   VIDEO_A_SOS <--> VIDEO_A   VIDEO_B_SOS <---> VIDEO_B

        int lock_status = -1;
        std::string dts_path;
        std::string file_name,file_name_bak,file_ths_name,file_ths_name_bak,file_type;
        char buf[512]={0};
        vector<string>::const_iterator c_it;	
		if(playlist_.size() <= 0){
            ret = -1;
	        goto out;
	    }
		c_it = playlist_.begin()+index;
		if(c_it == playlist_.end())
		{
            db_error("now is playlist.end ");
			ret = -1;
	        goto out;
		}
        file_name = (itr->second).filename;
        if(access(file_name.c_str(), F_OK) != 0)
        {
            db_error("filename is not exist !!!");
            ret = -1;
	        goto out;
        }
        string::size_type rc = file_name.rfind(".");
		if( rc == string::npos)
		{
			db_error("invalid fileName:%s",file_name.c_str());
			ret = -1;
	        goto out;
		}

        //change the mediaf_image fileType
        if((itr->second).fileType == VIDEO_A)		// 20190612_092811.ts -> 20190612_092811_SOS.ts
        {
            (itr->second).fileType = VIDEO_A_SOS;
            file_type = "videoA_SOS";
            lock_status = 1;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("F/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-(rc1+2));
			#else
			file_name_bak = file_name.substr(rc1,rc-(rc1));
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, EVENT_DIR_A);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
			file_name_bak+="_SOS.mp4";
			#else
            file_name_bak+="_SOS.ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_SOS_ths.jpg";
                file_ths_name = file_name.substr(0,rc);	
                file_ths_name += "_ths.jpg";	
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_A_SOS,index);
			//ivplayback_thumb_->SetIconItem_Icon(&icon_filetype_[6], index, 2);
			ivplayback_thumb_->SetIconItem_lock(index,1);
        }
        else if((itr->second).fileType == VIDEO_B)
        {
            (itr->second).fileType = VIDEO_B_SOS;
            file_type = "videoB_SOS";
            lock_status = 1;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("R/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-(rc1+2));	
			#else
			file_name_bak = file_name.substr(rc1,rc-(rc1));	
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, EVENT_DIR_B);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
            file_name_bak+="_SOS.mp4";
			#else
			file_name_bak+="_SOS.ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_SOS_ths.jpg";
                file_ths_name = file_name.substr(0,rc);	
                file_ths_name += "_ths.jpg";
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_B_SOS,index);
			//ivplayback_thumb_->SetIconItem_Icon(&icon_filetype_[6], index, 2);
			ivplayback_thumb_->SetIconItem_lock(index,1);
        }
        else if((itr->second).fileType == VIDEO_A_SOS)		// video/F/20190612_092811_SOS.ts	-> 20190612_092811.ts
        {
            (itr->second).fileType = VIDEO_A;
            file_type = "video_A";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("F/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));
			#else
			file_name_bak = file_name.substr(rc1,rc-strlen("_SOS")-(rc1));
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
			file_name_bak+=".mp4";
			#else
            file_name_bak+=".ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_SOS"));	
                file_ths_name += "_SOS_ths.jpg";
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_A,index);
			//ivplayback_thumb_->SetIconItem_Icon(NULL,index,2);
			ivplayback_thumb_->SetIconItem_lock(index,0);
        }
        else if((itr->second).fileType == VIDEO_B_SOS)
        {
            (itr->second).fileType = VIDEO_B;
            file_type = "video_B";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("R/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));	
			#else
			file_name_bak = file_name.substr(rc1,rc-strlen("_SOS")-(rc1));	
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
            file_name_bak+=".mp4";
			#else
			file_name_bak+=".ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;		// DST
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_SOS"));	
                file_ths_name += "_SOS_ths.jpg";	// SRC
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_B,index);
			//ivplayback_thumb_->SetIconItem_Icon(NULL,index,2);
			ivplayback_thumb_->SetIconItem_lock(index,0);
        }
        
        //update the playlist and filepath info
        if(!file_ths_name_bak.empty())
            (itr->second).filepath = file_ths_name_bak.c_str();
        (itr->second).filename = file_name_bak.c_str();
        playlist_[index] = file_name_bak.c_str();//update playlist file name
        db_debug("==========index %d playlist_ file name %s==========",
                index,playlist_[index].c_str());
        m_update_file_name.clear();
        m_update_file_name = file_name_bak.c_str();
        //mv the ths.jpg and ts
        snprintf(buf,sizeof(buf)-1,"mv %s %s",file_name.c_str(),file_name_bak.c_str());
		//db_error("run cmd: %s",buf);
        system(buf);
        if(!file_ths_name_bak.empty())
        {
            snprintf(buf,sizeof(buf)-1,"mv %s %s",file_ths_name.c_str(),file_ths_name_bak.c_str());
			//db_error("run cmd: %s",buf);
            system(buf);
        }
        //change the sql 
        db_debug("file_name %s file_name_bak %s index %d",file_name.c_str(),file_name_bak.c_str(),index);
        MediaFileManager::GetInstance()->SetFileInfoByName(file_name,file_name_bak,file_type,lock_status,0);
        listener_->sendmsg(this, PLAYBACK_CHANGE_FILE_NAME, index);

    }
    else if((itr->second).fileType == VIDEO_A_P || (itr->second).fileType == VIDEO_B_P)
    {
		#ifndef SUPPORT_CHANGEPARKFILE
		PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(WindowManager::GetInstance()->GetWindow(WINDOWID_PREVIEW));
	    pre_win->ShowPromptBox(PROMPT_BOX_PARKING_VIDEO);
		#else
		//lock <---> unlock   VIDEO_A_PARK --> VIDEO_A   VIDEO_B_PARK --> VIDEO_B

        int lock_status = -1;
        std::string dts_path;
        std::string file_name,file_name_bak,file_ths_name,file_ths_name_bak,file_type;
        char buf[512]={0};
        vector<string>::const_iterator c_it;	
		if(playlist_.size() <= 0){
            ret = -1;
	        goto out;
	    }
		c_it = playlist_.begin()+index;
		if(c_it == playlist_.end())
		{
            db_error("now is playlist.end ");
			ret = -1;
	        goto out;
		}
        file_name = (itr->second).filename;
		db_error("park filename: %s",file_name.c_str());
        if(access(file_name.c_str(), F_OK) != 0)
        {
            db_error("filename is not exist !!!");
            ret = -1;
	        goto out;
        }
        string::size_type rc = file_name.rfind(".");
		if( rc == string::npos)
		{
			db_error("invalid fileName:%s",file_name.c_str());
			ret = -1;
	        goto out;
		}

        //change the mediaf_image fileType
        if((itr->second).fileType == VIDEO_A_P)		// event/F/20190612_092811_PARK.ts	-> 20190612_092811.ts
        {
            (itr->second).fileType = VIDEO_A;		
            file_type = "video_A";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("F/");
			#else
			string::size_type rc1 = file_name.rfind("/");
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));
			#else
			file_name_bak = file_name.substr(rc1,rc-strlen("_PARK")-(rc1));
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
			file_name_bak+=".mp4";
			#else
            file_name_bak+=".ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;			// DST
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_PARK"));	
                file_ths_name += "_PARK_ths.jpg";		// SRC
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_A,index);
			//ivplayback_thumb_->SetIconItem_Icon(NULL,index,2);
			ivplayback_thumb_->SetIconItem_lock(index,0);
        }
        else if((itr->second).fileType == VIDEO_B_P)
        {
            (itr->second).fileType = VIDEO_B;
            file_type = "video_B";
            lock_status = 0;
			#ifdef USE_CAMB
            string::size_type rc1 = file_name.rfind("R/");
			#else
			string::size_type rc1 = file_name.rfind("/");		// 20180917_091235_PARK.ts
			#endif
    		if( rc1 == string::npos)
    		{
    			db_error("invalid fileName:%s",file_name.c_str());
    			ret = -1;
	            goto out;
    		}
			#ifdef USE_CAMB
            file_name_bak = file_name.substr(rc1+2,rc-strlen("_SOS")-(rc1+2));	
			#else
			file_name_bak = file_name.substr(rc1,rc-strlen("_PARK")-(rc1));	
			#endif
            dts_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
            dts_path+=file_name_bak;
            file_name_bak=dts_path;
			#ifdef VIDEOTYPE_MP4
            file_name_bak+=".mp4";
			#else
			file_name_bak+=".ts";
			#endif
            if((itr->second).isHaveThumJPG == 1)
            {
                file_ths_name_bak = dts_path;		// bak dst
                file_ths_name_bak+="_ths.jpg";
                file_ths_name = file_name.substr(0,rc-strlen("_PARK"));		// src
                file_ths_name += "_PARK_ths.jpg";
            }
            else
                file_ths_name="";

            //CtrlFileTypeIcon(VIDEO_B,index);
			//ivplayback_thumb_->SetIconItem_Icon(NULL,index,2);
			ivplayback_thumb_->SetIconItem_lock(index,0);
        }
        
        //update the playlist and filepath info
        if(!file_ths_name_bak.empty())
            (itr->second).filepath = file_ths_name_bak.c_str();
        (itr->second).filename = file_name_bak.c_str();
        playlist_[index] = file_name_bak.c_str();//update playlist file name
        db_debug("==========index %d playlist_ file name %s==========",
                index,playlist_[index].c_str());
        m_update_file_name.clear();
        m_update_file_name = file_name_bak.c_str();
        //mv the ths.jpg and ts  		// mv srcfile dstfile
        
        snprintf(buf,sizeof(buf)-1,"mv %s %s",file_name.c_str(),file_name_bak.c_str());
		//db_error("run cmd: %s",buf);	//  mv /mnt/extsd/event/20190718_163402_PARK.mp4 /mnt/extsd/video//20190718_163402.mp4
        system(buf);
        if(!file_ths_name_bak.empty())
        {
            snprintf(buf,sizeof(buf)-1,"mv %s %s",file_ths_name.c_str(),file_ths_name_bak.c_str());
			//db_error("run cmd: %s",buf);	// mv /mnt/extsd/event/20190718_163402_PARK_ths.jpg /mnt/extsd/video//20190718_163402_ths.jpg
            system(buf);
        }
        //change the sql 
        db_debug("file_name %s file_name_bak %s index %d",file_name.c_str(),file_name_bak.c_str(),index);
        MediaFileManager::GetInstance()->SetFileInfoByName(file_name,file_name_bak,file_type,lock_status,0);
        listener_->sendmsg(this, PLAYBACK_CHANGE_FILE_NAME, index);
		#endif
    }
    else
    {
        db_warn("current file index %d type %d is not the video type !!!",
                index, (itr->second).fileType);
        ret = -1;
    }
out:
    
    pthread_mutex_unlock(&m_lock_file_ctl);

    return ret;

}
#endif
void PlaybackWindow::UpdateListFileName(std::vector<std::string> &file_list,int index_)
{
	if(!file_list.empty())
	{
        file_list[index_] = m_update_file_name.c_str();
	}
}

void PlaybackWindow::OnLoadWindowUpdateLeftRightIcon(int sum)
{
    //切换到playback window的时候，第一次show left right icon status 
    if(sum <= IMAGEVIEW_NUM)
        listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_HIDE);
    else 
        listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,RIGHT_SHOW_ONLY);
}

void PlaybackWindow::PlayingToPlaybackUpdateShowLeftRightIcon()
{
    if(file_sum_ <= IMAGEVIEW_NUM) //mean the first page
    {
       listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_HIDE);
    }
    else
    {
        //db_warn("habo-->page_num_ = %d file_sum_ - (page_num_)*IMAGEVIEW_NUM = %d",page_num_,file_sum_ - (page_num_)*IMAGEVIEW_NUM);
        if(file_sum_ - (page_num_+1)*IMAGEVIEW_NUM >= IMAGEVIEW_NUM) //mean middle 
            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_RIGHT_SHOW);
        else //mean the last page
            listener_->sendmsg(this,MSG_PLAYBACK_PAGE_UP_DOWN_ICON,LEFT_SHOW_ONLY);
    }
}
#ifdef USEICONTHUMB
void PlaybackWindow::thumbIconItemClickProc(View *control, int index)
{
	int tag = index;
	
	db_error("IconItemClickProc index: %d",index);	// index is Highlighted Icon
	// do something like play selet ...
	if(m_flig_rl)
	{
        db_warn("now is right or left flig !!!");
        m_flig_rl = false;
        return;
    }
	if (file_sum_ <=0 ) return;
	if (select_status) {
		// in select status
		db_warn("in select status");
		select_file_ =  tag;
		int sel = ivplayback_thumb_->GetIconItem_select(select_file_);
		if (sel == 1) { // 待选择状态
			ivplayback_thumb_->SetIconItem_select(select_file_,2);	// 已选择状态
			SetFileInfoSelect(select_file_,2);
		} else if (sel == 2) { // 已选择状态
			ivplayback_thumb_->SetIconItem_select(select_file_,1);	// 待选择状态
			SetFileInfoSelect(select_file_,1);
		} 		
		ivplayback_thumb_->Refresh();
		return;
	}
    
    if (player_status_ == STOPED) 
   	{
		select_file_ =  tag;
    	
    	db_warn("[ghy], select_file_choose=%d select_file_ %d,file_sum_ %d",tag,select_file_,file_sum_);
		if(select_file_ >= file_sum_)
        {
			 if (select_file_ == 0 && file_sum_ == 1) //only 1 file
			 	goto Play;
			 else
             {
				 db_warn("no video play");
				 return ;
			 }
		}
Play:
		if (player_status_ == STOPED || player_status_ == COMPLETION) 
       {//if completion ,replay .if stoped ,start play
       		#if 0
			if(select_file_%IMAGEVIEW_NUM)
            {
				UpdateListView(false);
			} 
            else 
            {
				UpdateListView(true);
			}
			#endif
		}
		#ifdef S_PLAYB_STATUSBAR
		playBackSatutarbar(false);
		#endif
		listview_flag_ = true;
		keyPlayPauseButtonProc(); 	// hide thumb view 
		listener_->sendmsg(this, MSG_PLAYBACK_TO_PLAY_WINDOW, 0);
    }
    else
    {
    	db_error("video is play==============");
    }
}

void PlaybackWindow::SetPlaybackStatusIconInvalidFlag(bool invalid_flag)
{
	invalid_flag_ = invalid_flag;
	db_error("invalid_flag_: %d",invalid_flag_);
}

void PlaybackWindow::UpdateListView_threadstart()
{
#ifdef UPDATEIMGUSETHREAD
	if (m_UpdateIconviewImg_thread_id == 0) {
		UpdateIconviewImg_thread_flag = 0;
		ThreadCreate(&m_UpdateIconviewImg_thread_id,NULL,PlaybackWindow::IconviewUpdateImg_thread,this);
		//db_error("UpdateListView_threadstart");	
	}
#endif
}

void *PlaybackWindow::IconviewUpdateImg_thread(void *context)
{
#ifdef UPDATEIMGUSETHREAD
	PlaybackWindow *pbw = reinterpret_cast<PlaybackWindow*>(context);
	
	prctl(PR_SET_NAME, "ivUpdateImg_ths", 0, 0, 0);		// 最多15个字符
	
	while(1)
	{
		if(pbw->UpdateIconviewImg_thread_flag == 0){
			usleep(100*1000);
			continue;
		}
		pbw->UpdateIconviewImg_thread_flag = 0;
		// do update img
		int firstvis = pbw->ivplayback_thumb_->GetIconItem_FirstVisable();
		pbw->UpdateImgRange(firstvis);
		//sleep(1);
		
	}
	pbw->m_UpdateIconviewImg_thread_id = 0;
#endif
	return NULL;
}

// 加载从start开始num个文件的缩略图到对应的FileInfo里
// 并把0~(start - keepnum/2) 和 (start + keepnum/2)~end 的缩略图卸载
// 其目的是在内存中只保留最多keepnum张图片,以减少内存使用
// 一张320x240的bmp约240KB,16张约3840KB
/*
+----+  0   .begin
|    |
|----|  <-- (start            ) ---
|	 |                           ^
|	 |                           |
|	 |                           |keepnum
|	 |                           |
|	 |                           v
|----|	<-- (start + keepnum  ) ---
|	 |
|	 |
+----+  size() .end

*/

void PlaybackWindow::UpdateImgRange(int start)
{
#ifdef UPDATEIMGUSETHREAD	
	//db_error("UpdateImgRange %d lock: %d lastupdate: %d listview_flag_:%d",start, lockxx, lastupdate,listview_flag_);
	if ((listview_flag_==false) || lockxx || (lastupdate==start)){
		CancelUpdateListview = false;
		return;
	}
	//if (lockxx) goto out_;					// 上一次操作还没完成则忽略
	//if (lastupdate==start) goto out_;		// 上次操作和本次一样则忽略
	pthread_mutex_lock(&loadfile_ctl);
	lockxx = 1;							// 锁
	int istart = start;
	int startx = (start - 3)>=0? (start - 3) : 0;
	std::map<int, FileInfo>::iterator itr;
	db_error("----------------------------UpdateImg start (total: %d)------------",FileInfo_.size());
	//
	if (FileInfo_.size()) {
		int keepnum = 12;
		if (startx <0) startx = 0;
		if (startx >= FileInfo_.size()) startx = FileInfo_.size()-1;
		if (keepnum > FileInfo_.size()) {
			keepnum = FileInfo_.size();
			//start = keepnum/2;
		}
		if (startx + keepnum > FileInfo_.size()) keepnum = FileInfo_.size() - startx;
		//
		//db_error(" ------------------------------start: %d keepnum:%d total:%d",start,keepnum,FileInfo_.size());
		//int istart = (start - keepnum/2) >= 0? (start - keepnum/2): 0;
		//int inum   = (start + num ) > FileInfo_.size()? FileInfo_.size()-start : num;
		//num = inum;		// 实际可以更新的个数
		//db_error("start: %d keepnum:%d",start,keepnum);
		for (itr = FileInfo_.begin(); itr != FileInfo_.end(); itr++)
		{
			//db_error(" ------------------------------itr->first: %d ",itr->first);
			if ((itr->first) < (startx)) 
			{	// 0~(start)
				//db_error("111 %d",itr->first);
				if ((itr->second).thumbbmpload) {
					(itr->second).thumbbmpload = 0;
					if ( (itr->second).thumbbmp.bmBits ) {	// 卸载缩略图
						//db_error("unload bmp %d",itr->first);
						UnloadBitmap(&(itr->second).thumbbmp);
						memset(&(itr->second).thumbbmp,0,sizeof(BITMAP));	
					}	
				}
			} 
			else if((itr->first) < (startx + keepnum)) 
			{	// (start )~(start + keepnum)
				//db_error("222 ---%d",itr->first);
				if ( (itr->second).thumbbmpload == 0) {	// 没有缩略图则加载
					//db_error("load bmp %d",itr->first);
					memset(&FileInfo_[itr->first].thumbbmp, 0, sizeof(BITMAP));
					string bmp_path = FileInfo_[itr->first].filepath;
					::LoadBitmapFromFile(HDC_SCREEN, &FileInfo_[itr->first].thumbbmp, bmp_path.c_str());
					// 更新bmp到iconview
					ivplayback_thumb_->SetIconItem_Bmp(&FileInfo_[itr->first].thumbbmp,itr->first);
					(itr->second).thumbbmpload = 1;
					ivplayback_thumb_->Refresh();
					//usleep(25*1000);
				}
			} 
			else 
			{	// (start + keepnum)~end
				//db_error("333 %d",itr->first);
				if ((itr->second).thumbbmpload) {
					(itr->second).thumbbmpload = 0;
					if ( (itr->second).thumbbmp.bmBits ) {	// 卸载缩略图
						//db_error("unload bmp %d",itr->first);
						UnloadBitmap(&(itr->second).thumbbmp);
						memset(&(itr->second).thumbbmp,0,sizeof(BITMAP));	
				
					}
				}
			}
			//ivplayback_thumb_->Refresh();
			//usleep(5*1000);
			if (CancelUpdateListview) {
				db_error("cancel update iconview!!!!!!!!!");
				break;
			}
		}
		if (!CancelUpdateListview)
			ivplayback_thumb_->Refresh();

		//db_error("----end");
		
	}
	lockxx = 0;
	lastupdate=istart;

	db_error("============================UpdateImg end  ===========================");
	pthread_mutex_unlock(&loadfile_ctl);
#endif
}


void PlaybackWindow::SetFileInfoSelect(int index, int state)
{
	std::map<int, FileInfo>::iterator itr;
	if (index < 0) {
		for (itr = FileInfo_.begin(); itr != FileInfo_.end(); itr++)
		{
			(itr->second).fileselect = state;
		}
	} else {
		itr = FileInfo_.find(index);
		if (itr != FileInfo_.end()) {
			(itr->second).fileselect = state;
		}
	}
}

int PlaybackWindow::GetFileInfoSelectCount()
{
	int ret =0;
	if (FileInfo_.size() == 0)
		return 0;
	std::map<int, FileInfo>::iterator itr;
	for (itr = FileInfo_.begin(); itr != FileInfo_.end(); itr++)
	{
		if ((itr->second).fileselect == 2)
			ret++;
	}
	return ret;
}

void PlaybackWindow::FreeFileInfo()
{
	if (FileInfo_.size() > 0)
	{
		std::map<int, FileInfo>::iterator itr;
		for (itr = FileInfo_.begin(); itr != FileInfo_.end(); itr++)
		{
			if ((itr->second).thumbbmpload) {
				(itr->second).thumbbmpload = 0;
				if ( (itr->second).thumbbmp.bmBits ) {	// 卸载缩略图
					//db_error("unload bmp %d",itr->first);
					UnloadBitmap(&(itr->second).thumbbmp);
					memset(&(itr->second).thumbbmp,0,sizeof(BITMAP));	
				}	
			}
		}
		FileInfo_.clear();
		std::map<int, FileInfo> ().swap(FileInfo_);
	}
	ivplayback_thumb_->RemoveAllIconItem();
}

#endif

int PlaybackWindow::ChangePlayStatusPlaylist()
{
	db_error("ChangePlayStatusPlaylist");
	keyPlayStopButtonProc();	// -> stop
	if(GetPlayBCHandle()->getButtonDialogShowFlag())
		GetPlayBCHandle()->BCDoHide();
	select_flag_ = false;
	
	ivplayback_thumb_->SetIconItem_selectall(0);
	SetFileInfoSelect(-1,0);
	select_status = 0;

	SetPlaybackStatusIconInvalidFlag(false);					
	ivplayback_thumb_->Refresh();
	db_error("habo---> from playing window to return the playback windown !!!");
    HideStatusbar();
    Screensaver::GetInstance()->pause(false);//start the screensaver timer
    listener_->sendmsg(this, PLAYBACK_PLAY_STOP, 0);
    listener_->sendmsg(this, MSG_PLAY_TO_PLAYBACK_WINDOW, 0);
    PlayingToPlaybackUpdateShowLeftRightIcon();
    ResetFlagStatus();
    #ifdef S_PLAYB_STATUSBAR
    playBackSatutarbar(true);
    #endif
	return 0;
}

void PlaybackWindow::UpdateSelectIndexAndTotalFiles(bool flag)
{
	WindowManager *win_mg = ::WindowManager::GetInstance();
	StatusBarWindow *s_win = reinterpret_cast<StatusBarWindow *>(win_mg->GetWindow(WINDOWID_STATUSBAR));
	s_win->UpdatePlaybackFileInfo(select_file_,playlist_.size(),flag);
	UpdatePlaybackFileInfo(select_file_,playlist_.size(),flag);
	HidePlaybackTime(flag);
}

