#pragma once

#include "IOLib.h"
#include "IOSocket.h"

class CSocket;

class CServer : public XIOServer
{
public:
	static size_t Size();
	static void Stop();
	static void Shutdown();
	static void Start();
	static void Add( CSocket *pSocket);
	static void Remove( CSocket *pSocket);
	virtual XIOSocket* CreateSocket(SOCKET newSocket, sockaddr_in* addr);
	virtual void OnTimer(int nId);
};

