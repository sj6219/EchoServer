#pragma once


class XFile
{
public:
	enum	SeekPosition	{
		Begin	=	0,	
		Current	=	1,	
		End		=	2,	
	};
	char* m_current;
	char* m_begin;
	char* m_end;

	XFile() noexcept;
	XFile(void *pFile, size_t size) noexcept;
	~XFile() noexcept;
	char* begin() noexcept { return m_begin; }
	char* end() noexcept { return m_end; }
	char* current() noexcept { return m_current; }

	void Open(void *pVoid, size_t size) noexcept;
	size_t Read(void *lpBuf, size_t uiCount) noexcept;
	void Close() noexcept;
	size_t Seek(size_t lOffset, SeekPosition nFrom) noexcept;
	size_t	SeekToEnd( void) noexcept {	return	Seek( 0L, XFile::End);	}
	void	SeekToBegin( void) noexcept {	Seek( 0L, XFile::Begin);	}
	size_t	GetLength( void)	const noexcept { return (size_t) (m_end - m_begin);	}
	size_t	GetPosition( void) const noexcept { return (size_t) (m_current - m_begin); }

};

class XFileEx : public XFile
{
public:
	HANDLE	m_hFile;
	HANDLE	m_hMapping;

	XFileEx() noexcept;
	~XFileEx() noexcept;

	BOOL	Open(LPCTSTR szPath) noexcept;
	void	Close() noexcept;
	DWORD	Touch() noexcept;
};

