#pragma once

#include "IOLIb.h"
#include "lisp.h"
#include <vector>

class CForwardConfig  
{
	class CForward
	{
	public:
		int m_port;
		String m_forward_server;
		int m_forward_port;
	};
	typedef std::vector<CForward> ForwardVector;
public:
	static BOOL	Open();
	static void Close();
	static BOOL Load(lisp::var);

	static ForwardVector s_vForwardList;
	static String s_strMailFrom;
	static String s_strMailTo;
	static BOOL	s_bAutoStart;
	static int s_nNumberOfThreads;
	static int s_nMaxUser;
	static time_t s_nTimeStamp;
	static String s_strMailServer;
	static int	s_nMailBindPort;
};
