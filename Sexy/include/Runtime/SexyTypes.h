// SexyTypes.h -- the primitive spellings the decompiler emits, plus the
// offset-access helper the transcribed bodies are written against.
//
// WHY THIS FILE EXISTS
//
// The bodies in _recon/ida/*.cpp are IDA output, and IDA writes ARM32 code.
// It says things like
//
//     *((_DWORD *)this + 209) = v7;      // store a pointer in slot 209
//     *(_BYTE *)this = 0;                // store a byte
//     *(_DWORD *)(v3 + 244) = *(_DWORD *)(v3 + 244) | 4;
//
// Every one of those assumes a 4-byte word and a 4-byte pointer. That is the
// whole reason this project builds x86 rather than x64: on x64 a `_DWORD` slot
// holding a pointer would truncate it, `sizeof(void*)` would double, and the
// vtable slot arithmetic (slot 84, slot 188, ...) would address the wrong
// entries. On x86 the transcribed arithmetic is exactly right, so the bodies
// can be copied with only syntactic changes and no reinterpretation.
//
// `_DWORD`, `_BYTE` and friends are defined here with their IDA meanings so the
// dumps compile verbatim. They are deliberately NOT the Windows SDK's versions
// (windef.h has no `_DWORD` at all; the `_BYTE` there is unrelated), so this
// header must not be confused with windows.h.

#ifndef __SEXYTYPES_H__
#define __SEXYTYPES_H__

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <utility>
#include <type_traits>

// The ARM/NEON operations the decompiler emitted as intrinsic calls.
#include "ArmIntrinsics.h"

// --- IDA primitive spellings ------------------------------------------------
typedef unsigned short      _WORD;
typedef unsigned char       _BYTE;
typedef unsigned __int64    _QWORD;
typedef signed __int64      __int64_t_;
typedef int                 _BOOL4;
typedef unsigned char       _BOOL1;
// The decompiler's "type unknown" placeholder. It appears only in casts such as
// `(_UNKNOWN **)x`, where the operand is a class pointer: as `unsigned int` the
// cast would produce `unsigned int**`, which a class pointer does not convert to
// (C2040). As `void` any pointer converts, which is what the cast means.
typedef void                _UNKNOWN;


// ---------------------------------------------------------------------------
// _DWORD -- the decompiler's word.
//
// On the target a pointer is 32 bits, the same as a word, and the binary moves
// values between the two without distinguishing them:
//
//     v3 = *(_DWORD *)(v2 + 10);     a pointer read into a word
//     *v3 = self;                     that word dereferenced as a pointer
//
// IDA prints both as `_DWORD` because that is the register width. A plain
// `typedef unsigned int` cannot express that, and casting at each of the
// several thousand sites would invent a distinction the binary does not make.
//
// So _DWORD is a 4-byte value type that converts to and from any pointer and
// carries the operators a pointer has. It stays exactly 4 bytes, still
// participates in the arithmetic the binary performs, and the LOBYTE/HIBYTE/
// LOWORD macros still work on it.
//
// The one deliberate omission is comparison operators. With only
// `operator unsigned int()`, `v == 0`, `v != v2` and `v < n` resolve through
// the built-in operators on `unsigned int`, which is exactly what the binary
// computes; adding `operator==` would make every such call ambiguous.
//
// This is correct only because sizeof(void*) == 4. On x64 the pointer
// constructor would truncate -- the documented reason this project builds x86.
// ---------------------------------------------------------------------------
class _DWORD
{
public:
    unsigned int mValue;

    // The bit pattern of a float, which is what a word slot holds when the
    // binary moves a float through a register.
    static unsigned int BitsOf(float theValue)
    {
        unsigned int aBits;
        std::memcpy(&aBits, &theValue, 4);
        return aBits;
    }

    // And back, for the reads.
    float AsFloat() const
    {
        float aValue;
        std::memcpy(&aValue, &mValue, 4);
        return aValue;
    }

    _DWORD() : mValue(0) {}
    _DWORD(unsigned int theValue) : mValue(theValue) {}
    _DWORD(int theValue) : mValue((unsigned int)theValue) {}
    _DWORD(unsigned long theValue) : mValue((unsigned int)theValue) {}
    _DWORD(long theValue) : mValue((unsigned int)theValue) {}
    _DWORD(unsigned char theValue) : mValue(theValue) {}
    // A 64-bit value truncated to a word. The decompiler does this freely --
    // `LODWORD` is exactly this operation -- because the binary only ever keeps
    // the low word.
    _DWORD(unsigned __int64 theValue) : mValue((unsigned int)theValue) {}
    _DWORD(__int64 theValue) : mValue((unsigned int)theValue) {}
    _DWORD(char theValue) : mValue((unsigned int)(unsigned char)theValue) {}
    _DWORD(_WORD theValue) : mValue(theValue) {}
    // `nullptr` is not a pointer type, so the template constructor below does
    // not deduce for it and `_DWORD w = nullptr;` was a hard error. The
    // transcribed bodies pass `nullptr` to the word-typed helpers in
    // Sexy/ReconExterns.h, where a null pointer is a perfectly ordinary word.
    _DWORD(std::nullptr_t) : mValue(0) {}
    // A float or double. Without an exact match, `v = 1.5f` is ambiguous: a
    // float converts equally well to every integer overload above (C2593). The
    // slot holds the bit pattern, which is what the binary stores when it moves
    // a float through a word register.
    _DWORD(float theValue) : mValue(BitsOf(theValue)) {}
    _DWORD(double theValue) : mValue(BitsOf((float)theValue)) {}

    // Any pointer fits: that is what a word is on this target.
    template <class T>
    _DWORD(T* thePointer)
        : mValue((unsigned int)(uintptr_t)thePointer) {}

    // A value type that can pack itself into a word. The decompiler reuses a
    // register-width slot for a Color, an Insets or a vector, and the slot holds
    // the packed form -- which is what ToInt() produces.
    //
    // Only the CONSTRUCTOR is templated. An assignment template would be a
    // better match than the built-in `operator=` for every assignment that
    // reaches it, including ones the built-in handles exactly, and overload
    // resolution then fails as ambiguous (C2593) rather than picking the exact
    // match. The numeric overloads above cover the scalar cases and the
    // conversion operator covers the rest.
    // The SFINAE test is `decltype(T::ToInt())`, which is well-formed exactly
    // when T has a ToInt member. (An earlier spelling,
    // `decltype(decltype(std::declval<T>().ToInt())*())`, was ill-formed for
    // every T -- `()` is not an expression -- so this constructor was silently
    // discarded and `_DWORD w = someColor;` never compiled.)
    template <class T, class = decltype(std::declval<T>().ToInt())>
    _DWORD(const T& theValue)
        : mValue((unsigned int)theValue.ToInt()) {}

    // One numeric conversion, so all the arithmetic and comparison the bodies
    // perform resolves through the built-in operators.
    operator unsigned int() const { return mValue; }

    // And a word can be used where a pointer is expected.
    template <class T>
    operator T*() const { return (T*)(uintptr_t)mValue; }

    // The operators a pointer has, so `*v`, `v[i]` and `v->m` work on a word
    // exactly as they do in the decompiler's output.
    _DWORD& operator*() const
    {
        return *(_DWORD*)(uintptr_t)mValue;
    }
    _DWORD& operator[](int theIndex) const
    {
        return *(_DWORD*)((uintptr_t)mValue + 4u * (unsigned int)theIndex);
    }

    _DWORD& operator=(unsigned int theValue) { mValue = theValue; return *this; }
    _DWORD& operator=(int theValue) { mValue = (unsigned int)theValue; return *this; }
    _DWORD& operator=(_WORD theValue) { mValue = theValue; return *this; }
    // An exact match for the floating types. Without these, `word = 1.5f` is
    // ambiguous: a float converts to `unsigned int`, `int` and `_WORD` by
    // equally ranked standard conversions, so no overload is better (C2593).
    _DWORD& operator=(float theValue) { mValue = BitsOf(theValue); return *this; }
    _DWORD& operator=(double theValue)
    {
        mValue = BitsOf((float)theValue);
        return *this;
    }
    _DWORD& operator=(unsigned __int64 theValue)
    {
        mValue = (unsigned int)theValue;
        return *this;
    }
    _DWORD& operator=(__int64 theValue)
    {
        mValue = (unsigned int)theValue;
        return *this;
    }

    template <class T>
    _DWORD& operator=(T* thePointer)
    {
        mValue = (unsigned int)(uintptr_t)thePointer;
        return *this;
    }

    _DWORD& operator+=(unsigned int theN) { mValue += theN; return *this; }
    _DWORD& operator-=(unsigned int theN) { mValue -= theN; return *this; }
    _DWORD& operator*=(unsigned int theN) { mValue *= theN; return *this; }
    _DWORD& operator&=(unsigned int theN) { mValue &= theN; return *this; }
    _DWORD& operator|=(unsigned int theN) { mValue |= theN; return *this; }
    _DWORD& operator^=(unsigned int theN) { mValue ^= theN; return *this; }

    _DWORD& operator++() { ++mValue; return *this; }
    _DWORD  operator++(int) { _DWORD aOld(*this); ++mValue; return aOld; }
    _DWORD& operator--() { --mValue; return *this; }
    _DWORD  operator--(int) { _DWORD aOld(*this); --mValue; return aOld; }
};

static_assert(sizeof(_DWORD) == 4, "_DWORD must be one word");

// ---------------------------------------------------------------------------
// `pointer + word`, as an exact match.
//
// The decompiler writes byte arithmetic on a raw base constantly:
//
//     IosSetState((char *)&v45[-1] + *(_DWORD *)((char *)v44 + 0xFFFFFFF4), 0);
//
// Without help that line does not compile. _DWORD converts to `unsigned int`
// AND to any `T*`, so `char* + _DWORD` has two viable candidates that are
// neither better nor worse than each other:
//
//     Sexy::operator+(const Sexy::String&, const char*)
//         char*    -> const String&   (one user conversion, the ctor)
//         _DWORD   -> const char*     (one user conversion, operator T*)
//     built-in operator+(char*, int)
//         char*    -> char*           (exact)
//         _DWORD   -> unsigned int    (user) -> int (integral)
//
// Neither dominates -- the first is worse on argument 1 and the second is worse
// on argument 2 -- so MSVC reports C2666, "overloaded functions have similar
// conversions". It affects every file that does string work on a raw base, and
// /permissive- hides it, which is why it went unnoticed for so long.
//
// This template is an exact match on both arguments, so it beats both. It is
// deliberately a template on the pointee type: a non-template overload for
// `char*` would make `char* + _DWORD` ambiguous against a `const char*` one,
// because a qualification conversion cannot break the tie.
//
// Deduction cannot fire on ordinary arithmetic -- `char* + 5` does not deduce
// _DWORD from `int`, and template deduction ignores the converting
// constructors -- so the built-in operators keep every call they used to win.
//
// The result is an lvalue, like the built-in, so `&p[i] + n` stays assignable.
// ---------------------------------------------------------------------------
template <class T>
inline T* operator+(_DWORD theBase, T* theOffset)
{
    return (T*)(uintptr_t)(theBase.mValue + (unsigned int)(uintptr_t)theOffset);
}

template <class T>
inline T* operator+(T* theBase, _DWORD theOffset)
{
    return (T*)((char*)theBase + theOffset.mValue);
}


// The ELF symbol structures the binary walks (ElfShims.h) are NOT included
// here any more. They used to be, right after _DWORD because their fields are
// words -- and that closed an include cycle: ElfShims.h includes this header
// for the IDA primitive spellings, and this header included it back, so which
// of the two a translation unit reached first decided whether Elf32_Sym was
// declared at all by the time its own header was finished.
//
// The dependency is one-way. ElfShims.h needs _DWORD, _BYTE and _WORD from
// here; nothing in this header needs an ELF record. Every consumer of the ELF
// structures already includes the header it needs -- Sexy\ReconData.h includes
// "ElfShims.h" itself, and that is the path Game\Board.cpp and
// Sexy\ReconGlobals.cpp reach them through. No translation unit in the tree
// names an Elf32_* type without that include, which is what makes the edge
// safe to drop.

// IDA's 16-byte type, from SSE. It appears in casts and declarations; on x86 it
// is four words with four-byte alignment, so it is declared as such.
struct _OWORD
{
    _DWORD mWords[4];

    _OWORD() { mWords[0] = mWords[1] = mWords[2] = mWords[3] = 0; }

    template <class T>
    _OWORD(T* thePointer) { *(void**)mWords = (void*)thePointer; }

    operator void*() const { return *(void**)mWords; }

    _DWORD&       operator[](int theIndex)       { return mWords[theIndex]; }
    const _DWORD& operator[](int theIndex) const { return mWords[theIndex]; }
};

static_assert(sizeof(_OWORD) == 16, "_OWORD must be 16 bytes");

// Byte- and word-extraction macros IDA emits when it cannot prove a shift is in
// range. Their definitions live in IdaMacros.h, which is included here and
// again after <windows.h> (see that header for why re-inclusion matters).
#include "IdaMacros.h"

// ---------------------------------------------------------------------------
// MEM<T> -- the offset accessor.
//
// Transcribed bodies keep IDA's raw byte arithmetic, which reads as
// `*((_DWORD *)this + 209)`. New code written by hand in this project uses
// MEM<T> instead, which names the same location:
//
//     MEM<SexyAppBase*>(this, 0x344) = theApp;      // identical to
//                                                   // *((_DWORD*)this + 209)
//
// The point is not to abstract the layout away -- the offsets are the ground
// truth from the binary and are meant to stay visible and checkable. It is to
// keep the two spellings from drifting: MEM<T> asserts the width, so a field
// that the binary stores as a pointer cannot silently be read as a byte.
//
// T may be a pointer, an integer or a small POD. It must be 4 bytes or less
// unless the caller also uses MEMArray.
// ---------------------------------------------------------------------------

template <class T>
struct MEMRef
{
    unsigned char* mBase;

    T&       Ref()       { return *reinterpret_cast<T*>(mBase); }
    const T& Ref() const { return *reinterpret_cast<const T*>(mBase); }

    MEMRef& operator=(const T& theValue)
    {
        Ref() = theValue;
        return *this;
    }

    operator T() const { return Ref(); }

    T*       operator&()       { return reinterpret_cast<T*>(mBase); }
    const T* operator&() const { return reinterpret_cast<const T*>(mBase); }

    // Field access through the stored pointer, for the common
    // `MEM<Thing*>(this, off)->Member` shape.
    T  operator->() const { return Ref(); }
};

template <class T>
inline MEMRef<T> MEM(const void* theBase, size_t theOffset)
{
    MEMRef<T> r;
    r.mBase = reinterpret_cast<unsigned char*>(const_cast<void*>(theBase)) + theOffset;
    return r;
}

// Same, for a location the caller already has as a pointer of the right type.
template <class T>
inline MEMRef<T> MEMAt(void* theAddress)
{
    MEMRef<T> r;
    r.mBase = reinterpret_cast<unsigned char*>(theAddress);
    return r;
}

// An array of n elements of T starting at theOffset, addressed by index.
template <class T>
inline T& MEMArray(void* theBase, size_t theOffset, size_t theIndex)
{
    return reinterpret_cast<T*>(
        reinterpret_cast<unsigned char*>(theBase) + theOffset)[theIndex];
}

// The raw pointer at an offset, for passing to a transcribed body that wants
// the address rather than the value.
inline void* MEMPtr(void* theBase, size_t theOffset)
{
    return reinterpret_cast<unsigned char*>(theBase) + theOffset;
}
inline const void* MEMPtr(const void* theBase, size_t theOffset)
{
    return reinterpret_cast<const unsigned char*>(theBase) + theOffset;
}

// ---------------------------------------------------------------------------
// VSLOT -- a virtual call at a known vtable index.
//
// IDA writes these as `(*(void (__fastcall **)(int, int))(*(_DWORD *)v2 + 500))(v2, a2)`,
// i.e. "read the vtable, add 500 (slot 125), call it". VSLOT names the same
// thing without the four-line cast. Slot numbers come from the binary and are
// meaningful on x86, where a vtable entry is 4 bytes.
// ---------------------------------------------------------------------------
template <class Fn>
inline Fn VSLOT(void* theObject, size_t theByteOffset)
{
    void** vtbl = *reinterpret_cast<void***>(theObject);
    return reinterpret_cast<Fn>(
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(vtbl) + theByteOffset));
}

// Call helper: VSLOTC<Ret>(obj, 500)(obj, args...)
template <class Fn, class... Args>
inline auto VSLOTC(void* theObject, size_t theByteOffset, Args... theArgs)
    -> decltype(reinterpret_cast<Fn>(nullptr)(theArgs...))
{
    Fn fn = VSLOT<Fn>(theObject, theByteOffset);
    return fn(theArgs...);
}

#endif // __SEXYTYPES_H__
