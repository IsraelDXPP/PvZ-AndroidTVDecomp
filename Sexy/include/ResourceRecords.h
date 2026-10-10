// Resource records ResourceManager stores.
//
// The binary nests these as ResourceManager::BaseRes and friends. Callers
// spell them at namespace scope, so they are defined here and ResourceManager
// re-exports them. ResourceRef.h needs the names and cannot include
// ResourceManager.h.
//
// Every string is a member, so the constructor builds it. An uninitialised
// string crashes the first time its length is read. The loaded object is a
// real pointer of the type the loader stores. ImageRes keeps a SharedImageRef
// at the front of its extra fields; ResourceRef reads that object in place.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RESOURCERECORDS_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RESOURCERECORDS_H

#include "ReconTypes.h"
#include "SexyString.h"
#include "AnimInfo.h"
#include "SharedImageRef.h"
#include <map>

namespace Sexy
{

class ResourceManager;
class Font;
class Image;
class PopAnim;
class PIEffect;
class RenderEffectDefinition;
class GenericResFile;

enum ResourceManager_ResType
{
    ResType_Image = 0,
    ResType_Sound,
    ResType_Font,
    // The last four keep the names ResourceRef.h already compares against.
    // In this build they are pop-anim, particle, render-effect and generic file.
    ResType_Music,
    ResType_Reanim,
    ResType_Particle,
    ResType_Trail,
};

// Shared prefix. Deleted through BaseRes*, so the destructor is virtual.
// DeleteResource releases the loaded object. ApplyConfig re-reads tags and
// image flags after a load or a reparse.
struct ResourceManager_BaseRes
{
    virtual ~ResourceManager_BaseRes();
    virtual void DeleteResource();
    virtual void ApplyConfig();

    ResourceManager *mManager;
    // Caller's published slot. A successful load writes the object or sound id here.
    void *mResourceRef;
    // A "!ref:" font holds the source record here. The word is a ResourceRef*;
    // ResourceRef.h includes this header, so the complete type stays in the cpp.
    void *mHeldRef;
    int mRefCount;
    bool mRefHeld;
    int mGeneration;
    ResourceManager_ResType mResType;
    Sexy::String mId;
    Sexy::String mGroup;
    Sexy::String mParentGroup;
    Sexy::String mPath;
    Sexy::String mIdEncoded;
    Sexy::String mGroupEncoded;
    Sexy::String mParentEncoded;
    Sexy::String mPathEncoded;
    int mResId;
    int mLocale;
    std::map<Sexy::String, Sexy::String> mAttributes;
    // Path is the "!program" marker.
    bool mFromProgram;

    ResourceManager_BaseRes();
};

struct ResourceManager_SoundRes : public ResourceManager_BaseRes
{
    int mSoundId;
    double mVolume;
    int mPanning;

    ResourceManager_SoundRes();
    virtual void DeleteResource() override;
    virtual void ApplyConfig() override;
};

struct ResourceManager_FontRes : public ResourceManager_BaseRes
{
    Font *mFont;
    Image *mImage;
    Sexy::String mSourcePath;
    Sexy::String mTags;
    bool mIsSysFont;
    bool mBold;
    bool mItalic;
    bool mUnderline;
    bool mShadow;
    int mPointSize;

    ResourceManager_FontRes();
    virtual void DeleteResource() override;
    virtual void ApplyConfig() override;
};

struct ResourceManager_PopAnimRes : public ResourceManager_BaseRes
{
    PopAnim *mPopAnim;

    ResourceManager_PopAnimRes();
    virtual void DeleteResource() override;
};

struct ResourceManager_PIEffectRes : public ResourceManager_BaseRes
{
    PIEffect *mPIEffect;

    ResourceManager_PIEffectRes();
    virtual void DeleteResource() override;
};

struct ResourceManager_RenderEffectRes : public ResourceManager_BaseRes
{
    RenderEffectDefinition *mRenderEffect;
    Sexy::String mSourcePath;

    ResourceManager_RenderEffectRes();
    virtual void DeleteResource() override;
};

struct ResourceManager_GenericResFileRes : public ResourceManager_BaseRes
{
    GenericResFile *mGenericResFile;
    // The file body is the path. GenericResFile is only a forward declaration
    // in ReconTypes.h, so the loaded text lives here.
    Sexy::String mFileBody;

    ResourceManager_GenericResFileRes();
    virtual void DeleteResource() override;
};

// The shared image is the first extra field. ResourceRef treats that address
// as a SharedImageRef, so it has to be one.
struct ResourceManager_ImageRes : public ResourceManager_BaseRes
{
    Sexy::SharedImageRef mImage;
    Sexy::String mAlphaImagePath;
    Sexy::String mAlphaGridPath;
    Sexy::String mVariant;
    int mAlphaX;
    int mAlphaY;
    bool mNoAlpha;
    bool mNoPal;
    bool mUseA4R4G4B4;
    bool mUseA8R8G8B8;
    bool mUseA1R5G5B5;
    bool mUseR5G6B5;
    bool mNoBits;
    bool mMinSubdivide;
    bool mCubeMap;
    bool mVolumeMap;
    bool mHasBits;
    bool m2DBig;
    bool mAtlas;
    int mRows;
    int mCols;
    int mAlphaColor;
    Sexy::AnimInfo mAnim;
    Sexy::String mParentId;
    int mAlphaXOffset;
    int mAlphaYOffset;
    int mAlphaWidth;
    int mAlphaHeight;

    ResourceManager_ImageRes();
    virtual void DeleteResource() override;
    virtual void ApplyConfig() override;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RESOURCERECORDS_H
