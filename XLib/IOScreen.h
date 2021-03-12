#pragma once
#include "IOLib.h"

class XIOScreen
{
public:

	void Lock() { m_lock.lock(); }
	void Unlock() { m_lock.unlock(); }
	void Empty();
	void OnPaint();
	void Open(int nWidth, int nHeight);
	void Add(COLORREF color, LPCTSTR format, ...);
	void AddString(COLORREF color, LPCTSTR string);
	XIOScreen();
	~XIOScreen();



	LPTSTR m_pBuffer;
	int m_nWidth;
	int m_nHeight;
	int m_nLine;
	int m_nPitch;
	XLock m_lock;

	static HWND s_hWnd;
	static XIOScreen *s_pScreen;
};

