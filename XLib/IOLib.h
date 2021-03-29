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

#ifndef _DEBUG
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
//#include <crtdbg.h>
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
void TRACE(LPCTSTR lpszFormat, ...) noexcept;
#else
#define TRACE __noop
#endif

#ifdef _DEBUG
void BREAK() noexcept;
#define ASSERT(expr) if (!(expr)) BREAK()
#else
#define BREAK __noop
#define ASSERT __noop
#endif


#pragma warning(disable: 4995)

#ifdef	UNICODE
typedef	std::wstring tstring;
#else
typedef	std::string tstring;
#endif // UNICODE

tstring Format(LPCTSTR format, ...) noexcept;

void LOG_WARN(LPCTSTR format, ...);
void LOG_ERR(LPCTSTR format, ...);
void LOG_HACK(LPCTSTR format, ...);
void LOG_INFO(LPCTSTR format, ...);
void LOG_NORMAL(LPCTSTR format, ...);

template <class T> class XAutoVar
{
public:
	T* m_p;

	XAutoVar() noexcept : m_p(0) {}
	XAutoVar(XAutoVar<T> const & copy) noexcept : m_p(copy.m_p)
	{
		if (m_p)
			m_p->AddRefVar(); 
	}
	XAutoVar(T* p) noexcept : m_p(p)
	{
		if (p)
			p->AddRefVar(); 
	}
	~XAutoVar() noexcept
	{
		if (m_p)
			m_p->ReleaseVar();
	}
	T* operator = (T* p) noexcept
	{
		if (p)
			p->AddRefVar();
		T *pOld = (T*) InterlockedExchangePointer((void **)&m_p, p);
		if (pOld)
			pOld->ReleaseVar();
		return p;
	}
	T* operator = (XAutoVar<T> const &copy) noexcept
	{
		T* p = copy.m_p;
		if (p)
			p->AddRefVar();
		T* pOld = (T*) InterlockedExchangePointer((void **)&m_p, p);
		if (pOld)
			pOld->ReleaseVar();
		return p;
	}
	operator T* () const noexcept { return m_p; }
	T& operator * () const noexcept { return *m_p; }
	T* operator -> () const noexcept { return m_p; }
	T** operator & () const noexcept { return &m_p; }
	bool operator ! () const noexcept { return m_p == 0; }
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
	XLock() noexcept { InitializeCriticalSectionAndSpinCount(&m_lock, 0x00000400); }
	~XLock() noexcept { DeleteCriticalSection(&m_lock); }
	void Lock() noexcept { EnterCriticalSection(&m_lock); }
	void Unlock() noexcept { LeaveCriticalSection(&m_lock); }
	bool try_lock() noexcept { return TryEnterCriticalSection(&m_lock); }
};

template <typename T> class XUniqueLock
{
private:
	typename T* m_pT;
public:
	XUniqueLock(typename T & rT) noexcept : m_pT(&rT)
	{
		m_pT->Lock();
	}
	~XUniqueLock() noexcept
	{
		m_pT->Unlock();
	}
};

#endif





