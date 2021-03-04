#pragma once

#pragma warning(disable: 4509)

class XIOException
{
public:

	static BOOL SendMail( const char *host, const char *from, const char *to, LPCTSTR body, int  port = 0);
	static int Filter(LPEXCEPTION_POINTERS pExp);
	static void	DumpStack(int nThread, HANDLE *hThread, unsigned *nThreadId);
	static void Lock() { EnterCriticalSection(&s_lock); }
	static void Unlock() { LeaveCriticalSection(&s_lock); }
	static BOOL IsEnable();
	static BOOL	Disable();
	static void SendMail();
	static BOOL	ToggleAssert();
	static void	Open(LPCTSTR szMailServer, LPCTSTR szMailFrom, LPCTSTR szMailTo, int nPort = 0);
	static BOOL Enable();
	static CRITICAL_SECTION s_lock;
	static void Start();
	static void Stop();
	static void Touch();
	class CInit
	{
	public:
		CInit();
		~CInit();
	};
};

extern void __stdcall EBREAK();
extern void ELOG(LPCTSTR lpszFormat, ...);
#define EASSERT(expr) ((expr) || (EBREAK(), 0))



