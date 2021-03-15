#pragma once

#include "IOLIb.h"
#include <vector>

class CForwardConfig  
{
	class CForward
	{
	public:
		int m_port;
		tstring m_forward_server;
		int m_forward_port;
	};
	typedef std::vector<CForward> ForwardVector;
public:
	static BOOL	Open();
	static void Close();

	static ForwardVector s_vForwardList;
	static tstring s_strMailFrom;
	static tstring s_strMailTo;
	static BOOL	s_bAutoStart;
	static int s_nNumberOfThreads;
	static int s_nMaxUser;
	static time_t s_nTimeStamp;
	static tstring s_strMailServer;
	static int	s_nMailBindPort;
};
