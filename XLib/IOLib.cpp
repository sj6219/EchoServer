#include "pch.h"
#include <windows.h>
#include <tchar.h>
#include "IOLIb.h"
#include "IOException.h"
#include "IOSocket.h"
#include "IOScreen.h"

#if defined(_DEBUG)
void TRACE(LPCTSTR lpszFormat, ...)
{
	TCHAR buff[1024];
	va_list args;
	va_start(args, lpszFormat);
	StringCbVPrintf(buff, sizeof(buff), lpszFormat, args);
	OutputDebugString(buff);
	va_end(args);
}

void	BREAK()
{
	//__asm int 3 
	DebugBreak();
}
#endif

class XIOLibInit
{
public:
	XIOLibInit()
	{
		//XIOMemory::Start();
		//XIOException::Start();
	}
	~XIOLibInit()
	{
		//XIOLog::Stop();
		//XIOException::Stop();
		//XIOSocket::Stop();
		//XIOMemory::Stop();
	}
};
#pragma warning(disable: 4073) 
#pragma init_seg(lib)
XIOLibInit	theInit;















void XIOSpinLock::Wait()
{
	int count = 4000;
	while (--count >= 0)
	{
		if (InterlockedCompareExchange(&lock, 1, 0) == 0)
			return;
#ifndef	_WIN64
		__asm pause //__asm { rep nop} 
#endif
	}
	count = 4000;
	while (--count >= 0)
	{
		SwitchToThread(); //Sleep(0);
		if (InterlockedCompareExchange(&lock, 1, 0) == 0)
			return;
	}
	for ( ; ; )
	{
		Sleep(1000);
		if (InterlockedCompareExchange(&lock, 1, 0) == 0)
			return;
	}
}








#pragma	warning(disable: 4146)	// unary minus operator applied to unsigned type, result still unsigned

// 0x80000000 Read  Thread wating for m_hWEvent
// 0x7f000000 Write Thread wating for m_hWEvent
// 0x00ff0000 Read Thread wating for m_hREvent
// 0x0000ff00 Active Write Lock
// 0x000000ff Active Read Lock
XIORWLock::XIORWLock()  
{ 
	m_nCount = 0;
	m_nLock = 0;
	m_hREvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hWEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	m_nRWCount = 0;
}

XIORWLock::~XIORWLock() 
{ 
	ASSERT(m_nCount == 0);
	CloseHandle(m_hREvent); 
	CloseHandle(m_hWEvent);
}

void XIORWLock::Wait(/*int mode*/)
{
	int count;
	
	count = 4000;
	while (--count >= 0)  
	{
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0) 
			return;
#ifndef	_WIN64
		__asm pause //__asm { rep nop} 
#endif
	} 
	count = 4000;
	while (--count >= 0)
	{
		SwitchToThread(); //Sleep(0);
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0)
			return;
	}
	for ( ; ; )
	{
		Sleep(1000);
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0)
			return;
	}
}

void XIORWLock::ReadLock()
{
//	InterlockedExchangeAdd(&m_nRWCount, 0x10000);
	int nCount;

	nCount = InterlockedIncrement(&m_nCount);
	if (nCount & 0xff00)
	{
		Lock();
		nCount = m_nCount;
		do
		{
			if (nCount >= 0)
			{
				nCount = InterlockedExchangeAdd(&m_nCount, 0x80000000 - 1) + 0x80000000 - 1;
				if ((short)nCount == 0)
				{
					Unlock();
					SetEvent(m_hWEvent);
				}
				else
					Unlock();
				WaitForSingleObject(m_hWEvent, INFINITE);
				Lock();
				nCount = InterlockedExchangeAdd(&m_nCount, -0x80000000 + 1) -0x80000000 + 1;
			}
			else
			{
				nCount = InterlockedExchangeAdd(&m_nCount, 0x10000 - 1) + 0x10000 - 1;
				if ((short)nCount == 0)
				{
					Unlock();
					SetEvent(m_hWEvent);
				}
				else
					Unlock();
				WaitForSingleObject(m_hREvent, INFINITE);
				Lock();
				nCount = InterlockedExchangeAdd(&m_nCount, -0x10000 + 1) -0x10000 + 1;
			}
		}
		while ((nCount & 0xff00) != 0);

		if (nCount & 0xff0000)
		{
			Unlock();
			SetEvent(m_hREvent);
		}
		else
			Unlock();
	}
//	InterlockedExchangeAdd(&m_nRWCount, -0x10000 + 1);
}

void XIORWLock::ReadUnlock() 
{
//	InterlockedExchangeAdd(&m_nRWCount, 0x100000 - 1);

	int nCount = InterlockedDecrement(&m_nCount);
	ASSERT((nCount & 0x8080) == 0);
	if ((nCount & 0xff000000) && (short)nCount == 0)
	{
		SetEvent(m_hWEvent);
	}
//	InterlockedExchangeAdd(&m_nRWCount, -0x100000);
}

void XIORWLock::WriteLock() 
{
	int nCount;

//	InterlockedExchangeAdd(&m_nRWCount, 0x1000000);
	nCount = InterlockedExchangeAdd(&m_nCount, 0x100);
	if ((short)nCount)
	{
		Lock();
		do
		{
			nCount = InterlockedExchangeAdd(&m_nCount, 0x1000000 - 0x100) + 0x1000000 - 0x100;
			if ((short)nCount == 0)
			{
				Unlock();
				SetEvent(m_hWEvent);
			}
			else
				Unlock();
			WaitForSingleObject(m_hWEvent, INFINITE);
			Lock();
			nCount = InterlockedExchangeAdd(&m_nCount, -0x1000000 + 0x100);

		}
		while ((short)nCount);
		Unlock();
	}
//	InterlockedExchangeAdd(&m_nRWCount, -0x1000000 + 0x10);
}

void XIORWLock::WriteUnlock() 
{
//	InterlockedExchangeAdd(&m_nRWCount, 0x10000000 - 0x10);
	int	nCount = InterlockedExchangeAdd(&m_nCount, -0x100) - 0x100;
	ASSERT((nCount & 0x8080) == 0);
	if(( nCount & 0xff000000) && (short)nCount == 0)
	{
		SetEvent(m_hWEvent);
	}
//	InterlockedExchangeAdd(&m_nRWCount, -0x10000000);
}

BOOL XIORWLock::WriteTryLock()
{
	if ((short)InterlockedExchangeAdd(&m_nCount, 0x100))
	{
		InterlockedExchangeAdd(&m_nCount, -0x100);
		return FALSE;
	}
	return TRUE;
}




XIOScreen XIOLog::s_screen;

static FILE *g_pFile[5];
static COLORREF g_Color[] = { RGB(0, 0, 255), RGB(0, 0, 0), RGB(128, 0, 0), RGB(255, 0, 0), RGB(255, 0, 255) };
static UINT g_nNext[5];


void LOG_INFO(LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);
	XIOLog::AddV(XIOLog::INFO, format, va);
	va_end(va);
}

void LOG_NORMAL(LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);
	XIOLog::AddV(XIOLog::NORMAL, format, va);
	va_end(va);
}

void LOG_WARN(LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);
	XIOLog::AddV(XIOLog::WARN, format, va);
	va_end(va);
}

void LOG_ERR(LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);
	XIOLog::AddV(XIOLog::ERR, format, va);
	va_end(va);
}

void LOG_HACK(LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);
	XIOLog::AddV(XIOLog::HACK, format, va);
	va_end(va);
}

void	XIOLog::Stop()
{
	for (int i = 0; i < sizeof(g_pFile) / sizeof(*g_pFile); i++)
	{
		if (g_pFile[i])
			fclose(g_pFile[i]);
	}
}


void XIOLog::Flush()
{
	s_screen.Lock();
	for (int i = 0; i < sizeof(g_pFile) / sizeof(*g_pFile); i++)
	{
		if (g_pFile[i])
			fflush(g_pFile[i]);
	}
	s_screen.Unlock();
}

void XIOLog::AddV(int nType, LPCTSTR format, va_list va)
{
	TCHAR buffer[1024];

	_vstprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), format, va);

	s_screen.Lock();
	s_screen.AddString(g_Color[nType], buffer);
	if (nType >= NORMAL)
	{
		time_t t;
		time(&t);
		struct tm *tm = localtime(&t);
		if (t >= g_nNext[nType])
		{
			TCHAR szFileName[256];
			if (g_pFile[nType])
				fclose(g_pFile[nType]);

			_stprintf_s(szFileName, sizeof(szFileName)/sizeof(TCHAR), 
				(nType == NORMAL) ? _T("Log\\%02d%02d%02d.log") :
				(nType == WARN) ? _T("Log\\%02d%02d%02d.warn.log") :
				(nType == ERR) ? _T("Log\\%02d%02d%02d.err.log") : _T("Log\\%02d%02d%02d.hack.log"),
				tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday);
			struct tm tmOld = *tm;
			tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
			g_nNext[nType] = mktime(tm) + 3600 * 24;
			g_pFile[nType] = _tfopen(szFileName, _T("ab"));
#ifdef	UNICODE
			if (g_pFile[nType]) {
				fseek(g_pFile[nType], 0, SEEK_END);
				if (ftell(g_pFile[nType]) == 0) {
					fwrite(_T("\xFEFF"), 1, sizeof(TCHAR), g_pFile[nType]);
				}
			}
#endif
			*tm = tmOld;
		}

		if (g_pFile[nType])
			_ftprintf(g_pFile[nType], _T("%02d/%02d/%02d %02d:%02d:%02d> %s\r\n"), tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, buffer);
	}
	s_screen.Unlock();

	if (XIOScreen::s_pScreen == &XIOLog::s_screen)
		InvalidateRect(XIOScreen::s_hWnd, NULL, TRUE);
}

