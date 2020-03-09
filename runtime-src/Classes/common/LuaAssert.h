#ifndef _LUA_ASSERT_H_
#define _LUA_ASSERT_H_
#include "libiconv/include/iconv.h"

#include <vector>
#include <string>

#include "cocos2d.h"

typedef std::vector<unsigned short> u2string;
typedef std::string					u1string;

/**
* ¼ÆËãutf8×Ö·û´®³¤¶È
* @param utf8 utf8×Ö·û´®
* @return ×Ö·û´®³¤¶È
*/
int utf8_len(const char *utf8);

int utf8_cmp(const char* str1, const char* str2);

/**
* ¼ÆËãucs2×Ö·û´®³¤¶È
* @param ucs2 ucs2×Ö·û´®
* @return ×Ö·û´®³¤¶È
*/
int ucs2_len(const unsigned short* ucs2);

/**
* ½«utf8×Ö·û´®×ª»»Îªucs2×Ö·û´®
* @param utf8 ´ý×ª»»µÄutf8×Ö·û´®
* @return ×ª»»ºóucs2×Ö·û´®
*/
u2string utf8_ucs2(const char* utf8);

/**
* ½«ucs2×Ö·û´®×ª»»Îªutf8×Ö·û´®
* @param ucs2 ´ý×ª»»µÄucs2×Ö·û´®
* @return ×ª»»ºóutf8×Ö·û´®
*/
u1string ucs2_utf8(const unsigned short *ucs2);

/**
* ½«gbk×Ö·û´®×ª»»Îªutf8×Ö·û´®
* @param gbk ´ý×ª»»µÄgbk×Ö·û´®
* @return ×ª»»ºóutf8×Ö·û´®
*/
u1string gbk_utf8(const char* gbk);

/**
* ½«utf8×Ö·û´®×ª»»Îªgbk×Ö·û´®
* @param utf8 ´ý×ª»»µÄutf8×Ö·û´®
* @return ×ª»»ºógbk×Ö·û´®
*/
u1string utf8_gbk(const char* utf8);

//////////////////////////////////////////////////////////////////////////
// ¸¨Öúº¯Êý

#define a_u8(str)	gbk_utf8(str).c_str()
#define u8_a(str)	utf8_gbk(str).c_str()

#define u2_8(ucs2)	ucs2_utf8((const unsigned short*)ucs2).c_str()
#define u8_2(utf8)	&utf8_ucs2(utf8)[0]
#define u8_2s(utf8)	utf8_ucs2(utf8)

#endif