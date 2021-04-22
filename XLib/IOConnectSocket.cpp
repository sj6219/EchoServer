#include "pch.h"
#include "IOConnectSocket.h"
#include "Utility.h"
#include <mswsock.h>



XIOConnectSocket::XIOConnectSocket(SOCKET socket)
	: XIOSocket(socket)
{
}

XIOConnectSocket::~XIOConnectSocket()
{

}

void XIOConnectSocket::OnCreate()
{

}

void XIOConnectSocket::OnConnect()
{
	Read(0);
}

bool XIOConnectSocket::Connect(LPCTSTR server, int port)
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

void XIOConnectSocket::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if (lpOverlapped == &m_overlappedConnect) {
		if (!bSuccess)
		{
			if (m_hSocket != INVALID_SOCKET) {
				LOG_ERR(_T("connect callback error %d"), GetLastError());
				Close(); // ConnectFail();
			}
			ReleaseIO();
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

//void XIOConnectSocket::ConnectFail()
//{
//	m_lock.Lock();
//	SOCKET hSocket = m_hSocket;
//	m_hSocket = INVALID_SOCKET;
//	m_lock.Unlock();
//	if (hSocket != INVALID_SOCKET)
//	{
//		OnConnectFail();
//		//		LINGER linger;
//		//		linger.l_onoff = 1;
//		//		linger.l_linger = 0;
//		//		setsockopt(hSocket, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
//		closesocket(hSocket);
//		ReleaseSelf();
//	}
//}

