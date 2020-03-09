#ifndef _LUA_DEFINE_H_
#define _LUA_DEFINE_H_
#include "platform/CCPlatformConfig.h"
#include <string>

typedef signed char				int8;
typedef signed short			int16;
typedef signed int				int32;
typedef long long				int64;

typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned int			uint32;
typedef unsigned long long		uint64;

typedef long long				LONGLONG;

typedef unsigned char			BYTE;
typedef unsigned short			WORD;

typedef double					DOUBLE;
typedef short                   SHORT;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC )
	typedef unsigned int			DWORD;
	typedef int                     LONG;
#else
	typedef unsigned long			DWORD;
	typedef long					LONG;
#endif

#define TRUE				1
#define FALSE				0



#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC )
	typedef unsigned int			UINT;
#define ZeroMemory(x, size)	memset(x, 0, size)
#endif

#ifdef _WIN32
#define LLSTRING "%I64d"
#else
#define LLSTRING "%lld"
#endif

#define ASIO_STANDALONE 
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS
#include "asio/asio.hpp"


#define 	EMPTY_CHAR(p)							(p == NULL || p[0] == '\0')				//空字符
#endif
