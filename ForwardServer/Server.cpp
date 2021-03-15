#include "Stdafx.h"
#include "Utility.h"
#include "Server.h"
#include "Socket.h"
#include "ForwardConfig.h"


static std::vector<CServer*> g_server_list;


CServer::CServer(LPCTSTR server, int port)
	: m_server(server),
	m_port(port)
{

}

void CServer::Start()
{
	for (auto forward : CForwardConfig::s_vForwardList) {
		CServer* pServer = new CServer(forward.m_forward_server.c_str(), forward.m_forward_port);
		pServer->XIOServer::Start(forward.m_port);
		g_server_list.push_back(pServer);
	}
}

void CServer::Stop()
{
	for (CServer* pServer : g_server_list) {
		pServer->XIOServer::Stop();
		pServer->Shutdown();
		pServer->ReleaseSelf();
	}
}

void CServer::Shutdown()
{
	for (; ; )
	{
		m_lock.LockShared();
		if (m_link.empty()) {
			m_lock.UnlockShared();
			return;
		}
		CSocket* pSocket = m_link.front();
		pSocket->AddRef();
		m_lock.UnlockShared();
		pSocket->Close();
		pSocket->Release();
	}
}

void CServer::Remove( CSocket *pSocket)
{
	XUniqueLock<XRWLock> lock(m_lock);
	m_link.erase(pSocket);
}

int	CServer::Size()
{
	return m_link.size();
}


XIOSocket* CServer::CreateSocket( SOCKET newSocket, sockaddr_in* addr)
{
	if( CForwardConfig::s_nMaxUser <= Size())
		return NULL;
	return new CSocket( this, newSocket, addr->sin_addr);
}

void CServer::Add( CSocket *pSocket)
{
	XUniqueLock<XRWLock> lock(m_lock);
	m_link.push_back(pSocket);
}

