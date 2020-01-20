#include "device_model/media/media_file.h"
#include "device_model/media/media_definition.h"
#include "device_model/storage_manager.h"
#include "device_model/system/rtc.h"
#include "common/utils/utils.h"

#undef LOG_TAG
#define LOG_TAG "media_file.cpp"
#include "common/app_log.h"

#include <iostream>

using namespace EyeseeLinux;
using namespace std;

MediaFile::MediaFile(PhysicalCameraID cam_id, uint8_t media_type, bool p_fileVisible)
    : lock_flag_(false)
    , cam_id_(cam_id)
    , media_type_(media_type)
    , time_(0)
{
    db_msg("cam_id:%d, media_type:%d", cam_id, media_type);
    this->GenerateMediaFileName(p_fileVisible);
    in_list_ = true;
	m_fileVisible = p_fileVisible;
}

static void parase_timestring(const string &base_name, time_t *time)
{
    int year, month, day, hour, min, sec;
	string::size_type rc = base_name.rfind('/');
	if( rc == string::npos)
	{
		*time = 0;
		return ;
	}

    string filename = base_name.substr( rc + 1);

    if (filename.size() < 13) return;

    year  = atoi(filename.substr(0,  4).c_str());
    month = atoi(filename.substr(4,  2).c_str());
    day   = atoi(filename.substr(6,  2).c_str());
    hour  = atoi(filename.substr(9,  2).c_str());
    min   = atoi(filename.substr(11, 2).c_str());
    sec   = atoi(filename.substr(13, 2).c_str());

    //db_msg("time: %d-%d-%d, %d-%d-%d", year, month, day, hour, min, sec);

    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon  = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min  = min;
    tm.tm_sec  = sec;
    tm.tm_wday = 0;
    tm.tm_yday = 0;
    tm.tm_isdst = 0;

    *time = mktime(&tm);
     //db_msg("base name: %s, reault: %d", filename.c_str(), *time);
}

MediaFile::MediaFile(const string &file_name, bool p_fileVisible)
    : lock_flag_(false)
    , in_list_(false)
    , time_(0)
{
	m_fileVisible = p_fileVisible;
	string::size_type rc;
	#ifdef USE_CAMB
	rc = file_name.rfind("F");
    if ( rc != string::npos)
	{
        cam_id_ = CAM_A;
    }
	else
	{
		rc = file_name.rfind("R");
		if ( rc != string::npos)
    	    cam_id_ = CAM_B;
		else
		{
			cam_id_ = ERROR_CAM;
			full_name_.clear();
			base_name_.clear();
			media_type_ = UNKNOWN;
			return ;
		}			
    }
	#else
	cam_id_ = CAM_A;
	#endif
    full_name_ = file_name;
	//db_error("full_name_: %s",full_name_.c_str());
	rc = full_name_.rfind(".");
	if(rc == string::npos)
	{
		db_warn("invalid fileName:%s",full_name_.c_str());
		return ;
	}
    base_name_ = full_name_.substr(0, rc);
	//db_error("base_name_: %s",base_name_.c_str());
	string::size_type idx;
    string::size_type tmp;
    string suffix = full_name_.substr(rc);
    if (suffix == PHOTO_SUFFIX)
	{
        tmp = base_name_.rfind("photo");
        if( tmp == string::npos) {
            return;
        }
		media_type_ = PHOTO_A;
		#ifdef USE_CAMB
        //idx = full_name_.rfind("F", strlen("/mnt/extsd/photo/*/"));
		idx = base_name_.rfind("F", base_name_.length());
		if( idx != string::npos)
	    {
	    	media_type_ = PHOTO_A;
		}
		else
		{
			//idx = full_name_.find("R",strlen("/mnt/extsd/photo/*/"));
			idx = base_name_.rfind("R",base_name_.length());
			if( idx != string::npos) {
				media_type_ = PHOTO_B;
			}
		}
		#endif
    }
	else if (suffix == VIDEO_SUFFIX)
	{
		//idx = full_name_.find("F_SOS", strlen("/mnt/extsd/event/*/"));
		tmp = base_name_.rfind("video");
		
        if( tmp == string::npos) {	//  "video" no found
            tmp = base_name_.rfind("event");
            if( tmp == string::npos) {	//  "event" no found
                tmp = base_name_.rfind("park");
                if( tmp == string::npos) {	//  "park" no found
                    return;
                }
            }
        }
		#ifdef USE_CAMB
		idx = base_name_.rfind("F_SOS",base_name_.length());
		#else
		idx = base_name_.rfind("_SOS",base_name_.length());
		#endif
		if( idx != string::npos)
	    {
	    	media_type_ = VIDEO_A_SOS;
		}
		else
		{	// "_SOS" no found
			//idx = full_name_.find("R_SOS", strlen("/mnt/extsd/event/*/"));
			idx = base_name_.rfind("R_SOS",base_name_.length());
			if( idx != string::npos) {
				media_type_ = VIDEO_B_SOS;
			}
			else
			{
				//idx = full_name_.find("F_PARK", strlen("/mnt/extsd/park/*/"));
				#ifdef USE_CAMB
				idx = base_name_.rfind("F_PARK", base_name_.length());
				#else
				idx = base_name_.rfind("_PARK", base_name_.length());
				#endif
				if( idx != string::npos) {
					media_type_ = VIDEO_A_PARK;
				}
				else
				{
					//idx = full_name_.find("R_PARK", strlen("/mnt/extsd/park/*/"));
                    idx = base_name_.rfind("R_PARK", base_name_.length());
					if( idx != string::npos){
						media_type_ = VIDEO_B_PARK;
					}else{
					    //idx = full_name_.find("F", strlen("/mnt/extsd/video/*/"));
					    media_type_ = VIDEO_A;
					    #ifdef USE_CAMB
                        idx = base_name_.rfind("F", base_name_.length());
						
			
                           if(idx != string::npos)
                           {
                                media_type_ = VIDEO_A;
                           }else{
                                //idx = full_name_.find("R", strlen("/mnt/extsd/video/*/"));
                                idx = base_name_.rfind("R", base_name_.length());
                                if(idx != string::npos){
                                    media_type_ = VIDEO_B;
                                }
                           }
                    	#endif
                    }
				}
			}
		}
    }
	else if( suffix == AUDIO_SUFFIX )
	{
		media_type_ = AUDIO_B;
	}
	else {
        media_type_ = UNKNOWN;
    }
	
    if(media_type_ != PHOTO_A && media_type_ != PHOTO_B) 
	{
        video_thumb_pic_name_ = base_name_+ THUMB_SUFFIX + PHOTO_SUFFIX;		// 20190301_120019_ths.jpg
        video_thumb_name_ = base_name_ + THUMB_SUFFIX + VIDEO_SUFFIX;			// 20190301_120019_ths.mp4	?
    }
	//db_error("media_type_: %d",media_type_);
    parase_timestring(base_name_, &time_);
}

MediaFile& MediaFile::operator=(const MediaFile &file)
{
    lock_flag_ = file.lock_flag_;
    in_list_ = file.in_list_;
    cam_id_ = file.cam_id_;
    media_type_ = file.media_type_;
    video_suffix_ = file.video_suffix_;
    time_ = file.time_;
    base_name_ = file.base_name_;
    full_name_ = file.full_name_;
    return *this;
}

MediaFile::MediaFile(const MediaFile &file)
{
    *this = file;
}

bool MediaFile::IsLocked() const
{
    return lock_flag_;
}

void MediaFile::GenerateMediaFileName(bool p_fileVisible)
{
    char file[128] = {0};
	string file_type;

    string media_path;
    string media_suffix;
    struct tm *time = NULL;
    time_ = get_date_time(&time);

    // CAM_CSI->'A', CAM_UVC->'B'
	media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);

	switch(media_type_)
	{
		case PHOTO_A:
			media_suffix = PHOTO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "F";
			#else
			file_type = "";
			#endif
            media_path = DIR_2CAT(MOUNT_PATH, PHOTO_DIR_A);
			break;
		case PHOTO_B:
			media_suffix = PHOTO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "R";
			#else
			file_type = "";
			#endif
			media_path = DIR_2CAT(MOUNT_PATH, PHOTO_DIR_B);
			break;
		case VIDEO_A:
			media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "F";
			#else
			file_type = "";
			#endif
			break;
		case VIDEO_B:
			media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "R";
			#else
			file_type = "";
			#endif
            #if 0
			if( p_fileVisible )
				media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);
			else			
				media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
            #endif
            media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
			break;
		case VIDEO_A_SOS:
			media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "F_SOS";
			#else
			file_type = "_SOS";
			#endif
			break;
		case VIDEO_B_SOS:
			media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "R_SOS";
			#else
			file_type = "_SOS";
			#endif
            #if 0
			if( p_fileVisible )
				media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A);
			else			
				media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
            #endif
            media_path = DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B);
			break;
		case AUDIO_A:
			break;
		case AUDIO_B:
			media_suffix = AUDIO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "R";
			#else
			file_type = "";
			#endif
			media_path = DIR_2CAT(MOUNT_PATH, PARK_DIR_A);
			break;
        case VIDEO_A_PARK:
            media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "F_PARK";
			#else
			file_type = "_PARK";
			#endif
            media_path = DIR_2CAT(MOUNT_PATH, PARK_DIR_A);
            break;
        case VIDEO_B_PARK:
            media_suffix = VIDEO_SUFFIX;
			#ifdef USE_CAMB
			file_type = "R_PARK";
			#else
			file_type = "_PARK";
			#endif
            media_path = DIR_2CAT(MOUNT_PATH, PARK_DIR_B);
            break;
	}

    if (create_dir(media_path.c_str()) < 0) {
        db_warn("generate media file name failed");
        return;
    }
	
	if( cam_id_ == CAM_A )
	{
		snprintf(file, sizeof(file), "%s%04d%02d%02d_%02d%02d%02d%s", media_path.c_str(),
				time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
				time->tm_hour,time->tm_min, (time->tm_sec),file_type.c_str());
	}
	else
	{
		snprintf(file, sizeof(file), "%s%04d%02d%02d_%02d%02d%02d%s", media_path.c_str(),
				time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
				time->tm_hour,time->tm_min, (time->tm_sec),file_type.c_str());
	}
    base_name_ = file;
    full_name_ = base_name_ + media_suffix;
    if(media_type_ != PHOTO_A && media_type_ != PHOTO_B)
    {
        db_msg("[debug_jason]:generate the video thumb pic");
        video_thumb_pic_name_ = base_name_+ THUMB_SUFFIX + PHOTO_SUFFIX;
        video_thumb_name_ = base_name_+ THUMB_SUFFIX + VIDEO_SUFFIX;
    }
}

const string &MediaFile::GetVideoThumbPicFileName() const
{
    return video_thumb_pic_name_;
}


const string &MediaFile::GetVideoThumbFileName() const
{
    return video_thumb_name_;
}
const string &MediaFile::GetMediaFileName() const
{
    return full_name_;
}

void MediaFile::SetMediaFileName(const string &file_name)
{
    full_name_ = file_name;
}

const string &MediaFile::GetBaseName() const
{
    return base_name_;
}

PhysicalCameraID MediaFile::GetCamID() const
{
    return cam_id_;
}

uint8_t MediaFile::GetMediaType() const
{
    return media_type_;
}

time_t MediaFile::GetFileTime() const
{
    return time_;
}

unsigned long MediaFile::GetFileSize() const
{
    struct stat statbuff;
    if( stat(full_name_.c_str(), &statbuff) < 0)
    {
        db_error("get file size error, %s", strerror(errno));
        return -1;
    }
    
    return statbuff.st_size;
}

int MediaFile::getFileReallyName(std::string &p_FileName) const
{
	p_FileName.clear();
	string::size_type rc = full_name_.rfind("/");
	if( rc == string::npos)
		return -1;

 	p_FileName = full_name_.substr(rc+1);
	return 0;
}

int MediaFile::getFilePath(std::string &p_FilePath) const
{
	p_FilePath.clear();

	string::size_type rc = full_name_.rfind("/");
	if( rc == string::npos)
		return -1;

 	p_FilePath = full_name_.substr(0, rc);
	return 0;
}

int MediaFile::getFileType() const
{
	if( media_type_ == VIDEO_A || media_type_ == VIDEO_A_SOS || media_type_ == VIDEO_B || media_type_ == VIDEO_B_SOS || VIDEO_A_PARK || VIDEO_B_PARK )
		return 1;
	else if( media_type_ == PHOTO_A || media_type_ == PHOTO_B )
		return 0;
	else if( media_type_ == AUDIO_A || media_type_ == AUDIO_B )
		return 2;

	return -1;
}

int MediaFile::ReNameToSosFile()
{
	string::size_type rc = full_name_.rfind(".");
	if( rc == string::npos)
		return -1;

	std::string suffix = full_name_.substr(rc);
	if(  suffix == PHOTO_SUFFIX)
	{
		full_name_ = base_name_ + "_SOS" + PHOTO_SUFFIX;
	}
	else if( suffix == VIDEO_SUFFIX)
	{
		full_name_ = base_name_ + "_SOS" + VIDEO_SUFFIX;
		video_thumb_pic_name_ = base_name_+ "_SOS" + THUMB_SUFFIX + PHOTO_SUFFIX;
		video_thumb_name_ = base_name_ + "_SOS" + THUMB_SUFFIX + VIDEO_SUFFIX;
	}
	else if(suffix == AUDIO_SUFFIX)
	{
		full_name_ = base_name_ + "_SOS" + AUDIO_SUFFIX;
	}

	if(media_type_ == VIDEO_A)
		media_type_ = VIDEO_A_SOS;
	else if(media_type_ == VIDEO_B)
		media_type_ = VIDEO_B_SOS;

	return 0;
}

bool  MediaFile::GetFileVisibleFlag()
{
	return m_fileVisible;
}

int MediaFile::SetFileVisibleFlag(bool p_Visibleflag)
{
	m_fileVisible = p_Visibleflag;

	return 0;
}
