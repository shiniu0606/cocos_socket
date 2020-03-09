#include "ClientSocket.h"
#include "common/StringUtil.h"
using asio::ip::tcp;
using namespace cocos2d;

CClientSocket::CClientSocket(int nHandler)
	: m_socket(m_ios)
	, m_bIsAlive(false)
{
	m_nHandle = nHandler;
	m_bClosed = false;
	m_bufferPtr.reset(new NetBuffer);

	kernel.OnInit();
}
CClientSocket::~CClientSocket()
{
	m_nHandle = 0;
	Director::getInstance()->getScheduler()->unscheduleAllForTarget(this);
}

int CClientSocket::sendData(CCmd_Data* pData)
{
	return SendSocketData(pData->GetMainCmd(), pData->GetSubCmd(), pData->m_pBuffer, pData->GetBufferLenght()) ? 1 : 0;
}

bool CClientSocket::Connect(const char* szUrl, unsigned short wPort, unsigned char* pValidate /*= nullptr*/)
{
	if (IsAliveServer()) return false;
	m_bIsAlive = true;
	Director::getInstance()->getScheduler()->schedule(schedule_selector(CClientSocket::Poll), this, 0, false);
	
	m_ios.reset();
	
	tcp::resolver resolver(m_ios);

	CCLOG("CClientSocket::Connect:%s,%d", szUrl, wPort);
	tcp::resolver::query query(szUrl, StringUtil::ToString(wPort), tcp::resolver::query::address_configured /*| tcp::resolver::query::numeric_host*/);
	asio::error_code ec;
	auto iterator = resolver.resolve(query, ec);
	if (iterator == tcp::resolver::iterator())
	{
		CCLOG("fail to resolve %s %d %d", szUrl, wPort, ec.value());
		do_close();
		return false;
	}
	asio::async_connect(m_socket, iterator,
		[this](std::error_code ec, tcp::resolver::iterator)
	{
		if (!ec)
		{
			do_connect();
		}
		else
		{
			kernel.OnSocketErrorEvent(m_nHandle, ec.value(), ec.message());
			do_close();
		}
	});

	return true;
}
bool CClientSocket::SendSocketData(unsigned short wMain, unsigned short wSub, BYTE* pData/* = nullptr*/, unsigned short wDataSize/* = 0*/)
{
	if (!IsAliveServer()) return false;
	auto pack = kernel.pack(wMain, wSub, pData, wDataSize);

	//m_ios.post(
	//	[this, pack]()
	//{
		bool write_in_progress = !m_pendingList.empty();
		m_pendingList.push_back(pack);
		if (!write_in_progress)
		{
			do_write();
		}
	//});

	return true;
}
void CClientSocket::StopServer()
{
	do_close();
}
bool CClientSocket::IsAliveServer()
{
	return m_bIsAlive;
}

void CClientSocket::Poll(float dt)
{
	if (m_bClosed)
	{
		CC_SAFE_RELEASE(this);
		return;
	}
	m_ios.poll();
}

void CClientSocket::do_close()
{
	if (m_bClosed)
		return;

	m_bClosed = true;
	m_bIsAlive = false;
	m_pendingList.clear();

	//kernel->OnSocketCloseEvent(m_nHandle);
	m_nHandle = 0;
	// Initiate graceful connection closure.
	asio::error_code ignored_ec;
	m_socket.shutdown(asio::ip::tcp::socket::shutdown_both,
		ignored_ec);
	m_socket.close();
}

void CClientSocket::do_connect()
{
	kernel.OnSocketConnectEvent(m_nHandle);
	do_read_header();
}

void CClientSocket::do_read_header()
{
	m_bufferPtr->ResetHead();
	asio::async_read(m_socket,
		asio::buffer(m_bufferPtr->GetHeadBuffer(), STATIC_HEAD_SIZE),
		[this](std::error_code ec, std::size_t bytes_transferred)
	{
		if (!ec)
		{
			char header[STATIC_HEAD_SIZE] = ""; 
			//std::strncat(header, (const char*)m_bufferPtr->GetHeadBuffer(), sizeof(CMD_Head));
			memcpy(header, m_bufferPtr->GetHeadBuffer(), STATIC_HEAD_SIZE);
			CMD_Head * pHead = (CMD_Head *)header;
			
			if (bytes_transferred <= 0 || pHead->CmdInfo.wPacketSize < sizeof(CMD_Head))
			{
				CCLOG("wPacketSize is error");
				do_read_header();
			}
			else
			{
				m_bodyLength = pHead->CmdInfo.wPacketSize - sizeof(CMD_Head);
				if (m_bodyLength > 0)
				{
					do_read_body();
				}
				else
				{
					kernel.OnSocketDataEvent(m_nHandle, m_bufferPtr->GetHeadBuffer(), m_bufferPtr->GetBuffer(), m_bodyLength);

					do_read_header();
				}
			}
			
		}
		else
		{
			kernel.OnSocketErrorEvent(m_nHandle, (int)ec.value(), ec.message().c_str());
			do_close();
		}
	});
}

void CClientSocket::do_read_body()
{
	m_bufferPtr->Resize(m_bodyLength);
	asio::async_read(m_socket,
		asio::buffer(m_bufferPtr->GetBuffer(), m_bodyLength),
		[this](std::error_code ec, std::size_t length)
	{
		if (!ec && m_bodyLength != 0)
		{
			kernel.OnSocketDataEvent(m_nHandle, m_bufferPtr->GetHeadBuffer(),m_bufferPtr->GetBuffer(), m_bodyLength);

			do_read_header();
		}
		else
		{
			kernel.OnSocketErrorEvent(m_nHandle,(int)ec.value(), ec.message().c_str());
			do_close();
		}
	});
}

void CClientSocket::do_write()
{
	if (!IsAliveServer()) return;
	asio::async_write(m_socket,
		asio::buffer(m_pendingList.front().data,
			m_pendingList.front().size),
		[this](std::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			m_pendingList.pop_front();
			if (!m_pendingList.empty())
			{
				do_write();
			}
		}
		else
		{
			kernel.OnSocketErrorEvent(m_nHandle, (int)ec.value(), ec.message().c_str());
			do_close();
		}
	});
}