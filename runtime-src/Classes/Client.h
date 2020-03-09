#pragma once
extern "C" {
#include "lua.h"
#include "tolua++.h"
}
#include "tolua_fix.h"
#include "cocos2d.h"
#include "CCLuaEngine.h"
#include "LuaBasicConversions.h"


#include "common/DefineGlobal.h"
#include "common/base64.h"
#include "common/LuaAssert.h"

#include "hash/hmac.h"
#include "hash/sha1.h"
#include "common/MD5.h"
#include "hash/BCrypt.hpp"

class Client
{
public:
	~Client();

	static Client* GetInstance()
	{
		static Client s;
		return &s;
	}

	bool LoadGame();

	void RestartGame();

	static std::string base64Decode(std::string str);
	static std::string base64Encode(std::string str);
	static bool isBase64String(std::string str);
	static std::string gbkToutf8(std::string str);
	static std::string utf8Togbk(std::string str);

	static std::string makeSignature(std::string str);
	static std::string genSignature(std::string str, std::string key);
	static std::string makeBCrypt(std::string str);

    static std::string getMD5(const char* data, int len);
	static std::string getFileMD5Hash(const std::string filepath);
	static long long getTimeInMilliseconds();
    static bool is64CPU();

    static std::string getLocalIPAddress();
	static std::string getNetSecRegisterToken();
	static void getNetSecRegisterTokenAsync();
	static bool getNetSecRegisterTokenSussed();

	static std::string getNetSecLoginToken();
	static void getNetSecLoginTokenAsync();
	static bool getNetSecLoginTokenSussed();

	static bool checkNameValid(std::string realName);

	static void initBugly();
	static void setBuglyAppChannel(std::string channel);
	static void setBuglyAppVersion(std::string version);
	static void setBuglyAppUserId(std::string userId);

	static void saveToFile(std::string dataStr, const std::string& fullPath, std::function<void(bool)> callback);
};

static int register_all_client_data();
static int register_all_CMD5Encrypt_data();

