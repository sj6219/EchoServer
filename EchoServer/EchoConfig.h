#pragma once

#include "IOLIb.h"
#include <vector>

class CEchoConfig  
{

public:
	static BOOL	Open();
	static void Close();

	static tstring s_strMailFrom;
	static tstring s_strMailTo;
	static int s_nPort;
	static BOOL	s_bAutoStart;
	static int s_nNumberOfThreads;
	static int s_nMaxUser;
	static time_t s_nTimeStamp;
	static tstring s_strServer;
	static tstring s_strMailServer;
	static int	s_nMailBindPort;
};
	
