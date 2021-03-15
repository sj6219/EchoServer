#include "Stdafx.h"
#include "Utility.h"
#include "Server.h"
#include "Socket.h"
#include "EchoConfig.h"

static XRWLock g_lock;
static LINKED_LIST(CSocket, m_link) g_link;

static CServer g_server;


void CServer::Shutdown()
{
	XSharedLock<XRWLock> lock(g_lock);
	for (CSocket* pSocket : g_link)
		pSocket->Shutdown();
}


void CServer::Start()
{
	g_server.XIOServer::Start( CEchoConfig::s_nPort);
	g_server.AddTimer(1000);
}

void CServer::Remove( CSocket *pSocket)
{
	XUniqueLock<XRWLock> lock(g_lock);
	g_link.erase(pSocket);
}

int	CServer::Size()
{
	return g_link.size();
}

void CServer::Stop()
{
	g_server.XIOServer::Stop();
	CServer::Shutdown();
}

XIOSocket* CServer::CreateSocket( SOCKET newSocket, sockaddr_in* addr)
{
	if( CEchoConfig::s_nMaxUser <= Size())
		return NULL;
	return new CSocket( newSocket, addr->sin_addr);
}

void CServer::Add( CSocket *pSocket)
{
	XUniqueLock<XRWLock> lock(g_lock);
	g_link.push_back(pSocket);
}

void	CServer::OnTimer(int nId)
{
	{
		XSharedLock<XRWLock> lock(g_lock);

		DWORD dwTick = GetTickCount();
		for (CSocket* pSocket : g_link) {
			int dwTimeout = dwTick - pSocket->m_dwTimeout;
			if (dwTimeout > 0) {
				pSocket->Shutdown();
			}
		}
	}
	AddTimer(1000);
}

