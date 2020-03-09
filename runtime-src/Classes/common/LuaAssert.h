#ifndef _LUA_ASSERT_H_
#define _LUA_ASSERT_H_
#include "libiconv/include/iconv.h"

#include <vector>
#include <string>

#include "cocos2d.h"

typedef std::vector<unsigned short> u2string;
typedef std::string					u1string;

/**
* ����utf8�ַ�������
* @param utf8 utf8�ַ���
* @return �ַ�������
*/
int utf8_len(const char *utf8);

int utf8_cmp(const char* str1, const char* str2);

/**
* ����ucs2�ַ�������
* @param ucs2 ucs2�ַ���
* @return �ַ�������
*/
int ucs2_len(const unsigned short* ucs2);

/**
* ��utf8�ַ���ת��Ϊucs2�ַ���
* @param utf8 ��ת����utf8�ַ���
* @return ת����ucs2�ַ���
*/
u2string utf8_ucs2(const char* utf8);

/**
* ��ucs2�ַ���ת��Ϊutf8�ַ���
* @param ucs2 ��ת����ucs2�ַ���
* @return ת����utf8�ַ���
*/
u1string ucs2_utf8(const unsigned short *ucs2);

/**
* ��gbk�ַ���ת��Ϊutf8�ַ���
* @param gbk ��ת����gbk�ַ���
* @return ת����utf8�ַ���
*/
u1string gbk_utf8(const char* gbk);

/**
* ��utf8�ַ���ת��Ϊgbk�ַ���
* @param utf8 ��ת����utf8�ַ���
* @return ת����gbk�ַ���
*/
u1string utf8_gbk(const char* utf8);

//////////////////////////////////////////////////////////////////////////
// ��������

#define a_u8(str)	gbk_utf8(str).c_str()
#define u8_a(str)	utf8_gbk(str).c_str()

#define u2_8(ucs2)	ucs2_utf8((const unsigned short*)ucs2).c_str()
#define u8_2(utf8)	&utf8_ucs2(utf8)[0]
#define u8_2s(utf8)	utf8_ucs2(utf8)

#endif