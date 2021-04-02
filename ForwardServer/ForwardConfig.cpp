#include "Stdafx.h"
#include "ForwardConfig.h"
//#include "IOLib.h"
#include "IOScreen.h"
#include "Utility.h"
#include "Lisp.h"
#include "XParser.h"
#include "XFile.h"

#define USE_PARSELIST

int	CForwardConfig::s_nMailBindPort;
String CForwardConfig::s_strMailFrom;
String CForwardConfig::s_strMailTo;
BOOL	CForwardConfig::s_bAutoStart;
int	CForwardConfig::s_nNumberOfThreads;
int	CForwardConfig::s_nMaxUser;
time_t CForwardConfig::s_nTimeStamp;
String CForwardConfig::s_strMailServer;
CForwardConfig::ForwardVector CForwardConfig::s_vForwardList;


template <typename T> T GetValue(lisp::var var, LPCTSTR name, T defaultValue)
{
	lisp::var value = var.get(name).nth(1);
	return value.null() ? defaultValue : (T) value;
}

BOOL CForwardConfig::Load(lisp::var var)
{
	if (var.errorp())
	{
		LOG_ERR(_T("can't open ForwardConfig.txt"));
		return FALSE;
	}
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	s_strMailServer = GetValue<LPCTSTR>(var, _T("MailServer"), _T(""));
	s_nMailBindPort = GetValue<int>(var, _T("MailBindPort"), 25);
	s_strMailFrom = GetValue<LPCTSTR>(var, _T("MailFrom"), _T(""));
	s_strMailTo = GetValue<LPCTSTR>(var, _T("MailTo"), _T(""));
	s_nNumberOfThreads = GetValue<int>(var, _T("NumberOfThreads"), sysinfo.dwNumberOfProcessors * 2);
	s_nMaxUser = GetValue<int>(var, _T("MaxUser"), 5000);
	s_bAutoStart = GetValue<int>(var, _T("AutoStart"), 0);
	s_nTimeStamp = GetTimeStamp();
	LPCTSTR str = GetValue<LPCTSTR>(var, _T("Title"), _T(""));
	if (str[0])
	{
		SetWindowText(XIOScreen::s_hWnd, str);
	}

	lisp::var list = var.get(_T("ForwardList")).cdr();
	for (; ; ) {
		if (list.null())
			break;
		lisp::var item = list.car();
		list = list.cdr();

		CForwardConfig::CForward forward;
		forward.m_port = (int)item.car();
		item = item.cdr();
		forward.m_forward_server = (LPCTSTR)item.car();
		item = item.cdr();
		forward.m_forward_port = (int)item.car();
		item = item.cdr();
		s_vForwardList.push_back(forward);
	}
	return TRUE;

}



BOOL CForwardConfig::Open()
{
	XParser parser;

#ifdef USE_PARSELIST
	XFileEx file;
	if (!file.Open(_T("ForwardConfig.txt"))) {
		LOG_ERR(_T("can't open ForwardConfig.txt"));
		return FALSE;
	}

	parser.Open(&file);
	auto func = [](lisp::var var, void* param) noexcept ->DWORD_PTR 
	{
		return CForwardConfig::Load(var);
	};
	return parser.ParseList(func, NULL);
#else
	lisp::var var = parser.Load(_T("ForwardConfig.txt"));
	auto ret = CForwardConfig::Load(var);
	var.destroy();
	return ret;
#endif

}

void CForwardConfig::Close()
{
}

