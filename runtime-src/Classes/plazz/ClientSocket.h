#ifndef _CLIENT_SOCKET_H_
#define _CLIENT_SOCKET_H_

#include "cocos2d.h"
#include "common/DefineGlobal.h"
#include "common/CMD_Data.h"
#include "plazz/ClientKernel.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include "asio.hpp"

#ifdef __cplusplus
extern "C" {
#endif

NS_CC_BEGIN;

#define STATIC_BUF_SIZE 1024
#define STATIC_HEAD_SIZE 8

struct NetBuffer {
private:
	BYTE   m_sbuf[STATIC_BUF_SIZE];///小数据避免内存分配
	BYTE   m_headbuf[STATIC_BUF_SIZE];
	BYTE*  m_dbuf;
	uint32  m_datalen;
	uint32  m_capacity;
public:
	NetBuffer()
	{
		m_dbuf = 0;
		m_capacity = STATIC_BUF_SIZE;
		m_datalen = 0;
		memset(m_sbuf, 0,STATIC_BUF_SIZE);
		memset(m_headbuf, 0, STATIC_BUF_SIZE);
	}
	~NetBuffer()
	{
		if (m_dbuf)
		{
			free(m_dbuf);
			m_dbuf = NULL;
		}
		m_datalen = 0;
		m_capacity = STATIC_BUF_SIZE;
	}
	BYTE* GetBuffer()
	{
		if (m_dbuf)
			return m_dbuf;
		else
			return m_sbuf;
	}
	BYTE*   GetHeadBuffer() {
		return m_headbuf;
	}
	uint32	GetDataSize() { return m_datalen; }
	uint32	GetCapacity() { return m_capacity; }
	void	Resize(uint32 _len)
	{
		m_datalen = 0;
		ExpandBuffer(_len);
		m_datalen = _len;
	}
	void ExpandBuffer(uint32 needSize)
	{
		if (m_capacity >= needSize)
			return;

		BYTE * obuf = GetBuffer();
		//如果重复分配，就扩展为2倍，避免重复分配的碎片
		if (m_dbuf) //说明重复分配
			needSize = 2 * needSize;
		m_capacity = needSize;
		m_dbuf = (BYTE*)malloc(m_capacity);

		//把原来的数据copy过去
		if (m_datalen)
			memcpy(m_dbuf, obuf, m_datalen);

		//释放原来的内存
		if (obuf && obuf != m_sbuf) //不是静态内存
			free(obuf);
	}
	//重置内部状态
	void Reset()
	{
		m_datalen = 0;
	}
	void ResetHead()
	{
		memset(m_headbuf, 0, STATIC_BUF_SIZE);
	}
};

typedef	std::shared_ptr<NetBuffer>	NetBufferPtr;
typedef std::deque<PACK_DATA>		SendMsgList;

class CClientSocket :public cocos2d::Ref
{
public:
	CClientSocket(int nHandler);
	virtual ~CClientSocket();

	virtual bool Connect(const char* szUrl, unsigned short wPort, unsigned char* pValidate = nullptr);
	virtual bool SendSocketData(unsigned short wMain, unsigned short wSub, BYTE* pData = nullptr, unsigned short wDataSize = 0);
	virtual void StopServer();
	virtual bool IsAliveServer();
	virtual	void Poll(float dt);
public:
	int sendData(CCmd_Data* pData);

private:
	void do_close();
	void do_connect();
	void do_read_header();
	void do_read_body();
	void do_write();
private:
	int m_nHandle;
	bool m_bClosed;
	bool m_bIsAlive;
	asio::io_service m_ios;
	asio::ip::tcp::socket m_socket;

	std::size_t		m_bodyLength;
	NetBufferPtr    m_bufferPtr;
	SendMsgList	    m_pendingList;
	CClientKernel   kernel;
};

NS_CC_END

USING_NS_CC;

static int toLua_Client_Socket_create(lua_State* tolua_S)
{
	int argc = lua_gettop(tolua_S) - 1;

	if (argc == 1)
	{

		int handler = toluafix_ref_function(tolua_S, 2, 0);
		CCLOG("toLua_Client_Socket_create:[%d", (int)handler);
		if (handler != 0)
		{
			CClientSocket* tolua_ret = new CClientSocket(handler);
			tolua_ret->autorelease();
			tolua_ret->retain();

			int nID = (tolua_ret) ? tolua_ret->_ID : -1;
			int *pLuaID = (tolua_ret) ? &tolua_ret->_luaID : NULL;

			CCLOG("lua callback:[%d][%d]", nID, *pLuaID);
			toluafix_pushusertype_ccobject(tolua_S, nID, pLuaID, (void*)tolua_ret, "cc.CClientSocket");

			return 1;
		}
	}

	return 0;
}

static int toLua_Client_Socket_sendData(lua_State* tolua_S)
{
	int result = 0;
	do
	{
		CClientSocket* cobj = (CClientSocket*)tolua_tousertype(tolua_S, 1, 0);
		CC_BREAK_IF(cobj == NULL);
		CC_BREAK_IF(cobj->IsAliveServer() == false);

		int argc = lua_gettop(tolua_S) - 1;
		CC_BREAK_IF(argc != 1);

		CCmd_Data* pData = (CCmd_Data*)tolua_tousertype(tolua_S, 2, 0);
		CC_BREAK_IF(pData == NULL);

		result = cobj->sendData(pData);
	} while (false);

	lua_pushboolean(tolua_S, result);

	return result;
}


static int toLua_Client_Socket_close(lua_State* tolua_S)
{
	int result = 1;
	do
	{
		CClientSocket* cobj = (CClientSocket*)tolua_tousertype(tolua_S, 1, 0);
		CC_BREAK_IF(cobj == NULL);
		//CC_BREAK_IF(!cobj->IsServe());
		cobj->StopServer();
		result = true;
	} while (false);

	lua_pushboolean(tolua_S, result);

	return result;
}

static int toLua_Client_Socket_connect(lua_State* tolua_S)
{
	int result = 0;
	do
	{
		CClientSocket* cobj = (CClientSocket*)tolua_tousertype(tolua_S, 1, 0);
		CC_BREAK_IF(cobj == NULL);
		CC_BREAK_IF(cobj->IsAliveServer());
		const char* szUrl = lua_tostring(tolua_S, 2);
		WORD wPort = lua_tointeger(tolua_S, 3);
		int argc = lua_gettop(tolua_S);

		lua_pushboolean(tolua_S, cobj->Connect(szUrl, wPort) ? 1 : 0);

		result = 1;
	} while (false);

	return result;
}

static int toLua_Client_Socket_relaseSocket(lua_State* tolua_S)
{
	int result = 1;
	do
	{
		CClientSocket* cobj = (CClientSocket*)tolua_tousertype(tolua_S, 1, 0);
		if(cobj == NULL) return 0;
		cobj->StopServer();
		//cobj->release();
		result = true;
	} while (false);
	lua_pushboolean(tolua_S, result);
	return result;
}

static int register_all_client_socket()
{
	auto engine = LuaEngine::getInstance();
	ScriptEngineManager::getInstance()->setScriptEngine(engine);
	lua_State* tolua_S = engine->getLuaStack()->getLuaState();

	tolua_usertype(tolua_S, "cc.CClientSocket");
	tolua_cclass(tolua_S, "CClientSocket", "cc.CClientSocket", "cc.Node", nullptr);
	tolua_beginmodule(tolua_S, "CClientSocket");
	tolua_function(tolua_S, "createSocket", toLua_Client_Socket_create);
	tolua_function(tolua_S, "sendData", toLua_Client_Socket_sendData);
	tolua_function(tolua_S, "relaseSocket", toLua_Client_Socket_relaseSocket);
	tolua_function(tolua_S, "closeSocket", toLua_Client_Socket_close);
	tolua_function(tolua_S, "connectSocket", toLua_Client_Socket_connect);
	tolua_endmodule(tolua_S);

	std::string typeName = typeid(cocos2d::CClientSocket).name();
	g_luaType[typeName] = "cc.CClientSocket";
	g_typeCast["CClientSocket"] = "cc.CClientSocket";
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif