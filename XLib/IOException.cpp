	// C++/General/Debug Info/Program Database /Zi
	// C++/Listing Files/Listing File Type/Assembly, Machine Code, and Source
	// C++/Code Generation/Enable C++ Exceptions
	// #define _HAS_EXCEPTIONS 0
	// #define _SECURE_SCL 0
	// #define _HAS_ITERATOR_DEBUGGING 0
	// C++/Language/Enable Run-Time Type Info GR-
	// C++/Command Line /O2 /Oy- 
	//                  /Oi /Ob2 /GS /GF /Gy
	// Linker/General/Enable Incremental Linking /INCREMENTAL:NO
	// Linker/Debugging/Generate Debug Info /DEBUG
	// Linker/Debugging/Generate Program Database File /PDB
	// Linker/Optimization/References /OPT:REF
	// Linker/Optimization/COMDAT Folding /OPT:ICF
#include "pch.h"
#include <windows.h>
#include <tchar.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <winsock2.h>
#include <time.h>
#include <stdio.h>
#include "IOException.h"

#pragma comment(lib, "ws2_32.lib")

#if	defined(UNICODE) && defined(DBGHELP_TRANSLATE_TCHAR)
#error undef DBGHELP_TRANSLATE_TCHAR 
#endif

#pragma warning(disable : 4514)
#pragma warning(disable : 4201)

#ifdef	_WIN64
#define RIP	Rip
#else
#define RIP	Eip
#endif

#pragma comment( lib, "dbghelp.lib" )


const int NumCodeBytes = 16;	// Number of code bytes to record.

#define BELOW_STACKDUMP_NUM	3		// for argument + 2

#define STACK_COLUMN	8		// Number of columns in stack dump.

#define	ONEK			1024
#define	SIXTYFOURK		(64*ONEK)
#define	ONEM			(ONEK*ONEK)
#define	ONEG			(ONEK*ONEK*ONEK)
#define MAX_STACK_NUM	40
#define ABOVE_STACK_NUM	8		// for local variable
#define BELOW_STACK_NUM	3		// for argument + 2

#define MAX_STACKDUMP_NUM	40
#define ABOVE_STACKDUMP_NUM	4		// for local variable

static UINT_PTR	g_dwStack;
static UINT_PTR	g_dwStackTop;
static time_t	g_timeStart;
CRITICAL_SECTION XIOException::s_lock;
static long g_nEnable = 1;
static long g_bBreak = 1;
static long g_nBreak;
static long	g_nLog;

static XIOException::CInit theInit;

static TCHAR g_szLogPath[MAX_PATH];
static TCHAR g_szDumpPath[MAX_PATH];

static LPTOP_LEVEL_EXCEPTION_FILTER g_pPreviousFilter;
static char g_szMailServer[MAX_PATH];
static char g_szMailFrom[MAX_PATH];
static char g_szMailTo[MAX_PATH];
static int g_nMailPort;
static BOOL sendMail(const char *host, int port, const char *from, const char *to, LPCTSTR body);
static LPTSTR GetFilePart(LPTSTR source);
static BOOL sendFile(SOCKET sock, LPCTSTR name);
static LPCTSTR GetExceptionDescription(DWORD ExceptionCode);
static void __cdecl hprintf(HANDLE LogFile, LPCTSTR Format, ...);
static void RecordSystemInformation(HANDLE LogFile);
static void RecordModuleList(HANDLE LogFile);
static LONG WINAPI RecordExceptionInfo(PEXCEPTION_POINTERS data);

XIOException::CInit::CInit()
{
	XIOException::Start();
}

XIOException::CInit::~CInit()
{
	XIOException::Stop();
}

void XIOException::Touch()
{

}
static void sendMail()
{
	if (g_szMailServer)
		sendMail(g_szMailServer, g_nMailPort, g_szMailFrom, g_szMailTo, g_szLogPath);

	{
		TCHAR szNewPath[MAX_PATH];
		_tcscpy_s(szNewPath, g_szLogPath);
		LPTSTR szFilePart = GetFilePart(szNewPath);

		time_t t;
		time(&t);
		struct tm tm;
		localtime_s(&tm, &t);

		_stprintf_s(szFilePart, szNewPath - szFilePart + MAX_PATH, _T("Exception\\%02d%02d%02d.%02d%02d%02d.dbg.txt"), tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
		MoveFileEx(g_szLogPath, szNewPath, MOVEFILE_REPLACE_EXISTING);
		_stprintf_s(szFilePart, szNewPath - szFilePart + MAX_PATH, _T("Exception\\%02d%02d%02d.%02d%02d%02d.dbg.dmp"), tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
		MoveFileEx(g_szDumpPath, szNewPath, MOVEFILE_REPLACE_EXISTING);
	}
	DeleteFile(g_szLogPath);
	DeleteFile(g_szDumpPath);
}

static BOOL sendMail(const char *host, int port, const char *from, const char *to, LPCTSTR body)
{
	char packetBuffer[2048];
	SOCKET sock;
	const char *str;

	BOOL bReturn = FALSE;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
		return FALSE;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		WSACleanup();
		return FALSE;
	}
	int timeout = 5000;
	int result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	if (result == SOCKET_ERROR) {
		goto exit;
	}
	timeout = 5000;
	result = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
	if (result == SOCKET_ERROR) {
		goto exit;
	}
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
		goto exit;
	}
	addr.sin_family = AF_INET;
	//addr.sin_addr.s_addr = inet_addr(host);
	//inet_pton(AF_INET, host, &addr.sin_addr);
	//if (addr.sin_addr.s_addr == INADDR_NONE) {
	//	hostent *h = gethostbyname(host);
	//	if (h == NULL)
	//		goto exit;
	//	memcpy(&addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
	//}
	ADDRINFOA* res;
	if (GetAddrInfoA(host, 0, 0, &res) != 0 && res->ai_family != AF_INET) {
		goto exit;
	}
	addr.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;

	addr.sin_port = htons(25);
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
		goto exit;
	}
	recv(sock, packetBuffer, sizeof(packetBuffer), 0);
	str = "HELO www.polygontek.com\r\n";
	if (send(sock, str, (int) strlen(str), 0) <= 0)
		goto exit;
	recv(sock, packetBuffer, sizeof(packetBuffer), 0);
	sprintf_s(packetBuffer, "MAIL From: <%s>\r\n", from);
	if (send(sock, packetBuffer, (int) strlen(packetBuffer), 0) <= 0)
		goto exit;
	recv(sock, packetBuffer, sizeof(packetBuffer), 0);
	str = to;
	for ( ; ; ) {
		str += strspn(str, " ,");
		if (str[0] == 0)
			break;
		int len = (int) strcspn(str, ",");
		sprintf_s(packetBuffer, "RCPT To: <%.*s>\r\n", len, str);
		if (send(sock, packetBuffer, (int) strlen(packetBuffer), 0) <= 0)
			goto exit;
		recv(sock, packetBuffer, sizeof(packetBuffer), 0);
		str += len;
	}
	str = "DATA\r\n";
	if (send(sock, str, (int) strlen(str), 0) <= 0)
		goto exit;
	recv(sock, packetBuffer, sizeof(packetBuffer), 0);
	SOCKADDR_IN sockname;
	memset(&sockname, 0, sizeof(sockname));
	{
		int namelen = sizeof(sockname);
		getsockname(sock, (LPSOCKADDR) &sockname, &namelen);
	}
	sprintf_s(packetBuffer, "From: <%s>\r\n", from);
	if (send(sock, packetBuffer, (int) strlen(packetBuffer), 0) <= 0)
		goto exit;
	sprintf_s(packetBuffer, "Subject: Exception (%d.%d.%d.%d)\r\n\r\n\r\n",
		sockname.sin_addr.S_un.S_un_b.s_b1, sockname.sin_addr.S_un.S_un_b.s_b2,
		sockname.sin_addr.S_un.S_un_b.s_b3, sockname.sin_addr.S_un.S_un_b.s_b4);
	if (send(sock, packetBuffer, (int) strlen(packetBuffer), 0) <= 0)
		goto exit;
	if (!sendFile(sock, body)) 
		goto exit;
	str = "\r\n.\r\n";
	if (send(sock, str, (int) strlen(str), 0) <= 0)
		goto exit;
	recv(sock, packetBuffer, sizeof(packetBuffer), 0);
	str = "QUIT\r\n";
	if (send(sock, str, (int) strlen(str), 0) <= 0)
		goto exit;
	recv(sock, packetBuffer, (int) sizeof(packetBuffer), 0);
	bReturn = TRUE;
exit:
	closesocket(sock);
	WSACleanup();
	return bReturn;
}



static void PrintStack(HANDLE LogFile, DWORD_PTR begin, DWORD_PTR end)
{
	if (g_dwStack < begin)
		g_dwStack = begin & ~(STACK_COLUMN * sizeof(DWORD) - 1);
	if (end > g_dwStackTop)
		end = g_dwStackTop;
	else 
		end = (end + (STACK_COLUMN * sizeof(DWORD) - 1)) & ~(STACK_COLUMN * sizeof(DWORD) - 1);
	__try {
		TCHAR buffer[STACK_COLUMN * 9 + 20] = _T("");
		int len = 0;
		while (g_dwStack < end) {
			if ((g_dwStack & (STACK_COLUMN * sizeof(DWORD) - 1)) == 0) {
				len += _stprintf_s(buffer+len, sizeof(buffer)/sizeof(TCHAR)-len, _T("%p"), (void *)g_dwStack);
			}
			len += _stprintf_s(buffer+len, sizeof(buffer)/sizeof(TCHAR)-len, _T(" %08x"), *(DWORD *)g_dwStack);
			g_dwStack += sizeof(DWORD);
			if ((g_dwStack & (STACK_COLUMN * sizeof(DWORD) - 1)) == 0) {
				hprintf(LogFile, _T("%s\r\n"), buffer);
				len = 0;
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		hprintf(LogFile, _T("Exception encountered during stack dump\r\n"), g_dwStack);
		g_dwStackTop = g_dwStack;
	}
}

static void ImageHelpStackWalk(HANDLE LogFile, PCONTEXT ptrContext)
{
	hprintf(LogFile, _T("Call Stack Information:\r\n"));

	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));

#ifdef	_WIN64
	sf.AddrPC.Offset       = ptrContext->Rip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = ptrContext->Rsp;
	sf.AddrStack.Mode      = AddrModeFlat;
#else
	sf.AddrPC.Offset       = ptrContext->Eip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = ptrContext->Esp;
	sf.AddrStack.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset    = ptrContext->Ebp;
	sf.AddrFrame.Mode      = AddrModeFlat;
#endif
	while ( 1 )
	{
#ifdef	_WIN64
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
						GetCurrentProcess(),
						GetCurrentThread(),
						&sf,
						ptrContext,
						0,
						SymFunctionTableAccess64,
						SymGetModuleBase64,
						0))
			break;
#else
		if (!StackWalk64(IMAGE_FILE_MACHINE_I386,
						GetCurrentProcess(),
						GetCurrentThread(),
						&sf,
						ptrContext,
						0,
						SymFunctionTableAccess64,
						SymGetModuleBase64,
						0))
			break;
#endif
		if (0 == sf.AddrFrame.Offset) // Bail if frame is not okay.
			break;

		PrintStack(LogFile, (DWORD_PTR) sf.AddrFrame.Offset - ABOVE_STACK_NUM * sizeof(DWORD), 
			(DWORD_PTR) sf.AddrFrame.Offset + BELOW_STACK_NUM * sizeof(DWORD));

		hprintf(LogFile, _T("\r\n%p %p "), (char *) sf.AddrFrame.Offset, (char *) sf.AddrPC.Offset);
		char UnDName[512];
		UnDName[0] = 0;
#if	0
		ULONG64 symbolBuffer[(sizeof(SYMBOL_INFO) + 512 * sizeof(TCHAR) + sizeof(ULONG64)-1)/sizeof(ULONG64)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLen = 512;
		DWORD64 symDisplacement = 0;

		if (SymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset,
							&symDisplacement, pSymbol)) 
#else
		ULONG64 symbolBuffer[(sizeof(PIMAGEHLP_SYMBOL64 ) + 512 * sizeof(TCHAR) + sizeof(ULONG64)-1)/sizeof(ULONG64)];
		PIMAGEHLP_SYMBOL64  pSymbol = (PIMAGEHLP_SYMBOL64 )symbolBuffer;
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLength  = 512;
		DWORD64 symDisplacement = 0;

		if (SymGetSymFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset,
							&symDisplacement, pSymbol)) 
#endif
		{
#ifdef	UNICODE
			hprintf(LogFile, _T("%S +%x\r\n"), pSymbol->Name, symDisplacement);
#else
			hprintf(LogFile, _T("%s +%x\r\n"), pSymbol->Name, symDisplacement);
#endif
			UnDecorateSymbolName(pSymbol->Name, UnDName, 512, UNDNAME_COMPLETE);
		}

		{
			IMAGEHLP_LINE64 line;
			DWORD lineDisplacement = 0;
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &lineDisplacement, &line)) {
#ifdef	UNICODE
				hprintf(LogFile, _T("%S %d %S\r\n"), line.FileName, line.LineNumber, UnDName);
#else
				hprintf(LogFile, _T("%s %d %s\r\n"), line.FileName, line.LineNumber, UnDName);
#endif
				goto print_param;
			}
		} 
		{
			TCHAR CrashModulePathName[MAX_PATH];
			CrashModulePathName[0] = 0;
			MEMORY_BASIC_INFORMATION MemInfo;
			// VirtualQuery can be used to get the allocation base associated with a
			// code address, which is the same as the ModuleHandle. This can be used
			// to get the filename of the module that the crash happened in.
			if (VirtualQuery((void*)sf.AddrPC.Offset, &MemInfo, sizeof(MemInfo)))
				GetModuleFileName((HINSTANCE)MemInfo.AllocationBase,
					CrashModulePathName, MAX_PATH);
#ifdef		UNICODE
			hprintf(LogFile, _T("%s %S\r\n"), CrashModulePathName, UnDName);
#else
			hprintf(LogFile, _T("%s %s\r\n"), CrashModulePathName, UnDName);
#endif
		}
print_param:
		hprintf(LogFile, _T("Params:   %p %p %p %p\r\n"), (char *) sf.Params[0], (char *) sf.Params[1], (char *) sf.Params[2], (char *) sf.Params[3]);
	}
	hprintf(LogFile, _T("\r\n"));
}

static void __cdecl hprintf(HANDLE LogFile, LPCTSTR Format, ...)
{
		TCHAR buffer[2000];	// _stprintf never prints more than one K.

		va_list arglist;
		va_start( arglist, Format);
		int len = _vstprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), Format, arglist);
		va_end( arglist);

		DWORD NumBytes;

		WriteFile(LogFile, buffer, len * sizeof(TCHAR), &NumBytes, 0);
}

static void CreateMiniDump(PEXCEPTION_POINTERS pException)
{
	HANDLE hFile = CreateFile(g_szDumpPath, GENERIC_WRITE, FILE_SHARE_READ, 0,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	MINIDUMP_EXCEPTION_INFORMATION stExInfo ;
	PMINIDUMP_EXCEPTION_INFORMATION pExceptionParam;
	if (pException) {
		// Finally, set up the exception info structure.
		stExInfo.ThreadId = GetCurrentThreadId ( ) ;
		stExInfo.ClientPointers = TRUE ;
		stExInfo.ExceptionPointers = pException;
		pExceptionParam = &stExInfo;
	}
	else
		pExceptionParam = 0;
		
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, pExceptionParam, 0, 0);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	CloseHandle(hFile);
}

    

static BOOL sendFile(SOCKET sock, LPCTSTR name)
{
	HANDLE hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return true;

	TCHAR buf[1024];
	for ( ; ; ) {
		DWORD dwRead;
		if (!ReadFile(hFile, buf, sizeof(buf), &dwRead, NULL) || !dwRead)
			break;
#ifdef	UNICODE
		char tmp[1024 * 2];
		dwRead = WideCharToMultiByte(CP_ACP, 0, buf, dwRead/2, tmp, sizeof(tmp), 0, 0);
		if (send(sock, tmp, dwRead, 0) <= 0)
			return false;
#else
		if (send(sock, buf, dwRead, 0) <= 0)
			return false;
#endif
	}
	CloseHandle(hFile);

	return true;
}


BOOL	XIOException::Enable()
{
	return !InterlockedExchange(&g_nEnable, 1);
}

BOOL	XIOException::Disable()
{
	return InterlockedExchange(&g_nEnable, 0);
}

BOOL	XIOException::IsEnable()
{
	return g_nEnable;
}

void	XIOException::SendMail()
{
	Lock();
	sendMail();
	Unlock();
}

BOOL	XIOException::ToggleAssert()
{
	g_bBreak ^= 1;
	return g_bBreak;
}


static void GenerateExceptionReport(PEXCEPTION_POINTERS data)
{
	HANDLE LogFile = CreateFile(g_szLogPath, GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
	if (LogFile == INVALID_HANDLE_VALUE) {
		CreateMiniDump(data);
		return;
	}
#ifdef	UNICODE
	if (SetFilePointer(LogFile, 0, NULL, FILE_END) == 0) {
		hprintf(LogFile, _T("\xFEFF"));
	}
#else
	SetFilePointer(LogFile, 0, NULL, FILE_END);
#endif



	SYSTEMTIME st = {0};
	::GetLocalTime(&st);
	hprintf(LogFile,  
		_T("%#x(%d) %04d/%02d/%02d %02d:%02d:%02d> exception %d\r\n"), 
		GetCurrentThreadId(), GetCurrentThreadId(), 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, g_nBreak);


	TCHAR CrashModulePathName[MAX_PATH];
	LPCTSTR CrashModuleFileName = _T("Unknown");
	MEMORY_BASIC_INFORMATION MemInfo;

	CONTEXT context = *data->ContextRecord;
	if (VirtualQuery((void*)(UINT_PTR)context.RIP, &MemInfo, sizeof(MemInfo)) &&
			GetModuleFileName((HINSTANCE)MemInfo.AllocationBase,
			CrashModulePathName, MAX_PATH) > 0)
		CrashModuleFileName = GetFilePart(CrashModulePathName);


	struct tm tm;
	localtime_s(&tm, &g_timeStart);
	hprintf(LogFile, _T("start at %02d/%02d/%02d %02d:%02d:%02d\r\n"), tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	RecordSystemInformation(LogFile);

	PEXCEPTION_RECORD Exception = data->ExceptionRecord;
	hprintf(LogFile, _T("%s in module %s at %04x:%p.\r\n"),
			GetExceptionDescription(Exception->ExceptionCode),
			CrashModuleFileName, context.SegCs, context.RIP);

	if (Exception->ExceptionCode == STATUS_ACCESS_VIOLATION &&
			Exception->NumberParameters >= 2) {
		TCHAR DebugMessage[1000];
		LPCTSTR readwrite = _T("Read from");
		if (Exception->ExceptionInformation[0])
			readwrite = _T("Write to");
		_stprintf_s(DebugMessage,  _T("%s location %08llx caused an access violation.\r\n"),
				readwrite, Exception->ExceptionInformation[1]);

#ifdef	_DEBUG
		// The VisualC++ debugger doesn't actually tell you whether a read
		// or a write caused the access violation, nor does it tell what
		// address was being read or written. So I fixed that.
		OutputDebugString(_T("Exception handler: "));
		OutputDebugString(DebugMessage);
#endif
		hprintf(LogFile, _T("%s"), DebugMessage);
	}
	// Print out the register values in a Win95 error window compatible format.
	hprintf(LogFile, _T("\r\n"));
	hprintf(LogFile, _T("Registers:\r\n"));
#ifdef	_WIN64
	hprintf(LogFile, _T("RAX=%p CS=%04x RIP=%p ContextFlags=%08x\r\n"),
			context.Rax, context.SegCs, context.Rip, context.ContextFlags);
	hprintf(LogFile, _T("RBX=%p SS=%04x RSP=%p RBP=%p\r\n"),
			context.Rbx, context.SegSs, context.Rsp, context.Rbp);
	hprintf(LogFile, _T("RCX=%p DS=%04x RSI=%p FS=%04x\r\n"),
			context.Rcx, context.SegDs, context.Rsi, context.SegFs);
	hprintf(LogFile, _T("RDX=%p ES=%04x RDI=%p GS=%04x\r\n"),
			context.Rdx, context.SegEs, context.Rdi, context.SegGs);
#else
	hprintf(LogFile, _T("EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\r\n"),
			context.Eax, context.SegCs, context.Eip, context.EFlags);
	hprintf(LogFile, _T("EBX=%08x SS=%04x ESP=%08x EBP=%08x\r\n"),
			context.Ebx, context.SegSs, context.Esp, context.Ebp);
	hprintf(LogFile, _T("ECX=%08x DS=%04x ESI=%08x FS=%04x\r\n"),
			context.Ecx, context.SegDs, context.Esi, context.SegFs);
	hprintf(LogFile, _T("EDX=%08x ES=%04x EDI=%08x GS=%04x\r\n"),
			context.Edx, context.SegEs, context.Edi, context.SegGs);
#endif
	hprintf(LogFile, _T("\r\nBytes at CS:EIP:\r\n"));

	// Print out the bytes of code at the instruction pointer. Since the
	// crash may have been caused by an instruction pointer that was bad,
	// this code needs to be wrapped in an exception handler, in case there
	// is no memory to read. If the dereferencing of code[] fails, the
	// exception handler will print '??'.
	unsigned char *code = (unsigned char*) (DWORD_PTR) context.RIP;
	for (int codebyte = 0; codebyte < NumCodeBytes; codebyte++) {
		__try {
			hprintf(LogFile, _T("%02x "), code[codebyte]);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			hprintf(LogFile, _T("?? "));
		}
	}

	// Time to print part or all of the stack to the error log. This allows
	// us to figure out the call stack, parameters, local variables, etc.
	hprintf(LogFile, _T("\r\n\r\nStack dump:\r\n"));
		// Esp contains the bottom of the stack, or at least the bottom of
		// the currently used area.
#ifdef	_WIN64
	g_dwStack = context.Rsp & ~(STACK_COLUMN * sizeof(DWORD) - 1);
	g_dwStackTop = -1;

	PNT_TIB pTib = reinterpret_cast<PNT_TIB>(NtCurrentTeb());
	g_dwStackTop = (UINT_PTR) pTib->StackBase;
#else
	g_dwStack = context.Esp & ~(STACK_COLUMN * sizeof(DWORD) - 1);
	__asm {
		// Load the top (highest address) of the stack from the
		// thread information block. It will be found there in
		// Win9x and Windows NT.
		mov	eax, fs:[4]
		mov g_dwStackTop, eax
	}
#endif
	g_dwStackTop &= ~(STACK_COLUMN * sizeof(UINT_PTR) - 1);
	PrintStack(LogFile, g_dwStack, g_dwStack + MAX_STACK_NUM * sizeof(DWORD));
	hprintf(LogFile, _T("\r\n"));

	if (SymInitialize(GetCurrentProcess(), 0, TRUE)) {
		DWORD dwOpts = SymGetOptions () ;
		if ((dwOpts & ( SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS)) != 
			(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS))
			SymSetOptions ( (dwOpts & ~SYMOPT_UNDNAME) | (SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS));
		ImageHelpStackWalk(LogFile, &context);
		SymCleanup(GetCurrentProcess());
	}

	RecordModuleList(LogFile);
	CloseHandle(LogFile);

	CreateMiniDump(data);
}

static void RecordSystemInformation(HANDLE LogFile)
{
	FILETIME	CurrentTime;
	GetSystemTimeAsFileTime(&CurrentTime);
//	char TimeBuffer[100];
//	PrintTime(TimeBuffer, CurrentTime);
//	hprintf(LogFile, _T("Error occurred at %s.\r\n"), TimeBuffer);
	TCHAR	ModuleName[MAX_PATH];
	if (GetModuleFileName(0, ModuleName, MAX_PATH) <= 0)
		_stprintf_s(ModuleName,  _T("Unknown"));
	TCHAR	UserName[200];
	DWORD UserNameSize = sizeof(UserName) / sizeof(TCHAR);
	if (!GetUserName(UserName, &UserNameSize))
		_stprintf_s(UserName,  _T("Unknown"));
	hprintf(LogFile, _T("%s, run by %s.\r\n"), ModuleName, UserName);

	SYSTEM_INFO	SystemInfo;
	GetSystemInfo(&SystemInfo);
	hprintf(LogFile, _T("%d processor(s), type %d.\r\n"),
		SystemInfo.dwNumberOfProcessors, SystemInfo.dwProcessorType);

	MEMORYSTATUS	MemInfo;
	MemInfo.dwLength = sizeof(MemInfo);
	GlobalMemoryStatus(&MemInfo);
	// Print out the amount of physical memory, rounded up.
	hprintf(LogFile, _T("%d MBytes physical memory.\r\n"), (MemInfo.dwTotalPhys + ONEM - 1) / ONEM);

#if 0
	DWORD dwTotal, dwFree;
	DDSCAPS2 ddsCaps2;
	DDDEVICEIDENTIFIER2 ddIdentifer;
	ZeroMemory( &ddIdentifer, sizeof( DDDEVICEIDENTIFIER2));
	g_lpDxDraw->GetDDraw()->GetDeviceIdentifier( &ddIdentifer, DDGDI_GETHOSTIDENTIFIER);

	ZeroMemory( &ddsCaps2, sizeof( DDSCAPS2));
	ddsCaps2.dwCaps = DDSCAPS_LOCALVIDMEM;
	g_lpDxDraw->GetDDraw()->GetAvailableVidMem( &ddsCaps2, &dwTotal, &dwFree);
	dwTotal >>= 20;
	dwFree >>= 20;

	hprintf( LogFile, _T("\r\n\r\nName of the driver : %s\r\n"), ddIdentifer.szDriver);
	hprintf( LogFile, _T("Description of the driver : %s\r\n"), ddIdentifer.szDescription);
	hprintf( LogFile, _T("Driver Version : %d.%d.%d.%d\r\n"), HIWORD( ddIdentifer.liDriverVersion.HighPart), LOWORD( ddIdentifer.liDriverVersion.HighPart),
														HIWORD( ddIdentifer.liDriverVersion.LowPart), LOWORD( ddIdentifer.liDriverVersion.LowPart));
	hprintf( LogFile, _T("VendorID : 0x%08X\r\n"), ddIdentifer.dwVendorId);
	hprintf( LogFile, _T("DeviceID : 0x%08X\r\n\r\n"), ddIdentifer.dwDeviceId);
	hprintf( LogFile, _T("Total Video Memory : %d MBytes\r\n"), dwTotal);
	hprintf( LogFile, _T("Free Video Memory : %d MBytes\r\n\r\n"), dwFree);
#endif
}

// Translate the exception code into something human readable.

static LPCTSTR GetExceptionDescription(DWORD ExceptionCode)
{
		struct ExceptionNames
		{
				DWORD	ExceptionCode;
				LPCTSTR	ExceptionName;
		};

		ExceptionNames ExceptionMap[] =
		{
				{0x40010005, _T("a UIWindow-C")},
				{0x40010008, _T("a UIWindow-Break")},
				{0x80000002, _T("a Datatype Misalignment")},
				{0x80000003, _T("a Breakpoint")},
				{0xc0000005, _T("an Access Violation")},
				{0xc0000006, _T("an In Page Error")},
				{0xc0000017, _T("a No Memory")},
				{0xc000001d, _T("an Illegal Instruction")},
				{0xc0000025, _T("a Noncontinuable Exception")},
				{0xc0000026, _T("an Invalid Disposition")},
				{0xc000008c, _T("a Array Bounds Exceeded")},
				{0xc000008d, _T("a Float Denormal Operand")},
				{0xc000008e, _T("a Float Divide by Zero")},
				{0xc000008f, _T("a Float Inexact Result")},
				{0xc0000090, _T("a Float Invalid Operation")},
				{0xc0000091, _T("a Float Overflow")},
				{0xc0000092, _T("a Float Stack Check")},
				{0xc0000093, _T("a Float Underflow")},
				{0xc0000094, _T("an Integer Divide by Zero")},
				{0xc0000095, _T("an Integer Overflow")},
				{0xc0000096, _T("a Privileged Instruction")},
				{0xc00000fD, _T("a Stack Overflow")},
				{0xc0000142, _T("a DLL Initialization Failed")},
				{0xe06d7363, _T("a Microsoft C++ Exception")},
		};

		for (int i = 0; i < sizeof(ExceptionMap) / sizeof(ExceptionMap[0]); i++)
				if (ExceptionCode == ExceptionMap[i].ExceptionCode)
						return ExceptionMap[i].ExceptionName;

		return _T("Unknown exception type");
}


static void ImageHelpStackWalk(HANDLE LogFile, HANDLE hThread)
{

	SuspendThread(hThread);

	CONTEXT context;
	memset(&context, 0, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;
	GetThreadContext(hThread, &context);
	// Time to print part or all of the stack to the error log. This allows
	// us to figure out the call stack, parameters, local variables, etc.
	hprintf(LogFile, _T("\r\nRegisters:\r\n"));
#ifdef	_WIN64
	hprintf(LogFile, _T("RAX=%p CS=%04x RIP=%p ContextFlags=%08x\r\n"),
			context.Rax, context.SegCs, context.Rip, context.ContextFlags);
	hprintf(LogFile, _T("RBX=%p SS=%04x RSP=%p RBP=%p\r\n"),
			context.Rbx, context.SegSs, context.Rsp, context.Rbp);
	hprintf(LogFile, _T("RCX=%p DS=%04x RSI=%p FS=%04x\r\n"),
			context.Rcx, context.SegDs, context.Rsi, context.SegFs);
	hprintf(LogFile, _T("RDX=%p ES=%04x RDI=%p GS=%04x\r\n"),
			context.Rdx, context.SegEs, context.Rdi, context.SegGs);
#else
	hprintf(LogFile, _T("EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\r\n"),
			context.Eax, context.SegCs, context.Eip, context.EFlags);
	hprintf(LogFile, _T("EBX=%08x SS=%04x ESP=%08x EBP=%08x\r\n"),
			context.Ebx, context.SegSs, context.Esp, context.Ebp);
	hprintf(LogFile, _T("ECX=%08x DS=%04x ESI=%08x FS=%04x\r\n"),
			context.Ecx, context.SegDs, context.Esi, context.SegFs);
	hprintf(LogFile, _T("EDX=%08x ES=%04x EDI=%08x GS=%04x\r\n"),
			context.Edx, context.SegEs, context.Edi, context.SegGs);
#endif

	hprintf(LogFile, _T("\r\nStack dump:\r\n"));
#ifdef	_WIN64
	g_dwStack = context.Rsp & ~(STACK_COLUMN * sizeof(DWORD) - 1);
#else
	g_dwStack = context.Esp & ~(STACK_COLUMN * sizeof(DWORD) - 1);
#endif
	g_dwStackTop = -1;
	PrintStack(LogFile, g_dwStack, g_dwStack + MAX_STACKDUMP_NUM * sizeof(DWORD));

	hprintf(LogFile, _T("\r\nCall Stack:\r\n"));

	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));

#ifdef	_WIN64
	sf.AddrPC.Offset       = context.Rip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = context.Rsp;
	sf.AddrStack.Mode      = AddrModeFlat;
#else
	sf.AddrPC.Offset       = context.Eip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = context.Esp;
	sf.AddrStack.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset    = context.Ebp;
	sf.AddrFrame.Mode      = AddrModeFlat;
#endif

	while ( 1 )
	{
#ifdef	_WIN64
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
						GetCurrentProcess(),
						GetCurrentThread(),
						&sf,
						&context,
						0,
						SymFunctionTableAccess64,
						SymGetModuleBase64,
						0))
			break;
#else
		if (!StackWalk64(IMAGE_FILE_MACHINE_I386,
						GetCurrentProcess(),
						GetCurrentThread(),
						&sf,
						&context,
						0,
						SymFunctionTableAccess64,
						SymGetModuleBase64,
						0))
			break;
#endif

		if (0 == sf.AddrFrame.Offset) // Bail if frame is not okay.
			break;

		PrintStack(LogFile, (DWORD_PTR) sf.AddrFrame.Offset - ABOVE_STACKDUMP_NUM * sizeof(DWORD_PTR), 
			(DWORD_PTR) sf.AddrFrame.Offset + BELOW_STACKDUMP_NUM *sizeof(DWORD_PTR));
		hprintf(LogFile, _T("\r\n%p %p "), (char *) sf.AddrFrame.Offset, (char *) sf.AddrPC.Offset);

		char UnDName[512];
		UnDName[0] = 0;
#if	0
		ULONG64 symbolBuffer[(sizeof(SYMBOL_INFO) + 512 * sizeof(TCHAR) + sizeof(ULONG64)-1)/sizeof(ULONG64)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLen = 512;
		DWORD64 symDisplacement = 0;

		if (SymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset,
							&symDisplacement, pSymbol)) 
#else
		ULONG64 symbolBuffer[(sizeof(PIMAGEHLP_SYMBOL64) + 512 * sizeof(TCHAR) + sizeof(ULONG64)-1)/sizeof(ULONG64)];
		PIMAGEHLP_SYMBOL64  pSymbol = (PIMAGEHLP_SYMBOL64)symbolBuffer;
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = 512;
		DWORD64 symDisplacement = 0;

		if (SymGetSymFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset,
							&symDisplacement, pSymbol)) 
#endif
		{

#ifdef	UNICODE
			hprintf(LogFile, _T("%S +%x\r\n"), pSymbol->Name, symDisplacement);
#else
			hprintf(LogFile, _T("%s +%x\r\n"), pSymbol->Name, symDisplacement);
#endif
			UnDecorateSymbolName(pSymbol->Name, UnDName, 512, UNDNAME_COMPLETE);
		}
		{
			IMAGEHLP_LINE64 line;
			DWORD lineDisplacement = 0;
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &lineDisplacement, &line)) {
#ifdef	UNICODE
				hprintf(LogFile, _T("%S %d %S\r\n"), line.FileName, line.LineNumber, UnDName);
#else
				hprintf(LogFile, _T("%s %d %s\r\n"), line.FileName, line.LineNumber, UnDName);
#endif
				goto print_param;
			}
		} 
		{
			TCHAR CrashModulePathName[MAX_PATH];
			CrashModulePathName[0] = 0;
			MEMORY_BASIC_INFORMATION MemInfo;
			// VirtualQuery can be used to get the allocation base associated with a
			// code address, which is the same as the ModuleHandle. This can be used
			// to get the filename of the module that the crash happened in.
			if (VirtualQuery((void*)sf.AddrPC.Offset, &MemInfo, sizeof(MemInfo)))
				GetModuleFileName((HINSTANCE)MemInfo.AllocationBase,
					CrashModulePathName, MAX_PATH);
#ifdef	UNICODE
			hprintf(LogFile, _T("%s %S\r\n"), CrashModulePathName, UnDName);
#else
			hprintf(LogFile, _T("%s %s\r\n"), CrashModulePathName, UnDName);
#endif
		}
print_param:
		hprintf(LogFile, _T("Params:   %p %p %p %p\r\n"), (char *) sf.Params[0], (char *) sf.Params[1], (char *) sf.Params[2], (char *) sf.Params[3]);
	}
	hprintf(LogFile, _T("\r\n"));
	ResumeThread(hThread);
}

int XIOException::Filter(LPEXCEPTION_POINTERS pExp)
{
	if (g_nEnable && pExp->ExceptionRecord->ExceptionCode != 0xe0000001) {
		Lock();
		GenerateExceptionReport(pExp);
		Unlock();
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

void	XIOException::Open(LPCTSTR szMailServer, LPCTSTR szMailFrom, LPCTSTR szMailTo, int nPort)
{
#ifdef	UNICODE
	if (szMailServer && szMailServer[0]
		&& WideCharToMultiByte(CP_ACP, 0, szMailServer, -1, g_szMailServer, sizeof(g_szMailServer), NULL, NULL) 
		&& WideCharToMultiByte(CP_ACP, 0, szMailFrom, -1, g_szMailFrom, sizeof(g_szMailFrom), NULL, NULL)
		&& WideCharToMultiByte(CP_ACP, 0, szMailTo, -1, g_szMailTo, sizeof(g_szMailTo), NULL, NULL))
		;

#else
	if (szMailServer && szMailServer[0]) {
		StringCbCopy(g_szMailServer, sizeof(g_szMailServer), szMailServer);
		StringCbCopy(g_szMailFrom, sizeof(g_szMailFrom), szMailFrom);
		StringCbCopy(g_szMailTo, sizeof(g_szMailTo), szMailTo);
	}
#endif
	else {
		g_szMailServer[0] = 0;
		g_szMailFrom[0] = 0;
		g_szMailTo[0] = 0;
	}
	g_nMailPort = nPort;

	HANDLE hFile = CreateFile(g_szLogPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		SendMail();
	}
}

void	XIOException::DumpStack(int nThread, HANDLE *hThread, unsigned *nThreadId)
{
	Lock();
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	HANDLE LogFile = CreateFile(g_szLogPath, GENERIC_WRITE, FILE_SHARE_READ, 0,
	OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
	if (LogFile == INVALID_HANDLE_VALUE) {
		Unlock();
		return;
	}
	// Append to the error log.
#ifdef	UNICODE
	if (SetFilePointer(LogFile, 0, NULL, FILE_END) == 0) {
		hprintf(LogFile, _T("\xFEFF"));
	}
#else
	SetFilePointer(LogFile, 0, NULL, FILE_END);
#endif
	// Print out some blank lines to separate this error log from any previous ones.

	SYSTEMTIME st = {0};
	::GetLocalTime(&st);
	hprintf(LogFile,  
		_T("%#x(%d) %04d/%02d/%02d %02d:%02d:%02d> Stack Dump %d\r\n"), 
		GetCurrentThreadId(), GetCurrentThreadId(), 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, g_nBreak );
	struct tm tm;
	localtime_s(&tm, &g_timeStart);
	hprintf(LogFile, _T("start at %02d/%02d/%02d %02d:%02d:%02d\r\n"), tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (SymInitialize(GetCurrentProcess(), 0, TRUE)) {
		DWORD dwOpts = SymGetOptions () ;
		if ((dwOpts & ( SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS)) != 
			(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS))
			SymSetOptions ( (dwOpts & ~SYMOPT_UNDNAME) | (SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS));
		for (int i = 0; i < nThread; i++) {
			hprintf(LogFile, _T("Call Stack Information %d %#x(%d):\r\n"), i, nThreadId[i], nThreadId[i]);
			ImageHelpStackWalk(LogFile, hThread[i]);
		}
		RecordModuleList(LogFile);
		SymCleanup(GetCurrentProcess());
	}
	CloseHandle(LogFile);
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	CreateMiniDump(0);
	Unlock();
}

BOOL XIOException::SendMail( const char *host, const char *from, const char *to, LPCTSTR body, int port)
{
	return sendMail( host, port, from, to, body);
}


// Record information about the user's system, such as processor type, amount
// of memory, etc.






void XIOException::Start()
{
	if (g_pPreviousFilter) {
		return;
	}
	InitializeCriticalSection(&XIOException::s_lock); 

	GetModuleFileName(0, g_szLogPath, sizeof(g_szLogPath));
	LPTSTR FilePart = GetFilePart(g_szLogPath);
	_stprintf_s(FilePart, g_szLogPath + sizeof(g_szLogPath)/sizeof(TCHAR) - FilePart, _T("Exception.txt"));

	GetModuleFileName(0, g_szDumpPath, sizeof(g_szDumpPath));
	FilePart = GetFilePart(g_szDumpPath);
	_stprintf_s(FilePart, g_szDumpPath + sizeof(g_szDumpPath) / sizeof(TCHAR) - FilePart, _T("Exception.dmp"));

	g_timeStart = time(NULL);
	g_pPreviousFilter = SetUnhandledExceptionFilter(RecordExceptionInfo);
	// _set_security_error_handler(SecurityHandler);
}

void XIOException::Stop()
{
	if (!g_pPreviousFilter)
		return;
	SetUnhandledExceptionFilter(g_pPreviousFilter);
	DeleteCriticalSection(&XIOException::s_lock);
	g_pPreviousFilter = 0;
}

void ELOG(LPCTSTR lpszFormat, ...)
{
	if (!g_nEnable)
		return;
	if (InterlockedIncrement(&g_nLog) > 1000)
		return;
	XIOException::Lock();

//if (IsDebuggerPresent())
//		DebugBreak();

	va_list args;
	va_start(args, lpszFormat);

	TCHAR buff[1024];
	SYSTEMTIME st = {0};

	::GetLocalTime(&st);

	HANDLE hFile  = CreateFile(g_szLogPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
			OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		XIOException::Unlock();
		return;
	}


	DWORD dwWritten;
#ifdef	UNICODE
	if (SetFilePointer(hFile, 0, NULL, FILE_END) == 0) {
		WriteFile(hFile, _T("\xFEFF"), sizeof(TCHAR), &dwWritten, NULL);
	}
#else
	SetFilePointer(hFile, 0, NULL, FILE_END);
#endif

	int len;
	len = _stprintf_s(buff, sizeof(buff)/sizeof(TCHAR),  
		_T("%#x(%d) %04d/%02d/%02d %02d:%02d:%02d> "), 
		GetCurrentThreadId(), GetCurrentThreadId(), 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );
	WriteFile(hFile, buff, len * sizeof(TCHAR), &dwWritten, NULL);
	len = _vstprintf_s(buff, sizeof(buff)/sizeof(TCHAR), lpszFormat, args);
	WriteFile(hFile, buff, len * sizeof(TCHAR), &dwWritten, NULL);
	WriteFile(hFile, _T("\r\n"), 2 * sizeof(TCHAR), &dwWritten, NULL);

	va_end(args);

	CloseHandle(hFile);
	XIOException::Unlock();
}

#ifdef	_WIN64

#define NAKED 
#define SNAPPROLOG(Cntx)                                               
#define SNAPEPILOG(eRetVal)  return eRetVal;                                          
#define COPYKEYCONTEXTREGISTERS(stFinalCtx,stInitialCtx)               

#else

#define NAKED __declspec ( naked )
#define SNAPPROLOG(Cntx)                                               \
__asm PUSH  EBP                   /* Save EBP explictly.            */ \
__asm MOV   EBP , ESP             /* Move the stack.                */ \
__asm SUB   ESP , __LOCAL_SIZE    /* Space for the local variables. */ \
/* Copy over all the easy current registers values. */                 \
__asm MOV   Cntx.Eax , EAX                                             \
__asm MOV   Cntx.Ebx , EBX                                             \
__asm MOV   Cntx.Ecx , ECX                                             \
__asm MOV   Cntx.Edx , EDX                                             \
__asm MOV   Cntx.Edi , EDI                                             \
__asm MOV   Cntx.Esi , ESI                                             \
/* Zero put the whole EAX register and just copy the segments into  */ \
/* the lower word.  This avoids leaving the upper word uninitialized*/ \
/* as the context segment registers are really 32-bit values.       */ \
__asm XOR   EAX , EAX                                                  \
__asm MOV   AX , GS                                                    \
__asm MOV   Cntx.SegGs , EAX                                           \
__asm MOV   AX , FS                                                    \
__asm MOV   Cntx.SegFs , EAX                                           \
__asm MOV   AX , ES                                                    \
__asm MOV   Cntx.SegEs , EAX                                           \
__asm MOV   AX , DS                                                    \
__asm MOV   Cntx.SegDs , EAX                                           \
__asm MOV   AX , CS                                                    \
__asm MOV   Cntx.SegCs , EAX                                           \
__asm MOV   AX , SS                                                    \
__asm MOV   Cntx.SegSs , EAX                                           \
/* Get the previous EBP value. */                                      \
__asm MOV  EAX , DWORD PTR [EBP]                                       \
__asm MOV  Cntx.Ebp , EAX                                              \
/* Get the previous ESP value. */                                      \
__asm MOV  EAX , EBP                                                   \
/* Two DWORDs up from EBP is the previous stack address. */            \
__asm ADD  EAX , 8                                                     \
__asm MOV  Cntx.Esp , EAX                                              \
/* Save changed registers. */                                          \
__asm PUSH ESI                                                         \
__asm PUSH EDI                                                         \
__asm PUSH EBX                                                         \
__asm PUSH ECX                                                         \
__asm PUSH EDX


#define SNAPEPILOG(eRetVal)                                            \
__asm POP     EDX             /* Restore saved registers.  */          \
__asm POP     ECX                                                      \
__asm POP     EBX                                                      \
__asm POP     EDI                                                      \
__asm POP     ESI                                                      \
__asm MOV     EAX , eRetVal   /* Set the return value.      */         \
__asm MOV     ESP , EBP       /* Restore the stack pointer. */         \
__asm POP     EBP             /* Restore the frame pointer. */         \
__asm RET                     /* Return to caller.          */

// Just a wrapper to do the context copy.
#define COPYKEYCONTEXTREGISTERS(stFinalCtx,stInitialCtx)               \
stFinalCtx.Eax   = stInitialCtx.Eax   ;                                \
stFinalCtx.Ebx   = stInitialCtx.Ebx   ;                                \
stFinalCtx.Ecx   = stInitialCtx.Ecx   ;                                \
stFinalCtx.Edx   = stInitialCtx.Edx   ;                                \
stFinalCtx.Edi   = stInitialCtx.Edi   ;                                \
stFinalCtx.Esi   = stInitialCtx.Esi   ;                                \
stFinalCtx.SegGs = stInitialCtx.SegGs ;                                \
stFinalCtx.SegFs = stInitialCtx.SegFs ;                                \
stFinalCtx.SegEs = stInitialCtx.SegEs ;                                \
stFinalCtx.SegDs = stInitialCtx.SegDs ;                                \
stFinalCtx.SegCs = stInitialCtx.SegCs ;                                \
stFinalCtx.SegSs = stInitialCtx.SegSs ;                                \
stFinalCtx.Ebp   = stInitialCtx.Ebp
#endif

extern "C" void * _ReturnAddress(void);

#pragma intrinsic(_ReturnAddress)


DWORD NAKED  SnapCurrentProcessMiniDump(LPCONTEXT lpContext)
{
    CONTEXT stInitialCtx;
	stInitialCtx;

    DWORD	eRet;

    SNAPPROLOG ( stInitialCtx ) ;

    ZeroMemory (lpContext, sizeof ( CONTEXT ) ) ;
        
#ifdef	_WIN64
    lpContext->ContextFlags = CONTEXT_ALL;
								
#else
    lpContext->ContextFlags = CONTEXT_FULL                 |
                                CONTEXT_CONTROL            |
                                CONTEXT_DEBUG_REGISTERS    |
                                CONTEXT_EXTENDED_REGISTERS |
                                CONTEXT_FLOATING_POINT       ;
#endif
                                    
    // Get all the groovy context registers and such for this
    // thread.
    if (GetThreadContext ( GetCurrentThread ( ) ,lpContext))
    {
        COPYKEYCONTEXTREGISTERS ((*lpContext), stInitialCtx ) ;

        UINT_PTR nRetAddr = (UINT_PTR)_ReturnAddress ( ) ;

        *(PDWORD_PTR) &lpContext->RIP = nRetAddr ;
		eRet = TRUE;            
    }
	else
		eRet = FALSE;
    // Do the epilog.
    SNAPEPILOG ( eRet ) ;
}

static void GenerateExceptionReport()
{
	CONTEXT stContext;
    EXCEPTION_RECORD stExRec ;
    EXCEPTION_POINTERS stExpPtrs ;

	SnapCurrentProcessMiniDump(&stContext);
	// Zero out all the individual values.
	ZeroMemory ( &stExRec , sizeof ( EXCEPTION_RECORD )) ;
	ZeroMemory ( &stExpPtrs , sizeof ( EXCEPTION_POINTERS ) ) ;

	stExRec.ExceptionAddress = (PVOID)(DWORD_PTR) (stContext.RIP) ;

	// Set the exception pointers.
	stExpPtrs.ContextRecord = &stContext;
	stExpPtrs.ExceptionRecord = &stExRec ;

	GenerateExceptionReport(&stExpPtrs);
}



void __stdcall EBREAK()
{
	if (!g_nEnable)
		return;

//	if (IsDebuggerPresent())
//		DebugBreak();

	long nBreak = InterlockedIncrement(&g_nBreak);
#ifndef	_WIN64
	_asm {
		push edx
		push ecx
		push eax
	}
#endif
	if (InterlockedExchange(&g_bBreak, 0) != 0) {
		ELOG(_T("EASSERT %d"), nBreak);
		XIOException::Lock();
		GenerateExceptionReport();
		sendMail();
		XIOException::Unlock();
	}
#ifndef	_WIN64
	_asm {
		pop eax
		pop ecx
		pop edx
	}
#endif
}


static void ShowModuleInfo(HANDLE LogFile, HINSTANCE ModuleHandle)
{
	TCHAR ModName[MAX_PATH];
	__try
	{
		if (GetModuleFileName(ModuleHandle, ModName, MAX_PATH) > 0)
		{
			// If GetModuleFileName returns greater than zero then this must
			// be a valid code module address. Therefore we can try to walk
			// our way through its structures to find the link time stamp.
			IMAGE_DOS_HEADER *DosHeader = (IMAGE_DOS_HEADER*)ModuleHandle;
			if (IMAGE_DOS_SIGNATURE != DosHeader->e_magic)
				return;
			IMAGE_NT_HEADERS *NTHeader = (IMAGE_NT_HEADERS*)((char *)DosHeader
										+ DosHeader->e_lfanew);
			if (IMAGE_NT_SIGNATURE != NTHeader->Signature)
				return;
#if 1
			struct tm  tm;
			const __time32_t nTimeStamp = NTHeader->FileHeader.TimeDateStamp; // +9 * 3600;
			//_gmtime32_s(&tm, &nTimeStamp);
			_localtime32_s(&tm, &nTimeStamp);
			hprintf(LogFile, _T("%s, loaded at 0x%08x - %02d/%02d/%02d %02d:%02d:%02d (KR)\r\n"),
				ModName, ModuleHandle, 
				tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);


#else
			// Open the code module file so that we can get its file date
			// and size.
			HANDLE ModuleFile = CreateFile(ModName, GENERIC_READ,
									FILE_SHARE_READ, 0, OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL, 0);
			char TimeBuffer[100] = ")");
			DWORD FileSize = 0;
			if (ModuleFile != INVALID_HANDLE_VALUE)
			{
				FileSize = GetFileSize(ModuleFile, 0);
				FILETIME	LastWriteTime;
				if (GetFileTime(ModuleFile, 0, 0, &LastWriteTime))
				{
					sprintf(TimeBuffer, _T(" - file date is "));
					PrintTime(TimeBuffer + lstrlenA(TimeBuffer), LastWriteTime);
				}
				CloseHandle(ModuleFile);
			}
			hprintf(LogFile, _T("%s, loaded at 0x%08x - %d bytes - %08x%s\r\n"),
				ModName, ModuleHandle, FileSize,
				NTHeader->FileHeader.TimeDateStamp, TimeBuffer);
#endif
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

static void RecordModuleList(HANDLE LogFile)
{
	hprintf(LogFile, _T("Module list:\r\n"));
	ShowModuleInfo(LogFile, GetModuleHandle(0));


}

static LPTSTR GetFilePart(LPTSTR source)
{
		LPTSTR result = _tcsrchr(source, '\\');
		if (result)
				result++;
		else
				result = source;
		return result;
}

extern "C" void * _ReturnAddress(void);

#pragma intrinsic(_ReturnAddress)







#ifdef	_WIN64

#define NAKED 
#define SNAPPROLOG(Cntx)                                               
#define SNAPEPILOG(eRetVal)  return eRetVal;                                          
#define COPYKEYCONTEXTREGISTERS(stFinalCtx,stInitialCtx)               

#else

#define NAKED __declspec ( naked )
#define SNAPPROLOG(Cntx)                                               \
__asm PUSH  EBP                   /* Save EBP explictly.            */ \
__asm MOV   EBP , ESP             /* Move the stack.                */ \
__asm SUB   ESP , __LOCAL_SIZE    /* Space for the local variables. */ \
/* Copy over all the easy current registers values. */                 \
__asm MOV   Cntx.Eax , EAX                                             \
__asm MOV   Cntx.Ebx , EBX                                             \
__asm MOV   Cntx.Ecx , ECX                                             \
__asm MOV   Cntx.Edx , EDX                                             \
__asm MOV   Cntx.Edi , EDI                                             \
__asm MOV   Cntx.Esi , ESI                                             \
/* Zero put the whole EAX register and just copy the segments into  */ \
/* the lower word.  This avoids leaving the upper word uninitialized*/ \
/* as the context segment registers are really 32-bit values.       */ \
__asm XOR   EAX , EAX                                                  \
__asm MOV   AX , GS                                                    \
__asm MOV   Cntx.SegGs , EAX                                           \
__asm MOV   AX , FS                                                    \
__asm MOV   Cntx.SegFs , EAX                                           \
__asm MOV   AX , ES                                                    \
__asm MOV   Cntx.SegEs , EAX                                           \
__asm MOV   AX , DS                                                    \
__asm MOV   Cntx.SegDs , EAX                                           \
__asm MOV   AX , CS                                                    \
__asm MOV   Cntx.SegCs , EAX                                           \
__asm MOV   AX , SS                                                    \
__asm MOV   Cntx.SegSs , EAX                                           \
/* Get the previous EBP value. */                                      \
__asm MOV  EAX , DWORD PTR [EBP]                                       \
__asm MOV  Cntx.Ebp , EAX                                              \
/* Get the previous ESP value. */                                      \
__asm MOV  EAX , EBP                                                   \
/* Two DWORDs up from EBP is the previous stack address. */            \
__asm ADD  EAX , 8                                                     \
__asm MOV  Cntx.Esp , EAX                                              \
/* Save changed registers. */                                          \
__asm PUSH ESI                                                         \
__asm PUSH EDI                                                         \
__asm PUSH EBX                                                         \
__asm PUSH ECX                                                         \
__asm PUSH EDX


#define SNAPEPILOG(eRetVal)                                            \
__asm POP     EDX             /* Restore saved registers.  */          \
__asm POP     ECX                                                      \
__asm POP     EBX                                                      \
__asm POP     EDI                                                      \
__asm POP     ESI                                                      \
__asm MOV     EAX , eRetVal   /* Set the return value.      */         \
__asm MOV     ESP , EBP       /* Restore the stack pointer. */         \
__asm POP     EBP             /* Restore the frame pointer. */         \
__asm RET                     /* Return to caller.          */

// Just a wrapper to do the context copy.
#define COPYKEYCONTEXTREGISTERS(stFinalCtx,stInitialCtx)               \
stFinalCtx.Eax   = stInitialCtx.Eax   ;                                \
stFinalCtx.Ebx   = stInitialCtx.Ebx   ;                                \
stFinalCtx.Ecx   = stInitialCtx.Ecx   ;                                \
stFinalCtx.Edx   = stInitialCtx.Edx   ;                                \
stFinalCtx.Edi   = stInitialCtx.Edi   ;                                \
stFinalCtx.Esi   = stInitialCtx.Esi   ;                                \
stFinalCtx.SegGs = stInitialCtx.SegGs ;                                \
stFinalCtx.SegFs = stInitialCtx.SegFs ;                                \
stFinalCtx.SegEs = stInitialCtx.SegEs ;                                \
stFinalCtx.SegDs = stInitialCtx.SegDs ;                                \
stFinalCtx.SegCs = stInitialCtx.SegCs ;                                \
stFinalCtx.SegSs = stInitialCtx.SegSs ;                                \
stFinalCtx.Ebp   = stInitialCtx.Ebp
#endif


extern "C" void * _ReturnAddress(void);

#pragma intrinsic(_ReturnAddress)

//DWORD NAKED  SnapCurrentProcessMiniDump(LPCONTEXT lpContext);

static LONG WINAPI RecordExceptionInfo(PEXCEPTION_POINTERS data)
{
	static int BeenHere;

	if (g_nEnable && data->ExceptionRecord->ExceptionCode != 0xe0000001) {
		XIOException::Lock();
		if (BeenHere) {     // Going recursive! That must mean this routine crashed!
			XIOException::Unlock();
			ELOG(_T("Recursive Exception"));
			goto quit;
		}
		BeenHere = true;
		GenerateExceptionReport(data);
		sendMail();
		XIOException::Unlock();
	}

quit:
	if (g_pPreviousFilter)
		return g_pPreviousFilter(data);
	else
		return EXCEPTION_CONTINUE_SEARCH;
}
