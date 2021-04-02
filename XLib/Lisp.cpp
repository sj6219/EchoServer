#include "pch.h"
#include "IOLib.h"
#include <windows.h>
#include <tchar.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "lisp.h"
using namespace lisp;

#ifdef _DEBUG
#undef new
#endif

var	lisp::nreverse(var v) noexcept
{
	_object *pInput = v.m_pObject;
	if (!pInput->consp())
		return pInput;
	_object *pOutput = 0;

	for ( ; ; ) {
		_cons *pObject = (_cons *) pInput;
		pInput = pObject->m_cdr.m_pObject;
		pObject->m_cdr.m_pObject = pOutput;
		pOutput = pObject;
		if (!pInput->consp())
			break;
	}
	((_cons *) v.m_pObject)->m_cdr.m_pObject = pInput;
	return pOutput;		
}

lisp::_null lisp::s_nil;
lisp::var lisp::nil;

lisp::_error lisp::s_error;	
lisp::var const lisp::error(&s_error);


_object* lisp::_string::copy() const 
{ 
	size_t size = (_tcslen(m_pString) + 1) * sizeof(TCHAR);
	char *p = (char *) malloc(sizeof(_string) + size);
	return new (p) _string((LPCTSTR) memcpy(p + sizeof(_string), m_pString, size)); 
}

void lisp::_string::destroy() noexcept
{ 
	free(this); 
}

#ifdef _DEBUG
String lisp::_string::print(int level) const noexcept
{
	return String(_T("\"")) +  m_pString + _T('"');
}

String lisp::_integer::print(int level) const noexcept
{
	TCHAR buf[20];
	_stprintf_s(buf, _T("%d"), m_nValue);
	return buf;
}

String lisp::_float::print(int level) const noexcept
{
	TCHAR buf[20];
	_stprintf_s(buf,  _T("%.2f"), m_fValue);
	return buf;
}
#endif

int	lisp::_cons::length() const noexcept
{
	int len = 0;
	var v = m_cdr;
	for ( ; ; ) {
		++len;
		if (!v.consp()) {
			return len;
		}
		v = v.cdr();
	}
}

#ifdef _DEBUG
String	lisp::_cons::print(int level) const noexcept
{
	if (level >= 6)
		return _T("(...)");
	if (m_cdr.null())
		return _T('(')+m_car.print(level+1)+_T(')');
	if (m_cdr.consp()) 
		return _T('(') + m_car.print(level+4) + _T(' ') + (m_cdr.print(level+1).c_str() + 1);
	return _T('(') + m_car.print(level+4) + _T(" . ") + m_cdr.print(level+1) + _T(')');
}

String lisp::_null::print(int level) const noexcept
{
	return _T("()");
}

String lisp::_error::print(int level) const noexcept
{
	return _T("#error");
}

void	lisp::var::print() const noexcept
{
	LOG(_T("%s\n"), print(0).c_str());
}

#endif

void lisp::_cons::destroy() noexcept
{
	DWORD *vptr = *(DWORD **) this;
	_cons *parent = this;
	for ( ; ; ) {
		_object *child = parent->m_cdr.m_pObject;
		parent->m_car.m_pObject->destroy();
		delete parent;
		if (*(DWORD **)child != vptr) { // if (!child->consp())
			child->destroy();
			break;
		}
		parent = (_cons *) child;
	}
}

lisp::var lisp::var::get(LPCTSTR name) noexcept
{
	var parent = *this;
	for ( ; !parent.null() ; ) {
		var child = parent.pop();
		if (_tcscmp(name, child.car()) == 0)
			return child;
	}
	return nil;
}

lisp::var lisp::var::nth(int pos) noexcept
{
	var v = *this;
	for ( ; --pos >= 0; ) {
		v = v.cdr();
	}
	return v.car();
}

lisp::_object*	lisp::_cons::copy() const 
{
	DWORD *vptr = *(DWORD **) this;
	const _cons *parent = this;

	_object *root;
	_object **ppObject = &root;
	for ( ; ; ) {
		const _object *child = parent->m_cdr.m_pObject;
		_cons *cons = NEW _cons(parent->m_car.m_pObject->copy(), 0);
		*ppObject = cons; 
		ppObject = &cons->m_cdr.m_pObject;
		if (*(DWORD **)child != vptr) { // if (!child->consp())
			*ppObject = child->copy();
			break;
		}
		parent = (const _cons *) child;
	}
	return root;
}

lisp::var lisp::var::get(LPCTSTR name, int pos) noexcept
{
	var parent = *this;
	for ( ; !parent.null() ; ) {
		var child = parent.pop();
		if (_tcscmp(name, child.car()) == 0 && --pos < 0)
			return child;
	}
	return nil;
}

lisp::var lisp::var::nthcdr(int pos) noexcept
{
	var v = *this;
	for ( ; --pos >= 0; ) {
		v = v.cdr();
	}
	return v;
}



int	lisp::_string::GetInteger() const noexcept
{
	return _tcstol(m_pString, 0, 0);
}

float	lisp::_string::GetFloat() const noexcept
{
	return (float) _tstof(m_pString);
}

unsigned	lisp::_string::GetUnsigned() const noexcept
{
	return _tcstoul(m_pString, 0, 0);
}
