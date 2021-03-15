#include "Stdafx.h"
#include "Utility.h"
#include "Socket.h"
#include "Server.h"
#include "ForwardConfig.h"
#include <MSWSock.h>

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
	: CConnectSocket(socket),
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



CConnectSocket::CConnectSocket(SOCKET socket)
	: XIOSocket(socket)
{
}

CConnectSocket::~CConnectSocket()
{

}

void CConnectSocket::OnCreate()
{

}

bool CConnectSocket::Connect(LPCTSTR server, int port)
{
	LPFN_CONNECTEX ConnectEx;
	GUID guid = WSAID_CONNECTEX;
	DWORD dwBytes;
	WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid),
		&ConnectEx, sizeof(ConnectEx),
		&dwBytes, NULL, NULL);

	/* ConnectEx requires the socket to be initially bound. */
	{
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = 0;
		bind(m_hSocket, (SOCKADDR*)&addr, sizeof(addr));
	}

	/* Issue ConnectEx and wait for the operation to complete. */
	{
		ZeroMemory(&m_overlappedConnect, sizeof(m_overlappedConnect));

		sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(TtoA(server));
		addr.sin_port = htons(port);

		//connect(m_hSocket, (SOCKADDR*)&addr, sizeof(addr));
		AddRefIO();
		if (!ConnectEx(m_hSocket, (SOCKADDR*)&addr, sizeof(addr), NULL, 0, NULL, &m_overlappedConnect)
			&& GetLastError() != ERROR_IO_PENDING)
		{
			ReleaseIO();
			LOG_ERR(_T("ConnectEx error %d"), WSAGetLastError());
			goto fail;
		}
	}
	return true;
fail:
	Close();
	return false;

}

void CConnectSocket::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if (lpOverlapped == &m_overlappedConnect) {
		if (!bSuccess)
		{
			if (m_hSocket == INVALID_SOCKET)
				return;
			LOG_ERR(_T("connect callback error %d"), GetLastError());
			Close();
			return;
		}
		setsockopt(m_hSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		OnConnect();
		ReleaseIO();
	}
	else {
		XIOSocket::OnIOCallback(bSuccess, dwTransferred, lpOverlapped);
	}
}
