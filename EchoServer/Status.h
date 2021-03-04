#pragma once

#include "IOLib.h"

class CStatus  
{
public:
	static XIOScreen s_screen;
	static void Update();


	static bool Start();
	static void Stop();
};

