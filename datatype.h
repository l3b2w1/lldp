#ifndef _DATA_TYPE_H_
#define _DATA_TYPE_H_

/* 64-bit machine */
#ifdef _MACHINE_64_BIT_

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long s64;
typedef unsigned long u64;

#else 	/* _MACHINE_32_BIT_  */

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;


#endif

#endif
