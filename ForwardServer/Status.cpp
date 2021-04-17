#include "Stdafx.h"
//#include "IOLib.h"
#include "IOScreen.h"
#include <perflib.h>
#include <winperf.h>
#include "Status.h"
#include "Server.h"
#include "forwardcounter.h"

// ctrpp -o forwardcounter.h -rc forwardcounter.rc -ch forwardcountersymbol.h forward.man
// for /F %i in ("..\x64\Release") do lodctr /m:echo.man %~fi
// unlodctr  /m:forward.man
// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib\_V2Providers\{6b0b3135-4a43-4e48-84ec-f057457ac18e}

XIOScreen	CStatus::s_screen;

PPERF_COUNTERSET_INSTANCE g_instance;

void	CStatus::Update()
{
	{
		ULONG Status;

		Status = PerfSetULongCounterValue(ForwardUserModeCounters, g_instance, 1, XIOSocket::s_nRunningThread);
		_ASSERT(Status == ERROR_SUCCESS);
		//Status = PerfSetULongCounterValue(ForwardUserModeCounters, g_instance, 2, (ULONG) CServer::Size());
#ifndef _DEBUG
		PerfSetULongLongCounterValue(ForwardUserModeCounters, g_instance, 3, XIOMemory::GetTotalSize());
#endif

	}
	if (XIOScreen::s_pScreen == &CStatus::s_screen) {
		s_screen.Lock();
		s_screen.Empty();
		s_screen.Add(RGB(0, 0, 0), _T("Running Thread : %d"), XIOSocket::s_nRunningThread);
		//s_screen.Add(RGB(0, 0, 0), _T("Connection : %d"), CServer::Size());
#ifndef _DEBUG
		s_screen.Add(RGB(0, 0, 0), _T("Memory : %td"), XIOMemory::GetTotalSize());
#endif
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

	g_instance = PerfCreateInstance(ForwardUserModeCounters, &ForwardCounterSet1Guid, L"Instance_1", 0);
	if (g_instance == NULL) {
		return false;
	}
	return true;
}

void CStatus::Stop()
{
	CounterCleanup();
}