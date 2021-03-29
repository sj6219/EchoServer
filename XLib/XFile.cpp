#include "pch.h"
#include "XFile.h"
#include "Utility.h"

XFile::XFile()
{
	m_pViewBegin = 0;
	m_pView = 0;
	m_pViewEnd = 0;
}

XFile::XFile(void *pFile, DWORD size)
{
	m_pView = 0;
	m_pViewEnd = 0;

	Open(pFile, size);
}

XFile::~XFile()
{
}

void	XFile::Open(void *pVoid, DWORD size)
{
	m_pViewBegin = (char *) pVoid;
	m_pView = m_pViewBegin;
	m_pViewEnd = m_pViewBegin + size;
}

UINT	XFile::Read(void *lpBuf, UINT uiCount)
{
	uiCount =  std::min<UINT>(uiCount, (UINT)(m_pViewEnd - m_pView));
	memcpy(lpBuf, m_pView, uiCount);
	m_pView += uiCount;
	return uiCount;
}

void	XFile::Close()
{
	m_pView = 0;
	m_pViewEnd = 0;
}

long	XFile::Seek(long lOffset, UINT nFrom)
{
	if (nFrom == begin) 
		m_pView = m_pViewBegin + lOffset;
	else if (nFrom == current)
		m_pView += lOffset;
	else if (nFrom == end)
		m_pView = m_pViewEnd + lOffset;
	return (long)(m_pView - m_pViewBegin);
}

XFileEx::XFileEx()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapping = 0;
}

XFileEx::~XFileEx()
{
	Close();
}

BOOL	XFileEx::Open(LPCTSTR szPath) 
{
	_ASSERT(m_hFile == INVALID_HANDLE_VALUE);
	m_hFile = ::CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if( m_hFile == INVALID_HANDLE_VALUE) {
		_ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND);
		//TRACE(_T("XFileEx::Open(%s) failed %d\n"), szPath, GetLastError());
		return FALSE;
	}
	m_hMapping = 0;
	int nSize = GetFileSize(m_hFile, 0);

	m_hMapping = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, 0);
	if (m_hMapping == 0) {
		//CloseHandle(m_hFile);
		//m_hFile = INVALID_HANDLE_VALUE;
		//return FALSE;
		XFile::Open(0, 0);
		return TRUE;
	}
	LPVOID pMapView = MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0);
	if (pMapView == 0)	{
		CloseHandle(m_hMapping);
		CloseHandle(m_hFile);
		m_hMapping = 0;
		m_hFile = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	XFile::Open(pMapView, nSize);
	return TRUE;
}



void	XFileEx::Close()
{
	if (m_hMapping) {
		UnmapViewOfFile(m_pViewBegin);
		CloseHandle(m_hMapping);
		CloseHandle(m_hFile);
		m_hMapping = 0;
		m_hFile = INVALID_HANDLE_VALUE;
	}
	else if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

DWORD	XFileEx::Touch()
{
	DWORD checksum = 0;
	if (m_hMapping) {
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		for (char *ptr = m_pViewBegin; ptr < m_pViewEnd; ptr += sys_info.dwPageSize) {
			checksum += *(DWORD *) ptr;
		}
	}
	return checksum;
}


