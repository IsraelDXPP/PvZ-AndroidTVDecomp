// ReconHelpers.h -- the runtime helpers the transcribed bodies call by name.
//
// These are functions in the binary that the decompiler either left unnamed or
// printed with a library-qualified name. Each is used by many files, so the
// declarations live here rather than being repeated per translation unit.
//
// Every one is a real operation the binary performs; the implementation is in
// ReconHelpers.cpp. Where the original is a libc or libstdc++ function the
// implementation forwards to the platform's equivalent, because that is what
// the binary was linked against.

#pragma once

#include "SexyTypes.h"
#include "CoreTypes.h"
#include "SexyString.h"
#include "XMLWriter.h"

#include <cstdio>
#include <cstddef>
#include <cstdlib>

// ---------------------------------------------------------------------------
// StrFormat(dest, format, ...) -- sprintf into a Sexy::String.
// Returns the number of characters written.
// ---------------------------------------------------------------------------
int StrFormat(Sexy::String* theDest, const char* theFormat, ...);

// StringToInt(dest, text) -- parse an integer and also store the source text.
int StringToInt(Sexy::String* theDest, const char* theText);

// GetAppDataFolder(dest) -- the app's writable directory, as a string.
int GetAppDataFolder(Sexy::String* theDest);

namespace Sexy
{
void SetAppDataFolder(const Sexy::String& thePath);
}

// ---------------------------------------------------------------------------
// logfi / logfd -- the framework's own printf-style loggers.
//
// `fi` is the "info" channel and `fd` the "debug" one; both are what the
// binary writes its trace lines through. The return value is the character
// count, which some call sites store.
// ---------------------------------------------------------------------------
int p_addResource(const char* thePath, const char* theKind, int theFlags);

int logfi(const char* theFormat, ...);
int logfd(const char* theFormat, ...);
int logfe(const char* theFormat, ...);

// ---------------------------------------------------------------------------
// SMemW(dest, source, count, limit) -- copy a C string into a byte buffer.
// theCount is the element size and theLimit is the destination capacity.
// ---------------------------------------------------------------------------
int SMemW(char* theDest, const char* theSource, int theCount, int theLimit);

// The file helpers. The stream is a FILE*, the same pointer fopen hands back.
size_t p_fread(void* theBuffer, size_t theSize, size_t theCount, FILE* theStream);

// qmemcpy -- the compiler's inlined memcpy that IDA printed by name.
inline void* qmemcpy(void* theDest, const void* theSource, size_t theCount)
{
    return memcpy(theDest, theSource, theCount);
}

// ---------------------------------------------------------------------------
// Rand -- the framework's own random helper.
//
// The dump writes it as a call taking the object and a range:
//
//     Rand(theApp, theRange);
//
// which is the framework's "a random number in [0, theRange)" over the app's
// generator. Both arities appear in the transcription (52 two-argument sites
// and 16 one-argument ones), so both are provided.
// ---------------------------------------------------------------------------
// The binary exports exactly three overloads:
//   _ZN4Sexy4RandEv  @0x2f9ec8
//   _ZN4Sexy4RandEi  @0x2f9ed4
//   _ZN4Sexy4RandEf  @0x2f9ee4   <-- the float form
// Without the float one, `Rand(2.0f)` silently converts to the int overload.
//
// The zero-argument form is the generator's own `operator()` -- a full-range
// draw with no bound. Several files call it (`Game/Board.cpp` and
// `Game/MainMenu.cpp` had each declared their own local `int Rand();` to get
// past its absence), so it is declared here once instead.
// All three are mangled inside namespace Sexy (`_ZN4Sexy4RandE...`). The
// zero-argument form is the generator's own draw; the int and float forms
// scale it into [0, theRange). The double overload exists only so a bare
// `Rand(1.7)` resolves instead of being ambiguous between int and float.
namespace Sexy {
int Rand();
int Rand(void* theObject, int theRange);
int Rand(int theRange);
float Rand(float theRange);
float Rand(double theRange);
}
// The transcribed bodies call it `Rand(...)` with no qualifier. The symbol is
// inside namespace Sexy, so the unqualified name is an alias of that one.
using Sexy::Rand;

// _ZN4Sexy5UpperERKSs @0x2fb5ac -- the uppercase counterpart of Lower above,
// same sret shape (r0 = the caller's result buffer). ResourceManager.cpp calls
// it by this name and no header declared it.
int Upper(Sexy::String* theDest, const Sexy::String* theSource);

// Trim -- the third sibling of Upper/Lower, same sret shape. PIEffect.cpp is
// its only caller in this tree and had to declare it file-locally.
int Trim(Sexy::String* theDest, const Sexy::String* theSource);

// _ZN4Sexy13CommaSeperateEi @0x2fbe2c -- "1,234,567" formatting, called from
// LawnApp::GetMoneyString and SexyAppBase's memory dump. Returns a string BY
// VALUE. Defined in ReconHelpers.cpp.
Sexy::String CommaSeperate(int theValue);

// Sexy::SexyGetLocaleEncoding @0x3063f8 (666 bytes) / SexySetLocaleEncoding --
// the framework's locale accessors. The bodies call them by these names and the
// tree declared neither. The Windows platform layer maps them onto the CRT
// locale, which is the same contract.
const char* SexyGetLocaleEncoding();
int         SexySetLocaleEncoding(const char* theEncoding);

// abs32 -- IDA's 32-bit absolute-value builtin. Challenge.cpp and Board.cpp
// both hit it; without a definition the parser reads `abs32(x)` as an unknown
// function. The template keeps the operand's own type, so it works for both the
// signed and unsigned forms the dumps use.
template <class T>
inline T abs32(T theValue)
{
    return theValue < (T)0 ? (T)-theValue : theValue;
}

// ---------------------------------------------------------------------------
// Slot dispatch for the binary's hand-built / Itanium vtables.
//
// Several of the interfaces the tree talks to (the platform driver at +0x2EC,
// the audio interfaces, the file driver, the widget manager's drivers) are
// reached through vtables the binary laid out with the Itanium ABI. MSVC spends
// ONE slot on a virtual destructor where Itanium spends two, so writing them as
// C++ `virtual` classes would put every method one slot early. InvokeSlot reads
// the entry at the binary's own byte offset from the object's vtable pointer and
// calls it with the object first, which is what the ARM code does.
//
// This used to live in Sexy\SexyAppBase.cpp, which made it invisible to
// Sexy\BaseAppDriver.h once the driver tables moved there.
// ---------------------------------------------------------------------------
#if defined(_M_IX86)
    #define LTDRIVERCALL __cdecl
#else
    #define LTDRIVERCALL
#endif

template <class T, class... Args>
T InvokeSlot(void *theObject, int theByteOffset, Args... theArgs)
{
    typedef T (LTDRIVERCALL *SlotFn)(void *, Args...);

    // THE OBJECT'S FIRST WORD IS THE VTABLE POINTER, so the entry is read
    // THROUGH it -- `(*(void***)theObject)[offset/4]`, exactly what the VSLOT
    // helper in Runtime\SexyTypes.h does.
    //
    // This used to index the object itself, `((void**)theObject)[offset/4]`,
    // which is only correct when the object IS the table. For the platform
    // drivers it is not: Sexy\BaseAppDriver.h declares `void *mVtable;` at +0x00
    // and Win32AppDriver continues { mApp +0x04, mWindow +0x08, ... }, so
    // `Invoke(2)` read the word at +0x08 -- mWindow, a GLWindow pointer -- and
    // called it. That is the execute violation at a data address the exe
    // reported while `new LawnApp` ran BaseAppDriver::InitAppDriver.
    //
    // Every other caller passes a real polymorphic object whose vptr is at +0
    // (SexyAppBase's own slot dispatches, mTrialTimeCheck, mPlatformIntegration),
    // and for those the two spellings happen to agree only because their vptr is
    // the first word of the table they point at. Dereferencing is correct for
    // both cases and is what the ARM does.
    void** aTable = *static_cast<void***>(theObject);
    SlotFn aSlot = nullptr;
    if (aTable != nullptr)
    {
        void* aEntry = aTable[theByteOffset / 4];
        std::memcpy(&aSlot, &aEntry, sizeof(aSlot));
    }
    return aSlot(theObject, theArgs...);
}


// ---------------------------------------------------------------------------
// The framework's string conversions.
//
// The binary's own helpers for moving between its 4-byte string type, the wide
// form and the platform's ANSI form. Each is a real function in the binary; the
// bodies call them by these names.
// ---------------------------------------------------------------------------
void  StringToWString(Sexy::String* theDest, const char* theSource);
void  StringToWStringFast(Sexy::String* theDest, const char* theSource);
void  StringToSexyString(Sexy::String* theDest, const char* theSource);
void  WStringToString(Sexy::String* theDest, const wchar_t* theSource);
// Sexy::WStringToStringFast @0x2fa3ec (140 bytes) -- same conversion as
// WStringToString, taking the caller's already-widened buffer.
void  WStringToStringFast(Sexy::String* theDest, const wchar_t* theSource);
void  WStringFrom(Sexy::String* theDest, const wchar_t* theSource);
int   StringToInt(const char* theText);

// _ZN4Sexy14StringToDoubleE... -- two symbols, @0x2faa90 (228 bytes) and
// @0x2fd358 (102 bytes). PopAnim.cpp calls it with the text and the destination.
// Both parameters stay `void*` because the call sites pass a
// Sexy::String* and either a double* or an int* depending on the overload.
int   StringToDouble(void* theText, void* theDest);
void  StringToLower(Sexy::String* theDest, const char* theSource);
void  StringToUpper(Sexy::String* theDest, const char* theSource);
void  XMLEncodeString(Sexy::String* theDest, const char* theSource);
void  XMLDecodeString(Sexy::String* theDest, const char* theSource);
// Copy theCount bytes from the definition-cache cursor and advance it.
// theLimit is unused; it stays so older three-argument calls still match.
int   SMemR(unsigned char** theCursor, void* theDest, int theCount, int theLimit = 0);
int   SMemWStr(char* theDest, const char* theSource, int theLimit);

// The two-argument string-member forms. ImageFont.cpp calls SMemRStr seven times
// and SMemWStr twice and had to define them file-locally because only the
// three-argument SMemWStr above existed. The real symbols are two-argument:
//   _ZN4Sexy9SMemRStrE...  @0x2a8f70   -- read a string member
//   _ZN4Sexy9SMemWStrE...              -- write a string member
// Both copy the source string into the destination slot; the destination is a
// `Sexy::String*` in this tree's spelling.
int   SMemRStr(Sexy::String* theDest, const Sexy::String* theSource);
int   SMemWStr(Sexy::String* theDest, const Sexy::String* theSource);

// The remaining conversions and string utilities the bodies call by name.
// `Sexy::ToString` and `Sexy::SexyWStringToString` both copy a string through
// a destination slot; `GetPathFrom` splits a path; `logtfe` is the framework's
// formatted trace in the "file, line" style.
void  ToString(Sexy::String* theDest, const Sexy::String* theSource);
void  SexyWStringToString(Sexy::String* theDest, const Sexy::String* theSource);
void  ToWString(Sexy::String* theDest, const Sexy::String* theSource);
void  ToSexyString(Sexy::String* theDest, const Sexy::String* theSource);
void  GetPathFrom(Sexy::String* theOut, Sexy::String* theFirst, Sexy::String* theSecond);
int   logtfe(const Sexy::String& theFormat, const char* theFile, ...);
// Notepad-style directory creation, which the resource loader calls.
int   MkDir(const char* thePath);
int   MkDir(const Sexy::String& thePath);
int   MkDir(const Sexy::String* thePath);
// The string-to-int conversion the binary exports: the text arrives by value
// and the parsed number is written through the pointer.
int   StringToInt(Sexy::String theText, int* theDest);

// ---------------------------------------------------------------------------
// The SharedImageRef helpers.
//
//   _ZN4Sexy14SharedImageRefaSERKS0_  -- the assignment operator, spelled as a
//   free function because the call sites take the destination as a word.
//   SharedImageRefImage is the `operator Image*` conversion (the binary's
//   `_ZN4Sexy14SharedImageRefcvPNS_5ImageEEv`), which reads the +4 word.
//
// SharedImageRef is only forward-declared here. Including its header would
// close a cycle, so the definitions in ReconHelpers.cpp include that header.
// ---------------------------------------------------------------------------
namespace Sexy {
class SharedImageRef;
class Image;
} // namespace Sexy

int   SharedImageRefAssign(Sexy::SharedImageRef* theDest, const Sexy::SharedImageRef* theSource);
Sexy::Image* SharedImageRefImage(const Sexy::SharedImageRef* theRef);

// Find a string in a half-open range. Returns theEnd when the value is absent.
Sexy::String* FindInStringVector(Sexy::String* theBegin, Sexy::String* theEnd, const Sexy::String* theValue);

// The framework's own file helpers the bodies call.
int   p_fseek(FILE* theStream, long theOffset, int theOrigin);
FILE* p_fopen(const char* thePath, const char* theMode);
int   p_fclose(FILE* theStream);
long  p_ftell(FILE* theStream);
// _ZN4Sexy10GetFileDirERKSsb -- GetFileDir(dest, path, bool). The path is a
// Sexy::String, so call sites pass a pointer to that object. The two trailing
// parameters are the transcription's leftovers and stay defaulted so the
// existing four-argument call sites keep compiling.
int   GetFileDir(Sexy::String* theDest, const void* thePath,
                 int theUnused = 0, int theAlsoUnused = 0);

// Two more Sexy:: helpers ResourceManager.cpp calls and no header declared.
// Both are sret-shaped (a destination string plus the source):
//   Sexy::logtfd / Sexy::RemoveTrailingSlash
// logtfd is the "file, line" formatted trace the resource loader emits while
// resolving a path; RemoveTrailingSlash trims '/' and '\\' off the end.
int   logtfd(const char* theFormat, ...);
void  RemoveTrailingSlash(Sexy::String* thePath);
// _ZN4Sexy5LowerERKSs @0x2fb638 and _ZN4Sexy11GetFileNameERKSsb @0x2faee8.
// Both RETURN A STRING BY VALUE (sret in r0), which is why the bodies write
// `Lower(&dest, &src)` and `GetFileName(&dest, &name, theFlag)`. Several files
// (PopAnim, ResourceManager, RenderEffectDefinition, Mailbox) call them and no
// header declared them.
int   Lower(Sexy::String* theDest, const Sexy::String* theSource);
int   GetFileName(Sexy::String* theDest, const Sexy::String* theName, int theFlag);
// Checked memcpy: reports a length that would run past the destination.
void* ReconMemcpyChecked(void* theDest, const void* theSource, unsigned int theCount, void* theCaller);

// Sexy::GetEnvOption @0x2fb9f4 (_ZN4Sexy12GetEnvOptionEPKcb).
// Missing or unrecognized values return theDefault as 0 or 1. The recognized
// spellings are "1"/"yes"/"true"/"True" and "0"/"no"/"false"/"False".
namespace Sexy {
int GetEnvOption(const char* theName, bool theDefault);
}
const char* GetNameString(int theId);
void  BuildLine(Sexy::String* theDest, const char* theSource, int theWidth);

// ---------------------------------------------------------------------------
// The C runtime names the binary was linked against. On Windows these are the
// underscore-prefixed forms, so each is forwarded rather than reimplemented.
// ---------------------------------------------------------------------------
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef snprintf
#define snprintf _snprintf
#endif

// ---------------------------------------------------------------------------
// The C++ runtime entry points the binary calls by name. A statically linked
// libstdc++ exposes its internals as ordinary functions and the decompiler
// prints the calls to them verbatim; each is provided with the semantics the
// original has. Implementations are in CxxRuntime.cpp.
// ---------------------------------------------------------------------------
extern "C" int   CxaGuardAcquire(int* theGuard);
extern "C" void  CxaGuardRelease(int* theGuard);
extern "C" void* CxaAllocateException(size_t theSize);
extern "C" void  CxaFreeException(void* theException);
// The third argument is the destructor the binary registers. Callers pass a
// thunk that already takes the object pointer, so the parameter is a template
// rather than `void (*)(void *)`. This port does not invoke it: the throw
// terminates, the same as CxxRuntime.cpp used to do.
template <class T>
inline void CxaThrow(void* theException, void* /*theTypeInfo*/,
                     void (* /*theDestructor*/)(T*))
{
    const char* aWhat = "unhandled C++ exception from libGameMain";
    if (theException != nullptr)
    {
        void** aVtable = *static_cast<void ***>(theException);
        if (aVtable != nullptr && aVtable[1] != nullptr)
        {
            typedef const char* (*WhatFn)(void*);
            WhatFn aWhatFn = nullptr;
            void* aEntry = aVtable[1];
            std::memcpy(&aWhatFn, &aEntry, sizeof(aWhatFn));
            const char* aMsg = aWhatFn(theException);
            if (aMsg != nullptr)
                aWhat = aMsg;
        }
    }
    fprintf(stderr, "terminate called: %s\n", aWhat);
    fflush(stderr);
    abort();
}
extern "C" void* CxaBeginCatch(void* theException);
extern "C" void  CxaEndCatch();

namespace Sexy {

// The bodies sometimes spell these framework helpers with a `Sexy::` prefix,
// because that is the namespace they really live in in the binary (the mangled
// names are _ZN4Sexy9StrFormatEPKcz and friends). They are declared and defined
// at global scope in this project, so naming them here makes BOTH spellings
// resolve to the same function instead of two declarations that never meet.
using ::StrFormat;

namespace StringHelpers {

void    _M_leak_hard(String* theString);
void    _M_leak_hard(String** theString);
void    _M_mutate(String* theString, unsigned int thePos,
                  unsigned int theLen1, unsigned int theLen2);
int     _M_is_leaked(const String* theString);
int     _M_is_shared(const String* theString);
void    _M_set_leaked(String* theString);
void    _M_set_sharable(String* theString);
String* _S_construct(String* theResult, const char* theData, unsigned int theLength);
String* _S_create(String* theResult, unsigned int theLength);

} // namespace StringHelpers

} // namespace Sexy

// ---------------------------------------------------------------------------
// TM::Thread -- the framework's thread wrapper.
//
// The binary's TM::Thread is a handle plus a run function, which is what the
// transcribed constructors store and what Create starts. The Android
// implementation uses pthreads; the Windows equivalent is a Win32 thread,
// which is the same operation.
//
// ReconFreeFunctions.h already forward-declares TM::Thread as a class of
// static helpers; the real definition lives in Thread.h, which this header
// pulls in so the declarations agree.
// ---------------------------------------------------------------------------
#include "Thread.h"


// ---------------------------------------------------------------------------
// Late additions reported by the file agents. Each of these was a symbol the
// bodies call by name that no header declared, so individual .cpp files had to
// carry their own copies (and some of those copies disagreed with each other).
// ---------------------------------------------------------------------------

// `Upper` is declared at global scope above; the bodies also spell it
// `Sexy::Upper`, which did not resolve. Naming it here fixes both spellings.
using ::Upper;

namespace Sexy {

// _ZN4Sexy9setLocaleEPvPKc @0x3bc0a0 (18 bytes) -- install a locale by name.
const char* setLocale(void* theCategory, const char* theName);

// _ZN4Sexy19SexyStringToStringE... @0x2fa87c (12 bytes) -- the narrow-string
// copy. The dumps pass the source as a raw word (usually a map lookup's
// result), so it stays `unsigned int` here and callers holding a Sexy::String*
// cast it.
void SexyStringToString(void* theDest, unsigned int theSource);

// _ZN4Sexy20SWTri_AddDrawTriFuncE... @0x3ab6e4 (106 bytes) -- register a
// software triangle batch with the renderer. LawnApp::Init primes it.
int SWTri_AddDrawTriFunc(int theFormat, int theTexMode, int theAlphaMode,
                         int theModMode, int theGlobalMode, int theBlendMode, void* theFunc);

// _ZN4Sexy44DrawTriangle_8888_TEX1_TALPHA1_MOD0_GLOB0_BLEND1E... @0x369e2c
// (5504 bytes) -- the 32-bit software rasteriser the batcher binds to. One
// symbol per (format, texture, alpha, modulation, global-colour, blend)
// combination; this is the one the TV build uses.
int DrawTriangle_8888_TEX1_TALPHA1_MOD0_GLOB0_BLEND1();

// gResourceManagerInfo -- the manager's debug/statistics block. Only its
// address is taken. Storage is in ReconData.cpp.
extern int gResourceManagerInfo;

// RemoveTrailingSlash(dest, src) -- the two-argument form the resource loader
// uses (the one-argument version further up writes in place). Declared here so
// ResourceManager's local copy can go away.
void RemoveTrailingSlash(Sexy::String* theDest, const void* theSource);

// _ZN4Sexy10DividePolyE... @0x40f4f8 (726 bytes) -- the polygon splitter the
// particle engine's geometry path calls. PIEffect.cpp is its only caller in
// this tree and had to declare it file-locally.
// The call site passes the polygon, a source word, a scratch buffer, a
// vertex cap, and the output record.
int DividePoly(void* thePolygon, void* theSource, void* theScratch, int theMaxPoints, void* theOut);

// std::map<K,V>::operator[] spelled the static way in the dumps
// (`std::map<K,V>::operator[](mapObj, key)`). Several files carried their own
// helper for it; this is the shared one.
template <class K, class V>
inline V& MapAt(void* theMap, const K& theKey)
{
    return (*static_cast<std::map<K, V>*>(theMap))[theKey];
}

// Three more Sexy:: helpers SexyAppBase.cpp calls and no header declared:
//   _ZN4Sexy18GetResourcesFolderEv   @0x2fa0fc  (20 bytes)
//   _ZN4Sexy15GetEnvIntOptionE...    @0x2fba90  (22 bytes)
//   _ZN4Sexy9DebugInitE...           @0x3b9bdc  (38 bytes)
// (Already inside namespace Sexy.)
//
// CORRECTED against the binary after SexyAppBase.cpp stalled on these:
//   _ZN4Sexy18GetResourcesFolderEv @0x2fa0fc -- the body is
//       copies the resource-root string into the caller's buffer and returns it.
//   i.e. it RETURNS A STRING BY VALUE (r0 = the caller's result buffer, which
//   is also the receiver here). The earlier `const char* GetResourcesFolder(void)`
//   guess was wrong and every call site passes one argument.
int GetResourcesFolder(Sexy::String* theDest);
//   _ZN4Sexy15GetEnvIntOptionE... @0x2fba90 (22 bytes) -- read an environment
//   variable as an integer, parsing with theBase and falling back to theDefault.
int GetEnvIntOption(const char* theName, const char* theDefault, int theBase);
//   _ZN4Sexy9DebugInitEii @0x3b9bdc -- TWO parameters, not one.
int DebugInit(int theMode, int theFlags);
//   _ZN4Sexy24SWTri_AddAllDrawTriFuncsE... @0x3ab754 (1286 bytes) -- fills the
//   software-rasteriser function table. One argument.
int SWTri_AddAllDrawTriFuncs(void* theTable);

// Sexy::Ratio -- a CLASS in the binary, not a function:
//   _ZN4Sexy5RatioC2Ev   @0x409c94  -> Ratio(void), sets the first word to 1
//   _ZN4Sexy5Ratio3SetEii @0x409c9c -> Set(int, int)
// Three call sites in SexyAppBase.cpp use the type, so it needs a definition.
class Ratio
{
public:
    int mNumerator;
    int mDenominator;

    Ratio();
    int Set(int theNumerator, int theDenominator);
};

// NOTE: RbTree::equal_range is declared in ReconTypes.h, NOT here. That header
// defines RbTree::NodeBase and includes this one, so naming the type at this
// point would be a use before definition.

} // namespace Sexy

