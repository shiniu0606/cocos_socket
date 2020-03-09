#include "Client.h"
#include "CCLuaEngine.h"
#include "SimpleAudioEngine.h"
#include "cocos2d.h"
#include "base/ccUtils.h"
#include "lua_module_register.h"
#include "lua_extensions.h"
#include "CMD_Data.h"
#include "plazz/ClientSocket.h"
#include "MD5.h"
#include "runtime/Runtime.h"
#include <regex>
#include "scripting/lua-bindings/manual/tolua_fix.h"
#include "FFmpegVideoPlayer.h"

using namespace CocosDenshion;

USING_NS_CC;
using namespace std;

Client::~Client()
{

}

bool Client::LoadGame()
{
	// set default FPS
	Director::getInstance()->setAnimationInterval(1.0 / 60.0f);

	// register lua module
	auto engine = LuaEngine::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);
	lua_State* L = engine->getLuaStack()->getLuaState();

	lua_module_register(L);

	luaopen_lua_extensions(L);
	register_all_cmd_data();
	register_all_client_socket();
	register_all_Integer64();
	register_all_client_data();
	register_all_CMD5Encrypt_data();

	register_all_CFFmpegVideoPlayer(L);

	LuaStack* stack = engine->getLuaStack();
	stack->setXXTEAKeyAndSign("2dxLua", strlen("2dxLua"), "111", strlen("111"));


	return true;
}

void Client::RestartGame()
{
	std::string key = "RestartGame";
	Director::getInstance()->getScheduler()->schedule([](float delta){
		Director::getInstance()->getRunningScene()->removeFromParentAndCleanup(true);
		Director::getInstance()->getTextureCache()->removeAllTextures();
		Director::getInstance()->getScheduler()->unscheduleAllWithMinPriority(Scheduler::PRIORITY_NON_SYSTEM_MIN);
		ScriptEngineManager::getInstance()->removeScriptEngine();
		ScriptHandlerMgr::destroyInstance();
		Client::GetInstance()->LoadGame();
	}, Director::getInstance()->getRunningScene(), 0.1f, 0, 0.1f, false, key);
}

std::string Client::base64Decode(std::string str)
{
	if (is_base64_string(str))
	{
		str = base64_decode(str);
		return str;
	}
	return "";
}

std::string Client::base64Encode(std::string str)
{
	if (str.size()>0)
	{
		str = base64_encode((unsigned char*)str.c_str(), (unsigned int)str.size());
		return str;
	}
	return "";
}

bool Client::isBase64String(std::string str)
{
	return is_base64_string(str);
}

std::string Client::gbkToutf8(std::string str)
{
	return gbk_utf8(str.c_str());
}

std::string Client::utf8Togbk(std::string str)
{
	return utf8_gbk(str.c_str());
}

std::string Client::genSignature(std::string str, std::string key)
{
	return hmac<SHA1>(str, key);
}

std::string Client::makeSignature(std::string str)
{
    static const char *key = "s7sf#43*&6937()";
    return hmac<SHA1>(str.c_str(), key);
}

std::string Client::getMD5(const char* data, int len)
{
    MD5 md;

    md.update(data, len);

    md.finalize();

    return md.hexdigest();
}

std::string Client::getFileMD5Hash(const std::string filepath)
{
	return utils::getFileMD5Hash(filepath);
}

long long Client::getTimeInMilliseconds()
{
    return utils::getTimeInMilliseconds();
}

bool Client::is64CPU()
{
	return sizeof(void*) == 8;
}

std::string Client::makeBCrypt(std::string str)
{
	return BCrypt::generateHash(str, 6);
}

std::string Client::getLocalIPAddress()
{
    return getIPAddress();
}

bool Client::checkNameValid(std::string realName)
{
	regex pattern("^([\u4e00-\u9fa5][·\u4e00-\u9fa5]{0,}[\u4e00-\u9fa5])$");

	if (!regex_match(realName, pattern))
	{
		return false;
	}

	return true;
}

void Client::saveToFile(std::string dataStr, const std::string& fullPath, std::function<void(bool)> callback) 
{
	FileUtils::getInstance()->writeStringToFile(dataStr, fullPath, callback);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static int tolua_Client_GetInstance(lua_State* tolua_S)
{
	Client* tolua_ret = (Client*)Client::GetInstance();
	tolua_pushusertype(tolua_S, (void*)tolua_ret, "Client");

	return 1;
}

static int tolua_Client_RestartGame(lua_State* tolua_S)
{
	Client::GetInstance()->RestartGame();

	return 1;
}

static int tolua_Client_base64Decode(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:base64Decode");
		if (ok)
		{
			data = Client::base64Decode(data);
			tolua_pushcppstring(tolua_S, data);
		}
	}
	return 1;
}

static int tolua_Client_base64Encode(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:base64Encode");
		if (ok)
		{
			data = Client::base64Encode(data);
			tolua_pushcppstring(tolua_S, data);
		}
	}
	return 1;
}

static int tolua_Client_isBase64String(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:isBase64String");
		if (ok)
		{
			bool bRet = Client::isBase64String(data);
			tolua_pushboolean(tolua_S, bRet);
		}
	}
	return 1;
}

static int tolua_Client_gbkToutf8(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:gbkToutf8");
		if (ok)
		{
			std::string str = Client::gbkToutf8(data);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_utf8Togbk(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:utf8Togbk");
		if (ok)
		{
			std::string str = Client::utf8Togbk(data);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_genSignature(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 2)
	{
		std::string data, key;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:genSignature");
		ok &= luaval_to_std_string(tolua_S, 3, &key, "Client:genSignature");
		if (ok)
		{
			std::string str = Client::genSignature(data, key);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_makeSignature(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "Client:makeSignature");
		if (ok)
		{
			std::string str = Client::makeSignature(data);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_getMD5(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string d1;

		ok &= luaval_to_std_string(tolua_S, 2, &d1, "Client:getMD5");

		if (ok)
		{
			std::string str = Client::getMD5(d1.c_str(), d1.length());
			tolua_pushcppstring(tolua_S, str);
		}
	}

	return 1;
}

static int tolua_Client_getTimeInMilliseconds(lua_State* tolua_S)
{
    int s = Client::getTimeInMilliseconds();
	tolua_pushnumber(tolua_S, (lua_Number)s);
	return 1;
}

static int tolua_Client_is64CPU(lua_State* tolua_S)
{
	bool bRet = Client::is64CPU();
	tolua_pushboolean(tolua_S, bRet);
	return 1;
}

static int tolua_Client_makeBCrypt(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string d1;

		ok &= luaval_to_std_string(tolua_S, 2, &d1, "Client:makeBCrypt");

		if (ok)
		{
			std::string str = Client::makeBCrypt(d1);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_getLocalIPAddress(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 0)
	{
		std::string data = Client::getLocalIPAddress();
		tolua_pushcppstring(tolua_S, data);
	}
	return 1;
}

static int tolua_Client_getNetSecRegisterToken(lua_State* tolua_S)
{
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 0)
	{
		std::string data = Client::getNetSecRegisterToken();
		tolua_pushcppstring(tolua_S, data);
	}
	return 1;
}

static int tolua_Client_checkNameValid(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 1)
	{
		std::string d1;

		ok &= luaval_to_std_string(tolua_S, 2, &d1, "Client:checkNameValid");
		bool flag = Client::checkNameValid(d1);
		tolua_pushboolean(tolua_S, flag);
	}
	return 1;
}

static int tolua_Client_getFileMD5Hash(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 1)
	{
		std::string d1;

		ok &= luaval_to_std_string(tolua_S, 2, &d1, "Client:getFileMD5Hash");
		if (ok)
		{
			std::string str = Client::getFileMD5Hash(d1);
			tolua_pushcppstring(tolua_S, str);
		}
	}
	return 1;
}

static int tolua_Client_saveToFile(lua_State* tolua_S)
{
	bool ok = true;
	int argc = lua_gettop(tolua_S) - 1;
	if (argc == 1)
	{
		std::string arg0;
        ok &= luaval_to_std_string(tolua_S, 2,&arg0, "Client:saveToFile");
        
        std::string arg1;
        ok &= luaval_to_std_string(tolua_S, 3,&arg1, "Client:saveToFile");

        LUA_FUNCTION handler = (  toluafix_ref_function(tolua_S, 4, 0));
		if (ok)
		{
			Client::saveToFile(arg0, arg1, [=](bool ret){
				tolua_pushboolean(tolua_S, ret);
	            LuaEngine::getInstance()->getLuaStack()->executeFunctionByHandler(handler,1);
	            LuaEngine::getInstance()->removeScriptHandler(handler);
	        });
		}
	}
	return 1;
}

static int register_all_client_data()
{
	auto engine = LuaEngine::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);
	lua_State* tolua_S = engine->getLuaStack()->getLuaState();

	tolua_open(tolua_S);
	tolua_module(tolua_S, "cc", 0);
	tolua_beginmodule(tolua_S, "cc");
	tolua_usertype(tolua_S, "cc.Client");
	tolua_cclass(tolua_S, "Client", "cc.Client", "", nullptr);
	tolua_beginmodule(tolua_S, "Client");
	tolua_function(tolua_S, "GetInstance", tolua_Client_GetInstance);
	tolua_function(tolua_S, "RestartGame", tolua_Client_RestartGame);
	tolua_function(tolua_S, "base64Decode", tolua_Client_base64Decode);
	tolua_function(tolua_S, "base64Encode", tolua_Client_base64Encode);
	tolua_function(tolua_S, "isBase64String", tolua_Client_isBase64String);
	tolua_function(tolua_S, "gbkToutf8", tolua_Client_gbkToutf8);
	tolua_function(tolua_S, "utf8Togbk", tolua_Client_utf8Togbk);
	tolua_function(tolua_S, "genSignature", tolua_Client_genSignature);
	tolua_function(tolua_S, "makeSignature", tolua_Client_makeSignature);
	tolua_function(tolua_S, "getMD5", tolua_Client_getMD5);
	tolua_function(tolua_S, "getTimeInMilliseconds", tolua_Client_getTimeInMilliseconds);
	tolua_function(tolua_S, "is64CPU", tolua_Client_is64CPU);
	tolua_function(tolua_S, "makeBCrypt", tolua_Client_makeBCrypt);
	tolua_function(tolua_S, "getIPAddress", tolua_Client_getLocalIPAddress);
	tolua_function(tolua_S, "checkNameValid", tolua_Client_checkNameValid);
	tolua_function(tolua_S, "getFileMD5Hash", tolua_Client_getFileMD5Hash);
	tolua_function(tolua_S, "saveToFile", tolua_Client_saveToFile);
	tolua_endmodule(tolua_S);

	std::string typeName = typeid(Client).name();
	g_luaType[typeName] = "Client";
	g_typeCast["Client"] = "Client";

	tolua_endmodule(tolua_S);
	return 1;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static int tolua_CMD5Encrypt_EncryptData(lua_State* tolua_S)
{
	bool ok = true;

	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{
		std::string data;
		ok &= luaval_to_std_string(tolua_S, 2, &data, "CMD5Encrypt:EncryptData");
		if (ok)
		{
			char szMD5Result[34] = { '\0' };
			CMD5Encrypt::EncryptData(data.c_str(), szMD5Result);
			lua_pushlstring(tolua_S, szMD5Result, 33);
		}
	}
	return 1;
}

static int register_all_CMD5Encrypt_data()
{
	auto engine = LuaEngine::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);
	lua_State* tolua_S = engine->getLuaStack()->getLuaState();

	tolua_open(tolua_S);
	tolua_module(tolua_S, "cc", 0);
	tolua_beginmodule(tolua_S, "cc");
	tolua_usertype(tolua_S, "cc.CMD5Encrypt");
	tolua_cclass(tolua_S, "CMD5Encrypt", "cc.CMD5Encrypt", "", nullptr);
	tolua_beginmodule(tolua_S, "CMD5Encrypt");
	tolua_function(tolua_S, "EncryptData", tolua_CMD5Encrypt_EncryptData);
	tolua_endmodule(tolua_S);

	std::string typeName = typeid(CMD5Encrypt).name();
	g_luaType[typeName] = "CMD5Encrypt";
	g_typeCast["CMD5Encrypt"] = "CMD5Encrypt";

	tolua_endmodule(tolua_S);
	return 1;
}
