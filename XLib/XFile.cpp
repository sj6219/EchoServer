#include "pch.h"
#include "XFile.h"
#include "Utility.h"

XFile::XFile() noexcept
{
	m_begin = 0;
	m_current = 0;
	m_end = 0;
}

XFile::XFile(void *pFile, size_t size) noexcept
{
	m_current = 0;
	m_end = 0;

	Open(pFile, size);
}

XFile::~XFile() noexcept
{
}

void	XFile::Open(void *pVoid, size_t size) noexcept
{
	m_begin = (char *) pVoid;
	m_current = m_begin;
	m_end = m_begin + size;
}

size_t	XFile::Read(void *lpBuf, size_t uiCount) noexcept
{
	uiCount =  std::min<>(uiCount, (size_t)(m_end - m_current));
	memcpy(lpBuf, m_current, uiCount);
	m_current += uiCount;
	return uiCount;
}

void	XFile::Close() noexcept
{
	m_current = 0;
	m_end = 0;
}

size_t	XFile::Seek(size_t lOffset, SeekPosition nFrom) noexcept
{
	if (nFrom == Begin) 
		m_current = m_begin + lOffset;
	else if (nFrom == Current)
		m_current += lOffset;
	else if (nFrom == End)
		m_current = m_end + lOffset;
	return (long)(m_current - m_begin);
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
		UnmapViewOfFile(m_begin);
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
		for (char *ptr = m_begin; ptr < m_end; ptr += sys_info.dwPageSize) {
			checksum += *(DWORD *) ptr;
		}
	}
	return checksum;
}


