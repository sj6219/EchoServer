#pragma once

#define USE_SHARED_MUTEX


#pragma warning(disable: 4245) // 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
#pragma warning(disable: 4018) // 'expression' : signed/unsigned mismatch
#pragma warning(disable: 4710) // 'function' : function not inlined
#pragma warning(disable: 4995) // 'function': name was marked as #pragma deprecated
#pragma warning(disable: 4996) // 'function': was declared deprecated
#pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc 
#pragma warning(disable: 4293) // 'operator' : shift count negative or too big, undefined behavior
#pragma warning(disable: 4786) // 'identifier' : identifier was truncated to 'number' characters in the debug information
#pragma warning(disable: 4503) // 'identifier' : decorated name length exceeded, name was truncated
#pragma warning(disable: 4200) // nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable: 4100) // 'identifier' : unreferenced formal parameter
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#else
#define _HAS_EXCEPTIONS 0
#define _SECURE_SCL 0
#define _HAS_ITERATOR_DEBUGGING 0
#endif

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <crtdbg.h>
#include <dbghelp.h>
#include <Sql.h>
#include <Sqlext.h>
#include <Sqltypes.h>
//#include <Odbcss.h>

#include <algorithm>
#include <list>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <strsafe.h>

#include <memory>
#include <shared_mutex>

#define LPCPACKET LPCSTR
#define LPPACKET LPSTR

#undef TRACE
#undef ASSERT

#ifdef	_DEBUG
void TRACE(LPCTSTR lpszFormat, ...);
#else
#define TRACE __noop
#endif

#ifdef _DEBUG
void BREAK();
#define ASSERT(expr) if (!(expr)) BREAK()
#else
#define BREAK __noop
#define ASSERT __noop
#endif


template <typename T> class Zero
{
public:
	T m_n;
	Zero() : m_n(0) {}
	Zero(T n) : m_n(n) {}
	operator T () const { return m_n; }
	T operator -> () const { return m_n; }
	T* operator& () { return &m_n; }
	bool operator !() const { return m_n == 0; }
};

#define _A(str)	str
#pragma warning(disable: 4995)

#ifdef	UNICODE

typedef	std::wstring tstring;

inline LPTSTR WINAPI A_TCopy(LPTSTR lpt, LPCSTR lpa, int nChars)
{
	ASSERT(lpa != NULL);
	ASSERT(lpt != NULL);
	MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpt, nChars);
	return lpt;
}

inline LPSTR WINAPI T_ACopy(LPSTR lpa, LPCTSTR lpt, int nChars) 
{
	ASSERT(lpt != NULL);
	ASSERT(lpa != NULL);
	WideCharToMultiByte(CP_ACP, 0, lpt, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#define CONVERT	int _convert; (_convert); LPCSTR _lpa; (_lpa); LPCWSTR _lpt; (_lpt);
#define A_T(lpa) (lpa ? (_convert = (lstrlenA((_lpa = lpa)) + 1), A_TCopy((LPWSTR)_alloca(_convert * 2), _lpa, _convert)) : NULL)
#define T_A(lpt) (lpt ? (_convert = (lstrlenW((_lpt = lpt)) + 1) * 2, T_ACopy((LPSTR)_alloca(_convert), _lpt, _convert)) : NULL)

#else

typedef	std::string tstring;

inline LPTSTR WINAPI A_TCopy(LPTSTR lpt, LPCSTR lpa, int nChars)
{
	StringCchCopy(lpt, nChars, lpa);
	return lpt;
}

inline LPSTR WINAPI T_ACopy(LPSTR lpa, LPCTSTR lpt, int nChars) 
{
	StringCchCopy(lpa, nChars, lpt);
	return lpa;
}

#define CONVERT
#define A_T(lpa) (lpa)
#define T_A(lpt) (lpt)

#endif // UNICODE

tstring Format(LPCTSTR format, ...);

extern "C"
{
   LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
   LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
   LONG  __cdecl _InterlockedCompareExchange(LONG volatile *Dest, LONG Exchange, LONG Comp);
   LONG  __cdecl _InterlockedExchange(LONG volatile *Target, LONG Value);
   LONG  __cdecl _InterlockedExchangeAdd(LONG volatile *Addend, LONG Value);
}

#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange 

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

#ifdef	_WIN64
#define InterlockedExchangePointer	_InterlockedExchangePointer
#define InterlockedExchangeAdd64	_InterlockedExchangeAdd64
#endif

void LOG_WARN(LPCTSTR format, ...);
void LOG_ERR(LPCTSTR format, ...);
void LOG_HACK(LPCTSTR format, ...);
void LOG_INFO(LPCTSTR format, ...);
void LOG_NORMAL(LPCTSTR format, ...);





template <class T> class XIOAutoPtr
{
public:
	T* m_p;

	XIOAutoPtr(T* p) : m_p(p) { if (m_p) m_p->AddRefTemp(); }
	~XIOAutoPtr() { if (m_p) m_p->ReleaseTemp(); }
	operator T* () const { return m_p; }
	T& operator * () const { return *m_p; }
	T* operator -> () const { return m_p; }
	T** operator & () const { return &m_p; }
	bool operator ! () const { return m_p == 0; }
private:
	XIOAutoPtr(XIOAutoPtr<T> const &copy);
	T* operator = (XIOAutoPtr<T> const &copy);
	
};

template <class T> class XIOAutoVar
{
public:
	T* m_p;

	XIOAutoVar() : m_p(0) {}
	XIOAutoVar(XIOAutoVar<T> const & copy) : m_p(copy.m_p)
	{
		if (m_p)
			m_p->AddRefVar(); 
	}
	XIOAutoVar(T* p) : m_p(p) 
	{
		if (p)
			p->AddRefVar(); 
	}
	~XIOAutoVar()
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
	T* operator = (XIOAutoVar<T> const &copy)
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


class XIOSpinLock
{
protected :
	long lock;

	void Wait();
public:
	XIOSpinLock() { lock = 0; }
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






class XIORWLock
{
#ifdef USE_SHARED_MUTEX
	std::shared_mutex m_lock;
public:
	void WriteLock() { m_lock.lock(); }
	void WriteUnlock() { m_lock.unlock(); }
	BOOL WriteTryLock() { return m_lock.try_lock(); }
	void ReadLock() { m_lock.lock_shared(); }
	void ReadUnlock() { m_lock.unlock_shared(); }
#else
public:
	XIORWLock();
	~XIORWLock();
	void WriteLock();
	void WriteUnlock();
	BOOL WriteTryLock();
	void ReadLock();
	void ReadUnlock();

	void Lock() 
	{
		if (InterlockedCompareExchange(&m_nLock, 1, 0))
			Wait();
	}
	void Unlock()
	{
		InterlockedExchange(&m_nLock, 0);
	}
	void Wait();
	HANDLE m_hREvent;
	HANDLE m_hWEvent;
	long m_nCount;
	long m_nLock;
#endif

};

class CAutoReadLock
{
private:
	XIORWLock &m_lock;
public:
	CAutoReadLock(XIORWLock &lock) : m_lock(lock) { m_lock.ReadLock(); }
	~CAutoReadLock() { m_lock.ReadUnlock(); }
private:
   CAutoReadLock & operator=( const CAutoReadLock & ) {}
};

class CAutoWriteLock
{
private:
	XIORWLock &m_lock;
public:
	CAutoWriteLock(XIORWLock &lock) : m_lock(lock) { m_lock.WriteLock(); }
	~CAutoWriteLock() { m_lock.WriteUnlock(); }
private:
   CAutoWriteLock & operator=( const CAutoWriteLock & ) {}
};

template <typename T> class TAutoLock
{
private:
	typename T *m_pT;
public:
	TAutoLock(typename T *pT) : m_pT(pT)
	{
		m_pT->Lock();
	}
	~TAutoLock()
	{
		m_pT->Unlock();
	}
};

template <typename T> class TAutoReadLock
{
private:
	typename T *m_pT;
public:
	TAutoReadLock(typename T *pT) : m_pT(pT)
	{
		m_pT->ReadLock();
	}
	~TAutoReadLock()
	{
		m_pT->ReadUnlock();
	}
};

template <typename T> class TAutoWriteLock
{
private:
	typename T *m_pT;
public:
	TAutoWriteLock(typename T *pT) : m_pT(pT)
	{
		m_pT->WriteLock();
	}
	~TAutoWriteLock()
	{
		m_pT->WriteUnlock();
	}
};







class XIOScreen;

class XIOLog
{
public :

	static void	Stop();

	static void AddV(int nType, LPCTSTR format, va_list va);
	static void Flush();
	static XIOScreen s_screen;
	enum { INFO, NORMAL, WARN, ERR, HACK };
};






