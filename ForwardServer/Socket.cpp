#include "Stdafx.h"
#include "Utility.h"
#include "Socket.h"
#include "Server.h"
#include "ForwardConfig.h"

#pragma comment(lib, "Mswsock.lib")

CSocket::CSocket(CServer *pServer, SOCKET socket, in_addr addr) : 
	XIOSocket(socket),
	m_pServer(pServer)
{
	m_addr = addr;
}

CSocket::~CSocket()
{
}


void CSocket::OnCreate()
{
	SOCKET hConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hConnectSocket == INVALID_SOCKET)
	{
		LOG_ERR(_T("connect socket error %d"), WSAGetLastError());
		return;
	}
	
	CServer* pServer = m_pServer;
	LOG_INFO(_T("new connection %s  to  %s:%d (%p)"), AtoT(inet_ntoa(m_addr)), pServer->m_forward_server, pServer->m_forward_port, this);

	m_pForwardSocket = new CForwardSocket(this, hConnectSocket);
	m_pForwardSocket->Initialize();
	pServer->CServer::Add(this);
	m_pForwardSocket->Connect(pServer->m_forward_server, pServer->m_forward_port);
	m_pForwardSocket->ReleaseSelf();
}

void CSocket::OnRead()
{
	char* buffer = m_pReadBuf->m_buffer;
	int		len = m_pReadBuf->m_dwSize;

	//if( len == 0)
	//{
	//	Read(len);
	//	return;
	//}
	m_pForwardSocket->Write(buffer, len);
	Read(0);
}

void CSocket::OnClose()
{
	LOG_INFO(_T("connection close (%p)"),  this);
	m_pServer->CServer::Remove( this);
	m_pForwardSocket->Shutdown();
	m_pForwardSocket = 0;
}


CForwardSocket::CForwardSocket(CSocket* pSocket, SOCKET socket)
	: XIOSocketEx(socket),
	m_pSocket(pSocket)
{
}

CForwardSocket::~CForwardSocket()
{

}

void CForwardSocket::OnConnect()
{
	LOG_INFO(_T("forward connect (%p)"), this);
	Read(0);
	m_pSocket->Read(0);
}




void CForwardSocket::OnRead()
{
	char* buffer = m_pReadBuf->m_buffer;
	int		len = m_pReadBuf->m_dwSize;

	//if( len == 0)
	//{
	//	Read(len);
	//	return;
	//}
	m_pSocket->Write(buffer, len);
	Read(0);
}

void CForwardSocket::OnClose()
{
	LOG_INFO(_T("forward connection close (%p)"), this);
	m_pSocket->Shutdown();
}

void CForwardSocket::OnCloseEx()
{
	// this function is called before m_pSocket->Read() so m_pSocket->Shutdown() is not appropriate
	LOG_INFO(_T("forward connect fail (%p)"), this);
	m_pSocket->Close();
}
