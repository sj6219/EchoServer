#pragma once
#include "IOLib.h"

#define ACCEPT_SIZE 4
#include <winsock2.h>

#define BUFFER_SIZE 8000
#define XIOOBJECT_DEBUG

class XIOObject 
{
public:

	virtual ~XIOObject();

	virtual void OnFree() { delete this; }

	void AddTimer(DWORD dwTime, int nId = 0);
	virtual void OnTimer(int nId) {}
	virtual void OnTimerCallback(int nId = 0);
	long GetRef() { return m_nRef; }

	virtual void OnWaitCallback() {}
	BOOL RegisterWait(HANDLE handle);
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	inline BOOL PostObject(int nId, LPVOID pObject);
#ifdef XIOOBJECT_DEBUG
	XIOObject() : m_nRef(1), m_nGeneralRef(0), m_nSystemRef(1) {} // AddRefSelf()
	void AddRefTimer() { AddRef(&m_nSystemRef); }
	void ReleaseTimer() { Release(&m_nSystemRef); }
	void AddRefTemp() { AddRef(&m_nGeneralRef); }
	void ReleaseTemp() { Release(&m_nGeneralRef); }
	void AddRefVar() { AddRef(&m_nSystemRef); }
	void ReleaseVar() { Release(&m_nSystemRef); }
	void AddRef(LONG volatile *pRef);
	void Release(LONG volatile *pRef);
	void AddRef() { AddRef(&m_nGeneralRef); }
	void Release() { Release(&m_nGeneralRef); }
	void AddRefSelf() { AddRef(&m_nSystemRef); }
	void ReleaseSelf() { Release(&m_nSystemRef); }
	void AddRefIO() { AddRef(&m_nSystemRef); }
	void ReleaseIO() { Release(&m_nSystemRef); }

	long m_nRef;
	long m_nSystemRef;
	long m_nGeneralRef;

#else
	XIOObject() : m_nRef(1) {}
	void ReleaseTimer() { Release(); }
	void AddRefTemp() { AddRef(); }
	void ReleaseTemp() { Release(); }
	void AddRefVar() { AddRef(); }
	void ReleaseVar() { Release(); }
	void AddRef() { InterlockedIncrement(&m_nRef); }
	void Release() { if (InterlockedDecrement(&m_nRef) == 0) OnFree(); }
	void AddRefSelf() { AddRef(); }
	void ReleaseSelf() { Release(); }
	void AddRefTimer() { AddRef(); }
	void AddRefIO() { AddRef(); }
	void ReleaseIO() { Release(); }
	long m_nRef;

#endif
};

class XIOBuffer;
class XIOSocket;

class XIOServer : public XIOObject
{
public:
	virtual void OnWaitCallback();
	virtual XIOSocket *CreateSocket(SOCKET s, sockaddr_in *addr) = 0;
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);

	XIOServer();
	virtual ~XIOServer();
	BOOL Start( int nPort, LPCTSTR lpszSocketAddr = 0);
	void Close();
	void Stop();

	SOCKET m_hSocket;
#if ACCEPT_SIZE
	SOCKET m_hAcceptSocket[ACCEPT_SIZE];
	char	m_AcceptBuf[ACCEPT_SIZE][sizeof(struct sockaddr_in) * 2 + 32];
	OVERLAPPED m_overlappedAccept[ACCEPT_SIZE];
#else
	HANDLE	m_hAcceptEvent;
#endif

};

class XIOBuffer 
{
public :
	char m_buffer[BUFFER_SIZE];
	DWORD m_dwSize;
	LONG m_nRef;
	XIOBuffer *m_pNext;
	XIOBuffer*& GetNext() { return m_pNext; }

	static long	s_nCount;

public :
	static XIOBuffer *Alloc();
	static void FreeAll();

	XIOBuffer() { InterlockedIncrement(&s_nCount); }
	void Free();
	void AddRef() { InterlockedIncrement(&m_nRef); }
	void Release() { if (InterlockedDecrement(&m_nRef) == 0) Free(); }
};

class XIOSocket : public XIOObject
{
public :

	SOCKET m_hSocket;
	XLock m_lock;
	// There is no need to lock for variable m_hSocket inside the OnCreate() or OnRead() function.
	// This is because these functions is not called simultaneously in multiple threads.
	// Thus, inside these functions, the Write() function of other XIOSocket instance can be called without worrying about the deadlock.

	OVERLAPPED m_overlappedRead;
	OVERLAPPED m_overlappedWrite;
	XIOBuffer *m_pReadBuf;
	XIOBuffer *m_pFirstBuf;
	XIOBuffer *m_pLastBuf;
	long m_nPendingWrite;


	XIOSocket(SOCKET s);
	virtual ~XIOSocket();
	virtual void OnClose();
	virtual void OnIOCallback(BOOL bError, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	virtual void OnRead() = 0;
	virtual void OnCreate();
	void Close();
	void Shutdown();
	void Read( DWORD dwLeft);
	void FreeBuffer();
	void ReadCallback(DWORD dwTransferred);
	void WriteCallback(DWORD dwTransferred);
	void WriteWithLock(XIOBuffer *pBuffer);
	void Write(XIOBuffer *pBuffer) { m_lock.Lock(); WriteWithLock(pBuffer); }
	void Write(void *buf, DWORD size);
	void Initialize();
	long PendingWrite() { return m_nPendingWrite; }
	static void Start();
	static void Stop();
	static unsigned __stdcall IOThread(void *arglist);
	static void DumpStack();
	static BOOL CreateIOThread(int nThread);
	static unsigned AddIOThread();
	static BOOL CloseIOThread();
	static HANDLE s_hCompletionPort;
	static long s_nRunningThread;
	static unsigned __stdcall WaitThread(void *);

protected:
	static void	FreeIOThread();

public:
	class XIOTimer 
	{
	public :
		XIOObject *m_pObject;
		int m_nId;
		DWORD m_dwTime;

		XIOTimer(XIOObject *pObject, DWORD dwTime, int nId = 0)
			: m_dwTime(dwTime), m_pObject(pObject), m_nId(nId) {}

		bool operator < (const XIOTimer &rTimer) const
		{
			return (LONG)+(m_dwTime - rTimer.m_dwTime) > 0;
		}
	};

	class CInit
	{
	public :
		~CInit();
	};


	class XIOTimerInstance : public XIOObject
	{
	public :
		virtual void OnTimerCallback(int id=0);
		virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	};
};

inline	BOOL XIOObject::PostObject(int nId, LPVOID pObject)
{ 
	return PostQueuedCompletionStatus(XIOSocket::s_hCompletionPort, nId, (ULONG_PTR)this, (LPOVERLAPPED) pObject); 
}

