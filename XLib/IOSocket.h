#pragma once


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
	XIOObject() : m_nRef(1), m_nGeneralRef(0), m_nSystemRef(1) {}
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
	long m_nRef;

#endif
};

template <class T> class CIOAutoPtr
{
public:
	T* m_p;

	CIOAutoPtr(T* p) : m_p(p) { if (m_p) m_p->AddRefTemp(); }
	~CIOAutoPtr() { if (m_p) m_p->ReleaseTemp(); }
	operator T* () const { return m_p; }
	T& operator * () const { return *m_p; }
	T* operator -> () const { return m_p; }
	T** operator & () const { return &m_p; }
	bool operator ! () const { return m_p == 0; }
private:
	CIOAutoPtr(CIOAutoPtr<T> const &copy);
	T* operator = (CIOAutoPtr<T> const &copy);
	
};

template <class T> class CIOAutoVar
{
public:
	T* m_p;

	CIOAutoVar() : m_p(0) {}
	CIOAutoVar(CIOAutoVar<T> const & copy) : m_p(copy.m_p)
	{
		if (m_p)
			m_p->AddRefVar(); 
	}
	CIOAutoVar(T* p) : m_p(p) 
	{
		if (p)
			p->AddRefVar(); 
	}
	~CIOAutoVar()
	{
		T* pOld = (T*) InterlockedExchangePointer((void **)&m_p, 0);
		if (pOld)
			pOld->ReleaseVar();
	}
	T* operator = (T* p)
	{
		if (p)
			p->AddRefVar();
		T *pOld = (T*) InterlockedExchangePointer((void **)&m_p, p);
		if (pOld)
			pOld->ReleaseVar();
		return p;
	}
	T* operator = (CIOAutoVar<T> const &copy)
	{
		T* p = copy.m_p;
		if (p)
			p->AddRefVar();
		T* pOld = (T*) InterlockedExchangePointer((void **)&m_p, p);
		if (pOld)
			pOld->ReleaseVar();
		return p;
	}
	operator T* () const { return m_p; }
	T& operator * () const { return *m_p; }
	T* operator -> () const { return m_p; }
	T** operator & () const { return &m_p; }
	bool operator ! () const { return m_p == 0; }
};

class XIOBuffer;
class XIOCriticalSection 
{
public:
	void Lock() { EnterCriticalSection(&m_critical_section); }
	BOOL TryLock() { return TryEnterCriticalSection(&m_critical_section); }
	void Unlock() { LeaveCriticalSection(&m_critical_section); }
	XIOCriticalSection() { InitializeCriticalSection(&m_critical_section); }
	XIOCriticalSection(DWORD dwSpinCount) { InitializeCriticalSectionAndSpinCount(&m_critical_section, dwSpinCount); }
	~XIOCriticalSection() { DeleteCriticalSection(&m_critical_section); }
	CRITICAL_SECTION m_critical_section;
};

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

	HANDLE	m_hAcceptEvent;
	SOCKET m_hSocket;
};

class CIOSpinLock
{
protected :
	long lock;

	void Wait();
public:
	CIOSpinLock() { lock = 0; }
	void Lock() 
	{
		if (InterlockedCompareExchange(&lock, 1, 0))
			Wait();
	}
	void Unlock()
	{
		InterlockedExchange(&lock, 0);
	}
	BOOL TryLock()
	{
		return InterlockedCompareExchange(&lock, 1, 0) == 0;
	}
};



class XIOBuffer 
{
public :
	class CSlot
	{
	public :
		XIOBuffer *m_pBuffer;
		CIOSpinLock m_lock;

		CSlot() : m_pBuffer(NULL) {}
	};

	char m_buffer[BUFFER_SIZE];
	DWORD m_dwSize;
	LONG m_nRef;
	XIOBuffer *m_pNext;
	long m_nBufferIndex;

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


	XIOCriticalSection m_lock;
	OVERLAPPED m_overlappedRead;
	OVERLAPPED m_overlappedWrite;
	XIOBuffer *m_pReadBuf;
	SOCKET m_hSocket;
	XIOBuffer *m_pFirstBuf;
	XIOBuffer *m_pLastBuf;
	long m_nPendingWrite;


	virtual ~XIOSocket();
	virtual void OnClose();
	virtual void OnIOCallback(BOOL bError, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
	virtual void OnRead() = 0;
	virtual void OnCreate();
	void Close();
	void GracefulClose();
	void Disconnect();
	void Read( DWORD dwLeft);
	void FreeBuffer();
	void ReadCallback(DWORD dwTransferred);
	void WriteCallback(DWORD dwTransferred);
	XIOSocket(SOCKET s);
	static void Start();
	static void Stop();
#ifdef XIOOBJECT_DEBUG
	void AddRefIO() { AddRef(&m_nSystemRef); }
	void ReleaseIO() { Release(&m_nSystemRef); }
#else
	void AddRefIO() { AddRef(); }
	void ReleaseIO() { Release(); }
#endif
	void WriteWithLock(XIOBuffer *pBuffer);
	void Write(XIOBuffer *pBuffer) { m_lock.Lock(); WriteWithLock(pBuffer); }
	void Write(char *buf, DWORD size);
	void Initialize();
	long PendingWrite() { return m_nPendingWrite; }
	static unsigned __stdcall IOThread(void *arglist);
	static void DumpStack();
	static BOOL CreateIOThread(int nThread);
	static unsigned AddIOThread();
	static BOOL CloseIOThread();
	static HANDLE s_hCompletionPort;
	static long s_nRunningThread;
	static void	FreeIOThread();
	static unsigned __stdcall WaitThread(void *);

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

