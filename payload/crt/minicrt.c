#include <assert.h>
// #include <phnt.h>
#include <malloc.h>
#include <windows.h>

NTSYSAPI
void* NTAPI RtlGetCurrentPeb(VOID);

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(_In_ HANDLE HeapHandle, _In_opt_ ULONG Flags, _In_ SIZE_T Size);

NTSYSAPI
PVOID
NTAPI
RtlReAllocateHeap(_In_ HANDLE HeapHandle, _In_ ULONG Flags, _Frees_ptr_opt_ PVOID BaseAddress, _In_ SIZE_T Size);

NTSYSAPI
ULONG
NTAPI
RtlFreeHeap(_In_ HANDLE HeapHandle, _In_opt_ ULONG Flags, _Frees_ptr_opt_ _Post_invalid_ PVOID BaseAddress);

static HANDLE GetHeapHandle()
{
    HANDLE heap;
    memcpy(&heap, (char*)RtlGetCurrentPeb() + 0x30, sizeof(HANDLE));
    return heap;
}

void* __cdecl malloc(size_t size)
{
    return RtlAllocateHeap(GetHeapHandle(), HEAP_ZERO_MEMORY, size);
}

#ifndef _DEBUG
void* __cdecl _expand(void* block, size_t size)
{
    return RtlReAllocateHeap(GetHeapHandle(), HEAP_ZERO_MEMORY, block, size);
}
#endif

void __cdecl free(void* block)
{
    RtlFreeHeap(GetHeapHandle(), 0, block);
}

void* __cdecl calloc(size_t num, size_t size)
{
    return malloc(num * size);
}

void* __cdecl realloc(void* block, size_t size)
{
    return _expand(block, size);
}

int __cdecl puts(const char* s)
{
    DWORD  cbWritten;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(hStdOut, s, lstrlen(s), &cbWritten, 0);
    WriteFile(hStdOut, "\n", 1, &cbWritten, 0);
    return (int)(cbWritten ? cbWritten : -1);
}
