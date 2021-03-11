#include "Stdafx.h"
#include "Server.h"
#include "Socket.h"
#include "EchoConfig.h"

static XIOSpinLock g_lock;
static LINKED_LIST(CSocket, m_link) g_link;

static CServer g_server;


void CServer::Shutdown()
{
	for( ; ; )
	{
		g_lock.Lock();
		if (g_link.empty()) {
			g_lock.Unlock();
			return;
		}
		CSocket* pSocket = g_link.front();
		pSocket->AddRef();
		g_lock.Unlock();
		pSocket->Close();
		pSocket->Release();
	}
}


void CServer::Start()
{
	g_server.XIOServer::Start( CEchoConfig::s_nPort);
}

void CServer::Remove( CSocket *pSocket)
{
	g_lock.Lock();
	g_link.erase(pSocket);
	g_lock.Unlock();
}

int	CServer::Size()
{
	return g_link.size();
}

void CServer::Stop()
{
	g_server.XIOServer::Stop();
}

XIOSocket* CServer::CreateSocket( SOCKET newSocket, sockaddr_in* addr)
{
	if( CEchoConfig::s_nMaxUser <= Size())
		return NULL;
	return new CSocket( newSocket, addr->sin_addr);
}

void CServer::Add( CSocket *pSocket)
{
	g_lock.Lock();
	g_link.push_front(pSocket);
	g_lock.Unlock();
}

