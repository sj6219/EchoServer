#include "pch.h"
#include "XFile.h"
#include "Utility.h"

XFile::XFile() noexcept
{
	m_pViewBegin = 0;
	m_pViewCurrent = 0;
	m_pViewEnd = 0;
}

XFile::XFile(void *pFile, size_t size) noexcept
{
	m_pViewCurrent = 0;
	m_pViewEnd = 0;

	Open(pFile, size);
}

XFile::~XFile() noexcept
{
}

void	XFile::Open(void *pVoid, size_t size) noexcept
{
	m_pViewBegin = (char *) pVoid;
	m_pViewCurrent = m_pViewBegin;
	m_pViewEnd = m_pViewBegin + size;
}

size_t	XFile::Read(void *lpBuf, size_t uiCount) noexcept
{
	uiCount =  std::min<>(uiCount, (size_t)(m_pViewEnd - m_pViewCurrent));
	memcpy(lpBuf, m_pViewCurrent, uiCount);
	m_pViewCurrent += uiCount;
	return uiCount;
}

void	XFile::Close() noexcept
{
	m_pViewCurrent = 0;
	m_pViewEnd = 0;
}

size_t	XFile::Seek(size_t lOffset, SeekPosition nFrom) noexcept
{
	if (nFrom == Begin) 
		m_pViewCurrent = m_pViewBegin + lOffset;
	else if (nFrom == Current)
		m_pViewCurrent += lOffset;
	else if (nFrom == End)
		m_pViewCurrent = m_pViewEnd + lOffset;
	return (long)(m_pViewCurrent - m_pViewBegin);
}

XFileEx::XFileEx() noexcept
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapping = 0;
}

XFileEx::~XFileEx() noexcept
{
	Close();
}

BOOL	XFileEx::Open(LPCTSTR szPath) noexcept
{
	_ASSERT(m_hFile == INVALID_HANDLE_VALUE);
	m_hFile = ::CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if( m_hFile == INVALID_HANDLE_VALUE) {
		_ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND);
		//LOG(_T("XFileEx::Open(%s) failed %d\n"), szPath, GetLastError());
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

void	XFileEx::Close() noexcept
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

DWORD	XFileEx::Touch() noexcept
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


