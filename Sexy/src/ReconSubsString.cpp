// String and text-stream shims the game still calls by the recovered names.
// String work goes through Sexy::String. Text streams keep the characters in
// the embedded String and a cursor the extractors walk.

#include "SexyTypes.h"
#include "SexyString.h"
#include "ReconExterns.h"
#include "RawObject.h"

#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <typeinfo>

using Sexy::gEmptyStringData;
using Sexy::String;

namespace
{

const unsigned int kStringMax = 1073741820u;

// TextStream names the mode, character buffer, cursor and exhausted flag.

bool IsBlank(char theChar)
{
    return theChar == ' ' || theChar == '\t' || theChar == '\r' || theChar == '\n';
}

unsigned int NeedleLength(const char *theNeedle, unsigned int theCount)
{
    if (theCount != 0 || theNeedle == nullptr)
        return theCount;
    return (unsigned int)strlen(theNeedle);
}

void FillOwned(String *theString, unsigned int theCount, char theChar)
{
    if (theString == nullptr || theCount == 0)
        return;

    unsigned int aLength = theString->size();
    if (theCount > kStringMax - aLength)
        throw std::length_error("basic_string::append");

    std::string aFilled(theString->c_str(), aLength);
    aFilled.append(theCount, theChar);
    theString->Assign(aFilled.data(), (unsigned int)aFilled.size());
}

void ReplaceOwned(
    String *theString, unsigned int thePos, unsigned int theRemove, const char *theData, unsigned int theInsert)
{
    if (theString == nullptr)
        return;

    unsigned int aLength = theString->size();
    if (thePos > aLength)
        thePos = aLength;
    if (theRemove > aLength - thePos)
        theRemove = aLength - thePos;

    std::string aKept(theString->c_str(), aLength);
    const char *aInsert = (theData != nullptr) ? theData : "";
    aKept.replace(thePos, theRemove, aInsert, theInsert);
    theString->Assign(aKept.data(), (unsigned int)aKept.size());
}

char *FreshFilled(unsigned int theCount, char theChar)
{
    if (theCount == 0)
        return gEmptyStringData;

    String aBuilt(theCount, theChar);
    char *aData = aBuilt.data();
    aBuilt.mData = gEmptyStringData;
    return aData;
}

char *FreshRange(const char *theFirst, const char *theLast)
{
    if (theFirst == theLast || theFirst == nullptr)
    {
        if (theFirst == nullptr && theLast != nullptr)
            throw std::logic_error("basic_string::_S_construct null not valid");
        return gEmptyStringData;
    }

    String aBuilt(theFirst, (unsigned int)(theLast - theFirst));
    char *aData = aBuilt.data();
    aBuilt.mData = gEmptyStringData;
    return aData;
}

struct TextStream
{
    unsigned int mHead[10];
    unsigned int mMode;
    String mText;
    void *mStringBuf;
    unsigned int mCursor;
    unsigned int mGap[3];
    unsigned char mAtEnd;
};

TextStream *AsTextStream(void *theStream)
{
    return (TextStream *)theStream;
}

void MarkStreamFailed(TextStream *theStream)
{
    theStream->mAtEnd = (unsigned char)(theStream->mAtEnd | 2);
}

} // namespace

unsigned int StringFind(String *theString, const char *theText, unsigned int thePos, unsigned int theCount)
{
    if (theString == nullptr || theText == nullptr)
        return (unsigned int)-1;

    unsigned int aSize = theString->size();
    unsigned int aCount = NeedleLength(theText, theCount);
    if (aCount == 0)
        return (thePos <= aSize) ? thePos : (unsigned int)-1;
    if (aCount > aSize || thePos > aSize - aCount)
        return (unsigned int)-1;

    const char *aData = theString->c_str();
    for (unsigned int i = thePos; i + aCount <= aSize; ++i)
    {
        if (aData[i] == theText[0] && (aCount == 1 || memcmp(aData + i + 1, theText + 1, aCount - 1) == 0))
            return i;
    }
    return (unsigned int)-1;
}

unsigned int StringFindText(String *theString, const char *theText, unsigned int thePos)
{
    if (theText == nullptr)
        return (unsigned int)-1;
    return StringFind(theString, theText, thePos, (unsigned int)strlen(theText));
}

int StringFindChar(String *theString, int theChar, unsigned int thePos)
{
    if (theString == nullptr)
        return (int)(unsigned int)-1;

    unsigned int aSize = theString->size();
    if (thePos >= aSize)
        return (int)(unsigned int)-1;

    const char *aData = theString->c_str();
    const void *aHit = memchr(aData + thePos, (unsigned char)theChar, aSize - thePos);
    if (aHit == nullptr)
        return (int)(unsigned int)-1;
    return (int)((const char *)aHit - aData);
}

unsigned int StringRFind(String *theString, const void *theData, unsigned int thePos, unsigned int theCount)
{
    if (theString == nullptr)
        return (unsigned int)-1;
    if (theData == nullptr && theCount != 0)
        return (unsigned int)-1;

    unsigned int aSize = theString->size();
    if (theCount > aSize)
        return (unsigned int)-1;

    unsigned int aStart = aSize - theCount;
    if (aStart > thePos)
        aStart = thePos;

    const char *aData = theString->c_str();
    const char *aNeedle = (const char *)theData;
    for (;;)
    {
        if (theCount == 0 || memcmp(aData + aStart, aNeedle, theCount) == 0)
            return aStart;
        if (aStart == 0)
            return (unsigned int)-1;
        --aStart;
    }
}

int StringRFindChar(const String *theString, int theChar, unsigned int thePos)
{
    if (theString == nullptr || theString->size() == 0)
        return (int)(unsigned int)-1;

    unsigned int aPos = theString->size() - 1;
    if (thePos < aPos)
        aPos = thePos;

    const char *aData = theString->c_str();
    while (aPos != (unsigned int)-1 && (unsigned char)aData[aPos] != (unsigned char)theChar)
        --aPos;
    return (int)aPos;
}

unsigned int StringFindFirstNotOf(String *theString, const void *theData, unsigned int thePos, unsigned int theCount)
{
    if (theString == nullptr || theData == nullptr)
        return (unsigned int)-1;

    unsigned int aSize = theString->size();
    if (thePos >= aSize)
        return (unsigned int)-1;

    const char *aData = theString->c_str();
    unsigned int i = thePos;
    while (memchr(theData, (unsigned char)aData[i], theCount) != nullptr)
    {
        if (++i >= aSize)
            return (unsigned int)-1;
    }
    return i;
}

int Sexy::StringEqualCStr(const String *theString, const char *theText)
{
    if (theString == nullptr)
        return 0;
    return theString->compare(theText != nullptr ? theText : "");
}

char *CreateStringRep(unsigned int theCapacity, unsigned int theOldCapacity)
{
    (void)theOldCapacity;
    if (theCapacity > kStringMax)
        throw std::length_error("basic_string::_S_create");

    String aBuilt(theCapacity, '\0');
    char *aData = aBuilt.data();
    aBuilt.mData = gEmptyStringData;
    return aData;
}

char *StringFillConstruct(unsigned int theCapacity, int theChar)
{
    return FreshFilled(theCapacity, (char)(unsigned char)theChar);
}

char *StringFillSlot(char **theSlot, unsigned int theCapacity, int theChar)
{
    if (theSlot != nullptr)
        *theSlot = StringFillConstruct(theCapacity, theChar);
    return (char *)theSlot;
}

String *StringDestroy(String *theString)
{
    if (theString != nullptr)
        theString->clear();
    return theString;
}

String *StringMutate(String *theString, unsigned int thePos, unsigned int theCount, unsigned int theReplacement)
{
    ReplaceOwned(theString, thePos, theCount, nullptr, theReplacement);
    return theString;
}

char *StringLeakEnd(String *theString)
{
    if (theString == nullptr)
        return nullptr;
    return theString->data() + theString->size();
}

char *StringEraseRange(String *theString, char *theFirst, char *theLast)
{
    if (theString == nullptr)
        return theFirst;
    if (theFirst == nullptr || theLast == nullptr || theLast <= theFirst)
        return theFirst;

    unsigned int aPos = (unsigned int)(theFirst - theString->data());
    unsigned int aCount = (unsigned int)(theLast - theFirst);
    ReplaceOwned(theString, aPos, aCount, nullptr, 0);
    return theString->data() + aPos;
}

String *StringReplaceFill(
    String *theString, unsigned int thePos, unsigned int theCount, unsigned int theFill, unsigned char theChar)
{
    if (theString == nullptr)
        return theString;

    unsigned int aLength = theString->size();
    unsigned int aRemove = theCount;
    if (thePos > aLength)
        thePos = aLength;
    if (aRemove > aLength - thePos)
        aRemove = aLength - thePos;
    if (theFill > kStringMax - (aLength - aRemove))
        throw std::length_error("basic_string::_M_replace_aux");

    std::string aFill(theFill, (char)theChar);
    ReplaceOwned(theString, thePos, theCount, aFill.data(), theFill);
    return theString;
}

String *StringReserve(String *theString, unsigned int theCapacity)
{
    if (theString != nullptr)
        theString->reserve(theCapacity);
    return theString;
}

String *StringAppendText(String *theString, const char *theText)
{
    if (theString != nullptr && theText != nullptr)
        theString->Append(theText, (unsigned int)strlen(theText));
    return theString;
}

String *StringAppendFill(String *theString, unsigned int theCount, int theChar)
{
    FillOwned(theString, theCount, (char)(unsigned char)theChar);
    return theString;
}

String *StringResize(String *theString, unsigned int theSize, int theChar)
{
    if (theString == nullptr)
        return theString;
    if (theSize > kStringMax)
        throw std::length_error("basic_string::resize");

    unsigned int aSize = theString->size();
    if (theSize > aSize)
        FillOwned(theString, theSize - aSize, (char)(unsigned char)theChar);
    else if (theSize < aSize)
        ReplaceOwned(theString, theSize, aSize - theSize, nullptr, 0);
    return theString;
}

String *StringPushBack(String *theString, char theChar)
{
    if (theString != nullptr)
        theString->push_back(theChar);
    return theString;
}

String *StringCheckedSubstr(String *theResult, const String *theSource, unsigned int thePos, unsigned int theCount)
{
    if (theSource != nullptr && thePos > theSource->size())
        throw std::out_of_range("basic_string::substr");

    // A dropped count arrives as 0. The one-argument form means "to the end".
    unsigned int aCount = theCount;
    if (aCount == 0 && theSource != nullptr && thePos < theSource->size())
        aCount = theSource->size() - thePos;
    return StringSubstr(theResult, theSource, thePos, aCount);
}

char *StringConstructIter(const char *theFirst, const char *theLast, const void *theAlloc, int theTag)
{
    (void)theAlloc;
    (void)theTag;
    return FreshRange(theFirst, theLast);
}

String *StringAssignRange(String *theString, const char *theFirst, const char *theLast, int theChar)
{
    (void)theChar;
    if (theString == nullptr)
        return theString;

    if (theFirst == nullptr || theLast == nullptr || theLast < theFirst)
        theString->clear();
    else
        theString->Assign(theFirst, (unsigned int)(theLast - theFirst));
    return theString;
}

int StringReplace(
    String *theString, unsigned int thePos, unsigned int theCount, const char *theReplacement, unsigned int theFlags)
{
    if (theString == nullptr)
        return 0;

    unsigned int aLength = theString->size();
    if (thePos > aLength)
        throw std::out_of_range("basic_string::replace");

    unsigned int aRemove = theCount;
    if (aRemove > aLength - thePos)
        aRemove = aLength - thePos;
    if (theFlags > kStringMax - aLength + aRemove)
        throw std::length_error("basic_string::replace");

    const char *aInsert = theReplacement;
    ReplaceOwned(theString, thePos, theCount, aInsert, theFlags);
    return 1;
}

void Sexy::ThrowBadAlloc()
{
    throw std::bad_alloc();
}

void Sexy::ThrowBadCast()
{
    throw std::bad_cast();
}

void Sexy::ThrowInvalidArgument(const char *theMessage)
{
    throw std::logic_error(theMessage != nullptr ? theMessage : "logic error");
}

void Sexy::ThrowLengthError(const char *theMessage)
{
    throw std::length_error(theMessage != nullptr ? theMessage : "length error");
}

void Sexy::ThrowOutOfRange(const char *theMessage)
{
    throw std::out_of_range(theMessage != nullptr ? theMessage : "out of range");
}

void *IStreamReadInt(void *theStream, int *theValue)
{
    if (theStream == nullptr)
        return theStream;

    TextStream *aStream = AsTextStream(theStream);
    String *aText = &aStream->mText;
    const char *aData = aText->c_str();
    unsigned int aLength = aText->size();
    unsigned int aPos = aStream->mCursor;

    while (aPos < aLength && IsBlank(aData[aPos]))
        ++aPos;

    bool aNegative = false;
    if (aPos < aLength && (aData[aPos] == '-' || aData[aPos] == '+'))
    {
        aNegative = (aData[aPos] == '-');
        ++aPos;
    }

    unsigned int aFirstDigit = aPos;
    unsigned int aValue = 0;
    while (aPos < aLength && aData[aPos] >= '0' && aData[aPos] <= '9')
    {
        aValue = aValue * 10u + (unsigned int)(aData[aPos] - '0');
        ++aPos;
    }

    aStream->mCursor = aPos;
    if (aPos == aFirstDigit)
    {
        MarkStreamFailed(aStream);
        return theStream;
    }
    if (theValue != nullptr)
        *theValue = aNegative ? -(int)aValue : (int)aValue;
    return theStream;
}

void *IStreamGet(void *theStream, char *theText)
{
    if (theStream == nullptr)
        return theStream;

    TextStream *aStream = AsTextStream(theStream);
    unsigned int aPos = aStream->mCursor;
    if (aPos >= aStream->mText.size())
    {
        MarkStreamFailed(aStream);
        return theStream;
    }

    if (theText != nullptr)
        *theText = aStream->mText.c_str()[aPos];
    aStream->mCursor = aPos + 1;
    return theStream;
}

namespace
{
unsigned int gStreamToken = 1;
}

struct StringBuf
{
    void *mHeaderWord;
    unsigned int mZero[24];
    unsigned int mCapacity;
    char *mData;
};

void *StringBufConstruct(void *theStream)
{
    if (theStream == nullptr)
        return theStream;

    StringBuf *aBuf = (StringBuf *)theStream;
    memset(aBuf, 0, sizeof(StringBuf));
    aBuf->mCapacity = 8;
    return theStream;
}

void *StringBufDestroy(void *theStream)
{
    return theStream;
}

void *InitStreamToken(void *theStream)
{
    if (theStream != nullptr)
        *(unsigned int *)theStream = gStreamToken++;
    return theStream;
}

void *DestroyTextStream(void *theStream)
{
    if (theStream == nullptr)
        return theStream;

    TextStream *aStream = AsTextStream(theStream);
    aStream->mText.clear();
    aStream->mText.mData = gEmptyStringData;
    StringBufDestroy(aStream->mStringBuf);
    return theStream;
}

struct BufPointers
{
    void *mStreamHead;
    char *mBegin;
    char *mCurrent;
    char *mEnd;
    int mPad[4];
    int mMode;
    char *mText;
};

char *StringBufSetPointers(void *theBuffer, char *theData, int theOffset, int theCount)
{
    (void)theCount;
    if (theBuffer == nullptr)
        return theData;

    BufPointers *aBuf = (BufPointers *)theBuffer;
    unsigned int aLength = 0;
    unsigned int aCapacity = 0;
    if (aBuf->mText != nullptr && aBuf->mText != gEmptyStringData)
    {
        String aView(aBuf->mText);
        aLength = aView.size();
        aCapacity = aView.capacity();
        aView.mData = gEmptyStringData;
    }

    bool aOut = (aBuf->mMode & 8) != 0;
    bool aIn = (aBuf->mMode & 16) != 0;
    char *aEnd = theData + aLength;
    char *aResult = aEnd;
    char *aCurrent = theData;
    if (theData == aBuf->mText)
    {
        aResult = theData + aCapacity;
        aCurrent = theData + theOffset;
    }
    else
    {
        aEnd += theOffset;
        aResult = aEnd;
    }

    if (aOut)
    {
        aBuf->mBegin = theData;
        aBuf->mCurrent = aCurrent;
        aBuf->mEnd = aEnd;
    }
    if (aIn && !aOut)
    {
        aBuf->mBegin = aEnd;
        aBuf->mCurrent = aEnd;
        aBuf->mEnd = aEnd;
    }
    return aResult;
}

void *ConstructIStringStream(void *theStream, const String *theSource, int theMode)
{
    if (theStream == nullptr)
        return nullptr;

    TextStream *aStream = ConstructRaw(AsTextStream(theStream));
    aStream->mMode = (unsigned int)theMode | 8u;
    if (theSource != nullptr)
        aStream->mText = *theSource;
    return theStream;
}

struct FileBuf
{
    void *mHead[16];
    unsigned int mBufferSize;
    void *mTail[12];
};

void *FileBufConstruct(void *theFileBuf)
{
    if (theFileBuf == nullptr)
        return nullptr;

    FileBuf *aBuf = (FileBuf *)theFileBuf;
    memset(aBuf, 0, sizeof(FileBuf));
    aBuf->mBufferSize = 1024;
    InitStreamToken(aBuf->mTail);
    return theFileBuf;
}

void *FileBufClose(void *theFileBuf)
{
    return theFileBuf;
}

void *FileBufOpen(void *theFileBuf, const char *theFilename, int theMode)
{
    (void)theFilename;
    (void)theMode;
    return theFileBuf;
}

void *ConstructOutOfRange(void *theException, const String *theMessage)
{
    (void)theMessage;
    if (theException != nullptr)
    {
        int *aWords = (int *)theException;
        aWords[0] = 0;
        aWords[1] = 0;
    }
    return theException;
}

void *ConstructOutOfRangeCopy(void *theException, const String *theMessage)
{
    return ConstructOutOfRange(theException, theMessage);
}
