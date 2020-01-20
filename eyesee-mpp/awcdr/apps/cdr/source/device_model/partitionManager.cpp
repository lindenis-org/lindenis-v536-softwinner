/*
*版权声明:暂无
*文件名称:partitionManager.cpp
*创建者:陈振华
*创建日期:2018-6-8
*文件描述:对私有分区进行读写
*历史记录:无
*/

#include "partitionManager.h"
#include "common/app_log.h"
#include "dd_serv/common_define.h"
#include <queue>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "partitionManager.cpp"
#endif



#define FACTORY_SIZE        128*512
#define FACTORY_DATA_SIZE   (PRIVATE_SIZE - 8)

#define PRIVATE_SIZE        (64*1024)
#define PRIVATE_DATA_SIZE (PRIVATE_SIZE - 8)

#define PRIVATE_SIZE_SEC    (64*1024)
#define SIZE_KEY            (32)
#define SIZE_VALUE          (256)
#define SIZE_KEY_LIST       (3)



enum _part_key_
{
    eKEY_BINDFLAG = 0,
    eKEY_UPGRADE,
    eKEY_MAX,
};

typedef struct _partiton_key_node_
{
    char key[SIZE_KEY];
    char value[SIZE_VALUE];
} PartKeyNode;

typedef struct _partiton_key_list_
{
    char head[32];
    PartKeyNode partKeyNode[SIZE_KEY_LIST];
} PartKeyList;


static Mutex partitionManager_mutex;
static PartitionManager *m_partitionManager = NULL;

using namespace std;

#ifdef DEBUG_PART
extern queue<string> logRecordQue;
#endif

typedef struct tag_CRC32_DATA
{
	unsigned CRC;			
	unsigned CRC_32_Tbl[256];
}CRC32_DATA_t;

typedef struct private_data_img 
{
    unsigned int crc;
	unsigned int flag;  //if flag != '\0' mean not data
    char data[];
}private_t;

#define MAX_TEST_NUM   30

typedef struct ftest_result_
{
	
	unsigned int crc;		/* CRC32 over data bytes	*/
	unsigned int flag; 	/* if flag != '\0' mean not data */
	ftest_item fItem[MAX_TEST_NUM];
}ftest_result;


PartitionManager::PartitionManager()
{
	m_private_buff = NULL;
	m_bind_flag = 0;
	m_factory_buff = NULL;
	m_factory_init_flag = 0;
	sunxi_spinor_private_init();
	sunxi_spinor_factory_init();
}

PartitionManager::~PartitionManager()
{
	sunxi_spinor_private_exit();
	sunxi_spinor_factory_exit();
}

PartitionManager* PartitionManager::GetInstance(void)
{
	Mutex::Autolock _l(partitionManager_mutex);
	if( m_partitionManager != NULL )
		return m_partitionManager;

	m_partitionManager = new PartitionManager();
	if( m_partitionManager != NULL)
		return m_partitionManager;
	else
	{
		db_error("new partitionManager failed\n");
		return NULL;
	}
}

int PartitionManager::sunxi_spinor_private_clear()
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("sunxi_spinor_private_clear\n");
    }
#endif
	char buf[PRIVATE_SIZE];
	memset(buf, 0, PRIVATE_SIZE);
	memset(m_private_buff,0,PRIVATE_SIZE);
		
	flash_private_wirte(buf, sizeof(buf));
	
	sync();

	return 0;
}

int PartitionManager::sunxi_spinor_private_sec_get(const char *name, char *buf, unsigned int buf_len)
{
	int  ret;
	PartKeyList *p_buf_sec;
    if(! name || !buf || buf_len < 0) {
        db_error("sunxi_spinor_private_sec_get Invalid argument!");
        return -1;
    }
	if(!m_private_init_flag)
	{
		db_error("please run sunxi_spinor_private_init first to init private data \n");
		return -1;
	}

	p_buf_sec = (PartKeyList *)m_private_buff_sec;
    if(p_buf_sec == NULL) {
        db_error("Invalid p_buf_sec NULL!");
        return -1;
    }

    if(! strcmp(name, FKEY_BINDFLAG)) {
        if(! strncmp(p_buf_sec->partKeyNode[eKEY_BINDFLAG].key, FKEY_BINDFLAG, strlen(FKEY_BINDFLAG))) {
            if(buf_len >= SIZE_VALUE) {
                memcpy(buf, p_buf_sec->partKeyNode[eKEY_BINDFLAG].value, SIZE_VALUE - 1);
            }
            else {
                memcpy(buf, p_buf_sec->partKeyNode[eKEY_BINDFLAG].value, buf_len);
            }
        }
        else {
            char tmpbuf[64] = {0};
            if(sunxi_spinor_private_get(FKEY_BINDFLAG, tmpbuf, sizeof(tmpbuf)) == 0) {
                if(! strcmp(tmpbuf, "true") || ! strcmp(tmpbuf, "false")) {
                    sunxi_spinor_private_sec_set(FKEY_BINDFLAG, tmpbuf, strlen(tmpbuf));
                }
            }
        }
    }
    else if(! strcmp(name, FKEY_UPGRADEINFO)) {
        if(buf_len >= SIZE_VALUE) {
            memcpy(buf, p_buf_sec->partKeyNode[eKEY_UPGRADE].value, SIZE_VALUE - 1);
        }
        else {
            memcpy(buf, p_buf_sec->partKeyNode[eKEY_UPGRADE].value, buf_len);
        }
    }
    else if(! strcmp(name, FKEY_MAX)) {
        if(buf_len >= SIZE_VALUE) {
            memcpy(buf, p_buf_sec->partKeyNode[eKEY_MAX].value, SIZE_VALUE - 1);
        }
        else {
            memcpy(buf, p_buf_sec->partKeyNode[eKEY_MAX].value, buf_len);
        }
    }

    return 0;
}

int PartitionManager::sunxi_spinor_private_sec_set(const char *name, char *buf, unsigned int buf_len)
{
	int  ret;
	PartKeyList *p_buf_sec;
    if(! name || !buf || buf_len < 0) {
        db_error("sunxi_spinor_private_sec_set Invalid argument!");
        return -1;
    }
	if(!m_private_init_flag)
	{
		db_error("please run sunxi_spinor_private_init first to init private data \n");
		return -1;
	}

	p_buf_sec = (PartKeyList *)m_private_buff_sec;
    if(p_buf_sec == NULL) {
        db_error("Invalid p_buf_sec NULL!");
        return -1;
    }

    if(! strcmp(name, FKEY_BINDFLAG)) {
        char key[SIZE_KEY] = {0};
        char value[SIZE_VALUE] = {0};
        strncpy(key, FKEY_BINDFLAG, strlen(FKEY_BINDFLAG));
        strcat(key, "=");
        strncpy(value, buf, buf_len);
        memcpy(p_buf_sec->partKeyNode[eKEY_BINDFLAG].key, key, SIZE_KEY);
        memcpy(p_buf_sec->partKeyNode[eKEY_BINDFLAG].value, value, SIZE_VALUE);
        if(flash_private_wirte_sec(p_buf_sec->partKeyNode[eKEY_BINDFLAG].key, SIZE_KEY + SIZE_VALUE, 
                    sizeof(p_buf_sec->head) + (SIZE_KEY + SIZE_VALUE) * eKEY_BINDFLAG) != 0) {
            db_error("flash_private_wirte_sec failed!");
            return -1;
        }
    }
    else if(! strcmp(name, FKEY_UPGRADEINFO)) {
        char key[SIZE_KEY] = {0};
        char value[SIZE_VALUE] = {0};
        strncpy(key, FKEY_UPGRADEINFO, strlen(FKEY_UPGRADEINFO));
        strcat(key, "=");
        strncpy(value, buf, buf_len);
        memcpy(p_buf_sec->partKeyNode[eKEY_UPGRADE].key, key, SIZE_KEY);
        memcpy(p_buf_sec->partKeyNode[eKEY_UPGRADE].value, value, SIZE_VALUE);
        if(flash_private_wirte_sec(p_buf_sec->partKeyNode[eKEY_UPGRADE].key, SIZE_KEY + SIZE_VALUE, 
                    sizeof(p_buf_sec->head) + (SIZE_KEY + SIZE_VALUE) * eKEY_UPGRADE) != 0) {
            db_error("flash_private_wirte_sec failed!");
            return -1;
        }
    }
    else if(! strcmp(name, FKEY_MAX)) {
        char key[SIZE_KEY] = {0};
        char value[SIZE_VALUE] = {0};
        strncpy(key, FKEY_MAX, strlen(FKEY_MAX));
        strcat(key, "=");
        strncpy(value, buf, buf_len);
        memcpy(p_buf_sec->partKeyNode[eKEY_MAX].key, key, SIZE_KEY);
        memcpy(p_buf_sec->partKeyNode[eKEY_MAX].value, value, SIZE_VALUE);
        if(flash_private_wirte_sec(p_buf_sec->partKeyNode[eKEY_MAX].key, SIZE_KEY + SIZE_VALUE, 
                    sizeof(p_buf_sec->head) + (SIZE_KEY + SIZE_VALUE) * eKEY_MAX) != 0) {
            db_error("flash_private_wirte_sec failed!");
            return -1;
        }
    }

    return 0;
}

int PartitionManager::sunxi_spinor_private_get(const char *name, char *buf, unsigned int buff_len)
{
	int  ret;
	ftest_result *p_buf;
	if(!m_private_init_flag)
	{
		db_msg("please run sunxi_spinor_private_init first to init private data \n");
		return -1;
	}

	p_buf = (ftest_result *)m_private_buff;
	//get value by name
	ret = get_by_name(p_buf->fItem, name, buf, buff_len);
	if(ret)
	{
		//db_msg("can not find the %s value\n",name);
		return -1;
	}

	return 0;
}


int PartitionManager::sunxi_spinor_flash_get(const char *name, char *buf, unsigned int buff_len)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        std::string names = name;
        logRecordQue.push("sunxi_spinor_flash_get name:" + names + "\n");
    }
#endif
	int ret;
	char *buff = NULL;
	ftest_result *p_buf = NULL;
	buff = (char *)malloc(sizeof(char) * PRIVATE_SIZE);
	if(NULL == buff)
	{
		db_error("malloc error\n");
		return -1;
	}
	//read private pratation data
	ret = flash_private_read(buff, PRIVATE_SIZE);
	if(ret)
	{
		db_error("can't read the private partiton\n");
		goto ERROR_INIT;
	}

	p_buf = (ftest_result *)buff;

	ret = get_by_name(p_buf->fItem, name, buf, buff_len);
	if(ret){
		db_warn("can't get_by_name\n");
	}
	
	//checking if NULL data
ERROR_INIT:	
	if(NULL != buff){
		free(buff);
	}

	return ret;
}

int PartitionManager::sunxi_spinor_private_set_flag(int value)
{
	m_bind_flag = value;
	return 0;
}


int PartitionManager::sunxi_spinor_private_get_flag()
{
	return m_bind_flag;
}



int PartitionManager::sunxi_spinor_private_set(const char *name, const char *value)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        std::string names = name;
        std::string values = value;
        logRecordQue.push("sunxi_spinor_private_set name:" + names + "value:" + values + "\n");
    }
#endif
	if(!m_private_init_flag)
	{
	   db_msg("please run sunxi_spinor_private_init first to init private data \n");
	   return -1;
	}

	int  ret;

	ftest_result *p_buf;

	p_buf = (ftest_result *)m_private_buff;

	ret = set_by_name(p_buf->fItem, name, value);
	if(ret)
	{
		//db_msg("set %s fail.\n",name);
		return -1;
	}
	db_msg("set %s success\n",name);

	return 0;
}

int PartitionManager::flash_private_read(char *buf, unsigned int len)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("flash_private_read\n");
    }
#endif
	FILE * fp = NULL;
	fp = fopen("/dev/mtd_name/secure","r");
	if(fp == NULL)
	{
		db_error("open  fail\n");
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	fread(buf,len,1,fp);
	fclose(fp);

	return 0;
}

int PartitionManager::flash_private_wirte(char *buf, unsigned int len)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("flash_private_wirte\n");
    }
#endif
	FILE * fp = NULL;
	fp = fopen("/dev/mtd_name/secure","w");
	if(fp == NULL)
	{
		db_msg("open  fail\n");
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	fwrite(buf,len,1,fp);
	fclose(fp);

	

	return 0;
}

int PartitionManager::flash_private_read_sec(char *buf, unsigned int len)
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("flash_private_read_sec\n");
    }
#endif
	FILE * fp = NULL;
	fp = fopen("/dev/mtd_name/secure","r");
	if(fp == NULL)
	{
		db_error("open  fail\n");
		return -1;
	}
    fseek(fp, PRIVATE_SIZE, SEEK_SET);
	fread(buf,len,1,fp);
	fclose(fp);

	return 0;
}

int PartitionManager::flash_private_wirte_sec(char *buf, unsigned int len, unsigned int offset)
{
	
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("flash_private_wirte_sec\n");
    }
#endif
	FILE * fp = NULL;
	fp = fopen("/dev/mtd_name/secure","w");
	if(fp == NULL)
	{
		db_error("open  fail\n");
		return -1;
	}
    fseek(fp, PRIVATE_SIZE + offset, SEEK_SET);
	fwrite(buf,len,1,fp);
	fclose(fp);

	return 0;
}

int PartitionManager::sunxi_spinor_private_init()
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("PartitionManager::sunxi_spinor_private_init\n");
    }
#endif
	int ret;
	ftest_result *p_buf;
	PartKeyList *p_buf_sec = NULL;
	
	m_private_buff = (char *)malloc(sizeof(char) * PRIVATE_SIZE);
	if(NULL == m_private_buff)
	{
		db_msg("malloc error\n");
		return -1;
	}

	//read private pratation data
	ret = flash_private_read(m_private_buff, PRIVATE_SIZE);
	if(ret)
	{
		db_error("can't read the private partiton\n");
	    if(NULL != m_private_buff)
		    free(m_private_buff);
        return -1;
	}

	m_private_buff_sec = (char *)malloc(sizeof(char) * PRIVATE_SIZE_SEC);
	if(NULL == m_private_buff_sec)
	{
		db_error("malloc m_private_buff_sec error\n");
	    if(NULL != m_private_buff)
		    free(m_private_buff);
		return -1;
	}
	ret = flash_private_read_sec(m_private_buff_sec, PRIVATE_SIZE_SEC);
	if(ret)
	{
		db_error("can't read the private sec partiton\n");
	    if(NULL != m_private_buff)
		    free(m_private_buff);
	    if(NULL != m_private_buff_sec)
		    free(m_private_buff_sec);
        return -1;
	}

#ifdef DEBUG_PART
    char *p_tmp = (char*)malloc(sizeof(char) * PRIVATE_SIZE);
    if(NULL == p_tmp) {
        if(logRecordQue.size() < LOG_RECORD_MAX){
            logRecordQue.push("PartitionManager::sunxi_spinor_private_init malloc failed\n");
        }
    }
    else {
        memcpy(p_tmp, m_private_buff, PRIVATE_SIZE);
        for(int i = 0; i < 256; i++) {
            if(*(p_tmp + i) == '\0') {
                *(p_tmp + i) = 'x';
            }
        }
        std::string flashinfo = p_tmp;
        if(logRecordQue.size() < LOG_RECORD_MAX){
            logRecordQue.push("sunxi_spinor_private_init :" + flashinfo + "\n");
        }
        free(p_tmp);
    }

    p_tmp = NULL;
    p_tmp = (char*)malloc(sizeof(char) * PRIVATE_SIZE_SEC);
    if(NULL == p_tmp) {
        if(logRecordQue.size() < LOG_RECORD_MAX){
            logRecordQue.push("PartitionManager::sunxi_spinor_private_init malloc sec failed\n");
        }
    }
    else {
        memcpy(p_tmp, m_private_buff_sec, PRIVATE_SIZE_SEC);
        for(int i = 0; i < 32+SIZE_KEY+SIZE_VALUE; i++) {
            if(*(p_tmp + i) == '\0') {
                *(p_tmp + i) = 'x';
            }
        }
        std::string flashinfo = p_tmp;
        if(logRecordQue.size() < LOG_RECORD_MAX){
            logRecordQue.push("sunxi_spinor_private_init :" + flashinfo + "\n");
        }
        free(p_tmp);
    }
#endif
	
	//checking if NULL data
	p_buf = (ftest_result *)m_private_buff;
	p_buf_sec = (PartKeyList *)m_private_buff_sec;
	int i;
	db_error("p_buf->flag = %d \n",p_buf->flag);

	
#if 0  //
	
	if(p_buf->flag != 0x01)
	{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("PartitionManager::sunxi_spinor_private_init flag != 0x01\n");
    }
#endif
		memset(p_buf, 0x00, sizeof(ftest_result));
		
        p_buf->flag = 0x01;
        p_buf->crc = crc32(p_buf->fItem, sizeof(ftest_result)-8);
		db_error(" p_buf->crc  = 0x%x\n", p_buf->crc);

	}
#endif
	//set init_flag
	m_private_init_flag = 1;

	return 0;
	
//ERROR_INIT:
//	if(NULL != m_private_buff)
//		free(m_private_buff);
//
//	return -1;
}


int PartitionManager::sunxi_spinor_private_exit()
{
	m_private_init_flag = 0;

	if(NULL != m_private_buff)
		free(m_private_buff);
	
	return 0;
}

int PartitionManager::sunxi_spinor_private_save()
{
#ifdef DEBUG_PART
    if(logRecordQue.size() < LOG_RECORD_MAX){
        logRecordQue.push("sunxi_spinor_private_save write falsh\n");
    }
#endif
	if(!m_private_init_flag)
	{
	   db_msg("please run sunxi_spinor_private_init first to init private data \n");
	   return -1;
	}

	 ftest_result *p_buf = (ftest_result *)m_private_buff;

	//get new crc
	p_buf->crc = crc32(p_buf->fItem, sizeof(ftest_result) - 8);
    p_buf->flag = 0x01;
	int ret = flash_private_wirte((char *)m_private_buff, PRIVATE_SIZE);
	
	if(ret)
	{
		db_msg("save error\n");
		sync();
		return -1;
	}

	db_msg("update private data success \n");
	sync();

	return 0;
}

int PartitionManager::set_by_name(ftest_item *data, const char *name, const char *value)
{

    int ret = -1;
 	int i;
	if(strcmp("",name) == 0)
	{
		db_error("iitem error\n");
		return -1;
	}
	
	for(i = 0; i < MAX_TEST_NUM;i++)
	{
		if(strcmp(data[i].strItem,"") != 0 && isalnum(data[i].strItem[0]))
		{
			db_error("data[%d].strItem %s\n", i, data[i].strItem);
			if(strcmp(data[i].strItem,name) == 0)
			{

				db_error("set %d item name %s value %s", i, name,value);
				strncpy(data[i].strResult, value,255);
				
				ret = 0;
				break;
			}
		}
		else
		{
			
			strncpy(data[i].strItem, name,31);
			data[i].strItem[31] ='\0';
			strncpy(data[i].strResult, value,255);
			data[i].strResult[255] ='\0';
			db_error("set %d item name %s value %s", i, name,value);
			ret = 0;
			break;
		}
		
	}
	
    return ret;
	
}



int PartitionManager::get_by_name(ftest_item *data, const char *name, char *buf, unsigned int buff_len)
{
	int iRet = -1;
	int i; 
	if(strcmp("",name) == 0)
	{
		db_error("iitem error\n");
		return -1;
	}
	
	for(i = 0; i < MAX_TEST_NUM;i++)
	{
//		db_error("data[%d].strItem %s \n", i, data[i].strItem);
		if(strcmp(data[i].strItem,name) == 0)
		{
			
			strncpy(buf, data[i].strResult, buff_len);
			
			db_error(" FUN[%s] LINE[%d]  Find %s  value  =%s\n", __func__, __LINE__,name, buf);
			iRet = 0;	
		}
	}
	
    return iRet;

	

}


__u32 PartitionManager::crc32(void * buffer, unsigned length)
{
	__u32 i, j;
	CRC32_DATA_t crc32;
	__u32 CRC32 = 0xffffffff;

	crc32.CRC = 0;

	for( i = 0; i < 256; ++i)
	{
		crc32.CRC = i;
		for( j = 0; j < 8 ; ++j)
		{
			if(crc32.CRC & 1)
				crc32.CRC = (crc32.CRC >> 1) ^ 0xEDB88320;
			else 
				crc32.CRC >>= 1;
		}
		crc32.CRC_32_Tbl[i] = crc32.CRC;
	}

	CRC32 = 0xffffffff;
    for( i = 0; i < length; ++i)
    {
        CRC32 = crc32.CRC_32_Tbl[(CRC32^((unsigned char*)buffer)[i]) & 0xff] ^ (CRC32>>8);
    }

	return CRC32^0xffffffff;
}
 int  PartitionManager::flash_factory_read(char *buf, unsigned int len)
{
	FILE * fp = NULL;
	fp = fopen("/dev/mtd_name/private","r");
	if(fp == NULL)
	{
		db_error("open  fail\n");
		return -1;
	}
	fread(buf,len,1,fp);
	fclose(fp);

	return 0;
}

 int  PartitionManager::flash_factory_wirte(char *buf, unsigned int len)
{
	FILE * fp = NULL;
	
	fp = fopen("/dev/mtd_name/private","w");
	if(fp == NULL)
	{
		db_msg("open  fail\n");
		return -1;
	}
	fwrite(buf,len,1,fp);
	db_msg("write crc value: %d\n",*((int *)buf));
	fclose(fp);

	return 0;
}

 int  PartitionManager::factory_set_by_name(ftest_item *data, const char *name, const char *value)
 {
	  int ret = -1;
	 int i;
	 if(strcmp("",name) == 0)
	 {
		 db_msg("iitem error\n");
		 return -1;
	 }
	 for(i = 0; i < MAX_TEST_NUM;i++)
	 {
		 if(strcmp(data[i].strItem,"") != 0)
		 {
			 if(strcmp(data[i].strItem,name) == 0)
			 {
				 db_msg("iitem %d is right\n",i);
				 strncpy(data[i].strResult, value,255);
				 
				 ret = 0;
				 break;
			 }
		 }
		 else
		 {
			 db_error("new item %d\n",i);
			 strncpy(data[i].strItem, name,31);
			 data[i].strItem[31] ='\0';
			 strncpy(data[i].strResult, value,255);
			 data[i].strResult[255] ='\0';
			 
			 ret = 0;
			 break;
		 }
		 
	 }
	 
	 return ret;
 }
 
 int PartitionManager::factory_get_by_name(ftest_item *data, const char *name, char *buf, unsigned int buff_len)
 {
	 int iRet = -1;
	 int i; 
	 if(strcmp("",name) == 0)
	 {
		 db_msg("iitem error\n");
		 return -1;
	 }
	 
	 for(i = 0; i < MAX_TEST_NUM;i++)
	 {
		 if(strcmp(data[i].strItem,name) == 0)
		 {
			 strncpy(buf, data[i].strResult, buff_len);
			 
			 db_msg(" FUN[%s] LINE[%d]	Find  value  =%s\n", __func__, __LINE__, buf);
			 iRet = 0;
			 break;
		 }
	 }
	 
	 return iRet;
	 
 }

int  PartitionManager::sunxi_spinor_factory_set(const char *name, const char *value)
{
	if(!m_factory_init_flag)
	{
	   db_msg("please run sunxi_spinor_private_init first to init private data \n");
	   return -1;
	}

	int  ret;
	ftest_result *p_buf;

	p_buf = (ftest_result *)m_factory_buff;

	ret = factory_set_by_name(p_buf->fItem, name, value);
	if(ret)
	{
		db_msg("set %s fail.\n",name);
		return -1;
	}

	db_msg("set %s success\n",name);
	return 0;
}

int  PartitionManager::sunxi_spinor_factory_get(const char *name, char *buf, unsigned int buff_len)
{
	int  ret;

    ftest_result *p_buf;

    //checking if had run sunxi_spinor_private_init()
    if(!m_factory_init_flag)
    {
        db_msg("please run sunxi_spinor_private_init first to init private data \n");
        return -1;
    }

    p_buf = (ftest_result *)m_factory_buff;

    //get value by name
    ret = factory_get_by_name(p_buf->fItem, name, buf, buff_len);
    if(ret)
    {
        db_msg("can not find the %s value\n",name);
        return -1;
    }

	return 0;
}


int PartitionManager::sunxi_spinor_factory_save(void)
{
    //checking if had run sunxi_spinor_private_init()
    if(!m_factory_init_flag)
    {
        db_msg("please run sunxi_spinor_private_init first to init private data \n");
        return -1;
    }
    ftest_result *p_buf = (ftest_result *)m_factory_buff;
	
    //get new crc
    p_buf->crc = crc32(p_buf->fItem, sizeof(ftest_result) - 8);
	
    p_buf->flag = 0x01;
    //write in private partition
    int ret = flash_factory_wirte((char *)m_factory_buff, sizeof(ftest_result));
    if(ret)
    {
        db_msg("save error\n");
        sync();
        return -1;
    }

    db_msg("update private data success \n");
    sync();

    return 0;
}

int PartitionManager::sunxi_spinor_factory_init(void)
{
	int ret;
	ftest_result *p_buf;
	

	m_factory_buff = (char *)malloc(sizeof(ftest_result));
	if(NULL == m_factory_buff)
	{
		db_msg("malloc error\n");
		return -1;
	}

	//read private pratation data
	ret = flash_factory_read(m_factory_buff, sizeof(ftest_result));
	if(ret)
	{
		db_msg("can't read the private partiton\n");
		goto ERROR_INIT;
	}

	//checking if NULL data
	p_buf = (ftest_result *)m_factory_buff;
    if(p_buf->flag != 0x01)
    {
        db_msg("the private partition not data.will fotmat it\n");
        memset(p_buf, 0x00, sizeof(ftest_result));
		
        p_buf->flag = 0x01;
        p_buf->crc = crc32(p_buf->fItem, sizeof(ftest_result)-8);
		 db_msg(" p_buf->crc  = 0x%x\n", p_buf->crc);
    }

	//set init_flag
	m_factory_init_flag = 0x01;
	db_msg("sunxi_spinor_private_init over\n");

	return 0;

ERROR_INIT:
		if(NULL != m_factory_buff)
		free(m_factory_buff);
	return -1;
}

void PartitionManager::sunxi_spinor_factory_exit(void)
{
	//uninit
	m_factory_init_flag = 0x00;
	db_msg("111111sunxi_spinor_private function has exit \n");

	//free sourfe
	if(NULL != m_factory_buff)
		free(m_factory_buff);

	db_msg("sunxi_spinor_private function has exit \n");
}

