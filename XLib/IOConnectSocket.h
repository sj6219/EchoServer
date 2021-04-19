#pragma once

#include "IOSocket.h"

class XIOConnectSocket : public XIOSocket
{
	OVERLAPPED m_overlappedConnect;
public:
	XIOConnectSocket(SOCKET socket);
	virtual ~XIOConnectSocket();
	virtual void OnCreate();
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	virtual void OnConnect();
	virtual void OnConnectFail();
	bool Connect(LPCTSTR server, int port);
	void ConnectFail();
};
