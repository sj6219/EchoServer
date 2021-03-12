#pragma once


class XFile
{
public:
	enum	SeekPosition	{
		begin	=	0x0,	//	������ ���ο���.
		current	=	0x1,	//	���Ͽ� �ִ� ������ ��ġ����.
		end		=	0x2		//	������ ������.
	};

	XFile();
	XFile(void *pFile, DWORD size);
	~XFile();

	void Open(void *pVoid, DWORD size);
	UINT Read(void *lpBuf, UINT uiCount);
	void Close();
	long Seek(long lOffset, UINT nFrom);
	DWORD	SeekToEnd( void)	{	return	Seek( 0L, XFile::end);	}
	void	SeekToBegin( void)	{	Seek( 0L, XFile::begin);	}
	DWORD	GetLength( void)	const { return (DWORD)(m_pViewEnd - m_pViewBegin);	}
	DWORD	GetPosition( void)	const { return (DWORD)(m_pView - m_pViewBegin); }

	char	*m_pView;
	char	*m_pViewBegin;
	char	*m_pViewEnd;
};

class XFileEx : public XFile
{
public:
	HANDLE	m_hFile;
	HANDLE	m_hMapping;

	XFileEx();
	~XFileEx();

	BOOL	Open(LPCTSTR szPath);
	void	Close();
	DWORD	Touch();
};

;