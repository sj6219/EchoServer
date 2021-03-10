// Lisp.h: interface for the Lisp class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

#ifdef	UNICODE
typedef	std::wstring tstring;
#else
typedef	std::string tstring;
#endif

namespace lisp
{
	class var;
	class _cons;
	class _null;
	class _error;

	extern var nil;
	extern var const error;
	extern _null s_nil;
	extern _error s_error;

	class _object
	{
	public:
		virtual LPCTSTR GetString() const { return _T(""); }
		virtual int	GetInteger() const { return 0; }
		virtual float GetFloat() const { return 0; }
		virtual var& car() { return nil; }
		virtual var& cdr() { return nil; }
		virtual bool consp() const { return false; }
		virtual bool null() const { return false; }
		virtual bool listp() const { return false; }
		virtual bool stringp() const { return false; }
		virtual bool numberp() const { return false; }
		virtual bool integerp() const { return false; }
		virtual bool floatp() const { return false; }
		virtual bool errorp() const { return false; }
		virtual int	 length() const { return 0; }
		virtual _object* copy() const = 0;
		virtual void destroy() = 0;
#ifdef _DEBUG
		virtual tstring print(int level) const = 0;
#endif
	};




	class _string : public _object
	{
	protected:
		LPCTSTR m_pString;
	public:
		_string(LPCTSTR str) { m_pString = str; }
		virtual LPCTSTR GetString() const { return m_pString; }
		virtual int GetInteger() const;
		virtual float GetFloat() const;
		virtual unsigned GetUnsigned() const;
		virtual bool stringp() const { return true; }
		virtual _object* copy() const;
		virtual void destroy();
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif
	};
	class _null : public _object
	{
		virtual bool null() const { return true; }
		virtual bool listp() const { return true; }
		virtual _object* copy() const { return &s_nil; }
		virtual void destroy() {}
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif
	};

	class var
	{
	public:
		_object *m_pObject;
	public:
		var() { m_pObject = &s_nil; }
		var(const var& v) { m_pObject = v.m_pObject; }
		var(_object *p) { m_pObject = p; }
		var& car() const { return m_pObject->car(); }
		var& cdr() const { return m_pObject->cdr(); }
		bool consp() const { return m_pObject->consp(); }
		bool null() const { return m_pObject->null(); }
		bool stringp() const { return m_pObject->stringp(); }
		bool numberp() const { return m_pObject->numberp(); }
		bool integerp() const { return m_pObject->integerp(); }
		bool floatp() const { return m_pObject->floatp(); }
		bool errorp() const { return m_pObject->errorp(); }
		bool listp() const { return m_pObject->listp(); }
		var pop() { _object *pObject = m_pObject; m_pObject = m_pObject->cdr().m_pObject; return pObject->car(); }
		int	length() const { return m_pObject->length(); }
		void operator=(const var v) { _ASSERT(this != &nil); m_pObject = v.m_pObject; }
		var& operator*() { return m_pObject->car(); }
		operator LPCTSTR() const { return m_pObject->GetString(); }
		operator int() const { return (int)m_pObject->GetInteger(); }
		operator float() const { return m_pObject->GetFloat(); }
		operator double() const { return m_pObject->GetFloat(); }
		operator char() const { return (char)m_pObject->GetInteger(); }
		operator short() const { return (short)m_pObject->GetInteger(); }
		operator DWORD() const { return (DWORD)m_pObject->GetInteger(); }
		operator INT64() const { return (INT64)m_pObject->GetInteger(); }
		var copy() const { return m_pObject->copy(); } 
		void destroy() { m_pObject->destroy(); m_pObject = &s_nil; } 
		var get(LPCTSTR name);
		var nth(int pos);
		var get(LPCTSTR name, int pos);
		var nthcdr(int pos);
//		bool empty() const { return m_pObject == 0; }
#ifdef	_DEBUG
		void print() const;
		tstring print(int level) const { return m_pObject->print(level); }
#endif
	};

	class _cons : public _object
	{
	public:
		var m_car;
		var m_cdr;
	public:
		_cons(const var car, const var cdr) : m_car(car), m_cdr(cdr) {} 
		virtual var& car() { return m_car; }
		virtual var& cdr() { return m_cdr; }
		virtual bool consp() const { return true; }
		virtual bool listp() const { return true; }
		virtual int	length() const;
		virtual _object* copy() const;
		virtual void destroy();
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif
	};
	class _integer : public _object
	{
	protected:
		int	m_nValue;
	public:
		_integer(int n) { m_nValue = n; }
		virtual LPCTSTR GetString() const 
		{ 
			_ASSERT(0);
			return _T(""); 
		}
		virtual int GetInteger() const { return m_nValue; }
		virtual float GetFloat() const { return (float) m_nValue; }
		virtual bool numberp() const { return true; }
		virtual bool integerp() const { return true; }
		virtual _object* copy() const { return new _integer(m_nValue); }
		virtual void destroy() { delete this; }
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif		
	};

	class _float : public _object
	{
	protected:
		float	m_fValue;
	public:
		_float(float n) { m_fValue = n; }
		virtual LPCTSTR GetString() const 
		{ 
			_ASSERT(0);
			return _T("");
		}
		virtual int	GetInteger() const { return (int) m_fValue; }
		virtual float GetFloat() const { return m_fValue; }
		virtual bool numberp() const { return true; }
		virtual bool floatp() const { return true; }
		virtual _object* copy() const { return new _float(m_fValue); }
		virtual void destroy() { delete this; }
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif
	};



	class _error : public _object
	{
		virtual bool errorp() const { return true; }
		virtual _object* copy() const { return &s_error; }
		virtual void destroy() {}
#ifdef _DEBUG
		virtual tstring print(int level) const;
#endif
	};



	var nreverse(var v);
}

