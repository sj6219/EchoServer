#pragma once

#include "IOLib.h"
#include "IOSocket.h"
#include "Misc.h"

class CSocket : public XIOSocket
{
public:
	XLink	m_link;
	virtual void OnCreate();
	virtual void OnRead();
	virtual void OnClose();
	virtual void OnWrite();
	virtual void OnIOCallback( BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);

	CSocket( SOCKET socket, in_addr addr);
	virtual ~CSocket();
	in_addr GetAddr() { return m_addr; }
	void	Write( void* buf, int len);
	void	Read(DWORD dwLeft);
	virtual void OnTimer(int nId);
	
	in_addr m_addr;
	DWORD m_dwTimeout;
};

