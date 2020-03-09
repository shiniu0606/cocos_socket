
// StringUtil 字符串操作

#pragma once
#include <string>
#include <sstream>
#include <vector>
#include "DefineGlobal.h"
#include "Integer64.h"
#include "utf8.h"

class StringUtil
{
public:
	static void trim(std::string& str, bool left=true, bool right=true)
	{
		static const std::string delims = " \t\r \n";
		if(right)
			str.erase(str.find_last_not_of(delims)+1); // trim right
		if(left)
			str.erase(0, str.find_first_not_of(delims)); // trim left
	}
	static std::vector<std::string> split( const std::string& str,const std::string& delims = "\t\n ", unsigned int maxSplits = 0)
	{
		std::vector<std::string> ret;
		ret.reserve(maxSplits ? maxSplits+1 : 10);    // 10 is guessed capacity for most case
		
		unsigned int numSplits = 0;
		size_t start, pos;
		start = 0;
		do 
		{
			pos = str.find_first_of(delims, start);
			if (pos == start)
			{
				start = pos + 1;
			}
			else if (pos == std::string::npos || (maxSplits && numSplits == maxSplits))
			{
				// Copy the rest of the string
				ret.push_back( str.substr(start) );
				break;
			}
			else
			{
				// Copy up to delimiter
				ret.push_back( str.substr(start, pos - start) );
				start = pos + 1;
			}
			// parse up to next real data
			start = str.find_first_not_of(delims, start);
			++numSplits;
		} while (pos != std::string::npos);
		return ret;
	}
	// 以xx开始
	static bool startsWith(const std::string& str, const std::string& pattern)
	{
		size_t thisLen = str.length();
		size_t patternLen = pattern.length();
		if (thisLen < patternLen || patternLen == 0)
			return false;

		std::string startOfThis = str.substr(0, patternLen);

		return (startOfThis == pattern);
	}
	// 以xx结尾
	static bool endsWith(const std::string& str, const std::string& pattern)
	{
		size_t thisLen = str.length();
		size_t patternLen = pattern.length();
		if (thisLen < patternLen || patternLen == 0)
			return false;

		std::string endOfThis = str.substr(thisLen - patternLen, patternLen);

		return (endOfThis == pattern);
	}

	//从文件路径中推导文件名，不包含扩展名
	static std::string ExtractFileBaseName(const std::string & filePath)
	{
		std::string name = filePath;
		if (name.empty())
			return "";

		if(*name.rbegin() == '/' || *name.rbegin()=='\\')
			*name.rbegin()=0;

		int pos = name.find_last_of("\\/");
		if(pos != std::string::npos)
			name = name.substr(pos+1);
		pos = name.rfind('.');
		if(pos != name.npos)
			name = name.substr(0,pos);

		return name;
	}
	//从文件路径中推导文件名，包含扩展名
	static std::string ExtractFileName(const std::string & filePath)
	{
		std::string name = filePath;
		if (name.empty())
			return "";

		if(*name.rbegin() == '/' || *name.rbegin()=='\\')
			*name.rbegin()=0;

		int pos = name.find_last_of("\\/");
		if(pos != std::string::npos)
			name = name.substr(pos+1);

		return name;
	}
	//从文件路径中推导扩展名,不包含文件名
	static std::string ExtractFileExtName(const std::string& filePath)
	{
		std::string ext = filePath;

		if (ext.empty())
		{
			return "";
		}

		//去掉结束的/
		if(*ext.rbegin() == '/' || *ext.rbegin()=='\\')
			*ext.rbegin()=0;

		int pos = ext.find_last_of("\\/");
		if(pos != std::string::npos)
			ext = ext.substr(pos+1);
		pos = ext.rfind('.');
		if(pos != ext.npos)
			ext = ext.substr(pos+1);

		return ext;
	}
	//从文件路径中推导路径
	static std::string ExtractDirName(const std::string& path)
	{
		int pos = path.find_last_of("/\\");
		if(pos != std::string::npos)
			return path.substr(0,pos);
		else
			return "";
	}
	//抽取最后的目录名
	static std::string ExtractLastDirName(const std::string & _vfsdir)
	{
		std::string vfsdir = _vfsdir;
		std::vector<std::string> v;
		v = StringUtil::split(vfsdir, "/\\");
		if(v.size()>1)
			return v[v.size()-2];
		else
			return std::string();
	}
	static float ToFloat(const std::string & str)
	{
		return (float)atof(str.c_str());
	}
	static int32 ToInt(const std::string &str)
	{
		return atoi(str.c_str());
	}
	static int32 ToInt(const char *  str)
	{
		return atoi(str);
	}
	
	static std::string ToString(int32 num)
	{
//		return std::to_string(num);
		std::ostringstream os;
		os << num;
		return os.str();
	}
	static std::string  ToStringInt64(int64 val)
	{
//		return std::to_string(val);

		std::ostringstream os;
		os << val;
		return os.str();
	}
	static std::string ToStringFloat(float val)
	{
//		return std::to_string(val);
		std::ostringstream os;
		os << val;
		return os.str();
	}
	static std::string standardiseDir(std::string path)
	{
		std::replace(path.begin(),path.end(), '\\', '/' );
		if( path.length() &&  path[path.length() - 1] != '/' )
			path += '/';
		return path;
	}
	static std::string StandardiseFileName(std::string fileName)
	{
		std::replace(fileName.begin(),fileName.end(), '\\', '/' );
		return fileName;
	}
	static std::string Utf16ToUtf8(const std::wstring &wstr)
	{
		std::string str;
		utf8::utf16to8(wstr.begin(), wstr.end(),std::back_inserter(str));
		return str;
	}
	static bool Utf8ToUtf16(const std::string& str,std::wstring& wstr)
	{
		try
		{
			utf8::utf8to16(str.begin(),str.end(), back_inserter(wstr)); // 会拋异常
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	
	//utf8 支持
	static bool Utf8StringNextChar(std::string::iterator & it, std::string::iterator & end, uint32 &cp)
	{
		if(it== end)
			return false;

		utf8::uint32_t _cp; 
		utf8::internal::utf_error err=  utf8::internal::validate_next(it, end,_cp);
		if(err != utf8::internal::UTF8_OK)
			return false;
		cp = _cp;
		return true;
	}
};
