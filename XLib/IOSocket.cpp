#include "pch.h"
#include "IOSocket.h"
#include "IOLib.h"
#include "IOException.h"
#include "Utility.h"
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <queue>
#include <process.h>
#include <mswsock.h>


#define BUFFER_POOL_SIZE 16
#define EXIT_IO_THREAD

#pragma warning(disable: 4073)
#pragma init_seg(lib) // XIOSocket::CInit::~CInit

// g_buffer_stack shoud be initialized before theInit 
// because g_buffer_stack shold be freed after XIOBuffer freed
LockfreeStack<XIOBuffer *> g_buffer_stack;

// #pragma init_seg(lib) is required 
// because XIOBuffer cache shoud be freed after all XIOSocket freed
static XIOSocket::CInit theInit;



//#pragma optimize("gt", on)
//#pragma optimize("y", off)

static HANDLE g_vHandle[MAXIMUM_WAIT_OBJECTS];
static XIOObject *g_vObject[MAXIMUM_WAIT_OBJECTS];
static int g_nHandle;
static HANDLE *g_hThread;
static unsigned *g_nThreadId;
static int g_nThread;
static HANDLE	g_hWaitThread;
long XIOSocket::s_nRunningThread;
HANDLE XIOSocket::s_hCompletionPort;
static long g_nTerminating;
static HANDLE g_hTimer;
XLock g_lockTimer;
static DWORD g_dwTopTime;

typedef std::priority_queue<XIOSocket::XIOTimer> TimerQueue;
static TimerQueue g_timerQueue;

XIOObject::~XIOObject()
{
}

BOOL	XIOObject::RegisterWait(HANDLE handle)
{
	g_lockTimer.Lock();
	if (g_nHandle >= MAXIMUM_WAIT_OBJECTS)
	{
		g_lockTimer.Unlock();
		return FALSE;
	}
	g_vHandle[g_nHandle] = handle;
	g_vObject[g_nHandle] = this;
	g_nHandle++;
	SetEvent(g_hTimer);
	g_lockTimer.Unlock();
	return TRUE;
}


void XIOObject::AddTimer(DWORD dwTime, int nId)
{
	AddRefTimer();	// ¢¾TimerRef
	dwTime += GetTickCount();

	g_lockTimer.Lock();

	g_timerQueue.push(XIOSocket::XIOTimer(this, dwTime, nId));
	if ((LONG)+(g_dwTopTime - dwTime) > 0)
	{
		g_dwTopTime = dwTime;
		g_lockTimer.Unlock();
		SetEvent(g_hTimer);
	}
	else
		g_lockTimer.Unlock();
}

void XIOObject::OnTimerCallback(int nId)
{
	if (!PostQueuedCompletionStatus(XIOSocket::s_hCompletionPort, nId, (ULONG_PTR)this, NULL))
	{
		EBREAK();
		ReleaseTimer();
	}
}

void XIOObject::OnIOCallback(BOOL bSucess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	ASSERT(!lpOverlapped);
	OnTimer(dwTransferred);
	ReleaseTimer();
}



long XIOBuffer::s_nCount;

XIOBuffer *XIOBuffer::Alloc()
{
	XIOBuffer *newBuffer = g_buffer_stack.Pop();
	if (newBuffer == NULL)
	{
		newBuffer = new XIOBuffer;
	}
	newBuffer->m_dwSize = 0;
	newBuffer->m_nRef = 1;
	newBuffer->m_pNext = NULL;
	return newBuffer;
}

void XIOBuffer::Free()
{
	g_buffer_stack.Push(this);
}

void XIOBuffer::FreeAll()
{
	for (;;)
	{
		XIOBuffer *pBuffer = g_buffer_stack.Pop();
		if (pBuffer == NULL)
			break;
		delete pBuffer;
	}
}

static XIOSocket::XIOTimerInstance g_instance;

void XIOSocket::XIOTimerInstance::OnTimerCallback(int nId)
{
	g_timerQueue.push(XIOSocket::XIOTimer(this, GetTickCount() + 24 * 3600 * 1000, nId));
	g_dwTopTime = g_timerQueue.top().m_dwTime;
}

void XIOSocket::XIOTimerInstance::OnIOCallback(BOOL bSucess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
#ifdef EXIT_IO_THREAD
	if (dwTransferred == 0)
		SetEvent(g_hTimer);
	else
		g_instance.PostObject(dwTransferred - 1, 0);
	InterlockedDecrement(&s_nRunningThread);
	//SuspendThread(GetCurrentThread());
	ExitThread(0);
#endif
}

XIOSocket::CInit::~CInit()
{
	XIOSocket::Stop();
}

void XIOSocket::Start()
{
}

void XIOSocket::Stop()
{
	// #pragma init_seg(lib) is required because all XIOSocket should be freed before XIOBuffer be freed 
	XIOBuffer::FreeAll();	 
}

XIOSocket::XIOSocket(SOCKET s)
{
	m_hSocket = s;
	if (m_hSocket != INVALID_SOCKET)
		AddRefSelf();
	memset(&m_overlappedRead, 0, sizeof(OVERLAPPED));
	memset(&m_overlappedWrite, 0, sizeof(OVERLAPPED));
	m_pReadBuf = XIOBuffer::Alloc();
	m_nPendingWrite = 0;
	m_pFirstBuf = NULL;
}

XIOSocket::~XIOSocket()
{
	m_pReadBuf->Release();
	if (!EASSERT(!m_pFirstBuf))
		FreeBuffer();
}

void XIOSocket::Initialize()
{
	if (CreateIoCompletionPort((HANDLE)m_hSocket, s_hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		ELOG(_T("CreateIoCompletionPort: %d %x %x"), GetLastError(), m_hSocket, s_hCompletionPort);
		Close();
		return;
	}
	//int zero = 0;
	//setsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&zero, sizeof(zero));
	//zero = 0;
	//setsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero));

	OnCreate();
}
void XIOSocket::Write(void* buffer, DWORD size)
{
	char* buf = (char*)buffer;
	while (size)
	{
		XIOBuffer* pBuffer = XIOBuffer::Alloc();
		DWORD n = std::min<DWORD>(size,  BUFFER_SIZE);
		memcpy(pBuffer->m_buffer, buf, n);
		pBuffer->m_dwSize = n;
		Write(pBuffer);
		buf += n;
		size -= n;
	}
}

void XIOSocket::OnCreate()
{
	Read(0); // Read must be called last because that can cause OnRead()
}

void XIOSocket::ReadCallback(DWORD dwTransferred)
{
	if (dwTransferred == 0)
	{
		Close();
		return;
	}
	m_pReadBuf->m_dwSize += dwTransferred;
	OnRead();
}

unsigned __stdcall XIOSocket::IOThread(void* arglist)
{
	SetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS);
	for (; ; )
	{
		DWORD dwTransferred;
		XIOObject* pObject;
		LPOVERLAPPED lpOverlapped;

		BOOL bSuccess = GetQueuedCompletionStatus(s_hCompletionPort, &dwTransferred, (PULONG_PTR)&pObject, &lpOverlapped, INFINITE);

		//		if (InterlockedIncrement(&s_nRunningThread) == g_nThread)
		//			    EASSERT(g_nThread);
		InterlockedIncrement(&s_nRunningThread);
		pObject->OnIOCallback(bSuccess, dwTransferred, lpOverlapped);
		InterlockedDecrement(&s_nRunningThread);
	}
	//	return 0;
}

void XIOSocket::DumpStack()
{
	XIOException::DumpStack(g_nThread, g_hThread, g_nThreadId);
	XIOException::SendMail();
}

unsigned __stdcall XIOSocket::WaitThread(void *)
{
	for ( ; ; )
	{
		DWORD dwTick = GetTickCount();
		LONG nWait = (LONG)(g_dwTopTime - dwTick);
		if (nWait <= 0)
		{
			g_lockTimer.Lock();
			const XIOTimer &top = g_timerQueue.top();
			XIOObject *pObject = top.m_pObject;
			int id = top.m_nId;
			for ( ; ; )
			{
				g_timerQueue.pop();
				pObject->OnTimerCallback(id);

				const XIOTimer& top = g_timerQueue.top();
				nWait = (LONG)(top.m_dwTime - dwTick);
				if (nWait > 0)
				{
					g_dwTopTime = top.m_dwTime;
					break;
				}
				pObject = top.m_pObject;
				id = top.m_nId;
			}
			g_lockTimer.Unlock();
		}

		DWORD dwWaitResult = WaitForMultipleObjects(g_nHandle, g_vHandle, FALSE, nWait);

		if (g_nTerminating)
		{
			g_lockTimer.Lock();
			for ( ; ; )
			{
				if (g_timerQueue.empty())
					break;
				XIOObject *pObject = g_timerQueue.top().m_pObject;
				pObject->ReleaseTimer();	
				g_timerQueue.pop();
			}
			g_lockTimer.Unlock();
#ifdef EXIT_IO_THREAD
			// wait IOThread 
			g_instance.PostObject(g_nThread - 1, 0);
			WaitForSingleObject(g_hTimer, INFINITE);
#endif 

			// processing pending io
			for (; ; )
			{
				DWORD dwTransferred;
				XIOObject* pObject;
				LPOVERLAPPED lpOverlapped;

				BOOL bSuccess = GetQueuedCompletionStatus(s_hCompletionPort, &dwTransferred, (PULONG_PTR)&pObject, &lpOverlapped, 10);
				if (!bSuccess && GetLastError() == WAIT_TIMEOUT)
					break;
				pObject->OnIOCallback(bSuccess, dwTransferred, lpOverlapped);
			}

			CloseHandle(g_hTimer);
			return 0;
		}

		if (dwWaitResult >= WAIT_OBJECT_0 && dwWaitResult < WAIT_OBJECT_0 + g_nHandle)
		{
			dwWaitResult -= WAIT_OBJECT_0;
			g_vObject[dwWaitResult]->OnWaitCallback();
		}
	}
}

#ifdef XIOOBJECT_DEBUG

void XIOObject::AddRef(LONG volatile *pRef)
{
	if (InterlockedIncrement(&m_nRef) == 1) {
		EBREAK();
		m_nRef = 100;
	}
	InterlockedIncrement(pRef);
}

void XIOObject::Release(LONG volatile *pRef)
{
	long nRef1 = InterlockedDecrement(pRef);
	if (nRef1 < 0)
	{
		ELOG(_T("XIOObject::Release %p %p %x %d %d"), this, *(DWORD *)this, (char *)pRef - (char *)this, nRef1, m_nRef);
		EBREAK();
		*pRef = 100;
		m_nRef = 100;
		return;
	}
	long nRef2 = InterlockedDecrement(&m_nRef);
	if (nRef2 > 0)
		return;
	if (nRef2 < 0)
	{
		ELOG(_T("XIOObject::Release %p %p %d"), this, *(DWORD *) this, nRef2);
		EBREAK();
		m_nRef = 100;
		return;
	}
	if (*pRef != 0)
	{
		ELOG(_T("XIOObject::Release %p %p %x %d"), this, *(DWORD *)this, (char *)pRef - (char *)this, *pRef);
		EBREAK();
		*pRef = 100;
		m_nRef = 100;
		return;
	}
	OnFree();
}

#endif	// XIOOBJECT_DEBUG

void XIOSocket::WriteCallback(DWORD dwTransferred)
{
	m_lock.Lock();
	if (dwTransferred != m_pFirstBuf->m_dwSize)
	{
		LOG_ERR(_T("different write count %x(%x) %d != %d"), m_hSocket, this, dwTransferred, m_pFirstBuf->m_dwSize);
		m_lock.Unlock();
		FreeBuffer();
		return;	    
	}
	m_nPendingWrite -= m_pFirstBuf->m_dwSize;
	XIOBuffer* pFirstBuf = m_pFirstBuf;
	if ((m_pFirstBuf = m_pFirstBuf->m_pNext) != NULL)
	{
		//m_lock.Unlock();
		AddRefIO();
		WSABUF wsabuf;
		wsabuf.len = m_pFirstBuf->m_dwSize;
		wsabuf.buf = m_pFirstBuf->m_buffer;
		DWORD dwSent;
		if (WSASend(m_hSocket, &wsabuf, 1, &dwSent, 0, &m_overlappedWrite, NULL)
			&& GetLastError() != ERROR_IO_PENDING)
		{
			int nErr = GetLastError();
			m_lock.Unlock();
			if (nErr != WSAENOTSOCK && nErr != WSAECONNRESET && nErr != WSAECONNABORTED && nErr != WSAESHUTDOWN
				&& nErr != WSAEINVAL)
				LOG_ERR(_T("XIOSocket::WriteCallback %#x(%#x) err=%d"), m_hSocket, *(DWORD *)this, nErr);
			FreeBuffer();
			ReleaseIO();
		}
		else
			m_lock.Unlock();
	}
	else
	{
		m_lock.Unlock();
	}
	pFirstBuf->Free();
}


void XIOSocket::WriteWithLock(XIOBuffer *pBuffer)
{
	if (pBuffer->m_dwSize == 0)
	{
		m_lock.Unlock();
		pBuffer->Free();
		return;
	}
	EASSERT(pBuffer->m_dwSize <= BUFFER_SIZE);
	m_nPendingWrite += pBuffer->m_dwSize;
	if (m_pFirstBuf == NULL)
	{
		m_pFirstBuf = m_pLastBuf = pBuffer;
//		m_lock.Unlock();
		AddRefIO();
		WSABUF wsabuf;
		wsabuf.len = pBuffer->m_dwSize;
		wsabuf.buf = pBuffer->m_buffer;
		DWORD dwSent;
		if (WSASend(m_hSocket, &wsabuf, 1, &dwSent, 0, &m_overlappedWrite, NULL)
			&& GetLastError() != ERROR_IO_PENDING)
		{
			int nErr = GetLastError();
			m_lock.Unlock();
			if (nErr != WSAENOTSOCK && nErr != WSAECONNRESET && nErr != WSAECONNABORTED && nErr != WSAESHUTDOWN
				&& nErr != WSAEINVAL)
				LOG_ERR(_T("XIOSocket::Write %#x(%#x) err=%d"), m_hSocket, *(DWORD *)this, nErr);
			FreeBuffer();
			ReleaseIO(); 
		}
		else
			m_lock.Unlock();
	}
	else if (m_pFirstBuf != m_pLastBuf && m_pLastBuf->m_dwSize + pBuffer->m_dwSize <= BUFFER_SIZE)
	{
		memcpy(m_pLastBuf->m_buffer + m_pLastBuf->m_dwSize, pBuffer->m_buffer, pBuffer->m_dwSize);
		m_pLastBuf->m_dwSize += pBuffer->m_dwSize;
		m_lock.Unlock();
		pBuffer->Free();
	}
	else
	{
		m_pLastBuf->m_pNext = pBuffer;
		m_pLastBuf = pBuffer;
		m_lock.Unlock();
	}
}


void XIOSocket::FreeBuffer()
{
	m_lock.Lock();
	while (m_pFirstBuf)
	{
		XIOBuffer *pBuf = m_pFirstBuf;
		m_pFirstBuf = m_pFirstBuf->m_pNext;
		m_nPendingWrite -= pBuf->m_dwSize;
		pBuf->Free();
	}
	m_lock.Unlock();
}

void XIOSocket::OnClose()
{
}

void XIOSocket::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if (!bSuccess)
	{
		if (lpOverlapped == &m_overlappedRead) // closed only at read
			Close();
		else if (lpOverlapped == &m_overlappedWrite)
			FreeBuffer();
		ReleaseIO();
	}
	else if (lpOverlapped == &m_overlappedWrite)
	{
		WriteCallback(dwTransferred);
		ReleaseIO();
	}
	else if (lpOverlapped == &m_overlappedRead)
	{
		ReadCallback(dwTransferred);
		ReleaseIO();
	}
	else
	{
		_ASSERT(lpOverlapped == NULL);
		OnTimer(dwTransferred);
		ReleaseTimer();
	}
}



BOOL XIOSocket::CreateIOThread(int nThread)
{
	SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
	g_nTerminating = 0;
	g_hTimer = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	g_instance.OnTimerCallback();
	g_instance.AddRefTimer();

	if (!g_instance.RegisterWait(g_hTimer))
	{
		LOG_ERR(_T("RegisterWait error for timer"));
		return FALSE;
	}

	g_hThread = (HANDLE*)malloc(sizeof(HANDLE) * nThread);
	g_nThreadId = (unsigned*)malloc(sizeof(unsigned) * nThread);
	g_nThread = nThread;
	for (int i = 0; i < nThread; i++)
	{
		g_hThread[i] = (HANDLE)_beginthreadex(NULL, 0, IOThread, (void*)(INT_PTR)i, 0, &g_nThreadId[i]);
	}

	unsigned nWaitThreadId;
	g_hWaitThread = (HANDLE)_beginthreadex(NULL, 0, XIOSocket::WaitThread, 0, 0, &nWaitThreadId);

#if 0
	UINT threadId;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WaitThread, NULL, 0, &threadId);
	CloseHandle(hThread);
#endif
	return TRUE;
}

unsigned XIOSocket::AddIOThread()
{
	if (!s_hCompletionPort)
		return 0;

	g_hThread = (HANDLE*)realloc(g_hThread, sizeof(HANDLE) * (g_nThread + 1));
	g_nThreadId = (unsigned*)realloc(g_nThreadId, sizeof(unsigned) * (g_nThread + 1));
	unsigned nThreadID;
	g_hThread[g_nThread] = (HANDLE)_beginthreadex(NULL, 0, IOThread, (void*)(INT_PTR)g_nThread, 0, &nThreadID);
	g_nThreadId[g_nThread] = nThreadID;
	g_nThread++;
	return nThreadID;
}

BOOL XIOSocket::CloseIOThread()
{
	if (InterlockedExchange(&g_nTerminating, 1))
		return FALSE;
	SetEvent(g_hTimer);
	WaitForSingleObject(g_hWaitThread, INFINITE);
	FreeIOThread();
	return TRUE;
}

void XIOSocket::FreeIOThread()
{
	CloseHandle(g_hWaitThread);
	g_hWaitThread = 0;
#ifdef EXIT_IO_THREAD
	for (int i = 0; i < g_nThread; ++i) {
		CloseHandle(g_hThread[i]);
	}
	CloseHandle(XIOSocket::s_hCompletionPort);
#endif
	//_RPT(L"XIOSocket::FreeIOThread() \n");

	free(g_hThread);
	free(g_nThreadId);
	g_hThread = 0;
	g_nThreadId = 0;
}


void XIOSocket::Read(DWORD dwLeft)
{
	_ASSERT(m_pReadBuf->m_dwSize >= dwLeft);
	m_pReadBuf->m_dwSize -= dwLeft;
	if (m_pReadBuf->m_nRef != 1)
	{
		XIOBuffer* pNextBuf = XIOBuffer::Alloc();
		memcpy(pNextBuf->m_buffer, m_pReadBuf->m_buffer + m_pReadBuf->m_dwSize, dwLeft);
		m_pReadBuf->Release();
		m_pReadBuf = pNextBuf;
	}
	else
	{
		memmove(m_pReadBuf->m_buffer, m_pReadBuf->m_buffer + m_pReadBuf->m_dwSize, dwLeft);
	}
	m_pReadBuf->m_dwSize = dwLeft;

	AddRefIO();
	WSABUF wsabuf;
	wsabuf.len = BUFFER_SIZE - m_pReadBuf->m_dwSize;
	wsabuf.buf = m_pReadBuf->m_buffer + m_pReadBuf->m_dwSize;
	DWORD dwRecv;
	DWORD dwFlag = 0;
	m_lock.Lock();
	if (WSARecv(m_hSocket, &wsabuf, 1, &dwRecv, &dwFlag, &m_overlappedRead, NULL)
		&& GetLastError() != ERROR_IO_PENDING)
	{
		int nErr = GetLastError();
		m_lock.Unlock();
		if (nErr != WSAENOTSOCK && nErr != WSAECONNRESET && nErr != WSAECONNABORTED && nErr != WSAESHUTDOWN)
			LOG_ERR(_T("XIOSocket::Read %#x(%#x) err = %d"), m_hSocket, *(DWORD*)this, nErr);
		Shutdown();
		ReleaseIO();
	}
	else
		m_lock.Unlock();
}

void XIOSocket::Close()
{
	m_lock.Lock();
	SOCKET hSocket = m_hSocket;
	m_hSocket = INVALID_SOCKET;
	m_lock.Unlock();
	if (hSocket != INVALID_SOCKET)
	{
		OnClose();
//		LINGER linger;
//		linger.l_onoff = 1;
//		linger.l_linger = 0;
//		setsockopt(hSocket, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
		closesocket(hSocket);
		ReleaseSelf();
	}
}

void XIOSocket::Shutdown()
{
	m_lock.Lock();
	shutdown(m_hSocket, SD_BOTH);
	m_lock.Unlock();
}

XIOServer::XIOServer()
{
	m_hSocket = INVALID_SOCKET;
#if ACCEPT_SIZE
	for (int i = 0; i < ACCEPT_SIZE; ++i) {
		m_hAcceptSocket[i] = INVALID_SOCKET;
	}
	memset(&m_overlappedAccept, 0, sizeof(m_overlappedAccept));
#else
	m_hAcceptEvent = WSA_INVALID_EVENT;
#endif
}

XIOServer::~XIOServer()
{
	Close();
}

BOOL XIOServer::Start(int nPort, LPCTSTR lpszSocketAddr)
{
	m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_hSocket == INVALID_SOCKET)
	{
		LOG_ERR(_T("socket error %d"), WSAGetLastError());
		return FALSE;
}
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	if (lpszSocketAddr == NULL)
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	{
		ADDRINFOT* res;
		if (GetAddrInfo(lpszSocketAddr, 0, 0, &res) != 0 && res->ai_family != AF_INET) {
			LOG_ERR(_T("IP Address error: %d\n"), GetLastError());
			goto fail;
		}
		sin.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;
	}

	//sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(nPort);

	if (bind(m_hSocket, (struct sockaddr*)&sin, sizeof(sin)))
	{
		LOG_ERR(_T("bind error %d"), WSAGetLastError());
		goto fail;
	}
	if (listen(m_hSocket, 5))
	{
		LOG_ERR(_T("listen error %d"), WSAGetLastError());
		goto fail;
	}

#if ACCEPT_SIZE
	if (CreateIoCompletionPort((HANDLE)m_hSocket, XIOSocket::s_hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		ELOG(_T("CreateIoCompletionPort: %d %x %x"), GetLastError(), m_hSocket, XIOSocket::s_hCompletionPort);
		goto fail;
	}
	for (int i = 0; i < ACCEPT_SIZE; i++) {
		m_hAcceptSocket[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (m_hAcceptSocket[i] == INVALID_SOCKET)
		{
			LOG_ERR(_T("accept socket error %d"), WSAGetLastError());
			goto fail;
		}
		DWORD dwRecv;
		XIOServer::AddRefIO(); 
		if (!AcceptEx(m_hSocket, m_hAcceptSocket[i], m_AcceptBuf[i], 0, sizeof(struct sockaddr_in) + 16,
			sizeof(struct sockaddr_in) + 16, &dwRecv, &m_overlappedAccept[i]) && GetLastError() != ERROR_IO_PENDING)
		{
			LOG_ERR(_T("AcceptEx error %d"), WSAGetLastError());
			XIOServer::ReleaseIO();
			goto fail;
		}
	}
#else
	m_hAcceptEvent = WSACreateEvent();
	WSAEventSelect(m_hSocket, m_hAcceptEvent, FD_ACCEPT);
	if (!RegisterWait(m_hAcceptEvent))
	{
		LOG_ERR(_T("RegisterWait error on port %d"), nPort);
		goto fail;
	}
#endif
	return TRUE;
fail:
	Close();
	return FALSE;
}

void XIOServer::OnWaitCallback()
{
#if ACCEPT_SIZE
	EBREAK();
#else
	WSAResetEvent(m_hAcceptEvent);
	PostQueuedCompletionStatus(XIOSocket::s_hCompletionPort, 0, (ULONG_PTR)this, NULL);
#endif
}

void XIOServer::Stop()
{
	SOCKET hSocket = m_hSocket;
	m_hSocket = INVALID_SOCKET;
	closesocket(hSocket);
}

void XIOServer::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
	if (lpOverlapped) {
#if ACCEPT_SIZE
		XIOSocket* pSocket;
		int i = lpOverlapped - m_overlappedAccept;
		EASSERT(i < ACCEPT_SIZE);
		if (!bSuccess)
		{
			if (m_hSocket == INVALID_SOCKET) {
				XIOServer::ReleaseIO();
				return;
			}
			LOG_ERR(_T("accept callback error %d"), GetLastError());
			closesocket(m_hAcceptSocket[i]);
			m_hAcceptSocket[i] = INVALID_SOCKET;

			//goto retry;
			XIOServer::ReleaseIO();
			return;
		}
		struct sockaddr_in* paddrLocal, * paddrRemote;
		int	nLocalLen, nRemoteLen;
		GetAcceptExSockaddrs(m_AcceptBuf[i], 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
			(LPSOCKADDR*)&paddrLocal, &nLocalLen, (LPSOCKADDR*)&paddrRemote, &nRemoteLen);

		pSocket = CreateSocket(m_hAcceptSocket[i], paddrRemote);
		if (pSocket == NULL)
		{
			closesocket(m_hAcceptSocket[i]);
			goto retry;
		}
		setsockopt(m_hAcceptSocket[i], SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_hSocket, sizeof(m_hSocket));
		pSocket->Initialize();
		pSocket->ReleaseSelf();
	retry:
		m_hAcceptSocket[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (m_hAcceptSocket[i] == INVALID_SOCKET)
		{
			LOG_ERR(_T("accept socket error %d"), WSAGetLastError());
			XIOServer::ReleaseIO();
			return;
		}
		DWORD dwRecv;
		if (!AcceptEx(m_hSocket, m_hAcceptSocket[i], m_AcceptBuf[i], 0, sizeof(struct sockaddr_in) + 16,
			sizeof(struct sockaddr_in) + 16, &dwRecv, &m_overlappedAccept[i]) && GetLastError() != ERROR_IO_PENDING)
		{
			LOG_ERR(_T("AcceptEx error %d"), WSAGetLastError());
			XIOServer::ReleaseIO();
			return;
		}
#else	// ACCEPT_SIZE
		struct sockaddr_in clientAddress;
		int clientAddressLength = sizeof(clientAddress);
		SOCKET newSocket = accept(m_hSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (newSocket == INVALID_SOCKET)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				return;
			}
			else
			{
				if (m_hSocket != INVALID_SOCKET)
					LOG_ERR(_T("accept error: %d"), WSAGetLastError());
				return;
			}
		}
		XIOSocket* pSocket = CreateSocket(newSocket, &clientAddress);
		if (pSocket == NULL)
		{
			closesocket(newSocket);
			return;
		}
		pSocket->Initialize();
		pSocket->ReleaseSelf();
#endif // ACCEPT_SIZE
	}
	else {
		_ASSERT(bSuccess);
		OnTimer(dwTransferred);
		ReleaseTimer();
	}
}

void XIOServer::Close()
{
	if (m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
#if ACCEPT_SIZE
	for (int i = 0; i < ACCEPT_SIZE; ++i) {
		if (m_hAcceptSocket[i] != INVALID_SOCKET)
		{
			closesocket(m_hAcceptSocket[i]);
			m_hAcceptSocket[i] = INVALID_SOCKET;
		}
	}
#else
	if (m_hAcceptEvent != WSA_INVALID_EVENT)
	{
		WSACloseEvent(m_hAcceptEvent);
		m_hAcceptEvent = WSA_INVALID_EVENT;
	}
#endif
}






