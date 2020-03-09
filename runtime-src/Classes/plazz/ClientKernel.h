#ifndef _LUA_CLIENT_KERNEL_H_
#define _LUA_CLIENT_KERNEL_H_

#include "common/DefineGlobal.h"
#include "common/CMD_Data.h"
#include "cocos2d.h"

USING_NS_CC;

#define SOCKET_VER					0x89								//网络版本

static const int SIZE_TCP_BUFFER = 1024;
static const int SIZE_CMD_HEADER = 8;

static const DWORD g_dwPacketKey = 0x1FBFEF1F;

#pragma pack(push)
#pragma pack(1)
typedef struct _ME_GUID {
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
} ME_GUID;
#pragma pack(pop)

struct CMD_Info
{
	BYTE								cbVersion;							
	BYTE								cbCheckCode;					
	WORD								wPacketSize;					
};

struct CMD_Command
{
	WORD								wMainCmdID;							
	WORD								wSubCmdID;						
};

struct CMD_Head
{
	CMD_Info							CmdInfo;							
	CMD_Command							CommandInfo;						
};

struct PACK_DATA
{
	BYTE data[SIZE_TCP_BUFFER];
	WORD size;
};

class CClientKernel
{
public:
	CClientKernel();
public:
	virtual ~CClientKernel();
public:
	bool OnInit();

public:
	bool OnMessageHandler(int nHandler, WORD wMain, WORD wSub);

public:
	bool OnCallLuaSocketCallBack(int nHandler, Ref* data);

public:
	bool OnSocketConnectEvent(int nHandler);
	bool OnSocketDataEvent(int nHandler, BYTE* header, BYTE* data, WORD dataSize);
	bool OnSocketErrorEvent(int nHandler, int errorCode, std::string errorMessage);
	bool OnSocketCloseEvent(int nHandler);
	PACK_DATA pack(int main, int sub, BYTE* data, int size);
	CCmd_Data* unpack(BYTE* data, int start, int length);
private:
	WORD EncryptBuffer(BYTE pcbDataBuffer[], WORD wDataSize, WORD wBufferSize);
	WORD CrevasseBuffer(BYTE pcbDataBuffer[], WORD wDataSize);
	WORD SeedRandMap(WORD wSeed);
private:
	int mBufRevLength;
	BYTE mBufRecieve[SIZE_TCP_BUFFER * 10];
	BYTE mBufPack[SIZE_TCP_BUFFER];
	BYTE mBufUnPack[SIZE_TCP_BUFFER * 5];

protected:
	BYTE							m_cbSendRound;						//字节映射
	BYTE							m_cbRecvRound;						//字节映射
	DWORD							m_dwSendXorKey;						//发送密钥
	DWORD							m_dwRecvXorKey;						//接收密钥

	DWORD							m_dwSendPacketCount;				//发送计数
};

#endif