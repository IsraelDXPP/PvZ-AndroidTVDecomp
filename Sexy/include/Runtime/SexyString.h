// One native character pointer with deep-copy ownership and a shared empty buffer.
// StringRep stores character lengths, independent of the host pointer size.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYSTRING_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYSTRING_H

#include "SexyTypes.h"

#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

// ---------------------------------------------------------------------------
// The control block libstdc++ places immediately before the character data.
//
// _M_length and _M_capacity are size_t in libstdc++, i.e. 4 bytes on a 32-bit
// target, which is what the decompiler's `*(_DWORD *)(v6 - 12)` reads.
// ---------------------------------------------------------------------------
struct StringRep
{
    unsigned int mLength; // data - 12
    unsigned int mCapacity; // data -  8
    int mRefCount; // data -  4

    char *Data()
    {
        return reinterpret_cast<char *>(this + 1);
    }
};

// The header and characters refer to the same shared empty representation.
extern StringRep &gEmptyStringRep;
extern char *const gEmptyStringData;

// Refcount helpers. The binary uses __ldrex/__strex pairs; these are the same
// read-modify-write with x86 semantics.
void StringAddRef(StringRep *theRep);
// Returns the value the count had before the decrement, matching
// `v5 = __ldrex(v4); __strex(v5 - 1, v4); if (v5 <= 0) free(...)`.
int StringRelease(StringRep *theRep);

class String
{
public:
    // The only member; its size follows the host pointer size.
    char *mData;

public:
    String() :
        mData(gEmptyStringData)
    {
    }

    String(const char *theString) :
        mData(gEmptyStringData)
    {
        Assign(theString, theString ? (unsigned int)strlen(theString) : 0);
    }

    String(const char *theString, unsigned int theLength) :
        mData(gEmptyStringData)
    {
        Assign(theString, theLength);
    }

    String(unsigned int theCount, char theChar) :
        mData(gEmptyStringData)
    {
        char aByte = theChar;
        for (unsigned int aIndex = 0; aIndex < theCount; ++aIndex)
            Append(&aByte, 1);
    }

    String(const std::string &theString) :
        mData(gEmptyStringData)
    {
        Assign(theString.data(), (unsigned int)theString.size());
    }

    String(const String &theOther) :
        mData(gEmptyStringData)
    {
        Assign(theOther.mData, theOther.size());
    }

    // A char* is always copied. An adopting constructor would win direct-init
    // against String(const char*) for every non-const buffer, and a stack
    // array such as Fail's error text has no rep header in front of it.

    ~String()
    {
        Assign("", 0);
    }

    String &operator=(const String &theOther)
    {
        if (this != &theOther)
            Assign(theOther.mData, theOther.size());
        return *this;
    }

    String &operator=(const char *theString)
    {
        Assign(theString, theString ? (unsigned int)strlen(theString) : 0);
        return *this;
    }

    // --- observation --------------------------------------------------------

    const char *c_str() const
    {
        return mData != nullptr ? mData : "";
    }
    char *data() const
    {
        return mData != nullptr ? mData : const_cast<char *>("");
    }

    unsigned int size() const
    {
        return IsNotARealBuffer(mData) ? 0u : RepOf(mData)->mLength;
    }
    unsigned int length() const
    {
        return size();
    }
    bool empty() const
    {
        return size() == 0;
    }
    unsigned int capacity() const
    {
        return IsNotARealBuffer(mData) ? 0u : RepOf(mData)->mCapacity;
    }

    // True when this is the shared empty rep, matching the binary's
    // `(int *)(v - 12) != (int *)Sexy::gEmptyStringData` tests.
    bool IsEmptyRep() const
    {
        return mData == gEmptyStringData;
    }

    char operator[](unsigned int theIndex) const
    {
        return mData[theIndex];
    }
    char &operator[](unsigned int theIndex)
    {
        return mData[theIndex];
    }

    // --- mutation -----------------------------------------------------------

    void clear()
    {
        Assign("", 0);
    }

    // Assignment owns a deep copy of the source characters.
    String &assign(const String &theOther)
    {
        return *this = theOther;
    }

    String &assign(const char *theString)
    {
        Assign(theString ? theString : "", theString ? (unsigned int)strlen(theString) : 0u);
        return *this;
    }

    String &assign(const char *theString, unsigned int theLength)
    {
        Assign(theString ? theString : "", theLength);
        return *this;
    }

    String &operator+=(const String &theOther)
    {
        Append(theOther.mData, theOther.size());
        return *this;
    }
    String &operator+=(const char *theString)
    {
        Append(theString, theString ? (unsigned int)strlen(theString) : 0);
        return *this;
    }
    String &operator+=(char theChar)
    {
        Append(&theChar, 1);
        return *this;
    }

    // Assign a slice of an existing string. Mirrors StringSubstr, which is the
    // substr constructor (`basic_string::basic_string` is its throw message).
    String substr(unsigned int theStart, unsigned int theCount) const
    {
        unsigned int aLen = size();
        if (theStart > aLen)
            theStart = aLen;
        unsigned int aAvail = aLen - theStart;
        unsigned int aTake = (theCount > aAvail) ? aAvail : theCount;
        return String(mData + theStart, aTake);
    }

    // basic_string::substr(pos) -- to the end. libstdc++ declares both forms
    // and the XML parser's attributes call the one-argument one.
    String substr(unsigned int theStart) const
    {
        return substr(theStart, size());
    }

    // basic_string::push_back(char) -- the XML parser builds text a byte at a
    // time through it.
    void push_back(char theChar)
    {
        Append(&theChar, 1);
    }

    void Assign(const char *theData, unsigned int theLength);
    void Append(const char *theData, unsigned int theLength);

    // --- comparison ---------------------------------------------------------

    int compare(const char *theOther) const
    {
        return strcmp(mData ? mData : "", theOther ? theOther : "");
    }

    // The libstdc++ member-overload family the bodies spell out in full:
    //   compare(pos, len, other)
    int compare(unsigned int thePos, unsigned int theLen, const String &theOther) const
    {
        return substr(thePos, theLen).compare(theOther.mData);
    }
    int compare(unsigned int thePos, unsigned int theLen, const char *theOther) const
    {
        return substr(thePos, theLen).compare(theOther);
    }
    int compare(const String &theOther) const
    {
        return compare(theOther.mData);
    }

    bool operator==(const String &o) const
    {
        return compare(o.mData) == 0;
    }
    bool operator!=(const String &o) const
    {
        return compare(o.mData) != 0;
    }
    bool operator<(const String &o) const
    {
        return compare(o.mData) < 0;
    }
    bool operator==(const char *o) const
    {
        return compare(o) == 0;
    }
    bool operator!=(const char *o) const
    {
        return compare(o) != 0;
    }
    bool operator<(const char *o) const
    {
        return compare(o) < 0;
    }

    // Interop with the std::string-typed signatures the demangler produced for
    // free functions such as Sexy::StrFormat.
    operator std::string() const
    {
        return std::string(mData, size());
    }

    // --- internals ----------------------------------------------------------

    // Null and the shared empty buffer have no owned characters.
    static bool IsNotARealBuffer(const char *theData)
    {
        return theData == nullptr || theData == gEmptyStringData;
    }

    static StringRep *RepOf(char *theData)
    {
        if (IsNotARealBuffer(theData))
            return &gEmptyStringRep;
        return reinterpret_cast<StringRep *>(theData - sizeof(StringRep));
    }
    static const StringRep *RepOf(const char *theData)
    {
        if (IsNotARealBuffer(theData))
            return &gEmptyStringRep;
        return reinterpret_cast<const StringRep *>(theData - sizeof(StringRep));
    }

    StringRep *Rep()
    {
        return RepOf(mData);
    }
    const StringRep *Rep() const
    {
        return RepOf(mData);
    }

    // libstdc++'s reserve: make room for theCapacity characters without
    // changing the length. The transcribed bodies call this before writing
    // through the data pointer, exactly as the original did.
    void reserve(unsigned int theCapacity)
    {
        if (theCapacity > capacity())
            EnsureCapacity(theCapacity);
    }

    // True when the buffer is shared and a write must copy it first -- the
    // binary's `*(int *)(v6 - 4) > 0` test.
    bool IsShared() const
    {
        return false;
    }

    // Mark the buffer unshared and unfreeable (-1), as StringDisposeLeaked does.
    void LeakBuffer() {}

private:
    void AddRef() {}
    void Release()
    {
        Assign("", 0);
    }

    // Copy the buffer if it is shared, so a write cannot be observed through
    // another String. This is libstdc++'s _M_leak / _M_copy path.
    void Detach();
    void EnsureCapacity(unsigned int theCapacity);
};

// ---------------------------------------------------------------------------
// Free operators, so String works on either side of a comparison.
// ---------------------------------------------------------------------------
inline bool operator==(const char *a, const String &b)
{
    return b.compare(a) == 0;
}
inline bool operator!=(const char *a, const String &b)
{
    return b.compare(a) != 0;
}

inline String operator+(const String &a, const String &b)
{
    String r(a);
    r += b;
    return r;
}
inline String operator+(const String &a, const char *b)
{
    String r(a);
    r += b;
    return r;
}
inline String operator+(const char *a, const String &b)
{
    String r(a);
    r += b;
    return r;
}

// ---------------------------------------------------------------------------
// The runtime helpers the decompiler left unnamed.
//
// Each is a real function in the binary; naming them here is what lets the
// transcribed bodies read as code instead of as sub_XXXX calls. The address is
// recorded so a call site can still be traced back.
// ---------------------------------------------------------------------------

// @0x2ED3A4 (64 bytes) -- construct a string from a C string.
//   if (a2) v6 = (int)&a2[strlen(a2)]; else v6 = -1;
//   *a1 = StringConstructIter(a2, v6, a3, 0);
String *StringCtor(String *theResult, const char *theString, const void *theAlloc = nullptr);

// @0x2ECF40 (164 bytes) -- assign: refcount the source, release the target.
String *StringAssign(String *theTarget, const String *theSource);

// @0x2ECECC (112 bytes) -- copy-construct.
String *StringCopyCtor(String *theResult, const String *theSource);

// @0x2EC5C8 (20 bytes) -- release a temporary.
String *StringDispose(String *theString);

// @0x2EC788 (76 bytes) -- release and mark the buffer permanently leaked.
String *StringDisposeLeaked(String *theString);

// @0x2ECB7C (168 bytes) -- append a string.
String *StringAppend(String *theTarget, const String *theSource);

// @0x2EC458 -- compare a string with a C string. Returns 0 when they match.
int StringEqualCStr(const String *theString, const char *theText);

// @0x2ECA48 -- assign from a C string.
String *StringAssignCStr(String *theTarget, const char *theText);

// @0x2ECC28 (248 bytes) -- append a character range.
String *StringAppendRange(String *theTarget, const char *theData, unsigned int theCount);

// @0x2ED074 (92 bytes) -- substring constructor.
String *StringSubstr(String *theResult, const String *theSource, unsigned int theStart, unsigned int theCount);

void ThrowBadAlloc();
void ThrowBadCast();
void ThrowInvalidArgument(const char *theMessage);
void ThrowLengthError(const char *theMessage);
void ThrowOutOfRange(const char *theMessage);

} // namespace Sexy

// The game's code spells the type unqualified throughout.
using Sexy::String;
using Sexy::StringAppend;
using Sexy::StringAssign;
using Sexy::StringCopyCtor;
using Sexy::StringCtor;
using Sexy::StringDispose;
using Sexy::StringDisposeLeaked;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYSTRING_H
