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
		virtual LPCTSTR GetString() const noexcept { return _T(""); }
		virtual int	GetInteger() const noexcept { return 0; }
		virtual float GetFloat() const noexcept { return 0; }
		virtual var& car() noexcept { return nil; }
		virtual var& cdr() noexcept { return nil; }
		virtual bool consp() const noexcept { return false; }
		virtual bool null() const noexcept { return false; }
		virtual bool listp() const noexcept { return false; }
		virtual bool stringp() const noexcept { return false; }
		virtual bool numberp() const noexcept { return false; }
		virtual bool integerp() const noexcept { return false; }
		virtual bool floatp() const noexcept { return false; }
		virtual bool errorp() const noexcept { return false; }
		virtual int	 length() const noexcept { return 0; }
		virtual _object* copy() const  = 0;
		virtual void destroy() noexcept = 0;
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept = 0;
#endif
	};

	class _string : public _object
	{
	protected:
		LPCTSTR m_pString;
	public:
		_string(LPCTSTR str) noexcept { m_pString = str; }
		virtual LPCTSTR GetString() const noexcept { return m_pString; }
		virtual int GetInteger() const noexcept;
		virtual float GetFloat() const noexcept;
		virtual unsigned GetUnsigned() const noexcept;
		virtual bool stringp() const noexcept { return true; }
		virtual _object* copy() const ;
		virtual void destroy() noexcept;
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif
	};
	class _null : public _object
	{
		virtual bool null() const noexcept { return true; }
		virtual bool listp() const noexcept { return true; }
		virtual _object* copy() const  { return &s_nil; }
		virtual void destroy() noexcept {}
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif
	};

	class var
	{
	public:
		_object *m_pObject;
	public:
		var() noexcept { m_pObject = &s_nil; }
		var(const var& v) noexcept { m_pObject = v.m_pObject; }
		var(_object *p) noexcept { m_pObject = p; }
		var& car() noexcept { return m_pObject->car(); }
		var& cdr() noexcept { return m_pObject->cdr(); }
		bool consp() const noexcept { return m_pObject->consp(); }
		bool null() const noexcept { return m_pObject->null(); }
		bool stringp() const noexcept { return m_pObject->stringp(); }
		bool numberp() const noexcept { return m_pObject->numberp(); }
		bool integerp() const noexcept { return m_pObject->integerp(); }
		bool floatp() const noexcept { return m_pObject->floatp(); }
		bool errorp() const noexcept { return m_pObject->errorp(); }
		bool listp() const noexcept { return m_pObject->listp(); }
		var pop() noexcept { _object *pObject = m_pObject; m_pObject = m_pObject->cdr().m_pObject; return pObject->car(); }
		int	length() const noexcept { return m_pObject->length(); }
		void operator=(const var v) noexcept { _ASSERT(this != &nil); m_pObject = v.m_pObject; }
		var& operator*() noexcept { return m_pObject->car(); }
		operator LPCTSTR() const noexcept { return m_pObject->GetString(); }
		operator int() const noexcept { return (int)m_pObject->GetInteger(); }
		operator float() const noexcept { return m_pObject->GetFloat(); }
		operator double() const { return m_pObject->GetFloat(); }
		operator char() const noexcept { return (char)m_pObject->GetInteger(); }
		operator short() const noexcept { return (short)m_pObject->GetInteger(); }
		operator DWORD() const noexcept { return (DWORD)m_pObject->GetInteger(); }
		operator INT64() const noexcept { return (INT64)m_pObject->GetInteger(); }
		var copy() const  { return m_pObject->copy(); }
		void destroy() noexcept { m_pObject->destroy(); m_pObject = &s_nil; }
		var get(LPCTSTR name) noexcept;
		var nth(int pos) noexcept;
		var get(LPCTSTR name, int pos) noexcept;
		var nthcdr(int pos) noexcept;
//		bool empty() const { return m_pObject == 0; }
#ifdef	_DEBUG
		void print() const noexcept;
		tstring print(int level) const noexcept { return m_pObject->print(level); }
#endif
	};

	class _cons : public _object
	{
	public:
		var m_car;
		var m_cdr;
	public:
		_cons(const var car, const var cdr) noexcept : m_car(car), m_cdr(cdr) {}
		virtual var& car() noexcept { return m_car; }
		virtual var& cdr() noexcept { return m_cdr; }
		virtual bool consp() const noexcept { return true; }
		virtual bool listp() const noexcept { return true; }
		virtual int	length() const noexcept;
		virtual _object* copy() const ;
		virtual void destroy() noexcept;
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif
	};
	class _integer : public _object
	{
	protected:
		int	m_nValue;
	public:
		_integer(int n) noexcept { m_nValue = n; }
		virtual LPCTSTR GetString() const noexcept
		{ 
			_ASSERT(0);
			return _T(""); 
		}
		virtual int GetInteger() const noexcept { return m_nValue; }
		virtual float GetFloat() const noexcept { return (float) m_nValue; }
		virtual bool numberp() const noexcept { return true; }
		virtual bool integerp() const noexcept { return true; }
		virtual _object* copy() const  { return new _integer(m_nValue); }
		virtual void destroy() noexcept { delete this; }
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif		
	};

	class _float : public _object
	{
	protected:
		float	m_fValue;
	public:
		_float(float n) noexcept  { m_fValue = n; }
		virtual LPCTSTR GetString() const noexcept
		{ 
			_ASSERT(0);
			return _T("");
		}
		virtual int	GetInteger() const noexcept { return (int) m_fValue; }
		virtual float GetFloat() const noexcept { return m_fValue; }
		virtual bool numberp() const noexcept { return true; }
		virtual bool floatp() const noexcept { return true; }
		virtual _object* copy() const  { return new _float(m_fValue); }
		virtual void destroy() noexcept { delete this; }
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif
	};

	class _error : public _object
	{
		virtual bool errorp() const noexcept { return true; }
		virtual _object* copy() const  { return &s_error; }
		virtual void destroy() noexcept {}
#ifdef _DEBUG
		virtual tstring print(int level) const noexcept;
#endif
	};

	var nreverse(var v) noexcept;
}

