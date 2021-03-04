#include "Stdafx.h"
#include "update.h"
#include "Server.h"
#include "Socket.h"
#include "EchoConfig.h"

static XIOSpinLock g_lock;
static XLink	g_link;
static int g_nSocket;

static CServer g_server;


void CServer::Shutdown()
{
	for( ; ; )
	{
		g_lock.Lock();
		XLink* pLink = g_link.m_pNext;
		if( pLink == &g_link)
		{
			g_lock.Unlock();
			return;
		}
		CSocket* pSocket = LINK_POINTER( pLink, CSocket, m_link);
		pSocket->AddRef();
		g_lock.Unlock();
		pSocket->Close();
		pSocket->Release();
	}
}


void CServer::Start()
{
	g_server.XIOServer::Start( CEchoConfig::s_nPort);
	g_link.Initialize();
}

void CServer::Remove( CSocket *pSocket)
{
	g_lock.Lock();
	pSocket->m_link.Remove();
	g_nSocket--;
	g_lock.Unlock();
}

int	CServer::Size()
{
	return g_nSocket;
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
	pSocket->m_link.Insert( &g_link);
	g_nSocket++;
	g_lock.Unlock();
}

