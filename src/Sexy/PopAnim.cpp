// PopAnim loads a .pam (or a .txt mod that names one) and draws its sprites.
// Numeric scales, frame flags, and layout formulas are the binary's.

#include "PopAnim.h"

#include "MemoryImage.h"
#include "ReconHelpers.h"
#include "ReconExterns.h"
#include "SingleDataElement.h"

#include <cstdlib>
#include <cstring>

namespace Sexy {

namespace {

const float kFixedScale = 0.000015259f;     // 1/65536
const float kImageMatrixScale = 7.6293945e-7f;
const float kShortScale = 0.05f;
const float kAngleScale = 0.001f;
const int kPamMagic = -1158669996;          // 0xBAF01954
const int kPoolSlots = 128;

int ChannelMix(int theFrom, int theTo, unsigned int theBlend)
{
    return (int)(((theTo & 0xFF) * theBlend + (theFrom & 0xFF) * (256 - theBlend)) >> 8);
}

Color MixPacked(int theFrom, int theTo, float theFraction)
{
    if (theFrom == theTo)
    {
        Color aColor;
        Color::FromInt(&aColor, theFrom);
        return aColor;
    }
    unsigned int aBlend = (unsigned int)(theFraction * 256.0f);
    unsigned int aKeep = 256 - aBlend;
    Color aColor;
    aColor.mRed = (int)((((theTo & 0xFF0000) * aBlend + (theFrom & 0xFF0000) * aKeep) >> 24) & 0xFF);
    aColor.mGreen = (int)((((theTo & 0xFF00) * aBlend + (theFrom & 0xFF00) * aKeep) >> 16) & 0xFF);
    aColor.mBlue = ChannelMix(theFrom, theTo, aBlend);
    aColor.mAlpha = ChannelMix(theFrom >> 24, theTo >> 24, aBlend);
    return aColor;
}

Color LerpColor(Color const& theFrom, Color const& theTo, float theBlend)
{
    float aKeep = 1.0f - theBlend;
    return Color(
        (int)(theTo.mRed * theBlend + 0.5f + aKeep * (float)theFrom.mRed),
        (int)(theTo.mGreen * theBlend + 0.5f + aKeep * (float)theFrom.mGreen),
        (int)(theTo.mBlue * theBlend + 0.5f + aKeep * (float)theFrom.mBlue),
        (int)(theTo.mAlpha * theBlend + 0.5f + aKeep * (float)theFrom.mAlpha));
}

String Lowered(String const& theText)
{
    String aLower;
    Lower(&aLower, &theText);
    return aLower;
}

String Trimmed(String const& theText)
{
    String aTrimmed;
    Trim(&aTrimmed, &theText);
    return aTrimmed;
}

String ExtensionOf(String const& thePath)
{
    const char* aDot = strrchr(thePath.c_str(), '.');
    if (aDot == nullptr)
        return String();
    String aTail = thePath.substr((unsigned int)(aDot - thePath.c_str()));
    return Lowered(aTail);
}

int FindChar(String const& theText, char theChar, unsigned int theFrom)
{
    const char* aChars = theText.c_str();
    unsigned int aLength = theText.size();
    for (unsigned int aIndex = theFrom; aIndex < aLength; ++aIndex)
    {
        if (aChars[aIndex] == theChar)
            return (int)aIndex;
    }
    return -1;
}

int FindText(String const& theText, char const* theNeedle)
{
    const char* aFound = strstr(theText.c_str(), theNeedle);
    if (aFound == nullptr)
        return -1;
    return (int)(aFound - theText.c_str());
}

String StripSpaces(String const& theText)
{
    String aClean;
    const char* aChars = theText.c_str();
    for (unsigned int aIndex = 0; aIndex < theText.size(); ++aIndex)
    {
        if (aChars[aIndex] != ' ')
            aClean += aChars[aIndex];
    }
    return aClean;
}

int ReadCount(Buffer* theBuffer)
{
    int aCount = theBuffer->ReadByte();
    if (aCount == 255)
        aCount = theBuffer->ReadShort();
    return aCount;
}

struct CommandToken
{
    String mText;
    bool mMore;
};

CommandToken NextToken(String& theArgs)
{
    CommandToken aToken;
    int aComma = FindChar(theArgs, ',', 0);
    String aSlice = aComma < 0 ? theArgs : theArgs.substr(0, (unsigned int)aComma);
    aToken.mText = Trimmed(aSlice);
    aToken.mMore = aComma >= 0;
    if (aComma >= 0)
        theArgs = theArgs.substr((unsigned int)(aComma + 1));
    return aToken;
}

float PoseDistance(SexyMatrix3 const& theFrom, SexyMatrix3 const& theTo)
{
    SexyVector2 aOrigin(0.0f, 0.0f);
    SexyVector2 aStart = theFrom * aOrigin;
    SexyVector2 aEnd = theTo * aOrigin;
    float aDeltaX = aEnd.mX - aStart.mX;
    float aDeltaY = aEnd.mY - aStart.mY;
    return sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
}

PAObjectPos* PoseAt(PAFrame* theFrame, int theIndex, int theObjectId)
{
    if (theFrame == nullptr || theFrame->mObjects.empty())
        return nullptr;
    if (theIndex < (int)theFrame->mObjects.size())
    {
        PAObjectPos* aDirect = theFrame->mObjects[theIndex];
        if (aDirect != nullptr && aDirect->mDef != nullptr && aDirect->mDef->mIndex == theObjectId)
            return aDirect;
    }
    for (size_t aScan = 0; aScan < theFrame->mObjects.size(); ++aScan)
    {
        PAObjectPos* aCandidate = theFrame->mObjects[aScan];
        if (aCandidate != nullptr && aCandidate->mDef != nullptr && aCandidate->mDef->mIndex == theObjectId)
            return aCandidate;
    }
    return nullptr;
}

Image* ImageOf(SharedImageRef const& theRef)
{
    return theRef.operator Sexy::Image*();
}

} // namespace

// ---------------------------------------------------------------------------
// Element types
// ---------------------------------------------------------------------------

PAImage::PAImage()
    : mWidth(-1)
    , mHeight(-1)
    , mNumCols(1)
    , mNumRows(1)
    , mDrawMode(0)
    , mTransX(0.0f)
    , mTransY(0.0f)
{
    mTransform.LoadIdentity();
}

PAObjectDef::PAObjectDef()
    : mName(nullptr)
    , mIndex(0)
    , mIsSprite(false)
    , mAdditive(false)
    , mImageIndex(0)
    , mAdvanceCount(0)
    , mRateScale(1.0f)
    , mUseSrcRect(false)
    , mSrcRect(0, 0, 0, 0)
    , mNestedDef(nullptr)
{
}

PAObjectPos::PAObjectPos()
    : mPackedColor(-1)
    , mInterpolate(false)
    , mDef(nullptr)
{
    mMatrix.LoadIdentity();
}

PAFrame::PAFrame()
    : mHold(false)
{
}

PASpriteDef::PASpriteDef()
    : mName(nullptr)
    , mFirstFrame(0)
    , mLastFrame(0)
    , mPingPong(false)
    , mRate(1.0f)
{
}

int PASpriteDef::GetLabelFrame(String const& theName) const
{
    String aUpper;
    StringToUpper(&aUpper, theName.c_str());
    std::map<String, int>::const_iterator aFound = mLabels.find(aUpper);
    if (aFound == mLabels.end())
        return -1;
    return aFound->second;
}

PAParticleEffect::PAParticleEffect()
    : mBaseRes(nullptr)
    , mEffect(nullptr)
    , mUpdateStamp(0)
    , mDrawAdditive(false)
    , mFollowSprite(false)
    , mActive(false)
    , mOffsetX(0.0)
    , mOffsetY(0.0)
{
}

PAParticleEffect::PAParticleEffect(PAParticleEffect const& theOther)
    : mBaseRes(theOther.mBaseRes)
    , mEffect(theOther.mEffect)
    , mName(theOther.mName)
    , mUpdateStamp(theOther.mUpdateStamp)
    , mDrawAdditive(theOther.mDrawAdditive)
    , mFollowSprite(theOther.mFollowSprite)
    , mActive(theOther.mActive)
    , mOffsetX(theOther.mOffsetX)
    , mOffsetY(theOther.mOffsetY)
{
    if (mBaseRes != nullptr)
        ++mBaseRes->mRefCount;
}

PAParticleEffect& PAParticleEffect::operator=(PAParticleEffect const& theOther)
{
    if (this == &theOther)
        return *this;
    if (mBaseRes != nullptr)
    {
        ResourceRef aHolder;
        aHolder.mBaseRes = mBaseRes;
        aHolder.Release();
    }
    mBaseRes = theOther.mBaseRes;
    if (mBaseRes != nullptr)
        ++mBaseRes->mRefCount;
    mEffect = theOther.mEffect;
    mName = theOther.mName;
    mUpdateStamp = theOther.mUpdateStamp;
    mDrawAdditive = theOther.mDrawAdditive;
    mFollowSprite = theOther.mFollowSprite;
    mActive = theOther.mActive;
    mOffsetX = theOther.mOffsetX;
    mOffsetY = theOther.mOffsetY;
    return *this;
}

PAParticleEffect::~PAParticleEffect()
{
    if (mBaseRes != nullptr)
    {
        ResourceRef aHolder;
        aHolder.mBaseRes = mBaseRes;
        aHolder.Release();
        mBaseRes = nullptr;
    }
}

PAObjectInst::PAObjectInst()
    : mName(nullptr)
    , mInst(nullptr)
    , mPrevRed(0)
    , mPrevGreen(0)
    , mPrevBlue(0)
    , mPrevAlpha(255)
    , mBlending(false)
    , mColor(Color::White)
    , mQueryDraw(true)
    , mUseDrawHelper(true)
    , mQueryDrawn(true)
{
}

int PAObjectInst::GetObjectInst(String const& theName)
{
    if (mInst != nullptr)
        return mInst->GetObjectInst(theName);
    return 0;
}

PASpriteInst::PASpriteInst()
    : mParent(nullptr)
    , mHoldFrames(0)
    , mFrameTime(0.0f)
    , mLoopCount(0)
    , mFrameAdvanced(true)
    , mLastStamp(-1)
    , mColor(Color(0, 0, 0, 255))
    , mDef(nullptr)
{
}

PASpriteInst::~PASpriteInst()
{
    for (size_t aIndex = 0; aIndex < mParticles.size(); ++aIndex)
        delete mParticles[aIndex].mEffect;
    for (size_t aIndex = 0; aIndex < mObjects.size(); ++aIndex)
        delete mObjects[aIndex].mInst;
}

int PASpriteInst::GetObjectInst(String const& theName)
{
    for (size_t aIndex = 0; aIndex < mObjects.size(); ++aIndex)
    {
        const char* aName = mObjects[aIndex].mName;
        if (aName != nullptr && strcasecmp(aName, theName.c_str()) == 0)
            return (int)aIndex;
        if (mObjects[aIndex].mInst != nullptr)
        {
            int aNested = mObjects[aIndex].mInst->GetObjectInst(theName);
            if (aNested != 0)
                return aNested;
        }
    }
    return 0;
}

PASpriteTable::PASpriteTable()
    : mTransformBlock(nullptr)
    , mTransformSlot(0)
    , mDataBlock(nullptr)
    , mDataSlot(0)
    , mMainDef(nullptr)
    , mRefCount(0)
{
}

PASpriteTable::~PASpriteTable()
{
    for (size_t aIndex = 0; aIndex < mTransformBlocks.size(); ++aIndex)
        delete[] mTransformBlocks[aIndex];
    for (size_t aIndex = 0; aIndex < mDataBlocks.size(); ++aIndex)
        delete[] mDataBlocks[aIndex];
    delete mMainDef;
}

PAObjectPos* PASpriteTable::AllocTransform()
{
    if (mTransformBlock == nullptr || mTransformSlot >= kPoolSlots)
    {
        mTransformBlock = new PAObjectPos[kPoolSlots];
        mTransformBlocks.push_back(mTransformBlock);
        mTransformSlot = 0;
    }
    return &mTransformBlock[mTransformSlot++];
}

PAObjectDef* PASpriteTable::AllocData()
{
    if (mDataBlock == nullptr || mDataSlot >= kPoolSlots)
    {
        mDataBlock = new PAObjectDef[kPoolSlots];
        mDataBlocks.push_back(mDataBlock);
        mDataSlot = 0;
    }
    return &mDataBlock[mDataSlot++];
}

const char* PASpriteTable::Intern(String const& theName)
{
    mNames.push_back(theName);
    return mNames.back().c_str();
}

void PopAnimListener::PlaySample(String const&, int, double, double) {}
PIEffect* PopAnimListener::GetParticleEffect(String const&) { return nullptr; }
bool PopAnimListener::ShouldDrawObject(int, Graphics*, PASpriteInst*, PAObjectInst*, PATransform*, Color const&) { return true; }
bool PopAnimListener::DidDrawObject(int, Graphics*, PASpriteInst*, PAObjectInst*, PATransform*, Color const&) { return true; }
int PopAnimListener::DrawObjectHelper(int, PASpriteInst*, PAObjectInst*, float*, Image*, Graphics*, float) { return 0; }
bool PopAnimListener::OnSpriteInstFrameEnd(int) { return false; }
bool PopAnimListener::OnSpriteInstLoop(int, int) { return false; }
void PopAnimListener::UnusedSlot() {}
bool PopAnimListener::OnCommand(int, PASpriteInst*, String const&, String const&) { return false; }

PATransform::PATransform()
{
    mMatrix.LoadIdentity();
}

PATransform& PATransform::TransformSrc(PATransform const& theSource, float const* theValues)
{
    SexyMatrix3 aIncoming;
    aIncoming.LoadIdentity();
    if (theValues != nullptr)
    {
        aIncoming.m00 = theValues[0];
        aIncoming.m01 = theValues[1];
        aIncoming.m02 = theValues[2];
        aIncoming.m10 = theValues[3];
        aIncoming.m11 = theValues[4];
        aIncoming.m12 = theValues[5];
    }
    mMatrix = theSource.mMatrix * aIncoming;
    return *this;
}

PATransform& PATransform::InterpolateTo(PATransform const& theFrom, float const* theTo, float theFraction)
{
    float aKeep = 1.0f - theFraction;
    float const* aFrom = theFrom.mMatrix.m;
    for (int aIndex = 0; aIndex < 9; ++aIndex)
    {
        float aTarget = theTo != nullptr ? theTo[aIndex] : 0.0f;
        mMatrix.m[aIndex] = aFrom[aIndex] * aKeep + aTarget * theFraction;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Mod file
// ---------------------------------------------------------------------------

class PopAnimModParser : public DescParser
{
public:
    PopAnim* mAnim;
    int mStage;

    explicit PopAnimModParser(PopAnim* theAnim)
        : mAnim(theAnim)
        , mStage(1)
    {
        mCmdSep = CMDSEP_NO_INDENT;
    }

    bool Error(String const& theError) override
    {
        String aMessage = mError;
        aMessage += theError;
        String aLine;
        StrFormat(&aLine, "%d:\r\n\r\n", mCurrentLineNum);
        aMessage += aLine;
        aMessage += " on Line ";
        aMessage += mCurrentLine;
        if (gSexyAppBase != nullptr)
            gSexyAppBase->Popup(aMessage);
        return false;
    }

    int SetParamHelper(PASpriteDef* theDef, String const& theName, int* theIndex, String const& theValue)
    {
        if (theDef == nullptr || theIndex == nullptr || *theIndex == 0)
            return 0;
        for (size_t aFrame = 0; aFrame < theDef->mFrames.size(); ++aFrame)
        {
            std::vector<PACommand>& aCommands = theDef->mFrames[aFrame].mCommands;
            for (size_t aCommand = 0; aCommand < aCommands.size(); ++aCommand)
            {
                if (strcasecmp(aCommands[aCommand].mName.c_str(), theName.c_str()) != 0)
                    continue;
                aCommands[aCommand].mArgs = theValue;
                if (*theIndex > 1 || *theIndex >= 0)
                    --(*theIndex);
                if (*theIndex == 0)
                    return 1;
            }
        }
        return 0;
    }

    bool HandleCommand(ListDataElement const& theParams) override
    {
        if (theParams.mDataElements.empty())
            return Error(String("Unknown Command"));
        DataElement* aHead = theParams.mDataElements[0];
        String aCommand;
        if (DataToString(aHead, &aCommand) == 0)
            return Error(String("Unknown Command"));
        String aLower = Lowered(aCommand);
        int aCount = (int)theParams.mDataElements.size();

        if (aLower == "setpamfile")
        {
            if (mStage != 1)
                return true;
            if (aCount != 2)
                return Error(String("Invalid Number of Parameters"));
            String aFile;
            if (DataToString(theParams.mDataElements[1], &aFile) == 0)
                return Error(String("Invalid Paramater Type"));
            if (mAnim->mModPamFile.empty())
                mAnim->Load_SetModPamFile(aFile);
            return true;
        }
        if (aLower == "remap")
        {
            if (mStage != 1)
                return true;
            if (aCount != 3)
                return Error(String("Invalid Number of Parameters"));
            String aFrom;
            String aTo;
            if (DataToString(theParams.mDataElements[1], &aFrom) == 0
                || DataToString(theParams.mDataElements[2], &aTo) == 0)
                return Error(String("Invalid Paramater Type"));
            mAnim->Load_AddRemap(aFrom, aTo);
            return true;
        }
        if (aLower == "colorize" || aLower == "hueshift")
        {
            if (mStage != 2)
                return true;
            bool aIsHue = aLower == "hueshift";
            if (aIsHue)
            {
                if (aCount != 3)
                    return Error(String("Invalid Number of Parameters"));
            }
            else if (aCount != 5 && aCount != 6)
            {
                return Error(String("Invalid Number of Parameters"));
            }
            String aImageName;
            if (DataToString(theParams.mDataElements[1], &aImageName) == 0)
                return Error(String("Invalid Paramater Type"));
            int aRed = 255;
            int aGreen = 255;
            int aBlue = 255;
            int aAlpha = 255;
            int aHueValue = 0;
            if (aIsHue)
            {
                if (DataToInt(theParams.mDataElements[2], &aHueValue) == 0)
                    return Error(String("Invalid Paramater Type"));
            }
            else
            {
                if (DataToInt(theParams.mDataElements[2], &aRed) == 0
                    || DataToInt(theParams.mDataElements[3], &aGreen) == 0
                    || DataToInt(theParams.mDataElements[4], &aBlue) == 0)
                    return Error(String("Invalid Paramater Type"));
                if (aCount == 6 && DataToInt(theParams.mDataElements[5], &aAlpha) == 0)
                    return Error(String("Invalid Paramater Type"));
            }
            bool aFound = false;
            for (size_t aIndex = 0; aIndex < mAnim->mImages.size(); ++aIndex)
            {
                PAImage& aImage = mAnim->mImages[aIndex];
                if (strcasecmp(aImage.mName.c_str(), aImageName.c_str()) != 0)
                    continue;
                aFound = true;
                for (size_t aRef = 0; aRef < aImage.mRefs.size(); ++aRef)
                {
                    Image* aLoaded = ImageOf(aImage.mRefs[aRef]);
                    if (aLoaded == nullptr || gSexyAppBase == nullptr)
                        continue;
                    if (aIsHue)
                        gSexyAppBase->RotateImageHue(aLoaded->AsMemoryImage(), aHueValue);
                    else
                        gSexyAppBase->ColorizeImage(aLoaded, Color(aRed, aGreen, aBlue, aAlpha));
                }
            }
            if (!aFound)
                return Error(String("Unable to locate specified element"));
            return true;
        }
        if (aLower == "setparam")
        {
            if (mStage != 2)
                return true;
            if (aCount != 3)
                return Error(String("Invalid Number of Parameters"));
            String aName;
            String aValue;
            if (DataToString(theParams.mDataElements[1], &aName) == 0
                || DataToString(theParams.mDataElements[2], &aValue) == 0)
                return Error(String("Invalid Paramater Type"));
            int aIndex = 0;
            int aBracket = FindChar(aName, '[', 0);
            int aClose = FindChar(aName, ']', 0);
            if (aBracket >= 0 && aClose > aBracket)
            {
                String aIndexText = aName.substr((unsigned int)(aBracket + 1), (unsigned int)(aClose - aBracket - 1));
                aIndex = atoi(aIndexText.c_str());
                aName = aName.substr(0, (unsigned int)aBracket);
            }
            if (mAnim->mSpriteDef != nullptr)
            {
                SetParamHelper(mAnim->mSpriteDef->mMainDef, aName, &aIndex, aValue);
                for (size_t aDef = 0; aDef < mAnim->mSpriteDef->mNestedDefs.size(); ++aDef)
                    SetParamHelper(&mAnim->mSpriteDef->mNestedDefs[aDef], aName, &aIndex, aValue);
            }
            return true;
        }
        return Error(String("Unknown Command"));
    }
};

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

void PopAnim::Load_PostImageLoadHook(PAImage*)
{
}

int PopAnim::Load_SetModPamFile(String const& theFile)
{
    mModPamFile = theFile;
    return 1;
}

int PopAnim::Fail(String const& theMessage)
{
    mErrorString = theMessage;
    return 0;
}

void PopAnim::Load_AddRemap(String const& theFrom, String const& theTo)
{
    mRemaps.push_back(std::make_pair(theFrom, theTo));
}

int PopAnim::LoadFile(String const& theFile, bool theMirrored)
{
    Load_Init();
    mDrawMirrored = theMirrored;
    String aExt = ExtensionOf(theFile);
    if (aExt == ".pam")
        return Load_LoadPam(theFile);
    if (aExt == ".txt")
    {
        if (Load_LoadMod(theFile) == 0)
        {
            if (mErrorString.empty())
                mErrorString = "Mod file loading error";
            return 0;
        }
        return 1;
    }
    if (!aExt.empty())
        return 0;
    String aPam = theFile;
    aPam += ".pam";
    if (Load_LoadPam(aPam) != 0)
        return 1;
    String aTxt = theFile;
    aTxt += ".txt";
    return Load_LoadMod(aTxt);
}

int PopAnim::Load_Init()
{
    return Clear();
}

SharedImageRef PopAnim::Load_GetImageHook(String const& theDir, String const& theName, String const& theFile)
{
    if (theFile.empty())
    {
        Fail(String("No image file name specified"));
        return SharedImageRef();
    }
    if (mSearchPaths.empty())
    {
        String aMessage = String("Unable to load image: ") + theFile + " (" + theName + ")";
        Fail(aMessage);
        return SharedImageRef();
    }
    for (size_t aIndex = 0; aIndex < mSearchPaths.size(); ++aIndex)
    {
        String aPath;
        GetPathFrom(&aPath, &mSearchPaths[aIndex], const_cast<String*>(&theDir));
        if (!aPath.empty())
        {
            char aLast = aPath.c_str()[aPath.size() - 1];
            if (aLast != '\\' && aLast != '/')
                aPath += "\\";
        }
        aPath += theFile;
        String aVariant = mDrawMirrored ? String("MIRRORED") : String();
        mImageIsNew = false;
        SharedImageRef aImage = gSexyAppBase->GetSharedImage(aPath, aVariant, &mImageIsNew, true, false);
        if (aImage.operator Sexy::Image*() != nullptr)
        {
            aImage.operator->()->Palletize();
            return aImage;
        }
    }
    String aMessage = String("Unable to load image: ") + theFile + " (" + theName + ")";
    Fail(aMessage);
    return SharedImageRef();
}

int PopAnim::Load_GetImage(PAImage* theImage, String const& theDir, String const& theName, String const& theFile)
{
    SharedImageRef aRef = Load_GetImageHook(theDir, theName, theFile);
    Image* aLoaded = ImageOf(aRef);
    if (aLoaded == nullptr)
        return 0;
    aLoaded->mNumCols = theImage->mNumCols;
    aLoaded->mNumRows = theImage->mNumRows;
    if (theImage->mRefs.empty() && theImage->mWidth != -1 && theImage->mHeight != -1)
    {
        theImage->mTransX += ((float)theImage->mWidth * mScaleY - (float)aLoaded->mWidth)
            / (float)(aLoaded->mNumCols + 1);
        theImage->mTransY += ((float)theImage->mHeight * mScaleY - (float)aLoaded->mHeight)
            / (float)(aLoaded->mNumRows + 1);
    }
    theImage->mRefs.push_back(aRef);
    return 1;
}

int PopAnim::Load_LoadMod(String const& theFile)
{
    PopAnimModParser aParser(this);
    aParser.mError = String("PopAnim Mod File Error in ");
    aParser.mError += "\r\n";
    aParser.mStage = 1;
    if (!aParser.LoadDescriptor(theFile))
        return 0;
    if (mModPamFile.empty())
        return Fail(String("No Pam file specified"));
    String aDir;
    GetFileDir(&aDir, &theFile, 0);
    String aPamPath;
    GetPathFrom(&aPamPath, &mModPamFile, &aDir);
    if (Load_LoadPam(aPamPath) == 0)
    {
        String aFailed = String("Failed to load Pam: ");
        aFailed += "\r\n\r\n";
        aFailed += mErrorString;
        return aParser.Error(aFailed) ? 1 : 0;
    }
    aParser.mStage = 2;
    return aParser.LoadDescriptor(theFile) ? 1 : 0;
}

const String& PopAnim::Remap(String const& theName)
{
    static String sRemapped;
    if (mRemaps.empty())
        return theName;
    for (std::list<std::pair<String, String> >::iterator aIt = mRemaps.begin(); aIt != mRemaps.end(); ++aIt)
    {
        String const& aKey = aIt->first;
        String const& aValue = aIt->second;
        unsigned int aKeyLen = aKey.size();
        if (aKeyLen == 0)
            continue;
        const char* aKeyChars = aKey.c_str();
        const char* aInput = theName.c_str();
        unsigned int aInputLen = theName.size();
        if (aKeyChars[0] == '*')
        {
            if (aKeyLen == 1)
            {
                sRemapped = *SpliceRemapPattern(&theName, 0, aInputLen, &aValue);
                return sRemapped;
            }
            if (aKeyChars[aKeyLen - 1] != '*')
            {
                if (aKeyLen - 1 <= aInputLen
                    && strcasecmp(aKeyChars + 1, aInput + (aInputLen + 1 - aKeyLen)) == 0)
                {
                    unsigned int aTail = aInputLen + 1 - aKeyLen;
                    sRemapped = *SpliceRemapPattern(&theName, aTail, aInputLen, &aValue);
                    return sRemapped;
                }
                continue;
            }
            int aNeedle = (int)aKeyLen - 2;
            int aLast = (int)aInputLen - aNeedle;
            if (aLast < 0)
                continue;
            int aFound = 0;
            if (aNeedle > 0)
            {
                aFound = -1;
                for (int aPos = 0; aPos <= aLast; ++aPos)
                {
                    if (strncasecmp(aInput + aPos, aKeyChars + 1, (size_t)aNeedle) == 0)
                    {
                        aFound = aPos;
                        break;
                    }
                }
                if (aFound < 0)
                    continue;
            }
            sRemapped = *SpliceRemapPattern(&theName, (unsigned int)aFound,
                                            (unsigned int)(aNeedle + aFound), &aValue);
            return sRemapped;
        }
        if (aKeyChars[aKeyLen - 1] == '*')
        {
            unsigned int aPrefix = aKeyLen - 1;
            if (aPrefix <= aInputLen && strncasecmp(aKeyChars, aInput, aPrefix) == 0)
            {
                sRemapped = *SpliceRemapPattern(&theName, 0, aPrefix, &aValue);
                return sRemapped;
            }
            continue;
        }
        if (strcasecmp(aKeyChars, aInput) != 0)
            continue;
        if (aValue.empty() || (aValue.c_str()[aValue.size() - 1] != '*' && aValue.c_str()[0] != '*'))
        {
            sRemapped = aValue;
            return sRemapped;
        }
        if (aValue.c_str()[0] == '*')
            sRemapped = theName + aValue.substr(1);
        else
            sRemapped = aValue.substr(0, aValue.size() - 1) + theName;
        return sRemapped;
    }
    return theName;
}

int PopAnim::LoadSpriteDef(Buffer* theBuffer, PASpriteDef* theDef)
{
    if (mSpriteDef == nullptr || theDef == nullptr)
        return 0;
    if (mPamVersion > 3)
    {
        String aName = theBuffer->ReadString();
        theDef->mName = mSpriteDef->Intern(aName);
        theDef->mRate = (float)theBuffer->ReadLong() / 65536.0f;
        mPamBuffer.WriteString(String(theDef->mName));
    }
    else
    {
        theDef->mName = nullptr;
        theDef->mRate = (float)mMotion;
    }

    int aFrameCount = theBuffer->ReadShort();
    int aWork;
    if (mPamVersion > 4)
    {
        theDef->mFirstFrame = theBuffer->ReadShort();
        aWork = theBuffer->ReadShort();
    }
    else
    {
        theDef->mFirstFrame = 0;
        aWork = aFrameCount - 1;
    }
    int aLast = aFrameCount - 1;
    int aSum = theDef->mFirstFrame + aWork;
    theDef->mLastFrame = aSum <= aLast ? aSum - theDef->mFirstFrame : aLast - theDef->mFirstFrame;
    mPamBuffer.WriteShort((short)aFrameCount);

    if (aFrameCount < 0)
        return 1;
    theDef->mFrames.resize(aFrameCount == 0 ? 1u : (size_t)aFrameCount);

    std::map<int, PAObjectPos*> aLive;
    for (int aFrameIndex = 0; aFrameIndex < aFrameCount; ++aFrameIndex)
    {
        PAFrame& aFrame = theDef->mFrames[(size_t)aFrameIndex];
        int aFlags = theBuffer->ReadByte();

        if ((aFlags & 1) != 0)
        {
            int aRemoveCount = ReadCount(theBuffer);
            for (int aRemoved = 0; aRemoved < aRemoveCount; ++aRemoved)
            {
                int aKey = theBuffer->ReadShort();
                if (aKey > 2046)
                    aKey = theBuffer->ReadLong();
                aLive.erase(aKey);
            }
        }

        if ((aFlags & 2) != 0)
        {
            int aDefCount = ReadCount(theBuffer);
            for (int aDefIndex = 0; aDefIndex < aDefCount; ++aDefIndex)
            {
                PAObjectPos* aPos = mSpriteDef->AllocTransform();
                PAObjectDef* aData = mSpriteDef->AllocData();
                aPos->mDef = aData;
                unsigned int aPacked = (unsigned short)theBuffer->ReadShort();
                aData->mIndex = (int)(aPacked & 0x7FF);
                if ((aPacked & 0x7FF) == 0x7FF)
                    aData->mIndex = theBuffer->ReadLong();
                aData->mAdditive = (aPacked & 0x4000) != 0;
                aData->mIsSprite = (aPacked >> 15) != 0;
                aData->mImageIndex = theBuffer->ReadByte();
                aPos->mPackedColor = -1;
                aData->mRateScale = 1.0f;
                aData->mAdvanceCount = (aPacked & 0x2000) != 0 ? theBuffer->ReadShort() : 0;
                if ((aPacked & 0x1000) != 0)
                    aData->mName = mSpriteDef->Intern(theBuffer->ReadString());
                if ((aPacked & 0x800) != 0)
                    aData->mRateScale = (float)theBuffer->ReadLong() * kFixedScale;

                if (aData->mIndex >= (int)theDef->mObjects.size())
                    theDef->mObjects.resize((size_t)aData->mIndex + 1);
                PAObjectDef& aRecord = theDef->mObjects[(size_t)aData->mIndex];
                aRecord.mName = aData->mName;
                aRecord.mIndex = aData->mIndex;
                aRecord.mIsSprite = aData->mIsSprite;
                if (aData->mIsSprite && aData->mImageIndex < (int)mSpriteDef->mNestedDefs.size())
                {
                    aRecord.mNestedDef = &mSpriteDef->mNestedDefs[(size_t)aData->mImageIndex];
                    aData->mNestedDef = aRecord.mNestedDef;
                }
                aLive[aData->mIndex] = aPos;
            }
        }

        if ((aFlags & 4) != 0)
        {
            int aXformCount = ReadCount(theBuffer);
            for (int aXform = 0; aXform < aXformCount; ++aXform)
            {
                unsigned int aPacked = (unsigned short)theBuffer->ReadShort();
                int aKey = (int)(aPacked & 0x3FF);
                if (aKey == 1023)
                    aKey = theBuffer->ReadLong();
                std::map<int, PAObjectPos*>::iterator aFound = aLive.find(aKey);
                if (aFound == aLive.end() || aFound->second == nullptr)
                    continue;
                PAObjectPos* aPos = mSpriteDef->AllocTransform();
                *aPos = *aFound->second;
                aFound->second = aPos;
                aPos->mMatrix.LoadIdentity();
                if ((aPacked & 0x1000) != 0)
                {
                    aPos->mMatrix.m00 = (float)theBuffer->ReadLong() * kFixedScale;
                    aPos->mMatrix.m01 = (float)theBuffer->ReadLong() * kFixedScale;
                    aPos->mMatrix.m10 = (float)theBuffer->ReadLong() * kFixedScale;
                    aPos->mMatrix.m11 = (float)theBuffer->ReadLong() * kFixedScale;
                }
                else if ((aPacked & 0x4000) != 0)
                {
                    float aAngle = (float)theBuffer->ReadShort() * kAngleScale;
                    float aSin = sinf(aAngle);
                    float aCos = cosf(aAngle);
                    if (mPamVersion == 2)
                        aSin = -aSin;
                    aPos->mMatrix.m00 = aCos;
                    aPos->mMatrix.m11 = aCos;
                    aPos->mMatrix.m10 = aSin;
                    aPos->mMatrix.m01 = -aSin;
                }
                SexyMatrix3 aTranslate;
                aTranslate.LoadIdentity();
                if ((aPacked & 0x800) != 0)
                {
                    aTranslate.m02 = (float)theBuffer->ReadLong() * kShortScale;
                    aTranslate.m12 = (float)theBuffer->ReadLong() * kShortScale;
                }
                else
                {
                    aTranslate.m02 = (float)theBuffer->ReadShort() * kShortScale;
                    aTranslate.m12 = (float)theBuffer->ReadShort() * kShortScale;
                }
                aPos->mMatrix = aTranslate * aPos->mMatrix;
                if (aPos->mDef != nullptr)
                {
                    aPos->mDef->mUseSrcRect = (aPacked >> 15) != 0;
                    if (aPos->mDef->mUseSrcRect)
                    {
                        aPos->mDef->mSrcRect.mX = (short)(theBuffer->ReadShort() / 20);
                        aPos->mDef->mSrcRect.mY = (short)(theBuffer->ReadShort() / 20);
                        aPos->mDef->mSrcRect.mWidth = (short)(theBuffer->ReadShort() / 20);
                        aPos->mDef->mSrcRect.mHeight = (short)(theBuffer->ReadShort() / 20);
                    }
                }
                if ((aPacked & 0x2000) != 0)
                {
                    int aRaw = theBuffer->ReadLong();
                    std::memcpy(&aPos->mMatrix.m21, &aRaw, sizeof(aRaw));
                }
                if ((aPacked & 0x400) != 0)
                {
                    int aRaw = theBuffer->ReadShort();
                    std::memcpy(&aPos->mMatrix.m20, &aRaw, sizeof(aRaw));
                }
            }
        }

        if ((aFlags & 8) != 0)
        {
            String aLabel = theBuffer->ReadString();
            String aRemapped = Remap(aLabel);
            String aUpper;
            StringToUpper(&aUpper, aRemapped.c_str());
            theDef->mLabels[aUpper] = aFrameIndex;
        }
        if ((aFlags & 0x10) != 0)
            aFrame.mHold = true;
        if ((aFlags & 0x20) != 0)
        {
            int aCmdCount = theBuffer->ReadByte();
            aFrame.mCommands.resize((size_t)aCmdCount);
            for (int aCmd = 0; aCmd < aCmdCount; ++aCmd)
            {
                aFrame.mCommands[(size_t)aCmd].mName = Remap(theBuffer->ReadString());
                aFrame.mCommands[(size_t)aCmd].mArgs = Remap(theBuffer->ReadString());
            }
        }

        aFrame.mObjects.clear();
        for (std::map<int, PAObjectPos*>::iterator aIt = aLive.begin(); aIt != aLive.end(); ++aIt)
        {
            PAObjectPos* aPos = aIt->second;
            if (aPos != nullptr && aPos->mDef != nullptr && aPos->mDef->mIndex != 0)
            {
                PAObjectDef* aCopy = mSpriteDef->AllocData();
                *aCopy = *aPos->mDef;
                aCopy->mAdvanceCount = 0;
                aPos->mDef = aCopy;
            }
            aFrame.mObjects.push_back(aPos);
        }
    }

    if (theDef->mFrames.size() > 1)
    {
        int aSpan = (int)theDef->mFrames.size();
        for (int aFrameIndex = 0; aFrameIndex < aSpan; ++aFrameIndex)
        {
            PAFrame& aFrame = theDef->mFrames[(size_t)aFrameIndex];
            if ((int)aFrame.mObjects.size() <= 1)
                continue;
            int aOffsets[3] = { theDef->mLastFrame - 1, 1, 2 };
            for (int aObj = 0; aObj < (int)aFrame.mObjects.size(); ++aObj)
            {
                PAObjectPos* aPos = aFrame.mObjects[(size_t)aObj];
                if (aPos == nullptr || aPos->mDef == nullptr)
                    continue;
                PAObjectPos* aSamples[3] = { nullptr, nullptr, nullptr };
                for (int aSample = 0; aSample < 3; ++aSample)
                {
                    PAFrame* aOther = aFrame.mHold
                        ? &aFrame
                        : &theDef->mFrames[(size_t)((aOffsets[aSample] + aFrameIndex) % aSpan)];
                    aSamples[aSample] = PoseAt(aOther, aObj, aPos->mDef->mIndex);
                }
                if (aSamples[1] == nullptr || aSamples[0] == nullptr || aSamples[2] == nullptr)
                {
                    aPos->mInterpolate = aSamples[1] != nullptr;
                    continue;
                }
                float aPrevDist = PoseDistance(aSamples[0]->mMatrix, aPos->mMatrix);
                float aNextDist = PoseDistance(aSamples[1]->mMatrix, aSamples[2]->mMatrix);
                SexyVector2 aOrigin(0.0f, 0.0f);
                SexyVector2 aHere = aPos->mMatrix * aOrigin;
                SexyVector2 aNext = aSamples[1]->mMatrix * aOrigin;
                SexyVector2 aAfter = aSamples[2]->mMatrix * aOrigin;
                float aStepX = (aAfter.mX - aNext.mX) - (aNext.mX - aHere.mX);
                float aStepY = (aAfter.mY - aNext.mY) - (aNext.mY - aHere.mY);
                float aBend = sqrtf(aStepX * aStepX + aStepY * aStepY);
                float aLongest = aNextDist >= aPrevDist ? aNextDist : aPrevDist;
                aPos->mInterpolate = ((aPrevDist * 0.25f) + (aNextDist * 0.25f) + (aLongest * 0.5f)) * 4.0f >= aBend;
            }
        }
    }

    for (size_t aIndex = 0; aIndex < theDef->mObjects.size(); ++aIndex)
        mPamBuffer.WriteBoolean(theDef->mObjects[aIndex].mNestedDef != nullptr);
    return 1;
}

int PopAnim::Load_LoadPam(String const& theFile)
{
    mPamFileName = theFile;
    if (mSpriteDef != nullptr)
        return 0;

    mSpriteInst = new PASpriteInst();
    mSpriteDef = new PASpriteTable();

    Buffer aFile;
    String aDir;
    GetFileDir(&aDir, &theFile, 0);
    if (gSexyAppBase->ReadBufferFromFile(theFile, &aFile, true) == 0)
    {
        String aMessage = String("Unable to load file: ") + theFile;
        return Fail(aMessage);
    }
    if (aFile.ReadLong() != kPamMagic)
        return Fail(String("Invalid header"));
    mPamVersion = aFile.ReadLong();
    if (mPamVersion > 5)
        return Fail(String("Invalid version"));

    mMotion = aFile.ReadByte();
    mOriginX = (short)(aFile.ReadShort() / 20);
    mOriginY = (short)(aFile.ReadShort() / 20);
    mAnimWidth = (int)((unsigned short)aFile.ReadShort() / 20u);
    mAnimHeight = (int)((unsigned short)aFile.ReadShort() / 20u);

    int aImageCount = aFile.ReadShort();
    mImages.resize((size_t)aImageCount);
    for (int aIndex = 0; aIndex < aImageCount; ++aIndex)
    {
        PAImage& aImage = mImages[(size_t)aIndex];
        aImage.mDrawMode = 0;
        String aName = Remap(aFile.ReadString());
        String aVariant;
        int aOpen = FindChar(aName, '(', 0);
        int aClose = FindChar(aName, ')', 0);
        if (aOpen >= 0 && aClose > aOpen)
        {
            String aInside = aName.substr((unsigned int)(aOpen + 1), (unsigned int)(aClose - aOpen - 1));
            aVariant = Lowered(aInside);
            String aJoined = aName.substr(0, (unsigned int)aOpen) + aName.substr((unsigned int)(aClose + 1));
            aName = Trimmed(aJoined);
        }
        else
        {
            int aDollar = FindChar(aName, '$', 0);
            if (aDollar >= 0)
            {
                aVariant = Lowered(aName.substr(0, (unsigned int)aDollar));
                aName = Trimmed(aName.substr((unsigned int)(aDollar + 1)));
            }
        }
        aImage.mNumCols = 1;
        aImage.mNumRows = 1;
        int aBracket = FindChar(aName, '[', 0);
        int aBracketEnd = FindChar(aName, ']', 0);
        if (aBracket >= 0 && aBracketEnd > aBracket)
        {
            String aInside = Lowered(aName.substr((unsigned int)(aBracket + 1),
                                                  (unsigned int)(aBracketEnd - aBracket - 1)));
            String aJoined = aName.substr(0, (unsigned int)aBracket) + aName.substr((unsigned int)(aBracketEnd + 1));
            aName = Trimmed(aJoined);
            int aComma = FindChar(aInside, ',', 0);
            if (aComma >= 0)
            {
                aImage.mNumCols = atoi(aInside.substr(0, (unsigned int)aComma).c_str());
                aImage.mNumRows = atoi(aInside.substr((unsigned int)(aComma + 1)).c_str());
            }
        }
        if (FindText(aVariant, "add") >= 0)
            aImage.mDrawMode = 1;
        if (mPamVersion > 3)
        {
            aImage.mWidth = aFile.ReadShort();
            aImage.mHeight = aFile.ReadShort();
        }
        else
        {
            aImage.mWidth = -1;
            aImage.mHeight = -1;
        }
        if (mPamVersion == 1)
        {
            float aAngle = (float)aFile.ReadShort() * kAngleScale;
            float aSin = sinf(aAngle);
            float aCos = cosf(aAngle);
            aImage.mTransform.m00 = aCos;
            aImage.mTransform.m11 = aCos;
            aImage.mTransform.m01 = aSin;
            aImage.mTransform.m10 = -aSin;
        }
        else
        {
            aImage.mTransform.m00 = (float)aFile.ReadLong() * kImageMatrixScale;
            aImage.mTransform.m10 = (float)aFile.ReadLong() * kImageMatrixScale;
            aImage.mTransform.m01 = (float)aFile.ReadLong() * kImageMatrixScale;
            aImage.mTransform.m11 = (float)aFile.ReadLong() * kImageMatrixScale;
        }
        aImage.mTransX = (float)aFile.ReadShort() * kShortScale;
        aImage.mTransY = (float)aFile.ReadShort() * kShortScale;
        aImage.mName = aName;

        if (!aImage.mName.empty())
        {
            unsigned int aPos = 0;
            while (aPos < aName.size())
            {
                int aComma = FindChar(aName, ',', aPos);
                String aToken = aComma < 0
                    ? aName.substr(aPos)
                    : aName.substr(aPos, (unsigned int)(aComma - (int)aPos));
                Load_GetImage(&aImage, aDir, aToken, aToken);
                if (aComma < 0)
                    break;
                aPos = (unsigned int)(aComma + 1);
            }
        }
        else
        {
            bool aIsNew = false;
            SharedImageRef aRef = gSexyAppBase->GetSharedImage(String("!whitepixel"), String(), &aIsNew, true, false);
            if (aIsNew)
            {
                MemoryImage* aPixel = aRef.operator Sexy::MemoryImage*();
                if (aPixel != nullptr)
                {
                    aPixel->Create(1, 1);
                    aPixel->SetImageMode(false, false);
                    unsigned int* aBits = aPixel->GetBits();
                    if (aBits != nullptr)
                        aBits[0] = (unsigned int)-1;
                    aPixel->BitsChanged();
                }
            }
            aImage.mRefs.push_back(aRef);
        }
        if (!mErrorString.empty())
            return 0;
        if (mDrawMirrored && mImageIsNew)
        {
            for (size_t aRef = 0; aRef < aImage.mRefs.size(); ++aRef)
                gSexyAppBase->MirrorImage(ImageOf(aImage.mRefs[aRef]));
        }
        Load_PostImageLoadHook(&aImage);
    }

    int aBits = aFile.mReadBitPos;
    if (aBits < 0)
        aBits += 7;
    mPamCursor = aBits >> 3;

    int aDefCount = aFile.ReadShort();
    mSpriteDef->mNestedDefs.resize((size_t)aDefCount);
    for (int aIndex = 0; aIndex < aDefCount; ++aIndex)
    {
        if (LoadSpriteDef(&aFile, &mSpriteDef->mNestedDefs[(size_t)aIndex]) == 0)
            return 0;
    }
    if (mPamVersion <= 3 || aFile.ReadBoolean())
    {
        mSpriteDef->mMainDef = new PASpriteDef();
        if (LoadSpriteDef(&aFile, mSpriteDef->mMainDef) == 0)
            return 0;
    }
    mPamLoaded = true;
    mSaveRandomState = false;
    return 1;
}


int PopAnim::ResetAnimHelper(PASpriteInst* theSprite)
{
    if (theSprite == nullptr)
        return 0;
    theSprite->mLastStamp = -1;
    theSprite->mFrameTime = 0.0f;
    theSprite->mFrameAdvanced = true;
    theSprite->mLoopCount = 0;
    theSprite->mHoldFrames = 0;
    for (size_t aIndex = 0; aIndex < theSprite->mParticles.size(); ++aIndex)
    {
        if (theSprite->mParticles[aIndex].mEffect != nullptr)
            theSprite->mParticles[aIndex].mEffect->ResetAnim();
    }
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
    {
        if (theSprite->mObjects[aIndex].mInst != nullptr)
            ResetAnimHelper(theSprite->mObjects[aIndex].mInst);
    }
    mTransformsDirty = true;
    return 0;
}

int PopAnim::GetToFirstFrame()
{
    if (mSpriteInst == nullptr || mSpriteInst->mDef == nullptr)
        return 0;
    if (mSpriteInst->mFrameTime >= (float)mSpriteInst->mDef->mFrames.size())
        return 0;
    bool aWasActive = mActive;
    bool aWasDisabled = mParticlesDisabled;
    while (mSpriteInst->mDef != nullptr
           && mSpriteInst->mFrameTime < (float)mSpriteInst->mDef->mFrames.size())
    {
        mActive = true;
        mParticlesDisabled = false;
        Update();
        if (gSexyAppBase->mVSyncUpdates)
            UpdateF(1.0f);
        mActive = aWasActive;
        mParticlesDisabled = aWasDisabled;
    }
    return 0;
}

int PopAnim::InitSpriteInst(PASpriteInst* theSprite, PASpriteDef* theDef)
{
    if (theSprite == nullptr || theDef == nullptr)
        return 0;
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
        delete theSprite->mObjects[aIndex].mInst;
    theSprite->mObjects.clear();
    theSprite->mDef = theDef;
    theSprite->mLastStamp = -1;
    theSprite->mFrameTime = 0.0f;
    theSprite->mLoopCount = 0;
    theSprite->mHoldFrames = 0;
    theSprite->mFrameAdvanced = true;
    theSprite->mObjects.resize(theDef->mObjects.size());
    for (size_t aIndex = 0; aIndex < theDef->mObjects.size(); ++aIndex)
    {
        PAObjectInst& aObject = theSprite->mObjects[aIndex];
        aObject.mColor = Color::White;
        aObject.mName = theDef->mObjects[aIndex].mName;
        aObject.mBlending = false;
        PASpriteDef* aNested = theDef->mObjects[aIndex].mNestedDef;
        if (aNested != nullptr)
        {
            aObject.mInst = new PASpriteInst();
            aObject.mInst->mParent = theSprite;
            InitSpriteInst(aObject.mInst, aNested);
        }
    }
    if (mSpriteInst == theSprite)
        return GetToFirstFrame();
    return 0;
}

int PopAnim::SetupSpriteInst(String const& theName)
{
    if (mSpriteInst == nullptr)
        return 0;
    if (mSpriteInst->mDef != nullptr && theName.empty())
        return 1;
    if (mSpriteDef == nullptr)
        return 0;
    if (mSpriteDef->mMainDef != nullptr)
    {
        InitSpriteInst(mSpriteInst, mSpriteDef->mMainDef);
        return 1;
    }
    if (mSpriteDef->mNestedDefs.empty())
        return 0;
    String aName = theName.empty() ? String("main") : theName;
    PASpriteDef* aChosen = &mSpriteDef->mNestedDefs[0];
    for (size_t aIndex = 0; aIndex < mSpriteDef->mNestedDefs.size(); ++aIndex)
    {
        const char* aDefName = mSpriteDef->mNestedDefs[aIndex].mName;
        if (aDefName != nullptr && strcasecmp(aDefName, aName.c_str()) == 0)
            aChosen = &mSpriteDef->mNestedDefs[aIndex];
    }
    if (mSpriteInst->mDef != aChosen)
    {
        if (mSpriteInst->mDef != nullptr)
        {
            delete mSpriteInst;
            mSpriteInst = new PASpriteInst();
        }
        InitSpriteInst(mSpriteInst, aChosen);
        mTransformsDirty = true;
    }
    return 1;
}

int PopAnim::GetLabelFrame(String const& theName)
{
    if (mSpriteDef == nullptr || mSpriteDef->mMainDef == nullptr)
        return -1;
    return mSpriteDef->mMainDef->GetLabelFrame(theName);
}

const char** PopAnim::FindSpriteDef(char const* theName)
{
    if (mSpriteDef == nullptr || theName == nullptr)
        return nullptr;
    for (size_t aIndex = 0; aIndex < mSpriteDef->mNestedDefs.size(); ++aIndex)
    {
        if (strcasecmp(mSpriteDef->mNestedDefs[aIndex].mName, theName) == 0)
            return &mSpriteDef->mNestedDefs[aIndex].mName;
    }
    return nullptr;
}

int PopAnim::GetObjectInst(String const& theName)
{
    SetupSpriteInst(String());
    if (mSpriteInst == nullptr)
        return 0;
    return mSpriteInst->GetObjectInst(theName);
}

int PopAnim::ResetAnim()
{
    ResetAnimHelper(mSpriteInst);
    CleanParticles(mSpriteInst, true);
    mActive = false;
    mLoopNotified = false;
    GetToFirstFrame();
    mBlendDuration = 0.0f;
    mBlendTime = 0.0f;
    mDelay = 0.0f;
    return 0;
}

int PopAnim::Clear()
{
    mDrawMirrored = false;
    mColor = Color::White;
    mPamLoaded = false;
    mSaveRandomState = false;
    mActive = false;
    mLoopNotified = false;
    mErrorString = String();
    mRemaps.clear();
    mImages.clear();
    mModPamFile = String();
    mPamBuffer.Clear();
    if (mSpriteDef != nullptr)
    {
        if (mSpriteDef->mRefCount > 0)
            --mSpriteDef->mRefCount;
        else
            delete mSpriteDef;
    }
    delete mSpriteInst;
    mSpriteInst = nullptr;
    mSpriteDef = nullptr;
    mTransformsDirty = true;
    return 0;
}

int PopAnim::SetColor(Color const& theColor)
{
    mColor = theColor;
    MarkDirty();
    return 0;
}

int PopAnim::IncSpriteInstFrame(PASpriteInst* theSprite, PAObjectPos* thePose, float theStep)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theSprite->mDef->mFrames.empty())
        return 0;
    int aOldFrame = (int)theSprite->mFrameTime;
    if (aOldFrame < 0 || aOldFrame >= (int)theSprite->mDef->mFrames.size())
        return 0;
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aOldFrame];
    if (aFrame.mHold)
        return 0;

    float aRateScale = 1.0f;
    if (thePose != nullptr && thePose->mDef != nullptr)
        aRateScale = thePose->mDef->mRateScale;
    if (aRateScale == 0.0f)
        aRateScale = 1.0f;
    float aFrameTime = theSprite->mFrameTime
        + theSprite->mDef->mRate * 0.001f * (float)gSexyAppBase->mFrameTime * theStep / aRateScale;
    theSprite->mFrameTime = aFrameTime;

    int aNewFrame = (int)aFrameTime;
    if (theSprite == mSpriteInst)
    {
        int aEnd = theSprite->mDef->mFirstFrame + theSprite->mDef->mLastFrame;
        if (theSprite->mDef->mPingPong)
        {
            if (aNewFrame >= aEnd)
            {
                theSprite->mFrameAdvanced = true;
                theSprite->mFrameTime = (float)aEnd;
                if (theSprite->mDef->mLastFrame != 0)
                {
                    mActive = false;
                    if (mListener != nullptr)
                        mListener->OnSpriteInstFrameEnd(mId);
                    return 0;
                }
                ++theSprite->mLoopCount;
                aNewFrame = aEnd;
            }
        }
        else if (aEnd < aNewFrame)
        {
            ++theSprite->mLoopCount;
            aFrameTime -= (float)(theSprite->mDef->mLastFrame + 1);
            theSprite->mFrameTime = aFrameTime;
            aNewFrame = (int)aFrameTime;
        }
    }
    else
    {
        int aFrameCount = (int)theSprite->mDef->mFrames.size();
        if (aNewFrame >= aFrameCount)
        {
            if (mSpriteInst != nullptr && mSpriteInst->mObjects.size() > 1
                && theSprite == mSpriteInst->mObjects[1].mInst)
            {
                if (!mLoopNotified && mListener != nullptr)
                    mListener->OnSpriteInstLoop(mId, 0);
                mLoopNotified = true;
                aFrameTime = theSprite->mFrameTime;
                aFrameCount = (int)theSprite->mDef->mFrames.size();
            }
            aFrameTime -= (float)aFrameCount;
            ++theSprite->mLoopCount;
            theSprite->mFrameTime = aFrameTime;
            aNewFrame = (int)aFrameTime;
        }
    }

    bool aAdvanced = aOldFrame != aNewFrame;
    theSprite->mFrameAdvanced = aAdvanced;
    if (aAdvanced && theSprite->mHoldFrames > 0)
    {
        --theSprite->mHoldFrames;
        theSprite->mFrameAdvanced = false;
        theSprite->mFrameTime = (float)aOldFrame;
        return 0;
    }
    for (size_t aIndex = 0; aIndex < aFrame.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrame.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
            continue;
        int aObject = aPos->mDef->mIndex;
        if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
            continue;
        IncSpriteInstFrame(theSprite->mObjects[(size_t)aObject].mInst, nullptr, theStep / aRateScale);
    }
    return 0;
}

int* PopAnim::FrameHit(PASpriteInst* theSprite, PAFrame* theFrame, PAObjectPos* thePose)
{
    if (theSprite == nullptr || theFrame == nullptr)
        return nullptr;
    theSprite->mFrameAdvanced = false;
    for (size_t aIndex = 0; aIndex < theFrame->mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = theFrame->mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
            continue;
        int aObject = aPos->mDef->mIndex;
        if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
            continue;
        PASpriteInst* aNested = theSprite->mObjects[(size_t)aObject].mInst;
        if (aNested == nullptr || theSprite->mDef == nullptr || theSprite->mDef->mRate == 0.0f)
            continue;
        if (aPos->mDef->mAdvanceCount <= 0)
            continue;
        float aStep = 1000.0f / (theSprite->mDef->mRate * (float)gSexyAppBase->mFrameTime);
        for (int aRepeat = 0; aRepeat < aPos->mDef->mAdvanceCount; ++aRepeat)
            IncSpriteInstFrame(aNested, aPos, aStep);
    }
    if (theFrame->mCommands.empty())
        return nullptr;

    for (size_t aIndex = 0; aIndex < theFrame->mCommands.size(); ++aIndex)
    {
        PACommand const& aCommand = theFrame->mCommands[aIndex];
        if (mListener != nullptr && mListener->OnCommand(mId, theSprite, aCommand.mName, aCommand.mArgs))
            continue;
        String aLower = Lowered(aCommand.mName);
        if (aLower == "delay")
        {
            int aComma = FindChar(aCommand.mArgs, ',', 0);
            if (aComma < 0)
            {
                theSprite->mHoldFrames = atoi(Trimmed(aCommand.mArgs).c_str());
            }
            else
            {
                int aLo = atoi(Trimmed(aCommand.mArgs.substr(0, (unsigned int)aComma)).c_str());
                int aHi = atoi(Trimmed(aCommand.mArgs.substr((unsigned int)(aComma + 1))).c_str());
                if (aLo >= aHi)
                    aHi = aLo + 1;
                theSprite->mHoldFrames = Rand() % (aHi - aLo) + aLo;
            }
            continue;
        }
        if (aLower == "addparticleeffect")
        {
            String aArgs = aCommand.mArgs;
            PAParticleEffect aEntry;
            aEntry.mUpdateStamp = mUpdateCnt;
            String aEffectName;
            bool aOnce = false;
            bool aFirst = true;
            while (!aArgs.empty())
            {
                CommandToken aToken = NextToken(aArgs);
                if (aFirst)
                {
                    aEntry.mName = aToken.mText;
                    aEffectName = aToken.mText;
                    aFirst = false;
                }
                else
                {
                    String aClean = StripSpaces(aToken.mText);
                    if (strncasecmp(aClean.c_str(), "x=", 2) == 0)
                    {
                        String aNumber = aClean.substr(2);
                        StringToDouble(&aNumber, &aEntry.mOffsetX);
                    }
                    else if (strncasecmp(aClean.c_str(), "y=", 2) == 0)
                    {
                        String aNumber = aClean.substr(2);
                        StringToDouble(&aNumber, &aEntry.mOffsetY);
                    }
                    else if (strcasecmp(aClean.c_str(), "attachemitter") == 0)
                        aEntry.mFollowSprite = true;
                    else if (strcasecmp(aClean.c_str(), "once") == 0)
                        aOnce = true;
                    else if (strcasecmp(aClean.c_str(), "behind") == 0)
                        aEntry.mDrawAdditive = true;
                    else if (strcasecmp(aClean.c_str(), "transform") == 0)
                        aEntry.mActive = true;
                }
                if (!aToken.mMore)
                    break;
            }
            if (aOnce)
            {
                bool aLive = false;
                for (size_t aParticle = 0; aParticle < theSprite->mParticles.size(); ++aParticle)
                {
                    if (theSprite->mParticles[aParticle].mName == aEffectName)
                    {
                        aLive = true;
                        break;
                    }
                }
                if (aLive)
                    return nullptr;
            }

            String aParentPrefix = String("..\\");
            aParentPrefix += "\\";
            aParentPrefix += aEffectName;
            String aPamDir;
            GetFileDir(&aPamDir, &mPamFileName, 0);
            String aParentPath;
            GetPathFrom(&aParentPath, &aParentPrefix, &aPamDir);
            String aNamePrefix = aEffectName;
            aNamePrefix += "\\";
            aNamePrefix += aEffectName;
            String aNamePath;
            GetPathFrom(&aNamePath, &aNamePrefix, &aPamDir);

            if (mListener != nullptr)
                aEntry.mEffect = mListener->GetParticleEffect(aEffectName);
            if (aEntry.mEffect == nullptr && gSexyAppBase != nullptr && gSexyAppBase->mResourceManager != nullptr)
            {
                ResourceRef aLooked = gSexyAppBase->mResourceManager->GetResourceRefFromPath(aNamePath + ".ppf");
                if (aLooked.operator Sexy::PIEffect*() == nullptr)
                    aLooked = gSexyAppBase->mResourceManager->GetResourceRefFromPath(aParentPath + ".ppf");
                if (aLooked.operator Sexy::PIEffect*() == nullptr)
                {
                    String aUpper;
                    StringToUpper(&aUpper, aEffectName.c_str());
                    aLooked = gSexyAppBase->mResourceManager->GetResourceRef(4, String("PIEFFECT_") + aUpper);
                }
                PIEffect* aFound = aLooked.operator Sexy::PIEffect*();
                if (aFound != nullptr)
                {
                    aEntry.mEffect = aFound->Duplicate();
                    aEntry.mBaseRes = aLooked.mBaseRes;
                    if (aEntry.mBaseRes != nullptr)
                        ++aEntry.mBaseRes->mRefCount;
                }
            }
            if (aEntry.mEffect == nullptr)
            {
                aEntry.mEffect = new PIEffect();
                bool aFailed = aEntry.mEffect->LoadEffect(aNamePath + ".ppf") == 0;
                if (aFailed)
                    aFailed = aEntry.mEffect->LoadEffect(aParentPath + ".ppf") == 0;
                if (aFailed)
                    aFailed = aEntry.mEffect->LoadEffect(aNamePath + ".ip3") == 0;
                if (aFailed)
                {
                    for (size_t aPath = 0; aPath < mSearchPaths.size(); ++aPath)
                    {
                        String aJoined = mSearchPaths[aPath] + aEffectName + ".ip3";
                        if (aEntry.mEffect->LoadEffect(aJoined) != 0)
                        {
                            aFailed = false;
                            break;
                        }
                    }
                }
                if (aFailed)
                {
                    delete aEntry.mEffect;
                    aEntry.mEffect = nullptr;
                }
            }
            if (aEntry.mEffect == nullptr)
                continue;
            if (!mSaveRandomState)
            {
                mSaveRandomState = true;
                mRandom.SRand((unsigned int)Rand());
            }
            if (!aEntry.mEffect->mRandSeeds.empty())
            {
                unsigned int aPick = mRandom.Next() % (unsigned int)aEntry.mEffect->mRandSeeds.size();
                aEntry.mEffect->mRand.SRand((unsigned int)aEntry.mEffect->mRandSeeds[aPick]);
            }
            else
            {
                aEntry.mEffect->mRand.SRand(mRandom.Next());
            }
            aEntry.mEffect->mNeedsSeed = false;
            if (thePose != nullptr && thePose->mDef != nullptr
                && theSprite->mDef != nullptr && theSprite->mDef->mRate != 0.0f)
            {
                float aScale = thePose->mDef->mRateScale == 0.0f ? 1.0f : thePose->mDef->mRateScale;
                int aWarm = (int)((float)thePose->mDef->mAdvanceCount
                                  / (theSprite->mDef->mRate * aScale) * 100.0f + 0.5f);
                for (int aWarmStep = 0; aWarmStep < aWarm; ++aWarmStep)
                    aEntry.mEffect->Update();
            }
            theSprite->mParticles.push_back(aEntry);
            continue;
        }
        if (aLower != "playsample")
            continue;
        String aSampleArgs = aCommand.mArgs;
        int aPan = 0;
        double aVolume = 1.0;
        double aSteps = 0.0;
        String aSampleName;
        bool aFirst = true;
        while (!aSampleArgs.empty())
        {
            CommandToken aToken = NextToken(aSampleArgs);
            if (aFirst)
            {
                aSampleName = aToken.mText;
                aFirst = false;
            }
            else
            {
                String aClean = StripSpaces(aToken.mText);
                if (strncasecmp(aClean.c_str(), "volume=", 7) == 0)
                {
                    String aNumber = aClean.substr(7);
                    StringToDouble(&aNumber, &aVolume);
                }
                else if (strncasecmp(aClean.c_str(), "pan=", 4) == 0)
                {
                    String aNumber = aClean.substr(4);
                    StringToInt(aNumber, &aPan);
                }
                else if (strncasecmp(aClean.c_str(), "steps=", 6) == 0)
                {
                    String aNumber = aClean.substr(6);
                    StringToDouble(&aNumber, &aSteps);
                }
            }
            if (!aToken.mMore)
                break;
        }
        if (mListener != nullptr)
            mListener->PlaySample(aSampleName, aPan, aVolume, aSteps);
    }
    return nullptr;
}

int* PopAnim::DoFramesHit(PASpriteInst* theSprite, PAObjectPos* thePose)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theSprite->mDef->mFrames.empty())
        return nullptr;
    int aFrame = (int)theSprite->mFrameTime;
    if (aFrame < 0 || aFrame >= (int)theSprite->mDef->mFrames.size())
        return nullptr;
    PAFrame& aFrameRef = theSprite->mDef->mFrames[(size_t)aFrame];
    int* aResult = FrameHit(theSprite, &aFrameRef, thePose);
    for (size_t aIndex = 0; aIndex < aFrameRef.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrameRef.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
            continue;
        int aObject = aPos->mDef->mIndex;
        if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
            continue;
        PASpriteInst* aNested = theSprite->mObjects[(size_t)aObject].mInst;
        if (aNested != nullptr)
            aResult = DoFramesHit(aNested, nullptr);
    }
    return aResult;
}

void PopAnim::PrepSpriteInstFrame(PASpriteInst* theSprite, PAObjectPos* thePose)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theSprite->mDef->mFrames.empty())
        return;
    int aFrameIndex = (int)theSprite->mFrameTime;
    if (aFrameIndex < 0 || aFrameIndex >= (int)theSprite->mDef->mFrames.size())
        return;
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aFrameIndex];
    if (theSprite->mFrameAdvanced)
        FrameHit(theSprite, &aFrame, thePose);
    if (aFrame.mHold)
    {
        if (mSpriteInst == theSprite)
        {
            mActive = false;
            if (mListener != nullptr)
                mListener->OnSpriteInstFrameEnd(mId);
        }
        return;
    }
    int aStamp = aFrameIndex + theSprite->mLoopCount * (int)theSprite->mDef->mFrames.size();
    for (size_t aIndex = 0; aIndex < aFrame.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrame.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
            continue;
        int aObject = aPos->mDef->mIndex;
        if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
            continue;
        PASpriteInst* aNested = theSprite->mObjects[(size_t)aObject].mInst;
        if (aNested == nullptr)
            continue;
        if (aNested->mLastStamp != aStamp - 1 && aNested->mLastStamp != aStamp)
        {
            aNested->mFrameTime = 0.0f;
            aNested->mLoopCount = 0;
            aNested->mHoldFrames = 0;
            aNested->mFrameAdvanced = true;
        }
        PrepSpriteInstFrame(aNested, aPos);
        aNested->mLastStamp = aStamp;
    }
}

void PopAnim::AnimUpdate(float theStep)
{
    if (!mActive)
        return;
    if (mBlendDuration > 0.0f)
    {
        mBlendTime += theStep;
        if (mBlendDuration <= mBlendTime)
            mBlendDuration = 0.0f;
    }
    mTransformsDirty = true;
    if (mDelay <= 0.0f)
    {
        IncSpriteInstFrame(mSpriteInst, nullptr, theStep);
        PrepSpriteInstFrame(mSpriteInst, nullptr);
        MarkDirty();
        return;
    }
    mDelay -= theStep;
    if (mDelay <= 0.0f)
    {
        mDelay = 0.0f;
        DoFramesHit(mSpriteInst, nullptr);
    }
}

void PopAnim::UpdateF(float theFrac)
{
    if (!mParticlesDisabled)
        AnimUpdate(theFrac);
}

int PopAnim::Play(int theFrame, bool theReset)
{
    if (SetupSpriteInst(String()) == 0 || mSpriteInst == nullptr || mSpriteInst->mDef == nullptr)
        return 0;
    if (theFrame >= (int)mSpriteInst->mDef->mFrames.size())
    {
        mActive = false;
        return 0;
    }
    if (mSpriteInst->mFrameTime != (float)theFrame)
    {
        if (theReset)
        {
            ResetAnim();
            mParticlesDisabled = false;
            mActive = true;
            mSpriteInst->mHoldFrames = 0;
            mSpriteInst->mFrameTime = (float)theFrame;
            mSpriteInst->mLoopCount = 0;
            CleanParticles(mSpriteInst, true);
            if (mDelay == 0.0f)
                DoFramesHit(mSpriteInst, nullptr);
            MarkDirty();
            return 1;
        }
        float aPending = mDelay;
        mParticlesDisabled = false;
        mActive = true;
        mSpriteInst->mHoldFrames = 0;
        mSpriteInst->mFrameTime = (float)theFrame;
        mSpriteInst->mLoopCount = 0;
        if (aPending == 0.0f)
            DoFramesHit(mSpriteInst, nullptr);
        MarkDirty();
        return 1;
    }
    mParticlesDisabled = false;
    mActive = true;
    mSpriteInst->mHoldFrames = 0;
    mSpriteInst->mLoopCount = 0;
    CleanParticles(mSpriteInst, true);
    if (mDelay == 0.0f)
        DoFramesHit(mSpriteInst, nullptr);
    MarkDirty();
    return 1;
}

int PopAnim::Play(String const& theName, bool theReset)
{
    mActive = false;
    if (mSpriteDef == nullptr)
        return 0;
    if (mSpriteDef->mMainDef != nullptr)
    {
        if (SetupSpriteInst(String()) == 0)
            return 0;
        int aLabel = mSpriteDef->mMainDef->GetLabelFrame(theName);
        if (aLabel == -1)
            return 0;
        mAnimName = theName;
        return Play(aLabel, theReset);
    }
    SetupSpriteInst(theName);
    if (mSpriteInst == nullptr || mSpriteInst->mDef == nullptr)
        return 0;
    return Play(mSpriteInst->mDef->mFirstFrame, theReset);
}

int PopAnim::BlendTo(String const& theName, int theDuration, int theDelay)
{
    int aResult = SetupSpriteInst(String());
    if (aResult == 0 || mSpriteInst == nullptr || mSpriteInst->mDef == nullptr)
        return 0;
    if (mTransformsDirty)
    {
        UpdateTransforms(mSpriteInst, nullptr, mColor, false);
        mTransformsDirty = false;
    }

    struct BlendSrc
    {
        std::vector<PAParticleEffect> mParticles;
        PATransform mTransform;
        Color mColor;
    };
    std::multimap<String, BlendSrc> aSources;
    int aFrame = (int)mSpriteInst->mFrameTime;
    if (aFrame >= 0 && aFrame < (int)mSpriteInst->mDef->mFrames.size())
    {
        PAFrame& aFrameRef = mSpriteInst->mDef->mFrames[(size_t)aFrame];
        for (size_t aIndex = 0; aIndex < aFrameRef.mObjects.size(); ++aIndex)
        {
            PAObjectPos* aPos = aFrameRef.mObjects[aIndex];
            if (aPos == nullptr || aPos->mDef == nullptr)
                continue;
            int aObject = aPos->mDef->mIndex;
            if (aObject < 0 || aObject >= (int)mSpriteInst->mObjects.size())
                continue;
            PAObjectInst& aInst = mSpriteInst->mObjects[(size_t)aObject];
            if (aInst.mName == nullptr || aInst.mName[0] == 0)
                continue;
            BlendSrc aSrc;
            if (aPos->mDef->mIsSprite && aInst.mInst != nullptr)
            {
                aSrc.mColor = aInst.mInst->mColor;
                aSrc.mTransform.mMatrix = aInst.mInst->mTransform;
            }
            else
            {
                CalcObjectPos(mSpriteInst, (int)aIndex, false, &aSrc.mTransform, &aSrc.mColor);
            }
            if (aInst.mInst != nullptr)
                aSrc.mParticles.swap(aInst.mInst->mParticles);
            const char* aKey = aPos->mDef->mName != nullptr ? aPos->mDef->mName : "";
            aSources.insert(std::make_pair(String(aKey), aSrc));
        }
    }
    std::vector<PAParticleEffect> aSaved;
    aSaved.swap(mSpriteInst->mParticles);
    mBlendDuration = (float)theDuration;
    mDelay = (float)theDelay;
    mBlendTime = 0.0f;

    if (mSpriteDef->mMainDef != nullptr)
    {
        if (SetupSpriteInst(String()) == 0)
            return 0;
        int aLabel = mSpriteDef->mMainDef->GetLabelFrame(theName);
        if (aLabel == -1)
            return 0;
        mAnimName = theName;
        Play(aLabel, false);
        mTransformsDirty = true;
    }
    else
    {
        SetupSpriteInst(theName);
        if (mSpriteInst->mDef != nullptr)
            Play(mSpriteInst->mDef->mFirstFrame, false);
    }
    mSpriteInst->mParticles.swap(aSaved);

    for (size_t aIndex = 0; aIndex < mSpriteInst->mObjects.size(); ++aIndex)
    {
        PAObjectInst& aObject = mSpriteInst->mObjects[aIndex];
        if (aObject.mName == nullptr || aObject.mName[0] == 0)
            continue;
        std::multimap<String, BlendSrc>::iterator aFound = aSources.find(String(aObject.mName));
        if (aFound == aSources.end())
        {
            aObject.mBlending = false;
            continue;
        }
        aObject.mBlending = true;
        aObject.mPrevRed = aFound->second.mColor.mRed;
        aObject.mPrevGreen = aFound->second.mColor.mGreen;
        aObject.mPrevBlue = aFound->second.mColor.mBlue;
        aObject.mPrevAlpha = aFound->second.mColor.mAlpha;
        aObject.mPrevTransform = aFound->second.mTransform.mMatrix;
        if (aObject.mInst != nullptr && !aFound->second.mParticles.empty())
            aObject.mInst->mParticles.swap(aFound->second.mParticles);
        else
        {
            for (size_t aParticle = 0; aParticle < aFound->second.mParticles.size(); ++aParticle)
                delete aFound->second.mParticles[aParticle].mEffect;
            aFound->second.mParticles.clear();
        }
        aSources.erase(aFound);
    }
    for (std::multimap<String, BlendSrc>::iterator aLeft = aSources.begin(); aLeft != aSources.end(); ++aLeft)
    {
        for (size_t aParticle = 0; aParticle < aLeft->second.mParticles.size(); ++aParticle)
            delete aLeft->second.mParticles[aParticle].mEffect;
    }
    return aResult;
}

void PopAnim::Update()
{
    if (!mPamLoaded)
        return;
    Widget::Update();
    if (SetupSpriteInst(String()) == 0)
        return;
    if (!gSexyAppBase->mVSyncUpdates)
        UpdateF(1.0f);
    UpdateTransforms(mSpriteInst, nullptr, mColor, false);
    mTransformsDirty = false;
    if (!mParticlesDisabled)
    {
        UpdateParticles(mSpriteInst, nullptr);
        CleanParticles(mSpriteInst, false);
    }
}

int PopAnim::CalcObjectPos(PASpriteInst* theSprite, int theIndex, bool theNoInterp,
                           PATransform* theTransform, Color* theColor)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theTransform == nullptr || theColor == nullptr)
        return 0;
    int aFrameIndex = (int)theSprite->mFrameTime;
    if (aFrameIndex < 0 || aFrameIndex >= (int)theSprite->mDef->mFrames.size())
        return 0;
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aFrameIndex];
    if (theIndex < 0 || theIndex >= (int)aFrame.mObjects.size() || aFrame.mObjects[(size_t)theIndex] == nullptr)
        return 0;
    PAObjectPos* aPos = aFrame.mObjects[(size_t)theIndex];
    if (aPos->mDef == nullptr)
        return 0;
    int aObject = aPos->mDef->mIndex;
    if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
        return 0;
    PAObjectInst& aInst = theSprite->mObjects[(size_t)aObject];

    PATransform aPose;
    Color aColor;
    aPose.mMatrix = aPos->mMatrix;
    Color::FromInt(&aColor, aPos->mPackedColor);
    if (mInterpolatePoses && !theNoInterp)
    {
        int aNextIndex;
        if (mSpriteInst == theSprite && theSprite->mFrameTime >= (float)theSprite->mDef->mFirstFrame)
        {
            int aSpan = theSprite->mDef->mLastFrame + 1;
            if (aSpan <= 0)
                aSpan = 1;
            aNextIndex = ((aFrameIndex + 1 - theSprite->mDef->mFirstFrame) % aSpan) + theSprite->mDef->mFirstFrame;
        }
        else
        {
            int aCount = (int)theSprite->mDef->mFrames.size();
            aNextIndex = aCount == 0 ? 0 : (aFrameIndex + 1) % aCount;
        }
        PAObjectPos* aNext = nullptr;
        if (aNextIndex >= 0 && aNextIndex < (int)theSprite->mDef->mFrames.size())
            aNext = PoseAt(&theSprite->mDef->mFrames[(size_t)aNextIndex], theIndex, aPos->mDef->mIndex);
        float aFrac = theSprite->mFrameTime - (float)aFrameIndex;
        if (aNext == nullptr)
        {
            aPose.mMatrix = aPos->mMatrix;
            Color::FromInt(&aColor, aPos->mPackedColor);
        }
        else if (aPos == aNext || !aPos->mInterpolate)
        {
            PAObjectPos* aChosen = (aPos != aNext && aFrac >= 0.5f) ? aNext : aPos;
            aPose.mMatrix = aChosen->mMatrix;
            Color::FromInt(&aColor, aChosen->mPackedColor);
        }
        else
        {
            PATransform aFrom;
            aFrom.mMatrix = aPos->mMatrix;
            aPose.InterpolateTo(aFrom, aNext->mMatrix.m, aFrac);
            aColor = MixPacked(aPos->mPackedColor, aNext->mPackedColor, aFrac);
        }
    }

    aPose.mMatrix = aInst.mTransform * aPose.mMatrix;
    if (aInst.mBlending && mBlendDuration != 0.0f && mSpriteInst == theSprite)
    {
        float aBlend = mBlendTime / mBlendDuration;
        PATransform aFrom;
        aFrom.mMatrix = aInst.mPrevTransform;
        PATransform aCurrent = aPose;
        aPose.InterpolateTo(aFrom, aCurrent.mMatrix.m, aBlend);
        Color aPrev(aInst.mPrevRed, aInst.mPrevGreen, aInst.mPrevBlue, aInst.mPrevAlpha);
        aColor = LerpColor(aPrev, aColor, aBlend);
    }
    *theTransform = aPose;
    *theColor = aColor;
    return aColor.mRed;
}

int PopAnim::UpdateTransforms(PASpriteInst* theSprite, PATransform* theParent,
                              Color const& theColor, bool theNoInterp)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr)
        return 0;
    if (theParent != nullptr)
        theSprite->mTransform = theParent->mMatrix;
    else
        theSprite->mTransform = mTransform;
    theSprite->mColor = theColor;

    int aFrameIndex = (int)theSprite->mFrameTime;
    if (aFrameIndex < 0 || aFrameIndex >= (int)theSprite->mDef->mFrames.size())
        return 0;
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aFrameIndex];
    bool aSnap = theNoInterp || theSprite->mHoldFrames > 0 || aFrame.mHold;
    for (size_t aIndex = 0; aIndex < aFrame.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrame.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
            continue;
        PATransform aObject;
        Color aObjectColor;
        CalcObjectPos(theSprite, (int)aIndex, aSnap, &aObject, &aObjectColor);
        if (theParent != nullptr)
            aObject.TransformSrc(*theParent, aObject.mMatrix.m);
        int aObjectIndex = aPos->mDef->mIndex;
        if (aObjectIndex < 0 || aObjectIndex >= (int)theSprite->mObjects.size())
            continue;
        UpdateTransforms(theSprite->mObjects[(size_t)aObjectIndex].mInst, &aObject, aObjectColor, aSnap);
    }
    for (size_t aIndex = 0; aIndex < theSprite->mParticles.size(); ++aIndex)
    {
        PAParticleEffect& aEntry = theSprite->mParticles[aIndex];
        if (!aEntry.mFollowSprite || aEntry.mEffect == nullptr)
            continue;
        SexyTransform2D aEffect;
        if (aEntry.mActive)
        {
            aEffect.Translate((float)aEntry.mEffect->mParticleScale * 0.5f,
                              (float)aEntry.mEffect->mTimelineStart * 0.5f);
            aEffect = theSprite->mTransform * aEffect;
        }
        else
        {
            SexyVector2 aMoved = theSprite->mTransform * SexyVector2((float)aEntry.mOffsetX, (float)aEntry.mOffsetY);
            aEffect.Translate(aMoved.mX, aMoved.mY);
        }
        aEntry.mEffect->mWorldTransform = aEffect;
        aEntry.mEffect->mWorldTransform.Translate(mOffsetX, mOffsetY);
    }
    return 0;
}

float PopAnim::DrawParticleEffects(Graphics* theGraphics, PASpriteInst* theSprite,
                                   PATransform* theParent, Color const& theColor, bool theAdditive)
{
    float aResult = 0.0f;
    if (theSprite == nullptr)
        return aResult;
    for (size_t aIndex = 0; aIndex < theSprite->mParticles.size(); ++aIndex)
    {
        PAParticleEffect& aEntry = theSprite->mParticles[aIndex];
        if (aEntry.mEffect == nullptr)
            continue;
        if (aEntry.mActive)
        {
            if (aEntry.mFollowSprite)
            {
                aEntry.mEffect->mTransform.LoadIdentity();
                aEntry.mEffect->mTransform.Translate(-mOffsetX, -mOffsetY);
            }
            else if (theParent != nullptr)
            {
                SexyTransform2D aLocal;
                aLocal.Translate((float)aEntry.mEffect->mParticleScale * 0.5f,
                                 (float)aEntry.mEffect->mTimelineStart * 0.5f);
                aEntry.mEffect->mTransform = theParent->mMatrix * aLocal;
            }
            aEntry.mEffect->mColor = theColor;
        }
        if (aEntry.mDrawAdditive == !theAdditive)
            aResult = aEntry.mEffect->Draw(theGraphics);
    }
    return aResult;
}

namespace {

void DrawImagePose(Graphics* theGraphics, PopAnim* theAnim, Image* theImage,
                   SexyMatrix3& theMatrix, TRect<int> const& theSrc, bool theMirrored)
{
    if (theGraphics == nullptr || theImage == nullptr || theAnim == nullptr)
        return;
    if (theAnim->mScaleY != 1.0f)
    {
        float aTransX = theMatrix.m02;
        float aTransY = theMatrix.m12;
        PATransform aScale;
        aScale.mMatrix.m00 = 1.0f / theAnim->mScaleY;
        aScale.mMatrix.m11 = aScale.mMatrix.m00;
        PATransform aScaled;
        aScaled.TransformSrc(aScale, theMatrix.m);
        theMatrix = aScaled.mMatrix;
        theMatrix.m02 = aTransX;
        theMatrix.m12 = aTransY;
    }
    if (theMirrored && theAnim->mScaleX != 1.0f)
        theMatrix.m02 += (1.0f - theAnim->mScaleX) * (float)theAnim->mAnimWidth;

    bool aAxis = theMirrored
        ? (theMatrix.m00 == 1.0f && theMatrix.m01 == 0.0f && theMatrix.m10 == 0.0f && theMatrix.m11 == 1.0f)
        : (fabsf(theMatrix.m00 - 1.0f) <= 0.001f && fabsf(theMatrix.m01) <= 0.001f
           && fabsf(theMatrix.m10) <= 0.001f && fabsf(theMatrix.m11 - 1.0f) <= 0.001f);
    if (aAxis)
    {
        if (theMirrored)
        {
            float aX = ((float)(theAnim->mAnimWidth - theSrc.mWidth) * 0.5f - theMatrix.m02)
                - (float)(theSrc.mWidth - theAnim->mAnimWidth) * 0.5f;
            theGraphics->DrawImageF(theImage, aX, theMatrix.m12, theSrc);
            return;
        }
        if (fabsf(theMatrix.m02 - (float)(int)theMatrix.m02) > 0.001f
            || fabsf(theMatrix.m12 - (float)(int)theMatrix.m12) > 0.001f)
        {
            if (theAnim->mDebugDraw)
                theGraphics->SetColor(Color(128, 0, 255));
            theGraphics->DrawImageF(theImage, theMatrix.m02, theMatrix.m12, theSrc);
        }
        else
        {
            if (theAnim->mDebugDraw)
                theGraphics->SetColor(Color(128, 255, 0));
            theGraphics->DrawImage(theImage, (int)theMatrix.m02, (int)theMatrix.m12, theSrc);
        }
        return;
    }
    bool aPureRotate = theAnim->mPamVersion != 1
        && theMatrix.m00 == theMatrix.m11
        && theMatrix.m01 == -theMatrix.m10
        && fabsf(theMatrix.m00 * theMatrix.m00 + theMatrix.m01 * theMatrix.m01 - 1.0f) < 0.01f;
    if (!aPureRotate)
    {
        SexyMatrix3 aPivot;
        aPivot.LoadIdentity();
        aPivot.m02 = (float)theSrc.mWidth * 0.5f;
        aPivot.m12 = (float)theSrc.mHeight * 0.5f;
        SexyMatrix3 aComposed = theMatrix * aPivot;
        if (theMirrored)
        {
            aComposed.m02 = (float)theAnim->mAnimWidth - aComposed.m02;
            aComposed.m01 = -aComposed.m01;
            aComposed.m10 = -aComposed.m10;
        }
        else if (theAnim->mDebugDraw)
        {
            theGraphics->SetColor(Color(255, 0, 0));
        }
        theGraphics->DrawImageMatrix(theImage, aComposed, theSrc, 0.0f, 0.0f, true);
        return;
    }
    float aAngle = atan2f(theMatrix.m01, theMatrix.m00);
    float aDrawAngle = theMirrored ? -aAngle : aAngle;
    float aCos = cosf(theMirrored ? -aAngle : aAngle);
    float aSin = sinf(theMirrored ? -aAngle : aAngle);
    float aHalfW = (float)theSrc.mWidth * 0.5f;
    float aHalfH = (float)theSrc.mHeight * 0.5f;
    float aX;
    float aY;
    if (theMirrored)
    {
        float aBase = ((float)(theAnim->mAnimWidth - theSrc.mWidth) * 0.5f - theMatrix.m02)
            - (float)(theSrc.mWidth - theAnim->mAnimWidth) * 0.5f;
        aX = (aBase + aHalfW + aSin * aHalfH) - aCos * aHalfW;
        aY = (theMatrix.m12 - aHalfH + aSin * aHalfW) + aCos * aHalfH;
    }
    else
    {
        aX = (theMatrix.m02 - aHalfW) + aCos * aHalfW - aSin * aHalfH;
        aY = (theMatrix.m12 - aHalfH) + aSin * aHalfW + aCos * aHalfH;
        if (theAnim->mDebugDraw)
            theGraphics->SetColor(Color(0, 0, 255));
    }
    theGraphics->DrawImageRotatedF(theImage, aX, aY, (double)aDrawAngle, &theSrc);
}

} // namespace

int PopAnim::DrawSprite(Graphics* theGraphics, PASpriteInst* theSprite, PATransform* theParent,
                        Color const& theColor, bool theAdditive, bool theNoInterp)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theGraphics == nullptr)
        return 0;
    DrawParticleEffects(theGraphics, theSprite, theParent, theColor, false);
    int aFrameIndex = (int)theSprite->mFrameTime;
    if (aFrameIndex < 0 || aFrameIndex >= (int)theSprite->mDef->mFrames.size())
    {
        DrawParticleEffects(theGraphics, theSprite, theParent, theColor, true);
        theGraphics->SetColorizeImages(false);
        theGraphics->SetDrawMode(0);
        return 0;
    }
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aFrameIndex];
    bool aSnap = theNoInterp || theSprite->mHoldFrames > 0 || aFrame.mHold;
    for (size_t aIndex = 0; aIndex < aFrame.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrame.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr)
            continue;
        int aObjectIndex = aPos->mDef->mIndex;
        if (aObjectIndex < 0 || aObjectIndex >= (int)theSprite->mObjects.size())
            continue;
        PAObjectInst& aObject = theSprite->mObjects[(size_t)aObjectIndex];
        if (mListener != nullptr && aObject.mQueryDraw)
        {
            PATransform* aQuery = theParent;
            aObject.mQueryDraw = mListener->ShouldDrawObject(mId, theGraphics, theSprite, &aObject, aQuery, theColor);
        }

        PATransform aPose;
        Color aPoseColor;
        if (aPos->mDef->mIsSprite && aObject.mInst != nullptr)
        {
            aPose.mMatrix = aObject.mInst->mTransform;
            aPoseColor = aObject.mInst->mColor;
        }
        else
        {
            CalcObjectPos(theSprite, (int)aIndex, aSnap, &aPose, &aPoseColor);
        }
        if (theParent != nullptr)
        {
            if (aPos->mDef->mIsSprite)
            {
                if (mScaleX != 1.0f)
                {
                    SexyMatrix3 aScale;
                    aScale.LoadIdentity();
                    aScale.m00 = mScaleX;
                    aScale.m11 = mScaleX;
                    aPose.mMatrix = aScale * aPose.mMatrix;
                }
                aPose.mMatrix = mTransform * aPose.mMatrix;
            }
            else
            {
                aPose.TransformSrc(*theParent, aPose.mMatrix.m);
            }
        }
        else if (mScaleX != 1.0f)
        {
            SexyMatrix3 aScale;
            aScale.LoadIdentity();
            aScale.m00 = mScaleX;
            aScale.m11 = mScaleX;
            aPose.mMatrix = (mTransform * aScale) * aPose.mMatrix;
        }
        else
        {
            aPose.mMatrix = mTransform * aPose.mMatrix;
        }

        Color aTint(
            aObject.mColor.mRed * theColor.mRed * aPoseColor.mRed / 65025,
            aObject.mColor.mGreen * theColor.mGreen * aPoseColor.mGreen / 65025,
            aObject.mColor.mBlue * theColor.mBlue * aPoseColor.mBlue / 65025,
            aObject.mColor.mAlpha * theColor.mAlpha * aPoseColor.mAlpha / 65025);
        if (aTint.mAlpha == 0)
            continue;
        if (aPos->mDef->mIsSprite)
        {
            bool aChildAdditive = theAdditive || aPos->mDef->mAdditive;
            if (aObject.mInst != nullptr)
                DrawSprite(theGraphics, aObject.mInst, &aPose, aTint, aChildAdditive, aSnap);
        }
        else if (aPos->mDef->mImageIndex >= 0 && aPos->mDef->mImageIndex < (int)mImages.size())
        {
            PAImage& aImage = mImages[(size_t)aPos->mDef->mImageIndex];
            PATransform aImagePose;
            aImagePose.TransformSrc(aPose, aImage.mTransform.m);
            theGraphics->SetColorizeImages(true);
            theGraphics->SetColor(aTint);
            theGraphics->SetDrawMode((theAdditive || aPos->mDef->mAdditive) ? 1 : aImage.mDrawMode);
            if (!aImage.mRefs.empty())
            {
                Image* aLoaded = ImageOf(aImage.mRefs[0]);
                TRect<int> aSrc = aLoaded != nullptr ? aLoaded->GetCelRect(0) : TRect<int>();
                if (aPos->mDef->mUseSrcRect)
                    aSrc = aPos->mDef->mSrcRect;
                int aHelper = 0;
                if (mListener != nullptr && aObject.mUseDrawHelper)
                {
                    aHelper = mListener->DrawObjectHelper(mId, theSprite, &aObject, aImagePose.mMatrix.m,
                                                          aLoaded, theGraphics, 0.0f);
                    if (aHelper == 0)
                        aObject.mUseDrawHelper = false;
                }
                if (aHelper != 2)
                {
                    DrawImagePose(theGraphics, this, aLoaded, aImagePose.mMatrix, aSrc, false);
                    if (aHelper == 3)
                        DrawImagePose(theGraphics, this, aLoaded, aImagePose.mMatrix, aSrc, false);
                }
            }
        }
        if (mListener != nullptr && aObject.mQueryDrawn)
            aObject.mQueryDrawn = mListener->DidDrawObject(mId, theGraphics, theSprite, &aObject, theParent, theColor);
    }
    DrawParticleEffects(theGraphics, theSprite, theParent, theColor, true);
    theGraphics->SetColorizeImages(false);
    theGraphics->SetDrawMode(0);
    return 0;
}

int PopAnim::DrawSpriteMirrored(Graphics* theGraphics, PASpriteInst* theSprite, PATransform* theParent,
                                Color const& theColor, bool theAdditive, bool theNoInterp)
{
    if (theSprite == nullptr || theSprite->mDef == nullptr || theGraphics == nullptr)
        return 0;
    DrawParticleEffects(theGraphics, theSprite, theParent, theColor, false);
    int aFrameIndex = (int)theSprite->mFrameTime;
    if (aFrameIndex < 0 || aFrameIndex >= (int)theSprite->mDef->mFrames.size())
    {
        DrawParticleEffects(theGraphics, theSprite, theParent, theColor, true);
        theGraphics->SetColorizeImages(false);
        theGraphics->SetDrawMode(0);
        return 0;
    }
    PAFrame& aFrame = theSprite->mDef->mFrames[(size_t)aFrameIndex];
    bool aSnap = theNoInterp || theSprite->mHoldFrames > 0 || aFrame.mHold;
    for (size_t aIndex = 0; aIndex < aFrame.mObjects.size(); ++aIndex)
    {
        PAObjectPos* aPos = aFrame.mObjects[aIndex];
        if (aPos == nullptr || aPos->mDef == nullptr)
            continue;
        int aObjectIndex = aPos->mDef->mIndex;
        if (aObjectIndex < 0 || aObjectIndex >= (int)theSprite->mObjects.size())
            continue;
        PAObjectInst& aObject = theSprite->mObjects[(size_t)aObjectIndex];
        PATransform aPose;
        Color aPoseColor;
        if (aPos->mDef->mIsSprite && aObject.mInst != nullptr)
        {
            aPose.mMatrix = aObject.mInst->mTransform;
            aPoseColor = aObject.mInst->mColor;
        }
        else
        {
            CalcObjectPos(theSprite, (int)aIndex, aSnap, &aPose, &aPoseColor);
        }
        if (theParent != nullptr && !aPos->mDef->mIsSprite)
            aPose.TransformSrc(*theParent, aPose.mMatrix.m);
        else if (theParent == nullptr && mScaleX != 1.0f && !aPos->mDef->mIsSprite)
        {
            PATransform aScale;
            aScale.mMatrix.m00 = mScaleX;
            aScale.mMatrix.m11 = mScaleX;
            aPose.TransformSrc(aScale, aPose.mMatrix.m);
        }
        Color aTint(
            theColor.mRed * aPoseColor.mRed / 255,
            theColor.mGreen * aPoseColor.mGreen / 255,
            theColor.mBlue * aPoseColor.mBlue / 255,
            theColor.mAlpha * aPoseColor.mAlpha / 255);
        if (aTint.mAlpha == 0)
            continue;
        if (aPos->mDef->mIsSprite)
        {
            if (aObject.mInst != nullptr)
                DrawSpriteMirrored(theGraphics, aObject.mInst, nullptr, aTint, theAdditive, aSnap);
            continue;
        }
        if (aPos->mDef->mImageIndex < 0 || aPos->mDef->mImageIndex >= (int)mImages.size())
            continue;
        PAImage& aImage = mImages[(size_t)aPos->mDef->mImageIndex];
        PATransform aImagePose;
        aImagePose.TransformSrc(aPose, aImage.mTransform.m);
        theGraphics->SetColorizeImages(true);
        theGraphics->SetColor(aTint);
        theGraphics->SetDrawMode((theAdditive || aPos->mDef->mAdditive) ? 1 : aImage.mDrawMode);
        if (aImage.mRefs.empty())
            continue;
        Image* aLoaded = ImageOf(aImage.mRefs[0]);
        TRect<int> aSrc = aLoaded != nullptr ? aLoaded->GetCelRect(0) : TRect<int>();
        if (aPos->mDef->mUseSrcRect)
            aSrc = aPos->mDef->mSrcRect;
        DrawImagePose(theGraphics, this, aLoaded, aImagePose.mMatrix, aSrc, true);
    }
    DrawParticleEffects(theGraphics, theSprite, theParent, theColor, true);
    theGraphics->SetColorizeImages(false);
    theGraphics->SetDrawMode(0);
    return 0;
}

void PopAnim::Draw(Graphics* theGraphics)
{
    if (!mPamLoaded || theGraphics == nullptr)
        return;
    if (SetupSpriteInst(String()) == 0)
        return;
    if (mTransformsDirty)
    {
        UpdateTransforms(mSpriteInst, nullptr, mColor, false);
        mTransformsDirty = false;
    }
    if (mDrawMirrored)
        DrawSpriteMirrored(theGraphics, mSpriteInst, nullptr, mColor, mAdditive, false);
    else
        DrawSprite(theGraphics, mSpriteInst, nullptr, mColor, mAdditive, false);
}

int PopAnim::UpdateParticles(PASpriteInst* theSprite, PAObjectPos* thePose)
{
    if (theSprite == nullptr)
        return 0;
    for (int aIndex = 0; aIndex < (int)theSprite->mParticles.size(); ++aIndex)
    {
        PAParticleEffect& aEntry = theSprite->mParticles[(size_t)aIndex];
        if (aEntry.mEffect == nullptr)
            continue;
        SexyVector2 aOffset(0.0f, 0.0f);
        if (!aEntry.mFollowSprite)
            aOffset = theSprite->mTransform * SexyVector2((float)aEntry.mOffsetX, (float)aEntry.mOffsetY);
        aEntry.mEffect->mTransform.LoadIdentity();
        aEntry.mEffect->mTransform.Translate(aOffset.mX, aOffset.mY);
        if (mDrawMirrored)
        {
            aEntry.mEffect->mTransform.Translate((float)(mWidth / -2), 0.0f);
            aEntry.mEffect->mTransform.Scale(-1.0f, 1.0f);
            aEntry.mEffect->mTransform.Translate((float)(mWidth / 2), 0.0f);
        }
        aEntry.mEffect->mTransform.Scale(mScaleX, mScaleX);
        if (aEntry.mActive && thePose != nullptr && thePose->mDef != nullptr && thePose->mDef->mRateScale != 0.0f)
            aEntry.mEffect->mTimeScale = 1.0f / thePose->mDef->mRateScale;
        aEntry.mEffect->Update();
        aEntry.mUpdateStamp = mUpdateCnt;
        if (aEntry.mEffect->IsActive() == 0)
        {
            delete aEntry.mEffect;
            aEntry.mEffect = nullptr;
            theSprite->mParticles.erase(theSprite->mParticles.begin() + aIndex);
            --aIndex;
        }
    }
    int aResult = 0;
    if (theSprite->mDef != nullptr && !theSprite->mDef->mFrames.empty())
    {
        int aFrame = (int)theSprite->mFrameTime;
        if (aFrame >= 0 && aFrame < (int)theSprite->mDef->mFrames.size())
        {
            PAFrame& aFrameRef = theSprite->mDef->mFrames[(size_t)aFrame];
            aResult = (int)aFrameRef.mObjects.size();
            for (size_t aIndex = 0; aIndex < aFrameRef.mObjects.size(); ++aIndex)
            {
                PAObjectPos* aPos = aFrameRef.mObjects[aIndex];
                if (aPos == nullptr || aPos->mDef == nullptr || !aPos->mDef->mIsSprite)
                    continue;
                int aObject = aPos->mDef->mIndex;
                if (aObject < 0 || aObject >= (int)theSprite->mObjects.size())
                    continue;
                UpdateParticles(theSprite->mObjects[(size_t)aObject].mInst, nullptr);
            }
        }
    }
    return aResult;
}

PopAnim* PopAnim::CleanParticles(PASpriteInst* theSprite, bool theAll)
{
    if (theSprite == nullptr)
        return this;
    for (int aIndex = 0; aIndex < (int)theSprite->mParticles.size(); ++aIndex)
    {
        PAParticleEffect& aEntry = theSprite->mParticles[(size_t)aIndex];
        if (aEntry.mUpdateStamp != mUpdateCnt || theAll)
        {
            delete aEntry.mEffect;
            aEntry.mEffect = nullptr;
            theSprite->mParticles.erase(theSprite->mParticles.begin() + aIndex);
            --aIndex;
        }
    }
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
    {
        if (theSprite->mObjects[aIndex].mInst != nullptr)
            CleanParticles(theSprite->mObjects[aIndex].mInst, theAll);
    }
    return this;
}

int PopAnim::HasParticles(PASpriteInst* theSprite) const
{
    if (theSprite == nullptr)
        return 0;
    if (!theSprite->mParticles.empty())
        return 1;
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
    {
        if (theSprite->mObjects[aIndex].mInst != nullptr && HasParticles(theSprite->mObjects[aIndex].mInst))
            return 1;
    }
    return 0;
}

int PopAnim::IsActive() const
{
    if (mActive)
        return 1;
    return HasParticles(mSpriteInst);
}

PopAnim* PopAnim::SaveStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSprite)
{
    if (theSprite == nullptr)
        return this;
    theBuffer.WriteLong((int)(theSprite->mFrameTime * 65536.0f));
    theBuffer.WriteLong(theSprite->mHoldFrames);
    theBuffer.WriteLong(theSprite->mLastStamp);
    theBuffer.WriteShort((short)theSprite->mParticles.size());
    for (size_t aIndex = 0; aIndex < theSprite->mParticles.size(); ++aIndex)
    {
        PAParticleEffect& aEntry = theSprite->mParticles[aIndex];
        if (aEntry.mEffect != nullptr)
            aEntry.mEffect->SaveState(theBuffer, false);
        theBuffer.WriteString(aEntry.mName);
        theBuffer.WriteBoolean(aEntry.mDrawAdditive);
        theBuffer.WriteBoolean(aEntry.mFollowSprite);
        theBuffer.WriteBoolean(aEntry.mActive);
        theBuffer.WriteLong((int)(aEntry.mOffsetX * 65536.0));
        theBuffer.WriteLong((int)(aEntry.mOffsetY * 65536.0));
    }
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
    {
        if (theSprite->mObjects[aIndex].mInst != nullptr)
            SaveStateSpriteInst(theBuffer, theSprite->mObjects[aIndex].mInst);
    }
    return this;
}

int PopAnim::SaveState(Buffer& theBuffer)
{
    int aAligned = (theBuffer.mWriteBitPos + 7) & ~7;
    theBuffer.mWriteBitPos = aAligned;
    if (aAligned < 0)
        aAligned += 7;
    int aSaved = aAligned >> 3;
    theBuffer.WriteLong(0);
    theBuffer.WriteShort(2);
    theBuffer.WriteBoolean(mPamLoaded);
    if (mPamLoaded)
    {
        theBuffer.WriteString(mPamFileName);
        theBuffer.WriteLong((long)mPamBuffer.GetCRC32(0));
        theBuffer.WriteBoolean(mDrawMirrored);
        theBuffer.WriteBoolean(mActive);
        theBuffer.WriteBoolean(mParticlesDisabled);
        theBuffer.WriteBoolean(mLoopNotified);
        SetupSpriteInst(String());
        const char* aDefName = (mSpriteInst != nullptr && mSpriteInst->mDef != nullptr)
            ? mSpriteInst->mDef->mName : nullptr;
        theBuffer.WriteString(String(aDefName != nullptr ? aDefName : ""));
        SaveStateSpriteInst(theBuffer, mSpriteInst);
        theBuffer.WriteBoolean(mSaveRandomState);
        if (mSaveRandomState)
        {
            char aRandomBytes[sizeof(unsigned long) * MTRAND_N];
            std::memcpy(aRandomBytes, mRandom.mt, sizeof(aRandomBytes));
            theBuffer.WriteString(String(aRandomBytes, (unsigned int)sizeof(aRandomBytes)));
        }
    }
    int aPos = theBuffer.mWriteBitPos;
    if (aPos < 0)
        aPos += 7;
    int aSizeWord = (aPos >> 3) - aSaved - 4;
    if (aSaved >= 0 && aSaved + 4 <= (int)theBuffer.mData.size())
        std::memcpy(&theBuffer.mData[(size_t)aSaved], &aSizeWord, sizeof(aSizeWord));
    return 1;
}

void PopAnim::LoadStateSpriteInst(Buffer const& theBuffer, PASpriteInst* theSprite)
{
    if (theSprite == nullptr)
        return;
    theSprite->mLoopCount = 0;
    theSprite->mFrameTime = (float)theBuffer.ReadLong() * kFixedScale;
    theSprite->mHoldFrames = theBuffer.ReadLong();
    theSprite->mFrameAdvanced = false;
    theSprite->mLastStamp = theBuffer.ReadLong();
    int aCount = theBuffer.ReadShort();
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        PAParticleEffect aEntry;
        aEntry.mEffect = new PIEffect();
        aEntry.mEffect->LoadState(theBuffer, false);
        aEntry.mName = theBuffer.ReadString();
        aEntry.mDrawAdditive = theBuffer.ReadBoolean();
        aEntry.mFollowSprite = theBuffer.ReadBoolean();
        aEntry.mActive = theBuffer.ReadBoolean();
        aEntry.mOffsetX = (double)((float)theBuffer.ReadLong() * kFixedScale);
        aEntry.mOffsetY = (double)((float)theBuffer.ReadLong() * kFixedScale);
        theSprite->mParticles.push_back(aEntry);
    }
    for (size_t aIndex = 0; aIndex < theSprite->mObjects.size(); ++aIndex)
    {
        if (theSprite->mObjects[aIndex].mInst != nullptr)
            LoadStateSpriteInst(theBuffer, theSprite->mObjects[aIndex].mInst);
    }
}

int PopAnim::LoadState(Buffer const& theBuffer)
{
    theBuffer.mReadBitPos = (theBuffer.mReadBitPos + 7) & ~7;
    int aSize = theBuffer.ReadLong();
    int aPayloadPos = theBuffer.mReadBitPos;
    int aVersion = theBuffer.ReadShort();
    if (!theBuffer.ReadBoolean())
        return 1;
    String aPamName = theBuffer.ReadString();
    int aCrc = theBuffer.ReadLong();
    mDrawMirrored = theBuffer.ReadBoolean();
    if (mPamLoaded)
    {
        if (mSpriteInst != nullptr)
        {
            ResetAnimHelper(mSpriteInst);
            CleanParticles(mSpriteInst, true);
            mActive = false;
        }
    }
    else
    {
        Load_LoadPam(aPamName);
    }
    mActive = theBuffer.ReadBoolean();
    mParticlesDisabled = theBuffer.ReadBoolean();
    mLoopNotified = aVersion <= 1 ? false : theBuffer.ReadBoolean();
    if (aCrc != (int)mPamBuffer.GetCRC32(0))
    {
        if (aPayloadPos < 0)
            aPayloadPos += 7;
        theBuffer.mReadBitPos = 8 * (aSize + (aPayloadPos >> 3));
        return 0;
    }
    String aSpriteName = theBuffer.ReadString();
    SetupSpriteInst(aSpriteName);
    LoadStateSpriteInst(theBuffer, mSpriteInst);
    if (aVersion > 0)
    {
        mSaveRandomState = theBuffer.ReadBoolean();
        if (mSaveRandomState)
            mRandom.SRand(theBuffer.ReadString());
    }
    return 1;
}

PopAnim::PopAnim(int theId, PopAnimListener* theListener)
    : mId(theId)
    , mListener(theListener)
    , mPamVersion(0)
    , mScaleX(1.0f)
    , mScaleY(1.0f)
    , mPamLoaded(false)
    , mPamCursor(0)
    , mMotion(0)
    , mOriginX(0)
    , mOriginY(0)
    , mAnimWidth(0)
    , mAnimHeight(0)
    , mSpriteInst(nullptr)
    , mSpriteDef(nullptr)
    , mBlendDuration(0.0f)
    , mBlendTime(0.0f)
    , mDelay(0.0f)
    , mSaveRandomState(false)
    , mOffsetX(0.0f)
    , mOffsetY(0.0f)
    , mColor(Color::White)
    , mAdditive(false)
    , mTransformsDirty(true)
    , mActive(false)
    , mDrawMirrored(false)
    , mInterpolatePoses(true)
    , mImageIsNew(false)
    , mParticlesDisabled(false)
    , mDebugDraw(false)
    , mLoopNotified(false)
{
    mClip = false;
    Clear();
    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPopAnims.insert(this);
    gSexyAppBase->mCritSect.Unlock();
    mSearchPaths.push_back(String("images\\"));
    mSearchPaths.push_back(String());
}

PopAnim::PopAnim(PopAnim const& theOther)
    : Widget(theOther)
    , mId(theOther.mId)
    , mListener(theOther.mListener)
    , mSearchPaths(theOther.mSearchPaths)
    , mPamVersion(theOther.mPamVersion)
    , mPamBuffer(theOther.mPamBuffer)
    , mScaleX(theOther.mScaleX)
    , mScaleY(theOther.mScaleY)
    , mPamLoaded(theOther.mPamLoaded)
    , mPamCursor(theOther.mPamCursor)
    , mModPamFile(theOther.mModPamFile)
    , mPamFileName(theOther.mPamFileName)
    , mRemaps(theOther.mRemaps)
    , mMotion(theOther.mMotion)
    , mOriginX(theOther.mOriginX)
    , mOriginY(theOther.mOriginY)
    , mAnimWidth(theOther.mAnimWidth)
    , mAnimHeight(theOther.mAnimHeight)
    , mErrorString(theOther.mErrorString)
    , mAnimName(theOther.mAnimName)
    , mImages(theOther.mImages)
    , mSpriteInst(nullptr)
    , mSpriteDef(theOther.mSpriteDef)
    , mBlendDuration(theOther.mBlendDuration)
    , mBlendTime(theOther.mBlendTime)
    , mDelay(theOther.mDelay)
    , mRandom(theOther.mRandom)
    , mSaveRandomState(theOther.mSaveRandomState)
    , mOffsetX(theOther.mOffsetX)
    , mOffsetY(theOther.mOffsetY)
    , mTransform(theOther.mTransform)
    , mColor(theOther.mColor)
    , mAdditive(theOther.mAdditive)
    , mTransformsDirty(theOther.mTransformsDirty)
    , mActive(theOther.mActive)
    , mDrawMirrored(theOther.mDrawMirrored)
    , mInterpolatePoses(theOther.mInterpolatePoses)
    , mImageIsNew(theOther.mImageIsNew)
    , mParticlesDisabled(theOther.mParticlesDisabled)
    , mDebugDraw(theOther.mDebugDraw)
    , mLoopNotified(theOther.mLoopNotified)
{
    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPopAnims.insert(this);
    gSexyAppBase->mCritSect.Unlock();
    mSpriteInst = new PASpriteInst();
    if (mSpriteDef != nullptr)
        ++mSpriteDef->mRefCount;
}

PopAnim* PopAnim::Duplicate()
{
    return new PopAnim(*this);
}

PopAnim::~PopAnim()
{
    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPopAnims.erase(this);
    gSexyAppBase->mCritSect.Unlock();
    Clear();
}

} // namespace Sexy

