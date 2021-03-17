#pragma once

#include "IOLib.h"
#include "IOSocket.h"
#include "Socket.h"
#include "Utility.h"

class CServer : public XIOServer
{
	LPCTSTR m_forward_server;
	int		m_forward_port;
	XRWLock m_lock;
	LINKED_LIST(CSocket, m_link) m_link;

	friend class CSocket;
	friend class CForwardSocket;

public:
	CServer(LPCTSTR server, int port);
	~CServer();
	int Size();
	static void Start();
	static void Stop();
	void Shutdown();
	void Add( CSocket *pSocket);
	void Remove( CSocket *pSocket);
	virtual XIOSocket* CreateSocket(SOCKET newSocket, sockaddr_in* addr);
};

