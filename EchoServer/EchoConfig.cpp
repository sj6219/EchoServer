#include "Stdafx.h"
#include "EchoConfig.h"
#include "IOLib.h"
#include "IOScreen.h"
#include "Lisp.h"
#include "XParser.h"
#include "Update.h"
#include "Misc.h"

CEchoConfig::AddressVector CEchoConfig::s_vector;
long CEchoConfig::s_index;

int	CEchoConfig::s_nMailBindPort;
int	CEchoConfig::s_nBindPort;
int	CEchoConfig::s_nConnectPort;
tstring CEchoConfig::s_strMailFrom;
tstring CEchoConfig::s_strMailTo;
int CEchoConfig::s_nPort;
BOOL	CEchoConfig::s_bAutoStart;
BOOL	CEchoConfig::s_vbChannel[UPDATE_CHANNEL_SIZE];
int	CEchoConfig::s_nNumberOfThreads;
int	CEchoConfig::s_nMaxUser;
INT64	CEchoConfig::s_nMaxUpdateSize;
time_t CEchoConfig::s_nTimeStamp;
tstring CEchoConfig::s_strServer;
tstring CEchoConfig::s_strMailServer;

template <typename T> T GetValue(lisp::var var, LPCTSTR name, T defaultValue)
{
	lisp::var value = var.get(name).nth(1);
	return value.null() ? defaultValue : (T) value;
}




BOOL CEchoConfig::Open()
{
	CONVERT;

	LPCTSTR str;

	XParser parser;
	lisp::var var = parser.Load(_T("EchoConfig.txt"));
	if (var.errorp())
	{
		LOG_ERR(_T("can't open EchoConfig.txt"));
		return FALSE;
	}

	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo);
	s_strMailServer = GetValue<LPCTSTR>(var, _T("MailServer"), _T(""));
	s_nMailBindPort = GetValue<int>(var, _T("MailBindPort"), 0);
	s_strMailFrom = GetValue<LPCTSTR>(var, _T("MailFrom"), _T(""));
	s_strMailTo = GetValue<LPCTSTR>(var, _T("MailTo"), _T(""));
	s_nPort = GetValue<int>(var, _T("Port"), UPDATE_PORT);
	s_nConnectPort = GetValue<int>(var, _T("ConnectPort"), s_nPort);
	s_nBindPort = GetValue<int>(var, _T("BindPort"), 0);
	s_nNumberOfThreads = GetValue<int>(var, _T("NumberOfThreads"), sysinfo.dwNumberOfProcessors * 2);
	s_nMaxUser = GetValue<int>(var, _T("MaxUser"), 5000);
	s_bAutoStart = GetValue<int>(var, _T("AutoStart"), 0);
	s_nMaxUpdateSize = GetValue<int>(var, _T("MaxUpdateSize"), 1000) * 1024 * 1024;
	s_strServer = GetValue<LPCTSTR>(var, _T("Server"), _T(""));
	s_nTimeStamp = GetTimeStamp();
	str = GetValue<LPCTSTR>(var, _T("Title"), _T(""));
	if( str[0])
	{
		TCHAR buf[32];
		_stprintf( buf, _T("%s on port %d"), str, s_nPort);
		SetWindowText( XIOScreen::s_hWnd, buf);
	}

	lisp::var addrs = var.get(_T("Address")).cdr();
	for ( ; !addrs.null(); ) {
		LPCTSTR szAddr = addrs.pop();
		ULONG nAddr = inet_addr( T_A(szAddr));
		if( nAddr != INADDR_NONE) 
			s_vector.push_back( nAddr);
		else
			LOG_ERR( _T("invalid address %s"), szAddr);
	}
	lisp::var channels = var.get(_T("Channel")).cdr();
	for ( ; !channels.null(); ) {
		DWORD nChannel = channels.pop();
		if (nChannel < UPDATE_CHANNEL_SIZE) {
			s_vbChannel[nChannel] = TRUE;
		}
	}

	var.destroy();

	return TRUE;
}

void CEchoConfig::Close()
{
}

ULONG CEchoConfig::GetAddress()
{
	if( s_vector.size() == 0)
		return 0;
	long nIndex = InterlockedIncrement( &s_index) % (int) s_vector.size();
	return s_vector[nIndex];
}
