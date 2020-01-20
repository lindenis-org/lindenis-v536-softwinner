/*
*版权声明:暂无
*文件名称:partitionManager.h
*创建者:陈振华
*创建日期:2018-6-8
*文件描述:对私有分区进行读写
*历史记录:无
*/

#ifndef __PARTITION_MANAGER_H__
#define __PARTITION_MANAGER_H__

#include <linux/types.h>
#include "common/subject.h"
#include <utils/Mutex.h>
#include <pthread.h>

using namespace EyeseeLinux;


typedef struct ftest_item_
{
	char  strItem[32];
	char strResult[256];
}ftest_item;




class PartitionManager
	: public ISubjectWrap(PartitionManager)
{
	public:
		PartitionManager();
		~PartitionManager();
		static PartitionManager* GetInstance(void);

	public:
        int sunxi_spinor_private_sec_get(const char *name, char *buf, unsigned int buf_len);
        int sunxi_spinor_private_sec_set(const char *name, char *buf, unsigned int buf_len);
		int sunxi_spinor_private_get(const char *name, char *buf, unsigned int buff_len);
		int sunxi_spinor_private_set(const char *name, const char *value);
		int sunxi_spinor_private_save();
		int sunxi_spinor_private_clear();
		int sunxi_spinor_private_set_flag(int value);
		int sunxi_spinor_private_get_flag();
		int sunxi_spinor_flash_get(const char *name, char *buf, unsigned int buff_len);

		
		int  sunxi_spinor_factory_set(const char *name, const char *value);
		int  sunxi_spinor_factory_get(const char *name, char *buf, unsigned int buff_len);
		int sunxi_spinor_factory_save(void);

	private:
		int sunxi_spinor_private_init();
		int sunxi_spinor_private_exit();		
		int set_by_name(ftest_item *data, const char *name, const char *value);
		int get_by_name(ftest_item *data, const char *name, char *buf, unsigned int buff_len);
		
		__u32 crc32(void * buffer, unsigned length);
		int flash_private_read(char *buf, unsigned int len);
		int flash_private_wirte(char *buf, unsigned int len);
        int flash_private_read_sec(char *buf, unsigned int len);
        int flash_private_wirte_sec(char *buf, unsigned int len, unsigned int offset);

		int  factory_set_by_name(ftest_item *data, const char *name, const char *value);

		int factory_get_by_name(ftest_item *data, const char *name, char *buf, unsigned int buff_len);
		int flash_factory_read(char *buf, unsigned int len);
		 int  flash_factory_wirte(char *buf, unsigned int len);
		int sunxi_spinor_factory_init(void);
		void sunxi_spinor_factory_exit(void);

	private:
		Mutex m_mutex;
		char *m_private_buff;
		char *m_private_buff_sec;
		int  m_private_init_flag;
		int  m_bind_flag;

		char *m_factory_buff;
		int  m_factory_init_flag;
};

#endif
