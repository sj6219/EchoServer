#include "pch.h"
#include "Utility.h"
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <strsafe.h>
#include <malloc.h>
#include <iterator>

void XSpinLock::wait() noexcept
{
	int count = 4000;
	while (--count >= 0)
	{
		if (InterlockedCompareExchange(&m_lock, 1, 0) == 0)
			return;
#if !defined(_WIN64) && !defined(_M_ARM)
		__asm pause //__asm { rep nop} 
#endif // !defined(_WIN64) && !defined(_M_ARM)
	}
	count = 4000;
	while (--count >= 0)
	{
		SwitchToThread(); //Sleep(0);
		if (InterlockedCompareExchange(&m_lock, 1, 0) == 0)
			return;
	}
	for (; ; )
	{
		Sleep(1000);
		if (InterlockedCompareExchange(&m_lock, 1, 0) == 0)
			return;
	}
}

#ifndef USE_SRWLOCK

#pragma	warning(disable: 4146)	// unary minus operator applied to unsigned type, result still unsigned

// 0x80000000 Read  Thread wating for m_hWEvent
// 0x7f000000 Write Thread wating for m_hWEvent
// 0x00ff0000 Read Thread wating for m_hREvent
// 0x0000ff00 Active Write Lock
// 0x000000ff Active Read Lock
XRWLock::XRWLock()
{
	m_nCount = 0;
	m_nLock = 0;
	m_hREvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hWEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//	m_nRWCount = 0;
}

XRWLock::~XRWLock()
{
	ASSERT(m_nCount == 0);
	CloseHandle(m_hREvent);
	CloseHandle(m_hWEvent);
}

void XRWLock::Wait(/*int mode*/)
{
	int count;

	count = 4000;
	while (--count >= 0)
	{
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0)
			return;
#ifndef	_WIN64
		__asm pause //__asm { rep nop} 
#endif
	}
	count = 4000;
	while (--count >= 0)
	{
		SwitchToThread(); //Sleep(0);
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0)
			return;
	}
	for (; ; )
	{
		Sleep(1000);
		if (InterlockedCompareExchange(&m_nLock, 1, 0) == 0)
			return;
	}
}

void XRWLock::LockShared()
{
	//	InterlockedExchangeAdd(&m_nRWCount, 0x10000);
	int nCount;

	nCount = InterlockedIncrement(&m_nCount);
	if (nCount & 0xff00)
	{
		_Lock();
		nCount = m_nCount;
		do
		{
			if (nCount >= 0)
			{
				nCount = InterlockedExchangeAdd(&m_nCount, 0x80000000 - 1) + 0x80000000 - 1;
				if ((short)nCount == 0)
				{
					_Unlock();
					SetEvent(m_hWEvent);
				}
				else
					_Unlock();
				WaitForSingleObject(m_hWEvent, INFINITE);
				_Lock();
				nCount = InterlockedExchangeAdd(&m_nCount, -0x80000000 + 1) - 0x80000000 + 1;
			}
			else
			{
				nCount = InterlockedExchangeAdd(&m_nCount, 0x10000 - 1) + 0x10000 - 1;
				if ((short)nCount == 0)
				{
					_Unlock();
					SetEvent(m_hWEvent);
				}
				else
					_Unlock();
				WaitForSingleObject(m_hREvent, INFINITE);
				_Lock();
				nCount = InterlockedExchangeAdd(&m_nCount, -0x10000 + 1) - 0x10000 + 1;
			}
		} 		while ((nCount & 0xff00) != 0);

		if (nCount & 0xff0000)
		{
			_Unlock();
			SetEvent(m_hREvent);
		}
		else
			_Unlock();
	}
	//	InterlockedExchangeAdd(&m_nRWCount, -0x10000 + 1);
}

void XRWLock::UnlockShared()
{
	//	InterlockedExchangeAdd(&m_nRWCount, 0x100000 - 1);

	int nCount = InterlockedDecrement(&m_nCount);
	ASSERT((nCount & 0x8080) == 0);
	if ((nCount & 0xff000000) && (short)nCount == 0)
	{
		SetEvent(m_hWEvent);
	}
	//	InterlockedExchangeAdd(&m_nRWCount, -0x100000);
}

void XRWLock::Lock()
{
	int nCount;

	//	InterlockedExchangeAdd(&m_nRWCount, 0x1000000);
	nCount = InterlockedExchangeAdd(&m_nCount, 0x100);
	if ((short)nCount)
	{
		_Lock();
		do
		{
			nCount = InterlockedExchangeAdd(&m_nCount, 0x1000000 - 0x100) + 0x1000000 - 0x100;
			if ((short)nCount == 0)
			{
				_Unlock();
				SetEvent(m_hWEvent);
			}
			else
				_Unlock();
			WaitForSingleObject(m_hWEvent, INFINITE);
			_Lock();
			nCount = InterlockedExchangeAdd(&m_nCount, -0x1000000 + 0x100);

		} 		while ((short)nCount);
		_Unlock();
	}
	//	InterlockedExchangeAdd(&m_nRWCount, -0x1000000 + 0x10);
}

void XRWLock::Unlock()
{
	//	InterlockedExchangeAdd(&m_nRWCount, 0x10000000 - 0x10);
	int	nCount = InterlockedExchangeAdd(&m_nCount, -0x100) - 0x100;
	ASSERT((nCount & 0x8080) == 0);
	if ((nCount & 0xff000000) && (short)nCount == 0)
	{
		SetEvent(m_hWEvent);
	}
	//	InterlockedExchangeAdd(&m_nRWCount, -0x10000000);
}

BOOL XRWLock::try_lock()
{
	if ((short)InterlockedExchangeAdd(&m_nCount, 0x100))
	{
		InterlockedExchangeAdd(&m_nCount, -0x100);
		return FALSE;
	}
	return TRUE;
}

#endif

namespace Utility
{
	XMemoryPage *m_pFreePage;
	char	*m_pReserve;
	int		m_nReserve;
	int		m_nAllocPage;
#ifdef	_MT
	XLock		m_lockMemory;
	inline void	MemoryLock() { m_lockMemory.Lock(); }
	inline void	MemoryUnlock() { m_lockMemory.Unlock(); }
#else
	inline void	MemoryLock() {}
	inline void	MemoryUnlock() {}
#endif
}

#ifdef	_DEBUG
class XMemoryManager
{
public:
	~XMemoryManager()
	{
		_ASSERT(Utility::m_nAllocPage == 0);
	}

	static XMemoryManager s_self;
};

XMemoryManager XMemoryManager::s_self;
#endif

XMemoryPage* XMemoryPage::AllocPage()
{
	Utility::MemoryLock();
	XMemoryPage *pPage;
	if (Utility::m_pFreePage == 0) {
		if (Utility::m_nReserve == 0) {
			Utility::m_pReserve = (char *) VirtualAlloc(0, ALLOC_SIZE, MEM_RESERVE, PAGE_READWRITE);
			_ASSERT(((DWORD_PTR) Utility::m_pReserve & (PAGE_SIZE - 1)) == 0);
			Utility::m_nReserve = ALLOC_SIZE/PAGE_SIZE;
		}
		pPage = (XMemoryPage *)Utility::m_pReserve;
		Utility::m_pReserve += PAGE_SIZE;
		Utility::m_nReserve--;
		VirtualAlloc(pPage, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	else {
		pPage = Utility::m_pFreePage;
		Utility::m_pFreePage = (XMemoryPage *)Utility::m_pFreePage->m_pNext;
	}
	pPage->m_pFreeList = 0;
	pPage->m_nAlloc = 0;
	Utility::m_nAllocPage++;
	Utility::MemoryUnlock();
	return pPage;
}

void	XMemoryPage::FreePage(XMemoryPage *pPage)
{
	Utility::MemoryLock();
	Utility::m_nAllocPage--;
	pPage->m_pNext = (XLink *)Utility::m_pFreePage;
	Utility::m_pFreePage = pPage;
	Utility::MemoryUnlock();
}

//static char KTableA[21 * 19 * 2 + 1] =
//"가개갸걔거게겨계고과괘괴교구궈궤귀규그긔기"
//"까깨꺄꺠꺼께껴꼐꼬꽈꽤꾀꾜꾸꿔꿰뀌뀨끄끠끼"
//"나내냐냬너네녀녜노놔놰뇌뇨누눠눼뉘뉴느늬니"
//"다대댜걔더데뎌뎨도돠돼되됴두둬뒈뒤듀드듸디"
//"따때땨떄떠떼뗘뗴또똬뙈뙤뚀뚜뚸뛔뛰뜌뜨띄띠"
//"라래랴떄러레려례로롸뢔뢰료루뤄뤠뤼류르릐리"
//"마매먀먜머메며몌모뫄뫠뫼묘무뭐뭬뮈뮤므믜미"
//"바배뱌뱨버베벼볘보봐봬뵈뵤부붜붸뷔뷰브븨비"
//"빠빼뺘뺴뻐뻬뼈뼤뽀뽜뽸뾔뾰뿌뿨쀄쀠쀼쁘쁴삐"
//"사새샤섀서세셔셰소솨쇄쇠쇼수숴쉐쉬슈스싀시"
//"싸쌔쌰썌써쎄쎠쎼쏘쏴쐐쐬쑈쑤쒀쒜쒸쓔쓰씌씨"
//"아애야얘어에여예오와왜외요우워웨위유으의이"
//"자재쟈쟤저제져졔조좌좨죄죠주줘줴쥐쥬즈즤지"
//"짜째쨔쨰쩌쩨쪄쪠쪼쫘쫴쬐쬬쭈쭤쮀쮜쮸쯔쯰찌"
//"차채챠챼처체쳐쳬초촤쵀최쵸추춰췌취츄츠츼치"
//"카캐캬컈커케켜켸코콰쾌쾨쿄쿠쿼퀘퀴큐크킈키"
//"타태탸턔터테텨톄토톼퇘퇴툐투퉈퉤튀튜트틔티"
//"파패퍄퍠퍼페펴폐포퐈퐤푀표푸풔풰퓌퓨프픠피"
//"하해햐햬허헤펴혜호화홰회효후훠훼휘휴흐희히";

static wchar_t KTableW[21 * 19 + 1] =
L"가개갸걔거게겨계고과괘괴교구궈궤귀규그긔기"
L"까깨꺄꺠꺼께껴꼐꼬꽈꽤꾀꾜꾸꿔꿰뀌뀨끄끠끼"
L"나내냐냬너네녀녜노놔놰뇌뇨누눠눼뉘뉴느늬니"
L"다대댜걔더데뎌뎨도돠돼되됴두둬뒈뒤듀드듸디"
L"따때땨떄떠떼뗘뗴또똬뙈뙤뚀뚜뚸뛔뛰뜌뜨띄띠"
L"라래랴떄러레려례로롸뢔뢰료루뤄뤠뤼류르릐리"
L"마매먀먜머메며몌모뫄뫠뫼묘무뭐뭬뮈뮤므믜미"
L"바배뱌뱨버베벼볘보봐봬뵈뵤부붜붸뷔뷰브븨비"
L"빠빼뺘뺴뻐뻬뼈뼤뽀뽜뽸뾔뾰뿌뿨쀄쀠쀼쁘쁴삐"
L"사새샤섀서세셔셰소솨쇄쇠쇼수숴쉐쉬슈스싀시"
L"싸쌔쌰썌써쎄쎠쎼쏘쏴쐐쐬쑈쑤쒀쒜쒸쓔쓰씌씨"
L"아애야얘어에여예오와왜외요우워웨위유으의이"
L"자재쟈쟤저제져졔조좌좨죄죠주줘줴쥐쥬즈즤지"
L"짜째쨔쨰쩌쩨쪄쪠쪼쫘쫴쬐쬬쭈쭤쮀쮜쮸쯔쯰찌"
L"차채챠챼처체쳐쳬초촤쵀최쵸추춰췌취츄츠츼치"
L"카캐캬컈커케켜켸코콰쾌쾨쿄쿠쿼퀘퀴큐크킈키"
L"타태탸턔터테텨톄토톼퇘퇴툐투퉈퉤튀튜트틔티"
L"파패퍄퍠퍼페펴폐포퐈퐤푀표푸풔풰퓌퓨프픠피"
L"하해햐햬허헤펴혜호화홰회효후훠훼휘휴흐희히";

// Code point of Hangul = tail + (vowel-1)*28 + (lead-1)*588 + 44032

int	ExpandStringV(char* buffer, size_t count, const char* format, const char** argptr)
{
	if (count <= 0)
		return -1;

	char* ptr = buffer;
	char* end_ptr = (char*)((char*)buffer + count);
	for (; *format && ptr != end_ptr; ) {
		if (*format == '%') {
			format++;
			if (*format == '%') {
				*ptr++ = '%';
				format++;
			}
			else if (*format == 'n') {
				*ptr++ = '\n';
				format++;
			}
			else if (*format == '0') {
				*ptr++ = 0;
				format++;
			}
			else if (isdigit(*format)) {
				int nDigit = *format++ - '1';
				char szForm[64];
				const char* pszForm;
				if (*format == '!') {
					char* pForm = szForm;
					*pForm++ = '%';
					format++;
					for (; *format != '!'; ) {
						if (*format == 0)
							goto skip;
						if (pForm != (char*)((char*)szForm + sizeof(szForm)))
							*pForm++ = *format++;
					}
					format++;
					if (pForm == (char*)((char*)szForm + sizeof(szForm)))
						goto skip;
					*pForm++ = 0;
					pszForm = szForm;
				}
				else {
					pszForm = "%s";
				}
				char* last_ptr;
				if ((nDigit = sprintf_s(ptr, end_ptr - ptr, pszForm, argptr[nDigit])) < 0)
					goto skip;
				last_ptr = ptr + nDigit;
				if (*format == '%') {
					format++;
					if (last_ptr == end_ptr)
						goto skip;
					*last_ptr++ = 0;
					for (; ; ) {
						ptr += strcspn(ptr, "\"");
						if (*ptr == 0)
							break;
						if (last_ptr == end_ptr)
							goto skip;
						memmove(ptr + 1, ptr, last_ptr - ptr);
						last_ptr++;
						ptr += 2;
					}
				}
				else
					ptr = last_ptr;
				if (*format == ';') {
					format++;
					goto skip;
				}
				//if (nDigit < 1 || end_ptr - ptr < 1)
				//	goto skip;
				//const char* szSuffix[2];
				//if (strncmp(format, "이", 2) == 0 || strncmp(format, "가", 2) == 0) {
				//	szSuffix[0] = "가";
				//	szSuffix[1] = "이";
				//}
				//else if (strncmp(format, "을", 2) == 0 || strncmp(format, "를", 2) == 0) {
				//	szSuffix[0] = "를";
				//	szSuffix[1] = "을";
				//}
				//else if (strncmp(format, "은", 2) == 0 || strncmp(format, "는", 2) == 0) {
				//	szSuffix[0] = "는";
				//	szSuffix[1] = "은";
				//}
				//else
				//	goto skip;
				//if (wmemchr((wchar_t*)KTableA, *(WORD*)(ptr -1), 21 * 19) != 0)
				//	memcpy(ptr, szSuffix[0], 2);
				//else
				//	memcpy(ptr, szSuffix[1], 2);
				//ptr += 2;
				//format += 2;
			skip:
				;
			}
		}
		else {
			*ptr++ = *format++;
		}
	}
	if (ptr == end_ptr)
		--ptr;
	*ptr = 0;
	return (int)(ptr - buffer);

}

int	ExpandStringV(wchar_t* buffer, size_t count, const wchar_t* format, const wchar_t** argptr)
{
	if (count <= 0)
		return -1;

	wchar_t* ptr = buffer;
	wchar_t* end_ptr = (wchar_t*)((char*)buffer + count);
	for (; *format && ptr != end_ptr; ) {
		if (*format == '%') {
			format++;
			if (*format == '%') {
				*ptr++ = '%';
				format++;
			}
			else if (*format == 'n') {
				*ptr++ = '\n';
				format++;
			}
			else if (*format == '0') {
				*ptr++ = 0;
				format++;
			}
			else if (isdigit(*format)) {
				int nDigit = *format++ - '1';
				wchar_t szForm[64];
				const wchar_t* pszForm;
				if (*format == '!') {
					wchar_t* pForm = szForm;
					*pForm++ = '%';
					format++;
					for (; *format != '!'; ) {
						if (*format == 0)
							goto skip;
						if (pForm != (wchar_t*)((char*)szForm + sizeof(szForm)))
							*pForm++ = *format++;
					}
					format++;
					if (pForm == (wchar_t*)((char*)szForm + sizeof(szForm)))
						goto skip;
					*pForm++ = 0;
					pszForm = szForm;
				}
				else {
					pszForm = L"%s";
				}
				wchar_t* last_ptr;
				if ((nDigit =swprintf_s(ptr, end_ptr - ptr, pszForm, argptr[nDigit])) < 0)
					goto skip;
				last_ptr = ptr + nDigit;
				if (*format == '%') {
					format++;
					if (last_ptr == end_ptr)
						goto skip;
					*last_ptr++ = 0;
					for (; ; ) {
						ptr += wcscspn(ptr, L"\"");
						if (*ptr == 0)
							break;
						if (last_ptr == end_ptr)
							goto skip;
						wmemmove(ptr + 1, ptr, last_ptr - ptr);
						last_ptr++;
						ptr += 2;
					}
				}
				else
					ptr = last_ptr;
				if (*format == ';') {
					format++;
					goto skip;
				}
				if (nDigit < 1 || (char*)end_ptr - (char*)ptr < 2)
					goto skip;
				wchar_t szSuffix[2];
				if (*format == L'이' || *format == L'가') {
					szSuffix[0] = L'가';
					szSuffix[1] = L'이';
				}
				else if (*format == L'을' || *format == L'를') {
					szSuffix[0] = L'를';
					szSuffix[1] = L'을';
				}
				else if (*format == L'은' || *format == L'는') {
					szSuffix[0] = L'는';
					szSuffix[1] = L'은';
				}
				else
					goto skip;
				if ((ptr[-1] - 44032) % 28 == 0)
					*ptr = szSuffix[0];
				else
					*ptr = szSuffix[1];
				if (wmemchr((wchar_t*)KTableW, *(WORD*)(ptr - 1), 21 * 19) != 0)
					*ptr = szSuffix[0];
				else
					*ptr = szSuffix[1];
				ptr++;
				format++;
			skip:
				;
			}
		}
		else {
			*ptr++ = *format++;
		}
	}
	if (ptr == end_ptr)
		--ptr;
	*ptr = 0;
	return (int)(ptr - buffer);

}


int ExpandString(char *buffer, size_t count, const char *format, ...)
{
	va_list	valist;
	va_start(valist, format);
	int n = ExpandStringV(buffer, count, format, (const char **) valist);
	va_end(valist);
	return n;
}

int ExpandString(wchar_t * buffer, size_t count, const wchar_t * format, ...)
{
	va_list	valist;
	va_start(valist, format);
	int n = ExpandStringV(buffer, count, format, (const wchar_t **) valist);
	va_end(valist);
	return n;
}

#if	0
__declspec(naked) BOOL __fastcall _interlockedbittestandset(DWORD *pBit, int nOffset)
{
	__asm
	{
//		mov	ecx, dword ptr [pBit]
//		mov	edx, nOffset
		lock bts	dword ptr [ecx], edx	
		jnc __false
		mov eax, 1
		ret 0
__false:
		xor eax, eax
		ret 0
	}
}

__declspec(naked) BOOL __fastcall _interlockedbittestandreset(DWORD *pBit, int nOffset)
{
	__asm
	{
//		mov	ecx, dword ptr [pBit]
//		mov	edx, nOffset
		lock btr	dword ptr [ecx], edx	
		jnc __false
		mov eax, 1
		ret 0
__false:
		xor eax, eax
		ret 0
	}
}
#endif

char *__stdcall WritePacketV(char *packet, va_list va) 
{
	char *format = va_arg(va, char *);
	for ( ; ; )
	{
		switch (*format++)
		{
			case 'b':
			case 'B':
				*packet++ = va_arg(va, char);
				break;
			case 'w':
			case 'W':
				*(WORD *)packet = va_arg(va, WORD);
				packet += sizeof(WORD);
				break;
			case 'd':
				*(DWORD *)packet = va_arg(va, DWORD);
				packet += sizeof(DWORD);
				break;
			case 'I':	//	__int64
				*(__int64 *)packet = va_arg(va, __int64);
				packet += sizeof(__int64);
				break;
			case 's':
			case 'S':
				{
					LPTSTR str = va_arg(va, LPTSTR);
					if (str == 0)
					{
						*(LPTSTR)packet = 0;
						packet += sizeof(TCHAR);
						break;
					}
					for ( ; (*(LPTSTR)packet = *str++) != 0; )
						packet += sizeof(TCHAR);
					packet += sizeof(TCHAR);
				}
				break;
			case 'm':
			case 'M':
				{
					char *ptr = va_arg(va, char *);
					int	size = va_arg(va, int);
					memcpy(packet, ptr, size);
					packet += size;
				}
				break;
			case 0:
				return packet;
			default:
				_ASSERT(0);
		}
	}
}

char *WritePacket(char *packet, const char *, ...)
{
	va_list va;
	va_start(va, packet);
	packet = WritePacketV(packet, va);
	va_end(va);
	return packet;
}

LPCPACKET __stdcall ReadPacketV(LPCPACKET packet, va_list va)
{
	char *format = va_arg(va, char *);
	for ( ; ; )
	{
		switch (*format++)
		{
			case 'b':
				*va_arg(va, char *) = *packet++;
				break;
			case 'B':
				*va_arg(va, int *) = *packet++;
				break;
			case 'w':
				*va_arg(va, WORD *) = *(WORD *)packet;
				packet += sizeof(WORD);
				break;
			case 'W':
				*va_arg(va, int *) = *(WORD *)packet;
				packet += sizeof(WORD);
				break;
			case 'd':
				*va_arg(va, DWORD *) = *(DWORD *)packet;
				packet += sizeof(DWORD);
				break;
			case 'I':	//	__int64
				*va_arg(va, __int64 *) = *(__int64 *)packet;
				packet += sizeof(__int64);
				break;
			case 's':
				{
					LPTSTR str = va_arg(va, LPTSTR);
					int size = va_arg(va, int);
					_ASSERT(size >= sizeof(TCHAR));
					LPCPACKET last = packet + size - sizeof(TCHAR) * 2;
					for ( ; ; )
					{
						if (packet > last) {
							_ASSERT(*(LPTSTR) packet == 0);
							packet += sizeof(TCHAR);
							*str = 0;
							break;
						}
						if ((*str++ = *(LPTSTR) packet) == 0) {
							packet += sizeof(TCHAR);
							break;
						}
						packet += sizeof(TCHAR);
					}
				}
				break;
			case 'm':
				{
					char *ptr = va_arg(va, char *);
					int	size = va_arg(va, int);
					memcpy(ptr, packet, size);
					packet += size;
				}
				break;
			case 'M':
				{
					LPCPACKET* ptr = va_arg(va, LPCPACKET *);
					int	size = va_arg(va, int);
					*ptr = packet;
					packet += size;
				}
				break;
			case 0:
				return packet;
			default:
				_ASSERT(0);
		}
	}
}

LPCPACKET ReadPacket(LPCPACKET packet, const char *, ...)
{
	va_list va;
	va_start(va, packet);
	packet = ReadPacketV(packet, va);
	va_end(va);
	return packet;
}

LPCPACKET __stdcall ScanPacketV(LPCPACKET packet, LPCPACKET end, va_list va)
{
	char *format = va_arg(va, char *);
	for ( ; ; )
	{
		switch (*format++)
		{
			case 'b':
				if (packet < end)
					*va_arg(va, char *) = *packet++;
				break;
			case 'B':
				if (packet < end)
					*va_arg(va, int *) = *packet++;
				break;
			case 'w':
				if (packet + sizeof(WORD) <= end)
				{
					*va_arg(va, WORD *) = *(WORD *)packet;
					packet += sizeof(WORD);
				}
				break;
			case 'W':
				if (packet + sizeof(WORD) <= end)
				{
					*va_arg(va, int *) = *(WORD *)packet;
					packet += sizeof(WORD);
				}
				break;
			case 'd':
				if (packet + sizeof (DWORD) <= end)
				{
					*va_arg(va, DWORD *) = *(DWORD *)packet;
					packet += sizeof(DWORD);
				}
				break;
			case 'I':	//	__int64
				if (packet + sizeof(__int64) <= end)
				{
					*va_arg(va, __int64 *) = *(__int64 *)packet;
					packet += sizeof(__int64);
				}
				break;
			case 's':
				{
					LPTSTR str = va_arg(va, LPTSTR);
					int size = va_arg(va, int);
					_ASSERT(size >= sizeof(TCHAR));
					LPCPACKET last = std::min<LPCPACKET>(packet + size, end) - sizeof(TCHAR) * 2;
					for ( ; ; )
					{
						if (packet > last) {
							_ASSERT(*(LPTSTR) packet == 0);
							packet += sizeof(TCHAR);
							*str = 0;
							break;
						}
						if ((*str++ = *(LPTSTR) packet) == 0) {
							packet += sizeof(TCHAR);
							break;
						}
						packet += sizeof(TCHAR);
					}
				}
				break;
			case 'm':
				{
					char *ptr = va_arg(va, char *);
					int	size = va_arg(va, int);
					if (packet + size <= end)
					{
						memcpy(ptr, packet, size);
						packet += size;
					}
				}
				break;
			case 0:
				return packet;
			default:
				_ASSERT(0);
		}
	}
}

LPCPACKET ScanPacket(LPCPACKET packet, LPCPACKET end, const char *, ...)
{
	va_list va;
	va_start(va, end);
	packet = ScanPacketV(packet, end, va);
	va_end(va);
	return packet;
}

LPCPACKET __stdcall GetString(LPCPACKET packet, LPTSTR str, int size)
{
	_ASSERT(size >= sizeof(TCHAR));
	LPCPACKET last = packet + size - sizeof(TCHAR) * 2;
	for ( ; ; )
	{
		if (packet > last) {
			_ASSERT(*(LPTSTR) packet == 0);
			packet += sizeof(TCHAR);
			*str = 0;
			break;
		}
		if ((*str++ = *(LPTSTR) packet) == 0) {
			packet += sizeof(TCHAR);
			break;
		}
		packet += sizeof(TCHAR);
	}
	return packet;
}

LPCPACKET __stdcall ScanString(LPCPACKET packet, LPCPACKET end, LPTSTR str, int size)
{
	_ASSERT(size >= sizeof(TCHAR));
	LPCPACKET last = std::min<LPCPACKET>(packet + size, end) - sizeof(TCHAR) * 2;
	for ( ; ; )
	{
		if (packet > last) {
			_ASSERT(*(LPTSTR) packet == 0);
			packet += sizeof(TCHAR);
			*str = 0;
			break;
		}
		if ((*str++ = *(LPTSTR) packet) == 0) {
			packet += sizeof(TCHAR);
			break;
		}
		packet += sizeof(TCHAR);
	}
	return packet;
}

char *__stdcall PutString(char *packet, LPCTSTR str)
{
	LPTSTR _packet = (LPTSTR) packet;
	for ( ; (*_packet++ = *str++) != 0; ) 
		;
	return (char *) _packet;
}

char *__stdcall PutBuffer(char *packet, LPCTSTR str, int size)
{
	packet = PutNumeric(packet, size);
	LPTSTR _packet = (LPTSTR) packet;
	while (size-- > 0)
		*_packet++ = *str++;;
	return (char *) _packet;
}

BOOL CreatePath(LPCTSTR szPath)
{
	TCHAR szParent[MAX_PATH];

	LPCTSTR str = _tcsrchr(szPath, '\\');
	if (str == 0)
		return FALSE;
	_tcsncpy_s(szParent, szPath, str - szPath);
	if (CreateDirectory(szParent, NULL))
		return TRUE;
	if (GetLastError() != ERROR_PATH_NOT_FOUND)
		return FALSE;
	if (CreatePath(szParent))
		return CreateDirectory(szParent, NULL);
	else
		return FALSE;
}

BOOL MovePath(LPCTSTR szOldFile, LPCTSTR szNewFile)
{
	if (MoveFileEx(szOldFile, szNewFile, MOVEFILE_REPLACE_EXISTING))
		return TRUE;
	if (GetLastError() != ERROR_PATH_NOT_FOUND)
		return FALSE;
	if (CreatePath(szNewFile))
		return MoveFileEx(szOldFile, szNewFile, MOVEFILE_REPLACE_EXISTING);
	else
		return FALSE;
}

BOOL DeletePath(LPCTSTR ofname)
{
    struct _stat ostat; /* stat for ofname */
    if (_tstat(ofname, &ostat) != 0)
		return TRUE;
    (void)_tchmod(ofname, 0777);
	return (_tunlink(ofname) == 0);
}


void LogPacket(int nType, int nSize, char *buffer)
{
	LOG(_T("LogPacket nType(%d) nSize(%d)\n"), nType, nSize);
	char temp[32 * 3 + 10];
	if (nType == -1)
	{
		if (nSize > 1024)
			nSize = 1024;
	}
	else
	{
		if (nSize > 64)
			nSize = 64;
	}
	char *bufptr = buffer;
	for ( ; nSize > 0; )
	{
		char *ptr = temp;
		size_t size = sizeof(temp);
		for (int i = 0; nSize > 0 && i < 32; )
		{
			StringCbPrintfExA(ptr, size, &ptr, &size, 0, "%02x", *bufptr++ & 0xff);
			nSize--;
			i++;
			if ((i & 3) == 0)
			{
				StringCbPrintfExA(ptr, size, &ptr, &size, 0, " ");
			}
		}
		LOG(_T("%p: %s\n"), buffer, temp);
		buffer = bufptr;
	}
}

#ifdef	UNICODE
std::wstring ToWString(LPCSTR lpa) noexcept
{
	int len = (int) strlen(lpa) + 1;
	LPWSTR lpw = (LPWSTR) _alloca(len * 2);
	len = MultiByteToWideChar(CP_UTF8, 0, lpa, len, lpw, len);
	return lpw;
}

std::string ToString(LPCWSTR lpw) noexcept
{
	int len = (int) wcslen(lpw)+1;
	LPSTR lpa = (LPSTR) _alloca(len * 4);
	len = WideCharToMultiByte(CP_UTF8, 0, lpw, len, lpa, len * 4, 0, 0);
	return lpa;
}
#endif

String GetUniqueName() noexcept
{
	TCHAR path[MAX_PATH+1];
	GetModuleFileName(0, path, MAX_PATH);
	for (TCHAR *p = path; *p; p++) {
		if (*p == '\\')
			*p = '/';
		else 
			*p = tolower(*p);
	}
	return path;
}

LPCTSTR GetNamePart(LPCTSTR szPath) noexcept
{
	Str slice(szPath);
	Str<const TCHAR> s(_T("\\/"));

	return slice.rfind(_T("\\/"));
}

const DWORD Utility::m_vCRC32Table[] =
{
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};


DWORD	UpdateCRC(DWORD crc, void *buf, int len)
{
	BYTE *end = (BYTE *) buf + len;
	for (BYTE *ptr = (BYTE *) buf; ptr != end; ) {
		crc = Utility::m_vCRC32Table[(BYTE)(crc ^ *ptr++)] ^ (crc >> 8);
    }
	return crc;
}

DWORD __stdcall EncryptCRC(DWORD crc, void* buf, int len)
{
	BYTE *end = (BYTE *)buf + len;
	for (BYTE *ptr = (BYTE *) buf; ptr != end; ) {
		BYTE _xor = (BYTE) crc ^ *ptr;
		*ptr++ = _xor;
		crc = Utility::m_vCRC32Table[_xor] ^ _rotr(crc, 8);
	}
	return crc;
}

DWORD __stdcall DecryptCRC(DWORD crc, void* buf, int len)
{
	BYTE *end = (BYTE *)buf + len;
	for (BYTE *ptr = (BYTE *) buf; ptr != end; ) {
		BYTE _xor = *ptr;
		*ptr++ = (BYTE) crc ^ _xor;
		crc = Utility::m_vCRC32Table[_xor] ^ _rotr(crc, 8);
	}
	return crc;
}

DWORD __stdcall EncodeCRC(DWORD crc, void* buf, int len)
{
	// obsolete function replaced by EncryptCRC
	BYTE *end = (BYTE *)buf + len;
	for (BYTE *ptr = (BYTE *) buf; ptr != end; ) {
		crc ^= * (BYTE *) ptr;
		*ptr++ = (BYTE) crc;
		crc = Utility::m_vCRC32Table[(BYTE)crc] ^ (crc >> 8);
	}
	return crc;
}

DWORD __stdcall DecodeCRC(DWORD crc, void* buf, int len) 
{
	// obsolete function replaced by DecryptCRC
	BYTE *end = (BYTE *)buf + len;
	for (BYTE *ptr = (BYTE *) buf; ptr != end; ) {
		DWORD nByte = *ptr;
		*ptr++ = (BYTE) (crc ^ nByte);
		crc = Utility::m_vCRC32Table[nByte] ^ (crc >> 8);
	}
	return crc;
}

time_t GetTimeStamp() noexcept
{
	HMODULE hModule = GetModuleHandle(0);
	if (hModule == 0)
		return 0;
	IMAGE_DOS_HEADER *DosHeader = (IMAGE_DOS_HEADER *)hModule;
	if (IMAGE_DOS_SIGNATURE != DosHeader->e_magic)
		return 0;
	IMAGE_NT_HEADERS *NTHeader = (IMAGE_NT_HEADERS *)((char *)DosHeader
								+ DosHeader->e_lfanew);
	if (IMAGE_NT_SIGNATURE != NTHeader->Signature)
		return 0;
	return NTHeader->FileHeader.TimeDateStamp;
}


