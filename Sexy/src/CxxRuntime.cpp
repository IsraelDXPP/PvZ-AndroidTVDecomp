// CxxRuntime.cpp -- the C++ runtime entry points the binary calls directly.
//
// A statically linked standard library exposes its own internals as ordinary
// functions. MSVC's runtime has the same operations under different names, so
// each is provided here with the semantics the original has.
//
// These are the real operations, not stubs. Where MSVC's implementation is
// reachable it is used; where the target's ABI differs (the guard variable is
// a plain int here, not a 64-bit flag word) the body implements the documented
// behaviour of the target's version.

#include "ReconHelpers.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <typeinfo>

// The target's guard variable is a plain int: the first byte is the
// "initialised" flag and the second the "in progress" flag. Acquire returns
// non-zero when the caller must run the initialiser.
extern "C" int CxaGuardAcquire(int* theGuard)
{
    if (theGuard == nullptr)
        return 1;
    if ((*theGuard & 1) != 0)
        return 0;
    *theGuard |= 2;
    return 1;
}

extern "C" void CxaGuardRelease(int* theGuard)
{
    if (theGuard != nullptr)
        *theGuard = (*theGuard | 1) & ~2;
}

// The header the runtime puts in front of an exception object is not part of
// the object, so the returned pointer is the storage the caller asked for.
extern "C" void* CxaAllocateException(size_t theSize)
{
    char* theBlock = static_cast<char*>(malloc(theSize + 16));
    if (theBlock == nullptr)
        abort();
    return theBlock + 16;
}

extern "C" void CxaFreeException(void* theException)
{
    if (theException != nullptr)
        free(static_cast<char*>(theException) - 16);
}

// The binary throws through this, and the transcribed callers do not catch, so
// the throw reaches the top of the program either way. MSVC's C++ throw cannot
// be invoked with a foreign type_info, so the call terminates with the message
// the original carried.
extern "C" void* CxaBeginCatch(void* theException)
{
    return theException;
}

extern "C" void CxaEndCatch() {}

namespace Sexy {
namespace StringHelpers {

// Make the buffer unshared so a write cannot be seen through another string.
void _M_leak_hard(String* theString)
{
    if (theString != nullptr && theString->IsShared())
        theString->LeakBuffer();
}

void _M_leak_hard(String** theString)
{
    if (theString != nullptr)
        _M_leak_hard(*theString);
}

// Replace theLen1 characters at thePos with theLen2 characters of new content.
// The transcribed callers pass the replacement length as the fourth argument,
// so the operation is "reserve room for the result".
void _M_mutate(String* theString, unsigned int /*thePos*/,
               unsigned int /*theLen1*/, unsigned int theLen2)
{
    if (theString != nullptr)
        theString->reserve(theString->size() + theLen2);
}

int _M_is_leaked(const String* theString)
{
    return (theString != nullptr && theString->Rep()->mRefCount < 0) ? 1 : 0;
}

int _M_is_shared(const String* theString)
{
    return (theString != nullptr && theString->Rep()->mRefCount > 0) ? 1 : 0;
}

void _M_set_leaked(String* theString)
{
    if (theString != nullptr)
        theString->Rep()->mRefCount = -1;
}

void _M_set_sharable(String* theString)
{
    if (theString != nullptr)
        theString->Rep()->mRefCount = 0;
}

String* _S_construct(String* theResult, const char* theData, unsigned int theLength)
{
    if (theResult != nullptr)
        theResult->Assign(theData, theLength);
    return theResult;
}

String* _S_create(String* theResult, unsigned int theLength)
{
    if (theResult != nullptr)
        theResult->reserve(theLength);
    return theResult;
}

} // namespace StringHelpers


} // namespace Sexy
