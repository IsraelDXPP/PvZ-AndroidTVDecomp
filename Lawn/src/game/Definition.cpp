// Definition free path. The maps say which fields own heap data. Freeing a
// record walks that list: a string is released when its first byte is set, a
// float track releases its nodes, and an array frees each element then the
// block. Everything else is left alone.

#include "Definition.h"

#include "ReanimAtlas.h"
#include "ReanimatorDefinition.h"
#include "ReanimatorTransform.h"
#include "ReconData.h"
#include "TodParticleDefinition.h"
#include "TrailDefinition.h"
#include "TodHesitationBracket.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "PerfTimer.h"

#include "RawObject.h"
#include "ResourceManager.h"
#include "SharedImageRef.h"

#include <cstring>
#include <cstdio>
#include <map>
#include <string>
#include <zlib.h>
#include "Reanimation.h"

enum DefFieldType
{
    DEF_FIELD_INT = 1,
    DEF_FIELD_FLOAT = 2,
    DEF_FIELD_STRING = 3,
    DEF_FIELD_ENUM = 4,
    DEF_FIELD_ARRAY = 6,
    DEF_FIELD_TRACK = 7,
    DEF_FIELD_FLAGS = 8,
    DEF_FIELD_IMAGE = 9,
    DEF_FIELD_FONT = 10,
    DEF_FIELD_IMAGE_PAIR = 11,
    // Type 0xB in the TV field table. One field, two DefReadFromCacheImage calls.
};

struct DefinitionArrayField
{
    void *mData;
    int   mCount;
};

ParticleParamRecord *gParticleParamArray = nullptr;
int gParticleParamArraySize = 0;
TrailParamRecord *gTrailParamArray = nullptr;
int gTrailParamArraySize = 0;
int gParticleDefCount = 0;
int gTrailDefCount = 0;

// Declared first so the field tables can store their addresses. The headers
// are filled in EnsureDefMaps, before any walk.
static DefMap gParticleFieldDefMap;
static DefMap gTransformDefMap;
static DefMap gTrackDefMap;
static DefMap gEmitterDefMap;
static DefMap gReanimatorDefMap;

static const DefField gParticleFieldDefFields[] = {
    { "FieldType", offsetof(ParticleField, mFieldType), DEF_FIELD_ENUM, nullptr, 0 },
    { "x", offsetof(ParticleField, mX), DEF_FIELD_TRACK, nullptr, 4 },
    { "y", offsetof(ParticleField, mY), DEF_FIELD_TRACK, nullptr, 12 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTransformDefFields[] = {
    { "x", offsetof(ReanimatorTransform, mTransX), DEF_FIELD_FLOAT, nullptr, 0 },
    { "y", offsetof(ReanimatorTransform, mTransY), DEF_FIELD_FLOAT, nullptr, 4 },
    { "kx", offsetof(ReanimatorTransform, mSkewX), DEF_FIELD_FLOAT, nullptr, 8 },
    { "ky", offsetof(ReanimatorTransform, mSkewY), DEF_FIELD_FLOAT, nullptr, 12 },
    { "sx", offsetof(ReanimatorTransform, mScaleX), DEF_FIELD_FLOAT, nullptr, 16 },
    { "sy", offsetof(ReanimatorTransform, mScaleY), DEF_FIELD_FLOAT, nullptr, 20 },
    { "f", offsetof(ReanimatorTransform, mFrame), DEF_FIELD_FLOAT, nullptr, 24 },
    { "a", offsetof(ReanimatorTransform, mAlpha), DEF_FIELD_FLOAT, nullptr, 28 },
    // TV transform field table at 0x698c00. "i" is type 0xB, so one field reads
    // two images: id+resource into mImage, then id+resource into mImage2.
    // That is the decrypted <i><resource><i2><resource2>. font is type 0xA,
    // text is type 3.
    { "i", offsetof(ReanimatorTransform, mImage), DEF_FIELD_IMAGE_PAIR, nullptr, 32 },
    { "font", offsetof(ReanimatorTransform, mFont), DEF_FIELD_FONT, nullptr, 40 },
    { "text", offsetof(ReanimatorTransform, mText), DEF_FIELD_STRING, nullptr, 44 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTrackDefFields[] = {
    { "name", offsetof(ReanimatorTrack, mName), DEF_FIELD_STRING, nullptr, 4 },
    { "t", offsetof(ReanimatorTrack, mTransforms), DEF_FIELD_ARRAY, &gTransformDefMap, 8 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gReanimatorDefFields[] = {
    { "track", offsetof(ReanimatorDefinition, mTracks), DEF_FIELD_ARRAY, &gTrackDefMap, 0 },
    { "fps", offsetof(ReanimatorDefinition, mFPS), DEF_FIELD_FLOAT, nullptr, 8 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gEmitterDefFields[] = {
    { "Image", offsetof(TodEmitterDefinition, mImage), DEF_FIELD_IMAGE, nullptr, 0 },
    { "ImageRow", offsetof(TodEmitterDefinition, mImageRow), DEF_FIELD_INT, nullptr, 8 },
    { "ImageCol", offsetof(TodEmitterDefinition, mImageCol), DEF_FIELD_INT, nullptr, 4 },
    { "ImageFrames", offsetof(TodEmitterDefinition, mImageFrames), DEF_FIELD_INT, nullptr, 12 },
    { "Animated", offsetof(TodEmitterDefinition, mAnimated), DEF_FIELD_INT, nullptr, 16 },
    { "ParticleFlags", offsetof(TodEmitterDefinition, mFlags), DEF_FIELD_FLAGS, nullptr, 20 },
    { "EmitterType", offsetof(TodEmitterDefinition, mEmitterType), DEF_FIELD_ENUM, nullptr, 24 },
    { "Name", offsetof(TodEmitterDefinition, mName), DEF_FIELD_STRING, nullptr, 28 },
    { "SystemDuration", offsetof(TodEmitterDefinition, mSystemDuration), DEF_FIELD_TRACK, nullptr, 36 },
    { "OnDuration", offsetof(TodEmitterDefinition, mOnDuration), DEF_FIELD_STRING, nullptr, 32 },
    { "CrossFadeDuration", offsetof(TodEmitterDefinition, mCrossFadeDuration), DEF_FIELD_TRACK, nullptr, 44 },
    { "SpawnRate", offsetof(TodEmitterDefinition, mSpawnRate), DEF_FIELD_TRACK, nullptr, 52 },
    { "SpawnMinActive", offsetof(TodEmitterDefinition, mSpawnMinActive), DEF_FIELD_TRACK, nullptr, 60 },
    { "SpawnMaxActive", offsetof(TodEmitterDefinition, mSpawnMaxActive), DEF_FIELD_TRACK, nullptr, 68 },
    { "SpawnMaxLaunched", offsetof(TodEmitterDefinition, mSpawnMaxLaunched), DEF_FIELD_TRACK, nullptr, 76 },
    { "EmitterRadius", offsetof(TodEmitterDefinition, mEmitterRadius), DEF_FIELD_TRACK, nullptr, 84 },
    { "EmitterOffsetX", offsetof(TodEmitterDefinition, mEmitterOffsetX), DEF_FIELD_TRACK, nullptr, 92 },
    { "EmitterOffsetY", offsetof(TodEmitterDefinition, mEmitterOffsetY), DEF_FIELD_TRACK, nullptr, 100 },
    { "EmitterBoxX", offsetof(TodEmitterDefinition, mEmitterBoxX), DEF_FIELD_TRACK, nullptr, 108 },
    { "EmitterBoxY", offsetof(TodEmitterDefinition, mEmitterBoxY), DEF_FIELD_TRACK, nullptr, 116 },
    { "EmitterPath", offsetof(TodEmitterDefinition, mEmitterPath), DEF_FIELD_TRACK, nullptr, 140 },
    { "EmitterSkewX", offsetof(TodEmitterDefinition, mEmitterSkewX), DEF_FIELD_TRACK, nullptr, 124 },
    { "EmitterSkewY", offsetof(TodEmitterDefinition, mEmitterSkewY), DEF_FIELD_TRACK, nullptr, 132 },
    { "ParticleDuration", offsetof(TodEmitterDefinition, mParticleDuration), DEF_FIELD_TRACK, nullptr, 148 },
    { "SystemRed", offsetof(TodEmitterDefinition, mSystemRed), DEF_FIELD_TRACK, nullptr, 172 },
    { "SystemGreen", offsetof(TodEmitterDefinition, mSystemGreen), DEF_FIELD_TRACK, nullptr, 180 },
    { "SystemBlue", offsetof(TodEmitterDefinition, mSystemBlue), DEF_FIELD_TRACK, nullptr, 188 },
    { "SystemAlpha", offsetof(TodEmitterDefinition, mSystemAlpha), DEF_FIELD_TRACK, nullptr, 196 },
    { "SystemBrightness", offsetof(TodEmitterDefinition, mSystemBrightness), DEF_FIELD_TRACK, nullptr, 204 },
    { "LaunchSpeed", offsetof(TodEmitterDefinition, mLaunchSpeed), DEF_FIELD_TRACK, nullptr, 156 },
    { "LaunchAngle", offsetof(TodEmitterDefinition, mLaunchAngle), DEF_FIELD_TRACK, nullptr, 164 },
    { "Field", offsetof(TodEmitterDefinition, mParticleFields), DEF_FIELD_ARRAY, &gParticleFieldDefMap, 212 },
    { "SystemField", offsetof(TodEmitterDefinition, mSystemFields), DEF_FIELD_ARRAY, &gParticleFieldDefMap, 220 },
    { "ParticleRed", offsetof(TodEmitterDefinition, mParticleRed), DEF_FIELD_TRACK, nullptr, 228 },
    { "ParticleGreen", offsetof(TodEmitterDefinition, mParticleGreen), DEF_FIELD_TRACK, nullptr, 236 },
    { "ParticleBlue", offsetof(TodEmitterDefinition, mParticleBlue), DEF_FIELD_TRACK, nullptr, 244 },
    { "ParticleAlpha", offsetof(TodEmitterDefinition, mParticleAlpha), DEF_FIELD_TRACK, nullptr, 252 },
    { "ParticleBrightness", offsetof(TodEmitterDefinition, mParticleBrightness), DEF_FIELD_TRACK, nullptr, 260 },
    { "ParticleSpinAngle", offsetof(TodEmitterDefinition, mParticleSpinAngle), DEF_FIELD_TRACK, nullptr, 268 },
    { "ParticleSpinSpeed", offsetof(TodEmitterDefinition, mParticleSpinSpeed), DEF_FIELD_TRACK, nullptr, 276 },
    { "ParticleScale", offsetof(TodEmitterDefinition, mParticleScale), DEF_FIELD_TRACK, nullptr, 284 },
    { "ParticleStretch", offsetof(TodEmitterDefinition, mParticleStretch), DEF_FIELD_TRACK, nullptr, 292 },
    { "CollisionReflect", offsetof(TodEmitterDefinition, mCollisionReflect), DEF_FIELD_TRACK, nullptr, 300 },
    { "CollisionSpin", offsetof(TodEmitterDefinition, mCollisionSpin), DEF_FIELD_TRACK, nullptr, 308 },
    { "ClipTop", offsetof(TodEmitterDefinition, mClipTop), DEF_FIELD_TRACK, nullptr, 316 },
    { "ClipBottom", offsetof(TodEmitterDefinition, mClipBottom), DEF_FIELD_TRACK, nullptr, 324 },
    { "ClipLeft", offsetof(TodEmitterDefinition, mClipLeft), DEF_FIELD_TRACK, nullptr, 332 },
    { "ClipRight", offsetof(TodEmitterDefinition, mClipRight), DEF_FIELD_TRACK, nullptr, 340 },
    { "AnimationRate", offsetof(TodEmitterDefinition, mAnimationRate), DEF_FIELD_TRACK, nullptr, 348 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gParticleDefFields[] = {
    { "Emitter", offsetof(TodParticleDefinition, mEmitterDefs), DEF_FIELD_ARRAY, &gEmitterDefMap, 0 },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTrailDefFields[] = {
    { "Image", offsetof(TrailDefinition, mImage), DEF_FIELD_IMAGE, nullptr, 0 },
    { "MaxPoints", offsetof(TrailDefinition, mMaxPoints), DEF_FIELD_INT, nullptr, 4 },
    { "MinPointDistance", offsetof(TrailDefinition, mMinPointDistance), DEF_FIELD_FLOAT, nullptr, 8 },
    { "TrailFlags", offsetof(TrailDefinition, mTrailFlags), DEF_FIELD_FLAGS, nullptr, 12 },
    { "WidthOverLength", offsetof(TrailDefinition, mWidthOverLength), DEF_FIELD_TRACK, nullptr, 24 },
    { "WidthOverTime", offsetof(TrailDefinition, mWidthOverTime), DEF_FIELD_TRACK, nullptr, 32 },
    { "AlphaOverLength", offsetof(TrailDefinition, mAlphaOverLength), DEF_FIELD_TRACK, nullptr, 40 },
    { "AlphaOverTime", offsetof(TrailDefinition, mAlphaOverTime), DEF_FIELD_TRACK, nullptr, 48 },
    { "TrailDuration", offsetof(TrailDefinition, mTrailDuration), DEF_FIELD_TRACK, nullptr, 16 },
    { gEmptyCString, 0, 0, nullptr }
};

DefMap gParticleDefMap;
DefMap gTrailDefMap;

static void EnsureDefMaps()
{
    if (gParticleDefMap.mFields != nullptr)
        return;
    gParticleFieldDefMap.mFields = gParticleFieldDefFields;
    gParticleFieldDefMap.mSize = sizeof(ParticleField);
    gParticleFieldDefMap.mFileSize = 20;
    gTransformDefMap.mFields = gTransformDefFields;
    gTransformDefMap.mSize = sizeof(ReanimatorTransform);
    gTransformDefMap.mFileSize = 48;
    gTrackDefMap.mFields = gTrackDefFields;
    gTrackDefMap.mSize = sizeof(ReanimatorTrack);
    gTrackDefMap.mFileSize = 20;
    gEmitterDefMap.mFields = gEmitterDefFields;
    gEmitterDefMap.mSize = sizeof(TodEmitterDefinition);
    gEmitterDefMap.mFileSize = 356;
    gReanimatorDefMap.mFields = gReanimatorDefFields;
    gReanimatorDefMap.mSize = sizeof(ReanimatorDefinition);
    gReanimatorDefMap.mFileSize = 16;
    gParticleDefMap.mFields = gParticleDefFields;
    gParticleDefMap.mSize = sizeof(TodParticleDefinition);
    gParticleDefMap.mFileSize = 8;
    gTrailDefMap.mFields = gTrailDefFields;
    gTrailDefMap.mSize = sizeof(TrailDefinition);
    gTrailDefMap.mFileSize = 56;
}

static void DefinitionFree(const void *theData)
{
    if (theData != nullptr)
        operator delete[](const_cast<void *>(theData));
}

static void DefinitionFreeStringField(const char **theSlot)
{
    const char *text = *theSlot;
    if (text != nullptr && text[0] != 0)
        DefinitionFree(text);
    *theSlot = nullptr;
}

static void DefinitionFreeFloatTrackField(FloatParameterTrack *theTrack)
{
    if (theTrack->mCountNodes != 0)
        DefinitionFree(theTrack->mNodes);
    theTrack->mNodes = nullptr;
}

static void DefinitionFreeArrayField(DefinitionArrayField *theField, DefMap *theNestedMap)
{
    if (theField->mCount > 0)
    {
        int index = 0;
        do
        {
            char *element = static_cast<char *>(theField->mData) + theNestedMap->mSize * index;
            ++index;
            DefinitionFreeMap(theNestedMap, element);
        }
        while (theField->mCount > index);
    }
    DefinitionFree(theField->mData);
    theField->mData = nullptr;
}

void DefinitionFreeMap(DefMap *theMap, void *theObject)
{
    EnsureDefMaps();
    const DefField *field = theMap->mFields;
    if (field->mName[0] == 0)
        return;

    const DefField *next = field + 1;
    for (;;)
    {
        char *slot = static_cast<char *>(theObject) + field->mOffset;
        if (field->mType == DEF_FIELD_ARRAY)
        {
            DefinitionFreeArrayField(
                (DefinitionArrayField *)slot,
                static_cast<DefMap *>(field->mExtra));
        }
        else if (field->mType == DEF_FIELD_TRACK)
        {
            DefinitionFreeFloatTrackField((FloatParameterTrack *)slot);
        }
        else if (field->mType == DEF_FIELD_STRING)
        {
            DefinitionFreeStringField((const char **)slot);
        }

        if (next->mName[0] == 0)
            return;
        field = next;
        ++next;
    }
}

void ReanimationFreeDefinition(ReanimatorDefinition *theDefinition)
{
    if (theDefinition->mReanimAtlas != nullptr)
    {
        theDefinition->mReanimAtlas->ReanimAtlasDispose();
        operator delete(theDefinition->mReanimAtlas);
        theDefinition->mReanimAtlas = nullptr;
    }

    if (theDefinition->mTrackCount > 0)
    {
        ReanimatorTrack *track = theDefinition->mTracks;
        ReanimatorTrack *trackEnd = track + theDefinition->mTrackCount;
        while (track != trackEnd)
        {
            if (track->mTransformCount > 0)
            {
                ReanimatorTransform *transform = track->mTransforms;
                ReanimatorTransform *transformEnd = transform + track->mTransformCount;
                const char *previousText = nullptr;
                while (transform != transformEnd)
                {
                    const char *text = transform->mText;
                    if (text != nullptr && text[0] != 0 && previousText == text)
                        transform->mText = gEmptyCString;
                    previousText = text;
                    ++transform;
                }
            }
            ++track;
        }
    }

    DefinitionFreeMap(&gReanimatorDefMap, theDefinition);
}

void TodParticleFreeDefinitions()
{
    for (int index = 0; index < gParticleDefCount; ++index)
        DefinitionFreeMap(&gParticleDefMap, &gParticleDefArray[index]);

    if (gParticleDefArray != nullptr)
        delete[] gParticleDefArray;
    gParticleDefArray = nullptr;
    gParticleDefCount = 0;
    gParticleParamArray = nullptr;
    gParticleParamArraySize = 0;
}

static void* DefinitionAlloc(size_t theSize)
{
    void* aBlock = operator new[](theSize);
    std::memset(aBlock, 0, theSize);
    return aBlock;
}

static void DefinitionFreeBlock(void* theBlock)
{
    if (theBlock != nullptr)
        operator delete[](theBlock);
}

static int ReadCacheInt(const char*& theCursor, const char* theEnd)
{
    if (theCursor + 4 > theEnd)
        return 0;
    int aValue = 0;
    std::memcpy(&aValue, theCursor, 4);
    theCursor += 4;
    return aValue;
}

static int ReadCacheBytes(const char*& theCursor, const char* theEnd, void* theDest, int theCount)
{
    if (theCount < 0 || theCursor + theCount > theEnd)
        return 0;
    if (theCount > 0)
        std::memcpy(theDest, theCursor, (unsigned)theCount);
    theCursor += theCount;
    return 1;
}

static const char* ReadCacheCString(const char*& theCursor, const char* theEnd)
{
    int aLength = ReadCacheInt(theCursor, theEnd);
    if (aLength == 0)
        return gEmptyCString;
    if (aLength < 0)
        return nullptr;
    char* aText = static_cast<char*>(DefinitionAlloc(aLength + 1));
    if (ReadCacheBytes(theCursor, theEnd, aText, aLength) == 0)
    {
        DefinitionFreeBlock(aText);
        return nullptr;
    }
    aText[aLength] = 0;
    return aText;
}

static int DefMapReadFromCache(const char*& theCursor, const char* theEnd, const DefMap* theMap, void* theObject);

// Compiled TV resources retain their ARM record layout. Only scalar fields
// and array counts are copied; file pointer placeholders never become addresses.
static int ReadCacheRecord(const char *&theCursor, const char *theEnd,
                           const DefMap *theMap, void *theObject)
{
    if (theEnd - theCursor < theMap->mFileSize)
        return 0;
    for (const DefField *aField = theMap->mFields; aField->mName[0] != 0; ++aField)
    {
        char *aSlot = static_cast<char *>(theObject) + aField->mOffset;
        const char *aFileSlot = theCursor + aField->mFileOffset;
        switch (aField->mType)
        {
        case DEF_FIELD_INT:
        case DEF_FIELD_FLOAT:
        case DEF_FIELD_ENUM:
        case DEF_FIELD_FLAGS:
            std::memcpy(aSlot, aFileSlot, sizeof(int));
            break;
        case DEF_FIELD_ARRAY:
            std::memcpy(&reinterpret_cast<DefinitionArrayField *>(aSlot)->mCount,
                        aFileSlot + sizeof(int), sizeof(int));
            break;
        default:
            break;
        }
    }
    theCursor += theMap->mFileSize;
    return 1;
}

static int ReadCacheArray(const char*& theCursor, const char* theEnd, DefinitionArrayField* theField, const DefMap* theNested)
{
    // The integer in the file is the element stride. The element count was
    // already copied into the object with the raw record. TV writes the raw
    // records as one packed blob, then walks each element for its strings.
    // Zombie.reanim.compiled is 46 tracks of 663 frames: 48 float bytes each,
    // then i, resource, i2, resource2, font, text on every frame, including
    // the empty ones. Reading the strings inside the packed blob consumes the
    // next frame's floats as a name, which shears the sprout without crashing.
    int aStride = ReadCacheInt(theCursor, theEnd);
    if (aStride != theNested->mFileSize)
        return 0;
    if (theField->mCount < 0)
        return 0;
    if (theField->mCount == 0)
        return 1;
    theField->mData = DefinitionAlloc(theNested->mSize * theField->mCount);
    char *aRecord = static_cast<char *>(theField->mData);
    for (int aIndex = 0; aIndex < theField->mCount; ++aIndex)
    {
        if (!ReadCacheRecord(theCursor, theEnd, theNested, aRecord))
            return 0;
        aRecord += theNested->mSize;
    }
    char* aElement = static_cast<char*>(theField->mData);
    for (int aIndex = 0; aIndex < theField->mCount; ++aIndex)
    {
        if (DefMapReadFromCache(theCursor, theEnd, theNested, aElement) == 0)
            return 0;
        aElement += theNested->mSize;
    }
    return 1;
}

static int ReadCacheTrack(const char*& theCursor, const char* theEnd, FloatParameterTrack* theTrack)
{
    theTrack->mCountNodes = ReadCacheInt(theCursor, theEnd);
    if (theTrack->mCountNodes < 0)
        return 0;
    if (theTrack->mCountNodes == 0)
        return 1;
    theTrack->mNodes = static_cast<FloatParameterTrackNode*>(
        DefinitionAlloc(sizeof(FloatParameterTrackNode) * theTrack->mCountNodes));
    return ReadCacheBytes(theCursor, theEnd, theTrack->mNodes, sizeof(FloatParameterTrackNode) * theTrack->mCountNodes);
}

static int ReadCacheString(const char*& theCursor, const char* theEnd, const char** theSlot)
{
    const char* aText = ReadCacheCString(theCursor, theEnd);
    if (aText == nullptr)
        return 0;
    *theSlot = aText;
    return 1;
}

// DefinitionLoadImage @0x202570. The id is tried first. A miss with a path
// calls GetSharedImage(path, empty variant, null, commitBits, allowTriReps=0),
// which is the same virtual GamepadApp uses for every other picture. The five
// prefix pairs are gDefLoadResPaths at 0x698ea8.
static const char* kDefLoadPrefixes[] = {
    "IMAGE_", "particles/",
    "IMAGE_", "particles/",
    "IMAGE_REANIM_", "reanim/",
    "IMAGE_REANIM_", "images/",
    "IMAGE_REANIM_LAYER_", "reanim/mainmenu3/"
};

static bool IdEndsWithPng(const char* theId)
{
    if (theId == nullptr)
        return false;
    int aLength = (int)std::strlen(theId);
    if (aLength <= 4)
        return false;
    const char* aTail = theId + aLength - 4;
    return (aTail[0] == '.' )
        && (aTail[1] == 'P' || aTail[1] == 'p')
        && (aTail[2] == 'N' || aTail[2] == 'n')
        && (aTail[3] == 'G' || aTail[3] == 'g');
}

static Sexy::Image* LoadImageBySharedPath(const Sexy::String& thePath)
{
    if (gSexyAppBase == nullptr || thePath.empty())
        return nullptr;
    Sexy::String aVariant;
    Sexy::SharedImageRef aLoaded = gSexyAppBase->GetSharedImage(thePath, aVariant, nullptr, true, false);
    return aLoaded.mImage;
}

static int DefinitionLoadImage(Sexy::Image** theSlot, char* theId, const char* thePath)
{
    *theSlot = nullptr;
    if (theId == nullptr || theId[0] == 0)
        return 1;

    if (IdEndsWithPng(theId))
    {
        int aLength = (int)std::strlen(theId);
        theId[aLength - 4] = 0;
    }

    Sexy::String aId(theId);
    if (gSexyAppBase != nullptr && gSexyAppBase->mResourceManager != nullptr)
    {
        Sexy::SharedImageRef aLoaded = gSexyAppBase->mResourceManager->LoadImage(aId);
        if (aLoaded.mImage != nullptr)
        {
            *theSlot = aLoaded.mImage;
            return 1;
        }
    }

    if (thePath != nullptr && thePath[0] != 0)
    {
        Sexy::Image* aFound = LoadImageBySharedPath(Sexy::String(thePath));
        if (aFound != nullptr)
        {
            *theSlot = aFound;
            return 1;
        }
    }

    int aIdLength = (int)aId.size();
    for (int aPair = 0; aPair < 5; ++aPair)
    {
        const char* aPrefix = kDefLoadPrefixes[aPair * 2];
        const char* aReplacement = kDefLoadPrefixes[aPair * 2 + 1];
        int aPrefixLength = (int)std::strlen(aPrefix);
        if (aPrefixLength >= aIdLength)
            continue;
        Sexy::String aRewritten(aReplacement);
        aRewritten += aId.c_str() + aPrefixLength;
        Sexy::Image* aFound = LoadImageBySharedPath(aRewritten);
        if (aFound != nullptr)
        {
            *theSlot = aFound;
            return 1;
        }
    }

    // The id and the resource path were both read. A missing picture must not
    // abort the map walk: the next frame's strings still have to be consumed,
    // or every later frame shears. TV returns 0 from DefinitionLoadImage here;
    // the slot stays empty and the cursor has already passed both strings.
    *theSlot = nullptr;
    return 1;
}

// A compiled image is two length-prefixed names: the resource id, then the
// file path. An empty id leaves the slot empty and succeeds. A present id
// goes through DefinitionLoadImage.
static int ReadCacheImage(const char*& theCursor, const char* theEnd, Sexy::Image** theSlot)
{
    int aIdLength = ReadCacheInt(theCursor, theEnd);
    char* aIdText = nullptr;
    if (aIdLength < 0)
        return 0;
    if (aIdLength > 0)
    {
        aIdText = static_cast<char*>(DefinitionAlloc(aIdLength + 1));
        if (ReadCacheBytes(theCursor, theEnd, aIdText, aIdLength) == 0)
        {
            DefinitionFreeBlock(aIdText);
            return 0;
        }
        aIdText[aIdLength] = 0;
    }
    int aPathLength = ReadCacheInt(theCursor, theEnd);
    char* aPathText = nullptr;
    if (aPathLength < 0)
    {
        DefinitionFreeBlock(aIdText);
        return 0;
    }
    if (aPathLength > 0)
    {
        aPathText = static_cast<char*>(DefinitionAlloc(aPathLength + 1));
        if (ReadCacheBytes(theCursor, theEnd, aPathText, aPathLength) == 0)
        {
            DefinitionFreeBlock(aIdText);
            DefinitionFreeBlock(aPathText);
            return 0;
        }
        aPathText[aPathLength] = 0;
    }

    int aLoaded = DefinitionLoadImage(theSlot, aIdText, aPathText);
    DefinitionFreeBlock(aIdText);
    DefinitionFreeBlock(aPathText);
    return aLoaded;
}

static int ReadCacheFont(const char*& theCursor, const char* theEnd, Sexy::Font** theSlot)
{
    int aLength = ReadCacheInt(theCursor, theEnd);
    *theSlot = nullptr;
    if (aLength < 0)
        return 0;
    if (aLength == 0)
        return 1;
    char* aText = static_cast<char*>(DefinitionAlloc(aLength + 1));
    if (ReadCacheBytes(theCursor, theEnd, aText, aLength) == 0)
    {
        DefinitionFreeBlock(aText);
        return 0;
    }
    aText[aLength] = 0;
    if (gSexyAppBase != nullptr && gSexyAppBase->mResourceManager != nullptr)
        *theSlot = gSexyAppBase->mResourceManager->LoadFont(Sexy::String(aText));
    DefinitionFreeBlock(aText);
    // Same rule as a missing picture: the name was consumed, so the walk
    // continues and the slot stays empty.
    return 1;
}

template <class T>
T* FieldAt(void* theObject, int theOffset)
{
    char* aBytes = (char*)theObject;
    return (T*)(aBytes + theOffset);
}

static int DefMapReadFromCache(const char*& theCursor, const char* theEnd, const DefMap* theMap, void* theObject)
{
    if (theMap->mFields == nullptr || theMap->mFields->mName == nullptr || theMap->mFields->mName[0] == 0)
        return 1;
    for (const DefField* aField = theMap->mFields; aField->mName != nullptr && aField->mName[0] != 0; ++aField)
    {
        if (aField->mType == DEF_FIELD_STRING)
        {
            if (ReadCacheString(theCursor, theEnd, FieldAt<const char*>(theObject, aField->mOffset)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_ARRAY)
        {
            if (ReadCacheArray(theCursor, theEnd, FieldAt<DefinitionArrayField>(theObject, aField->mOffset),
                               static_cast<const DefMap*>(aField->mExtra)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_TRACK)
        {
            if (ReadCacheTrack(theCursor, theEnd, FieldAt<FloatParameterTrack>(theObject, aField->mOffset)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_IMAGE)
        {
            if (ReadCacheImage(theCursor, theEnd, FieldAt<Sexy::Image*>(theObject, aField->mOffset)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_FONT)
        {
            if (ReadCacheFont(theCursor, theEnd, FieldAt<Sexy::Font*>(theObject, aField->mOffset)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_IMAGE_PAIR)
        {
            Sexy::Image** aPair = FieldAt<Sexy::Image*>(theObject, aField->mOffset);
            if (ReadCacheImage(theCursor, theEnd, aPair) == 0)
                return 0;
            if (ReadCacheImage(theCursor, theEnd, aPair + 1) == 0)
                return 0;
        }

    }
    return 1;
}

static int DefinitionReadCompiledFile(const Sexy::String& thePath, DefMap* theMap, void* theObject)
{
    EnsureDefMaps();
    FILE* aFile = p_fopen(thePath.c_str(), "rb");
    if (aFile == nullptr)
        return 0;
    p_fseek(aFile, 0, SEEK_END);
    long aSize = p_ftell(aFile);
    p_fseek(aFile, 0, SEEK_SET);
    if (aSize < 4)
    {
        p_fclose(aFile);
        return 0;
    }
    char* aBytes = static_cast<char*>(DefinitionAlloc((int)aSize));
    int aRead = (int)p_fread(aBytes, 1, (size_t)aSize, aFile);
    p_fclose(aFile);
    if (aRead != (int)aSize)
    {
        DefinitionFreeBlock(aBytes);
        return 0;
    }
    // A compiled file is either a raw cache or a zlib packet whose first word
    // is 0xDEADFED4 and whose second word is the uncompressed size. This build's
    // schema hash is 0, so a non-zero hash means the packet does not match.
    const char* aCursor = aBytes;
    int aOwned = 0;
    char* aOwnedBytes = nullptr;
    if (aSize >= 8)
    {
        int aCookie = 0;
        std::memcpy(&aCookie, aBytes, 4);
        if (aCookie == (int)0xDEADFED4)
        {
            int aUncompressedSize = 0;
            std::memcpy(&aUncompressedSize, aBytes + 4, 4);
            if (aUncompressedSize <= 0)
            {
                DefinitionFreeBlock(aBytes);
                return 0;
            }
            aOwnedBytes = static_cast<char*>(DefinitionAlloc(aUncompressedSize));
            unsigned long aGot = (unsigned long)aUncompressedSize;
            unsigned char* aDest = (unsigned char*)aOwnedBytes;
            const unsigned char* aSource = (const unsigned char*)(aBytes + 8);
            int aStatus = uncompress(aDest, &aGot, aSource, (unsigned long)(aSize - 8));
            DefinitionFreeBlock(aBytes);
            if (aStatus != Z_OK || (int)aGot != aUncompressedSize)
            {
                DefinitionFreeBlock(aOwnedBytes);
                return 0;
            }
            aCursor = aOwnedBytes;
            aSize = aUncompressedSize;
            aOwned = 1;
        }
    }
    if (aSize < theMap->mFileSize + 4)
    {
        if (aOwned != 0)
            DefinitionFreeBlock(aOwnedBytes);
        else
            DefinitionFreeBlock(aBytes);
        return 0;
    }
    const char* aEnd = aCursor + aSize;
    int aHash = ReadCacheInt(aCursor, aEnd);
    if (aHash != 0)
    {
        if (aOwned != 0)
            DefinitionFreeBlock(aOwnedBytes);
        else
            DefinitionFreeBlock(aBytes);
        return 0;
    }
    if (ReadCacheRecord(aCursor, aEnd, theMap, theObject) == 0)
    {
        if (aOwned != 0)
            DefinitionFreeBlock(aOwnedBytes);
        else
            DefinitionFreeBlock(aBytes);
        return 0;
    }
    int aOk = DefMapReadFromCache(aCursor, aEnd, theMap, theObject);
    // TV DefinitionReadCompiledFile requires the cursor to land on the end.
    // A short read still returns the map walk's result, which hides a schema
    // that consumed the next frame's floats as a string.
    if (aOk != 0 && aCursor != aEnd)
        aOk = 0;
    if (aOwned != 0)
        DefinitionFreeBlock(aOwnedBytes);
    else
        DefinitionFreeBlock(aBytes);
    return aOk;
}

static int DefinitionCompileAndLoad(const Sexy::String& theXmlPath, DefMap* theMap, void* theObject)
{
    Sexy::String aCompiled("compiled/");
    aCompiled += theXmlPath;
    aCompiled.Append(".compiled", 9);
    return DefinitionReadCompiledFile(aCompiled, theMap, theObject);
}

static void FloatTrackSetDefault(FloatParameterTrack* theTrack, float theValue)
{
    if (theTrack->mNodes != nullptr || theValue == 0.0f)
        return;
    theTrack->mCountNodes = 1;
    theTrack->mNodes = static_cast<FloatParameterTrackNode*>(DefinitionAlloc(sizeof(FloatParameterTrackNode)));
    theTrack->mNodes->mTime = 0.0f;
    theTrack->mNodes->mLowValue = theValue;
    theTrack->mNodes->mHighValue = theValue;
    theTrack->mNodes->mCurveType = CURVE_CONSTANT;
    theTrack->mNodes->mDistribution = CURVE_LINEAR;
}

int TrailLoadADef(TrailDefinition* theDefinition, const char* theFileName)
{
    TodHesitationBracket aBracket("Load Trail '%s'", theFileName);
    int aLoaded = DefinitionCompileAndLoad(Sexy::String(theFileName), &gTrailDefMap, theDefinition);
    if (aLoaded != 0)
    {
        FloatTrackSetDefault(&theDefinition->mWidthOverTime, 1.0f);
        FloatTrackSetDefault(&theDefinition->mAlphaOverLength, 1.0f);
        FloatTrackSetDefault(&theDefinition->mWidthOverLength, 100.0f);
        FloatTrackSetDefault(&theDefinition->mAlphaOverTime, 1.0f);
        FloatTrackSetDefault(&theDefinition->mTrailDuration, 1.0f);
    }
    return aLoaded;
}

void TrailLoadDefinitions(TrailParamRecord* theParams, int theCount)
{
    TodHesitationBracket aBracket("TrailLoadDefinitions");
    gTrailParamArraySize = theCount;
    gTrailParamArray = theParams;
    gTrailDefCount = theCount;
    gTrailDefArray = new TrailDefinition[theCount];
    for (int aIndex = 0; aIndex < theCount; ++aIndex)
    {
        if (TrailLoadADef(&gTrailDefArray[aIndex], theParams[aIndex].mFileName) == 0)
        {
            char aMessage[1024];
            snprintf(aMessage, sizeof(aMessage), "Failed to load trail '%s'", theParams[aIndex].mFileName);
            TodErrorMessageBox(aMessage, "Error");
        }
    }
}

int TodParticleLoadADef(TodParticleDefinition* theDefinition, const char* theFileName)
{
    TodHesitationBracket aBracket("Load Particle '%s'", theFileName);
    int aLoaded = DefinitionCompileAndLoad(Sexy::String(theFileName), &gParticleDefMap, theDefinition);
    if (aLoaded == 0)
        return aLoaded;

    // Defaults from 0x207B4C. Constant defaults remain distinguishable from
    // explicitly keyed tracks through FloatTrackIsSet.
    for (int anIndex = 0; anIndex < theDefinition->mEmitterDefCount; ++anIndex)
    {
        TodEmitterDefinition& anEmitter = theDefinition->mEmitterDefs[anIndex];
        FloatTrackSetDefault(&anEmitter.mSpawnMinActive, -1.0f);
        FloatTrackSetDefault(&anEmitter.mSpawnMaxActive, -1.0f);
        FloatTrackSetDefault(&anEmitter.mSpawnMaxLaunched, -1.0f);
        FloatTrackSetDefault(&anEmitter.mParticleDuration, 100.0f);
        for (FloatParameterTrack* aTrack : {
            &anEmitter.mSystemRed, &anEmitter.mSystemGreen, &anEmitter.mSystemBlue,
            &anEmitter.mSystemAlpha, &anEmitter.mSystemBrightness,
            &anEmitter.mParticleRed, &anEmitter.mParticleGreen, &anEmitter.mParticleBlue,
            &anEmitter.mParticleAlpha, &anEmitter.mParticleBrightness,
            &anEmitter.mParticleScale, &anEmitter.mParticleStretch})
            FloatTrackSetDefault(aTrack, 1.0f);
    }
    return aLoaded;
}

void TodParticleLoadDefinitions(ParticleParamRecord* theParams, int theCount)
{
    TodHesitationBracket aBracket("TodParticleLoadDefinitions");
    gParticleParamArraySize = theCount;
    gParticleParamArray = theParams;
    gParticleDefCount = theCount;
    gParticleDefArray = new TodParticleDefinition[theCount];
    for (int aIndex = 0; aIndex < theCount; ++aIndex)
    {
        if (TodParticleLoadADef(&gParticleDefArray[aIndex], theParams[aIndex].mFileName) == 0)
        {
            char aMessage[1024];
            snprintf(aMessage, sizeof(aMessage), "Failed to load particle '%s'", theParams[aIndex].mFileName);
            TodErrorMessageBox(aMessage, "Error");
        }
        gSexyAppBase->mCompletedLoadingThreadTasks += 6;
    }
}

static float MissingSentinel()
{
    return -10000.0f;
}

// A compiled frame stores -10000 for a channel it does not key. The original
// walk (ReanimationFillInMissingData @0x1F935C) copies the previous keyed
// value into that slot and leaves the carried value alone, so the next missing
// frame inherits the same key. The comparison is an exact float compare.
static void FillMissingFloat(float& thePrevious, float& theValue)
{
    if (theValue == MissingSentinel())
        theValue = thePrevious;
    else
        thePrevious = theValue;
}

// ReanimationLoadDefinition @0x1F937C fills these after the file is in memory.
// The TV build only has the compiled packet, and DefinitionCompileAndLoad does
// not fill anything, so the walk has to run here. Carried defaults match the
// eight stores before the frame loop: translation and skew 0, scale 1, frame
// 0, alpha 1. An unkeyed frame channel stays 0, which is how a layer marks the
// frames it does not play.
static void FillMissingReanimFrames(ReanimatorDefinition* theDefinition)
{
    ReanimatorTrack* aTracks = theDefinition->mTracks;
    for (int aTrack = 0; aTrack < theDefinition->mTrackCount; ++aTrack)
    {
        float aTransX = 0.0f;
        float aTransY = 0.0f;
        float aSkewX = 0.0f;
        float aSkewY = 0.0f;
        float aScaleX = 1.0f;
        float aScaleY = 1.0f;
        float aFrame = 0.0f;
        float aAlpha = 1.0f;
        const char* aPreviousText = gEmptyCString;
        Sexy::Image* aImage = nullptr;
        Sexy::Image* aImage2 = nullptr;
        Sexy::Font* aFont = nullptr;
        ReanimatorTransform* aTransforms = aTracks[aTrack].mTransforms;
        for (int aFrameIndex = 0; aFrameIndex < aTracks[aTrack].mTransformCount; ++aFrameIndex)
        {
            ReanimatorTransform& aTransform = aTransforms[aFrameIndex];
            FillMissingFloat(aTransX, aTransform.mTransX);
            FillMissingFloat(aTransY, aTransform.mTransY);
            FillMissingFloat(aSkewX, aTransform.mSkewX);
            FillMissingFloat(aSkewY, aTransform.mSkewY);
            FillMissingFloat(aScaleX, aTransform.mScaleX);
            FillMissingFloat(aScaleY, aTransform.mScaleY);
            FillMissingFloat(aFrame, aTransform.mFrame);
            FillMissingFloat(aAlpha, aTransform.mAlpha);
            if (aTransform.mImage == nullptr)
                aTransform.mImage = aImage;
            else
                aImage = aTransform.mImage;
            if (aTransform.mImage2 == nullptr)
                aTransform.mImage2 = aImage2;
            else
                aImage2 = aTransform.mImage2;
            if (aTransform.mFont == nullptr)
                aTransform.mFont = aFont;
            else
                aFont = aTransform.mFont;
            if (aTransform.mText == nullptr || aTransform.mText[0] == 0)
                aTransform.mText = aPreviousText;
            else
                aPreviousText = aTransform.mText;
        }
    }
}

int ReanimationLoadDefinition(const Sexy::String& theFileName, ReanimatorDefinition* theDefinition)
{
    EnsureDefMaps();
    int aLoaded = DefinitionCompileAndLoad(theFileName, &gReanimatorDefMap, theDefinition);
    if (aLoaded == 0)
        return 0;
    FillMissingReanimFrames(theDefinition);
    return aLoaded;
}

const char* ReanimatorTrackNameToId(const char* theName)
{
    // Same intern table the startup assignment and the reanim loader share.
    // The returned pointer is the stored string's characters, so later
    // TrackExistsById compares identity, not text.
    static std::map<std::string, Sexy::String> aIds;
    Sexy::String aLower;
    StringToLower(&aLower, theName != nullptr ? theName : "");
    std::string aKey = aLower.c_str() != nullptr ? aLower.c_str() : "";
    std::map<std::string, Sexy::String>::iterator aFound = aIds.find(aKey);
    if (aFound != aIds.end())
        return aFound->second.c_str();
    Sexy::String& aStored = aIds[aKey];
    aStored = aLower;
    return aStored.c_str();
}

void InitReanimTrackIds()
{
    static bool aReady = false;
    if (aReady)
        return;
    aReady = true;
    ReanimTrackId__ground = ReanimatorTrackNameToId("_ground");
    ReanimTrackId_anim_walk = ReanimatorTrackNameToId("anim_walk");
    ReanimTrackId_anim_walk2 = ReanimatorTrackNameToId("anim_walk2");
    ReanimTrackId_anim_idle = ReanimatorTrackNameToId("anim_idle");
    ReanimTrackId_anim_idle2 = ReanimatorTrackNameToId("anim_idle2");
    ReanimTrackId_anim_sleep = ReanimatorTrackNameToId("anim_sleep");
    ReanimTrackId_anim_stem = ReanimatorTrackNameToId("anim_stem");
    ReanimTrackId_anim_head1 = ReanimatorTrackNameToId("anim_head1");
    ReanimTrackId_anim_death = ReanimatorTrackNameToId("anim_death");
    ReanimTrackId_anim_death2 = ReanimatorTrackNameToId("anim_death2");
    ReanimTrackId_anim_superlongdeath = ReanimatorTrackNameToId("anim_superlongdeath");
    ReanimTrackId_anim_walnutdeath = ReanimatorTrackNameToId("anim_walnutdeath");
    ReanimTrackId_anim_waterdeath = ReanimatorTrackNameToId("anim_waterdeath");
    ReanimTrackId_anim_swim = ReanimatorTrackNameToId("anim_swim");
    ReanimTrackId_boss_jaw = ReanimatorTrackNameToId("Boss_jaw");
    ReanimTrackId_fullscreen = ReanimatorTrackNameToId("fullscreen");
    ReanimTrackId_zombie_catapult_pole = ReanimatorTrackNameToId("Zombie_catapult_pole");
    ReanimTrackId_zombie_mustache = ReanimatorTrackNameToId("Zombie_mustache");
}

namespace {
struct ReanimTrackIdStartup
{
    ReanimTrackIdStartup() { InitReanimTrackIds(); }
};
ReanimTrackIdStartup gReanimTrackIdStartup;
}

int ReanimatorEnsureDefinitionLoaded(ReanimationType theType, bool theRequireCompiled)
{
    ReanimatorDefinition* aDefinition = &gReanimatorDefArray[(int)theType];
    if (aDefinition->mTracks != nullptr)
        return 1;
    if (theRequireCompiled)
    {
        if (gSexyAppBase->mLoadingFailed || (gAppCloseRequest != nullptr && gAppCloseRequest()))
            return 0;
    }
    else if (gGetCurrentLevelName != nullptr)
    {
        gGetCurrentLevelName();
    }

    ReanimationParamRecord* aParam = &gReanimationParamArray[(int)theType];
    Sexy::PerfTimer aTimer;
    aTimer.Start();
    TodHesitationBracket aBracket("Load Reanim '%s'", aParam->mFileName);
    int aLoaded = ReanimationLoadDefinition(Sexy::String(aParam->mFileName), aDefinition);
    if (aLoaded == 0)
    {
        aDefinition->mTracks = nullptr;
        aDefinition->mTrackCount = 0;
        char aMessage[1024];
        snprintf(aMessage, sizeof(aMessage), "Failed to load reanim '%s'", aParam->mFileName);
        TodErrorMessageBox(aMessage, "Error");
        return 0;
    }
    for (int aTrack = 0; aTrack < aDefinition->mTrackCount; ++aTrack)
    {
        ReanimatorTrack& aRow = aDefinition->mTracks[aTrack];
        aRow.mTrackId = ReanimatorTrackNameToId(aRow.mName);
        aRow.mIsAttacher = SDL_strncasecmp(aRow.mName, "attacher__", 10) == 0;
    }
    if (aTimer.GetDuration() > 100.0 && gGetCurrentLevelName != nullptr)
        gGetCurrentLevelName();
    return aLoaded;
}

void ReanimatorLoadDefinitions(ReanimationParamRecord* theParams, int theCount)
{
    TodHesitationBracket aBracket("ReanimatorLoadDefinitions");
    gReanimationParamArraySize = theCount;
    gReanimationParamArray = theParams;
    gReanimatorDefCount = theCount;
    gReanimatorDefArray = static_cast<ReanimatorDefinition*>(
        operator new[](sizeof(ReanimatorDefinition) * theCount));
    for (int aIndex = 0; aIndex < theCount; ++aIndex)
        ConstructRaw<ReanimatorDefinition>(&gReanimatorDefArray[aIndex]);
    // DefinitionIsCompiled is constantly true in this build, so a compiled
    // table is left for EnsureDefinitionLoaded to fill on first use.
    (void)theParams;
}

void TrailFreeDefinitions()
{
    for (int index = 0; index < gTrailDefCount; ++index)
        DefinitionFreeMap(&gTrailDefMap, &gTrailDefArray[index]);

    // TrailDefinition has a destructor, so the allocation carries the compiler's
    // array cookie. delete[] runs that empty destructor and releases the block.
    delete[] gTrailDefArray;
    gTrailDefArray = nullptr;
    gTrailDefCount = 0;
    gTrailParamArray = nullptr;
    gTrailParamArraySize = 0;
}
