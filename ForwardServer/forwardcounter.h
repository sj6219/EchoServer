//------------------------------------------------------------------
//                   !!! WARNING !!! 
//
// This file is auto generated by ctrpp.exe utility from performance
// counters manifest:
//
// forward.man
//
// It should be regenerated every time the code is built.
// Do not check it in.
//-------------------------------------------------------------------

#pragma once

#include <winperf.h>
#include <perflib.h>

#define ForwardRunningThread (1)
#define ForwardConnections (2)

EXTERN_C DECLSPEC_SELECTANY GUID ForwardUserModeCountersGuid = { 0x6b0b3135, 0x4a43, 0x4e48, 0x84, 0xec, 0xf0, 0x57, 0x45, 0x7a, 0xc1, 0x8e };

EXTERN_C DECLSPEC_SELECTANY GUID ForwardCounterSet1Guid = { 0x5a74954, 0x8c0e, 0x4c42, 0x8b, 0x18, 0xc7, 0x43, 0xf, 0x5a, 0xd8, 0xc9 };


EXTERN_C DECLSPEC_SELECTANY HANDLE ForwardUserModeCounters = NULL;

EXTERN_C DECLSPEC_SELECTANY struct {
    PERF_COUNTERSET_INFO CounterSet;
    PERF_COUNTER_INFO Counter0;
    PERF_COUNTER_INFO Counter1;
} ForwardCounterSet1Info = {
    { { 0x5a74954, 0x8c0e, 0x4c42, 0x8b, 0x18, 0xc7, 0x43, 0xf, 0x5a, 0xd8, 0xc9 }, { 0x6b0b3135, 0x4a43, 0x4e48, 0x84, 0xec, 0xf0, 0x57, 0x45, 0x7a, 0xc1, 0x8e }, 2, PERF_COUNTERSET_MULTI_INSTANCES },
    { 1, PERF_COUNTER_RAWCOUNT, 0, sizeof(ULONG), PERF_DETAIL_NOVICE, 1, 0 },
    { 2, PERF_COUNTER_RAWCOUNT, 0, sizeof(ULONG), PERF_DETAIL_NOVICE, 1, 0 },
};

EXTERN_C FORCEINLINE
VOID
CounterCleanup(
    VOID
    )
{
    if (ForwardUserModeCounters != NULL) {
        PerfStopProvider(ForwardUserModeCounters);
        ForwardUserModeCounters = NULL;
    }
}

EXTERN_C FORCEINLINE
ULONG
CounterInitialize(
    __in_opt PERFLIBREQUEST NotificationCallback,
    __in_opt PERF_MEM_ALLOC MemoryAllocationFunction,
    __in_opt PERF_MEM_FREE MemoryFreeFunction,
    __inout_opt PVOID MemoryFunctionsContext
    )
{
    ULONG Status;
    PERF_PROVIDER_CONTEXT ProviderContext;

    ZeroMemory(&ProviderContext, sizeof(PERF_PROVIDER_CONTEXT));
    ProviderContext.ContextSize = sizeof(PERF_PROVIDER_CONTEXT);
    ProviderContext.ControlCallback = NotificationCallback;
    ProviderContext.MemAllocRoutine = MemoryAllocationFunction;
    ProviderContext.MemFreeRoutine = MemoryFreeFunction;
    ProviderContext.pMemContext = MemoryFunctionsContext;

    Status = PerfStartProviderEx(&ForwardUserModeCountersGuid,
                                 &ProviderContext,
                                 &ForwardUserModeCounters);
    if (Status != ERROR_SUCCESS) {
        ForwardUserModeCounters = NULL;
        return Status;
    }

    Status = PerfSetCounterSetInfo(ForwardUserModeCounters,
                                   &ForwardCounterSet1Info.CounterSet,
                                   sizeof ForwardCounterSet1Info);
    if (Status != ERROR_SUCCESS) {
        CounterCleanup();
        return Status;
    }
    return ERROR_SUCCESS;
}
