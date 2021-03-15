#include "Stdafx.h"
//#include "IOLib.h"
#include "IOScreen.h"
#include <perflib.h>
#include <winperf.h>
#include "Status.h"
#include "Server.h"
#include "forwardcounter.h"

// ctrpp -o echocounter.h -rc echocounter.rc -ch echocountersymbol.h echo.man
// lodctr /m:echo.man C:\Users\sjpark\Documents\EchoServer\x64\Release
// unlodctr  /m:echo.man

XIOScreen	CStatus::s_screen;

PPERF_COUNTERSET_INSTANCE g_instance;

void	CStatus::Update()
{
	{
		ULONG Status;

		Status = PerfSetULongCounterValue(EchoUserModeCounters, g_instance, 1, XIOSocket::s_nRunningThread);
		_ASSERT(Status == ERROR_SUCCESS);
		Status = PerfSetULongCounterValue(EchoUserModeCounters, g_instance, 2, CServer::Size());

	}
	if (XIOScreen::s_pScreen == &CStatus::s_screen) {
		s_screen.Lock();
		s_screen.Empty();
		s_screen.Add(RGB(0, 0, 0), _T("Running Thread : %d"), XIOSocket::s_nRunningThread);
		s_screen.Add(RGB(0, 0, 0), _T("Connection : %d"), CServer::Size());
		s_screen.Unlock();
		InvalidateRect(XIOScreen::s_hWnd, NULL, TRUE);
	}

}

bool CStatus::Start()
{
	ULONG Status;

	Status = CounterInitialize(NULL, NULL, NULL, NULL);
	if (Status != ERROR_SUCCESS) {
		return false;
	}

	g_instance = PerfCreateInstance(EchoUserModeCounters, &EchoCounterSet1Guid, L"Instance_1", 0);
	if (g_instance == NULL) {
		return false;
	}
	return true;
}

void CStatus::Stop()
{
	CounterCleanup();
}