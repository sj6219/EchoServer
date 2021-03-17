
//

#include "Stdafx.h"
//#include "IOLib.h"
#include "IOException.h"
#include "IOScreen.h"
#include "Utility.h"
#include <commctrl.h>
#include "EchoServer.h"
#include "Resource.h"
#include "Status.h"
#include "EchoConfig.h"
#include "Server.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Comctl32.lib")

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
	CEchoConfig::Close();

	return (int) msg.wParam;
}



//

//

//

//





//
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

//

//

//

//


//
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


	CreateMutex( NULL, FALSE, tstring(_T("Global\\")).append(GetUniqueName()).c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(hWnd, _T("Another Server is running"), _T(""), MB_ICONERROR);
		return 0;
	}

	CEchoConfig::Open();
	XIOException::Open( CEchoConfig::s_strMailServer.c_str(), CEchoConfig::s_strMailFrom.c_str(), CEchoConfig::s_strMailTo.c_str(), CEchoConfig::s_nMailBindPort);


	WSADATA wsaData;
	int err = WSAStartup(0x0202, &wsaData);
	if( err)
	{
		LOG_ERR( _T("WSAStartup error 0x%x"), err);
		return 0;
	}
	
	XIOSocket::CreateIOThread( CEchoConfig::s_nNumberOfThreads);

	if (CEchoConfig::s_bAutoStart)
		SendMessage(hWnd, WM_COMMAND, IDM_START, 0);

	return TRUE;
}

void TestException()
{
	ELOG( _T("Exception Test"));
	XIOException::Enable();
	EASSERT(0);
	XIOException::Enable();
}

//

//

//



//
//
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
						struct tm* tm = localtime( &CEchoConfig::s_nTimeStamp);
						LOG_NORMAL( _T("Server is ready on port %d (time stamp: %02d/%02d/%02d %02d:%02d:%02d)"), CEchoConfig::s_nPort, 
							tm->tm_year % 100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
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
			PostQuitMessage(0);
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
