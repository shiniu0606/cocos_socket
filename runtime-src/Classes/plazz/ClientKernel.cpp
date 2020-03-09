#include "ClientKernel.h"
#include "common/CMD_Data.h"
#include "QPCipher.h"
#include "CCLuaEngine.h"

CClientKernel::CClientKernel():
	mBufRevLength(0)
{
}

CClientKernel::~CClientKernel()
{
}

bool CClientKernel::OnInit()
{
	m_cbSendRound = 0;
	m_cbRecvRound = 0;
	m_dwSendXorKey = 0x12345678;
	m_dwRecvXorKey = 0x12345678;
	m_dwSendPacketCount = 0;
	return true;
}

bool CClientKernel::OnCallLuaSocketCallBack(int nHandler, Ref* pData)
{

	bool result = false;
	do
	{
		CCmd_Data* pCmdData = (CCmd_Data*)pData;

		if (nHandler != 0 && pCmdData != NULL)
		{
			lua_State* tolua_S = LuaEngine::getInstance()->getLuaStack()->getLuaState();
			toluafix_get_function_by_refid(tolua_S, nHandler);
			if (lua_isfunction(tolua_S, -1))
			{
				int nID = (pData) ? pCmdData->_ID : -1;
				int *pLuaID = (pData) ? &pCmdData->_luaID : NULL;
				toluafix_pushusertype_ccobject(tolua_S, nID, pLuaID, (void*)pData, "cc.CCmd_Data");

				LuaEngine::getInstance()->getLuaStack()->executeFunctionByHandler(nHandler, 1);
			}
			else {
				CCLOG("OnCallLuaSocketCallBack-false:%d", nHandler);
			}
		}
	} while (false);
	return result == 0;
}

bool CClientKernel::OnSocketConnectEvent(int nHandler)
{
	bool result = false;
	do {
		CCmd_Data* pData = CCmd_Data::create(0);
		pData->SetCmdInfo(0, 1);

		result = OnCallLuaSocketCallBack(nHandler, pData);
	} while (false);
	return result;
}

PACK_DATA CClientKernel::pack(int main, int sub, BYTE* data, int size)
{
	//构造数据
	PACK_DATA pack;
	CMD_Head * pHead = (CMD_Head *)pack.data;
	pHead->CommandInfo.wMainCmdID = main;
	pHead->CommandInfo.wSubCmdID = sub;

	if (size > 0)
	{
		assert(data != NULL);
		memcpy(pHead + 1, data, size);
	}
	//加密数据
	pack.size = EncryptBuffer(pack.data, sizeof(CMD_Head) + size, sizeof(pack.data));
	return pack;
}
CCmd_Data* CClientKernel::unpack(BYTE* data, int start, int length)
{
	WORD wRealySize;

	// 解密
	if ((data[start] & QPCipher::getCipherMode()) > 0)
	{
		wRealySize = CrevasseBuffer(data, length);
	}

	CMD_Command Command = ((CMD_Head *)data)->CommandInfo;
	if (Command.wMainCmdID == 0 && Command.wSubCmdID == 1)
	{
		return NULL;   
	}

	// 附加数据
	if (length > SIZE_CMD_HEADER)
	{
		memcpy(mBufUnPack, &data[start + SIZE_CMD_HEADER], length - SIZE_CMD_HEADER);
	}

	length -= SIZE_CMD_HEADER;

	CCmd_Data* cmd_data =  CCmd_Data::create(length);
	cmd_data->SetCmdInfo(Command.wMainCmdID, Command.wSubCmdID);
	cmd_data->PushByteData(mBufUnPack, length);
	cmd_data->ResetCurrentIndex();
	return cmd_data;
}

WORD CClientKernel::EncryptBuffer(BYTE cbDataBuffer[], WORD wDataSize, WORD wBufferSize)
{
	//调整长度
	WORD wEncryptSize = wDataSize - sizeof(CMD_Command), wSnapCount = 0;
	if ((wEncryptSize % sizeof(DWORD)) != 0)
	{
		wSnapCount = sizeof(DWORD) - wEncryptSize % sizeof(DWORD);
		ZeroMemory(cbDataBuffer + sizeof(CMD_Info) + wEncryptSize, wSnapCount);
	}

	//字节映射
	BYTE cbCheckCode = 0;
	for (WORD i = sizeof(CMD_Info); i < wDataSize; i++)
	{
		cbCheckCode += cbDataBuffer[i];
		cbDataBuffer[i] = QPCipher::MapSendByte(cbDataBuffer[i], m_cbSendRound);
	}

	//填写信息头
	CMD_Head * pHead = (CMD_Head *)cbDataBuffer;
	pHead->CmdInfo.cbVersion = SOCKET_VER;
	pHead->CmdInfo.wPacketSize = wDataSize;
	pHead->CmdInfo.cbCheckCode = ~cbCheckCode + 1;

	//创建密钥
	DWORD dwXorKey = m_dwSendXorKey;
	if (m_dwSendPacketCount == 0)
	{
		//随机种子
		ME_GUID Guid;
		memset(&Guid, 1, sizeof(Guid));
		dwXorKey = 99999;//GetTickCount()*GetTickCount();
		dwXorKey ^= Guid.Data1;
		dwXorKey ^= Guid.Data2;
		dwXorKey ^= Guid.Data3;
		dwXorKey ^= *((DWORD *)Guid.Data4);

		//映射种子
		dwXorKey = SeedRandMap((WORD)dwXorKey);
		dwXorKey |= ((DWORD)SeedRandMap((WORD)(dwXorKey >> 16))) << 16;
		dwXorKey ^= g_dwPacketKey;
		m_dwSendXorKey = dwXorKey;
		m_dwRecvXorKey = dwXorKey;
	}

	//加密数据
	WORD * pwSeed = (WORD *)(cbDataBuffer + sizeof(CMD_Info));
	DWORD * pdwXor = (DWORD *)(cbDataBuffer + sizeof(CMD_Info));
	WORD wEncrypCount = (wEncryptSize + wSnapCount) / sizeof(DWORD);
	for (WORD i = 0; i < wEncrypCount; i++)
	{
		*pdwXor++ ^= dwXorKey;
		dwXorKey = SeedRandMap(*pwSeed++);
		dwXorKey |= ((DWORD)SeedRandMap(*pwSeed++)) << 16;
		dwXorKey ^= g_dwPacketKey;
	}

	//插入密钥
	if (m_dwSendPacketCount == 0)
	{
		memmove(cbDataBuffer + sizeof(CMD_Head) + sizeof(DWORD), cbDataBuffer + sizeof(CMD_Head), wDataSize);
		*((DWORD *)(cbDataBuffer + sizeof(CMD_Head))) = m_dwSendXorKey;
		pHead->CmdInfo.wPacketSize += sizeof(DWORD);
		wDataSize += sizeof(DWORD);
	}

	//设置变量
	m_dwSendPacketCount++;
	m_dwSendXorKey = dwXorKey;

	return wDataSize;
}
WORD CClientKernel::CrevasseBuffer(BYTE cbDataBuffer[], WORD wDataSize)
{
	if (wDataSize < SIZE_CMD_HEADER) return 0;
	if (((CMD_Head *)cbDataBuffer)->CmdInfo.wPacketSize != wDataSize) return 0;

	//调整长度
	WORD wSnapCount = 0;
	if ((wDataSize % sizeof(DWORD)) != 0)
	{
		wSnapCount = sizeof(DWORD) - wDataSize % sizeof(DWORD);
		ZeroMemory(cbDataBuffer + wDataSize, wSnapCount);
	}

	//解密数据
	DWORD dwXorKey = m_dwRecvXorKey;
	DWORD * pdwXor = (DWORD *)(cbDataBuffer + sizeof(CMD_Info));
	WORD  * pwSeed = (WORD *)(cbDataBuffer + sizeof(CMD_Info));
	WORD wEncrypCount = (wDataSize + wSnapCount - sizeof(CMD_Info)) / 4;
	for (WORD i = 0; i<wEncrypCount; i++)
	{
		if ((i == (wEncrypCount - 1)) && (wSnapCount>0))
		{
			BYTE * pcbKey = ((BYTE *)&m_dwRecvXorKey) + sizeof(DWORD) - wSnapCount;
			memcpy(cbDataBuffer + wDataSize, pcbKey, wSnapCount);
		}
		dwXorKey = SeedRandMap(*pwSeed++);
		dwXorKey |= ((DWORD)SeedRandMap(*pwSeed++)) << 16;
		dwXorKey ^= g_dwPacketKey;
		*pdwXor++ ^= m_dwRecvXorKey;
		m_dwRecvXorKey = dwXorKey;
	}
	//字节映射
	CMD_Head * pHead = (CMD_Head *)cbDataBuffer;
	BYTE cbCheckCode = pHead->CmdInfo.cbCheckCode;
	for (int i = sizeof(CMD_Info); i < wDataSize; i++)
	{
		cbDataBuffer[i] = QPCipher::MapRecvByte(cbDataBuffer[i], m_cbRecvRound);
		cbCheckCode += cbDataBuffer[i];
	}
	//if (cbCheckCode != 0) throw TEXT("数据包效验码错误1");

	return wDataSize;
}
WORD CClientKernel::SeedRandMap(WORD wSeed)
{
	DWORD dwHold = wSeed;
	return (WORD)((dwHold = dwHold * 241103L + 2533101L) >> 16);
}

bool CClientKernel::OnSocketDataEvent(int nHandler, BYTE* header,BYTE* data, WORD dataSize)
{
	//包头
	memcpy(&mBufRecieve[mBufRevLength], header, sizeof(CMD_Head));
	mBufRevLength += sizeof(CMD_Head);
	//消息体
	memcpy(&mBufRecieve[mBufRevLength], data, dataSize);
	// 接收长度增加
	mBufRevLength += dataSize;

	// 尝试解包
	int iUnpackIndex = 0;
	bool isReaded = false;
	int iDstLength = 4;
	BYTE cbDataBuffer[SIZE_TCP_BUFFER * 5 + sizeof(CMD_Head)];

	CMD_Head * pHead = (CMD_Head *)mBufRecieve;

	while (mBufRevLength >= sizeof(CMD_Head) && (mBufRevLength < 65262))
	{
		iDstLength = pHead->CmdInfo.wPacketSize;
		if (mBufRevLength < iDstLength) break;

		memcpy(cbDataBuffer, mBufRecieve, iDstLength);
		mBufRevLength -= iDstLength;
		memmove(mBufRecieve, mBufRecieve + iDstLength, mBufRevLength);

		// 解包数据并通知调用
		CCmd_Data* pData = unpack(cbDataBuffer, iUnpackIndex, iDstLength);   
		if (pData)
		{
			//CCLOG("OnSocketDataEvent main:%d sub:%d", pData->GetMainCmd(), pData->GetSubCmd());
			OnCallLuaSocketCallBack(nHandler, pData);
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CClientKernel::OnSocketErrorEvent(int nHandler, int errorCode, std::string errorMessage)
{
	if (!nHandler)return false;
	bool result = false;
	do
	{
		CCmd_Data* pData = CCmd_Data::create(4);
		pData->SetCmdInfo(0, 2);
		pData->PushByteData((BYTE*)&errorCode, 4);
		pData->ResetCurrentIndex();
		result = OnCallLuaSocketCallBack(nHandler, pData);

	} while (false);
	return result;
}

bool CClientKernel::OnSocketCloseEvent(int nHandler)
{
	if (!nHandler)return false;
	bool result = false;
	do
	{
		CCmd_Data* pData = CCmd_Data::create(2);
		pData->SetCmdInfo(0, 0);
		result = OnCallLuaSocketCallBack(nHandler, pData);

	} while (false);
	return result;
}
