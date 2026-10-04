// ResourceManager loads the XML manifest, registers each resource under its
// group, and loads a group on demand.
//
// The manifest is properties/resources.xml. A <Resources id="..."> group holds
// Image, Sound, Font, PopAnim, PIEffect, RenderEffect, File and SetDefaults.
// LoadNextResource walks the group's list and publishes the loaded object
// through the caller's slot when one was registered.

#include "ResourceManager.h"
#include "DeviceImage.h"
#include "Font.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "NativeDisplay.h"
#include "PopAnim.h"
#include "ReconHelpers.h"
#include "ResourceManagerException.h"
#include "SoundManager.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Sexy {

bool StringLessNoCase::operator()(Sexy::String const& theLeft, Sexy::String const& theRight) const
{
    return strcasecmp(theLeft.c_str(), theRight.c_str()) < 0;
}

} // namespace Sexy

namespace {

int LocaleFromText(Sexy::String const& theText)
{
    if (theText.empty())
        return 0;
    if (theText.size() == 4)
    {
        const char* aText = theText.c_str();
        return ((unsigned char)aText[0] << 24) | ((unsigned char)aText[1] << 16)
            | ((unsigned char)aText[2] << 8) | (unsigned char)aText[3];
    }
    return atoi(theText.c_str());
}

Sexy::String FileStem(Sexy::String const& thePath)
{
    const char* aText = thePath.c_str();
    const char* aSlash = strrchr(aText, '/');
    const char* aBack = strrchr(aText, '\\');
    const char* aName = aText;
    if (aSlash != nullptr && aSlash + 1 > aName)
        aName = aSlash + 1;
    if (aBack != nullptr && aBack + 1 > aName)
        aName = aBack + 1;
    return Sexy::String(aName);
}

Sexy::String FileDir(Sexy::String const& thePath)
{
    const char* aText = thePath.c_str();
    const char* aSlash = strrchr(aText, '/');
    const char* aBack = strrchr(aText, '\\');
    const char* aCut = aSlash;
    if (aBack != nullptr && (aCut == nullptr || aBack > aCut))
        aCut = aBack;
    if (aCut == nullptr)
        return Sexy::String();
    return Sexy::String(aText, (unsigned int)(aCut - aText));
}

static Sexy::String UpperCopy(Sexy::String const& theText)
{
    Sexy::String aUpper = theText;
    char* aData = aUpper.mData;
    if (aData == nullptr)
        return aUpper;
    for (unsigned int i = 0; i < aUpper.size(); ++i)
        aData[i] = (char)toupper((unsigned char)aData[i]);
    return aUpper;
}

bool StartsWithNoCase(Sexy::String const& theText, Sexy::String const& thePrefix)
{
    if (thePrefix.size() > theText.size())
        return false;
    return strncasecmp(theText.c_str(), thePrefix.c_str(), thePrefix.size()) == 0;
}

const char* AttributeOrNull(Sexy::XMLElement& theElement, const char* theName)
{
    Sexy::String aKey;
    aKey = theName;
    std::map<Sexy::String, Sexy::String>::iterator aFound = theElement.mAttributes.find(aKey);
    if (aFound == theElement.mAttributes.end())
        return nullptr;
    return aFound->second.c_str();
}

void ParseCommaInts(const char* theText, std::vector<int>& theOut)
{
    theOut.clear();
    if (theText == nullptr || theText[0] == '\0')
        return;
    const char* aCursor = theText;
    while (*aCursor != '\0')
    {
        while (*aCursor == ' ' || *aCursor == ',')
            ++aCursor;
        if (*aCursor == '\0')
            break;
        theOut.push_back(atoi(aCursor));
        while (*aCursor != '\0' && *aCursor != ',')
            ++aCursor;
    }
}

class HeldLock
{
public:
    explicit HeldLock(Sexy::CritSect& theSect)
        : mSect(theSect)
        , mHeld(true)
    {
        mSect.Lock();
    }

    ~HeldLock()
    {
        Unlock();
    }

    void Unlock()
    {
        if (mHeld)
        {
            mSect.Unlock();
            mHeld = false;
        }
    }

    void Lock()
    {
        if (!mHeld)
        {
            mSect.Lock();
            mHeld = true;
        }
    }

private:
    Sexy::CritSect& mSect;
    bool mHeld;
};

} // namespace

// ---------------------------------------------------------------------------
// Records. Strings are constructed. DeleteResource releases the loaded object
// and leaves the record in the map so a later load can fill it again.
// ---------------------------------------------------------------------------

Sexy::ResourceManager_BaseRes::ResourceManager_BaseRes()
    : mManager(nullptr)
    , mResourceRef(nullptr)
    , mHeldRef(nullptr)
    , mRefCount(0)
    , mRefHeld(false)
    , mGeneration(0)
    , mResType(ResType_Image)
    , mId()
    , mGroup()
    , mParentGroup()
    , mPath()
    , mIdEncoded()
    , mGroupEncoded()
    , mParentEncoded()
    , mPathEncoded()
    , mResId(0)
    , mLocale(0)
    , mAttributes()
    , mFromProgram(false)
{
}

Sexy::ResourceManager_BaseRes::~ResourceManager_BaseRes()
{
    if (mHeldRef != nullptr)
    {
        delete static_cast<Sexy::ResourceRef*>(mHeldRef);
        mHeldRef = nullptr;
    }
}

void Sexy::ResourceManager_BaseRes::DeleteResource()
{
}

void Sexy::ResourceManager_BaseRes::ApplyConfig()
{
}

Sexy::ResourceManager_SoundRes::ResourceManager_SoundRes()
    : mSoundId(-1)
    , mVolume(-1.0)
    , mPanning(0)
{
    mResType = ResType_Sound;
}

void Sexy::ResourceManager_SoundRes::DeleteResource()
{
    if (mSoundId >= 0 && mManager != nullptr && mManager->mApp != nullptr
        && mManager->mApp->mSoundManager != nullptr)
        mManager->mApp->mSoundManager->ReleaseSound((unsigned int)mSoundId);
    mSoundId = -1;
    if (mResourceRef != nullptr)
        *static_cast<int*>(mResourceRef) = -1;
}

void Sexy::ResourceManager_SoundRes::ApplyConfig()
{
    if (mSoundId < 0 || mManager == nullptr || mManager->mApp == nullptr)
        return;
    Sexy::SoundManager* aSounds = mManager->mApp->mSoundManager;
    if (aSounds == nullptr)
        return;
    if (mVolume >= 0.0)
        aSounds->SetBaseVolume((unsigned int)mSoundId, mVolume);
    if (mPanning != 0)
        aSounds->SetBasePan((unsigned int)mSoundId, mPanning);
}

Sexy::ResourceManager_FontRes::ResourceManager_FontRes()
    : mFont(nullptr)
    , mImage(nullptr)
    , mSourcePath()
    , mTags()
    , mIsSysFont(false)
    , mBold(false)
    , mItalic(false)
    , mUnderline(false)
    , mShadow(false)
    , mPointSize(0)
{
    mResType = ResType_Font;
}

void Sexy::ResourceManager_FontRes::DeleteResource()
{
    delete mFont;
    mFont = nullptr;
    delete mImage;
    mImage = nullptr;
    if (mResourceRef != nullptr)
        *static_cast<Sexy::Font**>(mResourceRef) = nullptr;
}

void Sexy::ResourceManager_FontRes::ApplyConfig()
{
    if (mFont == nullptr || mIsSysFont || mTags.empty())
        return;
    Sexy::ImageFont* aFont = dynamic_cast<Sexy::ImageFont*>(mFont);
    if (aFont == nullptr)
        return;

    char aTags[1024];
    strncpy(aTags, mTags.c_str(), sizeof(aTags) - 1);
    aTags[sizeof(aTags) - 1] = '\0';
    char* aToken = strtok(aTags, ", \r\n\t");
    while (aToken != nullptr)
    {
        Sexy::String aTag;
        aTag = aToken;
        aFont->AddTag(aTag);
        aToken = strtok(nullptr, ", \r\n\t");
    }
    aFont->Prepare();
}

Sexy::ResourceManager_PopAnimRes::ResourceManager_PopAnimRes()
    : mPopAnim(nullptr)
{
    mResType = ResType_Music;
}

void Sexy::ResourceManager_PopAnimRes::DeleteResource()
{
    delete mPopAnim;
    mPopAnim = nullptr;
    if (mResourceRef != nullptr)
        *static_cast<Sexy::PopAnim**>(mResourceRef) = nullptr;
}

Sexy::ResourceManager_PIEffectRes::ResourceManager_PIEffectRes()
    : mPIEffect(nullptr)
{
    mResType = ResType_Reanim;
}

void Sexy::ResourceManager_PIEffectRes::DeleteResource()
{
    delete mPIEffect;
    mPIEffect = nullptr;
    if (mResourceRef != nullptr)
        *static_cast<Sexy::PIEffect**>(mResourceRef) = nullptr;
}

Sexy::ResourceManager_RenderEffectRes::ResourceManager_RenderEffectRes()
    : mRenderEffect(nullptr)
    , mSourcePath()
{
    mResType = ResType_Particle;
}

void Sexy::ResourceManager_RenderEffectRes::DeleteResource()
{
    delete mRenderEffect;
    mRenderEffect = nullptr;
    if (mResourceRef != nullptr)
        *static_cast<Sexy::RenderEffectDefinition**>(mResourceRef) = nullptr;
}

Sexy::ResourceManager_GenericResFileRes::ResourceManager_GenericResFileRes()
    : mGenericResFile(nullptr)
    , mFileBody()
{
    mResType = ResType_Trail;
}

void Sexy::ResourceManager_GenericResFileRes::DeleteResource()
{
    mGenericResFile = nullptr;
    mFileBody.clear();
    if (mResourceRef != nullptr)
        *static_cast<void**>(mResourceRef) = nullptr;
}

Sexy::ResourceManager_ImageRes::ResourceManager_ImageRes()
    : mImage()
    , mAlphaImagePath()
    , mAlphaGridPath()
    , mVariant()
    , mAlphaX(0)
    , mAlphaY(0)
    , mNoAlpha(true)
    , mNoPal(true)
    , mUseA4R4G4B4(false)
    , mUseA8R8G8B8(false)
    , mUseA1R5G5B5(false)
    , mUseR5G6B5(false)
    , mNoBits(false)
    , mMinSubdivide(false)
    , mCubeMap(false)
    , mVolumeMap(false)
    , mHasBits(true)
    , m2DBig(false)
    , mAtlas(false)
    , mRows(1)
    , mCols(1)
    , mAlphaColor(0xFFFFFF)
    , mAnim()
    , mParentId()
    , mAlphaXOffset(0)
    , mAlphaYOffset(0)
    , mAlphaWidth(0)
    , mAlphaHeight(0)
{
    mResType = ResType_Image;
}

void Sexy::ResourceManager_ImageRes::DeleteResource()
{
    mImage.Release();
    if (mResourceRef != nullptr)
        *static_cast<Sexy::Image**>(mResourceRef) = nullptr;
}

void Sexy::ResourceManager_ImageRes::ApplyConfig()
{
    Sexy::Image* anImage = mImage;
    if (anImage == nullptr)
        return;
    anImage->mNumRows = mRows > 0 ? mRows : 1;
    anImage->mNumCols = mCols > 0 ? mCols : 1;
    if (mAnim.mAnimType != AnimType_None)
    {
        if (anImage->mAnimInfo == nullptr)
            anImage->mAnimInfo = new Sexy::AnimInfo(mAnim);
        else
            *anImage->mAnimInfo = mAnim;
    }
}

Sexy::ResourceManager::SubGroup::SubGroup()
    : mName()
    , mScale(0)
    , mLocale(0)
{
}

// ---------------------------------------------------------------------------
// The manager. Maps and strings construct themselves. The three kind pointers
// name the first three maps so a parser can hand the map to the shared path.
// ---------------------------------------------------------------------------

Sexy::ResourceManager::ResourceManager(Sexy::SexyAppBase* theApp)
    : mLoadedGroups()
    , mResMaps()
    , mResMapByPath()
    , mGlobalPtrMap()
    , mResFilePath()
    , mResGenDir()
    , mResGenResId(0)
    , mResGenLocale(0)
    , mResGenName()
    , mResGenPrefix()
    , mResGenSuffix()
    , mImageMap(&mResMaps[ResType_Image])
    , mSoundMap(&mResMaps[ResType_Sound])
    , mFontMap(&mResMaps[ResType_Font])
    , mUnusedPath()
    , mUnusedId()
    , mXMLParser(nullptr)
    , mErrorText()
    , mHasError(false)
    , mApp(theApp)
    , mParentResGroupName()
    , mCurResGroupName()
    , mCurResGroupResId(0)
    , mCurResGroupLocale(0)
    , mPathPrefix()
    , mIdPrefix()
    , mAllowMissingProgramImages(false)
    , mIsReparsing(false)
    , mReuseLoadedResources(false)
    , mResGroupGeneration(0)
    , mResGroupMap()
    , mCurResGroupList(nullptr)
    , mCurResGroupItr()
    , mCompositeResGroupMap()
    , mCompositeResGroupIndex(0)
    , mImageResDir(0)
    , mImageResScale(0)
    , mLocaleCode(0x454E5553)
    , mSubGroupList()
    , mManifestVersion(1)
    , mCritSect()
    , mResSuffix()
    , mLocaleSuffix()
    , mResLocaleSuffix()
{
}

Sexy::ResourceManager::~ResourceManager()
{
    for (int aKind = 0; aKind < 7; ++aKind)
        DeleteMap(mResMaps[aKind]);
    delete mXMLParser;
    mXMLParser = nullptr;
}

void Sexy::ResourceManager::DeleteMap(ResMap& theMap)
{
    for (ResMap::iterator anEntry = theMap.begin(); anEntry != theMap.end(); ++anEntry)
    {
        if (anEntry->second == nullptr)
            continue;
        anEntry->second->DeleteResource();
        delete anEntry->second;
    }
    theMap.clear();
}

void Sexy::ResourceManager::EnsureSuffixes()
{
    if (!mResSuffix.empty())
        return;
    char aText[64];
    sprintf(aText, "|%d", mImageResScale);
    mResSuffix = aText;
    sprintf(aText, "||%08x", mLocaleCode);
    mLocaleSuffix = aText;
    sprintf(aText, "|%d||%08x", mImageResScale, mLocaleCode);
    mResLocaleSuffix = aText;
}

Sexy::ResourceManager::ResList& Sexy::ResourceManager::GroupList(Sexy::String const& theName)
{
    return mResGroupMap[theName];
}

void Sexy::ResourceManager::Publish(BaseRes* theRes, void* theValue)
{
    if (theRes != nullptr && theRes->mResourceRef != nullptr)
        *static_cast<void**>(theRes->mResourceRef) = theValue;
}

void Sexy::ResourceManager::Hold(BaseRes* theRes)
{
    if (theRes == nullptr || theRes->mRefHeld)
        return;
    theRes->mRefHeld = true;
    ++theRes->mRefCount;
}

// ---------------------------------------------------------------------------
// Lookup. A missing record is null. A present record is tried as the bare id,
// then with the scale suffix, the locale suffix, and both.
// ---------------------------------------------------------------------------

Sexy::ResourceManager::BaseRes* Sexy::ResourceManager::GetBaseRes(int theType, Sexy::String const& theId)
{
    if (theType < 0 || theType > 6)
        return nullptr;
    EnsureSuffixes();
    ResMap& aMap = mResMaps[theType];
    ResMap::iterator aFound = aMap.find(theId);
    if (aFound != aMap.end())
        return aFound->second;

    Sexy::String aScaled = theId + mResSuffix;
    aFound = aMap.find(aScaled);
    if (aFound != aMap.end())
        return aFound->second;

    Sexy::String aLocal = theId + mLocaleSuffix;
    aFound = aMap.find(aLocal);
    if (aFound != aMap.end())
        return aFound->second;

    Sexy::String aBoth = theId + mResLocaleSuffix;
    aFound = aMap.find(aBoth);
    if (aFound != aMap.end())
        return aFound->second;
    return nullptr;
}

bool Sexy::ResourceManager::Fail(Sexy::String const& theText)
{
    if (mHasError)
        return false;
    mHasError = true;
    mErrorText = theText;
    if (mXMLParser != nullptr)
    {
        int aLine = mXMLParser->GetCurrentLineNum();
        if (aLine > 0)
        {
            char aNumber[16];
            sprintf(aNumber, "%d", aLine);
            mErrorText = mErrorText + " on Line " + aNumber;
        }
        Sexy::String aFile = mXMLParser->GetFileName();
        if (!aFile.empty())
            mErrorText = mErrorText + " in File '" + aFile + "'";
    }
    fprintf(stderr, "ResourceManager::Fail: '%s'\n", mErrorText.c_str());
    fflush(stderr);
    return false;
}

Sexy::String Sexy::ResourceManager::GetErrorText()
{
    return mErrorText;
}

bool Sexy::ResourceManager::HadError()
{
    return mHasError;
}

bool Sexy::ResourceManager::IsGroupLoaded(Sexy::String const& theGroup)
{
    return mLoadedGroups.find(theGroup) != mLoadedGroups.end();
}

void Sexy::ResourceManager::SetAllowMissingProgramImages(bool theAllow)
{
    mAllowMissingProgramImages = theAllow;
}

void Sexy::ResourceManager::ResourceLoadedHook(BaseRes* theRes)
{
    (void)theRes;
}

void Sexy::ResourceManager::PrepareLoadResources(Sexy::String const& theGroup)
{
    (void)theGroup;
}

void Sexy::ResourceManager::PrepareLoadResourcesList(char const** theGroups)
{
    (void)theGroups;
}

float Sexy::ResourceManager::GetLoadResourcesListProgress(char const** theGroups)
{
    (void)theGroups;
    return 0.0f;
}

int Sexy::ResourceManager::GetLocSetForLocaleName(Sexy::String const& theName)
{
    (void)theName;
    return 0;
}

bool Sexy::ResourceManager::InitResourceGen(Sexy::String const& theDir, Sexy::String const& theName,
                                            Sexy::String const& thePrefix, int theResId, int theLocale,
                                            Sexy::String const& theSuffix)
{
    mResGenDir = theDir;
    mResGenResId = theResId;
    mResGenLocale = theLocale;
    mResGenName = theName;
    mResGenPrefix = thePrefix;
    mResGenSuffix = theSuffix;
    return true;
}

void Sexy::ResourceManager::ReapplyConfigs()
{
    for (int aKind = 0; aKind < 7; ++aKind)
    {
        for (ResMap::iterator anEntry = mResMaps[aKind].begin();
             anEntry != mResMaps[aKind].end(); ++anEntry)
        {
            if (anEntry->second != nullptr)
                anEntry->second->ApplyConfig();
        }
    }
}

Sexy::String Sexy::ResourceManager::GetLocaleFolder(bool theTrailingSlash)
{
    const char* aLocale = Sexy::setLocale(nullptr, nullptr);
    Sexy::String aFolder;
    if (aLocale != nullptr && aLocale[0] != '\0')
        aFolder = aLocale;
    else
        aFolder = "ENUS";
    if (theTrailingSlash)
        aFolder += "/";
    return aFolder;
}

void Sexy::ResourceManager::DumpCurResGroup(Sexy::String& theDest)
{
    theDest = mCurResGroupName;
    theDest += "\n";
    if (mCurResGroupList == nullptr)
        return;
    for (ResList::iterator aRecord = mCurResGroupList->begin();
         aRecord != mCurResGroupList->end(); ++aRecord)
    {
        if (*aRecord == nullptr)
            continue;
        theDest += (*aRecord)->mId;
        theDest += " ";
        theDest += (*aRecord)->mPath;
        theDest += "\n";
    }
}

int Sexy::ResourceManager::GetNumResources(Sexy::String const& theGroup, ResMap& theMap,
                                           bool theFilterRes, bool theFilterLocale)
{
    int aCount = 0;
    for (ResMap::iterator anEntry = theMap.begin(); anEntry != theMap.end(); ++anEntry)
    {
        BaseRes* aRecord = anEntry->second;
        if (aRecord == nullptr || aRecord->mFromProgram)
            continue;
        if (theFilterRes && aRecord->mResId != 0 && aRecord->mResId != mImageResScale)
            continue;
        if (theFilterLocale && aRecord->mLocale != 0 && aRecord->mLocale != mLocaleCode)
            continue;
        if (!theGroup.empty() && aRecord->mGroup != theGroup && aRecord->mParentGroup != theGroup)
            continue;
        ++aCount;
    }
    return aCount;
}

int Sexy::ResourceManager::GetNumImages(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale)
{
    return GetNumResources(theGroup, mResMaps[ResType_Image], theFilterRes, theFilterLocale);
}

int Sexy::ResourceManager::GetNumSounds(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale)
{
    return GetNumResources(theGroup, mResMaps[ResType_Sound], theFilterRes, theFilterLocale);
}

int Sexy::ResourceManager::GetNumFonts(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale)
{
    return GetNumResources(theGroup, mResMaps[ResType_Font], theFilterRes, theFilterLocale);
}

int Sexy::ResourceManager::GetNumResources(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale)
{
    int aCount = 0;
    for (int aKind = 0; aKind < 7; ++aKind)
        aCount += GetNumResources(theGroup, mResMaps[aKind], theFilterRes, theFilterLocale);
    return aCount;
}

void Sexy::ResourceManager::Deref(BaseRes* theRes)
{
    if (theRes == nullptr)
        return;
    int aCount = theRes->mRefCount - 1;
    theRes->mRefCount = aCount;
    if (aCount == 0)
        theRes->DeleteResource();
}

// ---------------------------------------------------------------------------
// Parse. A missing path fails. A leading '!' is stored as written; "!program"
// marks a resource the program fills itself. Anything else is joined to the
// current path prefix. The id is the prefix plus the attribute, or the file
// stem when the attribute is absent.
// ---------------------------------------------------------------------------

bool Sexy::ResourceManager::ParseCommonResource(Sexy::XMLElement& theElement, BaseRes* theRes, ResMap& theMap)
{
    mReuseLoadedResources = false;
    theRes->mManager = this;
    theRes->mResourceRef = nullptr;

    Sexy::String aPathKey;
    aPathKey = "path";
    std::map<Sexy::String, Sexy::String>::iterator aPathFound = theElement.mAttributes.find(aPathKey);
    if (aPathFound == theElement.mAttributes.end() || aPathFound->second.empty())
        return Fail(Sexy::String("No path specified."));

    const Sexy::String& aPathText = aPathFound->second;
    theRes->mAttributes = theElement.mAttributes;
    theRes->mFromProgram = false;

    if (!aPathText.empty() && aPathText.c_str()[0] == '!')
    {
        theRes->mPathEncoded = aPathText;
        theRes->mPath = aPathText;
        if (aPathText == "!program")
            theRes->mFromProgram = true;
    }
    else
    {
        theRes->mPathEncoded = mPathPrefix + aPathText;
        theRes->mPath = theRes->mPathEncoded;
        Sexy::String aUpper = UpperCopy(theRes->mPathEncoded);
        for (unsigned int i = 0; i < aUpper.size(); ++i)
        {
            if (aUpper[i] == '/')
                aUpper[i] = '\\';
        }
        mResMapByPath[aUpper] = theRes;
    }

    Sexy::String aId;
    const char* anIdAttr = AttributeOrNull(theElement, "id");
    if (anIdAttr == nullptr)
        aId = mIdPrefix + FileStem(theRes->mPath);
    else
    {
        Sexy::String aGiven;
        aGiven = anIdAttr;
        aId = mIdPrefix + aGiven;
    }

    if (mCurResGroupResId != 0)
    {
        char aText[256];
        sprintf(aText, "%s|%d", aId.c_str(), mCurResGroupResId);
        aId = aText;
    }
    if (mCurResGroupLocale != 0)
    {
        char aText[256];
        sprintf(aText, "%s||%08x", aId.c_str(), mCurResGroupLocale);
        aId = aText;
    }

    theRes->mGroup = mCurResGroupName;
    theRes->mGroupEncoded = mCurResGroupName;
    theRes->mParentGroup = mParentResGroupName;
    theRes->mParentEncoded = mParentResGroupName;
    theRes->mId = aId;
    theRes->mIdEncoded = aId;
    theRes->mLocale = mCurResGroupLocale;
    theRes->mResId = mCurResGroupResId;
    theRes->mGeneration = mResGroupGeneration;

    ResMap::iterator aExisting = theMap.find(aId);
    if (aExisting != theMap.end())
    {
        mReuseLoadedResources = true;
        return false;
    }

    theMap[aId] = theRes;
    ResList& aGroup = GroupList(mCurResGroupName);
    aGroup.push_back(theRes);
    mCurResGroupList = &mResGroupMap[mCurResGroupName];
    return true;
}

bool Sexy::ResourceManager::ParseSetDefaults(Sexy::XMLElement& theElement)
{
    Sexy::String aPath = theElement.GetAttribute("path");
    if (!aPath.empty())
    {
        while (!aPath.empty() && (aPath[aPath.size() - 1] == '/' || aPath[aPath.size() - 1] == '\\'))
            aPath = aPath.substr(0, aPath.size() - 1);
        mPathPrefix = aPath + "/";
    }
    Sexy::String aPrefix = theElement.GetAttribute("idprefix");
    if (!aPrefix.empty())
        mIdPrefix = aPrefix;
    return true;
}

bool Sexy::ResourceManager::ParseSoundResource(Sexy::XMLElement& theElement)
{
    SoundRes* aNew = new SoundRes();
    SoundRes* aLive = aNew;
    if (!ParseCommonResource(theElement, aNew, *mSoundMap))
    {
        if (!mReuseLoadedResources || !mIsReparsing)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        aLive = static_cast<SoundRes*>((*mSoundMap)[aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        if (aLive == nullptr)
            return false;
    }

    const char* aVolume = AttributeOrNull(theElement, "volume");
    if (aVolume != nullptr)
        sscanf(aVolume, "%lf", &aLive->mVolume);
    const char* aPan = AttributeOrNull(theElement, "pan");
    if (aPan != nullptr)
        sscanf(aPan, "%d", &aLive->mPanning);
    aLive->ApplyConfig();
    return true;
}

void Sexy::ResourceManager::ApplyImageFlags(ImageRes* theRes)
{
    theRes->ApplyConfig();
}

bool Sexy::ResourceManager::ParseImageResource(Sexy::XMLElement& theElement)
{
    const char* anId = AttributeOrNull(theElement, "id");
    const char* aPath = AttributeOrNull(theElement, "path");
    if (anId == nullptr || anId[0] == '\0' || aPath == nullptr || aPath[0] == '\0')
        return true;

    ImageRes* aNew = new ImageRes();
    ImageRes* aLive = aNew;
    if (!ParseCommonResource(theElement, aNew, *mImageMap))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        aLive = static_cast<ImageRes*>((*mImageMap)[aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        if (aLive == nullptr)
            return false;
    }

    aLive->mNoPal = theElement.GetAttributeBool(Sexy::String("nopal"), false) == 0;
    aLive->mUseA4R4G4B4 = theElement.GetAttributeBool(Sexy::String("a4r4g4b4"), false) != 0;
    aLive->mUseA1R5G5B5 = theElement.GetAttributeBool(Sexy::String("a1r5g5b5"), false) != 0;
    aLive->mUseR5G6B5 = theElement.GetAttributeBool(Sexy::String("r5g6b5"), false) != 0;
    if (theElement.GetAttributeBool(Sexy::String("nobits"), false) != 0)
        aLive->mNoBits = true;
    else if (mApp != nullptr && mApp->Is3DAccelerated() != 0)
        aLive->mNoBits = theElement.GetAttributeBool(Sexy::String("nobits3d"), false) != 0;
    else
        aLive->mNoBits = theElement.GetAttributeBool(Sexy::String("nobits2d"), false) != 0;
    aLive->mUseA8R8G8B8 = theElement.GetAttributeBool(Sexy::String("a8r8g8b8"), false) != 0;
    aLive->mMinSubdivide = theElement.GetAttributeBool(Sexy::String("minsubdivide"), false) != 0;
    aLive->mNoAlpha = theElement.GetAttributeBool(Sexy::String("noalpha"), false) == 0;
    aLive->mCubeMap = theElement.GetAttributeBool(Sexy::String("cubemap"), false) != 0;
    aLive->mVolumeMap = theElement.GetAttributeBool(Sexy::String("volumemap"), false) != 0;
    aLive->mHasBits = true;
    aLive->m2DBig = theElement.GetAttributeBool(Sexy::String("2dbig"), false) != 0;
    aLive->mAtlas = theElement.GetAttributeBool(Sexy::String("atlas"), false) != 0;

    const char* aAlphaImage = AttributeOrNull(theElement, "alphaimage");
    if (aAlphaImage != nullptr)
    {
        Sexy::String aJoined = mPathPrefix;
        aJoined += aAlphaImage;
        aLive->mAlphaImagePath = aJoined;
    }
    aLive->mAlphaColor = 0xFFFFFF;
    const char* aAlphaColor = AttributeOrNull(theElement, "alphacolor");
    if (aAlphaColor != nullptr)
    {
        unsigned int aValue = 0xFFFFFFu;
        sscanf(aAlphaColor, "%x", &aValue);
        aLive->mAlphaColor = (int)aValue;
    }

    const char* aX = AttributeOrNull(theElement, "x");
    aLive->mAlphaX = aX == nullptr ? 0 : atoi(aX);
    const char* aY = AttributeOrNull(theElement, "y");
    aLive->mAlphaY = aY == nullptr ? 0 : atoi(aY);

    const char* aVariant = AttributeOrNull(theElement, "variant");
    if (aVariant != nullptr)
        aLive->mVariant = aVariant;
    const char* aAlphaGrid = AttributeOrNull(theElement, "alphagrid");
    if (aAlphaGrid != nullptr)
    {
        Sexy::String aJoined = mPathPrefix;
        aJoined += aAlphaGrid;
        aLive->mAlphaGridPath = aJoined;
    }

    const char* aRows = AttributeOrNull(theElement, "rows");
    aLive->mRows = aRows == nullptr ? 1 : atoi(aRows);
    const char* aCols = AttributeOrNull(theElement, "cols");
    aLive->mCols = aCols == nullptr ? 1 : atoi(aCols);

    const char* aParent = AttributeOrNull(theElement, "parent");
    if (aParent != nullptr)
    {
        aLive->mParentId = aParent;
        aLive->mAlphaXOffset = atoi(theElement.GetAttribute("ax").c_str());
        aLive->mAlphaYOffset = atoi(theElement.GetAttribute("ay").c_str());
        aLive->mAlphaWidth = atoi(theElement.GetAttribute("aw").c_str());
        aLive->mAlphaHeight = atoi(theElement.GetAttribute("ah").c_str());
    }

    if (aLive->mCubeMap && aLive->mRows * aLive->mCols != 6)
        return Fail(Sexy::String("Invalid CubeMap definition; must have 6 cells (check rows & cols values)."));
    if (aLive->mVolumeMap)
    {
        int aCells = aLive->mRows * aLive->mCols;
        if (aCells == 0 || (aCells & (aCells - 1)) != 0)
            return Fail(Sexy::String("Invalid VolumeMap definition; must have a pow2 cell count (check rows & cols values)."));
    }

    const char* aAnim = AttributeOrNull(theElement, "anim");
    if (aAnim == nullptr || strcasecmp(aAnim, "none") == 0)
    {
        aLive->mAnim.mAnimType = AnimType_None;
    }
    else
    {
        AnimType aType = AnimType_None;
        if (strcasecmp(aAnim, "once") == 0)
            aType = AnimType_Once;
        else if (strcasecmp(aAnim, "loop") == 0)
            aType = AnimType_Loop;
        else if (strcasecmp(aAnim, "pingpong") == 0)
            aType = AnimType_PingPong;
        else
            return Fail(Sexy::String("Invalid animation type."));

        aLive->mAnim.mAnimType = aType;
        int aNumCels = aLive->mRows > aLive->mCols ? aLive->mRows : aLive->mCols;
        const char* aFrameDelay = AttributeOrNull(theElement, "framedelay");
        if (aFrameDelay != nullptr)
            aLive->mAnim.mFrameDelay = atoi(aFrameDelay);
        int aBeginDelay = 0;
        const char* aBegin = AttributeOrNull(theElement, "begindelay");
        if (aBegin != nullptr)
            aBeginDelay = atoi(aBegin);
        int aEndDelay = 0;
        const char* aEnd = AttributeOrNull(theElement, "enddelay");
        if (aEnd != nullptr)
            aEndDelay = atoi(aEnd);
        const char* aPerFrame = AttributeOrNull(theElement, "perframedelay");
        if (aPerFrame != nullptr)
            ParseCommaInts(aPerFrame, aLive->mAnim.mPerFrameDelay);
        const char* aFrameMap = AttributeOrNull(theElement, "framemap");
        if (aFrameMap != nullptr)
            ParseCommaInts(aFrameMap, aLive->mAnim.mFrameMap);
        aLive->mAnim.Compute(aNumCels, aBeginDelay, aEndDelay);
    }

    ApplyImageFlags(aLive);
    return true;
}

bool Sexy::ResourceManager::ParseFontResource(Sexy::XMLElement& theElement)
{
    FontRes* aNew = new FontRes();
    FontRes* aLive = aNew;
    if (!ParseCommonResource(theElement, aNew, *mFontMap))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        aLive = static_cast<FontRes*>((*mFontMap)[aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        if (aLive == nullptr)
            return false;
    }

    const char* anImage = AttributeOrNull(theElement, "image");
    if (anImage != nullptr)
        aLive->mSourcePath = anImage;
    const char* aTags = AttributeOrNull(theElement, "tags");
    if (aTags != nullptr)
        aLive->mTags = aTags;

    if (strncmp(aLive->mPath.c_str(), "!sys:", 5) == 0)
    {
        aLive->mIsSysFont = true;
        Sexy::String aStripped = aLive->mPath.substr(5);
        aLive->mPathEncoded = aStripped;
        aLive->mPath = aStripped;
        const char* aSizeText = AttributeOrNull(theElement, "size");
        if (aSizeText == nullptr)
            return Fail(Sexy::String("SysFont needs point size"));
        int aSize = atoi(aSizeText);
        aLive->mPointSize = aSize;
        if (aSize <= 0)
            return Fail(Sexy::String("SysFont needs point size"));
        aLive->mBold = theElement.GetAttributeBool(Sexy::String("bold"), false) != 0;
        aLive->mItalic = theElement.GetAttributeBool(Sexy::String("italic"), false) != 0;
        aLive->mShadow = theElement.GetAttributeBool(Sexy::String("shadow"), false) != 0;
        aLive->mUnderline = theElement.GetAttributeBool(Sexy::String("underline"), false) != 0;
    }
    else
    {
        aLive->mIsSysFont = false;
    }
    aLive->ApplyConfig();
    return true;
}

bool Sexy::ResourceManager::ParsePopAnimResource(Sexy::XMLElement& theElement)
{
    PopAnimRes* aNew = new PopAnimRes();
    if (!ParseCommonResource(theElement, aNew, mResMaps[ResType_Music]))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        PopAnimRes* aLive = static_cast<PopAnimRes*>(mResMaps[ResType_Music][aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        return aLive != nullptr;
    }
    return true;
}

bool Sexy::ResourceManager::ParsePIEffectResource(Sexy::XMLElement& theElement)
{
    PIEffectRes* aNew = new PIEffectRes();
    if (!ParseCommonResource(theElement, aNew, mResMaps[ResType_Reanim]))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        PIEffectRes* aLive = static_cast<PIEffectRes*>(mResMaps[ResType_Reanim][aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        return aLive != nullptr;
    }
    return true;
}

bool Sexy::ResourceManager::ParseRenderEffectResource(Sexy::XMLElement& theElement)
{
    RenderEffectRes* aNew = new RenderEffectRes();
    RenderEffectRes* aLive = aNew;
    if (!ParseCommonResource(theElement, aNew, mResMaps[ResType_Particle]))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        aLive = static_cast<RenderEffectRes*>(mResMaps[ResType_Particle][aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        if (aLive == nullptr)
            return false;
    }
    aLive->mSourcePath.clear();
    const char* aSource = AttributeOrNull(theElement, "srcpath");
    if (aSource != nullptr && aSource[0] != '\0')
    {
        Sexy::String aJoined = mPathPrefix;
        aJoined += aSource;
        aLive->mSourcePath = aJoined;
    }
    return true;
}

bool Sexy::ResourceManager::ParseGenericResFileResource(Sexy::XMLElement& theElement)
{
    GenericResFileRes* aNew = new GenericResFileRes();
    if (!ParseCommonResource(theElement, aNew, mResMaps[ResType_Trail]))
    {
        if (!mReuseLoadedResources)
        {
            delete aNew;
            return false;
        }
        mErrorText.clear();
        mHasError = false;
        GenericResFileRes* aLive =
            static_cast<GenericResFileRes*>(mResMaps[ResType_Trail][aNew->mId]);
        if (aLive != nullptr)
        {
            aLive->mPath = aNew->mPath;
            aLive->mPathEncoded = aNew->mPathEncoded;
            aLive->mAttributes = aNew->mAttributes;
            aLive->mGeneration = mResGroupGeneration;
        }
        delete aNew;
        return aLive != nullptr;
    }
    return true;
}

bool Sexy::ResourceManager::ParseResources()
{
    for (;;)
    {
        Sexy::XMLElement aElement;
        if (mXMLParser->NextElement(&aElement) == 0)
            return false;
        if (aElement.mType == 2)
            return true;
        if (aElement.mType == 3)
        {
            if (aElement.mInstruction.empty())
                continue;
            return Fail(Sexy::String("Element Not Expected '") + aElement.mInstruction + Sexy::String("'"));
        }
        if (aElement.mType != 1)
            continue;

        bool aParsed = false;
        if (aElement.mInstruction == "SetDefaults")
            aParsed = ParseSetDefaults(aElement);
        else if (aElement.mInstruction == "Image")
            aParsed = ParseImageResource(aElement);
        else if (aElement.mInstruction == "Sound")
            aParsed = ParseSoundResource(aElement);
        else if (aElement.mInstruction == "Font")
            aParsed = ParseFontResource(aElement);
        else if (aElement.mInstruction == "PopAnim")
            aParsed = ParsePopAnimResource(aElement);
        else if (aElement.mInstruction == "PIEffect")
            aParsed = ParsePIEffectResource(aElement);
        else if (aElement.mInstruction == "RenderEffect")
            aParsed = ParseRenderEffectResource(aElement);
        else if (aElement.mInstruction == "File")
            aParsed = ParseGenericResFileResource(aElement);
        else
            return Fail(Sexy::String("Invalid Section '") + aElement.mInstruction + Sexy::String("'"));

        if (!aParsed)
            return false;

        Sexy::XMLElement aEnd;
        if (mXMLParser->NextElement(&aEnd) == 0 || aEnd.mType != 2)
            return Fail(Sexy::String("Unexpected element found."));
    }
}

bool Sexy::ResourceManager::DoParseResources()
{
    while (!mHasError && mXMLParser->HasFailed() == 0)
    {
        Sexy::XMLElement aElement;
        if (mXMLParser->NextElement(&aElement) == 0)
            break;
        if (aElement.mType != 1 || aElement.mInstruction.empty())
            continue;

        if (aElement.mInstruction == "Resources")
        {
            Sexy::String aId = aElement.GetAttribute("id");
            if (aId.empty())
                return Fail(Sexy::String("No id specified."));
            mCurResGroupName = aId;
            mCurResGroupList = &GroupList(aId);
            mParentResGroupName = aElement.GetAttribute("parent");
            Sexy::String aRes = aElement.GetAttribute("res");
            mCurResGroupResId = aRes.empty() ? 0 : atoi(aRes.c_str());
            mCurResGroupLocale = LocaleFromText(aElement.GetAttribute("loc"));
            if (!ParseResources())
                return false;
            continue;
        }

        if (aElement.mInstruction != "CompositeResources")
            return Fail(Sexy::String("Invalid Section '") + aElement.mInstruction + Sexy::String("'"));

        Sexy::String aId = aElement.GetAttribute("id");
        if (aId.empty())
            return Fail(Sexy::String("No id specified on CompositeGroup."));

        CompositeResGroup& aComposite = mCompositeResGroupMap[aId];
        aComposite.mSubGroups.clear();
        for (;;)
        {
            Sexy::XMLElement aChild;
            if (mXMLParser->NextElement(&aChild) == 0)
                return false;
            if (aChild.mType == 2)
                break;
            if (aChild.mType != 1)
                continue;
            if (aChild.mInstruction != "Group" && aChild.mInstruction != "Resources")
                return Fail(Sexy::String("Invalid Section '") + aChild.mInstruction + Sexy::String("'"));
            SubGroup aSub;
            aSub.mName = aChild.GetAttribute("id");
            Sexy::String aScale = aChild.GetAttribute("res");
            aSub.mScale = aScale.empty() ? 0 : atoi(aScale.c_str());
            aSub.mLocale = LocaleFromText(aChild.GetAttribute("loc"));
            aComposite.mSubGroups.push_back(aSub);
            Sexy::XMLElement aEnd;
            if (mXMLParser->NextElement(&aEnd) == 0 || aEnd.mType != 2)
                return Fail(Sexy::String("Unexpected element found."));
        }
    }
    return !mHasError;
}

bool Sexy::ResourceManager::ParseResourcesFile(Sexy::String const& theFileName)
{
    mResFilePath = theFileName;
    delete mXMLParser;
    mXMLParser = new Sexy::XMLParser();

    Sexy::String aDirectory = FileDir(theFileName);
    Sexy::String aName = FileStem(theFileName);
    Sexy::String aBinary = aDirectory;
    if (!aBinary.empty())
        aBinary += "/";
    aBinary += aName;
    aBinary += ".bxml";

    bool aOpened = mXMLParser->OpenBinaryFile(aBinary) != 0;
    if (!aOpened)
    {
        Sexy::Buffer aBuffer;
        if (mApp == nullptr || mApp->ReadBufferFromFile(theFileName, &aBuffer, false) == 0)
            return Fail(Sexy::String("Resource file not found: ") + theFileName);

        int aByteCount = aBuffer.mDataBitSize / 8;
        const char* aBytes = aBuffer.mData.empty() ? "" : (const char*)&aBuffer.mData[0];
        Sexy::String aSource(aBytes, (unsigned int)aByteCount);
        mXMLParser->SetStringSource(aSource);
        aOpened = mXMLParser->HasFailed() == 0;
    }

    if (!aOpened)
    {
        Sexy::String aError = mXMLParser->GetErrorText();
        if (!aError.empty())
            Fail(aError);
        return false;
    }

    bool aFoundManifest = false;
    while (mXMLParser->HasFailed() == 0)
    {
        Sexy::XMLElement aElement;
        if (mXMLParser->NextElement(&aElement) == 0)
        {
            Sexy::String aError = mXMLParser->GetErrorText();
            if (!aError.empty())
                Fail(aError);
            break;
        }
        if (aElement.mType != 1)
            continue;
        if (aElement.mInstruction != "ResourceManifest")
            break;

        aFoundManifest = true;
        Sexy::String aVersion = aElement.GetAttribute("version");
        int aManifestVersion = aVersion.empty() ? 1 : atoi(aVersion.c_str());
        mManifestVersion = aManifestVersion;
        if ((unsigned int)(aManifestVersion - 1) > 1u)
        {
            return Fail(Sexy::String("Expecting ResourceManifest tag with version 1 or 2; "
                                     "please make sure your version of ResourceGen3 is current"));
        }
        break;
    }

    if (!aFoundManifest)
        return Fail(Sexy::String("Expecting ResourceManifest tag"));
    return DoParseResources();
}

bool Sexy::ResourceManager::ReparseResourcesFile(Sexy::String const& theFileName)
{
    bool aWasReparsing = mIsReparsing;
    mIsReparsing = true;
    ++mResGroupGeneration;
    bool aParsed = ParseResourcesFile(theFileName);

    for (int aKind = 0; aKind < 7; ++aKind)
    {
        for (ResMap::iterator anEntry = mResMaps[aKind].begin();
             anEntry != mResMaps[aKind].end(); ++anEntry)
        {
            BaseRes* aRecord = anEntry->second;
            if (aRecord != nullptr && aRecord->mGeneration != mResGroupGeneration)
                aRecord->DeleteResource();
        }
    }
    mIsReparsing = aWasReparsing;
    return aParsed;
}

// ---------------------------------------------------------------------------
// Load. A program resource is skipped. A "!ref:" path copies another record.
// Everything else asks the app for the file and publishes the result.
// ---------------------------------------------------------------------------

bool Sexy::ResourceManager::DoLoadImage(ImageRes* theRes)
{
    const char* aPath = theRes->mPath.c_str();
    if (aPath == nullptr || aPath[0] == '\0')
        return false;

    if (strncmp(aPath, "!ref:", 5) == 0)
    {
        Sexy::String aRefId;
        aRefId = aPath + 5;
        Sexy::SharedImageRef aRef = GetImage(aRefId);
        if (aRef.mImage == nullptr)
            return Fail(Sexy::String("Ref Image not found: ") + aRefId);
        theRes->mImage = aRef;
        Publish(theRes, aRef.mImage);
        return true;
    }

    Sexy::String aVariant = theRes->mVariant;
    bool aIsNew = false;
    Sexy::SharedImageRef aLoaded;
    if (mApp != nullptr)
    {
        Sexy::String aAttrs;
        aLoaded = mApp->GetSharedImage(theRes->mPath, aVariant, &aIsNew, true, false);
    }
    if (aLoaded.mImage == nullptr)
        return Fail(Sexy::String("Failed to load image: ") + theRes->mPath);

    theRes->mImage = aLoaded;
    if (aLoaded.mImage != nullptr)
    {
        aLoaded.mImage->mNumRows = theRes->mRows > 0 ? theRes->mRows : 1;
        aLoaded.mImage->mNumCols = theRes->mCols > 0 ? theRes->mCols : 1;
    }
    if (!theRes->mAlphaImagePath.empty() && aLoaded.mImage != nullptr)
        LoadAlphaImage(theRes, aLoaded.mImage->AsMemoryImage());
    if (!theRes->mAlphaGridPath.empty() && aLoaded.mImage != nullptr)
        LoadAlphaGridImage(theRes, aLoaded.mImage->AsMemoryImage());
    ApplyImageFlags(theRes);
    Publish(theRes, aLoaded.mImage);
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadSound(SoundRes* theRes)
{
    HeldLock aLock(mCritSect);
    const char* aPath = theRes->mPath.c_str();
    if (strncmp(aPath, "!ref:", 5) == 0)
    {
        Sexy::String aRefId;
        aRefId = aPath + 5;
        BaseRes* aRef = GetBaseRes(ResType_Sound, aRefId);
        if (aRef == nullptr)
        {
            aLock.Unlock();
            return Fail(Sexy::String("Ref sound not found: ") + aRefId);
        }
        theRes->mSoundId = static_cast<SoundRes*>(aRef)->mSoundId;
        if (theRes->mResourceRef != nullptr)
            *static_cast<int*>(theRes->mResourceRef) = theRes->mSoundId;
        return true;
    }

    if (mApp == nullptr || mApp->mSoundManager == nullptr)
    {
        aLock.Unlock();
        return Fail(Sexy::String("Failed to load sound: ") + theRes->mPath);
    }

    Sexy::SoundManager* aSounds = mApp->mSoundManager;
    int anId = aSounds->GetFreeSoundId();
    if (anId < 0)
    {
        aLock.Unlock();
        return Fail(Sexy::String("Out of free sound ids"));
    }

    bool aLoaded = aSounds->LoadSound((unsigned int)anId, theRes->mPath);
    if (!aLoaded && !aSounds->IsPlaying((unsigned int)anId))
    {
        aLock.Unlock();
        return Fail(Sexy::String("Failed to load sound: ") + theRes->mPath);
    }

    theRes->mSoundId = anId;
    theRes->ApplyConfig();
    if (theRes->mResourceRef != nullptr)
        *static_cast<int*>(theRes->mResourceRef) = anId;
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::AcceptFont(FontRes* theRes, Sexy::Font* theFont)
{
    if (theFont == nullptr)
        return true;
    Sexy::ImageFont* anImageFont = dynamic_cast<Sexy::ImageFont*>(theFont);
    if (anImageFont == nullptr)
        return true;
    if (anImageFont->mFontData == nullptr || anImageFont->mFontData->mIsListFont == 0)
    {
        delete theFont;
        char aText[512];
        sprintf(aText, "Failed to load font: %s", theRes->mPath.c_str());
        Fail(Sexy::String(aText));
        return false;
    }
    theRes->mFont = theFont;
    theRes->ApplyConfig();
    return true;
}

bool Sexy::ResourceManager::DoLoadFont(FontRes* theRes)
{
    HeldLock aLock(mCritSect);
    Sexy::String aPath = theRes->mPath;

    char aPrefixText[64];
    sprintf(aPrefixText, "fonts\\%d\\", mImageResDir);
    Sexy::String aPrefix;
    aPrefix = aPrefixText;
    if (StartsWithNoCase(theRes->mPath, aPrefix))
    {
        Sexy::String aTail = aPath.substr(aPrefix.size());
        char aRewritten[512];
        sprintf(aRewritten, "fonts\\%d\\%s", mImageResScale, aTail.c_str());
        aPath = aRewritten;
    }
    else
    {
        sprintf(aPrefixText, "fonts/%d/", mImageResDir);
        aPrefix = aPrefixText;
        if (StartsWithNoCase(theRes->mPath, aPrefix))
        {
            Sexy::String aTail = aPath.substr(aPrefix.size());
            char aRewritten[512];
            sprintf(aRewritten, "fonts/%d/%s", mImageResScale, aTail.c_str());
            aPath = aRewritten;
        }
    }

    char aScaleKeyText[32];
    sprintf(aScaleKeyText, "path%d", mImageResScale);
    Sexy::String aScaleKey;
    aScaleKey = aScaleKeyText;
    std::map<Sexy::String, Sexy::String>::iterator aScale = theRes->mAttributes.find(aScaleKey);
    if (aScale != theRes->mAttributes.end())
        aPath = aScale->second;

    if (theRes->mIsSysFont)
    {
        Sexy::String aEmpty;
        Sexy::String aLocalized = TryLocalizedResourcePath(theRes->mPath, aEmpty, aEmpty);
        if (mApp == nullptr || mApp->mPlatformDriverOrQueue == nullptr)
            return false;
        Sexy::Font* aFont = mApp->mPlatformDriverOrQueue->CreateFont(
            mApp, aLocalized, theRes->mPointSize, theRes->mBold, theRes->mItalic, theRes->mUnderline);
        if (aFont == nullptr)
            return false;
        aFont->mFontFlag = false;
        aFont->mIsComposited = theRes->mShadow;
        if (!AcceptFont(theRes, aFont))
            return false;
        theRes->mFont = aFont;
        Publish(theRes, aFont);
        ResourceLoadedHook(theRes);
        return true;
    }

    if (!theRes->mSourcePath.empty())
    {
        Sexy::String aEmpty;
        Sexy::String aLocalized = GetLocalizedResourcePath(theRes->mSourcePath, aEmpty, aEmpty);
        Sexy::Image* anImage = mApp != nullptr ? mApp->GetImage(aLocalized, true) : nullptr;
        if (anImage == nullptr && mApp != nullptr)
            anImage = mApp->GetImage(theRes->mSourcePath, true);
        if (anImage == nullptr)
        {
            char aText[512];
            sprintf(aText, "Failed to load image: %s", theRes->mSourcePath.c_str());
            return Fail(Sexy::String(aText));
        }
        theRes->mImage = anImage;
        Sexy::ImageFont* aMade = new Sexy::ImageFont(anImage, aPath);
        if (!AcceptFont(theRes, aMade))
            return false;
        theRes->mFont = aMade;
        Publish(theRes, aMade);
        ResourceLoadedHook(theRes);
        return true;
    }

    if (strncmp(aPath.c_str(), "!ref:", 5) == 0)
    {
        Sexy::String aId = aPath.substr(5);
        aLock.Unlock();
        Sexy::ResourceRef aRef = GetFontRef(aId);
        aLock.Lock();
        if (theRes->mHeldRef != nullptr)
            delete static_cast<Sexy::ResourceRef*>(theRes->mHeldRef);
        theRes->mHeldRef = new Sexy::ResourceRef(aRef);
        Sexy::Font* aFont = static_cast<Sexy::ResourceRef*>(theRes->mHeldRef)->operator Sexy::Font*();
        if (aFont == nullptr)
            return Fail(Sexy::String("Ref Font not found: ") + aId);
        Sexy::Font* aCopy = aFont->Duplicate();
        if (aCopy != nullptr && !AcceptFont(theRes, aCopy))
            return false;
        theRes->mFont = aCopy;
        Publish(theRes, aCopy);
        ResourceLoadedHook(theRes);
        return true;
    }

    Sexy::String aEmpty;
    Sexy::String aLocalized = TryLocalizedResourcePath(aPath, aEmpty, aEmpty);
    Sexy::ImageFont* aMade = new Sexy::ImageFont(mApp, aLocalized);
    if (!AcceptFont(theRes, aMade))
        return false;
    theRes->mFont = aMade;
    Publish(theRes, aMade);
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadPopAnim(PopAnimRes* theRes)
{
    Sexy::PopAnim* aPop = new Sexy::PopAnim(0, nullptr);
    char aPrefixText[64];
    sprintf(aPrefixText, "images\\%d\\", mImageResDir);
    Sexy::String aPrefix;
    aPrefix = aPrefixText;
    if (mImageResDir != 0 && StartsWithNoCase(theRes->mPath, aPrefix))
    {
        float aScale = (float)mImageResScale / (float)mImageResDir;
        aPop->mScaleX = aScale;
        aPop->mScaleY = aScale;
    }
    sprintf(aPrefixText, "images/%d/", mImageResDir);
    aPrefix = aPrefixText;
    if (mImageResDir != 0 && StartsWithNoCase(theRes->mPath, aPrefix))
    {
        float aScale = (float)mImageResScale / (float)mImageResDir;
        aPop->mScaleX = aScale;
        aPop->mScaleY = aScale;
    }

    aPop->LoadFile(theRes->mPath, false);
    if (!aPop->mErrorString.empty())
    {
        Sexy::String aMessage = Sexy::String("PopAnim loading error: ") + aPop->mErrorString;
        aMessage += " on file ";
        aMessage += theRes->mPath;
        delete aPop;
        return Fail(aMessage);
    }
    theRes->mPopAnim = aPop;
    Publish(theRes, aPop);
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadPIEffect(PIEffectRes* theRes)
{
    Sexy::PIEffect* anEffect = new Sexy::PIEffect();
    anEffect->LoadEffect(theRes->mPath);
    if (!anEffect->mError.empty())
    {
        Sexy::String aMessage = Sexy::String("PIEffect loading error: ") + anEffect->mError;
        aMessage += " on file ";
        aMessage += theRes->mPath;
        delete anEffect;
        return Fail(aMessage);
    }
    theRes->mPIEffect = anEffect;
    Publish(theRes, anEffect);
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadRenderEffect(RenderEffectRes* theRes)
{
    Sexy::RenderEffectDefinition* anEffect = new Sexy::RenderEffectDefinition();
    int aLoaded = anEffect->LoadFromFile(theRes->mPath.c_str(), theRes->mSourcePath.c_str());
    if (aLoaded == 0)
    {
        Sexy::String aMessage = Sexy::String("RenderEffect loading error on file ");
        aMessage += theRes->mPath;
        delete anEffect;
        return Fail(aMessage);
    }
    theRes->mRenderEffect = anEffect;
    Publish(theRes, anEffect);
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadGenericResFile(GenericResFileRes* theRes)
{
    theRes->mFileBody = theRes->mPath;
    ResourceLoadedHook(theRes);
    return true;
}

bool Sexy::ResourceManager::DoLoadResource(BaseRes* theRes, bool* theKeepGoing)
{
    if (theKeepGoing != nullptr)
        *theKeepGoing = false;
    if (theRes == nullptr)
        return false;
    if (theRes->mFromProgram)
    {
        if (theKeepGoing != nullptr)
            *theKeepGoing = true;
        return true;
    }

    bool aLoaded = false;
    switch (theRes->mResType)
    {
    case ResType_Image:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_Image)");
        aLoaded = DoLoadImage(static_cast<ImageRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_Image)");
        break;
    case ResType_Sound:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_Sound)");
        aLoaded = DoLoadSound(static_cast<SoundRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_Sound)");
        break;
    case ResType_Font:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_Font)");
        aLoaded = DoLoadFont(static_cast<FontRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_Font)");
        break;
    case ResType_Music:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_PopAnim)");
        aLoaded = DoLoadPopAnim(static_cast<PopAnimRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_PopAnim)");
        break;
    case ResType_Reanim:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_PIEffect)");
        aLoaded = DoLoadPIEffect(static_cast<PIEffectRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_PIEffect)");
        break;
    case ResType_Particle:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_RenderEffect)");
        aLoaded = DoLoadRenderEffect(static_cast<RenderEffectRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_RenderEffect)");
        break;
    case ResType_Trail:
        SexyPerf::StartTiming("ResourceManager::DoLoadResource(ResType_GenericResFile)");
        aLoaded = DoLoadGenericResFile(static_cast<GenericResFileRes*>(theRes));
        SexyPerf::StopTiming("ResourceManager::DoLoadResource(ResType_GenericResFile)");
        break;
    default:
        break;
    }
    return aLoaded;
}

bool Sexy::ResourceManager::StartLoadResources(Sexy::String const& theGroup, bool theFromComposite)
{
    (void)theFromComposite;
    mErrorText.clear();
    mHasError = false;
    if (mParentResGroupName.empty())
        mCompositeResGroupIndex = 0;
    mCurResGroupName = theGroup;
    mCurResGroupList = &GroupList(theGroup);
    mCurResGroupItr = mCurResGroupList->begin();
    return true;
}

bool Sexy::ResourceManager::LoadNextResource()
{
    if (mHasError || mCurResGroupList == nullptr || (mApp != nullptr && mApp->mShutDown))
        return false;

    while (mCurResGroupItr != mCurResGroupList->end())
    {
        BaseRes* aRecord = *mCurResGroupItr;
        ++mCurResGroupItr;
        if (aRecord == nullptr)
            continue;

        if (aRecord->mRefCount != 0)
        {
            ++aRecord->mRefCount;
            aRecord->mRefHeld = true;
            return true;
        }

        bool aKeepGoing = true;
        bool aLoaded = DoLoadResource(aRecord, &aKeepGoing);
        aRecord->mRefHeld = true;
        ++aRecord->mRefCount;
        if (!aKeepGoing)
            return aLoaded;
    }

    if (!mParentResGroupName.empty())
    {
        std::map<Sexy::String, CompositeResGroup, StringLessNoCase>::iterator aFound =
            mCompositeResGroupMap.find(mParentResGroupName);
        if (aFound != mCompositeResGroupMap.end())
        {
            std::vector<SubGroup>& aSubs = aFound->second.mSubGroups;
            int aIndex = mCompositeResGroupIndex + 1;
            for (; aIndex < (int)aSubs.size(); ++aIndex)
            {
                SubGroup& aSub = aSubs[(size_t)aIndex];
                if (aSub.mName.empty())
                    continue;
                if (aSub.mScale != 0 && aSub.mScale != mImageResScale)
                    continue;
                if (aSub.mLocale != 0 && aSub.mLocale != mLocaleCode)
                    continue;
                if (!mCurResGroupName.empty())
                    mLoadedGroups.insert(mCurResGroupName);
                mCompositeResGroupIndex = aIndex;
                StartLoadResources(aSub.mName, true);
                return LoadNextResource();
            }
        }
    }

    if (!mCurResGroupName.empty())
        mLoadedGroups.insert(mCurResGroupName);
    if (!mParentResGroupName.empty())
        mLoadedGroups.insert(mParentResGroupName);
    return false;
}

bool Sexy::ResourceManager::LoadResources(Sexy::String const& theGroup)
{
    mErrorText.clear();
    mHasError = false;
    StartLoadResources(theGroup, false);
    while (LoadNextResource())
    {
    }
    return !mHasError;
}

void Sexy::ResourceManager::DeleteResources(ResMap& theMap, Sexy::String const& theGroup)
{
    if (theGroup.empty())
    {
        for (ResMap::iterator anEntry = theMap.begin(); anEntry != theMap.end(); ++anEntry)
        {
            BaseRes* aRecord = anEntry->second;
            if (aRecord != nullptr && aRecord->mRefHeld)
            {
                aRecord->mRefHeld = false;
                Deref(aRecord);
            }
        }
        return;
    }

    std::map<Sexy::String, CompositeResGroup, StringLessNoCase>::iterator aComposite =
        mCompositeResGroupMap.find(theGroup);
    if (aComposite == mCompositeResGroupMap.end())
    {
        for (ResMap::iterator anEntry = theMap.begin(); anEntry != theMap.end(); ++anEntry)
        {
            BaseRes* aRecord = anEntry->second;
            if (aRecord != nullptr && aRecord->mGroup == theGroup && aRecord->mRefHeld)
            {
                aRecord->mRefHeld = false;
                Deref(aRecord);
            }
        }
        return;
    }

    for (size_t i = 0; i < aComposite->second.mSubGroups.size(); ++i)
    {
        Sexy::String const& aSubName = aComposite->second.mSubGroups[i].mName;
        if (aSubName.empty())
            continue;
        for (ResMap::iterator anEntry = theMap.begin(); anEntry != theMap.end(); ++anEntry)
        {
            BaseRes* aRecord = anEntry->second;
            if (aRecord != nullptr && aRecord->mGroup == aSubName && aRecord->mRefHeld)
            {
                aRecord->mRefHeld = false;
                Deref(aRecord);
            }
        }
    }
}

void Sexy::ResourceManager::DeleteResources(Sexy::String const& theGroup)
{
    for (int aKind = 0; aKind < 7; ++aKind)
        DeleteResources(mResMaps[aKind], theGroup);
    mLoadedGroups.erase(theGroup);
}

void Sexy::ResourceManager::DeleteExtraImageBuffers(Sexy::String const& theGroup)
{
    for (ResMap::iterator anEntry = mResMaps[ResType_Image].begin();
         anEntry != mResMaps[ResType_Image].end(); ++anEntry)
    {
        ImageRes* aRecord = static_cast<ImageRes*>(anEntry->second);
        if (aRecord == nullptr)
            continue;
        if (!theGroup.empty() && aRecord->mGroup != theGroup)
            continue;
        Sexy::MemoryImage* anImage = aRecord->mImage;
        if (anImage != nullptr)
            anImage->DeleteExtraBuffers();
    }
}

void Sexy::ResourceManager::DeleteImage(Sexy::String const& theId)
{
    BaseRes* aRes = GetBaseRes(ResType_Image, theId);
    if (aRes == nullptr)
        return;
    aRes->mRefHeld = false;
    Deref(aRes);
}

Sexy::SharedImageRef Sexy::ResourceManager::LoadImage(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    ImageRes* aRes = static_cast<ImageRes*>(GetBaseRes(ResType_Image, theId));
    Sexy::SharedImageRef aResult;
    if (aRes == nullptr)
        return aResult;
    Hold(aRes);
    if (aRes->mImage.mImage == nullptr)
    {
        if (aRes->mFromProgram)
            return aResult;
        aLock.Unlock();
        bool aLoaded = DoLoadImage(aRes);
        aLock.Lock();
        if (aLoaded)
            aResult = aRes->mImage;
    }
    else
    {
        aResult = aRes->mImage;
    }
    return aResult;
}

int const* Sexy::ResourceManager::GetImageOffset(Sexy::String const& theId)
{
    static int aMissing[2] = { 0, 0 };
    ImageRes* aRes = static_cast<ImageRes*>(GetBaseRes(ResType_Image, theId));
    if (aRes == nullptr)
        return aMissing;
    return &aRes->mAlphaX;
}

Sexy::Font* Sexy::ResourceManager::LoadFont(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    FontRes* aRes = static_cast<FontRes*>(GetBaseRes(ResType_Font, theId));
    if (aRes == nullptr)
        return nullptr;
    Hold(aRes);
    if (aRes->mFont == nullptr && !aRes->mFromProgram)
    {
        aLock.Unlock();
        bool aLoaded = DoLoadFont(aRes);
        aLock.Lock();
        if (!aLoaded)
            return nullptr;
    }
    return aRes->mFont;
}

int Sexy::ResourceManager::LoadSound(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    SoundRes* aRes = static_cast<SoundRes*>(GetBaseRes(ResType_Sound, theId));
    if (aRes == nullptr)
        return -1;
    Hold(aRes);
    if (aRes->mSoundId == 0)
    {
        if (aRes->mFromProgram)
            return -1;
        aLock.Unlock();
        bool aLoaded = DoLoadSound(aRes);
        aLock.Lock();
        return aLoaded ? aRes->mSoundId : -1;
    }
    return aRes->mSoundId;
}

Sexy::PopAnim* Sexy::ResourceManager::LoadPopAnim(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    PopAnimRes* aRes = static_cast<PopAnimRes*>(GetBaseRes(ResType_Music, theId));
    if (aRes == nullptr)
        return nullptr;
    Hold(aRes);
    if (aRes->mPopAnim == nullptr && !aRes->mFromProgram)
    {
        aLock.Unlock();
        bool aLoaded = DoLoadPopAnim(aRes);
        aLock.Lock();
        if (!aLoaded)
            return nullptr;
    }
    return aRes->mPopAnim;
}

Sexy::PIEffect* Sexy::ResourceManager::LoadPIEffect(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    PIEffectRes* aRes = static_cast<PIEffectRes*>(GetBaseRes(ResType_Reanim, theId));
    if (aRes == nullptr)
        return nullptr;
    Hold(aRes);
    if (aRes->mPIEffect == nullptr && !aRes->mFromProgram)
    {
        aLock.Unlock();
        bool aLoaded = DoLoadPIEffect(aRes);
        aLock.Lock();
        if (!aLoaded)
            return nullptr;
    }
    return aRes->mPIEffect;
}

Sexy::RenderEffectDefinition* Sexy::ResourceManager::LoadRenderEffect(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    RenderEffectRes* aRes = static_cast<RenderEffectRes*>(GetBaseRes(ResType_Particle, theId));
    if (aRes == nullptr)
        return nullptr;
    Hold(aRes);
    if (aRes->mRenderEffect == nullptr && !aRes->mFromProgram)
    {
        aLock.Unlock();
        bool aLoaded = DoLoadRenderEffect(aRes);
        aLock.Lock();
        if (!aLoaded)
            return nullptr;
    }
    return aRes->mRenderEffect;
}

Sexy::GenericResFile* Sexy::ResourceManager::LoadGenericResFile(Sexy::String const& theId)
{
    HeldLock aLock(mCritSect);
    GenericResFileRes* aRes = static_cast<GenericResFileRes*>(GetBaseRes(ResType_Trail, theId));
    if (aRes == nullptr)
        return nullptr;
    Hold(aRes);
    if (aRes->mGenericResFile == nullptr && !aRes->mFromProgram)
    {
        aLock.Unlock();
        bool aLoaded = DoLoadGenericResFile(aRes);
        aLock.Lock();
        if (!aLoaded)
            return nullptr;
    }
    return aRes->mGenericResFile;
}

Sexy::SharedImageRef Sexy::ResourceManager::GetImage(Sexy::String const& theId)
{
    ImageRes* aRes = static_cast<ImageRes*>(GetBaseRes(ResType_Image, theId));
    if (aRes == nullptr)
        return Sexy::SharedImageRef();
    return aRes->mImage;
}

int Sexy::ResourceManager::GetSound(Sexy::String const& theId)
{
    SoundRes* aRes = static_cast<SoundRes*>(GetBaseRes(ResType_Sound, theId));
    if (aRes == nullptr)
        return -1;
    return aRes->mSoundId;
}

Sexy::Font* Sexy::ResourceManager::GetFont(Sexy::String const& theId)
{
    FontRes* aRes = static_cast<FontRes*>(GetBaseRes(ResType_Font, theId));
    if (aRes == nullptr)
        return nullptr;
    return aRes->mFont;
}

bool Sexy::ResourceManager::IsResourceLoaded(Sexy::String const& theId)
{
    Sexy::SharedImageRef anImage = GetImage(theId);
    if (anImage.mImage != nullptr)
        return true;
    if (GetFont(theId) != nullptr)
        return true;
    return GetSound(theId) != -1;
}

Sexy::PopAnim* Sexy::ResourceManager::GetPopAnim(Sexy::String const& theId)
{
    PopAnimRes* aRes = static_cast<PopAnimRes*>(GetBaseRes(ResType_Music, theId));
    if (aRes == nullptr)
        return nullptr;
    return aRes->mPopAnim;
}

Sexy::PIEffect* Sexy::ResourceManager::GetPIEffect(Sexy::String const& theId)
{
    PIEffectRes* aRes = static_cast<PIEffectRes*>(GetBaseRes(ResType_Reanim, theId));
    if (aRes == nullptr)
        return nullptr;
    return aRes->mPIEffect;
}

Sexy::RenderEffectDefinition* Sexy::ResourceManager::GetRenderEffect(Sexy::String const& theId)
{
    RenderEffectRes* aRes = static_cast<RenderEffectRes*>(GetBaseRes(ResType_Particle, theId));
    if (aRes == nullptr)
        return nullptr;
    return aRes->mRenderEffect;
}

Sexy::GenericResFile* Sexy::ResourceManager::GetGenericResFile(Sexy::String const& theId)
{
    GenericResFileRes* aRes = static_cast<GenericResFileRes*>(GetBaseRes(ResType_Trail, theId));
    if (aRes == nullptr)
        return nullptr;
    return aRes->mGenericResFile;
}

Sexy::SharedImageRef Sexy::ResourceManager::GetImageThrow(Sexy::String const& theId, int theScale, bool theRequired)
{
    if (gSexyAppBase != nullptr && gSexyAppBase->mShutDown)
        return Sexy::SharedImageRef();
    if (theScale != 0 && mImageResScale != theScale)
    {
        char aText[128];
        sprintf(aText, "Image scale mismatch for %s", theId.c_str());
        Fail(Sexy::String(aText));
        throw Sexy::ResourceManagerException(mErrorText);
    }
    Sexy::SharedImageRef anImage = GetImage(theId);
    if (anImage.mImage == nullptr && theRequired)
    {
        Fail(Sexy::String("Image not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return anImage;
}

Sexy::SharedImageRef Sexy::ResourceManager::GetImageThrow(Sexy::String const& theId, bool theRequired)
{
    return GetImageThrow(theId, 0, theRequired);
}

int Sexy::ResourceManager::GetSoundThrow(Sexy::String const& theId)
{
    int aSound = GetSound(theId);
    if (aSound == -1)
    {
        Fail(Sexy::String("Sound not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return aSound;
}

Sexy::Font* Sexy::ResourceManager::GetFontThrow(Sexy::String const& theId, int theScale)
{
    if (theScale != 0 && mImageResScale != theScale)
    {
        Fail(Sexy::String("Font scale mismatch: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    Sexy::Font* aFont = GetFont(theId);
    if (aFont == nullptr)
    {
        Fail(Sexy::String("Font not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return aFont;
}

Sexy::PopAnim* Sexy::ResourceManager::GetPopAnimThrow(Sexy::String const& theId)
{
    Sexy::PopAnim* anAnim = GetPopAnim(theId);
    if (anAnim == nullptr)
    {
        Fail(Sexy::String("PopAnim not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return anAnim;
}

Sexy::PIEffect* Sexy::ResourceManager::GetPIEffectThrow(Sexy::String const& theId)
{
    Sexy::PIEffect* anEffect = GetPIEffect(theId);
    if (anEffect == nullptr)
    {
        Fail(Sexy::String("PIEffect not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return anEffect;
}

Sexy::RenderEffectDefinition* Sexy::ResourceManager::GetRenderEffectThrow(Sexy::String const& theId)
{
    Sexy::RenderEffectDefinition* anEffect = GetRenderEffect(theId);
    if (anEffect == nullptr)
    {
        Fail(Sexy::String("RenderEffect not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return anEffect;
}

Sexy::GenericResFile* Sexy::ResourceManager::GetGenericResFileThrow(Sexy::String const& theId)
{
    Sexy::GenericResFile* aFile = GetGenericResFile(theId);
    if (aFile == nullptr)
    {
        Fail(Sexy::String("GenericResFile not found: ") + theId);
        throw Sexy::ResourceManagerException(mErrorText);
    }
    return aFile;
}

Sexy::ResourceRef Sexy::ResourceManager::GetResourceRef(BaseRes* theRes)
{
    Sexy::ResourceRef aRef;
    if (theRes == nullptr)
        return aRef;
    if (theRes->mRefCount == 0 && !theRes->mFromProgram)
    {
        bool aKeepGoing = false;
        DoLoadResource(theRes, &aKeepGoing);
    }
    ++theRes->mRefCount;
    aRef.mBaseRes = theRes;
    return aRef;
}

Sexy::ResourceRef Sexy::ResourceManager::GetResourceRef(void* theSlot)
{
    std::map<void*, BaseRes*>::iterator aFound = mGlobalPtrMap.find(theSlot);
    if (aFound == mGlobalPtrMap.end())
        return Sexy::ResourceRef();
    return GetResourceRef(aFound->second);
}

void Sexy::ResourceManager::GetImageRef(Sexy::Image*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetSoundRef(int& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetFontRef(Sexy::ImageFont*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetPopAnimRef(Sexy::PopAnim*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetPIEffectRef(Sexy::PIEffect*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetRenderEffectRef(Sexy::RenderEffectDefinition*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

void Sexy::ResourceManager::GetGenericResFileRef(Sexy::GenericResFile*& theSlot)
{
    GetResourceRef(static_cast<void*>(&theSlot));
}

Sexy::ResourceRef Sexy::ResourceManager::GetResourceRef(int theType, Sexy::String const& theId)
{
    return GetResourceRef(GetBaseRes(theType, theId));
}

Sexy::ResourceRef Sexy::ResourceManager::GetImageRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Image, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetSoundRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Sound, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetFontRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Font, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetPopAnimRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Music, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetPIEffectRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Reanim, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetRenderEffectRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Particle, theId);
}

Sexy::ResourceRef Sexy::ResourceManager::GetGenericResFileRef(Sexy::String const& theId)
{
    return GetResourceRef(ResType_Trail, theId);
}

bool Sexy::ResourceManager::ReplaceImage(Sexy::String const& theId, Sexy::Image* theImage)
{
    ImageRes* aRes = static_cast<ImageRes*>(GetBaseRes(ResType_Image, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    if (theImage == nullptr)
        return true;
    Sexy::SharedImage* aShared = new Sexy::SharedImage();
    aShared->mImage = theImage;
    aShared->mRefCount = 1;
    aRes->mImage = Sexy::SharedImageRef(aShared);
    aRes->mImage.mImage = theImage;
    Publish(aRes, theImage);
    return true;
}

bool Sexy::ResourceManager::ReplaceSound(Sexy::String const& theId, int theSoundId)
{
    SoundRes* aRes = static_cast<SoundRes*>(GetBaseRes(ResType_Sound, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mSoundId = theSoundId;
    if (aRes->mResourceRef != nullptr)
        *static_cast<int*>(aRes->mResourceRef) = theSoundId;
    return true;
}

bool Sexy::ResourceManager::DeleteSound(Sexy::String const& theId)
{
    return ReplaceSound(theId, -1);
}

bool Sexy::ResourceManager::ReplaceFont(Sexy::String const& theId, Sexy::Font* theFont)
{
    FontRes* aRes = static_cast<FontRes*>(GetBaseRes(ResType_Font, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mFont = theFont;
    Publish(aRes, theFont);
    return true;
}

bool Sexy::ResourceManager::DeleteFont(Sexy::String const& theId)
{
    return ReplaceFont(theId, nullptr);
}

bool Sexy::ResourceManager::ReplacePopAnim(Sexy::String const& theId, Sexy::PopAnim* theAnim)
{
    PopAnimRes* aRes = static_cast<PopAnimRes*>(GetBaseRes(ResType_Music, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mPopAnim = theAnim;
    Publish(aRes, theAnim);
    return true;
}

bool Sexy::ResourceManager::DeletePopAnim(Sexy::String const& theId)
{
    return ReplacePopAnim(theId, nullptr);
}

bool Sexy::ResourceManager::ReplacePIEffect(Sexy::String const& theId, Sexy::PIEffect* theEffect)
{
    PIEffectRes* aRes = static_cast<PIEffectRes*>(GetBaseRes(ResType_Reanim, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mPIEffect = theEffect;
    Publish(aRes, theEffect);
    return true;
}

bool Sexy::ResourceManager::DeletePIEffect(Sexy::String const& theId)
{
    return ReplacePIEffect(theId, nullptr);
}

bool Sexy::ResourceManager::ReplaceRenderEffect(Sexy::String const& theId, Sexy::RenderEffectDefinition* theEffect)
{
    RenderEffectRes* aRes = static_cast<RenderEffectRes*>(GetBaseRes(ResType_Particle, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mRenderEffect = theEffect;
    Publish(aRes, theEffect);
    return true;
}

bool Sexy::ResourceManager::DeleteRenderEffect(Sexy::String const& theId)
{
    return ReplaceRenderEffect(theId, nullptr);
}

bool Sexy::ResourceManager::ReplaceGenericResFile(Sexy::String const& theId, Sexy::GenericResFile* theFile)
{
    GenericResFileRes* aRes = static_cast<GenericResFileRes*>(GetBaseRes(ResType_Trail, theId));
    if (aRes == nullptr)
        return false;
    aRes->DeleteResource();
    aRes->mGenericResFile = theFile;
    Publish(aRes, theFile);
    return true;
}

bool Sexy::ResourceManager::DeleteGenericResFile(Sexy::String const& theId)
{
    return ReplaceGenericResFile(theId, nullptr);
}

Sexy::ResourceManager::AttrMap const& Sexy::ResourceManager::GetImageAttributes(Sexy::String const& theId)
{
    static AttrMap aMissing;
    ImageRes* aRes = static_cast<ImageRes*>(GetBaseRes(ResType_Image, theId));
    if (aRes == nullptr)
        return aMissing;
    return aRes->mAttributes;
}

Sexy::String Sexy::ResourceManager::GetIdByPath(Sexy::String const& thePath)
{
    Sexy::String aUpper = UpperCopy(thePath);
    for (unsigned int i = 0; i < aUpper.size(); ++i)
    {
        if (aUpper[i] == '/')
            aUpper[i] = '\\';
    }
    ResMap::iterator aFound = mResMapByPath.find(aUpper);
    if (aFound == mResMapByPath.end() || aFound->second == nullptr)
        return Sexy::String();
    return aFound->second->mId;
}

Sexy::String Sexy::ResourceManager::GetIdByPath(Sexy::String const& thePath, int theType)
{
    if ((unsigned int)theType > 6u)
        return Sexy::String();
    return GetIdByPath(thePath);
}

Sexy::String Sexy::ResourceManager::GetImageIdByPath(Sexy::String const& thePath)
{
    return GetIdByPath(thePath, ResType_Image);
}

Sexy::String Sexy::ResourceManager::GetSoundIdByPath(Sexy::String const& thePath)
{
    return GetIdByPath(thePath, ResType_Sound);
}

Sexy::String Sexy::ResourceManager::GetFontIdByPath(Sexy::String const& thePath)
{
    return GetIdByPath(thePath, ResType_Font);
}

Sexy::ResourceRef Sexy::ResourceManager::GetResourceRefFromPath(Sexy::String const& thePath)
{
    Sexy::String aUpper = UpperCopy(thePath);
    for (unsigned int i = 0; i < aUpper.size(); ++i)
    {
        if (aUpper[i] == '/')
            aUpper[i] = '\\';
    }
    ResMap::iterator aFound = mResMapByPath.find(aUpper);
    if (aFound == mResMapByPath.end())
        return Sexy::ResourceRef();
    return GetResourceRef(aFound->second);
}

void* Sexy::ResourceManager::RegisterGlobalPtr(Sexy::String const& theId, void* theSlot)
{
    BaseRes* aRes = nullptr;
    for (int aKind = 0; aKind < 7 && aRes == nullptr; ++aKind)
        aRes = GetBaseRes(aKind, theId);
    if (aRes != nullptr)
    {
        aRes->mResourceRef = theSlot;
        mGlobalPtrMap[theSlot] = aRes;
    }
    return theSlot;
}

Sexy::String Sexy::ResourceManager::GetLocalizedResourcePath(Sexy::String const& thePath,
                                                            Sexy::String const& theDir,
                                                            Sexy::String const& theLocale)
{
    Sexy::String aDir;
    if (!theDir.empty())
    {
        aDir = theDir;
        aDir += "/";
    }
    else
    {
        aDir = "./";
    }

    Sexy::String aLocale = theLocale;
    if (aLocale.empty())
    {
        const char* aName = Sexy::setLocale(nullptr, nullptr);
        aLocale = "locale/";
        if (aName != nullptr)
            aLocale += aName;
    }

    Sexy::String aJoined = aDir + aLocale;
    aJoined += "/";
    aJoined += thePath;
    return aJoined;
}

Sexy::String Sexy::ResourceManager::TryLocalizedResourcePath(Sexy::String const& thePath,
                                                            Sexy::String const& theDir,
                                                            Sexy::String const& theLocale)
{
    Sexy::String aLocalized = GetLocalizedResourcePath(thePath, theDir, theLocale);
    FILE* aFile = fopen(aLocalized.c_str(), "rb");
    if (aFile != nullptr)
    {
        fclose(aFile);
        return aLocalized;
    }
    return thePath;
}

Sexy::Image* Sexy::ResourceManager::LoadLocalizedImage(Sexy::String const& thePath, bool theAlpha,
                                                       Sexy::String const& theLocale)
{
    (void)theAlpha;
    if (mApp == nullptr)
        return nullptr;
    Sexy::String aEmpty;
    Sexy::String aLocalized = GetLocalizedResourcePath(thePath, aEmpty, theLocale);
    Sexy::Image* anImage = mApp->GetImage(aLocalized, true);
    if (anImage == nullptr)
        anImage = mApp->GetImage(thePath, true);
    return anImage;
}

bool Sexy::ResourceManager::LoadAlphaGridImage(ImageRes* theRes, Sexy::MemoryImage* theImage)
{
    if (theRes == nullptr || theImage == nullptr)
        return false;
    Sexy::String aEmpty;
    Sexy::Image* anAlpha = LoadLocalizedImage(theRes->mAlphaGridPath, true, aEmpty);
    if (anAlpha == nullptr)
    {
        char aText[512];
        sprintf(aText, "Failed to load image: %s", theRes->mAlphaGridPath.c_str());
        return Fail(Sexy::String(aText));
    }
    int aCols = theRes->mCols > 0 ? theRes->mCols : 1;
    int aRows = theRes->mRows > 0 ? theRes->mRows : 1;
    if (anAlpha->mWidth != theImage->mWidth / aCols || anAlpha->mHeight != theImage->mHeight / aRows)
    {
        char aText[512];
        sprintf(aText, "GridAlphaImage size mismatch between %s and %s",
                theRes->mPath.c_str(), theRes->mAlphaGridPath.c_str());
        delete anAlpha;
        return Fail(Sexy::String(aText));
    }
    delete anAlpha;
    return true;
}

bool Sexy::ResourceManager::LoadAlphaImage(ImageRes* theRes, Sexy::MemoryImage* theImage)
{
    if (theRes == nullptr || theImage == nullptr)
        return false;
    Sexy::String aEmpty;
    Sexy::Image* anAlpha = LoadLocalizedImage(theRes->mAlphaImagePath, true, aEmpty);
    if (anAlpha == nullptr)
    {
        char aText[512];
        sprintf(aText, "Failed to load image: %s", theRes->mAlphaImagePath.c_str());
        return Fail(Sexy::String(aText));
    }
    if (anAlpha->mWidth != theImage->mWidth || anAlpha->mHeight != theImage->mHeight)
    {
        char aText[512];
        sprintf(aText, "AlphaImage size mismatch between %s and %s",
                theRes->mPath.c_str(), theRes->mAlphaImagePath.c_str());
        delete anAlpha;
        return Fail(Sexy::String(aText));
    }
    delete anAlpha;
    return true;
}
