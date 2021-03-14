#pragma once

#include "IOLib.h"
#include "IOSocket.h"
#include "Link.h"

#define USE_IOBUFFER

class CSocket : public XIOSocket
{
public:
	XLink	m_link;
	in_addr m_addr;
	DWORD m_dwTimeout;

	CSocket(SOCKET socket, in_addr addr);
	virtual ~CSocket();

	virtual void OnCreate();
	virtual void OnRead();
	virtual void OnClose();
#ifndef USE_IOBUFFER
	void	Write(void* buf, int len);
	virtual void OnWrite();
	virtual void OnIOCallback( BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
#endif

	in_addr GetAddr() { return m_addr; }
	void	Read(DWORD dwLeft);
	void Shutdown();
	
};

