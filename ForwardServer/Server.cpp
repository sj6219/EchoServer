#include "Stdafx.h"
#include "Utility.h"
#include "Server.h"
#include "Socket.h"
#include "ForwardConfig.h"

static XRWLock g_lock;
static LINKED_LIST(CSocket, m_link) g_link;

static CServer g_server;


void CServer::Shutdown()
{
	for( ; ; )
	{
		g_lock.LockShared();
		if (g_link.empty()) {
			g_lock.UnlockShared();
			return;
		}
		CSocket* pSocket = g_link.front();
		pSocket->AddRef();
		g_lock.UnlockShared();
		pSocket->Close();
		pSocket->Release();
	}
}


void CServer::Start()
{
	g_server.XIOServer::Start(50004);
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
}

XIOSocket* CServer::CreateSocket( SOCKET newSocket, sockaddr_in* addr)
{
	if( CForwardConfig::s_nMaxUser <= Size())
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

