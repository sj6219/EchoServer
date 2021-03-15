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
	LOG_INFO(_T("new connection %s"), AtoT(inet_ntoa(m_addr)));
	
	SOCKET hConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hConnectSocket == INVALID_SOCKET)
	{
		LOG_ERR(_T("connect socket error %d"), WSAGetLastError());
		return;
	}
	
	CServer* pServer = m_pServer;

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
	m_pSocket->Shutdown();
}
