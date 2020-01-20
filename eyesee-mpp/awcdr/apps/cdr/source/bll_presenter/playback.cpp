/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file playback.h
 * @brief 回放presenter
 * @author id:826
 * @version v0.3
 * @date 2016-08-01
 */
#define NDEBUG 
#include "bll_presenter/playback.h"
#include "device_model/media/player/video_player.h"
#include "device_model/media/player/jpeg_player.h"
#include "device_model/system/event_manager.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/storage_manager.h"
#include "lua/lua_config_parser.h"
#include "window/window.h"
#include "window/user_msg.h"
#include "window/playback_window.h"
#include "window/playlist_window.h"
#include "window/status_bar_window.h"
#include "window/status_bar_bottom_window.h"//add by zhb
#include "window/preview_window.h"
#include "common/utils/utils.h"
#include "common/app_log.h"
#include "application.h"
#include "utils/utils.h"
#include "bll_presenter/audioCtrl.h"
#include "window/setting_window.h"
#include "window/prompt.h"
#include "window/promptBox.h"
#include "window/bulletCollection.h"
#include "device_model/dialog_status_manager.h"
#include "bll_presenter/screensaver.h"


#include <sstream>

#undef LOG_TAG
#define LOG_TAG "PlaybackPresenter"

using namespace EyeseeLinux;
using namespace std;

// #define DESTRUCT_PLAYER_WHEN_DETACH
#define DESTRUCT_PLAYER_EVERYTIME
// #define DO_NOT_DESTRUCT_PLAYER


PlaybackPresenter::PlaybackPresenter()
    : playback_win_(NULL)
    , playlist_win_(NULL)
    , player_(NULL)
    , jpeg_player_(NULL)
    , playlist_idx_(0)
{
	mTimer_Start_ = 0;
	prevnextphotoflag = 0;
    status_ = MODEL_UNINIT;
    pthread_mutex_init(&model_lock_, NULL);
    win_mg_ = ::WindowManager::GetInstance();

    playlist_.clear();
    demolist_.clear();

    string demo_filelist = "/home/playlist.lua";

    if (FILE_EXIST(demo_filelist.c_str())) {
        LuaConfig config;

        config.LoadFromFile(demo_filelist);

        for (int i = 1; i <= config.GetIntegerValue("#filelist"); i++) {
            stringstream ss;
            ss << "filelist[" << i << "]";
            string file = config.GetStringValue(ss.str());
            db_info("demo file: %s", file.c_str());
            demolist_.push_back(file);
        }
    }

#ifdef DO_NOT_DESTRUCT_PLAYER
    if (player_ == NULL) {
        player_ = new VideoPlayer();
        player_->SetLoopingMode(false)
        player_->Attach(this);
    }
    if(jpeg_player_ == NULL) {
        jpeg_player_ = new JpegPlayer();
        jpeg_player_->SetDisplay(HLAY(0, 0));
        jpeg_player_->Attach(this);
    }
#endif


   //add by zhb
    this->Attach(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
    this->Attach(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    // FIXME: 在这里调用GetWindow来初始化recplay_win_，这可能会导致window无法绑定到presenter
}

PlaybackPresenter::~PlaybackPresenter()
{
#ifdef DO_NOT_DESTRUCT_PLAYER
    if (player_ != NULL) {
        player_->Detach(this);
        delete player_;
        player_ = NULL;
    }
    if (jpeg_player_ != NULL) {
        jpeg_player_->Detach(this);
        delete jpeg_player_;
        jpeg_player_ = NULL;
    }
#endif
}


void PlaybackPresenter::OnWindowLoaded()
{
    db_error("PlaybackPresenter window load");
    std::string type;
    string result_alias_bak;
    StringVector::iterator it;
    playback_win_ = reinterpret_cast<PlaybackWindow *>(win_mg_->GetWindow(WINDOWID_PLAYBACK));
	playback_win_->m_playback_thumb_bk->Hide();
	StatusBarWindow* sbw = reinterpret_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	//sbw->Hide();
	//::MoveWindow(sbw->GetHandle(), 0, 60, 640, 60, false);
	sbw->WinstatusIconHander(-1);
   // playlist_win_ = playback_win_->GetPlaylistWindow();
 
    playlist_.clear();
    playlist_idx_ = 0;
    playback_win_->ResetPlayList();
    if (status_ != MODEL_INITED) {
        this->DeviceModelInit();
    }

	   db_msg("SetLayerAlpha LAYER_UI 150");
       Layer::GetInstance()->SetLayerAlpha(LAYER_UI, 150);
	#ifdef S_FAKE_BG
      	playback_win_->fakeBg(true);
	#endif
	#ifdef S_PLAYB_STATUSBAR
	playback_win_->UpdatePlaybackFileInfo(GetCurrentSelectIndex(),GetPlaylistTotal());
	playback_win_->playBackSatutarbar(true);
	#endif
	int ret = StorageManager::GetInstance()->GetStorageStatus();
    if( (ret != UMOUNT) && (ret != STORAGE_FS_ERROR) && (ret != FORMATTING)){	
	    // get playlist from database
		int sum = MediaFileManager::GetInstance()->GetMediaFileCnt("");//""
	    MediaFileManager::GetInstance()->GetMediaFileList(playlist_, 0, sum, false, "");
		//db_error("playlist_ size:%d",playlist_.size());
		playback_win_->UpdateFileList(playlist_, sum);
		playback_win_->HideVideoPlayImgAndIcon();
		#ifndef UPDATEIMGUSETHREAD
		playback_win_->UpdateListView(UPDATEVIEW_LOAD);
		#else
		playback_win_->UpdateListView_threadstart();
		playback_win_->UpdateListView(UPDATEVIEW_LOAD);
		#endif
		bkplaylist_ = playlist_.begin(); 
		if(sum == 0){
			db_msg("[debug_zhb]--->file is 0 ");
			playback_win_->CtrlNoFileOrSdcard(true,true);
			this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
			}else{
				playback_win_->CtrlNoFileOrSdcard(false,false);
	            playback_win_->OnLoadWindowUpdateLeftRightIcon(sum);
				}
		
   	}else{
		db_msg("MSG_NO_DETECT_SDCARD ===============");
		playback_win_->CtrlNoFileOrSdcard(true,false);
		this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
   	}
    lock_guard<mutex> lock(msg_mutex_);
    ignore_msg_ = false;
}

void PlaybackPresenter::OnWindowDetached()
{
    ignore_msg_ = true;
    db_error("PlaybackPresenter window detach");
	#ifdef USEICONTHUMB
	playback_win_ = reinterpret_cast<PlaybackWindow *>(win_mg_->GetWindow(WINDOWID_PLAYBACK));
	if (playback_win_->m_UpdateIconviewImg_thread_id > 0) {
        pthread_cancel(playback_win_->m_UpdateIconviewImg_thread_id);
		playback_win_->m_UpdateIconviewImg_thread_id = 0;
	}
	playback_win_->FreeFileInfo();
	#endif
    lock_guard<mutex> lock(msg_mutex_);
    
    if (status_ == MODEL_INITED) {
        this->DeviceModelDeInit();
    }
	playlist_.clear();
    std::vector<std::string> ().swap(playlist_);
	//StatusBarWindow* sbw = reinterpret_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	//sbw->WinstatusIconHander(STATU_PREVIEW);
	//::MoveWindow(sbw->GetHandle(), 0, 60, 640, 300, true);
}

int PlaybackPresenter::HandleGUIMessage(int msg, int val, int id)
{
    int ret = 0;
    switch(msg) {
        case PLAYBACK_SETTING_BUTTON:
            db_msg("setting button pushed");
            this->SettingButtonHandler(0, true);
            break;
        case PLAYBACK_PLAY_SEEK:
            db_msg("play seek, seek direction: %d", val);
            ret = this->PlaySeekHandler(val);
            break;
        case PLAYBACK_PLAY_PAUSE:
            {
                std::string type;
		        db_error("by hero *** PLAYBACK_PLAY_PAUSE 111\n");
                if(playlist_.empty()){
					db_error("playlist is empty or is the last/first video,should break");
                    break;
                }
				db_error("file: %s\n", bkplaylist_->c_str());
                type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
                if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
                        || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
                    if(val){
                        //playback_win_->HideVideoPlayImgAndIcon(false);
                    }
                    ret = this->PlayPauseHandler(val);
                }else if (type == "photo_B" || type == "photo_A") {
                    if (val) {	// 
                    	if (prevnextphotoflag){
							prevnextphotoflag = 0;
							break;
                    	}
                        playback_win_->HideVideoPlayImg();
                        playback_win_->HideVideoPlayImgAndIcon();
                        playback_win_->HideWindowBackImg();
                        playback_win_->HideStatusbar();
                        playback_win_->HidePlaybackTime(true);

                        if (player_) {
                            player_->Reset();
                        }
						
                        if(jpeg_player_ == NULL)
                            CreatJpegHandler();
                        db_warn("habo--->  ready to show photo ");
						jpeg_player_->ReleasePic();
						jpeg_player_->Reset();
                        int ret = jpeg_player_->PrepareFile(bkplaylist_->c_str(),type);
						db_warn("jpeg PrepareFile ret: %d",ret);
                        if(ret == 0){
                            jpeg_player_->ShowPic();
                            playback_win_->SetPlayerFinish();
                        } else {
							jpeg_player_->ReleasePic();
							jpeg_player_->Reset();
							jpeg_player_->PrepareFile("/usr/share/minigui/res/images/black.jpg","photo_A");
							jpeg_player_->ShowPic();
		                    //playback_win_->SetPlayerFinish();
							playback_win_->AbnormalPhotoPlayReSetStatus();	// 显示图片损坏删除窗口
                        }
                    }
                }
            }
            break;
        case PLAYBACK_PLAY_STOP:
            {
                //playback_win_->SetThumbShowCtrl(result_alias.c_str());
//				playback_win_->HidePlaybackTime();
                if(!playlist_.empty())
	        	{
	        		std::string type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
		            if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
		                    || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
		     		db_msg("[debug_zhb]------PLAYBACK_PLAY_STOP");
	    	        PlayStopHandler();
	        		}
                }
                if(jpeg_player_){
                    jpeg_player_->ReleasePic();
                    jpeg_player_->Reset();
                }

			playback_win_->HideVideoPlayImgAndIcon();
			#ifdef USEICONTHUMB
            playback_win_->UpdateListView(UPDATEVIEW_NOLOAD);	
			#else
			playback_win_->UpdateListView(UPDATEVIEW_LOAD);
			#endif
			mTimer_Start_ = 0;	
			int val = playback_win_->GetPlaybackPageStatus();
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
            }
            break;
	case PLAYBACK_PLAYING_STOP:
		{
			if(IsLastOrFirstVideo(val)){
				db_error("---this is the last or first video");
				//playback_win_->SetPlayerStatus(PlaybackWindow::PLAYING);
				playback_win_->SetIsLastFirstVideoStatus(true);
				break;
				}
				
				playback_win_->SetIsLastFirstVideoStatus(false);
				playback_win_->SetPlayerStatus(PlaybackWindow::STOPED);
			std::string type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
			if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
			        || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
			db_msg("[debug_zhb]------PLAYBACK_PLAYING_STOP");
			PlayStopHandler();
			}
			mTimer_Start_ = 0;
			usleep(50*1000);
		}
		break;
        case PLAYBACK_VOICE_CONTROL:
            this->VoiceControlHandler(val);
            break;
        case PLAYLIST_CONFIRM_BUTTON:
            // TODO: play curent file
            break;
	case PLAYBACK_FILELIST:
		{
			int sum;
			std::string type;
			playlist_.clear();

			switch(val){
				case VIDEO_A:
					type = "video_A";
					break;
				case VIDEO_B:
					type = "video_B";
					break;
				case PHOTO_A:
					type = "photo_A";
					break;
				case PHOTO_B:
					type = "photo_B";
				case VIDEO_A_SOS:
					type = "video_A_SOS";
					break;
				case VIDEO_B_SOS:
					type = "videoB_SOS";
					break;
                case VIDEO_A_PARK:
					type = "videoA_PARK";
					break;
				case VIDEO_B_PARK:
					type = "videoB_PARK";
					break;
				default:
					type = "";
					break;
			}
			sum = MediaFileManager::GetInstance()->GetMediaFileCnt(type.c_str());//""
			MediaFileManager::GetInstance()->GetMediaFileList(playlist_, 0, sum, false, type.c_str());
			playback_win_->UpdateFileList(playlist_, sum);
		}
		break;
        case PLAYLIST_FILLLIST: { // should fill listview item, before show playlist window
            db_msg("PLAYLIST_FILLLIST");

            // update playlist everytime, so need clean last playlist
            playlist_.clear();

            // get playlist from database
            MediaFileManager::GetInstance()->GetMediaFileList(playlist_, 0, 15, false, "video");

            playlist_win_->AddFileToList(playlist_);
        }
            break;
		case PLAYBACK_UPDATE_TIMER:
			{
//				mTimer_Start_++;
				if(player_ == NULL){
					db_warn("player_ is null,break");
					break;
				}
			    int cur_pos = player_->getCurrentPosition();
				mTimer_Start_ = cur_pos/1000;
				if((++mTimer_Start_) > val)
				    mTimer_Start_ = val;
				playback_win_->UpdatePlaybackTime(mTimer_Start_, val);
			}
			break;
		case PLAYBACK_COMPLETION_TIMER:
			{
		              playback_win_->SetPlayProgress(val*1000);
				//playback_win_->UpdatePlaybackTime(val, val);				
			}
			break;
        case PLAYLIST_CONFIRM_PLAYFILE:
            if( player_)
                player_->Start();
            break;
		case PLAYBACK_SET_SHOW_THS:{
				bkplaylist_ = playlist_.begin() + val;
				playlist_idx_ = val;
					
				if(!playlist_.empty())
					UpdateFileTime(bkplaylist_->c_str());
				 UpdateSelectIndexAndTotalFiles(false);
			}
			break;
		case PLAYBACK_SHOW_FILEOSD:{
                if(playlist_.empty()){
                    break;
                }			
				bkplaylist_ = playlist_.begin() + val;
				playlist_idx_ = val;
				UpdateSelectIndexAndTotalFiles();
			}
			break;
        case PLAYBACK_SHOW_NEXT_THS:{
            std::string type;
            string result_alias_bak;
            db_error("[debug_jason]:the PLAYBACK_SHOW_NEXT_THS");
            if(playlist_.empty()){
                db_msg("[debug_jaosn]:no media file");
                break;
            }
            bkplaylist_ ++;
	        playlist_idx_++;
            if(bkplaylist_ == playlist_.end()){
                db_msg("[debug_jaosn]:media file point to the end");
				playback_win_->SetIsLastFirstVideoStatus(true);
				bkplaylist_ --;
				playlist_idx_--;
				PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
				pre_win->ShowPromptBox(PROMPT_BOX_LAST_VIDEO);
				break;
            }
		  	if(!playlist_.empty())
		  		UpdateFileTime(bkplaylist_->c_str());
	    	playback_win_->SetIsLastFirstVideoStatus(false);
	        if(jpeg_player_ == NULL) {
	            CreatJpegHandler();
	        }
            if(jpeg_player_){
                jpeg_player_->ReleasePic();
                jpeg_player_->Reset();
            }
            type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
			db_error("MediaFileType: %d %s",type,bkplaylist_->c_str());
            if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
                    || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
                 result_alias_bak = bkplaylist_->c_str();
				 string::size_type rc = result_alias_bak.rfind(".");
				 if( rc == string::npos)
				 {
				 	db_warn("invalid fileName:%s",result_alias_bak.c_str());
					break;
				 }
                 result_alias= result_alias_bak.substr(0,rc);
                 result_alias += "_ths.jpg";
                 db_msg("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
                 //playback_win_->SetThumbShowCtrl(result_alias.c_str());
                 prevnextphotoflag = 0;
            }else{
                 
                 playback_win_->HideVideoPlayImg();
                 playback_win_->HideVideoPlayImgAndIcon();
                 playback_win_->HideWindowBackImg();
                 playback_win_->HideStatusbar();
                 playback_win_->HidePlaybackTime(true);
                 result_alias_bak = bkplaylist_->c_str();
				 
				 string::size_type rc = result_alias_bak.rfind(".");
				 if( rc == string::npos)
				 {
				 	db_warn("invalid fileName:%s",result_alias_bak.c_str());
					break;
				 }

                 result_alias= result_alias_bak.substr(0,rc);
                 result_alias += ".jpg";
                 db_error("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
                 //playback_win_->SetThumbShowCtrl(result_alias.c_str());
                 if (player_)
                    player_->Reset();
				 jpeg_player_->ReleasePic();
 				 jpeg_player_->Reset();
                 int ret = jpeg_player_->PrepareFile(result_alias.c_str(),type);
                 if(ret == 0){
                    jpeg_player_->ShowPic();
                    playback_win_->SetPlayerFinish();
                 } else {
				 	jpeg_player_->ReleasePic();
					jpeg_player_->Reset();
				 	jpeg_player_->PrepareFile("/usr/share/minigui/res/images/black.jpg","photo_A");
					jpeg_player_->ShowPic();
                    //playback_win_->SetPlayerFinish();
					playback_win_->AbnormalPhotoPlayReSetStatus();	// 显示图片损坏删除窗口
                 }
				 prevnextphotoflag = 1;
            }
            UpdateSelectIndexAndTotalFiles(false);
            break;
            }
        case PLAYBACK_SHOW_PRVE_THS:
            {
            std::string type;
            string result_alias_bak;
            if(playlist_.empty())
             {
                db_msg("[debug_jaosn]:no media file");
                break;
             }
		    db_msg("[debug_zhb]---> playlist_idx_ =%d",playlist_idx_);
            if(bkplaylist_-- == playlist_.begin())
            {
                //db_msg("*****this is the first video  ready to show promptbox  this is the first video***********");
                playback_win_->SetIsLastFirstVideoStatus(true);
                bkplaylist_++;
                PreviewWindow * pre_win = reinterpret_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
                pre_win->ShowPromptBox(PROMPT_BOX_FIRST_VIDEO);
                break;
            }
            else
            {
                db_msg("*****this is not  the first video***********");
    			playlist_idx_--;
    			playback_win_->SetIsLastFirstVideoStatus(false);
            }
		    if(!playlist_.empty())
	  		    UpdateFileTime(bkplaylist_->c_str());
		    if(jpeg_player_ == NULL) {
	             CreatJpegHandler();
		    }
	        if(jpeg_player_){
	             jpeg_player_->ReleasePic();
	             jpeg_player_->Reset();
	        }
            type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
            if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
                    || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
                 //playback_win_->ShowVideoPlayImg();
                 result_alias_bak = bkplaylist_->c_str();
				 string::size_type rc = result_alias_bak.rfind(".");
				 if( rc == string::npos)
				 {
				 	db_warn("invalid fileName:%s",result_alias_bak.c_str());
					break;
				 }
                 result_alias= result_alias_bak.substr(0,rc);
                 result_alias += "_ths.jpg";
                 db_msg("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
                 //playback_win_->SetThumbShowCtrl(result_alias.c_str());
                 prevnextphotoflag = 0;
            }else{
                 playback_win_->HideVideoPlayImg();
                 playback_win_->HideVideoPlayImgAndIcon();
                 playback_win_->HideWindowBackImg();
                 playback_win_->HideStatusbar();
                 playback_win_->HidePlaybackTime(true);
                 result_alias_bak = bkplaylist_->c_str();
				 string::size_type rc = result_alias_bak.rfind(".");
				 if( rc == string::npos)
				 {
				 	db_warn("invalid fileName:%s",result_alias_bak.c_str());
					break;
				 }
                 result_alias= result_alias_bak.substr(0,rc);
                 result_alias += ".jpg";
                 db_msg("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
                 //playback_win_->SetThumbShowCtrl(result_alias.c_str());
                 if (player_)
                    player_->Reset();
				  
                 int ret = jpeg_player_->PrepareFile(result_alias.c_str(),type);
                 if(ret == 0){
                   jpeg_player_->ShowPic();
                   playback_win_->SetPlayerFinish();
                 } else {
				 	jpeg_player_->ReleasePic();
					jpeg_player_->Reset();
					jpeg_player_->PrepareFile("/usr/share/minigui/res/images/black.jpg","photo_A");
					jpeg_player_->ShowPic();
                    //playback_win_->SetPlayerFinish();
					playback_win_->AbnormalPhotoPlayReSetStatus();	// 显示图片损坏删除窗口
                }
				prevnextphotoflag = 1;
            }
           UpdateSelectIndexAndTotalFiles(false);
            break;
            }
	case MSG_CHANG_STATU_PLAYBACK_TO_PREVIEW:
        	//UpdateStatusBarUI();
		this->Notify((MSG_TYPE)MSG_PLAYBACK_TO_PREIVEW_CHANG_STATUS_BAR_BOTTOM);
		break;
	case MSG_CHANG_STATU_PLAYBACK_DELETE_PLAY:
		this->Notify((MSG_TYPE)MSG_PLAYBACK_DELETE_PLAY_BUTTON,val);
		break;
	case MSG_CHANG_STATU_PLYABACK_PLAY_DELETE_SH:
		this->Notify((MSG_TYPE)MSG_PLAYBACK_DELETE_PLAY_IMAG_SH,val);
		break;
	case MSG_CHANG_STATU_PLAYBACK_NO_FILE_SDCARD:
		this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,val);
		break;
	case MSG_PLAYBACK_DELETE_FILE:
		switch(playback_win_->GetPlayBCHandle()->getButtonDialogCurrentId()){
			case BC_BUTTON_DIALOG_DELETE_VIDEO:
			case BC_BUTTON_DIALOG_DELETE_PHOTO:
				{
					db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DELETE_PHOTO/BC_BUTTON_DIALOG_DELETE_VIDEO");
					if(val == 1 ){ //按确认键会重新返回相册界面,无须更新底部状态icon
						DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
						if(playback_win_->IsPlayingWindow()){
							DoDeleteFileHander();
						}else{
							DoDeleteSelectFileHander();
						}
						DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
						Screensaver::GetInstance()->pause(false);//start screensaver timer
					} else {
						this->Notify((MSG_TYPE)MSG_PLAYBACK_SHOW_HIGHLIGHT_ICON,2);
					}
					db_warn("show delete file dialog end,set flag false");
					playback_win_->SetPlaybackStatusIconInvalidFlag(false);
//					else
//						ShowVideoPlayImg();
					#ifdef USEICONTHUMB
					playback_win_->select_status = 0;
					#endif
				}break;
			case BC_BUTTON_DIALOG_DELETE_ERROR_VIDEO:
				{
					db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DELETE_ERROR_VIDEO");
					if(val == 1){
						DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
						DeleteErrorFile();
						DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
					}
					db_warn("show delete error file dialog end,set flag false");
					playback_win_->SetPlaybackStatusIconInvalidFlag(false);
					int val = playback_win_->GetPlaybackPageStatus();
					if(playback_win_->GetPlaybackStatusIconInvalidFlag() == false){
						this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
						db_warn("Delete file end,update status bar bottom");
					}
					StatusBarBottomWindow *s_win = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
					s_win->PlaybackWindownButtonStatus(true,false,true);
					#ifdef USEICONTHUMB
					playback_win_->select_status = 0;
					#endif
				}
				break;
			case BC_BUTTON_DIALOG_DELETE_ERROR_PHOTO:
				{
					db_msg("[debug_zhb]------BC_BUTTON_DIALOG_DELETE_ERROR_PHOTO");
					if(val == 1){
						DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
						DeleteErrorFile();
						DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
					}
					db_warn("show delete error file dialog end,set flag false");
					playback_win_->SetPlaybackStatusIconInvalidFlag(false);
					int val = playback_win_->GetPlaybackPageStatus();
					if(playback_win_->GetPlaybackStatusIconInvalidFlag() == false){
						this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
						db_warn("Delete file end,update status bar bottom");
					}
					StatusBarBottomWindow *s_win = reinterpret_cast<StatusBarBottomWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR_BOTTOM));
					s_win->PlaybackWindownButtonStatus(true,false,true);
					#ifdef USEICONTHUMB
					playback_win_->select_status = 0;
					#endif
				}
				break;
			#ifdef USEICONTHUMB
			case BC_BUTTON_DIALOG_DELETE_ALLSELECTED:
				{
					db_error("[debug_zhb]------BC_BUTTON_DIALOG_DELETE_PHOTO/BC_BUTTON_DIALOG_DELETE_VIDEO");
					if(val == 1 ){ //按确认键会重新返回相册界面,无须更新底部状态icon
						DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
			        	DoDeleteSelectFileHander();
						DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
						Screensaver::GetInstance()->pause(false);//start screensaver timer
					} else {
						this->Notify((MSG_TYPE)MSG_PLAYBACK_SHOW_HIGHLIGHT_ICON,2);
					}

					playback_win_->select_status = 0;
					playback_win_->SetPlaybackStatusIconInvalidFlag(false);
					db_warn("show delete select file dialog end,set flag false");
				}break;
			#endif
			default:
				if(val == 1){
					db_msg("[debug_zhb]--->default p_camid = 1");
					}else{
						db_msg("[debug_zhb]--->default p_camid = 0");

						}
				break;
			}
		playback_win_->GetPlayBCHandle()->setButtonDialogShowFlag(false);	
		break;
	case MSG_PLAYBACK_SHOW_DELETE_DIALOG:
        	if(!playlist_.empty())
				ShowDeleteDialog(val);
		break;
	case MSG_PLAYBACK_TO_PLAY_WINDOW:
		this->Notify((MSG_TYPE)msg);
		break;
	case MSG_PLAY_TO_PLAYBACK_WINDOW:
		this->Notify((MSG_TYPE)msg);
		break;
    case MSG_PLAYBACK_PAGE_UP_DOWN_ICON:
		this->Notify((MSG_TYPE)msg,val);
		break;
	case PLAYBACK_DELETEFILE_ICONGRAY:
		this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,4);
		break;
	case MSG_PLAYBACK_SHOW_GRAY_ICON:
		this->Notify((MSG_TYPE)MSG_PLAYBACK_SHOW_GRAY_ICON);
		break;
    case PLAYBACK_CHANGE_FILE_NAME:
        {
            playback_win_->UpdateListFileName(playlist_,val);
        }
        break;
	case PLAYBACK_HIDETHUMB:
			db_error("Hide ivplayback_thumb_");
			playback_win_->ivplayback_thumb_->Hide();
		playback_win_->Refresh();
				break;
        default:
#if 0
            if (msg > PLAYLIST_BASE) { // make player prepared when user choose one file
                playlist_idx_ = msg-PLAYLIST_BASE-1;
                SetFileAndPrepare(playlist_[playlist_idx_++]);
				UpdateSelectIndexAndTotalFiles();
            } else
                db_msg("there is no this button defined");
#endif
            break;
    }

    return ret;
}

bool PlaybackPresenter::IsLastOrFirstVideo(int pre)
{
   if(pre == 1){
   	db_msg("debug_zhb---11-pre = %d----",pre);
	if(bkplaylist_== playlist_.begin())
		  return true;
    }else{
    	db_msg("debug_zhb----pre = %d----",pre);
	 if((++bkplaylist_) == playlist_.end()){
		bkplaylist_ --;
		db_msg("debug_zhb----ready to return true");
		return true;
	 	}
	 bkplaylist_ --;
   	}
	return false;
}

void PlaybackPresenter::BindGUIWindow(::Window *win)
{
    this->Attach(win);
}

int PlaybackPresenter::DeviceModelInit()
{
    db_msg("rec and play device model init");
	MediaFileManager *mm = MediaFileManager::GetInstance();
     mm->Attach(this);
	StorageManager *sm = StorageManager::GetInstance();
     sm->Attach(this);
#ifdef DESTRUCT_PLAYER_WHEN_DETACH
    if (player_ == NULL) {
        player_ = new VideoPlayer();
        player_->SetLoopingMode(false);
        player_->Attach(this);
    }
    if(jpeg_player_ == NULL) {
       jpeg_player_ = new JpegPlayer();
       jpeg_player_->SetDisplay(HLAY(0, 0));
       jpeg_player_->Attach(this);
    }
#endif

    Layer::GetInstance()->Attach(this);

    status_ = MODEL_INITED;

    return 0;
}

int PlaybackPresenter::DeviceModelDeInit()
{
	 MediaFileManager *mm = MediaFileManager::GetInstance();
     mm->Detach(this);
	 StorageManager *sm = StorageManager::GetInstance();
     sm->Detach(this);

     Layer::GetInstance()->Detach(this);
#ifdef DESTRUCT_PLAYER_EVERYTIME
    if (player_ != NULL) {
        player_->Reset();
    }
    if (jpeg_player_ != NULL) {
        jpeg_player_->Reset();
        jpeg_player_->Detach(this);
        delete jpeg_player_;
        jpeg_player_ = NULL;
    }
#endif

#ifdef DESTRUCT_PLAYER_WHEN_DETACH
    if( player_)
    {
        player_->Stop();
        player_->Detach(this);
        //sleep(1);
        delete player_;
        player_ = NULL;
    }
    if (jpeg_player_ != NULL) {
        jpeg_player_->Detach(this);
        delete jpeg_player_;
        jpeg_player_ = NULL;
    }

#endif

    status_ = MODEL_UNINIT;

    return 0;
}

// 底层通知回调
void PlaybackPresenter::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
    if (msg_mutex_.try_lock() == false) {
        db_warn("maybe presenter is detaching, ignore this msg");
        return;
    }
    if (ignore_msg_) {
        db_warn("presenter has been detached, do not response msg");
        msg_mutex_.unlock();
        return;
    }
    msg_mutex_.unlock();

    db_msg("msg: %d", msg);

    switch (msg) {
        case MSG_VIDEO_PLAY_PREPARED: { // not ok temporary
            int duration = 0;
            if( player_)
            {
                player_->GetCurrentFileDuration(duration);
            }
            playback_win_->SetPlayDuration(duration);
            break;
        }
        case MSG_VIDEO_PLAY_COMPLETION: {
            this->Notify(msg); // 转发消息给UI
            break;
        }
        case MSG_VIDEO_PLAY_PAUSE:
            this->Notify(msg); // 转发消息给UI
            break;
        case MSG_VIDEO_PLAY_START:
        case MSG_PIC_PLAY_START:
            this->Notify(msg);
            break;
        case MSG_VIDEO_PLAY_STOP:
            this->Notify(msg);
            break;
        case MSG_SHOW_HDMI_MASK:
        case MSG_HIDE_HDMI_MASK:
            Notify(msg);
            break;
        case MSG_HDMI_PLUGIN:
        case MSG_TVOUT_PLUG_IN:
            {
                db_debug("hdmi plugin");
                if( player_)
                {
                    player_->Stop();
                    player_->SwitchAOCard(PCM_CARD_TYPE_SNDHDMI);
                    AudioCtrl::GetInstance()->SwitchAOCard(1);
                }
                if (!result_alias.empty() && access(result_alias.c_str(), F_OK) == 0) {
                    playback_win_->ShowThumbImg();
                }
                jpeg_player_->ReleasePic();
                jpeg_player_->Reset();
                break;
            }
        case MSG_HDMI_PLUGOUT:
        case MSG_TVOUT_PLUG_OUT:
            {
                db_debug("hdmi plugout");
                if( player_)
                {
                    player_->Stop();
                    player_->SwitchAOCard(PCM_CARD_TYPE_AUDIOCODEC);
                    AudioCtrl::GetInstance()->SwitchAOCard(0);
                }
                if (!result_alias.empty() && access(result_alias.c_str(), F_OK) == 0) {
                    playback_win_->ShowThumbImg();
                }
                jpeg_player_->ReleasePic();
                jpeg_player_->Reset();
            }
            break;
		case MSG_STORAGE_UMOUNT:
            db_error("MSG_STORAGE_UMOUNT");
            if (ignore_msg_ == true) {
                db_warn("MSG_STORAGE_UMOUNT, ignore_msg_ is true");
                break;
            }
			//if delete file dialog has been showed,shuold close
			if(playback_win_->GetPlayBCHandle()->getButtonDialogShowFlag())
				playback_win_->GetPlayBCHandle()->BCDoHide();

		     if(playback_win_->IsPlayingWindow())//if playing or pause window sdcard out
			 	playback_win_->AbnormalVideoPlayReSetStatus(false);
	            RefreshUnmount();
		     playback_win_->CtrlNoFileOrSdcard(true,false);
		     this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
			break;
		case MSG_STORAGE_MOUNTED:
			break;
		case MSG_DATABASE_UPDATE_FINISHED:
            if (ignore_msg_ == true) {
                db_warn("MSG_DATABASE_UPDATE_FINISHED, ignore_msg_ is true");
                break;
            }
            db_error("MSG_DATABASE_UPDATE_FINISHED, start run RefreshMounted");
	        RefreshMounted();
			break;
		case MSG_DELETE_VIDEOFILE:
		case MSG_DELETE_PHOTOFILE:
			{
				playlist_.clear();
				// update playlist everytime, so need clean last playlist
				int sum = MediaFileManager::GetInstance()->GetMediaFileCnt("");//""
				//delete file is playing
				if((playback_win_->getCurrentFileIndex() >= sum - 1) && player_){
					PlayStopHandler();
					playback_win_->AbnormalVideoPlayReSetStatus(false);
				}
				
       			MediaFileManager::GetInstance()->GetMediaFileList(playlist_, 0, sum, false, "");
				playback_win_->UpdateFileList(playlist_, sum,false);
				int selectfile = playback_win_->getCurrentFileIndex();
				#ifndef  USEICONTHUMB
				if(sum - selectfile <= IMAGEVIEW_NUM){
					if(!player_)
					{
						playback_win_->ResetFlagStatus();
						playback_win_->UpdateListView(UPDATEVIEW_LOAD);
					}
				}
				#endif
			}
			break;
		case MSG_TO_PREVIEW_WINDOW:
		{
		 	playback_win_->DoClearOldOperation();
	    	}
		return ;
        default:
            break;
    }

    //this->Notify(msg);
}

int PlaybackPresenter::SetFileAndPrepare(const std::string &file)
{
    int ret;
    int duration = 0;

    if (player_ == NULL) return -1;

    player_->Reset();

    db_info("prepare to play file: %s", file.c_str());
    ret = player_->PreparePlay(file.c_str());

    if (ret < 0) return ret;

    player_->GetCurrentFileDuration(duration);
	if( duration < 1*1000 )
		return -1;
    if (playback_win_ == NULL) return -1;

    playback_win_->ResetPlayProgress();
    playback_win_->SetPlayDuration(duration);

    return ret;
}

void PlaybackPresenter::SettingButtonHandler(uint8_t id, bool value)
{
}

int PlaybackPresenter::PlaySeekHandler(int dir)
{
    int ret = 0;
    int step = 2 * 1000;
    // 用于设置前后不响应seek操作的时间
    int buffer = 5 * 1000;
    int new_pos = 0;
    if(NULL == player_)
    {
        db_warn("player is null");
        return ret;
    }
    int total = player_->getDuration();
    int cur_pos = player_->getCurrentPosition();

    if (dir == 0) {
        if (cur_pos <= buffer) {
            return 0;
        } else {
            new_pos = cur_pos - step;
            if (new_pos <= (step + buffer))
                new_pos = 0;
        }
    } else {
        if (cur_pos >= total - (step + buffer)) {
            return 0;
        } else {
            new_pos = cur_pos + step;
        }
    }

    db_debug("cur_pos: %dmsec, new_pos: %dmsec", cur_pos, new_pos);

    ret = player_->Seek(new_pos);
    playback_win_->SetPlayProgress(new_pos);
	mTimer_Start_ = new_pos/1000;
    return ret;
}

int PlaybackPresenter::PlayPauseHandler(bool value)
{
    int ret = 0;
    db_msg("value: %d", value);

#ifdef DESTRUCT_PLAYER_EVERYTIME
    if (player_ == NULL) {
        player_ = new VideoPlayer();
        player_->SetLoopingMode(false);
        player_->Attach(this);
    }
    if (value) {
        PlayerState stat = player_->GetStatus();
        if (stat == PLAYER_IDLE || stat == PLAYER_PLAY_COMPLETED) {
        db_warn("debug_zhb---> filename = %s",bkplaylist_->c_str());
            ret = SetFileAndPrepare(bkplaylist_->c_str());
            if (ret < 0) {		// 视频播放错误
                db_error("set file prepare failed");
                playback_win_->AbnormalVideoPlayReSetStatus();	// 显示视频损坏删除窗口
                return ret;
            }
        }
        //show playback statubar start
        playback_win_->ShowStatusbar();
        //show playback statubar end

        int cur_pos = player_->getCurrentPosition();
        playback_win_->SetPlayProgress(cur_pos);
        mTimer_Start_ = cur_pos/1000;
        int duration = 0;
        if( player_)
        {
            player_->GetCurrentFileDuration(duration);
        }

        ret = player_->Start();
        playback_win_->UpdatePlaybackTime(mTimer_Start_, duration/1000);
    } else {
        ret = player_->Pause();
    }
#endif

    return ret;
}

void PlaybackPresenter::CreatJpegHandler()
{
    if(jpeg_player_ == NULL) {
        jpeg_player_ = new JpegPlayer();
        jpeg_player_->SetDisplay(HLAY(0, 0));
        jpeg_player_->Attach(this);
    }
}

int PlaybackPresenter::PlayStopHandler()
{
    int ret = 0;
    db_msg("PlayStopHandler");
#ifdef DESTRUCT_PLAYER_EVERYTIME
    if (player_ != NULL) {
        ret = player_->Stop();
        player_->Detach(this);
        delete player_;
        player_ = NULL;
    }
#endif

    return ret;
}

void PlaybackPresenter::VoiceControlHandler(int level)
{
    int percent = level * 25;

    db_warn("level: %d, %d%%", level, percent);
    if( player_)
    {
        player_->SetVolume(percent);
    }
}


int PlaybackPresenter::GetCurrentSelectIndex(void)

{
	return playlist_idx_;
}

int PlaybackPresenter::GetPlaylistTotal(void)

{
	return playlist_.size();
}


void PlaybackPresenter::UpdateSelectIndexAndTotalFiles(bool flag)
{
	if(win_mg_->GetCurrentWinID() != WINDOWID_PLAYBACK){
        db_msg(" GetCurrentWinID = %d",win_mg_->GetCurrentWinID());
		return;
	}
	StatusBarWindow *s_win = reinterpret_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	s_win->UpdatePlaybackFileInfo(playback_win_->select_file_,GetPlaylistTotal(),flag);
	playback_win_->UpdatePlaybackFileInfo(playback_win_->select_file_,GetPlaylistTotal(),flag);
	playback_win_->HidePlaybackTime(flag);
}
void PlaybackPresenter::UpdateFileTime(const std::string &filename)
{
	db_warn("[habo]---> UpdateFileTime\n");
  #if 0 //habo
	if(win_mg_->GetCurrentWinID() != WINDOWID_PLAYBACK){
        db_msg(" GetCurrentWinID = %d",win_mg_->GetCurrentWinID());
		return;
	}
#endif
	string time_str;
	playback_win_->GetVideoCreatTime(filename,time_str);
	StatusBarWindow *s_win = reinterpret_cast<StatusBarWindow *>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
	s_win->UpdatePlaybackFileTime(time_str,true);
}


void PlaybackPresenter::RefreshMounted()
{
    db_warn("RefreshMounted===================");
   
        std::string type;
        string result_alias_bak;
        StringVector::iterator it;
    	if(playback_win_ == NULL)
    	{
            db_msg("get window error!");
    		return;
    	}
	playback_win_->setRefreshFlag(true);
	if(playback_win_->getIsPlayFlag())
   	{
		db_warn("no is playing window,should no to do refresh the playback ui");
		return;
   	}
        playlist_.clear();
        playlist_idx_ = 0;

        // get playlist from database
    	int sum = MediaFileManager::GetInstance()->GetMediaFileCnt("");
        MediaFileManager::GetInstance()->GetMediaFileList(playlist_, 0, sum, false, "");
        if(playlist_.empty()){
	     db_msg("[debug_zhb]---------no file");
            playback_win_->HideVideoPlayImgAndIcon();
	     playback_win_->CtrlNoFileOrSdcard(true,true);
	     this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
        }else{
        	int val = playback_win_->GetPlaybackPageStatus();
        		//hide the tips icon
        		playback_win_->CtrlNoFileOrSdcard(false,false);
			if(playback_win_->GetPlaybackStatusIconInvalidFlag() == false){
			this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
				db_warn("RefreshMounted update status bar bottom");
			}
			//update
			playback_win_->UpdateFileList(playlist_, sum);
			playback_win_->HideVideoPlayImgAndIcon();
			playback_win_->UpdateListView(UPDATEVIEW_LOAD);
			bkplaylist_ = playlist_.begin(); 			
            playback_win_->OnLoadWindowUpdateLeftRightIcon(sum);
        }
     	UpdateSelectIndexAndTotalFiles();
	playback_win_->setRefreshFlag(false);
}

void PlaybackPresenter::RefreshUnmount()
{
    db_warn("RefreshUnmount===================");
    std::string type;
    string result_alias_bak;
    StringVector::iterator it;
    playback_win_ = reinterpret_cast<PlaybackWindow *>(win_mg_->GetWindow(WINDOWID_PLAYBACK));

	if(playback_win_ == NULL)
	{
        db_msg("get window error!");
		return;
	}
    if( player_)
    {
        player_->Stop();
        player_->ResetLayer();
    } else if(jpeg_player_){
        db_msg("jepg reset...");
        jpeg_player_->ReleasePic();
        jpeg_player_->Reset();
    }
    playlist_.clear();
    playlist_idx_ = 0;
    mTimer_Start_ = 0;
    PreviewWindow *pre_win = reinterpret_cast<PreviewWindow *>(win_mg_->GetWindow(WINDOWID_PREVIEW));
    if( !pre_win->IsUsbAttach())
    {
        playback_win_->HideVideoPlayImgAndIcon();
		playback_win_->HideListView();
        UpdateSelectIndexAndTotalFiles();
    }
	
    // enable ui layer alpha to show preview, set 255 can disable it
   // Layer::GetInstance()->SetLayerAlpha(LAYER_UI, 150);
}

void PlaybackPresenter::UpdateStatusBarUI()
{
	db_msg("SetLayerAlpha LAYER_UI 255");
    Layer::GetInstance()->SetLayerAlpha(LAYER_UI, 255);
	StatusBarWindow *win = static_cast<StatusBarWindow*>(win_mg_->GetWindow(WINDOWID_STATUSBAR));
    win->SetWinStatus(0);
}
void PlaybackPresenter::DoDeleteFile2StopPlay()
{
	playback_win_->HidePlaybackTime();				  
	std::string type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
	if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
	        || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
        db_msg("[debug_zhb]------PLAYBACK_PLAY_STOP");
        PlayStopHandler();
	} else if(type == "photo_A" || type == "photo_B") {
	    if(jpeg_player_){
            jpeg_player_->ReleasePic();
            jpeg_player_->Reset();
	    }
	}
	mTimer_Start_ = 0;
	playback_win_->ResetFlagStatus();
//	playback_win_->UpdateVideoPlayImg();
	playback_win_->HideVideoPlayImg();
	playback_win_->HideStatusbar();
}

void PlaybackPresenter::DoDeleteFileHander()
{
	playback_win_->SetDeleteFileStatus(true);
	//stop the play 
	DoDeleteFile2StopPlay();
	
	string file_name;
	//get file name
    file_name = bkplaylist_->c_str();
	db_msg("delete file name:%s ",file_name.c_str());
	//delete from db
    int ret = MediaFileManager::GetInstance()->RemoveFile(file_name);
	if(ret < 0){
        db_msg("delete file error.");
		return;
	}


	//delete from list
	if(bkplaylist_ == playlist_.end()-1){
        playlist_.pop_back();
        bkplaylist_ = playlist_.end()-1;
		if(playlist_.size() > 0)
		    playlist_idx_ = playlist_.size()-1;
		else
			playlist_idx_ = 0;
		db_msg("[debug_zhanglm]:media file point to the end==playlist_idx_=%d=============",playlist_idx_);
    }else if(bkplaylist_ == playlist_.begin()){
        db_msg("[debug_zhanglm]:media file point to the begin================");
		bkplaylist_ = playlist_.erase(bkplaylist_);
        bkplaylist_ = playlist_.begin();
        playlist_idx_ = 0;
    }else{
        bkplaylist_ = playlist_.erase(bkplaylist_);
        playlist_idx_ = bkplaylist_ - playlist_.begin();
    }
	#ifdef USEICONTHUMB
	std::map<int, FileInfo>::iterator itr;
	itr = playback_win_->FileInfo_.find(playback_win_->select_file_);
	if (itr == playback_win_->FileInfo_.end()) {
		db_msg("delete file error.");
		return;
	}
	if ((itr->second).thumbbmp.bmBits) {
		//UnloadBitmap(&(itr->second).thumbbmp);	// 释放缩略图空间
	}
	#endif
	//update cur playback view
	playback_win_->UpdateFileList(playlist_, playlist_.size());
	playback_win_->HideVideoPlayImgAndIcon();
	playback_win_->UpdateListView(UPDATEVIEW_LOAD);
	UpdateSelectIndexAndTotalFiles();

	if(playlist_.size() == 0){
		     db_msg("[debug_zhb]----delete file and has no file ------");
		     playback_win_->CtrlNoFileOrSdcard(true,true);
		     this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
	}else{
		int val = playback_win_->GetPlaybackPageStatus();
		this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
		playback_win_->PlayingToPlaybackUpdateShowLeftRightIcon();
	}

#ifdef S_PLAYB_STATUSBAR
		playback_win_->playBackSatutarbar(true);
#endif
	playback_win_->SetDeleteFileStatus(false);
}

#ifdef USEICONTHUMB
// 删除多选文件
void PlaybackPresenter::DoDeleteSelectFileHander()
{
	playback_win_->CancelUpdateListview = true;
	usleep(50*1000);
	playback_win_->SetDeleteFileStatus(true);
	//stop the play 
	DoDeleteFile2StopPlay();
	db_error("delete all selected files ");
	int count = playback_win_->FileInfo_.size();
	string file_name;
	
	std::map<int, FileInfo>::iterator iter;
	#if 1
	playlist_.clear();
	for (iter = playback_win_->FileInfo_.begin(); iter != playback_win_->FileInfo_.end(); ) 
	{
		//db_error("file name:%s ",(iter->second).filename.c_str());
		if ((iter->second).fileselect == 2) {
			(iter->second).fileselect = 0;
			int type_ = (iter->second).fileType;
            if(type_ == VIDEO_A_SOS || type_ == VIDEO_B_SOS || type_ == VIDEO_A_P || type_ == VIDEO_B_P)
            {
				file_name = (iter->second).filename;
				playlist_.push_back(file_name);
				iter++;		// 迭代器指向下一个元素
				//db_error("SOS file");
            } else {
				int i = (iter->first);
				
				//bkplaylist_ = playlist_.begin() + i;
				
				//get file name 获取需要删除的文件名
				
			    //file_name = bkplaylist_->c_str();		// (用这个会有问题?)bkplaylist_ 为 当前选中文件的迭代器
			    file_name = (iter->second).filename;
				
				#if 1
				// 1.delete from db 先删除数据库相应文件
			    int ret = MediaFileManager::GetInstance()->RemoveFile(file_name);
				if(ret < 0){
			        db_error("delete file error.");
					//continue;
				}
				db_error("delete file name:%s ",file_name.c_str());
				// 2.delete from list 再删除播放列表相关文件
				
				// 3.处理FileInfo_
				//if ((iter->second).thumbbmp.bmBits) {
					//UnloadBitmap(&(iter->second).thumbbmp);	// 释放缩略图空间
					//(iter->second).thumbbmp.bmBits = NULL;
				//}
				//playback_win_->FileInfo_.erase(iter++);		// 迭代器指向下一个元素
				iter++;
				#endif
				usleep(50*1000);
            }
		} else {
			file_name = (iter->second).filename;
			playlist_.push_back(file_name);
			iter++;	// 非选择状态的文件,迭代器指向下一个元素
		}
		
	}	
	
	#else
	for (iter = playback_win_->FileInfo_.begin(); iter != playback_win_->FileInfo_.end(); iter++) 
	{
		if ((iter->second).fileselect == 2) {
			int type_ = (iter->second).fileType;
            if(type_ == VIDEO_A_SOS || type_ == VIDEO_B_SOS || type_ == VIDEO_A_P || type_ == VIDEO_B_P)
            {
            	continue;
            }
			int i = (iter->first);
			
			bkplaylist_ = playlist_.begin() + i;
			
			//get file name 获取需要删除的文件名
			
		    //file_name = bkplaylist_->c_str();		// (用这个会有问题?)bkplaylist_ 为 当前选中文件的迭代器
		    file_name = (iter->second).filename;
			db_error("delete file name:%s ",file_name.c_str());
			//1.delete from db 先删除数据库相应文件
		    int ret = MediaFileManager::GetInstance()->RemoveFile(file_name);
			if(ret < 0){
		        db_error("delete file error.");
				continue;
			}


			//2.delete from list 再删除播放列表相关文件
			if(bkplaylist_ == playlist_.end()-1){	// 最后一个文件
		        playlist_.pop_back();
		        bkplaylist_ = playlist_.end()-1;
				if(playlist_.size() > 0)
				    playlist_idx_ = playlist_.size()-1;
				else
					playlist_idx_ = 0;
				db_msg("[debug_zhanglm]:media file point to the end==playlist_idx_=%d=============",playlist_idx_);
		    }else if(bkplaylist_ == playlist_.begin()){	// 第0个文件
		        db_msg("[debug_zhanglm]:media file point to the begin================");
				bkplaylist_ = playlist_.erase(bkplaylist_);	// erase后bkplaylist_指向下一个
		        bkplaylist_ = playlist_.begin();
		        playlist_idx_ = 0;
		    }else{	// 中间文件
		        bkplaylist_ = playlist_.erase(bkplaylist_);
		        playlist_idx_ = bkplaylist_ - playlist_.begin();
		    }
			// 3. 处理FileInfo_
			if ((iter->second).thumbbmp.bmBits) {
				UnloadBitmap(&(iter->second).thumbbmp);	// 释放缩略图空间
			}
			usleep(50*1000);
		}
		
	}
	#endif
	//usleep(500*1000);
	//update cur playback view
	db_error("playlist_.size(): %d",playlist_.size());
	playback_win_->UpdateFileList(playlist_, playlist_.size());	// 更新PlaybackWindow的playlist_
	playback_win_->HideVideoPlayImgAndIcon();
	playback_win_->CancelUpdateListview = false;
	playback_win_->UpdateListView(UPDATEVIEW_LOAD);
	//UpdateSelectIndexAndTotalFiles();

	if(playlist_.size() == 0){
	     db_msg("[debug_zhb]----delete file and has no file ------");
	     playback_win_->CtrlNoFileOrSdcard(true,true);
	     this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
	}else{
		int val = playback_win_->GetPlaybackPageStatus();
		this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
		playback_win_->PlayingToPlaybackUpdateShowLeftRightIcon();
	}
		
#ifdef S_PLAYB_STATUSBAR
		playback_win_->playBackSatutarbar(true);
#endif
	playback_win_->SetDeleteFileStatus(false);
}
#endif

void PlaybackPresenter::DeleteErrorFile()
{
	playback_win_->SetDeleteFileStatus(true);
	string file_name;
	//get file name
    file_name = bkplaylist_->c_str();
	db_msg("delete file name:%s ",file_name.c_str());
	//delete from db
    int ret = MediaFileManager::GetInstance()->RemoveFile(file_name);
	if(ret < 0){
        db_msg("delete file error.");
		return;
	}
	//delete from list
	if(bkplaylist_ == playlist_.end()-1){
        playlist_.pop_back();
        bkplaylist_ = playlist_.end()-1;
		if(playlist_.size() > 0)
		    playlist_idx_ = playlist_.size()-1;
		else
			playlist_idx_ = 0;
    }else if(bkplaylist_ == playlist_.begin()){
		bkplaylist_ = playlist_.erase(bkplaylist_);
        bkplaylist_ = playlist_.begin();
        playlist_idx_ = 0;
    }else{
        bkplaylist_ = playlist_.erase(bkplaylist_);
        playlist_idx_ = bkplaylist_ - playlist_.begin();
    }
	
	//update cur playback view
	playback_win_->UpdateFileList(playlist_, playlist_.size());
	playback_win_->UpdateListView(UPDATEVIEW_LOAD);
	UpdateSelectIndexAndTotalFiles();
	if(playlist_.size() == 0){
	     db_msg("[debug_zhb]----delete file and has no file ------");
	     playback_win_->CtrlNoFileOrSdcard(true,true);
	     this->Notify((MSG_TYPE)MSG_PLAYBACK_NO_DETECT_FILE_SDCARD,0);
	}else{
		int val = playback_win_->GetPlaybackPageStatus();
		this->Notify((MSG_TYPE)MSG_CHANG_STATU_PLAYBACK,val);
	}
	playback_win_->SetDeleteFileStatus(false);
}

void PlaybackPresenter::UpdatePlaybackView()
{
    std::string type;
    string result_alias_bak;
    if(playlist_.empty()){
        db_msg("[debug_jaosn]:no media file");
        playback_win_->HideVideoPlayImgAndIcon();
	UpdateSelectIndexAndTotalFiles();
	return;
    }
    type = MediaFileManager::GetInstance()->GetMediaFileType(bkplaylist_->c_str());
    if(type == "video_A" || type == "video_B" || type == "videoA_SOS"
            || type == "videoB_SOS" || type == "videoA_PARK" || type == "videoB_PARK"){
        playback_win_->ShowVideoPlayImg();
        result_alias_bak = bkplaylist_->c_str();
		string::size_type rc = result_alias_bak.rfind(".");
		if( rc == string::npos)
		{
			db_warn("invalid fileName:%s",result_alias_bak.c_str());
			return ;
		}
        result_alias= result_alias_bak.substr(0,rc);
        result_alias += "_ths.jpg";
        db_msg("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
        playback_win_->SetThumbShowCtrl(result_alias.c_str());
        // SetFileAndPrepare(bkplaylist_->c_str());
    }else{
        playback_win_->HideVideoPlayImg();
        result_alias_bak = bkplaylist_->c_str();
		string::size_type rc = result_alias_bak.rfind(".");
		if( rc == string::npos)
		{
			db_warn("invalid fileName:%s",result_alias_bak.c_str());
			return ;
		}
        result_alias= result_alias_bak.substr(0,rc);
        result_alias += "_ths.jpg";
        db_msg("[debug_jaosn]:this is photo result_alias is %s",result_alias.c_str());
        playback_win_->SetThumbShowCtrl(result_alias.c_str());
    }
    UpdateSelectIndexAndTotalFiles();
}

void PlaybackPresenter::ShowDeleteDialog(int val)	// 0= photo 1=video 2=select
{
    DialogStatusManager::GetInstance()->setMDialogEventFinish(false);
    PlaybackWindow *win = static_cast<PlaybackWindow*>(win_mg_->GetWindow(WINDOWID_PLAYBACK));
    win->HideVideoPlayImg();
    usleep(500*1000);//wait hide finish 
    win->ShowDeleteDialog(val);
    DialogStatusManager::GetInstance()->setMDialogEventFinish(true);
}

void PlaybackPresenter::ShowVideoPlayImg()
{
	PlaybackWindow *win = static_cast<PlaybackWindow*>(win_mg_->GetWindow(WINDOWID_PLAYBACK));
    	win->ShowVideoPlayImg();
}


