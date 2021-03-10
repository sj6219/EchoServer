#include "Stdafx.h"
#include "Socket.h"
#include "Server.h"
#include "EchoConfig.h"
#define LOCAL_ADDRESS 0x0100007f
#pragma comment(lib, "Mswsock.lib")

CSocket::CSocket(SOCKET socket, in_addr addr) : XIOSocket(socket)
{
	m_addr = addr;
	//m_hFile = INVALID_HANDLE_VALUE;
	//m_pNotice = CNotice::GetNotice();
}

CSocket::~CSocket()
{
	//CloseHandle( m_hFile);
//	m_pNotice->Release();
}

void	CSocket::OnTimer(int nId)
{
	int dwTimeout = GetTickCount() - m_dwTimeout;
	if (dwTimeout > 0) {
		Close(); 
		return;
	}
	AddTimer(10000);
}

void	CSocket::Read(DWORD dwLeft)
{
	m_dwTimeout = GetTickCount() + 60000;
	XIOSocket::Read(dwLeft);
}

void CSocket::Write( void* buf, int len)
{
	AddRefIO();
	WSABUF wsabuf;
	wsabuf.buf = (char*)buf;
	wsabuf.len = len;
	DWORD dwSent;
	m_dwTimeout = GetTickCount() + 0x7fffffff;
	m_lock.Lock();
	if( WSASend( m_hSocket, &wsabuf, 1, &dwSent, 0, &m_overlappedWrite, NULL)
		&& GetLastError( ) != ERROR_IO_PENDING)
	{
		m_lock.Unlock();
		int nErr = GetLastError();
		if( nErr != WSAENOTSOCK && nErr != WSAECONNRESET && nErr != WSAECONNABORTED)
			LOG_ERR( _T("CSocket::WritePacket %x(%x) err=%d"), m_hSocket, this, nErr);
		Close();
		ReleaseIO();
	}
	else
		m_lock.Unlock();
}

void CSocket::OnIOCallback( BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if( !bSuccess)
	{ 
		if (lpOverlapped == &m_overlappedRead) // closed only at read
			Close();
		else if (lpOverlapped == &m_overlappedWrite)
			FreeBuffer();
		ReleaseIO();
	}
	else if( lpOverlapped == &m_overlappedRead)
	{
		ReadCallback(dwTransferred);
		ReleaseIO();
	}
	else if( lpOverlapped == &m_overlappedWrite)
	{
		OnWrite(); // WriteCallback(dwTransferred);
		ReleaseIO();
	}
	else
	{
		_ASSERT( lpOverlapped == NULL);
		OnTimer( dwTransferred);
		ReleaseTimer();	// ��TimerRef
	}
}







void CSocket::OnWrite()
{
	Read(0);
}









void CSocket::OnCreate()
{
	CONVERT;
	LOG_INFO( _T("new connection %s"), A_T(inet_ntoa( m_addr)));
	CServer::Add( this);

	AddTimer(10000);
	Read(0);
}





void CSocket::OnRead()
{
		int		len = m_pReadBuf->m_dwSize;
		if( len == 0)
		{
			Read(len);
			return;
		}
		Write(m_pReadBuf->m_buffer, len);
//	LOG_ERR( _T("Invalid packet at CSocket::OnRead"));
//	Close();
}

void CSocket::OnClose()
{
	CServer::Remove( this);
}
