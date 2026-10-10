// Low-address shims the game still calls by the recovered names.
// String copies go through Sexy::String. Pixel, sort, video and file-reader
// helpers do the job the name describes.

#include "SexyTypes.h"
#include "ReconExterns.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "FontLayer.h"
#include "MemoryImage.h"
#include "DeviceImage.h"
#include "PixelUtil.h"
#include "SexyAppBase.h"
#include "LawnApp.h"
#include "TitleScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

extern Sexy::Image *IMAGE_REANIM_CREDITS_DISCOLIGHTS;

namespace
{

struct CharRep
{
    unsigned int mLength;
    unsigned int mCapacity;
    int mRefCount;
    char mChars[1];
};
// CharRep is the 12-byte control block. mChars is the first character.

CharRep *AsRep(void *theRep)
{
    return static_cast<CharRep *>(theRep);
}

bool IsPinnedRep(CharRep *theRep)
{
    return theRep == nullptr || theRep == (CharRep *)&Sexy::gEmptyStringRep || theRep->mRefCount < 0;
}

// theRep is the control block, not the character pointer. A negative count
// means the String destructor owns the buffer, so this leaves it alone.
void ReleaseRep(char *theRep)
{
    CharRep *aRep = AsRep(theRep);
    if (IsPinnedRep(aRep))
        return;

    Sexy::String aOwned(aRep->mChars);
    aOwned.clear();
}

// Callers add the header size to this return to reach the characters, so the
// return stays the control block. Length is written on that block.
char *SetLength(char *theRep, unsigned int theLength)
{
    CharRep *aRep = AsRep(theRep);
    if (aRep == nullptr)
        return theRep;

    aRep->mLength = theLength;
    aRep->mChars[theLength] = '\0';
    return theRep;
}

char *FreshText(const char *theText, unsigned int theLength)
{
    if (theText == nullptr || theLength == 0)
        return Sexy::gEmptyStringData;

    Sexy::String aBuilt(theText, theLength);
    char *aData = aBuilt.data();
    aBuilt.mData = Sexy::gEmptyStringData;
    return aData;
}

bool WidgetLess(Sexy::Widget *const &theLeft, Sexy::Widget *const &theRight)
{
    if (theLeft == nullptr || theRight == nullptr)
        return theLeft < theRight;
    if (theLeft->mZOrder == theRight->mZOrder)
        return theLeft->mPriority < theRight->mPriority;
    return theLeft->mZOrder < theRight->mZOrder;
}

void SortWidgets(const char *theBegin, const char *theEnd)
{
    Sexy::Widget **aFirst = (Sexy::Widget **)theBegin;
    Sexy::Widget **aLast = (Sexy::Widget **)theEnd;
    if (aFirst == nullptr || aLast == nullptr || aLast <= aFirst)
        return;
    std::sort(aFirst, aLast, WidgetLess);
}

struct FontLayerNode
{
    explicit FontLayerNode(const Sexy::FontLayer &theLayer) :
        mNext(nullptr),
        mPrev(nullptr),
        mLayer(theLayer)
    {
    }

    void *mNext;
    void *mPrev;
    Sexy::FontLayer mLayer;
};

int ReadPacked(const unsigned char *theBytes, int theWidth)
{
    unsigned int aPacked = 0;
    for (int i = 0; i < theWidth && i < 4; ++i)
        aPacked |= (unsigned int)theBytes[i] << (8 * i);
    return (int)aPacked;
}

} // namespace

char *DisposeWStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeMenuStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeSharedStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeBufferStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeEditStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeGraphicsStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeAppStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeXmlStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeLayoutStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *DisposeFontStringRep(char *theRep)
{
    ReleaseRep(theRep);
    return theRep;
}

char *SetDescParserRepLength(char *theRep, unsigned int theLength)
{
    return SetLength(theRep, theLength);
}

char *SetFontRepLength(char *theRep, unsigned int theLength)
{
    return SetLength(theRep, theLength);
}

char *SetXmlRepLength(char *theRep, unsigned int theLength)
{
    return SetLength(theRep, theLength);
}

Sexy::FontLayer *CloneFontLayerNode(const Sexy::FontLayer *theSource)
{
    if (theSource == nullptr)
        return nullptr;
    FontLayerNode *aNode = new FontLayerNode(*theSource);
    return &aNode->mLayer;
}

Sexy::String *StringConcatCStr(Sexy::String *theResult, const char *theLeft, const Sexy::String *theRight)
{
    if (theResult == nullptr)
        return theResult;

    theResult->mData = Sexy::gEmptyStringData;
    if (theLeft != nullptr)
        *theResult = theLeft;
    if (theRight != nullptr)
        theResult->Append(theRight->c_str(), theRight->size());
    return theResult;
}

Sexy::String *StringConcat(Sexy::String *theResult, const Sexy::String *theLeft, const Sexy::String *theRight)
{
    if (theResult == nullptr)
        return theResult;

    theResult->mData = Sexy::gEmptyStringData;
    if (theLeft != nullptr)
        *theResult = *theLeft;
    if (theRight != nullptr)
        theResult->Append(theRight->c_str(), theRight->size());
    return theResult;
}

char *StringConstructRange(const char *theBegin, const char *theEnd, const void *theAlloc, int theTag)
{
    (void)theAlloc;
    (void)theTag;
    if (theBegin == nullptr)
    {
        if (theEnd != nullptr)
            Sexy::ThrowInvalidArgument("basic_string::_S_construct null not valid");
        return Sexy::gEmptyStringData;
    }
    if (theEnd == nullptr || theEnd < theBegin)
        return Sexy::gEmptyStringData;
    return FreshText(theBegin, (unsigned int)(theEnd - theBegin));
}

char *IntStringAllocRep(unsigned int theCapacity, unsigned int theOldCapacity)
{
    (void)theOldCapacity;
    if (theCapacity == 0)
        return (char *)&Sexy::gEmptyStringRep;

    return FreshText("", theCapacity);
}

int CloseIntroVideo()
{
    AGViewRemoveEventListener(OnIntroVideoEvent);
    AGViewShow(1);
    AGVideoClose();
    AGVideoShow(0);
    return AGVideoEnable(0);
}

int *OnIntroVideoEvent(int *theEvent)
{
    if (theEvent == nullptr)
        return theEvent;
    if ((unsigned int)(*theEvent - 13) > 1u)
        return theEvent;

    AGViewShow(1);
    AGVideoShow(0);
    if (gLawnApp != nullptr && gLawnApp->mTitleScreen != nullptr)
        gLawnApp->mTitleScreen->VideoCompleted();
    return theEvent;
}

int DrawCreditsDiscoLights(Sexy::Graphics *theGraphics, float theCentreX, float theAngle)
{
    if (theGraphics == nullptr || gSexyAppBase == nullptr)
        return 0;
    if (gSexyAppBase->Is3DAccelerated() == 0)
        return 0;
    if (IMAGE_REANIM_CREDITS_DISCOLIGHTS == nullptr)
        return 0;

    const float kQuadrant = 1.5708f;
    float aCentres[4] = {theAngle, theAngle + kQuadrant, theAngle + kQuadrant * 2.0f, theAngle + kQuadrant * 3.0f};

    Sexy::Color aSaved = theGraphics->GetColor();
    theGraphics->SetColor(Sexy::Color(255, 255, 255, 128));
    for (int i = 0; i < 4; ++i)
    {
        float aX = cosf(aCentres[i]) * 600.0f + theCentreX;
        float aY = sinf(aCentres[i]) * 200.0f + 450.0f;
        theGraphics->DrawImageF(IMAGE_REANIM_CREDITS_DISCOLIGHTS, aX, aY);
    }
    theGraphics->SetColor(aSaved);
    return 1;
}

int FillIntVectorFromCsv(Sexy::String *theText, void *theOut)
{
    if (theText == nullptr || theOut == nullptr)
        return 0;

    std::vector<int> *aOut = (std::vector<int> *)theOut;
    aOut->clear();

    const char *aData = theText->c_str();
    unsigned int aPos = 0;
    int aFound = 0;
    do
    {
        aOut->push_back(atoi(aData + aPos));
        aFound = StringFindChar(theText, ',', aPos);
        aPos = (unsigned int)(aFound + 1);
    } while (aFound != -1);
    return aFound;
}

Sexy::MemoryImage *MakeCursorImage(Sexy::SexyAppBase *theApp, const unsigned char *thePixels)
{
    Sexy::MemoryImage *aImage = new Sexy::MemoryImage(theApp);
    aImage->Create(24, 24);
    unsigned int *aBits = aImage->GetBits();
    if (aBits != nullptr && thePixels != nullptr)
        memcpy(aBits, thePixels, 2304);
    aImage->BitsChanged();
    return aImage;
}

const char *SortWidgetRangeFinal(const char *theBegin, const char *theEnd)
{
    SortWidgets(theBegin, theEnd);
    return theBegin;
}

const char *SortWidgetRangeIntro(const char *theBegin, const char *theEnd, int theFlags)
{
    (void)theFlags;
    SortWidgets(theBegin, theEnd);
    return theBegin;
}

int *GrowIntString(int *theRep, int theExtra)
{
    CharRep *aRep = AsRep(theRep);
    if (aRep == nullptr)
        return nullptr;

    unsigned int aLength = aRep->mLength + (unsigned int)theExtra;
    char *aFresh = FreshText(aRep->mChars, aLength);
    return (int *)aFresh;
}

int *GrowIntStringFresh(int *theRange, int theCount)
{
    return GrowIntString(theRange, theCount);
}

int ReadDevicePixel(void *theImage, int theX, int theY)
{
    Sexy::DeviceImage *aImage = (Sexy::DeviceImage *)theImage;
    if (aImage == nullptr)
        return 0;
    if (aImage->mSurface == nullptr || aImage->mSurfaceBits == nullptr)
        return (int)((Sexy::MemoryImage *)aImage)->MemoryImage::GetPixel(theX, theY);

    int aPitch = aImage->mSurfacePitch;
    if (aPitch <= 0)
        aPitch = aImage->GetPitch();

    Sexy::PixelFormat aFormat = (Sexy::PixelFormat)aImage->mSurfaceFormat;
    const unsigned char *aBytes =
        (const unsigned char *)aImage->mSurfaceBits + theX * Sexy::PixelUtil::GetBytePerPixel(aFormat) + aPitch * theY;

    if (aFormat == Sexy::PIXEL_FORMAT_A8R8G8B8)
        return ReadPacked(aBytes, 4);
    if (aFormat == Sexy::PIXEL_FORMAT_LUT8)
    {
        unsigned int *aPalette = aImage->GetPallete();
        if (aPalette == nullptr)
            return 0;
        return (int)aPalette[aBytes[0]];
    }
    return Sexy::PixelUtil::UnpackPixel(aFormat, aBytes);
}

unsigned char *ReadChecksummedBytes(void *theReader, void *theDest, int theCount)
{
    Sexy::PIEffect *aReader = (Sexy::PIEffect *)theReader;
    unsigned char *aDest = (unsigned char *)theDest;
    if (aReader == nullptr || aDest == nullptr || theCount <= 0)
        return aDest;

    unsigned char *aEnd = aDest + theCount;
    while (aDest != aEnd)
    {
        if (aReader->mReadPos == 1024)
        {
            aReader->mReadPos = 0;
            unsigned int aWords = 0;
            if (aReader->mReadFile != nullptr)
            {
                aWords = (unsigned int)p_fread(aReader->mReadBuf, 1, 1024, aReader->mReadFile) >> 2;
            }
            for (unsigned int i = 0; i < aWords; ++i)
            {
                aReader->mChecksum += aReader->mReadWords[i] ^ i;
            }
        }

        *aDest = aReader->mReadBuf[aReader->mReadPos];
        ++aDest;
        ++aReader->mReadPos;
    }
    return (unsigned char *)theDest;
}
