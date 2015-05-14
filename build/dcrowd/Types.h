/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef _TYPES_H
#define _TYPES_H

namespace dcrowd { 

typedef unsigned int UINT;

typedef void* PVOID;
typedef const void* CPVOID;

typedef char* PCHAR;
typedef const char* CPCHAR;

typedef unsigned char BYTE;
typedef unsigned char* PBYTE;
typedef const unsigned char* CPBYTE;

typedef signed   char INT8;
typedef unsigned char UINT8;

typedef signed   short INT16;
typedef unsigned short WORD;
typedef unsigned short UINT16;

typedef signed   int INT32;
typedef unsigned int UINT32;

typedef signed long 	LONG32;
typedef unsigned long 	ULONG32;

typedef signed long long 	LONG64;
typedef unsigned long long  ULONG64;

// time_t is signed
#define MAX_TIME_T 0x7FFFFFFF

#ifndef HIWORD
#define HIWORD(dw) ((dw)>>16)
#endif
#ifndef LOWORD
#define LOWORD(dw) ((dw)&0xffff)
#endif
#ifndef MAKEDWORD
#define MAKEDWORD(w1,w2) (((w1)<<16)|(w2))
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((UINT32)0xFFFFFFFF)
#endif
#ifndef INADDR_ANY
#define INADDR_ANY  ((UINT32)0x00000000)
#endif

}

#endif 
