#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <asm/types.h>

typedef __u16  __le16;
//typedef __u16  __be16;
typedef __u32  __le32;
//typedef __u32  __be32;
typedef __u64  __le64;
//typedef __u64  __be64;

typedef __u16  __sum16;
typedef __u32  __wsum;

typedef long long __kernel_loff_t;

#endif /* _LINUX_TYPES_H */