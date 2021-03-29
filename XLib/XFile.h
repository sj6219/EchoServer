#pragma once


class XFile
{
public:
	enum	SeekPosition	{
		Begin	=	0,	
		Current	=	1,	
		End		=	2,	
	};

	XFile() noexcept;
	XFile(void *pFile, size_t size) noexcept;
	~XFile() noexcept;

	void Open(void *pVoid, size_t size) noexcept;
	size_t Read(void *lpBuf, size_t uiCount) noexcept;
	void Close() noexcept;
	size_t Seek(size_t lOffset, SeekPosition nFrom) noexcept;
	size_t	SeekToEnd( void) noexcept {	return	Seek( 0L, XFile::End);	}
	void	SeekToBegin( void) noexcept {	Seek( 0L, XFile::Begin);	}
	size_t	GetLength( void)	const noexcept { return (size_t) (m_pViewEnd - m_pViewBegin);	}
	size_t	GetPosition( void) const noexcept { return (size_t) (m_pViewCurrent - m_pViewBegin); }

	char	*m_pViewCurrent;
	char	*m_pViewBegin;
	char	*m_pViewEnd;
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

