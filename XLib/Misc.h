#pragma once

#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <malloc.h>
#include "Utility.h"




/*




#ifdef	UNICODE
util::wstring AtoT(LPCSTR lpa);
util::string TtoA(LPCWSTR lpw);
#else
#define AtoT(lpa) lpa
#define TtoA(lpw) lpw
#endif

util::tstring FormatString(LPCTSTR format, ...);
util::tstring GetUniqueName();
LPCTSTR	GetNamePart(LPCTSTR szPath);
__forceinline LPTSTR	GetNamePart(LPTSTR szPath) { return (LPTSTR) GetNamePart((LPCTSTR) szPath); }




template <typename T> void	minimize(T &left, const T& right) { if (right < left) left = right; }
template <typename T> void	maximize(T &left, const T& right) { if (right > left) left = right; }
template <typename T> void	minmaximize(T &left, const T& min, const T& max) 
{ 
	if (left > max) 
		left = max;
	else if (left < min)
		left = min;
}

namespace util
{	
	extern const DWORD m_vCRC32Table[256];
}

inline DWORD	UpdateCRC(DWORD nCRC, DWORD nByte) { return util::m_vCRC32Table[(BYTE)(nCRC ^ nByte)] ^ (nCRC >> 8); }
DWORD	UpdateCRC(DWORD crc, void *buf, int len);
DWORD __stdcall EncryptCRC(DWORD crc, void* buf, int len);
DWORD __stdcall DecryptCRC(DWORD crc, void* buf, int len);
DWORD __stdcall EncodeCRC(DWORD crc, void* buf, int len);
DWORD __stdcall DecodeCRC(DWORD crc, void* buf, int len);
time_t GetTimeStamp();

#define ITERATE(T, it, container) for (T::iterator it = (container).begin(); it != (container).end(); it++)

#ifdef	_DEBUG
#define OBFUSCATE
#else

#define __PASTE__(a, b)	a ## b
#define _PASTE_(a, b)  __PASTE__(a, b)

#define OBFUSCATE \
	__asm mov eax, __LINE__ * 0x635186f1		\
	__asm cmp eax, __LINE__ * 0x9cb16d48		\
	__asm je _PASTE_(junk, __LINE__)			\
	__asm mov eax, _PASTE_(after, __LINE__)		\
	__asm jmp eax								\
	__asm _PASTE_(junk, __LINE__):				\
	__asm _emit(0xd8 + __LINE__ % 8)			\
	__asm _PASTE_(after, __LINE__):
#endif


int	ExpandStringV(char * buffer, size_t count, const char * format, const char ** argptr);
int	ExpandStringV(wchar_t * buffer, size_t count, const wchar_t * format, const wchar_t **argptr);
int ExpandString(char * buffer, size_t count, const char *format, ...);
int ExpandString(wchar_t * buffer, size_t count, const wchar_t * format, ...);


int _stdcall mbstricmp( const char* s1, const char* s2);

class iless
{
public:
#ifdef	UNICODE
	bool operator()(const std::wstring& a, const std::wstring& b) const {
		return _tcsicmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const std::wstring& b) const {
		return _tcsicmp(a, b.c_str()) < 0;
	}
	bool operator()(const std::wstring& a, LPCTSTR b) const {
		return _tcsicmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return _tcsicmp(a, b) < 0;
	}
#else
	bool operator()(const std::string& a, const std::string& b) const {
		return mbstricmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const std::string& b) const {
		return mbstricmp(a, b.c_str()) < 0;
	}
	bool operator()(const std::string& a, LPCTSTR b) const {
		return mbstricmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return mbstricmp(a, b) < 0;
	}
#endif
};

class StrCmp
{
public:
#ifdef	UNICODE
	bool operator()(const std::wstring& a, const std::wstring& b) const {
		return _tcscmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const std::wstring& b) const {
		return _tcscmp(a, b.c_str()) < 0;
	}
	bool operator()(const std::wstring& a, LPCTSTR b) const {
		return _tcscmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return _tcscmp(a, b) < 0;
	}
#else
	bool operator()(const std::string& a, const std::string& b) const {
		return _tcscmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const std::string& b) const {
		return _tcscmp(a, b.c_str()) < 0;
	}
	bool operator()(const std::string& a, LPCTSTR b) const {
		return _tcscmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return _tcscmp(a, b) < 0;
	}
#endif
};

char*	__stdcall WritePacketV(char *packet, va_list va);
char*	WritePacket(char *packet, const char *, ...);
LPCPACKET	__stdcall ReadPacketV(LPCPACKET packet, va_list va);
LPCPACKET	ReadPacket(LPCPACKET packet, const char *, ...);
LPCPACKET	__stdcall ScanPacketV(LPCPACKET packet, LPCPACKET end, va_list va);
LPCPACKET	ScanPacket(LPCPACKET packet, LPCPACKET end, const char *, ...);

LPCPACKET	__stdcall GetString(LPCPACKET packet, LPTSTR str, int size);
LPCPACKET	__stdcall ScanString(LPCPACKET packet, LPCPACKET end, LPTSTR str, int size);
char*	__stdcall PutString(char *packet, LPCTSTR str);

char*	__stdcall PutBuffer(char *packet, LPCTSTR str, int size);




class XFile
{
public:
	enum	SeekPosition	{
		begin	=	0x0,	
		current	=	0x1,	
		end		=	0x2		
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
	HANDLE	m_hMapping;
	HANDLE	m_hFile;

	XFileEx();
	~XFileEx();
	BOOL	Open(LPCTSTR szPath);
	void	Close();
	DWORD	Touch();

};


class XParser
{
public:
	enum FLAG
	{ F_NUMBER, F_INTEGER };

	enum {
		MAX_SYMBOL = 256,
	};

	typedef	DWORD_PTR (*PARSE_CALLBACK)(DWORD_PTR dwParam, lisp::var var);
	enum TOKEN 
	{ T_END, T_STRING, T_INTEGER, T_FLOAT, T_OPEN, T_CLOSE, T_ERROR };


protected:
	char	*m_szSymbolA;
	char	m_szMemoryA[MAX_SYMBOL];
#ifdef	UNICODE
	int		m_nSymbolW;
	TCHAR	*m_szSymbolW;
	TCHAR	m_szMemoryW[MAX_SYMBOL];
#endif
	void	AllocString();
	DWORD	m_dwFlags;
	XFile	*m_pFile;
	int		m_nLine;
	int		m_nDepth;
	int		m_nPosition;
	int		m_nReserve;
	int		m_nSymbolA;


public:
	XParser(void);
	~XParser(void);
#ifdef	UNICODE
	virtual TOKEN	GetToken();
	LPCTSTR GetString() { return m_szSymbolW; }
	virtual int		GetInteger() { return strtoul(m_szSymbolA, 0, 0); }
	virtual float	GetFloat() { return (float) atof(m_szSymbolA); }
#else
	TOKEN	GetToken();
	LPCTSTR GetString() { return m_szSymbolA; }
	int		GetInteger() { return strtoul(m_szSymbolA, 0, 0); }
	float	GetFloat() { return (float) atof(m_szSymbolA); }
#endif
	void	Open(XFile *pFile);
	lisp::var Load(LPCTSTR szPath);
	lisp::var Load(XFile *pFile);
	int	GetLine() { return m_nLine; }
	int	GetDepth() { return m_nDepth; }

	DWORD_PTR	ParseList(PARSE_CALLBACK Callback, DWORD_PTR dwParam);
	void	ResetDepth() { m_nDepth = 0; }

protected:
	lisp::var OnLoad();

	__forceinline void InitChar();
	__forceinline void PutChar(int ch);
	__forceinline void EndChar();
	__forceinline void MakeString();
	__forceinline int GetChar();
	__forceinline int PeekChar();
	__forceinline void UndoChar();

};
#ifdef	UNICODE
class XParserW : public XParser
{
public:
	virtual TOKEN	GetToken();
	virtual int		GetInteger() { return _tcstol(m_szSymbolW, 0, 0); }
	virtual float	GetFloat() { return (float) _tstof(m_szSymbolW); }

protected:

	__forceinline void EndChar();
	__forceinline void MakeString();
	__forceinline int isdigit(int);
	__forceinline int isalnum(int);
	__forceinline int isxdigit(int);
	__forceinline int GetChar();
	__forceinline int PeekChar();
	__forceinline void UndoChar();
	__forceinline void InitChar();
	__forceinline void PutChar(int ch);
};
#endif
*/
