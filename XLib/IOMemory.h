#pragma once

//C++ exception handler used, but unwind semantics are not enabled. Specify -GX
#pragma warning(disable: 4530)

#define PARAS_PER_PAGE      4096    //  tunable value
#define PAGES_PER_GROUP     8       //  tunable value
#define GROUPS_PER_REGION   32      //  tunable value (max 32)

#undef malloc
#undef calloc
#undef realloc
#undef _expand
#undef free
#undef _msize


#ifdef	_DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#undef new

//inline void * operator new(size_t cb, const char * szFileName, int nLine)
//{
//    return _malloc_dbg(cb, _NORMAL_BLOCK, szFileName, nLine);
//}

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW

#else
#include <cstdlib>
#define _expand(p, s)     XIOMemory::__expand(p,s)
#define free(p)           XIOMemory::_free(p)
#define _msize(p)         XIOMemory::__msize(p)
#define malloc(s)         XIOMemory::_malloc(s)
#define calloc(c, s)      XIOMemory::_calloc(c,s)
#define realloc(p, s)     XIOMemory::_realloc(p,s)

#endif // _DEBUG


class XIOMemory
{
public:

	static void Touch();
	void *	_realloc_base(void * pBlock, size_t newsize);
	void *	_expand_base(void * pBlock, size_t newsize);
	size_t	_msize_base(void * pblock);
	int  _heap_init();
	void _heap_term();
	__forceinline void *  _heap_alloc_base(size_t size);
	void *	__sbh_alloc_block(int intSize);
	static void *	_malloc(size_t size);
	static void	_free(void * pBlock);
	static void *	_calloc(size_t num, size_t size);
	static void *	_realloc(void * pBlock, size_t newsize);
	static void *	__expand(void * pBlock, size_t newsize);
	static size_t	__msize(void * pblock);
	static size_t	GetTotalSize() { return s_nTotalSize; }
	void    _mlock() { EnterCriticalSection(&_lock); }
	void    _munlock() { LeaveCriticalSection(&_lock); }
	int	__sbh_heap_init(size_t threshold);
	XIOMemory();
	~XIOMemory();
	void    _free_base (void * pBlock);
	void *	_calloc_base(size_t size);


	class CInit
	{
	public:
		CInit();
		~CInit();
	};


	typedef struct tagEntryEnd
	{
		int                 sizeBack;
	}
	ENTRYEND, *PENTRYEND;
	struct tagEntry;

	typedef struct tagListHead
	{
		struct tagEntry *   pEntryNext;
		struct tagEntry *   pEntryPrev;
	}
	LISTHEAD, *PLISTHEAD;
	typedef unsigned int    BITVEC;
	typedef struct tagGroup
	{
		int                 cntEntries;
		struct tagListHead  listHead[64];
	}
	GROUP, *PGROUP;
	typedef struct tagRegion
	{
		int                 indGroupUse;
		char                cntRegionSize[64];
		BITVEC              bitvGroupHi[GROUPS_PER_REGION];
		BITVEC              bitvGroupLo[GROUPS_PER_REGION];
		struct tagGroup     grpHeadList[GROUPS_PER_REGION];
	}
	REGION, *PREGION;
	typedef struct tagHeader
	{
		BITVEC              bitvEntryHi;
		BITVEC              bitvEntryLo;
		BITVEC              bitvCommit;
		void *              pHeapData;
		struct tagRegion *  pRegion;
	}
	HEADER, *PHEADER;


#pragma pack(push, 1)
	typedef struct tagEntry
	{
		int                 sizeFront;
		struct tagEntry *   pEntryNext;
		struct tagEntry *   pEntryPrev;
	}
	ENTRY, *PENTRY;
#pragma pack(pop)


	HANDLE _crtheap;
	static LONG_PTR s_nTotalSize;

	size_t   __sbh_threshold;

	PHEADER  __sbh_pHeaderList;        //  pointer to list start
	PHEADER  __sbh_pHeaderScan;        //  pointer to list rover
	int      __sbh_sizeHeaderList;     //  allocated size of list
	int      __sbh_cntHeaderList;      //  count of entries defined

	PHEADER  __sbh_pHeaderDefer;
	int      __sbh_indGroupDefer;

	CRITICAL_SECTION _lock;



	struct HEAP_HEADER
	{
		UINT_PTR	size;
		UINT_PTR	key;
	};

	PHEADER __sbh_find_block (void * pvAlloc);
	void	__sbh_free_block (PHEADER pHeader, void * pvAlloc);
	int	    __sbh_alloc_new_group (PHEADER pHeader);
	int		__sbh_resize_block (PHEADER pHeader, void * pvAlloc, int intNew);
	PHEADER __sbh_alloc_new_region();
};



