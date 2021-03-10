#pragma once
#include "Lisp.h"
#include <functional>

class XFile;


class XParser
{
public:
	enum FLAG
	{ F_NUMBER, F_INTEGER };

	enum {
		MAX_SYMBOL = 256,
	};

	typedef	DWORD_PTR (*PARSE_CALLBACK)(lisp::var var, void *param);
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
	void	ResetDepth() { m_nDepth = 0; }

	// Function that uses alloca() instead of malloc()
	DWORD_PTR	ParseList(PARSE_CALLBACK callback, void *param);
	BOOL	ParseFile(PARSE_CALLBACK callback, void *param);

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


