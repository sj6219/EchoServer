#pragma once

#define USE_SRWLOCK

#include "IOLib.h"
#include "Link.h"
#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <synchapi.h>
#include <atomic>

int	ExpandStringV(char * buffer, size_t count, const char * format, const char ** argptr);
int	ExpandStringV(wchar_t * buffer, size_t count, const wchar_t * format, const wchar_t **argptr);
int ExpandString(char * buffer, size_t count, const char *format, ...);
int ExpandString(wchar_t * buffer, size_t count, const wchar_t * format, ...);


class iless
{
public:
	bool operator()(const tstring& a, const tstring& b) const {
		return _tcsicmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const tstring& b) const {
		return _tcsicmp(a, b.c_str()) < 0;
	}
	bool operator()(const tstring& a, LPCTSTR b) const {
		return _tcsicmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return _tcsicmp(a, b) < 0;
	}
};

class StrCmp
{
public:
	bool operator()(const tstring& a, const tstring& b) const {
		return _tcscmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(LPCTSTR a, const tstring& b) const {
		return _tcscmp(a, b.c_str()) < 0;
	}
	bool operator()(tstring& a, LPCTSTR b) const {
		return _tcscmp(a.c_str(), b) < 0;
	}
	bool operator()(LPCTSTR a, LPCTSTR b) const {
		return _tcscmp(a, b) < 0;
	}
};


class XSpinLock
{
protected:
	long m_lock;

	void wait() noexcept;
public:
	XSpinLock() noexcept { m_lock = 0; }
	void Lock() noexcept
	{
		if (InterlockedCompareExchange(&m_lock, 1, 0))
			wait();
	}
	void Unlock() noexcept
	{
		InterlockedExchange(&m_lock, 0);
	}
	bool try_lock() noexcept
	{
		return InterlockedCompareExchange(&m_lock, 1, 0) == 0;
	}
};


class XRWLock
{
#ifdef USE_SRWLOCK
	SRWLOCK m_lock;
public:
	XRWLock() noexcept { InitializeSRWLock(&m_lock); }
	void Lock() noexcept { AcquireSRWLockExclusive(&m_lock); }
	void Unlock() noexcept { ReleaseSRWLockExclusive(&m_lock); }
	BOOL try_lock() noexcept { return TryAcquireSRWLockExclusive(&m_lock); }
	void LockShared() noexcept { AcquireSRWLockShared(&m_lock); }
	void UnlockShared() noexcept { ReleaseSRWLockShared(&m_lock);  }
#else
public:
	XRWLock();
	~XRWLock();
	void Lock();
	void Unlock();
	BOOL try_lock();
	void LockShared();
	void UnlockShared();

	void _Lock()
	{
		if (InterlockedCompareExchange(&m_nLock, 1, 0))
			Wait();
	}
	void _Unlock()
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
	XSharedLock(typename T& rT) noexcept : m_pT(&rT)
	{
		m_pT->LockShared();
	}
	~XSharedLock() noexcept
	{
		m_pT->UnlockShared();
	}
};

class XMemoryPage : public XLink
{
public:
	static const DWORD ALLOC_SIZE = 64 * 1024;
	static const DWORD PAGE_SIZE = 4 * 1024;

	static XMemoryPage* AllocPage();
	static void	FreePage(XMemoryPage* pPage);

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

	static const DWORD PAGE_SIZE = XMemoryPage::PAGE_SIZE;
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
			pPage = XMemoryPage::AllocPage();
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
			XMemoryPage::FreePage(pPage);
		}
		else {
			Unlock();
		}
	}

};

#pragma push_macro("new")
#undef new

template <int size>  typename XMemoryPool<size>  XMemoryPool<size>::s_self;

template <typename TYPE, typename ...Args> TYPE *XConstruct(Args... args)
{
	return new (XMemoryPool<sizeof(TYPE)>::Alloc()) TYPE(args...);
}

template <typename TYPE> void XDestruct(TYPE *ptr)
{
	ptr->~TYPE();
	XMemoryPool<sizeof(TYPE)>::Free(ptr);
}

#pragma pop_macro("new")





#define LPCPACKET LPCSTR
#define LPPACKET LPSTR

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

void LogPacket( int nType, int nSize, char* buffer);

#ifdef	UNICODE
std::wstring ToWString(LPCSTR lpa) noexcept;
std::string ToString(LPCWSTR lpw) noexcept;
#define AtoT(lpa) ToWString(lpa).c_str()
#define TtoA(lpw) ToString(lpw).c_str()
#else
#define AtoT(lpa) lpa
#define TtoA(lpw) lpw
#endif

tstring GetUniqueName() noexcept;
LPCTSTR	GetNamePart(LPCTSTR szPath) noexcept;
__forceinline LPTSTR	GetNamePart(LPTSTR szPath) noexcept { return (LPTSTR) GetNamePart((LPCTSTR) szPath); }


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

namespace Utility
{	
	extern const DWORD m_vCRC32Table[256];
}

inline DWORD	UpdateCRC(DWORD nCRC, DWORD nByte) { return Utility::m_vCRC32Table[(BYTE)(nCRC ^ nByte)] ^ (nCRC >> 8); }
DWORD	UpdateCRC(DWORD crc, void *buf, int len);
DWORD __stdcall EncryptCRC(DWORD crc, void* buf, int len);
DWORD __stdcall DecryptCRC(DWORD crc, void* buf, int len);
DWORD __stdcall EncodeCRC(DWORD crc, void* buf, int len);
DWORD __stdcall DecodeCRC(DWORD crc, void* buf, int len);
time_t GetTimeStamp() noexcept;

template<typename T> class LockfreeStack {
	std::atomic_int64_t m_top;
public:
	LockfreeStack() noexcept : m_top(0)
	{
	}
	void Push(T p) noexcept
	{
		_ASSERT(((__int64)p & 0xFF00000000000000) == 0);
		for (;;) {
			__int64 top = m_top.load(std::memory_order_consume);
			p->GetNext() = (T)(top & 0xFFFFFFFFFFFFFFll);
			if (m_top.compare_exchange_weak(top, (__int64)p + (top & 0xFF00000000000000ll), std::memory_order_release, std::memory_order_relaxed)) {
				break;
			}
		}
	}
	T Pop() noexcept
	{
		for (;;) {
			__int64 top = m_top.load(std::memory_order_consume);
			T p = (T)(top & 0xFFFFFFFFFFFFFFll);
			if (p == NULL)
				return NULL;
			if (m_top.compare_exchange_weak(top, (__int64)p->GetNext() + (top & 0xFF00000000000000ll) + 0x100000000000000ll, std::memory_order_release, std::memory_order_relaxed)) {
				return p;
				break;
			}
		}
	}
};

