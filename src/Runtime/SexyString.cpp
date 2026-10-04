// String.cpp -- the game's string.
//
// Each String owns its own character buffer. The object is still one pointer,
// so it occupies the same 4-byte slots the transcribed objects give it, but
// there is no shared buffer and no reference count to get wrong. Copying a
// string copies its characters. An empty string points at one static
// zero-terminated byte that is never freed.
//
// The header in front of the characters stays, because transcribed code reads
// the length from data - 12. The capacity and the old reference-count slot are
// unused; the count is kept negative so a leftover release never frees the
// block.

#include "SexyString.h"

#include <new>
#include <cstring>

namespace Sexy
{

// The shared empty rep. The 12-byte header is what a release guard subtracts
// before comparing, and the byte after it is the empty character data.
extern "C" char gEmptyRepStorageBytes[16] = {0};

StringRep gEmptyStringRep;
char* const gEmptyStringData = gEmptyRepStorageBytes + sizeof(StringRep);

struct EmptyRepInit
{
    EmptyRepInit()
    {
        gEmptyStringRep.mLength = 0;
        gEmptyStringRep.mCapacity = 0;
        gEmptyStringRep.mRefCount = -1;
        *gEmptyStringData = '\0';

        reinterpret_cast<StringRep*>(gEmptyRepStorageBytes)->mLength = 0;
        reinterpret_cast<StringRep*>(gEmptyRepStorageBytes)->mCapacity = 0;
        reinterpret_cast<StringRep*>(gEmptyRepStorageBytes)->mRefCount = -1;
    }
};

static EmptyRepInit gEmptyRepInit;

static bool Usable(const char* theData)
{
    return theData != nullptr
        && (uintptr_t)theData >= 0x10000u
        && theData != reinterpret_cast<const char*>(0xCCCCCCCCu)
        && theData != reinterpret_cast<const char*>(0xCDCDCDCDu);
}

static char* Fresh(const char* theText, unsigned int theLength)
{
    if (theLength == 0 || theText == nullptr)
        return gEmptyStringData;

    // A length past a few megabytes is a header read from a buffer that is
    // not a rep. The text is still a C string; copy that, do not ask for
    // gigabytes and throw bad_alloc.
    if (theLength > 8u * 1024u * 1024u)
    {
        theLength = (unsigned int)std::strlen(theText);
        if (theLength == 0 || theLength > 8u * 1024u * 1024u)
            return gEmptyStringData;
    }

    StringRep* aRep = static_cast<StringRep*>(
        ::operator new(sizeof(StringRep) + theLength + 1));
    aRep->mLength = theLength;
    aRep->mCapacity = theLength;
    aRep->mRefCount = -1;
    memcpy(aRep->Data(), theText, theLength);
    aRep->Data()[theLength] = '\0';
    return aRep->Data();
}

static void Drop(char* theData)
{
    if (!Usable(theData) || theData == gEmptyStringData)
        return;
    ::operator delete(reinterpret_cast<StringRep*>(theData) - 1);
}

void StringAddRef(StringRep*)
{
}

// Transcribed code releases a rep and the String's own destructor releases it
// again. The destructor owns the buffer, so this is a no-op.
int StringRelease(StringRep*)
{
    return -1;
}

void String::Detach()
{
}

void String::EnsureCapacity(unsigned int theCapacity)
{
    unsigned int aLength = size();
    if (theCapacity < aLength)
        theCapacity = aLength;
    if (mData != gEmptyStringData && theCapacity <= capacity())
        return;

    const char* aText = Usable(mData) ? mData : "";
    StringRep* aRep = static_cast<StringRep*>(
        ::operator new(sizeof(StringRep) + theCapacity + 1));
    aRep->mLength = aLength;
    aRep->mCapacity = theCapacity;
    aRep->mRefCount = -1;
    if (aLength != 0)
        memcpy(aRep->Data(), aText, aLength);
    aRep->Data()[aLength] = '\0';
    Drop(mData);
    mData = aRep->Data();
}

void String::Assign(const char* theData, unsigned int theLength)
{
    if (!Usable(theData))
    {
        theData = "";
        theLength = 0;
    }

    // The source may be this string's own buffer. Copy first, then drop.
    char* aFresh = Fresh(theData, theLength);
    Drop(mData);
    mData = aFresh;
}

void String::Append(const char* theData, unsigned int theLength)
{
    if (theLength == 0 || !Usable(theData))
        return;

    unsigned int aOld = size();
    if (theLength > 1073741820u - aOld)
        return;

    std::string aJoined;
    aJoined.reserve(aOld + theLength);
    if (aOld != 0 && Usable(mData))
        aJoined.append(mData, aOld);
    aJoined.append(theData, theLength);

    char* aFresh = Fresh(aJoined.data(), (unsigned int)aJoined.size());
    Drop(mData);
    mData = aFresh;
}

String* StringCtor(String* aResult, const char* theString, const void*)
{
    if (aResult == nullptr)
        return nullptr;
    aResult->mData = gEmptyStringData;
    aResult->Assign(theString ? theString : "",
                    theString ? (unsigned int)strlen(theString) : 0);
    return aResult;
}

String* StringAssign(String* aTarget, const String* aSource)
{
    if (aTarget != nullptr && aSource != nullptr)
        *aTarget = *aSource;
    return aTarget;
}

String* StringCopyCtor(String* aResult, const String* aSource)
{
    if (aResult == nullptr)
        return nullptr;
    aResult->mData = gEmptyStringData;
    if (aSource != nullptr)
        *aResult = *aSource;
    return aResult;
}

String* StringDispose(String* aString)
{
    if (aString != nullptr)
        aString->Assign("", 0);
    return aString;
}

String* StringDisposeLeaked(String* theString)
{
    return theString;
}

String* StringAppend(String* aTarget, const String* aSource)
{
    if (aTarget != nullptr && aSource != nullptr)
        aTarget->Append(aSource->c_str(), aSource->size());
    return aTarget;
}

String* StringAppendRange(String* aTarget, const char* aData, unsigned int theCount)
{
    if (aTarget != nullptr && aData != nullptr && theCount != 0)
        aTarget->Append(aData, theCount);
    return aTarget;
}

String* StringSubstr(String* aResult, const String* aSource,
                     unsigned int theStart, unsigned int theCount)
{
    if (aResult == nullptr)
        return nullptr;
    aResult->mData = gEmptyStringData;
    if (aSource != nullptr)
        *aResult = aSource->substr(theStart, theCount);
    return aResult;
}

} // namespace Sexy
