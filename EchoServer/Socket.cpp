#include "Stdafx.h"
#include "Utility.h"
#include "Socket.h"
#include "Server.h"
#include "EchoConfig.h"

#pragma comment(lib, "Mswsock.lib")

CSocket::CSocket(SOCKET socket, in_addr addr) : XIOSocket(socket)
{
	m_addr = addr;
}

CSocket::~CSocket()
{
}

void	CSocket::Read(DWORD dwLeft)
{
	m_dwTimeout = GetTickCount() + 60000;
	XIOSocket::Read(dwLeft);
}


void CSocket::OnCreate()
{
	LOG_INFO(_T("new connection from %s (%p)"), AtoT(inet_ntoa(m_addr)), this);
	m_dwTimeout = GetTickCount() + 0x7fffffff;
	CServer::Add(this);

	Read(0);
}

void CSocket::OnRead()
{
	char* buffer = m_pReadBuf->m_buffer;
	int		len = m_pReadBuf->m_dwSize;

	if( len == 0)
	{
		Read(len);
		return;
	}
	if (m_nPendingWrite > 100 * 1024) {
		Close();
		return;
	}
	Write(buffer, len);
#ifdef USE_IOBUFFER
	Read(0);
#endif
}

void CSocket::OnClose()
{
	LOG_INFO(_T("connection close (%p)"), this);
	CServer::Remove( this);
}

#ifndef USE_IOBUFFER
void CSocket::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if (!bSuccess)
	{
		if (lpOverlapped == &m_overlappedRead) // closed only at read
			Close();
		else if (lpOverlapped == &m_overlappedWrite)
			FreeBuffer();
		ReleaseIO();
	}
	else if (lpOverlapped == &m_overlappedRead)
	{
		ReadCallback(dwTransferred);
		ReleaseIO();
	}
	else if (lpOverlapped == &m_overlappedWrite)
	{
		OnWrite(); // WriteCallback(dwTransferred);
		ReleaseIO();
	}
	else
	{
		_ASSERT(lpOverlapped == NULL);
		OnTimer(dwTransferred);
		ReleaseTimer();	// ��TimerRef
	}
}

void CSocket::Write(void* buf, int len)
{
	AddRefIO();
	WSABUF wsabuf;
	wsabuf.buf = (char*)buf;
	wsabuf.len = len;
	DWORD dwSent;
	m_dwTimeout = GetTickCount() + 0x7fffffff;
	m_lock.Lock();
	if (WSASend(m_hSocket, &wsabuf, 1, &dwSent, 0, &m_overlappedWrite, NULL)
		&& GetLastError() != ERROR_IO_PENDING)
	{
		m_lock.Unlock();
		int nErr = GetLastError();
		if (nErr != WSAENOTSOCK && nErr != WSAECONNRESET && nErr != WSAECONNABORTED)
			LOG_ERR(_T("CSocket::WritePacket %x(%x) err=%d"), m_hSocket, this, nErr);
		Close();
		ReleaseIO();
	}
	else
		m_lock.Unlock();
}

void CSocket::OnWrite()
{
	Read(0);
}

#endif
