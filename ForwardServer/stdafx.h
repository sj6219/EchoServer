// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이 
// 들어 있는 포함 파일입니다.
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WIN32_WINNT 0x0600

//#define UNICODE
//#define _UNICODE
#ifndef	_DEBUG
#define _HAS_EXCEPTIONS 0
#endif

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new NEW
#else
#define NEW new
#endif

#include "IOMemory.h"
//#include "IOLib.h"

//#include <commctrl.h>
//#include <mswsock.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/utime.h>


#ifdef _DEBUG
#ifdef _UNICODE
#define _RPT(...)  _RPT_BASE_W(_CRT_WARN, NULL, 0, NULL,  __VA_ARGS__)
#else
#define _RPT(...)   _RPT_BASE(_CRT_WARN, NULL, 0, NULL,  __VA_ARGS__)
#endif
#else
#define _RPT(...)
#endif
