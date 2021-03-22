
#include "pch.h"
#include <cstdlib>
#include "IOMemory.h"
#include "IOException.h"

#pragma warning(disable: 4074) 
#pragma init_seg(compiler)

//#define DEBUG_LEAK 0
static XIOMemory::CInit  theInit;

#define MEMORY_HEAP_SIZE 16

#ifdef	DEBUG_LEAK
#define MEMORY_HEAP_SIZE 1
#define IO_HEAP
#endif
#ifndef _DEBUG
static XIOMemory g_memory[MEMORY_HEAP_SIZE]; 
static long g_nMemoryCount;
#endif

XIOMemory::CInit::CInit()
{
#ifdef	_DEBUG
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpFlag);
#endif
}

XIOMemory::CInit::~CInit()
{
#if 	!defined(_DEBUG) && !defined(DEBUG_LEAK)
	TCHAR buf[128];
	_stprintf_s(buf,  _T("Memory Total Size = %td\n"), XIOMemory::s_nTotalSize);
	OutputDebugString(buf);
	if (XIOMemory::s_nTotalSize && IsDebuggerPresent()) 
		DebugBreak();
#endif
}

void XIOMemory::Touch()
{

}
 
#ifndef	_DEBUG
#define KEY	0x8769e100

#ifdef	_WIN64

#define ENTRY_OFFSET        0x0000001cL     
#define OVERHEAD_PER_PAGE   0x00000020L     
#define MAX_FREE_ENTRY_SIZE (BYTES_PER_PAGE - OVERHEAD_PER_PAGE)
#define BITV_COMMIT_INIT    (((1 << GROUPS_PER_REGION) - 1) << \
											(32 - GROUPS_PER_REGION))
#define MAX_ALLOC_DATA_SIZE     0x7f8
#define MAX_ALLOC_ENTRY_SIZE    (MAX_ALLOC_DATA_SIZE + 2 * sizeof(int)) 
#define INTERLOCKEDEXCHANGEADD	_InterlockedExchangeAdd64

#define PARA_SHIFT			5	
#define BYTES_PER_PARA      32	
#define BYTES_PER_PAGE      (BYTES_PER_PARA * PARAS_PER_PAGE)
#define BYTES_PER_GROUP     (BYTES_PER_PAGE * PAGES_PER_GROUP)
#define BYTES_PER_REGION    (BYTES_PER_GROUP * GROUPS_PER_REGION)

#else

#define ENTRY_OFFSET        0x0000000cL     
#define OVERHEAD_PER_PAGE   0x00000010L     
#define MAX_FREE_ENTRY_SIZE (BYTES_PER_PAGE - OVERHEAD_PER_PAGE)
#define BITV_COMMIT_INIT    (((1 << GROUPS_PER_REGION) - 1) << \
											(32 - GROUPS_PER_REGION))
#define MAX_ALLOC_DATA_SIZE     0x3f8
#define MAX_ALLOC_ENTRY_SIZE    (MAX_ALLOC_DATA_SIZE + 2 * sizeof(int)) 
#define INTERLOCKEDEXCHANGEADD	_InterlockedExchangeAdd

#define PARA_SHIFT			4	
#define BYTES_PER_PARA      16	
#define BYTES_PER_PAGE      (BYTES_PER_PARA * PARAS_PER_PAGE)
#define BYTES_PER_GROUP     (BYTES_PER_PAGE * PAGES_PER_GROUP)
#define BYTES_PER_REGION    (BYTES_PER_GROUP * GROUPS_PER_REGION)

#endif

LONG_PTR XIOMemory::s_nTotalSize;

XIOMemory::XIOMemory()
{
	InitializeCriticalSection(&_lock);
	__sbh_indGroupDefer = 0;
	_heap_init();
}

XIOMemory::~XIOMemory()
{
#if	  defined(DEBUG_LEAK) 
	{
		HeapLock(_crtheap);
        PROCESS_HEAP_ENTRY entry;
		entry.lpData = 0;
        entry.wFlags = 0;
        entry.iRegionIndex = 0;
		for ( ; ; ) {
			if (!HeapWalk(_crtheap, &entry))
				break;
			if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
				HEAP_HEADER *pHeader = (HEAP_HEADER *) entry.lpData;
				if ((pHeader->key & -MEMORY_HEAP_SIZE) == KEY) {
					TCHAR buff[128];
					_stprintf_s(buff, sizeof(buff)/sizeof(TCHAR), _T("Unfree %p %d %d\n"), pHeader+1, pHeader->size, entry.cbData);
					OutputDebugString(buff);
				}
			}
		}
		HeapUnlock(_crtheap);
	}
#endif
	HeapDestroy(_crtheap);
}

__forceinline void *	XIOMemory::_heap_alloc_base(size_t size)
{

#ifndef	IO_HEAP
	void * pvReturn;
	if ( size <= __sbh_threshold )
	{
		_mlock();
		pvReturn = __sbh_alloc_block((int) size);
		if (pvReturn) {
			_munlock();
			return pvReturn;
		}
		_munlock();
	}
#endif

	return HeapAlloc(_crtheap, 0, size);
}

void XIOMemory::_free(void * pBlock)
{
	if (pBlock == NULL)
		return;
	HEAP_HEADER	*pHeader = ((HEAP_HEADER *)pBlock) - 1;
	HEAP_HEADER header = *pHeader;
	INTERLOCKEDEXCHANGEADD(&s_nTotalSize, -(INT_PTR)header.size);
#if		DEBUG_LEAK
	if (pHeader+1 == (HEAP_HEADER *) DEBUG_LEAK) {
		TCHAR buff[512];
		_stprintf_s(buff,  _T("free %p %d\n"), pHeader+1, header.size);
		OutputDebugString(buff);
	}
#endif

	if ((header.key & -MEMORY_HEAP_SIZE) != KEY) {
		EBREAK();
		return;
	}
	memset(pHeader, 0xfd, pHeader->size + sizeof(HEAP_HEADER));
	g_memory[header.key & (MEMORY_HEAP_SIZE-1)]._free_base(pHeader);
}

void	XIOMemory::_free_base(void * pBlock)
{
#ifndef	IO_HEAP
	PHEADER     pHeader;
	_mlock();
	
	if ((pHeader = __sbh_find_block(pBlock)) != NULL) {
		__sbh_free_block(pHeader, pBlock);
		_munlock();
		return;
	}
	_munlock();
#endif
	{
		if (!HeapFree(_crtheap, 0, pBlock)) {
			MEMORYSTATUS	ms;
			GlobalMemoryStatus(&ms);
			ELOG(_T("Memory::_free() failed : total=%d, phys=%d, virtual=%d"),  
				XIOMemory::s_nTotalSize, ms.dwAvailPhys, ms.dwAvailVirtual); 
			BOOL bValidate = HeapValidate(_crtheap, 0, 0);
			ELOG(_T("HeapValidate = %d"), bValidate);
			EBREAK();
		}
	}
}

void *	XIOMemory::_realloc_base(void * pBlock, size_t newsize)
{
    {
        {
#ifndef	IO_HEAP
			PHEADER     pHeader;
			size_t      oldsize;
			void *      pvReturn = NULL;
            _mlock();
            if ((pHeader = __sbh_find_block(pBlock)) != NULL)
            {
                if (newsize <= __sbh_threshold)
                {
                    if (__sbh_resize_block(pHeader, pBlock, (int) newsize))
                        pvReturn = pBlock;
                    else if ((pvReturn = __sbh_alloc_block((int) newsize)) != NULL)
                    {
                        oldsize = ((PENTRY)((char *)pBlock -
                                            sizeof(int)))->sizeFront - sizeof(int)*2 -1;
                        memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                        pHeader = __sbh_find_block(pBlock);
                        __sbh_free_block(pHeader, pBlock);
                    }
                }
                if (pvReturn == NULL)
                {
                    if (newsize == 0)
                        newsize = 1;
                    newsize = (newsize + BYTES_PER_PARA - 1) &
                              ~(BYTES_PER_PARA - 1);
                    if ((pvReturn = HeapAlloc(_crtheap, 0, newsize)) != NULL)
                    {
                        oldsize = ((PENTRY)((char *)pBlock -
                                            sizeof(int)))->sizeFront - sizeof(int)*2-1;
                        memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                        __sbh_free_block(pHeader, pBlock);
                    }
                }
				_munlock();
				return pvReturn;
            }
            _munlock();
#endif
            {
                return HeapReAlloc(_crtheap, 0, pBlock, newsize);
            }
        }
    }
}

void *	XIOMemory::_expand_base(void * pBlock, size_t newsize)
{
    /* validate size */
#ifndef	IO_HEAP
    PHEADER     pHeader;
    void *      pvReturn;
    _mlock();
    if ((pHeader = __sbh_find_block(pBlock)) != NULL)
    {
        pvReturn = NULL;
        if ( (newsize <= __sbh_threshold) && __sbh_resize_block(pHeader, pBlock, (int) newsize) ) {
			_munlock();
            return pBlock;
		}
    }
    _munlock();
#endif
    {
        return HeapReAlloc(_crtheap, HEAP_REALLOC_IN_PLACE_ONLY,
                               pBlock, newsize);
    }
}

XIOMemory::PHEADER XIOMemory::__sbh_find_block (void * pvAlloc)
{
		PHEADER         pHeaderLast = __sbh_pHeaderList + __sbh_cntHeaderList;
		PHEADER         pHeader;
		UINT_PTR    offRegion;

		pHeader = __sbh_pHeaderList;
		while (pHeader < pHeaderLast)
		{
			offRegion = (UINT_PTR) pvAlloc - (UINT_PTR)pHeader->pHeapData;
			if (offRegion < BYTES_PER_REGION)
				return pHeader;
			pHeader++;
		}
		return NULL;
}

XIOMemory::PHEADER XIOMemory::__sbh_alloc_new_region()
{
	PHEADER     pHeader;
	if (__sbh_cntHeaderList == __sbh_sizeHeaderList)
	{
		if (!(pHeader = (PHEADER)HeapReAlloc(_crtheap, 0, __sbh_pHeaderList,
							(__sbh_sizeHeaderList + 16) * sizeof(HEADER))))
			return NULL;
		__sbh_pHeaderList = pHeader;
		__sbh_sizeHeaderList += 16;
	}
	pHeader = __sbh_pHeaderList + __sbh_cntHeaderList;
	if (!(pHeader->pRegion = (PREGION)HeapAlloc(_crtheap, HEAP_ZERO_MEMORY,
									sizeof(REGION))))
		return NULL;
	if ((pHeader->pHeapData = VirtualAlloc(0, BYTES_PER_REGION,
									 MEM_RESERVE, PAGE_READWRITE)) == NULL)
	{
		HeapFree(_crtheap, 0, pHeader->pRegion);
		return NULL;
	}
	pHeader->bitvEntryHi = 0;
	pHeader->bitvEntryLo = 0;
	pHeader->bitvCommit = BITV_COMMIT_INIT;
	
	__sbh_cntHeaderList++;
	pHeader->pRegion->indGroupUse = -1;
	return pHeader;
}

size_t	XIOMemory::__msize(void * pBlock)
{
    if (pBlock == NULL)
        return 0;
	HEAP_HEADER	*pHeader = ((HEAP_HEADER *)pBlock) - 1;
	return pHeader->size;
}

void * operator new( size_t cb)
{
	return malloc( cb);
}

void operator delete( void * p)
{
    free( p );
}

void* operator new[](size_t cb)
{
    return malloc( cb);
}

void operator delete[](void* p)
{
    return free( p);
}

int	XIOMemory::__sbh_resize_block (PHEADER pHeader, void * pvAlloc, int intNew)
{
    PREGION         pRegion;
    PGROUP          pGroup;
    PENTRY          pHead;
    PENTRY          pEntry;
    PENTRY          pNext;
    int				sizeEntry;
    int				sizeNext;
    int				sizeNew;
    unsigned int    indGroup;
    unsigned int	indEntry;
    unsigned int	indNext;
    unsigned int    offRegion;
    
    sizeNew = (intNew + 2 * (int) sizeof(int) + (BYTES_PER_PARA - 1))
                                       & ~(BYTES_PER_PARA - 1);
    pRegion = pHeader->pRegion;

    offRegion = (unsigned int)((UINT_PTR)pvAlloc - (UINT_PTR)pHeader->pHeapData);
    indGroup = offRegion / BYTES_PER_GROUP;
    pGroup = &pRegion->grpHeadList[indGroup];
    
    pEntry = (PENTRY)((char *)pvAlloc - sizeof(int));
    sizeEntry = pEntry->sizeFront - 1;

    pNext = (PENTRY)((char *)pEntry + sizeEntry);
    sizeNext = pNext->sizeFront;

    if (sizeNew > sizeEntry)
    {
        if ((sizeNext & 1) || (sizeNew > sizeEntry + sizeNext))
            return FALSE;
        indNext = (sizeNext >> PARA_SHIFT) - 1;
        if (indNext > 63)
            indNext = 63;
        if (pNext->pEntryNext == pNext->pEntryPrev)
        {
            if (indNext < 32)
            {
                pRegion->bitvGroupHi[indGroup] &= ~(0x80000000L >> indNext);
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
            }
            else
            {
                pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indNext - 32));
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryLo &= ~(0x80000000L >> (indNext - 32));
            }
        }
        pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
        pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;
       
        if ((sizeNext = sizeEntry + sizeNext - sizeNew) > 0)
        {
            pNext = (PENTRY)((char *)pEntry + sizeNew);
            indNext = (sizeNext >> PARA_SHIFT) - 1;
            if (indNext > 63)
                indNext = 63;
            pHead = (PENTRY)((char *)&pGroup->listHead[indNext] -
                                                           sizeof(int));
            pNext->pEntryNext = pHead->pEntryNext;
            pNext->pEntryPrev = pHead;
            pHead->pEntryNext = pNext;
            pNext->pEntryNext->pEntryPrev = pNext;

            if (pNext->pEntryNext == pNext->pEntryPrev)
            {
                if (indNext < 32)
                {
                    if (pRegion->cntRegionSize[indNext]++ == 0)
                        pHeader->bitvEntryHi |= 0x80000000L >> indNext;
                    pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indNext;
                }
                else
                {
                    if (pRegion->cntRegionSize[indNext]++ == 0)
                        pHeader->bitvEntryLo |= 0x80000000L >> (indNext - 32);
                    pRegion->bitvGroupLo[indGroup] |=
                                                0x80000000L >> (indNext - 32);
                }
            }

            pNext->sizeFront = sizeNext;
            ((PENTRYEND)((char *)pNext + sizeNext -
                                sizeof(ENTRYEND)))->sizeBack = sizeNext;
        }

        pEntry->sizeFront = sizeNew + 1;
        ((PENTRYEND)((char *)pEntry + sizeNew -
                            sizeof(ENTRYEND)))->sizeBack = sizeNew + 1;
    }
    else if (sizeNew < sizeEntry)
    {
        pEntry->sizeFront = sizeNew + 1;
        ((PENTRYEND)((char *)pEntry + sizeNew -
                            sizeof(ENTRYEND)))->sizeBack = sizeNew + 1;
        pEntry = (PENTRY)((char *)pEntry + sizeNew);
        sizeEntry -= sizeNew;
        indEntry = (sizeEntry >> PARA_SHIFT) - 1;
        if (indEntry > 63)
            indEntry = 63;
        if ((sizeNext & 1) == 0)
        {
            indNext = (sizeNext >> PARA_SHIFT) - 1;
            if (indNext > 63)
                indNext = 63;
            if (pNext->pEntryNext == pNext->pEntryPrev)
            {
                if (indNext < 32)
                {
                    pRegion->bitvGroupHi[indGroup] &=
                                                ~(0x80000000L >> indNext);
                    if (--pRegion->cntRegionSize[indNext] == 0)
                        pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
                }
                else
                {
                    pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indNext - 32));
                    if (--pRegion->cntRegionSize[indNext] == 0)
                        pHeader->bitvEntryLo &=
                                            ~(0x80000000L >> (indNext - 32));
                }
            }

            pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
            pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;
            
            sizeEntry += sizeNext;
            indEntry = (sizeEntry >> PARA_SHIFT) - 1;
            if (indEntry > 63)
                indEntry = 63;
        }
        pHead = (PENTRY)((char *)&pGroup->listHead[indEntry] - sizeof(int));
        pEntry->pEntryNext = pHead->pEntryNext;
        pEntry->pEntryPrev = pHead;
        pHead->pEntryNext = pEntry;
        pEntry->pEntryNext->pEntryPrev = pEntry;
        if (pEntry->pEntryNext == pEntry->pEntryPrev)
        {
            if (indEntry < 32)
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryHi |= 0x80000000L >> indEntry;
                pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indEntry;
            }
            else
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryLo |= 0x80000000L >> (indEntry - 32);
                pRegion->bitvGroupLo[indGroup] |= 0x80000000L >>
                                                           (indEntry - 32);
            }
        }
        pEntry->sizeFront = sizeEntry;
        ((PENTRYEND)((char *)pEntry + sizeEntry -
                            sizeof(ENTRYEND)))->sizeBack = sizeEntry;
    }
    return TRUE;
}

void *	XIOMemory::_calloc(size_t num, size_t size)
{
	DWORD	nIndex = InterlockedIncrement(&g_nMemoryCount) & (MEMORY_HEAP_SIZE-1);
	HEAP_HEADER *pHeader = (HEAP_HEADER *) g_memory[nIndex]._calloc_base(num * size + sizeof(HEAP_HEADER));
	if (pHeader) {
		pHeader->size = num * size;
		pHeader->key = KEY + nIndex;
		INTERLOCKEDEXCHANGEADD(&s_nTotalSize, num * size);
		return pHeader+1;
	}
	else {
		MEMORYSTATUS	ms;
		GlobalMemoryStatus(&ms);
		ELOG(_T("Memory::_calloc(%d, %d) failed : index=%d, total=%d, phys=%d, virtual=%d"), num, size, nIndex, 
			XIOMemory::s_nTotalSize, ms.dwAvailPhys, ms.dwAvailVirtual); 
		BOOL bValidate = HeapValidate(g_memory[nIndex]._crtheap, 0, 0);
		ELOG(_T("HeapValidate = %d"), bValidate);
		EBREAK();
		return 0;
	}
}

void *	XIOMemory::__expand(void * pBlock, size_t newsize)
{
    if (pBlock == NULL)
        return NULL;

	HEAP_HEADER	*pHeader = ((HEAP_HEADER *)pBlock) - 1;
	HEAP_HEADER	header = *pHeader;
	if ((header.key & -MEMORY_HEAP_SIZE) != KEY) {
		EBREAK();
		return 0;
	}
	pHeader = (HEAP_HEADER *) g_memory[header.key & (MEMORY_HEAP_SIZE-1)]._expand_base(pHeader, 
		newsize + sizeof(HEAP_HEADER));
	if (pHeader) {
		pHeader->size = newsize;
		pHeader->key = header.key;
		INTERLOCKEDEXCHANGEADD(&s_nTotalSize, newsize - header.size);
		return pHeader+1;
	}
	else {
		return 0;
	}
}

int XIOMemory::__sbh_alloc_new_group (PHEADER pHeader)
{
	PREGION     pRegion = pHeader->pRegion;
	PGROUP      pGroup;
	PENTRY      pEntry;
	PENTRY      pHead;
	PENTRYEND   pEntryEnd;
	BITVEC      bitvCommit;
	int         indCommit;
	int         index;
	void *      pHeapPage;
	void *      pHeapStartPage;
	void *      pHeapEndPage;

	bitvCommit = pHeader->bitvCommit;
	indCommit = 0;
	while ((int)bitvCommit >= 0)
	{
		bitvCommit <<= 1;
		indCommit++;
	}
	pGroup = &pRegion->grpHeadList[indCommit];
	for (index = 0; index < 63; index++)
	{
		pEntry = (PENTRY)((char *)&pGroup->listHead[index] - sizeof(int));
		pEntry->pEntryNext = pEntry->pEntryPrev = pEntry;
	}
	pHeapStartPage = (void *)((char *)pHeader->pHeapData +
									   indCommit * BYTES_PER_GROUP);
	if ((VirtualAlloc(pHeapStartPage, BYTES_PER_GROUP, MEM_COMMIT,
									  PAGE_READWRITE)) == NULL)
		return -1;

	pHeapEndPage = (void *)((char *)pHeapStartPage +
						(PAGES_PER_GROUP - 1) * BYTES_PER_PAGE);
	for (pHeapPage = pHeapStartPage; pHeapPage <= pHeapEndPage;
			pHeapPage = (void *)((char *)pHeapPage + BYTES_PER_PAGE))
	{
		*(int *)((char *)pHeapPage + ENTRY_OFFSET - sizeof(int)) = -1;
		*(int *)((char *)pHeapPage + BYTES_PER_PAGE - sizeof(int)) = -1;

		pEntry = (PENTRY)((char *)pHeapPage + ENTRY_OFFSET);
		pEntry->sizeFront = MAX_FREE_ENTRY_SIZE;
		pEntry->pEntryNext = (PENTRY)((char *)pEntry +
											BYTES_PER_PAGE);
		pEntry->pEntryPrev = (PENTRY)((char *)pEntry -
											BYTES_PER_PAGE);
		pEntryEnd = (PENTRYEND)((char *)pEntry + MAX_FREE_ENTRY_SIZE -
											sizeof(ENTRYEND));
		pEntryEnd->sizeBack = MAX_FREE_ENTRY_SIZE;
	}

	pHead = (PENTRY)((char *)&pGroup->listHead[63] - sizeof(int));
	pEntry = pHead->pEntryNext =
						(PENTRY)((char *)pHeapStartPage + ENTRY_OFFSET);
	pEntry->pEntryPrev = pHead;

	pEntry = pHead->pEntryPrev =
						(PENTRY)((char *)pHeapEndPage + ENTRY_OFFSET);
	pEntry->pEntryNext = pHead;

	pRegion->bitvGroupHi[indCommit] = 0x00000000L;
	pRegion->bitvGroupLo[indCommit] = 0x00000001L;
	if (pRegion->cntRegionSize[63]++ == 0)
		pHeader->bitvEntryLo |= 0x00000001L;
	pHeader->bitvCommit &= ~(0x80000000L >> indCommit);
	return indCommit;
}

void *	XIOMemory::__sbh_alloc_block(int intSize)
{
	BITVEC      bitvEntryLo;
	BITVEC      bitvEntryHi;
	BITVEC      bitvTest;
	int         sizeEntry;
	int         indEntry;
	int         indGroupUse;
	int         sizeNewFree;
	int         indNewFree;
	PHEADER     pHeaderLast = __sbh_pHeaderList + __sbh_cntHeaderList;
	PHEADER     pHeader;
	PREGION     pRegion;
	PGROUP      pGroup;
	PENTRY      pEntry;
	PENTRY      pHead;

	sizeEntry = (intSize + 2 * (int) sizeof(int) + (BYTES_PER_PARA - 1))
										  & ~(BYTES_PER_PARA - 1);

	indEntry = (sizeEntry >> PARA_SHIFT) - 1;
	if (indEntry < 32)
	{
		bitvEntryHi = 0xffffffffUL >> indEntry;
		bitvEntryLo = 0xffffffffUL;
	}
	else
	{
		bitvEntryHi = 0;
		bitvEntryLo = 0xffffffffUL >> (indEntry - 32);
	}
	pHeader = __sbh_pHeaderScan;
	while (pHeader < pHeaderLast)
	{
		if ((bitvEntryHi & pHeader->bitvEntryHi) |
			(bitvEntryLo & pHeader->bitvEntryLo))
			break;
		pHeader++;
	}

	if (pHeader == pHeaderLast)
	{
		pHeader = __sbh_pHeaderList;
		while (pHeader < __sbh_pHeaderScan)
		{
			if ((bitvEntryHi & pHeader->bitvEntryHi) |
				(bitvEntryLo & pHeader->bitvEntryLo))
				break;
			pHeader++;
		}

		if (pHeader == __sbh_pHeaderScan)
		{
			while (pHeader < pHeaderLast)
			{
				if (pHeader->bitvCommit)
					break;
				pHeader++;
			}
			if (pHeader == pHeaderLast)
			{
				pHeader = __sbh_pHeaderList;
				while (pHeader < __sbh_pHeaderScan)
				{
					if (pHeader->bitvCommit)
						break;
					pHeader++;
				}
				if (pHeader == __sbh_pHeaderScan)
					if (!(pHeader = __sbh_alloc_new_region()))
						return NULL;
			}
			if ((pHeader->pRegion->indGroupUse =
									__sbh_alloc_new_group(pHeader)) == -1)
				return NULL;
		}
	}
	__sbh_pHeaderScan = pHeader;

	pRegion = pHeader->pRegion;
	indGroupUse = pRegion->indGroupUse;
	if (indGroupUse == -1 ||
					!((bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]) |
					  (bitvEntryLo & pRegion->bitvGroupLo[indGroupUse])))
	{
		indGroupUse = 0;
		while (!((bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]) |
				 (bitvEntryLo & pRegion->bitvGroupLo[indGroupUse])))
			indGroupUse++;
	}
	pGroup = &pRegion->grpHeadList[indGroupUse];
	indEntry = 0;
	if (!(bitvTest = bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]))
	{
		indEntry = 32;
		bitvTest = bitvEntryLo & pRegion->bitvGroupLo[indGroupUse];
	}
	while ((int)bitvTest >= 0)
	{
		   bitvTest <<= 1;
		   indEntry++;
	}
	pEntry = pGroup->listHead[indEntry].pEntryNext;
	sizeNewFree = pEntry->sizeFront - sizeEntry;
	indNewFree = (sizeNewFree >> PARA_SHIFT) - 1;
	if (indNewFree > 63)
		indNewFree = 63;
	if (indNewFree != indEntry)
	{
		if (pEntry->pEntryNext == pEntry->pEntryPrev)
		{
			if (indEntry < 32)
			{
				pRegion->bitvGroupHi[indGroupUse] &=
											~(0x80000000L >> indEntry);
				if (--pRegion->cntRegionSize[indEntry] == 0)
					pHeader->bitvEntryHi &= ~(0x80000000L >> indEntry);
			}
			else
			{
				pRegion->bitvGroupLo[indGroupUse] &=
											~(0x80000000L >> (indEntry - 32));
				if (--pRegion->cntRegionSize[indEntry] == 0)
					pHeader->bitvEntryLo &= ~(0x80000000L >> (indEntry - 32));
			}
		}
		pEntry->pEntryPrev->pEntryNext = pEntry->pEntryNext;
		pEntry->pEntryNext->pEntryPrev = pEntry->pEntryPrev;
		if (sizeNewFree != 0)
		{
			pHead = (PENTRY)((char *)&pGroup->listHead[indNewFree] -
														   sizeof(int));
			pEntry->pEntryNext = pHead->pEntryNext;
			pEntry->pEntryPrev = pHead;
			pHead->pEntryNext = pEntry;
			pEntry->pEntryNext->pEntryPrev = pEntry;
			if (pEntry->pEntryNext == pEntry->pEntryPrev)
			{
				if (indNewFree < 32)
				{
					if (pRegion->cntRegionSize[indNewFree]++ == 0)
						pHeader->bitvEntryHi |= 0x80000000L >> indNewFree;
					pRegion->bitvGroupHi[indGroupUse] |=
												0x80000000L >> indNewFree;
				}
				else
				{
					if (pRegion->cntRegionSize[indNewFree]++ == 0)
						pHeader->bitvEntryLo |=
										0x80000000L >> (indNewFree - 32);
					pRegion->bitvGroupLo[indGroupUse] |=
										0x80000000L >> (indNewFree - 32);
				}
			}
		}
	}
	if (sizeNewFree != 0)
	{
		pEntry->sizeFront = sizeNewFree;
		((PENTRYEND)((char *)pEntry + sizeNewFree -
					sizeof(ENTRYEND)))->sizeBack = sizeNewFree;
	}
	pEntry = (PENTRY)((char *)pEntry + sizeNewFree);
	pEntry->sizeFront = sizeEntry + 1;
	((PENTRYEND)((char *)pEntry + sizeEntry -
					sizeof(ENTRYEND)))->sizeBack = sizeEntry + 1;
	if (pGroup->cntEntries++ == 0)
	{
		if (pHeader == __sbh_pHeaderDefer &&
								  indGroupUse == __sbh_indGroupDefer)
			__sbh_pHeaderDefer = NULL;
	}
	pRegion->indGroupUse = indGroupUse;
	return (void *)((char *)pEntry + sizeof(int));
}

void *	XIOMemory::_realloc(void * pBlock, size_t newsize)
{
    if (pBlock == NULL)
        return(_malloc(newsize));
    if (newsize == 0)
    {
        _free(pBlock);
        return(NULL);
    }
	HEAP_HEADER	*pHeader = ((HEAP_HEADER *)pBlock) - 1;
	HEAP_HEADER	header = *pHeader;
	if ((header.key & -MEMORY_HEAP_SIZE) != KEY) {
		EBREAK();
		return 0;
	}
	pHeader = (HEAP_HEADER *) g_memory[header.key & (MEMORY_HEAP_SIZE-1)]._realloc_base(pHeader, 
		newsize + sizeof(HEAP_HEADER));
	if (pHeader) {
		pHeader->size = newsize;
		pHeader->key = header.key;
		INTERLOCKEDEXCHANGEADD(&s_nTotalSize, newsize - header.size);
		return pHeader+1;
	}
	else {
		MEMORYSTATUS	ms;
		GlobalMemoryStatus(&ms);
		ELOG(_T("Memory::_realloc(%d) failed : size=%d, key=%#x, total=%d, phys=%d, virtual=%d"), newsize, 
			header.size, header.key, XIOMemory::s_nTotalSize, ms.dwAvailPhys, ms.dwAvailVirtual); 
		BOOL bValidate = HeapValidate(g_memory[header.key & (MEMORY_HEAP_SIZE-1)]._crtheap, 0, 0);
		ELOG(_T("HeapValidate = %d"), bValidate);
		EBREAK();
		return 0;
	}
}

void *	XIOMemory::_calloc_base(size_t size)
{
    size_t  size_sbh;

    size_sbh = size;
#ifndef	IO_HEAP
    void *  pvReturn;
    if ( size_sbh <= __sbh_threshold )
    {
        _mlock();
        pvReturn = __sbh_alloc_block((int) size_sbh);
        _munlock();
		if (pvReturn != NULL) {
            memset(pvReturn, 0, size_sbh);
			return pvReturn;
		}
    }
#endif
    return HeapAlloc(_crtheap, HEAP_ZERO_MEMORY, size);
}

void *	XIOMemory::_malloc(size_t size)
{
	DWORD	nIndex = InterlockedIncrement(&g_nMemoryCount) & (MEMORY_HEAP_SIZE-1);
	HEAP_HEADER *pHeader = (HEAP_HEADER*) g_memory[nIndex]._heap_alloc_base(size + sizeof(HEAP_HEADER));
	if (pHeader) {
		pHeader->size = size;
		pHeader->key = KEY + nIndex;
		memset(++pHeader, 0xfc, size);
#if		DEBUG_LEAK
		if (pHeader == (HEAP_HEADER *) DEBUG_LEAK) {
			TCHAR buff[128];
			_stprintf_s(buff, _T("malloc %p %d\n"), pHeader, size);
			OutputDebugString(buff);
			DebugBreak();
		}
#endif
		INTERLOCKEDEXCHANGEADD(&s_nTotalSize, size);
		return pHeader;
	}
	else {
		MEMORYSTATUS	ms;
		GlobalMemoryStatus(&ms);
		ELOG(_T("Memory::_malloc(%d) failed : index=%d, total=%d, phys=%d, virtual=%d"), size, nIndex, 
			XIOMemory::s_nTotalSize, ms.dwAvailPhys, ms.dwAvailVirtual); 
		BOOL bValidate = HeapValidate(g_memory[nIndex]._crtheap, 0, 0);
		ELOG(_T("HeapValidate = %d"), bValidate);
		EBREAK();
		return 0;
	}
}

int XIOMemory::_heap_init()
{
	if ( (_crtheap = HeapCreate( 0, BYTES_PER_PAGE, 0 )) == NULL )
		return 0;
	if (__sbh_heap_init(MAX_ALLOC_DATA_SIZE) == 0)
	{
		HeapDestroy(_crtheap);
		_crtheap = NULL;
		return 0;
	}
	return 1;
}

void XIOMemory::_heap_term()
{
	if (!_crtheap)
		return;
#ifndef	IO_HEAP
	{
		PHEADER pHeader = __sbh_pHeaderList;
		int     cntHeader;
		for (cntHeader = 0; cntHeader < __sbh_cntHeaderList; cntHeader++)
		{
			VirtualFree(pHeader->pHeapData, BYTES_PER_REGION, MEM_DECOMMIT);
			VirtualFree(pHeader->pHeapData, 0, MEM_RELEASE);
			HeapFree(_crtheap, 0, pHeader->pRegion);
			pHeader++;
		}
		HeapFree(_crtheap, 0, __sbh_pHeaderList);
	}
#endif
	HeapDestroy(_crtheap);
}

int XIOMemory::__sbh_heap_init(size_t threshold)
{
	if (!(__sbh_pHeaderList = (PHEADER) HeapAlloc(_crtheap, 0, 16 * sizeof(HEADER))))
		return FALSE;

	__sbh_threshold = threshold;
	__sbh_pHeaderScan = __sbh_pHeaderList;
	__sbh_pHeaderDefer = NULL;
	__sbh_cntHeaderList = 0;
	__sbh_sizeHeaderList = 16;

	return TRUE;
}

void	XIOMemory::__sbh_free_block (PHEADER pHeader, void * pvAlloc)
{
	PREGION         pRegion;
	PGROUP          pGroup;
	PENTRY          pHead;
	PENTRY          pEntry;
	PENTRY          pNext;
	PENTRY          pPrev;
	void *          pHeapDecommit;
	int             sizeEntry;
	int             sizeNext;
	int             sizePrev;
	unsigned int    indGroup;
	unsigned int    indEntry;
	unsigned int    indNext;
	unsigned int    indPrev;
	unsigned int    offRegion;

	pRegion = pHeader->pRegion;

	offRegion = (unsigned int)((UINT_PTR)pvAlloc - (UINT_PTR)pHeader->pHeapData);
	indGroup = offRegion / BYTES_PER_GROUP;
	pGroup = &pRegion->grpHeadList[indGroup];

	pEntry = (PENTRY)((char *)pvAlloc - sizeof(int));
	sizeEntry = pEntry->sizeFront - 1;

	if ( (sizeEntry & 1 ) != 0 )
		return;

	pNext = (PENTRY)((char *)pEntry + sizeEntry);
	sizeNext = pNext->sizeFront;
	sizePrev = ((PENTRYEND)((char *)pEntry - sizeof(int)))->sizeBack;

	if ((sizeNext & 1) == 0)
	{
		indNext = (sizeNext >> PARA_SHIFT) - 1;
		if (indNext > 63)
			indNext = 63;

		if (pNext->pEntryNext == pNext->pEntryPrev)
		{
			if (indNext < 32)
			{
				pRegion->bitvGroupHi[indGroup] &= ~(0x80000000L >> indNext);
				if (--pRegion->cntRegionSize[indNext] == 0)
					pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
			}
			else
			{
				pRegion->bitvGroupLo[indGroup] &=
											~(0x80000000L >> (indNext - 32));
				if (--pRegion->cntRegionSize[indNext] == 0)
					pHeader->bitvEntryLo &= ~(0x80000000L >> (indNext - 32));
			}
		}

		pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
		pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;

		sizeEntry += sizeNext;
	}
	indEntry = (sizeEntry >> PARA_SHIFT) - 1;
	if (indEntry > 63)
		indEntry = 63;
	if ((sizePrev & 1) == 0)
	{
		pPrev = (PENTRY)((char *)pEntry - sizePrev);

		indPrev = (sizePrev >> PARA_SHIFT) - 1;
		if (indPrev > 63)
			indPrev = 63;
		sizeEntry += sizePrev;
		indEntry = (sizeEntry >> PARA_SHIFT) - 1;
		if (indEntry > 63)
			indEntry = 63;

		if (indPrev != indEntry)
		{
			if (pPrev->pEntryNext == pPrev->pEntryPrev)
			{
				if (indPrev < 32)
				{
					pRegion->bitvGroupHi[indGroup] &=
												~(0x80000000L >> indPrev);
					if (--pRegion->cntRegionSize[indPrev] == 0)
						pHeader->bitvEntryHi &= ~(0x80000000L >> indPrev);
				}
				else
				{
					pRegion->bitvGroupLo[indGroup] &=
											~(0x80000000L >> (indPrev - 32));
					if (--pRegion->cntRegionSize[indPrev] == 0)
						pHeader->bitvEntryLo &=
											~(0x80000000L >> (indPrev - 32));
				}
			}
			pPrev->pEntryPrev->pEntryNext = pPrev->pEntryNext;
			pPrev->pEntryNext->pEntryPrev = pPrev->pEntryPrev;
		}
		pEntry = pPrev;
	}
	if (!((sizePrev & 1) == 0 && indPrev == indEntry))
	{
		pHead = (PENTRY)((char *)&pGroup->listHead[indEntry] - sizeof(int));
		pEntry->pEntryNext = pHead->pEntryNext;
		pEntry->pEntryPrev = pHead;
		pHead->pEntryNext = pEntry;
		pEntry->pEntryNext->pEntryPrev = pEntry;

		if (pEntry->pEntryNext == pEntry->pEntryPrev)
		{
			if (indEntry < 32)
			{
				if (pRegion->cntRegionSize[indEntry]++ == 0)
					pHeader->bitvEntryHi |= 0x80000000L >> indEntry;
				pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indEntry;
			}
			else
			{
				if (pRegion->cntRegionSize[indEntry]++ == 0)
					pHeader->bitvEntryLo |= 0x80000000L >> (indEntry - 32);
				pRegion->bitvGroupLo[indGroup] |= 0x80000000L >>
														   (indEntry - 32);
			}
		}
	}

	pEntry->sizeFront = sizeEntry;
	((PENTRYEND)((char *)pEntry + sizeEntry -
						sizeof(ENTRYEND)))->sizeBack = sizeEntry;

	if (--pGroup->cntEntries == 0)
	{
		if (__sbh_pHeaderDefer)
		{
			pHeapDecommit = (void *)((char *)__sbh_pHeaderDefer->pHeapData +
									__sbh_indGroupDefer * BYTES_PER_GROUP);
			VirtualFree(pHeapDecommit, BYTES_PER_GROUP, MEM_DECOMMIT);

			__sbh_pHeaderDefer->bitvCommit |=
										  0x80000000 >> __sbh_indGroupDefer;

			__sbh_pHeaderDefer->pRegion->bitvGroupLo[__sbh_indGroupDefer] = 0;
			if (--__sbh_pHeaderDefer->pRegion->cntRegionSize[63] == 0)
				__sbh_pHeaderDefer->bitvEntryLo &= ~0x00000001L;

			if (__sbh_pHeaderDefer->bitvCommit == BITV_COMMIT_INIT)
			{
				
				VirtualFree(__sbh_pHeaderDefer->pHeapData, 0, MEM_RELEASE);
				HeapFree(_crtheap, 0, __sbh_pHeaderDefer->pRegion);
				memmove((void *)__sbh_pHeaderDefer,
							(void *)(__sbh_pHeaderDefer + 1),
							(UINT_PTR)(__sbh_pHeaderList + __sbh_cntHeaderList) -
							(UINT_PTR)(__sbh_pHeaderDefer + 1));
				__sbh_cntHeaderList--;

				if (pHeader > __sbh_pHeaderDefer)
					pHeader--;
				__sbh_pHeaderScan = __sbh_pHeaderList;
			}
		}
		__sbh_pHeaderDefer = pHeader;
		__sbh_indGroupDefer = indGroup;
	}
}
#endif
