#include "Stdafx.h"
#include "EchoConfig.h"
//#include "IOLib.h"
#include "IOScreen.h"
#include "Utility.h"
#include "Lisp.h"
#include "XParser.h"
#include "XFile.h"

#define ECHO_PORT 50004
//#define USE_PARSELIST

int	CEchoConfig::s_nMailBindPort;
tstring CEchoConfig::s_strMailFrom;
tstring CEchoConfig::s_strMailTo;
int CEchoConfig::s_nPort;
BOOL	CEchoConfig::s_bAutoStart;
int	CEchoConfig::s_nNumberOfThreads;
int	CEchoConfig::s_nMaxUser;
time_t CEchoConfig::s_nTimeStamp;
tstring CEchoConfig::s_strMailServer;

template <typename T> T GetValue(lisp::var var, LPCTSTR name, T defaultValue)
{
	lisp::var value = var.get(name).nth(1);
	return value.null() ? defaultValue : (T) value;
}


BOOL CEchoConfig::Load(lisp::var var)
{
	if (var.errorp())
	{
		LOG_ERR(_T("can't open EchoConfig.txt"));
		return FALSE;
	}

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	s_strMailServer = GetValue<LPCTSTR>(var, _T("MailServer"), _T(""));
	s_nMailBindPort = GetValue<int>(var, _T("MailBindPort"), 25);
	s_strMailFrom = GetValue<LPCTSTR>(var, _T("MailFrom"), _T(""));
	s_strMailTo = GetValue<LPCTSTR>(var, _T("MailTo"), _T(""));
	s_nPort = GetValue<int>(var, _T("Port"), ECHO_PORT);
	s_nNumberOfThreads = GetValue<int>(var, _T("NumberOfThreads"), sysinfo.dwNumberOfProcessors * 2);
	s_nMaxUser = GetValue<int>(var, _T("MaxUser"), 5000);
	s_bAutoStart = GetValue<int>(var, _T("AutoStart"), 0);
	s_nTimeStamp = GetTimeStamp();
	LPCTSTR str = GetValue<LPCTSTR>(var, _T("Title"), _T(""));
	if (str[0])
	{
		TCHAR buf[32];
		_stprintf(buf, _T("%s on port %d"), str, s_nPort);
		SetWindowText(XIOScreen::s_hWnd, buf);
	}
	return TRUE;
}

BOOL CEchoConfig::Open()
{

	XParser parser;

#ifdef USE_PARSELIST
	XFileEx file;
	if (!file.Open(_T("EchoConfig.txt")))
	{
		LOG_ERR(_T("can't open EchoConfig.txt"));
		return FALSE;
	}
	parser.Open(&file);
	auto func = [] (lisp::var var, void *param)->DWORD_PTR 
	{
		return CEchoConfig::Load(var);
	};
	return parser.ParseList(func, NULL);
#else
	lisp::var var = parser.Load(_T("EchoConfig.txt"));
	auto ret = CEchoConfig::Load(var);
	var.destroy();
	return ret;
#endif

}

void CEchoConfig::Close()
{
}

