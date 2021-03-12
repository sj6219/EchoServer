#include "pch.h"
#include "IOLIb.h"
#include "IOException.h"
#include "IOSocket.h"
#include "IOScreen.h"
#include "Utility.h"
#include <windows.h>
#include <tchar.h>

#if defined(_DEBUG)
void TRACE(LPCTSTR lpszFormat, ...)
{
	TCHAR buff[1024];
	va_list args;
	va_start(args, lpszFormat);
	_stprintf_s(buff,  lpszFormat, args);
	OutputDebugString(buff);
	va_end(args);
}

void	BREAK()
{
	//__asm int 3 
	DebugBreak();
}
#endif

//class XIOLibInit
//{
//public:
//	XIOLibInit()
//	{
//		//XIOMemory::Start();
//		//XIOException::Start();
//	}
//	~XIOLibInit()
//	{
//		//XIOLog::Stop();
//		//XIOException::Stop();
//		//XIOSocket::Stop();
//		//XIOMemory::Stop();
//	}
//};
//#pragma warning(disable: 4073) 
//#pragma init_seg(lib)
//XIOLibInit	theInit;

tstring Format(LPCTSTR format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	int n = _vsctprintf(format, arglist) + 1;
	TCHAR *p = (TCHAR *) _alloca(sizeof(TCHAR) * n);
	_vstprintf_s(p, n, format, arglist);
	va_end(arglist);
	return p;
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
				_T("Log\\%02d%02d%02d.err.log"),
				tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday);
			struct tm tmOld = *tm;
			tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
			g_nNext[nType] = mktime(tm) + 3600 * 24;
			CreatePath(szFileName);
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

