#include "pch.h"
#include <windows.h>
#include <tchar.h>
#include "IOScreen.h"

HWND XIOScreen::s_hWnd;
XIOScreen *XIOScreen::s_pScreen;

void XIOScreen::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(s_hWnd, &ps);

	Lock();
	TEXTMETRIC metric;
	GetTextMetrics(hdc, &metric);
	RECT rect;
	GetClientRect(s_hWnd, &rect);

	int y = 0;
	LPTSTR pBuffer;
	int nLine = rect.bottom / metric.tmHeight;
	if (m_nLine > nLine)
	{
		pBuffer = m_pBuffer + (m_nLine - nLine) * m_nPitch;
	}
	else
	{
		pBuffer = m_pBuffer;
		nLine = m_nLine;
	}

	for ( ; --nLine >= 0; )
	{
		SetTextColor(hdc, *(COLORREF *)pBuffer);
		TextOut(hdc, 0, y, (LPTSTR)((COLORREF *)pBuffer + 1), (int) _tcslen((LPTSTR)((COLORREF *)pBuffer + 1)));
		pBuffer += m_nPitch;
		y += metric.tmHeight;
	}
	Unlock();

	EndPaint(s_hWnd, &ps);
}


void XIOScreen::AddString(COLORREF color, LPCTSTR string)
{
	int nLine;
	if (m_nLine == m_nHeight)
	{
		nLine = m_nLine - 1;
		memmove(m_pBuffer, m_pBuffer + m_nPitch, sizeof(TCHAR) * nLine * m_nPitch);
	}
	else
	{
		nLine = m_nLine++;
	}

	COLORREF *pBuffer = (COLORREF *)(m_pBuffer + nLine * m_nPitch);
	*pBuffer++ = color;
	_stprintf_s((LPTSTR)pBuffer, m_nWidth, string);
}

void XIOScreen::Empty()
{
	m_nLine = 0;
}


void XIOScreen::Open(int nWidth, int nHeight)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nPitch = (nWidth + sizeof(COLORREF) + 1 + 3) & ~3;
	m_nLine = 0;
	m_pBuffer = new TCHAR[m_nPitch * nHeight];
	memset(m_pBuffer, 0, sizeof(TCHAR) * m_nPitch * nHeight);
}

void XIOScreen::Add(COLORREF color, LPCTSTR format, ...)
{
	va_list va;
	va_start(va, format);

	int nLine;
	if (m_nLine == m_nHeight)
	{
		nLine = m_nLine - 1;
		memmove(m_pBuffer, m_pBuffer + m_nPitch, sizeof(TCHAR) * nLine * m_nPitch);
	}
	else
	{
		nLine = m_nLine++;
	}

	COLORREF *pBuffer = (COLORREF *)(m_pBuffer + nLine * m_nPitch);
	*pBuffer++ = color;
	_vstprintf_s((LPTSTR)pBuffer, m_nWidth, format, va);

	va_end(va);
}

XIOScreen::XIOScreen()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_nPitch = 0;
	m_nLine = 0;
	m_pBuffer = NULL;
}

XIOScreen::~XIOScreen()
{
	delete [] m_pBuffer;
}
