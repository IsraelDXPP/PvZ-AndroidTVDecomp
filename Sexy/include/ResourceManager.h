// ResourceManager loads an XML resource manifest and hands out refs.
//
// No base class and no vtable of its own. RTTI for the class is a plain
// class_type_info, so nothing here is virtual. The nested record types are
// polymorphic; they live in ResourceRecords.h because ResourceRef.h needs
// their names and cannot include this header. Both spellings name the same
// type: ResourceManager::BaseRes here, ResourceManager_BaseRes there.
//
// PopAnim.h is not included. Every PopAnim use below is a pointer, and
// ReconTypes.h already forward-declares Sexy::PopAnim. Including it closed
// ResourceManager.h -> PopAnim.h -> Widget.h -> ... -> ResourceManager.h,
// which left PopAnim inheriting an incomplete Widget. ResourceManager.cpp
// includes PopAnim.h itself.

#pragma once

#include "ReconTypes.h"
#include "AnimInfo.h"
#include "Buffer.h"
#include "CritSect.h"
#include "Image.h"
#include "PIEffect.h"
#include "RenderEffectDefinition.h"
#include "ResourceRecords.h"
#include "ImageLib.h"
#include "ResourceRef.h"
#include "SexyAppBase.h"
#include "SexyPerf.h"
#include "SharedImageRef.h"
#include "XMLElement.h"
#include "XMLParser.h"
#include <map>
#include <set>
#include <vector>

namespace Sexy {

class Font;
class GenericResFile;
class ImageFont;
class MemoryImage;

struct StringLessNoCase
{
    bool operator()(Sexy::String const& theLeft, Sexy::String const& theRight) const;
};

class ResourceManager
{
public:
    using ResType = ResourceManager_ResType;
    using BaseRes = ResourceManager_BaseRes;
    using SoundRes = ResourceManager_SoundRes;
    using FontRes = ResourceManager_FontRes;
    using PopAnimRes = ResourceManager_PopAnimRes;
    using PIEffectRes = ResourceManager_PIEffectRes;
    using RenderEffectRes = ResourceManager_RenderEffectRes;
    using GenericResFileRes = ResourceManager_GenericResFileRes;
    using ImageRes = ResourceManager_ImageRes;

    using ResMap = std::map<Sexy::String, BaseRes*>;
    using ResList = std::vector<BaseRes*>;
    using ResGroupMap = std::map<Sexy::String, ResList, StringLessNoCase>;
    using AttrMap = std::map<Sexy::String, Sexy::String>;

    // One piece of a composite group: the child group name, then the art
    // scale and locale that select it. Zero means "any".
    struct SubGroup
    {
        Sexy::String mName;
        int mScale;
        int mLocale;

        SubGroup();
    };

    struct CompositeResGroup
    {
        std::vector<SubGroup> mSubGroups;
    };

    std::set<Sexy::String, StringLessNoCase> mLoadedGroups;
    ResMap mResMaps[7];
    ResMap mResMapByPath;
    std::map<void*, BaseRes*> mGlobalPtrMap;

    Sexy::String mResFilePath;
    Sexy::String mResGenDir;
    int mResGenResId;
    int mResGenLocale;
    Sexy::String mResGenName;
    Sexy::String mResGenPrefix;
    Sexy::String mResGenSuffix;
    ResMap *mImageMap;
    ResMap *mSoundMap;
    ResMap *mFontMap;
    Sexy::String mUnusedPath;
    Sexy::String mUnusedId;
    Sexy::XMLParser *mXMLParser;
    Sexy::String mErrorText;
    bool mHasError;
    Sexy::SexyAppBase *mApp;
    Sexy::String mParentResGroupName;
    Sexy::String mCurResGroupName;
    int mCurResGroupResId;
    int mCurResGroupLocale;
    Sexy::String mPathPrefix;
    Sexy::String mIdPrefix;
    bool mAllowMissingProgramImages;
    bool mIsReparsing;
    bool mReuseLoadedResources;
    int mResGroupGeneration;
    ResGroupMap mResGroupMap;
    ResList *mCurResGroupList;
    ResList::iterator mCurResGroupItr;
    std::map<Sexy::String, CompositeResGroup, StringLessNoCase> mCompositeResGroupMap;
    int mCompositeResGroupIndex;
    int mImageResDir;
    int mImageResScale;
    int mLocaleCode;
    std::vector<SubGroup> mSubGroupList;
    int mManifestVersion;
    Sexy::CritSect mCritSect;
    Sexy::String mResSuffix;
    Sexy::String mLocaleSuffix;
    Sexy::String mResLocaleSuffix;

    ResourceManager(Sexy::SexyAppBase *theApp);
    ~ResourceManager();

    bool ParseCommonResource(Sexy::XMLElement &theElement, BaseRes *theRes, ResMap &theMap);
    bool ParseSoundResource(Sexy::XMLElement &theElement);
    bool ParseImageResource(Sexy::XMLElement &theElement);
    bool ParseFontResource(Sexy::XMLElement &theElement);
    bool ParsePopAnimResource(Sexy::XMLElement &theElement);
    bool ParsePIEffectResource(Sexy::XMLElement &theElement);
    bool ParseRenderEffectResource(Sexy::XMLElement &theElement);
    bool ParseGenericResFileResource(Sexy::XMLElement &theElement);
    bool ParseSetDefaults(Sexy::XMLElement &theElement);
    bool ParseResources();

    void DeleteMap(ResMap &theMap);
    void DeleteResources(ResMap &theMap, Sexy::String const& theGroup);

    bool DoLoadImage(ImageRes *theRes);
    bool DoLoadFont(FontRes *theRes);
    bool DoLoadSound(SoundRes *theRes);
    bool DoLoadPopAnim(PopAnimRes *theRes);
    bool DoLoadPIEffect(PIEffectRes *theRes);
    bool DoLoadRenderEffect(RenderEffectRes *theRes);
    bool DoLoadGenericResFile(GenericResFileRes *theRes);
    bool DoLoadResource(BaseRes *theRes, bool *theKeepGoing);

    bool LoadNextResource();
    void ResourceLoadedHook(BaseRes *theRes);
    void PrepareLoadResources(Sexy::String const& theGroup);
    bool StartLoadResources(Sexy::String const& theGroup, bool theFromComposite);
    bool LoadResources(Sexy::String const& theGroup);

    Sexy::SharedImageRef GetImageThrow(Sexy::String const& theId, int theScale, bool theRequired);
    Sexy::SharedImageRef GetImageThrow(Sexy::String const& theId, bool theRequired);
    int GetSoundThrow(Sexy::String const& theId);
    Sexy::Font *GetFontThrow(Sexy::String const& theId, int theScale);
    Sexy::PopAnim *GetPopAnimThrow(Sexy::String const& theId);
    Sexy::PIEffect *GetPIEffectThrow(Sexy::String const& theId);
    Sexy::RenderEffectDefinition *GetRenderEffectThrow(Sexy::String const& theId);
    Sexy::GenericResFile *GetGenericResFileThrow(Sexy::String const& theId);
    void DeleteResources(Sexy::String const& theGroup);

    bool IsGroupLoaded(Sexy::String const& theGroup);
    void DeleteExtraImageBuffers(Sexy::String const& theGroup);
    Sexy::String GetErrorText();
    bool HadError();
    bool InitResourceGen(Sexy::String const& theDir, Sexy::String const& theName,
                         Sexy::String const& thePrefix, int theResId, int theLocale,
                         Sexy::String const& theSuffix);
    void ReapplyConfigs();
    Sexy::String GetLocaleFolder(bool theTrailingSlash);
    int GetLocSetForLocaleName(Sexy::String const& theName);
    void DumpCurResGroup(Sexy::String &theDest);

    int GetNumImages(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale);
    int GetNumSounds(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale);
    int GetNumFonts(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale);
    int GetNumResources(Sexy::String const& theGroup, bool theFilterRes, bool theFilterLocale);
    int GetNumResources(Sexy::String const& theGroup, ResMap &theMap,
                        bool theFilterRes, bool theFilterLocale);

    void SetAllowMissingProgramImages(bool theAllow);
    void Deref(BaseRes *theRes);

    Sexy::ResourceRef GetResourceRef(BaseRes *theRes);
    Sexy::ResourceRef GetResourceRef(void *theSlot);
    void GetImageRef(Sexy::Image *&theSlot);
    void GetSoundRef(int &theSlot);
    void GetFontRef(Sexy::ImageFont *&theSlot);
    void GetPopAnimRef(Sexy::PopAnim *&theSlot);
    void GetPIEffectRef(Sexy::PIEffect *&theSlot);
    void GetRenderEffectRef(Sexy::RenderEffectDefinition *&theSlot);
    void GetGenericResFileRef(Sexy::GenericResFile *&theSlot);

    float GetLoadResourcesListProgress(char const** theGroups);
    void PrepareLoadResourcesList(char const** theGroups);

    bool Fail(Sexy::String const& theText);

    Sexy::String GetLocalizedResourcePath(Sexy::String const& thePath,
                                          Sexy::String const& theDir,
                                          Sexy::String const& theLocale);
    Sexy::String TryLocalizedResourcePath(Sexy::String const& thePath,
                                          Sexy::String const& theDir,
                                          Sexy::String const& theLocale);
    Sexy::Image *LoadLocalizedImage(Sexy::String const& thePath, bool theAlpha,
                                    Sexy::String const& theLocale);
    bool LoadAlphaGridImage(ImageRes *theRes, Sexy::MemoryImage *theImage);
    bool LoadAlphaImage(ImageRes *theRes, Sexy::MemoryImage *theImage);

    BaseRes *GetBaseRes(int theType, Sexy::String const& theId);
    void DeleteImage(Sexy::String const& theId);
    Sexy::SharedImageRef LoadImage(Sexy::String const& theId);
    int const* GetImageOffset(Sexy::String const& theId);
    Sexy::Font *LoadFont(Sexy::String const& theId);
    int LoadSound(Sexy::String const& theId);
    Sexy::PopAnim *LoadPopAnim(Sexy::String const& theId);
    Sexy::PIEffect *LoadPIEffect(Sexy::String const& theId);
    Sexy::RenderEffectDefinition *LoadRenderEffect(Sexy::String const& theId);
    Sexy::GenericResFile *LoadGenericResFile(Sexy::String const& theId);

    Sexy::SharedImageRef GetImage(Sexy::String const& theId);
    int GetSound(Sexy::String const& theId);
    Sexy::Font *GetFont(Sexy::String const& theId);
    bool IsResourceLoaded(Sexy::String const& theId);
    Sexy::PopAnim *GetPopAnim(Sexy::String const& theId);
    Sexy::PIEffect *GetPIEffect(Sexy::String const& theId);
    Sexy::RenderEffectDefinition *GetRenderEffect(Sexy::String const& theId);
    Sexy::GenericResFile *GetGenericResFile(Sexy::String const& theId);

    Sexy::ResourceRef GetResourceRef(int theType, Sexy::String const& theId);
    Sexy::ResourceRef GetImageRef(Sexy::String const& theId);
    Sexy::ResourceRef GetSoundRef(Sexy::String const& theId);
    Sexy::ResourceRef GetFontRef(Sexy::String const& theId);
    Sexy::ResourceRef GetPopAnimRef(Sexy::String const& theId);
    Sexy::ResourceRef GetPIEffectRef(Sexy::String const& theId);
    Sexy::ResourceRef GetRenderEffectRef(Sexy::String const& theId);
    Sexy::ResourceRef GetGenericResFileRef(Sexy::String const& theId);

    bool ReplaceImage(Sexy::String const& theId, Sexy::Image *theImage);
    bool ReplaceSound(Sexy::String const& theId, int theSoundId);
    bool DeleteSound(Sexy::String const& theId);
    bool ReplaceFont(Sexy::String const& theId, Sexy::Font *theFont);
    bool DeleteFont(Sexy::String const& theId);
    bool ReplacePopAnim(Sexy::String const& theId, Sexy::PopAnim *theAnim);
    bool DeletePopAnim(Sexy::String const& theId);
    bool ReplacePIEffect(Sexy::String const& theId, Sexy::PIEffect *theEffect);
    bool DeletePIEffect(Sexy::String const& theId);
    bool ReplaceRenderEffect(Sexy::String const& theId, Sexy::RenderEffectDefinition *theEffect);
    bool DeleteRenderEffect(Sexy::String const& theId);
    bool ReplaceGenericResFile(Sexy::String const& theId, Sexy::GenericResFile *theFile);
    bool DeleteGenericResFile(Sexy::String const& theId);

    AttrMap const& GetImageAttributes(Sexy::String const& theId);
    Sexy::String GetIdByPath(Sexy::String const& thePath);
    Sexy::String GetIdByPath(Sexy::String const& thePath, int theType);
    Sexy::String GetImageIdByPath(Sexy::String const& thePath);
    Sexy::String GetSoundIdByPath(Sexy::String const& thePath);
    Sexy::String GetFontIdByPath(Sexy::String const& thePath);
    Sexy::ResourceRef GetResourceRefFromPath(Sexy::String const& thePath);

    bool DoParseResources();
    void *RegisterGlobalPtr(Sexy::String const& theId, void *theSlot);
    bool ParseResourcesFile(Sexy::String const& theFileName);
    bool ReparseResourcesFile(Sexy::String const& theFileName);

private:
    void EnsureSuffixes();
    void Publish(BaseRes *theRes, void *theValue);
    void Hold(BaseRes *theRes);
    ResList &GroupList(Sexy::String const& theName);
    bool AcceptFont(FontRes *theRes, Sexy::Font *theFont);
    void ApplyImageFlags(ImageRes *theRes);
};

} // namespace Sexy

using Sexy::ResourceManager;
