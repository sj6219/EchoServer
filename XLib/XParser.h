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

	//typedef	DWORD_PTR (*PARSE_CALLBACK)(DWORD_PTR dwParam, lisp::var var);
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

	template<typename func> DWORD_PTR	ParseList();
	template<typename func> BOOL	ParseFile();
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

template<typename Callback> DWORD_PTR	XParser::ParseList()
{
	TOKEN token;
	lisp::var top;
	lisp::var stack;

	for (; ; ) {
		token = GetToken();
		switch (token) {
		case T_END:
			if (m_nDepth != 0) {
				//_RPT(_T("Unmatched open parenthesis\n"));
				top = lisp::error;
				goto quit;
			}
			top = lisp::nreverse(top);
			_ASSERT(stack.null());
			goto quit;

		case T_CLOSE:
			top = lisp::nreverse(top);
			if (stack.consp()) {
				lisp::_cons* pObject = (lisp::_cons*)stack.m_pObject;
				stack = pObject->m_car;
				pObject->m_car = top;
				top = pObject;
			}
			else if (m_nDepth < 0) {
				//_RPT(_T("Unmatched close parenthesis %d\n"), GetLine());
				top = lisp::error;
				m_nDepth = 0;
				goto quit;
			}
			else
				goto quit;
			break;
		case T_OPEN:
			stack = new(_alloca(sizeof(lisp::_cons))) lisp::_cons(stack, top);
			top = lisp::nil;
			break;
		case T_STRING:
		{
#ifdef	UNICODE
			size_t size = (m_nSymbolW + 1) * sizeof(TCHAR);
#else
			size_t size = (m_nSymbolA + 1) * sizeof(TCHAR);
#endif
			std::pair<lisp::_cons, lisp::_string>* p = (std::pair<lisp::_cons, lisp::_string> *)
				_alloca(sizeof(std::pair<lisp::_cons, lisp::_string>) + size);
			new (&p->second) lisp::_string((LPCTSTR)memcpy((char*)(p + 1), GetString(), size));
			top = new(&p->first) lisp::_cons(&p->second, top);
		}
		break;
		case T_INTEGER:
		{
			std::pair<lisp::_cons, lisp::_integer>* p = (std::pair<lisp::_cons, lisp::_integer> *)
				_alloca(sizeof(std::pair<lisp::_cons, lisp::_integer>));
			new (&p->second) lisp::_integer(GetInteger());
			top = new(&p->first) lisp::_cons(&p->second, top);
		}
		break;
		case T_FLOAT:
		{
			std::pair<lisp::_cons, lisp::_float>* p = (std::pair<lisp::_cons, lisp::_float> *)
				_alloca(sizeof(std::pair<lisp::_cons, lisp::_float>));
			new(&p->second) lisp::_float(GetFloat());
			top = new(&p->first) lisp::_cons(&p->second, top);
		}
		break;
		default:
			top = lisp::error;
			goto quit;
		}
	}
quit:
	return (Callback)(top);
}

template<typename Callback> BOOL	XParser::ParseFile()
{
	TOKEN token;
	for (; ; ) {
		switch (token = GetToken()) {
		case T_END:
			return TRUE;
		case T_CLOSE:
			//_RPT(_T("Unmatched close parenthesis (%d)\n"), GetLine());
			if (!(Callback)(lisp::error))
				return FALSE;
			ResetDepth();
			break;
		case T_OPEN:
		{
			if (!ParseList<Callback>()) {
				//_RPT(_T("Invalid format (%d)"), GetLine());
				return	FALSE;
			}
		}
		break;
		case T_STRING:
		{
			//#ifdef	UNICODE
			//				size_t size = ( m_nSymbolW + 1) * sizeof( TCHAR);
			//#else
			//				size_t size = ( m_nSymbolA + 1) * sizeof( TCHAR);
			//#endif
			lisp::_string v = (GetString());
			if (!(Callback)(&v))
				return FALSE;
		}
		break;
		case T_INTEGER:
		{
			lisp::_integer v(GetInteger());
			if (!(Callback)(&v))
				return FALSE;
		}
		break;
		case T_FLOAT:
		{
			lisp::_float v(GetFloat());
			if (!(Callback)(&v))
				return FALSE;
		}
		break;
		default:
			//_RPT(_T("Invalid format (%d)\n"), GetLine());
			//_ASSERT(0);
			if (!(Callback)(lisp::error))
				return FALSE;
		}
	}
}




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


