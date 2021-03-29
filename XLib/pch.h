// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#ifndef	_DEBUG
#define _HAS_EXCEPTIONS 0
#endif
#include "framework.h"
#include <windows.h>
#include <tchar.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new NEW
#else
#define NEW new
#endif

//#ifdef _DEBUG
//#ifdef _UNICODE
//#define _RPT(...)  _RPT_BASE_W(_CRT_WARN, NULL, 0, NULL,  __VA_ARGS__)
//#else
//#define _RPT(...)   _RPT_BASE(_CRT_WARN, NULL, 0, NULL,  __VA_ARGS__)
//#endif
//#else
//#define _RPT(...)
//#endif
#endif //PCH_H
