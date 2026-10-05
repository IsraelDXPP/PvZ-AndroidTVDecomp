// DeleteGuard.cpp -- classify every deallocation, and refuse the impossible ones.
//
// TEMPORARY DIAGNOSTIC. Remove once the transcription errors that hand
// non-heap pointers to the allocator are fixed.
//
// Why this exists. The startup crash is STATUS_STACK_BUFFER_OVERRUN raised by
// __fastfail, and the chain that produced the earlier ones was always the same
// shape: a `Sexy::String` holding a data address rather than a buffer it owned,
// so the release path computed `mData - sizeof(StringRep)` and handed an address
// inside a loaded module -- or inside the thread stack -- to `operator delete`.
//
// Neither of the usual tools can see that:
//   * HeapValidate inspects heap METADATA, and a free of an address that was
//     never a heap block never reaches the metadata;
//   * a guard-paged heap allocator faults on overflows, but an invalid free is
//     not an overflow;
//   * __fastfail bypasses SEH and a VEH, so no handler runs.
//
// So the check has to sit in the deallocator itself. A pointer that
// VirtualQuery reports as MEM_IMAGE or MEM_MAPPED is not a heap block, and
// freeing it is the bug; this prints the classification and the caller's return
// address, and skips the free.
//
// Leaking is the deliberate trade. A wrong free corrupts the heap and ends the
// process; a leak costs memory until the real transcription error is found.
//
// The real heap blocks still go to free(), so ordinary allocation is unaffected.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{
    // Returns true when the block must NOT be freed.
    bool RefuseToFree(void* theBlock, const char* theKind)
    {
        if (theBlock == nullptr)
            return true;                    // deleting null is a no-op

        MEMORY_BASIC_INFORMATION aInfo;
        if (::VirtualQuery(theBlock, &aInfo, sizeof(aInfo)) != sizeof(aInfo))
            return false;                   // let the CRT deal with it

        if (aInfo.Type != MEM_IMAGE && aInfo.Type != MEM_MAPPED)
            return false;                   // a real heap block

        static int sReported = 0;
        if (sReported < 32)
        {
            ++sReported;
            fprintf(stderr,
                    "DELETEGUARD: operator delete%s(%p) refused -- %s at %p, "
                    "not a heap block (leaked on purpose; see DeleteGuard.cpp)\n",
                    theKind, theBlock,
                    aInfo.Type == MEM_IMAGE ? "MEM_IMAGE" : "MEM_MAPPED",
                    aInfo.AllocationBase);
            fflush(stderr);
        }
        return true;
    }
}

void operator delete(void* theBlock) noexcept
{
    if (RefuseToFree(theBlock, ""))
        return;
    ::free(theBlock);
}

void operator delete[](void* theBlock) noexcept
{
    if (RefuseToFree(theBlock, "[]"))
        return;
    ::free(theBlock);
}

// The sized forms the compiler emits for C++14 and later. They must exist or the
// sized calls bind to the CRT's own operators and bypass the check above.
void operator delete(void* theBlock, size_t) noexcept
{
    if (RefuseToFree(theBlock, ""))
        return;
    ::free(theBlock);
}

void operator delete[](void* theBlock, size_t) noexcept
{
    if (RefuseToFree(theBlock, "[]"))
        return;
    ::free(theBlock);
}
