#pragma once

#include "IOLIb.h"
#include <vector>

class CEchoConfig  
{

public:
	static BOOL	Open();
	static void Close();
	typedef std::vector<ULONG> AddressVector;
	static AddressVector s_vector;
	static long	s_index;

	static tstring s_strMailFrom;
	static tstring s_strMailTo;
	static int s_nPort;
	static ULONG GetAddress();
	static BOOL	s_bAutoStart;
	static int s_nNumberOfThreads;
	static INT64 s_nMaxUpdateSize;
	static int s_nMaxUser;
	static time_t s_nTimeStamp;
	static tstring s_strServer;
	static tstring s_strMailServer;
	static int	s_nMailBindPort;
	static int	s_nBindPort;
	static int  s_nConnectPort;
};
