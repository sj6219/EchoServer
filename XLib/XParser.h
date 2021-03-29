#pragma once
#include "Lisp.h"
//#include <functional>

class XFile;


class XParser
{
public:
	enum FLAG
	{ F_NUMBER, F_INTEGER };

	enum {
		MAX_SYMBOL = 256,
	};

	typedef 	DWORD_PTR (*PARSE_CALLBACK)(lisp::var var, void *param) noexcept;
	//typedef std::function<DWORD_PTR(lisp::var)> PARSE_CALLBACK;
	enum TOKEN 
	{ T_END, T_STRING, T_INTEGER, T_FLOAT, T_OPEN, T_CLOSE, T_ERROR };

	static UINT s_nCodePage;
protected:
	char	*m_szSymbolA;
	char	m_szMemoryA[MAX_SYMBOL];
#ifdef	UNICODE
	int		m_nSymbolW;
	TCHAR	*m_szSymbolW;
	TCHAR	m_szMemoryW[MAX_SYMBOL];
#endif
	void	AllocString() noexcept;
	DWORD	m_dwFlags;
	XFile	*m_pFile;
	int		m_nLine;
	int		m_nDepth;
	int		m_nPosition;
	int		m_nReserve;
	int		m_nSymbolA;


public:
	XParser(void) noexcept;
	~XParser(void) noexcept;
#ifdef	UNICODE
	virtual TOKEN	GetToken() noexcept;
	LPCTSTR GetString() noexcept { return m_szSymbolW; }
	virtual int		GetInteger() noexcept { return strtoul(m_szSymbolA, 0, 0); }
	virtual float	GetFloat() noexcept { return (float) atof(m_szSymbolA); }
#else
	TOKEN	GetToken();
	LPCTSTR GetString() { return m_szSymbolA; }
	int		GetInteger() { return strtoul(m_szSymbolA, 0, 0); }
	float	GetFloat() { return (float) atof(m_szSymbolA); }
#endif
	void	Open(XFile *pFile) noexcept;
	lisp::var Load(LPCTSTR szPath) noexcept;
	lisp::var Load(XFile *pFile) noexcept;
	int	GetLine() noexcept { return m_nLine; }
	int	GetDepth() noexcept { return m_nDepth; }
	void	ResetDepth() noexcept { m_nDepth = 0; }

	// Function that uses alloca() instead of malloc()
	DWORD_PTR	ParseList(PARSE_CALLBACK callback, void *param) noexcept;
	BOOL	ParseFile(PARSE_CALLBACK callback, void *param) noexcept;

protected:
	lisp::var OnLoad() noexcept;

	__forceinline void InitChar() noexcept;
	__forceinline void PutChar(int ch) noexcept;
	__forceinline void EndChar() noexcept;
	__forceinline void MakeString() noexcept;
	__forceinline int GetChar() noexcept;
	__forceinline int PeekChar() noexcept;
	__forceinline void UndoChar() noexcept;

};

#ifdef	UNICODE
class XParserW : public XParser
{
public:
	virtual TOKEN	GetToken() noexcept;
	virtual int		GetInteger() noexcept { return _tcstol(m_szSymbolW, 0, 0); }
	virtual float	GetFloat() noexcept { return (float) _tstof(m_szSymbolW); }

protected:

	__forceinline void EndChar() noexcept;
	__forceinline void MakeString() noexcept;
	__forceinline int isdigit(int) noexcept;
	__forceinline int isalnum(int) noexcept;
	__forceinline int isxdigit(int) noexcept;
	__forceinline int GetChar() noexcept;
	__forceinline int PeekChar() noexcept;
	__forceinline void UndoChar() noexcept;
	__forceinline void InitChar() noexcept;
	__forceinline void PutChar(int ch) noexcept;
};
#endif


