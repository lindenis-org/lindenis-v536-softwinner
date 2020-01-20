/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: media_file_manager.h
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2015-11-2
Description:
History:
*************************************************/
#include "window/window.h"
#include "window/user_msg.h"
#include "device_model/media/media_file_manager.h"
#include "device_model/media/media_file.h"
#include "device_model/system/event_manager.h"
#include "device_model/storage_manager.h"
#include "device_model/system/rtc.h"
#include "common/app_log.h"
#include "common/utils/utils.h"
#include "device_model/dataManager.h"
#include "bll_presenter/camRecCtrl.h"
#include "bll_presenter/fileLockManager.h"
#include <fnmatch.h>


//#define PHOTO_NO_TO_SQL
#undef LOG_TAG
#define LOG_TAG "media_file_manager.cpp"

#define DATA_TABLE "MediaFile"
#ifdef USB_MODE_WINDOW
#define DB_PATH "/tmp/sqlite"
#else
#define DB_PATH "/mnt/extsd/.tmp/sqlite"
#endif
#define DB_NAME "sunxi.db"
#define DB_FILE DB_PATH "/" DB_NAME
//#define DB_BACK_CAMERA_NAME "sunxi_back_cam.db"
//#define DB_BACK_CAMERA_FILE DB_PATH "/" DB_BACK_CAMERA_NAME

#define DATABASE_RESERVED_RECORD_CNT 5000

using namespace EyeseeLinux;
using namespace std;

MediaFileManager::MediaFileManager()
    : db_inited_(false)
    , db_stop_update_(false)
    , db_update_done_(false)
    , db_new_create_(false)
    //, db_backCam_new_create_(false)
    , db_(NULL)
    //, m_BackCam_db(NULL)
    , m_OrderStatus(0)
    ,DB_updata_thread_id(0)
    ,DB_init_thread_id(0)
    ,db_start_up_init_(false)
{
    m_orderId.clear();
	m_needUpdate = false;
	m_IgnoreScan = false;
	pthread_mutex_init(&m_lock, NULL);
	pthread_mutex_init(&m_db_lock, NULL);
    StorageManager::GetInstance()->Attach(this);
}

MediaFileManager::~MediaFileManager()
{
	 db_msg("destruct");
     StorageManager *sm = StorageManager::GetInstance();
	if(DB_updata_thread_id > 0)
		pthread_detach(DB_updata_thread_id);
	if(DB_init_thread_id > 0)
		pthread_detach(DB_init_thread_id);	

	 sm->Detach(this);

     db_stop_update_ = true;
     if (db_)
	 {
         delete db_;
         db_ = NULL;
     }
#if 0
	if( m_BackCam_db)
	{
		delete m_BackCam_db;
		m_BackCam_db = NULL;
	}
#endif
	pthread_mutex_destroy(&m_lock);
	pthread_mutex_destroy(&m_db_lock);
}

int MediaFileManager::startCreateDataBase()
{
	int ret = StorageManager::GetInstance()->GetStorageStatus();
    if ( (ret != UMOUNT) && (ret != STORAGE_FS_ERROR) && (ret != FORMATTING))
   {
	if(DB_updata_thread_id > 0)
		pthread_detach(DB_updata_thread_id);

	if(DB_init_thread_id > 0)
		pthread_detach(DB_init_thread_id);
	
	if(DB_init_thread_id == 0)
    		ThreadCreate(&DB_init_thread_id,NULL,MediaFileManager::DBInitThread, this);
            pthread_detach(DB_init_thread_id);
    	}
	return 0;
}

void *MediaFileManager::DBInitThread(void *arg)
{
    db_error("run db init thread");
    prctl(PR_SET_NAME, "DBInitThread", 0, 0, 0);
    MediaFileManager* mfm = reinterpret_cast<MediaFileManager*>(arg);
    mfm->DBInit();
    //pthread_join(mfm->DB_init_thread_id,NULL);
    mfm->DB_init_thread_id = 0;
    return NULL;
}

int8_t MediaFileManager::DBInit()
{
	int ret = StorageManager::GetInstance()->GetStorageStatus();
    if( (ret == UMOUNT) || (ret == STORAGE_FS_ERROR ) || (ret == FORMATTING) )
    {
		m_IgnoreScan = false;
    	return 0;
    }
	bool reCreateDb_A = false;
	//bool reCreateDb_B = false;

	ret = 0;
    if (db_inited_)
		goto out;
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
    try 
	{
        if (db_ != NULL )
        {
			goto exec;
        }

        if (access(DB_PATH, F_OK) < 0)
		{
            int status = system("mkdir -p " DB_PATH);
            if (WIFEXITED(status))
			{
                if (WEXITSTATUS(status) != 0)
				{
                    db_error("create dir '%s' failed, err: %s", DB_PATH, strerror(errno));
					m_IgnoreScan = false;
					 pthread_mutex_unlock(&m_lock);
					 pthread_mutex_unlock(&m_db_lock);
                    return -1;
                }
            }
			else
			{
                db_error("create dir '%s' failed, err: %s", DB_PATH, strerror(errno));
				 m_IgnoreScan = false;
			     pthread_mutex_unlock(&m_lock);
				 pthread_mutex_unlock(&m_db_lock);
                return -1;
            }
			reCreateDb_A = true;
			//reCreateDb_B = true;
        }
		else
		{
			if(access("/mnt/extsd/.tmp/sqlite/sunxi.db", F_OK) < 0)
				reCreateDb_A = true;
#if 0
			if(access("/mnt/extsd/.tmp/sqlite/sunxi_back_cam.db", F_OK) < 0)
				reCreateDb_B = true;
#endif
        }

		if( NULL == db_ )
		{
	        db_ = new SQLite::Database(DB_FILE, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
	        if (db_ == NULL)
			{
				db_error("create sunxi.db failed");
	            db_inited_ = false;
				m_IgnoreScan = false;
					pthread_mutex_unlock(&m_lock);
				pthread_mutex_unlock(&m_db_lock);
	            return ret;
		    }
		}
        #if 0
		if( NULL == m_BackCam_db )
		{
			m_BackCam_db = new SQLite::Database(DB_BACK_CAMERA_FILE, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
			if( NULL == m_BackCam_db )
			{
				db_error("create sunxi_back_cam.db failed");
				db_inited_ = false;
				m_IgnoreScan = false;
					pthread_mutex_unlock(&m_lock);
				 pthread_mutex_unlock(&m_db_lock);
				return ret;
			}
		}
        #endif
exec:

		if( db_ )
        {
	        if( reCreateDb_A )
	        {
	        	db_->exec("DROP TABLE IF EXISTS " DATA_TABLE);
		        db_->exec("CREATE TABLE " DATA_TABLE " (file VARCHAR(128) PRIMARY KEY, time INTEGER, type VARCHAR(32), info VARCHAR(32), size INTEGER, Lock INTEGER, LockTime INTEGER, checkstate INTEGER, keyString VARCHAR(128), orderNum VARCHAR(32))");
	        }
			db_new_create_ = true;
		}
#if 0
		if( m_BackCam_db )
		{
			if( reCreateDb_B )
			{
	        	m_BackCam_db->exec("DROP TABLE IF EXISTS " DATA_TABLE);
		        m_BackCam_db->exec("CREATE TABLE " DATA_TABLE " (file VARCHAR(128) PRIMARY KEY, time INTEGER, type VARCHAR(32), info VARCHAR(32), size INTEGER, Lock INTEGER, LockTime INTEGER, checkstate INTEGER, keyString VARCHAR(128), orderNum VARCHAR(32))");
			}
			db_backCam_new_create_ = true;
		}
#endif
    }
	catch (std::exception& e) 
	{
        db_error("SQLite exception: %s", e.what());
        db_inited_ = false;
        db_stop_update_ = true;
		m_IgnoreScan = false;
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
        return -1;
    }

    db_inited_ = true;
    db_stop_update_ = false;
    db_start_up_init_ = false;
    db_error("db init end");
    pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);

out:
    DBUpdate();
    EventManager::GetInstance()->sdcardFlag = false;
	db_warn("[debug_jaosn]:sdcard is ready\n");
    return ret;
}

int8_t MediaFileManager::DBReset()
{
	if( db_inited_ )
	{
		db_warn("database has been inited\n");
		m_IgnoreScan =false;
		return 0;
	}
	if(DB_init_thread_id == 0){
    	//	ThreadCreate(&DB_init_thread_id,NULL,MediaFileManager::DBInitThread, this);
		pthread_create(&DB_init_thread_id, NULL, MediaFileManager::DBInitThread, this);
		pthread_join(DB_init_thread_id,NULL);
	}
    return 0;
}

uint32_t MediaFileManager::GetMediaFileCnt(const string &media_type,int p_CamId)
{
    unsigned int  record_cnt = 0;
    string filter;
    if (!db_inited_)
	{
    	db_warn("db is not inited");
        return 0;
    }
    if (!media_type.empty())
	{
        if (media_type == "lock" || media_type == "normal")
		{
            filter = "where info='"+ media_type + "'";
        }
		else
		{
            filter = "where type='"+ media_type + "'";
        }
    }
    string sql = "select count() from ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if(0 == p_CamId || 1 == p_CamId )
	{
	    if( db_ != NULL)
	    {
	        try
			{
	            SQLite::Column col = db_->execAndGet(sql);
	    		if (col.isInteger()) 
				{
	                record_cnt = col;
	            }
				else
				{
	                db_error("get wrong data type");
	            }
	    	}
			catch(SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
	}
	else
	{
        #if 0
	    if( m_BackCam_db != NULL)
	    {
	        try
			{
	            SQLite::Column col = m_BackCam_db->execAndGet(sql);
	    		if (col.isInteger()) 
				{
	                record_cnt = col;
	            }
				else
				{
	                db_error("get wrong data type");
	            }
	    	}
			catch(SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
        #endif
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return record_cnt;
}



int8_t MediaFileManager::GetMediaFileList(vector<string> &file_list,
        uint32_t idx_s, uint16_t list_cnt, bool order, const string &media_type,int p_CamId)
{
    string file_name;
    string sort_order = order?"asc":"desc";
    string filter;
	pthread_mutex_lock(&m_db_lock);
    if (db_ == NULL  || db_inited_ == false)
	{
    	db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
        return -1;
    }
	pthread_mutex_unlock(&m_db_lock);

    if (!media_type.empty())
        filter = "where type='" + media_type + "' ";

    filter += "order by time " + sort_order;

    uint32_t media_cnt = GetMediaFileCnt(media_type, p_CamId);
    if (media_cnt == 0)
	{
    	db_warn("no media file");
        return -1;
    }

    string sql = "select * from ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);

	    	    if (col.isText())
	        	{
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
					{
	                	file_name += '\0';
	                	file_list.push_back(file_name);
	            	}
	        	}
	        	else
	        	{
	           	 	db_error("get wrong data type");
	        	}
	    	}
		}
		else
		{	
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);

	    	    if (col.isText())
	        	{
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
					{
	                	file_name += '\0';
	                	file_list.push_back(file_name);
	            	}
	        	}
	        	else
	        	{
	           	 	db_error("get wrong data type");
	        	}
	    	}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return 0;
}

time_t MediaFileManager::GetFileTimestampByName(const std::string &file_name,int p_CamId)
{
    string filter;
    time_t timestamp = -1;

    if (file_name.empty())
	{
        db_warn("file name is empty");
        return -1;
    }
	pthread_mutex_lock(&m_db_lock);
    if (db_ == NULL  || db_inited_ == false)
	{
    	db_warn("data base is null! db_: db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
    }
	pthread_mutex_unlock(&m_db_lock);
	//db_error("FileTimestamp: %s",file_name.c_str());
    filter = "where file='" + file_name + "'";

    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
    pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}

    try
	{
		if( 0 == p_CamId || 1 == p_CamId)
	    {
	    	SQLite::Statement query(*db_, sql);
	        query.executeStep();
	        timestamp = query.getColumn(1).getInt64();
		}
		else
	    {
            #if 0
		    SQLite::Statement query(*m_BackCam_db, sql);	
	        query.executeStep();
	        timestamp = query.getColumn(1).getInt64();
            #endif
		}
    }
	catch (SQLite::Exception e)
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return timestamp;
}

const std::string MediaFileManager::GetMediaFileType(const string &file_name, int p_CamId)
{
    string filter;
    string file_type;

    if (file_name.empty())
	{
        db_warn("file name is empty");
		return "error";
    }
	pthread_mutex_lock(&m_db_lock);
    if (db_ == NULL || db_inited_ == false)
	{
    	db_warn("data base is null! db_:%p db_inited:%d",db_,db_inited_);
		pthread_mutex_unlock(&m_db_lock);
        return "error";
    }
	pthread_mutex_unlock(&m_db_lock);
	//db_error("filename: %s",file_name.c_str());
    filter = "where file='" + file_name + "'";

    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return "error";
	}

	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
            //db_msg("haobo---> sql = %d",sql.c_str());
			SQLite::Statement query(*db_, sql);
			query.executeStep();
			file_type = query.getColumn(2).getString();
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
		    query.executeStep();
		    file_type = query.getColumn(2).getString();
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return file_type;
}


int8_t MediaFileManager::DeleteFileByName(const string &file_name, int p_CamId)
{
    int ret = 0;
    int ret1 = 0;
    int ret2 = 0;
    if (file_name.empty())
	{
        db_error("delete file failed: file name is empty");
        return ret;
    }
    db_msg("delete file: %s", file_name.c_str());
    StorageManager *sm = StorageManager::GetInstance();
    sm->RemoveFile(file_name.c_str());
	if( p_CamId == 0  || 1== p_CamId)
	{
	    string thumb_pic = GetThumbPicName(file_name,p_CamId);
	    sm->RemoveFile(thumb_pic.c_str());
	}
    string filter = "WHERE file='" + file_name + "'";
    string sql = "DELETE FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	if( 0 == p_CamId || 1 == p_CamId)
	{
	    if (db_ != NULL) 
		{
	        try 
			{
	            db_->exec(sql);
	        }
			catch (SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
	}
	else
	{
        #if 0 
		if (m_BackCam_db != NULL)
		{
	        try 
			{
	            m_BackCam_db->exec(sql);
	        }
			catch (SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
        #endif
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
#if 0
	FILE_INFO FileInfo;
	bzero(&FileInfo, sizeof(FILE_INFO));
	std::string FileName = getFileReallyName(file_name);
#ifdef DEBUG_DATABASE
    db_error("FileName:%s", FileName.c_str());
#endif
	if( !FileName.empty() )
	{
		strncpy(FileInfo.file_name, FileName.c_str(), sizeof(FileInfo.file_name));
		DataManager::GetInstance()->popFromFileMap(FileInfo);
	}
#endif
    return ret;
}

int8_t MediaFileManager::DeleteFilesByType(const string &media_type, int p_CamId)
{
    string filter;

    StorageManager *sm = StorageManager::GetInstance();
	if(0 == p_CamId )
	    sm->RemoveFiles(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_A));
	else 
		sm->RemoveFiles(DIR_2CAT(MOUNT_PATH, VIDEO_DIR_B));
    filter = "WHERE type='" + media_type + "'";

    string sql = "DELETE FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL )
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	if( 0 == p_CamId || 1 == p_CamId )
	{
	    if( db_ != NULL)
	    {
	    	try
			{
	            db_->exec(sql);
	        }
			catch(SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
	}
	else
	{
        #if 0
	    if( m_BackCam_db != NULL)
	    {
	    	try
			{
	            m_BackCam_db->exec(sql);
	        }
			catch(SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	    }
        #endif
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return 0;
}

int8_t MediaFileManager::DeleteLastFilesDatabaseIndex(const string &file_name, int p_CamId)
{
    int ret = 0;
    int ret1 = 0;
    int ret2 = 0;
    if (file_name.empty())
    {
       db_error("delete file failed: file name is empty");
       return ret;
    }
    db_error("delete file index: %s", file_name.c_str());
    StorageManager *sm = StorageManager::GetInstance();
    //sm->RemoveFile(file_name.c_str());
    if( p_CamId == 0  || 1== p_CamId)
    {
       string thumb_pic = GetThumbPicName(file_name,p_CamId);
       db_error("remove video file thumb pic %s",thumb_pic.c_str());
       sm->RemoveFile(thumb_pic.c_str());
    }
    string filter = "WHERE file='" + file_name + "'";
    string sql = "DELETE FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
    pthread_mutex_lock(&m_db_lock);
    pthread_mutex_lock(&m_lock);
    if (db_ == NULL)
    {
       pthread_mutex_unlock(&m_lock);
       pthread_mutex_unlock(&m_db_lock);
       return -1;
    }
    if( 0 == p_CamId || 1 == p_CamId)
    {
       if (db_ != NULL)
       {
           try
           {
               db_->exec(sql);
           }
           catch (SQLite::Exception e)
           {
               db_error("SQLITE ERROR: %s", e.what());
           }
       }
    }
    else
    {
       #if 0
       if (m_BackCam_db != NULL)
       {
           try
           {
               m_BackCam_db->exec(sql);
           }
           catch (SQLite::Exception e)
           {
               db_error("SQLITE ERROR: %s", e.what());
           }
       }
       #endif
    }
    pthread_mutex_unlock(&m_lock);
    pthread_mutex_unlock(&m_db_lock);
    return ret;
}

int64_t MediaFileManager::GetLastFileFallocateSizeByType(const std::string &media_type,int p_CamId)
{
        int64_t fallocateSize = 0;
        bool order = 0;
        bool m_force = 1;
        string sort;
        string filter;
        int LockStatus =-1;
        StorageManager *sm = StorageManager::GetInstance();
        sort = order?"desc":"asc";
        if(m_force)
        {
           if( media_type.empty())
               filter = " order by time " + sort;
           else
               filter = "where type= '" + media_type+"'"+" order by time " + sort;
        }
        else
        {
           if( media_type.empty())
               filter = "where Lock=0 order by time " + sort;
           else
               filter = "where type= '" + media_type+ "'"+" and Lock=0"+" order by time " + sort;
        }
        string sql = "SELECT * FROM ";
        sql.append(DATA_TABLE);
        sql.append(" ");
        sql.append(filter);
        pthread_mutex_lock(&m_db_lock);
        pthread_mutex_lock(&m_lock);
        if (db_ == NULL)
        {
           pthread_mutex_unlock(&m_lock);
           pthread_mutex_unlock(&m_db_lock);
           return 0;
        }
        try{
           if( 0 == p_CamId || 1 == p_CamId)
           {
               SQLite::Statement query(*db_, sql);
               if(query.executeStep())
               {
                   fallocateSize = query.getColumn(8).getInt64();
                   db_error("get video fallocate size = %lld",fallocateSize);
                   pthread_mutex_unlock(&m_lock);
                   pthread_mutex_unlock(&m_db_lock);
                   return fallocateSize;
               }
               else{
                   db_error("get video fallocate size failed");
               }
           }
           else
           {
           }
        }
        catch(SQLite::Exception e)
        {
           db_error("SQLITE ERROR: %s", e.what());
        }
        pthread_mutex_unlock(&m_lock);
        pthread_mutex_unlock(&m_db_lock);
        db_error("get last video file fallocateSize =  %lld",fallocateSize);
        return fallocateSize;
}

void MediaFileManager::GetLastFileByType(const std::string &media_type,int p_CamId,std::string &filename)
{
    int ret = -1;
    bool order = 0;
    bool m_force = 1;
    string sort;
    string filter;
    int LockStatus =-1;
    StorageManager *sm = StorageManager::GetInstance();
    sort = order?"desc":"asc";
    if(m_force)
    {
       if( media_type.empty())
           filter = " order by time " + sort;
       else
           filter = "where type= '" + media_type+"'"+" order by time " + sort;
    }
    else
    {
       if( media_type.empty())
           filter = "where Lock=0 order by time " + sort;
       else
           filter = "where type= '" + media_type+ "'"+" and Lock=0"+" order by time " + sort;
    }
    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
    pthread_mutex_lock(&m_db_lock);
    pthread_mutex_lock(&m_lock);
    if (db_ == NULL)
    {
       pthread_mutex_unlock(&m_lock);
       pthread_mutex_unlock(&m_db_lock);
       return;
    }
    try{
       if( 0 == p_CamId || 1 == p_CamId)
       {
           SQLite::Statement query(*db_, sql);
           if(query.executeStep())
           {
               filename = query.getColumn(0).getString();
               ret = 0;//find the media_type
           }
           else
               ret = NULL; //no media_type you want to find
       }
       else
       {

       }
    }
    catch(SQLite::Exception e)
    {
       db_error("SQLITE ERROR: %s", e.what());
    }
    pthread_mutex_unlock(&m_lock);
    pthread_mutex_unlock(&m_db_lock);
    if (filename.size() == 0)
    {
       db_error("no file found in database yet!!! p_CamId:%d ret:%d",p_CamId,ret);
       return;
    }
    db_error("filename %s",filename.c_str());
}

int8_t MediaFileManager::DeleteFileByOrder(const string &media_type, bool order, int p_CamId,bool m_force)
{
    int ret = -1;
    string sort;
    string filter;
    string file_name;
    int LockStatus =-1;
    StorageManager *sm = StorageManager::GetInstance();
    sort = order?"desc":"asc";
    if(m_force)
    {
    	if( media_type.empty())
			filter = " order by time " + sort;
		else
			filter = "where type= '" + media_type+"'"+" order by time " + sort;
    }
    else
    {
    	if( media_type.empty())
			filter = "where Lock=0 order by time " + sort;
		else
	   		filter = "where type= '" + media_type+ "'"+" and Lock=0"+" order by time " + sort;
    }
    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try{
		if( 0 == p_CamId || 1 == p_CamId)
	    {
	    	SQLite::Statement query(*db_, sql);
			if(query.executeStep())
			{
				file_name = query.getColumn(0).getString();
				ret = 0;//find the media_type
			}
			else
				ret = -1;//no media_type you want to find
		}
		else
	    {
            #if 0
	    	SQLite::Statement query(*m_BackCam_db, sql);
			if(query.executeStep())
			{
				file_name = query.getColumn(0).getString();
				if(query.getColumn(5).getInt() == 1 )
					StorageManager::GetInstance()->setLockFileCapacity(-query.getColumn(4).getInt64());
				ret = 0;//find the media_type
			}
			else
				ret = -1;//no media_type you want to find 
            #endif
	    }
    }
	catch(SQLite::Exception e)
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    if (file_name.size() == 0)
	{
        db_error("no file found in database yet!!! p_CamId:%d ret:%d",p_CamId,ret);
        return ret;
    }
    db_error("delete file name %s",file_name.c_str());
    ret = this->DeleteFileByName(file_name, p_CamId);

#if 0
	FILE_INFO FileInfo;
	bzero(&FileInfo, sizeof(FILE_INFO));
	std::string FileName = getFileReallyName(file_name);
	if( !FileName.empty() )
	{
		strncpy(FileInfo.file_name, FileName.c_str(), sizeof(FileInfo.file_name));
		DataManager::GetInstance()->popFromFileMap(FileInfo);
	}
#endif

    return ret;
}

int8_t MediaFileManager::DeleteLastFile(const string &media_type, int cnt, int p_CamId,bool m_force)
{
    int ret = -1;
    for (int i = 0; i < cnt; i++)
        ret = this->DeleteFileByOrder(media_type, false, p_CamId,m_force);

    return ret;
}

std::string MediaFileManager::getlockFileName()
{
    db_warn("getlockFileName m_lockFileName is :%s",m_lockFileName.c_str());
   // pthread_mutex_lock(&m_db_lock);
    return m_lockFileName;
}

std::string MediaFileManager::getlocThumbPickFileName()
{
    db_warn("getlockFileName m_lockthumbPicFileName is :%s",m_lockthumbPicFileName.c_str());
    return m_lockthumbPicFileName;
}


int8_t MediaFileManager::AddFile(const MediaFile &file, int p_CamId, int64_t keyValue ,bool lock_flag)
{
	pthread_mutex_lock(&m_db_lock);
    if (db_ == NULL || db_inited_ == false)
	{
    	db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
        return -1;
    }
	pthread_mutex_unlock(&m_db_lock);

    if (db_stop_update_)
	{
    	db_warn("database is not ready yet!");
        return -1;
    }
	string OrderId;
	OrderId.clear();
	if( !m_orderId.empty() )
		OrderId = m_orderId;
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		db_warn("data base is null! db_: db_inited:%d",db_,db_inited_);
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	if( 0 == p_CamId || 1 == p_CamId)
    {
    	SQLite::Statement stm(*db_, "INSERT INTO " DATA_TABLE " VALUES (?, ?, ?, ?, ?, ?, ?,?,?,?)");
    	pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		
		return AddFile(file, stm, -1, p_CamId,keyValue,OrderId,lock_flag);
	}
	else
    {
        #if 0
    	SQLite::Statement stm(*m_BackCam_db, "INSERT INTO " DATA_TABLE " VALUES (?, ?, ?, ?, ?, ?, ?, ?,?,?)");	
    	pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return AddFile(file, stm, -1, p_CamId,keyValue,OrderId);
        #endif
	}
}

int8_t MediaFileManager::AddFile(const MediaFile &file, SQLite::Statement &stm, int count, int p_CamId, int64_t keyValue,std::string order_Num,bool lock_flag)
{
    int ret = -1;
    int total_cnt = 0;
    string media_type;
    string info = "normal";
    string sql_str;
	string filetoadd;
	int filetypex = 0;
	pthread_mutex_lock(&m_db_lock);
	string media_ths_file;
    if (db_ == NULL || db_inited_ == false)
	{
    	db_warn("data base is null! db_:%p back_db:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
        return -1;
    }
	pthread_mutex_unlock(&m_db_lock);
    if (db_stop_update_)
	{
    	db_warn("database is not ready yet!");
        return -1;
    }
	filetypex = file.GetMediaType();
	switch(file.GetMediaType())
	{
		case PHOTO_A:
			media_type = "photo_A";
			
            #ifdef PHOTO_NO_TO_SQL
            return 0;
            #endif
			break;
		case PHOTO_B:
			media_type = "photo_B";
            #ifdef PHOTO_NO_TO_SQL
            return 0;
            #endif
			break;
		case VIDEO_A:
            if(lock_flag){
            media_type = "videoA_SOS";
            }else{
			media_type = "video_A";
            }
			break;
		case VIDEO_B:
			if(lock_flag){
            media_type = "videoB_SOS";
            }else{
			media_type = "video_B";
            }
			break;
		case VIDEO_A_SOS:
			media_type = "videoA_SOS";
			break;
		case VIDEO_B_SOS:
			media_type = "videoB_SOS";
			break;
        case VIDEO_A_PARK:
			media_type = "videoA_PARK";
			break;
		case VIDEO_B_PARK:
			media_type = "videoB_PARK";
			break;
		case AUDIO_A:
			media_type = "audio_A";
			break;
		case AUDIO_B:
			media_type = "audio_B";
			break;
	}
    if (count < 0)
        total_cnt = GetMediaFileCnt("");
    else
        total_cnt = count;
    if (total_cnt >= DATABASE_RESERVED_RECORD_CNT)
	{
        if (db_update_done_)
		{
        	db_warn("database is full, oldest record will be removed and new file will be added");
            DeleteLastFile(media_type, 1);
        }
		else
		{
            return -1;
        }
    }
    try
	{
        // Insert query
        // Bind the blob value to the first parameter of the SQL query
        //db_error("AddFile :%s", file.GetMediaFileName().c_str());
		
        unsigned long long fileSize = (unsigned long long)file.GetFileSize();
        stm.reset();
		
        if(lock_flag &&((file.GetMediaType() == VIDEO_A) || (file.GetMediaType() == VIDEO_B)))
        {	// 加锁的
            std::string s = file.GetMediaFileName();
			
            db_warn("before replace the video file name is %s",s.c_str());	// /mnt/extsd/video/20190718_135403.mp4
            std::string s1;
            std::string s2;
            int index = s.find("video");
            if(index == -1){
               db_warn("can't find video string");
            }else{
                s1 = s.replace(index,5,"event");
                db_warn("after replace the video file name  is %s",s1.c_str());	// /mnt/extsd/event/20190718_135403.mp4
                index = s1.find(".");
				#ifdef VIDEOTYPE_MP4
				s2 = s1.replace(index,4,"_SOS.mp4");	//(size_t pos, size_t len, const string& str) 用str替换指定字符串从起始位置pos开始长度为len的字符
				#else
                s2 = s1.replace(index,3,"_SOS.ts");
				#endif
                m_lockFileName = s2;
                db_warn("after replace the video file name  is %s",s2.c_str());	// /mnt/extsd/event/20190718_135403_SOS.mp4
            }
            stm.bind(1, s2.c_str());
			filetoadd = s2;
            //rename video ths pic file 
            s = file.GetVideoThumbPicFileName();
            db_warn("before replace the video thumb pic file name is %s",s.c_str());	// /mnt/extsd/video/20190718_135403_ths.jpg
            index = s.find("video");
            if(index == -1){
               db_warn("can't find thumb pic video string");
            }else{
                s1 = s.replace(index,5,"event");
                db_warn("after replace the video thumb pic file name  is %s",s1.c_str());	// /mnt/extsd/event/20190718_135403_ths.jpg
                index = s1.find("_ths");
                s2 = s1.replace(index,12,"_SOS_ths.jpg");
                m_lockthumbPicFileName = s2;
                db_warn("after replace the video thumb pic file name  is %s",s2.c_str());	// /mnt/extsd/event/20190718_135403_SOS_ths.jpg
				media_ths_file = s2;	// /mnt/extsd/event/20190718_131805_ths_SOS.jpg
            }
            
        }else
        {	// 普通/park,  开机扫描媒体文件
        	//db_error("nor / park");
			filetoadd = file.GetMediaFileName();
			if ((file.GetMediaType() == PHOTO_A) || (file.GetMediaType() == PHOTO_B)) {
				media_ths_file = file.GetBaseName() + THUMB_SUFFIX + PHOTO_SUFFIX;
			} else {
				media_ths_file = file.GetVideoThumbPicFileName();
			}
			stm.bind(1, file.GetMediaFileName());
			
        }
		#if 0
		string ss;
		GetMediaFileNameTime(file.GetMediaFileName(), ss);
		string thumb_pic;
		thumb_pic= file.GetVideoThumbPicFileName();
		//db_error("touch thumb file: %s",thumb_pic.c_str());		// 20190711_174153_ths.jpg
		char buff[128];
		if (thumb_pic.length() && access(thumb_pic.c_str(),0)==0) {
			sprintf(buff,"touch -c -d \"%s\" %s\n",ss.c_str(),thumb_pic.c_str());
			db_error("%s %s",buff,thumb_pic.c_str());
			system(buff);
		}
		if (file.GetMediaFileName().length() && access(file.GetMediaFileName().c_str(),0)==0) {
			sprintf(buff,"touch -c -d \"%s\" %s\n",ss.c_str(),file.GetMediaFileName().c_str());
			system(buff);
			db_error("%s %s",buff,file.GetMediaFileName().c_str());
			//db_error("touch media file: %s",file.GetMediaFileName().c_str());
		}
		#endif
		#if 1	// hide thumb file
		//db_error("try hide file: %s",media_ths_file.c_str());
		if (HideFile(media_ths_file.c_str(),1)==0) {
			//db_msg("hide thumb file: %s OK",media_ths_file.c_str());
		} else {
			//db_warn("hide thumb file: %s fail",media_ths_file.c_str());
		}
		#endif
		//db_msg("AddFile the keyValue = %lld",keyValue);
        stm.bind(2, (const long long)file.GetFileTime());
        stm.bind(3, media_type);
        stm.bind(4, info);
        stm.bind(5, (const long long)fileSize);
		stm.bind(6, (const long long)0);
		stm.bind(7, (const long long)0);
		stm.bind(8, (const long long)0);
		stm.bind(9, (const long long)keyValue);//fallocate size
		stm.bind(10, order_Num);//order number
        ret = stm.exec();
		//db_warn("add file to db: %s",filetoadd.c_str());
    }
	catch(SQLite::Exception e)
	{
        db_error("SQLITE ERROR: %s", e.what());
        if (string(e.what()).find("UNIQUE") >= 0)
		{
            return 0;
        }
        return -1;
    }
	#if 0
	FILE_INFO FileInfo;
	bzero(&FileInfo, sizeof(FILE_INFO));
	std::string FilePath, FileName;
	file.getFileReallyName(FileName);
	file.getFilePath(FilePath);
	strncpy(FileInfo.file_name, FileName.c_str(), sizeof(FileInfo.file_name));
	FileInfo.creat_time = file.GetFileTime();
	strncpy(FileInfo.file_path, FilePath.c_str(), sizeof(FileInfo.file_path));
	FileInfo.file_size = (int)file.GetFileSize();
	FileInfo.file_type = file.getFileType();
	strncpy(FileInfo.orderId, order_Num.c_str(), order_Num.length());
	if( p_CamId == 0)
	{
		FileInfo.shoot_type = 1;
	}
	else
	{
		FileInfo.shoot_type = 0;
	}
   
	DataManager::GetInstance()->pushToFileMap(FileInfo);
    #endif
    return ret;
}

static int file_filter(const struct dirent* dir)
{
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    stat(dir->d_name, &buf);
    if (!S_ISDIR(buf.st_mode))
	{
		#ifdef VIDEOTYPE_MP4
		if (fnmatch("[0-9]*_[0-9]*[FR].mp4", dir->d_name, FNM_CASEFOLD) == 0 ||
		#else
		if (fnmatch("[0-9]*_[0-9]*[FR].ts", dir->d_name, FNM_CASEFOLD) == 0 ||
		#endif
            fnmatch("[0-9]*_[0-9]*[FR].jpg", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*[FR].aac", dir->d_name, FNM_CASEFOLD) == 0 ||
        #ifdef VIDEOTYPE_MP4    
			fnmatch("[0-9]*_[0-9]*[FR]_SOS.mp4", dir->d_name, FNM_CASEFOLD) == 0)
		#else
            fnmatch("[0-9]*_[0-9]*[FR]_SOS.ts", dir->d_name, FNM_CASEFOLD) == 0)
        #endif
            return 1;
    }
    return 0;
}

static int video_file_filter(const struct dirent* dir)
{
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    stat(dir->d_name, &buf);
    if (!S_ISDIR(buf.st_mode))
	{
		#ifdef VIDEOTYPE_MP4
		#ifndef USE_CAMB
		
		if (fnmatch("[0-9]*_[0-9]*.mp4", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*_SOS.mp4", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*_PARK.mp4", dir->d_name, FNM_CASEFOLD) == 0 )
		{
			//db_error("dir->d_name: %s",dir->d_name);
			return 1;
        }
		#else
		if (fnmatch("[0-9]*_[0-9]*[FR].mp4", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*[FR]_SOS.mp4", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*[FR]_PARK.mp4", dir->d_name, FNM_CASEFOLD) == 0 )
			return 1;
		#endif
		#else
		#ifndef USE_CAMB
		if (fnmatch("[0-9]*_[0-9]*.ts", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*_SOS.ts", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*_PARK.ts", dir->d_name, FNM_CASEFOLD) == 0 )
		return 1;
		#else
		if (fnmatch("[0-9]*_[0-9]*[FR].ts", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*[FR]_SOS.ts", dir->d_name, FNM_CASEFOLD) == 0 ||
            fnmatch("[0-9]*_[0-9]*[FR]_PARK.ts", dir->d_name, FNM_CASEFOLD) == 0 )
		return 1;
		#endif
		#endif
    }
    return 0;
}

static int photo_file_filter(const struct dirent* dir)
{
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    stat(dir->d_name, &buf);
    if (!S_ISDIR(buf.st_mode))
	{
		#ifdef USE_CAMB
		if (fnmatch("[0-9]*_[0-9]*[FR].jpg", dir->d_name, FNM_CASEFOLD) == 0)
			return 1;
		#else
		
		if (fnmatch("[0-9]*_[0-9]*.jpg", dir->d_name, FNM_CASEFOLD) == 0) {
			if (strstr(dir->d_name,"_ths")) {		// thumb file
				return 0;
			} else {
				return 1;
			}
		}
		#endif
    }
    return 0;
}

bool MediaFileManager::getNewDbCreateFlag(int P_CamID)
{
	if(P_CamID ==0 )
		return db_new_create_;
	else
		return db_new_create_;
}


uint32_t MediaFileManager::getFilefallocateSize(const std::string p_DirName)
{

	char name[256] = {0};
	snprintf(name, sizeof(name), "du -a %s | sed -n '/.*mp4/'p | cut -f1 > /tmp/fallocate.txt", p_DirName.c_str());
	system(name);
	//snprintf(name, sizeof(name), "du -a %s | sed -n '/.*mp4/'p | cut -f2 > /tmp/filename.txt", p_DirName.c_str());
    //system(name);

    m_fallocateSize.clear();
    ifstream fin("/tmp/fallocate.txt");
    if(!fin)
    {
        db_error("some this is wrong, fin filed");
    }else
    {
        std::string fallocatesize;
        long long size = 0;
        while(getline(fin,fallocatesize))
        {
            size = std::atoll(fallocatesize.c_str()); //将字符串转换成长整型
           // db_msg("size = %lld",size);
            m_fallocateSize.push_back(size);
           // db_msg("read from fallocate.txt %s size = %lld",fallocatesize.c_str(),size);
        }
        fin.close();
        fin.clear();
    }
#if 0
    ifstream fin1("/tmp/filename.txt");
    if(!fin1)
    {
        db_error("some this is wrong, fin filed");
    }else
    {
        std::string filename;
        while(getline(fin1,filename))
        {
            db_msg("read from filename.txt %s",filename.c_str());
        }
        fin1.close();
        fin1.clear();
    }
#endif
	return 0;
}



void MediaFileManager::DoDBUpdateByCamIdEx(std::string p_scanPath,MediaFileManager *self,int P_CamID, int p_IsVideo)
{
    struct dirent **namelist = NULL;

	int ret;
	if( p_IsVideo )
		ret = scandir(p_scanPath.c_str(), &namelist, video_file_filter, alphasort);
	else
		ret = scandir(p_scanPath.c_str(), &namelist, photo_file_filter, alphasort);

 //   db_warn("p_scanPath:%s total file count: %d", p_scanPath.c_str(), ret);
    if (ret < 0) 
	{
        db_error("scan dir failed, %s", strerror(errno));
    }
	else
	{	
        getFilefallocateSize(p_scanPath);
		if(getNewDbCreateFlag(P_CamID))
		{
			for (int i = 0; i < ret; i++)
			{
				if( db_stop_update_ )
				{
					db_warn("something happend stop update db");
					return ;
				}

				string full_name = p_scanPath + namelist[i]->d_name;

				if(P_CamID == 0) //ignore recording file
				{
					Recorder *rec = CamRecCtrl::GetInstance()->GetRecorder(P_CamID,0);
					if( rec != NULL)
					{
						if( rec->RecorderIsBusy() )
						{
							std::string rec_fileName;
							rec->GetRecordingFileName(rec_fileName);
							if( full_name == rec_fileName )
								continue;
						}
					}
				}
				if(IsFileExist(full_name,P_CamID, 1) != true)
				{
					SQLite::Statement stm(*self->getDBHandle(P_CamID), "INSERT INTO " DATA_TABLE " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?,?)");
					MediaFile file(full_name);
                    long long size = 0;
                    if(p_IsVideo)
                    {
                        size = m_fallocateSize.at(i);
                       // db_error("add file full name : %s  fallocatesize = %lld",full_name.c_str(),size);
                    }
					if(self->AddFile(file, stm,0,0,size*1024)<0)
					{
						db_error("add file : %s  fail",full_name.c_str());
						free(namelist[i]);
		                continue;
					}
				}
				if( setCheckFlag(P_CamID,1,full_name) < 0 )
				{
					db_stop_update_ = true;
					free(namelist);
					return ;
				}
				
			}
            m_fallocateSize.clear();
			free(namelist);

			int ret_db = getFileCount(P_CamID);
			if(ret != ret_db)
			{
				db_warn("some file would be delete or insert: ret:%d ret_db:%d", ret, ret_db);
				if(deleteFileFromDB(P_CamID) < 0)
				{
					db_error("delete file from db fail");
					return ;
				}
			}					
		}
		else
		{
	        try
			{
				pthread_mutex_lock(&m_db_lock);
				pthread_mutex_lock(&m_lock);
				if (db_ == NULL)
				{
					pthread_mutex_unlock(&m_lock);
					pthread_mutex_unlock(&m_db_lock);
					return;
				}

	            self->getDBHandle(P_CamID)->exec("PRAGMA synchronous = OFF");
	            SQLite::Transaction transaction(*self->getDBHandle(P_CamID));
	            SQLite::Statement stm(*self->getDBHandle(P_CamID), "INSERT INTO " DATA_TABLE " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?,?)");
				pthread_mutex_unlock(&m_lock);
				pthread_mutex_unlock(&m_db_lock);

	            for (int i = 0; i < ret; i++)
				{
					if( db_stop_update_ )
					{
						db_warn("something happend stop update db");
						return ;
					}

	                string full_name = p_scanPath + namelist[i]->d_name;
	                MediaFile file(full_name);
	                if (self->AddFile(file, stm, i, 0) < 0)
					{
	                    free(namelist[i]);
	                    break;
	                }
					if( setCheckFlag(P_CamID,1,full_name) < 0 )
					{
						db_stop_update_ = true;
						free(namelist);
						return ;
					}
	            }
				pthread_mutex_lock(&m_lock);
	            free(namelist);
	            transaction.commit();
				pthread_mutex_unlock(&m_lock);
	        }
			catch(SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
	        }
	  }
    }
	
}




void MediaFileManager::DoDBUpdateByCamId(MediaFileManager *self,int P_CamID)
{
		if( db_stop_update_ )
	{
		db_warn("something happend stop update db P_CamID:%d",P_CamID);
		return ;
	}

    string media_dir_path ;
	media_dir_path= DIR_2CAT(MOUNT_PATH,VIDEO_DIR_A);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID ,true);
#ifdef USE_CAMB
	media_dir_path= DIR_2CAT(MOUNT_PATH,VIDEO_DIR_B);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,true);
#endif	
	media_dir_path= DIR_2CAT(MOUNT_PATH,EVENT_DIR_A);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,true);
#ifdef USE_CAMB	
	media_dir_path= DIR_2CAT(MOUNT_PATH,EVENT_DIR_B);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,true);
#endif
    media_dir_path= DIR_2CAT(MOUNT_PATH,PARK_DIR_A);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,true);
#ifdef USE_CAMB
	media_dir_path= DIR_2CAT(MOUNT_PATH,PARK_DIR_B);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,true);
#endif
	media_dir_path= DIR_2CAT(MOUNT_PATH,PHOTO_DIR_A);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,false);
#ifdef USE_CAMB
    media_dir_path= DIR_2CAT(MOUNT_PATH,PHOTO_DIR_B);
	DoDBUpdateByCamIdEx(media_dir_path, self, P_CamID,false);
#endif
}
	
void *MediaFileManager::DoDBUpdate(void *arg)
{
	prctl(PR_SET_NAME, "DoDBUpdate", 0, 0, 0);
	MediaFileManager* self = reinterpret_cast<MediaFileManager*>(arg);

	if( !self->m_IgnoreScan)
	{
	    self->db_update_done_ = false;
	    self->clearCheckFlag(0);
	    self->clearCheckFlag(1);
	    self->DoDBUpdateByCamId(self,0);
	 //   self->DoDBUpdateByCamId(self,1);
	}

    if (self->db_stop_update_)
	{
        db_info("database update has been interrupted");
    }
	else
	{
        db_error("database update finished");
        self->Notify((MSG_TYPE)MSG_DATABASE_UPDATE_FINISHED);
    }

    self->db_update_done_ = true;
	self->m_IgnoreScan = false;

  //  pthread_join(self->DB_updata_thread_id,NULL);
    self->DB_updata_thread_id = 0;
	return NULL;
}

int8_t MediaFileManager::DBUpdate()
{
    if(DB_updata_thread_id == 0){
    	//ThreadCreate(&DB_updata_thread_id,NULL,MediaFileManager::DoDBUpdate, this);
        //pthread_detach(DB_updata_thread_id);

        pthread_create(&DB_updata_thread_id, NULL, MediaFileManager::DoDBUpdate, this);
		pthread_join(DB_updata_thread_id,NULL);
    }
    return 0;
}

int8_t MediaFileManager::RemoveFile(const std::string &filename)
{
    return this->DeleteFileByName(filename);
}

void MediaFileManager::Update(MSG_TYPE msg, int p_CamID, int p_recordId)
{
	db_msg("handle msg:%d", msg);
    switch (msg) {
        case MSG_STORAGE_FORMAT_FINISHED:
			db_backCam_new_create_ = false;
			db_new_create_ = false;
			db_inited_ = false;
			m_IgnoreScan = true;
            DB_updata_thread_id = 0;
            DB_init_thread_id = 0;
            break;
        case MSG_STORAGE_MOUNTED:
			//FileLockManager::GetInstance()->SetCheckThreadStatus(true);
            this->DBReset();
            break;
        case MSG_STORAGE_UMOUNT:
            db_stop_update_ = true;
            db_inited_ = false;
			m_IgnoreScan = false;
            DB_updata_thread_id = 0;
            DB_init_thread_id = 0;
			//DataManager::GetInstance()->ClearMap();
			pthread_mutex_lock(&m_db_lock);
		 	if(db_ != NULL)
            {
				CamRecCtrl::GetInstance()->StopAllRecord();
				//FileLockManager::GetInstance()->SetCheckThreadStatus(false);
				//FileLockManager::GetInstance()->WaitCheckThreadStop();
                delete db_;
                db_ = NULL;
            }
            #if 0
			if( m_BackCam_db)
			{
				delete m_BackCam_db;
				m_BackCam_db = NULL;
			}
            #endif
            db_error("UMOUNT");
			pthread_mutex_unlock(&m_db_lock);
            break;
		case MSG_RECORD_START:
		case MSG_RECORD_STOP:
		case MSG_TAKE_THUMB_VIDEO:
		case MSG_RECORD_FILE_DONE:
		case MSG_STORAGE_CAP_NO_SUPPORT:
			Notify(msg,p_CamID,p_recordId);
			break;
        default:
            break;
    }
}

string MediaFileManager::GetThumbPicName(const string &file_name,int p_CamId)
{
    string result_alias_bak;
	std::string result_alias;
    result_alias_bak = file_name.c_str();
	string::size_type rc = result_alias_bak.rfind(".");
	if( rc == string::npos)
	{
		db_warn("invalid fileName:%s",result_alias_bak.c_str());
		return "";
	}
    string type = MediaFileManager::GetInstance()->GetMediaFileType(file_name,p_CamId);
    db_msg("file type %s",type.c_str());
    if(type == "videoA_SOS" || type == "videoB_SOS"){
        result_alias = result_alias_bak.substr(0,rc-strlen("_SOS"));
        result_alias += "_SOS_ths.jpg";
    } else if(type == "videoA_PARK" || type == "videoB_PARK") {
        result_alias = result_alias_bak.substr(0,rc-strlen("_PARK"));
        result_alias += "_PARK_ths.jpg";
    } else {
        result_alias= result_alias_bak.substr(0,rc);
        result_alias += "_ths.jpg";
    }
    db_error("thumb pic name:%s",result_alias.c_str());
	return result_alias;
}


string MediaFileManager::GetThumbVideoName(const string &file_name)
{
    string result_alias_bak;
	std::string result_alias;
    result_alias_bak = file_name.c_str();
	string::size_type rc = result_alias_bak.rfind(".");
	if( rc == string::npos)
	{
		db_warn("invalid fileName:%s",result_alias_bak.c_str());
		return "";
	}

    result_alias= result_alias_bak.substr(0,result_alias_bak.rfind("."));
    result_alias += "_ths.mp4";
    db_msg("thumb pic name:%s",result_alias.c_str());
	return result_alias;
}

bool MediaFileManager::IsFileExist(const string &p_FileName,int p_CamId, int p_CountCapacity)
{
	string filter;
    string file_name;
    bool ret = true;
    if (p_FileName.empty())
	{
    	db_warn("file name is empty");
		return false;
    }
	pthread_mutex_lock(&m_db_lock);

    if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
        return false;
    }
	pthread_mutex_unlock(&m_db_lock);

    filter = "where file='" + p_FileName + "'";
    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock); //A
	pthread_mutex_lock(&m_lock);//B
	if (db_ == NULL)
	{
		db_warn("data base is null! db_:%pdb_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return false;
	}
	try
	{
		ret= false;
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			if(query.executeStep())
			{
				ret= true;
				if( p_CountCapacity )
				{
					FILE_INFO FileInfo;
					bzero(&FileInfo, sizeof(FILE_INFO));
					std::string FilePath, FileName;
					FileName = getFileReallyName(p_FileName);
					FilePath = getFilePath(p_FileName);
					if( !FileName.empty())
						strncpy(FileInfo.file_name, FileName.c_str(), sizeof(FileInfo.file_name));
					if( !FilePath.empty() )
						strncpy(FileInfo.file_path, FilePath.c_str(), sizeof(FileInfo.file_path));
					FileInfo.creat_time = query.getColumn(1).getInt64();
					FileInfo.file_size = query.getColumn(4).getInt64();
					FileInfo.file_type = getFileType(query.getColumn(2).getString());
					strncpy(FileInfo.orderId, query.getColumn(9).getString().c_str(), sizeof(FileInfo.orderId));
					if( p_CamId == 0)
						FileInfo.shoot_type = 1;
					else
						FileInfo.shoot_type = 0;

                    #if 0
					DataManager::GetInstance()->pushToFileMap(FileInfo);
                    #endif
				}
			}
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
		    if(query.executeStep())
			{
				if( p_CountCapacity )
				{
					StorageManager::GetInstance()->setLockFileCapacity(query.getColumn(4).getInt64());	
					FILE_INFO FileInfo;
					bzero(&FileInfo, sizeof(FILE_INFO));
					std::string FilePath, FileName;
					FileName = getFileReallyName(p_FileName);
					FilePath = getFilePath(p_FileName);
					if( !FileName.empty())
						strncpy(FileInfo.file_name, FileName.c_str(), sizeof(FileInfo.file_name));
					if( !FilePath.empty() )
						strncpy(FileInfo.file_path, FilePath.c_str(), sizeof(FileInfo.file_path));
					FileInfo.creat_time = query.getColumn(1).getInt64();
					FileInfo.file_size = query.getColumn(4).getInt64();
					FileInfo.file_type = getFileType(query.getColumn(2).getString());					
					strncpy(FileInfo.orderId, query.getColumn(9).getString().c_str(), sizeof(FileInfo.orderId));
					if( p_CamId == 0)
						FileInfo.shoot_type = 1;
					else
						FileInfo.shoot_type = 0;
					
					DataManager::GetInstance()->pushToFileMap(FileInfo);
				}

				ret= true;
		    }
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

int  MediaFileManager::SetFileLockStatus(const string &p_FileName, int p_LockStatus,int p_CamId)
{
	string filter;
	string file_name;
	char buf[64]={0};
    snprintf(buf,sizeof(buf),"SET Lock=%d ",p_LockStatus);
	int ret = -1;
	if (p_FileName.empty())
	{
		db_error("file name is empty");
		return ret;
	}

	int LockStatus = -1;
	GetFileLockStatus(p_FileName, &LockStatus, p_CamId);
	if( p_LockStatus == LockStatus)
		return 0;

	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p back_db:%p db_inited:%d",db_, m_BackCam_db, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);

	filter = "where file='" + p_FileName + "'";

	string sql = "UPDATE ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(buf);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		ret= -1;
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			if(!query.executeStep())
				ret= 0;	
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			if(query.executeStep())
			{
				if( p_LockStatus )
					StorageManager::GetInstance()->setLockFileCapacity(query.getColumn(4).getInt64());
				else
					StorageManager::GetInstance()->setLockFileCapacity(-query.getColumn(4).getInt64());
					
				ret= 0;
			}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}
int  MediaFileManager::GetFileLockStatus(const string &p_FileName, int *p_LockStatus,int p_CamId)
{
	string filter;
	string file_name;
	int ret = -1;
	if (p_FileName.empty())
	{
		db_warn("file name is empty");
		return ret;
	}
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);
	
	filter = "where file='" + p_FileName + "'";
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		ret= -1;
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			if(query.executeStep())
			{
				*p_LockStatus = query.getColumn(5).getInt();
				ret= 0;
			}
		}
		else
		{
            #if 0
		    SQLite::Statement query(*m_BackCam_db, sql);	
 		    if(query.executeStep())
 		    {
	 		   	*p_LockStatus = query.getColumn(5).getInt();
	 			ret= 0;
 		    }
            #endif
 		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}

	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

void MediaFileManager:: timeString2Time_t(const string &timestr,time_t *time)
{	
	int year  = atoi(timestr.substr(0,  4).c_str());
	int month = atoi(timestr.substr(4,  2).c_str());
	int day   = atoi(timestr.substr(6,  2).c_str());
	int hour  = atoi(timestr.substr(8,  2).c_str());
	int min   = atoi(timestr.substr(10, 2).c_str());
	int sec   = atoi(timestr.substr(12, 2).c_str());

	db_msg("time: %d-%d-%d, %d-%d-%d", year, month, day, hour, min, sec);

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
	db_msg("timestr: %s, reault: %ld", timestr.c_str(), *time);	
}

int MediaFileManager::GetFileList(const std::string p_startTime, const std::string p_stopTime, std::vector<string> &p_FileNameList,int p_CamId, std::string p_OrderId)
{
	string file_name;
    string filter;
    time_t p_startTime_,p_stopTime_;
    char buf[128]={0};

	p_startTime_ = (unsigned int)strtol(p_startTime.c_str(), NULL, 10);
	p_stopTime_ = (unsigned int)strtol(p_stopTime.c_str(), NULL, 10);
    snprintf(buf,sizeof(buf),"where time>=%ld and time<=%ld",p_startTime_,p_stopTime_);
    string sql = "select * from ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(buf);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
	    	    if (col.isText())
	        	{
	            	file_name = col.getString();
	            	if (!file_name.empty())
	            	{
	            		if(p_OrderId.empty())
		                	p_FileNameList.push_back(file_name);
						else
						{
							SQLite::Column col_order= query.getColumn(9);
							if( col_order.getString() == p_OrderId)
								p_FileNameList.push_back(file_name);
						}
	            	}
	        	}
	        	else
	        	{
	           	 	db_error("get wrong data type");
	        	}
	    	}
		}
		else
		{
            #if 0
            if(m_BackCam_db==NULL)
            {
                db_error("get wrong m_BackCam_db \n "); 
                return 0;
            }

			SQLite::Statement query(*m_BackCam_db, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);

	    	    if (col.isText())
	        	{
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
	            	{
	            		if(p_OrderId.empty())
		                	p_FileNameList.push_back(file_name);
						else
						{
							SQLite::Column col_order= query.getColumn(9);
							if( col_order.getString() == p_OrderId)
								p_FileNameList.push_back(file_name);
						}
	            	}
	        	}
	        	else
	        	{
	           	 	db_error("get wrong data type");
	        	}
	    	}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return 0;
}

int MediaFileManager::setFileLockTime(const std::string p_FileName, const std::string p_LockTime, int p_CamId, int p_action)
{
	string filter;
	string file_name;
	int ret = -1;
	char buf[64]={0};
	if (p_FileName.empty())
	{
		db_warn("file name is empty");
		return ret;
	}
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL  || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);

	snprintf(buf,sizeof(buf),"SET LockTime=%ld",(long int)strtol(p_LockTime.c_str(), NULL, 10));
	filter = "where file='" + p_FileName + "'";
	string sql = "UPDATE ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(buf);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		ret = -1;
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			if(!query.executeStep())
				ret= 0;	
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			   if(!query.executeStep())
				ret= 0;
             #endif
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}
int MediaFileManager:: InsertLockInfoToMap(std::map<std::string, std::string> &p_Info,const std::string &file_name,const std::string &LockTime)
{
	std::map<std::string, std::string>::iterator it;
	it = p_Info.find(file_name.c_str());
	if(it!=p_Info.end())
		return 0;
	else
	{
		p_Info.insert(make_pair(file_name.c_str(),LockTime.c_str()));
		return 1;
	}

	return 0;
}
int MediaFileManager::getFileLockInfo(std::map<std::string, std::string > &p_Info, int p_CamId)
{
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);

	string file_name;
	string filter;
	long int LockTime = -1;
	int ret = -1;
	filter = "where Lock=1";
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL )
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			while (query.executeStep())
			{
		        SQLite::Column col = query.getColumn(0);
				LockTime = query.getColumn(6).getInt();
	    	    if (col.isText())
	        	{
					ret = 0;
            		file_name = col.getString();
	            	if (!file_name.empty())
						InsertLockInfoToMap(p_Info,file_name,to_string((unsigned long long)LockTime));
	        	}
	    	}
		}
		else
		{	
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);
			while (query.executeStep())
			{
		        SQLite::Column col = query.getColumn(0);
				LockTime = query.getColumn(6).getInt();
	    	    if (col.isText())
	        	{
					ret = 0;
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
	                	InsertLockInfoToMap(p_Info,file_name,to_string((unsigned long long)LockTime));
		        }
		     }
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);

	return ret;
}

int MediaFileManager::getLockFileCount(int p_CamId)
{
	string filter;
	string file_name;
	int mCount = 0;
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p back_db:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);
	
	filter = "where Lock=1";
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			while(query.executeStep())
				mCount++;
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			  while(query.executeStep())
				mCount++;
             #endif
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return mCount;
}
int MediaFileManager::getFileCount(int p_CamId)
{
	string file_name;
	int mCount = 0;
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);

	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			while(query.executeStep())
				mCount++;
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			  while(query.executeStep())
				mCount++;
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return mCount;
}

int8_t MediaFileManager::deleteFileFromDB(int p_CamId)
{
    int ret =0;
    string filter = "WHERE checkstate=0";
    string sql = "DELETE FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if( 0 == p_CamId || 1 == p_CamId)
	{
	    if (db_ != NULL) 
		{
	        try 
			{
	            db_->exec(sql);
	        }
			catch (SQLite::Exception e)
			{
            	db_error("SQLITE ERROR: %s", e.what());
				ret = -1;
	        }
	    }
	}
	else
	{
        #if 0
	    if (m_BackCam_db != NULL) 
		{
	        try 
			{
	            m_BackCam_db->exec(sql);
	        }
			catch (SQLite::Exception e)
			{
	            db_error("SQLITE ERROR: %s", e.what());
				ret = -1;
	        }
	    }
        #endif
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
    return ret;
}
int8_t  MediaFileManager::clearCheckFlag(int P_CamID)
{
	int ret = -1;
	if( db_stop_update_)
	{
		db_warn("database is not ready yet!");
		return 0;
	}
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p back_db:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);

    string filter = "SET checkstate=0";
	string sql = "UPDATE ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == P_CamID  || 1 == P_CamID)
		{
			SQLite::Statement query(*db_, sql);
			if(!query.executeStep())
				ret= 0;	
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			if(!query.executeStep())
				ret= 0
		    #endif;
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}
int8_t  MediaFileManager::setCheckFlag(int P_CamID,int checkF,const std::string & file_name)
{
	if (file_name.empty())
	{
		db_warn("file name is empty");
	    return -1;
   	}
	int ret = 0;
	char buf[104]={0};
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);
	snprintf(buf,sizeof(buf),"SET checkstate=%d",checkF);
	string filter = "WHERE file='" + file_name + "'";
	string sql = "UPDATE ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(buf);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}

	try
	{
		if( 0 == P_CamID || 1 == P_CamID)
		{
			SQLite::Statement query(*db_, sql);
			if(!query.executeStep())
				ret= 0;	
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);	
			if(!query.executeStep())
				ret= 0;
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
		ret = -1;
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);

	return ret;
}
int  MediaFileManager::GetFileKey(const string &p_FileName,string &keystring)
{
	string filter;
	string file_name;
	int ret = -1;
	if (p_FileName.empty())
	{
		 db_warn("file name is empty");
		 return -1;
	}
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL  || db_inited_ == false)
	{
		db_warn("data base is null! db_:%p back_db:%p db_inited:%d",db_, db_inited_);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	pthread_mutex_unlock(&m_db_lock);
	filter = "where file='" + p_FileName + "'";
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL )
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		SQLite::Statement query(*db_, sql);
		if(query.executeStep())
		{
			keystring = query.getColumn(8).getString();
			pthread_mutex_unlock(&m_lock);
			pthread_mutex_unlock(&m_db_lock);
			return 0;
		}
#if 0
		SQLite::Statement query1(*m_BackCam_db, sql);	
		if(query1.executeStep())
		{
		   	keystring = query1.getColumn(8).getString();
			pthread_mutex_unlock(&m_lock);
			pthread_mutex_unlock(&m_db_lock);
			return 0;
		}
#endif
	}
	catch(SQLite::Exception e) 
	{
		db_error("SQLITE ERROR: %s", e.what());
	}
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

int  MediaFileManager::GetDeleteFileList(std::map<std::string, std::string> &file_list, int p_CamId)
{
	int count = 0;
	string file_name;
    string filter = " order by time asc ";
	string unlock_time;
    std::size_t found;
	int ret = -1;
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
    sql.append(" ");
	sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId  || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
				unlock_time = query.getColumn(6).getString();
	    	    if (col.isText())
	        	{
	        		ret =0;
	            	file_name = col.getString();
		            if (!file_name.empty())
					{
                        found = file_name.rfind('/');
                        if (found!=std::string::npos) 
                        {
                            file_name = file_name.substr(found + 1);
                        }
                        else
                        {
                            db_error("first '/' not found ! ");
                        }
						count++;
						if( count > 3 )
							break;
						if( query.getColumn(5).getInt() )
							InsertLockInfoToMap(file_list,file_name,unlock_time);
		            }
	        	}
	    	}
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
				unlock_time = query.getColumn(6).getString();
	    	    if (col.isText())
	        	{
					ret = 0;
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
	                {
                        found = file_name.rfind('/');
                        if (found!=std::string::npos) 
                        {
                            file_name = file_name.substr(found + 1);
                        }
                        else
                        {
                            db_error("first '/' not found ! ");
                        }
						count++;
						if( count > 3 )
							break;
						if( query.getColumn(5).getInt() )
		                	InsertLockInfoToMap(file_list,file_name,unlock_time);
	            	}
	        	}
	    	}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;	
}

int MediaFileManager::GetFileList(std::map<std::string, std::string> &file_list, int p_CamId)
{
	string file_name;
	string filter;
	string keystring;
	int ret = -1;
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL )
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
				keystring = query.getColumn(8).getString();
	    	    if (col.isText())
	        	{
	        		ret =0;
	            	file_name = col.getString();
		            if (!file_name.empty())
						InsertLockInfoToMap(file_list,file_name,keystring);
	        	}
	    	}
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
				keystring = query.getColumn(8).getString();
	    	    if (col.isText())
	        	{
					ret = 0;
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
	                	InsertLockInfoToMap(file_list,file_name,keystring);
	        	}
	    	}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

int MediaFileManager::GetAllInfoFileList(std::vector<MixFileInfo_>&file_list, int p_CamId)
{
	MixFileInfo_ fileinfo;
	string file_name;
	string filter;
	string keystring;
	int ret = -1;
	string sql = "SELECT * FROM ";
	sql.append(DATA_TABLE);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId  || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
			while (query.executeStep())
			{
				SQLite::Column col = query.getColumn(0);
			    if (col.isText())
			    {
	            	file_name = col.getString();
	            	if (!file_name.empty()) 
					    {
						    ret = 0;
						 	memset(&fileinfo,0,sizeof(MixFileInfo_));
							fileinfo.file_name = query.getColumn(0).getString();
							fileinfo.time_value=(const long long)query.getColumn(1).getInt64();
							fileinfo.typeStr = query.getColumn(2).getString();
							fileinfo.infoStr = query.getColumn(3).getString();
							fileinfo.sizeValue=(const long long)query.getColumn(4).getInt64();
							fileinfo.LockValue =(const long long)query.getColumn(5).getInt64();
							fileinfo.LockTimeValue = (const long long)query.getColumn(6).getInt64();
							fileinfo.CheckStateValue = (const long long)query.getColumn(7).getInt64();
							fileinfo.KeyStr =query.getColumn(8).getString();
							fileinfo.orderNumStr =query.getColumn(9).getString();
						 	file_list.push_back(fileinfo);	
			        	}
		    	   }
			 }
		}
		else
		{
            #if 0
			SQLite::Statement query(*m_BackCam_db, sql);
		    while (query.executeStep())
		    {
		        SQLite::Column col = query.getColumn(0);
	    	    if (col.isText())
	        	{
		            file_name = col.getString();
		            if (!file_name.empty()) 
					{
						ret = 0;
					 	memset(&fileinfo,0,sizeof(MixFileInfo_));
						fileinfo.file_name = query.getColumn(0).getString();
						fileinfo.time_value=(const long long)query.getColumn(1).getInt64();
						fileinfo.typeStr = query.getColumn(2).getString();
						fileinfo.infoStr = query.getColumn(3).getString() ;
						fileinfo.sizeValue=(const long long)query.getColumn(4).getInt64();
						fileinfo.LockValue =(const long long)query.getColumn(5).getInt64();
						fileinfo.LockTimeValue = (const long long)query.getColumn(6).getInt64();
						fileinfo.CheckStateValue = (const long long)query.getColumn(7).getInt64();
						fileinfo.KeyStr =query.getColumn(8).getString();
						fileinfo.orderNumStr =query.getColumn(9).getString();
					 	file_list.push_back(fileinfo);
		        	}
	    		}
			}
            #endif
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
		ret = -1;
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

int MediaFileManager::setOrderId(const std::string p_OrderId,int p_Status)
{
#if 1
	if( !p_Status )
	{
		m_orderId.clear();
	}
	else
#endif
	{
		m_orderId = p_OrderId;
	}

	m_OrderStatus = p_Status;

	return 0;
}

int MediaFileManager::getOrderId(std::string &p_OrderId)
{
    p_OrderId = m_orderId;
    return 0;
}
int MediaFileManager::getOrderIdByName(const std::string &p_FileName, std::string &p_OrderId)
{
    string filter;
    string file_name;
    string order_id;
    int ret = -1;
    if (p_FileName.empty())
    {
        db_warn("file name is empty");
        return -1;
    }

    pthread_mutex_lock(&m_db_lock);
    if (db_ == NULL  || db_inited_ == false)
    {
        db_warn("data base is null! db_:%p  db_inited:%d",db_, db_inited_);
        pthread_mutex_unlock(&m_db_lock);
        return -1;
    }
    pthread_mutex_unlock(&m_db_lock);

    filter = "where file='" + p_FileName + "'";
    string sql = "SELECT * FROM ";
    sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(filter);
    pthread_mutex_lock(&m_db_lock);
    pthread_mutex_lock(&m_lock);
    if (db_ == NULL)
    {
        pthread_mutex_unlock(&m_lock);
        pthread_mutex_unlock(&m_db_lock);
        return -1;
    }
    try
    {
        SQLite::Statement query(*db_, sql);
        if(query.executeStep())
        {
            order_id = query.getColumn(9).getString();

            p_OrderId = order_id;
            pthread_mutex_unlock(&m_lock);
            pthread_mutex_unlock(&m_db_lock);
            return 0;
        }
#if 0
        SQLite::Statement query1(*m_BackCam_db, sql);	
        if(query1.executeStep())
        {
            order_id = query1.getColumn(9).getString();

            p_OrderId = order_id;
            pthread_mutex_unlock(&m_lock);
            pthread_mutex_unlock(&m_db_lock);

            return 0;
        }
#endif
    }
    catch(SQLite::Exception e) 
    {
        db_error("SQLITE ERROR: %s", e.what());
    }
    pthread_mutex_unlock(&m_lock);
    pthread_mutex_unlock(&m_db_lock);

    return ret;
}

bool MediaFileManager::DataBaseIsAlready()
{
	pthread_mutex_lock(&m_db_lock);
	if (db_ == NULL ||  db_inited_ == false || db_update_done_== false)
	{
		pthread_mutex_unlock(&m_db_lock);
		return false;
	}
	pthread_mutex_unlock(&m_db_lock);

	return true;
}

int MediaFileManager::GetUpdateFlag(bool &p_flag)
{
	p_flag = m_needUpdate;

	return 0;
}

int MediaFileManager::SetUpdateFlag(bool p_flag)
{
	m_needUpdate = p_flag;

	return 0;
}

std::string MediaFileManager::getFileReallyName(std::string p_FileName)
{
	string::size_type rc = p_FileName.rfind("/");
	if( rc == string::npos)
		return "";
	
	return p_FileName.substr(rc+1);
}

std::string MediaFileManager::getFilePath(std::string p_FilePath)
{
	string::size_type rc = p_FilePath.rfind("/");
	if( rc == string::npos)
		return "";
	
	return p_FilePath.substr(0, rc);
}

int MediaFileManager::getFileType(std::string fileType)
{
	if( !fileType.empty())
	{
		if( strstr(fileType.c_str(), "photo") != NULL)
			return 0;
		if( strstr(fileType.c_str(), "video") != NULL)
			return 1;
		if( strstr(fileType.c_str(), "audio") != NULL)
			return 2;
	}

	return 1;
}


int MediaFileManager::SetFileInfoByName(const std::string &fileName,const std::string &fileName_dts,const std::string typeStr,int lockStatus, int p_CamId)
{
	int ret = -1;
    char buf[256]={0};
    snprintf(buf,sizeof(buf),"SET file='%s',type='%s',Lock=%d ",fileName_dts.c_str(),typeStr.c_str(),lockStatus);
    string filter;
    filter = "where file='" + fileName + "'";
	string sql = "UPDATE ";
	sql.append(DATA_TABLE);
    sql.append(" ");
    sql.append(buf);
    sql.append(" ");
    sql.append(filter);
	pthread_mutex_lock(&m_db_lock);
	pthread_mutex_lock(&m_lock);
	if (db_ == NULL)
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_db_lock);
		return -1;
	}
	try
	{
		if( 0 == p_CamId  || 1 == p_CamId)
		{
			SQLite::Statement query(*db_, sql);
	        if(!query.executeStep())
                ret=0;
		}
	}
	catch(SQLite::Exception e) 
	{
        db_error("SQLITE ERROR: %s", e.what());
		ret = -1;
    }
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_unlock(&m_db_lock);
	return ret;
}

int MediaFileManager::GetThumbFromMainPic(const std::string& src_pic,const std::string& dest_pic)
{
    int fd = 0,ths_fd = 0;
    int ret = 0;
    int jpeg_thumb_offset = 0;
    int jpeg_thumb_len = 0;
    int jpeg_size = 0;
    int src_data_size = 0;
    void *data = NULL;

    fd = open(src_pic.c_str(), O_RDWR, 0666);
    if (fd < 0) {
        db_error("open file %s failed(%s)!", src_pic.c_str(), strerror(errno));
        return -1;
    }
    ths_fd = open(dest_pic.c_str(), O_RDWR | O_CREAT, 0666);
    if (ths_fd < 0) {
        db_error("open file %s failed(%s)!", dest_pic.c_str(), strerror(errno));
        ret = -1;
        goto out2;
    }
    struct stat stFstat;
    ret = fstat(fd, &stFstat);
    if (ret < 0) {
        db_error("get file status failed(%s)!", strerror(errno));
        ret = -1;
        goto out1;
    }
    src_data_size = stFstat.st_size;
    db_msg("src_data_size %d",src_data_size);
    if(src_data_size != 0)
    {
        jpeg_size = src_data_size-(sizeof(off_t) + sizeof(size_t) + sizeof(size_t));
        ret = lseek(fd,jpeg_size,SEEK_SET);
        if (ret < 0) {
            db_error("lseek file thumb_offset failed(%s)!", strerror(errno));
            ret = -1;
            goto out1;
        }
        ret = read(fd,&jpeg_thumb_offset,sizeof(int));
        if (ret < 0) {
            db_error("read file thumb_offset failed(%s)!", strerror(errno));
            ret = -1;
            goto out1;
        }
        ret = lseek(fd,jpeg_size+sizeof(off_t),SEEK_SET);
        if (ret < 0) {
            db_error("lseek file jpeg_thumb_len failed(%s)!", strerror(errno));
            ret = -1;
            goto out1;
        }
        ret = read(fd,&jpeg_thumb_len,sizeof(int));
        if (ret < 0) {
            db_error("read file jpeg_thumb_len failed(%s)!", strerror(errno));
            ret = -1;
            goto out1;
        }
        data = malloc(jpeg_thumb_len);
        if(data == NULL)
        {
            db_error("malloc src data failed(%s)!", strerror(errno));
            ret = -1;
            goto out1;
        }
        ret = lseek(fd,jpeg_thumb_offset,SEEK_SET);
        if (ret < 0) {
            db_error("read file thumb_offset failed(%s)!", strerror(errno));
            ret = -1;
            goto out0;
        }
        ret = read(fd,data,jpeg_thumb_len);
        if (ret < 0) {
            db_error("read file jpeg_thumb_len filed(%s)!", strerror(errno));
            ret = -1;
            goto out0;
        }
        db_msg("jpeg_size %d thumb_offset %d jpeg_thumb_len %d",
                      jpeg_size,jpeg_thumb_offset,jpeg_thumb_len);
        ret = write(ths_fd, data, jpeg_thumb_len);
        if (ret < 0) {
            db_error("write data filed(%s)!", strerror(errno));
            ret = -1;
            goto out0;
        }
    } else {
        db_error("fatal! src_data_size is zero");
        ret = -1;
        goto out1;
    }

out0:
    if(data != NULL)
    {
        free(data);
    }
out1:
    close(ths_fd);
out2:
    close(fd);

    return ret;
}

int MediaFileManager::GetMediaFileNameTime(const std::string &fileName, std::string &fileNameTime)
{	// /mnt/extsd/video/20190711_175513.mp4
	// /mnt/extsd/video/20190711_175513_ths.jpg
	char bug[32];
	int rc0 = fileName.rfind(".");
	if (rc0 != string::npos) {
		int rc1 = fileName.rfind("/",rc0);
		string ss = fileName.substr(rc1+1,rc0-rc1-1);	// "20190711_180104"
		db_error("filename time: %s",ss.c_str());
		if (ss.length() < 15) {
			return -1;
		}
		string syear = ss.substr(0,4);
		string smonth = ss.substr(4,2);
		string sday = ss.substr(6,2);
		string shour = ss.substr(9,2);
		string smin = ss.substr(11,2);
		string ssecond = ss.substr(13,2);
		// 2019-07-11 19:21:37
		fileNameTime = syear+"-"+smonth+"-"+sday+" "+shour+":"+smin+":"+ssecond;
		return 0;
	}
	return -1;
}

