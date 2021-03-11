#pragma once

#define USE_SRWLOCK

#include "IOLib.h"
#include "Link.h"
#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <synchapi.h>

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

#define CHGSIGN(f) (*(DWORD *) &f ^= 0x80000000)
#define NEGATIVE(f) (*(int *) &f < 0)

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


class XSpinLock
{
protected:
	long m_lock;

	void wait();
public:
	XSpinLock() { m_lock = 0; }
	void lock()
	{
		if (InterlockedCompareExchange(&m_lock, 1, 0))
			wait();
	}
	void unlock()
	{
		InterlockedExchange(&m_lock, 0);
	}
	bool try_lock()
	{
		return InterlockedCompareExchange(&m_lock, 1, 0) == 0;
	}
};


class XRWLock
{
#ifdef USE_SRWLOCK
	SRWLOCK m_lock;
public:
	XRWLock() { InitializeSRWLock(&m_lock); }
	void lock() { AcquireSRWLockExclusive(&m_lock); }
	void unlock() { ReleaseSRWLockExclusive(&m_lock); }
	BOOL try_lock() { return TryAcquireSRWLockExclusive(&m_lock); }
	void lock_shared() { AcquireSRWLockShared(&m_lock); }
	void unlock_shared() { ReleaseSRWLockShared(&m_lock);  }
#else
public:
	XRWLock();
	~XRWLock();
	void lock();
	void unlock();
	BOOL try_lock();
	void lock_shared();
	void unlock_shared();

	void Lock()
	{
		if (InterlockedCompareExchange(&m_nLock, 1, 0))
			Wait();
	}
	void Unlock()
	{
		InterlockedExchange(&m_nLock, 0);
	}
	void Wait();
	HANDLE m_hREvent;
	HANDLE m_hWEvent;
	long m_nCount;
	long m_nLock;
#endif

};

template <typename T> class XSharedLock
{
private:
	typename T* m_pT;
public:
	XSharedLock(typename T* pT) : m_pT(pT)
	{
		m_pT->lock_shared();
	}
	~XSharedLock()
	{
		m_pT->unlock_shared();
	}
};

template <typename T> class XUniqueLock
{
private:
	typename T* m_pT;
public:
	XUniqueLock(typename T* pT) : m_pT(pT)
	{
		m_pT->lock();
	}
	~XUniqueLock()
	{
		m_pT->unlock();
	}
};


class XMemoryPage;
class XMemoryBase;

namespace util
{
	const DWORD ALLOC_SIZE = 64 * 1024;
	const DWORD PAGE_SIZE = 4 * 1024;

	XMemoryPage *AllocPage();
	void	FreePage(XMemoryPage *pPage);
}

#pragma push_macro("new")
#undef new

class XMemoryPage : public XLink
{
public:

	char	*m_pFreeList;
	DWORD	m_nAlloc;
};

template <int size> class XMemoryPool 
{
public:

	union U {
		char m_buffer[size];
		U *m_pU;
	};

	static const DWORD PAGE_SIZE = util::PAGE_SIZE;
	typedef	linked_list_<XMemoryPage *> PAGELIST;

	PAGELIST	m_listPage;
#ifdef	_MT
	XLock	m_lock;
	static void Lock()		{ s_self.m_lock.Lock();}
	static void Unlock()	{ s_self.m_lock.Unlock();}
#else
	static void Lock()		{}
	static void Unlock()	{}
#endif

	static XMemoryPool s_self;
	static const DWORD COUNT = (PAGE_SIZE - sizeof(XMemoryPage))/size;

#ifdef	_DEBUG
	XMemoryPool()
	{
		ASSERT(sizeof(U) + sizeof(XMemoryPage) <= PAGE_SIZE);
	}
#endif
		

	static void *Alloc() 
	{
		XMemoryPage *pPage;
		Lock();
		if (s_self.m_listPage.empty()) {
			Unlock();
			pPage = util::AllocPage();
			Lock();
			s_self.m_listPage.push_back(pPage);
		}
		else
			pPage = s_self.m_listPage.back();
		char *pFree = pPage->m_pFreeList;
		if (pFree == 0) {
			pFree = (char *) pPage + sizeof(XMemoryPage) + pPage->m_nAlloc * sizeof(U);
		}
		else
			pPage->m_pFreeList = *(char **) pFree;
		if (++pPage->m_nAlloc == COUNT) 
			s_self.m_listPage.erase(pPage);
		Unlock();
#ifdef	_DEBUG
		memset(pFree, 0xfc, sizeof(U));
#endif
		return pFree;
	}

	static void Free(void *pT)
	{
#ifdef	_DEBUG
		memset(pT, 0xfd, sizeof(U));
#endif
		Lock();
		XMemoryPage *pPage = (XMemoryPage *) ((DWORD_PTR) pT & -(int) PAGE_SIZE);
		*(char **) pT = pPage->m_pFreeList;
		pPage->m_pFreeList = (char *) pT;
		if (pPage->m_nAlloc == COUNT) 
			s_self.m_listPage.push_back(pPage);
		if (--pPage->m_nAlloc == 0) {
			s_self.m_listPage.erase(pPage);
			Unlock();
			util::FreePage(pPage);
		}
		else {
			Unlock();
		}
	}

};


template <int size>  typename XMemoryPool<size>  XMemoryPool<size>::s_self;

template <typename TYPE> TYPE *XConstruct()
{
	return new (XMemoryPool<sizeof(TYPE)>::Alloc()) TYPE;
	//return new TYPE;
}

template <typename TYPE> void XDestruct(TYPE *ptr)
{
	ptr->~TYPE();
	XMemoryPool<sizeof(TYPE)>::Free(ptr);
}

template <int size> class YMemoryPool {
public:
	union U {
		char m_buffer[size];
		U *m_pU;
	};

	U* m_pU;
#ifdef	_MT
	XSpinLock	m_lock;
	static void Lock()		{ s_self.m_lock.Lock();}
	static void Unlock()	{ s_self.m_lock.Unlock();}
#else
	static void Lock()		{}
	static void Unlock()	{}
#endif
	~YMemoryPool() { YMemoryPool::FreeAll(); }

	static YMemoryPool s_self;
		
	static void *Alloc() 
	{
		Lock();
		U* pU;
		if ((pU = s_self.m_pU) != NULL) {
			s_self.m_pU = pU->m_pU;
		Unlock();
#ifdef	_DEBUG
			pU = (U*) realloc(pU, sizeof(U));
#endif
		}
		else {
			Unlock();
			pU = (U*) malloc(sizeof(U));
		}
#ifdef	_DEBUG
		memset(pU, 0xfc, sizeof(U));
#endif
		return pU;
	}

	static void Free(void *pT)
	{
#ifdef	_DEBUG
		memset(pT, 0xfd, sizeof(U));
#endif
		Lock();
		U* pU = (U*) pT;
		pU->m_pU = s_self.m_pU;
		s_self.m_pU = pU;
		Unlock();
	}

	static void FreeAll()
	{
		Lock();
		U *pU;
		while ((pU = s_self.m_pU) != NULL) {
			s_self.m_pU = pU->m_pU;
			free(pU);
		}
		Unlock();
	}
};

template <int size>  typename YMemoryPool<size>  YMemoryPool<size>::s_self;

template <typename TYPE> TYPE *YConstruct()
{
	return new (YMemoryPool<sizeof(TYPE)>::Alloc()) TYPE;
}

template <typename TYPE> void YDestruct(TYPE *ptr)
{
	ptr->~TYPE();
	YMemoryPool<sizeof(TYPE)>::Free(ptr);
}


template <class T> class ZMemoryPool {
public:
	T *	m_pT;
#ifdef	_MT
	XSpinLock	m_lock;
	static void Lock()		{ s_self.m_lock.Lock();}
	static void Unlock()	{ s_self.m_lock.Unlock();}
#else
	static void Lock()		{}
	static void Unlock()	{}
#endif
	~ZMemoryPool() { ZMemoryPool::FreeAll(); }

	static ZMemoryPool s_self;
		
	static T* Alloc()
	{
		T* pT;
		Lock();
		if ((pT = s_self.m_pT) != NULL) {
			s_self.m_pT = (T *) pT->m_pNext;
			Unlock();
			return pT;
		}
		else {
			Unlock();
			return new T;
		}
	}
	static void Free(T* pT) 
	{
		Lock();
		pT->m_pNext = s_self.m_pT;
		s_self.m_pT = pT;
		Unlock();
	}
	static void FreeAll()
	{
		Lock();
		T *pT;
		while ((pT = s_self.m_pT) != NULL) {
			s_self.m_pT = (T *) pT->m_pNext;
			delete pT;
		}
		Unlock();
	}
};

template <class T> typename ZMemoryPool<T> ZMemoryPool<T>::s_self;

template <typename TYPE> TYPE *ZConstruct()
{
	return ZMemoryPool<TYPE>::Alloc();
}

template <typename TYPE> void ZDestruct(TYPE *ptr)
{
	ZMemoryPool<TYPE>::Free(ptr);
}
#pragma pop_macro("new")




void InitRandom();
unsigned _Random();
inline int Random()
{
	return _Random() >> 1;
}

inline int Random( int nMin, int nMax)
{
	if( nMin < nMax)
		return _Random() % (nMax - nMin + 1) + nMin;
	else
		return nMin;
}

#ifndef LPPACKET
#define LPCPACKET LPCSTR
#define LPPACKET LPSTR
#endif

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

template <class TYPE>
LPCPACKET GetNumeric(LPCPACKET packet, TYPE* Num)
{
	*Num = *(TYPE *) packet;
	return packet + sizeof( TYPE);
}

template <class TYPE>
LPCPACKET GetNumeric(LPCPACKET packet, TYPE* Num, const int nArry)
{
	memcpy( Num, packet, sizeof( TYPE) * nArry);
	return packet + sizeof( TYPE) * nArry;
}

template <class TYPE>
LPCPACKET ScanNumeric(LPCPACKET packet, LPCPACKET end, TYPE* Num)
{
	if (packet + sizeof(TYPE) > end)
		return packet;
	*Num = *(TYPE *) packet;
	return packet + sizeof( TYPE);
}

template <class TYPE>
LPCPACKET ScanNumeric(LPCPACKET packet, LPCPACKET end, TYPE* Num, const int nArray)
{
	if (packet + sizeof(TYPE) * nArray > end)
		return packet;
	memcpy( Num, packet, sizeof( TYPE) * nArray);
	return packet + sizeof( TYPE) * nArray;
}

template <class TYPE>
LPPACKET PutNumeric(LPPACKET packet, const TYPE Num)
{
	*(TYPE *) packet = Num;
	return packet + sizeof( TYPE);
}

template <class TYPE>
LPPACKET PutNumeric(LPPACKET packet, const TYPE* Num, const int nArry)
{
	memcpy( packet, Num, sizeof(TYPE) * nArry);
	return packet + sizeof( TYPE) * nArry;
}

BOOL CreatePath( LPCTSTR szPath);
BOOL MovePath( LPCTSTR szOldFile, LPCTSTR szNewFile);
BOOL DeletePath( LPCTSTR ofname);
FILE* FOpen(LPCTSTR filename);
BOOL CheckResidentNum( char* szResident, char* szNum1, char* szNum2);	//	주민번호 검사.('-' 포함 14Byte)
BOOL CheckResidentNum( char* szNum1, char* szNum2);	//	주민번호 검사.
BOOL CheckResidentNum( char* szNum);	//	주민번호 검사.
BOOL	__stdcall IsValidString(LPCTSTR str, LPCTSTR = _T(" \n"));
BOOL	__stdcall IsValidName(LPCTSTR name);
BOOL	__stdcall IsValidAdminName(LPCTSTR name, LPCTSTR = _T("<>"));
BOOL	__stdcall IsValidAddress(void* p);

inline BOOL	IsValidAddress(DWORD_PTR p)
{
	return IsValidAddress( (void*)p);
}

inline BOOL IsValidObject(void *p)
{
	DWORD_PTR vptr = *(DWORD_PTR *)p;
#ifdef _WIN64
	return (vptr >= 0x400000 && vptr < 0x7ff0000000000000 && (vptr & 3) == 0);
#else
	return (vptr >= 0x400000 && vptr < 0x7ffe0000 && (vptr & 3) == 0);
#endif
}


inline double Distance(const POINT& ptP1, const POINT& ptP2)
{
	double dx = ptP1.x - ptP2.x;
	double dy = ptP1.y - ptP2.y;
	return sqrt(dx * dx + dy * dy);
}

inline double Distance(double dx, double dy)
{
	return sqrt(dx * dx + dy * dy);
}

inline double Distance(double dx, double dy, double dz)
{
	return sqrt(dx * dx + dy * dy + dz * dz);
}

inline int PowDistance( const POINT& ptP1, const POINT& ptP2)
{
	int dx = ptP1.x - ptP2.x;
	int dy = ptP1.y - ptP2.y;
	return dx * dx + dy * dy;
}

inline __int64 PowDistance64( const POINT& ptP1, const POINT& ptP2)
{
	__int64 dx = ptP1.x - ptP2.x;
	__int64 dy = ptP1.y - ptP2.y;
	return dx * dx + dy * dy;
}

void LogPacket( int nType, int nSize, char* buffer);


#ifdef	UNICODE
std::wstring ToWString(LPCSTR lpa);
std::string ToString(LPCWSTR lpw);
#define AtoT(lpa) ToWString(lpa).c_str()
#define TtoA(lpw) ToString(lpw).c_str()
#else
#define AtoT(lpa) lpa
#define TtoA(lpw) lpw
#endif

tstring GetUniqueName();
LPCTSTR	GetNamePart(LPCTSTR szPath);
__forceinline LPTSTR	GetNamePart(LPTSTR szPath) { return (LPTSTR) GetNamePart((LPCTSTR) szPath); }

template <typename TYPE>  class Secure
{
public:	
	TYPE m_nValue;
	static TYPE m_nKey;

	Secure() {}
	Secure(TYPE nValue) { m_nValue = nValue ^ m_nKey; }

	operator TYPE () { return m_nValue ^ m_nKey; }
	void	operator = (TYPE nValue) { m_nValue = nValue ^ m_nKey; }
};

template <typename TYPE> TYPE Secure<TYPE>::m_nKey;

template <typename TYPE> class deque
{
	typedef	std::vector<TYPE> QueueType;
	QueueType	m_queue;
	typename QueueType::iterator	m_front;
	typename QueueType::iterator	m_back;
	long	m_nSize;
 
public:

	deque()
	{
		m_nSize = 0;
		m_front = m_back = m_queue.insert(m_queue.begin(), TYPE());
	}

	void push(const TYPE &element) 
	{
		*m_back = element;
		if( ++m_back == m_queue.end())
			m_back = m_queue.begin();
		if( m_front == m_back)
		{
			m_back = m_queue.insert( m_back, TYPE());
			m_front = m_back + 1;
		}
		m_nSize++;
	}
	
	void pop()
	{
		if( ++m_front == m_queue.end())
			m_front = m_queue.begin();
		m_nSize--;
	}
	
	TYPE &front()
	{
		return *m_front;
	}

	TYPE &back()
	{
		if (m_back == m_queue.begin())
			return m_queue.end()[-1];
		else
			return m_back[-1];
	}

	int	size() const
	{
		return m_nSize;
	}

	bool	empty() const
	{
		return m_nSize == 0;
	}

};

template <typename TYPE> class priority_queue 
{
public:
	typedef std::vector<TYPE> container_type;
	typedef typename container_type::value_type value_type;
	typedef typename container_type::size_type size_type;
	typedef	typename container_type::iterator	iterator;
	typedef	typename container_type::difference_type	difference_type;

public:
	bool empty() const
		{	// test if queue is empty
		return (c.empty());
		}

	size_type size() const
		{	// return length of queue
		return (c.size());
		}

	const value_type& top() const
		{	// return highest-priority element
		return (c.front());
		}

	value_type& top()
		{	// return mutable highest-priority element (retained)
		return (c.front());
		}

	void push(const value_type& _Pred)
		{	// insert value in priority order
		c.push_back(_Pred);
		push_heap(c.begin(), c.end());
		}

	void pop()
		{	// erase highest-priority element
		pop_heap(c.begin(), c.end());
		c.pop_back();
		}

	iterator begin()
	{
		return c.begin();
	}

	iterator	end()
	{
		return	c.end();
	}

	void	erase(iterator _Where)
	{
		TYPE _Val = *(c.end() - 1);
		//*(c.end()-1) = *_Where;
		
		//_Adjust_heap(_RanIt _First, _Diff _Hole, _Diff _Bottom, _Ty _Val)
		iterator	_First = c.begin();
		difference_type _Hole = _Where - _First;
		difference_type _Bottom = c.end() - _First - 1;

//		difference_type _Top = _Hole;
		difference_type _Top = 0;
		difference_type _Idx = 2 * _Hole + 2;

		for (; _Idx < _Bottom; _Idx = 2 * _Idx + 2)
			{	// move _Hole down to larger child
			if (*(_First + _Idx) < *(_First + (_Idx - 1)))
				--_Idx;
			*(_First + _Hole) = *(_First + _Idx), _Hole = _Idx;
			}

		if (_Idx == _Bottom)
			{	// only child at bottom, move _Hole down to it
			*(_First + _Hole) = *(_First + (_Bottom - 1));
			_Hole = _Bottom - 1;
			}
		
		// _Push_heap(_First, _Hole, _Top, _Val);
		for (_Idx = (_Hole - 1) / 2;
			_Top < _Hole && *(_First + _Idx) < _Val;
			_Idx = (_Hole - 1) / 2)
			{	// move _Hole up to parent
			*(_First + _Hole) = *(_First + _Idx);
			_Hole = _Idx;
			}

		*(_First + _Hole) = _Val;	// drop _Val into final hole

		//
		c.pop_back();
	}
	void	raise(iterator _Where)
	{
		TYPE _Val = *_Where;
		iterator _First = c.begin();
		difference_type _Hole = _Where - _First;
		difference_type _Idx;

		// _Push_heap(_First, _Hole, _Top, _Val);
		for (_Idx = (_Hole - 1) / 2;
			0 < _Hole && *(_First + _Idx) < _Val;
			_Idx = (_Hole - 1) / 2)
			{	// move _Hole up to parent
			*(_First + _Hole) = *(_First + _Idx);
			_Hole = _Idx;
			}

		*(_First + _Hole) = _Val;	// drop _Val into final hole
	}

protected:
	container_type c;

};

template<class _FwdIt, class _Ty> inline _FwdIt equal(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
{	
	typename std::iterator_traits<_FwdIt>::difference_type _Count = _Last - _First;

	for (; 0 < _Count; )
	{	// divide and conquer, find half that contains answer
		typename std::iterator_traits<_FwdIt>::difference_type _Count2 = _Count / 2;
		_FwdIt _Mid = _First + _Count2;

		int nCmp = *_Mid - _Val;
		if (nCmp < 0)
			_First = ++_Mid, _Count -= _Count2 + 1;
		else if (nCmp > 0)
			_Count = _Count2;
		else 
			return _Mid;
	}
	return (_Last);
}

template<class _FwdIt, class _Ty> inline _FwdIt lower_bound(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
{
	// find first element not before _Val, using operator<
	typename std::iterator_traits<_FwdIt>::difference_type _Count = _Last - _First;

	for (; 0 < _Count; )
	{	// divide and conquer, find half that contains answer
		typename std::iterator_traits<_FwdIt>::difference_type _Count2 = _Count / 2;
		_FwdIt _Mid = _First + _Count2;

		if ((*_Mid - _Val) < 0)
			_First = ++_Mid, _Count -= _Count2 + 1;
		else
			_Count = _Count2;
	}
	return (_First);
}

template<class _FwdIt, class _Ty> inline _FwdIt upper_bound(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
{
	// find first element not before _Val, using operator<
	typename std::iterator_traits<_FwdIt>::difference_type _Count = _Last - _First;

	for (; 0 < _Count; )
	{	// divide and conquer, find half that contains answer
		typename std::iterator_traits<_FwdIt>::difference_type _Count2 = _Count / 2;
		_FwdIt _Mid = _First + _Count2;

		if ((*_Mid - _Val) <= 0)
			_First = ++_Mid, _Count -= _Count2 + 1;
		else
			_Count = _Count2;
	}
	return (_First);
}

template<class _FwdIt, class _Ty> inline std::pair<_FwdIt, _FwdIt> equal_range(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
{	// find range equivalent to _Val, using operator<
	typename std::iterator_traits<_FwdIt>::difference_type _Count = _Last - _First;

	for (; 0 < _Count; )
	{	// divide and conquer, check midpoint
		typename std::iterator_traits<_FwdIt>::difference_type _Count2 = _Count / 2;
		_FwdIt _Mid = _First + _Count2;

		int nCmp = *_Mid - _Val;
		if (nCmp < 0) {	
			// range begins above _Mid, loop
			_First = ++_Mid;
			_Count -= _Count2 + 1;
		} 
		else if (nCmp > 0) {
			_Count = _Count2;	// range in first half, loop
		}
		else {	
			// range straddles mid, find each end and return
			_FwdIt _First2 = ::lower_bound(_First, _Mid, _Val);
			_First += _Count;
			_FwdIt _Last2 = ::upper_bound(++_Mid, _First, _Val);
			return (std::pair<_FwdIt, _FwdIt>(_First2, _Last2));
		}
	}

	return (std::pair<_FwdIt, _FwdIt>(_First, _First));	// empty range
}

template <typename T> void	minimize(T &left, const T& right) { if (right < left) left = right; }
template <typename T> void	maximize(T &left, const T& right) { if (right > left) left = right; }
template <typename T> void	minmaximize(T &left, const T& min, const T& max) 
{ 
	if (left > max) 
		left = max;
	else if (left < min)
		left = min;
}

class CReserveVoid
{
public:
	void	*m_pMemory;
	void	*m_pAlloc;
	size_t	m_nSize;

	CReserveVoid()
	{
		m_nSize = 0;
		m_pMemory = 0;
		m_pAlloc = 0;
	}

	CReserveVoid(size_t nSize, void *pMemory)
	{
		m_nSize = nSize;
		m_pMemory = pMemory;
		m_pAlloc = 0;
	}

	~CReserveVoid()
	{
		free(m_pAlloc);
	}

	void	Reserve(size_t nSize, void *pMemory)
	{
		free(m_pAlloc);
		m_nSize = nSize;
		m_pMemory = pMemory;
		m_pAlloc = 0;
	}

	void	*Alloc(size_t nSize)
	{
		if (nSize <= m_nSize)
			return m_pMemory;
		m_pMemory = m_pAlloc = realloc(m_pAlloc, nSize);
		m_nSize = nSize;
		return m_pMemory;
	}
};

template <typename TYPE> class CReserveMemory : public CReserveVoid
{
public:
	CReserveMemory() {}
	CReserveMemory(size_t nSize, TYPE *pMemory) : CReserveVoid(sizeof(TYPE)*nSize, pMemory) {}
	void	Reserve(size_t nSize, TYPE *pMemory) { CReserveVoid::Reserve(sizeof(TYPE)*nSize, pMemory); }
	TYPE* Alloc(size_t nSize) { return (TYPE*) CReserveVoid::Alloc(sizeof(TYPE)*nSize); }
	operator TYPE* () { return (TYPE*) m_pMemory; }
	TYPE * operator -> () { return (TYPE*) m_pMemory; }
};

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

