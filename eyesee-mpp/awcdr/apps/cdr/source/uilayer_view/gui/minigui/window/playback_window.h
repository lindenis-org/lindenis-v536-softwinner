/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file playback_window.h
 * @brief 回放界面
 * @author id:826
 * @version v0.3
 * @date 2016-11-03
 */
#pragma once

//#include "common/app_def.h"
#include "window/window.h"
#include "window/user_msg.h"
#include <signal.h>
#include "device_model/media/media_file_manager.h"
#include "widgets/text_view.h"

#include <sys/time.h>
#define S_PLAYB_STATUSBAR

#ifdef USEICONTHUMB
#include "widgets/icon_view.h"
struct ThumbIconViewBmp {
    std::string name;
    BITMAP bmp;
	BITMAP icon_filetype;	//icon_lefttop;
	BITMAP icon_fileselect;	//icon_righttop;
	BITMAP icon_filelock;
};
#endif
//#define S_FAKE_BG
/**
 * 定义每个窗体内部的button/msg
 */
#define PLAYBACK_SETTING_BUTTON  (USER_MSG_BASE+1)
#define PLAYBACK_PLAY_SEEK       (USER_MSG_BASE+2)
#define PLAYBACK_PLAY_PAUSE      (USER_MSG_BASE+3)
#define PLAYBACK_PLAY_STOP       (USER_MSG_BASE+4)
#define PLAYBACK_VOICE_CONTROL   (USER_MSG_BASE+5)
#define PLAYBACK_SHOW_NEXT_THS   (USER_MSG_BASE+6)
#define PLAYBACK_SHOW_PRVE_THS   (USER_MSG_BASE+7)
#define PLAYBACK_SET_SHOW_THS    (USER_MSG_BASE+8)
#define PLAYBACK_SHOW_FILEOSD	 (USER_MSG_BASE+9)
#define PLAYBACK_FILELIST        (USER_MSG_BASE+10)
#define PLAYBACK_UPDATE_TIMER        (USER_MSG_BASE+11)
#define PLAYBACK_COMPLETION_TIMER    (USER_MSG_BASE+12)
#define PLAYBACK_PLAYING_STOP  (USER_MSG_BASE+13)
#define PLAYBACK_DELETEFILE_ICONGRAY (USER_MSG_BASE+14)
#define PLAYBACK_CHANGE_FILE_NAME   (USER_MSG_BASE+15)
#define PLAYBACK_HIDETHUMB   (USER_MSG_BASE+16)


#define UPDATEVIEW_LOAD		1
#define UPDATEVIEW_NOLOAD	0

#define IMAGEVIEW_NUM 6
#define UPDATEIMGUSETHREAD

enum FileType_t{
	PHOTO_A = 0,
	PHOTO_B,
	VIDEO_A,
	VIDEO_B,
	VIDEO_A_SOS,
	VIDEO_B_SOS,
	VIDEO_A_P,
	VIDEO_B_P,
	FileType_UNKNOWN_TYPE,
};

struct FileInfo{    
	std::string filename;    
	std::string filepath;
	std::string fileCreatTime;//2018-07-01 12:10
	int isHaveThumJPG;
	int fileType;
	#ifdef USEICONTHUMB
	int 		fileselect;		// 0-未选择 1=待选择 2=已选择
	BITMAP 		thumbbmp;
	int 		thumbbmpload;
	BITMAP 		icon[3];
	#endif
};

enum PlayMode_t{
	PLAY_STOP = 0,
	PLAY_DELETE,
};

class Dialog;

class GraphicView;

class TextView;

class SwitchButton;
class ProgressBar;
class PlaylistWindow;

class PromptBox;

class BulletCollection;
class USBModeWindow;

#ifdef USEICONTHUMB
class IconView;
#endif
class PlaybackWindow
        : public SystemWindow {
    DECLARE_DYNCRT_CLASS(PlaybackWindow, Runtime)

    enum PlayerStatus {
        STOPED = 0,
        PLAYING,
        PAUSED,
        COMPLETION,
    };

	typedef struct S_RECT_
	{
		GraphicView * select_rect_icon_top;
		GraphicView * select_rect_icon_bottom;
		GraphicView * select_rect_icon_left;
		GraphicView * select_rect_icon_right;
	}S_RECT;

public:
	PlaybackWindow(IComponent *parent);

	virtual ~PlaybackWindow();

	virtual int OnMouseUp(unsigned int button_status, int x, int y);

	std::string GetResourceName();

	void SettingButtonProc(View *control);

	void ShowPlaylistButtonProc(View *control);

	void PlayProgressSeekProc(View *control, int pos);

	void PlayPauseButtonProc(View *control);

	void PlayStopButtonProc(View *control);

	void VoiceControlButtonProc(View *control);

	void PlaybackWindowProc(View *control);
#ifdef USEICONTHUMB
	void thumbIconItemClickProc(View *control, int index);
#endif		
	void Update(MSG_TYPE msg, int p_CamID=0, int p_recordId=0);

	int HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam);

	void PreInitCtrl(View *ctrl, std::string &ctrl_name);

	static void PlayProgressUpdate(union sigval sigval);

	void ResetPlayProgress();

	void SetPlayDuration(int msec);

	void SetPlayProgress(int msec);

	PlaylistWindow *GetPlaylistWindow();

	void HideControlBar();

	void ShowControlBar();

	static void CtrlbarTimerProc(union sigval sigval);

	void NotifyProxy(class Window *from, int msg, int val);

	int SendMsgProxy(class Window *from, int msg, int val);
	
	void keyProc(int keyCode, int isLongPress);
	
	void keyPlayPauseButtonProc();
	
	void keyPlayStopButtonProc();
	
	void SetThumbShowCtrl(const char * path);
	
	void ShowVideoPlayImg();
	
	void HideVideoPlayImg();
	
	void HideWindowBackImg();
	
	void ShowThumbImg();
	
	void ShowVideoPlayImgAndIcon();
	
	void HideVideoPlayImgAndIcon(bool mshow = true);
	
	void ShowDeleteDialog(int val);

	void HideDeleteDialog();
	
	void ShowStatusbar();
	
	void HideStatusbar();
	
	int getPlayerStatus();
	
#ifndef USEICONTHUMB	
	int getFileInfo(bool flag);
#else
	int getFileInfo(int flag);
#endif	
	void UpdateFileList(const std::vector<std::string> file_list, int sum,bool reset_flag = true);
#ifndef USEICONTHUMB	
	void UpdateListView(bool flag);
#else
	void UpdateListView(int flag);
	void UpdateListViewSelect(bool flag);
#endif	
	void HideListView();
	
	void ShowListView();
	
	void UpdateVideoPlayImg();
	
	std::string GetVideoCreatTime(const std::string &filename);
	
	int getCurrentFileType();
	
	void videoTypeIcon(int videoType_id, int videoType,bool videoType_s);

	void fileTypeIcon(int fileType_id, int fileType,bool fileType_s);

	void CtrlFileTypeIcon(int fileType,int id_);
	
	void UpdatePlaybackTime(int timer_start, int timer_end);
	
	void HidePlaybackTime(bool flag = true);
	
	void CtrlNoFileOrSdcard(bool mshow, bool mfile);
	
	void AbnormalVideoPlayReSetStatus(bool flag = true);

	void AbnormalPhotoPlayReSetStatus(bool flag = true);
	
	void ResetFlagStatus();
#ifdef S_PLAYB_STATUSBAR
	void playBackSatutarbar(bool mshow);

	void UpdatePlaybackFileInfo(int index,int count,bool flag = true);
#endif
	int NextVideoPlay();

	int PreVideoPlay();

	BulletCollection * GetPlayBCHandle(){return play_BulletCollection_;}

	void ResetPlayList();

	void SetPlayerStatus(PlayerStatus val){player_status_ = val;}

	void SetIsLastFirstVideoStatus(bool flag){last_first_video_flag_ = flag;}

	void OnLanguageChanged();

	void SetDeleteFileStatus(int val){m_delete_flag = val;}
	void SetPreviewButtonStatus();
	//do clear old operation and change to preview window
	void DoClearOldOperation();
	static void ClearOldOperationTimerProc(union sigval sigval);
	void ClearOldOperation();
	#ifdef S_FAKE_BG
	void fakeBg(bool m_show);
	#endif
	int  GetVideoCreatTime(const std::string &filename,std::string &fileTime);
	bool getIsPlayFlag(){return play_flag_;}
	int getCurrentFileIndex();
	bool getRefreshFlag(){return m_refresh;}
	void setRefreshFlag(bool flag){m_refresh = flag;}
	int GetPlaybackPageStatus();
	#ifndef USEICONTHUMB
	inline void SetPlaybackStatusIconInvalidFlag(bool invalid_flag){invalid_flag_ = invalid_flag;}
	#else
	void SetPlaybackStatusIconInvalidFlag(bool invalid_flag);
	#endif
	inline bool GetPlaybackStatusIconInvalidFlag(){return invalid_flag_;}
    bool IsPlayingWindow();

    bool IsDeleteDialogWindow();
	#ifndef USEICONTHUMB
	int FileLockCtl();
	#else
    int FileLockCtl(int index);
	int FileLockCtl();
	#endif
    

    void UpdateListFileName(std::vector<std::string> &file_list,int index_);

    void OnLoadWindowUpdateLeftRightIcon(int sum);

    void PlayingToPlaybackUpdateShowLeftRightIcon();

    void SetPlayerFinish();
	#ifdef USEICONTHUMB
	void SetFileInfoSelect(int index, int state);
	void FreeFileInfo();
	int GetFileInfoSelectCount();
	void UpdateListView_threadstart();
	std::map<int, FileInfo> FileInfo_;
	std::map<int, ThumbIconViewBmp> thumb_icon_views_;
	#endif
    inline void SetPlaybackWinMessageReceiveFlag(bool flag){ignore_message_flag_ = flag;}
	#ifdef USEICONTHUMB
	IconView *ivplayback_thumb_;
	GraphicView* m_playback_thumb_bk;
	//当前选择文件
	int select_file_;
	int select_status;
	pthread_t m_UpdateIconviewImg_thread_id;
	bool CancelUpdateListview;
	#endif
	int ChangePlayStatusPlaylist();
private:
	void CtrlSelectFileRect(int val,bool m_show);

    void ButtonClickProc(View *control);

    void ImageViewClickProc(View *control);
	#ifdef USEICONTHUMB
	static void *IconviewUpdateImg_thread(void *context);
	
	void UpdateImgRange(int start);
	#endif
	void UpdateSelectIndexAndTotalFiles(bool flag);
private:
	BulletCollection *play_BulletCollection_;
	TextView *del_info_text;
	PlaylistWindow *playlist_win_;
	ProgressBar *progress_bar_;
	TextView* playback_time_label;
	TextView* playback_front_camera_video;
	GraphicView *show_thumb_jpg_;
	GraphicView *play_back_icon_;
	GraphicView *delete_icon_;
	GraphicView *play_back_icon_s_;
	GraphicView *delete_icon_s_;		
	GraphicView * fake_bg_icon;
	#ifndef USEICONTHUMB
	S_RECT s_rect_[6];
	#endif
	timer_t play_timer_id_;
	timer_t ctrlbar_timer_id_;
	PromptBox * p_PromptBox_;
	PlayerStatus player_status_;

	std::vector<std::string> playlist_;
	TextView *title_window;
	#ifndef USEICONTHUMB
	GraphicView *show_thumb_view_[6];
	GraphicView *show_thumb_view_s_[6];
	TextView *show_thumb_time_[6];
	#endif
	
	#ifndef USEICONTHUMB
	FileInfo mediaf_image_[6];
	#endif
	
	
	

	#ifdef USEICONTHUMB
	int UpdateIconviewImg_thread_flag;
	int CancelIconviewImg_thread_flag;
	//std::map<int, BITMAP> iconlist_;
	BITMAP icon_filetype_[10];
	#endif
	//文件总数
	int file_sum_; 
	//记录当前页数
	int page_num_;
	//记录总页数
	int page_sum_;
	#ifndef USEICONTHUMB
	int select_file_;
	#endif
	//选中文件标识
	int file_index_; 
	//回放删除选择模式
	int select_mode_;
	//是否在回放刘表界面
	bool listview_flag_;
	//是否在回放删除选择界面
	bool select_flag_;
	//是否在回放界面
	bool play_flag_;
	//在播放界面切换上下个视频时候是否在播放状态
	bool last_first_video_flag_;
	//当前没有sdcard or no file status
	bool m_nofile_nosdcard_;
	bool m_delete_flag;	
	bool invalid_flag_;
	bool ignore_message_flag_;
	//video timer
	int playback_timer_;
	NotifyEvent OnClick;
    struct timeval tm1,tm2;

	void StartPlay();

	void PausePlay();

	void StopPlay();

	HWND hwnd_p;
	bool m_refresh;//playback window sdcard insert will refresh six-square grid interface flag
	timer_t p_clearOldOperation_timer_id;
    pthread_mutex_t playback_proc_lock_,m_lock_file_ctl;
	#ifdef USEICONTHUMB
	pthread_mutex_t loadfile_ctl;	
	#endif
    bool m_flig_rl;//flig right left flag
    std::string m_update_file_name;//use for update file lock status name

	
};
