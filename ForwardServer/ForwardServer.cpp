
//

#include "Stdafx.h"
//#include "IOLib.h"
#include "IOException.h"
#include "IOScreen.h"
#include "Utility.h"
#include <commctrl.h>
#include "ForwardServer.h"
#include "Resource.h"
#include "Status.h"
#include "ForwardConfig.h"
#include "Server.h"
//#include <commctrl.h>
#include <shellapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Comctl32.lib")
#include "eventlog.h"

/*
mc eventlog.mc

reg add HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /v CategoryCount /t REG_DWORD /d 0 /f
reg add HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /v TypesSupported /t REG_DWORD /d 7 /f
for /F %i in ("..\Release\ForwardServer.exe") do reg add HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /v CategoryMessageFile /t REG_SZ /d %~fi /f
for /F %i in ("..\Release\ForwardServer.exe") do reg add HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /v EventMessageFile /t REG_SZ /d %~fi /f
for /F %i in ("..\Release\ForwardServer.exe") do reg add HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /v ParameterMessageFile /t REG_SZ /d %~fi /f

reg delete HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\ForwardServer /f
*/

#define MAX_LOADSTRING 100


HINSTANCE g_hInst;								
TCHAR szTitle[MAX_LOADSTRING];					
TCHAR szWindowClass[MAX_LOADSTRING];			


ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
static HANDLE g_hStartThread;
static BOOL	g_bStarted;


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	{
		HANDLE event_log = RegisterEventSource(NULL, _T("ForwardServer"));
		LPCTSTR message = _T("Start up");
		ReportEvent(event_log, EVENTLOG_INFORMATION_TYPE, 0, MSG_GENERAL_COMMAND, NULL, 1, 0, &message, NULL);
		DeregisterEventSource(event_log);
	}

	MSG msg;
	HACCEL hAccelTable;

	
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ECHOSERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_ECHOSERVER);

	
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	XIOException::Disable();
	if( g_bStarted)
	{
		CStatus::Stop();
		CServer::Stop();
	}
	XIOSocket::CloseIOThread();
	CForwardConfig::Close();

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDC_ECHOSERVER);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_ECHOSERVER;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

	INITCOMMONCONTROLSEX theInit;
	theInit.dwSize = sizeof(theInit);
	theInit.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&theInit);

	HWND hWnd;

   g_hInst = hInstance; 

   
   XIOMemory::Touch();
	XIOLog::s_screen.Open( 200, 50);
	CStatus::s_screen.Open( 80, 20);
	XIOScreen::s_pScreen = &XIOLog::s_screen;

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	XIOScreen::s_hWnd = hWnd;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	CreateMutex( NULL, FALSE, String(_T("Global\\")).append(GetUniqueName()).c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(hWnd, _T("Another Server is running"), _T(""), MB_ICONERROR);
		return 0;
	}

	CForwardConfig::Open();
	XIOException::Open( CForwardConfig::s_strMailServer.c_str(), CForwardConfig::s_strMailFrom.c_str(), CForwardConfig::s_strMailTo.c_str(), CForwardConfig::s_nMailBindPort);


	WSADATA wsaData;
	int err = WSAStartup(0x0202, &wsaData);
	if( err)
	{
		LOG_ERR( _T("WSAStartup error 0x%x"), err);
		return 0;
	}
	
	XIOSocket::CreateIOThread( CForwardConfig::s_nNumberOfThreads);

	if (CForwardConfig::s_bAutoStart)
		SendMessage(hWnd, WM_COMMAND, IDM_START, 0);

	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);

	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.hWnd = hWnd;
	nid.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
	nid.uCallbackMessage = WM_USER + 1;

	//LoadIconMetric(hInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &(nid.hIcon));
	Shell_NotifyIcon(NIM_ADD, &nid);

	return TRUE;
}

void TestException()
{
	ELOG( _T("Exception Test"));
	XIOException::Enable();
	EASSERT(0);
	XIOException::Enable();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				case IDM_START:
					if( !g_bStarted)
					{
						CServer::Start();
						CStatus::Start();
						struct tm* tm = localtime( &CForwardConfig::s_nTimeStamp);
						g_bStarted = TRUE;
						SetTimer(hWnd, 1, 3000, NULL);
					}
					else
					{
						MessageBox( hWnd, _T("Already Started"), NULL, 0);
					}
					break;
				case IDM_STATUS:
//					KillTimer( hWnd, 1);
					XIOScreen::s_pScreen = &CStatus::s_screen;
					CStatus::Update();
//					SetTimer( hWnd, 1, 3000, NULL);
					break;
				case IDM_LOG:
//					KillTimer( hWnd, 1);
					XIOScreen::s_pScreen = &XIOLog::s_screen;
					InvalidateRect( hWnd, NULL, TRUE);
					break;
				case IDM_EXCEPTION:
//					KillTimer( hWnd, 1);
					XIOScreen::s_pScreen = &XIOLog::s_screen;
					LOG_INFO( _T("Exception test started"));
					UpdateWindow( hWnd);
					TestException();
					LOG_INFO( _T("Exception test completed"));
					break;
				case IDM_STACK_DUMP:
//					KillTimer( hWnd, 1);
					XIOScreen::s_pScreen = &XIOLog::s_screen;
					LOG_INFO( _T("Stack dump started"));
					UpdateWindow( hWnd);
					XIOSocket::DumpStack();
					LOG_INFO( _T("Stack dump completed"));
					break;
				case IDM_LOG_FLUSH:
					XIOLog::Flush();
					break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_TIMER:
			CStatus::Update();
			return 0;
		case WM_PAINT:
			XIOScreen::s_pScreen->OnPaint();
			break;
		case WM_DESTROY:
		{
			NOTIFYICONDATA nid;
			memset(&nid, 0, sizeof(NOTIFYICONDATA));
			nid.cbSize = sizeof(NOTIFYICONDATA);

			nid.hWnd = hWnd;

			//LoadIconMetric(hInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &(nid.hIcon));
			Shell_NotifyIcon(NIM_DELETE, &nid);

		}
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED) {
				ShowWindow(hWnd, SW_HIDE);
			}
			break;
		case WM_USER + 1:
			switch (lParam) {
			case WM_LBUTTONDOWN:
			{
				ShowWindow(hWnd, SW_SHOW);
				ShowWindow(hWnd, SW_RESTORE);
			}
			break;
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
	return 0;
}


LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
