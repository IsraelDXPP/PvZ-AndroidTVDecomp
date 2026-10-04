// Engine helpers the decompiler left unnamed: the debug-draw flag, the
// four-byte-character string rep, remap splicing, and one Bezier arc-length
// piece. Signatures match ReconExterns.h.

#include "SexyTypes.h"
#include "ReconExterns.h"
#include "ReconTypes.h"
#include "Bezier.h"
#include "SexyString.h"

#include <cstring>
#include <new>

namespace
{

inline Sexy::StringRep* RepAt(void* theRep)
{
    return static_cast<Sexy::StringRep*>(theRep);
}

inline float Lerp(float theT, float theA, float theB)
{
    return (theA * (1.0f - theT)) + (theB * theT);
}

Sexy::String& RemapResult()
{
    static Sexy::String sResult;
    return sResult;
}

} // namespace

// Cached SEXY_DEBUG_DRAW_STRING option. Graphics asks on every string draw.
bool DebugDrawStringEnabled()
{
    static bool sDebugDrawStringCached = false;
    static bool sDebugDrawStringValue = false;

    if (sDebugDrawStringCached)
        return sDebugDrawStringValue;

    sDebugDrawStringValue = Sexy::GetEnvOption("SEXY_DEBUG_DRAW_STRING", false) != 0;
    sDebugDrawStringCached = true;
    return sDebugDrawStringValue;
}

// Clone a wide-character control block, leaving room for theExtra more
// characters. theRep is the old control block (character pointer minus the
// header). The old buffer is not released.
char* CloneWideStringChars(char* theRep, int theExtra)
{
    Sexy::StringRep* aOld = RepAt(theRep);
    if (aOld == nullptr)
        return nullptr;

    unsigned int aLength = aOld->mLength;
    char* aNewRep = CreateWideStringRep(aLength + (unsigned int)theExtra, aOld->mCapacity);
    Sexy::StringRep* aNew = RepAt(aNewRep);
    if (aNew == nullptr)
        return nullptr;

    if (aLength != 0)
        memmove(aNew->Data(), aOld->Data(), 4u * aLength);

    if (aNew != &Sexy::gEmptyStringRep)
    {
        aNew->mLength = aLength;
        aNew->mRefCount = 0;
        void* aCharStorage = aNew->Data();
        unsigned int* aChars = static_cast<unsigned int*>(aCharStorage);
        aChars[aLength] = 0;
    }

    return aNew->Data();
}

// Read a length-prefixed string from a definition-cache cursor.
int ReadEndianString(void* theStream, Sexy::String* theText)
{
    if (theText == nullptr)
        return 0;

    int aLength = 0;
    SMemR(static_cast<unsigned char**>(theStream), &aLength, 4);
    aLength = EndianInt(aLength);
    if (aLength < 0)
        aLength = 0;

    StringResize(theText, (unsigned int)aLength, 0);
    return SMemR(static_cast<unsigned char**>(theStream), theText->data(), aLength);
}

// Drop one reference on a wide-character control block and free it when the
// count reaches zero. A null argument is a call site that lost the pointer.
void ReleaseWideStringRep(unsigned int* theRep)
{
    Sexy::StringRep* aRep = RepAt(theRep);
    if (aRep == nullptr)
        return;

    Sexy::StringRelease(aRep);
}

// Allocate a wide-character control block. Length is left for the caller.
char* CreateWideStringRep(unsigned int theCapacity, unsigned int theOldCapacity)
{
    unsigned int aCapacity = theCapacity;

    if (aCapacity > 0x0FFFFFFEu)
        Sexy::ThrowLengthError("basic_string::_S_create");

    if (aCapacity > theOldCapacity && aCapacity < 2 * theOldCapacity)
        aCapacity = 2 * theOldCapacity;

    unsigned int aBytes = 4 * (aCapacity + 4);
    bool aRoundToPage = (aBytes + 16 > 0x1000) && (theOldCapacity < aCapacity);
    if (aRoundToPage)
    {
        aCapacity += (4096 - ((aBytes + 16) & 0xFFFu)) >> 2;
        if (aCapacity >= 0x0FFFFFFEu)
            aCapacity = 268435454u;
        aBytes = 4 * (aCapacity + 4);
    }

    void* aStorage = new char[aBytes];
    Sexy::StringRep* aRep = static_cast<Sexy::StringRep*>(aStorage);
    aRep->mLength = 0;
    aRep->mCapacity = aCapacity;
    aRep->mRefCount = 0;
    return static_cast<char*>(aStorage);
}

// Hand back the characters of a wide-character rep. A negative refcount means
// the buffer is pinned, so the caller gets a private copy. Otherwise the
// count is raised and the caller lowers it with ReleaseWideStringRep.
unsigned int* LeakWideStringData(unsigned int* theRep)
{
    Sexy::StringRep* aRep = RepAt(theRep);
    if (aRep == nullptr)
        return nullptr;

    if (aRep->mRefCount < 0)
    {
        unsigned int aLength = aRep->mLength;
        char* aNewRep = CreateWideStringRep(aLength, aRep->mCapacity);
        Sexy::StringRep* aNew = RepAt(aNewRep);
        if (aNew == nullptr)
            return nullptr;

        if (aLength != 0)
            memmove(aNew->Data(), aRep->Data(), 4u * aLength);

        if (aNew != &Sexy::gEmptyStringRep)
            SetFontRepLength(aNewRep, aLength);

        void* aCharStorage = aNew->Data();
        return static_cast<unsigned int*>(aCharStorage);
    }

    if (aRep != &Sexy::gEmptyStringRep)
        Sexy::StringAddRef(aRep);

    void* aCharStorage = aRep->Data();
    return static_cast<unsigned int*>(aCharStorage);
}

// Splice a matched remap pattern. A star on the replacement keeps that side
// of the input. The result lives in a function-local static; the caller
// copies it immediately.
Sexy::String* SpliceRemapPattern(const Sexy::String* theInput, unsigned int theBegin,
                                 unsigned int theEnd, const Sexy::String* theValue)
{
    Sexy::String& aResult = RemapResult();
    if (theInput == nullptr || theValue == nullptr)
        return &aResult;

    unsigned int aValueLength = theValue->size();
    if (aValueLength == 0)
    {
        aResult.clear();
        return &aResult;
    }

    bool aWildcardHead = ((*theValue)[0] == '*');
    bool aWildcardTail = ((*theValue)[aValueLength - 1] == '*');

    if (!aWildcardHead && !aWildcardTail)
        aResult = *theValue;
    else if (!aWildcardHead)
        aResult = theValue->substr(0, aValueLength - 1) + theInput->substr(theEnd);
    else if (aValueLength == 1)
        aResult = theInput->substr(0, theBegin) + theInput->substr(theEnd);
    else if (!aWildcardTail)
        aResult = theInput->substr(0, theBegin) + theValue->substr(1, aValueLength - 1);
    else
        aResult = theInput->substr(0, theBegin)
                + theValue->substr(1, aValueLength - 2)
                + theInput->substr(theEnd);

    return &aResult;
}

// Arc length of one cubic piece. theCurve is a Bezier. theStart and theEnd
// are the parameters along the segment, clamped to 0..1.
float MeasureCurvePiece(void* theCurve, int theIndex, float theStart, float theEnd)
{
    if (theCurve == nullptr)
        return 0.0f;

    Sexy::Bezier* aCurve = static_cast<Sexy::Bezier*>(theCurve);
    const float* aControls = aCurve->mDerivativePoints;
    const float* aPoints = aCurve->mPoints;
    if (aControls == nullptr || aPoints == nullptr)
        return 0.0f;

    float aFirst = theStart;
    float aSecond = theEnd;
    if (aFirst < 0.0f)
        aFirst = 0.0f;
    if (aSecond > 1.0f)
        aSecond = 1.0f;

    float aP0x = aPoints[2 * theIndex];
    float aP0y = aPoints[2 * theIndex + 1];
    float aP1x = aControls[4 * theIndex];
    float aP1y = aControls[4 * theIndex + 1];
    float aP2x = aControls[4 * theIndex + 2];
    float aP2y = aControls[4 * theIndex + 3];
    float aP3x = aPoints[2 * theIndex + 2];
    float aP3y = aPoints[2 * theIndex + 3];

    float aL1ax = Lerp(aSecond, aP0x, aP1x);
    float aL1ay = Lerp(aSecond, aP0y, aP1y);
    float aL1bx = Lerp(aSecond, aP1x, aP2x);
    float aL1by = Lerp(aSecond, aP1y, aP2y);
    float aL1cx = Lerp(aSecond, aP2x, aP3x);
    float aL1cy = Lerp(aSecond, aP2y, aP3y);

    float aL2ax = Lerp(aSecond, aL1ax, aL1bx);
    float aL2ay = Lerp(aSecond, aL1ay, aL1by);
    float aL2bx = Lerp(aSecond, aL1bx, aL1cx);
    float aL2by = Lerp(aSecond, aL1by, aL1cy);
    float aL3x = Lerp(aSecond, aL2ax, aL2bx);
    float aL3y = Lerp(aSecond, aL2ay, aL2by);

    float aM1x = Lerp(aFirst, aP0x, aL1ax);
    float aM1y = Lerp(aFirst, aP0y, aL1ay);
    float aM2x = Lerp(aFirst, aL1ax, aL2ax);
    float aM2y = Lerp(aFirst, aL1ay, aL2ay);
    float aM3x = Lerp(aFirst, aL2ax, aL3x);
    float aM3y = Lerp(aFirst, aL2ay, aL3y);
    float aN1x = Lerp(aFirst, aM1x, aM2x);
    float aN1y = Lerp(aFirst, aM1y, aM2y);
    float aN2x = Lerp(aFirst, aM2x, aM3x);
    float aN2y = Lerp(aFirst, aM2y, aM3y);
    float aOx = Lerp(aFirst, aN1x, aN2x);
    float aOy = Lerp(aFirst, aN1y, aN2y);

    TPoint<float> aPoint1(aOx, aOy);
    TPoint<float> aPoint2(aN2x, aN2y);
    TPoint<float> aPoint3(aM3x, aM3y);
    TPoint<float> aPoint4(aL3x, aL3y);
    return aCurve->SubdivideLength(aPoint1, aPoint2, aPoint3, aPoint4);
}

Sexy::String* AssignStringThunk(Sexy::String* theTarget, const Sexy::String* theSource)
{
    return StringAssign(theTarget, theSource);
}

void* HookListNode(void* theNode, void* thePos)
{
    if (theNode == nullptr || thePos == nullptr)
        return nullptr;
    return ListHook(theNode, thePos);
}

Sexy::String* MutateStringThunk(Sexy::String* theString, unsigned int thePos,
                                unsigned int theCount, unsigned int theReplacement)
{
    return StringMutate(theString, thePos, theCount, theReplacement);
}
