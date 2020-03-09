//////////////////////////////////////////////////////////////////////////
// 加密解密类
//////////////////////////////////////////////////////////////////////////
#pragma once
#include "common/DefineGlobal.h"


#define DTP_NULL			0

/** 验证包大小 **/
static const int SIZE_VALIDATE = 136;

class QPCipher
{
public:

	static BYTE MapSendByte(BYTE const cbData, BYTE & cbSendRound);
	static BYTE MapRecvByte(BYTE const cbData, BYTE & cbRecvRound);

	static BYTE* encryptBuffer(BYTE* data, int dataSize);
	static BYTE* decryptBuffer(BYTE* data, int start, int dataSize);

	static int   getCipherMode();

	static int   getMainCommand(BYTE* data, int start);
	static int   getSubConmmand(BYTE* data, int start);
	static int   getPackSize(BYTE* data, int start);
	static void  setPackInfo(BYTE* data, int dataSize , int main, int sub);

	static int   getCipher(BYTE* data, int start);
	static int   getCipherCode(BYTE* data, int start);

	static BYTE* tcpValidate(BYTE* data, int start);
};

struct tagDataDescribe
{
	WORD							wDataSize;						//????￥?–°
	WORD							wDataDescribe;					//????√???
};

//数据包辅助类
class CSendPacketHelper
{
	//变量定义
protected:
	WORD								m_wDataSize;					//数据大小
	WORD								m_wBufferSize;					//缓冲大小
	BYTE								* m_pcbBuffer;					//缓冲指针

	//函数定义
public:
	//构造函数
	CSendPacketHelper(void * pcbBuffer, WORD wBufferSize)
	{
		m_wDataSize=0;
		m_wBufferSize=wBufferSize;
		m_pcbBuffer=(BYTE *)pcbBuffer;
	}
	//析构函数
	virtual ~CSendPacketHelper() {}

	//功能函数
public:
	//清理数据
	void CleanData() { m_wDataSize=0; }
	//获取大小
	WORD GetDataSize() { return m_wDataSize; }
	//获取缓冲
	void * GetDataBuffer() { return m_pcbBuffer; }
	//插入数据
	bool AddPacket(void * pData, WORD wDataSize, WORD wDataType)
	{
		//效验大小
		assert((wDataSize+sizeof(tagDataDescribe)+m_wDataSize)<=m_wBufferSize);
		if ((wDataSize+sizeof(tagDataDescribe)+m_wDataSize)>m_wBufferSize) return false;

		//插入数据
		assert(m_pcbBuffer!=NULL);
		tagDataDescribe * pDataDescribe=(tagDataDescribe *)(m_pcbBuffer+m_wDataSize);
		pDataDescribe->wDataSize=wDataSize;
		pDataDescribe->wDataDescribe=wDataType;

		//插入数据
		if (wDataSize>0)
		{
			assert(pData!=NULL);
			memcpy(pDataDescribe+1,pData,wDataSize);
		}

		//设置数据
		m_wDataSize+=sizeof(tagDataDescribe)+wDataSize;

		return true;
	}

};

/////////////////////////////////////////////////////////////////////////////////////////

//数据包辅助类
class CRecvPacketHelper
{
	//变量定义
protected:
	WORD								m_wDataPos;						//数据点
	WORD								m_wDataSize;					//数据大小
	BYTE								* m_pcbBuffer;					//缓冲指针

	//函数定义
public:
	//构造函数
	CRecvPacketHelper(void *pcbBuffer, WORD wDataSize)
	{
		m_wDataPos=0;
		m_wDataSize=wDataSize;
		m_pcbBuffer=(BYTE *)pcbBuffer;
	}
	//析构函数
	virtual ~CRecvPacketHelper() {}

	//功能函数
public:
	//获取数据
	void * GetData(tagDataDescribe & DataDescribe)
	{
		//效验数据
		if (m_wDataPos>=m_wDataSize) 
		{
			assert(m_wDataPos==m_wDataSize);
			DataDescribe.wDataSize=0;
			DataDescribe.wDataDescribe=DTP_NULL;
			return NULL;
		}

		//获取数据
		assert((m_wDataPos+sizeof(tagDataDescribe))<=m_wDataSize);
		memcpy(&DataDescribe,m_pcbBuffer+m_wDataPos,sizeof(tagDataDescribe));
		assert((m_wDataPos+sizeof(tagDataDescribe)+DataDescribe.wDataSize)<=m_wDataSize);

		//效验数据
		if ((m_wDataPos+sizeof(tagDataDescribe)+DataDescribe.wDataSize)>m_wDataSize)
		{
			DataDescribe.wDataSize=0;
			DataDescribe.wDataDescribe=DTP_NULL;
			return NULL;
		}

		//设置数据
		void * pData=NULL;
		if (DataDescribe.wDataSize>0) pData=m_pcbBuffer+m_wDataPos+sizeof(tagDataDescribe);
		m_wDataPos+=sizeof(tagDataDescribe)+DataDescribe.wDataSize;
		return pData;
	};
};
