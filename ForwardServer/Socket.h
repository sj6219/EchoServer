#pragma once

#include "IOLib.h"
#include "IOSocket.h"
#include "Link.h"

#define USE_IOBUFFER

class CServer;
class CForwardSocket;

class CSocket : public XIOSocket
{
public:
	XLink	m_link;
	in_addr m_addr;
	XAutoVar<CServer> m_pServer;
	XAutoVar<CForwardSocket> m_pForwardSocket;

	CSocket(CServer *pServer, SOCKET socket, in_addr addr);
	virtual ~CSocket();

	virtual void OnCreate();
	virtual void OnRead();
	virtual void OnClose();

	in_addr GetAddr() { return m_addr; }
	
};

class CForwardSocket : public XIOSocket
{
	OVERLAPPED m_overlappedConnect;

public:
	XAutoVar<CSocket> m_pSocket;
	CForwardSocket(CSocket* pSocket, SOCKET socket);
	virtual ~CForwardSocket();

	virtual void OnCreate();
	virtual void OnRead();
	virtual void OnClose();
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	void	OnConnect();
	bool Connect();

};
