#pragma once

#include "common/common_inc.h"
#include <media/mm_common.h>
#include "device_model/media/camera/camera.h"
#include <string>

namespace EyeseeLinux {

/**
 * @addtogroup DeviceModel
 * @{
 */

/**
 * @addtogroup Media
 * @{
 */

/**
 * @addtogroup MediaFile
 * @brief 多媒体文件类
 * @{
 */

// 缩略图文件名后缀
#define THUMB_SUFFIX "_ths"
// 加锁文件名后缀
#define LOCK_SUFFIX "_SOS"

#define VIDEO_SUFFIX ".mp4"
//#define VIDEO_SUFFIX ".ts"

#define PHOTO_SUFFIX ".jpg"
#define AUDIO_SUFFIX ".aac"

// special file type
enum SPEC_FILE_TYPE {
    THUMB_FILE = 0,
    LOCK_FILE,
};
// media type
enum MEDIA_TYPE {
    PHOTO_A = 0,
	PHOTO_B,
    VIDEO_A,
    VIDEO_B,
    VIDEO_A_SOS,
    VIDEO_B_SOS,
    VIDEO_A_PARK,
    VIDEO_B_PARK,
    AUDIO_A,
    AUDIO_B,
    UNKNOWN,
};

enum PIC_TYPE {
    BMP = 0,
    JPEG,
};


/*
 * 在需要完成连拍时，可预先生成一个文件名队列，存储时再取出使用
 */
class MediaFile
{
    public:
        MediaFile(PhysicalCameraID cam_id, uint8_t media_type, bool p_fileVisible=true);
        MediaFile(const std::string &file_name, bool p_fileVisible=true);
        MediaFile &operator=(const MediaFile &file);
        MediaFile(const MediaFile &file);
        MediaFile(){}
        bool IsLocked() const;
        const std::string &GetMediaFileName() const;
        const std::string &GetVideoThumbPicFileName() const;
        const std::string &GetVideoThumbFileName() const;
        void SetMediaFileName(const std::string &file_name);
        const std::string &GetBaseName() const;
        PhysicalCameraID GetCamID() const;
        uint8_t GetMediaType() const;
        time_t GetFileTime() const;
        unsigned long GetFileSize() const;
        int getFileReallyName(std::string &p_FileName) const;
		int getFilePath(std::string &p_FilePath) const;
		int getFileType() const;
		int ReNameToSosFile();
		bool GetFileVisibleFlag();
		int  SetFileVisibleFlag(bool p_Visibleflag);
    private:
        bool lock_flag_;
        bool in_list_;
        // maybe use bit to store.
        PhysicalCameraID cam_id_; //A B
        uint8_t media_type_; // VIDEO PHOTO
        uint8_t video_suffix_; // .mp4 .mov
        time_t time_;
        // eg: /mnt/extsd/video/19991231_193015A
        std::string base_name_;
        // eg: /mnt/extsd/video/19991231_193015A.mp4
        std::string full_name_;
        std::string video_thumb_pic_name_;
        std::string video_thumb_name_;
		bool m_fileVisible;
    private:
        void GenerateMediaFileName(bool p_fileVisible);
}; /* MediaFile */

/** @} */
/** @} */
/** @} */

} /* EyeseeLinux */
