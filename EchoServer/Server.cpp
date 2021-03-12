#include "Stdafx.h"
#include "Utility.h"
#include "Server.h"
#include "Socket.h"
#include "EchoConfig.h"

static XLock g_lock;
static LINKED_LIST(CSocket, m_link) g_link;

static CServer g_server;


void CServer::Shutdown()
{
	for( ; ; )
	{
		g_lock.lock();
		if (g_link.empty()) {
			g_lock.unlock();
			return;
		}
		CSocket* pSocket = g_link.front();
		pSocket->AddRef();
		g_lock.unlock();
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
	XUniqueLock<XLock> lock(g_lock);
	g_link.erase(pSocket);
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
	XUniqueLock<XLock> lock(g_lock);
	g_link.push_front(pSocket);
}

