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

class CConnectSocket : public XIOSocket
{
	OVERLAPPED m_overlappedConnect;
public:
	CConnectSocket(SOCKET socket);
	virtual ~CConnectSocket();
	virtual void OnCreate();
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	virtual void OnConnect() = 0;
	bool Connect(LPCTSTR server, int port);
};

class CForwardSocket : public CConnectSocket
{

public:
	XAutoVar<CSocket> m_pSocket;

	CForwardSocket(CSocket* pSocket, SOCKET socket);
	virtual ~CForwardSocket();

	virtual void OnRead();
	virtual void OnClose();
	void	OnConnect();

};
