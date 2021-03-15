#pragma once

#include "IOSocket.h"

class XIOSocketEx : public XIOSocket
{
	OVERLAPPED m_overlappedConnect;
public:
	XIOSocketEx(SOCKET socket);
	virtual ~XIOSocketEx();
	virtual void OnCreate();
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	virtual void OnConnect();
	bool Connect(LPCTSTR server, int port);
};
