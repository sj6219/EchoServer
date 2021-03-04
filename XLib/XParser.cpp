#include "pch.h"
#include <malloc.h>
#include "XParser.h"
#include "XFile.h"

#ifdef _DEBUG
#undef new
#endif

UINT XParser::s_nCodePage = CP_UTF8;
#ifdef	UNICODE

__forceinline int		XParserW::GetChar() 
{ 
	int ch = *(TCHAR *)m_pFile->m_pView; 
	m_pFile->m_pView += sizeof(TCHAR); 
	return ch; 
}
__forceinline int		XParserW::PeekChar() { return *(TCHAR *) m_pFile->m_pView; }
__forceinline void	XParserW::UndoChar() { m_pFile->m_pView -= sizeof(TCHAR); }
__forceinline void	XParserW::InitChar() { m_nSymbolW = 0; }
__forceinline void	XParserW::PutChar(int ch) 
{ 
	m_szSymbolW[m_nSymbolW++] = ch; 
	if (m_nSymbolW >= m_nReserve)
		AllocString();
}
__forceinline void	XParserW::EndChar() { m_szSymbolW[m_nSymbolW] = 0; }
__forceinline void	XParserW::MakeString() { m_szSymbolW[m_nSymbolW] = 0;}
__forceinline int	XParserW::isdigit(int ch) { return _istdigit(ch); }
__forceinline int	XParserW::isalnum(int ch) { return _istalnum(ch); }
__forceinline int	XParserW::isxdigit(int ch) { return _istxdigit(ch); }


XParser::TOKEN		XParserW::GetToken()
{
	int ch;

	{
st_start:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_END;
		switch (ch = GetChar()) {
		case 0 :
			UndoChar();
			return T_END;
		case ';':	// comment
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_END;
				switch(GetChar()) {
				case 0:
				case '\n':
					UndoChar();
					goto st_start;
				}
			}
		case '\n':
			m_nLine++;
			goto st_start;
		case '(':
			m_nDepth++;
			return T_OPEN;
		case ')':
			m_nDepth--;
			return T_CLOSE;
		case '"':
			InitChar();
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_ERROR;
				switch(ch = GetChar()) {
				case 0:
					UndoChar();
					return T_ERROR;
				case '"':
					if (m_pFile->m_pView == m_pFile->m_pViewEnd)
						goto return_string;
					if ((ch = PeekChar()) != '"') 
						goto return_string;
					else
						PutChar('"');
					break;
				case '\r':
				case '\n':
					UndoChar();
					return T_ERROR;
				default:
					PutChar(ch);
					break;
				}
			}
		case '\'':
			InitChar();
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_ERROR;
				switch(ch = GetChar()) {
				case 0:
					UndoChar();
					return T_ERROR;
				case '\'':
					if (m_pFile->m_pView == m_pFile->m_pViewEnd)
						goto return_string;
					if ((ch = PeekChar()) != '\'') 
						goto return_string;
					else
						PutChar('\'');
					break;
				case '\r':
				case '\n':
					UndoChar();
					return T_ERROR;
				default:
					PutChar(ch);
					break;
				}
			}
		case ' ':
		case '\t':
		case '\r':
#ifdef	UNICODE
		case 0xfeff:
#endif
			goto st_start;
		case '+':
		case '-':
			InitChar();
			PutChar(ch);
			goto st_sign;
		case '0':
			InitChar();
			PutChar(ch);
			goto st_zero;
		case '.':
			InitChar();
			PutChar(ch);
			goto st_dot;
		case '*':
		case '/':
		case '_':
			InitChar();
			PutChar(ch);
			goto st_string;
		default:
			if (isdigit(ch)) {
				InitChar();
				PutChar(ch);
				goto st_digit;
			}
			else if (isalnum(ch)) {
				InitChar();
				PutChar(ch);
				goto st_string;
			}
			return T_ERROR;
		}
st_string:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			goto return_string;
		switch(ch = GetChar()) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '.':
		case '_':
			PutChar(ch);
			goto st_string;
		default:
			if (isalnum(ch)) {
				PutChar(ch);
				goto st_string;
			}
			UndoChar();
			goto return_string;
		}
st_sign:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			goto return_string;
		switch (ch = GetChar()) {
		case '0':
			PutChar(ch);
			goto st_zero;
		case '.':
			PutChar(ch);
			goto st_dot;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			goto return_string;
		}
st_zero:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case 'x':
		case 'X':
			PutChar(ch);
			goto st_hexa;
		case '8':
		case '9':
			return T_ERROR;
		case '.':
			PutChar(ch);
			goto st_dot;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_octal;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_octal:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case '8':
		case '9':
			return T_ERROR;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_octal;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_hexa:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		if (isxdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_hexa;
		}
		if (isalnum(ch))
			return T_ERROR;
		UndoChar();
		EndChar();
		return T_INTEGER;
st_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case '.':
			PutChar(ch);
			goto st_dot;
		case 'd':
		case 'D':
		case 'e':
		case 'E':
			PutChar(ch);
			goto st_exp;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_dot:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_dot_digit;
		}
		// 1.#QNAN0
		UndoChar();
		return T_ERROR;
		
st_dot_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_FLOAT;
		}
		switch (ch = GetChar()) {
		case 'd':
		case 'D':
		case 'e':
		case 'E':
			PutChar(ch);
			goto st_exp;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_dot_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_FLOAT;
		}
st_exp:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		switch (ch = GetChar()) {
		case '+':
		case '-':
			PutChar(ch);
			goto st_exp_sign;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_exp_digit;
			}
			UndoChar();
			return T_ERROR;
		}
st_exp_sign:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_exp_digit;
		}
		UndoChar();
		return T_ERROR;
st_exp_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_FLOAT;
		}
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_exp_digit;
		}
		if (isalnum(ch))
			return T_ERROR;
		UndoChar();
		EndChar();
		return T_FLOAT;
return_string:
		MakeString();
		return T_STRING;
	}
}
#endif
XParser::XParser(void)
{
	m_nReserve = MAX_SYMBOL;
	m_szSymbolA = m_szMemoryA;
#ifdef	UNICODE
	m_szSymbolW = m_szMemoryW;
#endif
}

XParser::~XParser(void)
{
	if (m_nReserve > MAX_SYMBOL) {
		free(m_szSymbolA);
#ifdef	UNICODE
		free(m_szSymbolW);
#endif
	}
}
lisp::var	XParser::OnLoad()
{
	TOKEN token;
	lisp::var varList;
	lisp::var *pList = &varList;
	for ( ; ; ) {
		switch (token = GetToken()) {
		case T_END:
		case T_CLOSE:
			return varList;
		case T_OPEN:
			{
				lisp::var child = OnLoad();
				if (!child.listp()) {
					varList.destroy();
					return child;
				}
				*pList = new lisp::_cons(child, lisp::nil);
				pList = &pList->cdr();
			}
			break;
		case T_STRING:
			{
#ifdef	UNICODE
				size_t size = (m_nSymbolW+1) * sizeof(TCHAR);
#else
				size_t size = (m_nSymbolA+1) * sizeof(TCHAR);
#endif
				char *p = (char *) malloc(sizeof(lisp::_string) + size);
				*pList = new lisp::_cons(new (p) lisp::_string((LPCTSTR) memcpy(p + sizeof(lisp::_string), GetString(), size)), 
					     lisp::nil);
				pList = &pList->cdr();
			}
			break;
		case T_FLOAT:
			{
				*pList = new lisp::_cons(new lisp::_float(GetFloat()), lisp::nil);
				pList = &pList->cdr();
			}
			break;
		case T_INTEGER:
			{
				*pList = new lisp::_cons(new lisp::_integer(GetInteger()), lisp::nil);
				pList = &pList->cdr();
			}
			break;
		default:
			varList.destroy();
			_RPT(_T("XParser::OnLoad(): Invalid format at line %d\n"), GetLine());
			return lisp::error;
		}
	}
}

void	XParser::AllocString()
{
	if (m_nReserve <= MAX_SYMBOL) {
		m_nReserve += MAX_SYMBOL;
		m_szSymbolA = (char *) malloc(m_nReserve);
		memcpy(m_szSymbolA, m_szMemoryA, sizeof(m_szMemoryA));
#ifdef	UNICODE
		m_szSymbolW = (TCHAR *) malloc(m_nReserve * sizeof(TCHAR));
		memcpy(m_szSymbolW, m_szMemoryW, sizeof(m_szMemoryW));
#endif
	}
	else {
		m_nReserve += MAX_SYMBOL;
		m_szSymbolA = (char *) realloc(m_szSymbolA, m_nReserve);
#ifdef	UNICODE
		m_szSymbolW = (TCHAR *) realloc(m_szSymbolW, m_nReserve * sizeof(TCHAR));
#endif
	}
}

void	XParser::Open(XFile *pFile)
{
	m_pFile = pFile;
	m_nLine = 1;
	m_nDepth = 0;
}

lisp::var	XParser::Load(LPCTSTR szPath)
{
	XFileEx	file;
	if (!file.Open(szPath)) {
		m_nLine = 0;
		return lisp::error;
	}
	return Load(&file);
}

lisp::var	XParser::Load(XFile *pFile)
{
	m_pFile = pFile;
	m_nLine = 1;
	m_nDepth = 0;
	return OnLoad();
}


__forceinline int		XParser::GetChar() { return *m_pFile->m_pView++; }
__forceinline int		XParser::PeekChar() { return *m_pFile->m_pView; }
__forceinline void	XParser::UndoChar() { m_pFile->m_pView--; }
__forceinline void	XParser::InitChar() { m_nSymbolA = 0; }
__forceinline void	XParser::PutChar(int ch) 
{ 
	m_szSymbolA[m_nSymbolA++] = ch; 
	if (m_nSymbolA >= m_nReserve) 
		AllocString();
}
__forceinline void	XParser::EndChar() { m_szSymbolA[m_nSymbolA] = 0; }
__forceinline void	XParser::MakeString() 
{

	m_szSymbolA[m_nSymbolA] = 0;
#ifdef	UNICODE
	m_nSymbolW = MultiByteToWideChar(s_nCodePage, 0, m_szSymbolA, m_nSymbolA, m_szSymbolW, m_nReserve * sizeof(TCHAR));
	m_szSymbolW[m_nSymbolW] = 0;
#endif
}
DWORD_PTR	XParser::ParseList(PARSE_CALLBACK &Callback)
{
	TOKEN token;
	lisp::var top;
	lisp::var stack;

	for ( ; ; ) {
		token = GetToken();
		switch (token) {
			case T_END:
				if (m_nDepth != 0) {
					_RPT(_T("Unmatched open parenthesis\n"));
					top = lisp::error;
					goto quit;
				}
				top = lisp::nreverse(top);
				_ASSERT(stack.null());
				goto quit;

			case T_CLOSE:
				top = lisp::nreverse(top);
				if (stack.consp()) {
					lisp::_cons *pObject = (lisp::_cons *) stack.m_pObject;
					stack = pObject->m_car;
					pObject->m_car = top;
					top = pObject;
				}
				else if (m_nDepth < 0) {
					_RPT(_T("Unmatched close parenthesis %d\n"), GetLine());
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
					std::pair<lisp::_cons, lisp::_string> *p = (std::pair<lisp::_cons, lisp::_string> *) 
						_alloca(sizeof(std::pair<lisp::_cons, lisp::_string>) + size );
					new (&p->second) lisp::_string((LPCTSTR) memcpy((char *) (p + 1), GetString(), size));
					top = new(&p->first) lisp::_cons(&p->second, top);
				}
				break;
			case T_INTEGER:
				{
					std::pair<lisp::_cons, lisp::_integer> *p = (std::pair<lisp::_cons, lisp::_integer> *)
						_alloca(sizeof(std::pair<lisp::_cons, lisp::_integer>));
					new (&p->second) lisp::_integer(GetInteger());
					top = new(&p->first) lisp::_cons(&p->second, top);
				}
				break;
			case T_FLOAT:
				{
					std::pair<lisp::_cons, lisp::_float> *p = (std::pair<lisp::_cons, lisp::_float> *)
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
	return (Callback)( top);
}

BOOL	XParser::ParseFile(PARSE_CALLBACK &Callback)
{
	TOKEN token;
	for (; ; ) {
		switch (token = GetToken()) {
		case T_END:
			return TRUE;
		case T_CLOSE:
			_RPT(_T("Unmatched close parenthesis (%d)\n"), GetLine());
			if (!(Callback)( lisp::error))
				return FALSE;
			ResetDepth();
			break;
		case T_OPEN:
		{
			if (!ParseList(Callback)) {
				_RPT(_T("Invalid format (%d)"), GetLine());
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
			if (!(Callback)( &v))
				return FALSE;
		}
		break;
		case T_FLOAT:
		{
			lisp::_float v(GetFloat());
			if (!(Callback)( &v))
				return FALSE;
		}
		break;
		default:
			_RPT(_T("Invalid format (%d)\n"), GetLine());
			_ASSERT(0);
			for (; ; ) {
				if (GetDepth() == 0)
					break;
				if (GetToken() == T_END)
					break;
			}
		}
	}
}


XParser::TOKEN		XParser::GetToken()
{
	int ch;

	{
st_start:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_END;
		switch (ch = GetChar()) {
		case 0 :
			UndoChar();
			return T_END;
		case ';':	// comment
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_END;
				switch(GetChar()) {
				case 0:
				case '\n':
					UndoChar();
					goto st_start;
				}
			}
		case '\n':
			m_nLine++;
			goto st_start;
		case '(':
			m_nDepth++;
			return T_OPEN;
		case ')':
			m_nDepth--;
			return T_CLOSE;
		case '"':
			InitChar();
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_ERROR;
				switch(ch = GetChar()) {
				case 0:
					UndoChar();
					return T_ERROR;
				case '"':
					if (m_pFile->m_pView == m_pFile->m_pViewEnd)
						goto return_string;
					if ((ch = PeekChar()) != '"') 
						goto return_string;
					else
						PutChar('"');
					break;
				case '\r':
				case '\n':
					UndoChar();
					return T_ERROR;
				default:
					PutChar(ch);
					break;
				}
			}
		case '\'':
			InitChar();
			for ( ; ; ) {
				if (m_pFile->m_pView == m_pFile->m_pViewEnd)
					return T_ERROR;
				switch(ch = GetChar()) {
				case 0:
					UndoChar();
					return T_ERROR;
				case '\'':
					if (m_pFile->m_pView == m_pFile->m_pViewEnd)
						goto return_string;
					if ((ch = PeekChar()) != '\'') 
						goto return_string;
					else
						PutChar('\'');
					break;
				case '\r':
				case '\n':
					UndoChar();
					return T_ERROR;
				default:
					PutChar(ch);
					break;
				}
			}
		case ' ':
		case '\t':
		case '\r':
#ifdef	UNICODE
		case 0xfeff:
#endif
			goto st_start;
		case '+':
		case '-':
			InitChar();
			PutChar(ch);
			goto st_sign;
		case '0':
			InitChar();
			PutChar(ch);
			goto st_zero;
		case '.':
			InitChar();
			PutChar(ch);
			goto st_dot;
		case '*':
		case '/':
		case '_':
			InitChar();
			PutChar(ch);
			goto st_string;
		default:
			if (isdigit(ch)) {
				InitChar();
				PutChar(ch);
				goto st_digit;
			}
			else if (isalnum(ch)) {
				InitChar();
				PutChar(ch);
				goto st_string;
			}
			return T_ERROR;
		}
st_string:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			goto return_string;
		switch(ch = GetChar()) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '.':
		case '_':
			PutChar(ch);
			goto st_string;
		default:
			if (isalnum(ch)) {
				PutChar(ch);
				goto st_string;
			}
			UndoChar();
			goto return_string;
		}
st_sign:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			goto return_string;
		switch (ch = GetChar()) {
		case '0':
			PutChar(ch);
			goto st_zero;
		case '.':
			PutChar(ch);
			goto st_dot;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			goto return_string;
		}
st_zero:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case 'x':
		case 'X':
			PutChar(ch);
			goto st_hexa;
		case '8':
		case '9':
			return T_ERROR;
		case '.':
			PutChar(ch);
			goto st_dot;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_octal;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_octal:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case '8':
		case '9':
			return T_ERROR;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_octal;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_hexa:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		if (isxdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_hexa;
		}
		if (isalnum(ch))
			return T_ERROR;
		UndoChar();
		EndChar();
		return T_INTEGER;
st_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_INTEGER;
		}
		switch (ch = GetChar()) {
		case '.':
			PutChar(ch);
			goto st_dot;
		case 'd':
		case 'D':
		case 'e':
		case 'E':
			PutChar(ch);
			goto st_exp;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_INTEGER;
		}
st_dot:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_dot_digit;
		}
		// 1.#QNAN0
		UndoChar();
		return T_ERROR;
		
st_dot_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_FLOAT;
		}
		switch (ch = GetChar()) {
		case 'd':
		case 'D':
		case 'e':
		case 'E':
			PutChar(ch);
			goto st_exp;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_dot_digit;
			}
			if (isalnum(ch))
				return T_ERROR;
			UndoChar();
			EndChar();
			return T_FLOAT;
		}
st_exp:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		switch (ch = GetChar()) {
		case '+':
		case '-':
			PutChar(ch);
			goto st_exp_sign;
		default:
			if (isdigit(ch)) {
				PutChar(ch);
				goto st_exp_digit;
			}
			UndoChar();
			return T_ERROR;
		}
st_exp_sign:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd)
			return T_ERROR;
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_exp_digit;
		}
		UndoChar();
		return T_ERROR;
st_exp_digit:
		if (m_pFile->m_pView == m_pFile->m_pViewEnd) {
			EndChar();
			return T_FLOAT;
		}
		if (isdigit(ch = GetChar())) {
			PutChar(ch);
			goto st_exp_digit;
		}
		if (isalnum(ch))
			return T_ERROR;
		UndoChar();
		EndChar();
		return T_FLOAT;
return_string:
		MakeString();
		return T_STRING;
	}
}

