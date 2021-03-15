#pragma once


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
//#define _CRTDBG_MAP_ALLOC
#else
//#define _HAS_EXCEPTIONS 0
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
#else
typedef	std::string tstring;
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

template <class T> class XAutoVar
{
public:
	T* m_p;

	XAutoVar() : m_p(0) {}
	XAutoVar(XAutoVar<T> const & copy) : m_p(copy.m_p)
	{
		if (m_p)
			m_p->AddRefVar(); 
	}
	XAutoVar(T* p) : m_p(p)
	{
		if (p)
			p->AddRefVar(); 
	}
	~XAutoVar()
	{
		if (m_p)
			m_p->ReleaseVar();
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
	T* operator = (XAutoVar<T> const &copy)
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








class XIOScreen;

class XIOLog
{
public :

	static void	Stop();

	static void AddV(int nType, LPCTSTR format, va_list va);
	static void Flush();
	static XIOScreen s_screen;
	enum { INFO, NORMAL, WARN, ERR };
};

#ifdef	_MT

class XLock
{
protected:
	CRITICAL_SECTION m_lock;

public:
	XLock() { InitializeCriticalSectionAndSpinCount(&m_lock, 0x00000400); }
	~XLock() { DeleteCriticalSection(&m_lock); }
	void Lock() { EnterCriticalSection(&m_lock); }
	void Unlock() { LeaveCriticalSection(&m_lock); }
	bool try_lock() { return TryEnterCriticalSection(&m_lock); }
};

template <typename T> class XUniqueLock
{
private:
	typename T* m_pT;
public:
	XUniqueLock(typename T & rT) : m_pT(&rT)
	{
		m_pT->Lock();
	}
	~XUniqueLock()
	{
		m_pT->Unlock();
	}
};

#endif





